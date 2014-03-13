#ifndef _INCLUDED_TEXT_LAYER_H_
#define _INCLUDED_TEXT_LAYER_H_

#include <gtk/gtk.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eTEXT_STYLE
{
	TEXT_STYLE_NORMAL,
	TEXT_STYLE_ITALIC,
	TEXT_STYLE_OBLIQUE
} eTEXT_STYLE;

/*********************************
* eTEXT_LAYER_FLAGS列挙体        *
* テキストレイヤーの表示用フラグ *
*********************************/
typedef enum _eTEXT_LAYER_FLAGS
{
	TEXT_LAYER_VERTICAL = 0x01,	// 縦書き
	TEXT_LAYER_BOLD = 0x02,		// 太字
	TEXT_LAYER_ITALIC = 0x04,	// イタリック体
	TEXT_LAYER_OBLIQUE = 0x08	// 斜体
} eTEXT_LAYER_FLAGS;

/*******************************
* TEXT_LAYER構造体             *
* テキストレイヤーの情報を格納 *
*******************************/
typedef struct _TEXT_LAYER
{
	GtkWidget* text_field;	// テキストビューウィジェット
	GtkTextBuffer* buffer;	// 描画テキストバッファ
	char* text;				// バッファから取り出したテキスト情報
	gdouble x, y;			// テキスト表示領域左上の座標
	gdouble width, height;	// テキスト表示領域の幅、高さ
	// ドラッグの開始座標
	gdouble drag_start_x, drag_start_y;
	int base_size;			// フォントサイズの倍率
	gdouble font_size;		// 表示テキストのサイズ
	int32 font_id;			// フォントファイル名のID
	uint8 color[3];			// 表示文字色
	uint32 flags;			// 縦書き、太字等のフラグ
} TEXT_LAYER;

/*****************************************
* TEXT_LAYER_BASE_DATA構造体             *
* テキストレイヤーの基本情報(書き出し用) *
*****************************************/
typedef struct _TEXT_LAYER_BASE_DATA
{
	gdouble x, y;			// テキスト表示領域左上の座標
	gdouble width, height;	// テキスト表示領域の幅、高さ
	gdouble font_size;		// 表示テキストのサイズ
	uint8 base_size;		// 文字サイズの倍率
	uint8 color[3];			// 表示文字色
	guint32 flags;			// 縦書き、太字等のフラグ
} TEXT_LAYER_BASE_DATA;

/*********************************************
* CreateTextLayer関数                        *
* テキストレイヤーのデータメモリ確保と初期化 *
* 引数                                       *
* x			: レイヤーのX座標                *
* y			: レイヤーのY座標                *
* width		: レイヤーの幅                   *
* height	: レイヤーの高さ                 *
* base_size	: 文字サイズの倍率               *
* font_size	: 文字サイズ                     *
* color		: 文字色                         *
* flags		: テキスト表示のフラグ           *
* 返り値                                     *
*	初期化された構造体のアドレス             *
*********************************************/
extern TEXT_LAYER* CreateTextLayer(
	gdouble x,
	gdouble y,
	gdouble width,
	gdouble height,
	int base_size,
	gdouble font_size,
	int32 font_id,
	uint8 color[3],
	uint32 flags
);

/*****************************************************
* DeleteTextLayer関数                                *
* テキストレイヤーのデータを削除する                 *
* 引数                                               *
* layer	: テキストレイヤーのデータポインタのアドレス *
*****************************************************/
extern void DeleteTextLayer(TEXT_LAYER** layer);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_TEXT_LAYER_H_
