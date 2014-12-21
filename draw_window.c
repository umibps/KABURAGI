// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <math.h>
#include "memory.h"
#include "draw_window.h"
#include "display.h"
#include "layer_window.h"
#include "application.h"
#include "input.h"
#include "widgets.h"
#include "selection_area.h"
#include "memory_stream.h"
#include "image_read_write.h"
#include "utils.h"

# include "MikuMikuGtk+/ui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAW_AREA_FRAME_RATE 60

/*********************************************
* TimerCallBack関数                          *
* 時限(1/60秒)で呼び出されるコールバック関数 *
* 引数                                       *
* window	: 対応する描画領域               *
* 返り値                                     *
*	常にTRUE                                 *
*********************************************/
gboolean TimerCallBack(DRAW_WINDOW* window)
{
	if((window->flags & (DRAW_WINDOW_UPDATE_ACTIVE_OVER | DRAW_WINDOW_UPDATE_ACTIVE_UNDER)) != 0)
	{
		gtk_widget_queue_draw(window->window);
	}
	else if(g_timer_elapsed(window->timer, NULL) >= (FLOAT_T)1/DRAW_AREA_FRAME_RATE)
	{
		g_timer_start(window->timer);

		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			gtk_widget_queue_draw(window->window);
		}
	}

	return TRUE;
}

static gboolean AutoSaveCallBack(DRAW_WINDOW* window)
{
	if(g_timer_elapsed(window->auto_save_timer, NULL) >= window->app->preference.auto_save_time)
	{
		AutoSave(window);
	}

	return TRUE;
}

/*****************************************
* AutoSave関数                           *
* バックアップとしてファイルを保存する   *
* 引数                                   *
* window	: バックアップを取る描画領域 *
*****************************************/
void AutoSave(DRAW_WINDOW* window)
{
	FILE *fp;
	char file_name[4096];
	gchar *path, *system_path, *temp_path;

	(void)sprintf(file_name, "%d.kbt", GetWindowID(window, window->app));
	if(window->app->backup_directory_path[0] == '.'
		&& (window->app->backup_directory_path[1] == '/' || window->app->backup_directory_path[1] == '\\'))
	{
		path = g_build_filename(window->app->current_path, "kabtmp", NULL);
		temp_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);
		g_free(path);
		path = g_build_filename(window->app->current_path, file_name, NULL);
	}
	else
	{
		path = g_build_filename(window->app->backup_directory_path, "kabtmp", NULL);
		temp_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);
		g_free(path);
		path = g_build_filename(window->app->backup_directory_path, file_name, NULL);
	}
	system_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);

	fp = fopen(temp_path, "wb");

	if(fp != NULL)
	{
#if GTK_MAJOR_VERSION >= 3
		GdkWindow *status_window = gtk_widget_get_window(window->app->status_bar);
#endif
		guint context_id = gtk_statusbar_get_context_id(
			GTK_STATUSBAR(window->app->status_bar), "Execute Back Up");
		guint message_id = gtk_statusbar_push(GTK_STATUSBAR(window->app->status_bar),
			context_id, window->app->labels->status_bar.auto_save);
		GdkEvent *queued_event;
		while(gtk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event == NULL)
			{
				break;
			}

#if GTK_MAJOR_VERSION <= 2
			if(queued_event->any.window == window->app->status_bar->window)
#else
			if(queued_event->any.window == status_window)
#endif
			{
				gdk_event_free(queued_event);
				break;
			}
			else
			{
				gdk_event_free(queued_event);
			}
		}

		WriteOriginalFormat((void*)fp, (stream_func_t)fwrite, window, 0, 3);
#ifdef _DEBUG
		(void)printf("Execute Auto Save.\n");
#endif
		(void)fclose(fp);
		(void)remove(system_path);
		(void)rename(temp_path, system_path);
		(void)remove(temp_path);

		gtk_statusbar_remove(GTK_STATUSBAR(window->app->status_bar),
			context_id, message_id);
		while(gtk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event == NULL)
			{
				break;
			}

#if GTK_MAJOR_VERSION <= 2
			if(queued_event->any.window == window->app->status_bar->window)
#else
			if(queued_event->any.window == status_window)
#endif
			{
				gdk_event_free(queued_event);
				break;
			}
			else
			{
				gdk_event_free(queued_event);
			}
		}
	}

	g_free(path);
	g_free(system_path);
	g_free(temp_path);

	g_timer_start(window->auto_save_timer);
}

/***********************************************
* OnCloseDrawWindow関数                        *
* タブが閉じられるときのコールバック関数       *
* 引数                                         *
* data	: 描画領域のデータ                     *
* page	: 閉じるタブのID                       *
* 返り値                                       *
*	閉じる操作の中止:TRUE 閉じる操作続行:FALSE *
***********************************************/
gboolean OnCloseDrawWindow(void* data, gint page)
{
	// 描画領域の情報にキャスト
	DRAW_WINDOW* window = (DRAW_WINDOW*)data;
	// for文用のカウンタ
	int i, j;

	if((window->history.flags & HISTORY_UPDATED) != 0)
	{	// 保存するかどうかのウィンドウを表示
		GtkWidget* dialog = gtk_dialog_new_with_buttons(
			NULL,
			GTK_WINDOW(window->app->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_SAVE,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_NO,
			GTK_RESPONSE_NO,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
		// ダイアログに入れるラベル
		GtkWidget* label = gtk_label_new(window->app->labels->save.close_with_unsaved_chage);
		// ダイアログの選択結果
		gint result;

		// ダイアログにラベルを入れる
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
			label, FALSE, FALSE, 0);
		gtk_widget_show(label);

		// ダイアログを実行
		result = gtk_dialog_run(GTK_DIALOG(dialog));

		// ダイアログを閉じる
		gtk_widget_destroy(dialog);

		// ダイアログの選択によって処理を切り替え
		switch(result)
		{
		case GTK_RESPONSE_ACCEPT:	// 保存実行
			{
				int16 before_id = window->app->active_window;
				window->app->active_window = (int16)GetWindowID(window, window->app);
				ExecuteSave(window->app);
				window->app->active_window = before_id;
			}
			break;
		case GTK_RESPONSE_REJECT:	// キャンセル
			// 閉じる操作を止める
			return TRUE;
		}
	}

	// アプリケーションの描画領域配列を移動
	for(i=0; i<window->app->window_num; i++)
	{
		if(window->app->draw_window[i] == window)
		{
			for(j=i+1; j<window->app->window_num; j++)
			{
				// タブに設定されているページ数を再設定
				g_object_set_data(G_OBJECT(g_object_get_data(G_OBJECT(
					gtk_notebook_get_tab_label(GTK_NOTEBOOK(window->app->note_book),
						window->app->draw_window[j]->scroll)), "button_widget")), "page", GINT_TO_POINTER(j-1));
			}

			if(i <= window->app->active_window)
			{	// アクティブな描画領域を必要に応じて変更
				if(window->app->active_window > 0)
				{
					window->app->active_window--;
				}
			}

			(void)memmove(&window->app->draw_window[i],
				&window->app->draw_window[i+1],
				sizeof(window->app->draw_window[0])*(window->app->window_num-i)
			);
		}
	}
	// 描画領域の数を更新
	window->app->window_num--;

	// 描画領域の削除実行
	DeleteDrawWindow(&window);

	return FALSE;
}

/*****************************************************
* WindowResizeCallBack関数                           *
* ウィンドウのサイズが変更された時のコールバック関数 *
* 引数                                               *
* widget	: 描画領域のウィジェット                 *
* event		: サイズ変更イベントの内容               *
* window	: 描画領域を管理する構造体のアドレス     *
*****************************************************/
static void WindowResizeCallBack(
	GtkWidget *widget,
	GdkEvent *event,
	DRAW_WINDOW *window
)
{
	// 描画領域のサイズ取得用
	GtkAllocation allocation;

	// 現在の描画領域スクロールのサイズを取得
	gtk_widget_get_allocation(window->scroll, &allocation);

	// 現在のスクロールの位置を取得
	window->scroll_x = (int)gtk_range_get_value(GTK_RANGE(gtk_scrolled_window_get_hscrollbar(
		GTK_SCROLLED_WINDOW(window->scroll))));
	window->scroll_y = (int)gtk_range_get_value(GTK_RANGE(gtk_scrolled_window_get_vscrollbar(
		GTK_SCROLLED_WINDOW(window->scroll))));
}

/*********************************************************************
* ScrollCallBack関数                                                 *
* 描画領域のスクロールが操作されたときに呼び出されるコールバック関数 +
* 引数                                                               *
* scroll	: スクロールドウィンドウ                                 *
* window	: 描画領域の情報                                         *
*********************************************************************/
static void ScrollCallBack(
	GtkScrolledWindow* scroll,
	DRAW_WINDOW* window
)
{	// ナビゲーションの内容を更新
	gtk_widget_queue_draw(window->app->navigation_window.draw_area);

	window->scroll_x = (int)gtk_adjustment_get_value(gtk_scrolled_window_get_hadjustment(
		GTK_SCROLLED_WINDOW(window->scroll)));
	window->scroll_y = (int)gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(window->scroll)));

	UpdateDrawWindowClippingArea(window);
}

/***************************************************************
* ScrollChangeSize関数                                         *
* 描画領域のサイズが変更された時に呼び出されるコールバック関数 *
* 引数                                                         *
* scroll		: スクロールドウィンドウ                       *
* allocation	: 設定されたサイズ                             *
* window		: 描画領域の情報                               *
***************************************************************/
static void ScrollSizeChange(
	GtkScrolledWindow* scroll,
	GtkAllocation* allocation,
	DRAW_WINDOW* window
)
{
	UpdateDrawWindowClippingArea(window);
}

/*******************************************************
* SetDrawWindowCallbacks関数                           *
* 描画領域のコールバック関数の設定を行う               *
* 引数                                                 *
* widget	: コールバック関数をセットするウィジェット *
* window	: 描画領域の情報                           *
*******************************************************/
void SetDrawWindowCallbacks(
	GtkWidget* widget,
	DRAW_WINDOW* window
)
{
	window->callbacks.configure = g_signal_connect(G_OBJECT(widget), "configure-event",
		G_CALLBACK(DrawWindowConfigurEvent), window);
#if GTK_MAJOR_VERSION <= 2
	window->callbacks.display = g_signal_connect(G_OBJECT(widget), "expose_event",
		G_CALLBACK(DisplayDrawWindow), window);
#else
	window->callbacks.display = g_signal_connect(G_OBJECT(widget), "draw",
		G_CALLBACK(DisplayDrawWindow), window);
#endif
	// 入力のコールバック関数をセット
	window->callbacks.mouse_button_press =g_signal_connect(G_OBJECT(widget), "button_press_event",
		G_CALLBACK(ButtonPressEvent), window);
	window->callbacks.mouse_move = g_signal_connect(G_OBJECT(widget), "motion_notify_event",
		G_CALLBACK(MotionNotifyEvent), window);
	window->callbacks.mouse_button_release = g_signal_connect(G_OBJECT(widget), "button_release_event",
		G_CALLBACK(ButtonReleaseEvent), window);
	window->callbacks.mouse_wheel = g_signal_connect(G_OBJECT(widget), "scroll_event",
		G_CALLBACK(MouseWheelEvent), window);
#if GTK_MAJOR_VERSION >= 3
	(void)g_signal_connect(G_OBJECT(widget), "touch-event",
		G_CALLBACK(TouchEvent), window);
#endif
	window->callbacks.enter = g_signal_connect(G_OBJECT(widget), "enter_notify_event",
		G_CALLBACK(EnterNotifyEvent), window);
	window->callbacks.leave = g_signal_connect(G_OBJECT(widget), "leave_notify_event",
		G_CALLBACK(LeaveNotifyEvent), window);

	if(widget == window->gl_area)
	{
		window->flags |= DRAW_WINDOW_DISCONNECT_3D;
	}
}

/***************************************************
* DisconnectDrawWindowCallbacks関数                *
* 描画領域のコールバック関数を止める               *
* 引数                                             *
* widget	: コールバック関数を止めるウィジェット *
* window	: 描画領域の情報                       *
***************************************************/
void DisconnectDrawWindowCallbacks(
	GtkWidget* widget,
	DRAW_WINDOW* window
)
{
	if(window->callbacks.display != 0)
	{
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.display);
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.mouse_button_press);
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.mouse_move);
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.mouse_button_release);
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.mouse_wheel);
		window->callbacks.display = 0;
	}

	if(window->callbacks.configure != 0)
	{
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.configure);
		window->callbacks.configure = 0;
	}
}

/***************************************************************
* CreateDrawWindow関数                                         *
* 描画領域を作成する                                           *
* 引数                                                         *
* width		: キャンバスの幅                                   *
* height	: キャンバスの高さ                                 *
* channel	: キャンバスのチャンネル数(RGB:3, RGBA:4)          *
* name		: キャンバスの名前                                 *
* note_book	: 描画領域タブウィジェット                         *
* window_id	: 描画領域配列中のID                               *
* app		: アプリケーションの情報を管理する構造体のアドレス *
* 返り値                                                       *
*	描画領域の情報を管理する構造体のアドレス                   *
***************************************************************/
DRAW_WINDOW* CreateDrawWindow(
	int32 width,
	int32 height,
	uint8 channel,
	const gchar* name,
	GtkWidget* note_book,
	uint16 window_id,
	APPLICATION* app
)
{
	// 返り値
	DRAW_WINDOW* ret =
		(DRAW_WINDOW*)MEM_ALLOC_FUNC(sizeof(*ret));
	// 中央配置用
	GtkWidget *alignment;
	// タブのラベル
	GtkWidget *tab_label;
	// 表示領域のサイズ
	size_t disp_size;
	// 回転処理用の行列データ
	cairo_matrix_t matrix;
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// キャンバスウィジェット
	GtkWidget *canvas;
	GtkWidget *canvas_box;

	// 新規作成描画領域作成中のフラグを立てる
	app->flags |= APPLICATION_IN_MAKE_NEW_DRAW_AREA;

	// 0初期化
	(void)memset(ret, 0, sizeof(*ret));

	// オリジナルの幅と高さを記憶
	ret->original_width = width;
	ret->original_height = height;

	// 高さ・幅を4の倍数に
	width += (4 - (width % 4)) % 4;
	height += (4 - (height % 4)) % 4;

	// 表示サイズを計算
	disp_size = (size_t)(2 * sqrt((width/2)*(width/2)+(height/2)*(height/2)) + 1);

	// 値のセット
	ret->channel = channel;
	ret->file_name = (name == NULL) ? NULL : MEM_STRDUP_FUNC(name);
	ret->width = width;
	ret->height = height;
	ret->stride = width * channel;
	ret->pixel_buf_size = ret->stride * height;
	ret->zoom = 100;
	ret->zoom_rate = 1;
	ret->rev_zoom = 1;
	ret->app = app;
	ret->cursor_x = ret->cursor_y = -50000;

	// 背景のピクセルメモリを確保
	ret->back_ground = (uint8*)MEM_ALLOC_FUNC(sizeof(*ret->back_ground)*ret->pixel_buf_size);
	(void)memset(ret->back_ground, 0xff, sizeof(*ret->back_ground)*ret->pixel_buf_size);

	// ブラシ用のバッファを確保
	ret->brush_buffer = (uint8*)MEM_ALLOC_FUNC(sizeof(*ret->brush_buffer)*ret->pixel_buf_size);

	// 描画用のレイヤーを作成
	ret->disp_layer = CreateLayer(0, 0, width, height, channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, ret);
	ret->scaled_mixed = CreateLayer(0, 0, width, height, channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, ret);

	// レイヤー合成のフラグを立てる
	ret->flags = DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	// 描画領域を作成
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	if(GetHas3DLayer(app) != FALSE)
	{
		ret->first_project = ProjectContextNew(app->modeling,
			width, height, (void**)&ret->gl_area);
		canvas_box = gtk_hbox_new(FALSE, 0);
		canvas = ret->gl_area;
		gtk_box_pack_start(GTK_BOX(canvas_box), ret->gl_area, TRUE, TRUE, 0);
		ret->window = gtk_drawing_area_new();
		gtk_box_pack_start(GTK_BOX(canvas_box), ret->window, TRUE, TRUE, 0);
	}
	else
#endif
	{
		canvas = ret->window = gtk_drawing_area_new();
	}

	// イベントの種類をセット
	gtk_widget_set_events(ret->window,
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_SCROLL_MASK
		| GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_RELEASE_MASK
#if GTK_MAJOR_VERSION >= 3
			| GDK_TOUCH_MASK
#else
			| GDK_POINTER_MOTION_HINT_MASK
#endif
	);
	// 描画領域を入れるスクロールを作成
	ret->scroll = gtk_scrolled_window_new(NULL, NULL);
	// スクロールがリサイズされた時のコールバック関数を設定
	(void)g_signal_connect(G_OBJECT(ret->scroll), "size-allocate",
		G_CALLBACK(ScrollSizeChangeEvent), ret);
	// サイズを設定
	gtk_widget_set_size_request(ret->window, (gint)disp_size, (gint)disp_size);
	// 中央配置の設定
	alignment = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
	// タブのラベルを作成
	tab_label = CreateNotebookLabel(ret->file_name, note_book, window_id,
		OnCloseDrawWindow, ret);
	// スクロール表示の条件をセット
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	// スクロールドウィンドウに描画領域を追加
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ret->scroll), alignment);
	// タブに描画領域を追加
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), ret->scroll, tab_label);
	// スクロール移動時のコールバック関数をセット
	{
		GtkWidget* bar = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(ret->scroll));
		(void)g_signal_connect(G_OBJECT(bar), "value-changed", G_CALLBACK(ScrollCallBack), ret);
		bar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(ret->scroll));
		(void)g_signal_connect(G_OBJECT(bar), "value-changed", G_CALLBACK(ScrollCallBack), ret);
	}
	// 描画領域サイズ変更時のコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(ret->scroll), "size-allocate",
		G_CALLBACK(ScrollSizeChange), ret);
	g_object_set_data(G_OBJECT(ret->window), "draw-window", ret);
	// 背景色を設定
	if((app->flags & APPLICATION_SET_BACK_GROUND_COLOR) != 0)
	{
		GdkColor color = {0};
		color.red = app->preference.canvas_back_ground[0]
			| (app->preference.canvas_back_ground[0] << 8);
		color.green = app->preference.canvas_back_ground[1]
			| (app->preference.canvas_back_ground[1] << 8);
		color.blue = app->preference.canvas_back_ground[2]
			| (app->preference.canvas_back_ground[2] << 8);
		gtk_widget_modify_bg(ret->window, GTK_STATE_NORMAL, &color);
		gtk_widget_modify_bg(ret->scroll, GTK_STATE_NORMAL, &color);
	}

	// 描画用のコールバック関数をセット
	SetDrawWindowCallbacks(canvas, ret);

	// 描画領域を表示
	if(GetHas3DLayer(app) != FALSE)
	{
		gtk_container_add(GTK_CONTAINER(alignment), canvas_box);
		gtk_widget_show_all(ret->scroll);
		gtk_widget_hide(ret->window);
	}
	else
	{
		gtk_container_add(GTK_CONTAINER(alignment), ret->window);
		gtk_widget_show_all(ret->scroll);
	}

#if GTK_MAJOR_VERSION <= 2
	gtk_notebook_set_page(GTK_NOTEBOOK(note_book), window_id);
#else
	gtk_notebook_set_current_page(GTK_NOTEBOOK(note_book), window_id);
#endif

	// 一定時間毎に再描画されるようにコールバック関数をセット
	//ret->timer_id = g_idle_add((GSourceFunc)TimerCallBack, ret);
	ret->timer_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000 / FRAME_RATE,
		(GSourceFunc)TimerCallBack, ret, NULL);
	ret->timer = g_timer_new();
	g_timer_start(ret->timer);

	// レイヤー合成用の関数ポインタをセット
	ret->layer_blend_functions = app->layer_blend_functions;
	ret->part_layer_blend_functions = app->part_layer_blend_functions;

	// 自動保存のコールバック関数をセット
	if(app->preference.auto_save != 0)
	{
		//ret->auto_save_id = g_idle_add((GSourceFunc)AutoSaveCallBack, ret);
		ret->auto_save_id = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT_IDLE, AUTO_SAVE_INTERVAL,
			(GSourceFunc)AutoSaveCallBack, ret, NULL);
		//ret->auto_save_id = g_timeout_add_full(
		//	G_PRIORITY_DEFAULT_IDLE, 1000 / FRAME_RATE, (GSourceFunc)AutoSaveCallBack, ret, NULL);
		ret->auto_save_timer = g_timer_new();
		g_timer_start(ret->auto_save_timer);
	}

#if GTK_MAJOR_VERSION <= 2
	// 拡張デバイスを有効に
	gtk_widget_set_extension_events(ret->window, GDK_EXTENSION_EVENTS_ALL);
#endif

	// レイヤーの名前をセット
	(void)sprintf(layer_name, "%s 1", app->labels->layer_window.new_layer);

	// 最初のレイヤーを作成する
	ret->active_layer = ret->layer = CreateLayer(0, 0, width, height,
		channel, TYPE_NORMAL_LAYER, NULL, NULL, layer_name, ret);
	ret->num_layer++;

	// レイヤーウィンドウに最初のレイヤーを表示
	LayerViewAddLayer(ret->active_layer, ret->active_layer->prev,
		app->layer_window.view, ret->num_layer);

	// 作業用のレイヤーを作成
	ret->work_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// アクティブレイヤーと作業用レイヤーを一時的に合成するレイヤー
	ret->temp_layer = CreateLayer(0, 0, width, height, 5, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// レイヤーを合成したもの
	ret->mixed_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	(void)memcpy(ret->mixed_layer->pixels, ret->back_ground, ret->pixel_buf_size);
	// 合成結果に対して拡大・縮小を設定するためのパターン
	ret->mixed_pattern = cairo_pattern_create_for_surface(ret->mixed_layer->surface_p);
	cairo_pattern_set_filter(ret->mixed_pattern, CAIRO_FILTER_FAST);

	// 選択領域のレイヤーを作成
	ret->selection = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// 一番上でエフェクト表示を行うレイヤーを作成
	ret->effect = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// テクスチャ用のレイヤーを作成
	ret->texture = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// 下のレイヤーでマスキング、バケツツールでのマスキング用
	ret->mask = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	ret->mask_temp = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	// ブラシ描画時の不透明度設定用サーフェース、イメージ
	ret->alpha_surface = cairo_image_surface_create_for_data(ret->mask->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->alpha_cairo = cairo_create(ret->alpha_surface);
	ret->alpha_temp = cairo_image_surface_create_for_data(ret->temp_layer->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->alpha_temp_cairo = cairo_create(ret->alpha_temp);
	ret->gray_mask_temp = cairo_image_surface_create_for_data(ret->mask_temp->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->gray_mask_cairo = cairo_create(ret->gray_mask_temp);

	// 表示用に拡大・縮小した後の一次記憶メモリ
	ret->disp_temp = CreateDispTempLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER, ret);

	// 回転表示用の値を計算
	ret->cos_value = 1;
	ret->trans_x = - ret->half_size + ((width / 2) * ret->cos_value + (height / 2) * ret->sin_value);
	ret->trans_y = - ret->half_size - ((width / 2) * ret->sin_value - (height / 2) * ret->cos_value);
	cairo_matrix_init_rotate(&matrix, 0);
	cairo_matrix_translate(&matrix, ret->trans_x, ret->trans_y);
	ret->rotate = cairo_pattern_create_for_surface(ret->disp_layer->surface_p);
	cairo_pattern_set_filter(ret->rotate, CAIRO_FILTER_FAST);
	cairo_pattern_set_matrix(ret->rotate, &matrix);

	// カーソル座標補正用の値を計算
	ret->add_cursor_x = - (ret->half_size - ret->disp_layer->width / 2) + ret->half_size;
	ret->add_cursor_y = - (ret->half_size - ret->disp_layer->height / 2) + ret->half_size;
	ret->rev_add_cursor_x = ret->disp_layer->width/2 + (ret->half_size - ret->disp_layer->width/2);
	ret->rev_add_cursor_y = ret->disp_layer->height/2 + (ret->half_size - ret->disp_layer->height/2);

	// アクティブレイヤーより下を合成した画像の保存用
	ret->under_active = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	(void)memcpy(ret->under_active->pixels, ret->back_ground, ret->pixel_buf_size);

	// 描画領域の新規作成作成中のフラグを降ろす
	app->flags &= ~(APPLICATION_IN_MAKE_NEW_DRAW_AREA);

	// テクスチャを設定
	FillTextureLayer(ret->texture, &app->textures);

	return ret;
}

/***************************************************************
* CreateTempDrawWindow関数                                     *
* 一時的な描画領域を作成する                                   *
* 引数                                                         *
* width		: キャンバスの幅                                   *
* height	: キャンバスの高さ                                 *
* channel	: キャンバスのチャンネル数(RGB:3, RGBA:4)          *
* name		: キャンバスの名前                                 *
* note_book	: 描画領域タブウィジェット                         *
* window_id	: 描画領域配列中のID                               *
* app		: アプリケーションの情報を管理する構造体のアドレス *
* 返り値                                                       *
*	描画領域の情報を管理する構造体のアドレス                   *
***************************************************************/
DRAW_WINDOW* CreateTempDrawWindow(
	int32 width,
	int32 height,
	uint8 channel,
	const gchar* name,
	GtkWidget* note_book,
	uint16 window_id,
	APPLICATION* app
)
{
	// 返り値
	DRAW_WINDOW* ret =
		(DRAW_WINDOW*)MEM_ALLOC_FUNC(sizeof(*ret));
	// 表示領域のサイズ
	size_t disp_size;
	// 回転処理用の行列データ
	cairo_matrix_t matrix;

	// 新規作成描画領域作成中のフラグを立てる
	app->flags |= APPLICATION_IN_MAKE_NEW_DRAW_AREA;

	// 0初期化
	(void)memset(ret, 0, sizeof(*ret));

	// オリジナルの幅と高さを記憶
	ret->original_width = width;
	ret->original_height = height;

	// 高さ・幅を4の倍数に
	width += (4 - (width % 4)) % 4;
	height += (4 - (height % 4)) % 4;

	// 表示サイズを計算
	disp_size = (size_t)(2 * sqrt((width/2)*(width/2)+(height/2)*(height/2)) + 1);

	// 値のセット
	ret->channel = channel;
	ret->file_name = (name == NULL) ? NULL : MEM_STRDUP_FUNC(name);
	ret->width = width;
	ret->height = height;
	ret->stride = width * channel;
	ret->pixel_buf_size = ret->stride * height;
	ret->zoom = 100;
	ret->zoom_rate = 1;
	ret->rev_zoom = 1;
	ret->app = app;

	// 背景のピクセルメモリを確保
	ret->back_ground = (uint8*)MEM_ALLOC_FUNC(sizeof(*ret->back_ground)*ret->pixel_buf_size);
	(void)memset(ret->back_ground, 0xff, sizeof(*ret->back_ground)*ret->pixel_buf_size);

	// ブラシ用のバッファを確保
	ret->brush_buffer = (uint8*)MEM_ALLOC_FUNC(sizeof(*ret->brush_buffer)*ret->pixel_buf_size);

	// 描画用のレイヤーを作成
	ret->disp_layer = CreateLayer(0, 0, width, height, channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, ret);
	ret->scaled_mixed = CreateLayer(0, 0, width, height, channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, ret);

	// レイヤー合成のフラグを立てる
	ret->flags = DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

#if 0 // GTK_MAJOR_VERSION <= 2
	// 拡張デバイスを有効に
	gtk_widget_set_extension_events(ret->window, GDK_EXTENSION_EVENTS_ALL);
#endif

	// 作業用のレイヤーを作成
	ret->work_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// アクティブレイヤーと作業用レイヤーを一時的に合成するレイヤー
	ret->temp_layer = CreateLayer(0, 0, width, height, 5, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// レイヤーを合成したもの
	ret->mixed_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	(void)memcpy(ret->mixed_layer->pixels, ret->back_ground, ret->pixel_buf_size);
	// 合成結果に対して拡大・縮小を設定するためのパターン
	ret->mixed_pattern = cairo_pattern_create_for_surface(ret->mixed_layer->surface_p);
	cairo_pattern_set_filter(ret->mixed_pattern, CAIRO_FILTER_FAST);

	// 選択領域のレイヤーを作成
	ret->selection = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// 一番上でエフェクト表示を行うレイヤーを作成
	ret->effect = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// テクスチャ用のレイヤーを作成
	ret->texture = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// 下のレイヤーでマスキング、バケツツールでのマスキング用
	ret->mask = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	ret->mask_temp = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	// ブラシ描画時の不透明度設定用サーフェース、イメージ
	ret->alpha_surface = cairo_image_surface_create_for_data(ret->mask->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->alpha_cairo = cairo_create(ret->alpha_surface);
	ret->alpha_temp = cairo_image_surface_create_for_data(ret->temp_layer->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->alpha_temp_cairo = cairo_create(ret->alpha_temp);
	ret->gray_mask_temp = cairo_image_surface_create_for_data(ret->mask_temp->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->gray_mask_cairo = cairo_create(ret->gray_mask_temp);

	// 表示用に拡大・縮小した後の一次記憶メモリ
	ret->disp_temp = CreateDispTempLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER, ret);

	// 回転表示用の値を計算
	ret->cos_value = 1;
	ret->trans_x = - ret->half_size + ((width / 2) * ret->cos_value + (height / 2) * ret->sin_value);
	ret->trans_y = - ret->half_size - ((width / 2) * ret->sin_value - (height / 2) * ret->cos_value);
	cairo_matrix_init_rotate(&matrix, 0);
	cairo_matrix_translate(&matrix, ret->trans_x, ret->trans_y);
	ret->rotate = cairo_pattern_create_for_surface(ret->disp_layer->surface_p);
	cairo_pattern_set_filter(ret->rotate, CAIRO_FILTER_FAST);
	cairo_pattern_set_matrix(ret->rotate, &matrix);

	// カーソル座標補正用の値を計算
	ret->add_cursor_x = - (ret->half_size - ret->disp_layer->width / 2) + ret->half_size;
	ret->add_cursor_y = - (ret->half_size - ret->disp_layer->height / 2) + ret->half_size;
	ret->rev_add_cursor_x = ret->disp_layer->width/2 + (ret->half_size - ret->disp_layer->width/2);
	ret->rev_add_cursor_y = ret->disp_layer->height/2 + (ret->half_size - ret->disp_layer->height/2);

	// レイヤー合成用の関数ポインタをセット
	ret->layer_blend_functions = app->layer_blend_functions;
	ret->part_layer_blend_functions = app->part_layer_blend_functions;

	// アクティブレイヤーより下を合成した画像の保存用
	ret->under_active = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	(void)memcpy(ret->under_active->pixels, ret->back_ground, ret->pixel_buf_size);

	return ret;
}

/*********************************
* Change2FocalMode関数           *
* 局所キャンバスモードに移行する *
* 引数                           *
* parent_window	: 親キャンバス   *
*********************************/
void Change2FocalMode(DRAW_WINDOW* parent_window)
{
	// アプリケーション全体を管理するデータ
	APPLICATION *app = parent_window->app;
	// 局所キャンバス
	DRAW_WINDOW *focal_window;
	// レイヤー登録用
	LAYER *prev_layer = NULL;
	LAYER *src_layer;
	LAYER *target;
	// 局所キャンバスのサイズ
	int focal_width, focal_height, focal_stride;
	// コピー開始座標
	int start_x, start_y;
	// for文用のカウンタ
	int y;

	// 選択範囲が無ければ終了
	if((parent_window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{
		return;
	}

	// 処理中のコールバック関数が呼ばれないように先に止める
	DisconnectDrawWindowCallbacks(parent_window->window, parent_window);

	// 選択範囲部分にフォーカスする
	start_x = parent_window->selection_area.min_x;
	start_y = parent_window->selection_area.min_y;
	focal_width = parent_window->selection_area.max_x - parent_window->selection_area.min_x;
	focal_height = parent_window->selection_area.max_y - parent_window->selection_area.min_y;
	focal_stride = focal_width * parent_window->channel;

	// キャンバス生成
	focal_window = parent_window->focal_window = CreateTempDrawWindow(focal_width, focal_height,
		parent_window->channel, parent_window->file_name, NULL, 0, app);
	focal_window->focal_window = parent_window;
	focal_window->scroll = parent_window->scroll;

	if(GetHas3DLayer(parent_window->app) != FALSE)
	{
		focal_window->first_project = parent_window->first_project;
	}
	focal_window->flags |= DRAW_WINDOW_IS_FOCAL_WINDOW;

	// 親キャンバスのレイヤー情報をコピーする
	for(src_layer = parent_window->layer; src_layer != NULL; src_layer = src_layer->next)
	{
		prev_layer = CreateLayer(0, 0, focal_window->width, focal_window->height, focal_window->channel,
			(eLAYER_TYPE)src_layer->layer_type, prev_layer, NULL, src_layer->name, focal_window);
		prev_layer->layer_mode = src_layer->layer_mode;
		prev_layer->flags = src_layer->flags;
		prev_layer->alpha = src_layer->alpha;

		for(y=0; y<focal_height; y++)
		{
			(void)memcpy(&prev_layer->pixels[y*prev_layer->stride],
				&src_layer->pixels[(start_y+y)*src_layer->stride+start_x*src_layer->channel], focal_stride);
			if(src_layer->layer_type == TYPE_LAYER_SET)
			{
				LAYER *child = prev_layer->prev;
				target = src_layer->prev;
				while(target->layer_set == src_layer)
				{
					child->layer_set = prev_layer;
					child = child->prev;
					target = target->prev;
				}

				if(parent_window->active_layer_set == src_layer)
				{
					focal_window->active_layer_set = prev_layer;
				}
			}
			if(src_layer == parent_window->active_layer)
			{
				focal_window->active_layer = prev_layer;
			}
		}
		focal_window->num_layer++;
	}
	// 一番下のレイヤーを新しいキャンバスに設定
	while(prev_layer->prev != NULL)
	{
		prev_layer = prev_layer->prev;
	}
	focal_window->layer = prev_layer;

	// レイヤービューをリセット
	ClearLayerView(&app->layer_window);
	LayerViewSetDrawWindow(&app->layer_window, focal_window);

	// キャンバスへのコールバック関数を変更
	focal_window->window = parent_window->window;
	SetDrawWindowCallbacks(focal_window->window, focal_window);

	// ウィジェットのサイズを変更
	focal_window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	DrawWindowChangeZoom(focal_window, focal_window->zoom);

	// ナビゲーションの表示を切り替え
	ChangeNavigationDrawWindow(&app->navigation_window, focal_window);
	FillTextureLayer(focal_window->texture, &app->textures);
}

/*********************************
* ReturnFromFocalMode関数        *
* 局所キャンバスモードから戻る   *
* 引数                           *
* parent_window	: 親キャンバス   *
*********************************/
void ReturnFromFocalMode(DRAW_WINDOW* parent_window)
{
	// アプリケーション全体を管理するデータ
	APPLICATION *app = parent_window->app;
	// 局所キャンバス
	DRAW_WINDOW *focal_window = parent_window->focal_window;
	// レイヤー情報移動用
	LAYER *prev_layer = NULL;
	LAYER *src_layer;
	LAYER *dst_layer;
	LAYER *target;
	LAYER *check_layer;
	// 局所キャンバスのサイズ
	int focal_width, focal_height, focal_stride;
	// コピー開始座標
	int start_x, start_y;
	// for文用のカウンタ
	int y;

	// 局所キャンバスが無ければ終了
	if(focal_window == NULL)
	{
		return;
	}

	// 処理中にコールバック関数が呼ばれないように先に止める
	DisconnectDrawWindowCallbacks(focal_window->window, focal_window);

	// 選択範囲部分にピクセルデータを戻す
	start_x = parent_window->selection_area.min_x;
	start_y = parent_window->selection_area.min_y;
	focal_width = parent_window->selection_area.max_x - parent_window->selection_area.min_x;
	focal_height = parent_window->selection_area.max_y - parent_window->selection_area.min_y;
	focal_stride = focal_width * parent_window->channel;
	src_layer = focal_window->layer;
	dst_layer = parent_window->layer;
	parent_window->num_layer = 0;
	while(src_layer != NULL && dst_layer != NULL)
	{
		dst_layer->layer_mode = src_layer->layer_mode;
		dst_layer->flags = src_layer->flags;
		dst_layer->alpha = src_layer->alpha;
		switch(src_layer->layer_type)
		{
		case TYPE_NORMAL_LAYER:
			for(y=0; y<focal_height; y++)
			{
				(void)memcpy(&dst_layer->pixels[(start_y+y)*dst_layer->stride + start_x*dst_layer->channel],
					&src_layer->pixels[y*src_layer->stride], focal_stride);
			}
			break;
		case TYPE_LAYER_SET:
			target = dst_layer->prev;
			check_layer = src_layer->prev;
			while(check_layer != NULL && check_layer->layer_set == src_layer)
			{
				target->layer_set = dst_layer;
				target = target->prev;
				check_layer = check_layer->prev;
			}
			if(focal_window->active_layer_set == src_layer)
			{
				parent_window->active_layer_set = dst_layer;
			}
			break;
		}
		dst_layer = dst_layer->next;
		src_layer = src_layer->next;
		parent_window->num_layer++;

		if(src_layer == focal_window->active_layer)
		{
			parent_window->active_layer = dst_layer;
		}
	}

	// レイヤービューをリセット
	ClearLayerView(&app->layer_window);
	LayerViewSetDrawWindow(&app->layer_window, parent_window);

	// コールバック関数を設定し直す
	SetDrawWindowCallbacks(parent_window->window, parent_window);

	// ウィジェットのサイズを変更
	parent_window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	DrawWindowChangeZoom(parent_window, parent_window->zoom);

	DeleteDrawWindow(&parent_window->focal_window);

	// ナビゲーションの表示を切り替え
	ChangeNavigationDrawWindow(&app->navigation_window, parent_window);
	FillTextureLayer(parent_window->texture, &app->textures);
}

/***************************************
* DeleteDrawWindow関数                 *
* 描画領域の情報を削除                 *
* 引数                                 *
* window	: 描画領域の情報のアドレス *
***************************************/
void DeleteDrawWindow(DRAW_WINDOW** window)
{
	// 削除するレイヤーと次に削除するレイヤー
	LAYER* delete_layer, *next_delete;
	// for文用のカウンタ
	int i;

	// 時間で呼ばれるコールバック関数を停止
	if((*window)->timer_id != 0)
	{
		(void)g_source_remove((*window)->timer_id);
	}
	if((*window)->auto_save_id != 0)
	{
		(void)g_source_remove((*window)->auto_save_id);
	}
	if((*window)->timer != NULL)
	{
		g_timer_destroy((*window)->timer);
	}

	// ブラシ描画用のサーフェース、イメージを削除
	cairo_destroy((*window)->alpha_cairo);
	cairo_surface_destroy((*window)->alpha_surface);
	cairo_destroy((*window)->alpha_temp_cairo);
	cairo_surface_destroy((*window)->alpha_temp);
	cairo_destroy((*window)->gray_mask_cairo);
	cairo_surface_destroy((*window)->gray_mask_temp);

	// ファイル名、ファイルパスを削除
	MEM_FREE_FUNC((*window)->file_name);
	MEM_FREE_FUNC((*window)->file_path);

	// 背景のピクセルデータを開放
	MEM_FREE_FUNC((*window)->back_ground);

	// レイヤーを全て削除
	delete_layer = (*window)->layer;
	while(delete_layer != NULL)
	{
		next_delete = delete_layer->next;
		DeleteLayer(&delete_layer);
		delete_layer = next_delete;
		(*window)->layer = next_delete;
	}
	DeleteLayer(&(*window)->mixed_layer);
	DeleteLayer(&(*window)->temp_layer);
	DeleteLayer(&(*window)->selection);
	DeleteLayer(&(*window)->under_active);
	DeleteLayer(&(*window)->mask);
	DeleteLayer(&(*window)->mask_temp);
	DeleteLayer(&(*window)->texture);
	DeleteLayer(&(*window)->work_layer);

#ifdef OLD_SELECTION_AREA
	// 選択範囲の情報を開放
	for(i=0; i<(*window)->selection_area.num_area; i++)
	{
		MEM_FREE_FUNC((*window)->selection_area.area_data[i].points);
	}
	MEM_FREE_FUNC((*window)->selection_area.area_data);
#else
	if((*window)->selection_area.pixels != NULL)
	{
		MEM_FREE_FUNC((*window)->selection_area.pixels);
		cairo_surface_destroy((*window)->selection_area.surface_p);
	}
#endif

	// 履歴データの情報を開放
	for(i=0; i<HISTORY_BUFFER_SIZE; i++)
	{
		MEM_FREE_FUNC((*window)->history.history[i].data);
	}

	MEM_FREE_FUNC(*window);
	*window = NULL;
}

/*****************************************************
* SwapDrawWindowFromMemoryStream関数                 *
* メモリー上の描画領域データと入れ替える             *
* 引数                                               *
* window	: 描画領域の情報                         *
* stream	: メモリー上の描画領域のデータストリーム *
*****************************************************/
void SwapDrawWindowFromMemoryStream(DRAW_WINDOW* window, MEMORY_STREAM_PTR stream)
{
	// 削除するレイヤーと次に削除するレイヤー
	LAYER* delete_layer, *next_delete;
	// 前のアクティブレイヤーの名前を記憶
	char active_name[MAX_LAYER_NAME_LENGTH];
#ifdef OLD_SELECTION_AREA
	// for文用のカウンタ
	int i;
#endif

	// アクティブなレイヤーの名前を記憶
	(void)strcpy(active_name, window->active_layer->name);

	// 背景のピクセルデータを開放
	MEM_FREE_FUNC(window->back_ground);

	// レイヤーを全て削除
	delete_layer = window->layer;
	while(delete_layer != NULL)
	{
		next_delete = delete_layer->next;
		DeleteLayer(&delete_layer);
		delete_layer = next_delete;
	}
	DeleteLayer(&window->texture);
	DeleteLayer(&window->mixed_layer);
	DeleteLayer(&window->temp_layer);
	DeleteLayer(&window->selection);
	DeleteLayer(&window->under_active);
	DeleteLayer(&window->mask);
	DeleteLayer(&window->mask_temp);
	DeleteLayer(&window->work_layer);

	// ブラシ周りのサーフェース、イメージを一度削除
	cairo_destroy(window->alpha_cairo);
	cairo_surface_destroy(window->alpha_surface);
	cairo_destroy(window->alpha_temp_cairo);
	cairo_surface_destroy(window->alpha_temp);
	cairo_destroy(window->gray_mask_cairo);
	cairo_surface_destroy(window->gray_mask_temp);

	// レイヤービューのウィジェットを全て削除
		// レイヤービューのリスト
	{
		GList* view_list = gtk_container_get_children(
			GTK_CONTAINER(window->app->layer_window.view));
		while(view_list != NULL)
		{
			gtk_widget_destroy(GTK_WIDGET(view_list->data));
			view_list = view_list->next;
		}

		g_list_free(view_list);
	}

	// 選択範囲の情報を開放
#ifdef OLD_SELECTION_AREA
	for(i=0; i<window->selection_area.num_area; i++)
	{
		MEM_FREE_FUNC(window->selection_area.area_data[i].points);
	}
	MEM_FREE_FUNC(window->selection_area.area_data);
#else
	if(window->selection_area.pixels != NULL)
	{
		MEM_FREE_FUNC(window->selection_area.pixels);
		cairo_surface_destroy(window->selection_area.surface_p);
	}
#endif

	// 前の状態からデータを復元
	ReadOriginalFormatMemoryStream(window, stream);

	// 表示用のバッファを更新
	DrawWindowChangeZoom(window, window->zoom);

	// 合成結果に対して拡大・縮小を設定するためのパターン作成し直す
	window->mixed_pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);
	cairo_pattern_set_filter(window->mixed_pattern, CAIRO_FILTER_FAST);

	// ナビゲーションの表示設定
	ChangeNavigationDrawWindow(&window->app->navigation_window, window);
	// テクスチャ用のレイヤーを更新
	FillTextureLayer(window->texture, &window->app->textures);

	// アクティブレイヤーを再設定
	window->active_layer = SearchLayer(window->layer, active_name);
	// アクティブレイヤーの表示を更新
	ChangeActiveLayer(window, window->active_layer);
}

/***********************************************************
* GetWindowID関数                                          *
* 描画領域のIDを取得する                                   *
* 引数                                                     *
* window	: 描画領域の情報                               *
* app		: 	アプリケーションを管理する構造体のアドレス *
* 返り値                                                   *
*	描画領域のID (不正な描画領域ならば-1)                  *
***********************************************************/
int GetWindowID(DRAW_WINDOW* window, APPLICATION* app)
{
	// for文用のカウンタ
	int i;

	for(i=0; i<app->window_num; i++)
	{
		if(window == app->draw_window[i])
		{
			return i;
		}
	}

	return -1;
}

/***************************************
* ResizeDispTemp関数                   *
* 表示用の一時保存のバッファを変更     *
* 引数                                 *
* window		: 描画領域の情報       *
* new_width		: 描画領域の新しい幅   *
* new_height	: 描画領域の新しい高さ *
***************************************/
void ResizeDispTemp(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
)
{
	// CAIROに設定するフォーマット情報
	cairo_format_t format = (window->channel == 4) ?
		CAIRO_FORMAT_ARGB32 : (window->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;
	// 回転処理用の行列データ
	cairo_matrix_t matrix;

	window->disp_size = (int)(2 * sqrt((new_width/2)*(new_width/2)+(new_height/2)*(new_height/2)) + 1);
	window->disp_stride = window->disp_size * 4;
	window->half_size = window->disp_size * 0.5;
	window->trans_x = - window->half_size + ((new_width / 2) * window->cos_value + (new_height / 2) * window->sin_value);
	window->trans_y = - window->half_size - ((new_width / 2) * window->sin_value - (new_height / 2) * window->cos_value);

	window->add_cursor_x = - (window->half_size - window->disp_layer->width / 2) + window->half_size;
	window->add_cursor_y = - (window->half_size - window->disp_layer->height / 2) + window->half_size;
	window->rev_add_cursor_x = window->disp_layer->width/2 + (window->half_size - window->disp_layer->width/2);
	window->rev_add_cursor_y = window->disp_layer->height/2 + (window->half_size - window->disp_layer->height/2);

	cairo_surface_destroy(window->disp_temp->surface_p);
	cairo_destroy(window->disp_temp->cairo_p);

	window->disp_temp->width = new_width;
	window->disp_temp->height = new_height;
	window->disp_temp->stride = new_width * window->disp_temp->channel;

	window->disp_temp->pixels = (uint8*)MEM_REALLOC_FUNC(window->disp_temp->pixels,
		window->disp_stride * window->disp_size);
	window->disp_temp->surface_p = cairo_image_surface_create_for_data(
		window->disp_temp->pixels, format, new_width, new_height, window->disp_temp->stride);
	window->disp_temp->cairo_p = cairo_create(window->disp_temp->surface_p);

	cairo_matrix_init_rotate(&matrix, window->angle);
	cairo_matrix_translate(&matrix, window->trans_x, window->trans_y);

	if(window->rotate != NULL)
	{
		cairo_pattern_destroy(window->rotate);
	}
	window->rotate = cairo_pattern_create_for_surface(window->disp_layer->surface_p);
	cairo_pattern_set_filter(window->rotate, CAIRO_FILTER_FAST);
	cairo_pattern_set_matrix(window->rotate, &matrix);

	UpdateDrawWindowClippingArea(window);
}

/*********************************
* DrawWindowChangeZoom関数       *
* 描画領域の拡大縮小率を変更する *
* 引数                           *
* window	: 描画領域の情報     *
* zoom		: 新しい拡大縮小率   *
*********************************/
void DrawWindowChangeZoom(
	DRAW_WINDOW* window,
	int16 zoom
)
{
	// 拡大率設定用の行列データ
	cairo_matrix_t matrix;

	// 新しい拡大率で行列データを初期化
	cairo_matrix_init_scale(&matrix, 1/(zoom*0.01), 1/(zoom*0.01));
	// 合成画像データのパターンに設定
	cairo_pattern_set_matrix(window->mixed_pattern, &matrix);

	// 新しい拡大縮小率をセット
	window->zoom = zoom;
	window->zoom_rate = zoom * 0.01;
	window->rev_zoom = 1 / window->zoom_rate;

	// 表示用、エフェクト用、ブラシ表示用のレイヤーのバッファを再確保
	ResizeLayerBuffer(window->disp_layer, (int32)(window->width*zoom*0.01), (int32)(window->height*zoom*0.01));
	ResizeLayerBuffer(window->scaled_mixed, (int32)(window->width*zoom*0.01), (int32)(window->height*zoom*0.01));
	ResizeLayerBuffer(window->effect, (int32)(window->width*zoom*0.01), (int32)(window->height*zoom*0.01));
	ResizeDispTemp(window, (int32)(window->width*zoom*0.01), (int32)(window->height*zoom*0.01));

	// ウィジェットのサイズを修正
	gtk_widget_set_size_request(window->window, window->disp_size, window->disp_size);
	gtk_widget_show(window->window);

	// ナビゲーションの表示を更新
	if(window->app->navigation_window.draw_area != NULL)
	{
		gtk_widget_queue_draw(window->app->navigation_window.draw_area);
	}

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

#ifndef OLD_SELECTION_AREA
	if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
	{
		(void)UpdateSelectionArea(&window->selection_area, window->selection, window->disp_temp);
	}
#endif
}

/*****************************************
* FlipDrawWindowHorizontally関数         *
* 描画領域を水平反転する                 *
* 引数                                   *
* window	: 水平反転する描画領域の情報 *
*****************************************/
void FlipDrawWindowHorizontally(DRAW_WINDOW* window)
{
	// 水平反転するレイヤー
	LAYER* target = window->layer;
	int i;

	while(target != NULL)
	{
		// レイヤーのピクセルデータを水平反転
		FlipLayerHorizontally(target, window->temp_layer);

		// ベクトルレイヤーならば
		if(target->layer_type == TYPE_VECTOR_LAYER)
		{	// ベクトルデータの座標を水平反転
			VECTOR_LINE* line = target->layer_data.vector_layer_p->base->next;

			while(line != NULL)
			{
				for(i=0; i<line->num_points; i++)
				{
					line->points[i].x = window->width - line->points[i].x;
				}

				line = line->next;
			}

			// ベクトルをラスタライズ
			target->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ALL;
			RasterizeVectorLayer(window, target, target->layer_data.vector_layer_p);
		}

		target = target->next;
	}

	// 描画内容を更新する
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

/*****************************************
* FlipDrawWindowVertically関数           *
* 描画領域を垂直反転する                 *
* 引数                                   *
* window	: 垂直反転する描画領域の情報 *
*****************************************/
void FlipDrawWindowVertically(DRAW_WINDOW* window)
{
	// 垂直反転するレイヤー
	LAYER* target = window->layer;
	int i;

	// 全てのレイヤーを垂直反転する
	while(target != NULL)
	{	// レイヤーのピクセルデータを垂直反転
		FlipLayerVertically(target, window->temp_layer);

		// ベクトルレイヤーならば
		if(target->layer_type == TYPE_VECTOR_LAYER)
		{	// ベクトルの座標データを垂直反転する
			VECTOR_LINE* line = target->layer_data.vector_layer_p->base->next;

			while(line != NULL)
			{
				for(i=0; i<line->num_points; i++)
				{
					line->points[i].y = window->height - line->points[i].y;
				}

				line = line->next;
			}

			// ベクトルをラスタライズ
			target->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ALL;
			RasterizeVectorLayer(window, target, target->layer_data.vector_layer_p);
		}

		target = target->next;
	}

	// 描画内容を更新する
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

/***************************************
* LayerAlpha2SelectionArea関数         *
* レイヤーの不透明部分を選択範囲にする *
* 引数                                 *
* window	: 描画領域の情報           *
***************************************/
void LayerAlpha2SelectionArea(DRAW_WINDOW* window)
{
	int i, j;	// for文用のカウンタ

	// レイヤーのピクセルデータ中のα値を選択範囲にする
	for(i=0; i<window->active_layer->height; i++)
	{
		for(j=0; j<window->active_layer->width; j++)
		{
			window->selection->pixels[(window->active_layer->y+i)*window->width+j] =
				window->active_layer->pixels[i*window->active_layer->stride+j*4+3];
		}
	}

	// 選択範囲を更新する
	if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == 0)
	{	// 選択範囲無
		window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
	}
	else
	{	// 選択範囲有
		window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
	}
}

/*****************************************
* LayerAlphaAddSelectionArea関数         *
* レイヤーの不透明部分を選択範囲に加える *
* 引数                                   *
* window	: 描画領域の情報             *
*****************************************/
void LayerAlphaAddSelectionArea(DRAW_WINDOW* window)
{
	int i, j;	// for文用のカウンタ

	// レイヤーのピクセルデータ中のα値を選択範囲にする
	for(i=0; i<window->active_layer->height; i++)
	{
		for(j=0; j<window->active_layer->width; j++)
		{
			if(window->selection->pixels[(window->active_layer->y+i)*window->width+j]
				< window->active_layer->pixels[i*window->active_layer->stride+j*4+3])
			{
				window->selection->pixels[(window->active_layer->y+i)*window->width+j] =
					window->active_layer->pixels[i*window->active_layer->stride+j*4+3];
			}
		}
	}

	// 選択範囲を更新する
	if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == 0)
	{	// 選択範囲無
		window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
	}
	else
	{	// 選択範囲有
		window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
	}
}

/*****************************
* MergeAllLayer関数          *
* 全てのレイヤーを結合する   *
* 引数                       *
* window	: 描画領域の情報 *
*****************************/
void MergeAllLayer(DRAW_WINDOW* window)
{
	// 全てのレイヤーを結合したレイヤー
	LAYER* merge = MixLayerForSave(window);
	// 削除するレイヤー
	LAYER* delete_layer = window->layer;
	// 次に削除するレイヤー
	LAYER* next_delete;
	// 一番下のレイヤーに適用する名前
	char* layer_name = MEM_STRDUP_FUNC(window->layer->name);

	ChangeActiveLayer(window, window->layer);

	// 元のレイヤー情報は全て削除
	while(delete_layer != NULL)
	{
		next_delete = delete_layer->next;
		DeleteLayer(&delete_layer);
		window->layer = next_delete;
		delete_layer = next_delete;
	}

	// 一番下のレイヤーを新たに作成
	window->layer = CreateLayer(0, 0, window->width, window->height,
		window->channel, TYPE_NORMAL_LAYER, NULL, NULL, layer_name, window);
	window->num_layer = 1;

	// ピクセルデータをコピー
	(void)memcpy(window->layer->pixels, merge->pixels, window->pixel_buf_size);

	// アクティブレイヤーを変更して表示
	window->active_layer = window->layer;
	LayerViewAddLayer(window->layer, window->layer, window->app->layer_window.view, 1);
	LayerViewSetActiveLayer(window->layer, window->app->layer_window.view);

	MEM_FREE_FUNC(layer_name);
	DeleteLayer(&merge);
}

/**********************************************
* CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY構造体 *
* 解像度変更の履歴データ                      *
**********************************************/
typedef struct _CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY
{
	int32 new_width, new_height;	// 変更後の解像度
	int32 x, y;						// レイヤーの座標
	size_t before_data_size;		// 変更前のデータサイズ
	uint8 *before_data;				// 変更前のデータ
} CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY;

/*************************************
* ChangeDrawWindowResolutionUndo関数 *
* 解像度変更を元に戻す               *
* 引数                               *
* window	: 描画領域の情報         *
* p			: 履歴データ             *
*************************************/
static void ChangeDrawWindowResolutionUndo(DRAW_WINDOW* window, void* p)
{
	// 変更前の状態データのバイト数
	size_t before_data_size;
	// 履歴データをバイト単位にキャスト
	uint8 *byte_data = (uint8*)p;
	// 前の状態を読み込むためのストリーム
	MEMORY_STREAM stream;

	// 変更前の状態データのバイト数を読み込む
	(void)memcpy(
		&before_data_size,
		&byte_data[offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data_size)],
		sizeof(before_data_size)
	);

	// ストリームの設定
	stream.data_point = 0;
	stream.block_size = 1;
	stream.buff_ptr = &byte_data[offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data)];
	stream.data_size = before_data_size;

	// 変更前の状態の描画領域と入れ替える
	SwapDrawWindowFromMemoryStream(window, &stream);
}

/*************************************
* ChangeDrawWindowResolutionRedo関数 *
* 解像度変更をやり直す               *
* 引数                               *
* window	: 描画領域の情報         *
* p			: 履歴データ             *
*************************************/
static void ChangeDrawWindowResolutionRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY history_data;

	// 変更後の幅、高さ読み込むためにデータコピー
	(void)memcpy(&history_data, p,
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data_size));

	// 解像度変更を実行
	ChangeDrawWindowResolution(window,
		history_data.new_width, history_data.new_height);
}

/*************************************************
* AddChangeDrawWindowResolutionHistory関数       *
* 解像度変更の履歴データを追加する               *
* 引数                                           *
* window		: 解像度を変更する描画領域の情報 *
* new_width		: 新しい幅                       *
* new_height	: 新しい高さ                     *
*************************************************/
void AddChangeDrawWindowResolutionHistory(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
)
{
	// 履歴データ
	CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY history_data =
		{new_width, new_height, 0, 0, 0, NULL};
	// データ登録用ストリーム
	MEMORY_STREAM_PTR history_stream;
	// 現在の状態を記憶するためのストリーム
	MEMORY_STREAM_PTR layers_data;
	// ストリームのサイズ
	size_t stream_size;

	// レイヤーの数*ピクセルデータ + 8k分メモリを確保しておく
	stream_size = 8192 +
		window->num_layer * window->width * window->height * window->channel;

	// ストリーム作成
	layers_data = CreateMemoryStream(stream_size);
	// 現在の状態をメモリストリームに書き出す
	WriteOriginalFormat((void*)layers_data,
		(stream_func_t)MemWrite, window, 0, window->app->preference.compress);

	// 現在の状態のデータサイズを記憶する
	history_data.before_data_size = layers_data->data_point;

	// 履歴データを作成する
	stream_size = layers_data->data_point +
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data);
	history_stream = CreateMemoryStream(stream_size);
	(void)MemWrite(&history_data, 1,
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data), history_stream);
	(void)MemWrite(layers_data->buff_ptr, 1, history_data.before_data_size, history_stream);

	AddHistory(&window->history, window->app->labels->menu.change_resolution,
		history_stream->buff_ptr, (uint32)stream_size, ChangeDrawWindowResolutionUndo, ChangeDrawWindowResolutionRedo);

	(void)DeleteMemoryStream(layers_data);
	(void)DeleteMemoryStream(history_stream);
}

/*******************************************
* ChangeDrawWindowResolution関数           *
* 解像度を変更する                         *
* 引数                                     *
* window		: 解像度を変更する描画領域 *
* new_width		: 新しい幅                 *
* new_height	: 新しい高さ               *
*******************************************/
void ChangeDrawWindowResolution(DRAW_WINDOW* window, int32 new_width, int32 new_height)
{
	// 拡大縮小を行うレイヤー
	LAYER *layer = window->layer;
	// レイヤー座標変更用の拡大縮小率
	gdouble zoom_x = (gdouble)new_width / (gdouble)window->width,
		zoom_y = (gdouble)new_height / (gdouble)window->height;
	// 現在のアクティブレイヤーの名前
	char active_name[MAX_LAYER_NAME_LENGTH];

	// 現在のアクティブレイヤーの名前を記憶
	(void)strcpy(active_name, window->active_layer->name);

	// 背景のピクセルデータを更新
	window->back_ground = (uint8*)MEM_REALLOC_FUNC(
		window->back_ground, new_width*new_height*window->channel);
	(void)memset(window->back_ground, 0xff, new_width*new_height*window->channel);

	// 作業レイヤー、一時保存レイヤーのサイズ変更
	DeleteLayer(&window->temp_layer);
	window->temp_layer = CreateLayer(0, 0, new_width, new_height, 5,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
	ResizeLayerBuffer(window->mask, new_width, new_height);
	ResizeLayerBuffer(window->mask_temp, new_width, new_height);
	ResizeLayerBuffer(window->work_layer, new_width, new_height);
	ResizeLayerBuffer(window->mixed_layer, new_width, new_height);
	ResizeLayerBuffer(window->under_active, new_width, new_height);
	ResizeLayerBuffer(window->selection, new_width, new_height);
	ResizeLayerBuffer(window->texture, new_width, new_height);

	// 新しい幅と高さを描画領域の情報にセット
	window->width = new_width, window->height = new_height;
	// ピクセルデータのバイト数、1行分のバイト数を計算
	window->stride = new_width * window->channel;
	window->pixel_buf_size = window->stride * new_height;

	// 合成結果に対して拡大・縮小を設定するためのパターン作成し直す
	window->mixed_pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);
	cairo_pattern_set_filter(window->mixed_pattern, CAIRO_FILTER_FAST);

	// ナビゲーションの表示設定
	ChangeNavigationDrawWindow(&window->app->navigation_window, window);
	// テクスチャ用のレイヤーを更新
	FillTextureLayer(window->texture, &window->app->textures);

	// 全てのレイヤーをリサイズ
	while(layer != NULL)
	{
		ResizeLayer(layer, new_width, new_height);

		layer = layer->next;
	}

	// 表示用のレイヤーをリサイズ
	DrawWindowChangeZoom(window, window->zoom);

	// アクティブレイヤーを元に戻す
	ChangeActiveLayer(window, SearchLayer(window->layer, active_name));

	// レイヤーを合成し直す
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

/***********************************
* ChangeDrawWindowSizeUndo関数     *
* キャンバスサイズの変更を元に戻す *
* 引数                             *
* window	: 描画領域の情報       *
* p			: 履歴データ           *
***********************************/
// 動作は解像度の変更と同一なのでマクロで対処
#define ChangeDrawWindowSizeUndo ChangeDrawWindowResolutionUndo

/***********************************
* ChangeDrawWindowSizeRedo関数     *
* キャンバスサイスの変更をやり直す *
* 引数                             *
* window	: 描画領域の情報       *
* p			: 履歴データ           *
***********************************/
static void ChangeDrawWindowSizeRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY history_data;

	// 変更後の幅、高さ読み込むためにデータコピー
	(void)memcpy(&history_data, p,
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data_size));

	// キャンバスサイズの変更を実行
	ChangeDrawWindowResolution(window,
		history_data.new_width, history_data.new_height);
}

/*************************************************
* AddChangeDrawWindowSizeHistory関数             *
* キャンバスサイズ変更の履歴データを追加する     *
* 引数                                           *
* window		: 解像度を変更する描画領域の情報 *
* new_width		: 新しい幅                       *
* new_height	: 新しい高さ                     *
*************************************************/
void AddChangeDrawWindowSizeHistory(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
)
{
	// 履歴データ
	CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY history_data =
		{new_width, new_height, 0, 0, 0, NULL};
	// データ登録用ストリーム
	MEMORY_STREAM_PTR history_stream;
	// 現在の状態を記憶するためのストリーム
	MEMORY_STREAM_PTR layers_data;
	// ストリームのサイズ
	size_t stream_size;

	// レイヤーの数*ピクセルデータ + 8k分メモリを確保しておく
	stream_size = 8192 +
		window->num_layer * window->width * window->height * window->channel;

	// ストリーム作成
	layers_data = CreateMemoryStream(stream_size);
	// 現在の状態をメモリストリームに書き出す
	WriteOriginalFormat((void*)layers_data,
		(stream_func_t)MemWrite, window, 0, window->app->preference.compress);

	// 現在の状態のデータサイズを記憶する
	history_data.before_data_size = layers_data->data_point;

	// 履歴データを作成する
	stream_size = layers_data->data_point +
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data);
	history_stream = CreateMemoryStream(stream_size);
	(void)MemWrite(&history_data, 1,
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data), history_stream);
	(void)MemWrite(layers_data->buff_ptr, 1, history_data.before_data_size, history_stream);

	AddHistory(&window->history, window->app->labels->menu.change_canvas_size,
		history_stream->buff_ptr, (uint32)stream_size, ChangeDrawWindowSizeUndo, ChangeDrawWindowSizeRedo);

	(void)DeleteMemoryStream(layers_data);
	(void)DeleteMemoryStream(history_stream);
}

/*******************************************
* ChangeDrawWindowSize関数                 *
* 描画領域のサイズを変更する               *
* 引数                                     *
* window		: 解像度を変更する描画領域 *
* new_width		: 新しい幅                 *
* new_height	: 新しい高さ               *
*******************************************/
void ChangeDrawWindowSize(DRAW_WINDOW* window, int32 new_width, int32 new_height)
{
	// サイズ変更を行うレイヤー
	LAYER *layer = window->layer;
	// 現在のアクティブレイヤーの名前
	char active_name[MAX_LAYER_NAME_LENGTH];

	// 現在のアクティブレイヤーの名前を記憶
	(void)strcpy(active_name, window->active_layer->name);

	// 背景のピクセルデータを更新
	window->back_ground = (uint8*)MEM_REALLOC_FUNC(
		window->back_ground, new_width*new_height*window->channel);
	(void)memset(window->back_ground, 0xff, new_width*new_height*window->channel);

	// 作業レイヤー、一時保存レイヤーのサイズ変更
	DeleteLayer(&window->temp_layer);
	window->temp_layer = CreateLayer(0, 0, new_width, new_height, 5,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
	ResizeLayerBuffer(window->mask, new_width, new_height);
	ResizeLayerBuffer(window->mask_temp, new_width, new_height);
	ResizeLayerBuffer(window->work_layer, new_width, new_height);
	ResizeLayerBuffer(window->mixed_layer, new_width, new_height);
	ResizeLayerBuffer(window->under_active, new_width, new_height);
	ResizeLayerBuffer(window->selection, new_width, new_height);

	// 新しい幅と高さを描画領域の情報にセット
	window->width = new_width, window->height = new_height;
	// ピクセルデータのバイト数、1行分のバイト数を計算
	window->stride = new_width * window->channel;
	window->pixel_buf_size = window->stride * new_height;

	// 合成結果に対して拡大・縮小を設定するためのパターン作成し直す
	window->mixed_pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);
	cairo_pattern_set_filter(window->mixed_pattern, CAIRO_FILTER_FAST);

	// ナビゲーションの表示設定
	ChangeNavigationDrawWindow(&window->app->navigation_window, window);
	// テクスチャ用のレイヤーを更新
	FillTextureLayer(window->texture, &window->app->textures);

	// 全てのレイヤーをリサイズ
	while(layer != NULL)
	{
		ChangeLayerSize(layer, new_width, new_height);

		layer = layer->next;
	}

	// 表示用のレイヤーをリサイズ
	DrawWindowChangeZoom(window, window->zoom);

	// アクティブレイヤーを元に戻す
	ChangeActiveLayer(window, SearchLayer(window->layer, active_name));

	// レイヤーを合成し直す
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

/*****************************************
* UpdateDrawWindowClippingArea           *
* 画面更新時にクリッピングする領域を更新 *
* 引数                                   *
* window	: 描画領域の情報             *
*****************************************/
void UpdateDrawWindowClippingArea(DRAW_WINDOW* window)
{
	GtkAllocation allocation;
	int half_width = window->disp_layer->width / 2;
	int half_height = window->disp_layer->height / 2;
	int add_x = half_width - window->disp_size / 2;
	int add_y = half_height - window->disp_size / 2;
	int right, bottom;

	gtk_widget_get_allocation(window->scroll, &allocation);
	right = window->scroll_x + allocation.width;
	bottom = window->scroll_y + allocation.height;

	window->update_clip_area[0][0] =
		(int)((window->scroll_x + add_x - half_width) * window->cos_value
			- (window->scroll_y + add_y - half_height) * window->sin_value) + half_width;
	window->update_clip_area[0][1] =
		(int)((window->scroll_x + add_x - half_width) * window->sin_value
			+ (window->scroll_y + add_y - half_height) * window->cos_value) + half_height;
	window->update_clip_area[1][0] =
		(int)((window->scroll_x + add_x - half_width) * window->cos_value
			- (bottom + add_y - half_height) * window->sin_value) + half_width;
	window->update_clip_area[1][1] =
		(int)((window->scroll_x + add_x - half_width) * window->sin_value
			+ (bottom + add_y - half_height) * window->cos_value) + half_height;
	window->update_clip_area[2][0] =
		(int)((right + add_x - half_width) * window->cos_value
			- (bottom + add_y - half_height) * window->sin_value) + half_width;
	window->update_clip_area[2][1] =
		(int)((right + add_x - half_width) * window->sin_value
			+ (bottom + add_y - half_height) * window->cos_value) + half_height;
	window->update_clip_area[3][0] =
		(int)((right + add_x - half_width) * window->cos_value
			- (window->scroll_y + add_y - half_height) * window->sin_value) + half_width;
	window->update_clip_area[3][1] =
		(int)((right + add_x - half_width) * window->sin_value
			+ (window->scroll_y + add_y - half_height) * window->cos_value) + half_height;

	gtk_widget_queue_draw(window->window);
}

/*************************************************
* ClipUpdateArea関数                             *
* 画面のスクロールに入っている部分でクリッピング *
* 引数                                           *
* window	: 描画領域の情報                     *
* cairo_p	: Cairo情報                          *
*************************************************/
void ClipUpdateArea(DRAW_WINDOW* window, cairo_t* cairo_p)
{
	cairo_move_to(cairo_p, window->update_clip_area[0][0], window->update_clip_area[0][1]);
	cairo_line_to(cairo_p, window->update_clip_area[1][0], window->update_clip_area[1][1]);
	cairo_line_to(cairo_p, window->update_clip_area[2][0], window->update_clip_area[2][1]);
	cairo_line_to(cairo_p, window->update_clip_area[3][0], window->update_clip_area[3][1]);
	cairo_close_path(cairo_p);
	cairo_clip(cairo_p);
}

/*******************************************************************
* DrawWindowSetIccProfile関数                                      *
* キャンバスにICCプロファイルを割り当てる                          *
* 引数                                                             *
* window	: 描画領域の情報(icc_profile_dataにデータ割り当て済み) *
* data_size	: ICCプロファイルのデータのバイト数                    *
* ask_set	: ソフトプルーフ表示を適用するかを尋ねるか否か         *
*******************************************************************/
void DrawWindowSetIccProfile(DRAW_WINDOW* window, int32 data_size, gboolean ask_set)
{
	APPLICATION *app = window->app;
	cmsHPROFILE *monitor_profile;

	if(ask_set != FALSE)
	{
		GtkWidget *dialog = gtk_dialog_new_with_buttons(
			"",
			GTK_WINDOW(window->app->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_YES, GTK_RESPONSE_YES,
			GTK_STOCK_NO, GTK_RESPONSE_NO,
			NULL
		);
		GtkWidget *label;
		char str[2048];
		char label_str[2048] = "";
		char *copy_start;
		char *p;
		char *next;
		int result;

		(void)strcpy(str, app->labels->make_new.adopt_icc_profile);
		copy_start = p = str;
		next = g_utf8_next_char(p);
		while(*next != '\0')
		{
			if((uint8)*p == 0x5c && *(p+1) == 'n')
			{
				*p = '\n';
				*(p+1) = '\0';
				(void)strcat(label_str, copy_start);
				copy_start = g_utf8_next_char(next);
			}
			else if((next - p) >= 2 && (uint8)*p == 0xc2
				&& (uint8)(*(p+1)) == 0xa5 && (uint8)*next == 'n')
			{
				*p = '\n';
				*(p+1) = '\0';
				(void)strcat(label_str, copy_start);
				copy_start = g_utf8_next_char(next);
			}

			p = next;
			next = g_utf8_next_char(next);
		}
		(void)strcat(label_str, copy_start);

		label = gtk_label_new(label_str);

		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label, FALSE, FALSE, 2);

		gtk_widget_show_all(dialog);

		result = gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_YES;

		gtk_widget_destroy(dialog);

		if(result != GTK_RESPONSE_YES)
		{
			return;
		}
	}

	window->input_icc = cmsOpenProfileFromMem(window->icc_profile_data, data_size);
	window->icc_profile_size = data_size;

	if(window->input_icc != NULL)
	{
		monitor_profile = GetPrimaryMonitorProfile();

		if(app->output_icc != NULL)
		{
			cmsBool bpc[] = {TRUE, TRUE, TRUE, TRUE};
			cmsHPROFILE h_profiles[] = {window->input_icc, app->output_icc, app->output_icc, monitor_profile};
			cmsUInt32Number intents[] = { INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC };
			cmsFloat64Number adaptation_states[] = {0, 0, 0, 0};

			window->icc_transform = cmsCreateExtendedTransform(cmsGetProfileContextID(h_profiles[1]), 4, h_profiles,
				bpc, intents, adaptation_states, NULL, 0, TYPE_BGRA_8, TYPE_BGRA_8, 0);
		}
		else
		{
			window->icc_transform = cmsCreateTransform(window->input_icc, TYPE_BGRA_8,
				monitor_profile, TYPE_BGRA_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_BLACKPOINTCOMPENSATION);
		}

		window->display_filter_mode = DISPLAY_FUNC_TYPE_ICC_PROFILE;
		app->display_filter.filter_func = app->tool_window.color_chooser->filter_func =
			g_display_filter_funcs[DISPLAY_FUNC_TYPE_ICC_PROFILE];
		app->display_filter.filter_data = app->tool_window.color_chooser->filter_data = (void*)app;

		gtk_widget_queue_draw(app->tool_window.color_chooser->widget);
		UpdateColorBox(app->tool_window.color_chooser);
		gtk_widget_queue_draw(app->tool_window.color_chooser->pallete_widget);

		gtk_check_menu_item_set_active(
			GTK_CHECK_MENU_ITEM(app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_ICC_PROFILE]),
			TRUE
		);
		app->flags |= APPLICATION_DISPLAY_SOFT_PROOF;

		cmsCloseProfile(monitor_profile);
	}
}

gboolean DrawWindowConfigurEvent(GtkWidget* widget, GdkEventConfigure* event_info, DRAW_WINDOW* window)
{
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	if(GetHas3DLayer(window->app) != FALSE)
	{
		LAYER *layer = window->layer;

		if(window->focal_window != NULL)
		{
			return FALSE;
		}

		if((window->flags & DRAW_WINDOW_INITIALIZED) == 0)
		{
			if(ConfigureEvent(widget, event_info, window->first_project) == FALSE)
			{
				return FALSE;
			}
			window->flags |= DRAW_WINDOW_INITIALIZED;
		}

		while(layer != NULL)
		{
			if(layer->modeling_data != NULL)
			{
				MEMORY_STREAM stream = {(uint8*)layer->modeling_data, 0, layer->modeling_data_size, 1024};
				LoadProjectContextData(layer->layer_data.project, (void*)&stream, (size_t (*)(void*, size_t, size_t, void*))MemRead,
					(int (*)(void*, long, int))MemSeek);
				MEM_FREE_FUNC(layer->modeling_data);
				layer->modeling_data = NULL;
			}
			layer = layer->next;
		}

		if(window->active_layer == NULL)
		{
			return FALSE;
		}
		else if(window->active_layer->layer_type != TYPE_3D_LAYER
			&& (window->flags & DRAW_WINDOW_DISCONNECT_3D) != 0)
		{
			gtk_widget_hide(window->gl_area);
			DisconnectDrawWindowCallbacks(window->gl_area, window);
			gtk_widget_show(window->window);
			SetDrawWindowCallbacks(window->window, window);
			g_signal_handler_disconnect(window->window, window->callbacks.configure);
			window->callbacks.configure = 0;
			gtk_widget_queue_draw(window->window);

			window->flags &= ~(window->flags & DRAW_WINDOW_DISCONNECT_3D);
		}
	}
	else
#endif
	{
		g_signal_handler_disconnect(window->window, window->callbacks.configure);
		window->callbacks.configure = 0;
		window->flags |= DRAW_WINDOW_INITIALIZED;
	}

	return FALSE;
}

void ScrollSizeChangeEvent(GtkWidget* scroll, GdkRectangle* size, DRAW_WINDOW* window)
{
	LAYER *layer = window->active_layer;

	if(layer->layer_type == TYPE_3D_LAYER
		&& (window->flags & DRAW_WINDOW_EDITTING_3D_MODEL) != 0)
	{
		int width = size->width;
		int height = size->height;
		if(width > SCROLLED_WINDOW_MARGIN)
		{
			width -= SCROLLED_WINDOW_MARGIN;
		}
		if(height > SCROLLED_WINDOW_MARGIN)
		{
			height -= SCROLLED_WINDOW_MARGIN;
		}
		gtk_widget_set_size_request(layer->window->window, width, height);
		gtk_widget_show_all(layer->window->window);
	}
}

void UpdateDrawWindow(DRAW_WINDOW* window)
{
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	gtk_widget_queue_draw(window->window);
}

#ifdef __cplusplus
}
#endif
