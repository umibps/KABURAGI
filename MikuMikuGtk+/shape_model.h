#ifndef _INCLUDED_SHAPE_MODEL_H_
#define _INCLUDED_SHAPE_MODEL_H_

#include "types.h"
#include "model.h"
#include "vertex.h"

#define DEFAULT_SHAPE_MODEL_NUM_LINES 4
#define DEFAULT_SHAPE_SIZE 1

typedef struct _SHAPE_VERTEX
{
	VECTOR3 position;
	VECTOR3 normal;
} SHAPE_VERTEX;

typedef enum _eSHAPE_TYPE
{
	SHAPE_TYPE_CUBOID,
	SHAPE_TYPE_CONE
} eSHAPE_TYPE;

typedef struct _SHAPE_MODEL
{
	MODEL_INTERFACE interface_data;
	eSHAPE_TYPE shape_type;
	STRUCT_ARRAY *vertices;
	WORD_ARRAY *indices;
	uint16 num_model_indices;
	uint16 num_edge_indices;
	uint16 num_line_indices;
	VECTOR3 position;
	QUATERNION rotation;
	uint8 surface_color[3];
	uint8 line_color[3];
	uint8 edge_color[3];
	float line_width;
	int num_lines;

	unsigned int flags;
} SHAPE_MODEL;

typedef struct _SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER
{
	MODEL_STATIC_VERTEX_BUFFER_INTERFACE interface_data;
	SHAPE_MODEL *model;
} SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER;

typedef struct _SHAPE_DEFAULT_INDEX_BUFFER
{
	MODEL_INDEX_BUFFER_INTERFACE interface_data;
	SHAPE_MODEL *model;
} SHAPE_DEFAULT_INDEX_BUFFER;

typedef enum _eSHAPE_MODEL_FLAGS
{
	SHAPE_MODEL_FLAG_VISIBLE = 0x01,
	SHAPE_MODEL_FLAG_DRAW_LINE = 0x02,
	SHAPE_MODEL_FLAG_UPDATE = 0x04,
	SHAPE_MODEL_FLAG_RENDER_UPDATE = 0x08,
	SHAPE_MODEL_FLAG_INDEX_BUFFER_UPDATE = 0x10
} eSHAPE_MODEL_FLAGS;

typedef struct _SHAPE_MODEL_DEFAULT_DYNAMIC_BUFFER_UNIT
{
	float position[3];
	float normal[3];
} SHAPE_MODEL_DEFAULT_DYNAMIC_BUFFER_UNIT;

typedef struct _CUBOID_MODEL
{
	SHAPE_MODEL base_data;
	float width, height, depth;
} CUBOID_MODEL;

typedef struct _CONE_MODEL
{
	SHAPE_MODEL base_data;
	float upper_radius, lower_radius;
	float height;
} CONE_MODEL;

typedef union _SHAPE_MODEL_DATA
{
	CUBOID_MODEL cuboid;
	CONE_MODEL cone;
} SHAPE_MODEL_DATA;

#ifdef __cplusplus
extern "C" {
#endif

extern void ReleaseShapeModel(SHAPE_MODEL* model);

extern void ShapeDefaultDynamicVertexBufferPerformTransform(
	SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer,
	void* address,
	float* camera_position
);

extern int ReadShapeModelDataAndState(
	SHAPE_MODEL* model,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	void* scene
);

extern size_t WriteShapeModel(
	SHAPE_MODEL* model,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_SHAPE_MODEL_H_
