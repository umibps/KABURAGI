#ifndef _INCLUDED_PMX_MODEL_H_
#define _INCLUDED_PMX_MODEL_H_

#include "types.h"
#include "utils.h"
#include "bone.h"
#include "material.h"
#include "morph.h"
#include "model_label.h"
#include "rigid_body.h"
#include "joint.h"
#include "soft_body.h"
#include "ght_hash_table.h"
#include "text_encode.h"
#include "vertex.h"
#include "model.h"

typedef struct _PMX_DEFAULT_STATIC_BUFFER_UNIT
{
	float texture_coord[3];
	float bone_indices[4];
	float bone_weights[4];
} PMX_DEFAULT_STATIC_BUFFER_UNIT;

typedef struct _PMX_DEFAULT_DYNAMIC_BUFFER_UNIT
{
	float position[3];
	float normal[4];
	float delta[3];
	float edge[4];
	float uva1[4];
	float uva2[4];
	float uva3[4];
	float uva4[4];
} PMX_DEFAULT_DYNAMIC_BUFFER_UNIT;

typedef struct _PMX_DEFAULT_BUFFER_IDENT
{
	int index_buffer_ident;
	PMX_DEFAULT_STATIC_BUFFER_UNIT static_buffer_ident;
	PMX_DEFAULT_DYNAMIC_BUFFER_UNIT dynamic_buffer_ident;
} PMX_DEFAULT_BUFFER_IDENT;

typedef struct _PMX_DEFAULT_STATIC_VERTEX_BUFFER
{
	MODEL_STATIC_VERTEX_BUFFER_INTERFACE interface_data;
	float texture_coord[3];
	float bone_indices[4];
	float bone_weights[4];
	struct _PMX_MODEL *model;
	POINTER_ARRAY *bone_index_hashes;
} PMX_DEFAULT_STATIC_VERTEX_BUFFER;

typedef struct _PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER
{
	MODEL_DYNAMIC_VERTEX_BUFFER_INTERFACE interface_data;
	float position[3];
	float normal[3];
	float delta[3];
	float edge[3];
	float uva1[4];
	float uva2[4];
	float uva3[4];
	float uva4[4];
	struct _PMX_MODEL *model;
	MODEL_INDEX_BUFFER_INTERFACE *index_buffer;
	int enable_parallel_update;
} PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER;

typedef struct _PMX_DEFAULT_INDEX_BUFFER
{
	MODEL_INDEX_BUFFER_INTERFACE interface_data;
	eMODEL_INDEX_BUFFER_TYPE index_type;
	union
	{
		int32 *indices32;
		int16 *indices16;
		int8 *indices8;
	} indices;
	int num_indices;
	struct _PMX_MODEL *model;
} PMX_DEFAULT_INDEX_BUFFER;

typedef struct _PMX_SKINNING_MESHES
{
	POINTER_ARRAY *bones;
	POINTER_ARRAY *matrices;
} PMX_SKKINNING_MESHES;

typedef struct _PMX_DEFAULT_MATRIX_BUFFER
{
	MODEL_MATRIX_BUFFER_INTERFACE interface_data;
	ght_hash_table_t *bones;
	POINTER_ARRAY *matrices;
	struct _PMX_MODEL *model;
	PMX_DEFAULT_INDEX_BUFFER *index_buffer;
	PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER *dynamic_buffer;
	PMX_SKKINNING_MESHES meshes;
} PMX_DEFAULT_MATRIX_BUFFER;

typedef enum _ePMX_STRIDE_TYPE
{
	PMX_STRIDE_TYPE_VERTEX,
	PMX_STRIDE_TYPE_NORMAL,
	PMX_STRIDE_TYPE_TEXTURE_COORD,
	PMX_STRIDE_TYPE_EDGE_SIZE,
	PMX_STRIDE_TYPE_TOON_COORD,
	PMX_STRIDE_TYPE_EDGE_VERTEX,
	PMX_STRIDE_TYPE_VERTEX_INDEX,
	PMX_STRIDE_TYPE_BONE_INDEX,
	PMX_STRIDE_TYPE_WEIGHT,
	PMX_STRIDE_TYPE_UVA1,
	PMX_STRIDE_TYPE_UVA2,
	PMX_STRIDE_TYPE_UVA3,
	PMX_STRIDE_TYPE_UVA4,
	PMX_STRIDE_TYPE_INDEX
} ePMX_STRIDE_TYPE;

typedef enum _ePMX_FLAGS
{
	PMX_FLAG_VISIBLE = 0x01,
	PMX_FLAG_ENABLE_PHYSICS = 0x02
} ePMX_FLAGS;

typedef struct _PMX_DATA_INFO
{
	TEXT_ENCODE *encoding;
	eTEXT_TYPE codec;
	float version;
	uint8 *base;
	uint8 *name;
	size_t additional_uv_size;
	size_t vertex_index_size;
	size_t texture_index_size;
	size_t material_index_size;
	size_t bone_index_size;
	size_t morph_index_size;
	size_t rigid_body_index_size;
	int name_size;
	uint8 *english_name;
	int english_name_size;
	uint8 *comment;
	int comment_size;
	uint8 *english_comment;
	int english_comment_size;
	uint8 *vertices;
	size_t vertices_count;
	uint8 *indices;
	size_t indices_count;
	uint8 *textures;
	size_t textures_count;
	uint8 *materials;
	size_t materials_count;
	uint8 *bones;
	size_t bones_count;
	uint8 *morphs;
	size_t morphs_count;
	uint8 *labels;
	size_t labels_count;
	uint8 *rigid_bodies;
	size_t rigid_bodies_count;
	uint8 *joints;
	size_t joints_count;
	uint8 *soft_bodies;
	size_t soft_bodies_count;
	uint8 *end;
} PMX_DATA_INFO;

typedef struct _PMX_MODEL
{
	MODEL_INTERFACE interface_data;
	TEXT_ENCODE *encoding;
	struct _PMX_MODEL *self;
	struct _SCENE *parent_scene;
	STRUCT_ARRAY *vertices;
	UINT32_ARRAY *indices;
	POINTER_ARRAY *textures;
	ght_hash_table_t *name2texture;
	ght_hash_table_t *texture_indices;
	STRUCT_ARRAY *materials;
	STRUCT_ARRAY *bones;
	POINTER_ARRAY *bones_before_physics;
	POINTER_ARRAY *bones_after_physics;
	STRUCT_ARRAY *morphs;
	STRUCT_ARRAY *labels;
	STRUCT_ARRAY *rigid_bodies;
	STRUCT_ARRAY *joints;
	STRUCT_ARRAY *soft_bodies;
	ght_hash_table_t *name2bone;
	ght_hash_table_t *name2morph;
	uint8 edge_color[3];
	float aa_bb_max[3];
	float aa_bb_min[3];
	float position[3];
	QUATERNION rotation;
	PMX_DATA_INFO data_info;

	unsigned int flags;
} PMX_MODEL;

#ifdef __cplusplus
extern "C" {
#endif

extern int LoadPmxModel(PMX_MODEL* model, uint8* data, size_t data_size);

extern void ReadPmxModelDataAndState(
	void *scene,
	PMX_MODEL* model,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int)
);

extern size_t WritePmxModelDataAndState(
	PMX_MODEL* model,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
);

extern void InitializePmxVertex(PMX_VERTEX* vertex, PMX_MODEL* model);

extern void ReleasePmxModel(PMX_MODEL* model);

extern int PmxVertexPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
);

extern void PmxVertexRead(
	PMX_VERTEX* vertex,
	uint8* data,
	PMX_DATA_INFO* info,
	size_t* data_size
);

extern void PmxVertexMergeMorphByUV(PMX_VERTEX* vertex, MORPH_UV* morph, FLOAT_T weight);

extern void PmxVertexMergeMorphByVertex(PMX_VERTEX* vertex, MORPH_VERTEX* morph, FLOAT_T wiehgt);

extern int PmxMaterialPreparse(
	uint8 *data,
	size_t *data_size,
	size_t rest,
	PMX_DATA_INFO* info
);

extern void PmxMaterialRead(
	PMX_MATERIAL* material,
	uint8* data,
	PMX_DATA_INFO* info,
	size_t* data_size
);

extern void PmxMaterialMergeMorph(PMX_MATERIAL* material, MORPH_MATERIAL* morph, FLOAT_T weight);

extern void InitializePmxBone(PMX_BONE* bone, PMX_MODEL* model);

extern int PmxBonePreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
);

extern void ReadPmxBone(
	PMX_BONE* bone,
	uint8* data,
	PMX_DATA_INFO* info,
	size_t* data_size
);

extern void PmxBoneMergeMorph(PMX_BONE* bone, MORPH_BONE* morph, const FLOAT_T weight);

extern void InitializePmxMorph(PMX_MORPH* morph, PMX_MODEL* model);

extern int PmxMorphPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
);

extern void PmxMorphRead(
	PMX_MORPH* morph,
	uint8* data,
	size_t* data_size,
	PMX_DATA_INFO* info
);

extern void InitializePmxModelLabel(PMX_MODEL_LABEL* label, PMX_MODEL* model);

extern int PmxModelLabelPreparse(uint8* data, size_t* data_size, size_t rest, PMX_DATA_INFO* info);

extern void PmxModelLabelRead(
	PMX_MODEL_LABEL* label,
	uint8* data,
	size_t* data_size,
	PMX_DATA_INFO* info
);

extern void InitializePmxRigidBody(PMX_RIGID_BODY* body, PMX_MODEL* model, void* application_context);

extern int PmxRigidBodyPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
);

extern void PmxRigidBodyRead(
	PMX_RIGID_BODY* body,
	uint8* data,
	PMX_DATA_INFO* info,
	size_t* data_size
);

extern void PmxRigidBodyMergeMorph(PMX_RIGID_BODY* body, MORPH_IMPULSE* impulse, FLOAT_T weight);

extern void InitializePmxJoint(PMX_JOINT* joint, PMX_MODEL* model);

extern int PmxJointPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
);

extern void PmxJointRead(PMX_JOINT* joint, uint8* data, PMX_DATA_INFO* info, size_t* data_size);

extern void InitializePmxSoftBody(PMX_SOFT_BODY* body, PMX_MODEL* model);

extern int PmxSoftBodyPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
);

extern void PmxSoftBodyRead(
	PMX_SOFT_BODY* body,
	uint8* data,
	size_t* data_size,
	PMX_DATA_INFO* info
);

extern void PmxModelGetIndexBuffer(PMX_MODEL* model, PMX_DEFAULT_INDEX_BUFFER** buffer);
extern void PmxModelGetStaticVertexBuffer(PMX_MODEL* model, PMX_DEFAULT_STATIC_VERTEX_BUFFER** buffer);
extern void PmxModelGetDynamicVertexBuffer(
	PMX_MODEL* model,
	PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER** buffer,
	PMX_DEFAULT_INDEX_BUFFER* index_buffer
);
extern void PmxModelGetMatrixBuffer(
	PMX_MODEL* model,
	PMX_DEFAULT_MATRIX_BUFFER** buffer,
	PMX_DEFAULT_DYNAMIC_VERTEX_BUFFER* dynamic_buffer,
	PMX_DEFAULT_INDEX_BUFFER* index_buffer
);

extern PMX_DEFAULT_STATIC_VERTEX_BUFFER* PmxDefaultStaticVertexBufferNew(PMX_MODEL* model);

extern void DeletePmxDefaultStaticVertexBuffer(PMX_DEFAULT_STATIC_VERTEX_BUFFER* buffer);

extern void PmxModelJoinWorld(PMX_MODEL* model, void* world);

extern void PmxModelLeaveWorld(PMX_MODEL* model, void* world);

extern void PmxModelPerformUpdate(PMX_MODEL* model, int force_sync);

extern void PmxModelResetMotionState(PMX_MODEL* model, void* world);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_PMX_MODEL_H_
