#ifndef _INCLUDED_PREVIEW_WINDOW_H_
#define _INCLUDED_PREVIEW_WINDOW_H_

#include <gtk/gtk.h>
#include "draw_window.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PREVIEW_WINDOW
{
	GtkWidget *window;				// ウィンドウ
	GtkWidget *menu_item;			// 表示/非表示を切り替えるメニューウィジェット
	GtkWidget *image;				// プレビューに表示するイメージ
	int window_x, window_y;			// ウィンドウの座標
	int window_width;				// ウィンドウの幅
	int window_height;				// ウィンドウの高さ
	cairo_t *cairo_p;				// 表示用CAIRO情報
	cairo_surface_t *surface_p;		// 表示用CAIROサーフェース
	int width, height;				// ウィンドウの幅と高さ
	int draw_width, draw_height;	// 表示するイメージの幅と高さ
	uint8* pixels;					// 表示するイメージのピクセルデータ
	uint8* reverse_buff;			// 左右反転用のバッファ
	gdouble zoom, rev_zoom;			// プレビューするイメージの拡大率
	DRAW_WINDOW* draw;				// プレビューする描画領域
} PREVIEW_WINDOW;

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_PREVIEW_WINDOW_H_
