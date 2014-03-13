#ifndef _INCLUDED_DEBUG_DRAWER_H_
#define _INCLUDED_DEBUG_DRAWER_H_

#include <GL/glew.h>
#include "world.h"
#include "shader_program.h"
#include "utils.h"
#include "vertex.h"
#include "bone.h"

typedef struct _DEBUG_VERTEX
{
	float position[3];
	float color[3];
} DEBUG_VERTEX;

typedef struct _DEBUG_DRAWER_PROGRAM
{
	SHADER_PROGRAM base_data;
	GLint model_view_projection_matrix;
} DEBUG_DRAWER_PROGRAM;

typedef enum _eDEBGU_DRAW_FLAGS
{
	DEBUG_DRAW_FLAG_VISIBLE = 0x01
} eDEBGU_DRAW_FLAGS;

typedef struct _DEBUG_DRAWER
{
	DEBUG_DRAWER_PROGRAM program;
	STRUCT_ARRAY *vertices;
	UINT32_ARRAY *indices;
	VERTEX_BUNDLE *bundle;
	VERTEX_BUNDLE_LAYOUT *layout;
	void *drawer;
	unsigned int flags;
	int index;
	void *project_context;
} DEBUG_DRAWER;

extern void InitializeDebugDrawer(DEBUG_DRAWER* drawer, void* project_context);

extern void DebugDrawerDrawWorld(DEBUG_DRAWER* drawer, WORLD* world, unsigned int flags);

extern void DebugDrawerLoad(DEBUG_DRAWER* drawer);

extern void DebugDrawerDrawBones(DEBUG_DRAWER* drawer, MODEL_INTERFACE* model, POINTER_ARRAY* selected_bones);

extern void DebugDrawerDrawBoneTransform(DEBUG_DRAWER* drawer, BONE_INTERFACE* bone, MODEL_INTERFACE* model, int mode);

extern void DebugDrawerDrawMovableBone(DEBUG_DRAWER* drawer, BONE_INTERFACE* bone, MODEL_INTERFACE* model);

#endif	// #ifndef _INCLUDED_DEBUG_DRAWER_H_
