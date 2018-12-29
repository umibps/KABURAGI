// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "application.h"
#include "display.h"

#include "./gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
* ClipBoardImageRecieveCallBack関数                        *
* クリップボードの画像データを受けった時のコールバック関数 *
* 引数                                                     *
* clipboard	: クリップボードの情報                         *
* pixbuf	: ピクセルバッファ                             *
* data		: アプリケーションを管理する構造体の           *
***********************************************************/
static void ClipBoardImageRecieveCallBack(
	GtkClipboard *clipboard,
	GdkPixbuf* pixbuf,
	gpointer data
)
{
	// アプリケーションの情報にキャスト
	APPLICATION* app = (APPLICATION*)data;
	// ピクセルデータ
	uint8* pixels;
	// 画像データの幅、高さ、一行分のバイト数
	int32 width, height, stride;
	// ピクセルデータの破棄要否
	gboolean delete_pixbuf = FALSE;
	// for文用のカウンタ
	int i;

	// ピクセルバッファ作成に失敗しているときは終了
	if(pixbuf == NULL)
	{
		return;
	}

	// αチャンネルがない場合は追加
	if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
	{
		pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
		delete_pixbuf = TRUE;
	}

	// 画像の幅と高さを取得
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);

	// ピクセルと画像一列分のバイト数を取得
	pixels = gdk_pixbuf_get_pixels(pixbuf);
	stride = gdk_pixbuf_get_rowstride(pixbuf);
	
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	{
		uint8 r;
		for(i=0; i<width*height; i++)
		{
			r = pixels[i*4];
			pixels[i*4] = pixels[i*4+2];
			pixels[i*4+2] = r;
		}
	}
#endif

	// 描画領域がないときは新規作成
	if(app->window_num == 0)
	{
		// 描画領域を新たに追加
		app->active_window = app->window_num;
		app->draw_window[app->window_num] = CreateDrawWindow(
			width, height, 4, app->labels->menu.clip_board, app->widgets->note_book, app->window_num, app);

		// 画像データを一番下のレイヤーにコピー
		for(i=0; i<height; i++)
		{
			(void)memcpy(&app->draw_window[app->active_window]->active_layer->pixels[
				app->draw_window[app->active_window]->active_layer->stride*i],
					&pixels[i*stride], app->draw_window[app->active_window]->active_layer->stride
			);
		}

		// 一番下のレイヤーをアクティブ表示に
		LayerViewSetActiveLayer(
			app->draw_window[app->active_window]->active_layer, app->layer_window.view
		);

		// 無効にしていた一部のメニューを有効に
		for(i=0; i<app->menus.num_disable_if_no_open; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
		}
		gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
		gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

		// 描画領域のカウンタを更新
		app->window_num++;

		// ウィンドウのタイトルを画像名に
		UpdateWindowTitle(app);
	}
	else
	{	// 描画領域に新たにレイヤーを作成してコピー
			// 貼り付ける描画領域
		DRAW_WINDOW *window = GetActiveDrawWindow(app);
		// 追加するレイヤー
		LAYER *layer;
		// レイヤーの名前
		char layer_name[256];
		// コピー元の一行分のバイト数
		int32 original_stride = stride;
		// コピー先の座標
		int32 x = 0, y = 0;

		// 選択範囲があるときは座標を設定
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			x = window->selection_area.min_x;
			y = window->selection_area.min_y;
		}

		// レイヤーの名前を作成
		i = 1;
		(void)strcpy(layer_name, app->labels->layer_window.pasted_layer);
		while(CorrectLayerName(window->layer, layer_name) == 0)
		{
			(void)sprintf(layer_name, "%s (%d)", app->labels->layer_window.pasted_layer, i);
			i++;
		}

		// アクティブな描画領域にレイヤーを追加
		layer = CreateLayer(0, 0, window->width,
			window->height, 4, TYPE_NORMAL_LAYER,
			window->active_layer,
			window->active_layer->next,
			layer_name,
			window
		);
		window->num_layer++;
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

		// 画像データを追加したレイヤーにコピー
		if(x + width > layer->width)
		{
			width = layer->width - x;
			stride = width * 4;
		}
		if(y + height > layer->height)
		{
			height = layer->height - y;
		}
		for(i=0; i<height; i++)
		{
			(void)memcpy(&layer->pixels[layer->stride*(i+y)+(x*4)],
					&pixels[i*stride], stride
			);
		}

		// 履歴データを追加
		AddNewLayerWithImageHistory(layer, pixels, width, height,
			original_stride, 4, app->labels->menu.open_as_layer);

		// 追加したレイヤーをアクティブに
		LayerViewAddLayer(layer, window->layer,
			app->layer_window.view, window->num_layer);
		ChangeActiveLayer(app->draw_window[app->active_window], layer);
		LayerViewSetActiveLayer(
			window->active_layer, app->layer_window.view
		);
	}

	if(delete_pixbuf != FALSE)
	{
		// ピクセルバッファを開放
		g_object_unref(pixbuf);
	}
}

/*****************************************************
* ExecutePaste関数                                   *
* 貼り付けを実行する                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecutePaste(APPLICATION* app)
{
	// クリップボードに対してデータを要求
	gtk_clipboard_request_image(gtk_clipboard_get(GDK_NONE),
		ClipBoardImageRecieveCallBack, app);
}

/*****************************************************
* ExecuteCopy関数                                    *
* コピーを実行                                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteCopy(APPLICATION* app)
{
	// コピーするピクセルバッファ
	GdkPixbuf* pixbuf;
	// アクティブな描画領域
	DRAW_WINDOW* window = GetActiveDrawWindow(app);
	// コピー先のピクセルデータ
	uint8* pixels;
	// コピー開始座標
	int x, y;
	// コピーする幅、高さ
	int width, height;
	// コピーする一行分のバイト数
	int stride;
	// for文用のカウンタ
	int i;

	// コピーする座標、幅、高さを決定
	x = window->selection_area.min_x;
	y = window->selection_area.min_y;
	width = window->selection_area.max_x - x;
	height = window->selection_area.max_y - y;
	stride = width * window->channel;

	// 選択範囲のピクセルのみを残す
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(window->temp_layer->cairo_p, window->active_layer->surface_p, 0, 0);
	cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);

	// ピクセルバッファを作成
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, window->channel == 4, 8, width, height);

	// ピクセルデータを取得
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	// データを写す
	for(i=0; i<height; i++)
	{
		(void)memcpy(
			&pixels[i*stride],
			&window->active_layer->pixels[(y+i)*window->active_layer->stride+x*window->active_layer->channel],
			stride
		);
	}
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	{
		uint8 r;
		for(i=0; i<width*height; i++)
		{
			r = pixels[i*4];
			pixels[i*4] = pixels[i*4+2];
			pixels[i*4+2] = r;
		}
	}
#endif

	// クリップボードへデータをコピー
	gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), pixbuf);

	// ピクセルバッファを開放
	g_object_unref(pixbuf);
}

/*****************************************************
* ExecuteCopyVisible関数                             *
* 可視部コピーを実行                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteCopyVisible(APPLICATION* app)
{
	// コピーするピクセルバッファ
	GdkPixbuf* pixbuf;
	// アクティブな描画領域
	DRAW_WINDOW* window = GetActiveDrawWindow(app);
	// 合成したレイヤー
	LAYER *mixed = MixLayerForSave(window);
	// コピー先のピクセルデータ
	uint8* pixels;
	// コピー開始座標
	int x, y;
	// コピーする幅、高さ
	int width, height;
	// コピーする一行分のバイト数
	int stride;
	// for文用のカウンタ
	int i;

	// コピーする座標、幅、高さを決定
	x = window->selection_area.min_x;
	y = window->selection_area.min_y;
	width = window->selection_area.max_x - x;
	height = window->selection_area.max_y - y;
	stride = width * window->channel;

	// 選択範囲のピクセルのみを残す
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(window->temp_layer->cairo_p, mixed->surface_p, 0, 0);
	cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);

	// ピクセルバッファを作成
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, window->channel == 4, 8, width, height);

	// ピクセルデータを取得
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	// データを写す
	for(i=0; i<height; i++)
	{
		(void)memcpy(
			&pixels[i*stride],
			&window->active_layer->pixels[(y+i)*window->active_layer->stride+x*window->active_layer->channel],
			stride
		);
	}
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	{
		uint8 r;
		for(i=0; i<width*height; i++)
		{
			r = pixels[i*4];
			pixels[i*4] = pixels[i*4+2];
			pixels[i*4+2] = r;
		}
	}
#endif

	// クリップボードへデータをコピー
	gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), pixbuf);

	// ピクセルバッファを開放
	g_object_unref(pixbuf);

	// 合成したレイヤーは既に不要
	DeleteLayer(&mixed);
}

/*****************************************************
* ExecuteCut関数                                     *
* 切り取りを実行                                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteCut(APPLICATION* app)
{
	// コピーするピクセルバッファ
	GdkPixbuf* pixbuf;
	// アクティブな描画領域
	DRAW_WINDOW* window = GetActiveDrawWindow(app);
	// コピー先のピクセルデータ
	uint8* pixels;
	// コピー開始座標
	int x, y;
	// コピーする幅、高さ
	int width, height;
	// コピーする一行分のバイト数
	int stride;
	// for文用のカウンタ
	int i;

	// コピーする座標、幅、高さを決定
	x = window->selection_area.min_x;
	y = window->selection_area.min_y;
	width = window->selection_area.max_x - x;
	height = window->selection_area.max_y - y;
	stride = width * window->channel;

	// 選択範囲のピクセルのみを残す
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(window->temp_layer->cairo_p, window->active_layer->surface_p, 0, 0);
	cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);

	// ピクセルバッファを作成
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, window->channel == 4, 8, width, height);

	// ピクセルデータを取得
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	// データを写す
	for(i=0; i<height; i++)
	{
		(void)memcpy(
			&pixels[i*stride],
			&window->active_layer->pixels[(y+i)*window->active_layer->stride+x*window->active_layer->channel],
			stride
		);
	}
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	{
		uint8 r;
		for(i=0; i<width*height; i++)
		{
			r = pixels[i*4];
			pixels[i*4] = pixels[i*4+2];
			pixels[i*4+2] = r;
		}
	}
#endif

	// クリップボードへデータをコピー
	gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), pixbuf);

	// ピクセルバッファを開放
	g_object_unref(pixbuf);

	// 履歴データを追加
	AddDeletePixelsHistory(app->labels->menu.cut, window, window->active_layer,
		window->selection_area.min_x, window->selection_area.min_y,
		window->selection_area.max_x, window->selection_area.min_y
	);

	// 選択範囲のピクセルデータを削除
	DeleteSelectAreaPixels(window, window->active_layer, window->selection);

	// 画面を更新
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

#ifdef __cplusplus
}
#endif
