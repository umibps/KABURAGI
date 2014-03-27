#ifndef _INCLUDED_VERTEX_H_
#define _INCLUDED_VERTEX_H_

#include <stdlib.h>
#include <GL/glew.h>
#include "ght_hash_table.h"
#include "utils.h"
#include "types.h"

typedef enum _eBASE_VERTEX_TYPE
{
	BASE_VERTEX_TYPE_B_DEF1,
	BASE_VERTEX_TYPE_B_DEF2,
	BASE_VERTEX_TYPE_B_DEF4,
	BASE_VERTEX_TYPE_S_DEF,
	BASE_VERTEX_TYPE_Q_DEF,
	MAX_BASE_VERTEX_TYPE
} eBASE_VERTEX_TYPE;

typedef struct _VERTEX_INTERFACE
{
	void (*set_material)(void*, void*);
	void (*reset)(void*);
	void* (*get_bone)(void*, int);
	void (*get_delta)(void*, float*);
	void (*get_uv)(void*, int, float*);
	FLOAT_T edge_size;
	void (*perform_skinning)(void*, float*, float*);
	void* (*get_material)(void*);
	eBASE_VERTEX_TYPE type;
	int index;
} VERTEX_INTERFACE;

typedef struct _PARALLEL_SKINNING_VERTEX
{
	STRUCT_ARRAY *vertices;
	float aa_bb_min[3];
	float aa_bb_max[3];
	float local_min[3];
	float local_max[3];
	FLOAT_T edge_scale_factor;
	struct _MODEL_BUFFER_UNIT_INTERFACE *unit;
	void (*update)(void*, struct _VERTEX_INTERFACE*, FLOAT_T, int, float*);
	size_t unit_size;
} PARALLEL_SKINNING_VERTEX;

#define PMX_VERTEX_MAX_BONES 4
#define PMX_VERTEX_MAX_MORPHS 5

typedef struct _PMX_VERTEX
{
	VERTEX_INTERFACE interface_data;
	struct _PMX_MODEL *model;
	struct _PMX_BONE *bone[PMX_VERTEX_MAX_BONES];
	struct _PMX_MATERIAL *material;
	float origin_uvs[PMX_VERTEX_MAX_MORPHS][4];
	float morph_uvs[PMX_VERTEX_MAX_MORPHS][4];
	float origin[3];
	float morph_delta[3];
	float normal[3];
	float tex_coord[3];
	float c[3];
	float r0[3];
	float r1[3];
	FLOAT_T weight[PMX_VERTEX_MAX_BONES];
	int bone_indices[PMX_VERTEX_MAX_BONES];
} PMX_VERTEX;

#define PMD_VERTEX_MAX_BONES 2

typedef struct _PMD2_VERTEX
{
	VERTEX_INTERFACE interface_data;
	struct _PMD2_MODEL *model;
	VECTOR3 origin;
	VECTOR3 normal;
	VECTOR3 tex_coord;
	VECTOR3 morph_delta;
	FLOAT_T weight;
	struct _MATERIAL_INTERFACE *material;
	struct _BONE_INTERFACE *bone[PMD_VERTEX_MAX_BONES];
	int bone_indices[PMD_VERTEX_MAX_BONES];
} PMD2_VERTEX;

typedef struct _ASSET_VERTEX
{
	VERTEX_INTERFACE interface_data;
	struct _ASSET_MODEL *model;
	VECTOR3 origin;
	VECTOR3 normal;
	VECTOR3 tex_coord;
} ASSET_VERTEX;

typedef enum _eVERTEX_BUNDLE_BUFFER_TYPE
{
	VERTEX_BUNDLE_VERTEX_BUFFER,
	VERTEX_BUNDLE_INDEX_BUFFER,
	MAX_VERTEX_BUFFER_TYPE
} eVERTEX_BUNDLE_BUFFER_TYPE;

typedef struct _VERTEX_BUNDLE
{
	eVERTEX_BUNDLE_BUFFER_TYPE type;
	ght_hash_table_t *vertex_buffers;
	GLuint index_buffer;
	int has_map_buffer_range;
} VERTEX_BUNDLE;

typedef struct _VERTEX_BUNDLE_LAYOUT
{
	GLuint name;
	int has_extension;
} VERTEX_BUNDLE_LAYOUT;

extern void ResetVertex(void* vertex, int index, void* dummy);

extern int LoadPmx2Vertices(STRUCT_ARRAY* vertices, STRUCT_ARRAY* bones);

extern void Pmd2VertexPerformSkinning(PMD2_VERTEX* vertex, float* position, float* normal);

extern VERTEX_BUNDLE* VertexBundleNew(void);

extern void DeleteVertexBundle(VERTEX_BUNDLE** bundle);

extern void VertexBundleBind(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	GLuint key
);

extern void MakeVertexBundle(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	GLuint key,
	GLenum usage,
	const void *pointer,
	size_t size
);

void VertexBundleRelease(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	GLuint key
);

extern void VertexBundleUnbind(
	eVERTEX_BUNDLE_BUFFER_TYPE type
);

extern void* VertexBundleMap(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	size_t offset,
	size_t size
);

extern void VertexBundleUnmap(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	void* address
);

extern void VertexBundleAllocate(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	GLuint usage,
	size_t size,
	const void *data
);

extern void VertexBundleWrite(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	size_t offset,
	size_t size,
	void* data
);

extern VERTEX_BUNDLE_LAYOUT* VertexBundleLayoutNew(void);

extern void DeleteVertexBundleLayout(VERTEX_BUNDLE_LAYOUT** layout);

extern int MakeVertexBundleLayout(VERTEX_BUNDLE_LAYOUT* layout);

extern int VertexBundleLayoutBind(
	VERTEX_BUNDLE_LAYOUT* layout
);

extern int VertexBundleLayoutUnbind(
	VERTEX_BUNDLE_LAYOUT* layout
);

extern int PmxVerticesLoad(STRUCT_ARRAY* vertices, STRUCT_ARRAY* bones);

#endif	// #ifndef _INCLUDED_VERTEX_H_
