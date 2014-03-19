#ifndef _INCLUDED_GRID_H_
#define _INCLUDED_GRID_H_

#include "shader_program.h"
#include "types.h"
#include "vertex.h"
#include "utils.h"

typedef enum _eGRID_VERTEX_TYPE
{
	GRID_VERTEX_TYPE_POSITION,
	GRID_VERTEX_TYPE_COLOR
} eGRID_VERTEX_TYPE;

typedef enum _eGRID_FLAGS
{
	GRID_FLAG_VISIBLE = 0x01
} eGRID_FLAGS;

typedef struct _GRID_PROGRAM
{
	SHADER_PROGRAM base_data;
	GLint model_view_projection_matrix;
} GRID_PROGRAM;

typedef struct _GRID
{
	GRID_PROGRAM program;
	VERTEX_BUNDLE *bundle;
	VERTEX_BUNDLE_LAYOUT *layout;
	STRUCT_ARRAY *vertices;
	UINT32_ARRAY *indices;
	float size[4];
	uint8 line_color[3];
	uint8 axis_x_color[3];
	uint8 axis_y_color[3];
	uint8 axis_z_color[3];
	int num_indices;

	unsigned int flags;
} GRID;

extern void InitializeGrid(GRID* grid);

extern void DrawGrid(GRID* grid, float* matrix);

extern void GridAddLine(
	GRID* grid,
	const float* from,
	const float* to,
	const uint8* color,
	int* index
);

extern void LoadGrid(GRID* grid, void* project_context);

extern void GridBindVertexBundle(GRID* grid, int bundle);

extern void GridReleaseVertexBundle(GRID* grid, int bundle);

#endif	// #ifndef _INCLUDED_GRID_H_
