// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "configure.h"
#include "pattern.h"
#include "image_read_write.h"
#include "utils.h"
#include "memory.h"

#if !defined(USE_QT) || (defined(USE_QT) && USE_QT != 0)
# include "gui/GTK/utils_gtk.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
* InitializePatterns�֐�                                     *
* �p�^�[����������                                           *
* ����                                                       *
* pattern			: �p�^�[�������Ǘ�����\���̂̃A�h���X *
* directory_path	: �p�^�[���t�@�C���̂���f�B���N�g���p�X *
* buffer_size		: ���݂̃o�b�t�@�T�C�Y                   *
*************************************************************/
void InitializePattern(PATTERNS* pattern, const char* directory_path, int* buffer_size)
{
#define PATTERN_BUFFER_SIZE 16
	// �p�^�[���t�@�C��������f�B���N�g�����J��
	GDir* dir = g_dir_open(directory_path, 0, NULL);
	// �t�@�C����
	const gchar* file_name;
	// �t�@�C���p�X
	char file_path[4096];
	// �t�@�C���ǂݍ��ݗp
	GFile* fp;
	GFileInputStream* stream;
	// �摜�̃o�C�g��
	size_t num_byte;
	// 1�s���̃o�C�g��
	int stride;
	// for���p�̃J�E���^
	int i;

	// �f�B���N�g���I�[�v�����s
	if(dir == NULL)
	{
		pattern->patterns = NULL;
	}

	// �p�^�[�����������Ƃ肠�������߂Ɋm��
	if(pattern->patterns == NULL)
	{
		*buffer_size = PATTERN_BUFFER_SIZE;
		pattern->patterns = (PATTERN*)MEM_ALLOC_FUNC(sizeof(*pattern->patterns)*(*buffer_size));
	}

	// �f�B���N�g���̃t�@�C����ǂݍ���
	while((file_name = g_dir_read_name(dir)) != NULL)
	{	// GLib�̃��C�u�����֐��Ńt�@�C���X�g���[���I�[�v��
			// �T�u�f�B���N�g��������΂�������Q��
		GDir *child_dir;
		// �f�B���N�g���p�X+�t�@�C�����Ńt�@�C���p�X�쐬
		(void)sprintf(file_path, "%s/%s", directory_path, file_name);
		child_dir = g_dir_open(file_path, 0, NULL);
		if(child_dir != NULL)
		{
			g_dir_close(child_dir);
			InitializePattern(pattern, file_path, buffer_size);
		}
			
		if((fp = g_file_new_for_path(file_path)) != NULL)
		{
			if((stream = g_file_read(fp, NULL, NULL)) != NULL)
			{	// PNG�f�[�^��ǂݍ���
				pattern->patterns[pattern->num_pattern].pixels =
					ReadPNGStream(stream, (stream_func_t)FileRead,
						&pattern->patterns[pattern->num_pattern].width,
						&pattern->patterns[pattern->num_pattern].height,
						&pattern->patterns[pattern->num_pattern].stride
					);

				// �摜�̍ő�o�C�g�����v�Z
				num_byte = pattern->patterns[pattern->num_pattern].width
					* pattern->patterns[pattern->num_pattern].height * 4;
				if(pattern->pattern_max_byte < num_byte)
				{
					pattern->pattern_max_byte = num_byte;
				}

				// ��s���̃o�C�g����4�̔{���łȂ��ꍇ���삪�ۏ؂ł��Ȃ��̂Œǉ����~
				if(pattern->patterns[pattern->num_pattern].width > 0
					&& pattern->patterns[pattern->num_pattern].pixels != NULL)
				{
					pattern->patterns[pattern->num_pattern].channel =
						(uint8)(pattern->patterns[pattern->num_pattern].stride / pattern->patterns[pattern->num_pattern].width);
					if(pattern->patterns[pattern->num_pattern].channel == 1)
					{	// �O���[�X�P�[���Ȃ�Ύg���₷���悤�Ƀs�N�Z���f�[�^���]
						uint8 *old_pixels = pattern->patterns[pattern->num_pattern].pixels;
						int j;

						stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, pattern->patterns[pattern->num_pattern].width);
						pattern->patterns[pattern->num_pattern].pixels = (uint8*)MEM_ALLOC_FUNC(
							stride * pattern->patterns[pattern->num_pattern].height);
						(void)memset(pattern->patterns[pattern->num_pattern].pixels, 0,
							stride * pattern->patterns[pattern->num_pattern].height);

						for(i=0;
							i<pattern->patterns[pattern->num_pattern].height; i++)
						{
							for(j=0; j<pattern->patterns[pattern->num_pattern].width; j++)
							{
								pattern->patterns[pattern->num_pattern].pixels[i*stride+j] =
									0xff - old_pixels[i*pattern->patterns[pattern->num_pattern].width+j];
							}
						}

						MEM_FREE_FUNC(old_pixels);
						pattern->patterns[pattern->num_pattern].stride = stride;
					}
					else if(pattern->patterns[pattern->num_pattern].channel == 2)
					{
						uint8 *old_pixels = pattern->patterns[pattern->num_pattern].pixels;
						stride = cairo_format_stride_for_width(
							CAIRO_FORMAT_A8, pattern->patterns[pattern->num_pattern].width);
						pattern->patterns[pattern->num_pattern].pixels = (uint8*)MEM_ALLOC_FUNC(
							stride * 2 * pattern->patterns[pattern->num_pattern].height);
						(void)memset(pattern->patterns[pattern->num_pattern].pixels, 0,
							stride * 2 * pattern->patterns[pattern->num_pattern].height);
						for(i=0; i<pattern->patterns[pattern->num_pattern].height; i++)
						{
							(void)memcpy(&pattern->patterns[pattern->num_pattern].pixels[stride*i*2],
								&old_pixels[i*pattern->patterns[pattern->num_pattern].width*2], pattern->patterns[pattern->num_pattern].width*2);
						}

						MEM_FREE_FUNC(old_pixels);
						pattern->patterns[pattern->num_pattern].stride = stride * 2;
					}
					else
					{
						stride = pattern->patterns[pattern->num_pattern].width * 4;
						if(pattern->patterns[pattern->num_pattern].channel == 3)
						{	// RGB��RGBA�ɕϊ�����
							uint8* old_pixels = pattern->patterns[pattern->num_pattern].pixels;
							pattern->patterns[pattern->num_pattern].pixels = (uint8*)MEM_ALLOC_FUNC(
								pattern->patterns[pattern->num_pattern].width*pattern->patterns[pattern->num_pattern].height*4);
							for(i=0;
								i<pattern->patterns[pattern->num_pattern].width*pattern->patterns[pattern->num_pattern].height; i++)
							{
								pattern->patterns[pattern->num_pattern].pixels[i*4] = old_pixels[i*3+2];
								pattern->patterns[pattern->num_pattern].pixels[i*4+1] = old_pixels[i*3+1];
								pattern->patterns[pattern->num_pattern].pixels[i*4+2] = old_pixels[i*3];
								pattern->patterns[pattern->num_pattern].pixels[i*4+3] = 0xff;
							}
							pattern->patterns[pattern->num_pattern].channel = 4;
							pattern->patterns[pattern->num_pattern].stride =
								pattern->patterns[pattern->num_pattern].width * 4;

							MEM_FREE_FUNC(old_pixels);
						}
						else
						{
							uint8 r;
							for(i=0; i<pattern->patterns[pattern->num_pattern].width*pattern->patterns[pattern->num_pattern].height; i++)
							{
								r = pattern->patterns[pattern->num_pattern].pixels[i*4];
								pattern->patterns[pattern->num_pattern].pixels[i*4] = pattern->patterns[pattern->num_pattern].pixels[i*4+2];
								pattern->patterns[pattern->num_pattern].pixels[i*4+2] = r;
							}
						}

						pattern->patterns[pattern->num_pattern].stride = stride;
					}

					// �p�^�[�����X�V
					pattern->num_pattern++;

					// �o�b�t�@�̎c�肪�s�����Ă�����Ŋm��
					if(pattern->num_pattern >= (*buffer_size)-1)
					{
						(*buffer_size) += PATTERN_BUFFER_SIZE;
						pattern->patterns = (PATTERN*)MEM_REALLOC_FUNC(
							pattern->patterns, sizeof(*pattern->patterns)*(*buffer_size));
					}
				}

				g_object_unref(stream);
			}
			g_object_unref(fp);
		}
	}

	// �p�^�[���̍ő�o�C�g���Ńo�b�t�@�쐬
	pattern->pattern_pixels = (uint8*)MEM_ALLOC_FUNC(pattern->pattern_max_byte);
	pattern->pattern_pixels_temp = (uint8*)MEM_ALLOC_FUNC(pattern->pattern_max_byte);
	pattern->pattern_mask = (uint8*)MEM_ALLOC_FUNC(pattern->pattern_max_byte / 4);

	// �A�N�e�B�u�ȃp�^�[���͂Ƃ肠�����擪�ɂ��Ă���
	pattern->active_pattern = pattern->patterns;

	g_dir_close(dir);
}

/***********************************************************
* ClipBoardImageRecieveCallBack�֐�                        *
* �N���b�v�{�[�h�̉摜�f�[�^���󂯂������̃R�[���o�b�N�֐� *
* ����                                                     *
* clipboard	: �N���b�v�{�[�h�̏��                         *
* pixbuf	: �s�N�Z���o�b�t�@                             *
* data		: �A�v���P�[�V�������Ǘ�����\���̂�           *
***********************************************************/
static void ClipBoardImageRecieveCallBack(
	GtkClipboard *clipboard,
	GdkPixbuf* pixbuf,
	gpointer data
)
{
	// �p�^�[�����ɃL���X�g
	PATTERNS* patterns = (PATTERNS*)data;
	// �s�N�Z���o�b�t�@�̃o�C�g��
	size_t num_bytes;

	if(pixbuf != NULL)
	{	// �s�N�Z���o�b�t�@�쐬�ɐ������Ă�����
			// ���`�����l�����Ȃ���Βǉ�
		if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
		{
			pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
		}

		patterns->clip_board.width = gdk_pixbuf_get_width(pixbuf);
		patterns->clip_board.height = gdk_pixbuf_get_height(pixbuf);
		patterns->clip_board.stride = gdk_pixbuf_get_rowstride(pixbuf);
		patterns->clip_board.channel = 4;
		num_bytes = patterns->clip_board.stride * patterns->clip_board.height;
		if(patterns->pattern_max_byte < num_bytes)
		{
			patterns->pattern_max_byte = num_bytes;
			patterns->pattern_pixels = (uint8*)MEM_REALLOC_FUNC(patterns->pattern_pixels, num_bytes);
			patterns->pattern_pixels_temp = (uint8*)MEM_REALLOC_FUNC(patterns->pattern_pixels_temp, num_bytes);
			patterns->pattern_mask = (uint8*)MEM_REALLOC_FUNC(patterns->pattern_mask, num_bytes / 4);
		}

		patterns->clip_board.pixels = (uint8*)MEM_REALLOC_FUNC(patterns->clip_board.pixels, num_bytes);
		(void)memcpy(patterns->clip_board.pixels, gdk_pixbuf_get_pixels(pixbuf), num_bytes);

		patterns->has_clip_board_pattern = TRUE;

		g_object_unref(pixbuf);
	}
	else
	{
		patterns->has_clip_board_pattern = FALSE;
	}
}

/*********************************************************
* UpdateClipBoardPattern�֐�                             *
* �N���b�v�{�[�h����摜�f�[�^�����o���ăp�^�[���ɂ��� *
* ����                                                   *
* patterns	: �p�^�[�����Ǘ�����\���̂̃A�h���X         *
*********************************************************/
void UpdateClipBoardPattern(PATTERNS* patterns)
{
	// �N���b�v�{�[�h�ɑ΂��ăf�[�^��v��
	gtk_clipboard_request_image(gtk_clipboard_get(GDK_NONE),
		ClipBoardImageRecieveCallBack, patterns);
}

/*************************************************
* CreatePatternSurface�֐�                       *
* �p�^�[���h��ׂ����s�p��CAIRO�T�[�t�F�[�X�쐬  *
* ����                                           *
* patterns	: �p�^�[�����Ǘ�����\���̂̃A�h���X *
* scale		: �p�^�[���̊g�嗦                   *
* rgb		: ���݂̕`��F                       *
* back_rgb	: ���݂̔w�i�F                       *
* flags		: ���E���]�A�㉺���]�̃t���O         *
* mode		: �`�����l������2�̂Ƃ��̍쐬���[�h  *
* flow		: �p�^�[���̔Z�x                     *
* �Ԃ�l                                         *
*	�쐬�����T�[�t�F�[�X                         *
*************************************************/
cairo_surface_t* CreatePatternSurface(
	PATTERNS* patterns,
	uint8 rgb[3],
	uint8 back_rgb[3],
	int flags,
	int mode,
	gdouble flow
)
{
	// CAIRO���g���ăs�N�Z���f�[�^���T�[�t�F�[�X��
	cairo_t* cairo_p;
	// �Ԃ�l
	cairo_surface_t *surface_p;
	// �p�^�[���̃s�N�Z���f�[�^�T�[�t�F�[�X
	cairo_surface_t *image;
	// �쐬����T�[�t�F�[�X�̕��A����
	int width, height;
	// �s�N�Z���f�[�^���R�s�[���Ȃ����̃t���O
	int no_copy_pixels = 0;
	// �쐬����T�[�t�F�[�X�̃t�H�[�}�b�g
	cairo_format_t format =
		(patterns->active_pattern->channel == 4) ? CAIRO_FORMAT_ARGB32 :
			(patterns->active_pattern->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;
	// for���p�̃J�E���^
	int i, j, k;

	// �T�[�t�F�[�X�̕��A����������
	width = (int)(patterns->active_pattern->width);
	// �����v�Z
	height = (int)(patterns->active_pattern->height);

	// ���܂��͍���0�Ȃ�I��
	if(width == 0 || height == 0)
	{
		return NULL;
	}

	// ���E���]
	if((flags & PATTERN_FLIP_HORIZONTALLY) != 0)
	{
		int stride = cairo_format_stride_for_width(format, patterns->active_pattern->width);
		int row = (patterns->active_pattern->channel <= 2) ? stride :
					stride / patterns->active_pattern->channel;

		if(patterns->active_pattern->channel == 2)
		{
			stride *= 2;
		}

		no_copy_pixels++;
		for(i=0; i<patterns->active_pattern->height; i++)
		{
			for(j=0; j<patterns->active_pattern->width; j++)
			{
				for(k=0; k<patterns->active_pattern->channel; k++)
				{
					patterns->pattern_pixels[i*stride
						+j*patterns->active_pattern->channel+k] =
							patterns->active_pattern->pixels[i*stride
								+(patterns->active_pattern->width-j-1)*patterns->active_pattern->channel+k];
				}
			}
		}
	}
	// �㉺���]
	if((flags & PATTERN_FLIP_VERTICALLY) != 0)
	{
		int stride = cairo_format_stride_for_width(format, patterns->active_pattern->width);
		int row = (patterns->active_pattern->channel <= 2) ? stride :
					stride / patterns->active_pattern->channel;
		if(patterns->active_pattern->channel == 2)
		{
			stride *= 2;
		}

		no_copy_pixels++;
		if((flags & PATTERN_FLIP_HORIZONTALLY) != 0)
		{
			for(i=0; i<patterns->active_pattern->height; i++)
			{
				for(j=0; j<row; j++)
				{
					for(k=0; k<patterns->active_pattern->channel; k++)
					{
						patterns->pattern_pixels_temp[i*stride
							+j*patterns->active_pattern->channel+k] =
								patterns->pattern_pixels[(patterns->active_pattern->height-i-1)*stride
									+j*patterns->active_pattern->channel+k];
					}
				}
			}

			(void)memcpy(patterns->pattern_pixels, patterns->pattern_pixels_temp,
				patterns->active_pattern->stride*patterns->active_pattern->height);
		}
		else
		{
			for(i=0; i<patterns->active_pattern->height; i++)
			{
				for(j=0; j<row; j++)
				{
					for(k=0; k<patterns->active_pattern->channel; k++)
					{
						patterns->pattern_pixels[i*stride
							+j*patterns->active_pattern->channel+k] =
							patterns->active_pattern->pixels[(patterns->active_pattern->height-i-1)*stride
								+j*patterns->active_pattern->channel+k];
					}
				}
			}
		}
	}
	if(no_copy_pixels == 0)
	{
		(void)memcpy(patterns->pattern_pixels, patterns->active_pattern->pixels,
			patterns->active_pattern->stride*patterns->active_pattern->height);
	}

	// �Ԃ�l�쐬
	surface_p = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_p = cairo_create(surface_p);

	// �A�N�e�B�u�ȃp�^�[���̃`�����l�����ŏ����؂�ւ�
	switch(patterns->active_pattern->channel)
	{
	case 1:	// �`�����l����1�Ȃ�`��F�œh��ׂ��ăp�^�[���Ń}�X�N
		// �s�����x�ݒ�
		for(i=0; i<patterns->active_pattern->stride*patterns->active_pattern->height; i++)
		{
			patterns->pattern_pixels[i] = (uint8)(patterns->pattern_pixels[i]*flow);
		}
		image = cairo_image_surface_create_for_data(patterns->pattern_pixels,
			format, width, height, patterns->active_pattern->stride);
		cairo_set_source_rgb(cairo_p, rgb[0]*DIV_PIXEL, rgb[1]*DIV_PIXEL, rgb[2]*DIV_PIXEL);
		cairo_rectangle(cairo_p, 0, 0, width, height);
		cairo_mask_surface(cairo_p, image, 0, 0);
		break;
	case 2:	// �`�����l����2�Ȃ烂�[�h�ɂ���ď����؂�ւ�
		{
			// �F�ݒ�p
			HSV hsv;
			uint8 color[3];
			// �摜�̃��`�����l���Ń}�X�N����
			cairo_surface_t* mask;
			// 1�s���̃o�C�g��
			int stride;

			// RGB��HSV�ɕϊ�
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			if(mode == PATTERN_MODE_FORE_TO_BACK)
			{
				color[0] = rgb[0], color[1] = rgb[1], color[2] = rgb[2];
			}
			else
			{
				color[0] = rgb[2], color[1] = rgb[1], color[2] = rgb[0];
			}
#else
			color[0] = rgb[0], color[1] = rgb[1], color[2] = rgb[2];
#endif
			RGB2HSV_Pixel(color, &hsv);

			stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, patterns->active_pattern->width);

			// �s�����x�ݒ�
			for(i=0; i<patterns->active_pattern->height; i++)
			{
				for(j=0; j<patterns->active_pattern->width; j++)
				{
					patterns->pattern_mask[i*stride+j] =
						(uint8)(patterns->pattern_pixels[i*stride*2+j*2+1]*flow);
				}
			}
		
			switch(mode)
			{
			case PATTERN_MODE_SATURATION:	// �ʓx�Ńp�^�[���쐬
				for(i=0; i<patterns->active_pattern->height; i++)
				{
					for(j=0; j<patterns->active_pattern->width; j++)
					{
						hsv.s = 0xff - patterns->pattern_pixels[i*stride*2+j*2];
						HSV2RGB_Pixel(&hsv, &patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4]);
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+3] = 0xff;
					}
				}
				break;
			case PATTERN_MODE_BRIGHTNESS:	// ���x�Ńp�^�[���쐬
				for(i=0; i<patterns->active_pattern->height; i++)
				{
					for(j=0; j<patterns->active_pattern->width; j++)
					{
						hsv.v = 0xff - patterns->pattern_pixels[i*stride*2+j*2];
						HSV2RGB_Pixel(&hsv, &patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4]);
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+3] = 0xff;
					}
				}
				break;
			case PATTERN_MODE_FORE_TO_BACK:	// �`��F����w�i�F��
				for(i=0; i<patterns->active_pattern->height; i++)
				{
					for(j=0; j<patterns->active_pattern->width; j++)
					{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+2] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[0] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[0]) >> 8;
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+1] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[1] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[1]) >> 8;
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[2] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[2]) >> 8;
#else
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[0] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[0]) >> 8;
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+1] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[1] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[1]) >> 8;
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+2] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[2] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[2]) >> 8;
#endif
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+3] = 0xff;
					}
				}
				break;
			}
			mask = cairo_image_surface_create_for_data(patterns->pattern_mask, format,
				patterns->active_pattern->width, patterns->active_pattern->height, stride);
			image = cairo_image_surface_create_for_data(patterns->pattern_pixels_temp, CAIRO_FORMAT_ARGB32,
				patterns->active_pattern->width, patterns->active_pattern->height, patterns->active_pattern->width*4);
			cairo_set_source_surface(cairo_p, image, 0, 0);
			cairo_mask_surface(cairo_p, mask, 0, 0);
			cairo_surface_destroy(mask);
		}
		break;
	default:
		image = cairo_image_surface_create_for_data(patterns->pattern_pixels,
			format, patterns->active_pattern->width,
			patterns->active_pattern->height, patterns->active_pattern->stride);
		cairo_set_source_surface(cairo_p, image, 0, 0);
		cairo_paint_with_alpha(cairo_p, flow);
	}

	// �s�v�ɂȂ����f�[�^���폜
	cairo_surface_destroy(image);
	cairo_destroy(cairo_p);
	return surface_p;
}

#ifdef __cplusplus
}
#endif
