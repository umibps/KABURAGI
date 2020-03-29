// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include "../../memory.h"
#include "../../application.h"
#include "../../utils.h"
#include "../../menu.h"
#include "input_gtk.h"
#include "gtk_widgets.h"

#ifdef _OPENMP
# include <omp.h>
#endif

typedef struct _SPLASH_WINDOW
{
	APPLICATION *app;
	GtkWidget *window;
	GtkWidget *image;
	guint timer_id;
} SPLASH_WINDOW;

typedef struct _REALIZE_DATA
{
	guint signal_id;
	APPLICATION *app;
} REALIZE_DATA;

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************
* OnCloseMainWindow関数                                  *
* メインウィンドウが閉じられるときのコールバック関数     *
* 引数                                                   *
* window	: ウィンドウウィジェット                     *
* event		: ウィジェット削除のイベント情報             *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	終了中止:TRUE	終了続行:FALSE                       *
*********************************************************/
static gboolean OnCloseMainWindow(
	GtkWindow* window,
	GdkEvent* event,
	APPLICATION* app
)
{
	return OnQuitApplication(app);
}

/*********************************************************
* OnChangeMainWindowState関数                            *
* ウィンドウのフルスクリーン・最大化が切り替わった時に   *
*   呼び出されるコールバック関数                         *
* window                                                 *
* state                                                  *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	常にFALSE                                            *
*********************************************************/
static gboolean OnChangeMainWindowState(
	GtkWindow* window,
	GdkEventWindowState* state,
	APPLICATION* app
)
{
	if((state->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0)
	{
		app->flags |= APPLICATION_FULL_SCREEN;
	}
	else
	{
		app->flags &= ~(APPLICATION_FULL_SCREEN);
	}

	if((state->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
	{
		app->flags |= APPLICATION_WINDOW_MAXIMIZE;
	}
	else
	{
		app->flags &= ~(APPLICATION_WINDOW_MAXIMIZE);
	}

	return FALSE;
}

/*************************************************************
* Move2ActiveLayer                                           *
* レイヤービューをアクティブなレイヤーにスクロール           *
* widget		: アクティブレイヤーのウィジェット           *
* allocation	: ウィジェットに割り当てられたサイズ         *
* app			: アプリケーションを管理する構造体のアドレス *
*************************************************************/
void Move2ActiveLayer(GtkWidget* widget, GdkRectangle* allocation, APPLICATION* app)
{
	GtkAllocation place;
	LAYER *top, *layer;

	top = app->draw_window[app->active_window]->layer;
	while(top->next != NULL)
	{
		top = top->next;
	}
	layer = top;

	while(layer != NULL)
	{
		if(layer->layer_type == TYPE_LAYER_SET && (layer->flags & LAYER_SET_CLOSE) != 0)
		{
			LAYER *prev = layer->prev;
			LayerSetHideChildren(layer, &prev);
			layer = prev;
		}
		else
		{
			layer = layer->prev;
		}
	}

	gtk_widget_get_allocation(widget, &place);
	gtk_range_set_value(GTK_RANGE(gtk_scrolled_window_get_vscrollbar(
		GTK_SCROLLED_WINDOW(app->layer_window.scrolled_window))), place.y);
	g_signal_handler_disconnect(G_OBJECT(widget),
		GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), "signal_id")));
}

/*****************************************************************
* OnChangeCurrentTab関数                                         *
* 描画領域のタブが変わった時の関数                               *
* 引数                                                           *
* note_book			: タブウィジェット                           *
* note_book_page	: タブの情報                                 *
* page				: タブのID                                   *
* app				: アプリケーションを管理する構造体のアドレス *
*****************************************************************/
static void OnChangeCurrentTab(
	GtkNotebook* note_book,
	GtkWidget* note_book_page,
	gint page,
	APPLICATION* app
)
{
	// レイヤービューのウィジェットを全て削除
		// レイヤービューのリスト
	ClearLayerView(&app->layer_window);

	// 描画領域変更中のフラグを立てる
	app->flags |= APPLICATION_IN_SWITCH_DRAW_WINDOW;

	// 描画領域の新規作成中でなければ新たにアクティブになる描画領域のレイヤーをビューに追加
	if((app->flags & APPLICATION_IN_MAKE_NEW_DRAW_AREA) == 0)
	{
		DRAW_WINDOW* window;	// アクティブにする描画領域
		LAYER* layer;			// ビューに追加するレイヤー
		int i;					// for文用のカウンタ

		// アクティブな描画領域をセット
		app->active_window = (int16)page;
		window = GetActiveDrawWindow(app);

		if(window->focal_window != NULL)
		{
			window = window->focal_window;
		}

		// 一番下のレイヤーを設定
		layer = window->layer;

		// ビューに全てのレイヤーを追加
		for(i=0; i<window->num_layer && layer != NULL; i++)
		{
			LayerViewAddLayer(layer, window->layer,
				app->layer_window.view, i+1);
			layer = layer->next;
		}
		window->num_layer = (uint16)i;

		// アクティブレイヤーをセット
		LayerViewSetActiveLayer(window->active_layer, app->layer_window.view);

		// ナビゲーションの拡大縮小率を変更
		gtk_adjustment_set_value(app->navigation_window.zoom_slider,
			window->zoom);

		// 表示を反転の設定
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->menu_data.reverse_horizontally),
			window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE);

		// 背景色変更のメニューの状態を設定
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->menus.change_back_ground_menu),
			window->flags & DRAW_WINDOW_SECOND_BG);
		if(app->layer_window.change_bg_button != NULL)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->layer_window.change_bg_button),
				window->flags & DRAW_WINDOW_SECOND_BG);
		}

		// 描画領域があればナビゲーション、レイヤーウィンドウを更新
		if(app->window_num > 0)
		{
			ChangeNavigationDrawWindow(&app->navigation_window, window);
			FillTextureLayer(window->texture, &app->textures);
			g_object_set_data(G_OBJECT(window->active_layer->widget->box), "signal_id",
				GUINT_TO_POINTER(g_signal_connect(G_OBJECT(window->active_layer->widget->box), "size-allocate",
					G_CALLBACK(Move2ActiveLayer), app)));
			ChangeActiveLayer(window, window->active_layer);
			UpdateWindowTitle(app);

			// 表示用フィルターの状態を設定
			app->display_filter.filter_func = app->tool_window.color_chooser->filter_func =
				g_display_filter_funcs[window->display_filter_mode];
			app->display_filter.filter_data = app->tool_window.color_chooser->filter_data = (void*)app;

			gtk_widget_queue_draw(app->tool_window.color_chooser->widget);
			UpdateColorBox(app->tool_window.color_chooser);
			gtk_widget_queue_draw(app->tool_window.color_chooser->pallete_widget);

			gtk_check_menu_item_set_active(
				GTK_CHECK_MENU_ITEM(app->menus.display_filter_menus[window->display_filter_mode]),
				TRUE
			);
		}
		else
		{	// 描画領域が無ければメインウィンドウのキャプションを変更
			char window_title[512];
			(void)sprintf(window_title, "Paint Soft %s %d.%d.%d.%d",
				(GetHas3DLayer(app) == FALSE) ? "KABURAGI" : "MIAKDO",
					MAJOR_VERSION, MINOR_VERSION, RELEASE_VERSION, BUILD_VERSION);
			gtk_window_set_title(GTK_WINDOW(app->widgets->window), window_title);
		}
	}

	// 描画領域切り替え終了
	app->flags &= ~(APPLICATION_IN_SWITCH_DRAW_WINDOW);
}

/*******************************
* CompareFontFamilyName関数    *
* フォントソート用の比較関数   *
* 引数                         *
* font1	: フォントデータ1      *
* font2	: フォントデータ2      *
* 返り値                       *
*	フォント名文字列比較の結果 *
*******************************/
static int CompareFontFamilyName(PangoFontFamily** font1, PangoFontFamily** font2)
{
	// フォントからフォントの名前を取り出す
	const gchar* font_name1, *font_name2;
	font_name1 = pango_font_family_get_name(*font1);
	font_name2 = pango_font_family_get_name(*font2);

	// どちらかがNULLなら終了
	if(font_name1 == NULL)
	{
		return 1;
	}
	if(font_name2 == NULL)
	{
		return -1;
	}

	return strcmp(font_name1, font_name2);
}

/***********************************************************
* ChangeSmoothMethod関数                                   *
* 手ブレ補正の方式を変更するラジオボタンのコールバック関数 *
* 引数                                                     *
* button	: ラジオボタンウィジェット                     *
* smoother	: 手ブレ補正の情報を管理する構造体のアドレス   *
***********************************************************/
static void ChangeSmoothMethod(GtkRadioButton* button, SMOOTHER* smoother)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		smoother->mode = GPOINTER_TO_INT(g_object_get_data(
			G_OBJECT(button), "smooth_method"));
	}
}

/***********************************************************
* ChangeSmootherQuality関数                                *
* 手ブレ補正に使用するサンプル数変更時のコールバック関数   *
* 引数                                                     *
* spin		: 手ブレ補正の品質変更ウィジェットのアジャスタ *
* smoother	: 手ブレ補正の情報を管理する構造体のアドレス   *
***********************************************************/
static void ChangeSmootherQuality(GtkAdjustment* spin, SMOOTHER* smoother)
{
	smoother->num_use = (int)gtk_adjustment_get_value(spin);

#if GTK_MAJOR_VERSION >= 3
	{
		APPLICATION *app = (APPLICATION*)g_object_get_data(
			G_OBJECT(spin), "application");
		int i;
		for(i=0; i<MAX_TOUCH; i++)
		{
			app->tool_window.touch_smoother[i].num_use = smoother->num_use;
		}
	}
#endif
}

/***************************************************************
* ChangeSmootherRate関数                                       *
* 手ブレ補正の適用割合変更時のコールバック関数                 *
* 引数                                                         *
* spin		: 手ブレ補正の適用割合変更ウィジェットのアジャスタ *
* smoother	: 手ブレ補正の情報を管理する構造体のアドレス       *
***************************************************************/
static void ChangeSmootherRate(GtkAdjustment* spin, SMOOTHER* smoother)
{
	smoother->rate = gtk_adjustment_get_value(spin);

#if GTK_MAJOR_VERSION >= 3
	{
		APPLICATION *app = (APPLICATION*)g_object_get_data(
			G_OBJECT(spin), "application");
		int i;
		for(i=0; i<MAX_TOUCH; i++)
		{
			app->tool_window.touch_smoother[i].rate = smoother->rate;
		}
	}
#endif
}

/*********************************************************
* DisplayReverseButtonClicked関数                        *
* 左右反転表示ボタンがクリックされた時のコールバック関数 *
* 引数                                                   *
* button	: 左右反転表示ボタン                         *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void DisplayReverseButtonClicked(GtkWidget* button, APPLICATION* app)
{
	gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	char mark_up_buff[256];

	if((app->flags & APPLICATION_IN_REVERSE_OPERATION) == 0)
	{
		app->flags |= APPLICATION_IN_REVERSE_OPERATION;

		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->menu_data.reverse_horizontally), state);

		app->flags &= ~(APPLICATION_IN_REVERSE_OPERATION);
	}

	if(state == FALSE)
	{
		(void)sprintf(mark_up_buff, "%s", app->labels->window.normal);
	}
	else
	{
		(void)sprintf(mark_up_buff, "<span color=\"red\">%s</span>", app->labels->window.reverse);
	}

	gtk_label_set_markup(GTK_LABEL(app->widgets->reverse_label), mark_up_buff);
}

/*********************************************************
* DisplayEditSelectionButtonClicked関数                  *
* 選択範囲編集ボタンがクリックされた時のコールバック関数 *
* 引数                                                   *
* button	: 選択範囲編集ボタン                         *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void DisplayEditSelectionButtonClicked(GtkWidget* button, APPLICATION* app)
{
	gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	if((app->flags & APPLICATION_IN_EDIT_SELECTION) == 0)
	{

		app->flags |= APPLICATION_IN_EDIT_SELECTION;

		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->menu_data.edit_selection), state);

		app->flags &= ~(APPLICATION_IN_EDIT_SELECTION);
	}

	app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

typedef enum _eDROP_LIST
{
	DROP_URI = 10,
	NUM_DROP_LIST = 1
} eDROP_LIST;

/**********************************************************************
* DragDataRecieveCallBack関数                                         *
* ファイルがドラッグ&ドロップされたときに呼び出されるコールバック関数 *
* 引数                                                                *
* widget			: ドロップ先のウィジェット                        *
* context			: ドラッグ&ドロップの情報                         *
* x					: ドロップされたときのマウスのX座標               *
* y					: ドロップされたときのマウスのY座標               *
* selection_data	: ドラッグ&ドロップのデータ                       *
* target_type		: ドロップされたデータのタイプ                    *
* time_stamp		: ドロップされた時間                              *
* app				: アプリケーションを管理する構造体のアドレス      *
**********************************************************************/
static void DragDataRecieveCallBack(
	GtkWidget* widget,
	GdkDragContext* context,
	gint x,
	gint y,
	GtkSelectionData* selection_data,
	guint target_type,
	guint time_stamp,
	APPLICATION* app
)
{
	if((selection_data != NULL) && (gtk_selection_data_get_length(selection_data) >= 0))
	{
		if(target_type == DROP_URI)
		{
			gchar *uri = (gchar*)gtk_selection_data_get_data(selection_data);
			gchar *file_path = g_filename_from_uri(uri, NULL, NULL);
			gchar *c = file_path;

			while(*c != '\0')
			{
				if(*c == '\r')
				{
					*c = '\0';
					break;
				}
				c = g_utf8_next_char(c);
			}

			OpenFile(file_path, app);

			g_free(file_path);
		}

		gtk_drag_finish(context, TRUE, TRUE, time_stamp);
	}
}

/***************************************************************
* MainWindowRealizeCallBack関数                                *
* メインウィンドウが表示された時に呼び出されるコールバック関数 *
* 引数                                                         *
* window		: ウィンドウウィジェット                       *
* allocation	: ウィンドウのサイズ                           *
* realize_data	: 処理用のデータ                               *
***************************************************************/
static void MainWindowRealizeCallBack(
	GtkWidget* window,
	GdkRectangle* allocation,
	REALIZE_DATA* realize_data
)
{
	GtkWidget *child;
	APPLICATION *app = realize_data->app;

	child = gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane));
	if(child != NULL)
	{
		gtk_paned_set_position(GTK_PANED(app->widgets->left_pane), app->left_pane_position);
	}

	child = gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane));
	if(child != NULL)
	{
		gtk_paned_set_position(GTK_PANED(app->widgets->right_pane), app->right_pane_position);
	}

	gtk_paned_set_position(GTK_PANED(app->tool_window.pane), app->tool_window.pane_position);

	g_signal_handler_disconnect(G_OBJECT(window), realize_data->signal_id);

	MEM_FREE_FUNC(realize_data);
}

/*********************************************************
* SplashWindowTimeOut関数                                *
* 一定時間スプラッシュウィンドウを表示したら呼び出される *
* 引数                                                   *
* window	: スプラッシュウィンドウウィジェット         *
* 返り値                                                 *
*	FALSE(関数呼び出し終了)                              *
*********************************************************/
static int SplashWindowTimeOut(GtkWidget* window)
{
	gtk_widget_destroy(window);

	return FALSE;
}

/*********************************************************
* InitializeSplashWindow関数                             *
* スプラッシュウィンドウの初期化&表示                    *
* 引数                                                   *
* window		: スプラッシュウィンドウのデータアドレス *
*********************************************************/
void InitializeSplashWindow(SPLASH_WINDOW* window)
{
	// イメージ画像のパス(Mac対策)
	gchar *file_path;
	// ウィンドウに設定するタイトル
	char title[256];

	// ウィンドウ作成
	window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	// ウィンドウタイトルを設定
	(void)sprintf(title, "Paint Soft %s", (GetHas3DLayer(window->app) == FALSE) ? "KABURAGI" : "MIKADO");
	gtk_window_set_title(GTK_WINDOW(window->window), title);
	// スプラッシュウィンドウへ
	gtk_window_set_type_hint(GTK_WINDOW(window->window), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
	// イメージをロード
	file_path = g_build_filename(window->app->current_path, "image/splash.png", NULL);
	window->image = gtk_image_new_from_file(file_path);
	g_free(file_path);

	gtk_container_add(GTK_CONTAINER(window->window), window->image);

	// ウィンドウを表示
	gtk_widget_show_all(window->window);

	// 待機中のイベントを処理する
	while(gtk_events_pending() != FALSE)
	{
		gtk_main_iteration();
	}

	// 時間で消去
	window->timer_id = g_timeout_add(2000, (GSourceFunc)SplashWindowTimeOut, window->window);
}

static gboolean UpdateCallback(APPLICATION* app)
{
	DRAW_WINDOW *active;

	active = GetActiveDrawWindow(app);
	if(active != NULL)
	{
		GdkEvent *queued_event = NULL;

		if(app->tool_window.motion_queue.num_items > 0)
		{
			GdkEvent *event_data;
			ExecuteMotionQueue(active);

			event_data = gdk_event_new(GDK_EXPOSE);
			gdk_display_put_event(gtk_widget_get_display(active->window), event_data);
			gdk_event_free(event_data);

			while(gdk_events_pending() != FALSE)
			{
				queued_event = gdk_event_get();
				gtk_main_iteration();
				if(queued_event != NULL)
				{
					if(queued_event->any.type == GDK_EXPOSE)
					{
						gdk_event_free(queued_event);
						break;
					}
					else
					{
						gdk_event_free(queued_event);
					}
				}
				else
				{
					//break;
				}
			}
		}
	}

	return TRUE;
}

/*********************************************************************
* InitializeApplication関数                                          *
* アプリケーションの初期化                                           *
* 引数                                                               *
* app				: アプリケーション全体を管理する構造体のアドレス *
* argv				: main関数の第一引数                             *
* argc				: main関数の第二引数                             *
* init_file_name	: 初期化ファイルの名前                           *
*********************************************************************/
void InitializeApplication(APPLICATION* app, char** argv, int argc, char* init_file_name)
{
	// 各種ディレクトリへのパス(Mac対策)
	gchar *file_path;
	// スプラッシュウィンドウのデータ
	SPLASH_WINDOW splash = {app, NULL, NULL, 0};
	// 初期化データ
	INITIALIZE_DATA init_data;
	// メニューバー
	GtkWidget* menu;
	// フォントソート用
	PangoContext* context;
	// ファイルドロップ対応用
	GtkTargetEntry target_list[] = {{"text/uri-list", 0, DROP_URI}};
	// シングルウィンドウ用の左右のパッキングボックス
	GtkWidget *left_box, *right_box;
	// ウィンドウのタイトル
	char window_title[512];
	// アプリケーションデータのディレクトリ
	char *app_dir_path;

	app->timer_callback_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 1000 / FRAME_RATE,
		(GSourceFunc)UpdateCallback, app, NULL);

	app->widgets = CreateMainWindowWidgets(app);

	app_dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);

	// 初期化ファイルを読み込む
		// まずはアプリケーションデータのディレクトリから
	file_path = g_build_filename(app_dir_path, init_file_name, NULL);
	if(ReadInitializeFile(app, file_path, &init_data) != 0)
	{	// 失敗したらプログラムファイルのディレクトリ
		g_free(file_path);
		file_path =  g_build_filename(app->current_path, init_file_name, NULL);
		if(ReadInitializeFile(app, file_path, &init_data) != 0)
		{	// それも失敗したら終了
			GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Failed to read data file.");
			(void)gtk_dialog_run(GTK_DIALOG(dialog));
			exit(EXIT_FAILURE);
		}
	}
	g_free(file_path);

	// トップレベルウィンドウを作成
	app->widgets->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

#if GTK_MAJOR_VERSION >= 3
	gtk_container_set_reallocate_redraws(GTK_CONTAINER(app->widgets->window), TRUE);
#endif

	// スプラッシュウィンドウ表示でスタートアップが
		// 終了しないようにする
	gtk_window_set_auto_startup_notification(FALSE);
	// スプラッシュウィンドウを表示
	InitializeSplashWindow(&splash);
	// スプラッシュウィンドウを表示したのでスタートアップの
		// 設定を元に戻す
	gtk_window_set_auto_startup_notification(TRUE);

#ifdef _OPENMP
	app->max_threads = omp_get_num_procs();
#endif

	// アプリケーション名の設定
	(void)sprintf(window_title, "Paint Soft %s",
		(GetHas3DLayer(app) == FALSE) ? "KABURAGI" : "MIKADO");
	g_set_application_name(window_title);

	// フルスクリーンの設定
	if((app->flags & APPLICATION_FULL_SCREEN) != 0)
	{
		gtk_window_fullscreen(GTK_WINDOW(app->widgets->window));
	}
	// 最大化の設定
	if((app->flags & APPLICATION_WINDOW_MAXIMIZE) != 0)
	{
		gtk_window_maximize(GTK_WINDOW(app->widgets->window));
	}

	file_path = g_build_filename(app->current_path, "image/icon.png", NULL);
	gtk_window_set_default_icon_from_file(file_path, NULL);
	g_free(file_path);

	(void)sprintf(window_title, "Paint Soft %s %d.%d.%d.%d",
		(GetHas3DLayer(app) == FALSE) ? "KABURAGI" : "MIAKDO",
			MAJOR_VERSION, MINOR_VERSION, RELEASE_VERSION, BUILD_VERSION);
			
	// ラベルの文字列を読み込む
	if(app->language_file_path != NULL)
	{
		char *system_path;
		if(app->language_file_path[0] == '.'
			&& app->language_file_path[1] == '/')
		{
			file_path = g_build_filename(app->current_path,
				&app->language_file_path[2], NULL);
			system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
			LoadLabels(app->labels, app->fractal_labels, system_path);
			g_free(file_path);
		}
		else
		{
			system_path = g_locale_from_utf8(app->language_file_path, -1, NULL, NULL, NULL);
			LoadLabels(app->labels, app->fractal_labels, app->language_file_path);
		}
		g_free(system_path);
	}

	// 拡張デバイスを有効に
#if GTK_MAJOR_VERSION <= 2
	gtk_widget_set_extension_events(app->widgets->window, GDK_EXTENSION_EVENTS_CURSOR);
#endif
	// ウィンドウタイトルを設定
	UpdateWindowTitle(app);
	// ウィンドウの位置を設定
	gtk_window_move(GTK_WINDOW(app->widgets->window), app->window_x, app->window_y);
	gtk_window_resize(GTK_WINDOW(app->widgets->window), app->window_width, app->window_height);
	// ウィンドウが閉じるときのコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(app->widgets->window), "delete_event",
		G_CALLBACK(OnCloseMainWindow), app);
	// キーボードのコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(app->widgets->window), "key-press-event",
		G_CALLBACK(KeyPressEvent), app);
	(void)g_signal_connect(G_OBJECT(app->widgets->window), "key-release-event",
		G_CALLBACK(KeyPressEvent), app);

	// パターンを初期化
	if(app->pattern_path[0] == '.' && app->pattern_path[1] == '/')
	{
		int buffer_size = 0;
		file_path = g_build_filename(app->current_path, &app->pattern_path[2], NULL);
		InitializePattern(&app->patterns, file_path, &buffer_size);
		g_free(file_path);
	}
	else
	{
		int buffer_size = 0;
		InitializePattern(&app->patterns, app->pattern_path, &buffer_size);
	}
	app->patterns.scale = 100;

	// テクスチャを初期化
	if(app->texture_path[0] == '.' && app->texture_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->texture_path[2], NULL);
		LoadTexture(&app->textures, file_path);
		g_free(file_path);
	}
	else
	{
		LoadTexture(&app->textures, app->texture_path);
	}

	// スタンプを初期化
	if(app->stamp_path[0] == '.' && app->stamp_path[1] == '/')
	{
		int buffer_size = 0;
		file_path = g_build_filename(app->current_path, &app->stamp_path[2], NULL);
		InitializePattern(&app->stamps, file_path, &buffer_size);
		g_free(file_path);
	}
	else
	{
		int buffer_size = 0;
		InitializePattern(&app->stamps, app->stamp_path, &buffer_size);
	}
	app->stamp_shape = (uint8*)MEM_ALLOC_FUNC(app->stamps.pattern_max_byte);
	app->stamp_buff_size = app->stamps.pattern_max_byte;

	// メニューバーと描画領域をパッキングするボックスを作成
	app->widgets->vbox = gtk_vbox_new(FALSE, 0);

	// ファイルのドロップを設定
	gtk_drag_dest_set(
		app->widgets->window,
		GTK_DEST_DEFAULT_ALL,
		target_list,
		NUM_DROP_LIST,
		GDK_ACTION_COPY
	);
	gtk_drag_dest_add_uri_targets(app->widgets->window);
	(void)g_signal_connect(G_OBJECT(app->widgets->window), "drag-data-received",
		G_CALLBACK(DragDataRecieveCallBack), app);

	// フルスクリーン・最大化変更時のコールバック関数を設定
	(void)g_signal_connect(G_OBJECT(app->widgets->window), "window_state_event",
		G_CALLBACK(OnChangeMainWindowState), app);

	// スクリプトの読み込み
	if(app->script_path[0] == '.' && app->script_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->script_path[2], NULL);
		InitializeScripts(&app->scripts, file_path);
		g_free(file_path);
	}
	else
	{
		InitializeScripts(&app->scripts, app->script_path);
	}

	// メニューバーを作成
	app->widgets->menu_bar = menu = GetMainMenu(app, app->widgets->window, app->language_file_path);
	// 描画領域のタブを作成
	app->widgets->note_book = gtk_notebook_new();
	// タブは下側に
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(app->widgets->note_book), GTK_POS_BOTTOM);
	// タブの高さを設定
#if GTK_MAJOR_VERSION <= 2
	gtk_notebook_set_tab_vborder(GTK_NOTEBOOK(app->widgets->note_book), 0);
#endif
	// ウィンドウに追加
	gtk_container_add(
		GTK_CONTAINER(app->widgets->window), app->widgets->vbox);
	// ボックスにメニューバーを追加
	gtk_box_pack_start(GTK_BOX(app->widgets->vbox), menu, FALSE, FALSE, 0);

	// ステータスバーを作成し、ウィンドウ下側に配置
	app->widgets->status_bar = gtk_statusbar_new();
	gtk_box_pack_end(GTK_BOX(app->widgets->vbox), app->widgets->status_bar, FALSE, FALSE, 0);
	// プログレスバーは必要に応じて色を変更
	app->widgets->progress = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(app->widgets->status_bar), app->widgets->progress, FALSE, FALSE, 20);
	{
		GtkStyle *window_style, *progress_style;

		window_style = gtk_widget_get_style(app->widgets->window);
		progress_style = gtk_widget_get_style(app->widgets->window);
		if(window_style->bg[GTK_STATE_PRELIGHT].red == progress_style->bg[GTK_STATE_PRELIGHT].red
			&& window_style->bg[GTK_STATE_PRELIGHT].green == progress_style->bg[GTK_STATE_PRELIGHT].green
			&& window_style->bg[GTK_STATE_PRELIGHT].blue == progress_style->bg[GTK_STATE_PRELIGHT].blue
		)
		{
			gtk_widget_modify_bg(app->widgets->progress, GTK_STATE_PRELIGHT, &progress_style->bg[GTK_STATE_SELECTED]);
		}
	}

	// 手ブレ補正の設定変更、拡大・縮小、表示反転、回転、選択範囲編集ウィジェットを作成
	{
		// 手ブレ補正設定変更ウィジェット用のパッキングボックス
		GtkWidget* smooth_box = gtk_hbox_new(FALSE, 0);
		// 手ブレ補正の設定変更ウィジェット
		GtkWidget* smooth_spin;
		// 拡大・縮小用ボタン
		GtkWidget* button;
		// 手ブレ補正の設定の変更用アジャスタ
		GtkAdjustment* smooth_adjustment;
		// 左右反転用のバッファ
		GdkPixbuf *image_buf;
		uint8 *reverse_buf, *image_pixels;
		// ラベルに入れる文字列
		gchar str[256];
		// 画像の幅、高さ、チャンネル数
		gint image_width, image_height;
		gint image_channel;
		int i, j, k;	// for文用のカウンタ

		// 「手ブレ補正」のラベル
		(void)sprintf(str, "%s : ", app->labels->tool_box.smooth);
		gtk_box_pack_start(GTK_BOX(smooth_box),
			gtk_label_new(str), FALSE, FALSE, 0);

		// 手ブレ補正の方式選択ラジオボタン
		button = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.smooth_gaussian);
		g_object_set_data(G_OBJECT(button), "smooth_method", GINT_TO_POINTER(0));
		(void)g_signal_connect(G_OBJECT(button), "toggled",
			G_CALLBACK(ChangeSmoothMethod), &app->tool_window.smoother);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
			GTK_RADIO_BUTTON(button)), app->labels->tool_box.smooth_average);
		g_object_set_data(G_OBJECT(button), "smooth_method", GINT_TO_POINTER(1));
		(void)g_signal_connect(G_OBJECT(button), "toggled",
			G_CALLBACK(ChangeSmoothMethod), &app->tool_window.smoother);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		if(app->tool_window.smoother.mode == SMOOTH_AVERAGE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		}

		// 補正レベル変更ウィジェット
		smooth_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(app->tool_window.smoother.num_use, 0,
			SMOOTHER_POINT_BUFFER_SIZE, 1, 1, 0));
		smooth_spin = gtk_spin_button_new(smooth_adjustment, 1, 1);
#if GTK_MAJOR_VERSION <= 2
		GTK_WIDGET_UNSET_FLAGS(smooth_spin, GTK_CAN_DEFAULT);
#else
		g_object_set_data(G_OBJECT(smooth_adjustment), "application", app);
#endif
		// コールバック関数の設定
		(void)g_signal_connect(G_OBJECT(smooth_adjustment), "value_changed",
			G_CALLBACK(ChangeSmootherQuality), &app->tool_window.smoother);
		// 小数点以下は表示しない
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(smooth_spin), 0);
		gtk_box_pack_start(GTK_BOX(smooth_box), gtk_label_new(
			app->labels->tool_box.smooth_quality), FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(smooth_box), smooth_spin, FALSE, TRUE, 0);

		// 適用割合変更ウィジェット
		smooth_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(app->tool_window.smoother.rate, 0,
			SMOOTHER_RATE_MAX, 1, 1, 0));
		smooth_spin = gtk_spin_button_new(smooth_adjustment, 1, 1);
		gtk_box_pack_start(GTK_BOX(smooth_box), gtk_label_new(
			app->labels->tool_box.smooth_rate), FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(smooth_box), smooth_spin, FALSE, TRUE, 0);
#if GTK_MAJOR_VERSION >= 3
		g_object_set_data(G_OBJECT(smooth_adjustment), "application", app);
#endif
		// コールバック関数の設定
		(void)g_signal_connect(G_OBJECT(smooth_adjustment), "value_changed",
			G_CALLBACK(ChangeSmootherRate), &app->tool_window.smoother);

		// 拡大ボタン
		button = gtk_button_new_with_label(app->labels->menu.zoom_in);
		gtk_widget_set_sensitive(button, FALSE);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteZoomIn), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 3);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 縮小ボタン
		button = gtk_button_new_with_label(app->labels->menu.zoom_out);
		gtk_widget_set_sensitive(button, FALSE);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteZoomOut), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 等倍表示ボタン
		button = gtk_button_new_with_label(app->labels->menu.zoom_reset);
		gtk_widget_set_sensitive(button, FALSE);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteZoomReset), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 表示反転ウィジェット
		app->widgets->reverse_label = gtk_label_new(app->labels->window.normal);
		app->widgets->reverse_button = gtk_toggle_button_new();
		gtk_widget_set_sensitive(app->widgets->reverse_button, FALSE);
		(void)g_signal_connect(G_OBJECT(app->widgets->reverse_button), "toggled",
			G_CALLBACK(DisplayReverseButtonClicked), app);
		gtk_container_add(GTK_CONTAINER(app->widgets->reverse_button), app->widgets->reverse_label);
		gtk_box_pack_start(GTK_BOX(smooth_box), app->widgets->reverse_button, FALSE, FALSE, 3);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = app->widgets->reverse_button;
		app->menus.num_disable_if_no_open++;

		// 反時計回りボタン
		button = gtk_button_new();
		file_path = g_build_filename(app->current_path, "image/counter_clockwise.png", NULL);
		if(app->gui_scale == 1.0)
		{
			gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_file(file_path));
		}
		else
		{
			GdkPixbuf *image_buff = gdk_pixbuf_new_from_file(file_path, NULL);
			GdkPixbuf *old_buf;
			GtkWidget *image_widget;
			old_buf = image_buff;
			image_buff = gdk_pixbuf_scale_simple(old_buf,
				(int)(gdk_pixbuf_get_width(old_buf) * app->gui_scale),
				(int)(gdk_pixbuf_get_height(old_buf) * app->gui_scale),
				GDK_INTERP_NEAREST
			);
			image_widget = gtk_image_new_from_pixbuf(image_buff);
			g_object_unref(old_buf);
			gtk_container_add(GTK_CONTAINER(button), image_widget);
		}
		g_free(file_path);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteRotateCounterClockwise), app);
		gtk_widget_set_sensitive(button, FALSE);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 3);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 時計回りボタン
		button = gtk_button_new();
		file_path = g_build_filename(app->current_path, "image/clockwise.png", NULL);
		if(app->gui_scale == 1.0)
		{
			gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_file(file_path));
		}
		else
		{
			GdkPixbuf *image_buff = gdk_pixbuf_new_from_file(file_path, NULL);
			GdkPixbuf *old_buf;
			GtkWidget *image_widget;
			old_buf = image_buff;
			image_buff = gdk_pixbuf_scale_simple(old_buf,
				(int)(gdk_pixbuf_get_width(old_buf) * app->gui_scale),
				(int)(gdk_pixbuf_get_height(old_buf) * app->gui_scale),
				GDK_INTERP_NEAREST
			);
			image_widget = gtk_image_new_from_pixbuf(image_buff);
			g_object_unref(old_buf);
			gtk_container_add(GTK_CONTAINER(button), image_widget);
		}
		g_free(file_path);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteRotateClockwise), app);
		gtk_widget_set_sensitive(button, FALSE);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 回転リセットボタン
		button = gtk_button_new_with_label(app->labels->menu.reset_rotate);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteRotateReset), app);
		gtk_widget_set_sensitive(button, FALSE);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 元に戻す・やり直しボタン
		file_path = g_build_filename(app->current_path, "image/arrow.png", NULL);
		image_buf = gdk_pixbuf_new_from_file(file_path, NULL);
		if(app->gui_scale != 1.0)
		{
			GdkPixbuf *old_buf = image_buf;
			image_buf = gdk_pixbuf_scale_simple(old_buf,
				(int)(gdk_pixbuf_get_width(old_buf) * app->gui_scale),
				(int)(gdk_pixbuf_get_height(old_buf) * app->gui_scale),
				GDK_INTERP_NEAREST
			);
			g_object_unref(old_buf);
		}
		g_free(file_path);
		button = gtk_button_new();
		gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_pixbuf(image_buf));
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteUndo), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 3);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 画像の情報を取得して左右反転
		image_buf = gdk_pixbuf_copy(image_buf);
		image_width = gdk_pixbuf_get_width(image_buf);
		image_height = gdk_pixbuf_get_height(image_buf);
		image_channel = gdk_pixbuf_get_rowstride(image_buf);
		reverse_buf = (uint8*)MEM_ALLOC_FUNC(image_channel);
		image_channel /= image_width;
		image_pixels = gdk_pixbuf_get_pixels(image_buf);

		for(i=0; i<image_height; i++)
		{
			for(j=0; j<image_width; j++)
			{
				for(k=0; k<image_channel; k++)
				{
					reverse_buf[j*image_channel+k] = image_pixels[
						i*image_width*image_channel+(image_width-j-1)*image_channel+k];
				}
			}

			(void)memcpy(&image_pixels[i*image_width*image_channel], reverse_buf, image_width*image_channel);
		}
		MEM_FREE_FUNC(reverse_buf);

		button = gtk_button_new();
		gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_pixbuf(image_buf));
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteRedo), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 選択範囲編集ボタン
		app->widgets->edit_selection = gtk_toggle_button_new_with_label(app->labels->window.edit_selection);
		gtk_widget_set_sensitive(app->widgets->edit_selection, FALSE);
		gtk_box_pack_end(GTK_BOX(smooth_box), app->widgets->edit_selection, FALSE, FALSE, 0);
		(void)g_signal_connect(G_OBJECT(app->widgets->edit_selection), "toggled",
			G_CALLBACK(DisplayEditSelectionButtonClicked), app);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = app->widgets->edit_selection;
		app->menus.num_disable_if_no_open++;

		gtk_box_pack_start(GTK_BOX(app->widgets->vbox), smooth_box, FALSE, FALSE, 0);
	}

	// 描画領域のタブを追加
		// 先に左右のペーンに入れるパッキングボックスを作成
	app->widgets->left_pane = gtk_hpaned_new();
	app->widgets->right_pane = gtk_hpaned_new();
	// 左右のペーンに作成したパッキングボックスを追加
	left_box = gtk_hbox_new(FALSE, 0);
	right_box = gtk_hbox_new(FALSE, 0);
	gtk_paned_pack1(GTK_PANED(app->widgets->left_pane), left_box, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED(app->widgets->left_pane), app->widgets->right_pane, TRUE, TRUE);
	gtk_paned_pack2(GTK_PANED(app->widgets->right_pane), right_box, TRUE, FALSE);
	// ペーンの間に描画領域のタブを入れる
	gtk_paned_pack1(GTK_PANED(app->widgets->right_pane), app->widgets->note_book, TRUE, TRUE);
	// ウィンドウに追加
	gtk_box_pack_start(GTK_BOX(app->widgets->vbox), app->widgets->left_pane, TRUE, TRUE, 0);

	// デフォルトのウィジェットを空にする
		// (手ぶれ補正にショートカット使用時にフォーカスを防止)
	gtk_window_set_default(GTK_WINDOW(app->widgets->window), NULL);
	gtk_window_set_focus(GTK_WINDOW(app->widgets->window), NULL);

	// タブ変更時のコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(app->widgets->note_book), "switch_page",
		G_CALLBACK(OnChangeCurrentTab), app);

	// フォントのリストを取得
	context = gtk_widget_get_pango_context(app->widgets->window);
	pango_context_list_families(context, &app->font_list, &app->num_font);

	// 文字列順で並べ替える
	qsort(app->font_list, app->num_font, sizeof(*app->font_list),
		(int (*)(const void*, const void*))CompareFontFamilyName);

	// ツールボックスウィンドウを作成
	app->tool_window.window = CreateToolBoxWindow(app, app->widgets->window);
	// パレット情報を読み込む
	file_path = g_build_filename(app_dir_path, PALLETE_FILE_NAME, NULL);
	if(LoadPalleteFile(app->tool_window.color_chooser, file_path) != 0)
	{
		g_free(file_path);
		file_path = g_build_filename(app->current_path, PALLETE_FILE_NAME, NULL);
		(void)LoadPalleteFile(app->tool_window.color_chooser, file_path);
	}
	g_free(file_path);

	// レイヤービューをドッキングするならば
	if((app->layer_window.flags & LAYER_WINDOW_DOCKED) != 0)
	{
		app->widgets->navi_layer_pane = gtk_vpaned_new();
		gtk_paned_set_position(GTK_PANED(app->widgets->navi_layer_pane), app->layer_window.pane_position);

		if((app->layer_window.flags & LAYER_WINDOW_PLACE_RIGHT) == 0)
		{
			gtk_box_pack_start(GTK_BOX(left_box), app->widgets->navi_layer_pane, TRUE, TRUE, 0);
			gtk_box_reorder_child(GTK_BOX(left_box), app->widgets->navi_layer_pane, 0);
		}
		else
		{
			gtk_box_pack_end(GTK_BOX(right_box), app->widgets->navi_layer_pane, TRUE, TRUE, 0);
		}
	}

	// レイヤーウィンドウを作成
	app->layer_window.window =
		CreateLayerWindow(app, app->widgets->window, &app->layer_window.view);

	// ナビゲーションウィンドウを作成
	InitializeNavigation(&app->navigation_window, app, app->widgets->navi_layer_pane);

	// プレビューウィンドウを作成
	InitializePreviewWindow(&app->preview_window, app);

	// ドラッグ&ドロップの間隔を設定
	g_object_set(gtk_settings_get_default(), "gtk-dnd-drag-threshold",
		DND_THRESHOLD, NULL);

	// ウィンドウを表示
	if((app->tool_window.flags & TOOL_DOCKED) == 0)
	{
		gtk_widget_show_all(app->tool_window.window);
	}
	if((app->layer_window.flags & LAYER_WINDOW_DOCKED) == 0)
	{
		gtk_widget_show_all(app->layer_window.window);
		gtk_widget_show_all(app->navigation_window.window);
	}

	// 色データを設定
	{
		HSV hsv;
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		{
			uint8 temp;
			temp = init_data.fg_color[0];
			init_data.fg_color[0] = init_data.fg_color[2];
			init_data.fg_color[2] = temp;

			temp = init_data.bg_color[0];
			init_data.bg_color[0] = init_data.bg_color[2];
			init_data.bg_color[2] = temp;
		}
#endif
		(void)memcpy(app->tool_window.color_chooser->rgb, init_data.fg_color, 3);
		(void)memcpy(app->tool_window.color_chooser->back_rgb, init_data.bg_color, 3);
		RGB2HSV_Pixel(init_data.bg_color, &hsv);
		app->tool_window.color_chooser->back_hsv = hsv;
		RGB2HSV_Pixel(init_data.fg_color, &hsv);
		SetColorChooserPoint(app->tool_window.color_chooser, &hsv, TRUE);
	}
	
	// メインウィンドウを表示
	gtk_widget_show_all(app->widgets->window);

	// 色選択ウィジェットの表示・非表示を設定
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->tool_window.color_chooser->circle_button),
		app->tool_window.flags & TOOL_SHOW_COLOR_CIRCLE);
	if((app->tool_window.flags & TOOL_SHOW_COLOR_CIRCLE) != 0)
	{
		app->tool_window.color_chooser->flags |= COLOR_CHOOSER_SHOW_CIRCLE;
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->tool_window.color_chooser->pallete_button),
		app->tool_window.flags & TOOL_SHOW_COLOR_PALLETE);
	if((app->tool_window.flags & TOOL_SHOW_COLOR_PALLETE) != 0)
	{
		app->tool_window.color_chooser->flags |= COLOR_CHOOSER_SHOW_PALLETE;
	}

	// レイヤー合成の設定
	SetLayerBlendFunctions(app->layer_blend_functions);
	SetPartLayerBlendFunctions(app->part_layer_blend_functions);
	SetLayerBlendOperators(app->layer_blend_operators);

	// フィルターの設定
	SetFilterFunctions(app->filter_funcs);
	SetSelectionFilterFunctions(app->selection_filter_funcs);

	// 吹き出し描画の設定
	SetTextLayerDrawBalloonFunctions(app->draw_balloon_functions);

	// ブラシプレビュー用のキャンバス作成
	app->brush_preview_canvas = CreateTempDrawWindow(BRUSH_PREVIEW_CANVAS_WIDTH,
		BRUSH_PREVIEW_CANVAS_HEIGHT, 4, NULL, NULL, 0, app);
	app->brush_preview_canvas->layer = app->brush_preview_canvas->active_layer =
		CreateLayer(0, 0, app->brush_preview_canvas->width, app->brush_preview_canvas->height, 4, TYPE_NORMAL_LAYER,
			NULL, NULL, "Preview", app->brush_preview_canvas);

	// 入力デバイスの設定
	{
		GList *device_list;
		GdkDevice *device;
		const gchar *device_name;

#if GTK_MAJOR_VERSION <= 2
		device_list = gdk_devices_list();
#else
		device = gdk_device_manager_get_client_pointer(
			gdk_display_get_device_manager(gdk_display_get_default()));
		device_list = gdk_device_list_slave_devices(device);
#endif

#ifdef _WIN32
# if GTK_MAJOR_VERSION <= 2
		while(device_list != NULL)
		{
			device = (GdkDevice*)device_list->data;
			device_name = device->name;
			//device_name = gdk_device_get_name(device);

			if(StringStringIgnoreCase(device_name, "ERASER") != NULL)
			{
				gdk_device_set_source(device, GDK_SOURCE_ERASER);
			}
			device_list = device_list->next;
		}
# else
		{
			GList *check_list = device_list;

			while(check_list != NULL)
			{
				device = (GdkDevice*)check_list->data;
				device_name = gdk_device_get_name(device);

				if(StringStringIgnoreCase(device_name, "ERASER") != NULL)
				{
					g_object_set(G_OBJECT(device), "input-source",
						GINT_TO_POINTER(GDK_SOURCE_ERASER), NULL);
					//gdk_device_set_source(device, GDK_SOURCE_ERASER);
				}

				check_list = check_list->next;
			}
		}
# endif
#else
		app->num_device = 0;
		while(device_list != NULL)
		{
			app->num_device++;
			device_list = device_list->next;
		}

		app->input_sources = (GdkInputSource*)MEM_ALLOC_FUNC(
				sizeof(*app->input_sources)*app->num_device);
		app->set_input_modes = (gboolean*)MEM_ALLOC_FUNC(
				sizeof(*app->set_input_modes)*app->num_device);

#if GTK_MAJOR_VERSION <= 2
		device_list = gdk_devices_list();
#else
		device_list = gdk_device_list_slave_devices(NULL);
#endif
		app->num_device = 0;

		while(device_list != NULL)
		{
			device = (GdkDevice*)device_list->data;
			device_name = gdk_device_get_name(device);
			if(StringStringIgnoreCase(device_name, "CORE") == NULL)
			{
				app->set_input_modes[app->num_device] = TRUE;
			}
			else
			{
				app->set_input_modes[app->num_device] = FALSE;
			}

#if GTK_MAJOR_VERSION <= 2
			if(StringStringIgnoreCase(device_name, "STYLUS") != NULL)
			{
				gdk_device_set_source(device, GDK_SOURCE_PEN);
			}
			else if(StringStringIgnoreCase(device_name, "ERASER") != NULL)
			{
				gdk_device_set_source(device, GDK_SOURCE_ERASER);
#endif
			}
			app->input_sources[app->num_device] = gdk_device_get_source(device);

			device_list = device_list->next;
			app->num_device++;
		}
#endif

#if GTK_MAJOR_VERSION >= 3
		g_list_free(device_list);

		{
			GList *check_list = device_list;
			device_list = gdk_device_manager_list_devices(
				gdk_display_get_device_manager(gdk_display_get_default()), GDK_DEVICE_TYPE_MASTER);
			check_list = device_list;
			while(check_list != NULL)
			{
				device = (GdkDevice*)check_list->data;
				(void)gdk_device_set_mode(device, GDK_MODE_SCREEN);
				check_list = check_list->next;
			}
			g_list_free(device_list);
		}
#endif
	}

	// 左右のペーンにウィジェットが入っていなければ削除
	{
		GList *child;
		REALIZE_DATA *realize = (REALIZE_DATA*)MEM_ALLOC_FUNC(sizeof(*realize));

		(void)memset(realize, 0, sizeof(*realize));
		realize->app = app;

		child = gtk_container_get_children(GTK_CONTAINER(left_box));
		if(child == NULL)
		{
			gtk_widget_destroy(left_box);
		}
		else
		{
			gtk_paned_set_position(GTK_PANED(app->widgets->left_pane), app->left_pane_position);
		}
		g_list_free(child);

		child = gtk_container_get_children(GTK_CONTAINER(right_box));
		if(child == NULL)
		{
			gtk_widget_destroy(right_box);
		}
		else
		{
			gtk_paned_set_position(GTK_PANED(app->widgets->right_pane), app->right_pane_position);
		}
		g_list_free(child);

		// ウィンドウが表示された時にもう一度ペーンの位置を調整
		realize->signal_id = g_signal_connect(G_OBJECT(app->widgets->window), "size-allocate",
			G_CALLBACK(MainWindowRealizeCallBack), realize);
	}

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	// 3Dモデリングの準備
	if(GetHas3DLayer(app) != FALSE)
	{
		app->modeling = ApplicationContextNew(app->window_width, app->window_height,
			app->current_path);
		// ラベルの文字列を読み込む
		if(app->language_file_path != NULL)
		{
			if(app->language_file_path[0] == '.'
				&& app->language_file_path[1] == '/')
			{
				char *system_path;
				file_path = g_build_filename(app->current_path,
					&app->language_file_path[2], NULL);
				system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
				Load3dModelingLabels(app, system_path);

				g_free(file_path);
				g_free(system_path);
			}
			else
			{
				Load3dModelingLabels(app, app->language_file_path);
			}
		}
	}
#endif

	// 初期化済みのフラグを立てる
	app->flags |= APPLICATION_INITIALIZED;

	// バックアップファイルを復元する
	RecoverBackUp(app);
}

#ifdef __cplusplus
}
#endif
