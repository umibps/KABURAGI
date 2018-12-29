// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <ctype.h>
#include <math.h>
#include "application.h"
#include "text_layer.h"
#include "widgets.h"
#include "utils.h"
#include "bezier.h"
#include "memory.h"

#if !defined(USE_QT) || (defined(USE_QT) && USE_QT != 0)
# include "gui/GTK/utils_gtk.h"
# include "gui/GTK/gtk_widgets.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_ARROW_WIDTH (G_PI/12)

/*********************************************
* CreateTextLayer関数                        *
* テキストレイヤーのデータメモリ確保と初期化 *
* 引数                                       *
* window		: キャンバスの情報           *
* x				: レイヤーのX座標            *
* y				: レイヤーのY座標            *
* width			: レイヤーの幅               *
* height		: レイヤーの高さ             *
* base_size		: 文字サイズの倍率           *
* font_size		: 文字サイズ                 *
* color			: 文字色                     *
* balloon_type	: 吹き出しのタイプ           *
* back_color	: 吹き出しの背景色           *
* line_color	: 吹き出しの線の色           *
* line_width	: 吹き出しの線の太さ         *
* balloon_data	: 吹き出し形状の詳細データ   *
* flags			: テキスト表示のフラグ       *
* 返り値                                     *
*	初期化された構造体のアドレス             *
*********************************************/
TEXT_LAYER* CreateTextLayer(
	void* window,
	gdouble x,
	gdouble y,
	gdouble width,
	gdouble height,
	int base_size,
	gdouble font_size,
	int32 font_id,
	const uint8 color[3],
	uint16 balloon_type,
	const uint8 back_color[4],
	const uint8 line_color[4],
	gdouble line_width,
	TEXT_LAYER_BALLOON_DATA* balloon_data,
	uint32 flags
)
{
	TEXT_LAYER *ret = (TEXT_LAYER*)MEM_ALLOC_FUNC(sizeof(*ret));
	DRAW_WINDOW *canvas = (DRAW_WINDOW*)window;
	gdouble center_x, center_y;
	gdouble angle;
	FLOAT_T edge_point[2];

	(void)memset(ret, 0, sizeof(*ret));

	ret->x = x;
	ret->y = y;
	ret->width = width;
	ret->height = height;
	ret->base_size = base_size;
	ret->font_size = font_size;
	ret->font_id = font_id;
	if(canvas != NULL)
	{
		ret->edge_position[0][0] = canvas->width * 0.5;
		ret->edge_position[0][1] = canvas->height * 0.5;
	}
	(void)memcpy(ret->color, color, 3);
	(void)memcpy(ret->back_color, back_color, 4);
	(void)memcpy(ret->line_color, line_color, 4);
	ret->balloon_type = balloon_type;
	if(balloon_data != NULL)
	{
		ret->balloon_data = *balloon_data;
	}
	ret->line_width = line_width;
	ret->flags = flags;

	center_x = x + width * 0.5;
	center_y = y + height * 0.5;
	angle = atan2(center_y - ret->edge_position[0][1], center_x - ret->edge_position[0][0]) + G_PI;

	ret->arc_start = angle + DEFAULT_ARROW_WIDTH;
	ret->arc_end = angle - DEFAULT_ARROW_WIDTH + 2 * G_PI;

	TextLayerGetBalloonArcPoint(ret, ret->arc_start, edge_point);
	ret->edge_position[1][0] = (edge_point[0] + ret->edge_position[0][0]) * 0.5;
	ret->edge_position[1][1] = (edge_point[1] + ret->edge_position[0][1]) * 0.5;

	TextLayerGetBalloonArcPoint(ret, ret->arc_end, edge_point);
	ret->edge_position[2][0] = (edge_point[0] + ret->edge_position[0][0]) * 0.5;
	ret->edge_position[2][1] = (edge_point[1] + ret->edge_position[0][1]) * 0.5;

	return ret;
}

/*****************************************************
* DeleteTextLayer関数                                *
* テキストレイヤーのデータを削除する                 *
* 引数                                               *
* layer	: テキストレイヤーのデータポインタのアドレス *
*****************************************************/
void DeleteTextLayer(TEXT_LAYER** layer)
{
	MEM_FREE_FUNC((*layer)->drag_points);
	MEM_FREE_FUNC((*layer)->text);
	MEM_FREE_FUNC(*layer);

	*layer = NULL;
}

void TextLayerButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkEventButton* event_info
)
{
	if(event_info->button == 1)
	{
		TEXT_LAYER* layer = window->active_layer->layer_data.text_layer_p;
		gdouble points[4][2];
		gdouble compare = TEXT_LAYER_SELECT_POINT_DISTANCE * window->rev_zoom;
		int i;

		points[0][0] = layer->x;
		points[0][1] = layer->y;
		points[1][0] = layer->x;
		points[1][1] = layer->y + layer->height;
		points[2][0] = layer->x + layer->width;
		points[2][1] = layer->y + layer->height;
		points[3][0] = layer->x + layer->width;
		points[3][1] = layer->y;

		for(i=0; i<4; i++)
		{
			if(fabs(points[i][0] - x) <= compare
				&& fabs(points[i][1] - y) <= compare)
			{
				layer->drag_points = (TEXT_LAYER_POINTS*)MEM_REALLOC_FUNC(
					layer->drag_points, sizeof(*layer->drag_points));
				layer->drag_points->point_index = i;
				(void)memcpy(layer->drag_points->points, points, sizeof(points));
				return;
			}
		}

		if(layer->back_color[3] != 0 ||
			(layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
		{
			gdouble diff_x, diff_y;
			gdouble center_x, center_y;
			FLOAT_T edge_position[2];

			diff_x = layer->edge_position[0][0] - x;
			diff_y = layer->edge_position[0][1] - y;

			center_x = layer->x + layer->width * 0.5;
			center_y = layer->y + layer->height * 0.5;

			if(diff_x*diff_x + diff_y*diff_y < TEXT_LAYER_SELECT_POINT_DISTANCE * TEXT_LAYER_SELECT_POINT_DISTANCE)
			{
				layer->drag_points = (TEXT_LAYER_POINTS*)MEM_REALLOC_FUNC(
					layer->drag_points, sizeof(*layer->drag_points));
				layer->drag_points->point_index = 4;
				(void)memcpy(layer->drag_points->points, points, sizeof(points));
				return;
			}

			TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_position);
			diff_x = edge_position[0] - x;
			diff_y = edge_position[1] - y;
			if(diff_x*diff_x + diff_y*diff_y < TEXT_LAYER_SELECT_POINT_DISTANCE * TEXT_LAYER_SELECT_POINT_DISTANCE)
			{
				layer->drag_points = (TEXT_LAYER_POINTS*)MEM_REALLOC_FUNC(
					layer->drag_points, sizeof(*layer->drag_points));
				layer->drag_points->point_index = 5;
				(void)memcpy(layer->drag_points->points, points, sizeof(points));
				return;
			}

			TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_position);
			diff_x = edge_position[0] - x;
			diff_y = edge_position[1] - y;
			if(diff_x*diff_x + diff_y*diff_y < TEXT_LAYER_SELECT_POINT_DISTANCE * TEXT_LAYER_SELECT_POINT_DISTANCE)
			{
				layer->drag_points = (TEXT_LAYER_POINTS*)MEM_REALLOC_FUNC(
					layer->drag_points, sizeof(*layer->drag_points));
				layer->drag_points->point_index = 6;
				(void)memcpy(layer->drag_points->points, points, sizeof(points));
				return;
			}

			diff_x = layer->edge_position[1][0] - x;
			diff_y = layer->edge_position[1][1] - y;
			if(diff_x*diff_x + diff_y*diff_y < TEXT_LAYER_SELECT_POINT_DISTANCE * TEXT_LAYER_SELECT_POINT_DISTANCE)
			{
				layer->drag_points = (TEXT_LAYER_POINTS*)MEM_REALLOC_FUNC(
					layer->drag_points, sizeof(*layer->drag_points));
				layer->drag_points->point_index = 7;
				(void)memcpy(layer->drag_points->points, points, sizeof(points));
				return;
			}

			diff_x = layer->edge_position[2][0] - x;
			diff_y = layer->edge_position[2][1] - y;
			if(diff_x*diff_x + diff_y*diff_y < TEXT_LAYER_SELECT_POINT_DISTANCE * TEXT_LAYER_SELECT_POINT_DISTANCE)
			{
				layer->drag_points = (TEXT_LAYER_POINTS*)MEM_REALLOC_FUNC(
					layer->drag_points, sizeof(*layer->drag_points));
				layer->drag_points->point_index = 8;
				(void)memcpy(layer->drag_points->points, points, sizeof(points));
				return;
			}
		}
	}
}

void TextLayerMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkModifierType state
)
{
	TEXT_LAYER* layer = window->active_layer->layer_data.text_layer_p;

	if((state & GDK_BUTTON1_MASK) != 0 && layer->drag_points != NULL)
	{
		gdouble x_min, y_min, x_max, y_max;
		FLOAT_T edge_position[2];
		int i;

		layer->drag_points->points[layer->drag_points->point_index][0] = x;
		layer->drag_points->points[layer->drag_points->point_index][1] = y;

		switch(layer->drag_points->point_index)
		{
		case 0:
			layer->drag_points->points[1][0] = x;
			layer->drag_points->points[3][1] = y;
			break;
		case 1:
			layer->drag_points->points[0][0] = x;
			layer->drag_points->points[2][1] = y;
			break;
		case 2:
			layer->drag_points->points[3][0] = x;
			layer->drag_points->points[1][1] = y;
			break;
		case 3:
			layer->drag_points->points[2][0] = x;
			layer->drag_points->points[0][1] = y;
			break;
		}

		x_min = x_max = layer->drag_points->points[0][0];
		y_min = y_max = layer->drag_points->points[0][1];
		for(i=1; i<4; i++)
		{
			if(x_min > layer->drag_points->points[i][0])
			{
				x_min = layer->drag_points->points[i][0];
			}
			if(x_max < layer->drag_points->points[i][0])
			{
				x_max = layer->drag_points->points[i][0];
			}
			if(y_min > layer->drag_points->points[i][1])
			{
				y_min = layer->drag_points->points[i][1];
			}
			if(y_max < layer->drag_points->points[i][1])
			{
				y_max = layer->drag_points->points[i][1];
			}
		}

		if(x_min < 0)
		{
			x_min = 0;
		}
		else if(x_min > window->width)
		{
			x_min = window->width;
		}
		if(x_max < 0)
		{
			x_max = 0;
		}
		else if(x_max > window->width)
		{
			x_max = window->width;
		}
		if(y_min < 0)
		{
			y_min = 0;
		}
		else if(y_min > window->height)
		{
			y_min = window->height;
		}
		if(y_max < 0)
		{
			y_max = 0;
		}
		else if(y_max > window->height)
		{
			y_max = window->height;
		}

		switch(layer->drag_points->point_index)
		{
		case 4:
			{
				gdouble center_x,	center_y;
				gdouble angle;

				layer->edge_position[0][0] = x;
				layer->edge_position[0][1] = y;

				center_x = layer->x + layer->width * 0.5;
				center_y = layer->y + layer->height * 0.5;
				angle = atan2(center_y - layer->edge_position[0][1], center_x - layer->edge_position[0][0]) + G_PI;

				layer->arc_start = angle + DEFAULT_ARROW_WIDTH;
				layer->arc_end = angle - DEFAULT_ARROW_WIDTH + 2 * G_PI;

				TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_position);
				layer->edge_position[1][0] = (edge_position[0] + layer->edge_position[0][0]) * 0.5;
				layer->edge_position[1][1] = (edge_position[1] + layer->edge_position[0][1]) * 0.5;

				TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_position);
				layer->edge_position[2][0] = (edge_position[0] + layer->edge_position[0][0]) * 0.5;
				layer->edge_position[2][1] = (edge_position[1] + layer->edge_position[0][1]) * 0.5;
			}
			break;
		case 5:
			{
				gdouble center_x,	center_y;
				gdouble angle;

				center_x = layer->x + layer->width * 0.5;
				center_y = layer->y + layer->height * 0.5;
				angle = atan2(center_y - y, center_x - x) + G_PI;
				layer->arc_start = angle;

				TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_position);
				layer->edge_position[1][0] = (edge_position[0] + layer->edge_position[0][0]) * 0.5;
				layer->edge_position[1][1] = (edge_position[1] + layer->edge_position[0][1]) * 0.5;

				TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_position);
				layer->edge_position[2][0] = (edge_position[0] + layer->edge_position[0][0]) * 0.5;
				layer->edge_position[2][1] = (edge_position[1] + layer->edge_position[0][1]) * 0.5;
			}
			break;
		case 6:
			{
				gdouble center_x,	center_y;
				gdouble angle;

				center_x = layer->x + layer->width * 0.5;
				center_y = layer->y + layer->height * 0.5;
				angle = atan2(center_y - y, center_x - x) + G_PI;
				layer->arc_end = angle;

				TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_position);
				layer->edge_position[1][0] = (edge_position[0] + layer->edge_position[0][0]) * 0.5;
				layer->edge_position[1][1] = (edge_position[1] + layer->edge_position[0][1]) * 0.5;

				TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_position);
				layer->edge_position[2][0] = (edge_position[0] + layer->edge_position[0][0]) * 0.5;
				layer->edge_position[2][1] = (edge_position[1] + layer->edge_position[0][1]) * 0.5;
			}
			break;
		case 7:
			layer->edge_position[1][0] = x,	layer->edge_position[1][1] = y;
			break;
		case 8:
			layer->edge_position[2][0] = x,	layer->edge_position[2][1] = y;
			break;
		}

		layer->x = x_min;
		layer->y = y_min;
		layer->width = x_max - x_min;
		layer->height = y_max - y_min;

		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
}

void TextLayerButtonReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkEventButton* event_info
)
{
	TEXT_LAYER* layer = window->active_layer->layer_data.text_layer_p;

	if(event_info->button == 1)
	{
		MEM_FREE_FUNC(layer->drag_points);
		layer->drag_points = NULL;
	}
}

/*********************************************************************
* CreateBalloonDetailSettingWidget関数                               *
* 吹き出しの詳細形状を設定するウィジェットを作成する                 *
* 引数                                                               *
* data					: 詳細設定データ                             *
* widgets				: 詳細設定用のウィジェット群                 *
* setting_callback		: 設定変更時のコールバック関数の関数ポインタ *
* setting_callback_data	: 設定変更時のコールバック関数に使うデータ   *
* application			: アプリケーションを管理するデータ           *
* 返り値                                                             *
*	詳細設定用ウィジェット                                           *
*********************************************************************/
GtkWidget* CreateBalloonDetailSettinWidget(
	TEXT_LAYER_BALLOON_DATA* data,
	TEXT_LAYER_BALLOON_DATA_WIDGETS* widgets,
	void (*setting_callback)(void*),
	void* setting_callback_data,
	void* application
)
{
	APPLICATION *app = (APPLICATION*)application;
	GtkAdjustment *adjustment;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *control;
	char str[256];

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", app->labels->tool_box.num_edge);
	widgets->num_edge = adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		data->num_edge, 1, 0xffff, 1, 1, 0));
	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	control = gtk_spin_button_new(adjustment, 1, 0);
	SetAdjustmentChangeValueCallBack(adjustment,
		setting_callback, setting_callback_data);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(AdjustmentChangeValueCallBackUint16), &data->num_edge);
	gtk_box_pack_start(GTK_BOX(hbox), control, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	widgets->edge_size = adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		data->edge_size * 100, 0.1, 100, 1, 1, 0));
	control = SpinScaleNew(adjustment, app->labels->tool_box.edge_size, 1);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(AdjustmentChangeValueCallBackDoubleRate), &data->edge_size);
	SetAdjustmentChangeValueCallBack(adjustment,
		setting_callback, setting_callback_data);
	gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", app->labels->tool_box.rand_seed);
	widgets->random_seed = adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		data->random_seed, 1, INT_MAX, 1, 1, 0));
	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	control = gtk_spin_button_new(adjustment, 1, 0);
	SetAdjustmentChangeValueCallBack(adjustment,
		setting_callback, setting_callback_data);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(AdjustmentChangeValueCallBackInt32), &data->random_seed);
	gtk_box_pack_start(GTK_BOX(hbox), control, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	widgets->edge_random_size = adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		data->edge_random_size * 100, 0, 200, 1, 1, 0));
	control = SpinScaleNew(adjustment, app->labels->tool_box.random_edge_size, 1);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(AdjustmentChangeValueCallBackDoubleRate), &data->edge_random_size);
	SetAdjustmentChangeValueCallBack(adjustment,
		setting_callback, setting_callback_data);
	gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

	widgets->edge_random_distance = adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		data->edge_random_distance * 100, 0, 200, 1, 1, 0));
	control = SpinScaleNew(adjustment, app->labels->tool_box.random_edge_distance, 1);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(AdjustmentChangeValueCallBackDoubleRate), &data->edge_random_distance);
	SetAdjustmentChangeValueCallBack(adjustment,
		setting_callback, setting_callback_data);
	gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

	widgets->num_children = adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		data->num_children, 1, 20, 1, 1, 0));
	control = SpinScaleNew(adjustment, app->labels->tool_box.num_children, 0);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(AdjustmentChangeValueCallBackUint16), &data->num_children);
	SetAdjustmentChangeValueCallBack(adjustment,
		setting_callback, setting_callback_data);
	gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

	widgets->start_child_size = adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		data->start_child_size * 100, 0, 100, 1, 1, 0));
	control = SpinScaleNew(adjustment, app->labels->tool_box.start_child_size, 1);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(AdjustmentChangeValueCallBackDoubleRate), &data->start_child_size);
	SetAdjustmentChangeValueCallBack(adjustment,
		setting_callback, setting_callback_data);
	gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

	widgets->end_child_size = adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		data->end_child_size * 100, 0, 100, 1, 1, 0));
	control = SpinScaleNew(adjustment, app->labels->tool_box.end_child_size, 1);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(AdjustmentChangeValueCallBackDoubleRate), &data->end_child_size);
	SetAdjustmentChangeValueCallBack(adjustment,
		setting_callback, setting_callback_data);
	gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

	return vbox;
}

static void ChangeTextLayerFontFamily(GtkComboBox *combo_box, LAYER* layer)
{
	layer->layer_data.text_layer_p->font_id = gtk_combo_box_get_active(combo_box);
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextLayerFontSize(GtkAdjustment* slider, LAYER* layer)
{
	layer->layer_data.text_layer_p->font_size = gtk_adjustment_get_value(slider);
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextColorButtonCallBack(GtkWidget* button, LAYER* layer)
{
	(void)memcpy(layer->layer_data.text_layer_p->color,
		layer->window->app->tool_window.color_chooser->rgb, 3);
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextDirectionCallBack(GtkWidget* widget, LAYER* layer)
{
	if(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "radio_id")) == 0)
	{
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_VERTICAL);
	}
	else
	{
		layer->layer_data.text_layer_p->flags |= TEXT_LAYER_VERTICAL;
	}
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextLayerBold(GtkWidget* widget, LAYER* layer)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_BOLD);
	}
	else
	{
		layer->layer_data.text_layer_p->flags |= TEXT_LAYER_BOLD;
	}
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextLayerStyle(GtkWidget* widget, LAYER* layer)
{
	int mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "text-style"));
	switch(mode)
	{
	case TEXT_STYLE_NORMAL:
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_ITALIC | TEXT_LAYER_OBLIQUE);
		break;
	case TEXT_STYLE_ITALIC:
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_OBLIQUE);
		layer->layer_data.text_layer_p->flags |= TEXT_LAYER_ITALIC;
		break;
	case TEXT_STYLE_OBLIQUE:
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_ITALIC);
		layer->layer_data.text_layer_p->flags |= TEXT_LAYER_OBLIQUE;
		break;
	}
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextLayerCenteringHorizontally(GtkWidget* button, LAYER* layer)
{
	TEXT_LAYER *layer_data = layer->layer_data.text_layer_p;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		layer_data->flags &= ~(TEXT_LAYER_CENTERING_HORIZONTALLY);
	}
	else
	{
		layer_data->flags |= TEXT_LAYER_CENTERING_HORIZONTALLY;
	}

	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(layer->window->window);
}

static void ChangeTextLayerCenteringVertically(GtkWidget* button, LAYER* layer)
{
	TEXT_LAYER *layer_data = layer->layer_data.text_layer_p;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		layer_data->flags &= ~(TEXT_LAYER_CENTERING_VERTICALLY);
	}
	else
	{
		layer_data->flags |= TEXT_LAYER_CENTERING_VERTICALLY;
	}

	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(layer->window->window);
}

static void ChangeTextLayerBalloonHasEdge(GtkWidget* button, LAYER* layer)
{
	TEXT_LAYER *layer_data = layer->layer_data.text_layer_p;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		layer_data->flags &= ~(TEXT_LAYER_BALLOON_HAS_EDGE);
	}
	else
	{
		layer_data->flags |= TEXT_LAYER_BALLOON_HAS_EDGE;
	}

	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(layer->window->window);
}

static void ChangeTextLayerBackGroundColor(GtkColorButton* button, LAYER* layer)
{
	TEXT_LAYER *layer_data = layer->layer_data.text_layer_p;
	GdkColor color;
	guint16 alpha;

	gtk_color_button_get_color(button, &color);
	layer_data->back_color[0] = (uint8)(color.red >> 8);
	layer_data->back_color[1] = (uint8)(color.green >> 8);
	layer_data->back_color[2] = (uint8)(color.blue >> 8);

	alpha = gtk_color_button_get_alpha(button);
	layer_data->back_color[3] = (uint8)(alpha >> 8);
	
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(layer->window->window);
}

static void ChangeTextLayerLineColor(GtkColorButton* button, LAYER* layer)
{
	TEXT_LAYER *layer_data = layer->layer_data.text_layer_p;
	GdkColor color;
	guint16 alpha;

	gtk_color_button_get_color(button, &color);
	layer_data->line_color[0] = (uint8)(color.red >> 8);
	layer_data->line_color[1] = (uint8)(color.green >> 8);
	layer_data->line_color[2] = (uint8)(color.blue >> 8);

	alpha = gtk_color_button_get_alpha(button);
	layer_data->line_color[3] = (uint8)(alpha >> 8);
	
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(layer->window->window);
}

static void ChangeTextLayerLineWidth(GtkAdjustment* slider, LAYER* layer)
{
	TEXT_LAYER *layer_data = layer->layer_data.text_layer_p;

	layer_data->line_width = gtk_adjustment_get_value(slider);

	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(layer->window->window);
}

static void ChangeTextLayerAdjust2Text(GtkWidget* button, LAYER* layer)
{
	TEXT_LAYER *layer_data = layer->layer_data.text_layer_p;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		layer_data->flags &= ~(TEXT_LAYER_ADJUST_RANGE_TO_TEXT);
	}
	else
	{
		layer_data->flags |= TEXT_LAYER_ADJUST_RANGE_TO_TEXT;
	}

	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(layer->window->window);
}

static void ChangeTextLayerBalloonType(LAYER* layer)
{
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(layer->window->window);
}

static void ChangeTextLayerBalloonSetting(LAYER* layer)
{
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(layer->window->window);
}

GtkWidget* CreateTextLayerDetailUI(APPLICATION* app, LAYER* target, TEXT_LAYER* layer)
{
#define MAX_STR_LENGTH 256
#define UI_FONT_SIZE 8.0
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0), *hbox = gtk_hbox_new(FALSE, 0);
#if GTK_MAJOR_VERSION <= 2
	GtkWidget *font_selection = gtk_combo_box_new_text();
#else
	GtkWidget *font_selection = gtk_combo_box_text_new();
#endif
	GtkWidget *font_scale, *label, *button;
	GtkWidget *control;
	GtkWidget *base_scale;
	GtkWidget *scale;
	GtkWidget *radio_buttons[3];
	GtkAdjustment *adjust;
	GdkColor color;
	GList *list;
	const gchar *font_name;
	char item_str[MAX_STR_LENGTH];
	int i;

	for(i=0; i<app->num_font; i++)
	{
		font_name = pango_font_family_get_name(app->font_list[i]);
		(void)sprintf(item_str, "<span font_family=\"%s\">%s</span>",
			font_name, font_name);
#if GTK_MAJOR_VERSION <= 2
		gtk_combo_box_append_text(GTK_COMBO_BOX(font_selection), item_str);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(font_selection), item_str);
#endif
	}
	list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(font_selection));
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(font_selection), list->data, "markup", 0, NULL);
	gtk_widget_set_size_request(font_selection, 128, 32);
	gtk_combo_box_set_active(GTK_COMBO_BOX(font_selection), layer->font_id);
	(void)g_signal_connect(G_OBJECT(font_selection), "changed", G_CALLBACK(ChangeTextLayerFontFamily), target);
	g_list_free(list);

	gtk_box_pack_start(GTK_BOX(vbox), font_selection, FALSE, TRUE, 0);

	switch(layer->base_size)
	{
	case 0:
		adjust = GTK_ADJUSTMENT(gtk_adjustment_new(layer->font_size, 0.1, 10, 0.1, 0.1, 0));
		break;
	case 1:
		adjust = GTK_ADJUSTMENT(gtk_adjustment_new(layer->font_size, 1, 100, 1, 10, 0));
		break;
	default:
		adjust = GTK_ADJUSTMENT(gtk_adjustment_new(layer->font_size, 5, 500, 1, 10, 0));
	}

	font_scale = SpinScaleNew(adjust,
		app->labels->tool_box.scale, 1);
	(void)g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextLayerFontSize), target);

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		label = gtk_label_new("");
		(void)sprintf(item_str, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), item_str);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION <= 2
		base_scale = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[2]);
#else
		base_scale = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), layer->base_size);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), font_scale, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	g_object_set_data(G_OBJECT(base_scale), "scale", font_scale);
	(void)g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &layer->base_size);


	button = gtk_button_new_with_label(app->labels->tool_box.change_text_color);
	(void)g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(ChangeTextColorButtonCallBack), target);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(TRUE, 0);
	radio_buttons[0] = gtk_radio_button_new_with_label(
		NULL, app->labels->tool_box.text_horizonal);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.text_vertical
	);

	for(i=0; i<2; i++)
	{
		g_object_set_data(G_OBJECT(radio_buttons[i]), "radio_id", GINT_TO_POINTER(i));
		(void)g_signal_connect(G_OBJECT(radio_buttons[i]), "toggled",
			G_CALLBACK(ChangeTextDirectionCallBack), target);
		gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[i], FALSE, TRUE, 0);
	}

	if((layer->flags & TEXT_LAYER_VERTICAL) == 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
	}

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 2);
	label = gtk_label_new("");
	(void)sprintf(item_str, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.text_style);
	gtk_label_set_markup(GTK_LABEL(label), item_str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	button = gtk_check_button_new_with_label(app->labels->tool_box.text_bold);
	(void)g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(ChangeTextLayerBold), target);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), layer->flags & TEXT_LAYER_BOLD);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(TRUE, 0);
	radio_buttons[0] = gtk_radio_button_new_with_label(
		NULL, app->labels->tool_box.text_normal);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.text_italic
	);
	radio_buttons[2] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.text_oblique
	);

	for(i=0; i<3; i++)
	{
		g_object_set_data(G_OBJECT(radio_buttons[i]), "text-style",
			GINT_TO_POINTER(i));
		(void)g_signal_connect(G_OBJECT(radio_buttons[i]), "toggled",
			G_CALLBACK(ChangeTextLayerStyle), target);
	}
	gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[1], FALSE, FALSE, 0);

	if((layer->flags & TEXT_LAYER_ITALIC) != 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
	}
	else if((layer->flags & TEXT_LAYER_OBLIQUE) != 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[2]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
	}
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[2], FALSE, TRUE, 0);

	button = gtk_check_button_new_with_label(app->labels->tool_box.centering_horizontally);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
		layer->flags & TEXT_LAYER_CENTERING_HORIZONTALLY);
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(ChangeTextLayerCenteringHorizontally), target);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_label(app->labels->tool_box.centering_vertically);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
		layer->flags & TEXT_LAYER_CENTERING_VERTICALLY);
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(ChangeTextLayerCenteringVertically), target);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	control = TextLayerSelectBalloonTypeWidgetNew(
		&layer->balloon_type, (void (*)(void*))ChangeTextLayerBalloonType, target, &layer->balloon_widgets, app);
	gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(item_str, "%s : ", app->labels->unit.bg);
	color.red = (layer->back_color[0] << 8) | layer->back_color[0];
	color.green = (layer->back_color[1] << 8) | layer->back_color[1];
	color.blue = (layer->back_color[2] << 8) | layer->back_color[2];
	button = gtk_color_button_new_with_color(&color);
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(button), TRUE);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(button), (layer->back_color[3] << 8) | layer->back_color[3]);
	(void)g_signal_connect(G_OBJECT(button), "color-set",
		G_CALLBACK(ChangeTextLayerBackGroundColor), target);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(item_str), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(item_str, "%s : ", app->labels->tool_box.line_color);
	color.red = (layer->line_color[0] << 8) | layer->line_color[0];
	color.green = (layer->line_color[1] << 8) | layer->line_color[1];
	color.blue = (layer->line_color[2] << 8) | layer->line_color[2];
	button = gtk_color_button_new_with_color(&color);
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(button), TRUE);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(button), (layer->line_color[3] << 8) | layer->line_color[3]);
	(void)g_signal_connect(G_OBJECT(button), "color-set",
		G_CALLBACK(ChangeTextLayerLineColor), target);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(item_str), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(layer->line_width, 0.1, 100, 1, 1, 0));
	scale = SpinScaleNew(adjust, app->labels->tool_box.line_width, 1);
	(void)g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextLayerLineWidth), target);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_label(app->labels->tool_box.balloon_has_edge);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
		layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE);
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(ChangeTextLayerBalloonHasEdge), target);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_label(app->labels->tool_box.adjust_range_to_text);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
		layer->flags & TEXT_LAYER_ADJUST_RANGE_TO_TEXT);
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(ChangeTextLayerAdjust2Text), target);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	control = CreateBalloonDetailSettinWidget(&layer->balloon_data, &layer->balloon_widgets,
		(void (*)(void*))ChangeTextLayerBalloonSetting, target, app);
	gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

	return vbox;
}

void RenderTextLayer(DRAW_WINDOW* window, LAYER* target, TEXT_LAYER* layer)
{
#define RIGHT_MOVE_AMOUNT 0.3f
#define UPPER_MOVE_AMOUNT 0.3f
	gdouble draw_y;
	char* show_text, *str;
	size_t length, i;
	cairo_font_slant_t font_slant;
	cairo_font_weight_t font_weight;
	gdouble draw_width = 0, draw_height = 0;
	gdouble max_size = 0;

	(void)memset(window->mask_temp->pixels, 0, window->pixel_buf_size);
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	(void)memset(target->pixels, 0, target->stride*target->height);

	if(layer->back_color[3] != 0
		|| (layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		window->app->draw_balloon_functions[layer->balloon_type](layer, target, window);
	}

	if(layer->text == NULL)
	{
		return;
	}

	if((layer->flags & TEXT_LAYER_ITALIC) != 0)
	{
		font_slant = CAIRO_FONT_SLANT_ITALIC;
	}
	else if((layer->flags & TEXT_LAYER_OBLIQUE) != 0)
	{
		font_slant = CAIRO_FONT_SLANT_OBLIQUE;
	}
	else
	{
		font_slant = CAIRO_FONT_SLANT_NORMAL;
	}

	if((layer->flags & TEXT_LAYER_BOLD) != 0)
	{
		font_weight = CAIRO_FONT_WEIGHT_BOLD;
	}
	else
	{
		font_weight = CAIRO_FONT_WEIGHT_NORMAL;
	}

	length = strlen(layer->text);

	cairo_save(window->mask_temp->cairo_p);

	cairo_set_source_rgb(window->mask_temp->cairo_p, layer->color[0]*DIV_PIXEL,
		layer->color[1]*DIV_PIXEL, layer->color[2]*DIV_PIXEL);
	cairo_select_font_face(
		window->mask_temp->cairo_p,
		pango_font_family_get_name(window->app->font_list[layer->font_id]),
		font_slant, font_weight
	);
	cairo_set_font_size(window->mask_temp->cairo_p, layer->font_size);

	if((layer->flags & TEXT_LAYER_VERTICAL) == 0)
	{
		// 文字列の描画結果の幅・高さ取得用
		cairo_text_extents_t character_info;

		draw_y = layer->y + layer->font_size;
		show_text = str = MEM_STRDUP_FUNC(layer->text);

		for(i=0; i<length; i++)
		{
			if(str[i] == '\n')
			{
				str[i] = '\0';
				cairo_move_to(window->mask_temp->cairo_p, layer->x, draw_y);
				cairo_show_text(window->mask_temp->cairo_p, show_text);

				cairo_text_extents(window->mask_temp->cairo_p, show_text, &character_info);

				draw_y += layer->font_size;
				show_text = &str[i+1];

				draw_height += layer->font_size;
				if(max_size < character_info.width)
				{
					max_size = character_info.width;
				}
			}
		}
		cairo_move_to(window->mask_temp->cairo_p, layer->x, draw_y);
		cairo_show_text(window->mask_temp->cairo_p, show_text);

		cairo_text_extents(window->mask_temp->cairo_p, show_text, &character_info);
		draw_height += layer->font_size;
		if(max_size < character_info.width)
		{
			max_size = character_info.width;
		}
		draw_width = max_size;

		MEM_FREE_FUNC(str);
	}
	else
	{
		gdouble draw_x = layer->x + layer->width - layer->font_size;
		gchar character_buffer[8];
		guint character_size, j;
		char* next_char;
		uint32 utf8_code;
		gdouble right_move, upper_move, rotate;

		show_text = str = layer->text;
		draw_y = layer->y;
		draw_width = layer->font_size;

		for(i=0; i<length; i++)
		{
			right_move = upper_move = rotate = 0;

			if(show_text[i] == '\n')
			{
				draw_y = layer->y;
				draw_x -= layer->font_size;
				draw_width += layer->font_size;

				if(max_size < draw_height)
				{
					max_size = draw_height;
				}
				draw_height = 0;
			}
			else if(show_text[i] >= 0x20 && show_text[i] <= 0x7E)
			{
				character_buffer[0] = show_text[i];
				character_buffer[1] = '\0';

				cairo_move_to(window->mask_temp->cairo_p,
					draw_x, draw_y);
				cairo_rotate(window->mask_temp->cairo_p, (FLOAT_T)G_PI*0.5);
				cairo_show_text(window->mask_temp->cairo_p, character_buffer);
				cairo_rotate(window->mask_temp->cairo_p, - (FLOAT_T)G_PI*0.5);
				draw_y += layer->font_size * 0.7f;
			}
			else
			{
				str = (char*)(&utf8_code);
				next_char = g_utf8_next_char(&show_text[i]);
				character_size = (guint)(next_char - &show_text[i]);
				for(j=0; j<sizeof(character_buffer)/sizeof(character_buffer[0]); j++)
				{
					character_buffer[j] = 0;
				}
				for(j=0; j<character_size; j++)
				{
					character_buffer[j] = show_text[i+j];
				}
				character_buffer[character_size] = '\0';
				i += character_size - 1;

				utf8_code = 0;
#ifndef __BIG_ENDIAN
				for(j=0; j<character_size; j++)
				{
					str[character_size-j-1] = character_buffer[j];
				}
#else
				for(j=0; j<character_size; j++)
				{
					str[sizeof(guint)-character_size+j] = character_buffer[j];
				}
#endif
				if(
					// 「ぁ」「ぃ」「ぅ」「ぇ」「ぉ」
						// 「っ」「ゃ」「ゅ」「ょ」「ゎ」
					utf8_code == 0xE38181
					|| utf8_code == 0xE38183
					|| utf8_code == 0xE38185
					|| utf8_code == 0xE38187
					|| utf8_code == 0xE38189
					|| utf8_code == 0xE381A3
					|| utf8_code == 0xE38283
					|| utf8_code == 0xE38285
					|| utf8_code == 0xE38287
					|| utf8_code == 0xE3828E
					// 「ァ」「ィ」「ゥ」「ェ」「ォ」
						// 「ッ」「ャ」「ュ」「ョ」「ヮ」
					|| utf8_code == 0xE382A1
					|| utf8_code == 0xE382A3
					|| utf8_code == 0xE382A5
					|| utf8_code == 0xE382A7
					|| utf8_code == 0xE382A9
					|| utf8_code == 0xE38383
					|| utf8_code == 0xE383A3
					|| utf8_code == 0xE383A5
					|| utf8_code == 0xE383A7
					|| utf8_code == 0xE383AE
					// 「ヵ」「ヶ」
					|| utf8_code == 0xE383B5
					|| utf8_code == 0xE383B6
				)
				{
					right_move = layer->font_size * RIGHT_MOVE_AMOUNT;
					upper_move = layer->font_size * UPPER_MOVE_AMOUNT;
					draw_x += right_move;
					draw_y -= upper_move;
					draw_height -= upper_move;
					upper_move *= 0.5;
				}
				else if(
					// 「、」「。」
					utf8_code == 0xE38081 || utf8_code == 0xE38082
				)
				{
					right_move = layer->font_size * RIGHT_MOVE_AMOUNT * 2;
					upper_move = layer->font_size * UPPER_MOVE_AMOUNT * 2;
					draw_x += right_move;
					draw_y -= upper_move;
					draw_height -= upper_move;
					upper_move *= 0.25;
				}
				else if(
					// 「：」
					utf8_code == 0xEFBC9A
					// 「‥」
					|| utf8_code == 0xE280A5
					// 「…」
					|| utf8_code == 0xE280A6
					// 「ー」
					|| utf8_code == 0xE383BC
					// 「―」
					|| utf8_code == 0xE28095
					// 「～」
					|| utf8_code == 0xE3809C
					// 「（）」、「〔〕」、「［］」、「〈〉」、「《》」、
						// 「「」」、「『』」、「【】」
					|| utf8_code == 0xEFBC88
					|| utf8_code == 0xEFBC89
					|| utf8_code == 0xE38094
					|| utf8_code == 0xE38095
					|| utf8_code == 0xEFBCBB
					|| utf8_code == 0xEFBCBD
					|| utf8_code == 0xEFBD9B
					|| utf8_code == 0xEFBD9D
					|| utf8_code == 0xE38088
					|| utf8_code == 0xE38089
					|| utf8_code == 0xE3808A
					|| utf8_code == 0xE3808B
					|| utf8_code == 0xE3808C
					|| utf8_code == 0xE3808D
					|| utf8_code == 0xE3808E
					|| utf8_code == 0xE3808F
					|| utf8_code == 0xE38090
					|| utf8_code == 0xE38091
				)
				{
					right_move = layer->font_size * RIGHT_MOVE_AMOUNT * 0.5;
					upper_move = layer->font_size * UPPER_MOVE_AMOUNT * 3;
					rotate = (FLOAT_T)G_PI * 0.5;
					draw_x += right_move;
					draw_y -= upper_move;
					draw_height -= upper_move;
					upper_move *= - 0.75;
				}
				else
				{
					// 文字サイズを取得してX座標を修正
					cairo_text_extents_t character_info;
					cairo_text_extents(window->mask_temp->cairo_p, character_buffer, &character_info);
					if(layer->font_size > character_info.width + character_info.x_bearing)
					{
						right_move = (layer->font_size - (character_info.width + character_info.x_bearing)) * 0.5;
					}
					else
					{
						right_move = ((character_info.width + character_info.x_bearing) - layer->font_size) * 0.5;
					}

					right_move -= character_info.x_bearing;
					draw_x += right_move;
				}

				cairo_move_to(window->mask_temp->cairo_p,
					draw_x - layer->font_size * 0.2f, draw_y + layer->font_size);
				cairo_rotate(window->mask_temp->cairo_p, rotate);
				cairo_show_text(window->mask_temp->cairo_p, character_buffer);
				cairo_rotate(window->mask_temp->cairo_p, - rotate);

				draw_x -= right_move;

				draw_y += layer->font_size * 1.2f - upper_move;
				draw_height += layer->font_size * 1.2f - upper_move;
			}	// for(i=0; i<length; i++)
		}

		if(max_size < draw_height)
		{
			max_size = draw_height;
		}

		draw_height = max_size;
	}

	if((layer->flags & TEXT_LAYER_ADJUST_RANGE_TO_TEXT) != 0)
	{
		layer->width = draw_width;
		layer->height = draw_height;
	}

	cairo_set_source_rgb(window->temp_layer->cairo_p, layer->color[0]*DIV_PIXEL,
		layer->color[1]*DIV_PIXEL, layer->color[2]*DIV_PIXEL);
	cairo_rectangle(window->temp_layer->cairo_p, layer->x, layer->y, layer->width, layer->height);
	cairo_fill(window->temp_layer->cairo_p);
	if((layer->flags & (TEXT_LAYER_CENTERING_HORIZONTALLY | TEXT_LAYER_CENTERING_VERTICALLY)) == 0)
	{
		cairo_set_source_surface(target->cairo_p, window->mask_temp->surface_p, 0, 0);
		cairo_mask_surface(target->cairo_p, window->temp_layer->surface_p, 0, 0);
	}
	else
	{
		cairo_pattern_t *draw_pattern;
		cairo_matrix_t trans_matrix;
		gdouble draw_x, draw_y;

		if((layer->flags & TEXT_LAYER_VERTICAL) == 0)
		{
			if((layer->flags & TEXT_LAYER_CENTERING_HORIZONTALLY) != 0)
			{
				draw_x = - layer->width * 0.5 + draw_width * 0.5;
			}
			else
			{
				draw_x = 0;
			}
			if((layer->flags & TEXT_LAYER_CENTERING_VERTICALLY) != 0)
			{
				draw_y = - layer->height * 0.5 + draw_height * 0.5;
			}
			else
			{
				draw_y = 0;
			}
		}
		else
		{
			if((layer->flags & TEXT_LAYER_CENTERING_HORIZONTALLY) != 0)
			{
				draw_x = (layer->width - draw_width - layer->font_size * 0.5) * 0.5;
			}
			else
			{
				draw_x = 0;
			}
			if((layer->flags & TEXT_LAYER_CENTERING_VERTICALLY) != 0)
			{
				draw_y = - layer->height * 0.5 + draw_height * 0.5;
			}
			else
			{
				draw_y = 0;
			}
		}

		cairo_matrix_init_translate(&trans_matrix, draw_x, draw_y);
		draw_pattern = cairo_pattern_create_for_surface(window->mask_temp->surface_p);
		cairo_pattern_set_matrix(draw_pattern, &trans_matrix);

		cairo_set_source(target->cairo_p, draw_pattern);
		cairo_mask_surface(target->cairo_p, window->temp_layer->surface_p, 0, 0);

		cairo_pattern_destroy(draw_pattern);
	}

	cairo_restore(window->mask_temp->cairo_p);
}

void DisplayTextLayerRange(DRAW_WINDOW* window, TEXT_LAYER* layer)
{
	gdouble points[4][2];
	const gdouble select_range = TEXT_LAYER_SELECT_POINT_DISTANCE;
	int i;

	cairo_set_line_width(window->disp_temp->cairo_p, 1);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_rectangle(window->disp_temp->cairo_p, layer->x * window->zoom_rate,
		layer->y * window->zoom_rate, layer->width * window->zoom_rate,
		layer->height * window->zoom_rate);
	cairo_stroke(window->disp_temp->cairo_p);
	
	points[0][0] = layer->x * window->zoom_rate;
	points[0][1] = layer->y * window->zoom_rate;
	points[1][0] = layer->x * window->zoom_rate;
	points[1][1] = (layer->y + layer->height) * window->zoom_rate;
	points[2][0] = (layer->x + layer->width) * window->zoom_rate;
	points[2][1] = (layer->y + layer->height) * window->zoom_rate;
	points[3][0] = (layer->x + layer->width) * window->zoom_rate;
	points[3][1] = layer->y * window->zoom_rate;

	for(i=0; i<4; i++)
	{
		cairo_rectangle(window->disp_temp->cairo_p, points[i][0] - select_range,
			points[i][1] - select_range, select_range * 2, select_range * 2);
		cairo_stroke(window->disp_temp->cairo_p);
	}

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		gdouble center_x,	center_y;
		FLOAT_T edge_start_point[2];

		cairo_arc(window->disp_temp->cairo_p, layer->edge_position[0][0] * window->zoom_rate,
			layer->edge_position[0][1] * window->zoom_rate, select_range, 0, 2 * G_PI);
		cairo_stroke(window->disp_temp->cairo_p);

		cairo_arc(window->disp_temp->cairo_p, layer->edge_position[1][0] * window->zoom_rate,
			layer->edge_position[1][1] * window->zoom_rate, select_range, 0, 2 * G_PI);
		cairo_stroke(window->disp_temp->cairo_p);

		cairo_arc(window->disp_temp->cairo_p, layer->edge_position[2][0] * window->zoom_rate,
			layer->edge_position[2][1] * window->zoom_rate, select_range, 0, 2 * G_PI);
		cairo_stroke(window->disp_temp->cairo_p);

		center_x = layer->x + layer->width * 0.5;
		center_y = layer->y + layer->height * 0.5;

		TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_start_point);
		cairo_arc(window->disp_temp->cairo_p, edge_start_point[0] * window->zoom_rate,
			edge_start_point[1] * window->zoom_rate, select_range, 0, 2 * G_PI);
		cairo_stroke(window->disp_temp->cairo_p);

		TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_start_point);
		cairo_arc(window->disp_temp->cairo_p, edge_start_point[0] * window->zoom_rate,
			edge_start_point[1] * window->zoom_rate, select_range, 0, 2 * G_PI);
		cairo_stroke(window->disp_temp->cairo_p);
	}
}

void OnChangeTextCallBack(GtkTextBuffer* buffer, LAYER* layer)
{
	GtkTextIter start, end;
	char *p;
	MEM_FREE_FUNC(layer->layer_data.text_layer_p->text);
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	layer->layer_data.text_layer_p->text = MEM_STRDUP_FUNC(
		p = gtk_text_buffer_get_text(buffer, &start, &end, FALSE));
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	g_free(p);
}

/*************************************************************
* TextFieldFocusIn関数                                       *
* テキストレイヤーの編集ウィジェットがフォーカスされた時に   *
*   呼び出されるコールバック関数                             *
* 引数                                                       *
* text_field	: テキストレイヤーの編集ウィジェット         *
* focus			: イベントの情報                             *
* app			: アプリケーションを管理する構造体のアドレス *
* 返り値                                                     *
*	常にFALSE                                                *
*************************************************************/
gboolean TextFieldFocusIn(GtkWidget* text_field, GdkEventFocus* focus, APPLICATION* app)
{
	if(app->tool_window.window == NULL)
	{
		app->flags |= APPLICATION_REMOVE_ACCELARATOR;
		gtk_window_remove_accel_group(GTK_WINDOW(app->widgets->window), app->widgets->hot_key);
	}

	return FALSE;
}

/*******************************************************************
* TextFieldFocusOut関数                                            *
* テキストレイヤーの編集ウィジェットからフォーカスが移動したた時に *
*   呼び出されるコールバック関数                                   *
* 引数                                                             *
* text_field	: テキストレイヤーの編集ウィジェット               *
* focus			: イベントの情報                                   *
* app			: アプリケーションを管理する構造体のアドレス       *
* 返り値                                                           *
*	常にFALSE                                                      *
*******************************************************************/
gboolean TextFieldFocusOut(GtkWidget* text_field, GdkEventFocus* focus, APPLICATION* app)
{
	if(app->tool_window.window == NULL
		&& (app->flags & APPLICATION_REMOVE_ACCELARATOR) != 0)
	{
		app->flags &= ~(APPLICATION_REMOVE_ACCELARATOR);
		gtk_window_add_accel_group(GTK_WINDOW(app->widgets->window), app->widgets->hot_key);
	}

	return FALSE;
}

/*************************************************************
* OnDestroyTextField関数                                     *
* テキストレイヤーの編集ウィジェットが削除される時に         *
*   呼び出されるコールバック関数                             *
* 引数                                                       *
* text_field	: テキストレイヤーの編集ウィジェット         *
* app			: アプリケーションを管理する構造体のアドレス *
*************************************************************/
void OnDestroyTextField(GtkWidget* text_field, APPLICATION* app)
{
	if((app->flags & APPLICATION_REMOVE_ACCELARATOR) != 0)
	{
		app->flags &= ~(APPLICATION_REMOVE_ACCELARATOR);
		gtk_window_add_accel_group(GTK_WINDOW(app->widgets->window), app->widgets->hot_key);
	}
}

/***************************************************
* TextLayerGetBalloonArcPoint関数                  *
* 吹き出しの尖り開始座標を取得する                 *
* layer	: テキストレイヤーの情報                   *
* angle	: 吹き出しの中心に対する尖り開始位置の角度 *
* point	: 座標を取得する配列                       *
***************************************************/
void TextLayerGetBalloonArcPoint(
	TEXT_LAYER* layer,
	FLOAT_T angle,
	FLOAT_T point[2]
)
{
	FLOAT_T center_x,	center_y;
	FLOAT_T sin_value,	cos_value;
	FLOAT_T tan_value;

	center_x = layer->x + layer->width * 0.5;
	center_y = layer->y + layer->height * 0.5;

	sin_value = sin(angle);
	cos_value = cos(angle);
	tan_value = tan(angle);

	switch(layer->balloon_type)
	{
	case TEXT_LAYER_BALLOON_TYPE_SQUARE:
		if(fabs(layer->width * 0.5 * tan_value) <= layer->height * 0.5)
		{
			point[0] = (cos_value <= 0) ? layer->x : layer->x + layer->width;
			point[1] = center_y + layer->width * 0.5 * tan_value;
		}
		else
		{
			point[0] = center_x + layer->height * 0.5 * (1 / tan_value);;
			point[1] = (sin_value <= 0) ? layer->y : layer->y + layer->height;
		}
		break;
	default:
		point[0] = center_x + layer->width * 0.5 * cos_value;
		point[1] = center_y + layer->height * 0.5 * sin_value;
	}
}

static void DrawBalloonEdge(TEXT_LAYER* layer, cairo_t* cairo_p)
{
	BEZIER_POINT calc[3], inter[2];
	FLOAT_T edge_position[2];

	TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_position);
	calc[0].x = edge_position[0],			calc[0].y = edge_position[1];
	calc[1].x = layer->edge_position[2][0],	calc[1].y = layer->edge_position[2][1];
	calc[2].x = layer->edge_position[0][0],	calc[2].y = layer->edge_position[0][1];
	MakeBezier3EdgeControlPoint(calc, inter);
	cairo_line_to(cairo_p, calc[0].x, calc[0].y);
	cairo_curve_to(cairo_p, calc[0].x, calc[0].y, inter[0].x, inter[0].y,
		layer->edge_position[2][0], layer->edge_position[2][1]);
	cairo_curve_to(cairo_p, layer->edge_position[2][0], layer->edge_position[2][1],
		inter[1].x, inter[1].y, layer->edge_position[0][0], layer->edge_position[0][1]);

	TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_position);
	calc[0].x = layer->edge_position[0][0],	calc[0].y = layer->edge_position[0][1];
	calc[1].x = layer->edge_position[1][0],	calc[1].y = layer->edge_position[1][1];
	calc[2].x = edge_position[0],			calc[2].y = edge_position[1];
	MakeBezier3EdgeControlPoint(calc, inter);
	cairo_curve_to(cairo_p, calc[0].x, calc[0].y, inter[0].x, inter[0].y,
		layer->edge_position[1][0], layer->edge_position[1][1]);
	cairo_curve_to(cairo_p, layer->edge_position[1][0], layer->edge_position[1][1],
		inter[1].x, inter[1].y, edge_position[0], edge_position[1]);
}

static void DrawEclipseBalloonEdge(TEXT_LAYER* layer, cairo_t* cairo_p)
{
	BEZIER_POINT calc[3], inter[2];
	FLOAT_T edge_position[2];
	FLOAT_T r;
	FLOAT_T x_scale, y_scale;

	if(layer->width > layer->height)
	{
		r = layer->height * 0.5;
		x_scale = (gdouble)layer->width / layer->height;
		y_scale = 1;
	}
	else
	{
		r = layer->width * 0.5;
		x_scale = 1;
		y_scale = (gdouble)layer->height / layer->width;
	}
	x_scale = 1 / x_scale;
	y_scale = 1 / y_scale;

	TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_position);
	calc[0].x = edge_position[0],			calc[0].y = edge_position[1];
	calc[1].x = layer->edge_position[2][0],	calc[1].y = layer->edge_position[2][1];
	calc[2].x = layer->edge_position[0][0],	calc[2].y = layer->edge_position[0][1];
	MakeBezier3EdgeControlPoint(calc, inter);
	cairo_curve_to(cairo_p, calc[0].x * x_scale, calc[0].y * y_scale, inter[0].x * x_scale, inter[0].y * y_scale,
		layer->edge_position[2][0] * x_scale, layer->edge_position[2][1] * y_scale);
	cairo_curve_to(cairo_p, layer->edge_position[2][0] * x_scale, layer->edge_position[2][1] * y_scale,
		inter[1].x * x_scale, inter[1].y * y_scale, layer->edge_position[0][0] * x_scale, layer->edge_position[0][1] * y_scale);

	TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_position);
	calc[0].x = layer->edge_position[0][0],	calc[0].y = layer->edge_position[0][1];
	calc[1].x = layer->edge_position[1][0],	calc[1].y = layer->edge_position[1][1];
	calc[2].x = edge_position[0],			calc[2].y = edge_position[1];
	MakeBezier3EdgeControlPoint(calc, inter);
	cairo_curve_to(cairo_p, calc[0].x * x_scale, calc[0].y * y_scale, inter[0].x * x_scale, inter[0].y * y_scale,
		layer->edge_position[1][0] * x_scale, layer->edge_position[1][1] * y_scale);
	cairo_curve_to(cairo_p, layer->edge_position[1][0] * x_scale, layer->edge_position[1][1] * y_scale,
		inter[1].x * x_scale, inter[1].y * y_scale, edge_position[0] * x_scale, edge_position[1] * y_scale);
}

/*************************************
* DrawSquareBalloon関数              *
* 長方形の吹き出しを描画する         *
* 引数                               *
* layer		: テキストレイヤーの情報 *
* target	: 描画を実施するレイヤー *
* canvas	: キャンバスの情報       *
*************************************/
static void DrawSquareBalloon(
	TEXT_LAYER* layer,
	LAYER* target,
	DRAW_WINDOW* canvas
)
{
#define FUZZY_ZERO_VALUE 0.3
	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		FLOAT_T center_x,	center_y;
		FLOAT_T edge_start_point[2];
		FLOAT_T edge_end_point[2];

		center_x = layer->x + layer->width * 0.5;
		center_y = layer->y + layer->height * 0.5;

		TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_start_point);
		TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_end_point);

		if(fabs(edge_start_point[0] - layer->x) <= FUZZY_ZERO_VALUE
			|| fabs(edge_start_point[0] - (layer->x + layer->width)) <= FUZZY_ZERO_VALUE)
		{
			if(fabs(edge_start_point[0] - edge_end_point[0]) <= FUZZY_ZERO_VALUE)
			{
				if(fabs(edge_start_point[0] - layer->x) <= FUZZY_ZERO_VALUE)
				{
					if(edge_start_point[1] < edge_end_point[1])
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
					else
					{
						cairo_move_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
				}
				else
				{
					if(edge_start_point[1] < edge_end_point[1])
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
					else
					{
						cairo_move_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
				}
			}
			else
			{
				if(fabs(edge_start_point[0] - layer->x) <= FUZZY_ZERO_VALUE)
				{
					if(edge_start_point[1] < edge_end_point[1])
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
					else
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);\
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
				}
				else
				{
					if(edge_start_point[1] < edge_end_point[1])
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
					else
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
				}
			}
		}
		else
		{
			if(fabs(edge_start_point[1] - edge_end_point[1]) <= FUZZY_ZERO_VALUE)
			{
				if(fabs(edge_start_point[1] - layer->y) <= FUZZY_ZERO_VALUE)
				{
					if(edge_start_point[0] < edge_end_point[0])
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
					else
					{
						cairo_move_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
				}
				else
				{
					if(edge_start_point[0] < edge_end_point[0])
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
					else
					{
						cairo_move_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
				}
			}
			else
			{
				if(fabs(edge_start_point[1] - layer->y) <= FUZZY_ZERO_VALUE)
				{
					if(edge_start_point[0] < edge_end_point[0])
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
					else
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
				}
				else
				{
					if(edge_start_point[0] < edge_end_point[0])
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
					else
					{
						cairo_move_to(target->cairo_p, edge_start_point[0], edge_start_point[1]);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y + layer->height);
						cairo_line_to(target->cairo_p, layer->x + layer->width, layer->y);
						cairo_line_to(target->cairo_p, layer->x, layer->y);
						cairo_line_to(target->cairo_p, edge_end_point[0], edge_end_point[1]);
						DrawBalloonEdge(layer, target->cairo_p);
					}
				}
			}
		}

		cairo_close_path(target->cairo_p);
		cairo_set_source_rgba(target->cairo_p, layer->back_color[0] * DIV_PIXEL,
			layer->back_color[1] * DIV_PIXEL, layer->back_color[2] * DIV_PIXEL, layer->back_color[3] * DIV_PIXEL);
		cairo_fill_preserve(target->cairo_p);
		cairo_set_source_rgba(target->cairo_p, layer->line_color[0] * DIV_PIXEL,
			layer->line_color[1] * DIV_PIXEL, layer->line_color[2] * DIV_PIXEL, layer->line_color[3] * DIV_PIXEL);
		cairo_set_line_width(target->cairo_p, layer->line_width);
		cairo_stroke(target->cairo_p);
	}
	else
	{
		cairo_rectangle(target->cairo_p, layer->x, layer->y, layer->width, layer->height);
		cairo_set_source_rgba(target->cairo_p, layer->back_color[0] * DIV_PIXEL,
			layer->back_color[1] * DIV_PIXEL, layer->back_color[2] * DIV_PIXEL, layer->back_color[3] * DIV_PIXEL);
		cairo_fill_preserve(target->cairo_p);
		cairo_set_source_rgba(target->cairo_p, layer->line_color[0] * DIV_PIXEL,
			layer->line_color[1] * DIV_PIXEL, layer->line_color[2] * DIV_PIXEL, layer->line_color[3] * DIV_PIXEL);
		cairo_set_line_width(target->cairo_p, layer->line_width);
		cairo_stroke(target->cairo_p);
	}
#undef FUZZY_ZERO_VALUE
}

/*************************************
* DrawEclipseBalloon関数             *
* 楕円形の吹き出しを描画する         *
* 引数                               *
* layer		: テキストレイヤーの情報 *
* target	: 描画を実施するレイヤー *
* canvas	: キャンバスの情報       *
*************************************/
static void DrawEclipseBalloon(
	TEXT_LAYER* layer,
	LAYER* target,
	DRAW_WINDOW* canvas
)
{
	cairo_t *cairo_p = cairo_create(target->surface_p);
	gdouble r;
	gdouble x_scale, y_scale;

	if(layer->width > layer->height)
	{
		r = layer->height * 0.5;
		x_scale = (gdouble)layer->width / layer->height;
		y_scale = 1;
	}
	else
	{
		r = layer->width * 0.5;
		x_scale = 1;
		y_scale = (gdouble)layer->height / layer->width;
	}

	cairo_set_line_width(cairo_p, layer->line_width);
	cairo_scale(cairo_p, x_scale, y_scale);

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) == 0)
	{
		cairo_arc(cairo_p, (layer->x + layer->width * 0.5) / x_scale,
			(layer->y + layer->height * 0.5) / y_scale, r, 0, 2 * G_PI);
	}
	else
	{
		cairo_arc(cairo_p, (layer->x + layer->width * 0.5) / x_scale,
			(layer->y + layer->height * 0.5) / y_scale, r, layer->arc_start, layer->arc_end);
		DrawEclipseBalloonEdge(layer, cairo_p);
	}
	cairo_close_path(cairo_p);

	cairo_set_source_rgba(cairo_p, layer->back_color[0] * DIV_PIXEL,
		layer->back_color[1] * DIV_PIXEL, layer->back_color[2] * DIV_PIXEL, layer->back_color[3] * DIV_PIXEL);
	cairo_fill_preserve(cairo_p);

	cairo_set_source_rgba(cairo_p, layer->line_color[0] * DIV_PIXEL,
		layer->line_color[1] * DIV_PIXEL, layer->line_color[2] * DIV_PIXEL, layer->line_color[3] * DIV_PIXEL);
	cairo_stroke(cairo_p);

	cairo_destroy(cairo_p);
}

/******************************************
* DrawEclipse関数                         *
* 楕円を描画する                          *
* 引数                                    *
* layer		: テキストレイヤーの情報      *
* cairp_p	: 描画を行うCairoコンテキスト *
* x			: 描画する範囲の左上のX座標   *
* y			: 描画する範囲の左上のY座標   *
* width		: 描画する範囲の幅            *
* height	: 描画する範囲の高さ          *
******************************************/
static void DrawEclipse(
	TEXT_LAYER* layer,
	cairo_t* cairo_p,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height
)
{
	FLOAT_T r;
	FLOAT_T x_scale, y_scale;

	if(width > height)
	{
		r = height * 0.5;
		x_scale = width / height;
		y_scale = 1;
	}
	else
	{
		r = width * 0.5;
		x_scale = 1;
		y_scale = height / width;
	}

	cairo_save(cairo_p);

	cairo_set_line_width(cairo_p, layer->line_width);
	cairo_scale(cairo_p, x_scale, y_scale);

	cairo_arc(cairo_p, (x + width * 0.5) / x_scale,
		(y + height * 0.5) / y_scale, r, 0, 2 * G_PI);
	cairo_close_path(cairo_p);

	cairo_set_source_rgba(cairo_p, layer->back_color[0] * DIV_PIXEL,
		layer->back_color[1] * DIV_PIXEL, layer->back_color[2] * DIV_PIXEL, layer->back_color[3] * DIV_PIXEL);
	cairo_fill_preserve(cairo_p);

	cairo_set_source_rgba(cairo_p, layer->line_color[0] * DIV_PIXEL,
		layer->line_color[1] * DIV_PIXEL, layer->line_color[2] * DIV_PIXEL, layer->line_color[3] * DIV_PIXEL);
	cairo_stroke(cairo_p);

	cairo_restore(cairo_p);
}

/***************************************
* DrawEclipseThinkingBalloon関数       *
* 楕円形の心情描写の吹き出しを描画する *
* 引数                                 *
* layer		: テキストレイヤーの情報   *
* target	: 描画を実施するレイヤー   *
* canvas	: キャンバスの情報         *
***************************************/
static void DrawEclipseThinkingBalloon(
	TEXT_LAYER* layer,
	LAYER* target,
	DRAW_WINDOW* canvas
)
{
	BEZIER_POINT calc[4], inter[2], draw;
	FLOAT_T mid_position[2][2];
	FLOAT_T scale;
	cairo_t *cairo_p;
	FLOAT_T t;
	FLOAT_T dt = 1.0 / (layer->balloon_data.num_children + 1);
	int i;

	cairo_p = cairo_create(target->surface_p);

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		{
			FLOAT_T edge_position[2][2];
			TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_position[0]);
			TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_position[1]);
			mid_position[0][0] = (edge_position[0][0] + edge_position[1][0]) * 0.5;
			mid_position[0][1] = (edge_position[0][1] + edge_position[1][1]) * 0.5;

			mid_position[1][0] = (layer->edge_position[1][0] + layer->edge_position[2][0]) * 0.5;
			mid_position[1][1] = (layer->edge_position[1][1] + layer->edge_position[2][1]) * 0.5;
		}

		calc[0].x = layer->edge_position[0][0],	calc[0].y = layer->edge_position[0][1];
		calc[1].x = mid_position[1][0],			calc[1].y = mid_position[1][1];
		calc[2].x = mid_position[0][0],			calc[2].y = mid_position[0][1];
		MakeBezier3EdgeControlPoint(calc, inter);
		for(i=0, t=dt; i<layer->balloon_data.num_children; i++, t+=dt)
		{
			if(t < 0.5)
			{
				calc[0].x = layer->edge_position[0][0],	calc[0].y = layer->edge_position[0][1];
				calc[1].x = layer->edge_position[0][0],	calc[1].y = layer->edge_position[0][1];
				calc[2] = inter[0];
				calc[3].x = mid_position[1][0],			calc[3].y = mid_position[1][1];
				CalcBezier3(calc, t * 2, &draw);
			}
			else
			{
				calc[0].x = mid_position[1][0],	calc[0].y = mid_position[1][1];
				//calc[1].x = mid_position[1][0],	calc[1].y = mid_position[1][1];
				calc[1] = inter[1];
				calc[2].x = mid_position[0][0],	calc[2].y = mid_position[0][1];
				calc[3].x = mid_position[0][0],	calc[3].y = mid_position[0][1];
				CalcBezier3(calc, t * 2 - 1, &draw);
			}

			scale = layer->balloon_data.start_child_size * t + layer->balloon_data.end_child_size * (1 - t);

			DrawEclipse(layer, cairo_p, draw.x - layer->width * 0.5 * scale, draw.y - layer->height * 0.5 * scale,
				layer->width * scale, layer->height * scale);
		}
	}

	DrawEclipse(layer, cairo_p, layer->x, layer->y, layer->width, layer->height);

	cairo_destroy(cairo_p);
}

/*************************************
* DrawCrashBalloon関数               *
* 爆発形(直線)の吹き出しを描画する   *
* 引数                               *
* layer		: テキストレイヤーの情報 *
* target	: 描画を実施するレイヤー *
* canvas	: キャンバスの情報       *
*************************************/
static void DrawCrashBalloon(
	TEXT_LAYER* layer,
	LAYER* target,
	DRAW_WINDOW* canvas
)
{
	cairo_t *cairo_p = cairo_create(target->surface_p);
	FLOAT_T arc_start,	arc_end;
	FLOAT_T x,	y;
	FLOAT_T angle_step;
	FLOAT_T angle;
	const FLOAT_T half_width = layer->width * 0.5;
	const FLOAT_T half_height = layer->height * 0.5;
	const FLOAT_T center_x = layer->x + half_width;
	const FLOAT_T center_y = layer->y + half_height;
	FLOAT_T random_angle;
	FLOAT_T random_size;
	const int num_points = (layer->balloon_data.num_edge + 1) * 2 + 1;
	int i;

	srand(layer->balloon_data.random_seed);

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		if(layer->arc_start < layer->arc_end)
		{
			arc_start = layer->arc_start,	arc_end = layer->arc_end;
		}
		else
		{
			arc_start = layer->arc_start,		arc_end = layer->arc_end + 2 * G_PI;
		}
	}
	else
	{
		arc_start = 0,	arc_end = 2 * G_PI;
	}

	angle = arc_start;
	angle_step = (arc_end - arc_start) / (num_points + 1);
	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		x = center_x + half_width * cos(angle);
		y = center_y + half_height * sin(angle);
	}
	else
	{
		random_angle = angle + angle_step
			* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
				- layer->balloon_data.edge_random_distance * 0.5);
		random_size = 1 - ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_size);
		random_size *= (1 - layer->balloon_data.edge_size);
		x = center_x + half_width * random_size * cos(random_angle);
		y = center_y + half_height * random_size * sin(random_angle);
	}

	cairo_move_to(cairo_p, x, y);
	angle += angle_step;
	for(i=0; i<num_points; i++, angle += angle_step)
	{
		random_angle = angle + angle_step
			* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
				- layer->balloon_data.edge_random_distance * 0.5);
		if((i & 0x01) != 0)
		{
			random_size = 1 - ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_size);
			random_size *= (1 - layer->balloon_data.edge_size);
		}
		else
		{
			random_size = 1 + ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_size);
			random_size *= (1 + layer->balloon_data.edge_size);
		}

		x = center_x + half_width * random_size * cos(random_angle);
		y = center_y + half_height * random_size * sin(random_angle);

		cairo_line_to(cairo_p, x, y);
	}

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		x = center_x + half_width * cos(layer->arc_end);
		y = center_y + half_height * sin(layer->arc_end);
		cairo_line_to(cairo_p, x, y);
		DrawBalloonEdge(layer, cairo_p);
	}
	cairo_close_path(cairo_p);

	cairo_set_line_width(cairo_p, layer->line_width);

	cairo_set_source_rgba(cairo_p, layer->back_color[0] * DIV_PIXEL,
		layer->back_color[1] * DIV_PIXEL, layer->back_color[2] * DIV_PIXEL, layer->back_color[3] * DIV_PIXEL);
	cairo_fill_preserve(cairo_p);

	cairo_set_source_rgba(cairo_p, layer->line_color[0] * DIV_PIXEL,
		layer->line_color[1] * DIV_PIXEL, layer->line_color[2] * DIV_PIXEL, layer->line_color[3] * DIV_PIXEL);
	cairo_stroke(cairo_p);

	cairo_destroy(cairo_p);
}

/*************************************
* DrawSmokeBalloon関数               *
* モヤモヤ形の吹き出しを描画する     *
* 引数                               *
* layer		: テキストレイヤーの情報 *
* target	: 描画を実施するレイヤー *
* canvas	: キャンバスの情報       *
*************************************/
static void DrawSmokeBalloon(
	TEXT_LAYER* layer,
	LAYER* target,
	DRAW_WINDOW* canvas
)
{
	cairo_t *cairo_p = cairo_create(target->surface_p);
	const FLOAT_T half_width = layer->width * 0.5;
	const FLOAT_T half_height = layer->height * 0.5;
	const FLOAT_T center_x = layer->x + half_width;
	const FLOAT_T center_y = layer->y + half_height;
	FLOAT_T arc_start,	arc_end;
	FLOAT_T angle;
	FLOAT_T angle_step;
	FLOAT_T random_angle;
	FLOAT_T random_size;
	FLOAT_T before_angle;
	FLOAT_T mid_angle;
	FLOAT_T x, y;
	FLOAT_T mid_x,	mid_y;
	FLOAT_T before_x, before_y;
	const int num_points = layer->balloon_data.num_edge + 1;
	int i;

	srand(layer->balloon_data.random_seed);

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		if(layer->arc_start < layer->arc_end)
		{
			arc_start = layer->arc_start,	arc_end = layer->arc_end;
		}
		else
		{
			arc_start = layer->arc_start,		arc_end = layer->arc_end + 2 * G_PI;
		}
	}
	else
	{
		arc_start = 0,	arc_end = 2 * G_PI;
	}

	angle = arc_start;
	angle_step = (arc_end - arc_start) / num_points;
	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		x = center_x + half_width * cos(angle);
		y = center_y + half_height * sin(angle);
		before_angle = angle + angle_step
			* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
				- layer->balloon_data.edge_random_distance * 0.5);
		angle += angle_step;
	}
	else
	{
		random_angle = angle_step
			* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
				- layer->balloon_data.edge_random_distance * 0.5);
		x = center_x + half_width * cos(random_angle);
		y = center_y + half_height * sin(random_angle);

		angle = angle_step;
		arc_end += before_angle = random_angle;
	}

	cairo_move_to(cairo_p, x, y);
	before_x = x,	before_y = y;
	for(i=0; i<num_points-1; i++, angle += angle_step)
	{
		random_angle = angle + angle_step
			* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
				- layer->balloon_data.edge_random_distance * 0.5);
		random_size = 1 + ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_size);
		random_size *= (1 + layer->balloon_data.edge_size);

		x = center_x + half_width * cos(random_angle);
		y = center_y + half_height * sin(random_angle);

		mid_angle = (random_angle + before_angle) * 0.5;
		mid_x = center_x + half_width * random_size * cos(mid_angle);
		mid_y = center_y + half_height * random_size * sin(mid_angle);

		cairo_curve_to(cairo_p, (before_x + mid_x) * 0.5, (before_y + mid_y) * 0.5,
			(x + mid_x) * 0.5, (y + mid_y) * 0.5, x, y);

		before_angle = random_angle;
		before_x = x,	before_y = y;
	}
	random_angle = arc_end;
	random_size = 1 + ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_size);
	random_size *= (1 + layer->balloon_data.edge_size);

	x = center_x + half_width * cos(random_angle);
	y = center_y + half_height * sin(random_angle);

	mid_angle = (random_angle + before_angle) * 0.5;
	mid_x = center_x + half_width * random_size * cos(mid_angle);
	mid_y = center_y + half_height * random_size * sin(mid_angle);

	cairo_curve_to(cairo_p, (before_x + mid_x) * 0.5, (before_y + mid_y) * 0.5,
		(x + mid_x) * 0.5, (y + mid_y) * 0.5, x, y);

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		DrawBalloonEdge(layer, cairo_p);
	}
	cairo_close_path(cairo_p);

	cairo_set_line_width(cairo_p, layer->line_width);

	cairo_set_source_rgba(cairo_p, layer->back_color[0] * DIV_PIXEL,
		layer->back_color[1] * DIV_PIXEL, layer->back_color[2] * DIV_PIXEL, layer->back_color[3] * DIV_PIXEL);
	cairo_fill_preserve(cairo_p);

	cairo_set_source_rgba(cairo_p, layer->line_color[0] * DIV_PIXEL,
		layer->line_color[1] * DIV_PIXEL, layer->line_color[2] * DIV_PIXEL, layer->line_color[3] * DIV_PIXEL);
	cairo_stroke(cairo_p);

	cairo_destroy(cairo_p);
}

/******************************************
* DrawSmoke関数                           *
* モヤモヤを描画する                      *
* 引数                                    *
* layer		: テキストレイヤーの情報      *
* cairp_p	: 描画を行うCairoコンテキスト *
* x			: 描画する範囲の左上のX座標   *
* y			: 描画する範囲の左上のY座標   *
* width		: 描画する範囲の幅            *
* height	: 描画する範囲の高さ          *
******************************************/
static void DrawSmoke(
	cairo_t* cairo_p,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height,
	TEXT_LAYER* layer
)
{
	const FLOAT_T half_width = width * 0.5;
	const FLOAT_T half_height = height * 0.5;
	const FLOAT_T center_x = x + half_width;
	const FLOAT_T center_y = y + half_height;
	FLOAT_T arc_start,	arc_end;
	FLOAT_T angle;
	FLOAT_T angle_step;
	FLOAT_T random_angle;
	FLOAT_T random_size;
	FLOAT_T before_angle;
	FLOAT_T mid_angle;
	FLOAT_T mid_x,	mid_y;
	FLOAT_T before_x, before_y;
	const int num_points = layer->balloon_data.num_edge + 1;
	int i;

	arc_start = 0,	arc_end = 2 * G_PI;

	angle = arc_start;
	angle_step = (arc_end - arc_start) / num_points;

	random_angle = angle_step
		* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
			- layer->balloon_data.edge_random_distance * 0.5);
	x = center_x + half_width * cos(random_angle);
	y = center_y + half_height * sin(random_angle);

	angle = angle_step;
	arc_end += before_angle = random_angle;

	cairo_move_to(cairo_p, x, y);
	before_x = x,	before_y = y;
	for(i=0; i<num_points-1; i++, angle += angle_step)
	{
		random_angle = angle + angle_step
			* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
				- layer->balloon_data.edge_random_distance * 0.5);
		random_size = 1 + ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_size);
		random_size *= (1 + layer->balloon_data.edge_size);

		x = center_x + half_width * cos(random_angle);
		y = center_y + half_height * sin(random_angle);

		mid_angle = (random_angle + before_angle) * 0.5;
		mid_x = center_x + half_width * random_size * cos(mid_angle);
		mid_y = center_y + half_height * random_size * sin(mid_angle);

		cairo_curve_to(cairo_p, (before_x + mid_x) * 0.5, (before_y + mid_y) * 0.5,
			(x + mid_x) * 0.5, (y + mid_y) * 0.5, x, y);

		before_angle = random_angle;
		before_x = x,	before_y = y;
	}
	random_angle = arc_end;
	random_size = 1 + ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_size);
	random_size *= (1 + layer->balloon_data.edge_size);

	x = center_x + half_width * cos(random_angle);
	y = center_y + half_height * sin(random_angle);

	mid_angle = (random_angle + before_angle) * 0.5;
	mid_x = center_x + half_width * random_size * cos(mid_angle);
	mid_y = center_y + half_height * random_size * sin(mid_angle);

	cairo_curve_to(cairo_p, (before_x + mid_x) * 0.5, (before_y + mid_y) * 0.5,
		(x + mid_x) * 0.5, (y + mid_y) * 0.5, x, y);

	cairo_close_path(cairo_p);

	cairo_set_line_width(cairo_p, layer->line_width);

	cairo_set_source_rgba(cairo_p, layer->back_color[0] * DIV_PIXEL,
		layer->back_color[1] * DIV_PIXEL, layer->back_color[2] * DIV_PIXEL, layer->back_color[3] * DIV_PIXEL);
	cairo_fill_preserve(cairo_p);

	cairo_set_source_rgba(cairo_p, layer->line_color[0] * DIV_PIXEL,
		layer->line_color[1] * DIV_PIXEL, layer->line_color[2] * DIV_PIXEL, layer->line_color[3] * DIV_PIXEL);
	cairo_stroke(cairo_p);
}

/******************************************
* DrawSmokeThinkingBalloon1関数           *
* 煙形の心情描写の吹き出しを描画するその1 *
* 引数                                    *
* layer		: テキストレイヤーの情報      *
* target	: 描画を実施するレイヤー      *
* canvas	: キャンバスの情報            *
******************************************/
static void DrawSmokeThinkingBalloon1(
	TEXT_LAYER* layer,
	LAYER* target,
	DRAW_WINDOW* canvas
)
{
	BEZIER_POINT calc[4], inter[2], draw;
	FLOAT_T mid_position[2][2];
	FLOAT_T scale;
	cairo_t *cairo_p;
	FLOAT_T t;
	FLOAT_T dt = 1.0 / (layer->balloon_data.num_children + 1);
	int i;

	srand(layer->balloon_data.random_seed);

	cairo_p = cairo_create(target->surface_p);

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		{
			FLOAT_T edge_position[2][2];
			TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_position[0]);
			TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_position[1]);
			mid_position[0][0] = (edge_position[0][0] + edge_position[1][0]) * 0.5;
			mid_position[0][1] = (edge_position[0][1] + edge_position[1][1]) * 0.5;

			mid_position[1][0] = (layer->edge_position[1][0] + layer->edge_position[2][0]) * 0.5;
			mid_position[1][1] = (layer->edge_position[1][1] + layer->edge_position[2][1]) * 0.5;
		}

		calc[0].x = layer->edge_position[0][0],	calc[0].y = layer->edge_position[0][1];
		calc[1].x = mid_position[1][0],			calc[1].y = mid_position[1][1];
		calc[2].x = mid_position[0][0],			calc[2].y = mid_position[0][1];
		MakeBezier3EdgeControlPoint(calc, inter);
		for(i=0, t=dt; i<layer->balloon_data.num_children; i++, t+=dt)
		{
			if(t < 0.5)
			{
				calc[0].x = layer->edge_position[0][0],	calc[0].y = layer->edge_position[0][1];
				calc[1].x = layer->edge_position[0][0],	calc[1].y = layer->edge_position[0][1];
				calc[2] = inter[0];
				calc[3].x = mid_position[1][0],			calc[3].y = mid_position[1][1];
				CalcBezier3(calc, t * 2, &draw);
			}
			else
			{
				calc[0].x = mid_position[1][0],	calc[0].y = mid_position[1][1];
				calc[1] = inter[1];
				calc[2].x = mid_position[0][0],	calc[2].y = mid_position[0][1];
				calc[3].x = mid_position[0][0],	calc[3].y = mid_position[0][1];
				CalcBezier3(calc, t * 2 - 1, &draw);
			}

			scale = layer->balloon_data.start_child_size * t + layer->balloon_data.end_child_size * (1 - t);

			DrawEclipse(layer, cairo_p, draw.x - layer->width * 0.5 * scale, draw.y - layer->height * 0.5 * scale,
				layer->width * scale, layer->height * scale);
		}
	}

	DrawSmoke(cairo_p, layer->x, layer->y, layer->width, layer->height, layer);

	cairo_destroy(cairo_p);
}

/******************************************
* DrawSmokeThinkingBalloon2関数           *
* 煙形の心情描写の吹き出しを描画するその2 *
* 引数                                    *
* layer		: テキストレイヤーの情報      *
* target	: 描画を実施するレイヤー      *
* canvas	: キャンバスの情報            *
******************************************/
static void DrawSmokeThinkingBalloon2(
	TEXT_LAYER* layer,
	LAYER* target,
	DRAW_WINDOW* canvas
)
{
	BEZIER_POINT calc[4], inter[2], draw;
	FLOAT_T mid_position[2][2];
	FLOAT_T scale;
	cairo_t *cairo_p;
	FLOAT_T t;
	FLOAT_T dt = 1.0 / (layer->balloon_data.num_children + 1);
	int i;

	srand(layer->balloon_data.random_seed);

	cairo_p = cairo_create(target->surface_p);

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		{
			FLOAT_T edge_position[2][2];
			TextLayerGetBalloonArcPoint(layer, layer->arc_start, edge_position[0]);
			TextLayerGetBalloonArcPoint(layer, layer->arc_end, edge_position[1]);
			mid_position[0][0] = (edge_position[0][0] + edge_position[1][0]) * 0.5;
			mid_position[0][1] = (edge_position[0][1] + edge_position[1][1]) * 0.5;

			mid_position[1][0] = (layer->edge_position[1][0] + layer->edge_position[2][0]) * 0.5;
			mid_position[1][1] = (layer->edge_position[1][1] + layer->edge_position[2][1]) * 0.5;
		}

		calc[0].x = layer->edge_position[0][0],	calc[0].y = layer->edge_position[0][1];
		calc[1].x = mid_position[1][0],			calc[1].y = mid_position[1][1];
		calc[2].x = mid_position[0][0],			calc[2].y = mid_position[0][1];
		MakeBezier3EdgeControlPoint(calc, inter);
		for(i=0, t=dt; i<layer->balloon_data.num_children; i++, t+=dt)
		{
			if(t < 0.5)
			{
				calc[0].x = layer->edge_position[0][0],	calc[0].y = layer->edge_position[0][1];
				calc[1].x = layer->edge_position[0][0],	calc[1].y = layer->edge_position[0][1];
				calc[2] = inter[0];
				calc[3].x = mid_position[1][0],			calc[3].y = mid_position[1][1];
				CalcBezier3(calc, t * 2, &draw);
			}
			else
			{
				calc[0].x = mid_position[1][0],	calc[0].y = mid_position[1][1];
				calc[1] = inter[1];
				calc[2].x = mid_position[0][0],	calc[2].y = mid_position[0][1];
				calc[3].x = mid_position[0][0],	calc[3].y = mid_position[0][1];
				CalcBezier3(calc, t * 2 - 1, &draw);
			}

			scale = layer->balloon_data.start_child_size * t + layer->balloon_data.end_child_size * (1 - t);

			DrawSmoke(cairo_p, draw.x - layer->width * 0.5 * scale, draw.y - layer->height * 0.5 * scale,
				layer->width * scale, layer->height * scale, layer);
		}
	}

	DrawSmoke(cairo_p, layer->x, layer->y, layer->width, layer->height, layer);

	cairo_destroy(cairo_p);
}

/*************************************
* DrawBurstBalloon関数               *
* 爆発形(曲線)の吹き出しを描画する   *
* 引数                               *
* layer		: テキストレイヤーの情報 *
* target	: 描画を実施するレイヤー *
* canvas	: キャンバスの情報       *
*************************************/
static void DrawBurstBalloon(
	TEXT_LAYER* layer,
	LAYER* target,
	DRAW_WINDOW* canvas
)
{
	cairo_t *cairo_p = cairo_create(target->surface_p);
	const FLOAT_T half_width = layer->width * 0.5;
	const FLOAT_T half_height = layer->height * 0.5;
	const FLOAT_T center_x = layer->x + half_width;
	const FLOAT_T center_y = layer->y + half_height;
	FLOAT_T arc_start,	arc_end;
	FLOAT_T angle;
	FLOAT_T angle_step;
	FLOAT_T random_angle;
	FLOAT_T random_size;
	FLOAT_T before_angle;
	FLOAT_T mid_angle;
	FLOAT_T x, y;
	FLOAT_T mid_x,	mid_y;
	FLOAT_T before_x, before_y;
	const int num_points = layer->balloon_data.num_edge + 1;
	int i;

	srand(layer->balloon_data.random_seed);

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		if(layer->arc_start < layer->arc_end)
		{
			arc_start = layer->arc_start,	arc_end = layer->arc_end;
		}
		else
		{
			arc_start = layer->arc_start,		arc_end = layer->arc_end + 2 * G_PI;
		}
	}
	else
	{
		arc_start = 0,	arc_end = 2 * G_PI;
	}

	angle = arc_start;
	angle_step = (arc_end - arc_start) / num_points;
	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		x = center_x + half_width * cos(angle);
		y = center_y + half_height * sin(angle);
		before_angle = angle + angle_step
			* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
				- layer->balloon_data.edge_random_distance * 0.5);
		angle += angle_step;
	}
	else
	{
		random_angle = angle_step
			* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
				- layer->balloon_data.edge_random_distance * 0.5);
		x = center_x + half_width * cos(random_angle);
		y = center_y + half_height * sin(random_angle);

		angle = angle_step;
		arc_end += before_angle = random_angle;
	}

	cairo_move_to(cairo_p, x, y);
	before_x = x,	before_y = y;
	for(i=0; i<num_points-1; i++, angle += angle_step)
	{
		random_angle = angle + angle_step
			* ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_distance
				- layer->balloon_data.edge_random_distance * 0.5);
		random_size = 1 + ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_size);
		random_size *= (1 - layer->balloon_data.edge_size);

		x = center_x + half_width * cos(random_angle);
		y = center_y + half_height * sin(random_angle);

		mid_angle = (random_angle + before_angle) * 0.5;
		mid_x = center_x + half_width * random_size * cos(mid_angle);
		mid_y = center_y + half_height * random_size * sin(mid_angle);

		cairo_curve_to(cairo_p, (before_x + mid_x) * 0.5, (before_y + mid_y) * 0.5,
			(x + mid_x) * 0.5, (y + mid_y) * 0.5, x, y);

		before_angle = random_angle;
		before_x = x,	before_y = y;
	}
	random_angle = arc_end;
	random_size = 1 + ((rand() / (FLOAT_T)RAND_MAX) * layer->balloon_data.edge_random_size);
	random_size *= (1 - layer->balloon_data.edge_size);

	x = center_x + half_width * cos(random_angle);
	y = center_y + half_height * sin(random_angle);

	mid_angle = (random_angle + before_angle) * 0.5;
	mid_x = center_x + half_width * random_size * cos(mid_angle);
	mid_y = center_y + half_height * random_size * sin(mid_angle);

	cairo_curve_to(cairo_p, (before_x + mid_x) * 0.5, (before_y + mid_y) * 0.5,
		(x + mid_x) * 0.5, (y + mid_y) * 0.5, x, y);

	if((layer->flags & TEXT_LAYER_BALLOON_HAS_EDGE) != 0)
	{
		DrawBalloonEdge(layer, cairo_p);
	}
	cairo_close_path(cairo_p);

	cairo_set_line_width(cairo_p, layer->line_width);

	cairo_set_source_rgba(cairo_p, layer->back_color[0] * DIV_PIXEL,
		layer->back_color[1] * DIV_PIXEL, layer->back_color[2] * DIV_PIXEL, layer->back_color[3] * DIV_PIXEL);
	cairo_fill_preserve(cairo_p);

	cairo_set_source_rgba(cairo_p, layer->line_color[0] * DIV_PIXEL,
		layer->line_color[1] * DIV_PIXEL, layer->line_color[2] * DIV_PIXEL, layer->line_color[3] * DIV_PIXEL);
	cairo_stroke(cairo_p);

	cairo_destroy(cairo_p);
}

/***********************************************************************
* SetTextLayerDrawBalloonFunctions関数                                 *
* 吹き出しを描画する関数の関数ポインタ配列の中身を設定                 *
* 引数                                                                 *
* draw_balloon_functions	: 吹き出しを描画する関数の関数ポインタ配列 *
***********************************************************************/
void SetTextLayerDrawBalloonFunctions(void (*draw_balloon_functions[])(TEXT_LAYER*, LAYER*, DRAW_WINDOW*)
)
{
	draw_balloon_functions[TEXT_LAYER_BALLOON_TYPE_SQUARE] = DrawSquareBalloon;
	draw_balloon_functions[TEXT_LAYER_BALLOON_TYPE_ECLIPSE] = DrawEclipseBalloon;
	draw_balloon_functions[TEXT_LAYER_BALLOON_TYPE_ECLIPSE_THINKING] = DrawEclipseThinkingBalloon;
	draw_balloon_functions[TEXT_LAYER_BALLOON_TYPE_CRASH] = DrawCrashBalloon;
	draw_balloon_functions[TEXT_LAYER_BALLOON_TYPE_SMOKE] = DrawSmokeBalloon;
	draw_balloon_functions[TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING1] = DrawSmokeThinkingBalloon1;
	draw_balloon_functions[TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING2] = DrawSmokeThinkingBalloon2;
	draw_balloon_functions[TEXT_LAYER_BALLOON_TYPE_BURST] = DrawBurstBalloon;
}

/***********************************************
* LoadDefaultBallonData関数                    *
* 吹き出しの詳細データのデフォルト値を設定する *
* 引数                                         *
* balloon_type	: 吹き出しのタイプ             *
* balloon_data	: 吹き出しの詳細データ         *
***********************************************/
void LoadDefaultBallonData(uint16 balloon_type, TEXT_LAYER_BALLOON_DATA* balloon_data)
{
	switch(balloon_type)
	{
	case TEXT_LAYER_BALLOON_TYPE_SQUARE:
		break;
	case TEXT_LAYER_BALLOON_TYPE_ECLIPSE:
		break;
	case TEXT_LAYER_BALLOON_TYPE_ECLIPSE_THINKING:
		balloon_data->num_children = 3;
		balloon_data->start_child_size = 0.4;
		balloon_data->end_child_size = 0.2;
		break;
	case TEXT_LAYER_BALLOON_TYPE_CRASH:
		balloon_data->num_edge = 5;
		balloon_data->random_seed = 0;
		balloon_data->edge_size = 0.15;
		balloon_data->edge_random_size = 0.1;
		balloon_data->edge_random_distance = 0.1;
		break;
	case TEXT_LAYER_BALLOON_TYPE_SMOKE:
		balloon_data->num_edge = 15;
		balloon_data->random_seed = 0;
		balloon_data->edge_size = 0.15;
		balloon_data->edge_random_size = 0.1;
		balloon_data->edge_random_distance = 0.1;
		break;
	case TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING1:
	case TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING2:
		balloon_data->num_edge = 15;
		balloon_data->random_seed = 0;
		balloon_data->edge_size = 0.15;
		balloon_data->edge_random_size = 0.1;
		balloon_data->edge_random_distance = 0.1;
		balloon_data->num_children = 3;
		balloon_data->start_child_size = 0.4;
		balloon_data->end_child_size = 0.2;
		break;
	case TEXT_LAYER_BALLOON_TYPE_BURST:
		balloon_data->num_edge = 15;
		balloon_data->random_seed = 0;
		balloon_data->edge_size = 0.15;
		balloon_data->edge_random_size = 0.1;
		balloon_data->edge_random_distance = 0.1;
		break;
	}
}

/***************************************************************
* SetBallonWidgetDefault関数                                   *
* 吹き出しの詳細データのデフォルト値をウィジェットに設定する   *
* 引数                                                         *
* balloon_type	: 吹き出しのタイプ                             *
* balloon_data	: 吹き出しの詳細データを設定するウィジェット群 *
***************************************************************/
void SetBallonWidgetDefault(uint16 balloon_type, TEXT_LAYER_BALLOON_DATA_WIDGETS* balloon_widgets)
{
	switch(balloon_type)
	{
	case TEXT_LAYER_BALLOON_TYPE_SQUARE:
		break;
	case TEXT_LAYER_BALLOON_TYPE_ECLIPSE:
		break;
	case TEXT_LAYER_BALLOON_TYPE_ECLIPSE_THINKING:
		gtk_adjustment_set_value(balloon_widgets->num_children, 3);
		gtk_adjustment_set_value(balloon_widgets->start_child_size, 40);
		gtk_adjustment_set_value(balloon_widgets->end_child_size, 20);
		break;
	case TEXT_LAYER_BALLOON_TYPE_CRASH:
		gtk_adjustment_set_value(balloon_widgets->num_edge, 5);
		gtk_adjustment_set_value(balloon_widgets->random_seed, 0);
		gtk_adjustment_set_value(balloon_widgets->edge_size, 15);
		gtk_adjustment_set_value(balloon_widgets->edge_random_size, 10);
		gtk_adjustment_set_value(balloon_widgets->edge_random_distance, 10);
		break;
	case TEXT_LAYER_BALLOON_TYPE_SMOKE:
		gtk_adjustment_set_value(balloon_widgets->num_edge, 15);
		gtk_adjustment_set_value(balloon_widgets->random_seed, 0);
		gtk_adjustment_set_value(balloon_widgets->edge_size, 15);
		gtk_adjustment_set_value(balloon_widgets->edge_random_size, 10);
		gtk_adjustment_set_value(balloon_widgets->edge_random_distance, 10);
		break;
	case TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING1:
	case TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING2:
		gtk_adjustment_set_value(balloon_widgets->num_children, 3);
		gtk_adjustment_set_value(balloon_widgets->start_child_size, 40);
		gtk_adjustment_set_value(balloon_widgets->end_child_size, 20);
		gtk_adjustment_set_value(balloon_widgets->num_edge, 15);
		gtk_adjustment_set_value(balloon_widgets->random_seed, 0);
		gtk_adjustment_set_value(balloon_widgets->edge_size, 15);
		gtk_adjustment_set_value(balloon_widgets->edge_random_size, 10);
		gtk_adjustment_set_value(balloon_widgets->edge_random_distance, 10);
		break;
	case TEXT_LAYER_BALLOON_TYPE_BURST:
		gtk_adjustment_set_value(balloon_widgets->num_edge, 15);
		gtk_adjustment_set_value(balloon_widgets->random_seed, 0);
		gtk_adjustment_set_value(balloon_widgets->edge_size, 15);
		gtk_adjustment_set_value(balloon_widgets->edge_random_size, 10);
		gtk_adjustment_set_value(balloon_widgets->edge_random_distance, 10);
		break;
	}
}

static void OnDestroyBalloonTypeTable(GtkWidget** buttons)
{
	MEM_FREE_FUNC(g_object_get_data(G_OBJECT(buttons[0]), "changing_flag"));
	MEM_FREE_FUNC(buttons);
}

static void OnClickedBalloonSelectButton(GtkWidget* button, uint16* balloon_type)
{
	TEXT_LAYER_BALLOON_DATA_WIDGETS *widgets =
		(TEXT_LAYER_BALLOON_DATA_WIDGETS*)g_object_get_data(G_OBJECT(button), "widgets");
	GtkWidget **buttons = (GtkWidget**)g_object_get_data(G_OBJECT(button), "buttons");
	gboolean *is_changing_flag;
	void (*select_callback)(void*);
	int i;

	is_changing_flag = (gboolean*)g_object_get_data(G_OBJECT(button), "changing_flag");
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE
		|| *is_changing_flag != FALSE)
	{
		return;
	}

	*is_changing_flag = TRUE;
	*balloon_type = (uint16)(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "balloon_type")));
	select_callback = (void (*)(void*))g_object_get_data(G_OBJECT(button), "select_callback");

	if(select_callback != NULL)
	{
		select_callback(g_object_get_data(G_OBJECT(button), "callback_data"));
	}

	if(widgets != NULL)
	{
		SetBallonWidgetDefault(*balloon_type, widgets);
	}

	for(i=0; i<NUM_TEXT_LAYER_BALLOON_TYPE; i++)
	{
		if(buttons[i] != button)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[i]), FALSE);
		}
	}

	*is_changing_flag = FALSE;
}

/***********************************************************************
* TextLayerSelectBalloonTypeWidgetNew関数                              *
* 吹き出しのタイプを選択するウィジェットを作成する                     *
* 引数                                                                 *
* balloon_type			: 吹き出しのタイプを記憶する変数のアドレス     *
* select_callback		: タイプ選択時のコールバック関数の関数ポインタ *
* select_callback_data	: タイプ選択時のコールバック関数で使うデータ   *
* widgets				: 詳細設定を行うウィジェット群を記憶する構造体 *
* application			: アプリケーションを管理するデータ             *
* 返り値                                                               *
*	吹き出しのタイプを選択するボタンウィジェットのテーブルウィジェット *
***********************************************************************/
GtkWidget* TextLayerSelectBalloonTypeWidgetNew(
	uint16* balloon_type,
	void (*select_callback)(void*),
	void* select_callback_data,
	TEXT_LAYER_BALLOON_DATA_WIDGETS* widgets,
	void* application
)
{
#define TABLE_WIDTH 4
#define BUTTON_IMAGE_SIZE 32
#define BALLOON_DRAW_MARGINE 2
#define BALLOON_LINE_WIDTH 2
	APPLICATION *app = (APPLICATION*)application;
	DRAW_WINDOW *temp_canvas;
	LAYER *draw_layer;
	TEXT_LAYER *text_layer;
	FLOAT_T angle;
	GtkWidget *table;
	GtkWidget **buttons;
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	gboolean *is_changing_flag;
	FLOAT_T edge_position[2];
	const uint8 back_color[4] = {0xff, 0xff, 0xff, 0xff};
	const uint8 line_color[4] = {0x00, 0x00, 0x00, 0xff};
	uint8 *pixels;
	int x, y;
	int i;

	temp_canvas = CreateTempDrawWindow(BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE, 4, NULL, NULL, 0, app);
	draw_layer = CreateLayer(0, 0, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE, 4, TYPE_TEXT_LAYER,
		NULL, NULL, NULL, temp_canvas);
	text_layer = CreateTextLayer(NULL, BALLOON_DRAW_MARGINE, BALLOON_DRAW_MARGINE,
		BUTTON_IMAGE_SIZE - BALLOON_DRAW_MARGINE * 2, (BUTTON_IMAGE_SIZE * 2 / 3) - BALLOON_DRAW_MARGINE * 2,
			0, 1, 0, back_color, TEXT_LAYER_BALLOON_TYPE_SQUARE, back_color, line_color, BALLOON_LINE_WIDTH, NULL, TEXT_LAYER_BALLOON_HAS_EDGE);
	text_layer->edge_position[0][0] = BUTTON_IMAGE_SIZE - BALLOON_DRAW_MARGINE;
	text_layer->edge_position[0][1] = BUTTON_IMAGE_SIZE - BALLOON_DRAW_MARGINE;
	angle = atan2(text_layer->y + text_layer->height * 0.5 - text_layer->edge_position[0][1],
		text_layer->x + text_layer->width * 0.5 - text_layer->edge_position[0][0]) + G_PI;
	text_layer->arc_start = angle + DEFAULT_ARROW_WIDTH;
	text_layer->arc_end = angle - DEFAULT_ARROW_WIDTH + 2 * G_PI;
	draw_layer->layer_data.text_layer_p = text_layer;

	TextLayerGetBalloonArcPoint(text_layer, text_layer->arc_start, edge_position);
	text_layer->edge_position[1][0] = (edge_position[0] + text_layer->edge_position[0][0]) * 0.5;
	text_layer->edge_position[1][1] = (edge_position[1] + text_layer->edge_position[0][1]) * 0.5;

	TextLayerGetBalloonArcPoint(text_layer, text_layer->arc_end, edge_position);
	text_layer->edge_position[2][0] = (edge_position[0] + text_layer->edge_position[0][0]) * 0.5;
	text_layer->edge_position[2][1] = (edge_position[1] + text_layer->edge_position[0][1]) * 0.5;

	table = gtk_table_new((NUM_TEXT_LAYER_BALLOON_TYPE + TABLE_WIDTH - 1) / TABLE_WIDTH,
		TABLE_WIDTH, TRUE);

	buttons = (GtkWidget**)MEM_ALLOC_FUNC(sizeof(*buttons)*NUM_TEXT_LAYER_BALLOON_TYPE);
	is_changing_flag = (gboolean*)MEM_ALLOC_FUNC(sizeof(*is_changing_flag));
	g_object_set_data(G_OBJECT(table), "buttons", buttons);
	*is_changing_flag = FALSE;
	(void)g_signal_connect_swapped(G_OBJECT(table), "destroy",
		G_CALLBACK(OnDestroyBalloonTypeTable), buttons);

	for(i=0, x=0, y=0; i<NUM_TEXT_LAYER_BALLOON_TYPE; i++, x++)
	{
		if(x == TABLE_WIDTH)
		{
			y++;
			x = 0;
		}

		LoadDefaultBallonData((uint16)i, &text_layer->balloon_data);

		buttons[i] = gtk_toggle_button_new();
		g_object_set_data(G_OBJECT(buttons[i]), "buttons", buttons);
		g_object_set_data(G_OBJECT(buttons[i]), "changing_flag", is_changing_flag);
		g_object_set_data(G_OBJECT(buttons[i]), "balloon_type", GINT_TO_POINTER(i));
		g_object_set_data(G_OBJECT(buttons[i]), "select_callback", select_callback);
		g_object_set_data(G_OBJECT(buttons[i]), "callback_data", select_callback_data);
		g_object_set_data(G_OBJECT(buttons[i]), "widgets", widgets);

		(void)memset(draw_layer->pixels, 0, draw_layer->stride * draw_layer->height);
		app->draw_balloon_functions[i](text_layer, draw_layer, temp_canvas);
		pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, draw_layer->width, draw_layer->height);
		pixels = gdk_pixbuf_get_pixels(pixbuf);
		(void)memcpy(pixels, draw_layer->pixels, draw_layer->stride * draw_layer->height);

		image = gtk_image_new_from_pixbuf(pixbuf);
		gtk_container_add(GTK_CONTAINER(buttons[i]), image);

		if(i == *balloon_type)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[i]), TRUE);
		}

		(void)g_signal_connect(G_OBJECT(buttons[i]), "toggled",
			G_CALLBACK(OnClickedBalloonSelectButton), balloon_type);

		gtk_table_attach_defaults(GTK_TABLE(table), buttons[i], x, x+1, y, y+1);
	}

	DeleteLayer(&draw_layer);
	DeleteDrawWindow(&temp_canvas);

	return table;
#undef TABLE_WIDTH
#undef BUTTON_IMAGE_SIZE
}

#ifdef __cplusplus
}
#endif
