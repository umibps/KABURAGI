// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <math.h>
#include "color_gtk.h"
#include "../../application.h"
#include "../../utils.h"
#include "../../memory.h"
#include "utils_gtk.h"
#include "gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COLOR_BOX_SIZE 32
#define COLOR_CIRCLE_WIDTH 15
#define HUE_ADD 150

static void UpdateColorChooserBoxPixel(COLOR_CHOOSER* chooser)
{
	cairo_t *cairo_p;
	int size = (chooser->line_width * COLOR_BOX_SIZE) / COLOR_CIRCLE_WIDTH;
	int half_size = size / 2;
	int quater_size = size / 4;

	(void)memset(chooser->color_box_pixel_data, 0, ((size*3)/2*((size*3)/2))*4);

	cairo_p = cairo_create(chooser->color_box_surface);

	cairo_set_line_width(cairo_p, chooser->line_width / 15.0);

	cairo_set_source_rgb(cairo_p, 0, 0, 0);
	cairo_move_to(cairo_p, size, quater_size);
	cairo_line_to(cairo_p, size + quater_size, quater_size);
	cairo_line_to(cairo_p, size + quater_size, half_size);
	cairo_stroke(cairo_p);
	cairo_move_to(cairo_p, size, quater_size);
	cairo_line_to(cairo_p, size + size / 8, quater_size - size / 8);
	cairo_stroke(cairo_p);
	cairo_move_to(cairo_p, size, quater_size);
	cairo_line_to(cairo_p, size + size / 8, quater_size + size / 8);
	cairo_stroke(cairo_p);
	cairo_move_to(cairo_p, size + quater_size, half_size);
	cairo_line_to(cairo_p, size + quater_size - size / 8, half_size - size / 8);
	cairo_stroke(cairo_p);
	cairo_move_to(cairo_p, size + quater_size, half_size);
	cairo_line_to(cairo_p, size + quater_size + size / 8, half_size - size / 8);
	cairo_stroke(cairo_p);

	cairo_rectangle(cairo_p, half_size, half_size, size, size);
	cairo_stroke_preserve(cairo_p);
	cairo_set_source_rgb(cairo_p,
		chooser->back_rgb[0] * DIV_PIXEL, chooser->back_rgb[1] * DIV_PIXEL, chooser->back_rgb[2] * DIV_PIXEL
	);
	cairo_fill(cairo_p);

	cairo_set_source_rgb(cairo_p, 0, 0, 0);
	cairo_rectangle(cairo_p, 0, 0, size, size);
	cairo_stroke_preserve(cairo_p);
	cairo_set_source_rgb(cairo_p,
		chooser->rgb[0] * DIV_PIXEL, chooser->rgb[1] * DIV_PIXEL, chooser->rgb[2] * DIV_PIXEL
	);
	cairo_fill(cairo_p);

	cairo_destroy(cairo_p);
}

void UpdateColorBox(COLOR_CHOOSER* chooser)
{
	uint8* pixels = chooser->color_box_pixel_data;
	int size = (chooser->line_width * COLOR_BOX_SIZE) / COLOR_CIRCLE_WIDTH;

	UpdateColorChooserBoxPixel(chooser);

	if(chooser->filter_func != NULL)
	{
		chooser->filter_func(pixels, pixels,
			((size*3)/2)*((size*3)/2), chooser->filter_data);
	}

	// 再描画
	gtk_widget_queue_draw(chooser->color_box);
}

void AddColorHistory(COLOR_CHOOSER* chooser, const uint8 color[3])
{
	int i;

	for(i=0; i<MAX_COLOR_HISTORY_NUM-1; i++)
	{
		chooser->color_history[MAX_COLOR_HISTORY_NUM-i-1][0] =
			chooser->color_history[MAX_COLOR_HISTORY_NUM-i-2][0];
		chooser->color_history[MAX_COLOR_HISTORY_NUM-i-1][1] =
			chooser->color_history[MAX_COLOR_HISTORY_NUM-i-2][1];
		chooser->color_history[MAX_COLOR_HISTORY_NUM-i-1][2] =
			chooser->color_history[MAX_COLOR_HISTORY_NUM-i-2][2];
	}

	chooser->color_history[0][0] = color[0];
	chooser->color_history[0][1] = color[1];
	chooser->color_history[0][2] = color[2];

	if(chooser->num_color_history < MAX_COLOR_HISTORY_NUM)
	{
		chooser->num_color_history++;
	}
}

void SetColorChooserPoint(COLOR_CHOOSER* chooser, HSV* set_hsv, gboolean add_history)
{
	FLOAT_T arg = atan2(set_hsv->v, set_hsv->s);
	// 選択用の円を描画する座標
	FLOAT_T draw_x, draw_y;
	FLOAT_T d = sqrt(set_hsv->v*set_hsv->v+set_hsv->s*set_hsv->s);
	// 彩度、明度選択の領域の幅
	int box_size = (chooser->widget_width + chooser->line_width)/2;
	int r = chooser->widget_width/2;

	d /= 255.0/box_size;
	draw_x = chooser->widget_width/2 - box_size/2 + d * cos(arg);
	draw_y = chooser->widget_height/2 + box_size/2 - d * sin(arg);

	// 彩度・輝度移動中のフラグを立てる
	chooser->flags |= COLOR_CHOOSER_MOVING_SQUARE;

	// 明度、彩度の座標を記憶
	chooser->sv_x = draw_x,	chooser->sv_y = draw_y;

	// 色を変更
	set_hsv->h -= HUE_ADD;
	if(set_hsv->h < 0)
	{
		set_hsv->h += 360;
	}
	chooser->hsv = *set_hsv;

	// 再描画
	gtk_widget_queue_draw(chooser->choose_area);

	// 描画色・背景色表示を更新
	UpdateColorBox(chooser);

	// 色変更時のコールバック関数を呼び出す
	if(chooser->color_change_callback != NULL)
	{
		chooser->color_change_callback(chooser->widget, chooser->rgb, chooser->data);
	}

	if(add_history != FALSE)
	{
		// 色選択の履歴に追加する
		AddColorHistory(chooser, chooser->rgb);
	}
}

static gboolean OnColorChooserClicked(GtkWidget* widget, GdkEventButton* event, COLOR_CHOOSER* chooser)
{
	GdkModifierType state;
	gint x, y;
	gint width, height;
	FLOAT_T d, arg;
	int r = chooser->widget_width/2;
	// 中心の選択領域のサイズ
	int32 box_size = (chooser->widget_width + chooser->line_width)/2;

	width = chooser->widget_width,	height = chooser->widget_height;

	// クリックされた座標を取得
	x = (gint)event->x, y = (gint)event->y;
	state = event->state;
	
	// ウィジェットの中心からの距離を求める
	d = sqrt((x-width/2)*(x-width/2)
		+ (y-height/2)*(y-height/2));
	// 中心からの角度を求める
	arg = atan2(y-height/2, x-width/2);
	// 距離によって色相の操作か明度、彩度の操作かを決める
	if(d > r-chooser->line_width && d < r+chooser->line_width)
	{	// 色相の操作
		// 色相を選択する円を描画する座標
		FLOAT_T draw_x, draw_y;
		// HSVの色情報
		HSV hsv = {0, 255, 255};
		// 色相選択の円を描画する座標を計算
		hsv.h = (int16)(arg * 180 / G_PI);
		
		// 若干の加工
		if(hsv.h < 0)
		{
			hsv.h += 360;
		}
		else if(hsv.h >= 360)
		{
			hsv.h -= 360;
		}
		
		// 色相情報を記憶
		chooser->hsv.h = hsv.h;

		// 色相移動中のフラグを立てる
		chooser->flags |= COLOR_CHOOSER_MOVING_CIRCLE;

		// RGB色情報を記憶
			// ウィジェットの示している座標から色を計算
		draw_x = (chooser->sv_x - (chooser->widget_width - box_size) / 2) / box_size;
		draw_y = 1.0 - (chooser->sv_y - (chooser->widget_height - box_size) / 2) / box_size;
		// 範囲外にならないように調整
		if(draw_x > 1.0)
		{
			draw_x = 1.0;
		}
		if(draw_y < 0.0)
		{
			draw_y = 0.0;
		}
		hsv.h = hsv.h + HUE_ADD;
		if(hsv.h >= 360)
		{
			hsv.h -= 360;
		}
		hsv.s = (uint8)(draw_x * 255);
		hsv.v = (uint8)(draw_y * 255);
		HSV2RGB_Pixel(&hsv, chooser->rgb);

		// 再描画
		gtk_widget_queue_draw(chooser->choose_area);
	}
	else if(x >= (width-box_size)/2 && y >= (height-box_size)/2
		&& x <= (width+box_size)/2 && y <= (height+box_size)/2)
	{	// 明度、彩度の変更
		// 円の色決定用
		HSV hsv = chooser->hsv;
		// 選択用の円を描画する座標
		FLOAT_T draw_x, draw_y;
		draw_x = chooser->widget_width/2 + d * cos(arg);
		draw_y = chooser->widget_height/2 + d * sin(arg);
		hsv.s = 255; hsv.v = 255;

		// 彩度・輝度移動中のフラグを立てる
		chooser->flags |= COLOR_CHOOSER_MOVING_SQUARE;

		// 明度、彩度の座標を記憶
		chooser->sv_x = draw_x,	chooser->sv_y = draw_y;
		
		// RGB色情報を記憶
			// ウィジェットの示している座標から色を計算
		draw_x = (draw_x - (chooser->widget_width - box_size) / 2) / box_size;
		draw_y = 1.0 - (draw_y - (chooser->widget_height - box_size) / 2) / box_size;
		// 範囲外にならないように調整
		if(draw_x > 1.0)
		{
			draw_x = 1.0;
		}
		if(draw_y < 0.0)
		{
			draw_y = 0.0;
		}
		hsv.h += HUE_ADD;
		if(hsv.h >= 360)
		{
			hsv.h -= 360;
		}
		hsv.s = (uint8)(draw_x * 255);
		hsv.v = (uint8)(draw_y * 255);
		chooser->hsv.s = hsv.s;
		chooser->hsv.v = hsv.v;
		HSV2RGB_Pixel(&hsv, chooser->rgb);

		// 再描画
		gtk_widget_queue_draw(chooser->choose_area);
	}

	// 色表示ウィジェットを更新
	UpdateColorBox(chooser);

	return TRUE;
}

static gboolean OnColorChooserButtonReleased(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	COLOR_CHOOSER *chooser = (COLOR_CHOOSER*)data;

	chooser->flags &= ~(COLOR_CHOOSER_MOVING_CIRCLE | COLOR_CHOOSER_MOVING_SQUARE);

	if(chooser->color_change_callback != NULL)
	{
		chooser->color_change_callback(widget, chooser->rgb, chooser->data);
	}

	AddColorHistory(chooser, chooser->rgb);

	return TRUE;
}

static gboolean OnColorChooserMotion(GtkWidget *widget, GdkEventMotion *event, COLOR_CHOOSER* chooser)
{
	GdkModifierType state;
	gint x, y;
	gint width, height;
	FLOAT_T d, arg;
	int r = chooser->widget_width/2;
	// 中心の選択領域のサイズ
	int32 box_size = (chooser->widget_width + chooser->line_width)/2;

	width = chooser->widget_width,	height = chooser->widget_height;

	// クリックされた座標を取得
	if(event->is_hint != 0)
	{
		gdk_window_get_pointer(event->window, &x, &y, &state);
	}
	else
	{
		x = (gint)event->x, y = (gint)event->y;
		state = event->state;
	}
	
	// ウィジェットの中心からの距離を求める
	d = sqrt((x-width/2)*(x-width/2)
		+ (y-height/2)*(y-height/2));
	// 中心からの角度を求める
	arg = atan2(y-height/2, x-width/2);

	if((chooser->flags & COLOR_CHOOSER_MOVING_CIRCLE) != 0)
	{	// 色相の操作
		// 色相を選択する円を描画する座標
		FLOAT_T draw_x, draw_y;
		// HSVの色情報
		HSV hsv = {0, 255, 255};
		// 色相選択の円を描画する座標を計算
		draw_x = chooser->widget_width/2 + cos(arg) * (r-chooser->line_width/2);
		draw_y = chooser->widget_height/2 + sin(arg) * (r-chooser->line_width/2);
		hsv.h = (int16)(arg * 180 / G_PI);
		
		// 範囲外は修正
		if(hsv.h < 0)
		{
			hsv.h += 360;
		}
		else if(hsv.h >= 360)
		{
			hsv.h -= 360;
		}
		
		// 色相情報を記憶
		chooser->hsv.h = hsv.h;

		// 色相移動中のフラグを立てる
		chooser->flags |= COLOR_CHOOSER_MOVING_CIRCLE;

		// RGB色情報を記憶
			// ウィジェットの示している座標から色を計算
		draw_x = (chooser->sv_x - (chooser->widget_width - box_size) / 2) / box_size;
		draw_y = 1.0 - (chooser->sv_y - (chooser->widget_height - box_size) / 2) / box_size;
		// 範囲外にならないように調整
		if(draw_x > 1.0)
		{
			draw_x = 1.0;
		}
		if(draw_y < 0.0)
		{
			draw_y = 0.0;
		}
		hsv.h += HUE_ADD;
		if(hsv.h >= 360)
		{
			hsv.h -= 360;
		}
		hsv.s = (uint8)(draw_x * 255);
		hsv.v = (uint8)(draw_y * 255);
		HSV2RGB_Pixel(&hsv, chooser->rgb);

		// 再描画
		gtk_widget_queue_draw(chooser->choose_area);
	}
	else if((chooser->flags & COLOR_CHOOSER_MOVING_SQUARE) != 0)
	{	// 明度、彩度の変更
		// 円の色決定用
		HSV hsv = chooser->hsv;
		// 選択用の円を描画する座標
		FLOAT_T draw_x, draw_y;
		draw_x = chooser->widget_width/2 + d * cos(arg);
		draw_y = chooser->widget_height/2 + d * sin(arg);
		hsv.s = 255; hsv.v = 255;

		// 四角領域の外に出てしまったら補正
		if(draw_x < (chooser->widget_width - box_size) / 2)
		{
			draw_x = (chooser->widget_width - box_size) / 2;
		}
		else if(draw_x >= (chooser->widget_width + box_size) / 2)
		{
			draw_x = (chooser->widget_width + box_size) / 2;
		}
		if(draw_y < (chooser->widget_height - box_size) / 2)
		{
			draw_y = (chooser->widget_height - box_size) / 2;
		}
		else if(draw_y >= (chooser->widget_height + box_size) / 2)
		{
			draw_y = (chooser->widget_height + box_size) / 2;
		}

		// 彩度・輝度移動中のフラグを立てる
		chooser->flags |= COLOR_CHOOSER_MOVING_SQUARE;

		// 明度、彩度の座標を記憶
		chooser->sv_x = draw_x,	chooser->sv_y = draw_y;
		
		// RGB色情報を記憶
			// ウィジェットの示している座標から色を計算
		draw_x = (draw_x - (chooser->widget_width - box_size) / 2) / box_size;
		draw_y = 1.0 - (draw_y - (chooser->widget_height - box_size) / 2) / box_size;
		// 範囲外にならないように調整
		if(draw_x < 0.0)
		{
			draw_x = 0.0;
		}
		else if(draw_x > 1.0)
		{
			draw_x = 1.0;
		}
		if(draw_y < 0.0)
		{
			draw_y = 0.0;
		}
		else if(draw_y > 1.0)
		{
			draw_y = 1.0;
		}
		hsv.h += HUE_ADD;
		if(hsv.h >= 360)
		{
			hsv.h -= 360;
		}
		chooser->hsv.s = hsv.s = (uint8)(draw_x * 255.5);
		chooser->hsv.v = hsv.v = (uint8)(draw_y * 255.5);
		HSV2RGB_Pixel(&hsv, chooser->rgb);

		// 再描画
		gtk_widget_queue_draw(chooser->choose_area);
	}

	// 色表示ウィジェットを更新
	UpdateColorBox(chooser);

	return TRUE;
}

#if GTK_MAJOR_VERSION <= 2
gboolean DisplayColorChooser(GtkWidget* widget, GdkEventExpose* expose, COLOR_CHOOSER* chooser)
#else
gboolean DisplayColorChooser(GtkWidget* widget, cairo_t* cairo_p, COLOR_CHOOSER* chooser)
#endif
{
#if GTK_MAJOR_VERSION <= 2
	// 描画用のCairo情報
	//cairo_t *cairo_p = kaburagi_cairo_create((struct _GdkWindow*)widget->window);
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
#endif
	// グラデーションパターン
	cairo_pattern_t *pattern;
	// 色相の円の半径
	int r = chooser->widget_width/2;
	// 選択用の円を描画する座標
	FLOAT_T arg;
	FLOAT_T draw_x, draw_y;
	FLOAT_T d;
	// 中心の四角形のサイズ
	int box_size = (chooser->widget_width + chooser->line_width)/2;
	// 着色用HSV情報
	HSV hsv = {chooser->hsv.h, 0xff, 0xff};
	// グラデーション用RGB情報
	uint8 color[3];

	if(cairo_p == NULL)
	{
		gtk_widget_queue_draw(widget);
		return TRUE;
	}

	// 表示をリセット
	(void)memset(chooser->chooser_pixel_data, 0, chooser->widget_width * 4 * chooser->widget_height);

	// 周囲の円を描画
	cairo_set_source_surface(chooser->chooser_cairo, chooser->circle_surface, 0, 0);
	cairo_paint(chooser->chooser_cairo);

	// 中心の四角形を描画
		// 白→選択中の色でグラデーション
	hsv.h += HUE_ADD;
	if(hsv.h >= 360)
	{
		hsv.h -= 360;
	}
	HSV2RGB_Pixel(&hsv, color);
	pattern = cairo_pattern_create_linear(
		(chooser->widget_width-box_size)/2, (chooser->widget_height-box_size)/2,
		(chooser->widget_width+box_size)/2, (chooser->widget_height-box_size)/2
	);
	cairo_pattern_add_color_stop_rgb(pattern, 0, 1, 1, 1);
	cairo_pattern_add_color_stop_rgb(pattern, 1, color[0]*DIV_PIXEL,
		color[1]*DIV_PIXEL, color[2]*DIV_PIXEL);
	// 作成したグラデーションで正方形を描画
	cairo_rectangle(
		chooser->chooser_cairo,
		(chooser->widget_width-box_size)/2,
		(chooser->widget_height-box_size)/2,
		box_size,
		box_size
	);
	cairo_set_source(chooser->chooser_cairo, pattern);
	cairo_fill(chooser->chooser_cairo);
	cairo_pattern_destroy(pattern);

	// 白→黒 + αブレンドありでグラデーション
	pattern = cairo_pattern_create_linear(
		(chooser->widget_width-box_size)/2, (chooser->widget_height-box_size)/2,
		(chooser->widget_width-box_size)/2, (chooser->widget_height+box_size)/2
	);
	cairo_pattern_add_color_stop_rgba(pattern, 0, 0, 0, 0, 0);
	cairo_pattern_add_color_stop_rgba(pattern, 1, 0, 0, 0, 1);
	// 作成したグラデーションで正方形を描画
	cairo_rectangle(
		chooser->chooser_cairo,
		(chooser->widget_width-box_size)/2,
		(chooser->widget_height-box_size)/2,
		box_size,
		box_size
	);
	cairo_set_source(chooser->chooser_cairo, pattern);
	cairo_fill(chooser->chooser_cairo);

	if(chooser->filter_func != NULL)
	{
		chooser->filter_func(chooser->chooser_pixel_data, chooser->chooser_pixel_data,
			chooser->widget_width * chooser->widget_height, chooser->filter_data);
	}

	cairo_set_source_surface(cairo_p, chooser->chooser_surface, 0, 0);
	cairo_paint(cairo_p);

	// 選択中の位置を示す円を描画
		// 円の色を決定(選択中の色相の反対)
	hsv.h += 180;
	if(hsv.h  >= 360)
	{
		hsv.h -= 360;
	}
	HSV2RGB_Pixel(&hsv, color);
	// 彩度・明度選択の円を描画
	d = box_size * DIV_PIXEL;
	draw_x = chooser->widget_width/2 - box_size/2 + d * chooser->hsv.s;
	draw_y = chooser->widget_height/2 + box_size/2 - d * chooser->hsv.v;
	cairo_set_line_width(cairo_p, chooser->line_width * 0.2);
	cairo_set_source_rgb(cairo_p, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL);
	cairo_arc(cairo_p, draw_x, draw_y, chooser->line_width * (9.0/15.0), 0, (FLOAT_T)2*G_PI);
	cairo_stroke(cairo_p);
	// 色相選択の円を描画
	arg = (chooser->hsv.h) * (FLOAT_T)G_PI / 180;
	draw_x = chooser->widget_width/2 + cos(arg) * (r-chooser->line_width/2);
	draw_y = chooser->widget_height/2 + sin(arg) * (r-chooser->line_width/2);
	cairo_arc(cairo_p, draw_x, draw_y, chooser->line_width * (9.0/15.0), 0, (FLOAT_T)2.0*G_PI);
	cairo_stroke(cairo_p);

#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

void DrawColorCircle(
	COLOR_CHOOSER* chooser,
	FLOAT_T cx,
	FLOAT_T cy,
	FLOAT_T r
)
{
	cairo_t *cairo_p = cairo_create(chooser->circle_surface);
	HSV hsv = {0, 255, 255};
	uint8 color[3];
	FLOAT_T w;
	FLOAT_T dw = ((FLOAT_T)G_PI * 2.0) / 360.0;
	FLOAT_T next_w;

	cairo_set_line_width(cairo_p, chooser->line_width);

	for(w = -150 * (G_PI/(FLOAT_T)180), next_w = w + dw; hsv.h<360; w = next_w, hsv.h++)
	{
		HSV2RGB_Pixel(&hsv, color);
		next_w += dw;
		cairo_set_source_rgb(cairo_p, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL);
		cairo_arc(cairo_p, cx, cy, r, w, next_w);
		cairo_stroke(cairo_p);
	}

	cairo_destroy(cairo_p);
}

#if GTK_MAJOR_VERSION <= 2
gboolean DrawColorBox(GtkWidget *widget, GdkEventExpose *event, COLOR_CHOOSER* chooser)
#else
gboolean DrawColorBox(GtkWidget *widget, cairo_t* cairo_p, COLOR_CHOOSER* chooser)
#endif
{
#if GTK_MAJOR_VERSION <= 2
	//cairo_t *cairo_p = kaburagi_cairo_create((struct _GdkWindow*)widget->window);
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
#endif

	if(cairo_p == NULL)
	{
		gtk_widget_queue_draw(widget);
		return TRUE;
	}

	cairo_set_source_surface(cairo_p, chooser->color_box_surface, 0, 0);
	cairo_paint(cairo_p);

#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

void ColorBoxButtonPressCalBack(GtkWidget *widget, GdkEventButton *event, COLOR_CHOOSER* chooser)
{
	if(event->x > COLOR_BOX_SIZE * chooser->ui_scale && event->y < (COLOR_BOX_SIZE/2) * chooser->ui_scale)
	{
		uint8 temp_color[3];
		HSV hsv;

		(void)memcpy(temp_color, chooser->back_rgb, 3);
		(void)memcpy(chooser->back_rgb, chooser->rgb, 3);
		(void)memcpy(chooser->rgb, temp_color, 3);

		hsv = chooser->back_hsv;
		chooser->back_hsv = chooser->hsv;
		chooser->hsv = hsv;

		(void)memcpy(temp_color, chooser->rgb, 3);
		RGB2HSV_Pixel(temp_color, &hsv);

		SetColorChooserPoint(chooser, &hsv, FALSE);

		if(chooser->color_change_callback != NULL)
		{
			chooser->color_change_callback(chooser->widget, chooser->rgb, chooser->data);
		}
	}
}

#if GTK_MAJOR_VERSION <= 2
static gboolean DisplayPallete(GtkWidget* widget, GdkEventExpose* expose, COLOR_CHOOSER* chooser)
#else
static gboolean DisplayPallete(GtkWidget* widget, cairo_t* cairo_p, COLOR_CHOOSER* chooser)
#endif
{
#if GTK_MAJOR_VERSION <= 2
	//cairo_t *cairo_p = kaburagi_cairo_create((struct _GdkWindow*)expose->window);
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
#endif
	uint8 *pixels = chooser->pallete_pixels;
	uint8 *ref;
	int stride = (PALLETE_WIDTH*(int)(PALLETE_BOX_SIZE*chooser->ui_scale)+PALLETE_WIDTH+1) * 4;
	int index;
	int i, j, k, l;

	if(cairo_p != NULL)
	{
		guint32 *pixels32 = (guint32*)pixels;
		for(i=0; i<(PALLETE_WIDTH*(int)(PALLETE_BOX_SIZE*chooser->ui_scale)+PALLETE_WIDTH+1)*((int)(PALLETE_BOX_SIZE*chooser->ui_scale)*PALLETE_HEIGHT+PALLETE_HEIGHT+1); i++)
		{
			pixels32[i] = 0xff000000;
		}
	}
	else
	{
		gtk_widget_queue_draw(widget);
		return TRUE;
	}

	for(i=0, index=0; i<PALLETE_HEIGHT; i++)
	{
		for(j=0; j<PALLETE_WIDTH; j++)
		{
			ref = &pixels[(i*((int)(PALLETE_BOX_SIZE*chooser->ui_scale)+1)+1)*stride
				+(j*(((int)(PALLETE_BOX_SIZE*chooser->ui_scale)+1)*4))+4];
			if(FLAG_CHECK(chooser->pallete_flags, index))
			{
				for(k=0; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); k++)
				{
					for(l=0; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); l++)
					{
#if 1 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
						ref[stride*k+l*4+0] = chooser->pallete_data[index][2];
						ref[stride*k+l*4+1] = chooser->pallete_data[index][1];
						ref[stride*k+l*4+2] = chooser->pallete_data[index][0];
#else
						ref[stride*k+l*4+0] = chooser->pallete_data[index][0];
						ref[stride*k+l*4+1] = chooser->pallete_data[index][1];
						ref[stride*k+l*4+2] = chooser->pallete_data[index][2];
#endif
					}
				}
			}
			else
			{
				for(k=0; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; k++)
				{
					for(l=0; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; l++)
					{
						ref[stride*k+l*4+0] = 0xCC;
						ref[stride*k+l*4+1] = 0xCC;
						ref[stride*k+l*4+2] = 0xCC;
					}
				}

				for(k=(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); k++)
				{
					for(l=0; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; l++)
					{
						ref[stride*k+l*4+0] = 0x88;
						ref[stride*k+l*4+1] = 0x88;
						ref[stride*k+l*4+2] = 0x88;
					}
				}

				for(k=0; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; k++)
				{
					for(l=(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); l++)
					{
						ref[stride*k+l*4+0] = 0x88;
						ref[stride*k+l*4+1] = 0x88;
						ref[stride*k+l*4+2] = 0x88;
					}
				}

				for(k=(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); k++)
				{
					for(l=(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); l++)
					{
						ref[stride*k+l*4+0] = 0xCC;
						ref[stride*k+l*4+1] = 0xCC;
						ref[stride*k+l*4+2] = 0xCC;
					}
				}
			}
			index++;
		}
	}

	if(chooser->filter_func != NULL)
	{
		chooser->filter_func(pixels, pixels,
			(PALLETE_WIDTH*(int)(PALLETE_BOX_SIZE*chooser->ui_scale)+PALLETE_WIDTH+1) * ((int)(PALLETE_BOX_SIZE*chooser->ui_scale)*PALLETE_HEIGHT+PALLETE_HEIGHT+1),
				chooser->filter_data);
	}

	cairo_set_source_surface(cairo_p, chooser->pallete_surface, 0, 0);
	cairo_paint(cairo_p);

#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

static void PalleteAddColor(COLOR_CHOOSER* chooser)
{
	(void)memcpy(chooser->pallete_data[chooser->selected_pallete], chooser->rgb, 3);
	FLAG_ON(chooser->pallete_flags, chooser->selected_pallete);
	gtk_widget_queue_draw(chooser->pallete_widget);
}

/***************************************
* RegisterColorPallete関数             *
* パレットに色を追加する               *
* 引数                                 *
* chooser	: 色選択ウィジェットの情報 *
* color		: 追加する色               *
***************************************/
void RegisterColorPallete(COLOR_CHOOSER* chooser, const uint8 color[3])
{
	int i;
	for(i=0; i<PALLETE_WIDTH*PALLETE_HEIGHT; i++)
	{
		if(FLAG_CHECK(chooser->pallete_flags, i) == 0)
		{
			break;
		}
	}

	if(i == PALLETE_WIDTH * PALLETE_HEIGHT)
	{
		return;
	}

	(void)memcpy(chooser->pallete_data[i], color, 3);
	FLAG_ON(chooser->pallete_flags, i);
	gtk_widget_queue_draw(chooser->pallete_widget);
}

static void ExecuteLoadPallete(COLOR_CHOOSER* chooser)
{
	APPLICATION *app = (APPLICATION*)chooser->data;
	GtkWidget *file_chooser = gtk_file_chooser_dialog_new(
		NULL,
		GTK_WINDOW(app->widgets->window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL
	);
	GtkFileFilter *filter = gtk_file_filter_new();

	gtk_file_filter_set_name(filter, "Pallete File");
	gtk_file_filter_add_pattern(filter, "*.aco");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), filter);

	if(gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT)
	{
		const gchar *file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
		char *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		int read_num;
		int i;

		(void)memset(chooser->pallete_flags, 0, ((PALLETE_WIDTH*PALLETE_HEIGHT)+7)/8);

		read_num = LoadPallete(system_path, chooser->pallete_data, (PALLETE_WIDTH*PALLETE_HEIGHT));

		for(i=0; i<read_num; i++)
		{
			FLAG_ON(chooser->pallete_flags, i);
		}

		g_free(system_path);
	}

	gtk_widget_destroy(file_chooser);

	gtk_widget_queue_draw(chooser->pallete_widget);
}

static void ExecuteAddPallete(COLOR_CHOOSER* chooser)
{
	APPLICATION *app = (APPLICATION*)chooser->data;
	GtkWidget *file_chooser = gtk_file_chooser_dialog_new(
		NULL,
		GTK_WINDOW(app->widgets->window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL
	);
	GtkFileFilter *filter = gtk_file_filter_new();

	gtk_file_filter_set_name(filter, "Pallete File");
	gtk_file_filter_add_pattern(filter, "*.aco");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), filter);

	if(gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT)
	{
		const gchar *file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
		int read_num;

		read_num = LoadPalleteAdd(chooser, file_path);
	}

	gtk_widget_destroy(file_chooser);
}

static void ExecuteWritePallete(COLOR_CHOOSER* chooser)
{
	APPLICATION *app = (APPLICATION*)chooser->data;
	GtkWidget *file_chooser = gtk_file_chooser_dialog_new(
		NULL,
		GTK_WINDOW(app->widgets->window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL
	);
	GtkFileFilter *filter = gtk_file_filter_new();

	// 上書きする前に警告を出す
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser), TRUE);
	// ファイルタイプのフィルターをセット
	gtk_file_filter_set_name(filter, "Pallete File");
	gtk_file_filter_add_pattern(filter, "*.aco");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), filter);

	if(gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT)
	{
		GFile *fp;
		GFileOutputStream *stream;
		const gchar* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
		uint8 (*data)[3] = (uint8 (*)[3])MEM_ALLOC_FUNC(sizeof(*data)*(PALLETE_WIDTH*PALLETE_HEIGHT));
		char file_name[4096];
		size_t name_length = strlen(path);
		int num_data = 0;
		int i;

		if(StringCompareIgnoreCase(&path[name_length-4], ".aco") != 0)
		{
			(void)sprintf(file_name, "%s.aco", path);
		}
		else
		{
			(void)strcpy(file_name, path);
		}

		fp = g_file_new_for_path(file_name);
		stream = g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL);

		if(stream == NULL)
		{
			stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);
		}

		if(stream != NULL)
		{
			(void)memset(data, 0, sizeof(*data)*(PALLETE_WIDTH*PALLETE_HEIGHT));
			for(i=0; i<(PALLETE_WIDTH*PALLETE_HEIGHT); i++)
			{
				if(FLAG_CHECK(chooser->pallete_flags, i))
				{
					data[num_data][0] = chooser->pallete_data[i][0];
					data[num_data][1] = chooser->pallete_data[i][1];
					data[num_data][2] = chooser->pallete_data[i][2];
					num_data++;
				}
			}

			WriteACO((void*)stream, (stream_func_t)FileWrite, data, num_data);
			g_object_unref(stream);
		}

		g_object_unref(fp);
		MEM_FREE_FUNC(data);
	}

	gtk_widget_destroy(file_chooser);
}

static void PalleteDeleteColor(COLOR_CHOOSER* chooser)
{
	(void)memset(chooser->pallete_data[chooser->selected_pallete], 0, 3);
	FLAG_OFF(chooser->pallete_flags, chooser->selected_pallete);
	gtk_widget_queue_draw(chooser->pallete_widget);
}

static void ClearPallete(COLOR_CHOOSER* chooser)
{
	int i;

	for(i=0; i<PALLETE_WIDTH * PALLETE_HEIGHT; i++)
	{
		FLAG_OFF(chooser->pallete_flags, i);
	}
	gtk_widget_queue_draw(chooser->pallete_widget);
}

static gboolean PalleteButtonPressCallBack(
	GtkWidget* pallete,
	GdkEventButton *event_info,
	COLOR_CHOOSER* chooser
)
{
	APPLICATION *app = (APPLICATION*)chooser->data;
	int int_x = (int)event_info->x, int_y = (int)event_info->y;
	chooser->selected_pallete =
		(int_y / ((int)(PALLETE_BOX_SIZE*chooser->ui_scale)+1)) * PALLETE_WIDTH + int_x / ((int)(PALLETE_BOX_SIZE*chooser->ui_scale)+1);

	if(event_info->button == 1)
	{
		if(FLAG_CHECK(chooser->pallete_flags, chooser->selected_pallete))
		{
			(void)memcpy(chooser->rgb, chooser->pallete_data[chooser->selected_pallete], 3);
			RGB2HSV_Pixel(chooser->pallete_data[chooser->selected_pallete], &chooser->hsv);
			SetColorChooserPoint(chooser, &chooser->hsv, TRUE);
		}
	}
	else if(event_info->button == 3)
	{
		GtkWidget *menu = gtk_menu_new();
		GtkWidget *menu_item;

		menu_item = gtk_menu_item_new_with_label(
			app->labels->tool_box.pallete_add);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(PalleteAddColor), chooser);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(
			app->labels->tool_box.pallete_delete);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(PalleteDeleteColor), chooser);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(
			app->labels->tool_box.load_pallete);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteLoadPallete), chooser);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(
			app->labels->tool_box.load_pallete_after);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteAddPallete), chooser);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(
			app->labels->tool_box.write_pallete);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteWritePallete), chooser);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(
			app->labels->tool_box.clear_pallete);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ClearPallete), chooser);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
			NULL, event_info->button, event_info->time);

		gtk_widget_show_all(menu);
	}

	return TRUE;
}

static GtkWidget* CreatePallete(COLOR_CHOOSER* chooser)
{
	GtkWidget *pallete_scroll = gtk_scrolled_window_new(NULL, NULL);
	uint8 *ref;
	const int stride = (PALLETE_WIDTH*(int)(PALLETE_BOX_SIZE*chooser->ui_scale)+PALLETE_WIDTH+1) * 4;
	int index;
	int i, j, k, l;

	chooser->pallete_pixels = (uint8*)MEM_ALLOC_FUNC(stride * ((int)(PALLETE_BOX_SIZE*chooser->ui_scale)*PALLETE_HEIGHT+PALLETE_HEIGHT+1));
	(void)memset(chooser->pallete_pixels, 0xff, stride * ((int)(PALLETE_BOX_SIZE*chooser->ui_scale)*PALLETE_HEIGHT+PALLETE_HEIGHT+1));
	chooser->pallete_surface = cairo_image_surface_create_for_data(
		chooser->pallete_pixels, CAIRO_FORMAT_ARGB32, PALLETE_WIDTH*(int)(PALLETE_BOX_SIZE*chooser->ui_scale)+PALLETE_WIDTH+1,
			(int)(PALLETE_BOX_SIZE*chooser->ui_scale)*PALLETE_HEIGHT+PALLETE_HEIGHT+1, stride);

	for(i=0, index=0; i<PALLETE_HEIGHT; i++)
	{
		for(j=0; j<PALLETE_WIDTH; j++)
		{
			ref = &chooser->pallete_pixels[(i*((int)(PALLETE_BOX_SIZE*chooser->ui_scale)+1)+1)*stride
				+(j*(((int)(PALLETE_BOX_SIZE*chooser->ui_scale)+1)*4))+4];
			if(FLAG_CHECK(chooser->pallete_flags, index))
			{
				for(k=0; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); k++)
				{
					for(l=0; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); l++)
					{
#if 0 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
						ref[stride*k+l*4+0] = chooser->pallete_data[index][2];
						ref[stride*k+l*4+1] = chooser->pallete_data[index][1];
						ref[stride*k+l*4+2] = chooser->pallete_data[index][0];
#else
						ref[stride*k+l*4+0] = chooser->pallete_data[index][0];
						ref[stride*k+l*4+1] = chooser->pallete_data[index][1];
						ref[stride*k+l*4+2] = chooser->pallete_data[index][2];
#endif
					}
				}
			}
			else
			{
				for(k=0; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; k++)
				{
					for(l=0; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; l++)
					{
						ref[stride*k+l*4+0] = 0xCC;
						ref[stride*k+l*4+1] = 0xCC;
						ref[stride*k+l*4+2] = 0xCC;
					}
				}

				for(k=(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); k++)
				{
					for(l=0; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; l++)
					{
						ref[stride*k+l*4+0] = 0x88;
						ref[stride*k+l*4+1] = 0x88;
						ref[stride*k+l*4+2] = 0x88;
					}
				}

				for(k=0; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; k++)
				{
					for(l=(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); l++)
					{
						ref[stride*k+l*4+0] = 0x88;
						ref[stride*k+l*4+1] = 0x88;
						ref[stride*k+l*4+2] = 0x88;
					}
				}

				for(k=(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; k<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); k++)
				{
					for(l=(int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2; l<(int)(PALLETE_BOX_SIZE*chooser->ui_scale); l++)
					{
						ref[stride*k+l*4+0] = 0xCC;
						ref[stride*k+l*4+1] = 0xCC;
						ref[stride*k+l*4+2] = 0xCC;
					}
				}
			}
			index++;
		}
	}

	chooser->pallete_widget = gtk_drawing_area_new();
	gtk_widget_set_events(chooser->pallete_widget,
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
	gtk_widget_set_size_request(chooser->pallete_widget,
		PALLETE_WIDTH*(int)(PALLETE_BOX_SIZE*chooser->ui_scale)+PALLETE_WIDTH+1, (int)(PALLETE_BOX_SIZE*chooser->ui_scale)*PALLETE_HEIGHT+PALLETE_HEIGHT+1);
	gtk_container_set_resize_mode(GTK_CONTAINER(pallete_scroll), GTK_RESIZE_QUEUE);
	
	(void)g_signal_connect(G_OBJECT(chooser->pallete_widget), "button_press_event",
		G_CALLBACK(PalleteButtonPressCallBack), chooser);
#if GTK_MAJOR_VERSION <= 2
	(void)g_signal_connect(G_OBJECT(chooser->pallete_widget), "expose_event",
		G_CALLBACK(DisplayPallete), chooser);
#else
	(void)g_signal_connect(G_OBJECT(chooser->pallete_widget), "draw",
		G_CALLBACK(DisplayPallete), chooser);
#endif

	gtk_widget_set_size_request(pallete_scroll,
		PALLETE_WIDTH * (int)(PALLETE_BOX_SIZE*chooser->ui_scale) + PALLETE_WIDTH + 1,
		((int)(PALLETE_BOX_SIZE*chooser->ui_scale)/2) * 13);

	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(pallete_scroll), chooser->pallete_widget);
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(pallete_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	return pallete_scroll;
}

static void ColorChooserDetailSelect(GtkWidget* button, COLOR_CHOOSER* chooser)
{
	APPLICATION *app = (APPLICATION*)chooser->data;
	GtkWidget *dialog = gtk_color_selection_dialog_new("");
#if 0 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
	GdkColor color = {0, (chooser->rgb[2] << 8) | chooser->rgb[2],
		(chooser->rgb[1] << 8) | chooser->rgb[1], (chooser->rgb[0] << 8) | chooser->rgb[0]};
#else
	GdkColor color = {0, (chooser->rgb[0] << 8) | chooser->rgb[0],
		(chooser->rgb[1] << 8) | chooser->rgb[1], (chooser->rgb[2] << 8) | chooser->rgb[2]};
#endif
	GtkWidget *selection = gtk_color_selection_dialog_get_color_selection(
		GTK_COLOR_SELECTION_DIALOG(dialog));
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(selection), &color);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->widgets->window));
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(selection), &color);
#if 0 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
		chooser->rgb[2] = color.red >> 8;
		chooser->rgb[1] = color.green >> 8;
		chooser->rgb[0] = color.blue >> 8;
#else
		chooser->rgb[0] = color.red >> 8;
		chooser->rgb[1] = color.green >> 8;
		chooser->rgb[2] = color.blue >> 8;
#endif
		RGB2HSV_Pixel(chooser->rgb, &chooser->hsv);
		SetColorChooserPoint(chooser, &chooser->hsv, TRUE);
	}

	gtk_widget_destroy(dialog);
}

static void ChangeColorChooserWidgetVisible(GtkWidget* button, GtkWidget* change_widget)
{
	COLOR_CHOOSER *chooser = (COLOR_CHOOSER*)g_object_get_data(G_OBJECT(button), "color_chooser");
	int flag = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "flag_value"));

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		chooser->flags &= ~(flag);
		gtk_widget_hide(change_widget);
	}
	else
	{
		chooser->flags |= flag;
		gtk_widget_show_all(change_widget);
	}
}

COLOR_CHOOSER *CreateColorChooser(
	int32 width,
	int32 height,
	uint8 line_width,
	int32 start_rad,
	uint8 (*pallete)[3],
	uint8 *pallete_use,
	const gchar* base_path,
	void* application_labels,
	FLOAT_T ui_scale
)
{
	// 返り値
	COLOR_CHOOSER* ret =
		(COLOR_CHOOSER*)MEM_ALLOC_FUNC(sizeof(*ret));
	// UIに使用する文字列
	APPLICATION_LABELS *labels = (APPLICATION_LABELS*)application_labels;
	// 中心の選択領域の色
	HSV hsv = {(int16)(start_rad + 180), 255, 255};
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox;
	// ウィジェット選択表示・非表示切り替えウィジェット
	GtkWidget *button;
	GtkWidget *button_image;
	// ボタンに表示する画像
	GdkPixbuf *button_pixbuf;
	// 画像リサイズ用
	GdkPixbuf *old_pixbuf;
	// 中心の選択領域のサイズ
	int box_size = (width + line_width) / 2;
	// 1列分の画像データサイズ
	int stride = width * 4;
	// 画像ファイルへのパス
	gchar *image_file_path;

	// 0初期化
	(void)memset(ret, 0, sizeof(*ret));

	// 値のセット
	ret->widget_width = width;
	ret->widget_height = height;
	ret->line_width = line_width;
	ret->pallete_data = pallete;
	ret->pallete_flags = pallete_use;
	ret->ui_scale = ui_scale;

	// カラーサークルを描画
		// 描画結果を記憶しておく
	ret->color_circle_data = (uint8*)MEM_ALLOC_FUNC(
		stride * ret->widget_height);
	(void)memset(ret->color_circle_data, 0, stride * ret->widget_height);
	ret->circle_surface = cairo_image_surface_create_for_data(
		ret->color_circle_data, CAIRO_FORMAT_ARGB32, width, height, width*4);
	DrawColorCircle(ret, width/2, height/2, (width-line_width)/2);

	// ウィジェット表示用のピクセルメモリを確保
	ret->chooser_pixel_data = (uint8*)MEM_ALLOC_FUNC(
		stride * ret->widget_height);
	(void)memset(ret->chooser_pixel_data, 0, stride * ret->widget_height);
	ret->chooser_surface = cairo_image_surface_create_for_data(
		ret->chooser_pixel_data, CAIRO_FORMAT_ARGB32, width, height, stride);
	ret->chooser_cairo = cairo_create(ret->chooser_surface);

	// 明度、彩度の座標を記憶
	ret->sv_x = (width-box_size)/2;
	ret->sv_y = (height-box_size)/2;
	
	// WindowsならRGB→BGR
#if 0 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
	{
		uint8 r = ret->rgb[0];
		ret->rgb[0] = ret->rgb[2];
		ret->rgb[2] = r;
	}
#endif

	// HSV情報を記憶
	RGB2HSV_Pixel(ret->rgb, &ret->hsv);

	// 背景色を設定
	(void)memcpy(ret->back_rgb, ret->rgb, 3);
	// 背景色のHSVを設定
	RGB2HSV_Pixel(ret->rgb, &ret->back_hsv);

	// 色選択ウィジェットを生成
	ret->choose_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(ret->choose_area, width, height);

	// マウスクリックのイベントを追加
	(void)g_signal_connect(G_OBJECT(ret->choose_area), "button_press_event",
		G_CALLBACK(OnColorChooserClicked), ret);
	(void)g_signal_connect(G_OBJECT(ret->choose_area), "motion-notify-event",
		G_CALLBACK(OnColorChooserMotion), ret);
	(void)g_signal_connect(G_OBJECT(ret->choose_area), "button-release-event",
		G_CALLBACK(OnColorChooserButtonReleased), ret);
#if GTK_MAJOR_VERSION <= 2
	(void)g_signal_connect(G_OBJECT(ret->choose_area), "expose-event",
		G_CALLBACK(DisplayColorChooser), ret);
#else
	(void)g_signal_connect(G_OBJECT(ret->choose_area), "draw",
		G_CALLBACK(DisplayColorChooser), ret);
#endif
	gtk_widget_add_events(ret->choose_area,
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);

	// 色選択ウィジェットと色表示ウィジェットを横並びにする
	ret->widget = gtk_hbox_new(FALSE, 0);
	ret->chooser_box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ret->chooser_box), ret->choose_area, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ret->widget), ret->chooser_box, TRUE, TRUE, 0);

	// パレットを作成
	ret->pallete = CreatePallete(ret);
	gtk_widget_add_events(ret->pallete, GDK_BUTTON_PRESS_MASK);

	// 色表示ウィジェットを作成
	{
		int size = (ret->line_width * COLOR_BOX_SIZE) / COLOR_CIRCLE_WIDTH;
		uint8 *pixels = ret->color_box_pixel_data =
			(uint8*)MEM_ALLOC_FUNC(((size*3)/2)*((size*3)/2)*4);
		int stride = ((size*3)/2)*4;

		ret->color_box_surface = cairo_image_surface_create_for_data(pixels,
			CAIRO_FORMAT_ARGB32, (size*3)/2, (size*3)/2, ((size*3)/2)*4);

		UpdateColorChooserBoxPixel(ret);
	}
	ret->color_box = gtk_drawing_area_new();

#if GTK_MAJOR_VERSION <= 2
	gtk_drawing_area_size(GTK_DRAWING_AREA(ret->color_box),
		(int)(ui_scale* ((COLOR_BOX_SIZE*3)/2)), (int)(ui_scale * ((COLOR_BOX_SIZE*3)/2)));
#else
	gtk_widget_set_size_request(ret->color_box, (COLOR_BOX_SIZE*3)/2, (COLOR_BOX_SIZE*3)/2);
#endif
	gtk_widget_add_events(ret->color_box, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
#if GTK_MAJOR_VERSION <= 2
	(void)g_signal_connect(G_OBJECT(ret->color_box), "expose_event",
		G_CALLBACK(DrawColorBox), ret);
#else
	(void)g_signal_connect(G_OBJECT(ret->color_box), "draw",
		G_CALLBACK(DrawColorBox), ret);
#endif
	(void)g_signal_connect(G_OBJECT(ret->color_box), "button_press_event",
		G_CALLBACK(ColorBoxButtonPressCalBack), ret);

	gtk_box_pack_end(GTK_BOX(vbox), ret->color_box, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(ret->widget), vbox, FALSE, FALSE, 3);

	image_file_path = g_build_filename(base_path, "image/circle.png", NULL);
	button_pixbuf = gdk_pixbuf_new_from_file(image_file_path, NULL);
	if(button_pixbuf != NULL && ui_scale != 1.0)
	{
		old_pixbuf = button_pixbuf;
		button_pixbuf = gdk_pixbuf_scale_simple(old_pixbuf, (int)(ui_scale * gdk_pixbuf_get_width(old_pixbuf)),
			(int)(ui_scale * gdk_pixbuf_get_height(old_pixbuf)), GDK_INTERP_NEAREST);
		g_object_unref(old_pixbuf);
	}
	button_image = gtk_image_new_from_pixbuf(button_pixbuf);
	ret->circle_button = button = gtk_toggle_button_new();
	gtk_container_add(GTK_CONTAINER(button), button_image);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
	g_object_set_data(G_OBJECT(button), "color_chooser", ret);
	g_object_set_data(G_OBJECT(button), "flag_value", GINT_TO_POINTER(COLOR_CHOOSER_SHOW_CIRCLE));
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(ChangeColorChooserWidgetVisible), ret->choose_area);
	g_free(image_file_path);

	image_file_path = g_build_filename(base_path, "image/pallete.png", NULL);
	button_pixbuf = gdk_pixbuf_new_from_file(image_file_path, NULL);
	if(button_pixbuf != NULL && ui_scale != 1.0)
	{
		old_pixbuf = button_pixbuf;
		button_pixbuf = gdk_pixbuf_scale_simple(old_pixbuf, (int)(ui_scale * gdk_pixbuf_get_width(old_pixbuf)),
			(int)(ui_scale * gdk_pixbuf_get_height(old_pixbuf)), GDK_INTERP_NEAREST);
		g_object_unref(old_pixbuf);
	}
	button_image = gtk_image_new_from_pixbuf(button_pixbuf);
	ret->pallete_button = button = gtk_toggle_button_new();
	gtk_container_add(GTK_CONTAINER(button), button_image);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
	g_object_set_data(G_OBJECT(button), "color_chooser", ret);
	g_object_set_data(G_OBJECT(button), "flag_value", GINT_TO_POINTER(COLOR_CHOOSER_SHOW_PALLETE));
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(ChangeColorChooserWidgetVisible), ret->pallete);
	g_free(image_file_path);

	button = gtk_button_new_with_label(labels->unit.detail);
	(void)g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(ColorChooserDetailSelect), ret);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), ret->widget, FALSE, TRUE, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), ret->pallete, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	(void)g_signal_connect_swapped(G_OBJECT(vbox), "destroy",
		G_CALLBACK(DestroyColorChooser), ret);
	ret->widget = vbox;

	return ret;
}

void DestroyColorChooser(COLOR_CHOOSER* chooser)
{
	MEM_FREE_FUNC(chooser->color_circle_data);
	cairo_surface_destroy(chooser->circle_surface);
	cairo_destroy(chooser->chooser_cairo);
	cairo_surface_destroy(chooser->chooser_surface);
	MEM_FREE_FUNC(chooser->chooser_pixel_data);
	cairo_surface_destroy(chooser->color_box_surface);
	MEM_FREE_FUNC(chooser->color_box_pixel_data);
	cairo_surface_destroy(chooser->pallete_surface);
	MEM_FREE_FUNC(chooser->pallete_pixels);
}

/*********************************************************
* SetColorChangeCallBack関数                             *
* 色が変更されたときに呼び出されるコールバック関数を設定 *
* 引数                                                   *
* chooser	: 色選択用のデータ                           *
* function	: コールバック関数                           *
* data		: コールバック関数で使用するデータ           *
*********************************************************/
void SetColorChangeCallBack(
	COLOR_CHOOSER* chooser,
	void (*function)(GtkWidget* widget, const uint8 color[3], void* data),
	void *data
)
{
	chooser->color_change_callback = function;
	chooser->data = data;
}

/*************************************
* LoadPalleteFile関数                *
* パレットの情報を読み込む           *
* chooser	: 色選択用の情報         *
* file_path	: 読み込むファイルのパス *
* 返り値                             *
*	正常終了:0	失敗:負の値          *
*************************************/
int LoadPalleteFile(COLOR_CHOOSER* chooser, const gchar* file_path)
{
	GFile *fp = g_file_new_for_path(file_path);
	GFileInputStream *stream = g_file_read(fp, NULL, NULL);
	char c;
	int i;

	if(stream == NULL)
	{
		g_object_unref(fp);
		return -1;
	}

	for(i=0; i<(PALLETE_WIDTH*PALLETE_HEIGHT); i++)
	{
		if(FileRead(&c, 1, 1, stream) == 0)
		{
			return -1;
		}

		if(c != 0)
		{
			FLAG_ON(chooser->pallete_flags, i);
			if(FileRead(chooser->pallete_data[i], 1, 3, stream) < 3)
			{
				return -1;
			}
		}
		else
		{
			FLAG_OFF(chooser->pallete_flags, i);
		}
	}

	gtk_widget_queue_draw(chooser->pallete_widget);

	g_object_unref(stream);
	g_object_unref(fp);

	return 0;
}

/***********************************
* LoadPalleteAdd関数               *
* パレットデータを追加読み込み     *
* 引数                             *
* chooser	: 色選択の情報         *
* file_path	: データファイルのパス *
* 返り値                           *
*	読み込んだデータの数           *
***********************************/
int LoadPalleteAdd(
	COLOR_CHOOSER* chooser,
	const gchar* file_path
)
{
	uint8 (*data)[3];
	int add_num = 0;
	int read_num;
	int index;
	int i;

	data = (uint8 (*)[3])MEM_ALLOC_FUNC(sizeof(*data)*(PALLETE_WIDTH*PALLETE_HEIGHT));
	(void)memset(data, 0, sizeof(*data)*(PALLETE_WIDTH*PALLETE_HEIGHT));

	read_num = LoadPallete(file_path, data, (PALLETE_WIDTH*PALLETE_HEIGHT));

	for(i=0, index=0; i<read_num; i++)
	{
		while(FLAG_CHECK(chooser->pallete_flags, index) != 0 && index < (PALLETE_WIDTH*PALLETE_HEIGHT))
		{
			index++;
		}

		if(index == (PALLETE_WIDTH*PALLETE_HEIGHT))
		{
			MEM_FREE_FUNC(data);
			gtk_widget_queue_draw(chooser->pallete_widget);

			return add_num;
		}
		else
		{
			FLAG_ON(chooser->pallete_flags, index);
			chooser->pallete_data[index][0] = data[i][0];
			chooser->pallete_data[index][1] = data[i][1];
			chooser->pallete_data[index][2] = data[i][2];
			read_num++;
			index++;
		}
	}

	MEM_FREE_FUNC(data);

	gtk_widget_queue_draw(chooser->pallete_widget);

	return read_num;
}

/*************************************
* WritePalleteFile関数               *
* パレットの情報を書き込む           *
* chooser	: 色選択用の情報         *
* file_path	: 書き込むファイルのパス *
* 返り値                             *
*	正常終了:0	失敗:負の値          *
*************************************/
int WritePalleteFile(COLOR_CHOOSER* chooser, const gchar* file_path)
{
	FILE* fp;
	gchar *system_path;
	uint8 rgb[3];
	char c;
	int i;

	system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
	fp = fopen(system_path, "wb");
	g_free(system_path);

	if(fp == NULL)
	{
		return -1;
	}

	for(i=0; i<(PALLETE_WIDTH*PALLETE_HEIGHT); i++)
	{
		if(FLAG_CHECK(chooser->pallete_flags, i))
		{
			c = 1;
			(void)fwrite(&c, 1, 1, fp);
			rgb[0] = chooser->pallete_data[i][0];
			rgb[1] = chooser->pallete_data[i][1];
			rgb[2] = chooser->pallete_data[i][2];
			(void)fwrite(rgb, 1, 3, fp);
		}
		else
		{
			c = 0;
			(void)fwrite(&c, 1, 1, fp);
		}
	}

	(void)fclose(fp);

	return 0;
}

#ifdef __cplusplus
}
#endif
