#ifndef _INCLUDED_EFFECT_ENGINE_H_
#define _INCLUDED_EFFECT_ENGINE_H_

#include "parameter.h"
#include "technique.h"
#include "effect.h"

typedef struct _RECTANGLE_RENDER_ENGINE
{
	GLuint vertex_bundle;
	GLuint vertices_buffer;
	GLuint indices_buffer;
} RECTAGNLE_RENDER_ENGINE;
/*
typedef struct _EFFECT_ENGINE
{
	EFFECT *effect;
	EFFECT *default_effect;
	struct _APPLICATION *context;
	RECTAGNLE_RENDER_ENGINE *rectangle_render;
	TECHNIQUES techniques;
	TECHNIQUES default_techniques;
} EFFECT_ENGINE;
*/
#endif	// #ifndef _INCLUDED_EFFECT_ENGINE_H_
