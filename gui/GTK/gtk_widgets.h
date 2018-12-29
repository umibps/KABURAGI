#ifndef _INCLUDED_GTK_WIDGETS_H_
#define _INCLUDED_GTK_WIDGETS_H_

#include <gtk/gtk.h>

typedef struct _MAIN_WINDOW_WIDGETS
{
	// ウィンドウ、メニュー、タブを入れるパッキングボックス、タブ
	GtkWidget *window, *vbox, *note_book;
	// メニューバー
	GtkWidget *menu_bar;
	// ステータスバー
	GtkWidget *status_bar;
	// 保存、フィルター等の進捗状況を表すプログレスバー
	GtkWidget *progress;
	// シングルウィンドウ用にツールを入れる左右のペーン
	GtkWidget *left_pane, *right_pane;
	// ナビゲーションとレイヤービューをドッキングするためのボックス
	GtkWidget *navi_layer_pane;
	// 左右反転のボタンとラベル
	GtkWidget *reverse_button, *reverse_label;
	// 選択範囲編集のボタン
	GtkWidget *edit_selection;
	// 使用テクスチャのラベル
	GtkWidget *texture_label;
	// ショートカットキー
	GtkAccelGroup *hot_key;
} MAIN_WINDOW_WIDGETS;

#endif	// #ifndef _INCLUDED_GTK_WIDGETS_H_
