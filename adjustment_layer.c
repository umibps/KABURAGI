#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "application.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************
* InitializeAdjustmentLyaerChangeBrightContrast関数 *
* 明るさ・コントラスト調整機能を初期化              *
* 引数                                              *
* layer		: 調整レイヤーの情報                    *
* target	: 調整レイヤーの下のレイヤー            *
* mixed		: 調整レイヤーより下を合成したレイヤー  *
****************************************************/
static void InitializeAdjustmentLyaerChangeBrightContrast(
	ADJUSTMENT_LAYER* layer,
	LAYER* target,
	LAYER* mixed
)
{
	uint64 sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
	uint8 *pixels;
	int i;

	if(layer->target_layer == NULL)
	{
		return;
	}

	layer->filter_data.bright_contrast.pixels =
		(uint8*)MEM_ALLOC_FUNC(target->stride * target->height);
	(void)memset(layer->filter_data.bright_contrast.pixels, 0, target->stride * target->height);

	layer->filter_data.bright_contrast.bright = 0;
	layer->filter_data.bright_contrast.contrast = 0;

	switch(layer->target)
	{
	case ADJUSTMENT_LAYER_TARGET_UNDER_LAYER:
		pixels = target->pixels;
		break;
	case ADJUSTMENT_LAYER_TARGET_UNDER_MIXED:
		pixels = mixed->pixels;
		break;
	}

#ifdef _OPENMP
#pragma omp parallel for firstprivate(layer, target, pixels) reduction(+:sum_r, sum_g, sum_b, sum_a)
#endif
	for(i=0; i<target->width * target->height; i++)
	{
		uint8 *p = &layer->filter_data.bright_contrast.pixels[i*4];
		p[0] = pixels[i*4+0];
		sum_r += p[0] * p[3];
		p[1] = pixels[i*4+1];
		sum_g += p[1] * p[3];
		p[2] = pixels[i*4+2];
		sum_b += p[2] * p[3];
		p[3] = pixels[i*4+3];
		sum_a += p[3];
	}

	layer->filter_data.bright_contrast.sum_r = sum_r;
	layer->filter_data.bright_contrast.sum_g = sum_g;
	layer->filter_data.bright_contrast.sum_b = sum_b;
	layer->filter_data.bright_contrast.sum_a = sum_a;
}

/*************************************************
* ReleaseAdjustmentLayerChangeBrightContrast関数 *
* 明るさ・コントラスト調整のメモリを開放         *
* 引数                                           *
* layer	: 調整レイヤーの情報                     *
*************************************************/
static void ReleaseAdjustmentLayerChangeBrightContrast(ADJUSTMENT_LAYER* layer)
{
	MEM_FREE_FUNC(layer->filter_data.bright_contrast.pixels);
	layer->filter_data.bright_contrast.pixels = NULL;
}

/****************************************************
* UpdateAdjustmentLayerChangeBrightContrast関数     *
* 引数                                              *
* layer		: 調整レイヤーの情報                    *
* target	: 調整レイヤーの下のレイヤー            *
* mixed		: 調整レイヤーより下を合成したレイヤー  *
* start_x	: 更新領域の左上のX座標                 *
* start_y	: 更新領域の左上のY座標                 *
* width		: 更新領域の幅                          *
* height	: 更新領域の高さ                        *
****************************************************/
static void UpdateAdjustmentLayerChangeBrightContrast(
	ADJUSTMENT_LAYER* layer,
	LAYER* target,
	LAYER* mixed,
	int start_x,
	int start_y,
	int width,
	int height
)
{
	uint8 *pixels;
	int i, j;
	
	if(layer->target_layer == NULL)
	{
		return;
	}

	switch(layer->target)
	{
	case ADJUSTMENT_LAYER_TARGET_UNDER_LAYER:
		pixels = target->pixels;
		break;
	case ADJUSTMENT_LAYER_TARGET_UNDER_MIXED:
		pixels = mixed->pixels;
		break;
	}

#ifdef _OPENMP
#pragma omp parallel for firstprivate(layer, target, width, height, pixels)
#endif
	for(i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			uint8 *filter_pixel = &layer->filter_data.bright_contrast.pixels[(i+start_y)*target->stride+(j+start_x)*4];
			uint8 *p = &pixels[(i+start_y)*target->stride+(j+start_x)*4];
			layer->filter_data.bright_contrast.sum_r -= filter_pixel[0] * filter_pixel[3];
			layer->filter_data.bright_contrast.sum_r += p[0] * p[3];
			filter_pixel[0] = p[0];
			layer->filter_data.bright_contrast.sum_g -= filter_pixel[1] * filter_pixel[3];
			layer->filter_data.bright_contrast.sum_g += p[1] * p[3];
			filter_pixel[1] = p[1];
			layer->filter_data.bright_contrast.sum_b -= filter_pixel[2] * filter_pixel[3];
			layer->filter_data.bright_contrast.sum_b += p[2] * p[3];
			filter_pixel[2] = p[2];
			layer->filter_data.bright_contrast.sum_a -= filter_pixel[3];
			layer->filter_data.bright_contrast.sum_a += p[3];
			filter_pixel[3] = p[3];
		}
	}
}

/******************************************
* AdjustmentLayerChangeBrightContrast関数 *
* 明るさ・コントラスト調整                *
* 引数                                    *
* layer		: 調整レイヤーの情報          *
* input		: 入力                        *
* output	: 出力                        *
* length	: 操作するピクセルの数        *
* target	: 対象のレイヤー              *
******************************************/
static void AdjustmentLayerChangeBrightContrast(
	ADJUSTMENT_LAYER* layer,
	uint8* input,
	uint8* output,
	unsigned int length,
	LAYER* target
)
{
	uint8 *pixels;
	uint8 *in = input;
	uint8 *out = output;
	double a = tan((((double)((int)layer->filter_data.bright_contrast.contrast + 127) / 255.0) * 90.0) * G_PI / 180.0);
	double average_color[3] =
	{
		(double)layer->filter_data.bright_contrast.sum_r / layer->filter_data.bright_contrast.sum_a,
		(double)layer->filter_data.bright_contrast.sum_g / layer->filter_data.bright_contrast.sum_a,
		(double)layer->filter_data.bright_contrast.sum_b / layer->filter_data.bright_contrast.sum_a
	};
	double intercept[3] =
	{
		average_color[0] * (1 - a),
		average_color[1] * (1 - a),
		average_color[2] * (1 - a)
	};
	uint8 contrast_r[UCHAR_MAX+1], contrast_g[UCHAR_MAX+1], contrast_b[UCHAR_MAX+1];
	int i, j;

	pixels = input;

#ifdef _OPENMP
#pragma omp parallel for firstprivate(layer, out, length)
#endif
	for(i=0; i<(int)length; i++)
	{
		int r, g, b;
		uint8 *p;
		uint8 *o;

		p = &pixels[i*4];
		o = &out[i*4];
		r = p[0] + layer->filter_data.bright_contrast.bright;
		g = p[1] + layer->filter_data.bright_contrast.bright;
		b = p[2] + layer->filter_data.bright_contrast.bright;

		if(r < 0)
		{
			r = 0;
		}
		else if(r > UCHAR_MAX)
		{
			r = UCHAR_MAX;
		}

		if(g < 0)
		{
			g = 0;
		}
		else if(g > UCHAR_MAX)
		{
			g = UCHAR_MAX;
		}

		if(b < 0)
		{
			b = 0;
		}
		else if(b > UCHAR_MAX)
		{
			b = UCHAR_MAX;
		}

		o[0] = (uint8)r;
		o[1] = (uint8)g;
		o[2] = (uint8)b;
	}

	if(layer->filter_data.bright_contrast.contrast < CHAR_MAX)
	{
		for(i=0; i<(int)length; i++)
		{
			int r, g, b;
			uint8 *p;

			p = &pixels[i*4];
		
#ifdef _OPENMP
#pragma omp parallel for
#endif
			for(j=0; j<=UCHAR_MAX; j++)
			{
				r = (int)(a * j + intercept[0]);
				g = (int)(a * j + intercept[1]);
				b = (int)(a * j + intercept[2]);

				if(r < 0)
				{
					r = 0;
				}
				else if(r > UCHAR_MAX)
				{
					r = UCHAR_MAX;
				}

				if(g < 0)
				{
					g = 0;
				}
				else if(g > UCHAR_MAX)
				{
					g = UCHAR_MAX;
				}

				if(b < 0)
				{
					b = 0;
				}
				else if(b > UCHAR_MAX)
				{
					b = UCHAR_MAX;
				}

				contrast_r[j] = (uint8)r;
				contrast_g[j] = (uint8)g;
				contrast_b[j] = (uint8)b;
			}
		}
	}
	else
	{
		(void)memset(contrast_r, 0, (int)average_color[0]+1);
		(void)memset(&contrast_r[(int)average_color[0]+1], 0xff, UCHAR_MAX-((int)average_color[0]+1));
		(void)memset(contrast_g, 0, (int)average_color[1]+1);
		(void)memset(&contrast_g[(int)average_color[1]+1], 0xff, UCHAR_MAX-((int)average_color[1]+1));
		(void)memset(contrast_b, 0, (int)average_color[2]+1);
		(void)memset(&contrast_b[(int)average_color[2]+1], 0xff, UCHAR_MAX-((int)average_color[2]+1));
	}

#ifdef _OPENMP
#pragma omp parallel for firstprivate(out, length)
#endif
	for(i=0; i<(int)length; i++)
	{
		uint8 *o = &out[i*4];

		o[0] = contrast_r[o[0]];
		if(o[0] > o[3])
		{
			o[0] = o[3];
		}
		o[1] = contrast_g[o[1]];
		if(o[1] > o[3])
		{
			o[1] = o[3];
		}
		o[2] = contrast_b[o[2]];
		if(o[2] > o[3])
		{
			o[2] = o[3];
		}
	}
}

/****************************************************
* InitializeAdjustmentLyaerChangeHSV関数            *
* HSV調整機能を初期化                               *
* 引数                                              *
* layer		: 調整レイヤーの情報                    *
* target	: 調整レイヤーの下のレイヤー            *
* mixed		: 調整レイヤーより下を合成したレイヤー  *
****************************************************/
static void InitializeAdjustmentLyaerChangeHSV(
	ADJUSTMENT_LAYER* layer,
	LAYER* target,
	LAYER* mixed
)
{
	layer->filter_data.hue_saturation.h = layer->filter_data.hue_saturation.s =
		layer->filter_data.hue_saturation.v = 0;
}

/**************************************
* ReleaseAdjustmentLayerChangeHSV関数 *
* HSV調整のメモリを開放               *
* 引数                                *
* layer	: 調整レイヤーの情報          *
**************************************/
static void ReleaseAdjustmentLayerChangeHSV(ADJUSTMENT_LAYER* layer)
{
}

/****************************************************
* UpdateAdjustmentLayerChangeHSV関数                *
* 引数                                              *
* layer		: 調整レイヤーの情報                    *
* target	: 調整レイヤーの下のレイヤー            *
* mixed		: 調整レイヤーより下を合成したレイヤー  *
* start_x	: 更新領域の左上のX座標                 *
* start_y	: 更新領域の左上のY座標                 *
* width		: 更新領域の幅                          *
* height	: 更新領域の高さ                        *
****************************************************/
static void UpdateAdjustmentLayerChangeHSV(
	ADJUSTMENT_LAYER* layer,
	LAYER* target,
	LAYER* mixed,
	int start_x,
	int start_y,
	int width,
	int height
)
{
}

/***********************************
* AdjustmentLayerChangeHSV関数     *
* HSV調整                          *
* 引数                             *
* layer		: 調整レイヤーの情報   *
* input		: 入力                 *
* output	: 出力                 *
* length	: 操作するピクセルの数 *
* target	: 対象のレイヤー       *
***********************************/
static void AdjustmentLayerChangeHSV(
	ADJUSTMENT_LAYER* layer,
	uint8* input,
	uint8* output,
	unsigned int length,
	LAYER* target
)
{
	uint8 *pixels = input;
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	// uint8 r;
#endif
	int i;

#ifdef _OPENMP
#pragma omp parallel for firstprivate(layer, input, length, pixels)
#endif
	for(i=0; i<(int)length; i++)
	{
		HSV hsv;
		int h, s, v;
		uint8 *p = &pixels[i*4];
		uint8 rgb[3];

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
		rgb[0] = p[2];
		rgb[1] = p[1];
		rgb[2] = p[0];
#else
		rgb[0] = p[0];
		rgb[1] = p[1];
		rgb[2] = p[2];
#endif
		p[3] = input[i*4+3];
		RGB2HSV_Pixel(rgb, &hsv);

		h = hsv.h + layer->filter_data.hue_saturation.h;
		s = hsv.s + layer->filter_data.hue_saturation.s;
		v = hsv.v + layer->filter_data.hue_saturation.v;

		if(h < 0)
		{
			h = 360 - h % 360;
		}
		else if(h > 360)
		{
			h = h % 360;
		}
		if(s < 0)
		{
			s = 0;
		}
		else if(s > UCHAR_MAX)
		{
			s = UCHAR_MAX;
		}
		if(v < 0)
		{
			v = 0;
		}
		else if(v > UCHAR_MAX)
		{
			v = UCHAR_MAX;
		}

		hsv.h = (int16)h;
		hsv.s = (uint8)s;
		hsv.v = (uint8)v;

		HSV2RGB_Pixel(&hsv, rgb);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
		p[0] = rgb[2];
		p[1] = rgb[1];
		p[2] = rgb[0];
#else
		p[0] = rgb[0];
		p[1] = rgb[1];
		p[2] = rgb[2];
#endif
	}
}

/********************************************************************************
* SetAdjustmentLayerType                                                        *
* 調整レイヤーのモードを設定                                                    *
* 引数                                                                          *
* layer			: 調整レイヤー                                                  *
* type			; 調整レイヤーのタイプ                                          *
********************************************************************************/
void SetAdjustmentLayerType(
	ADJUSTMENT_LAYER* layer,
	eADJUSTMENT_LAYER_TYPE type
)
{
	if(layer->release != NULL)
	{
		layer->release(layer);
	}

	layer->type = type;
	switch(type)
	{
	case ADJUSTMENT_LAYER_TYPE_BRIGHT_CONTRAST:
		layer->initialize = (void (*)(ADJUSTMENT_LAYER*, LAYER*, LAYER*))InitializeAdjustmentLyaerChangeBrightContrast;
		layer->release = (void (*)(ADJUSTMENT_LAYER*))ReleaseAdjustmentLayerChangeBrightContrast;
		layer->update =
			(void (*)(ADJUSTMENT_LAYER*, LAYER*, LAYER*, int, int, int, int))UpdateAdjustmentLayerChangeBrightContrast;
		layer->filter_func =
			(void (*)(ADJUSTMENT_LAYER*, uint8*, uint8*, unsigned int, LAYER*))AdjustmentLayerChangeBrightContrast;
		break;
	case ADJUSTMENT_LAYER_TYPE_HUE_SATURATION:
		layer->initialize = (void (*)(ADJUSTMENT_LAYER*, LAYER*, LAYER*))InitializeAdjustmentLyaerChangeHSV;
		layer->release = (void (*)(ADJUSTMENT_LAYER*))ReleaseAdjustmentLayerChangeHSV;
		layer->update =
			(void (*)(ADJUSTMENT_LAYER*, LAYER*, LAYER*, int, int, int, int))UpdateAdjustmentLayerChangeHSV;
		layer->filter_func =
			(void (*)(ADJUSTMENT_LAYER*, uint8*, uint8*, unsigned int, LAYER*))AdjustmentLayerChangeHSV;
		break;
	}

	layer->initialize(layer, layer->target_layer, layer->target_layer->window->mixed_layer);
	layer->update(layer, layer->target_layer, layer->target_layer->window->mixed_layer,
		0, 0, layer->target_layer->width, layer->target_layer->height);
}

/****************************************************************************
* SetAdjustmentLayerTarget関数                                              *
* 調整レイヤーを適用する相手を設定                                          *
* layer		: 調整レイヤー                                                  *
* target	: 調整レイヤーが適用する相手(直下のレイヤー/下のレイヤーの統合) *
****************************************************************************/
void SetAdjustmentLayerTarget(ADJUSTMENT_LAYER* layer, eADJUSTMENT_LAYER_TARGET target)
{
	layer->target = (uint8)target;
	layer->update(layer, layer->target_layer, layer->target_layer->window->mixed_layer,
		0, 0, layer->target_layer->width, layer->target_layer->height);
}

/*************************************************
* SetAdjustmentLayerTargetLayer関数              *
* 調整レイヤーを適用するレイヤーを設定           *
* layer			: 調整レイヤー                   *
* target_layer	: 調整レイヤーを適用するレイヤー *
*************************************************/
void SetAdjustmentLayerTargetLayer(ADJUSTMENT_LAYER* layer, LAYER* target_layer)
{
	layer->target_layer = target_layer;
	layer->update(layer, layer->target_layer, layer->target_layer->window->mixed_layer,
		0, 0, layer->target_layer->width, layer->target_layer->height);
}

/********************************************************************************
* CreateAdjutmentLayer関数                                                      *
* 調整レイヤーのメモリ確保と初期化                                              *
* 引数                                                                          *
* type			: 調整レイヤーのモード                                          *
* target		: 調整レイヤーが適用する相手(直下のレイヤー/下のレイヤーの統合) *
* target_layer	: 調整レイヤーを適用するレイヤー                                *
* self			: 調整レイヤー自身のアドレス                                    *
* 返り値                                                                        *
*	初期化された構造体のアドレス                                                *
********************************************************************************/
ADJUSTMENT_LAYER* CreateAdjustmentLayer(
	eADJUSTMENT_LAYER_TYPE type,
	eADJUSTMENT_LAYER_TARGET target,
	LAYER* target_layer,
	LAYER* self
)
{
	ADJUSTMENT_LAYER *ret = (ADJUSTMENT_LAYER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->target = (uint8)target;
	ret->target_layer = target_layer;
	ret->type = ADJUSTMENT_LAYER_TYPE_BRIGHT_CONTRAST;
	ret->self = self;

	SetAdjustmentLayerType(ret, type);

	return ret;
}

/*********************************
* ReleaseAdjustmentLayer関数     *
* 調整レイヤーのメモリを開放する *
* 引数                           *
* layer	: 調整レイヤー           *
*********************************/
void ReleaseAdjustmentLayer(ADJUSTMENT_LAYER** layer)
{
	(*layer)->release(*layer);
	MEM_FREE_FUNC(*layer);
	*layer = NULL;
}

/****************************
* DeleteAdjustmentLayer関数 *
* 調整レイヤーを削除する    *
* 引数                      *
* layer	: 調整レイヤー      *
****************************/
void DeleteAdjustmentLayer(LAYER* layer)
{
	ReleaseAdjustmentLayer(&layer->layer_data.adjustment_layer_p);
}

/*****************************************
* ReadAdjustmentLayerData関数            *
* 調整レイヤーのデータを読み込む         *
* 引数                                   *
* stream	: 読み込み元のデータ         *
* read_func	: 読み込みに使う関数ポインタ *
* layer		: 読み込み先の調整レイヤー   *
*****************************************/
void ReadAdjustmentLayerData(void* stream, stream_func_t read_func, ADJUSTMENT_LAYER* layer)
{
	int32 read_data;
	(void)read_func(&layer->target, sizeof(layer->target), 1, stream);
	(void)read_func(&layer->type, sizeof(layer->target), 1, stream);

	switch(layer->type)
	{
	case ADJUSTMENT_LAYER_TYPE_BRIGHT_CONTRAST:
		(void)read_func(&read_data, sizeof(read_data), 1, stream);
		layer->filter_data.bright_contrast.bright = read_data;
		(void)read_func(&read_data, sizeof(read_data), 1, stream);
		layer->filter_data.bright_contrast.contrast = read_data;
		break;
	case ADJUSTMENT_LAYER_TYPE_HUE_SATURATION:
		(void)read_func(&read_data, sizeof(read_data), 1, stream);
		layer->filter_data.hue_saturation.h = read_data;
		(void)read_func(&read_data, sizeof(read_data), 1, stream);
		layer->filter_data.hue_saturation.s = read_data;
		(void)read_func(&read_data, sizeof(read_data), 1, stream);
		layer->filter_data.hue_saturation.v = read_data;
		break;
	}
}

/*********************************************
* WriteAdjustmentLayerData関数               *
* 調整レイヤーのデータを書き出す             *
* 引数                                       *
* stream		: 書き出し先のポインタ       *
* write_func	: 書き出しに使う関数ポインタ *
* layer			: 書き出す調整レイヤー       *
*********************************************/
void WriteAdjustmentLayerData(void* stream, stream_func_t write_func, ADJUSTMENT_LAYER* layer)
{
	int32 write_data;
	(void)write_func(&layer->target, sizeof(layer->target), 1, stream);
	(void)write_func(&layer->type, sizeof(layer->type), 1, stream);

	switch(layer->type)
	{
	case ADJUSTMENT_LAYER_TYPE_BRIGHT_CONTRAST:
		write_data = layer->filter_data.bright_contrast.bright;
		(void)write_func(&write_data, sizeof(write_data), 1, stream);
		write_data = layer->filter_data.bright_contrast.contrast;
		(void)write_func(&write_data, sizeof(write_data), 1, stream);
		break;
	case ADJUSTMENT_LAYER_TYPE_HUE_SATURATION:
		write_data = layer->filter_data.hue_saturation.h;
		(void)write_func(&write_data, sizeof(write_data), 1, stream);
		write_data = layer->filter_data.hue_saturation.s;
		(void)write_func(&write_data, sizeof(write_data), 1, stream);
		write_data = layer->filter_data.hue_saturation.v;
		(void)write_func(&write_data, sizeof(write_data), 1, stream);
		break;
	}
}

#ifdef __cplusplus
}
#endif
