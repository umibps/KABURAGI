#ifndef _INCLUDED_BRUSH_CORE_H_
#define _INCLUDED_BRUSH_CORE_H_

#include <gtk/gtk.h>
#include "layer.h"
#include "draw_window.h"
#include "types.h"
#include "configure.h"

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
	BRUSH_TYPE_PICKER_IMAGE_BRUSH,
	BRUSH_TYPE_SCRIPT_BRUSH,
	NUM_BRUSH_TYPE
} eBRUSH_TYPE;

typedef void (*brush_core_func)(DRAW_WINDOW* window, gdouble x, gdouble y,
	gdouble pressure, struct _BRUSH_CORE *core, void* state);

typedef void (*brush_update_func)(DRAW_WINDOW* window, gdouble x, gdouble y, void* data);

typedef struct _BRUSH_CORE
{
	struct _APPLICATION *app;
	gdouble max_x, max_y, min_x, min_y;
	uint32 flags;
	size_t detail_data_size;

	cairo_surface_t *brush_surface;
	cairo_surface_t *temp_surface;
	cairo_pattern_t *brush_pattern;
	cairo_pattern_t *temp_pattern;
	cairo_t *temp_cairo;
	uint8 **brush_pattern_buff, **temp_pattern_buff;
	int stride;

	gchar* name;
	char* image_file_path;
	uint8 (*color)[3];
	uint8 (*back_color)[3];
	uint8 brush_type;
	gchar hot_key;

	void* brush_data;
	brush_core_func press_func, motion_func, release_func;
	void (*key_press_func)(DRAW_WINDOW* window, GdkEventKey* key, void* data);
	void (*draw_cursor)(DRAW_WINDOW* window, gdouble x, gdouble y, void* data);
	brush_update_func button_update, motion_update;
	GtkWidget* (*create_detail_ui)(struct _APPLICATION *app, struct _BRUSH_CORE* core);
	void (*color_change)(const uint8 color[3], void* data);
	void (*change_editting_selection)(void* data, int is_editting);
	GtkWidget* button;
} BRUSH_CORE;

extern void ChangeBrush(
	BRUSH_CORE* core,
	void* brush_data,
	brush_core_func press_func,
	brush_core_func motion_func,
	brush_core_func release_func
);

extern void BrushCorePointReset(BRUSH_CORE* core);

extern void AddBrushHistory(
	BRUSH_CORE* core,
	LAYER* active
);

extern void AddSelectionEditHistory(BRUSH_CORE* core, LAYER* selection);

/*****************************************************
* SetBrushBaseScale関数                              *
* ブラシサイズの倍率を設定する                       *
* 引数                                               *
* widget	: 倍率設定用のコンボボックスウィジェット *
* index		: 倍率のインデックスを保持するアドレス   *
*****************************************************/
extern void SetBrushBaseScale(GtkWidget* widget, int* index);

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
extern void BrushCoreSetCirclePattern(
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
extern void BrushCoreSetGrayCirclePattern(
	BRUSH_CORE* core,
	FLOAT_T r,
	FLOAT_T outline_hardness,
	FLOAT_T blur,
	FLOAT_T alpha
);

extern void BrushCoreUndoRedo(DRAW_WINDOW* window, void* p);

/***************************************
* DefaultToolUpdate関数                *
* デフォルトのツールアップデートの関数 *
* 引数                                 *
* window	: アクティブな描画領域     *
* x			: マウスカーソルのX座標    *
* y			: マウスカーソルのY座標    *
* dummy		: ダミーポインタ           *
***************************************/
extern void DefaultToolUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, void* dummy);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_BRUSH_CORE_H_
