#ifndef _INCLUDED_NAVIGATION_H_
#define _INCLUDED_NAVIGATION_H_

#include <gtk/gtk.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************
* NAVIGATION構造体               *
* ナビゲーションウィンドウの情報 *
*********************************/
typedef struct _NAVIGATION_WINDOW
{
	GtkWidget *window;				// ナビゲーションウィンドウ
	GtkWidget *draw_area;			// 画像の表示領域
	GtkWidget *vbox;				// ナビゲーションのウィジェットを入れるボックス
	int window_x, window_y;			// ウィンドウの座標
	int window_width;				// ウィンドウの幅
	int window_height;				// ウィンドウの高さ
	cairo_pattern_t *pattern;		// 表示用パターン
	uint8* pixels;					// 表示する画像のピクセルデータ]
	uint8* reverse_buff;			// 左右反転用のバッファ
	int width;						// 表示画像の幅
	int height;						// 表示画像の高さ
	int draw_width;					// 描画領域の幅
	int draw_height;				// 描画領域の高さ
	// ナビゲーションの座標関係
	struct
	{
		gdouble x, y;				// ナビゲーションの指定している座標
		gdouble ratio_x;			// 表示X座標補正用
		gdouble ratio_y;			// 表示Y座標補正用
		gdouble width, height;		// ナビゲーションの指定している幅・高さ
		gdouble angle;				// ナビゲーションの回転角度
		uint16 zoom;				// ナビゲーションの拡大縮小率
	} point;

	GtkAdjustment *zoom_slider;		// 拡大縮小率を操作するスライダのアジャスタ
	GtkAdjustment *rotate_slider;	// 回転角度を操作するスライダのアジャスタ

	// ウィンドウ削除時の無効化ウィジェット指定用
	uint16 zoom_index, rotate_index;
} NAVIGATION_WINDOW;

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_NAVIGATION_H_
