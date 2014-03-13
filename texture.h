#ifndef _INCLUDED_TEXTURE_H_
#define _INCLUDED_TEXTURE_H_

#include <gtk/gtk.h>
#include "types.h"

#define TEXTURE_ICON_SIZE 32

typedef struct _TEXTURE
{
	uint8 *pixels;
	char *name;
	gint32 width, height, stride;
	GdkPixbuf *thumbnail;
} TEXTURE;

typedef struct _TEXTURES
{
	TEXTURE *texture;
	int num_texture;
	int active_texture;
	FLOAT_T strength;
	FLOAT_T scale;
	FLOAT_T angle;
} TEXTURES;

/*****************************************************
* LoadTexture関数                                    *
* テクスチャをロードする                             *
* 引数                                               *
* textures	: テクスチャを管理する構造体のアドレス   *
* directly	: テクスチャ画像のあるディレクトリのパス *
*****************************************************/
extern void LoadTexture(TEXTURES* textures, char* directly);

#endif	// #ifndef _INCLUDED_TEXTURE_H_
