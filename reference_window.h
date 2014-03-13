#ifndef _INCLUDED_REFERENCE_WINDOW_H_
#define _INCLUDED_REFERENCE_WINDOW_H_

#include <gtk/gtk.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _REFERENCE_IMAGE
{
	uint8 *pixels;
	int width, height, stride;
	uint8 channel;
	cairo_surface_t *surface_p;
	gdouble zoom;
	gdouble rev_zoom;
	GtkWidget *draw_area;
	GtkWidget *scroll;
	struct _APPLICATION *app;
} REFERENCE_IMAGE;

typedef struct _REFERENCE_WINDOW_DATA
{
	GtkWidget *window;
	GtkWidget *note_book;
	GtkWidget *scale;
	GtkAdjustment *scale_adjust;
	int16 num_image;
	int16 active_image;
	REFERENCE_IMAGE *images[32];
} REFERENCE_WINDOW_DATA;

typedef struct _REFERENCE_WINDOW
{
	gint window_x, window_y;		// ウィンドウの座標
	gint window_width;				// ウィンドウの幅
	gint window_height;				// ウィンドウの高さ
	GtkWidget *menu_item;			// 表示/非表示を切り替えるメニューアイテム
	REFERENCE_WINDOW_DATA *data;	// 表示画像のデータ
	struct _APPLICATION *app;		// アプリケーションを管理する構造体のアドレス
} REFERENCE_WINDOW;

// 関数のプロトタイプ宣言
/*********************************************************
* InitializeReferenceWindow関数                          *
* 参考用画像表示ウィンドウの初期化                       *
* 引数                                                   *
* reference	: 参考用画像表示ウィンドウの情報             *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
extern void InitializeReferenceWindow(REFERENCE_WINDOW* reference, struct _APPLICATION* app);

/***************************************************************
* DisplayReferenceWindowMenuActivate関数                       *
* 参考用画像表示ウィンドウを表示するメニューのコールバック関数 *
* 引数                                                         *
* menu_item	: メニューアイテムウィジェット                     *
* app		: アプリケーションを管理する構造体のアドレス       *
***************************************************************/
extern void DisplayReferenceWindowMenuActivate(GtkWidget* menu_item, struct _APPLICATION* app);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_REFERENCE_WINDOW_H_
