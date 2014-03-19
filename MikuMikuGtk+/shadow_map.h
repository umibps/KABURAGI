#ifndef _INCLUDED_SHADOW_MAP_H_
#define _INCLUDED_SHADOW_MAP_H_

#include <stdlib.h>
#include <GL/glew.h>
#include "motion.h"

typedef struct _SHADOW_MAP_INTERFACE
{
	void* (*get_texture)(void*);
	void (*get_size)(void*, float*);
} SHADOW_MAP_INTERFACE;

typedef struct _SHADOW_MAP
{
	SHADOW_MAP_INTERFACE interface_data;
	MOTION_INTERFACE *motion;
	float position[3];
	float size[3];
	GLuint color_texture;
	GLuint frame_buffer;
	GLuint depth_buffer;
	GLuint *color_texture_ptr;
	float distance;

	void *scene;
} SHADOW_MAP;

extern void InitializeShadowMap(
	SHADOW_MAP* shadow_map,
	int width,
	int height,
	void* scene
);

extern void MakeShadowMap(
	SHADOW_MAP* shadow_map
);

extern void ReleaseShadowMap(
	SHADOW_MAP* shadow_map
);

extern void ResizeShadowMap(
	SHADOW_MAP* shadow_map,
	int width,
	int height
);

extern void ResetShadowMap(SHADOW_MAP* shadow_map);

#endif	// #ifndef _INCLUDED_SHADOW_MAP_H_
