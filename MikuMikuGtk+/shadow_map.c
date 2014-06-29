#include <string.h>
#include "shadow_map.h"
#include "utils.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void* ShadowMapGetTexture(SHADOW_MAP* shadow_map)
{
	return (void*)shadow_map->color_texture_ptr;
}

void ShadowMapGetSize(SHADOW_MAP* shadow_map, float* size)
{
	COPY_VECTOR3(size, shadow_map->size);
}

void InitializeShadowMap(
	SHADOW_MAP* shadow_map,
	int width,
	int height,
	void* scene
)
{
	(void)memset(shadow_map, 0, sizeof(*shadow_map));
	shadow_map->size[0] = (float)width;
	shadow_map->size[1] = (float)height;
	shadow_map->size[2] = 1.0f;

	shadow_map->color_texture_ptr = &shadow_map->color_texture;

	shadow_map->distance = 7.5f;
	shadow_map->scene = scene;

	shadow_map->interface_data.get_texture =
		(void* (*)(void*))ShadowMapGetTexture;
	shadow_map->interface_data.get_size =
		(void (*)(void*, float*))ShadowMapGetSize;
}

SHADOW_MAP_INTERFACE* ShadowMapNew(
	int width,
	int height,
	void* scene
)
{
	SHADOW_MAP *ret = (SHADOW_MAP*)MEM_ALLOC_FUNC(sizeof(*ret));
	InitializeShadowMap(ret, width, height, scene);

	return (SHADOW_MAP_INTERFACE*)ret;
}

void MakeShadowMap(
	SHADOW_MAP* shadow_map
)
{
	GLsizei width, height;
	width = (GLsizei)shadow_map->size[0], height = (GLsizei)shadow_map->size[1];
	ReleaseShadowMap(shadow_map);
	glGenFramebuffers(1, &shadow_map->frame_buffer);
	glGenTextures(1, &shadow_map->color_texture);
	glBindTexture(GL_TEXTURE_2D, shadow_map->color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenRenderbuffers(1, &shadow_map->depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, shadow_map->depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map->frame_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, shadow_map->color_texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, shadow_map->depth_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ReleaseShadowMap(
	SHADOW_MAP* shadow_map
)
{
	shadow_map->motion = NULL;
	glDeleteFramebuffers(1, &shadow_map->frame_buffer);
	shadow_map->frame_buffer = 0;
	glDeleteTextures(1, &shadow_map->color_texture);
	shadow_map->color_texture = 0;
	glDeleteRenderbuffers(1, &shadow_map->depth_buffer);
	shadow_map->depth_buffer = 0;
}

void ResizeShadowMap(
	SHADOW_MAP* shadow_map,
	int width,
	int height
)
{
	shadow_map->size[0] = (float)width;
	shadow_map->size[1] = (float)height;
}

void ResetShadowMap(SHADOW_MAP* shadow_map)
{
	shadow_map->position[0] = 0.0f;
	shadow_map->position[1] = 0.0f;
	shadow_map->position[2] = 0.0f;

	shadow_map->distance = 7.5f;
}

#ifdef __cplusplus
}
#endif
