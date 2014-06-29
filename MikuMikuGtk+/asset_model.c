// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <zlib.h>
#include <assimp/postprocess.h>
#include "asset_model.h"
#include "application.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

static void** AssetModelGetBones(ASSET_MODEL* model, int* num_bones)
{
	void **bones = (void**)MEM_ALLOC_FUNC(sizeof(void*)*model->bones->num_data);
	int i;

	*num_bones = (int)model->bones->num_data;
	for(i=0; i<*num_bones; i++)
	{
		bones[i] = model->bones->buffer[i];
	}
	return bones;
}

static void** AssetModelGetLabels(ASSET_MODEL* model, int* num_labels)
{
	void **labels = (void**)MEM_ALLOC_FUNC(sizeof(void*)*model->labels->num_data);
	ASSET_MODEL_LABEL *asset_labels = (ASSET_MODEL_LABEL*)model->labels->buffer;
	int i;

	*num_labels = (int)model->labels->num_data;
	for(i=0; i<*num_labels; i++)
	{
		labels[i] = (void*)&asset_labels[i];
	}
	return labels;
}

static void AssetModelSetVisible(ASSET_MODEL* model, int visible)
{
	model->visible = visible;
}

static int AssetModelGetVisible(ASSET_MODEL* model)
{
	return model->visible;
}

static void AssetModelGetWorldTranslation(ASSET_MODEL* model, float* translation)
{
	COPY_VECTOR3(translation, model->position);
}

static void AssetModelGetWorldRotation(ASSET_MODEL* model, float* rotation)
{
	COPY_VECTOR4(rotation, model->rotation);
}

static void AssetModelSetWorldPosition(ASSET_MODEL* model, float* position)
{
	COPY_VECTOR3(model->position, position);
}

static void AssetModelSetWorldOrientation(ASSET_MODEL* model, float* rotation)
{
	COPY_VECTOR4(model->rotation, rotation);
}

void InitializeAssetModel(
	ASSET_MODEL* model,
	SCENE* scene,
	void* application_context
)
{
	ASSET_MODEL_LABEL *label;

	(void)memset(model, 0, sizeof(*model));

	model->bones = PointerArrayNew(ASSET_MODEL_BUFFER_SIZE);
	model->labels = StructArrayNew(sizeof(ASSET_MODEL_LABEL), ASSET_MODEL_BUFFER_SIZE);
	model->morphs = StructArrayNew(sizeof(ASSET_OPACITY_MORPH), ASSET_MODEL_BUFFER_SIZE);
	model->name2bone = ght_create(ASSET_MODEL_BUFFER_SIZE);
	ght_set_hash(model->name2bone, (ght_fn_hash_t)GetStringHash);
	model->name2morph = ght_create(ASSET_MODEL_BUFFER_SIZE);
	ght_set_hash(model->name2morph, (ght_fn_hash_t)GetStringHash);
	model->interface_data.opacity = 1;
	model->interface_data.scale_factor = 10;
	model->interface_data.scene = scene;
	model->interface_data.get_bones =
		(void** (*)(void*, int*))AssetModelGetBones;
	model->interface_data.get_labels =
		(void** (*)(void*, int*))AssetModelGetLabels;
	model->interface_data.set_visible =
		(void (*)(void*, int))AssetModelSetVisible;
	model->rotation[3] = 1;
	model->application = (APPLICATION*)application_context;

	InitializeAssetRootBone(&model->root_bone, model, application_context);
	PointerArrayAppend(model->bones, (void*)&model->root_bone);
	InitializeAssetScaleBone(&model->scale_bone, model, application_context);
	PointerArrayAppend(model->bones, (void*)&model->scale_bone);
	label = (ASSET_MODEL_LABEL*)StructArrayReserve(model->labels);
	InitializeAssetModelLabel(label, model, model->bones);
	InitializeAssetOpacityMorph((ASSET_OPACITY_MORPH*)StructArrayReserve(model->morphs),
		model, application_context);
	model->materials = StructArrayNew(sizeof(ASSET_MATERIAL), DEFAULT_BUFFER_SIZE);
	model->vertices = StructArrayNew(sizeof(ASSET_VERTEX), DEFAULT_BUFFER_SIZE);

	model->interface_data.get_morphs =
		(void** (*)(void*, int*))DummyFuncNoReturn2;
	model->interface_data.get_world_position =
		(void (*)(void*, float*))AssetModelGetWorldTranslation;
	model->interface_data.get_world_translation =
		(void (*)(void*, float*))AssetModelGetWorldTranslation;
	model->interface_data.get_world_orientation =
		(void (*)(void*, float*))AssetModelGetWorldRotation;
	model->interface_data.set_world_position =
		(void (*)(void*, float*))AssetModelSetWorldPosition;
	model->interface_data.set_world_orientation =
		(void (*)(void*, float*))AssetModelSetWorldOrientation;
	model->interface_data.perform_update =
		(void (*)(void*, int))DummyFuncNoReturn2;
	model->interface_data.reset_motion_state =
		(void (*)(void*, void*))DummyFuncNoReturn2;
	model->interface_data.is_visible =
		(int (*)(void*))AssetModelGetVisible;
}

void AssetModelSetMaterialsRecurse(
	ASSET_MODEL* model,
	const struct aiScene* scene,
	const struct aiNode* node
)
{
	ASSET_MATERIAL *m;
	const unsigned int num_meshes = node->mNumMeshes;
	unsigned int i, j;
	for(i=0; i<num_meshes; i++)
	{
		const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		struct aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
		const struct aiFace *faces = mesh->mFaces;
		const unsigned int num_faces = mesh->mNumFaces;
		unsigned int num_indices = 0;
		for(j=0; j<num_faces; j++)
		{
			num_indices += faces[j].mNumIndices;
		}
		m = (ASSET_MATERIAL*)StructArrayReserve(model->materials);
		InitializeAssetMaterial(m, model, material,
			(int)num_indices, (int)i, model->application);
	}
}

void AssetModelSetVerticesRecurse(
	ASSET_MODEL* model,
	const struct aiScene* scene,
	const struct aiNode* node
)
{
	ASSET_VERTEX *v;
	float vertex[3];
	float normal[3];
	float tex_coord[3];
	const unsigned int num_meshes = node->mNumMeshes;
	unsigned int i, j;
	for(i=0; i<num_meshes; i++)
	{
		const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		const struct aiVector3D *mesh_vertices = mesh->mVertices;
		const struct aiVector3D *mesh_normals = (mesh->mNormals != NULL && mesh->mNumVertices > 0) ?
			mesh->mNormals : NULL;
		const struct aiVector3D *mesh_texture_coords = (aiMeshHasTextureCoords(mesh, 0) != FALSE)
			? mesh->mTextureCoords[0] : NULL;
		const unsigned int num_vertices = mesh->mNumVertices;
		for(j=0; j<num_vertices; j++)
		{
			vertex[0] = mesh_vertices[j].x;
			vertex[1] = mesh_vertices[j].y;
			vertex[2] = mesh_vertices[j].z;
			if(mesh_normals != NULL)
			{
				normal[0] = mesh_vertices[j].x;
				normal[1] = mesh_vertices[j].y;
				normal[2] = mesh_vertices[j].z;
			}
			else
			{
				normal[0] = normal[1] = normal[2] = 0;
			}
			if(mesh_texture_coords != NULL)
			{
				tex_coord[0] = mesh_texture_coords[j].x;
				tex_coord[1] = mesh_texture_coords[j].y;
				tex_coord[2] = mesh_texture_coords[j].z;
			}
			else
			{
				tex_coord[0] = tex_coord[1] = tex_coord[2] = 0;
			}
			v = (ASSET_VERTEX*)StructArrayReserve(model->vertices);
			InitializeAssetVertex(v, model, vertex, normal, tex_coord, (int)j);
		}
	}
}

int LoadAssetModel(
	ASSET_MODEL* model,
	uint8* data,
	size_t data_size,
	const char* file_name,
	const char* file_type,
	const char* model_path
)
{
	BONE_INTERFACE **bones = (BONE_INTERFACE**)model->bones->buffer;
	BONE_INTERFACE *bone;
	ASSET_OPACITY_MORPH *morphs = (ASSET_OPACITY_MORPH*)model->morphs->buffer;
	ASSET_OPACITY_MORPH *morph;
	const int num_bones = (int)model->bones->num_data;
	const int num_morphs = (int)model->morphs->num_data;
	unsigned int flags = aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs
		| aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType;
	char *str;
	char *last_period = NULL;
	int i;

	model->scene = (struct aiScene*)aiImportFileFromMemory((const char*)data, (unsigned int)data_size,
		flags, file_type);
	model->file_type = MEM_STRDUP_FUNC(file_type);
	model->model_data = data;
	model->model_data_size = data_size;
	model->model_path = MEM_STRDUP_FUNC(model_path);
	model->interface_data.name = str = MEM_STRDUP_FUNC(file_name);
	while(*str != '\0')
	{
		if(*str == '.')
		{
			last_period = str;
		}
		str = NextCharUTF8(str);
	}
	if(last_period != NULL)
	{
		*last_period = '\0';
	}
	model->interface_data.english_name = MEM_STRDUP_FUNC(model->interface_data.name);

	for(i=0; i<num_bones; i++)
	{
		bone = bones[i];
		(void)ght_insert(model->name2bone, bone, (unsigned int)strlen(bone->name), bone->name);
	}
	for(i=0; i<num_morphs; i++)
	{
		morph = &morphs[i];
		(void)ght_insert(model->name2morph, morph,
			(unsigned int)strlen(morph->interface_data.name), morph->interface_data.name);
	}
	if(model->scene != NULL)
	{
		AssetModelSetMaterialsRecurse(model, model->scene, model->scene->mRootNode);
		AssetModelSetVerticesRecurse(model, model->scene, model->scene->mRootNode);

		return TRUE;
	}

	return FALSE;
}

typedef struct _MATERIAL_DATA
{
	char *name;
	size_t data_start;
	size_t data_size;
} MATERIAL_DATA;

static void ReleaseMaterialData(MATERIAL_DATA* data)
{
	MEM_FREE_FUNC(data->name);
}

uint8* WriteAssetModelMaterials(
	ASSET_MODEL* model,
	size_t* out_data_size
)
{
	MEMORY_STREAM *image_stream = CreateMemoryStream(1024 * 1024);
	STRUCT_ARRAY *materials_data;
	MATERIAL_DATA material_data;
	struct aiScene *scene;
	struct aiString texture_path;
	char full_path[4096];
	char *path;
	char *main_texture;
	char *sub_texture;
	char *system_path;
	FILE *fp;
	uint8 *ret;
	size_t image_buffer_size = 0;
	uint8 *image_data = NULL;
	ght_hash_table_t *name_table;
	size_t total_image_size = 0;
	unsigned int num_items;
	unsigned int name_length;
	uint32 data32;
	unsigned int i;

	materials_data = StructArrayNew(sizeof(material_data), model->materials->num_data+1);
	name_table = ght_create((unsigned int)model->materials->num_data+1);
	ght_set_hash(name_table, (ght_fn_hash_t)GetStringHash);

	scene = model->scene;
	num_items = scene->mNumMaterials;
	for(i=0; i<num_items; i++)
	{
		struct aiMaterial *material = scene->mMaterials[i];
		enum aiReturn found = AI_SUCCESS;
		int texture_index = 0;
		do
		{
			if((found = aiGetMaterialTexture(material, aiTextureType_DIFFUSE, texture_index, &texture_path,
				NULL, NULL, NULL, NULL, NULL, NULL)) != aiReturn_SUCCESS)
			{
				break;
			}
			path = texture_path.data;
			if(AssetModelSplitTexturePath(path, &main_texture, &sub_texture) != FALSE)
			{
				name_length = (unsigned int)strlen(main_texture);
				if(ght_get(name_table, name_length, main_texture) == NULL)
				{
					(void)sprintf(full_path, "%s/%s", model->model_path, main_texture);
					system_path = LocaleFromUTF8(full_path);
					if((fp = fopen(system_path, "rb")) != NULL)
					{
						(void)ght_insert(name_table, (void*)1, name_length, main_texture);
						material_data.name = MEM_STRDUP_FUNC(main_texture);
						material_data.data_start = total_image_size;
						(void)fseek(fp, 0, SEEK_END);
						material_data.data_size = (size_t)ftell(fp);
						total_image_size += material_data.data_size;
						rewind(fp);

						if(material_data.data_size > image_buffer_size)
						{
							image_data = (uint8*)MEM_REALLOC_FUNC(image_data, material_data.data_size);
							image_buffer_size = material_data.data_size;
						}
						(void)fread(image_data, 1, material_data.data_size, fp);
						(void)MemWrite(image_data, 1, material_data.data_size, image_stream);
						StructArrayAppend(materials_data, &material_data);

						(void)fclose(fp);
					}
					MEM_FREE_FUNC(system_path);
				}

				name_length = (unsigned int)strlen(sub_texture);
				if(ght_get(name_table, name_length, sub_texture) == NULL)
				{
					(void)sprintf(full_path, "%s/%s", model->model_path, sub_texture);
					system_path = LocaleFromUTF8(full_path);
					if((fp = fopen(system_path, "rb")) != NULL)
					{
						(void)ght_insert(name_table, (void*)1, name_length, sub_texture);
						material_data.name = MEM_STRDUP_FUNC(main_texture);
						material_data.data_start = total_image_size;
						(void)fseek(fp, 0, SEEK_END);
						material_data.data_size = (size_t)ftell(fp);
						total_image_size += material_data.data_size;
						rewind(fp);

						if(material_data.data_size > image_buffer_size)
						{
							image_data = (uint8*)MEM_REALLOC_FUNC(image_data, material_data.data_size);
							image_buffer_size = material_data.data_size;
						}
						(void)fread(image_data, 1, material_data.data_size, fp);
						(void)MemWrite(image_data, 1, material_data.data_size, image_stream);
						StructArrayAppend(materials_data, &material_data);

						(void)fclose(fp);
					}
					MEM_FREE_FUNC(system_path);
				}
			}
			else
			{
				name_length = (unsigned int)strlen(main_texture);
				if(ght_get(name_table, name_length, main_texture) == NULL)
				{
					(void)sprintf(full_path, "%s/%s", model->model_path, main_texture);
					system_path = LocaleFromUTF8(full_path);
					if((fp = fopen(system_path, "rb")) != NULL)
					{
						(void)ght_insert(name_table, (void*)1, name_length, main_texture);
						material_data.name = MEM_STRDUP_FUNC(main_texture);
						material_data.data_start = total_image_size;
						(void)fseek(fp, 0, SEEK_END);
						material_data.data_size = (size_t)ftell(fp);
						total_image_size += material_data.data_size;
						rewind(fp);

						if(material_data.data_size > image_buffer_size)
						{
							image_data = (uint8*)MEM_REALLOC_FUNC(image_data, material_data.data_size);
							image_buffer_size = material_data.data_size;
						}
						(void)fread(image_data, 1, material_data.data_size, fp);
						(void)MemWrite(image_data, 1, material_data.data_size, image_stream);
						StructArrayAppend(materials_data, &material_data);

						(void)fclose(fp);
					}
					MEM_FREE_FUNC(system_path);
				}
			}
			texture_index++;
		} while(found == AI_SUCCESS);
	}

	{
		MEMORY_STREAM result_stream = {0};
		MATERIAL_DATA *materials = (MATERIAL_DATA*)materials_data->buffer;
		size_t image_start = sizeof(data32);
		num_items = (unsigned int)materials_data->num_data;
		for(i=0; i<num_items; i++)
		{
			MATERIAL_DATA *material = &materials[i];
			image_start += strlen(material->name) + sizeof(uint32) * 3 + 1;
		}

		ret = (uint8*)MEM_ALLOC_FUNC(image_start + image_stream->data_point);
		result_stream.buff_ptr = ret;
		result_stream.data_size = image_start + image_stream->data_point;
		data32 = num_items;
		(void)MemWrite(&data32, sizeof(data32), 1, &result_stream);
		for(i=0; i<num_items; i++)
		{
			MATERIAL_DATA *material = &materials[i];
			material->data_start += image_start;
			data32 = (uint32)strlen(material->name) + 1;
			(void)MemWrite(&data32, sizeof(data32), 1, &result_stream);
			(void)MemWrite(material->name, 1, data32, &result_stream);
			data32 = (uint32)material->data_start;
			(void)MemWrite(&data32, sizeof(data32), 1, &result_stream);
			data32 = (uint32)material->data_size;
			(void)MemWrite(&data32, sizeof(data32), 1, &result_stream);
		}
		(void)MemWrite(image_stream->buff_ptr, 1, image_stream->data_point, &result_stream);

		if(out_data_size != NULL)
		{
			*out_data_size = result_stream.data_point;
		}
	}

	StructArrayDestroy(&materials_data, (void (*)(void*))ReleaseMaterialData);
	(void)DeleteMemoryStream(image_stream);

	return ret;
}

void ReadAssetModelDataAndState(
	void *scene,
	ASSET_MODEL* model,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int)
)
{
	SCENE *scene_ptr = (SCENE*)scene;
	char *file_name;
	float float_values[4];
	uint32 data32;
	uint32 data_size;
	uint32 section_size;

	(void)read_func(&data_size, sizeof(data_size), 1, src);

	// ファイル名を読み込む
	(void)read_func(&data32, sizeof(data32), 1, src);
	file_name = (char*)MEM_ALLOC_FUNC(data32);
	(void)read_func(file_name, 1, data32, src);

	// モデルのデータを読み込む
	{
		uint32 decode_size;
		uint8 *section_data;
		uint8 *decode_data;
		(void)read_func(&section_size, sizeof(section_size), 1, src);
		(void)read_func(&decode_size, sizeof(decode_size), 1, src);
		section_data = (uint8*)MEM_ALLOC_FUNC(section_size);
		decode_data = (uint8*)MEM_ALLOC_FUNC(decode_size);
		(void)read_func(section_data, 1, section_size, src);
		if(InflateData(section_data, decode_data, section_size, decode_size, NULL) != 0)
		{
			return;
		}
		LoadAssetModel(model, decode_data, decode_size, file_name,
			GetFileExtention(file_name), "./");
		MEM_FREE_FUNC(section_data);
	}

	// モデルの位置・向きを読み込む
	(void)read_func(model->position, sizeof(*model->position), 3, src);
	(void)read_func(model->rotation, sizeof(*model->rotation), 4, src);
	// モデルのサイズを読み込む
	(void)read_func(float_values, sizeof(*float_values), 1, src);
	model->interface_data.scale_factor = float_values[0];

	// テクスチャデータを読み込む
	{
		(void)read_func(&section_size, sizeof(section_size), 1, src);
		model->interface_data.texture_archive_size = section_size;
		model->interface_data.texture_archive = MEM_ALLOC_FUNC(section_size);
		(void)read_func(model->interface_data.texture_archive, 1, section_size, src);

		PointerArrayAppend(scene_ptr->engines, SceneCreateRenderEngine(
			scene_ptr, RENDER_ENGINE_ASSET, &model->interface_data, 0, scene_ptr->project));
	}

	// 親ボーンの読み込み
	(void)read_func(&data32, sizeof(data32), 1, src);
	if(data32 > 0)
	{
		model->interface_data.parent_model =
			(MODEL_INTERFACE*)MEM_ALLOC_FUNC(data32);
		(void)read_func(model->interface_data.parent_model, 1,
			data32, src);
		(void)read_func(&data32, sizeof(data32), 1, src);
		model->interface_data.parent_bone =
			(BONE_INTERFACE*)MEM_ALLOC_FUNC(data32);
		(void)read_func(model->interface_data.parent_bone, 1, data32, src);
	}

	MEM_FREE_FUNC(file_name);
}

size_t WriteAssetModelDataAndState(
	ASSET_MODEL* model,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
)
{
	// 最後にサイズを書き出すので位置を記憶
	long size_position = tell_func(dst);
	// ZIP圧縮用
	uint8 *compressed_data = (uint8*)MEM_ALLOC_FUNC(model->model_data_size*2);
	// 圧縮後のサイズ
	size_t compressed_size;
	// 浮動小数点数書き出し用
	float float_values[4];
	// 4バイト書き出し用
	uint32 data32;

	// データサイズ(4バイト)分をスキップ
	(void)seek_func(dst, sizeof(uint32), SEEK_CUR);

	// ファイル名を書き出す
	data32 = (uint32)strlen(model->interface_data.name) + 1;
	(void)write_func(&data32, sizeof(data32), 1, dst);
	(void)write_func(model->interface_data.name, 1, data32, dst);

	// モデルのデータを書き出す
		// ZIP圧縮して書き出し
	(void)DeflateData(model->model_data, compressed_data,
		model->model_data_size, model->model_data_size * 2, &compressed_size, Z_DEFAULT_COMPRESSION);
	data32 = (uint32)compressed_size;
	(void)write_func(&data32, sizeof(data32), 1, dst);
	data32 = (uint32)model->model_data_size;
	(void)write_func(&data32, sizeof(data32), 1, dst);
	(void)write_func(compressed_data, 1, compressed_size, dst);

	MEM_FREE_FUNC(compressed_data);

	// モデルの位置・向きを書き出す
	(void)write_func(model->position, sizeof(*model->position), 3, dst);
	(void)write_func(model->rotation, sizeof(*model->rotation), 4, dst);
	// モデルのサイズを書き出す
	float_values[0] = (float)model->interface_data.scale_factor;
	(void)write_func(float_values, sizeof(*float_values), 1, dst);

	// テクスチャデータの書き出し
	if(model->interface_data.texture_archive_size > 0)
	{
		data32 = (uint32)model->interface_data.texture_archive_size;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(model->interface_data.texture_archive, 1,
			model->interface_data.texture_archive_size, dst);
	}
	else
	{
		size_t write_data_size;
		uint8 *texture_data = WriteAssetModelMaterials(model, &write_data_size);
		data32 = (uint32)write_data_size;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(texture_data, 1, write_data_size, dst);
		MEM_FREE_FUNC(texture_data);
	}

	// 親モデル・ボーンの書き出し
	if(model->interface_data.parent_bone != NULL)
	{
		data32 = (uint32)strlen(model->interface_data.parent_model->name) + 1;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(model->interface_data.parent_model->name, 1, data32, dst);
		data32 = (uint32)strlen(model->interface_data.parent_bone->name) + 1;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(model->interface_data.parent_bone->name, 1, data32, dst);
	}
	else
	{
		data32 = 0;
		(void)write_func(&data32, sizeof(data32), 1, dst);
	}

	data32 = (uint32)tell_func(dst);
	(void)seek_func(dst, size_position, SEEK_SET);
	(void)write_func(&data32, sizeof(data32), 1, dst);
	(void)seek_func(dst, data32, SEEK_SET);

	return data32;
}

void AssetModelSetWorldPositionInternal(ASSET_MODEL* model, const float* translation)
{
	COPY_VECTOR3(model->position, translation);
}

void AssetModelSetWorldRotationInternal(ASSET_MODEL* model, const float* rotation)
{
	COPY_VECTOR4(model->rotation, rotation);
}

#ifdef __cplusplus
}
#endif
