#ifndef _INCLUDED_SELECTION_AREA_H_
#define _INCLUDED_SELECTION_AREA_H_

#include <gtk/gtk.h>
#include "layer.h"
#include "types.h"
#include "configure.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eSELECT_FUZZY_DIRECTION
{
	FUZZY_SELECT_DIRECTION_QUAD,
	FUZZY_SELECT_DIRECTION_OCT
} eSELECT_FUZZY_DIRECTION;

typedef struct _SELECTION_SEGMENT_POINT
{
	int32 x, y;
} SELECTION_SEGMENT_POINT;

typedef struct _SELECTION_SEGMENT
{
	SELECTION_SEGMENT_POINT* points;
	int32 num_points;
} SELECTION_SEGMENT;

typedef struct _SELECTION_AREA
{
	int32 min_x, min_y, max_x, max_y;
#ifdef OLD_SELECTION_AREA
	int32 num_area;
	int16 index;
	SELECTION_SEGMENT* area_data;
#else
	cairo_surface_t *surface_p;
	int stride;
	int index;
	uint8 *pixels;
#endif
} SELECTION_AREA;

// 選択範囲編集用の関数ポインタ配列
extern void (*g_blend_selection_funcs[])(LAYER* work, LAYER* selection);

typedef enum _eSELECTION_BLEND_MODE
{
	SELECTION_BLEND_NORMAL,
	SELECTION_BLEND_SOURCE,
	SELECTION_BLEND_COPY,
	SELECTION_BLEND_MINUS
} eSELECTION_BLEND_MODE;

// 関数のプロトタイプ宣言
extern gboolean UpdateSelectionArea(SELECTION_AREA* area, LAYER* selection, LAYER* temp);

/***************************************************************
* LaplacianFilter関数                                          *
* ラプラシアンフィルタでグレースケール情報からエッジを抽出する *
* 引数                                                         *
* pixels		: グレースケールのイメージ                     *
* width			: 画像の幅                                     *
* height		: 画像の高さ                                   *
* stride		: 1行分のバイト数                              *
* write_buff	: エッジデータを書き出すバッファ               *
***************************************************************/
extern void LaplacianFilter(
	uint8* pixels,
	int width,
	int height,
	int stride,
	uint8* write_buff
);

extern void DrawSelectionArea(
	SELECTION_AREA* area,
	struct _DRAW_WINDOW* window
);

extern void AddSelectionAreaChangeHistory(
	struct _DRAW_WINDOW* window,
	const gchar* tool_name,
	int32 min_x,
	int32 min_y,
	int32 max_x,
	int32 max_y
);

extern void DetectSameColorArea(
	LAYER* target,
	uint8* buff,
	uint8* temp_buff,
	int32 start_x,
	int32 start_y,
	uint8 *color,
	uint8 channel,
	int16 threshold,
	int32* min_x,
	int32* min_y,
	int32* max_x,
	int32* max_y,
	eSELECT_FUZZY_DIRECTION direction
);

/*****************************************
* AddSelectionAreaByColor関数            *
* 色域選択を実行                         *
* 引数                                   *
* target	: 色比較を行うレイヤー       *
* buff		: 選択範囲を記憶するバッファ *
* color		: 選択する色                 *
* threshold	: 選択判断の閾値             *
* min_x		: 選択範囲の最小のX座標      *
* min_y		: 選択範囲の最小のY座標      *
* max_x		: 選択範囲の最大のX座標      *
* max_y		: 選択範囲の最大のY座標      *
* 返り値                                 *
*	選択範囲有り:1 選択範囲無し:0        *
*****************************************/
extern int AddSelectionAreaByColor(
	LAYER* target,
	uint8* buff,
	uint8* color,
	int channel,
	int32 threshold,
	int32* min_x,
	int32* min_y,
	int32* max_x,
	int32* max_y
);

/*****************************************
* ExtendSelectionAreaOneStep関数         *
* 選択範囲を1ピクセル拡大する            *
* 引数                                   *
* select	: 選択範囲を管理するレイヤー *
* temp		: 一時保存用のレイヤー       *
*****************************************/
extern void ExtendSelectionAreaOneStep(LAYER* select, LAYER* temp);

/*****************************************
* RecuctSelectionAreaOneStep関数         *
* 選択範囲を1ピクセル縮小する            *
* 引数                                   *
* select	: 選択範囲を管理するレイヤー *
* temp		: 一時保存用のレイヤー       *
*****************************************/
extern void ReductSelectionAreaOneStep(LAYER* select, LAYER* temp);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_SELECTION_AREA_H_
