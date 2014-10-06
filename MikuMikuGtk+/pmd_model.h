#ifndef _INCLUDED_PMD_MODEL_H_
#define _INCLUDED_PMD_MODEL_H_

#include <stdio.h>
#include "ght_hash_table.h"
#include "types.h"
#include "utils.h"
#include "memory_stream.h"
#include "vertex.h"
#include "bone.h"
#include "material.h"
#include "model_label.h"
#include "morph.h"
#include "rigid_body.h"
#include "joint.h"
#include "ik.h"
#include "model.h"

#define PMD_MODEL_NAME_SIZE 20
#define PMD_MODEL_COMMEMT_SIZE 256
#define PMD_MODEL_CUSTOM_TOON_TEXTURE_NAME_SIZE 100
#define PMD_MODEL_MAX_CUSTOM_TOON_TEXTURES 10

#define PMD_MODEL_DEFAULT_BUFFER_SIZE 32

typedef struct _PMD_DATA_INFO
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
	uint8 *ik_constraints;
	size_t ik_constraints_count;
	uint8 *bone_category_names;
	size_t bone_category_names_count;
	uint8 *bone_labels;
	size_t bone_labels_count;
	uint8 *morph_labels;
	size_t morph_labels_count;
	uint8 *bone_names;
	uint8 *english_bone_names;
	uint8 *english_face_names;
	uint8 *english_bone_frames;
	uint8 *custom_toon_texture_names;
	uint8 *end;
} PMD_DATA_INFO;

typedef struct _PMD_MODEL
{
	MODEL_INTERFACE interface_data;
	STRUCT_ARRAY *vertices;
	PMD_DATA_INFO info;
} PMD_MODEL;

typedef enum _ePMD2_MODEL_FLAGS
{
	PMD2_MODEL_FLAG_HAS_ENGLISH = 0x01,
	PMD2_MODEL_FLAG_VISIBLE = 0x02,
	PMD2_MODEL_FLAG_ENABLE_PHYSICS = 0x04
} ePMD2_MODEL_FLAGS;

typedef struct _PMD2_MODEL
{
	MODEL_INTERFACE interface_data;
	STRUCT_ARRAY *vertices;
	UINT32_ARRAY *indices;
	ght_hash_table_t *textures;
	STRUCT_ARRAY *materials;
	STRUCT_ARRAY *bones;
	STRUCT_ARRAY *morphs;
	STRUCT_ARRAY *labels;
	STRUCT_ARRAY *rigid_bodies;
	STRUCT_ARRAY *joints;
	STRUCT_ARRAY *constraints;
	STRUCT_ARRAY *raw_ik_constraints;
	POINTER_ARRAY *custom_toon_textures;
	POINTER_ARRAY *sorted_bones;
	ght_hash_table_t *name2bone;
	ght_hash_table_t *name2morph;
	VECTOR3 position;
	QUATERNION rotation;
	float edge_color[4];
	VECTOR3 aa_bb_max;
	VECTOR3 aa_bb_min;
	unsigned int flags;
	PMD_DATA_INFO info;
} PMD2_MODEL;

typedef struct _PMD2_DEFAULT_STATIC_BUFFER_UNIT
{
	VECTOR3 tex_coord;
	VECTOR4 bone_indices;
	VECTOR4 bone_weights;
} PMD2_DEFAULT_STATIC_BUFFER_UNIT;

typedef struct _PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT
{
	VECTOR4 position;
	VECTOR4 normal;
	VECTOR4 delta;
	VECTOR4 edge;
} PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT;

typedef struct _PMD2_DEFAULT_BUFFER_IDENT
{
	uint16 index_buffer_ident;
	PMD2_DEFAULT_STATIC_BUFFER_UNIT static_buffer_ident;
	PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT dynamic_buffer_ident;
} PMD2_DEFAULT_BUFFER_IDENT;

typedef struct _PMD2_DEFAULT_STATIC_BUFFER
{
	MODEL_STATIC_VERTEX_BUFFER_INTERFACE interface_data;
	PMD2_MODEL *model;
} PMD2_DEFAULT_STATIC_BUFFER;

typedef struct _PMD2_DEFAULT_DYNAMIC_BUFFER
{
	MODEL_DYNAMIC_VERTEX_BUFFER_INTERFACE interface_data;
	PMD2_MODEL *model;
	struct _PMD2_DEFAULT_INDEX_BUFFER *index_buffer;
} PMD2_DEFAULT_DYNAMIC_BUFFER;

typedef struct _PMD2_DEFAULT_INDEX_BUFFER
{
	MODEL_INDEX_BUFFER_INTERFACE interface_data;
	PMD2_MODEL *model;
	WORD_ARRAY *indices;
} PMD2_DEFAULT_INDEX_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif

extern int LoadPmd2Model(PMD2_MODEL* model, uint8* data, size_t data_size);

extern int ReadPmd2ModelDataAndState(
	void *scene,
	PMD2_MODEL* model,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int)
);

extern size_t WritePmd2ModelDataAndState(
	PMD2_MODEL* model,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
);

extern void Pmd2ModelPerformUpdate(PMD2_MODEL* model, int force_sync);

extern void Pmd2ModelSolveInverseKinematics(PMD2_MODEL* model);

extern void Pmd2ModelJoinWorld(PMD2_MODEL* model, void* world);

extern void Pmd2ModelResetMotionState(PMD2_MODEL* model, void* world);

extern void Pmd2ModelGetDefaultStaticVertexBuffer(PMD2_MODEL* model, PMD2_DEFAULT_STATIC_BUFFER** buffer);

extern void Pmd2ModelGetDefaultDynamicVertexBuffer(
	PMD2_MODEL* model,
	PMD2_DEFAULT_DYNAMIC_BUFFER** buffer,
	PMD2_DEFAULT_INDEX_BUFFER* index_buffer
);

extern void Pmd2ModelGetDefaultIndexBuffer(PMD2_MODEL* model, PMD2_DEFAULT_INDEX_BUFFER** buffer);

extern void InitializePmd2Vertex(
	PMD2_VERTEX* vertex, 
	PMD2_MODEL* model,
	MATERIAL_INTERFACE* default_material
);

extern int Pmd2VertexPreparse(
	MEMORY_STREAM_PTR stream,
	size_t* data_size,
	PMD_DATA_INFO* info
);

extern void ReadPmd2Vertex(
	PMD2_VERTEX* vertex,
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info,
	size_t* data_size
);

extern void InitializePmd2Bone(PMD2_BONE* bone, MODEL_INTERFACE* model, void* application_context);

extern int Pmd2BonePreparse(
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info
);

extern void ReadPmd2Bone(
	PMD2_BONE* bone,
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info,
	size_t* data_size
);

extern void InitializePmd2Material(
	PMD2_MATERIAL* material,
	PMD2_MODEL* model,
	void* application_context
);

extern int Pmd2MaterialPreparse(
	MEMORY_STREAM_PTR stream,
	size_t* data_size,
	PMD_DATA_INFO* info
);

extern void InitializePmd2Morph(
	PMD2_MORPH* morph,
	PMD2_MODEL* model,
	void* application_context
);

extern int Pmd2MorphPreparse(
	MEMORY_STREAM_PTR stream,
	size_t* data_size,
	PMD_DATA_INFO* info
);

extern void InitializePmd2ModelLabel(
	PMD2_MODEL_LABEL* label,
	PMD2_MODEL* model,
	const char* name,
	ePMD2_MODEL_LABEL_TYPE type,
	void* application_context
);

extern int Pmd2ModelLabelPreparse(
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info
);

extern void InitializePmd2RigidBody(PMD2_RIGID_BODY* body, PMD2_MODEL* model, void* application_context);

extern int Pmd2RigidBodyPreparse(
	MEMORY_STREAM_PTR stream,
	size_t* data_size,
	PMD_DATA_INFO* info
);

extern void ReadPmd2RigidBody(
	PMD2_RIGID_BODY* body,
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info,
	size_t* data_size
);

extern void InitializePmd2Joint(PMD2_JOINT* joint, PMD2_MODEL* model);

extern int Pmd2JointPreparse(MEMORY_STREAM_PTR stream, PMD_DATA_INFO* info);

extern void ReadPmd2Joint(PMD2_JOINT* joint, MEMORY_STREAM_PTR stream, size_t* data_size, PMD_DATA_INFO* info);

extern void Pmd2ModelAddTexture(PMD2_MODEL* model, const char* texture);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_PMD_MODEL_H_
