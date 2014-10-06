#ifndef _INCLUDED_FILTER_H_
#define _INCLUDED_FILTER_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*filter_func)(struct _DRAW_WINDOW* window, struct _LAYER** layers,
							uint16 num_layer, void* data);
typedef void (*selection_filter_func)(struct _DRAW_WINDOW* window, void* data);

// フィルター関数配列のインデックス
typedef enum _eFILTER_FUNC_ID
{
	FILTER_FUNC_BLUR,
	FILTER_FUNC_MOTION_BLUR,
	FILTER_FUNC_BRIGHTNESS_CONTRAST,
	FILTER_FUNC_HUE_SATURATION,
	FILTER_FUNC_LUMINOSITY2OPACITY,
	FILTER_FUNC_COLOR2ALPHA,
	FILTER_FUNC_COLORIZE_WITH_UNDER,
	FILTER_FUNC_GRADATION_MAP,
	FILTER_FUNC_FILL_WITH_VECTOR,
	FILTER_FUNC_PERLIN_NOISE,
	FILTER_FUNC_FRACTAL,
	NUM_FILTER_FUNC
} eFILTER_FUNC_ID;

// フィルター関数ポインタ配列
extern filter_func g_filter_funcs[NUM_FILTER_FUNC];
extern selection_filter_func g_selection_filter_funcs[NUM_FILTER_FUNC];

/*****************************************************
* ExecuteBlurFilter関数                              *
* ぼかしフィルタを実行                               *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteBlurFilter(APPLICATION* app);

/*********************************************************
* ApplyBlurFilter関数                                    *
* ぼかしフィルターを適用する                             *
* 引数                                                   *
* target	: ぼかしフィルターを適用するレイヤー         *
* size		: ぼかしフィルターで平均色を計算する矩形範囲 *
*********************************************************/
EXTERN void ApplyBlurFilter(LAYER* target, int size);

/*****************************************************
* ExecuteMotionBlurFilter関数                        *
* モーションぼかしフィルタを実行                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteMotionBlurFilter(APPLICATION* app);

/*****************************************************
* ExecuteChangeBrightContrastFilter関数              *
* 明るさコントラストフィルタを実行                   *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteChangeBrightContrastFilter(APPLICATION* app);

/*****************************************************
* ExecuteChangeHueSaturationFilter関数               *
* 色相・彩度フィルタを実行                           *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteChangeHueSaturationFilter(APPLICATION* app);

/*****************************************************
* ExecuteLuminosity2OpacityFilter関数                *
* 輝度を透明度へ変換を実行                           *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteLuminosity2OpacityFilter(APPLICATION* app);

/*****************************************************
* ExecuteColor2AlphaFilter関数                       *
* 指定色を透明度へ変換を実行                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteColor2AlphaFilter(APPLICATION* app);

/*****************************************************
* ExecuteColorizeWithUnderFilter関数                 *
* 下のレイヤーで着色を実行                           *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteColorizeWithUnderFilter(APPLICATION* app);

/*****************************************************
* ExecuteGradationMapFilter関数                      *
* グラデーションマップを実行                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteGradationMapFilter(APPLICATION* app);

/*****************************************************
* ExecuteFillWithVectorLineFilter関数                *
* ベクトルレイヤーを利用して下のレイヤーを塗り潰す   *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteFillWithVectorLineFilter(APPLICATION* app);

/*****************************************************
* ExecutePerlinNoiseFilter関数                       *
* パーリンノイズで下塗りを実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecutePerlinNoiseFilter(APPLICATION* app);

/*****************************************************
* ExecuteFractal関数                                 *
* フラクタル図形で下塗り実行                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteFractal(APPLICATION* app);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_FILTER_H_
