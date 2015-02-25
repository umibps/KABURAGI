#include <string.h>
#include <math.h>
#include "configure.h"
#include "navigation.h"
#include "application.h"
#include "memory.h"
#include "widgets.h"
#include "input.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************
* ChangeNavigationRotate関数                     *
* ナビゲーションの回転角度変更時に呼び出す関数   *
* 引数                                           *
* navigation	: ナビゲーションウィンドウの情報 *
* draw_window	: 表示する描画領域               *
*************************************************/
void ChangeNavigationRotate(NAVIGATION_WINDOW* navigation, DRAW_WINDOW* window)
{
	cairo_matrix_t matrix;
	FLOAT_T zoom_x, zoom_y;
	FLOAT_T trans_x, trans_y;
	FLOAT_T half_size;
	int half_width, half_height;

	if(window->width > window->height)
	{
		half_size = window->width * ROOT2;
	}
	else
	{
		half_size = window->height * ROOT2;
	}

	half_width = navigation->width / 2;
	half_height = navigation->height / 2;

	zoom_x = ((gdouble)window->width) / navigation->width;
	zoom_y = ((gdouble)window->height) / navigation->height;


	if((window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) == 0)
	{
		trans_x = - half_width*zoom_x +
			(half_width * (1/ROOT2) * zoom_x * window->cos_value + half_height * (1/ROOT2) * zoom_x * window->sin_value);
		trans_x /= zoom_x;
		trans_y = - half_height*zoom_y - (
			half_width * (1/ROOT2) * zoom_y * window->sin_value - half_height * (1/ROOT2) * zoom_y * window->cos_value);
		trans_y /= zoom_y;

		cairo_matrix_init_scale(&matrix, zoom_x*ROOT2, zoom_y*ROOT2);
		cairo_matrix_rotate(&matrix, window->angle);
		cairo_matrix_translate(&matrix, trans_x, trans_y);
	}
	else
	{
		trans_x = - half_width*zoom_x +
			(- half_width * (1/ROOT2) * zoom_x * window->cos_value + half_height * (1/ROOT2) * zoom_x * window->sin_value);
		trans_x /= zoom_x;
		trans_y = - half_height*zoom_y - (
			- half_width * (1/ROOT2) * zoom_y * window->sin_value - half_height * (1/ROOT2) * zoom_y * window->cos_value);
		trans_y /= zoom_y;

		cairo_matrix_init_scale(&matrix, - zoom_x*ROOT2, zoom_y*ROOT2);
		cairo_matrix_rotate(&matrix, window->angle);
		cairo_matrix_translate(&matrix, trans_x, trans_y);
	}

	cairo_pattern_set_matrix(navigation->pattern, &matrix);
}

/*************************************************
* ChangeNavigationDrawWindow関数                 *
* ナビゲーションで表示する描画領域を変更する     *
* 引数                                           *
* navigation	: ナビゲーションウィンドウの情報 *
* draw_window	: 表示する描画領域               *
*************************************************/
void ChangeNavigationDrawWindow(NAVIGATION_WINDOW* navigation, DRAW_WINDOW* window)
{
	GtkAllocation draw_allocation, navi_allocation;

	// アクティブな描画領域のウィンドウサイズを取得
	gtk_widget_get_allocation(window->scroll, &draw_allocation);
	// ナビゲーションのサムネイルのサイズを取得
	gtk_widget_get_allocation(navigation->draw_area, &navi_allocation);

	if(navigation->pattern != NULL)
	{
		cairo_pattern_destroy(navigation->pattern);
	}
	navigation->pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);

	navigation->point.width =
		(gdouble)draw_allocation.width / ((gdouble)window->disp_size);
	if(navigation->point.width > 1.0)
	{
		navigation->point.width = 1.0;
	}
	// ナビゲーションが指定している高さ
	navigation->point.height =
		(gdouble)draw_allocation.height / ((gdouble)window->disp_size);
	if(navigation->point.height > 1.0)
	{
		navigation->point.height = 1.0;
	}

	// 描画領域の幅・高さを記憶
	navigation->draw_width = draw_allocation.width;
	navigation->draw_height = draw_allocation.height;

	// 表示座標補正用の値を計算
	navigation->point.ratio_x =
		(1.0 / window->disp_size) * navi_allocation.width;
	navigation->point.ratio_y =
		(1.0 / window->disp_size) * navi_allocation.height;

	// 回転角度を記憶
	navigation->point.angle = window->angle;
	// 拡大縮小率を記憶
	navigation->point.zoom = window->zoom;

	// ナビゲーションのパターンを回転
	ChangeNavigationRotate(navigation, window);
}

/*************************************************************
* DisplayNavigation関数                                      *
* ナビゲーションウィンドウの描画処理                         *
* 引数                                                       *
* widget	: ナビゲーションウィンドウの描画領域ウィジェット *
* event		: 描画イベントの情報                             *
* app		: アプリケーションを管理する構造体のアドレス     *
* 返り値                                                     *
*	常にTRUE                                                 *
*************************************************************/
#if GTK_MAJOR_VERSION <= 2
static gboolean DisplayNavigation(
	GtkWidget *widget,
	GdkEventExpose *event,
	APPLICATION *app
)
#else
static gboolean DisplayNavigation(
	GtkWidget *widget,
	cairo_t *cairo_p,
	APPLICATION *app
)
#endif
{
	// 表示する描画領域
	DRAW_WINDOW *window;
#if GTK_MAJOR_VERSION <= 2
	// 画面表示用のCairo情報
	cairo_t *cairo_p;
#endif
	// 画面表示用のピクセルデータの1行分のバイト数
	int32 stride;
	// 描画領域とナビゲーションのサムネイルのサイズ取得用
	GtkAllocation draw_allocation, navi_allocation;

	// キャンバスの数が0なら終了
	if(app->window_num == 0)
	{
		return TRUE;
	}

	window = GetActiveDrawWindow(app);

	// 描画領域のサイズを取得
	gtk_widget_get_allocation(window->scroll, &draw_allocation);
	// ナビゲーションのサムネイルのサイズを取得
	gtk_widget_get_allocation(
		app->navigation_window.draw_area, &navi_allocation);
	// 表示画像のピクセルデータの1行分のバイト数を計算
	stride = navi_allocation.width * 4;

	// ナビゲーションのサイズが変更されていたら
		// ピクセルデータのメモリを再確保
	if(app->navigation_window.width != navi_allocation.width
		|| app->navigation_window.height != navi_allocation.height)
	{
		app->navigation_window.pixels = (uint8*)MEM_REALLOC_FUNC(
			app->navigation_window.pixels, navi_allocation.height*stride);
		app->navigation_window.reverse_buff = (uint8*)MEM_REALLOC_FUNC(
			app->navigation_window.reverse_buff, stride);
		app->navigation_window.width = navi_allocation.width;
		app->navigation_window.height = navi_allocation.height;

		// ナビゲーションが指定している幅
		app->navigation_window.point.width =
			(gdouble)draw_allocation.width / window->disp_size;
		if(app->navigation_window.point.width > 1.0)
		{
			app->navigation_window.point.width = 1.0;
		}
		// ナビゲーションが指定している高さ
		app->navigation_window.point.height =
			(gdouble)draw_allocation.height / window->disp_size;
		if(app->navigation_window.point.height > 1.0)
		{
			app->navigation_window.point.height = 1.0;
		}

		// 描画領域の幅・高さを記憶
		app->navigation_window.draw_width = draw_allocation.width;
		app->navigation_window.draw_height = draw_allocation.height;

		// 表示座標補正用の値を計算
		app->navigation_window.point.ratio_x =
			(1.0 / (window->disp_size))
				* navi_allocation.width;
		app->navigation_window.point.ratio_y =
			(1.0 / (window->disp_size))
				* navi_allocation.height;

		// 回転角度を記憶
		app->navigation_window.point.angle = window->angle;
		// 拡大縮小率を記憶
		app->navigation_window.point.zoom = window->zoom;

		// 表示パターンを更新
		ChangeNavigationDrawWindow(&app->navigation_window, window);
	}	// ナビゲーションのサイズが変更されていたら
			// ピクセルデータのメモリを再確保
		// if(app->navigation_window.width != navi_allocation.width
			// || app->navigation_window.height != navi_allocation.height)
	else if(app->navigation_window.point.zoom != window->zoom
		|| app->navigation_window.point.angle != window->angle
		|| app->navigation_window.draw_width != draw_allocation.width
			|| app->navigation_window.draw_height != draw_allocation.height)
	{	// 拡大縮小率またはウィンドウサイズが変更されていたら
			// ナビゲーションが指定している幅
		app->navigation_window.point.width =
			(gdouble)draw_allocation.width / ((gdouble)window->disp_size);
		if(app->navigation_window.point.width > 1.0)
		{
			app->navigation_window.point.width = 1.0;
		}
		// ナビゲーションが指定している高さ
		app->navigation_window.point.height =
			(gdouble)draw_allocation.height / ((gdouble)window->disp_size);
		if(app->navigation_window.point.height > 1.0)
		{
			app->navigation_window.point.height = 1.0;
		}

		// 描画領域の幅・高さを記憶
		app->navigation_window.draw_width = draw_allocation.width;
		app->navigation_window.draw_height = draw_allocation.height;

		// 表示座標補正用の値を計算
		app->navigation_window.point.ratio_x =
			(1.0 / window->disp_size) * navi_allocation.width;
		app->navigation_window.point.ratio_y =
			(1.0 / window->disp_size) * navi_allocation.height;

		// 回転角度を記憶
		app->navigation_window.point.angle = window->angle;
		// 拡大縮小率を記憶
		app->navigation_window.point.zoom = window->zoom;

		// ナビゲーションのパターンを回転
		ChangeNavigationRotate(&app->navigation_window, window);
	}

	// 画像を拡大縮小してナビゲーションの画像へ
#if GTK_MAJOR_VERSION <= 2
	//cairo_p = kaburagi_cairo_create((struct _GdkWindow*)app->navigation_window.draw_area->window);
	cairo_p = gdk_cairo_create(app->navigation_window.draw_area->window);
#endif
	cairo_set_operator(cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cairo_p, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_set_source(cairo_p, app->navigation_window.pattern);
	cairo_paint(cairo_p);
	cairo_set_operator(cairo_p, CAIRO_OPERATOR_OVER);

	// 左右反転表示中なら表示内容を左右反転
	if((window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) != 0)
	{
		uint8 *ref, *src;
		int x, y;

		for(y=0; y<navi_allocation.height; y++)
		{
			ref = app->navigation_window.reverse_buff;
			src = &app->navigation_window.pixels[(y+1)*stride-4];

			for(x=0; x<navi_allocation.width; x++, ref += 4, src -= 4)
			{
				ref[0] = src[0], ref[1] = src[1], ref[2] = src[2], ref[3] = src[3];
			}
			(void)memcpy(src+4, app->navigation_window.reverse_buff, stride);
		}
	}

	// 描画領域が画面に収まり切らないときは
		// 現在の表示位置を表示する
	if(draw_allocation.width <= window->disp_size || draw_allocation.height <= window->disp_size)
	{	// 表示座標
		gdouble x, y, width, height;

		// スクロールバーの位置から表示座標を計算
		x = gtk_range_get_value(GTK_RANGE(gtk_scrolled_window_get_hscrollbar(
			GTK_SCROLLED_WINDOW(window->scroll))));
		app->navigation_window.point.x = x;
		x *= app->navigation_window.point.ratio_x;
		width = app->navigation_window.point.width * navi_allocation.width;
		if(x + width > navi_allocation.width)
		{
			width = navi_allocation.width - x;
		}

		y = gtk_range_get_value(GTK_RANGE(gtk_scrolled_window_get_vscrollbar(
			GTK_SCROLLED_WINDOW(window->scroll))));
		app->navigation_window.point.y = y;
		y *= app->navigation_window.point.ratio_y;
		height = app->navigation_window.point.height * navi_allocation.height;
		if(y + height > navi_allocation.height)
		{
			height = navi_allocation.height - y;
		}

#if 0 // defined(DISPLAY_BGR) && DISPLAY_BGR != 0
		cairo_set_source_rgb(cairo_p, 0, 0, 1);
#else
		cairo_set_source_rgb(cairo_p, 1, 0, 0);
#endif

		cairo_rectangle(cairo_p, x, y, width, height);
		cairo_stroke_preserve(cairo_p);
		// 表示していない範囲は暗くする
		cairo_set_source_rgba(cairo_p, 0, 0, 0, 0.5);
		cairo_rectangle(cairo_p, 0, 0,
			navi_allocation.width, navi_allocation.height);
		cairo_fill(cairo_p);
	}	// 描画領域が画面に収まり切らないときは
			// 現在の表示位置を表示する
		// if(draw_allocation.width <= app->draw_window[app->active_window]->width * app->draw_window[app->active_window]->zoom * 0.01
			// || draw_allocation.height <= app->draw_window[app->active_window]->height * app->draw_window[app->active_window]->zoom * 0.01)

#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

/*********************************************************
* NavigationButtonPressCallBack関数                      *
* ナビゲーションウィンドウでのクリックに対する処理       *
* 引数                                                   *
* widget	: ナビゲーションの描画領域                   *
* event		: マウスの情報                               *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	常にTRUE                                             *
*********************************************************/
static gboolean NavigationButtonPressCallBack(
	GtkWidget* widget,
	GdkEventButton* event,
	APPLICATION* app
)
{
	// 描画領域のスクロールバー
	GtkWidget* scroll_bar;
	// ナビゲーションが指定している範囲の
		// 左上の座標
	gint x, y;
	// ナビゲーションが指定している範囲の幅・高さ
	gdouble width, height;
	// 表示している描画領域
	DRAW_WINDOW *window;
	// ナビゲーションの座標指定用のサイズ
	gdouble size;

	// キャンバスの数が0なら終了
	if(app->window_num == 0)
	{
		return TRUE;
	}

	window = GetActiveDrawWindow(app);
	size = window->disp_size;

	// ナビゲーションが指定している範囲の幅・高さを計算
	width = app->navigation_window.point.width * app->navigation_window.width;
	height = app->navigation_window.point.height * app->navigation_window.height;

	// マウスの座標を取得
	x = (gint)event->x,	y = (gint)event->y;

	// クリックされた座標を指定範囲の中心にする
	x = (gint)((x - (app->navigation_window.point.width * app->navigation_window.width) * 0.5)
		* (gdouble)size / (gdouble)app->navigation_window.width);
	y = (gint)((y - (app->navigation_window.point.height * app->navigation_window.height) * 0.5)
		* (gdouble)size / (gdouble)app->navigation_window.height);

	// 外にはみ出たら修正
	if(x < 0)
	{
		x = 0;
	}
	else if(x + width > size)
	{
		x = (gint)(size - width);
	}

	if(y < 0)
	{
		y = 0;
	}
	else if(y + height > size)
	{
		y = (gint)(size - height);
	}

	// 描画領域のスクロールに座標を設定
	scroll_bar = gtk_scrolled_window_get_hscrollbar(
		GTK_SCROLLED_WINDOW(app->draw_window[app->active_window]->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar), x);
	scroll_bar = gtk_scrolled_window_get_vscrollbar(
		GTK_SCROLLED_WINDOW(app->draw_window[app->active_window]->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar), y);

	return TRUE;
}

/*********************************************************
* NavigationMotionNotifyCallBack関数                     *
* ナビゲーションウィンドウでのクリックに対する処理       *
* 引数                                                   *
* widget	: ナビゲーションの描画領域                   *
* event		: マウスの情報                               *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	常にTRUE                                             *
*********************************************************/
static gboolean NavigationMotionNotifyCallBack(
	GtkWidget* widget,
	GdkEventMotion* event,
	APPLICATION* app
)
{
	// 描画領域のスクロールバー
	GtkWidget* scroll_bar;
	// ナビゲーションが指定している範囲の
		// 左上の座標
	gint x, y;
	// マウスの状態
	GdkModifierType state;
	// ナビゲーションが指定している範囲の幅・高さ
	gdouble width, height;
	// 表示している描画領域
	DRAW_WINDOW *window;
	// ナビゲーションの座標指定用のサイズ
	gdouble size;

	// マウスの座標を取得
	if(event->is_hint != 0)
	{
		gdk_window_get_pointer(event->window, &x, &y, &state);
	}
	else
	{
		x = (gint)event->x,	y = (gint)event->y;
		state = event->state;
	}

	// キャンバスの数が0または左クリックされていなければ終了
	if(app->window_num == 0 || (state & GDK_BUTTON1_MASK) == 0)
	{
		return TRUE;
	}

	window = GetActiveDrawWindow(app);
	size = window->disp_size;

	// ナビゲーションが指定している範囲の幅・高さを計算
	width = app->navigation_window.point.width * app->navigation_window.width;
	height = app->navigation_window.point.height * app->navigation_window.height;

	// クリックされた座標を指定範囲の中心にする
	x = (gint)((x - (app->navigation_window.point.width * app->navigation_window.width) * 0.5)
		* (gdouble)size / (gdouble)app->navigation_window.width);
	y = (gint)((y - (app->navigation_window.point.height * app->navigation_window.height) * 0.5)
		* (gdouble)size / (gdouble)app->navigation_window.height);

	// 外にはみ出たら修正
	if(x < 0)
	{
		x = 0;
	}
	else if(x + width > app->draw_window[app->active_window]->disp_size)
	{
		x = (gint)(app->draw_window[app->active_window]->disp_size - width);
	}

	if(y < 0)
	{
		y = 0;
	}
	else if(y + height > app->draw_window[app->active_window]->disp_size)
	{
		y = (gint)(app->draw_window[app->active_window]->disp_size - height);
	}

	// 描画領域のスクロールに座標を設定
	scroll_bar = gtk_scrolled_window_get_hscrollbar(
		GTK_SCROLLED_WINDOW(app->draw_window[app->active_window]->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar), x);
	scroll_bar = gtk_scrolled_window_get_vscrollbar(
		GTK_SCROLLED_WINDOW(app->draw_window[app->active_window]->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar), y);

	// 画面更新
	gtk_widget_queue_draw(app->navigation_window.draw_area);

	return TRUE;
}

/***************************************************************************
* ChangeNavigationZoomSlider関数                                           *
* ナビゲーションウィンドウの拡大縮小率変更スライダ操作時のコールバック関数 *
* 引数                                                                     *
* slider	: ナビゲーションウィンドウのスライダ用のアジャスタ             *
* app		: アプリケーションを管理する構造体のアドレス                   *
***************************************************************************/
static void ChangeNavigationZoomSlider(GtkAdjustment* slider, APPLICATION* app)
{
	DRAW_WINDOW *window;
	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);
	// アクティブな描画領域に拡大縮小率を設定
	window->zoom = (uint16)gtk_adjustment_get_value(slider);

	DrawWindowChangeZoom(window, window->zoom);
}

/*************************************************************************
* ChangeNavigationRotateSlider関数                                       *
* ナビゲーションウィンドウの回転角度変更スライダ操作時のコールバック関数 *
* 引数                                                                   *
* slider	: ナビゲーションウィンドウのスライダ用のアジャスタ           *
* app		: アプリケーションを管理する構造体のアドレス                 *
*************************************************************************/
static void ChangeNavigationRotateSlider(GtkAdjustment* slider, APPLICATION* app)
{
	DRAW_WINDOW *window;
	FLOAT_T angle;
	cairo_matrix_t matrix;

	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);

	// アクティブな描画領域に回転角度を設定
	angle = window->angle =
		- gtk_adjustment_get_value(slider) * G_PI / 180;

	// 回転角を設定
	window->sin_value = sin(angle);
	window->cos_value = cos(angle);

	window->trans_x = - window->half_size + ((window->disp_layer->width / 2) * window->cos_value + (window->disp_layer->height / 2) * window->sin_value);
	window->trans_y = - window->half_size - ((window->disp_layer->width / 2) * window->sin_value - (window->disp_layer->height / 2) * window->cos_value);
	
	window->add_cursor_x = - (window->half_size - window->disp_layer->width / 2) + window->half_size;
	window->add_cursor_y = - (window->half_size - window->disp_layer->height / 2) + window->half_size;

	cairo_matrix_init_rotate(&matrix, angle);
	cairo_matrix_translate(&matrix, window->trans_x, window->trans_y);
	cairo_pattern_set_matrix(window->rotate, &matrix);

	UpdateDrawWindowClippingArea(window);

	gtk_widget_queue_draw(app->navigation_window.draw_area);
}

/*************************************************************
* OnDeleteNavigationWindow関数                               *
* ナビゲーションウィンドウが削除されるときに呼び出される     *
* 引数                                                       *
* window		: ナビゲーションウィンドウウィジェット       *
* event_info	: 削除イベントの情報                         *
* app			: アプリケーションを管理する構造体のアドレス *
* 返り値                                                     *
*	常にFALSE(ウィンドウ削除)                                *
*************************************************************/
static gboolean OnDeleteNavigationWindow(GtkWidget* window, GdkEvent* event_info, APPLICATION* app)
{
	gint x, y;
	gint width, height;

	gtk_window_get_position(GTK_WINDOW(window), &x, &y);
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);

	app->navigation_window.window_x = x, app->navigation_window.window_y = y;
	app->navigation_window.window_width = width, app->navigation_window.window_height = height;

	return FALSE;
}

/*************************************************************
* OnDestroyNavigationWidget関数                              *
* ナビゲーションのウィジェットが削除されるときに呼び出される *
* 引数                                                       *
* app	: アプリケーションを管理する構造体のアドレス         *
*************************************************************/
static void OnDestroyNavigationWidget(APPLICATION* app)
{
	MEM_FREE_FUNC(app->navigation_window.pixels);
	app->navigation_window.pixels = NULL;
	MEM_FREE_FUNC(app->navigation_window.reverse_buff);
	app->navigation_window.reverse_buff = NULL;

	app->menus.disable_if_no_open[app->navigation_window.zoom_index] = NULL;
	app->menus.disable_if_no_open[app->navigation_window.rotate_index] = NULL;

	app->navigation_window.window = NULL;
	app->navigation_window.draw_area = NULL;
	app->navigation_window.vbox = NULL;
	app->navi_layer_pane = NULL;
	app->navigation_window.width = 0;
	app->navigation_window.height = 0;
}

/*********************************************************************
* InitializeNavigation関数                                           *
* ナビゲーションウィンドウを初期化                                   *
* 引数                                                               *
* navigation	: ナビゲーションウィンドウを管理する構造体のアドレス *
* app			: アプリケーションを管理する構造体のアドレス         *
* box			: ドッキングする場合はボックスウィジェットを指定     *
*********************************************************************/
void InitializeNavigation(
	NAVIGATION_WINDOW* navigation,
	APPLICATION *app,
	GtkWidget* box
)
{
	// ウィジェット整列用
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	// ボタンウィジェット
	GtkWidget *button;

	if(box == NULL)
	{
		// ウィンドウを作成
		navigation->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		// ウィンドウの位置とサイズを設定
		gtk_window_move(GTK_WINDOW(navigation->window), navigation->window_x, navigation->window_y);
		gtk_window_resize(GTK_WINDOW(navigation->window),
			navigation->window_width, navigation->window_height);
		// 閉じるボタンのみのウィンドウへ
		gtk_window_set_type_hint(GTK_WINDOW(navigation->window), GDK_WINDOW_TYPE_HINT_UTILITY);
		// サイズを指定
		gtk_widget_set_size_request(navigation->window, 150, 150);
		// ウィンドウタイトルをセット
		gtk_window_set_title(GTK_WINDOW(navigation->window),
			app->labels->navigation.title);
		// 親ウィンドウを登録
		gtk_window_set_transient_for(
			GTK_WINDOW(navigation->window), GTK_WINDOW(app->window));
		// タスクバーには表示しない
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(navigation->window), TRUE);
		// ショートカットキーを登録
		gtk_window_add_accel_group(GTK_WINDOW(navigation->window), app->hot_key);
		// ウィンドウを削除するときにイベントを設定
		(void)g_signal_connect(G_OBJECT(navigation->window), "delete_event",
			G_CALLBACK(OnDeleteNavigationWindow), app);

		// キーボードのコールバック関数をセット
		(void)g_signal_connect(G_OBJECT(navigation->window), "key-press-event",
			G_CALLBACK(KeyPressEvent), app);
		(void)g_signal_connect(G_OBJECT(navigation->window), "key-release-event",
			G_CALLBACK(KeyPressEvent), app);
	}
	else
	{
		navigation->window = NULL;
		gtk_paned_add1(GTK_PANED(box), vbox);
	}
	// ウィジェット削除時のコールバック関数をセット
	(void)g_signal_connect_swapped(G_OBJECT(vbox), "destroy",
		G_CALLBACK(OnDestroyNavigationWidget), app);

	// 画像の表示領域を作成
	navigation->draw_area = gtk_drawing_area_new();
	// ボックスに追加
	gtk_box_pack_start(GTK_BOX(vbox), navigation->draw_area, TRUE, TRUE, 0);

	// 描画用のコールバック関数をセット
#if GTK_MAJOR_VERSION <= 2
	(void)g_signal_connect(G_OBJECT(navigation->draw_area), "expose_event",
		G_CALLBACK(DisplayNavigation), app);
#else
	(void)g_signal_connect(G_OBJECT(navigation->draw_area), "draw",
		G_CALLBACK(DisplayNavigation), app);
#endif
	// マウスクリックのコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(navigation->draw_area), "button_press_event",
		G_CALLBACK(NavigationButtonPressCallBack), app);
	// マウスオーバーのコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(navigation->draw_area), "motion_notify_event",
		G_CALLBACK(NavigationMotionNotifyCallBack), app);

	// イベントの種類をセット
	gtk_widget_add_events(navigation->draw_area,
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_HINT_MASK |
		GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);

	// 拡大縮小率変更、回転角度変更用のスライダを追加
	{
		GtkWidget *scale;
		GtkAdjustment *adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(100, MIN_ZOOM, MAX_ZOOM, 1, 10, 0));

		navigation->zoom_slider = adjustment;

		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeNavigationZoomSlider), app);
		scale = SpinScaleNew(adjustment, app->labels->menu.zoom, 0);
		gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, TRUE, 2);

		if((app->flags & APPLICATION_INITIALIZED) == 0)
		{
			navigation->zoom_index = app->menus.num_disable_if_no_open;
			app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = scale;
			gtk_widget_set_sensitive(scale, FALSE);
			app->menus.num_disable_if_no_open++;
		}
		else
		{
			app->menus.disable_if_no_open[navigation->zoom_index] = scale;
			gtk_widget_set_sensitive(scale, app->window_num > 0);
		}

		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0, -180, 180, 1, 15, 0));

		navigation->rotate_slider = adjustment;

		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeNavigationRotateSlider), app);
		scale = SpinScaleNew(adjustment, app->labels->menu.rotate, 0);
		gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, TRUE, 2);

		if((app->flags & APPLICATION_INITIALIZED) == 0)
		{
			navigation->rotate_index = app->menus.num_disable_if_no_open;
			app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = scale;
			gtk_widget_set_sensitive(scale, FALSE);
			app->menus.num_disable_if_no_open++;
		}
		else
		{
			app->menus.disable_if_no_open[navigation->rotate_index] = scale;
			gtk_widget_set_sensitive(scale, app->window_num > 0);
		}
	}

	// 等倍表示ボタンを追加
	button = gtk_button_new_with_label(app->labels->menu.zoom_reset);
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteZoomReset), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	// 回転角度リセットボタンを追加
	button = gtk_button_new_with_label(app->labels->menu.reset_rotate);
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteRotateReset), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	if(navigation->window != NULL)
	{
		gtk_container_add(GTK_CONTAINER(navigation->window), vbox);
		// ウィンドウ表示
		gtk_widget_show_all(navigation->window);
	}

	navigation->vbox = vbox;
}

#ifdef __cplusplus
}
#endif
