#ifndef _INCLUDED_LAYER_H_
#define _INCLUDED_LAYER_H_

#define MAX_LAYER_NAME_LENGTH 256

#include <gtk/gtk.h>
#include <cairo.h>
#include "configure.h"
#include "types.h"
#include "vector_layer.h"
#include "text_layer.h"
#include "layer_set.h"
#include "pattern.h"
#include "texture.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LAYER_CHAIN_BUFFER_SIZE 1024
#define MAX_LAYER_EXTRA_DATA_NUM 8

typedef enum _eLAYER_FLAGS
{
	LAYER_FLAG_INVISIBLE = 0x01,
	LAYER_MASKING_WITH_UNDER_LAYER = 0x02,
	LAYER_LOCK_OPACITY = 0x04,
	LAYER_CHAINED = 0x08,
	LAYER_SET_CLOSE = 0x10
} eLAYER_FLAGS;

typedef enum _eLAYER_TYPE
{
	TYPE_NORMAL_LAYER,
	TYPE_VECTOR_LAYER,
	TYPE_TEXT_LAYER,
	TYPE_LAYER_SET,
	TYPE_3D_LAYER
} eLAYER_TYPE;

typedef enum _eLAYER_BLEND_MODE
{
	LAYER_BLEND_NORMAL,
	LAYER_BLEND_ADD,
	LAYER_BLEND_MULTIPLY,
	LAYER_BLEND_SCREEN,
	LAYER_BLEND_OVERLAY,
	LAYER_BLEND_LIGHTEN,
	LAYER_BLEND_DARKEN,
	LAYER_BLEND_DODGE,
	LAYER_BLEND_BURN,
	LAYER_BLEND_HARD_LIGHT,
	LAYER_BLEND_SOFT_LIGHT,
	LAYER_BLEND_DIFFERENCE,
	LAYER_BLEND_EXCLUSION,
	LAYER_BLEND_HSL_HUE,
	LAYER_BLEND_HSL_SATURATION,
	LAYER_BLEND_HSL_COLOR,
	LAYER_BLEND_HSL_LUMINOSITY,
	LAYER_BLEND_BINALIZE,
	LAYER_BLEND_SLELECTABLE_NUM,
	LAYER_BLEND_COLOR_REVERSE = LAYER_BLEND_SLELECTABLE_NUM,
	LAYER_BLEND_GREATER,
	LAYER_BLEND_ALPHA_MINUS,
	LAYER_BLEND_SOURCE,
	LAYER_BLEND_ATOP,
	LAYER_BLEND_SOURCE_OVER,
	LAYER_BLEND_OVER
} eLAYER_BLEND_MODE;

/***************************
* EXTRA_LAYER_DATA構造体   *
* レイヤーの追加情報を格納 *
***************************/
typedef struct _EXTRA_LAYER_DATA
{
	char *name;
	size_t data_size;
	void *data;
} EXTRA_LAYER_DATA;

/***********************
* LAYER構造体          *
* レイヤーの情報を格納 *
***********************/
typedef struct _LAYER
{
	uint8 *pixels;			// ピクセルデータ
	gchar* name;			// レイヤー名
	uint16 layer_type;		// レイヤータイプ(ノーマル、ベクトル、テキスト)
	uint16 layer_mode;		// レイヤーの合成モード
	int32 x, y;				// レイヤー左上隅の座標
	// レイヤーの幅、高さ、一行分のバイト数
	int width, height, stride;
	unsigned int flags;		// レイヤー表示・非表示等のフラグ
	int8 alpha;				// レイヤーの不透明度
	int8 channel;			// レイヤーのチャンネル数
	uint8 num_extra_data;	// 追加情報の数

	// CAIROの書き込みサーフェイス
	cairo_surface_t *surface_p;
	cairo_t *cairo_p;	// CAIROコンテキスト

	// layer_data共用体
		// ベクトルレイヤー、テキストレイヤーの情報を格納
	union
	{
		VECTOR_LAYER* vector_layer_p;
		TEXT_LAYER* text_layer_p;
		LAYER_SET* layer_set_p;

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
		// 3Dモデリング用データ
		void *project;
#endif
	} layer_data;

	// 前後のレイヤーへのポインタ
	struct _LAYER *prev, *next;
	// レイヤーセットへのポインタ
	struct _LAYER *layer_set;
	// レイヤービューのウィジェット
	struct _LAYER_WIDGET *widget;

	// 追加情報
	EXTRA_LAYER_DATA extra_data[MAX_LAYER_EXTRA_DATA_NUM];

	// 描画領域へのポインタ
	struct _DRAW_WINDOW *window;
} LAYER;

/*************************************
* LAYER_BASE_DATA構造体              *
* レイヤーの基本情報。書き出しに使用 *
*************************************/
typedef struct _LAYER_BASE_DATA
{
	uint16 layer_type;	// レイヤータイプ(ノーマル、ベクトル、テキスト)
	uint16 layer_mode;	// レイヤーの合成モード
	gint32 x, y;		// レイヤー左上隅の座標
	// レイヤーの幅、高さ
	gint32 width, height;
	guint32 flags;		// レイヤー表示・非表示等のフラグ
	int8 alpha;			// レイヤーの不透明度
	int8 channel;		// レイヤーのチャンネル数
	int8 layer_set;		// レイヤーセット内のレイヤーか否か
} LAYER_BASE_DATA;

/********************
* RGAB_DATA構造体   *
* 1ピクセルのデータ *
********************/
typedef struct _RGBA_DATA
{
	uint8 r, g, b, a;
} RGBA_DATA;

// 関数のプロトタイプ宣言
extern LAYER* CreateLayer(
	int32 x,
	int32 y,
	int32 width,
	int32 height,
	int8 channel,
	eLAYER_TYPE layer_type,
	LAYER* prev_layer,
	LAYER* next_layer,
	const char *name,
	struct _DRAW_WINDOW* window
);

/*****************************************************
* CreateDispTempLayer関数                            *
* 表示用一時保存レイヤーを作成                       *
* 引数                                               *
* x				: レイヤー左上のX座標                *
* y				: レイヤー左上のY座標                *
* width			: レイヤーの幅                       *
* height		: レイヤーの高さ                     *
* channel		: レイヤーのチャンネル数             *
* layer_type	: レイヤーのタイプ                   *
* window		: 追加するレイヤーを管理する描画領域 *
* 返り値                                             *
*	初期化された構造体のアドレス                     *
*****************************************************/
LAYER* CreateDispTempLayer(
	int32 x,
	int32 y,
	int32 width,
	int32 height,
	int8 channel,
	eLAYER_TYPE layer_type,
	struct _DRAW_WINDOW* window
);

/*************************************
* g_layer_blend_funcs配列            *
* レイヤー合成用関数へのポインタ配列 *
* 引数                               *
* src	: 上側のレイヤー             *
* dst	: 下側のレイヤー             *
*************************************/
extern void (*g_layer_blend_funcs[])(LAYER* src, LAYER* dst);

/*********************************************
* DeleteLayer関数                            *
* レイヤーを削除する                         *
* 引数                                       *
* layer	: 削除するレイヤーポインタのアドレス *
*********************************************/
extern void DeleteLayer(LAYER** layer);

/*********************************************
* DeleteTempLayer関数                        *
* 一時的に作成したレイヤーを削除する         *
* 引数                                       *
* layer	: 削除するレイヤーポインタのアドレス *
*********************************************/
extern void DeleteTempLayer(LAYER** layer);

/*********************************
* AddDeleteLayerHistory関数      *
* レイヤーの削除履歴データを作成 *
* 引数                           *
* window	: 描画領域のデータ   *
* target	: 削除するレイヤー   *
*********************************/
extern void AddDeleteLayerHistory(
	struct _DRAW_WINDOW* window,
	LAYER* target
);

/***********************************************************
* ResizeLayerBuffer関数                                    *
* レイヤーのピクセルデータのサイズを変更する               *
* 引数                                                     *
* target		: ピクセルデータのサイズを変更するレイヤー *
* new_width		: 新しいレイヤーの幅                       *
* new_height	: 新しいレイヤーの高さ                     *
***********************************************************/
extern void ResizeLayerBuffer(
	LAYER* target,
	int32 new_width,
	int32 new_height
);

/*******************************************
* ResizeLayer関数                          *
* レイヤーのサイズを変更する(拡大縮小有)   *
* 引数                                     *
* target		: サイズを変更するレイヤー *
* new_width		: 新しいレイヤーの幅       *
* new_height	: 新しいレイヤーの高さ     *
*******************************************/
extern void ResizeLayer(
	LAYER* target,
	int32 new_width,
	int32 new_height
);

/*******************************************
* ChangeLayerSize関数                      *
* レイヤーのサイズを変更する(拡大縮小無)   *
* 引数                                     *
* target		: サイズを変更するレイヤー *
* new_width		: 新しいレイヤーの幅       *
* new_height	: 新しいレイヤーの高さ     *
*******************************************/
extern void ChangeLayerSize(
	LAYER* target,
	int32 new_width,
	int32 new_height
);

extern int CorrectLayerName(const LAYER* bottom_layer, const char* name);

extern void ChangeActiveLayer(struct _DRAW_WINDOW* window, LAYER* layer);

/*******************************
* LayerMergeDown関数           *
* 下のレイヤーと結合する       *
* 引数                         *
* target	: 結合するレイヤー *
*******************************/
extern void LayerMergeDown(LAYER* target);

/*******************************************
* AddLayerMergeDownHistory関数             *
* 下のレイヤーと結合の履歴データを追加する *
* 引数                                     *
* window	: 描画領域の情報               *
* target	: 結合するレイヤー             *
*******************************************/
extern void AddLayerMergeDownHistory(
	struct _DRAW_WINDOW* window,
	LAYER* target
);

extern void ChangeLayerOrder(LAYER* change_layer, LAYER* new_prev, LAYER** bottom);

extern LAYER* SearchLayer(LAYER* bottom_layer, const gchar* name);

extern int32 GetLayerID(const LAYER* bottom, const LAYER* prev, uint16 num_layer);

/*********************************
* AddDeleteLayerHistory関数      *
* レイヤーの削除履歴データを作成 *
* 引数                           *
* window	: 描画領域のデータ   *
* target	: 削除するレイヤー   *
*********************************/
extern void AddNewLayerHistory(
	const LAYER* new_layer,
	uint16 layer_type
);

/*********************************************
* AddNewLayerWithImageHistory関数            *
* 画像データ付きでのレイヤー追加の履歴追加   *
* 引数                                       *
* new_layer		: 追加したレイヤー           *
* pixels		: 画像データのピクセルデータ *
* width			: 画像データの幅             *
* height		: 画像データの高さ           *
* stride		: 画像データ一行分のバイト数 *
* channel		: 画像データのチャンネル数   *
* history_name	: 履歴の名前                 *
*********************************************/
extern void AddNewLayerWithImageHistory(
	const LAYER* new_layer,
	uint8* pixels,
	int32 width,
	int32 height,
	int32 stride,
	uint8 channel,
	const char* history_name
);

/*************************************************
* AddChangeLayerOrderHistory関数                 *
* レイヤーの順序変更の履歴データを追加           *
* 引数                                           *
* change_layer	: 順序変更したレイヤー           *
* before_prev	: 順序変更前の前のレイヤー       *
* after_prev	: 順序変更後の前のレイヤー       *
* before_parent	: 順序変更前の所属レイヤーセット *
*************************************************/
extern void AddChangeLayerOrderHistory(
	const LAYER* change_layer,
	const LAYER* before_prev,
	const LAYER* after_prev,
	const LAYER* before_parent
);

/***********************************************************
* AddChangeLyaerSetHistory関数                             *
* レイヤーの所属レイヤーセット変更の履歴データを追加       *
* 引数                                                     *
* change_layer	: 所属レイヤーセットを変更するレイヤー     *
* before_parent	: レイヤーセット変更前の所属レイヤーセット *
* after_parent	: レイヤーセット変更後の所属レイヤーセット *
***********************************************************/
extern void AddChangeLayerSetHistory(
	const LAYER* change_layer,
	const LAYER* before_parent,
	const LAYER* after_parent
);

extern void FillLayerColor(LAYER* target, uint8 color[3]);

extern void FlipLayerHorizontally(LAYER* target, LAYER* temp);
extern void FlipLayerVertically(LAYER* target, LAYER* temp);

/***************************************
* RasterizeLayer関数                   *
* レイヤーをラスタライズ               *
* 引数                                 *
* target	: ラスタライズするレイヤー *
***************************************/
extern void RasterizeLayer(LAYER* target);

/*****************************
* CreateLayerCopy関数        *
* レイヤーのコピーを作成する *
* 引数                       *
* src	: コピー元のレイヤー *
* 返り値                     *
*	作成したレイヤー         *
*****************************/
extern LAYER* CreateLayerCopy(LAYER* src);

/*************************************************
* GetLayerChain関数                              *
* ピン留めされたレイヤー配列を取得する           *
* 引数                                           *
* window	: 描画領域の情報                     *
* num_layer	: レイヤー数を格納する変数のアドレス *
* 返り値                                         *
*	レイヤー配列                                 *
*************************************************/
extern LAYER** GetLayerChain(struct _DRAW_WINDOW* window, uint16* num_layer);

/***************************************************
* LayerSetShowChildren関数                         *
* レイヤーセットの子レイヤーを表示する             *
* 引数                                             *
* layer_set	: 子を表示するレイヤーセット           *
* prev		: 関数終了後の次にチェックするレイヤー *
***************************************************/
extern void LayerSetShowChildren(LAYER* layer_set, LAYER **prev);

/***************************************************
* LayerSetHideChildren関数                         *
* レイヤーセットの子レイヤーを非表示する           *
* 引数                                             *
* layer_set	: 子を表示するレイヤーセット           *
* prev		: 関数終了後の次にチェックするレイヤー *
***************************************************/
extern void LayerSetHideChildren(LAYER* layer_set, LAYER **prev);

/***********************************************
* DeleteSelectAreaPixels関数                   *
* 選択範囲のピクセルデータを削除する           *
* 引数                                         *
* window	: 描画領域の情報                   *
* target	: ピクセルデータを削除するレイヤー *
* selection	: 選択範囲を管理するレイヤー       *
***********************************************/
extern void DeleteSelectAreaPixels(
	struct _DRAW_WINDOW* window,
	LAYER* target,
	LAYER* selection
);

/*******************************************************
* AddDeletePixelsHistory関数                           *
* ピクセルデータ消去操作の履歴データを追加する         *
* 引数                                                 *
* tool_name	: ピクセルデータ消去に使用したツールの名前 *
* window	: 描画領域の情報                           *
* min_x		: ピクセルデータのX座標最小値              *
* min_y		: ピクセルデータのY座標最小値              *
* max_x		: ピクセルデータのX座標最大値              *
* max_y		: ピクセルデータのY座標最大値              *
*******************************************************/
extern void AddDeletePixelsHistory(
	const char* tool_name,
	struct _DRAW_WINDOW* window,
	LAYER* target,
	int32 min_x,
	int32 min_y,
	int32 max_x,
	int32 max_y
);

/*****************************************************
* CalcLayerAverageRGBA関数                           *
* レイヤーの平均ピクセルデータ値を計算               *
* 引数                                               *
* target	: 平均ピクセルデータ値を計算するレイヤー *
* 返り値                                             *
*	対象レイヤーの平均ピクセルデータ値               *
*****************************************************/
extern RGBA_DATA CalcLayerAverageRGBA(LAYER* target);

/*****************************************************
* CalcLayerAverageRGBAwithAlpha関数                  *
* α値を考慮してレイヤーの平均ピクセルデータ値を計算 *
* 引数                                               *
* target	: 平均ピクセルデータ値を計算するレイヤー *
* 返り値                                             *
*	対象レイヤーの平均ピクセルデータ値               *
*****************************************************/
extern RGBA_DATA CalcLayerAverageRGBAwithAlpha(LAYER* target);

extern void OnChangeTextCallBack(GtkTextBuffer* buffer, LAYER* layer);

extern VECTOR_LINE_LAYER* CreateVectorLineLayer(
	struct _LAYER* work,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
);

/*****************************************************
* CreateLayerSetChildButton関数                      *
* レイヤーセットの子レイヤー表示・非表示ボタンを作成 *
* 引数                                               *
* layer_set	: ボタン作成対象のレイヤーセット         *
* 返り値                                             *
*	作成したボタンウィジェット                       *
*****************************************************/
extern GtkWidget* CreateLayerSetChildButton(LAYER* layer_set);

/*******************************************************
* FillTextureLayer関数                                 *
* テクスチャレイヤーを選択されたパラメーターで塗り潰す *
* 引数                                                 *
* layer		: テクスチャレイヤー                       *
* textures	: テクスチャのパラメーター                 *
*******************************************************/
extern void FillTextureLayer(LAYER* layer, TEXTURES* textures);

/******************************************
* GetAverageColor関数                     *
* 指定座標周辺の平均色を取得              *
* 引数                                    *
* target	: 色を取得するレイヤー        *
* x			: X座標                       *
* y			: Y座標                       *
* size		: 色を取得する範囲            *
* color		: 取得した色を格納(4バイト分) *
******************************************/
extern void GetAverageColor(LAYER* target, int x, int y, int size, uint8 color[4]);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_LAYER_H_
