#ifndef _INCLUDED_FILTER_H_
#define _INCLUDED_FILTER_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

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
* ExecuteGaussianBlurFilter関数                      *
* ガウシアンぼかしフィルタを実行                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteGaussianBlurFilter(APPLICATION* app);

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
* ExecuteColorLevelAdjust関数                        *
* 色のレベル補正フィルターを実行                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteColorLevelAdjust(APPLICATION* app);

/*****************************************************
* ExecuteToneCurve関数                               *
* トーンカーブフィルターを実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteToneCurve(APPLICATION* app);

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
