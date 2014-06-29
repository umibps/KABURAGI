#include <string.h>
#include "texture.h"

#ifdef __cplusplus
extern "C" {
#endif

void BaseTextureMake(BASE_TEXTURE* texture)
{
	if(texture->interface_data.name == 0)
	{
		glGenTextures(1, &texture->interface_data.name);
	}
}

void BaseTextureBind(BASE_TEXTURE* texture)
{
	glBindTexture(texture->format.target, texture->interface_data.name);
}

void BaseTextureUnbind(BASE_TEXTURE* texture)
{
	glBindTexture(texture->format.target, 0);
}

void BaseTextureRelease(BASE_TEXTURE* texture)
{
	if(texture->interface_data.name != 0)
	{
		glDeleteTextures(1, &texture->interface_data.name);
	}
}

void GenerateTexture2D(TEXTURE_2D* texture)
{
	glTexImage2D(texture->base_data.format.target, 0, texture->base_data.format.internal,
		texture->base_data.size[0], texture->base_data.size[1], 0, texture->base_data.format.external,
			texture->base_data.format.type, NULL);
}

void InitializeTexture2D(TEXTURE_2D* texture, TEXTURE_FORMAT* format, int* size, GLuint sampler)
{
	(void)memset(texture, 0, sizeof(*texture));
	texture->base_data.format.target = GL_TEXTURE_2D;
	texture->base_data.format = *format;
	texture->base_data.size[0] = size[0];
	texture->base_data.size[1] = size[1];
	texture->base_data.size[2] = size[2];
	texture->base_data.sampler = sampler;

	texture->base_data.generate = (void (*)(void*))GenerateTexture2D;
}

#ifdef __cplusplus
}
#endif
