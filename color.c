// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef _WIN32 // #ifdef G_OS_WIN32
# define INCLUDE_WIN_DEFAULT_API
# define STRICT
# include <windows.h>
# define LCMS_WIN_TYPES_ALREADY_DEFINED
#endif

#include <math.h>
#include <string.h>
#include "application.h"
#include "configure.h"
#include "memory.h"
#include "color.h"
#include "utils.h"
#include "srgb_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

void RGB2HSV_Pixel(uint8 rgb[3], HSV* hsv)
{
#define MAX_IS_RED 0
#define MAX_IS_GREEN 1
#define MAX_IS_BLUE 2

	unsigned char maximum, minimum;
	int channel = MAX_IS_RED;
	FLOAT_T d;
	FLOAT_T cr, cg, cb;
	FLOAT_T h;
	
	maximum = rgb[0];
	if(maximum < rgb[1])
	{
		channel = MAX_IS_GREEN;
		maximum = rgb[1];
	}
	if(maximum < rgb[2])
	{
		channel = MAX_IS_BLUE;
		maximum = rgb[2];
	}

	minimum = rgb[0];
	if(minimum > rgb[1])
	{
		minimum = rgb[1];
	}
	if(minimum > rgb[2])
	{
		minimum = rgb[2];
	}

	d = 1.0 / (maximum - minimum);
	hsv->v = maximum;

	if(maximum == 0)
	{
		hsv->s = 0;
		hsv->h = 0;
	}
	else
	{
		hsv->s = (uint8)(255*(maximum - minimum)/(FLOAT_T)maximum);
		cr = (maximum - rgb[0])*d;
		cg = (maximum - rgb[1])*d;
		cb = (maximum - rgb[2])*d;

		switch(channel)
		{
		case MAX_IS_RED:
			h = cb - cg;
			break;
		case MAX_IS_GREEN:
			h = 2 + cr - cb;
			break;
		default:
			h = 4 + cg - cr;
		}

		h *= 60;
		if(h < 0)
		{
			h += 360;
		}
		hsv->h = (int16)h;
	}
}

HSV* RGB2HSV(
	uint8 *pixels,
	int32 width,
	int32 height,
	int32 channel
)
{
	int stride = width * channel;
	HSV *ret = (HSV*)MEM_ALLOC_FUNC(sizeof(*ret)*stride*height);
	int i;

#ifdef _OPENMP
#pragma omp parallel for firstprivate(width, pixels, ret, stride, channel)
#endif
	for(i=0; i<height; i++)
	{
		int j;
		for(j=0; j<width; j++)
		{
			RGB2HSV_Pixel(&pixels[i*stride+j*channel], &ret[i*width+j]);
		}
	}

	return ret;
}

void HSV2RGB_Pixel(HSV* hsv, uint8 rgb[3])
{
	int tempi, tempm, tempn, tempk;
	FLOAT_T tempf, div_f, div_s;

	if(hsv->v == 0)
	{
		rgb[0] = rgb[1] = rgb[2] = 0;
	}
	else
	{
		div_f = hsv->h/(FLOAT_T)60;
		div_s = hsv->s/(FLOAT_T)255;
		tempi = (int)div_f;
		tempf = div_f - tempi;
		tempm = (int)(hsv->v*(1 - div_s));
		tempn = (int)(hsv->v*(1 - div_s*tempf));
		tempk = (int)(hsv->v*(1 - div_s*(1 - tempf)));

		switch(tempi)
		{
		case 0:
			rgb[0] = hsv->v;
			rgb[1] = (uint8)tempk;
			rgb[2] = (uint8)tempm;
			break;
		case 1:
			rgb[0] = tempn;
			rgb[1] = hsv->v;
			rgb[2] = tempm;
			break;
		case 2:
			rgb[0] = tempm;
			rgb[1] = hsv->v;
			rgb[2] = tempk;
			break;
		case 3:
			rgb[0] = tempm;
			rgb[1] = tempn;
			rgb[2] = hsv->v;
			break;
		case 4:
			rgb[0] = tempk;
			rgb[1] = tempm;
			rgb[2] = hsv->v;
			break;
		default:
			rgb[0] = hsv->v;
			rgb[1] = tempm;
			rgb[2] = tempn;
		}
	}
}

void HSV2RGB(
	HSV* hsv,
	uint8 *pixels,
	int32 width,
	int32 height,
	int32 channel
)
{
	int stride = width * channel;
	int i, j;

	for(i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			HSV2RGB_Pixel(&hsv[i*width+j], &pixels[i*stride+j*channel]);
		}
	}
}

void RGB2CMYK_Pixel(uint8 rgb[3], CMYK* cmyk)
{
	cmyk->k = rgb[0];
	if(cmyk->k > rgb[1])
	{
		cmyk->k = rgb[1];
	}
	if(cmyk->k > rgb[2])
	{
		cmyk->k = rgb[2];
	}

	cmyk->c = 0xff - (0xff*rgb[0])/0xff;
	cmyk->m = 0xff - (0xff*rgb[1])/0xff;
	cmyk->y = 0xff - (0xff*rgb[2])/0xff;

	if(cmyk->c == cmyk->m && cmyk->c == cmyk->y)
	{
		cmyk->k = cmyk->c;
		cmyk->c = cmyk->m = cmyk->y = 0;
	}
}

void CMYK2RGB_Pixel(CMYK* cmyk, uint8 rgb[3])
{
	unsigned int value;
	value = (cmyk->c*(0xff-cmyk->k))/0xff+cmyk->k;
	rgb[0] = (value > 0xff) ? 0 : 0xff - value;
	value = (cmyk->m*(0xff-cmyk->k))/0xff+cmyk->k;
	rgb[1] = (value > 0xff) ? 0 : 0xff - value;
	value = (cmyk->y*(0xff-cmyk->k))/0xff+cmyk->k;
	rgb[2] = (value > 0xff) ? 0 : 0xff - value;
}

void CMYK16_to_RGB8(CMYK16* cmyk, uint8 rgb[3])
{
	unsigned int value;
	unsigned int rgb16[3];
	value = (cmyk->c*(0xffff-cmyk->k))/0xffff+cmyk->k;
	rgb16[0] = (value > 0xffff) ? 0 : 0xffff - value;
	value = (cmyk->m*(0xffff-cmyk->k))/0xffff+cmyk->k;
	rgb16[1] = (value > 0xffff) ? 0 : 0xffff - value;
	value = (cmyk->y*(0xffff-cmyk->k))/0xffff+cmyk->k;
	rgb16[2] = (value > 0xffff) ? 0 : 0xffff - value;

	rgb[0] = rgb16[0] >> 8;
	rgb[1] = rgb16[1] >> 8;
	rgb[2] = rgb16[2] >> 8;
}

void RGB16_to_RGB8(uint16 rgb16[3], uint8 rgb8[3])
{
	rgb8[0] = rgb16[0] >> 8;
	rgb8[1] = rgb16[1] >> 8;
	rgb8[2] = rgb16[2] >> 8;
}

void RGB8_to_RGB16(uint8 rgb8[3], uint16 rgb16[3])
{
	rgb16[0] = rgb8[0] | (rgb8[0] << 8);
	rgb16[1] = rgb8[1] | (rgb8[1] << 8);
	rgb16[2] = rgb8[2] | (rgb8[2] << 8);
}

void Gray10000_to_RGB8(uint16 gray, uint8 rgb8[3])
{
	rgb8[0] = rgb8[1] = rgb8[2] = (gray * 255) / 10000;
}

void Gray16_to_RGB8(uint16 gray, uint8 rgb8[3])
{
	rgb8[0] = rgb8[1] = rgb8[2] = gray >> 8;
}

/*************************************************
* ReadACO関数                                    *
* Adobe Color Fileを読み込む                     *
* 引数                                           *
* src			: データへのポインタ             *
* read_func		: 読み込みに使用する関数ポインタ *
* rgb			: 読み込んだRGBデータの格納先    *
* buffer_size	: 最大読み込み数                 *
* 返り値                                         *
*	読み込みに成功したデータの数                 *
*************************************************/
int ReadACO(
	void* src,
	stream_func_t read_func,
	uint8 (*rgb)[3],
	int buffer_size
)
{
	uint8 value_data[10];
	uint16 value[5];
	uint16 count;
	char c;
	int read_num = 0;
	int i;

	{
		uint8 count_data[sizeof(count)];
		(void)read_func(value_data, sizeof(*value), 1, src);
		(void)read_func(count_data, sizeof(count), 1, src);

		value[0] = (value_data[0] << 8) | value_data[1];
		count = (count_data[0] << 8) | count_data[1];
	}

	switch(value[0])
	{
	case 1:
		for(i=0; i<count && read_num <buffer_size; i++)
		{
			(void)read_func(value_data, sizeof(*value), 5, src);
			value[0] = (value_data[0] << 8) | value_data[1];
			value[1] = (value_data[2] << 8) | value_data[3];
			value[2] = (value_data[4] << 8) | value_data[5];
			value[3] = (value_data[6] << 8) | value_data[7];
			value[4] = (value_data[8] << 8) | value_data[9];

			switch(value[0])
			{
			case 0:
				RGB16_to_RGB8(&value[1], rgb[read_num]);
				read_num++;
				break;
			case 1:
				{
					HSV hsv;
					hsv.h = value[1];
					hsv.s = value[2] >> 8;
					hsv.v = value[3] >> 8;
					HSV2RGB_Pixel(&hsv, rgb[read_num]);
					read_num++;
				}
				break;
			case 2:
				{
					CMYK16 cmyk = {0xffff-value[1], 0xffff-value[2], 0xffff-value[3], 0xffff-value[4]};
					CMYK16_to_RGB8(&cmyk, rgb[read_num]);
					read_num++;
				}
				break;
			case 8:
				Gray10000_to_RGB8(value[1], rgb[read_num]);
				rgb[read_num][0] = rgb[read_num][1] = rgb[read_num][2] = ~rgb[read_num][0];
				read_num++;
				break;
			}
		}
		break;
	case 2:
		for(i=0; i<count && read_num <buffer_size; i++)
		{
			(void)read_func(value_data, sizeof(*value), 5, src);
			value[0] = (value_data[0] << 8) | value_data[1];
			value[1] = (value_data[2] << 8) | value_data[3];
			value[2] = (value_data[4] << 8) | value_data[5];
			value[3] = (value_data[6] << 8) | value_data[7];
			value[4] = (value_data[8] << 8) | value_data[9];

			switch(value[0])
			{
			case 0:
				RGB16_to_RGB8(&value[1], rgb[read_num]);
				read_num++;
				break;
			case 1:
				{
					HSV hsv;
					hsv.h = value[1];
					hsv.s = value[2] >> 8;
					hsv.v = value[3] >> 8;
					HSV2RGB_Pixel(&hsv, rgb[read_num]);
					read_num++;
				}
				break;
			case 2:
				{
					CMYK16 cmyk = {value[1], value[2], value[3], value[4]};
					CMYK16_to_RGB8(&cmyk, rgb[read_num]);
					read_num++;
				}
				break;
			case 8:
				Gray10000_to_RGB8(value[1], rgb[read_num]);
				rgb[read_num][0] = rgb[read_num][1] = rgb[read_num][2] = ~rgb[read_num][0];
				read_num++;
				break;
			}

			(void)read_func(&c, sizeof(c), 1, src);
			while(c != '\0')
			{
				(void)read_func(&c, sizeof(c), 1, src);
			}
		}
		break;
	}

#if 0 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
	{
		uint8 r;
		for(i=0; i<read_num; i++)
		{
			r = rgb[i][0];
			rgb[i][0] = rgb[i][2];
			rgb[i][2] = r;
		}
	}
#endif

	return read_num;
}

/***********************************************
* WriteACOファイル                             *
* Adobe Color Fileに書き出す                   *
* 引数                                         *
* dst			: 書き出し先へのポインタ       *
* write_func	: 書き出し用の関数へのポインタ *
* rgb			: 書きだすRGBデータ配列        *
* write_num		: 書きだすデータの数           *
***********************************************/
void WriteACO(
	void* dst,
	stream_func_t write_func,
	uint8 (*rgb)[3],
	int write_num
)
{
	uint16 value[3] = {0};
	uint8 value_data[6];
	int i;

	value[0] = 1;
	value_data[0] = (uint8)(value[0] >> 8);
	value_data[1] = (uint8)(value[0] & 0xFF);
	(void)write_func(value_data, sizeof(*value), 1, dst);

	value_data[0] = (uint8)((write_num >> 8) & 0xFF);
	value_data[1] = (uint8)(write_num & 0xFF);
	(void)write_func(value_data, sizeof(*value), 1, dst);

	for(i=0; i<write_num; i++)
	{
		value[0] = 0;
		(void)write_func(value, sizeof(*value), 1, dst);
#if 0 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
		color[2] = rgb[i][0];
		color[1] = rgb[i][1];
		color[0] = rgb[i][2];
		RGB8_to_RGB16(color, value);
#else
		RGB8_to_RGB16(rgb[i], value);
#endif
		(void)write_func(value, sizeof(*value), 3, dst);
		value[0] = 0;
		(void)write_func(value, sizeof(*value), 1, dst);
	}
}

/***************************************
* LoadPallete関数                      *
* パレットデータを読み込む             *
* 引数                                 *
* file_path	: データファイルのパス     *
* rgb		: 読み込んだデータの格納先 *
* max_read	: 読み込みを行う最大数     *
* 返り値                               *
*	読み込んだデータの数               *
***************************************/
int LoadPallete(
	const char* file_path,
	uint8 (*rgb)[3],
	int max_read
)
{
	FILE *fp = fopen(file_path, "rb");
	int read_num;

	if(fp == NULL)
	{
		return 0;
	}

	read_num = ReadACO((void*)fp, (stream_func_t)fread, rgb, max_read);

	(void)fclose(fp);

	return read_num;
}

cmsHPROFILE* CreateDefaultSrgbProfile(void)
{
	return cmsOpenProfileFromMem(sRGB_profile, sizeof(sRGB_profile));
}

cmsHPROFILE* GetPrimaryMonitorProfile(void)
{
#ifdef _WIN32  // #ifdef G_OS_WIN32
	HDC hdc = GetDC(NULL);
	cmsHPROFILE *profile = NULL;

	if (hdc)
	{
		char *path;
		DWORD len = 0;

		GetICMProfileA (hdc, &len, NULL);
		path = MEM_ALLOC_FUNC(len);
		(void)memset(path, 0, len);

		if (GetICMProfileA (hdc, &len, (LPSTR)path))
			profile = cmsOpenProfileFromFile (path, "r");

		MEM_FREE_FUNC(path);
		ReleaseDC (NULL, hdc);
	}

	if (profile)
		return profile;
#elif defined GDK_WINDOWING_X11
	cmsHPROFILE *profile = NULL;
	GdkScreen *screen;
	GdkAtom type = GDK_NONE;
	gint format = 0;
	gint nitems = 0;
	gint monitor = 0;
	gchar *atom_name = NULL;
	guchar *data = NULL;

	//screen = gtk_widget_get_screen(app->window);
	//monitor = gdk_screen_get_monitor_at_window(screen, app->window->window);
	screen = gdk_screen_get_default();
	monitor = 0;

	if (monitor > 0)
	{
		atom_name = g_strdup_printf("_ICC_PROFILE_%d", monitor);
	}
	else
	{
		atom_name = g_strdup("_ICC_PROFILE");
	}

	if (gdk_property_get(gdk_screen_get_root_window (screen),
		gdk_atom_intern(atom_name, FALSE),
		GDK_NONE,
		0, 64 * 1024 * 1024, FALSE,
		&type, &format, &nitems, &data) && nitems > 0)
	{
	  profile = cmsOpenProfileFromMem(data, nitems);
	  g_free(data);
	}

	g_free(atom_name);

	if(profile)
	{
		return profile;
	}
#endif
	return cmsOpenProfileFromMem(sRGB_profile, sizeof(sRGB_profile));
}

void GetColor(eCOLOR color_index, uint8* color)
{
	const uint8 colors[NUM_COLOR][3] =
	{
		{128, 128, 128},
		{255, 255, 0},
		{221, 160, 221},
		{255, 0, 0},
		{50, 205, 50},
		{0, 255, 255},
		{0, 255, 0},
		{128, 0, 0},
		{0, 0, 128},
		{128, 128, 0},
		{128, 0, 128},
		{0, 128, 128},
		{0, 0, 255}
	};

	color[0] = colors[color_index][0];
	color[1] = colors[color_index][1];
	color[2] = colors[color_index][2];
}

/****************************************************
* GetLayerColorHistgram関数                         *
* レイヤーのRGBA、CMYK、HSVのヒストグラムを取得する *
* 引数                                              *
* histgram	: ヒストグラムのデータを受ける変数      *
* target	: ヒストグラムを取得するレイヤー        *
****************************************************/
void GetLayerColorHistgram(COLOR_HISTGRAM* histgram, LAYER* target)
{
#define RED_RATE 0.298912
#define GREEN_RATE 0.586611
#define BLUE_RATE 0.114477
	int i;

	// 0クリア
	(void)memset(histgram, 0, sizeof(*histgram));

	if((target->window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{
#ifdef _OPENMP
#pragma omp parallel for firstprivate(target, histgram)
#endif
		for(i=0; i<target->width*target->height; i++)
		{
			HSV hsv;
			CMYK cmyk;
			uint8 *color;
			uint8 rgb[3];
			uint8 alpha;
			unsigned int alpha_sum = 0;

			color = &target->pixels[i*4];
			alpha = color[3];

			if(alpha > 0)
			{
				// RGB
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				rgb[0] = color[2];
				rgb[1] = color[1];
				rgb[2] = color[0];
#else
				rgb[0] = color[0];
				rgb[1] = color[1];
				rgb[2] = color[2];
#endif
				histgram->r[MINIMUM((rgb[0]*255)/alpha, 255)]++;
				histgram->g[MINIMUM((rgb[1]*255)/alpha, 255)]++;
				histgram->b[MINIMUM((rgb[2]*255)/alpha, 255)]++;
				histgram->a[alpha]++;
				alpha_sum += alpha;

				// HSV
				RGB2HSV_Pixel(rgb, &hsv);
				histgram->h[hsv.h]++;
				histgram->s[hsv.s]++;
				histgram->v[MINIMUM(((int)(rgb[0]*RED_RATE+rgb[1]*GREEN_RATE+rgb[2]*BLUE_RATE)*255)/alpha, 255)]++;

				// CMYK
				RGB2CMYK_Pixel(rgb, &cmyk);

				histgram->c[MINIMUM((cmyk.c*255)/alpha, 255)]++;
				histgram->m[MINIMUM((cmyk.m*255)/alpha, 255)]++;
				histgram->y[MINIMUM((cmyk.y*255)/alpha, 255)]++;
				histgram->k[MINIMUM((cmyk.k*255)/alpha, 255)]++;
			}
		}
	}
	else
	{
#ifdef _OPENMP
#pragma omp parallel for firstprivate(target, histgram)
#endif
		for(i=0; i<target->width*target->height; i++)
		{
			HSV hsv;
			CMYK cmyk;
			uint8 *color;
			uint8 rgb[3];
			uint8 alpha;
			uint8 selection;
			unsigned int alpha_sum = 0;

			color = &target->pixels[i*4];
			selection = target->window->selection->pixels[i];

			if(selection > 0)
			{
				alpha = (color[3] * selection) / 255;

				if(alpha > 0)
				{
					// RGB
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
					rgb[0] = color[2];
					rgb[1] = color[1];
					rgb[2] = color[0];
#else
					rgb[0] = color[0];
					rgb[1] = color[1];
					rgb[2] = color[2];
#endif
					histgram->r[MINIMUM((rgb[0]*255)/alpha, 255)]++;
					histgram->g[MINIMUM((rgb[1]*255)/alpha, 255)]++;
					histgram->b[MINIMUM((rgb[2]*255)/alpha, 255)]++;
					histgram->a[alpha]++;
					alpha_sum += alpha;

					// HSV
					RGB2HSV_Pixel(rgb, &hsv);
					histgram->h[hsv.h]++;
					histgram->s[hsv.s]++;
					histgram->v[MINIMUM(((int)(rgb[0]*RED_RATE+rgb[1]*GREEN_RATE+rgb[2]*BLUE_RATE)*255)/alpha, 255)]++;

					// CMYK
					RGB2CMYK_Pixel(rgb, &cmyk);

					histgram->c[MINIMUM((cmyk.c*255)/alpha, 255)]++;
					histgram->m[MINIMUM((cmyk.m*255)/alpha, 255)]++;
					histgram->y[MINIMUM((cmyk.y*255)/alpha, 255)]++;
					histgram->k[MINIMUM((cmyk.k*255)/alpha, 255)]++;
				}
			}
		}
	}

	for(i=0; i<256; i++)
	{
		if(histgram->r[i] > 0)
		{
			histgram->r_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->r[i] > 0)
		{
			histgram->r_max = (uint8)i;
			break;
		}
	}
	for(i=0; i<256; i++)
	{
		if(histgram->g[i] > 0)
		{
			histgram->g_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->g[i] > 0)
		{
			histgram->g_max = (uint8)i;
			break;
		}
	}
	for(i=0; i<256; i++)
	{
		if(histgram->b[i] > 0)
		{
			histgram->b_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->b[i] > 0)
		{
			histgram->b_max = (uint8)i;
			break;
		}
	}
	for(i=0; i<256; i++)
	{
		if(histgram->a[i] > 0)
		{
			histgram->a_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->a[i] > 0)
		{
			histgram->a_max = (uint8)i;
			break;
		}
	}
	for(i=0; i<256; i++)
	{
		if(histgram->s[i] > 0)
		{
			histgram->s_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->s[i] > 0)
		{
			histgram->s_max = (uint8)i;
			break;
		}
	}
	for(i=0; i<256; i++)
	{
		if(histgram->v[i] > 0)
		{
			histgram->v_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->v[i] > 0)
		{
			histgram->v_max = (uint8)i;
			break;
		}
	}
	for(i=0; i<256; i++)
	{
		if(histgram->c[i] > 0)
		{
			histgram->c_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->c[i] > 0)
		{
			histgram->c_max = (uint8)i;
			break;
		}
	}
	for(i=0; i<256; i++)
	{
		if(histgram->m[i] > 0)
		{
			histgram->m_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->m[i] > 0)
		{
			histgram->m_max = (uint8)i;
			break;
		}
	}
	for(i=0; i<256; i++)
	{
		if(histgram->y[i] > 0)
		{
			histgram->y_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->y[i] > 0)
		{
			histgram->y_max = (uint8)i;
			break;
		}
	}
	for(i=0; i<256; i++)
	{
		if(histgram->k[i] > 0)
		{
			histgram->k_min = (uint8)i;
			break;
		}
	}
	for(i=255; i>=0; i--)
	{
		if(histgram->k[i] > 0)
		{
			histgram->k_max = (uint8)i;
			break;
		}
	}
#undef RED_RATE
#undef GREEN_RATE
#undef BLUE_RATE
}

#ifdef __cplusplus
}
#endif
