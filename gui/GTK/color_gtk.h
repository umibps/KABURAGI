#ifndef _INCLUDED_COLOR_GTK_H_
#define _INCLUDED_COLOR_GTK_H_

#include <gtk/gtk.h>
#include "../../color.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _COLOR_CHOOSER
{
	GtkWidget* widget, *choose_area, *color_box, *pallete;
	GtkWidget *chooser_box;
	GtkWidget *pallete_widget;
	GtkWidget *circle_button, *pallete_button;
	cairo_surface_t *color_box_surface;
	uint8 *color_box_pixel_data;
	cairo_surface_t *circle_surface;
	uint8 *color_circle_data;
	uint8 *chooser_pixel_data;
	cairo_surface_t *chooser_surface;
	cairo_t *chooser_cairo;
	uint8 *pallete_pixels;
	cairo_surface_t *pallete_surface;
	void (*filter_func)(
		uint8* source, uint8* destination, int size, void* filter_data);
	void *filter_data;
	int32 widget_width;
	int32 widget_height;
	HSV hsv, back_hsv;
	uint8 rgb[3], back_rgb[3];
	uint8 color_history[MAX_COLOR_HISTORY_NUM][3];
	uint8 (*pallete_data)[3];
	uint8 *pallete_flags;
	uint8 line_width;
	uint8 flags;
	uint8 num_color_history;
	uint16 selected_pallete;
	FLOAT_T sv_x, sv_y;
	FLOAT_T ui_scale;
	void (*color_change_callback)(
		GtkWidget* widget, const uint8 color[3], void* data);
	void* data;
} COLOR_CHOOSER;

EXTERN void DrawColorCircle(
	COLOR_CHOOSER* chooser,
	FLOAT_T cx,
	FLOAT_T cy,
	FLOAT_T r
);

EXTERN COLOR_CHOOSER *CreateColorChooser(
	int32 width,
	int32 height,
	uint8 line_width,
	int32 start_rad,
	uint8 (*pallete)[3],
	uint8 *pallete_use,
	const gchar* base_path,
	struct _APPLICATION_LABELS* labels,
	FLOAT_T ui_scale
);

EXTERN void SetColorChooserPoint(COLOR_CHOOSER* chooser, HSV* set_hsv, gboolean add_history);

EXTERN void UpdateColorBox(COLOR_CHOOSER* chooser);

EXTERN void DestroyColorChooser(COLOR_CHOOSER* chooser);

/*********************************************************
* SetColorChangeCallBack関数                             *
* 色が変更されたときに呼び出されるコールバック関数を設定 *
* 引数                                                   *
* chooser	: 色選択用のデータ                           *
* function	: コールバック関数                           *
* data		: コールバック関数で使用するデータ           *
*********************************************************/
EXTERN void SetColorChangeCallBack(
	COLOR_CHOOSER* chooser,
	void (*function)(GtkWidget* widget, const uint8 color[3], void* data),
	void *data
);


/***********************************
* LoadPalleteAdd関数               *
* パレットデータを追加読み込み     *
* 引数                             *
* chooser	: 色選択の情報         *
* file_path	: データファイルのパス *
* 返り値                           *
*	読み込んだデータの数           *
***********************************/
EXTERN int LoadPalleteAdd(
	COLOR_CHOOSER* chooser,
	const char* file_path
);

/*************************************
* LoadPalleteFile関数                *
* パレットの情報を読み込む           *
* chooser	: 色選択用の情報         *
* file_path	: 読み込むファイルのパス *
* 返り値                             *
*	正常終了:0	失敗:負の値          *
*************************************/
EXTERN int LoadPalleteFile(COLOR_CHOOSER* chooser, const gchar* file_path);

/*************************************
* WritePalleteFile関数               *
* パレットの情報を書き込む           *
* chooser	: 色選択用の情報         *
* file_path	: 書き込むファイルのパス *
* 返り値                             *
*	正常終了:0	失敗:負の値          *
*************************************/
EXTERN int WritePalleteFile(COLOR_CHOOSER* chooser, const gchar* file_path);

/***************************************
* RegisterColorPallete関数             *
* パレットに色を追加する               *
* 引数                                 *
* chooser	: 色選択ウィジェットの情報 *
* color		: 追加する色               *
***************************************/
EXTERN void RegisterColorPallete(COLOR_CHOOSER* chooser, const uint8 color[3]);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_COLOR_GTK_H_
