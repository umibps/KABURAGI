#ifndef _INCLUDED_BRUSH_CORE_H_
#define _INCLUDED_BRUSH_CORE_H_

#include <gtk/gtk.h>
#include "layer.h"
#include "draw_window.h"
#include "types.h"
#include "configure.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BRUSH_STEP (FLOAT_T)(0.0750)
#define MIN_BRUSH_STEP (FLOAT_T)(BRUSH_STEP * MINIMUM_PRESSURE)
#define BRUSH_UPDATE_MARGIN 7

typedef enum _eBRUSH_SHAPE
{
	BRUSH_SHAPE_CIRCLE,
	BRUSH_SHAPE_IMAGE
} eBRUSH_SHAPE;

typedef enum _eBRUSH_TYPE
{
	BRUSH_TYPE_PENCIL,
	BRUSH_TYPE_HARD_PEN,
	BRUSH_TYPE_AIR_BRUSH,
	BRUSH_TYPE_BLEND_BRUSH,
	BRUSH_TYPE_OLD_AIR_BRUSH,
	BRUSH_TYPE_WATER_COLOR_BRUSH,
	BRUSH_TYPE_PICKER_BRUSH,
	BRUSH_TYPE_ERASER,
	BRUSH_TYPE_BUCKET,
	BRUSH_TYPE_PATTERN_FILL,
	BRUSH_TYPE_BLUR,
	BRUSH_TYPE_SMUDGE,
	BRUSH_TYPE_MIX_BRUSH,
	BRUSH_TYPE_GRADATION,
	BRUSH_TYPE_TEXT,
	BRUSH_TYPE_STAMP_TOOL,
	BRUSH_TYPE_IMAGE_BRUSH,
	BRUSH_TYPE_BLEND_IMAGE_BRUSH,
	BRUSH_TYPE_PICKER_IMAGE_BRUSH,
	BRUSH_TYPE_SCRIPT_BRUSH,
	BRUSH_TYPE_CUSTOM_BRUSH,
	BRUSH_TYPE_PLUG_IN,
	NUM_BRUSH_TYPE
} eBRUSH_TYPE;

typedef void (*brush_core_func)(DRAW_WINDOW* window, FLOAT_T x, FLOAT_T y,
	FLOAT_T pressure, struct _BRUSH_CORE* core, void* state);

typedef void (*brush_update_func)(DRAW_WINDOW* window, FLOAT_T x, FLOAT_T y, void* data);

typedef struct _BRUSH_CORE
{
	struct _APPLICATION *app;
	FLOAT_T max_x, max_y, min_x, min_y;
	uint32 flags;
	size_t detail_data_size;

	cairo_surface_t *brush_surface;
	cairo_surface_t *temp_surface;
	cairo_pattern_t *brush_pattern;
	cairo_pattern_t *temp_pattern;
	cairo_t *temp_cairo;
	uint8 **brush_pattern_buff, **temp_pattern_buff;
	int stride;

	gchar *name;
	char *image_file_path;
	uint8 (*color)[3];
	uint8 (*back_color)[3];
	uint8 brush_type;
	gchar hot_key;

	void* brush_data;
	brush_core_func press_func, motion_func, release_func;
	void (*key_press_func)(DRAW_WINDOW* window, GdkEventKey* key, void* data);
	void (*draw_cursor)(DRAW_WINDOW* window, FLOAT_T x, FLOAT_T y, void* data);
	brush_update_func button_update, motion_update;
	GtkWidget* (*create_detail_ui)(struct _APPLICATION *app, struct _BRUSH_CORE* core);
	void (*color_change)(const uint8 color[3], void* data);
	void (*change_editting_selection)(void* data, int is_editting);
	GtkWidget *button;
} BRUSH_CORE;

typedef struct _BRUSH_UPDATE_INFO
{
	// 更新を行う範囲
	FLOAT_T min_x, min_y, max_x, max_y;
	// 更新範囲の左上の座標
	int start_x, start_y;
	// 更新範囲の幅、高さ
	FLOAT_T width, height;
	// 描画を実行するか否か
	int update;
} BRUSH_UPDATE_INFO;

typedef struct _BRUSH_UPDATE_AREA
{
	// 更新を行う範囲
	FLOAT_T min_x, min_y, max_x, max_y;
	// 初期化済みフラグ
	int initialized;
} BRUSH_UPDATE_AREA;

typedef struct _BRUSH_CHAIN_ITEM
{
	POINTER_ARRAY *names;
} BRUSH_CHAIN_ITEM;

typedef struct _BRUSH_CHAIN
{
	int key;
	int current;
	int change_key;
	unsigned int timer_id;
	int bursh_button_stop;
	POINTER_ARRAY *chains;
} BRUSH_CHAIN;

EXTERN void ChangeBrush(
	BRUSH_CORE* core,
	void* brush_data,
	brush_core_func press_func,
	brush_core_func motion_func,
	brush_core_func release_func
);

EXTERN void BrushCorePointReset(BRUSH_CORE* core);

EXTERN void AddBrushHistory(
	BRUSH_CORE* core,
	LAYER* active
);

EXTERN void AddSelectionEditHistory(BRUSH_CORE* core, LAYER* selection);

/*****************************************************
* SetBrushBaseScale関数                              *
* ブラシサイズの倍率を設定する                       *
* 引数                                               *
* widget	: 倍率設定用のコンボボックスウィジェット *
* index		: 倍率のインデックスを保持するアドレス   *
*****************************************************/
EXTERN void SetBrushBaseScale(GtkWidget* widget, int* index);

/***************************************
* BrushCoreSetCirclePattern関数        *
* ブラシの円形画像パターンを作成       *
* 引数                                 *
* core				: ブラシの基本情報 *
* r					: 半径             *
* outline_hardness	: 輪郭の硬さ       *
* blur				: ボケ足           *
* alpha				: 不透明度         *
* color				: 色               *
***************************************/
EXTERN void BrushCoreSetCirclePattern(
	BRUSH_CORE* core,
	FLOAT_T r,
	FLOAT_T outline_hardness,
	FLOAT_T blur,
	FLOAT_T alpha,
	const uint8 color[3]
);

/***********************************************
* BrushCoreSetGrayCirclePattern関数            *
* ブラシのグレースケール円形画像パターンを作成 *
* 引数                                         *
* core				: ブラシの基本情報         *
* r					: 半径                     *
* outline_hardness	: 輪郭の硬さ               *
* blur				: ボケ足                   *
* alpha				: 不透明度                 *
***********************************************/
EXTERN void BrushCoreSetGrayCirclePattern(
	BRUSH_CORE* core,
	FLOAT_T r,
	FLOAT_T outline_hardness,
	FLOAT_T blur,
	FLOAT_T alpha
);

EXTERN void BrushCoreUndoRedo(DRAW_WINDOW* window, void* p);

/*****************************************************
* DrawCircleBrush関数                                *
* ブラシをマスクレイヤーに描画する                   *
* 引数                                               *
* window		: キャンバス                         *
* core			: ブラシの基本情報                   *
* x				: 描画範囲の左上のX座標              *
* y				: 描画範囲の左上のY座標              *
* width			: 描画範囲の幅                       *
* height		: 描画範囲の高さ                     *
* mask			: 作業レイヤーにコピーする際のマスク *
* zoom			: 拡大・縮小率                       *
* alpha			: 不透明度                           *
* blend_mode	: 合成モード                         *
*****************************************************/
EXTERN void DrawCircleBrush(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height,
	uint8** mask,
	FLOAT_T zoom,
	FLOAT_T alpha,
	uint16 blend_mode
);

/*************************************************
* DrawCircleBrushWorkLayer関数                   *
* ブラシを作業レイヤーに描画する                 *
* 引数                                           *
* window	: キャンバス                         *
* core		: ブラシの基本情報                   *
* x			: 描画範囲の左上のX座標              *
* y			: 描画範囲の左上のY座標              *
* width		: 描画範囲の幅                       *
* height	: 描画範囲の高さ                     *
* mask		: 作業レイヤーにコピーする際のマスク *
* zoom		: 拡大・縮小率                       *
* alpha		: 不透明度                           *
*************************************************/
EXTERN void DrawCircleBrushWorkLayer(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height,
	uint8** mask,
	FLOAT_T zoom,
	FLOAT_T alpha
);

/*************************************************
* DrawImageBrush関数                             *
* 画像ブラシをマスクレイヤーに描画する           *
* 引数                                           *
* window		: キャンバスの情報               *
* core			: ブラシの基本情報               *
* x				: 描画範囲の中心X座標            *
* y				: 描画範囲の中心Y座標            *
* width			: 描画範囲の幅                   *
* height		: 描画範囲の高さ                 *
* scale			: 描画する拡大率                 *
* size			: 画像の長辺の長さ               *
* angle			: 画像の角度                     *
* image_width	: 画像の幅                       *
* image_height	: 画像の高さ                     *
* mask			: 合成時のマスクを受けるポインタ *
* alpha			: 濃度                           *
* blend_mode	: 合成モード                     *
*************************************************/
EXTERN void DrawImageBrush(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height,
	FLOAT_T scale,
	FLOAT_T size,
	FLOAT_T angle,
	FLOAT_T image_width,
	FLOAT_T image_height,
	uint8** mask,
	FLOAT_T alpha,
	uint16 blend_mode
);

/*************************************************
* DrawImageBrushWorkLayer関数                    *
* 画像ブラシを作業レイヤーに描画する             *
* 引数                                           *
* window		: キャンバスの情報               *
* core			: ブラシの基本情報               *
* x				: 描画範囲の中心X座標            *
* y				: 描画範囲の中心Y座標            *
* width			: 描画範囲の幅                   *
* height		: 描画範囲の高さ                 *
* scale			: 描画する拡大率                 *
* size			: 画像の長辺の長さ               *
* angle			: 画像の角度                     *
* image_width	: 画像の幅                       *
* image_height	: 画像の高さ                     *
* mask			: 合成時のマスクを受けるポインタ *
* alpha			: 濃度                           *
*************************************************/
EXTERN void DrawImageBrushWorkLayer(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height,
	FLOAT_T scale,
	FLOAT_T size,
	FLOAT_T angle,
	FLOAT_T image_width,
	FLOAT_T image_height,
	uint8** mask,
	FLOAT_T alpha
);

/*************************************************
* AdaptNormalBrush関数                           *
* 通常のブラシの描画結果を作業レイヤーに反映する *
* 引数                                           *
* window		: キャンバスの情報               *
* draw_pixel	: 描画結果の入ったピクセルデータ *
* width			: 描画範囲の幅                   *
* height		: 描画範囲の高さ                 *
* start_x		: 描画範囲の左上のX座標          *
* start_y		: 描画範囲の左上のY座標          *
* anti_alias	: アンチエイリアスを行うか否か   *
*************************************************/
EXTERN void AdaptNormalBrush(
	DRAW_WINDOW* window,
	uint8* draw_pixel,
	int width,
	int height,
	int start_x,
	int start_y,
	int anti_alias
);

/*************************************************
* AdaptBlendBrush関数                            *
* 合成ブラシの描画結果を作業レイヤーに反映する   *
* 引数                                           *
* window		: キャンバスの情報               *
* draw_pixel	: 描画結果の入ったピクセルデータ *
* width			: 描画範囲の幅                   *
* height		: 描画範囲の高さ                 *
* start_x		: 描画範囲の左上のX座標          *
* start_y		: 描画範囲の左上のY座標          *
* anti_alias	: アンチエイリアスを行うか否か   *
* blend_mode	: 合成モード                     *
*************************************************/
EXTERN void AdaptBlendBrush(
	DRAW_WINDOW* window,
	uint8* draw_pixel,
	int width,
	int height,
	int start_x,
	int start_y,
	int anti_alias,
	int blend_mode
);

/******************************************************
* UpdateBrushButtonPressDrawArea関数                  *
* ブラシのクリックに対する更新する範囲を設定する      *
* 引数                                                *
* window		: キャンバスの情報                    *
* area			: 更新範囲を記憶するアドレス          *
* core			: ブラシの基本情報                    *
* x				: 描画範囲の中心のX座標               *
* y				: 描画範囲の中心のY座標               *
* size			: ブラシの長辺の長さ                  *
* brush_area	: ブラシストローク終了時の更新範囲    *
******************************************************/
EXTERN void UpdateBrushButtonPressDrawArea(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T size,
	BRUSH_UPDATE_AREA* brush_area
);

/******************************************************
* UpdateBrushMotionDrawArea関数                       *
* ブラシのドラッグに対する更新する範囲を設定する      *
* 引数                                                *
* window		: キャンバスの情報                    *
* area			: 更新範囲を記憶するアドレス          *
* core			: ブラシの基本情報                    *
* x				: 描画範囲の中心のX座標               *
* y				: 描画範囲の中心のY座標               *
* before_x		: 前回描画時のマウスのX座標           *
* before_y		: 前回描画時のマウスのY座標           *
* size			: ブラシの長辺の長さ                  *
* brush_area	: ブラシストローク終了時の更新範囲    *
******************************************************/
EXTERN void UpdateBrushMotionDrawArea(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T before_x,
	FLOAT_T before_y,
	FLOAT_T size,
	BRUSH_UPDATE_AREA* brush_area
);

/*************************************
* UpdateBrushMotionDrawAreaSize関数  *
* ブラシ更新範囲のサイズを更新する   *
* 引数                               *
* window	: キャンバスの情報       *
* area	: 更新範囲を記憶するアドレス *
*************************************/
EXTERN void UpdateBrushMotionDrawAreaSize(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area
);

/***************************************************
* UpdateBrushScatterDrawArea関数                　 *
* ブラシの散布に対する更新する範囲を設定する    　 *
* 引数                                             *
* window		: キャンバスの情報                 *
* area			: 更新範囲を記憶するアドレス       *
* core			: ブラシの基本情報                 *
* x				: 描画範囲の中心のX座標            *
* y				: 描画範囲の中心のY座標            *
* size			: ブラシの長辺の長さ               *
* brush_area	: ブラシストローク終了時の更新範囲 *
***************************************************/
EXTERN void UpdateBrushScatterDrawArea(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T size,
	BRUSH_UPDATE_AREA* brush_area
);

/***************************************
* DefaultToolUpdate関数                *
* デフォルトのツールアップデートの関数 *
* 引数                                 *
* window	: アクティブな描画領域     *
* x			: マウスカーソルのX座標    *
* y			: マウスカーソルのY座標    *
* dummy		: ダミーポインタ           *
***************************************/
EXTERN void DefaultToolUpdate(DRAW_WINDOW* window, FLOAT_T x, FLOAT_T y, void* dummy);

/***************************************************
* UpdateBrushPreviewWindow関数                     *
* ブラシのプレビューキャンバスを更新する           *
* 引数                                             *
* window		: ブラシのプレビューキャンバス     *
* core			: プレビューするブラシの基本情報   *
* press_func	: クリック時のコールバック関数     *
* motion_func	: ドラッグ中のコールバック関数     *
* release_func	: ドラッグ終了時のコールバック関数 *
***************************************************/
EXTERN void UpdateBrushPreviewWindow(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	brush_core_func press_func,
	brush_core_func motion_func,
	brush_core_func release_func
);

/*********************************************************
* AdaptSmudge関数                                        *
* 指先ツールの作業レイヤーとの合成処理                   *
* 引数                                                   *
* canvas		: キャンバスの情報                       *
* start_x		: 描画範囲の左上のX座標                  *
* start_y		: 描画範囲の左上のY座標                  *
* width			: 描画範囲の幅                           *
* height		: 描画範囲の高さ                         *
* before_width	: 前回描画時の描画範囲の幅               *
* before_height	: 前回描画時の描画範囲の高さ             *
* mask			: ブラシの描画結果の入ったピクセルデータ *
* extend		: 色延びの割合                           *
* initialized	: 初期化済みか否か(初期化済み:0以外)     *
*********************************************************/
EXTERN void AdaptSmudge(
	DRAW_WINDOW* canvas,
	int start_x,
	int start_y,
	int width,
	int height,
	int before_width,
	int before_height,
	uint8* mask,
	uint8 extend,
	int initialized
);

/*****************************************************
* AdaptPickerBrush関数                               *
*スポイトブラシの描画結果を作業レイヤーに反映する    *
* 引数                                               *
* window			: キャンバスの情報               *
* draw_pixel		: 描画結果の入ったピクセルデータ *
* width				: 描画範囲の幅                   *
* height			: 描画範囲の高さ                 *
* start_x			: 描画範囲の左上のX座標          *
* start_y			: 描画範囲の左上のY座標          *
* anti_alias		: アンチエイリアスを行うか否か   *
* pick_target		: 色を拾う対象                   *
* picker_mode		: 色の収集モード                 *
* blend_mode		: ブラシの合成モード             *
* change_hue		: 色相の変化量                   *
* change_saturation	: 彩度の変化量                   *
* change_brightness	: 明度の変化量                   *
*****************************************************/
EXTERN void AdaptPickerBrush(
	DRAW_WINDOW* window,
	uint8* draw_pixel,
	int width,
	int height,
	int start_x,
	int start_y,
	int anti_alias,
	int pick_target,
	int picker_mode,
	int blend_mode,
	int16 change_hue,
	int16 change_saturation,
	int16 change_brightness
);

/*********************************************************
* AdaptSmudgeScatter関数                                 *
* 指先ツールの作業レイヤーとの合成処理(散布用)           *
* 引数                                                   *
* canvas		: キャンバスの情報                       *
* start_x		: 描画範囲の左上のX座標                  *
* start_y		: 描画範囲の左上のY座標                  *
* width			: 描画範囲の幅                           *
* height		: 描画範囲の高さ                         *
* before_width	: 前回描画時の描画範囲の幅               *
* before_height	: 前回描画時の描画範囲の高さ             *
* mask			: ブラシの描画結果の入ったピクセルデータ *
* extend		: 色延びの割合                           *
* initialized	: 初期化済みか否か(初期化済み:0以外)     *
*********************************************************/
EXTERN void AdaptSmudgeScatter(
	DRAW_WINDOW* canvas,
	int start_x,
	int start_y,
	int width,
	int height,
	int before_width,
	int before_height,
	uint8* mask,
	uint8 extend,
	int initialized
);

/*********************************************************
* BlendWaterBrush関数                                    *
* 水彩ブラシの作業レイヤーとの合成処理                   *
* 引数                                                   *
* canvas				: キャンバスの情報               *
* core					: ブラシの基本情報               *
* x						: 描画するX座標                  *
* y						: 描画するY座標                  *
* before_x				: 前回描画時のX座標              *
* before_y				: 前回描画時のY座標              *
* brush_size			: ブラシの半径                   *
* start_x				: 描画範囲の左上のX座標          *
* start_y				: 描画範囲の左上のY座標          *
* width					: 描画範囲の幅                   *
* height				: 描画範囲の高さ                 *
* work_pixel			: 作業レイヤーのピクセルデータ   *
* mask					: 描画結果の入ったピクセルデータ *
* brush_alpha			: ブラシの濃度                   *
* brush_before_color	: 前回描画時に記憶した色         *
* mix					: 混色する割合                   *
* extend				: 色を延ばす割合                 *
*********************************************************/
EXTERN void BlendWaterBrush(
	DRAW_WINDOW* canvas,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T before_x,
	FLOAT_T before_y,
	FLOAT_T brush_size,
	int start_x,
	int start_y,
	int width,
	int height,
	uint8* work_pixel,
	uint8* mask,
	FLOAT_T brush_alpha,
	uint8 brush_before_color[4],
	uint8 mix,
	uint8 extend
);

/*****************************************
* InitializeBrushChain関数               *
* 簡易ブラシ切り替えのデータを初期化する *
* 引数                                   *
* chain	: 簡易ブラシ切り替えのデータ     *
*****************************************/
void InitializeBrushChain(BRUSH_CHAIN* chain);

/********************************************
* InitializeBrushChainItem関数              *
* 簡易ブラシ切り替えの1セット分を初期化する *
* 引数                                      *
* item	: 簡易ブラシ切り替えの1セット       *
********************************************/
void InitializeBrushChainItem(BRUSH_CHAIN_ITEM* item);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_BRUSH_CORE_H_
