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
/********************************************
* AntiAlias関数                             *
* アンチエイリアシング処理を実行            *
* 引数                                      *
* in_buff	: 入力データ                    *
* out_buff	: 出力データ                    *
* width		: 入出力データの幅              *
* height	: 入出力データの高さ            *
* stride	: 入出力データの1行分のバイト数 *
* channel	: 入出力データのチャンネル数    *
********************************************/
extern void AntiAlias(
	uint8* in_buff,
	uint8* out_buff,
	int width,
	int height,
	int stride,
	int channel
);

/*********************************************************
* AntiAliasLayer関数                                     *
* レイヤーに対して範囲を指定してアンチエイリアスをかける *
* layer	: アンチエイリアスをかけるレイヤー               *
* rect	: アンチエイリアスをかける範囲                   *
*********************************************************/
extern void AntiAliasLayer(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect);

/*********************************************************************
* AntiAliasVectorLine関数                                            *
* ベクトルレイヤーの線に対して範囲を指定してアンチエイリアスをかける *
* layer	: アンチエイリアスをかけるレイヤー                           *
* rect	: アンチエイリアスをかける範囲                               *
*********************************************************************/
extern void AntiAliasVectorLine(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_ANTI_ALIAS_H_
