// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "image_read_write.h"
#include "texture.h"
#include "display_filter.h"
#include "utils.h"
#include "memory.h"

#if !defined(USE_QT) || (defined(USE_QT) && USE_QT != 0)
# include "gui/GTK/utils_gtk.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TEXTURE_BLOCK_SIZE 32

void LoadTextureLoop(TEXTURES* textures, char* directly, int* buff_size)
{
	// ディレクトリをオープン
	GDir *dir = g_dir_open(directly, 0, NULL);
	// 下層ディレクトリ
	GDir *child_dir;
	// 下層ディレクトリのパス
	char child_dir_path[4096];
	// ファイル名
	gchar *file_name;

	// ディレクトリオープン失敗
	if(dir == NULL)
	{
		return;
	}

	// ディレクトリのファイルを読み込む
	while((file_name = (gchar*)g_dir_read_name(dir)) != NULL)
	{
		(void)sprintf(child_dir_path, "%s/%s", directly, file_name);
		child_dir = g_dir_open(child_dir_path, 0, NULL);
		if(child_dir != NULL)
		{
			g_dir_close(child_dir);
			LoadTextureLoop(textures, child_dir_path, buff_size);
		}
		else
		{
			// PNGファイル判定
			size_t name_length = strlen(file_name);
			if(StringCompareIgnoreCase(&file_name[name_length-4], ".png") == 0)
			{
				GFile *fp = g_file_new_for_path(child_dir_path);
				GFileInputStream *stream = g_file_read(fp, NULL, NULL);
				uint8 *pixels;
				gint32 width, height, stride;
				int channel;
				char *name;
				size_t i;

				name = file_name;
				for(i=0; i<name_length; i++)
				{
					if(file_name[i] == '/' || file_name[i] == '\\')
					{
						name = &file_name[i+1];
					}
				}
				file_name[name_length-4] = '\0';
				textures->texture[textures->num_texture].name =
					MEM_STRDUP_FUNC(name);

				pixels = ReadPNGStream((void*)stream, (stream_func_t)FileRead, &width,
					&height, &stride);

				if(pixels != NULL)
				{
					textures->texture[textures->num_texture].width = width;
					textures->texture[textures->num_texture].height = height;
					channel = stride / width;
					switch(channel)
					{
					case 1:
						if(stride % 4 != 0)
						{
							int new_stride = stride + (4 - (stride % 4));
							textures->texture[textures->num_texture].pixels =
								(uint8*)MEM_ALLOC_FUNC(new_stride * height);
							for(i=0; i<(size_t)height; i++)
							{
								(void)memcpy(&textures->texture[textures->num_texture].pixels[new_stride*i],
									&pixels[stride*i], stride);
							}
							MEM_FREE_FUNC(pixels);
							textures->texture[textures->num_texture].stride = new_stride;
						}
						else
						{
							textures->texture[textures->num_texture].pixels = pixels;
							textures->texture[textures->num_texture].stride = stride;
						}
						break;
					case 2:
						{
							size_t j;
							textures->texture[textures->num_texture].stride = width;
							if(textures->texture[textures->num_texture].stride % 4 != 0)
							{
								textures->texture[textures->num_texture].stride +=
									4 - (textures->texture[textures->num_texture].stride % 4);
							}

							textures->texture[textures->num_texture].pixels = (uint8*)MEM_ALLOC_FUNC(
								textures->texture[textures->num_texture].stride * height);
							for(i=0; i<(size_t)height; i++)
							{
								for(j=0; j<(size_t)width; j++)
								{
									textures->texture[textures->num_texture].pixels[
										textures->texture[textures->num_texture].stride*i + j] = pixels[stride*i + j*2];
								}
							}

							MEM_FREE_FUNC(pixels);
						}
						break;
					case 3:
						{
							size_t j;
							textures->texture[textures->num_texture].stride = width;
							if(textures->texture[textures->num_texture].stride % 4 != 0)
							{
								textures->texture[textures->num_texture].stride +=
									4 - (textures->texture[textures->num_texture].stride % 4);
							}

							RGB2GrayScaleFilter(pixels, pixels, (stride / channel) * height, NULL);

							textures->texture[textures->num_texture].pixels = (uint8*)MEM_ALLOC_FUNC(
								textures->texture[textures->num_texture].stride * height);
							for(i=0; i<(size_t)height; i++)
							{
								for(j=0; j<(size_t)width; j++)
								{
									textures->texture[textures->num_texture].pixels[
										textures->texture[textures->num_texture].stride*i + j] = pixels[stride*i + j*3];
								}
							}

							MEM_FREE_FUNC(pixels);
						}
						break;
					case 4:
						{
							size_t j;
							textures->texture[textures->num_texture].stride = width;
							if(textures->texture[textures->num_texture].stride % 4 != 0)
							{
								textures->texture[textures->num_texture].stride +=
									4 - (textures->texture[textures->num_texture].stride % 4);
							}

							textures->texture[textures->num_texture].pixels = (uint8*)MEM_ALLOC_FUNC(
								textures->texture[textures->num_texture].stride * height);
							for(i=0; i<(size_t)height; i++)
							{
								for(j=0; j<(size_t)width; j++)
								{
									textures->texture[textures->num_texture].pixels[
										textures->texture[textures->num_texture].stride * i + j] = pixels[stride*i + j*4+3];
								}
							}

							MEM_FREE_FUNC(pixels);
						}
						break;
					}

					(void)FileSeek((void*)stream, 0, SEEK_SET);
					textures->texture[textures->num_texture].thumbnail =
						gdk_pixbuf_new_from_stream_at_scale(G_INPUT_STREAM(stream),
							TEXTURE_ICON_SIZE, TEXTURE_ICON_SIZE, TRUE, NULL, NULL);
					textures->num_texture++;
					if(textures->num_texture >= (*buff_size) - 1)
					{
						*buff_size += TEXTURE_BLOCK_SIZE;
						textures->texture = (TEXTURE*)MEM_REALLOC_FUNC(
							textures->texture, sizeof(*textures->texture)*(*buff_size));
					}
				}

				g_object_unref(stream);
				g_object_unref(fp);
			}
		}
	}

	g_dir_close(dir);
}

/*****************************************************
* LoadTexture関数                                    *
* テクスチャをロードする                             *
* 引数                                               *
* textures	: テクスチャを管理する構造体のアドレス   *
* directly	: テクスチャ画像のあるディレクトリのパス *
*****************************************************/
void LoadTexture(TEXTURES* textures, char* directly)
{
	int block_size = TEXTURE_BLOCK_SIZE;

	textures->num_texture = 0;
	textures->texture = (TEXTURE*)MEM_ALLOC_FUNC(
		sizeof(*textures->texture)*block_size);

	LoadTextureLoop(textures, directly, &block_size);
}

#ifdef __cplusplus
}
#endif
