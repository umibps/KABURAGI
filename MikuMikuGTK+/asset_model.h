#ifndef _INCLUDED_ASSET_MODEL_H_
#define _INCLUDED_ASSET_MODEL_H_

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include "types.h"
#include "utils.h"
#include "vertex.h"
#include "bone.h"
#include "material.h"
#include "morph.h"
#include "model.h"

#define ASSET_MODEL_BUFFER_SIZE 8

typedef struct _ASSET_MODEL
{
	MODEL_INTERFACE interface_data;
	struct aiScene *scene;
	char *model_path;
	ASSET_ROOT_BONE root_bone;
	ASSET_SCALE_BONE scale_bone;
	ASSET_OPACITY_MORPH opacity_morph;
	POINTER_ARRAY *bones;
	STRUCT_ARRAY *labels;
	STRUCT_ARRAY *materials;
	STRUCT_ARRAY *morphs;
	STRUCT_ARRAY *vertices;
	UINT32_ARRAY *indices;
	ght_hash_table_t *name2bone;
	ght_hash_table_t *name2morph;
	VECTOR3 aa_bb_max;
	VECTOR3 aa_bb_min;
	VECTOR3 position;
	QUATERNION rotation;
	int visible;
	struct _APPLICATION *application;
} ASSET_MODEL;

extern int LoadAssetModel(ASSET_MODEL* model, uint8* data, size_t data_size, const char* file_type, const char* model_path);

extern void AssetModelSetWorldPositionInternal(ASSET_MODEL* model, const float* translation);

extern void AssetModelSetWorldRotationInternal(ASSET_MODEL* model, const float* rotation);

extern void InitializeAssetVertex(
	ASSET_VERTEX* vertex,
	ASSET_MODEL* model,
	const float origin[3],
	const float normal[3],
	const float tex_coord[3],
	int index
);

extern void InitializeAssetRootBone(
	ASSET_ROOT_BONE* bone,
	ASSET_MODEL* model,
	void* application_context
);

extern void InitializeAssetScaleBone(
	ASSET_SCALE_BONE* bone,
	ASSET_MODEL* model,
	void* application_context
);

extern void InitializeAssetMaterial(
	ASSET_MATERIAL* material,
	ASSET_MODEL* model,
	struct aiMaterial* material_ref,
	int num_indices,
	int index,
	void* application_context
);

extern void InitializeAssetOpacityMorph(
	ASSET_OPACITY_MORPH* morph,
	ASSET_MODEL* model,
	void* application_context
);

#endif	// #ifndef _INCLUDED_ASSET_MODEL_H_
