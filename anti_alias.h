#ifndef _INCLUDED_ANTI_ALIAS_H_
#define _INCLUDED_ANTI_ALIAS_H_

#include "types.h"
#include "layer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************
* ANTI_ALIAS_RECTANGLE構造体   *
* アンチエイリアスの範囲指定用 *
*******************************/
typedef struct _ANTI_ALIAS_RECTANGLE
{
	int x, y;			// アンチエイリアスをかける左上の座標
	int width, height;	// アンチエイリアスをかける範囲の幅・高さ
} ANTI_ALIAS_RECTANGLE;

// 関数のプロトタイプ宣言
/***************************************************
* OldAntiAlias関数                                 *
* アンチエイリアシング処理を実行                   *
* 引数                                             *
* in_buff	: 入力データ                           *
* out_buff	: 出力データ                           *
* width		: 入出力データの幅                     *
* height	: 入出力データの高さ                   *
* stride	: 入出力データの1行分のバイト数        *
* app		: アプリケーション全体を管理する構造体 *
***************************************************/
EXTERN void OldAntiAlias(
	uint8* in_buff,
	uint8* out_buff,
	int width,
	int height,
	int stride,
	void* app
);

/*******************************************************
* OldAntiAliasEditSelection関数                        *
* クイックマスク中のアンチエイリアシング処理を実行     *
* 引数                                                 *
* in_buff		: 入力データ                           *
* out_buff		: 出力データ                           *
* width			: 入出力データの幅                     *
* height		: 入出力データの高さ                   *
* stride		: 入出力データの1行分のバイト数        *
* application	: アプリケーション全体を管理する構造体 *
*******************************************************/
EXTERN void OldAntiAliasEditSelection(
	uint8* in_buff,
	uint8* out_buff,
	int width,
	int height,
	int stride,
	void* application
);

/*************************************************************
* OldAntiAliasWithSelectionOrAlphaLock関数                   *
* アンチエイリアシング処理を実行(選択範囲または透明度保護有) *
* 引数                                                       *
* selection_layer	: 選択範囲                               *
* target_layer		: 透明度保護に対象レイヤー               *
* in_buff			: 入力データ                             *
* out_buff			: 出力データ                             *
* start_x			: アンチエイリアスをかける左上のX座標    *
* start_y			: アンチエイリアスをかける左上のY座標    *
* width				: 入出力データの幅                       *
* height			: 入出力データの高さ                     *
* stride			: 入出力データの1行分のバイト数          *
* application		: アプリケーション全体を管理する構造体   *
*************************************************************/
EXTERN void OldAntiAliasWithSelectionOrAlphaLock(
	LAYER* selection_layer,
	LAYER* target_layer,
	uint8* in_buff,
	uint8* out_buff,
	int start_x,
	int start_y,
	int width,
	int height,
	int stride,
	void* application
);

/*********************************************************************
* AntiAliasLayer関数                                                 *
* レイヤーに対して範囲を指定してアンチエイリアスをかける             *
* layer			: アンチエイリアスをかけるレイヤー                   *
* out			: 結果を入れるレイヤー                               *
* rect			: アンチエイリアスをかける範囲                       *
* applicatoion	: アプリケーション全体を管理する構造体管理する構造体 *
*********************************************************************/
EXTERN void AntiAliasLayer(LAYER *layer, LAYER* out, ANTI_ALIAS_RECTANGLE *rect, void* application);

/*************************************************************************************
* AntiAliasLayerWithSelectionOrAlphaLock関数                                         *
* レイヤーに対して範囲を指定してアンチエイリアスをかける(選択範囲または透明度保護有) *
* layer			: アンチエイリアスをかけるレイヤー                                   *
* out			: 結果を入れるレイヤー                                               *
* selection		: 選択範囲                                                           *
* target_layer	: アクティブなレイヤー                                               *
* rect			: アンチエイリアスをかける範囲                                       *
* applicatoion	: アプリケーション全体を管理する構造体管理する構造体                 *
*************************************************************************************/
EXTERN void AntiAliasLayerWithSelectionOrAlphaLock(
	LAYER *layer,
	LAYER* out,
	LAYER* selection,
	LAYER* target_layer,
	ANTI_ALIAS_RECTANGLE *rect,
	void* application
);

/*********************************************************************
* OldAntiAliasLayer関数                                              *
* レイヤーに対して範囲を指定してアンチエイリアスをかける             *
* layer			: アンチエイリアスをかけるレイヤー                   *
* rect			: アンチエイリアスをかける範囲                       *
* applicatoion	: アプリケーション全体を管理する構造体管理する構造体 *
*********************************************************************/
EXTERN void OldAntiAliasLayer(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect, void* application);

/*************************************************************************************
* OldAntiAliasLayerWithSelectionOrAlphaLock関数                                      *
* レイヤーに対して範囲を指定してアンチエイリアスをかける(選択範囲または透明度保護有) *
* layer			: アンチエイリアスをかけるレイヤー                                   *
* selection		: 選択範囲                                                           *
* target_layer	: アクティブなレイヤー                                               *
* rect			: アンチエイリアスをかける範囲                                       *
* applicatoion	: アプリケーション全体を管理する構造体管理する構造体                 *
*************************************************************************************/
EXTERN void OldAntiAliasLayerWithSelectionOrAlphaLock(
	LAYER *layer,
	LAYER* temp,
	LAYER* selection,
	LAYER* target_layer,
	ANTI_ALIAS_RECTANGLE *rect,
	void* application
);

/*********************************************************************
* AntiAliasVectorLine関数                                            *
* ベクトルレイヤーの線に対して範囲を指定してアンチエイリアスをかける *
* layer	: アンチエイリアスをかけるレイヤー                           *
* rect	: アンチエイリアスをかける範囲                               *
*********************************************************************/
EXTERN void AntiAliasVectorLine(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_ANTI_ALIAS_H_
