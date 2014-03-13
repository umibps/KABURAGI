#ifndef _INCLUDED_TEXTURE_H_
#define _INCLUDED_TEXTURE_H_

#include <GL/glew.h>

typedef struct _TEXTURE_FORMAT
{
	GLenum external;
	GLenum internal;
	GLenum type;
	GLenum target;
} TEXTURE_FORMAT;

typedef enum _eTEXTURE_FLAGS
{
	TEXTURE_FLAG_TEXTURE_2D = 0x01,
	TEXTURE_FLAG_TOON_TEXTURE = 0x02,
	TEXTURE_FLAG_TEXTURE_3D = 0x04,
	TEXTURE_FLAG_TEXTURE_CUBE = 0x08,
	TEXTURE_FLAG_GENERATE_TEXTURE_MIPMAP = 0x10,
	TEXTURE_FLAG_SYSTEM_TOON_TEXTURE = 0x20,
	TEXTURE_FLAG_ASYNC_LOADING_TEXTURE = 0x40,
} eTEXTURE_FLAGS;

typedef struct _TEXTURE_INTERFACE
{
	GLuint name;
} TEXTURE_INTERFACE;

typedef struct _TEXTURE_DATA_BRIDGE
{
	TEXTURE_INTERFACE *texture;
	unsigned int flags;
} TEXTURE_DATA_BRIDGE;

typedef struct _BASE_TEXTURE
{
	TEXTURE_INTERFACE interface_data;
	void (*generate)(void*);
	TEXTURE_FORMAT format;
	int size[3];
	GLuint sampler;
} BASE_TEXTURE;

typedef struct _TEXTURE_2D
{
	BASE_TEXTURE base_data;
} TEXTURE_2D;

extern void BaseTextureMake(BASE_TEXTURE* texture);
extern void BaseTextureBind(BASE_TEXTURE* texture);
extern void BaseTextureUnbind(BASE_TEXTURE* texture);
extern void BaseTextureRelease(BASE_TEXTURE* texture);

extern void InitializeTexture2D(TEXTURE_2D* texture, TEXTURE_FORMAT* format, int* size, GLuint sampler);

#endif	// #ifndef _INCLUDED_TEXTURE_H_
