// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <assimp/postprocess.h>
#include "asset_model.h"
#include "application.h"
#include "memory.h"

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

void AssetModelSetWorldPositionInternal(ASSET_MODEL* model, const float* translation)
{
	COPY_VECTOR3(model->position, translation);
}

void AssetModelSetWorldRotationInternal(ASSET_MODEL* model, const float* rotation)
{
	COPY_VECTOR4(model->rotation, rotation);
}
