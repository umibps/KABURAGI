#ifndef _INCLUDED_MODEL_H_
#define _INCLUDED_MODEL_H_

#include <stdio.h>
#include <assimp/scene.h>
#include "types.h"
#include "text_encode.h"

typedef enum _eMODEL_TYPE
{
	MODEL_TYPE_UNKNOWN = -1,
	MODEL_TYPE_ASSET_MODEL,
	MODEL_TYPE_PMD_MODEL,
	MODEL_TYPE_PMX_MODEL,
	MAX_MODEL_TYPE
} eMODEL_TYPE;

typedef struct _MODEL_INTERFACE
{
	eMODEL_TYPE type;
	char *name;
	char *english_name;
	char *comment;
	char *english_comment;
	char *model_path;
	float opacity;
	float scale_factor;
	FLOAT_T edge_width;
	struct _MODEL_INTERFACE *parent_model;
	struct _BONE_INTERFACE *parent_bone;
	void* (*find_bone)(void*, const char*);
	void* (*find_morph)(void*, const char*);
	void (*get_index_buffer)(void*, void**);
	void (*get_static_vertex_buffer)(void*, void**);
	void (*get_dynanic_vertex_buffer)(void*, void**, void*);
	void (*get_matrix_buffer)(void*, void**, void*, void*);
	void** (*get_bones)(void*, int*);
	void** (*get_morphs)(void*, int*);
	void** (*get_materials)(void*, int*);
	void** (*get_rigid_bodies)(void*, int*);
	void** (*get_labels)(void*, int*);
	void (*get_world_position)(void*, float*);
	void (*get_world_translation)(void*, float*);
	void (*get_world_orientation)(void*, float*);
	FLOAT_T (*get_edge_scale_factor)(void*, float*);
	int (*is_visible)(void*);
	void (*set_world_position)(void*, float*);
	void (*set_world_orientation)(void*, float*);
	void (*set_visible)(void*, int);
	void (*set_opacity)(void*, float);
	void (*set_aa_bb)(void*, float*, float*);
	void (*perform_update)(void*, int);
	void (*reset_motion_state)(void*, void*);
	void (*set_enable_physics)(void*, int);
	void* (*rigid_body_transform_new)(struct _BASE_RIGID_BODY*);
	struct _SCENE *scene;
} MODEL_INTERFACE;

typedef enum _eMODEL_STRIDE_TYPE
{
	MODEL_VERTEX_STRIDE,
	MODEL_NORMAL_STRIDE,
	MODEL_TEXTURE_COORD_STRIDE,
	MODEL_MORPH_DELTA_STRIDE,
	MODEL_EDGE_SIZE_STRIDE,
	MODEL_EDGE_VERTEX_STRIDE,
	MODEL_VERTEX_INDEX_STRIDE,
	MODEL_BONE_INDEX_STRIDE,
	MODEL_BONE_WEIGHT_STRIDE,
	MODEL_UVA1_STRIDE,
	MODEL_UVA2_STRIDE,
	MODEL_UVA3_STRIDE,
	MODEL_UVA4_STRIDE,
	MODEL_INDEX_STRIDE,
	MAX_MODEL_STRIDE_TYPE
} eMODEL_STRIDE_TYPE;

typedef struct _MODEL_BUFFER_INTERFACE
{
	size_t (*get_size)(void*);
	size_t (*get_stride_offset)(void*, eMODEL_STRIDE_TYPE);
	size_t (*get_stride_size)(void*);
	void* (*ident)(void*);
	void (*delete_func)(void*);
} MODEL_BUFFER_INTERFACE;

/*
typedef struct _MODEL_BUFFER_UNIT_INTERFACE
{
	void (*update)(void*, struct _VERTEX_INTERFACE*, FLOAT_T, int, float*);
} MODEL_BUFFER_UNIT_INTERFACE;
*/
typedef struct _MODEL_DYNAMIC_VERTEX_BUFFER_INTERFACE
{
	MODEL_BUFFER_INTERFACE base;
	void (*setup_bind_pose)(void*, void*);
	void (*perform_transform)(void*, void*, const float*, float*, float*);
	void (*set_parallel_update_enable)(void*, int);
} MODEL_DYNAMIC_VERTEX_BUFFER_INTERFACE;

typedef struct _MODEL_STATIC_VERTEX_BUFFER_INTERFACE
{
	MODEL_BUFFER_INTERFACE base;
	void (*update)(void*, void*);
} MODEL_STATIC_VERTEX_BUFFER_INTERFACE;

typedef enum _eMODEL_INDEX_BUFFER_TYPE
{
	MODEL_INDEX_BUFFER_TYPE_INDEX8,
	MODEL_INDEX_BUFFER_TYPE_INDEX16,
	MODEL_INDEX_BUFFER_TYPE_INDEX32,
	MAX_MODEL_INDEX_BUFFER_TYPE
} eMODEL_INDEX_BUFFER_TYPE;

typedef struct _MODEL_INDEX_BUFFER_INTERFACE
{
	MODEL_BUFFER_INTERFACE base;
	void* (*bytes)(void*);
	int (*index_at)(void*, int);
	eMODEL_INDEX_BUFFER_TYPE (*get_type)(void*);
} MODEL_INDEX_BUFFER_INTERFACE;

typedef struct _MODEL_MATRIX_BUFFER_INTERFACE
{
	MODEL_BUFFER_INTERFACE base;
	void (*update)(void*, void*);
	float* (*bytes)(void*, int);
	size_t (*get_size)(void*, int);
} MODEL_MATRIX_BUFFER_INTERFACE;

typedef struct _MODEL_BUFFER
{
	eMODEL_STRIDE_TYPE stride_type;
	size_t size;
	size_t stride_offset;
	size_t stride_size;
	void *ident;
} MODEL_BUFFER;

typedef enum _eMODEL_INDEX_TYPE
{
	MODEL_INDEX8,
	MODEL_INDEX16,
	MODEL_INDEX32,
	MAX_MODEL_INDEX
} eMODEL_INDEX_TYPE;

typedef struct _MODEL_INDEX_BUFFER
{
	MODEL_BUFFER model_buffer;
	eMODEL_INDEX_TYPE index;
} MODEL_INDEX_BUFFER;

typedef struct _MODEL_MATRIX_BUFFER
{
	MODEL_BUFFER model_buffer;
	float *bytes;
	size_t size;
} MODEL_MATRIX_BUFFER;

typedef enum _eMODEL_OBJECT_TYPE
{
	MODEL_OBJECT_TYPE_UNKNOWN = -1,
	MODEL_OBJECT_TYPE_BONE,
	MODEL_OBJECT_TYPE_IK,
	MODEL_OBJECT_TYPE_INDEX,
	MODEL_OBJECT_TYPE_JOINT,
	MODEL_OBJECT_TYPE_MATERIAL,
	MODEL_OBJECT_TYPE_MORPH,
	MODEL_OBJECT_TYPE_RIGID_BODY,
	MODEL_OBJECT_TYPE_SOFT_BODY,
	MODEL_OBJECT_TYPE_TEXTURE,
	MODEL_OBJECT_TYPE_VERTEX,
	MAX_MODEL_OBJECT_TYPE
} eMODEL_OBJECT_TYPE;

typedef enum _eMODEL_FLAGS
{
	MODEL_FLAG_VISIBLE = 0x01
} eMODEL_FLAGS;

typedef struct _MODEL
{
	MODEL_BUFFER *dynamic_vertex_buffer;
	MODEL_BUFFER *static_vertex_buffer;
	MODEL_INDEX_BUFFER *index_buffer;
	MODEL_MATRIX_BUFFER *matrix_buffer;
	struct aiScene *ai_scene;
	eMODEL_OBJECT_TYPE object_type;
	eMODEL_TYPE type;
	float position[3];
	float rotate[3];
	float scale_factor;
	struct _BONE *parent_bone;
	unsigned int flags;
} MODEL;

extern void DeleteModel(MODEL_INTERFACE* model);

extern void ReleaseModelInterface(MODEL_INTERFACE* model);

extern char** GetChildBoneNames(MODEL_INTERFACE* model, void* application_context);

#endif	// #ifndef _INCLUDED_MODEL_H_
