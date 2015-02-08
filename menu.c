// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "application.h"
#include "draw_window.h"
#include "clip_board.h"
#include "ini_file.h"
#include "menu.h"
#include "input.h"
#include "display.h"
#include "memory.h"
#include "filter.h"
#include "transform.h"
#include "printer.h"
#include "plug_in.h"

#ifdef __cplusplus
extern "C" {
#endif

static void ExecuteNew(APPLICATION *app);
void Change2LoupeMode(APPLICATION* app);
/*********************************************************
* ExecuteDisplayReverseHorizontally関数                  *
* 表示を左右反転                                         *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteDisplayReverseHorizontally(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* ExecuteChangeDisplayPreview関数                        *
* プレビューの表示/非表示を切り替える                    *
* 引数                                                   *
* menu_item	: 表示切り替えメニューアイテムウィジェット   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteChangeDisplayPreview(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* ExecuteSwitchFullScreen関数                            *
* フルスクリーンの切り替えを行う                         *
* 引数                                                   *
* menu_item	: 表示切り替えメニューアイテムウィジェット   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteSwitchFullScreen(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* ExecuteMoveToolWindowTopLeft関数                       *
* ツールボックスウィンドウを左上に移動する               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteMoveToolWindowTopLeft(APPLICATION* app);

/*********************************************************
* ExecuteMoveLayerWindowTopLeft関数                      *
* レイヤーウィンドウを左上に移動する                     *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteMoveLayerWindowTopLeft(APPLICATION* app);

/*********************************************************
* ExecuteMoveNavigationWindowTopLeft関数                 *
* ナビゲーションウィンドウを左上に移動する               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteMoveNavigationWindowTopLeft(APPLICATION* app);

/*************************************************
* ParentItemSelected関数                         *
* 親メニューアイテムが選択された時に呼び出される *
* 引数                                           *
* widget	: 親メニューアイテム                 *
* data		: ダミー                             *
*************************************************/
void ParentItemSelected(GtkWidget* widget, gpointer* data)
{
	gtk_menu_item_select(GTK_MENU_ITEM(widget));
}

/*********************************************************
* OnDestroyMainMenu関数                                  *
* メインメニューが削除されたときに呼び出される           *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void OnDestroyMainMenu(APPLICATION* app)
{
	app->menus.disable_if_single_layer[app->layer_window.layer_control.delete_menu_index] = NULL;
	app->menus.disable_if_single_layer[app->layer_window.layer_control.merge_down_menu_index] = NULL;
	app->menus.disable_if_single_layer[app->layer_window.layer_control.flatten_menu_index] = NULL;
}

/*********************************************************
* GetMainMenu関数                                        *
* メインメニューを作成する                               *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* window	: メインウィンドウのウィジェット             *
* path		:                                            *
* 返り値                                                 *
*	メニューバーウィジェット                             *
*********************************************************/
GtkWidget* GetMainMenu(
	APPLICATION* app,
	GtkWidget* window,
	const char* path
)
{
	// メニューバー、サブメニューとメニューアイテム
	GtkWidget* menu_bar, *menu, *menu_item, *sub_menu;
	// ラジオメニューの先頭アイテム
	GtkWidget *radio_top;
	// サブメニューを区切るためのセパレータ
	GtkWidget* separator = gtk_separator_menu_item_new();
	// ショートカットキー用
	GtkAccelGroup* accel_group;
	// 表示文字列作成用のバッファ
	char buff[2048];
	int i;	// for文用のカウンタ

	// メニューバー作り直しに備えてウィジェットの数を記憶しておく
	app->menus.menu_start_disable_if_no_open = app->menus.num_disable_if_no_open;
	app->menus.menu_start_disable_if_no_select = app->menus.num_disable_if_no_select;
	app->menus.menu_start_disable_if_single_layer = app->menus.num_disable_if_single_layer;
	app->menus.menu_start_disable_if_normal_layer = app->menus.num_disable_if_normal_layer;

	// ショートカットキー登録の準備
	if(app->hot_key == NULL)
	{
		app->hot_key = accel_group = gtk_accel_group_new();
	}
	else
	{
		accel_group = app->hot_key;
	}
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	// メニューバーを作成
	menu_bar = gtk_menu_bar_new();
	// メニューバー削除時のコールバック関数をセット
	(void)g_signal_connect_swapped(G_OBJECT(menu_bar), "destroy",
		G_CALLBACK(OnDestroyMainMenu), app);

	// 「ファイル」メニュー
	(void)sprintf(buff, "_%s(_F)", app->labels->menu.file);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'F', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// 「ファイル」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 「新規作成」
	(void)sprintf(buff, "%s", app->labels->menu.make_new);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'N', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteNew), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// 「開く」
	(void)sprintf(buff, "%s", app->labels->menu.open);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'O', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteOpenFile), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// 「レイヤーとして開く」
	(void)sprintf(buff, "%s", app->labels->menu.open_as_layer);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'O', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteOpenFileAsLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「上書き保存」
	(void)sprintf(buff, "%s", app->labels->menu.save);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'S', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSave), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「名前を付けて保存」
	(void)sprintf(buff, "%s", app->labels->menu.save_as);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'S', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSaveAs), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「印刷」
	(void)sprintf(buff, "%s", "Print");
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecutePrint), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「閉じる」
	(void)sprintf(buff, "%s", app->labels->menu.close);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'W', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteClose), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「終了」
	(void)sprintf(buff, "%s", app->labels->menu.quit);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		GDK_Escape, 0, GTK_ACCEL_VISIBLE);
	//	GDK_KEY_Escape, 0, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(OnQuitApplication), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// 「編集」メニュー
	(void)sprintf(buff, "_%s(_E)", app->labels->menu.edit);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'E', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// 「編集」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 「元に戻す」
	(void)sprintf(buff, "%s", app->labels->menu.undo);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'Z', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteUndo), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「やり直し」
	(void)sprintf(buff, "%s", app->labels->menu.redo);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'Y', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteRedo), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「コピー」
	(void)sprintf(buff, "%s", app->labels->menu.copy);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'C', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteCopy), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// 「可視部をコピー」
	(void)sprintf(buff, "%s", app->labels->menu.copy_visible);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'C', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteCopyVisible), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// 「切り取り」
	(void)sprintf(buff, "%s", app->labels->menu.cut);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'X', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteCut), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// 「貼り付け」
	(void)sprintf(buff, "%s", app->labels->menu.paste);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'V', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecutePaste), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「変形」
	(void)sprintf(buff, "%s", app->labels->menu.transform);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'T', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteTransform), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// 「射影変換」
	(void)sprintf(buff, "%s", app->labels->menu.projection);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'T', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteProjection), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「環境設定」
	(void)sprintf(buff, "%s", app->labels->preference.title);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSetPreference), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// 「キャンバス」メニュー
	(void)sprintf(buff, "_%s(_C)", app->labels->menu.canvas);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'C', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「キャンバス」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 「解像度の変更」
	(void)sprintf(buff, "%s", app->labels->menu.change_resolution);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeResolution), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「キャンバスサイズの変更」
	(void)sprintf(buff, "%s", app->labels->menu.change_canvas_size);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeCanvasSize), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「水平反転」
	(void)sprintf(buff, "%s", app->labels->menu.flip_canvas_horizontally);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FlipImageHorizontally), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「垂直反転」
	(void)sprintf(buff, "%s", app->labels->menu.flip_canvas_vertically);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FlipImageVertically), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「背景色を切り替え」
	(void)sprintf(buff, "%s", app->labels->menu.switch_bg_color);
	app->menus.change_back_ground_menu = 
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
			menu_item = gtk_check_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(SwitchSecondBackColor), app);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'B', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「2つめの背景色を変更」
	(void)sprintf(buff, "%s", app->labels->menu.change_2nd_bg_color);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(Change2ndBackColor), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「キャンバスのICCプロファイルを変更」
	(void)sprintf(buff, "%s", app->labels->menu.canvas_icc);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeCanvasIccProfile), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「レイヤー」メニュー
	(void)sprintf(buff, "_%s(_L)", app->labels->menu.layer);	
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'L', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「レイヤー」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 「新規通常レイヤー」
	(void)sprintf(buff, "%s", app->labels->menu.new_color);	
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'N', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMakeColorLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「新規ベクトルレイヤー」
	(void)sprintf(buff, "%s", app->labels->menu.new_vector);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'P', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMakeVectorLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「新規レイヤーセット」
	(void)sprintf(buff, "%s", app->labels->menu.new_layer_set);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'G', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMakeLayerSet), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「3Dレイヤー」
	if(GetHas3DLayer(app) != FALSE)
	{
		(void)sprintf(buff, "%s", app->labels->menu.new_3d_modeling);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
			menu_item = gtk_menu_item_new_with_mnemonic(buff);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMake3DLayer), app);
		app->menus.num_disable_if_no_open++;
	}

	// 「レイヤーを複製」
	(void)sprintf(buff, "%s", app->labels->menu.copy_layer);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteCopyLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「可視部をレイヤーに」
	(void)sprintf(buff, "%s", app->labels->menu.visible2layer);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteVisible2Layer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「レイヤーを削除」
	(void)sprintf(buff, "%s", app->labels->menu.delete_layer);
	app->layer_window.layer_control.delete_menu_index = app->menus.num_disable_if_single_layer;
	app->menus.disable_if_single_layer[app->menus.num_disable_if_single_layer] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(DeleteActiveLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_single_layer++;

	// 「下のレイヤーと結合」
	(void)sprintf(buff, "%s", app->labels->menu.merge_down_layer);
	app->layer_window.layer_control.merge_down_menu_index = app->menus.num_disable_if_single_layer;
	app->menus.merge_down_menu =
		app->menus.disable_if_single_layer[app->menus.num_disable_if_single_layer] =
			menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(MergeDownActiveLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_single_layer++;

	// 「画像を統合」
	(void)sprintf(buff, "%s", app->labels->menu.merge_all_layer);
	app->layer_window.layer_control.flatten_menu_index = app->menus.num_disable_if_single_layer;
	app->menus.disable_if_single_layer[app->menus.num_disable_if_single_layer] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FlattenImage), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_single_layer++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「描画色で塗り潰す」
	(void)sprintf(buff, "%s", app->labels->menu.fill_layer_fg_color);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'F', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FillForeGroundColor), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「パターンで塗り潰す」
	(void)sprintf(buff, "%s", app->labels->menu.fill_layer_pattern);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FillPattern), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「レイヤーをラスタライズ」
	(void)sprintf(buff, "%s", app->labels->menu.rasterize_layer);
	app->menus.disable_if_normal_layer[app->menus.num_disable_if_normal_layer] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(RasterizeActiveLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_widget_set_sensitive(menu_item, FALSE);
	app->menus.num_disable_if_normal_layer++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「不透明部分を選択範囲に」
	(void)sprintf(buff, "%s", app->labels->layer_window.alpha_to_select);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ActiveLayerAlpha2SelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「不透明部分を選択範囲に加える」
	(void)sprintf(buff, "%s", app->labels->layer_window.alpha_add_select);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ActiveLayerAlphaAddSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「選択範囲」メニュー
	(void)sprintf(buff, "_%s(_S)", app->labels->menu.select);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'S', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「選択範囲」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 「選択を解除」
	(void)sprintf(buff, "%s", app->labels->menu.select_none);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'D', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(UnSetSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// 「選択を反転」
	(void)sprintf(buff, "%s", app->labels->menu.select_invert);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'I', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(InvertSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「選択範囲を拡大」
	(void)sprintf(buff, "%s", app->labels->menu.selection_extend);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExtendSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// 「選択範囲を縮小」
	(void)sprintf(buff, "%s", app->labels->menu.selection_reduct);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ReductSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// 「選択範囲を編集」
	(void)sprintf(buff, "%s", app->labels->window.edit_selection);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		app->menu_data.edit_selection = menu_item = gtk_check_menu_item_new_with_label(buff);
#if GTK_MAJOR_VERSION <= 2
	gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
#endif
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'Q', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ChangeEditSelectionMode), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// 「全て選択」
	(void)sprintf(buff, "%s", app->labels->menu.select_all);	
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'A', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSelectAll), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「フィルター」メニュー
	(void)sprintf(buff, "_%s(_T)", app->labels->menu.filters);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);	
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'T', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「フィルター」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 「明るさコントラスト」
	(void)sprintf(buff, "%s", app->labels->menu.bright_contrast);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeBrightContrastFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「色相・彩度」
	(void)sprintf(buff, "%s", app->labels->menu.hue_saturtion);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeHueSaturationFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「ぼかし」
	(void)sprintf(buff, "%s", app->labels->menu.blur);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteBlurFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「モーションぼかし」
	(void)sprintf(buff, "%s", app->labels->menu.motion_blur);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMotionBlurFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「輝度を透明度に」
	(void)sprintf(buff, "%s", app->labels->menu.luminosity2opacity);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteLuminosity2OpacityFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「輝度を透明度に」
	(void)sprintf(buff, "%s", app->labels->menu.color2alpha);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteColor2AlphaFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「下のレイヤーで着色」
	(void)sprintf(buff, "%s", app->labels->menu.colorize_with_under);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteColorizeWithUnderFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「グラデーションマップ」
	(void)sprintf(buff, "%s", app->labels->menu.gradation_map);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteGradationMapFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「ベクトルレイヤーで下のレイヤーを塗り潰す」
	(void)sprintf(buff, "%s", app->labels->menu.fill_with_vector);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteFillWithVectorLineFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「下塗り」メニュー
	(void)sprintf(buff, "%s",app->labels->menu.render);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「下塗り」メニューの下にサブメニュー作成
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// 「雲」
	(void)sprintf(buff, "%s", app->labels->menu.cloud);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecutePerlinNoiseFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「フラクタル」
	(void)sprintf(buff, "%s", app->labels->menu.fractal);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteFractal), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//----------------------------------------------------------//

	// 「ビュー」メニュー
	(void)sprintf(buff, "_%s(_V)", app->labels->menu.view);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'V', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「ビュー」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 「拡大・縮小」メニュー
	(void)sprintf(buff, "%s", app->labels->menu.zoom);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「拡大・縮小」メニューの下にサブメニュー作成
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// 「拡大」
	(void)sprintf(buff, "%s", app->labels->menu.zoom_in);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'+', 0, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteZoomIn), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「縮小」
	(void)sprintf(buff, "%s", app->labels->menu.zoom_out);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'-', 0, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteZoomOut), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「等倍表示」
	(void)sprintf(buff, "%s", app->labels->menu.zoom_reset);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteZoomReset), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「回転」
	(void)sprintf(buff, "%s", app->labels->menu.rotate);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_label(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「回転」メニューの下にサブメニュー作成
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// 「時計回り」
	(void)sprintf(buff, "%s", app->labels->tool_box.clockwise);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'R', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteRotateClockwise), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「反時計回り」
	(void)sprintf(buff, "%s", app->labels->tool_box.counter_clockwise);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'L', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteRotateCounterClockwise), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「回転をリセット」
	(void)sprintf(buff, "%s", app->labels->menu.reset_rotate);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteRotateReset), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「表示を反転」
	(void)sprintf(buff, "%s", app->labels->menu.reverse_horizontally);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		app->menu_data.reverse_horizontally = menu_item = gtk_check_menu_item_new_with_mnemonic(buff);
#if GTK_MAJOR_VERSION <= 2
	gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
#endif
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'H', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteDisplayReverseHorizontally), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「表示フィルター」
	(void)sprintf(buff, "%s", app->labels->menu.display_filter);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_label(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「表示フィルター」メニューの下にサブメニュー作成
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// 「無し」
	(void)sprintf(buff, "%s", app->labels->menu.no_filter);
	app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_NO_CONVERT] =
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		radio_top = gtk_radio_menu_item_new_with_mnemonic(NULL, buff);
	(void)g_signal_connect_swapped(G_OBJECT(radio_top), "activate",
		G_CALLBACK(NoDisplayFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), radio_top);
	app->menus.num_disable_if_no_open++;

	// 「グレースケール」
	(void)sprintf(buff, "%s", app->labels->menu.gray_scale);
	app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_GRAY_SCALE] =
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_radio_menu_item_new_with_mnemonic(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(radio_top)), buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(GrayScaleDisplayFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「グレースケール(YIQ)」
	(void)sprintf(buff, "%s", app->labels->menu.gray_scale_yiq);
	app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_GRAY_SCALE_YIQ] =
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_radio_menu_item_new_with_mnemonic(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(radio_top)), buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(GrayScaleDisplayFilterYIQ), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;
	
	// 「ICCプロファイル」
	(void)sprintf(buff, "%s", "ICC PROFILE");
	app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_ICC_PROFILE] =
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_radio_menu_item_new_with_mnemonic(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(radio_top)), buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(IccProfileDisplayFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//-----------------------------------------------------------//
	// 「ウィンドウ」メニュー
	(void)sprintf(buff, "_%s(_W)", app->labels->window.window);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'W', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// 「ウィンドウ」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 「ツールボックス」
	(void)sprintf(buff, "%s", app->labels->tool_box.title);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// 「ツールボックス」メニューの下にサブメニュー作成
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// 「ウィンドウ」
	(void)sprintf(buff, "%s", app->labels->window.window);
	app->tool_window.menu_item = menu_item = gtk_radio_menu_item_new_with_label(NULL, buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->tool_window.flags & TOOL_DOCKED) == 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_WINDOW));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeToolWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// 左に配置
	(void)sprintf(buff, "%s", app->labels->window.place_left);
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->tool_window.flags & TOOL_DOCKED) != 0 && (app->tool_window.flags & TOOL_PLACE_RIGHT) == 0);
	(void)g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_LEFT));
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeToolWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// 右に配置
	(void)sprintf(buff, "%s", app->labels->window.place_right);
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->tool_window.flags & TOOL_DOCKED) != 0 && (app->tool_window.flags & TOOL_PLACE_RIGHT) != 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_RIGHT));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeToolWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// ウィンドウを左上に移動
	(void)sprintf(buff, "%s", app->labels->window.move_top_left);
	menu_item = gtk_menu_item_new_with_label(buff);
	g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMoveToolWindowTopLeft), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// 「レイヤービュー & ナビゲーション」
	(void)sprintf(buff, "%s & %s", app->labels->layer_window.title, app->labels->navigation.title);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// 「レイヤービュー & ナビゲーション」メニューの下にサブメニュー作成
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// 「ウィンドウ」
	(void)sprintf(buff, "%s", app->labels->window.window);
	app->tool_window.menu_item = menu_item = gtk_radio_menu_item_new_with_label(NULL, buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->layer_window.flags & LAYER_WINDOW_DOCKED) == 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_WINDOW));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeNavigationLayerWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// 左に配置
	(void)sprintf(buff, "%s", app->labels->window.place_left);
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->layer_window.flags & LAYER_WINDOW_DOCKED) != 0 && (app->layer_window.flags & LAYER_WINDOW_PLACE_RIGHT) == 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_LEFT));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeNavigationLayerWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// 右に配置
	(void)sprintf(buff, "%s", app->labels->window.place_right);
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->layer_window.flags & LAYER_WINDOW_DOCKED) != 0 && (app->layer_window.flags & LAYER_WINDOW_PLACE_RIGHT) != 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_RIGHT));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeNavigationLayerWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// レイヤービューを左上に移動
	(void)sprintf(buff, "%s (%s)", app->labels->window.move_top_left, app->labels->layer_window.title);
	menu_item = gtk_menu_item_new_with_label(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMoveLayerWindowTopLeft), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// ナビゲーションを左上に移動
	(void)sprintf(buff, "%s (%s)", app->labels->window.move_top_left, app->labels->navigation.title);
	menu_item = gtk_menu_item_new_with_label(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMoveNavigationWindowTopLeft), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// 「プレビュー」ウィンドウ
	(void)sprintf(buff, "%s", app->labels->unit.preview);
	app->preview_window.menu_item = menu_item = gtk_check_menu_item_new_with_label(buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeDisplayPreview), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// 「参考用画像」ウィンドウ
	(void)sprintf(buff, "%s", app->labels->window.reference);
	app->reference_window.menu_item = menu_item = gtk_check_menu_item_new_with_label(buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), FALSE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(DisplayReferenceWindowMenuActivate), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// 「フルスクリーン」
	(void)sprintf(buff, "%s", app->labels->window.fullscreen);
	menu_item = gtk_check_menu_item_new_with_label(buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), app->flags & APPLICATION_FULL_SCREEN);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSwitchFullScreen), app);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		GDK_Return, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	//	GDK_KEY_Return, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	//-----------------------------------------------------------//

	// 「プラグイン」メニュー
	menu_item = PlugInMenuItemNew(app);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'P', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	//-----------------------------------------------------------//
	// 「スクリプト」メニュー
	(void)sprintf(buff, "_%s(_S)", app->labels->menu.script);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'S', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// 「スクリプト」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 読み込んだスクリプトの名前をメニューに追加
	for(i=0; i<app->scripts.num_script; i++)
	{
		(void)sprintf(buff, "%s", app->scripts.file_names[i]);
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
		(void)g_signal_connect(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteScript), app);
		g_object_set_data(G_OBJECT(menu_item), "script_id", GINT_TO_POINTER(i));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	}

	//-----------------------------------------------------------//
	// 「ヘルプ」メニュー
	(void)sprintf(buff, "_%s(_H)", app->labels->menu.help);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'H', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// 「ヘルプ」メニューの下にサブメニュー作成
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// 「バージョン情報」
	(void)sprintf(buff, "%s", app->labels->menu.version);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(DisplayVersion), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// キャンバスの数が0のときに無効なメニューを設定
	for(i=0; i<app->menus.num_disable_if_no_open; i++)
	{
		gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], FALSE);
	}

	// 選択範囲がないときに無効なメニューを設定
	for(i=0; i<app->menus.num_disable_if_no_select; i++)
	{
		gtk_widget_set_sensitive(app->menus.disable_if_no_select[i], FALSE);
	}

	return menu_bar;
}

/*****************************************************
* ExecuteNew関数                                     *
* 新規作成を実行                                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
static void ExecuteNew(APPLICATION* app)
{
	// 画像の幅と高さを決めるダイアログボックスを作成
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->make_new.title,
			GTK_WINDOW(app->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
	// ウィジェットの配置用
	GtkWidget *table, *spin1, *spin2, *label;
	GtkWidget *dpi;
	GtkWidget *second_color;
	GtkAdjustment *adjust;
	// 2つめの背景色
	GdkColor second_back_rgb;
	// OK, キャンセルの結果を受ける
	gint ret;
	// 作成する画像の幅と高さ
	int32 width, height;
	// for文用のカウンタ
	int i;

	table = gtk_table_new(4, 3, FALSE);

	// ダイアログボックスに幅のラベルと値設定用のウィジェットを登録
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);
	label = gtk_label_new(app->labels->make_new.width);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->menu_data.make_new.width, 1, LONG_MAX, 1, 10, 0));
	spin1 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin1, 1, 2, 0, 1);

	// ダイアログボックスに高さのラベルと値設定用のウィジェットを登録
	label = gtk_label_new(app->labels->make_new.height);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->menu_data.make_new.height, 1, LONG_MAX, 1, 10, 0));
	spin2 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin2, 1, 2, 1, 2);

	// 解像度設定用のウィジェットを登録
	label = gtk_label_new(app->labels->unit.resolution);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->menu_data.make_new.resolution,
		1, 1200, 1, 1, 0));
	dpi = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), dpi, 1, 2, 2, 3);
	label = gtk_label_new("dpi");
	gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 2, 3);

	// 2つめの背景色設定用のウィジェットを登録
	label = gtk_label_new(app->labels->make_new.second_bg_color);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
	second_back_rgb.red = (app->menu_data.make_new.second_bg_color[0] << 8) | app->menu_data.make_new.second_bg_color[0];
	second_back_rgb.green = (app->menu_data.make_new.second_bg_color[1] << 8) | app->menu_data.make_new.second_bg_color[1];
	second_back_rgb.blue = (app->menu_data.make_new.second_bg_color[2] << 8) | app->menu_data.make_new.second_bg_color[2];
	second_color = gtk_color_button_new_with_color(&second_back_rgb);
	gtk_color_button_set_title(GTK_COLOR_BUTTON(second_color), app->labels->make_new.second_bg_color);
	gtk_table_attach_defaults(GTK_TABLE(table), second_color, 1, 2, 3, 4);

	// ダイアログボックスを表示
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	// ユーザーが「OK」、「キャンセル」をクリックまで待つ
	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	// 「OK」が押されたら
	if(ret == GTK_RESPONSE_ACCEPT)
	{	// 入力された幅と高さの値を取得
		width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin1));
		height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin2));

		app->menu_data.make_new.width = width;
		app->menu_data.make_new.height = height;
		app->menu_data.make_new.resolution = (uint16)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dpi));

		// 描画領域を作成
		app->draw_window[app->window_num] =
			CreateDrawWindow(width, height, 4, app->labels->make_new.name,
			app->note_book, app->window_num, app);
		app->active_window = app->window_num;
		app->draw_window[app->active_window]->resolution =
			(uint16)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dpi));
		gtk_color_button_get_color(GTK_COLOR_BUTTON(second_color), &second_back_rgb);
		app->menu_data.make_new.second_bg_color[0] = second_back_rgb.red / 256;
		app->menu_data.make_new.second_bg_color[1] = second_back_rgb.green / 256;
		app->menu_data.make_new.second_bg_color[2] = second_back_rgb.blue / 256;
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
		app->draw_window[app->active_window]->second_back_ground[2] = app->menu_data.make_new.second_bg_color[0];
		app->draw_window[app->active_window]->second_back_ground[1] = app->menu_data.make_new.second_bg_color[1];
		app->draw_window[app->active_window]->second_back_ground[0] = app->menu_data.make_new.second_bg_color[2];
#else
		app->draw_window[app->active_window]->second_back_ground[0] = app->menu_data.make_new.second_bg_color[0];
		app->draw_window[app->active_window]->second_back_ground[1] = app->menu_data.make_new.second_bg_color[1];
		app->draw_window[app->active_window]->second_back_ground[2] = app->menu_data.make_new.second_bg_color[2];
#endif
		app->window_num++;

		ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

		// アクティブレイヤーの表示
		LayerViewSetActiveLayer(
			app->draw_window[app->active_window]->active_layer,
			app->layer_window.view
		);

		// ウィンドウのタイトルバーを「新規作成」に
		gtk_window_set_title(GTK_WINDOW(app->window), app->labels->make_new.name);
		
		// 無効にしていた一部のメニューを有効に
		for(i=0; i<app->menus.num_disable_if_no_open; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
		}
		gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
		gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);
	}

	// ダイアログボックスを閉じる
	gtk_widget_destroy(dialog);
}

/*****************************************************
* ExecuteMakeColorLayer関数                          *
* 通常レイヤー作成を実行                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteMakeColorLayer(APPLICATION* app)
{
	// アクティブな描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// 作成したレイヤーのアドレス
	LAYER* layer;
	// 新規作成するレイヤーの名前
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// レイヤーの名前決定用のカウンタ
	int counter = 1;

	//AUTO_SAVE(app->draw_window[app->active_window]);

	// 「レイヤー○」の○に入る値を決定
		// (レイヤー名の重複を避ける)
	do
	{
		(void)sprintf(layer_name, "%s %d", app->labels->layer_window.new_layer, counter);
		counter++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// 現在のアクティブレイヤーが通常レイヤー以外ならばツール変更
	if(window->active_layer->layer_type != TYPE_NORMAL_LAYER)
	{
		gtk_widget_destroy(app->tool_window.brush_table);
		CreateBrushTable(app, &app->tool_window, app->tool_window.brushes);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(app->tool_window.brush_scroll),
			app->tool_window.brush_table);
		gtk_widget_show_all(app->tool_window.brush_table);
	}

	// レイヤー作成
	layer =CreateLayer(
		0,
		0,
		window->width,
		window->height,
		window->channel,
		TYPE_NORMAL_LAYER,
		window->active_layer,
		window->active_layer->next,
		layer_name,
		window
	);
	// 作業レイヤーのピクセルデータを初期化
	(void)memset(window->work_layer->pixels, 0, layer->stride*layer->height);

	// 局所キャンバスモードなら親キャンバスでもレイヤー作成
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = app->draw_window[app->active_window];
		LAYER *parent_prev = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next = (parent_prev == NULL) ? parent->layer : parent_prev->next;
		LAYER *parent_new = CreateLayer(0, 0, parent->width, parent->height, parent->channel,
			(eLAYER_TYPE)layer->layer_type, parent_prev, parent_next, layer->name, parent);
		parent->num_layer++;
		AddNewLayerHistory(parent_new, parent_new->layer_type);
	}

	// レイヤーの数を更新
	window->num_layer++;

	// レイヤーウィンドウに追加してアクティブレイヤーに
	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);

	// 「新規レイヤー」の履歴を登録
	AddNewLayerHistory(layer, layer->layer_type);
}

/*****************************************************
* ExecuteMakeVectorLayer関数                         *
* 新規ベクトルレイヤー作成を実行                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteMakeVectorLayer(APPLICATION *app)
{
	// アクティブな描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// 新規作成したレイヤーのアドレスを受け取る
	LAYER* layer;
	// 作成するレイヤーの名前
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// レイヤー名決定用のカウンタ
	int counter = 1;

	AUTO_SAVE(app->draw_window[app->active_window]);

	// 「ベクトル○」の○に入る数値を決定
		// (レイヤー名の重複を避ける)
	do
	{
		(void)sprintf(layer_name, "%s %d", app->labels->layer_window.new_vector, counter);
		counter++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// アクティブレイヤーがベクトルレイヤーでなければツール変更
	if(window->active_layer->layer_type != TYPE_VECTOR_LAYER)
	{
		gtk_widget_destroy(app->tool_window.brush_table);
		CreateVectorBrushTable(app, &app->tool_window, app->tool_window.vector_brushes);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(app->tool_window.brush_scroll),
			app->tool_window.brush_table);
		gtk_widget_show_all(app->tool_window.brush_table);

		gtk_widget_destroy(app->tool_window.detail_ui);
		app->tool_window.detail_ui =
			app->tool_window.active_vector_brush[app->input]->create_detail_ui(app, app->tool_window.active_vector_brush[app->input]->brush_data);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
		gtk_widget_show_all(app->tool_window.detail_ui);
	}

	// レイヤー作成
	layer = CreateLayer(
		0,
		0,
		window->width,
		window->height,
		window->channel,
		TYPE_VECTOR_LAYER,
		window->active_layer,
		window->active_layer->next,
		layer_name,
		window
	);

	// 局所キャンバスモードなら親キャンバスでもレイヤー作成
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = app->draw_window[app->active_window];
		LAYER *parent_prev = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next = (parent_prev == NULL) ? parent->layer : parent_prev->next;
		LAYER *parent_new = CreateLayer(0, 0, parent->width, parent->height, parent->channel,
			(eLAYER_TYPE)layer->layer_type, parent_prev, parent_next, layer->name, parent);
		parent->num_layer++;
		AddNewLayerHistory(parent_new, parent_new->layer_type);
	}
	// レイヤー数を更新
	window->num_layer++;

	// レイヤーウィンドウに登録してアクティブレイヤーに
	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);

	// 「新規ベクトルレイヤー」の履歴を登録
	AddNewLayerHistory(layer, layer->layer_type);
}

/*****************************************************
* ExecuteMakeLayerSet関数                            *
* 新規レイヤーセット作成を実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteMakeLayerSet(APPLICATION *app)
{
	// アクティブな描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// 新規作成したレイヤーのアドレスを受け取る
	LAYER* layer;
	// 作成するレイヤーの名前
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// レイヤー名決定用のカウンタ
	int counter = 1;

	AUTO_SAVE(app->draw_window[app->active_window]);

	// 「レイヤーセット○」の○に入る数値を決定
		// (レイヤー名の重複を避ける)
	do
	{
		(void)sprintf(layer_name, "%s %d", app->labels->layer_window.new_layer_set, counter);
		counter++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// レイヤー作成
	layer = CreateLayer(
		0,
		0,
		window->width,
		window->height,
		window->channel,
		TYPE_LAYER_SET,
		window->active_layer,
		window->active_layer->next,
		layer_name,
		window
	);

	// 局所キャンバスモードなら親キャンバスでもレイヤー作成
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = app->draw_window[app->active_window];
		LAYER *parent_prev = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next = (parent_prev == NULL) ? parent->layer : parent_prev->next;
		LAYER *parent_new = CreateLayer(0, 0, parent->width, parent->height, parent->channel,
			(eLAYER_TYPE)layer->layer_type, parent_prev, parent_next, layer->name, parent);
		parent->num_layer++;
		AddNewLayerHistory(parent_new, parent_new->layer_type);
	}

	// レイヤー数を更新
	app->draw_window[app->active_window]->num_layer++;

	// レイヤーウィンドウに登録してアクティブレイヤーに
	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);

	// 「新規ベクトルレイヤー」の履歴を登録
	AddNewLayerHistory(layer, layer->layer_type);
}

/*****************************************************
* ExecuteMake3DLayer関数                             *
* 3Dモデリングレイヤー作成を実行                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteMake3DLayer(APPLICATION* app)
{
	// アクティブな描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// 新規作成したレイヤーのアドレスを受け取る
	LAYER* layer;
	// 作成するレイヤーの名前
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// レイヤー名決定用のカウンタ
	int counter = 1;

	AUTO_SAVE(app->draw_window[app->active_window]);

	// 「レイヤーセット○」の○に入る数値を決定
		// (レイヤー名の重複を避ける)
	do
	{
		(void)sprintf(layer_name, "%s %d", app->labels->layer_window.new_3d_modeling, counter);
		counter++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// レイヤー作成
	layer = CreateLayer(
		0,
		0,
		window->width,
		window->height,
		window->channel,
		TYPE_3D_LAYER,
		window->active_layer,
		window->active_layer->next,
		layer_name,
		window
	);

	// 局所キャンバスモードなら親キャンバスでもレイヤー作成
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = app->draw_window[app->active_window];
		LAYER *parent_prev = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next = (parent_prev == NULL) ? parent->layer : parent_prev->next;
		LAYER *parent_new = CreateLayer(0, 0, parent->width, parent->height, parent->channel,
			(eLAYER_TYPE)layer->layer_type, parent_prev, parent_next, layer->name, parent);
		parent->num_layer++;
		AddNewLayerHistory(parent_new, parent_new->layer_type);
	}

	// レイヤー数を更新
	window->num_layer++;

	// レイヤーウィンドウに登録してアクティブレイヤーに
	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);

	// 「新規ベクトルレイヤー」の履歴を登録
	AddNewLayerHistory(layer, layer->layer_type);
}

/*****************************************************
* ExecuteUpLayer関数                                 *
* レイヤーの順序を1つ上に変更する                    *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteUpLayer(APPLICATION* app)
{
	DRAW_WINDOW *window;
	LAYER *parent;
	LAYER *before_prev, *after_prev, *before_parent;
	int hierarchy = 0;

	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);
	if(window->active_layer->next == NULL)
	{
		return;
	}

	before_prev = window->active_layer->prev;
	after_prev = window->active_layer->next;
	before_parent = window->active_layer->layer_set;

	if(window->active_layer->layer_type == TYPE_LAYER_SET)
	{
		LAYER *target = window->active_layer->prev;
		LAYER *new_prev = window->active_layer->next;
		LAYER *bottom_child = window->active_layer->prev;
		LAYER *next_target;

		while(bottom_child != NULL && bottom_child->layer_set == window->active_layer)
		{
			before_prev = bottom_child = bottom_child->prev;
		}
		if(bottom_child == NULL && new_prev->layer_type == TYPE_LAYER_SET)
		{
			return;
		}

		window->active_layer->layer_set = window->active_layer->next->layer_set;

		ChangeLayerOrder(window->active_layer, window->active_layer->next,
			&window->layer);

		while(target != NULL && target->layer_set == window->active_layer)
		{
			next_target = target->prev;
			ChangeLayerOrder(target, new_prev, &window->layer);
			target = next_target;
		}

		ClearLayerView(&app->layer_window);
		LayerViewSetDrawWindow(&app->layer_window, window);
	}
	else
	{
		window->active_layer->layer_set = window->active_layer->next->layer_set;

		ChangeLayerOrder(window->active_layer, window->active_layer->next,
			&window->layer);

		gtk_box_reorder_child(
			GTK_BOX(app->layer_window.view),
			window->active_layer->widget->box,
			GetLayerID(window->layer, window->active_layer->prev, window->num_layer)
		);
	}

	AddChangeLayerOrderHistory(window->active_layer, before_prev, after_prev, before_parent);

	parent = window->active_layer->layer_set;
	while(parent != NULL)
	{
		hierarchy++;
		parent = parent->layer_set;
	}

	gtk_alignment_set_padding(GTK_ALIGNMENT(window->active_layer->widget->alignment),
		0, 0, hierarchy * LAYER_SET_DISPLAY_OFFSET, 0);

	ChangeActiveLayer(window, window->active_layer);
}

/*****************************************************
* ExecuteDownLayer関数                               *
* レイヤーの順序を1つ下に変更する                    *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteDownLayer(APPLICATION* app)
{
	DRAW_WINDOW *window;
	LAYER *before_prev, *after_prev, *before_parent;
	int hierarchy = 0;

	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);
	if(window->active_layer->prev == NULL)
	{
		return;
	}

	before_prev = window->active_layer->prev;
	after_prev = window->active_layer->prev->prev;
	before_parent = window->active_layer->layer_set;

	if(window->active_layer->layer_type == TYPE_LAYER_SET)
	{
		LAYER *target = window->active_layer->prev;
		LAYER *new_prev = window->active_layer->prev;
		LAYER *next_target;

		while(new_prev != NULL && new_prev->layer_set == window->active_layer)
		{
			new_prev = new_prev->prev;
		}

		if(new_prev != NULL)
		{
			if(new_prev->layer_type == TYPE_LAYER_SET)
			{
				window->active_layer->layer_set = new_prev;
			}
			else
			{
				window->active_layer->layer_set = new_prev->layer_set;
			}
		}
		else
		{
			window->active_layer->layer_set = NULL;
		}

		ChangeLayerOrder(window->active_layer, window->active_layer->prev->prev,
			&window->layer);

		while(target != NULL && target->layer_set == window->active_layer)
		{
			next_target = target->prev;
			ChangeLayerOrder(target, new_prev, &window->layer);
			target = next_target;
		}
	}
	else
	{
		if(window->active_layer->prev->layer_type == TYPE_LAYER_SET)
		{
			window->active_layer->layer_set = window->active_layer->prev;
		}
		else
		{
			window->active_layer->layer_set = window->active_layer->prev->layer_set;
		}

		ChangeLayerOrder(window->active_layer, window->active_layer->prev->prev,
			&window->layer);
	}

	AddChangeLayerOrderHistory(window->active_layer, before_prev, after_prev, before_parent);

	ClearLayerView(&app->layer_window);
	LayerViewSetDrawWindow(&app->layer_window, window);

	ChangeActiveLayer(window, window->active_layer);
}

/*****************************************************
* ExecuteZoomIn関数                                  *
* 拡大を実行                                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteZoomIn(APPLICATION *app)
{
	// アクティブな描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// アクティブな描画領域の拡大率変更
	window->zoom += 10;
	// 最大値を超えていたら修正
	if(window->zoom > MAX_ZOOM)
	{
		window->zoom = MAX_ZOOM;
	}

	// ナビゲーションの拡大縮小率スライダを動かして
		// 描画領域のリサイズ等を行う
	gtk_adjustment_set_value(app->navigation_window.zoom_slider,
		window->zoom);
}

/*****************************************************
* ExecuteZoomOut関数                                 *
* 縮小を実行                                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteZoomOut(APPLICATION *app)
{
	// アクティブな描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// アクティブな描画領域の拡大率を変更
	window->zoom -= 10;
	// 最小値を割っていたら修正
	if(window->zoom < MIN_ZOOM)
	{
		window->zoom = MIN_ZOOM;
	}

	// ナビゲーションの拡大縮小率スライダを動かして
		// 描画領域のリサイズ等を行う
	gtk_adjustment_set_value(app->navigation_window.zoom_slider,
		window->zoom);
}

/*****************************************************
* ExecuteZoomReset関数                               *
* 等倍表示を実行                                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteZoomReset(APPLICATION* app)
{
	// アクティブな描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// 描画領域がなければ終了
	if(app->window_num == 0)
	{
		return;
	}

	// アクティブな描画領域の拡大率をリセット
	window->zoom = 100;

	// ナビゲーションの拡大縮小率スライダを動かして
		// 描画領域のリサイズ等を行う
	gtk_adjustment_set_value(app->navigation_window.zoom_slider,
		window->zoom);
}

#define ROTATE_STEP 15
/*****************************************************
* ExecuteRotateClockwise関数                         *
* 表示を時計回りに回転                               *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteRotateClockwise(APPLICATION* app)
{
	// 描画領域の情報
	DRAW_WINDOW *window;
	// 新しい角度
	FLOAT_T angle;

	// 描画領域がなければ終了
	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);

	// 現在の角度を取得
	angle = gtk_adjustment_get_value(app->navigation_window.rotate_slider) + ROTATE_STEP;
	// 180度を超えていたら修正
	if(angle > 180)
	{
		angle -= 360;
	}

	// 角度を設定
	gtk_adjustment_set_value(app->navigation_window.rotate_slider, angle);
}

/*****************************************************
* ExecuteRotateCounterClockwise関数                  *
* 表示を反時計回りに回転                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteRotateCounterClockwise(APPLICATION* app)
{
	// 描画領域の情報
	DRAW_WINDOW *window;
	// 新しい角度
	FLOAT_T angle;

	// 描画領域がなければ終了
	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);

	// 現在の角度を取得
	angle = gtk_adjustment_get_value(app->navigation_window.rotate_slider) - ROTATE_STEP;
	// -180度を超えていたら修正
	if(angle < -180)
	{
		angle += 360;
	}

	// 角度を設定
	gtk_adjustment_set_value(app->navigation_window.rotate_slider, angle);
}

/*****************************************************
* ExecuteRotateReset関数                             *
* 回転角度をリセットする                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteRotateReset(APPLICATION* app)
{
	// 回転角度に0を設定
	gtk_adjustment_set_value(app->navigation_window.rotate_slider, 0);
}

/*********************************************************
* ExecuteDisplayReverseHorizontally関数                  *
* 表示を左右反転                                         *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
void ExecuteDisplayReverseHorizontally(GtkWidget* menu_item, APPLICATION* app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	GtkAdjustment *scroll_x_adjust;
	GtkAllocation allocation;
	double x, max_x;

	gboolean state = gtk_check_menu_item_get_active(
		GTK_CHECK_MENU_ITEM(menu_item));

	if((app->flags & APPLICATION_IN_REVERSE_OPERATION) == 0)
	{
		app->flags |= APPLICATION_IN_REVERSE_OPERATION;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->reverse_button), state);
		app->flags &= ~(APPLICATION_IN_REVERSE_OPERATION);
	}

	if(state == FALSE)
	{
		window->flags &= ~(DRAW_WINDOW_DISPLAY_HORIZON_REVERSE);
	}
	else
	{
		window->flags |= DRAW_WINDOW_DISPLAY_HORIZON_REVERSE;
	}

#if GTK_MAJOR_VERSION >= 3
	gtk_widget_get_allocation(window->scroll, &allocation);
#else
	allocation = window->scroll->allocation;
#endif
	scroll_x_adjust = gtk_scrolled_window_get_hadjustment(
		GTK_SCROLLED_WINDOW(window->scroll));
	x = gtk_adjustment_get_value(scroll_x_adjust);
	max_x = gtk_adjustment_get_upper(scroll_x_adjust);
	gtk_adjustment_set_value(scroll_x_adjust, max_x - x - allocation.width);

	// 画面更新することでナビゲーションとプレビューの内容を更新
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

/*********************************************************
* ExecuteChangeToolWindowPlace関数                       *
* ツールボックスの位置を変更する                         *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
void ExecuteChangeToolWindowPlace(GtkWidget* menu_item, APPLICATION* app)
{
	HSV hsv, back_hsv;
	uint8 fg_color[3], bg_color[3];
	GtkWidget *box;

	if((app->flags & APPLICATION_INITIALIZED) == 0
		|| (app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) != 0)
	{
		return;
	}

	// ペーンの座標を記憶
	if(gtk_paned_get_child1(GTK_PANED(app->left_pane)) != NULL)
	{
		app->left_pane_position = gtk_paned_get_position(GTK_PANED(app->left_pane));
	}
	if(gtk_paned_get_child2(GTK_PANED(app->right_pane)) != NULL)
	{
		app->right_pane_position = gtk_paned_get_position(GTK_PANED(app->right_pane));
	}

	// 現在の色を記憶
	hsv = app->tool_window.color_chooser->hsv;
	back_hsv = app->tool_window.color_chooser->back_hsv;
	(void)memcpy(fg_color, app->tool_window.color_chooser->rgb, 3);
	(void)memcpy(bg_color, app->tool_window.color_chooser->back_rgb, 3);

	if(menu_item != NULL)
	{
		app->tool_window.place = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu_item), "change_mode"));
	}

	switch(app->tool_window.place)
	{
	case UTILITY_PLACE_WINDOW:
		if(app->tool_window.window == NULL
			&& (app->flags & APPLICATION_IN_DELETE_EVENT) == 0)
		{
			if(app->tool_window.ui != NULL)
			{
				gtk_widget_destroy(app->tool_window.ui);
			}

			app->tool_window.flags &= ~(TOOL_DOCKED);
			app->tool_window.window = CreateToolBoxWindow(app, app->window);

			gtk_widget_show_all(app->tool_window.window);
		}
		break;
	case UTILITY_PLACE_LEFT:
		if(app->tool_window.window != NULL)
		{
			gtk_widget_destroy(app->tool_window.window);
		}

		if(app->tool_window.ui != NULL)
		{
			gtk_widget_destroy(app->tool_window.ui);
		}

		app->tool_window.flags |= TOOL_DOCKED;
		app->tool_window.flags &= ~(TOOL_PLACE_RIGHT);

		app->tool_window.window = CreateToolBoxWindow(app, app->window);
		gtk_widget_show_all(app->tool_window.ui);

		break;
	case UTILITY_PLACE_RIGHT:
		if(app->tool_window.window != NULL)
		{
			gtk_widget_destroy(app->tool_window.window);
		}

		if(app->tool_window.ui != NULL)
		{
			gtk_widget_destroy(app->tool_window.ui);
		}

		app->tool_window.flags |= TOOL_DOCKED;
		app->tool_window.flags |= TOOL_PLACE_RIGHT;

		app->tool_window.window = CreateToolBoxWindow(app, app->window);
		gtk_widget_show_all(app->tool_window.ui);

		break;
	}

	if((app->tool_window.flags & TOOL_USING_BRUSH) == 0)
	{
		app->tool_window.detail_ui = app->tool_window.active_common_tool->create_detail_ui(
			app, app->tool_window.active_common_tool->tool_data);
		gtk_scrolled_window_add_with_viewport(
			GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
		gtk_widget_show_all(app->tool_window.detail_ui);
	}
	else
	{
		if(app->window_num == 0 || app->current_tool == TYPE_NORMAL_LAYER)
		{
			app->tool_window.detail_ui = app->tool_window.active_brush[app->input]->create_detail_ui(
				app, app->tool_window.active_brush[app->input]);
			gtk_scrolled_window_add_with_viewport(
				GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
			gtk_widget_show_all(app->tool_window.detail_ui);
		}
		else
		{
			app->current_tool = TYPE_NORMAL_LAYER;
			if(menu_item != NULL)
			{
				ChangeActiveLayer(app->draw_window[app->active_window], app->draw_window[app->active_window]->active_layer);
			}
		}
	}

	(void)memcpy(app->tool_window.color_chooser->back_rgb, bg_color, 3);
	app->tool_window.color_chooser->back_hsv = back_hsv;
	(void)memcpy(app->tool_window.color_chooser->rgb, fg_color, 3);
	SetColorChooserPoint(app->tool_window.color_chooser, &hsv, TRUE);

	box = gtk_paned_get_child1(GTK_PANED(app->left_pane));
	if(box != NULL)
	{
		GList *child = gtk_container_get_children(GTK_CONTAINER(box));
		if(child == NULL)
		{
			gtk_widget_destroy(box);
		}
		g_list_free(child);
	}

	box = gtk_paned_get_child2(GTK_PANED(app->right_pane));
	if(box != NULL)
	{
		GList *child = gtk_container_get_children(GTK_CONTAINER(box));
		if(child == NULL)
		{
			gtk_widget_destroy(box);
		}
		g_list_free(child);
	}
}

/*********************************************************
* ExecuteChangeNavigationLayerWindowPlace関数            *
* ナビゲーションとレイヤービューの位置を変更する         *
* 引数                                                   *
* menu_item	: 位置変更メニューアイテムウィジェット       *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
void ExecuteChangeNavigationLayerWindowPlace(GtkWidget* menu_item, APPLICATION* app)
{
	// ウィンドウとドッキング時にウィジェットを入れるパッキングボックス
	GtkWidget *box;

	if((app->flags & APPLICATION_INITIALIZED) == 0
		|| (app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) != 0)
	{
		return;
	}

	// ペーンの座標を記憶
	if(gtk_paned_get_child1(GTK_PANED(app->left_pane)) != NULL)
	{
		app->left_pane_position = gtk_paned_get_position(GTK_PANED(app->left_pane));
	}
	if(gtk_paned_get_child2(GTK_PANED(app->right_pane)) != NULL)
	{
		app->right_pane_position = gtk_paned_get_position(GTK_PANED(app->right_pane));
	}

	if(menu_item != NULL)
	{
		app->layer_window.place = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu_item), "change_mode"));
	}

	switch(app->layer_window.place)
	{
	case UTILITY_PLACE_WINDOW:
		app->layer_window.flags &= ~(LAYER_WINDOW_DOCKED | LAYER_WINDOW_PLACE_RIGHT);
		if(app->layer_window.window == NULL)
		{
			if(app->navi_layer_pane != NULL)
			{
				app->layer_window.pane_position = gtk_paned_get_position(
					GTK_PANED(app->navi_layer_pane));
				gtk_widget_destroy(app->navi_layer_pane);
				app->navi_layer_pane = NULL;
			}

			app->layer_window.window = CreateLayerWindow(app, app->window, &app->layer_window.view);
			gtk_widget_show_all(app->layer_window.window);
		}

		if(app->navigation_window.window == NULL)
		{
			InitializeNavigation(&app->navigation_window, app, NULL);
		}

		box = gtk_paned_get_child1(GTK_PANED(app->left_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->right_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		break;
	case UTILITY_PLACE_LEFT:
		app->layer_window.flags |= LAYER_WINDOW_DOCKED;
		app->layer_window.flags &= ~(LAYER_WINDOW_PLACE_RIGHT);

		if(app->layer_window.window != NULL)
		{
			gtk_widget_destroy(app->layer_window.window);
		}

		if(app->navigation_window.window != NULL)
		{
			gtk_widget_destroy(app->navigation_window.window);
		}

		if(app->navi_layer_pane != NULL)
		{
			app->layer_window.pane_position = gtk_paned_get_position(
					GTK_PANED(app->navi_layer_pane));
			gtk_widget_destroy(app->navi_layer_pane);
		}

		app->navi_layer_pane = gtk_vpaned_new();
		gtk_paned_set_position(GTK_PANED(app->navi_layer_pane), app->layer_window.pane_position);
		InitializeNavigation(&app->navigation_window, app, app->navi_layer_pane);
		app->layer_window.window = CreateLayerWindow(app, app->window, &app->layer_window.view);

		box = gtk_paned_get_child1(GTK_PANED(app->left_pane));
		if(box == NULL)
		{
			box = gtk_hbox_new(FALSE, 0);
			gtk_paned_pack1(GTK_PANED(app->left_pane), box, TRUE, FALSE);
			gtk_widget_show_all(box);
		}
		gtk_box_pack_start(GTK_BOX(box), app->navi_layer_pane, TRUE, TRUE, 0);
		gtk_box_reorder_child(GTK_BOX(box), app->navi_layer_pane, 0);

		gtk_widget_show_all(app->navi_layer_pane);

		box = gtk_paned_get_child1(GTK_PANED(app->left_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->right_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		break;
	case UTILITY_PLACE_RIGHT:
		app->layer_window.flags |= LAYER_WINDOW_DOCKED | LAYER_WINDOW_PLACE_RIGHT;

		if(app->layer_window.window != NULL)
		{
			gtk_widget_destroy(app->layer_window.window);
		}

		if(app->navigation_window.window != NULL)
		{
			gtk_widget_destroy(app->navigation_window.window);
		}

		if(app->navi_layer_pane != NULL)
		{
			app->layer_window.pane_position = gtk_paned_get_position(
					GTK_PANED(app->navi_layer_pane));
			gtk_widget_destroy(app->navi_layer_pane);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->right_pane));
		if(box == NULL)
		{
			box = gtk_hbox_new(FALSE, 0);
			gtk_paned_pack2(GTK_PANED(app->right_pane), box, TRUE, FALSE);
			gtk_widget_show_all(box);
		}

		app->navi_layer_pane = gtk_vpaned_new();
		gtk_paned_set_position(GTK_PANED(app->navi_layer_pane), app->layer_window.pane_position);
		InitializeNavigation(&app->navigation_window, app, app->navi_layer_pane);
		app->layer_window.window = CreateLayerWindow(app, app->window, &app->layer_window.view);

		gtk_box_pack_end(GTK_BOX(box), app->navi_layer_pane, TRUE, TRUE, 0);

		gtk_widget_show_all(app->navi_layer_pane);

		box = gtk_paned_get_child1(GTK_PANED(app->left_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->right_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		break;
	}
}

/*********************************************************
* ExecuteChangeDisplayPreview関数                        *
* プレビューの表示/非表示を切り替える                    *
* 引数                                                   *
* menu_item	: 表示切り替えメニューアイテムウィジェット   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteChangeDisplayPreview(GtkWidget* menu_item, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_DELETE_EVENT) != 0
		|| (app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) != 0)
	{
		return;
	}

	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)) == FALSE)
	{
		if(app->preview_window.window != NULL)
		{
			gtk_widget_destroy(app->preview_window.window);
			app->preview_window.window = NULL;
		}
	}
	else
	{
		if(app->preview_window.window == NULL)
		{
			InitializePreviewWindow(&app->preview_window, app);
			gtk_widget_show_all(app->preview_window.window);
		}
	}
}

/*********************************************************
* ExecuteSwitchFullScreen関数                            *
* フルスクリーンの切り替えを行う                         *
* 引数                                                   *
* menu_item	: 表示切り替えメニューアイテムウィジェット   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteSwitchFullScreen(GtkWidget* menu_item, APPLICATION* app)
{
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)) == FALSE)
	{
		gtk_window_unfullscreen(GTK_WINDOW(app->window));
	}
	else
	{
		gtk_window_fullscreen(GTK_WINDOW(app->window));
	}
}

/*********************************************************
* ExecuteMoveToolWindowTopLeft関数                       *
* ツールボックスウィンドウを左上に移動する               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteMoveToolWindowTopLeft(APPLICATION* app)
{
	if(app->tool_window.window != NULL)
	{
		gtk_window_move(GTK_WINDOW(app->tool_window.window), 0, 0);
	}
}

/*********************************************************
* ExecuteMoveLayerWindowTopLeft関数                      *
* レイヤーウィンドウを左上に移動する                     *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteMoveLayerWindowTopLeft(APPLICATION* app)
{
	if(app->layer_window.window != NULL)
	{
		gtk_window_move(GTK_WINDOW(app->layer_window.window), 0, 0);
	}
}

/*********************************************************
* ExecuteMoveNavigationWindowTopLeft関数                 *
* ナビゲーションウィンドウを左上に移動する               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void ExecuteMoveNavigationWindowTopLeft(APPLICATION* app)
{
	if(app->navigation_window.window != NULL)
	{
		gtk_window_move(GTK_WINDOW(app->navigation_window.window), 0, 0);
	}
}

#ifdef __cplusplus
}
#endif
