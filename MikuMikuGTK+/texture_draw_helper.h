#ifndef _INCLUDED_TEXTURE_DRAW_HELPER_H_
#define _INCLUDED_TEXTURE_DRAW_HELPER_H_

#include "shader_program.h"
#include "vertex.h"

typedef struct _TEXTURE_DRAW_HELPER
{
	TEXTURE_DRAW_HELPER_PROGRAM program;
	VERTEX_BUNDLE *bundle;
	VERTEX_BUNDLE_LAYOUT *layout;
	int width, height;
	int linked;
} TEXTURE_DRAW_HELPER;

extern void InitializeTextureDrawHelper(TEXTURE_DRAW_HELPER* helper, int width, int height);

extern void ReleaseTextureDrawHelper(TEXTURE_DRAW_HELPER* helper);

extern void TextureDrawHelperDraw(
	TEXTURE_DRAW_HELPER* helper,
	float* rect,
	const float* position,
	GLuint texture_name
);

#endif	// #ifndef _INCLUDED_TEXTURE_DRAW_HELPER_H_
