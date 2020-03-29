#include <string.h>
#include "preview_window.h"
#include "application.h"
#include "memory.h"

#include "./gui/GTK/input_gtk.h"
#include "./gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

static gboolean OnClosePreviewWindow(GtkWidget* window, GdkEvent* event_info, PREVIEW_WINDOW* preview)
{
	APPLICATION *app = (APPLICATION*)g_object_get_data(G_OBJECT(window), "application_data");
	gint x, y;
	gint width, height;

	gtk_window_get_position(GTK_WINDOW(window), &x, &y);
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);
	preview->window_x = x, preview->window_y = y;
	preview->window_width = width, preview->window_height = height;

	cairo_destroy(preview->cairo_p);
	preview->cairo_p = NULL;
	cairo_surface_destroy(preview->surface_p);
	preview->surface_p = NULL;
	MEM_FREE_FUNC(preview->pixels);
	preview->pixels = NULL;
	MEM_FREE_FUNC(preview->reverse_buff);
	preview->reverse_buff = NULL;

	app->flags |= APPLICATION_IN_DELETE_EVENT;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preview->menu_item), FALSE);
	app->flags &= ~(APPLICATION_IN_DELETE_EVENT);

	preview->window = NULL;

	return FALSE;
}

/*********************************************************
* UpdatePreviewWindow関数                                *
* プレビューウィンドウの画面更新                         *
* 引数                                                   *
* widget	: プレビューウィンドウの描画領域ウィジェット *
* display	: 描画イベントの情報                         *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	常にTRUE                                             *
*********************************************************/
#if GTK_MAJOR_VERSION <= 2
static gboolean UpdatePreviewWindow(
	GtkWidget* widget,
	GdkEventExpose *display,
	APPLICATION* app
)
#else
static gboolean UpdatePreviewWindow(
	GtkWidget* widget,
	cairo_t* cairo_p,
	APPLICATION* app
)
#endif
{
	// 描画領域のサイズ取得用
	GtkAllocation preview_allocation;
#if GTK_MAJOR_VERSION <= 2
	// 描画用のCairo情報
	cairo_t *cairo_p;
#endif
	// 描画領域1行分のバイト数
	int stride;
	// 拡大率更新のフラグ
	int update_zoom = 0;

	if(app->window_num == 0)
	{
		return FALSE;
	}

	gtk_widget_get_allocation(app->preview_window.image, &preview_allocation);

	stride = preview_allocation.width * 4;

	if(app->preview_window.width != preview_allocation.width
		|| app->preview_window.height != preview_allocation.height)
	{
		app->preview_window.pixels = (uint8*)MEM_REALLOC_FUNC(
			app->preview_window.pixels, preview_allocation.height * stride);
		app->preview_window.reverse_buff = (uint8*)MEM_REALLOC_FUNC(
			app->preview_window.reverse_buff, stride);
		app->preview_window.width = preview_allocation.width;
		app->preview_window.height = preview_allocation.height;

		if(app->preview_window.cairo_p != NULL)
		{
			cairo_destroy(app->preview_window.cairo_p);
			cairo_surface_destroy(app->preview_window.surface_p);
		}

		// CAIRO情報作り直し
		app->preview_window.surface_p =
			cairo_image_surface_create_for_data(app->preview_window.pixels, CAIRO_FORMAT_ARGB32,
				preview_allocation.width, preview_allocation.height, stride);
		app->preview_window.cairo_p = cairo_create(app->preview_window.surface_p);

		// 拡大率の更新
		update_zoom++;
	}

	// 描画領域の変更やウィンドウサイズの変更があれば拡大率を更新
	if(update_zoom != 0 || app->draw_window[app->active_window] != app->preview_window.draw)
	{
		gdouble zoom_x = (gdouble)preview_allocation.width
			/ app->draw_window[app->active_window]->width;
		gdouble zoom_y = (gdouble)preview_allocation.height
			/ app->draw_window[app->active_window]->height;

		if(zoom_x < zoom_y)
		{
			app->preview_window.zoom = zoom_x;
			app->preview_window.rev_zoom = 1 / zoom_x;
		}
		else
		{
			app->preview_window.zoom = zoom_y;
			app->preview_window.rev_zoom = 1 / zoom_y;
		}

		app->preview_window.draw_width =
			(int32)(app->draw_window[app->active_window]->width * app->preview_window.zoom);
		app->preview_window.draw_height =
			(int32)(app->draw_window[app->active_window]->height * app->preview_window.zoom);
	}

	// 画像を拡大縮小して描画
	cairo_scale(app->preview_window.cairo_p, app->preview_window.zoom, app->preview_window.zoom);
	cairo_set_source_surface(app->preview_window.cairo_p,
		app->draw_window[app->active_window]->mixed_layer->surface_p, 0, 0);
	cairo_paint(app->preview_window.cairo_p);
	// 拡大率を元に戻す
	cairo_scale(app->preview_window.cairo_p, app->preview_window.rev_zoom, app->preview_window.rev_zoom);

	// 左右反転表示中ならピクセルデータを反転
	if((app->draw_window[app->active_window]->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) != 0)
	{
		uint8 *src, *ref;
		int x, y;

		for(y=0; y<app->preview_window.draw_height; y++)
		{
			ref = app->preview_window.reverse_buff;
			src = &app->preview_window.pixels[y*stride+app->preview_window.draw_width*4-4];

			for(x=0; x<app->preview_window.draw_width; x++, ref += 4, src -= 4)
			{
				ref[0] = src[0], ref[1] = src[1], ref[2] = src[2], ref[3] = src[3];
			}
			(void)memcpy(src+4, app->preview_window.reverse_buff, app->preview_window.draw_width * 4);
		}
	}

	// 画面更新
#if GTK_MAJOR_VERSION <= 2
	//cairo_p = kaburagi_cairo_create((struct _GdkWindow*)app->preview_window.image->window);
	cairo_p = gdk_cairo_create(app->preview_window.image->window);
#endif
	cairo_rectangle(cairo_p, 0, 0, app->preview_window.draw_width, app->preview_window.draw_height);
	cairo_clip(cairo_p);
	cairo_set_source_surface(cairo_p, app->preview_window.surface_p, 0, 0);
	cairo_paint(cairo_p);
#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

void InitializePreviewWindow(PREVIEW_WINDOW* preview, APPLICATION* app)
{
	// ウィンドウ作成
	preview->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	// アプリケーションの情報を設定
	g_object_set_data(G_OBJECT(preview->window), "application_data", app);
	// ウィンドウの座標とサイズを設定
	gtk_window_move(GTK_WINDOW(preview->window), preview->window_x, preview->window_y);
	gtk_window_resize(GTK_WINDOW(preview->window),
		preview->window_width, preview->window_height);
	// ウィンドウをタスクバーに表示しない場合
	if((app->flags & APPLICATION_SHOW_PREVIEW_ON_TASK_BAR) == 0)
	{
		// 閉じるボタンのみのウィンドウへ
		gtk_window_set_type_hint(GTK_WINDOW(preview->window), GDK_WINDOW_TYPE_HINT_UTILITY);
		// タスクバーには表示しない
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(preview->window), TRUE);
		// 親ウィンドウを登録
		gtk_window_set_transient_for(
			GTK_WINDOW(preview->window), GTK_WINDOW(app->widgets->window));
	}
	else
	{
		gtk_window_set_type_hint(GTK_WINDOW(preview->window), GDK_WINDOW_TYPE_HINT_NORMAL);
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(preview->window), FALSE);
		gtk_window_set_skip_pager_hint(GTK_WINDOW(preview->window), FALSE);
	}
	// ウィンドウタイトルを設定
	gtk_window_set_title(GTK_WINDOW(preview->window), app->labels->unit.preview);
	// ショートカットキーを登録
	gtk_window_add_accel_group(GTK_WINDOW(preview->window), app->widgets->hot_key);
	// ウィンドウの位置を設定
	if(preview->window_x < - (int)0x80000000 || preview->window_x > 0x80000000)
	{
		preview->window_x = 0;
	}
	if(preview->window_y < - (int)0x80000000 || preview->window_y > 0x80000000)
	{
		preview->window_y = 0;
	}
	gtk_window_move(GTK_WINDOW(preview->window), preview->window_x, preview->window_y);
	// ウィンドウのサイズを設定
	gtk_window_resize(GTK_WINDOW(preview->window), preview->window_width, preview->window_height);

	// 画像表示領域を作成
	preview->image = gtk_drawing_area_new();
	// ウィンドウに追加
	gtk_container_add(GTK_CONTAINER(preview->window), preview->image);
	// 描画用のコールバック関数をセット
#if GTK_MAJOR_VERSION <= 2
	(void)g_signal_connect(G_OBJECT(preview->image), "expose_event",
		G_CALLBACK(UpdatePreviewWindow), app);
#else
	(void)g_signal_connect(G_OBJECT(preview->image), "draw",
		G_CALLBACK(UpdatePreviewWindow), app);
#endif

	// イベントの種類をセット
	gtk_widget_add_events(preview->image, GDK_EXPOSURE_MASK);

	// ウィンドウが閉じられた時のコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(preview->window), "delete_event",
		G_CALLBACK(OnClosePreviewWindow), preview);

	// キーボードのコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(preview->window), "key-press-event",
		G_CALLBACK(KeyPressEvent), app);
	(void)g_signal_connect(G_OBJECT(preview->window), "key-release-event",
		G_CALLBACK(KeyPressEvent), app);

	// ウィンドウ表示
	gtk_widget_show_all(preview->window);
}

#ifdef __cplusplus
}
#endif
