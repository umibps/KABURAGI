// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include "pmx_model.h"
#include "vertex.h"
#include "model_helper.h"
#include "memory_stream.h"
#include "memory.h"
#include "scene.h"
#include "tbb.h"
#include "project.h"

#define PMX_HEADER_SIZE 8
#define PMX_FLAGS_SIZE 8

#define PMX_DEFAULT_BUFFER_SIZE 32

void* PmxModelFindBone(PMX_MODEL* model, const char* bone_name)
{
	if(bone_name != NULL)
	{
		return ght_get(model->name2bone, (unsigned int)strlen(bone_name), bone_name);
	}
	return NULL;
}

void* PmxModelFindMorph(PMX_MODEL* model, const char* morph_name)
{
	if(morph_name != NULL)
	{
		return ght_get(model->name2morph, (unsigned int)strlen(morph_name), morph_name);
	}
	return NULL;
}

void** PmxModelGetBones(PMX_MODEL* model, int* num_bones)
{
	PMX_BONE *bones = (PMX_BONE*)model->bones->buffer;
	void **ret;
	int i;

	*num_bones = (int)model->bones->num_data;
	ret = (void**)MEM_ALLOC_FUNC(sizeof(*ret)*(*num_bones));
	for(i=0; i<*num_bones; i++)
	{
		ret[i] = (void*)&bones[i];
	}

	return ret;
}

void** PmxModelGetMaterials(PMX_MODEL* model, int* num_materials)
{
	PMX_MATERIAL *materials = (PMX_MATERIAL*)model->materials->buffer;
	void **ret;
	int i;

	*num_materials = (int)model->materials->num_data;
	ret = (void**)MEM_ALLOC_FUNC(sizeof(*ret)* (*num_materials));
	for(i=0; i<*num_materials; i++)
	{
		ret[i] = (void*)&materials[i];
	}

	return ret;
}

void** PmxModelGetRigidBodies(PMX_MODEL* model, int* num_bodies)
{
	PMX_RIGID_BODY *bodies = (PMX_RIGID_BODY*)model->rigid_bodies->buffer;
	void **ret;
	int i;

	*num_bodies = (int)model->rigid_bodies->num_data;
	ret = (void**)MEM_ALLOC_FUNC(sizeof(*ret)*(*num_bodies));
	for(i=0; i<*num_bodies; i++)
	{
		ret[i] = (void*)&bodies[i];
	}

	return ret;
}

void** PmxModelGetLabels(PMX_MODEL* model, int* num_labels)
{
	PMX_MODEL_LABEL *labels = (PMX_MODEL_LABEL*)model->labels->buffer;
	void **ret;
	int i;

	*num_labels = (int)model->labels->num_data;
	ret = (void**)MEM_ALLOC_FUNC(sizeof(*ret)*(*num_labels));
	for(i=0; i<*num_labels; i++)
	{
		ret[i] = (void*)&labels[i];
	}

	return ret;
}

int PmxModelIsVisible(PMX_MODEL* model)
{
	return model->flags & PMX_FLAG_VISIBLE;
}

void PmxModelSetVisible(PMX_MODEL* model, int is_visible)
{
	if(is_visible == FALSE && (model->flags & PMX_FLAG_VISIBLE) != 0)
	{
		// ADD_EVENT_QUEUE
		model->flags &= ~(PMX_FLAG_VISIBLE);
	}
	else if(is_visible != FALSE && (model->flags & PMX_FLAG_VISIBLE) == 0)
	{
		// ADD_EVENT_QUEUE
		model->flags |= PMX_FLAG_VISIBLE;
	}
}

void PmxModelGetWorldTranslation(PMX_MODEL* model, float* translation)
{
	COPY_VECTOR3(translation, model->position);
}

void PmxModelGetWorldOrientation(PMX_MODEL* model, float* orientation)
{
	COPY_VECTOR4(orientation, model->rotation);
}

FLOAT_T PmxModelGetEdgeScaleFactor(PMX_MODEL* model, float* camera_position)
{
	FLOAT_T length = 0;
	if(model->bones->num_data > 1)
	{
		PMX_BONE *bones = (PMX_BONE*)model->bones->buffer;
		PMX_BONE *bone = &bones[1];
		float origin[3];

		BtTransformGetOrigin(bone->world_transform, origin);
		origin[0] = camera_position[0] - origin[0];
		origin[1] = camera_position[1] - origin[1];
		origin[2] = camera_position[2] - origin[2];
		length = Length3DVector(origin) * model->interface_data.edge_width;
	}
	return length / 1000.0;
}

void PmxModelSetWorldPosition(PMX_MODEL* model, float* position)
{
	COPY_VECTOR3(model->position, position);
}

void PmxModelSetWorldOrientation(PMX_MODEL* model, float* rotation)
{
	COPY_VECTOR4(model->rotation, rotation);
}

void PmxModelSetOpacity(PMX_MODEL* model, float opacity)
{
	if(model->interface_data.opacity != opacity)
	{
		// ADD_QUEUE_EVENT
		model->interface_data.opacity = opacity;
	}
}

void PmxModelSetAaBb(PMX_MODEL* model, float* minimum, float* maximum)
{
	if(CompareVector3(model->aa_bb_min, minimum) || CompareVector3(model->aa_bb_max, maximum))
	{
		// ADD_QUEUE_EVENT
		COPY_VECTOR3(model->aa_bb_min, minimum);
		COPY_VECTOR3(model->aa_bb_max, maximum);
	}
}

void PmxModelSetEnablePhysics(PMX_MODEL* model, int enable_physics)
{
	if(enable_physics != FALSE)
	{
		model->flags |= PMX_FLAG_ENABLE_PHYSICS;
	}
	else
	{
		model->flags &= ~(PMX_FLAG_ENABLE_PHYSICS);
	}
}

void InitializePmxModel(
	PMX_MODEL* model,
	SCENE* scene,
	TEXT_ENCODE* encode,
	const char* model_path
)
{
	(void)memset(model, 0, sizeof(*model));

	model->encoding = encode;
	model->parent_scene = scene;
	model->vertices = StructArrayNew(sizeof(PMX_VERTEX), PMX_DEFAULT_BUFFER_SIZE);
	model->indices = Uint32ArrayNew(PMX_DEFAULT_BUFFER_SIZE);
	model->textures = PointerArrayNew(PMX_DEFAULT_BUFFER_SIZE);
	model->name2texture = ght_create(PMX_DEFAULT_BUFFER_SIZE);
	ght_set_hash(model->name2texture, (ght_fn_hash_t)GetStringHash);
	model->texture_indices = ght_create(PMX_DEFAULT_BUFFER_SIZE);
	model->materials = StructArrayNew(sizeof(PMX_MATERIAL), PMX_DEFAULT_BUFFER_SIZE);
	model->bones = StructArrayNew(sizeof(PMX_BONE), PMX_DEFAULT_BUFFER_SIZE);
	model->bones_before_physics = PointerArrayNew(PMX_DEFAULT_BUFFER_SIZE);
	model->bones_after_physics = PointerArrayNew(PMX_DEFAULT_BUFFER_SIZE);
	model->morphs = StructArrayNew(sizeof(PMX_MORPH), PMX_DEFAULT_BUFFER_SIZE);
	model->labels = StructArrayNew(sizeof(PMX_MODEL_LABEL), PMX_DEFAULT_BUFFER_SIZE);
	model->rigid_bodies = StructArrayNew(sizeof(PMX_RIGID_BODY), PMX_DEFAULT_BUFFER_SIZE);
	model->joints = StructArrayNew(sizeof(PMX_JOINT), PMX_DEFAULT_BUFFER_SIZE);
	model->soft_bodies = StructArrayNew(sizeof(PMX_SOFT_BODY), PMX_DEFAULT_BUFFER_SIZE);
	model->name2bone = ght_create(PMX_DEFAULT_BUFFER_SIZE);
	ght_set_hash(model->name2bone, (ght_fn_hash_t)GetStringHash);
	model->name2morph = ght_create(PMX_DEFAULT_BUFFER_SIZE);
	ght_set_hash(model->name2morph, (ght_fn_hash_t)GetStringHash);
	model->rotation[3] = 1;
	model->interface_data.opacity = 1;
	model->interface_data.scale_factor = 1;

	model->interface_data.scene = scene;

	model->interface_data.type = MODEL_TYPE_PMX_MODEL;
	model->interface_data.model_path = MEM_STRDUP_FUNC(model_path);
	model->interface_data.find_bone =
		(void* (*)(void*, const char*))PmxModelFindBone;
	model->interface_data.find_morph =
		(void* (*)(void*, const char*))PmxModelFindMorph;
	model->interface_data.get_index_buffer =
		(void (*)(void*, void**))PmxModelGetIndexBuffer;
	model->interface_data.get_static_vertex_buffer =
		(void (*)(void*, void**))PmxModelGetStaticVertexBuffer;
	model->interface_data.get_dynanic_vertex_buffer =
		(void (*)(void*, void**, void*))PmxModelGetDynamicVertexBuffer;
	model->interface_data.get_matrix_buffer =
		(void (*)(void*, void**, void*, void*))PmxModelGetMatrixBuffer;
	model->interface_data.get_bones =
		(void** (*)(void*, int*))PmxModelGetBones;
	model->interface_data.get_materials =
		(void** (*)(void*, int*))PmxModelGetMaterials;
	model->interface_data.get_rigid_bodies =
		(void** (*)(void*, int*))PmxModelGetRigidBodies;
	model->interface_data.get_labels =
		(void** (*)(void*, int*))PmxModelGetLabels;
	model->interface_data.is_visible =
		(int (*)(void*))PmxModelIsVisible;
	model->interface_data.set_world_position =
		(void (*)(void*, float*))PmxModelSetWorldPosition;
	model->interface_data.set_world_orientation =
		(void (*)(void*, float*))PmxModelSetWorldOrientation;
	model->interface_data.set_visible =
		(void (*)(void*, int))PmxModelSetVisible;
	model->interface_data.get_world_position =
		(void (*)(void*, float*))PmxModelGetWorldTranslation;
	model->interface_data.get_world_translation =
		(void (*)(void*, float*))PmxModelGetWorldTranslation;
	model->interface_data.get_world_orientation =
		(void (*)(void*, float*))PmxModelGetWorldOrientation;
	model->interface_data.get_edge_scale_factor =
		(FLOAT_T (*)(void*, float*))PmxModelGetEdgeScaleFactor;
	model->interface_data.set_opacity =
		(void (*)(void*, float))PmxModelSetOpacity;
	model->interface_data.set_aa_bb =
		(void (*)(void*, float*, float*))PmxModelSetAaBb;
	model->interface_data.perform_update =
		(void (*)(void*, int))PmxModelPerformUpdate;
	model->interface_data.reset_motion_state =
		(void (*)(void*, void*))PmxModelResetMotionState;
	model->interface_data.set_enable_physics =
		(void (*)(void*, int))PmxModelSetEnablePhysics;
}

void ReleasePmxModel(PMX_MODEL* model)
{
	StructArrayDestroy(&model->vertices, NULL);
	Uint32ArrayDestroy(&model->indices);
	PointerArrayDestroy(&model->textures, (void (*)(void*))MEM_FREE_FUNC);
	// TODO : ハッシュテーブルのテクスチャを他に同じパスのモデルが無い場合に削除する
	ReleaseModelInterface(&model->interface_data);
	ght_finalize(model->name2texture);
	ght_finalize(model->texture_indices);
	StructArrayDestroy(&model->materials, (void (*)(void*))ReleasePmxMaterial);
	StructArrayDestroy(&model->bones, (void (*)(void*))ReleasePmxBone);
	PointerArrayDestroy(&model->bones_before_physics, NULL);
	PointerArrayDestroy(&model->bones_after_physics, NULL);
	StructArrayDestroy(&model->morphs, (void (*)(void*))ReleasePmxMorph);
	StructArrayDestroy(&model->labels, (void (*)(void*))ReleasePmxModelLabel);
	StructArrayDestroy(&model->joints, (void (*)(void*))ReleaseBaseJoint);
	StructArrayDestroy(&model->rigid_bodies, (void (*)(void*))ReleaseBaseRigidBody);
	StructArrayDestroy(&model->soft_bodies, (void (*)(void*))ReleasePmxSoftBody);
	ght_finalize(model->name2bone);
	ght_finalize(model->name2morph);
	MEM_FREE_FUNC(model->data_info.base);
}

typedef struct _PMX_HEADER
{
	uint8 signature[4];
	float version;
} PMX_HEADER;

typedef struct _PMX_FLAGS
{
	uint8 codec;
	uint8 additional_uv_size;
	uint8 vertex_index_size;
	uint8 texture_index_size;
	uint8 material_index_size;
	uint8 bone_index_size;
	uint8 morph_index_size;
	uint8 rigid_body_index_size;
} PMX_FLAGS;

static void ClampPmxFlags(PMX_FLAGS* flags)
{
	ClampByteData(&flags->additional_uv_size, 0, 4);
	ClampByteData(&flags->vertex_index_size, 1, 4);
	ClampByteData(&flags->texture_index_size, 1, 4);
	ClampByteData(&flags->material_index_size, 1, 4);
	ClampByteData(&flags->bone_index_size, 1, 4);
	ClampByteData(&flags->morph_index_size, 1, 4);
	ClampByteData(&flags->rigid_body_index_size, 1, 4);
}

static void CopyPmaxFlags2DataInfo(PMX_FLAGS* flags, PMX_DATA_INFO* info)
{
	info->codec = (flags->codec == 1) ? TEXT_TYPE_UTF8 : TEXT_TYPE_UTF16;
	info->additional_uv_size = flags->additional_uv_size;
	info->vertex_index_size = flags->vertex_index_size;
	info->texture_index_size = flags->texture_index_size;
	info->material_index_size = flags->material_index_size;
	info->bone_index_size = flags->bone_index_size;
	info->morph_index_size = flags->morph_index_size;
	info->rigid_body_index_size = flags->rigid_body_index_size;
}

int PmxModelPreparse(
	PMX_MODEL* model,
	uint8* data,
	size_t data_size,
	PMX_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	PMX_HEADER header;
	PMX_FLAGS flags;
	size_t section_data_size;
	int32 num_indices;
	int32 num_textures;
	uint8 flag_size;
	int i;

	if(data == NULL || PMX_HEADER_SIZE > data_size)
	{
		return FALSE;
	}

	(void)MemRead(header.signature, 1, sizeof(header.signature), &stream);
	(void)MemRead(&header.version, sizeof(header.version), 1, &stream);

	info->base = data;

	// 「PMX 」がきちんと入っているか
	if(memcmp(header.signature, "PMX ", 4) != 0)
	{
		return FALSE;
	}

	// バージョンチェック
	if(header.version < 2.0f)
	{
		return FALSE;
	}
	info->version = header.version;

	// フラグデータ
	(void)MemRead(&flag_size, sizeof(flag_size), 1, &stream);
	if(stream.data_point > data_size || flag_size != PMX_FLAGS_SIZE)
	{
		return FALSE;
	}
	flags.codec = data[stream.data_point];
	stream.data_point++;
	flags.additional_uv_size = data[stream.data_point];
	stream.data_point++;
	flags.vertex_index_size = data[stream.data_point];
	stream.data_point++;
	flags.texture_index_size = data[stream.data_point];
	stream.data_point++;
	flags.material_index_size = data[stream.data_point];
	stream.data_point++;
	flags.bone_index_size = data[stream.data_point];
	stream.data_point++;
	flags.morph_index_size = data[stream.data_point];
	stream.data_point++;
	flags.rigid_body_index_size = data[stream.data_point];
	stream.data_point++;
	ClampPmxFlags(&flags);
	CopyPmaxFlags2DataInfo(&flags, info);

	TextEncodeSetSource(model->encoding, GetTextTypeString(flags.codec));

	// 日本語名
	if((info->name_size = GetTextFromStream(
		(char*)&data[stream.data_point], &info->name)) == 0)
	{
		return FALSE;
	}
	stream.data_point += sizeof(int32) + info->name_size;

	// 英語名
	if((info->english_name_size = GetTextFromStream(
		(char*)&data[stream.data_point], &info->english_name)) == 0)
	{
		return FALSE;
	}
	stream.data_point += sizeof(int32) + info->english_name_size;

	// 日本語のコメント
	if((info->comment_size = GetTextFromStream(
		(char*)&data[stream.data_point], &info->comment)) == 0)
	{
		return FALSE;
	}
	stream.data_point += sizeof(int32) + info->comment_size;

	// 英語のコメント
	if((info->english_comment_size = GetTextFromStream(
		(char*)&data[stream.data_point], &info->english_comment)) == 0)
	{
		return FALSE;
	}
	stream.data_point += sizeof(int32) + info->english_comment_size;

	// 頂点データ
	if(PmxVertexPreparse(&data[stream.data_point], &section_data_size,
		data_size - stream.data_point, info) == FALSE)
	{
		return FALSE;
	}
	stream.data_point += section_data_size;

	// インデックスデータ
	if(MemRead(&num_indices, sizeof(num_indices), 1, &stream) == 0)
	{
		return FALSE;
	}
	if(num_indices * info->vertex_index_size + stream.data_point > data_size)
	{
		return FALSE;
	}
	info->indices = &data[stream.data_point];
	info->indices_count = num_indices;
	stream.data_point += num_indices * info->vertex_index_size;

	// テクスチャのルックアップテーブル
	if(MemRead(&num_textures, sizeof(num_textures), 1, &stream) == 0)
	{
		return FALSE;
	}
	info->textures = &data[stream.data_point];
	{
		int name_size;
		char *name_ptr;
		for(i=0; i<num_textures; i++)
		{
			if((name_size = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
			{
				return FALSE;
			}
			stream.data_point += sizeof(int32) + name_size;
		}
	}
	info->textures_count = num_textures;

	// 材質
	if(PmxMaterialPreparse(&data[stream.data_point], &section_data_size,
		data_size - stream.data_point, info) == FALSE)
	{
		return FALSE;
	}
	stream.data_point += section_data_size;

	// ボーン
	if(PmxBonePreparse(&data[stream.data_point], &section_data_size,
		data_size - stream.data_point, info) == FALSE)
	{
		return FALSE;
	}
	stream.data_point += section_data_size;

	// モーフィング
	if(PmxMorphPreparse(&data[stream.data_point], &section_data_size,
		data_size - stream.data_point, info) == FALSE)
	{
		return FALSE;
	}
	stream.data_point += section_data_size;

	// 表示文字列
	if(PmxModelLabelPreparse(&data[stream.data_point], &section_data_size,
		data_size - stream.data_point, info) == FALSE)
	{
		return FALSE;
	}
	stream.data_point += section_data_size;

	// ボディー
	if(PmxRigidBodyPreparse(&data[stream.data_point], &section_data_size,
		data_size - stream.data_point, info) == FALSE)
	{
		return FALSE;
	}
	stream.data_point += section_data_size;

	// ジョイント
	if(PmxJointPreparse(&data[stream.data_point], &section_data_size,
		data_size - stream.data_point, info) == FALSE)
	{
		return FALSE;
	}
	stream.data_point += section_data_size;

	// ソフトボディー
	if(PmxSoftBodyPreparse(&data[stream.data_point], &section_data_size,
		data_size - stream.data_point, info) == FALSE)
	{
		return FALSE;
	}
	stream.data_point += section_data_size;

	info->encoding = model->encoding;
	model->data_info = *info;

	return TRUE;
}

static void PmxModelRelease(PMX_MODEL* model)
{
	model->textures->num_data = 0;
	model->vertices->num_data = 0;
	model->materials->num_data = 0;
	model->bones->num_data = 0;
	model->morphs->num_data = 0;
	model->labels->num_data = 0;
	model->rigid_bodies->num_data = 0;
	model->joints->num_data = 0;
	model->soft_bodies->num_data = 0;
	model->data_info.version = 2.0f;
	MEM_FREE_FUNC(model->interface_data.name);
	MEM_FREE_FUNC(model->interface_data.english_name);
	MEM_FREE_FUNC(model->interface_data.comment);
	MEM_FREE_FUNC(model->interface_data.english_comment);
	model->interface_data.parent_bone = NULL;
	model->edge_color[0] = model->edge_color[1] = model->edge_color[2] = 0;
	model->aa_bb_max[0] = model->aa_bb_max[1] = model->aa_bb_max[2] = 0;
	model->aa_bb_min[0] = model->aa_bb_min[1] = model->aa_bb_min[2] = 0;
	model->position[0] = model->position[1] = model->position[2] = 0;
	model->rotation[0] = model->rotation[1] = model->rotation[2] = 0;
	model->rotation[3] = 1;
	model->interface_data.opacity = 1;
	model->interface_data.scale_factor = 1;
}

static void PmxModelParseNamesAndComments(PMX_MODEL* model)
{
	char str[4096 * 4];

	(void)memcpy(str, model->data_info.name, model->data_info.name_size);
	str[model->data_info.name_size+1] = '\0';
	model->interface_data.name = EncodeText(model->encoding, str, model->data_info.name_size);

	(void)memcpy(str, model->data_info.english_name, model->data_info.english_name_size);
	str[model->data_info.english_name_size+1] = '\0';
	model->interface_data.english_name = EncodeText(model->encoding, str, model->data_info.english_name_size);

	(void)memcpy(str, model->data_info.comment, model->data_info.comment_size);
	str[model->data_info.comment_size+1] = '\0';
	model->interface_data.comment = EncodeText(model->encoding, str, model->data_info.comment_size);

	(void)memcpy(str, model->data_info.english_comment, model->data_info.english_comment_size);
	str[model->data_info.english_comment_size+1] = '\0';
	model->interface_data.english_comment = EncodeText(model->encoding, str, model->data_info.english_comment_size);
}

static void PmxModelParseVertices(PMX_MODEL* model)
{
	const int num_vertices = (int)model->data_info.vertices_count;
	uint8 *data_pointer = model->data_info.vertices;
	size_t data_size;
	int i;

	for(i=0; i<num_vertices; i++)
	{
		PMX_VERTEX *vertex = (PMX_VERTEX*)StructArrayReserve(model->vertices);
		InitializePmxVertex(vertex, model);
		PmxVertexRead(vertex, data_pointer, &model->data_info, &data_size);
		data_pointer += data_size;
	}
}

static void PmxModelParseIndices(PMX_MODEL* model)
{
	const int num_indices = (int)model->data_info.indices_count;
	const int num_vertices = (int)model->data_info.vertices_count;
	uint8 *data_pointer = model->data_info.indices;
	size_t data_size = model->data_info.vertex_index_size;
	int index;
	int i;

	for(i=0; i<num_indices; i++)
	{
		index = GetSignedValue(data_pointer, (int)data_size);

		if(!CHECK_BOUND(index, 0, num_vertices))
		{
			index = 0;
		}
		Uint32ArrayAppend(model->indices, (uint32)index);

		data_pointer += data_size;
	}
}

static void PmxModelParseTextures(PMX_MODEL* model)
{
	const int num_textures = (int)model->data_info.textures_count;
	size_t data_size;
	uint8 *data_pointer = model->data_info.textures;
	char *name_ptr;
	char *texture_name;
	int i;

	model->textures->buffer = (void**)MEM_REALLOC_FUNC(
		model->textures->buffer, sizeof(void*)*((num_textures/model->textures->block_size + 1)*model->textures->block_size));
	model->textures->buffer_size = ((num_textures/model->textures->block_size + 1)*model->textures->block_size);

	for(i=0; i<num_textures; i++)
	{
		data_size = GetTextFromStream((char*)data_pointer, &name_ptr);
		texture_name = EncodeText(model->data_info.encoding, name_ptr, data_size);
		(void)ght_insert(model->name2texture, texture_name, (unsigned int)strlen(texture_name), texture_name);
		PointerArrayAppend(model->textures, texture_name);
		data_pointer += sizeof(int32) + data_size;
	}
}

static void PmxModelParseMaterials(PMX_MODEL* model)
{
	PMX_MATERIAL *materials;
	PMX_VERTEX *v = (PMX_VERTEX*)model->vertices->buffer;
	const int num_materials = (int)model->data_info.materials_count;
	const int num_indices = (int)model->data_info.indices_count;
	uint8 *data_pointer = model->data_info.materials;
	size_t data_size;
	size_t offset = 0;
	int i, j;

	model->materials->buffer_size =
		(num_materials / model->materials->block_size + 1) * model->materials->block_size;
	model->materials->buffer = (uint8*)MEM_REALLOC_FUNC(
		model->materials->buffer, model->materials->buffer_size * model->materials->data_size);
	(void)memset(model->materials->buffer, 0, model->materials->buffer_size * model->materials->data_size);
	materials = (PMX_MATERIAL*)model->materials->buffer;

	for(i=0; i<num_materials; i++)
	{
		PMX_MATERIAL *material = &materials[i];
		MATERIAL_INDEX_RANGE range;
		int offset_to;

		InitializePmxMaterial(material);
		PmxMaterialRead(material, data_pointer, &model->data_info, &data_size);
		data_pointer += data_size;

		range = material->index_range;
		offset_to = (int)offset + range.count;
		range.start = num_indices;
		range.end = 0;
		for(j=(int)offset; j<offset_to; j++)
		{
			const int index = (int)model->indices->buffer[j];
			PMX_VERTEX *vertex = &v[index];
			vertex->interface_data.set_material(vertex, material);
			if(range.start > index)
			{
				range.start = index;
			}
			if(range.end < index)
			{
				range.end = index;
			}
		}
		material->interface_data.set_index_range(material, &range);
		offset = offset_to;

		model->materials->num_data++;
	}
}

static void PmxModelParseBones(PMX_MODEL* model)
{
	const int num_bones = (int)model->data_info.bones_count;
	uint8 *data_pointer = model->data_info.bones;
	size_t data_size;
	int i;

	model->bones->buffer_size = num_bones + 1;
	model->bones->buffer = (uint8*)MEM_REALLOC_FUNC(
		model->bones->buffer, sizeof(PMX_BONE) * model->bones->buffer_size);
	for(i=0; i<num_bones; i++)
	{
		PMX_BONE *bone = (PMX_BONE*)StructArrayReserve(model->bones);
		InitializePmxBone(bone, model);
		ReadPmxBone(bone, data_pointer, &model->data_info, &data_size);
		(void)ght_insert(model->name2bone, bone, (unsigned int)strlen(bone->interface_data.name), bone->interface_data.name);
		(void)ght_insert(model->name2bone, bone, (unsigned int)strlen(bone->interface_data.english_name), bone->interface_data.english_name);
		data_pointer += data_size;
	}
}

static void PmxModelParseMorphs(PMX_MODEL* model)
{
	const int num_morphs = (int)model->data_info.morphs_count;
	uint8 *data_pointer = model->data_info.morphs;
	size_t data_size;
	int i;

	model->morphs->buffer_size = num_morphs + 1;
	model->morphs->buffer = (uint8*)MEM_REALLOC_FUNC(
		model->morphs->buffer, sizeof(PMX_MORPH) * model->morphs->buffer_size);
	for(i=0; i<num_morphs; i++)
	{
		PMX_MORPH *morph = (PMX_MORPH*)StructArrayReserve(model->morphs);
		InitializePmxMorph(morph, model);
		PmxMorphRead(morph, data_pointer, &data_size, &model->data_info);
		(void)ght_insert(model->name2morph, morph, (unsigned int)strlen(morph->interface_data.name), morph->interface_data.name);
		(void)ght_insert(model->name2morph, morph, (unsigned int)strlen(morph->interface_data.english_name), morph->interface_data.english_name);
		data_pointer += data_size;
	}
}

static void PmxModelParseLabels(PMX_MODEL* model)
{
	const int num_labels = (int)model->data_info.labels_count;
	uint8 *data_pointer = model->data_info.labels;
	size_t data_size;
	int i;

	for(i=0; i<num_labels; i++)
	{
		PMX_MODEL_LABEL *label = (PMX_MODEL_LABEL*)StructArrayReserve(model->labels);
		InitializePmxModelLabel(label, model);
		PmxModelLabelRead(label, data_pointer, &data_size, &model->data_info);
		data_pointer += data_size;
	}
}

static void PmxModelParseRigidBodies(PMX_MODEL* model)
{
	const int num_rigid_bodies = (int)model->data_info.rigid_bodies_count;
	uint8 *data_pointer = model->data_info.rigid_bodies;
	size_t data_size;
	int i;

	for(i=0; i<num_rigid_bodies; i++)
	{
		PMX_RIGID_BODY *rigid_body = (PMX_RIGID_BODY*)StructArrayReserve(model->rigid_bodies);
		InitializePmxRigidBody(rigid_body, model, model->parent_scene->project->application_context);
		PmxRigidBodyRead(rigid_body, data_pointer, &model->data_info, &data_size);
		data_pointer += data_size;
	}
}

static void PmxModelParseJoints(PMX_MODEL* model)
{
	const int num_joints = (int)model->data_info.joints_count;
	uint8 *data_pointer = model->data_info.joints;
	size_t data_size;
	int i;

	for(i=0; i<num_joints; i++)
	{
		PMX_JOINT *joint = (PMX_JOINT*)StructArrayReserve(model->joints);
		InitializePmxJoint(joint, model);
		PmxJointRead(joint, data_pointer, &model->data_info, &data_size);
		data_pointer += data_size;
	}
}

static void PmxModelParseSoftBody(PMX_MODEL* model)
{
	if(model->data_info.version >= 2.1f)
	{
		const int num_soft_bodies = (int)model->data_info.soft_bodies_count;
		uint8 *data_pointer = model->data_info.soft_bodies;
		size_t data_size;
		int i;

		for(i=0; i<num_soft_bodies; i++)
		{
			PMX_SOFT_BODY *body = (PMX_SOFT_BODY*)StructArrayReserve(model->soft_bodies);
			InitializePmxSoftBody(body, model);
			PmxSoftBodyRead(body, data_pointer, &data_size, &model->data_info);
			data_pointer += data_size;
		}
	}
}

void PmxModelUpdateLocalTransform(PMX_MODEL* model, POINTER_ARRAY* bones)
{
	const int num_bones = (int)bones->num_data;
	void *processor;
	int i;

	for(i=0; i<num_bones; i++)
	{
		PMX_BONE *bone = (PMX_BONE*)bones->buffer[i];
		PmxBonePerformTransform(bone);
		PmxBoneSolveInverseKinematics(bone);
	}

	processor = SimpleParallelProcessorNew((void*)bones->buffer,
		sizeof(void*), num_bones, UpdateBoneLocalTransform, NULL);
	ExecuteSimpleParallelProcessor(processor);
	DeleteSimpleParallelProcessor(processor);
}

void PmxModelPerformUpdate(PMX_MODEL* model, int force_sync)
{
	PMX_BONE *b = (PMX_BONE*)model->bones->buffer;
	PMX_MORPH *m = (PMX_MORPH*)model->morphs->buffer;
	const int num_bones = (int)model->bones->num_data;
	const int num_morphs = (int)model->morphs->num_data;
	void *processor;
	int i;

	for(i=0; i<num_bones; i++)
	{
		PMX_BONE *bone = &b[i];
		PmxBoneResetIkLink(bone);
	}

	processor = SimpleParallelProcessorNew((void*)model->vertices->buffer, sizeof(PMX_VERTEX),
		(int)model->vertices->num_data, (void (*)(void*, int, void*))ResetVertex, NULL);
	ExecuteSimpleParallelProcessor(processor);
	DeleteSimpleParallelProcessor(processor);

	for(i=0; i<num_morphs; i++)
	{
		PMX_MORPH *morph = &m[i];
		PmxMorphSyncWeight(morph);
	}

	for(i=0; i<num_morphs; i++)
	{
		PMX_MORPH *morph = &m[i];
		PmxMorphUpdate(morph);
	}

	PmxModelUpdateLocalTransform(model, model->bones_before_physics);
	if((model->flags & PMX_FLAG_ENABLE_PHYSICS) != 0 || force_sync != FALSE)
	{
		processor = SimpleParallelProcessorNew((void*)model->rigid_bodies->buffer,
			sizeof(PMX_RIGID_BODY), (int)model->rigid_bodies->num_data, RigidBodySyncLocalTransform, NULL);
		ExecuteSimpleParallelProcessor(processor);
		DeleteSimpleParallelProcessor(processor);
	}
	PmxModelUpdateLocalTransform(model, model->bones_after_physics);
}

int LoadPmxModel(PMX_MODEL* model, uint8* data, size_t data_size)
{
	PMX_DATA_INFO info = {0};
	if(PmxModelPreparse(model, data, data_size, &info) != FALSE)
	{
		PmxModelRelease(model);
		PmxModelParseNamesAndComments(model);
		PmxModelParseVertices(model);
		PmxModelParseIndices(model);
		PmxModelParseTextures(model);
		PmxModelParseMaterials(model);
		PmxModelParseBones(model);
		PmxModelParseMorphs(model);
		PmxModelParseLabels(model);
		PmxModelParseRigidBodies(model);
		PmxModelParseJoints(model);
		PmxModelParseSoftBody(model);

		if(LoadPmxBones(model->bones) == FALSE)
		{
			return FALSE;
		}
		if(LoadPmxMaterials(model->materials, model->textures, (int)model->indices->num_data) == FALSE)
		{
			return FALSE;
		}
		if(PmxVerticesLoad(model->vertices, model->bones) == FALSE)
		{
			return FALSE;
		}
		if(PmxMorphLoad(model->morphs, model->bones, model->materials, model->rigid_bodies, model->vertices) == FALSE)
		{
			return FALSE;
		}
		if(LoadPmxModelLabels(model->labels, model->bones, model->morphs) == FALSE)
		{
			return FALSE;
		}
		if(PmxRigidBodyLoad(model->rigid_bodies, model->bones) == FALSE)
		{
			return FALSE;
		}
		if(LoadPmxJoints(model->joints, model->rigid_bodies) == FALSE)
		{
			return FALSE;
		}
		if(LoadPmxSoftBodies(model->soft_bodies) == FALSE)
		{
			return FALSE;
		}

		SortPmxBones(model->bones, model->bones_after_physics, model->bones_before_physics);
		PmxModelPerformUpdate(model, FALSE);

		return TRUE;
	}

	return FALSE;
}

void PmxModelResetMotionState(PMX_MODEL* model, void* world)
{
	PMX_BONE **bones;
	const float gravity[3] = {0, -9.8f, 0};
	int num_bones;
	PMX_RIGID_BODY *bodies = (PMX_RIGID_BODY*)model->rigid_bodies->buffer;
	PMX_RIGID_BODY *body;
	const int num_bodies = (int)model->rigid_bodies->num_data;
	PMX_JOINT *joints = (PMX_JOINT*)model->joints->buffer;
	PMX_JOINT *joint;
	const int num_joints = (int)model->joints->num_data;
	int i;

	if(world == NULL)
	{
		return;
	}

	num_bones = (int)model->bones_before_physics->num_data;
	bones = (PMX_BONE**)model->bones_before_physics->buffer;
	for(i=0; i<num_bones; i++)
	{
		PmxBoneResetIkLink(bones[i]);
	}

	PmxModelUpdateLocalTransform(model, model->bones_before_physics);

	for(i=0; i<num_bodies; i++)
	{
		body = &bodies[i];
		ResetBaseRigidBody((BASE_RIGID_BODY*)body, world);
		BaseRigidBodyUpdateTransform((BASE_RIGID_BODY*)body);
		BaseRigidBodySetActivation((BASE_RIGID_BODY*)body, TRUE);
	}

	for(i=0; i<num_joints; i++)
	{
		joint = &joints[i];
		BaseJointUpdateTransform((BASE_JOINT*)joint);
	}

	PmxModelUpdateLocalTransform(model, model->bones_after_physics);

	BtDynamicsWorldSetGravity(world, gravity);
}

static void AddBoneIndices(void *vertex, int num_indices, UINT32_ARRAY* bone_indices)
{
	VERTEX_INTERFACE *inter = (VERTEX_INTERFACE*)vertex;
	BONE_INTERFACE *bone;
	int size;
	uint32 index;
	int i;

	if(num_indices <= 0)
	{
		switch(inter->type)
		{
		case BASE_VERTEX_TYPE_B_DEF1:
			num_indices = 1;
			break;
		case BASE_VERTEX_TYPE_B_DEF2:
		case BASE_VERTEX_TYPE_S_DEF:
			num_indices = 2;
			break;
		case BASE_VERTEX_TYPE_B_DEF4:
		case BASE_VERTEX_TYPE_Q_DEF:
			num_indices = 4;
			break;
		default:
			return;
		}
	}

	size = (int)bone_indices->num_data;
	for(i=0; i<num_indices; i++)
	{
		bone = (BONE_INTERFACE*)inter->get_bone(vertex, i);
		index = (uint32)bone->index;
		if(bsearch(&index, bone_indices->buffer, bone_indices->num_data, sizeof(uint32),
			(int (*)(const void*, const void*))CompareInt32) == NULL)
		{
			Uint32ArrayAppend(bone_indices, index);
			qsort(bone_indices->buffer, bone_indices->num_data, sizeof(uint32),
				(int (*)(const void*, const void*))CompareInt32);
			size++;
		}
	}
}

size_t PmxDefaultStaticVertexBufferGetSize(PMX_DEFAULT_STATIC_VERTEX_BUFFER* buffer)
{
	return buffer->interface_data.base.get_stride_size(buffer) * buffer->model->vertices->num_data;
}

size_t PmxDefaultStaticVertexBufferGetStrideSize(PMX_DEFAULT_STATIC_VERTEX_BUFFER* buffer)
{
	return sizeof(PMX_DEFAULT_STATIC_BUFFER_UNIT);
}

size_t PmxDefaultStaticVertexBufferGetStrideOffset(PMX_DEFAULT_STATIC_VERTEX_BUFFER* buffer, eMODEL_STRIDE_TYPE type)
{
	switch(type)
	{
	case MODEL_BONE_INDEX_STRIDE:
		return offsetof(PMX_DEFAULT_STATIC_BUFFER_UNIT, bone_indices);
	case MODEL_BONE_WEIGHT_STRIDE:
		return offsetof(PMX_DEFAULT_STATIC_BUFFER_UNIT, bone_weights);
	case MODEL_TEXTURE_COORD_STRIDE:
		return offsetof(PMX_DEFAULT_STATIC_BUFFER_UNIT, texture_coord);
	}

	return 0;
}

float PmxDefaultStaticVertexBufferResolveRelativeBoneIndex(
	PMX_DEFAULT_STATIC_BUFFER_UNIT* unit,
	const PMX_VERTEX* vertex,
	int offset,
	ght_hash_table_t* bone_index_hashes
)
{
	BONE_INTERFACE *bone;
	if((bone = (BONE_INTERFACE*)vertex->interface_data.get_bone((void*)vertex, offset)) != NULL)
	{
		int bone_index = bone->index;
		//int *bone_index_ptr;
		if(/*bone_index_hashes != NULL &&*/ bone_index >= 0)
		{
			//if((bone_index_ptr = (int*)ght_get(bone_index_hashes, sizeof(int), (void*)&bone_index)) != NULL)
			{
				//const int relative_bone_index = (int)bone_index_ptr;
				const int relative_bone_index = (int)bone_index;
				return (float)relative_bone_index;
			}
		}
	}

	return -1;
}

void PmxDefaultStaticVertexBufferUpdateUnit(
	PMX_DEFAULT_STATIC_BUFFER_UNIT* unit,
	PMX_VERTEX* vertex,
	ght_hash_table_t* bone_index_hashes
)
{
	int i;

	for(i=0; i<PMX_VERTEX_MAX_BONES; i++)
	{
		unit->bone_indices[i] = PmxDefaultStaticVertexBufferResolveRelativeBoneIndex(
			unit, vertex, i, bone_index_hashes);
		unit->bone_weights[i] = (float)vertex->weight[i];
	}
	COPY_VECTOR3(unit->texture_coord, vertex->tex_coord);
}

void PmxDefaultStaticVertexBufferAddBoneIndicesData(
	const PMX_VERTEX* vertex,
	int num_indices,
	ght_hash_table_t* bone_indices
)
{
	unsigned int size = ght_size(bone_indices);
	int i;

	for(i=0; i<num_indices; i++)
	{
		int index = ((BONE_INTERFACE*)vertex->interface_data.get_bone((void*)vertex, i))->index;
		if(index == 0 || ght_get(bone_indices, sizeof(index), (void*)&index) == NULL)
		{
			unsigned int key = (unsigned int)ght_size(bone_indices);
			(void)ght_insert(bone_indices, (void*)index, sizeof(key), (void*)&key);
			size = ght_size(bone_indices);
		}
	}
}

void PmxDefaultStaticVertexBufferAddBoneIndices(
	PMX_VERTEX* vertex,
	ght_hash_table_t* bone_indices
)
{
	switch(vertex->interface_data.type)
	{
	case BASE_VERTEX_TYPE_B_DEF1:
		PmxDefaultStaticVertexBufferAddBoneIndicesData(vertex, 1, bone_indices);
		break;
	case BASE_VERTEX_TYPE_B_DEF2:
	case BASE_VERTEX_TYPE_S_DEF:
		PmxDefaultStaticVertexBufferAddBoneIndicesData(vertex, 2, bone_indices);
		break;
	case BASE_VERTEX_TYPE_B_DEF4:
	case BASE_VERTEX_TYPE_Q_DEF:
		PmxDefaultStaticVertexBufferAddBoneIndicesData(vertex, 4, bone_indices);
		break;
	}
}

void PmxDefaultStaticVertexBufferUpdateBoneIndexHashes(
	PMX_DEFAULT_STATIC_VERTEX_BUFFER* buffer,
	PMX_MODEL* model,
	POINTER_ARRAY* bone_index_hashes
)
{
	PMX_MATERIAL *materials = (PMX_MATERIAL*)model->materials->buffer;
	const int num_materials = (int)model->materials->num_data;
	PMX_VERTEX *vertices = (PMX_VERTEX*)model->vertices->buffer;
	uint32 *indices = model->indices->buffer;
	ght_hash_table_t *bone_indices;
	ght_hash_table_t *hash_indices;
	int offset = 0;
	int i, j;

	PointerArrayRelease(bone_index_hashes, (void (*)(void*))ght_finalize);
	for(i=0; i<num_materials; i++)
	{
		MATERIAL_INTERFACE *material = (MATERIAL_INTERFACE*)&materials[i];
		const int num_indices = material->get_index_range((void*)material).count;
		int num_bone_indices;

		bone_indices = ght_create(DEFAULT_BUFFER_SIZE);

		bone_index_hashes->num_data = 0;
		for(j=0; j<num_indices; j++)
		{
			const int vertex_index = indices[offset + j];
			PMX_VERTEX *vertex = &vertices[vertex_index];
			PmxDefaultStaticVertexBufferAddBoneIndices(vertex, bone_indices);
		}
		num_bone_indices = ght_size(bone_indices);
		hash_indices = ght_create(DEFAULT_BUFFER_SIZE);
		PointerArrayAppend(bone_index_hashes, (void*)hash_indices);
		for(j=0; j<num_bone_indices; j++)
		{
			const int bone_index = (int)ght_get(bone_indices, sizeof(j), (void*)&j);
			ght_insert(hash_indices, (void*)bone_index, sizeof(bone_index), (void*)&j);
		}
		offset += num_indices;
	}

	ght_finalize(bone_indices);
}

void PmxDefaultStaticVertexBufferUpdate(PMX_DEFAULT_STATIC_VERTEX_BUFFER* buffer, void* address)
{
	PMX_MATERIAL *materials = (PMX_MATERIAL*)buffer->model->materials->buffer;
	const int num_materials = (int)buffer->model->materials->num_data;
	uint32 *indices = (uint32*)buffer->model->indices->buffer;
	PMX_VERTEX *vertices = (PMX_VERTEX*)buffer->model->vertices->buffer;
	PMX_DEFAULT_STATIC_BUFFER_UNIT *unit = (PMX_DEFAULT_STATIC_BUFFER_UNIT*)address;
	int offset = 0;
	int i, j;

	PmxDefaultStaticVertexBufferUpdateBoneIndexHashes(buffer, buffer->model, buffer->bone_index_hashes);
	for(i=0; i<num_materials; i++)
	{
		MATERIAL_INTERFACE *material = (MATERIAL_INTERFACE*)&materials[i];
		ght_hash_table_t *bone_index_hash = (ght_hash_table_t*)buffer->bone_index_hashes->buffer[i];
		const int num_indices = material->get_index_range(material).count;

		for(j=0; j<num_indices; j++)
		{
			const int vertex_index = indices[offset + j];
			PMX_VERTEX *vertex = &vertices[vertex_index];
			PmxDefaultStaticVertexBufferUpdateUnit(&unit[vertex_index], vertex, bone_index_hash);
		}
		offset += num_indices;
	}
	PointerArrayRelease(buffer->bone_index_hashes, (void (*)(void*))ght_finalize);

#ifdef _DEBUG
/*
	{
		FILE *fp = fopen("my_staticx64.csv", "wt");

		for(i=0; i<buffer->model->vertices->num_data; i++)
		{
			fprintf(fp, "index, %d\n", i);
			fprintf(fp, "texture, %f\n, %f\n, %f\n", unit[i].texture_coord[0], unit[i].texture_coord[1], unit[i].texture_coord[2]);
			fprintf(fp, "bone, %f\n, %f\n, %f\n, %f\n", unit[i].bone_indices[0], unit[i].bone_indices[1], unit[i].bone_indices[2], unit[i].bone_indices[3]);
			fprintf(fp, "weight, %f\n, %f\n, %f\n, %f\n", unit[i].bone_weights[0], unit[i].bone_weights[1], unit[i].bone_weights[2], unit[i].bone_weights[3]);
		}

		fclose(fp);
	}
*/
#endif
}

PMX_DEFAULT_STATIC_VERTEX_BUFFER* PmxDefaultStaticVertexBufferNew(PMX_MODEL* model)
{
	PMX_DEFAULT_STATIC_VERTEX_BUFFER* ret;

	ret = (PMX_DEFAULT_STATIC_VERTEX_BUFFER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->model = model;
	ret->bone_index_hashes = PointerArrayNew(DEFAULT_BUFFER_SIZE);

	ret->interface_data.base.get_size =
		(size_t (*)(void*))PmxDefaultStaticVertexBufferGetSize;
	ret->interface_data.base.get_stride_size =
		(size_t (*)(void*))PmxDefaultStaticVertexBufferGetStrideSize;
	ret->interface_data.base.get_stride_offset =
		(size_t (*)(void*, eMODEL_STRIDE_TYPE))PmxDefaultStaticVertexBufferGetStrideOffset;
	ret->interface_data.base.delete_func =
		(void (*)(void*))DeletePmxDefaultStaticVertexBuffer;
	ret->interface_data.update =
		(void (*)(void*, void*))PmxDefaultStaticVertexBufferUpdate;

	return ret;
}

void DeletePmxDefaultStaticVertexBuffer(PMX_DEFAULT_STATIC_VERTEX_BUFFER* buffer)
{
	PointerArrayDestroy(&buffer->bone_index_hashes, (void (*)(void*))ght_finalize);
	MEM_FREE_FUNC(buffer);
}

void* PmxDefaultDynamicVertexBufferGetIdent(PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer)
{
	return (void*)&buffer->model->interface_data.scene->project->pmx_default_buffer_ident.dynamic_buffer_ident;
}

size_t PmxDefaultDynamicVertexBufferGetSize(PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer)
{
	return buffer->interface_data.base.get_stride_size(buffer) * buffer->model->vertices->num_data;
}

size_t PmxDefaultDynamicVertexBufferGetStrideSize(PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer)
{
	return sizeof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT);
}

size_t PmxDefaultDynamicVertexByfferGetStrideOffset(PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer, eMODEL_STRIDE_TYPE type)
{
	switch(type)
	{
	case MODEL_VERTEX_STRIDE:
		return offsetof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT, position);
	case MODEL_NORMAL_STRIDE:
		return offsetof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT, normal);
	case MODEL_MORPH_DELTA_STRIDE:
		return offsetof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT, delta);
	case MODEL_EDGE_VERTEX_STRIDE:
		return offsetof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT, edge);
	case MODEL_EDGE_SIZE_STRIDE:
		{
			uint8 *base = (uint8*)&buffer->model->interface_data.scene->project->pmx_default_buffer_ident.dynamic_buffer_ident;
			return (size_t)((uint8*)&buffer->model->interface_data.scene->project->pmx_default_buffer_ident.dynamic_buffer_ident.normal[3] - base);
		}
	case MODEL_VERTEX_INDEX_STRIDE:
		{
			uint8 *base = (uint8*)&buffer->model->interface_data.scene->project->pmx_default_buffer_ident.dynamic_buffer_ident;
			return (size_t)((uint8*)&buffer->model->interface_data.scene->project->pmx_default_buffer_ident.dynamic_buffer_ident.edge[3] - base);
		}
	case MODEL_UVA1_STRIDE:
		return offsetof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT, uva1);
	case MODEL_UVA2_STRIDE:
		return offsetof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT, uva2);
	case MODEL_UVA3_STRIDE:
		return offsetof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT, uva3);
	case MODEL_UVA4_STRIDE:
		return offsetof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT, uva4);
	}

	return 0;
}

void PmxDefaultDynamicVertexBufferUnitUpdateMorph(
	PMX_DEFAULT_DYNAMIC_BUFFER_UNIT* unit,
	VERTEX_INTERFACE* vertex
)
{
	vertex->get_delta(vertex, unit->delta);
	vertex->get_uv(vertex, 0, unit->uva1);
	vertex->get_uv(vertex, 1, unit->uva2);
	vertex->get_uv(vertex, 2, unit->uva3);
	vertex->get_uv(vertex, 3, unit->uva4);
}

void PmxDefaultDynamicVertexBufferUnitUpdate(
	PMX_DEFAULT_DYNAMIC_BUFFER_UNIT* unit,
	VERTEX_INTERFACE* vertex,
	FLOAT_T material_edge_size,
	int index,
	float* p
)
{
	float n[3];
	FLOAT_T edge_size = vertex->edge_size * material_edge_size;
	vertex->perform_skinning((void*)vertex, p, n);
	COPY_VECTOR3(unit->position, p);
	COPY_VECTOR3(unit->normal, n);
	unit->normal[3] = (float)edge_size;
	COPY_VECTOR3(unit->edge, unit->normal);
	ScaleVector3(unit->edge, (float)edge_size);
	PlusVector3(unit->edge, unit->position);
	PmxDefaultDynamicVertexBufferUnitUpdateMorph(unit, vertex);
}

void PmxDefaultDynamicVertexBufferPerformTransform(
	PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer,
	void* address,
	float* camera_position,
	float* aa_bb_min,
	float* aa_bb_max
)
{
	PMX_VERTEX *vertices = (PMX_VERTEX*)buffer->model->vertices->buffer;
	PMX_DEFAULT_DYNAMIC_BUFFER_UNIT *unit = (PMX_DEFAULT_DYNAMIC_BUFFER_UNIT*)address;
	PARALLEL_SKINNING_VERTEX skinning;
	void *processor;

	InitializeSkinningVertex(&skinning, (MODEL_INTERFACE*)buffer->model, buffer->model->vertices,
		camera_position, address, (void (*)(void*, struct _VERTEX_INTERFACE*, FLOAT_T, int, float*))PmxDefaultDynamicVertexBufferUnitUpdate,
		sizeof(PMX_DEFAULT_DYNAMIC_BUFFER_UNIT));
	processor = ParallelProcessorNew((void*)buffer->model->vertices->buffer, (void (*)(void*, int, void*))SkinningVertexProcess,
		(void (*)(void*))SkinningVertexStart, (void (*)(void*))SkinningVertexEnd, (void (*)(void*, void*))SkinningVertexMakeNext,
		(void (*)(void*, void*))SkinningVertexJoin, buffer->model->vertices->data_size, (int)buffer->model->vertices->num_data,
		(void*)&skinning, sizeof(skinning), PARALLEL_PROCESSOR_TYPE_REDUCTION);
	ExecuteParallelProcessor(processor);
	DeleteParallelProcessor(processor);
	COPY_VECTOR3(aa_bb_min, skinning.aa_bb_min);
	COPY_VECTOR3(aa_bb_max, skinning.aa_bb_max);

#ifdef _DEBUG
	/*
	{
		static int count = 0;
		char file_name[128];
		FILE *fp;
		int i;
		sprintf(file_name, "my_dynamicx64%d.csv", count);
		count++;
		fp = fopen(file_name, "wt");
		for(i=0; i<buffer->model->vertices->num_data; i++)
		{
			fprintf(fp, "index, %d\n", i);
			fprintf(fp, "position, %f\n, %f\n, %f\n", unit[i].position[0], unit[i].position[1], unit[i].position[2]);
			fprintf(fp, "normal, %f\n, %f\n, %f\n", unit[i].normal[0], unit[i].normal[1], unit[i].normal[2], unit[i].normal[3]);
			fprintf(fp, "delta, %f\n, %f\n, %f\n", unit[i].delta[0], unit[i].delta[1], unit[i].delta[2]);
			fprintf(fp, "edge, %f\n, %f\n, %f\n", unit[i].edge[0], unit[i].edge[1], unit[i].edge[2], unit[i].edge[3]);
			fprintf(fp, "uva1, %f\n, %f\n, %f\n, %f\n", unit[i].uva1[0], unit[i].uva1[1], unit[i].uva1[2], unit[i].uva1[3]);
		}
		fclose(fp);
	}
	*/
#endif

}

void DeletePmxDefaultDynamicVertexBuffer(PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer)
{
	MEM_FREE_FUNC(buffer);
}

PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* PmxDefaultDynamicVertexBufferNew(
	PMX_MODEL* model,
	MODEL_INDEX_BUFFER_INTERFACE* index_buffer
)
{
	PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* ret;

	ret = (PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->model = model;
	ret->index_buffer = index_buffer;

	ret->interface_data.base.ident = (void* (*)(void*))PmxDefaultDynamicVertexBufferGetIdent;
	ret->interface_data.base.get_size = (size_t (*)(void*))PmxDefaultDynamicVertexBufferGetSize;
	ret->interface_data.base.get_stride_size = (size_t (*)(void*))PmxDefaultDynamicVertexBufferGetStrideSize;
	ret->interface_data.base.get_stride_offset = (size_t (*)(void*, eMODEL_STRIDE_TYPE))PmxDefaultDynamicVertexByfferGetStrideOffset;
	ret->interface_data.base.delete_func = (void (*)(void*))DeletePmxDefaultDynamicVertexBuffer;
	ret->interface_data.perform_transform = (void (*)(void*, void*, const float*, float*, float*))PmxDefaultDynamicVertexBufferPerformTransform;

	return ret;
}

void PmxDefaultIndexBufferSetIndexAt(PMX_DEFAULT_INDEX_BUFFER* buffer, int index, int value)
{
	switch(buffer->index_type)
	{
	case MODEL_INDEX8:
		buffer->indices.indices8[index] = (int8)value;
		break;
	case MODEL_INDEX16:
		buffer->indices.indices16[index] = (int16)value;
		break;
	case MODEL_INDEX32:
		buffer->indices.indices32[index] = (int32)value;
		break;
	}
}

void* PmxDefaultIndexBufferGetIdent(PMX_DEFAULT_INDEX_BUFFER* buffer)
{
	return (void*)&buffer->model->interface_data.scene->project->pmx_default_buffer_ident.index_buffer_ident;
}

eMODEL_INDEX_BUFFER_TYPE PmxDefaultIndexBufferGetType(PMX_DEFAULT_INDEX_BUFFER* buffer)
{
	return buffer->index_type;
}

size_t PmxDefaultIndexBufferGetSize(PMX_DEFAULT_INDEX_BUFFER* buffer)
{
	return buffer->interface_data.base.get_stride_size(buffer) * buffer->num_indices;
}

size_t PmxDefaultIndexBufferGetStrideSize(PMX_DEFAULT_INDEX_BUFFER* buffer)
{
	switch(buffer->index_type)
	{
	case MODEL_INDEX_BUFFER_TYPE_INDEX8:
		return sizeof(uint8);
	case MODEL_INDEX_BUFFER_TYPE_INDEX16:
		return sizeof(uint16);
	case MODEL_INDEX_BUFFER_TYPE_INDEX32:
		return sizeof(uint32);
	}

	return 0;
}

void* PmxDefaultIndexBufferGetBytes(PMX_DEFAULT_INDEX_BUFFER* buffer)
{
	return (void*)buffer->indices.indices8;
}

void DeletePmxDefaultIndexBuffer(PMX_DEFAULT_INDEX_BUFFER* buffer)
{
	MEM_FREE_FUNC(buffer->indices.indices8);
	MEM_FREE_FUNC(buffer);
}

PMX_DEFAULT_INDEX_BUFFER* PmxDefaultIndexBufferNew(
	UINT32_ARRAY* indices,
	const int num_vertices,
	PMX_MODEL* model
)
{
	PMX_DEFAULT_INDEX_BUFFER *ret;
	int i;

	ret = (PMX_DEFAULT_INDEX_BUFFER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->num_indices = (int)indices->num_data;

	if(ret->num_indices <= 0xFF)
	{
		ret->index_type = MODEL_INDEX8;
		ret->indices.indices8 = (uint8*)MEM_CALLOC_FUNC(ret->num_indices, sizeof(uint8));
	}
	else if(ret->num_indices <= 0xFFFF)
	{
		ret->index_type = MODEL_INDEX16;
		ret->indices.indices16 = (uint16*)MEM_CALLOC_FUNC(ret->num_indices, sizeof(uint16));
	}
	else
	{
		ret->index_type = MODEL_INDEX32;
		ret->indices.indices32 = (uint32*)MEM_CALLOC_FUNC(ret->num_indices, sizeof(uint32));
	}

	for(i=0; i<ret->num_indices; i++)
	{
		const int index = (int)indices->buffer[i];
		if(index >= 0 && index < num_vertices)
		{
			PmxDefaultIndexBufferSetIndexAt(ret, i, index);
		}
		else
		{
			PmxDefaultIndexBufferSetIndexAt(ret, i, 0);
		}
	}

	switch(ret->index_type)
	{
	case MODEL_INDEX8:
		{
			uint8 temp;
			for(i=0; i<ret->num_indices; i+=3)
			{
				temp = ret->indices.indices8[i];
				ret->indices.indices8[i] = ret->indices.indices8[i+1];
				ret->indices.indices8[i+1] = temp;
			}
		}
		break;
	case MODEL_INDEX16:
		{
			uint16 temp;
			for(i=0; i<ret->num_indices; i+=3)
			{
				temp = ret->indices.indices16[i];
				ret->indices.indices16[i] = ret->indices.indices16[i+1];
				ret->indices.indices16[i+1] = temp;
			}
		}
		break;
	case MODEL_INDEX32:
		{
			uint32 temp;
			for(i=0; i<ret->num_indices; i+=3)
			{
				temp = ret->indices.indices32[i];
				ret->indices.indices32[i] = ret->indices.indices32[i+1];
				ret->indices.indices32[i+1] = temp;
			}
		}
		break;
	}

	ret->model = model;

	ret->interface_data.base.ident = (void* (*)(void*))PmxDefaultIndexBufferGetIdent;
	ret->interface_data.get_type = (eMODEL_INDEX_BUFFER_TYPE (*)(void*))PmxDefaultIndexBufferGetType;
	ret->interface_data.bytes = (void* (*)(void*))PmxDefaultIndexBufferGetBytes;
	ret->interface_data.base.get_size = (size_t (*)(void*))PmxDefaultIndexBufferGetSize;
	ret->interface_data.base.get_stride_size = (size_t (*)(void*))PmxDefaultIndexBufferGetStrideSize;
	ret->interface_data.base.delete_func = (void (*)(void*))DeletePmxDefaultIndexBuffer;

	return ret;
}

size_t PmxDefaultMatrixBufferGetSize(PMX_DEFAULT_MATRIX_BUFFER* buffer, int material_index)
{
	const int num_bones = (int)buffer->meshes.bones->num_data;
	if(CHECK_BOUND(material_index, 0, num_bones))
	{
		UINT32_ARRAY *int_buffer;
		int_buffer = (UINT32_ARRAY*)buffer->meshes.bones->buffer[material_index];
		return int_buffer->num_data;
	}
	return 0;
}

float* PmxDefaultMatrixBufferGetBytes(PMX_DEFAULT_MATRIX_BUFFER* buffer, int material_index)
{
	const int num_matrices = (int)buffer->meshes.matrices->num_data;
	return CHECK_BOUND(material_index, 0, num_matrices) ? buffer->meshes.matrices->buffer[material_index] : NULL;
}

static void UpdateBoneLocalTransforms(PMX_DEFAULT_MATRIX_BUFFER* buffer)
{
	PMX_BONE *bones = (PMX_BONE*)buffer->model->bones->buffer;
	PMX_MATERIAL *materials = (PMX_MATERIAL*)buffer->model->materials->buffer;
	//const float basis[9] = IDENTITY_MATRIX3x3;
	uint8 local_bone_transform[TRANSFORM_SIZE];
	uint8 static_bone_local_transform[TRANSFORM_SIZE];
	const int num_materials = (int)buffer->model->materials->num_data;
	int i, j;

	BtTransformSetIdentity(local_bone_transform);
	BtTransformSetIdentity(static_bone_local_transform);
	for(i=0; i<num_materials; i++)
	{
		UINT32_ARRAY *bone_indices = (UINT32_ARRAY*)buffer->meshes.bones->buffer[i];
		const int num_bone_indices = (int)bone_indices->num_data;
		float *matrices = (float*)buffer->meshes.matrices->buffer[i];
		int32 *indices = (int32*)bone_indices->buffer;
		BtTransformGetOpenGLMatrix(static_bone_local_transform, matrices);
		for(j=1; j<num_bone_indices; j++)
		{
			const int bone_index = indices[j];

			bones[bone_index].interface_data.get_local_transform(&bones[bone_index], local_bone_transform);
			BtTransformGetOpenGLMatrix(local_bone_transform, &matrices[j * 16]);
		}
	}
}

void PmxDefaultMatrixBufferUpdate(PMX_DEFAULT_MATRIX_BUFFER* buffer, void* address)
{
	UpdateBoneLocalTransforms(buffer);
}

void InitializePmxDefaultMatrixBuffer(PMX_DEFAULT_MATRIX_BUFFER* buffer)
{
	PMX_MATERIAL *materials = (PMX_MATERIAL*)buffer->model->materials->buffer;
	PMX_VERTEX *vertices = (PMX_VERTEX*)buffer->model->vertices->buffer;
	uint32 *indices = (uint32*)buffer->model->indices->buffer;
	const int num_materials = (int)buffer->model->materials->num_data;
	UINT32_ARRAY *bone_indices = Uint32ArrayNew(DEFAULT_BUFFER_SIZE);
	float *matrix;
	size_t size;
	int offset = 0;
	int i, j;

	buffer->meshes.bones = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	buffer->meshes.matrices = PointerArrayNew(DEFAULT_BUFFER_SIZE);

	for(i=0; i<num_materials; i++)
	{
		PMX_MATERIAL *material = &materials[i];
		const int num_indices = material->index_range.count;
		bone_indices->num_data = 0;
		//Uint32ArrayAppend(bone_indices, (uint32)-1);
		for(j=0; j<num_indices; j++)
		{
			const int vertex_index = indices[offset + j];
			const PMX_VERTEX *vertex = &vertices[vertex_index];
			AddBoneIndices((void*)vertex, 0, bone_indices);
		}
		size = bone_indices->num_data * 16;
		matrix = (float*)MEM_ALLOC_FUNC(sizeof(*matrix)*size);
		(void)memset(matrix, 0, sizeof(*matrix)*size);
		PointerArrayAppend(buffer->meshes.matrices, matrix);
		PointerArrayAppend(buffer->meshes.bones, bone_indices);
		offset += num_indices;
	}

	UpdateBoneLocalTransforms(buffer);
}

void DeletePmxDefaultMatrixBuffer(PMX_DEFAULT_MATRIX_BUFFER* buffer)
{
	size_t i;

	for(i=0; i<buffer->meshes.bones->num_data; i++)
	{
		UINT32_ARRAY *p = (UINT32_ARRAY*)buffer->meshes.bones->buffer[i];
		Uint32ArrayDestroy(&p);
	}
	PointerArrayDestroy(&buffer->meshes.bones, NULL);
	PointerArrayDestroy(&buffer->meshes.matrices, MEM_FREE_FUNC);

	MEM_FREE_FUNC(buffer);
}

PMX_DEFAULT_MATRIX_BUFFER* PmxDefaultMatrixBufferNew(
	PMX_MODEL* model,
	PMX_DEFAULT_INDEX_BUFFER* index_buffer,
	PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* dynamic_buffer
)
{
	PMX_DEFAULT_MATRIX_BUFFER *ret;

	ret = (PMX_DEFAULT_MATRIX_BUFFER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->model = model;
	ret->index_buffer = index_buffer;
	ret->dynamic_buffer = dynamic_buffer;
	ret->interface_data.update = (void (*)(void*, void*))PmxDefaultMatrixBufferUpdate;
	ret->interface_data.get_size = (size_t (*)(void*, int))PmxDefaultMatrixBufferGetSize;
	ret->interface_data.base.delete_func = (void (*)(void*))DeletePmxDefaultMatrixBuffer;
	ret->interface_data.bytes = (float* (*)(void*, int))PmxDefaultMatrixBufferGetBytes;

	InitializePmxDefaultMatrixBuffer(ret);

	return ret;
}

void PmxModelGetIndexBuffer(PMX_MODEL* model, PMX_DEFAULT_INDEX_BUFFER** buffer)
{
	if(buffer != NULL)
	{
		if(*buffer != NULL)
		{
			DeletePmxDefaultIndexBuffer(*buffer);
		}
		*buffer = PmxDefaultIndexBufferNew(model->indices, (int)model->vertices->num_data, model);
	}
}

void PmxModelGetStaticVertexBuffer(PMX_MODEL* model, PMX_DEFAULT_STATIC_VERTEX_BUFFER** buffer)
{
	if(buffer != NULL)
	{
		if(*buffer != NULL)
		{
			DeletePmxDefaultStaticVertexBuffer(*buffer);
		}
		*buffer = PmxDefaultStaticVertexBufferNew(model);
	}
}

void PmxModelGetDynamicVertexBuffer(
	PMX_MODEL* model,
	PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER** buffer,
	PMX_DEFAULT_INDEX_BUFFER* index_buffer
)
{
	if(buffer != NULL)
	{
		if(*buffer != NULL)
		{
			DeletePmxDefaultDynamicVertexBuffer(*buffer);
		}

		if(index_buffer != NULL
			&& index_buffer->interface_data.base.ident(index_buffer)
				== &model->interface_data.scene->project->pmx_default_buffer_ident.index_buffer_ident)
		{
			*buffer = PmxDefaultDynamicVertexBufferNew(model, (MODEL_INDEX_BUFFER_INTERFACE*)index_buffer);
		}
		else
		{
			*buffer = NULL;
		}
	}
}

void PmxModelGetMatrixBuffer(
	PMX_MODEL* model,
	PMX_DEFAULT_MATRIX_BUFFER** buffer,
	PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* dynamic_buffer,
	PMX_DEFAULT_INDEX_BUFFER* index_buffer
)
{
	if(buffer != NULL)
	{
		if(*buffer != NULL)
		{
			DeletePmxDefaultMatrixBuffer(*buffer);
		}

		if(index_buffer != NULL && index_buffer->interface_data.base.ident(index_buffer) == &model->interface_data.scene->project->pmx_default_buffer_ident.index_buffer_ident
			&& dynamic_buffer != NULL && dynamic_buffer->interface_data.base.ident(dynamic_buffer) == &model->interface_data.scene->project->pmx_default_buffer_ident.dynamic_buffer_ident
		)
		{
			*buffer = PmxDefaultMatrixBufferNew(model, index_buffer, dynamic_buffer);
		}
		else
		{
			*buffer = NULL;
		}
	}
}

void PmxModelJoinWorld(PMX_MODEL* model, void* world)
{
	if(world != NULL)
	{
		const int num_rigid_bodies = (int)model->rigid_bodies->num_data;
		const int num_joints = (int)model->joints->num_data;
		PMX_RIGID_BODY *bodies = (PMX_RIGID_BODY*)model->rigid_bodies->buffer;
		PMX_RIGID_BODY *body;
		PMX_JOINT *joints = (PMX_JOINT*)model->joints->buffer;
		PMX_JOINT *joint;
		int i;

		for(i=0; i<num_rigid_bodies; i++)
		{
			body = &bodies[i];
			BaseRigidBodyJoinWorld((BASE_RIGID_BODY*)body, world);
		}
		for(i=0; i<num_joints; i++)
		{
			joint = &joints[i];
			BaseJointJoinWorld((BASE_JOINT*)joint, world);
		}
	}
}

void PmxModelLeaveWorld(PMX_MODEL* model, void* world)
{
	if(world != NULL)
	{
		const int num_rigid_bodies = (int)model->rigid_bodies->num_data;
		const int num_joints = (int)model->joints->num_data;
		PMX_RIGID_BODY *bodies = (PMX_RIGID_BODY*)model->rigid_bodies->buffer;
		PMX_RIGID_BODY *body;
		PMX_JOINT *joints = (PMX_JOINT*)model->joints->buffer;
		PMX_JOINT *joint;
		int i;

		for(i=0; i<num_rigid_bodies; i++)
		{
			body = &bodies[i];
			BaseRigidBodyLeaveWorld((BASE_RIGID_BODY*)body, world);
		}
		for(i=0; i<num_joints; i++)
		{
			joint = &joints[i];
			BaseJointLeaveWorld((BASE_JOINT*)joint, world);
		}
	}
}
