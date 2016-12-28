#ifndef _INCLUDED_TRACE_H_
#define _INCLUDED_TRACE_H_

#include "types.h"

// ビットマップデータトレースのモード
typedef enum _eBITMAP_TRACE_MODE
{
	BITMAP_TRACE_MODE_GRAY_SCALE,
	BITMAP_TRACE_MODE_BRIGHTNESS,
	BITMAP_TRACE_MODE_COLOR
} eBITMAP_TRACE_MODE;

typedef enum _eBITMAP_TRACE_TURN_POLICY
{
	BITMAP_TRACE_TURN_POLICY_BLACK,
	BITMAP_TRACE_TURN_POLICY_WHITE,
	BITMAP_TRACE_TURN_POLICY_LEFT,
	BITMAP_TRACE_TURN_POLICY_RIGHT,
	BITMAP_TRACE_TURN_POLICY_MINORITY,
	BITMAP_TRACE_TURN_POLICY_MAJORITY,
	BITMAP_TRACE_TURN_POLICY_RANDOM
} eBITMAP_TRACE_TURN_POLICY;

typedef enum _eBITMAP_TRACE_TARGET
{
	BITMAP_TRACE_TARGET_LAYER,
	BITMAP_TRACE_TARGET_CANVAS,
	NUM_BITMAP_TRACE_TARGET
} eBITMAP_TRACE_TARGET;

typedef enum _eBITMAP_TRACE_FALGS
{
	BITMAP_TRACE_FALGS_USE_SPECIFY_LINE_COLOR = 0x01
} eBITMAP_TRACE_FALGS;

typedef struct _BITMAP_TRACER
{
	void *path;
	uint8* trace_pixels;
	int width;
	int height;
	int trace_mode;
	int minimum_path_size;
	int minimum_point_distance;
	int optimize_curve;
	unsigned int flags;
	FLOAT_T optimize_tolerance;
	FLOAT_T alpha_max;
	uint8 trace_target;
	uint8 line_color[4];
	int (*threshold)(struct _BITMAP_TRACER*, uint8*, int, int);
	int (*sign)(struct _BITMAP_TRACER*, uint8*, int, int);
	void (*clear)(struct _BITMAP_TRACER*, uint8*, int, int);
	int (*blur)(struct _BITMAP_TRACER*, uint8*, int, int);
	void (*progress_update)(void*, FLOAT_T);
	eBITMAP_TRACE_TURN_POLICY trace_policy;
	void *progress_data;

	struct
	{
		int black_white_threshold;
		int opacity_threshold;
	} threshold_data;
} BITMAP_TRACER;

// 関数のプロトタイプ宣言
/*****************************************************
* InitializeBitmapTracer関数                         *
* ビットマップデータをトレースするデータを初期化する *
* 引数                                               *
* tracer	: トレーサーのデータ                     *
* pixels	: ビットマップデータ                     *
* width		: ビットマップデータの幅                 *
* height	: ビットマップデータの高さ               *
*****************************************************/
EXTERN void InitializeBitmapTracer(BITMAP_TRACER* tracer, uint8* pixels, int width, int height);

/***************************************************
* ReleaseBitmapTracer関数                          *
* ビットマップデータをトレースするデータを開放する *
* 引数                                             *
* tracer	: トレーサーのデータ                   *
***************************************************/
EXTERN void ReleaseBitmapTracer(BITMAP_TRACER* tracer);

/***********************************************
* TraceBitmap関数                              *
* ビットマップデータをベクトルデータに変換する *
* 引数                                         *
* tracer	: トレーサーのデータ               *
* 返り値                                       *
*	正常終了:TRUE	異常終了:FALSE             *
***********************************************/
EXTERN int TraceBitmap(BITMAP_TRACER* tracer);

/***********************************************
* AdoptTraceBitmapResult関数                   *
* トレースした結果をベクトルレイヤーに適用する *
* 引数                                         *
* tracer	: トレーサーのデータ               *
* layer		: 適用先のベクトルレイヤー         *
***********************************************/
EXTERN void AdoptTraceBitmapResult(BITMAP_TRACER* tracer, void* layer);

#endif	// #ifndef _INCLUDED_TRACE_H_
