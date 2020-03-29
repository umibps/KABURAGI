// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "../../memory.h"
#include "../../layer.h"
#include "../../layer_window.h"
#include "../../application.h"
#include "../../drag_and_drop.h"
#include "input_gtk.h"
#include "gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LAYER_DRAG_STRING "LAYER_DRAG"

static GtkTargetEntry g_drag_targets[] = {{LAYER_DRAG_STRING, GTK_TARGET_SAME_WIDGET, 0}};

static gint LayerWidgetNameClickedCallBack(GtkWidget *widget, GdkEventButton* event, gpointer p);

static void LayerWidgetDragDataSet(
	GtkWidget* widget,
	GdkDragContext* context,
	GtkSelectionData* selection,
	guint target_type,
	guint t,
	gpointer data
);

static void LayerWidgetDragEnd(GtkWidget* widget, GdkDragContext* context, APPLICATION* app);

static gboolean OnCloseLayerWindow(GtkWidget* widget, GdkEvent* event_info, APPLICATION* app)
{
	gint x, y;
	gint width, height;

	gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
	gtk_window_get_size(GTK_WINDOW(widget), &width, &height);

	app->layer_window.window_x = x, app->layer_window.window_y = y;
	app->layer_window.window_width = width, app->layer_window.window_height = height;

	app->layer_window.window = NULL;

	return FALSE;
}

static void OnDestroyLayerViewWidget(APPLICATION* app)
{
	app->menus.disable_if_no_open[app->layer_window.change_bg_index] = NULL;
	app->menus.disable_if_no_open[app->layer_window.layer_control.opacity_index] = NULL;
	app->menus.disable_if_no_open[app->layer_window.layer_control.new_box_index] = NULL;
	app->menus.disable_if_single_layer[app->layer_window.layer_control.merge_index] = NULL;
	app->layer_window.change_bg_button = NULL;
	g_object_unref(app->layer_window.eye);
	g_object_unref(app->layer_window.pin);
	g_object_unref(app->layer_window.close);
	g_object_unref(app->layer_window.open);
}

static void ChangeLayerBlendModeCallBack(GtkComboBox *combo_box, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_OPEN_OPERATION) == 0)
	{
		DRAW_WINDOW *window = GetActiveDrawWindow(app);
		GList *container_list = gtk_container_get_children(GTK_CONTAINER(
			window->active_layer->widget->mode));
		GtkWidget* label = GTK_WIDGET(container_list->data);
		window->active_layer->layer_mode =
			gtk_combo_box_get_active(combo_box);
		gtk_label_set_label(GTK_LABEL(label),
			app->labels->layer_window.blend_modes[window->active_layer->layer_mode]);

		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

		g_list_free(container_list);
	}
}

static void ChangeLayerOpacityCallBack(GtkAdjustment* slider, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_OPEN_OPERATION) == 0)
	{
		DRAW_WINDOW *window = GetActiveDrawWindow(app);
		GList *container_list = gtk_container_get_children(GTK_CONTAINER(
			window->active_layer->widget->alpha));
		char buff[16];
		window->active_layer->alpha =
			(int8)gtk_adjustment_get_value(slider);
		gtk_adjustment_set_value(slider, window->active_layer->alpha);

		if(container_list != NULL)
		{
			(void)sprintf(buff, "%d%%", window->active_layer->alpha);
			gtk_label_set_label(GTK_LABEL(container_list->data), buff);
		}

		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

		g_list_free(container_list);
	}
}

static void SetLockLayerOpacity(GtkCheckButton* button, APPLICATION* app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		window->active_layer->flags &=
			~(LAYER_LOCK_OPACITY);
	}
	else
	{
		window->active_layer->flags |=
			LAYER_LOCK_OPACITY;
	}
}

static void SetLayerMaskingWithUnderLayer(GtkCheckButton* button, APPLICATION* app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		window->active_layer->flags &=
			~(LAYER_MASKING_WITH_UNDER_LAYER);
	}
	else
	{
		window->active_layer->flags |=
			LAYER_MASKING_WITH_UNDER_LAYER;
	}

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

static void LayerViewChangeLayerOrder(
	LAYER_WINDOW* layer_view,
	DRAW_WINDOW* window,
	int pointer_y
)
{
	LAYER *new_prev = layer_view->drag_prev, *layer = window->layer;
	LAYER *parent = window->active_layer->layer_set;
	int hierarchy = 0;

	// 一番下にレイヤーセットがこないようにする
	if((window->active_layer->next != NULL && window->active_layer->next->layer_type == TYPE_LAYER_SET && window->active_layer == window->layer)
		|| (window->active_layer->layer_type == TYPE_LAYER_SET && new_prev == NULL))
	{
		return;
	}

	// 移動後の子レイヤーの親の設定変更
	if(window->active_layer->layer_type == TYPE_LAYER_SET)
	{
		LAYER *target = new_prev->next;
		while(target != NULL && target->layer_set == window->active_layer)
		{
			target->layer_set = window->active_layer->layer_set;
			if(target->layer_set == target)
			{
				target->layer_set = NULL;
				break;
			}
			target = target->next;
		}
	}

	if(layer_view->drop_layer_set != NULL)
	{
		GtkStyle* style = gtk_widget_get_style(layer_view->view);
		gtk_widget_modify_bg(layer_view->drop_layer_set->widget->eye->widget, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
		gtk_widget_modify_bg(layer_view->drop_layer_set->widget->pin->widget, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
		gtk_widget_modify_bg(layer_view->drop_layer_set->widget->thumbnail, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
		gtk_widget_modify_bg(layer_view->drop_layer_set->widget->box, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
		gtk_widget_modify_bg(layer_view->drop_layer_set->widget->name, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
		gtk_widget_modify_bg(layer_view->drop_layer_set->widget->mode, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
		gtk_widget_modify_bg(layer_view->drop_layer_set->widget->alpha, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);

		gtk_alignment_set_padding(GTK_ALIGNMENT(window->active_layer->widget->alignment), 0, 0, LAYER_SET_DISPLAY_OFFSET, 0);
		parent = window->active_layer->layer_set = layer_view->drop_layer_set;
		window->active_layer_set = layer_view->drop_layer_set;
	}
	else
	{
		parent = window->active_layer->layer_set = NULL;
	}


	if(window->active_layer->prev != new_prev
		&& window->active_layer != new_prev)
	{
		LAYER* before_prev = window->active_layer->prev;

		if(window->active_layer->layer_type == TYPE_LAYER_SET)
		{
			LAYER *target = before_prev;
			LAYER *next_target;

			while(before_prev != NULL && before_prev->layer_set == window->active_layer)
			{
				before_prev = before_prev->prev;
			}

			ChangeLayerOrder(window->active_layer, new_prev,
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
			ChangeLayerOrder(window->active_layer, new_prev,
				&window->layer);
		}

		AddChangeLayerOrderHistory(window->active_layer, before_prev,
			window->active_layer->prev, layer_view->before_layer_set);

		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}
	else if(layer_view->before_layer_set != layer_view->drop_layer_set)
	{
		AddChangeLayerSetHistory(window->active_layer,
			layer_view->before_layer_set, layer_view->drop_layer_set);
	}

	window->active_layer_set = window->active_layer->layer_set;
	LayerViewSetActiveLayer(window->active_layer, window->app->layer_window.view);

	ClearLayerView(&window->app->layer_window);
	LayerViewSetDrawWindow(&window->app->layer_window, window);
}

gboolean DragTimeOutCallBack(APPLICATION* app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	LAYER *new_prev = NULL, *layer = window->layer;
	LAYER *parent;
	gint x, y;
	int no_check;

#if GTK_MAJOR_VERSION >= 3
	GtkAllocation allocation, next_allocation;
#endif

	gtk_widget_get_pointer(app->layer_window.view, &x, &y);

#if GTK_MAJOR_VERSION <= 2
	if(y < layer->widget->box->allocation.y + layer->widget->box->allocation.height)
#else
	gtk_widget_get_allocation(layer->widget->box, &allocation);
	if(y < allocation.y + allocation.height)
#endif
	{
		while(layer != NULL)
		{
			no_check = 0;
#if GTK_MAJOR_VERSION >= 3
			gtk_widget_get_allocation(layer->widget->box, &allocation);
#endif
			if(layer->layer_set != 0)
			{
				parent = layer->layer_set ;
				do
				{
					if((parent->flags & LAYER_SET_CLOSE) != 0)
					{
						no_check++;
						break;
					}
					parent = parent->layer_set;
				} while(parent != NULL);
			}

			if(no_check == 0)
			{
				if(layer->layer_type == TYPE_LAYER_SET)
				{
#if GTK_MAJOR_VERSION <= 2
					if(y > layer->widget->box->allocation.y - layer->widget->box->allocation.height/2)
#else
					if(y > allocation.y - allocation.height/2)
#endif
					{
						new_prev = layer;
						break;
					}
				}
				else
				{
#if GTK_MAJOR_VERSION <= 2
					if(y > layer->widget->box->allocation.y - layer->widget->box->allocation.height/2)
#else
					if(y > allocation.y - allocation.height/2)
#endif
					{
						new_prev = layer;
						break;
					}
				}
			}

			layer = layer->next;
		}
	}

	if(app->layer_window.drag_prev != new_prev)
	{
		GdkColor color;
		app->layer_window.drag_prev = new_prev;

		if(app->layer_window.drag_position_separator != NULL)
		{
			gtk_widget_destroy(app->layer_window.drag_position_separator);
			app->layer_window.drag_position_separator = NULL;
		}

		app->layer_window.drag_position_separator = gtk_hseparator_new();

		gtk_box_pack_start(GTK_BOX(app->layer_window.view),
			app->layer_window.drag_position_separator, FALSE, TRUE, 2);
		gtk_widget_show(app->layer_window.drag_position_separator);
		(void)gdk_color_parse("black", &color);
		gtk_widget_modify_fg(app->layer_window.drag_position_separator, GTK_STATE_NORMAL, &color);
		gtk_widget_modify_bg(app->layer_window.drag_position_separator, GTK_STATE_NORMAL, &color);
		gtk_box_reorder_child(GTK_BOX(app->layer_window.view),
			app->layer_window.drag_position_separator,
			GetLayerID(window->layer,
				new_prev, window->num_layer) + 1
		);
	}

	if(app->layer_window.drag_prev != NULL)
	{
#if GTK_MAJOR_VERSION >= 3
		gtk_widget_get_allocation(app->layer_window.drag_prev->next->widget->box, &allocation);
#endif
		if(app->layer_window.drag_prev->next != NULL
			&& app->layer_window.drag_prev->next->layer_type == TYPE_LAYER_SET)
		{
#if GTK_MAJOR_VERSION <= 2
			int quarter = app->layer_window.drag_prev->next->widget->box->allocation.height/4;
#else
			int quarter = allocation.height/4;
#endif

#if GTK_MAJOR_VERSION <= 2
			if(y >= app->layer_window.drag_prev->next->widget->box->allocation.y + quarter
				&& y <= app->layer_window.drag_prev->next->widget->box->allocation.y + quarter*3)
#else
			gtk_widget_get_allocation(app->layer_window.drag_prev->next->widget->box, &next_allocation);
			if(y >= allocation.y + quarter
				&& y <= next_allocation.y + quarter*3)
#endif
			{
				GdkColor color;
				LAYER *target;
				(void)gdk_color_parse("red", &color);
				app->layer_window.drop_layer_set = app->layer_window.drag_prev->next;
				target = app->layer_window.drop_layer_set;

				gtk_widget_modify_bg(target->widget->eye->widget, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->pin->widget, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->thumbnail, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->box, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->name, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->mode, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->alpha, GTK_STATE_NORMAL, &color);
			}
			else
			{
				LAYER *target = app->layer_window.drop_layer_set;

				if(app->layer_window.drag_prev->layer_set != NULL)
				{
					GdkColor color;
					(void)gdk_color_parse("red", &color);
					app->layer_window.drop_layer_set = app->layer_window.drag_prev->layer_set;
					target = app->layer_window.drop_layer_set;

					gtk_widget_modify_bg(target->widget->eye->widget, GTK_STATE_NORMAL, &color);
					gtk_widget_modify_bg(target->widget->pin->widget, GTK_STATE_NORMAL, &color);
					gtk_widget_modify_bg(target->widget->thumbnail, GTK_STATE_NORMAL, &color);
					gtk_widget_modify_bg(target->widget->box, GTK_STATE_NORMAL, &color);
					gtk_widget_modify_bg(target->widget->name, GTK_STATE_NORMAL, &color);
					gtk_widget_modify_bg(target->widget->mode, GTK_STATE_NORMAL, &color);
					gtk_widget_modify_bg(target->widget->alpha, GTK_STATE_NORMAL, &color);
				}
				else
				{
					if(target != NULL)
					{
						GtkStyle* style = gtk_widget_get_style(app->layer_window.view);
						gtk_widget_modify_bg(target->widget->eye->widget, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
						gtk_widget_modify_bg(target->widget->pin->widget, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
							gtk_widget_modify_bg(target->widget->thumbnail, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
						gtk_widget_modify_bg(target->widget->box, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
						gtk_widget_modify_bg(target->widget->name, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
						gtk_widget_modify_bg(target->widget->mode, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
						gtk_widget_modify_bg(target->widget->alpha, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
						app->layer_window.drop_layer_set = NULL;
					}
				}
			}
		}
		else if(app->layer_window.drag_prev->layer_set != NULL)
		{
			GdkColor color;
			LAYER *target = app->layer_window.drag_prev->layer_set;
			app->layer_window.drop_layer_set = app->layer_window.drag_prev->layer_set;
			gdk_color_parse("red", &color);

			gtk_widget_modify_bg(target->widget->eye->widget, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(target->widget->pin->widget, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(target->widget->thumbnail, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(target->widget->box, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(target->widget->name, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(target->widget->mode, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(target->widget->alpha, GTK_STATE_NORMAL, &color);
		}
		else if(app->layer_window.drop_layer_set != NULL)
		{
			LAYER *target = app->layer_window.drop_layer_set;
			GtkStyle* style = gtk_widget_get_style(app->layer_window.view);
			gtk_widget_modify_bg(target->widget->eye->widget, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(target->widget->pin->widget, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(target->widget->thumbnail, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(target->widget->box, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(target->widget->name, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(target->widget->mode, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(target->widget->alpha, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			app->layer_window.drop_layer_set = NULL;
		}
	}
	
	return TRUE;
}

gboolean LayerWidgetDragBegin(GtkWidget *widget, GdkDragContext *context, APPLICATION* app)
{
	DRAW_WINDOW *window;
	if(app->window_num == 0)
	{
		return FALSE;
	}

	window = GetActiveDrawWindow(app);
	app->layer_window.flags |= LAYER_WINDOW_IN_DRAG_OPERATION;
	app->layer_window.drag_prev = window->active_layer->prev;
	app->layer_window.drop_layer_set = NULL;
	app->layer_window.before_layer_set = window->active_layer->layer_set;

	app->layer_window.timer_id = g_timeout_add(1000/60, (GSourceFunc)DragTimeOutCallBack, app);

	return TRUE;
}

static gboolean DragMotion(GtkWidget* widget, void* data)
{
	//static int num = 0;
	//printf("drag motion %d\n", num++);

	return TRUE;
}

static void LayerWidgetDragDataSet(
	GtkWidget* widget,
	GdkDragContext* context,
	GtkSelectionData* selection,
	guint target_type,
	guint t,
	gpointer data
)
{
	if(target_type == DRAG_ID_LAYER_WIDGET)
	{
#if GTK_MAJOR_VERSION <= 2
		gtk_selection_data_set(selection, selection->target, sizeof(void*)*8,
			(const guchar*)&data, sizeof(void*));
#else
		GdkAtom target;
		target = gtk_selection_data_get_target(selection);
		gtk_selection_data_set(selection, target, sizeof(void*)*8,
			(const guchar*)&data, sizeof(void*));
#endif
	}
}

void LayerWidgetDragDataReceived(
	GtkWidget* widget,
	GdkDragContext* context,
	gint x,
	gint y,
	GtkSelectionData* selection,
	guint target_type,
	guint t,
	APPLICATION* app
)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	gboolean drag_success = FALSE;
	gboolean delete_data = FALSE;

#if GTK_MAJOR_VERSION <= 2
	if(selection != NULL && selection->length > 0)
#else
	if(selection != NULL && gtk_selection_data_get_length > 0)
#endif
	{
#if GTK_MAJOR_VERSION <= 2
		if(context->action == GDK_ACTION_MOVE)
#else
		if(gdk_drag_context_get_actions(context) == GDK_ACTION_MOVE)
#endif
		{
			if(target_type == DRAG_ID_LAYER_WIDGET
				&& (app->layer_window.flags & LAYER_WINDOW_IN_DRAG_OPERATION) != 0)
			{
				gtk_widget_destroy(app->layer_window.drag_position_separator);
				app->layer_window.drag_position_separator = NULL;
				LayerViewChangeLayerOrder(&app->layer_window, window, y);
				drag_success = TRUE;
				delete_data = TRUE;

				app->layer_window.flags &= ~(LAYER_WINDOW_IN_DRAG_OPERATION);
			}
		}
	}

	gtk_drag_finish(context, drag_success, delete_data, t);

	(void)g_source_remove(app->layer_window.timer_id);
}

gboolean LayerWidgetDropData(
	GtkWidget *widget,
	GdkDragContext* context,
	gint x,
	gint y,
	guint t,
	gpointer data
)
{
	gboolean valid_drop_site = FALSE;
	GdkAtom target_type;

#if GTK_MAJOR_VERSION <= 2
	if(context->targets != NULL)
	{
		target_type = GDK_POINTER_TO_ATOM(
			g_list_nth_data(context->targets, DRAG_ID_LAYER_WIDGET));
		gtk_drag_get_data(widget, context, target_type, t);
		valid_drop_site = TRUE;
	}
#else
	GList *list = gdk_drag_context_list_targets(context);
	if(list != NULL)
	{
		target_type = GDK_POINTER_TO_ATOM(g_list_nth_data(list, DRAG_ID_LAYER_WIDGET));
		gtk_drag_get_data(widget, context, target_type, t);
		valid_drop_site = TRUE;
	}
#endif

	return valid_drop_site;
}

static void LayerWidgetDragEnd(GtkWidget* widget, GdkDragContext* context, APPLICATION* app)
{
	if((app->layer_window.flags & LAYER_WINDOW_IN_DRAG_OPERATION) != 0)
	{
		DRAW_WINDOW *window = GetActiveDrawWindow(app);
		gint y;

		gtk_widget_destroy(app->layer_window.drag_position_separator);
		app->layer_window.drag_position_separator = NULL;
		gtk_widget_get_pointer(app->layer_window.view, NULL, &y);
		//gdk_window_get_pointer(app->layer_window.view->window, NULL, &y, NULL);

		LayerViewChangeLayerOrder(&app->layer_window, window, y);

		app->layer_window.flags &= ~(LAYER_WINDOW_IN_DRAG_OPERATION);

		(void)g_source_remove(app->layer_window.timer_id);
	}
}

static void OnClickedChangeBackGroundButton(GtkWidget* button, APPLICATION* app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	char buff[256];
	gboolean button_state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	if((app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) == 0)
	{
		if(button_state == FALSE)
		{
			(void)memset(window->back_ground,
				0xff, window->pixel_buf_size);
			window->flags &= ~(DRAW_WINDOW_SECOND_BG);
		}
		else
		{
			int i;

			for(i=0; i<window->width * window->height; i++)
			{
				window->back_ground[i*4+0] = window->second_back_ground[0];
				window->back_ground[i*4+1] = window->second_back_ground[1];
				window->back_ground[i*4+2] = window->second_back_ground[2];
				window->back_ground[i*4+3] = 0xff;
			}

			window->flags |= DRAW_WINDOW_SECOND_BG;
		}

		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
		gtk_widget_queue_draw(window->window);

		app->flags |= APPLICATION_IN_SWITCH_DRAW_WINDOW;
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->menus.change_back_ground_menu),
			button_state);
		app->flags &= ~(APPLICATION_IN_SWITCH_DRAW_WINDOW);
	}

	if(button_state == FALSE)
	{
		(void)sprintf(buff, "%s 1", app->labels->unit.bg);
	}
	else
	{
		(void)sprintf(buff, "<span color=\"red\">%s 2</span>", app->labels->unit.bg);
	}

	gtk_label_set_markup_with_mnemonic(GTK_LABEL(app->layer_window.change_bg_label), buff);
}

GtkWidget* CreateLayerView(APPLICATION* app, LAYER_WINDOW* window, GtkWidget** box)
{
#define UI_FONT_SIZE 8.0
	//const GtkTargetEntry target = {LAYER_DRAG_STRING, GTK_TARGET_SAME_WIDGET, DRAG_ID_LAYER_WIDGET};
	// レイヤー表示用のスクロールドウィンドウ
	GtkWidget *view = gtk_scrolled_window_new(NULL, NULL);
	// 合成モード、不透明度等とは切り分けてUI作成
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0), *hbox = gtk_hbox_new(FALSE, 0);
	// 合成モード、不透明度を表示
	GtkWidget *label;
	// 不透明度のスライダ
	GtkWidget *opacity_slider;
	// ボタンに表示するイメージ
	GtkWidget *image;
	// 新規レイヤー等のボタン
	GtkWidget *button;
	// レイヤー移動ボタン用の画像及び回転用
	GdkPixbuf *src, *image_buff;
	// 画像ファイルへのパス(Mac対策)
	gchar *file_path;
	// ラベルのフォントを設定するためにマークアップの文字列を作成
	char mark_up_buff[128];
	// for文用のカウンタ
	int i;

	// 合成モードのラベル
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE * app->gui_scale, app->labels->layer_window.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	// 合成モードを選択するコンボボックスを作成
#if GTK_MAJOR_VERSION <= 2
	window->layer_control.mode = gtk_combo_box_new_text();
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(window->layer_control.mode),
			app->labels->layer_window.blend_modes[i]);
	}
#else
	window->layer_control.mode = gtk_combo_box_text_new();
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX(window->layer_control.mode),
			app->labels->layer_window.blend_modes[i]);
	}
#endif
	// コンボボックスの値変更時のコールバック関数を設定
	(void)g_signal_connect(G_OBJECT(window->layer_control.mode), "changed", G_CALLBACK(ChangeLayerBlendModeCallBack), app);

	// ウィンドウ作成時は有効なイメージがないはずなので無効にしておく
	gtk_widget_set_sensitive(window->layer_control.mode, app->window_num > 0);
	
	// ラベルとコンボボックスを水平に並べて入れる
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), window->layer_control.mode, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	// 不透明度のラベル
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE * app->gui_scale, app->labels->layer_window.opacity);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	// 不透明度設定のスライダを作成
	window->layer_control.opacity = GTK_ADJUSTMENT(gtk_adjustment_new(100, 0, 100, 1, 1, 0));
	opacity_slider = gtk_hscale_new(window->layer_control.opacity);
	// 値変更時のコールバック関数を設定
	(void)g_signal_connect(G_OBJECT(window->layer_control.opacity), "value_changed",
		G_CALLBACK(ChangeLayerOpacityCallBack), app);
	// 数値表示は左側に
	gtk_scale_set_value_pos(GTK_SCALE(opacity_slider), GTK_POS_LEFT);
	// 端数表示なし
	gtk_scale_set_digits(GTK_SCALE(opacity_slider), 0);
	// 今度はラベルとスライダを水平に並べる
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), opacity_slider, TRUE, TRUE, 0);

	// ウィンドウ作成時は有効なイメージがないはずなので無効にしておく
	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		gtk_widget_set_sensitive(opacity_slider, FALSE);
		window->layer_control.opacity_index = (uint16)app->menus.num_disable_if_no_open;
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = opacity_slider;
		app->menus.num_disable_if_no_open++;
	}
	else
	{
		gtk_widget_set_sensitive(opacity_slider, app->window_num > 0);
		app->menus.disable_if_no_open[window->layer_control.opacity_index] = opacity_slider;
	}

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	// 不透明度保護
	app->layer_window.layer_control.lock_opacity =
		gtk_check_button_new_with_label(app->labels->layer_window.lock_opacity);
	(void)g_signal_connect(G_OBJECT(app->layer_window.layer_control.lock_opacity),
		"toggled", G_CALLBACK(SetLockLayerOpacity), app);
	gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), app->layer_window.layer_control.lock_opacity, FALSE, FALSE, 0);

	// 下のレイヤーでマスキングチェックボックス
	app->layer_window.layer_control.mask_width_under =
		gtk_check_button_new_with_label(app->labels->layer_window.mask_with_under);
	(void)g_signal_connect(G_OBJECT(app->layer_window.layer_control.mask_width_under),
		"toggled", G_CALLBACK(SetLayerMaskingWithUnderLayer), app);

	// 複数レイヤーがなければ無効
	gtk_widget_set_sensitive(app->layer_window.layer_control.mask_width_under, FALSE);

	gtk_box_pack_start(GTK_BOX(vbox), app->layer_window.layer_control.mask_width_under, FALSE, FALSE, 0);

	// 新規レイヤー関連のボタン
	window->layer_control.new_box = gtk_hbox_new(FALSE, 0);
	file_path = g_build_filename(app->current_path, "image/new_document.png", NULL);
	if(app->gui_scale == 1.0)
	{
		image = gtk_image_new_from_file(file_path);
	}
	else
	{
		src = gdk_pixbuf_new_from_file(file_path, NULL);
		image_buff = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
		image = gtk_image_new_from_pixbuf(image_buff);
		g_object_unref(src);
	}
	g_free(file_path);
	button = gtk_button_new();
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteMakeColorLayer), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	file_path = g_build_filename(app->current_path, "image/new_vector.png", NULL);
	if(app->gui_scale == 1.0)
	{
		image = gtk_image_new_from_file(file_path);
	}
	else
	{
		src = gdk_pixbuf_new_from_file(file_path, NULL);
		image_buff = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
		image = gtk_image_new_from_pixbuf(image_buff);
		g_object_unref(src);
	}
	g_free(file_path);
	button = gtk_button_new();
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteMakeVectorLayer), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	file_path = g_build_filename(app->current_path, "image/folder.png", NULL);
	if(app->gui_scale == 1.0)
	{
		image = gtk_image_new_from_file(file_path);
	}
	else
	{
		src = gdk_pixbuf_new_from_file(file_path, NULL);
		image_buff = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
		image = gtk_image_new_from_pixbuf(image_buff);
		g_object_unref(src);
	}
	g_free(file_path);
	button = gtk_button_new();
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteMakeLayerSet), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	if(GetHas3DLayer(app) != FALSE)
	{
		file_path = g_build_filename(app->current_path, "image/3d_icon.png", NULL);
		if(app->gui_scale == 1.0)
		{
			image = gtk_image_new_from_file(file_path);
		}
		else
		{
			src = gdk_pixbuf_new_from_file(file_path, NULL);
			image_buff = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
				(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
			image = gtk_image_new_from_pixbuf(image_buff);
			g_object_unref(src);
		}
		g_free(file_path);
		button = gtk_button_new();
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteMake3DLayer), app);
		gtk_container_add(GTK_CONTAINER(button), image);
		gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	}
	file_path = g_build_filename(app->current_path, "image/arrow.png", NULL);
	src = gdk_pixbuf_new_from_file(file_path, NULL);
	image_buff = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_CLOCKWISE);
	g_free(file_path);
	g_object_unref(src);
	if(app->gui_scale != 1.0)
	{
		src = image_buff;
		image_buff = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
		g_object_unref(src);
	}
	window->layer_control.up = button = gtk_button_new();
	image = gtk_image_new_from_pixbuf(image_buff);
	gtk_container_add(GTK_CONTAINER(button), image);
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteUpLayer), app);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	gtk_widget_set_sensitive(window->layer_control.up, FALSE);
	src = image_buff;
	window->layer_control.down = button = gtk_button_new();
	image_buff = gdk_pixbuf_flip(src, FALSE);
	image = gtk_image_new_from_pixbuf(image_buff);
	gtk_container_add(GTK_CONTAINER(button), image);
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteDownLayer), app);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	gtk_widget_set_sensitive(window->layer_control.down, FALSE);
	g_object_unref(src);
	g_object_unref(image_buff);

	gtk_box_pack_start(GTK_BOX(vbox), window->layer_control.new_box, FALSE, FALSE, 0);

	// ウィンドウ作成時は無効に
	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		gtk_widget_set_sensitive(window->layer_control.new_box, FALSE);
		window->layer_control.new_box_index = (uint16)app->menus.num_disable_if_no_open;
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = window->layer_control.new_box;
		app->menus.num_disable_if_no_open++;
	}
	else
	{
		gtk_widget_set_sensitive(window->layer_control.new_box, app->window_num > 0);
		app->menus.disable_if_no_open[window->layer_control.new_box_index] = window->layer_control.new_box;
	}

	// 下のレイヤーと結合関連のボタン
	hbox = gtk_hbox_new(FALSE, 0);
	window->layer_control.merge_box = gtk_hbox_new(FALSE, 0);
	file_path = g_build_filename(app->current_path, "image/merge_down.png", NULL);
	if(app->gui_scale == 1.0)
	{
		image = gtk_image_new_from_file(file_path);
	}
	else
	{
		src = gdk_pixbuf_new_from_file(file_path, NULL);
		image_buff = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
		image = gtk_image_new_from_pixbuf(image_buff);
		g_object_unref(src);
	}
	g_free(file_path);
	window->layer_control.merge_down = button = gtk_button_new();
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(MergeDownActiveLayer), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.merge_box), button, FALSE, TRUE, 0);
	file_path = g_build_filename(app->current_path, "image/trash_box.png", NULL);
	if(app->gui_scale == 1.0)
	{
		image = gtk_image_new_from_file(file_path);
	}
	else
	{
		src = gdk_pixbuf_new_from_file(file_path, NULL);
		image_buff = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
		image = gtk_image_new_from_pixbuf(image_buff);
		g_object_unref(src);
	}
	g_free(file_path);
	window->layer_control.delete_layer = button = gtk_button_new();
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(DeleteActiveLayer), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.merge_box), button, FALSE, TRUE, 0);

	// 背景色切り替えボタン
	window->change_bg_button = gtk_toggle_button_new();
	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		gtk_widget_set_sensitive(window->change_bg_button, FALSE);
		window->change_bg_index = app->menus.num_disable_if_no_open;
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = window->change_bg_button;
		app->menus.num_disable_if_no_open++;
	}
	else
	{
		app->menus.disable_if_no_open[window->change_bg_index] = window->change_bg_button;
		gtk_widget_set_sensitive(window->change_bg_button, app->window_num > 0);
	}
	window->change_bg_label = gtk_label_new_with_mnemonic("");
	gtk_container_add(GTK_CONTAINER(window->change_bg_button), window->change_bg_label);
	if(app->window_num > 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window->change_bg_button),
			(app->draw_window[app->active_window]->flags & DRAW_WINDOW_SECOND_BG));
		if((app->draw_window[app->active_window]->flags & DRAW_WINDOW_SECOND_BG) != 0)
		{
			(void)sprintf(mark_up_buff, "<span color=\"red\">%s 2</span>", app->labels->unit.bg);
		}
		else
		{
			(void)sprintf(mark_up_buff, "%s 1", app->labels->unit.bg);
		}
	}
	else
	{
		(void)sprintf(mark_up_buff, "%s 1", app->labels->unit.bg);
	}
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(window->change_bg_label), mark_up_buff);
	(void)g_signal_connect(G_OBJECT(window->change_bg_button), "toggled",
		G_CALLBACK(OnClickedChangeBackGroundButton), app);
	gtk_box_pack_end(GTK_BOX(hbox), window->change_bg_button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), window->layer_control.merge_box, FALSE, FALSE, 0);

	// 新規調整レイヤーボタン
	window->layer_control.new_adjustment = button = gtk_button_new();
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteMakeAdjustmentLayer), app);
	file_path = g_build_filename(app->current_path, "image/new_adjustment.png", NULL);
	if(app->gui_scale == 1.0)
	{
		image = gtk_image_new_from_file(file_path);
	}
	else
	{
		src = gdk_pixbuf_new_from_file(file_path, NULL);
		image_buff = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
		image = gtk_image_new_from_pixbuf(image_buff);
		g_object_unref(src);
	}
	g_free(file_path);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		gtk_widget_set_sensitive(window->layer_control.new_adjustment, FALSE);
		window->layer_control.new_adjustment_index = (uint16)app->menus.num_disable_if_no_open;
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = window->layer_control.new_adjustment;
		app->menus.num_disable_if_no_open++;
	}
	else
	{
		gtk_widget_set_sensitive(window->layer_control.new_adjustment, app->window_num > 0);
		app->menus.disable_if_no_open[window->layer_control.new_adjustment_index] = window->layer_control.new_adjustment;
	}

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		window->layer_control.merge_index = (uint16)app->menus.num_disable_if_single_layer;
		app->menus.disable_if_single_layer[app->menus.num_disable_if_single_layer] =
			window->layer_control.merge_box;
		app->menus.num_disable_if_single_layer++;

		// 複数レイヤーがなければ無効
		gtk_widget_set_sensitive(window->layer_control.merge_box, FALSE);
	}
	else
	{
		app->menus.disable_if_single_layer[window->layer_control.merge_index] =
			window->layer_control.merge_box;
		if(app->window_num > 0)
		{
			gtk_widget_set_sensitive(window->layer_control.merge_box,
				app->draw_window[app->active_window]->num_layer > 1);
		}
	}

	// レイヤー表示部をボックスへ
	gtk_box_pack_start(GTK_BOX(vbox), view, TRUE, TRUE, 0);

	// レイヤー表示用スクロールウィンドウのスクロールバーの位置を設定
	gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(view),
		((window->flags & LAYER_WINDOW_SCROLLBAR_PLACE_LEFT) == 0) ? GTK_CORNER_TOP_LEFT : GTK_CORNER_BOTTOM_RIGHT);
	// レイヤー表示用スクロールウィンドウを記憶
	window->scrolled_window = view;
	
	// レイヤーは縦に並べる
	*box = gtk_vbox_new(FALSE, 0);

	gtk_drag_source_set(
		*box,
		GDK_BUTTON1_MASK,
		g_drag_targets,//&target,
		1,
		GDK_ACTION_PRIVATE
	);
	gtk_drag_dest_set(
		*box,
		GTK_DEST_DEFAULT_ALL,//GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
		g_drag_targets,//&target,
		1,
		GDK_ACTION_PRIVATE
	);
	(void)g_signal_connect(G_OBJECT(*box), "drag-begin",
		G_CALLBACK(LayerWidgetDragBegin), app);
	(void)g_signal_connect(G_OBJECT(*box), "drag-motion",
		G_CALLBACK(DragMotion), app);
	(void)g_signal_connect(G_OBJECT(*box), "drag-end",
		G_CALLBACK(LayerWidgetDragEnd), app);
	(void)g_signal_connect(G_OBJECT(*box), "drag-data-received",
		G_CALLBACK(LayerWidgetDragDataReceived), app);
	(void)g_signal_connect(G_OBJECT(*box), "drag-data-get",
		G_CALLBACK(LayerWidgetDragDataSet), app);
	(void)g_signal_connect(G_OBJECT(*box), "drag-drop",
		G_CALLBACK(LayerWidgetDropData), app);

	gtk_drag_dest_set_track_motion(*box, TRUE);

	// ウィジェットの間隔をセット
	gtk_container_set_border_width(GTK_CONTAINER(view), 0);
	// スクロールバーを表示する条件をセット
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(view),
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC
	);
	// レイヤービューをスクロールドウィンドウに追加
	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(view), *box);

	// チェックボックス用の画像をロード
	file_path = g_build_filename(app->current_path, "image/eye.png", NULL);
	window->eye = gdk_pixbuf_new_from_file(file_path, NULL);
	g_free(file_path);
	file_path = g_build_filename(app->current_path, "image/pin.png", NULL);
	window->pin = gdk_pixbuf_new_from_file(file_path, NULL);
	g_free(file_path);

	if(app->gui_scale != 1.0)
	{
		GdkPixbuf *old_buf;

		old_buf = window->eye;
		window->eye = gdk_pixbuf_scale_simple(old_buf,
			(int)(gdk_pixbuf_get_width(old_buf) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(old_buf) * app->gui_scale),
			GDK_INTERP_NEAREST
		);
		g_object_unref(old_buf);

		old_buf = window->pin;
		window->pin = gdk_pixbuf_scale_simple(old_buf,
			(int)(gdk_pixbuf_get_width(old_buf) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(old_buf) * app->gui_scale),
			GDK_INTERP_NEAREST
		);
		g_object_unref(old_buf);
	}

	// レイヤーセット用の画像をロード
	file_path = g_build_filename(app->current_path, "image/open.png", NULL);
	window->open = gdk_pixbuf_new_from_file(file_path, NULL);
	if(app->gui_scale != 1.0)
	{
		src = window->open;
		window->open = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
		g_object_unref(src);
	}
	g_free(file_path);
	file_path = g_build_filename(app->current_path, "image/close.png", NULL);
	window->close = gdk_pixbuf_new_from_file(file_path, NULL);
	if(app->gui_scale != 1.0)
	{
		src = window->close;
		window->close = gdk_pixbuf_scale_simple(src, (int)(gdk_pixbuf_get_width(src) * app->gui_scale),
			(int)(gdk_pixbuf_get_height(src) * app->gui_scale), GDK_INTERP_NEAREST);
		g_object_unref(src);
	}
	g_free(file_path);

	// ウィジェット削除時のコールバック関数をセット
	(void)g_signal_connect_swapped(G_OBJECT(vbox), "destroy",
		G_CALLBACK(OnDestroyLayerViewWidget), app);

	return vbox;
#undef UI_FONT_SIZE
}

GtkWidget* CreateLayerWindow(APPLICATION* app, GtkWidget *parent, GtkWidget** view)
{
	// レイヤービューのウィンドウ
	GtkWidget *window = NULL;
	// レイヤーサムネイル画像の幅、高さ
	int size;
	// レイヤーサムネイル画像の一行分のバイト数
	int stride;
	// レイヤーサムネイルの背景画像のパターンサイズ
	int step;
	// レイヤーサムネイルの一行分のピクセルデータ
	uint8 *line_pixel_data[2];
	// レイヤーサムネイル用のピクセルへのポインタ
	uint8 *pixels;
	// for文用のカウンタ
	int i, j;

	size = (int)(LAYER_THUMBNAIL_SIZE * app->gui_scale);
	stride = size * 4;
	pixels = app->layer_window.thumb_back_pixels =
		(uint8*)MEM_ALLOC_FUNC(stride * size);
	line_pixel_data[0] = (uint8*)MEM_ALLOC_FUNC(stride);
	line_pixel_data[1] = (uint8*)MEM_ALLOC_FUNC(stride);
	step = size / 8;

	if((app->layer_window.flags & LAYER_WINDOW_DOCKED) == 0 )
	{
		if((window = app->layer_window.window) == NULL)
		{
			// ウィンドウ作成
			window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			// ウィンドウの位置とサイズを設定
			gtk_window_move(GTK_WINDOW(window), app->layer_window.window_x, app->layer_window.window_y);
			gtk_window_resize(GTK_WINDOW(window), app->layer_window.window_width, app->layer_window.window_height);
			// ウィンドウが閉じるときのコールバック関数をセット
			(void)g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(OnCloseLayerWindow), app);
			// 親ウィンドウを登録
			gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent));
			// タイトルをセット
			gtk_window_set_title(GTK_WINDOW(window), app->labels->layer_window.title);
			// 閉じるボタンのみのウィンドウへ
			gtk_window_set_type_hint(GTK_WINDOW(window),
				((app->layer_window.flags & LAYER_WINDOW_POP_UP) == 0) ? GDK_WINDOW_TYPE_HINT_UTILITY : GDK_WINDOW_TYPE_HINT_DOCK);
			// ショートカットキーを登録
			gtk_window_add_accel_group(GTK_WINDOW(window), app->widgets->hot_key);
			// タスクバーには表示しない
			gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

			// キーボードのコールバック関数をセット
			(void)g_signal_connect(G_OBJECT(window), "key-press-event",
				G_CALLBACK(KeyPressEvent), app);
			(void)g_signal_connect(G_OBJECT(window), "key-release-event",
				G_CALLBACK(KeyPressEvent), app);
		}
	}

	// レイヤーのサムネイルの背景作成
	{
		int index;
		gboolean white;
		uint8 pixel_value;

		white = FALSE;
		for(i=0, j=0; i<size; i++, j++)
		{
			if(j >= step)
			{
				white = !white;
				j = 0;
			}
			pixel_value = (white == FALSE) ? 0xff : 0x88;
			line_pixel_data[0][i*4] = pixel_value;
			line_pixel_data[0][i*4+1] = pixel_value;
			line_pixel_data[0][i*4+2] = pixel_value;
			line_pixel_data[0][i*4+3] = 0xff;
		}
		white = TRUE;
		for(i=0, j=0; i<size; i++, j++)
		{
			if(j >= step)
			{
				white = !white;
				j = 0;
			}
			pixel_value = (white == FALSE) ? 0xff : 0x88;
			line_pixel_data[1][i*4] = pixel_value;
			line_pixel_data[1][i*4+1] = pixel_value;
			line_pixel_data[1][i*4+2] = pixel_value;
			line_pixel_data[1][i*4+3] = 0xff;
		}

		white = FALSE;
		for(i=0, j=0, index=1; i<size; i++, j++)
		{
			if(j >= step)
			{
				white = !white;
				index = (white == FALSE) ? 1 : 0;
				j = 0;
			}
			(void)memcpy(&pixels[i*stride],
				line_pixel_data[index], stride);
		}
	}
	app->layer_window.thumb_back = cairo_image_surface_create_for_data(
		pixels, CAIRO_FORMAT_ARGB32, size, size, stride);

	app->layer_window.vbox = CreateLayerView(app, &app->layer_window, view);

#if GTK_MAJOR_VERSION <= 2
	// 拡張デバイスを有効に
	gtk_widget_set_extension_events(app->layer_window.vbox, GDK_EXTENSION_EVENTS_CURSOR);
#endif

	if((app->layer_window.flags & LAYER_WINDOW_DOCKED) == 0)
	{
		// レイヤー用のツリービューを作成して登録
		gtk_container_add(GTK_CONTAINER(window), app->layer_window.vbox);
	}
	else
	{
		gtk_paned_add2(GTK_PANED(app->widgets->navi_layer_pane), app->layer_window.vbox);
	}

	MEM_FREE_FUNC(line_pixel_data[0]);
	MEM_FREE_FUNC(line_pixel_data[1]);

	return window;
}

/***********************************************
* UpadateLayerThumbnail関数                    *
* レイヤーのサムネイルを更新する               *
* 引数                                         *
* widget	: サムネイルの描画領域ウィジェット *
* display	: 描画イベントの情報               *
* layer		: サムネイルを更新するレイヤー     *
***********************************************/
#if GTK_MAJOR_VERSION <= 2
static void UpadateLayerThumbnail(
	GtkWidget* widget,
	GdkEventExpose *display,
	LAYER *layer
)
#else
static void UpadateLayerThumbnail(
	GtkWidget* widget,
	cairo_t* cairo_p,
	LAYER *layer
)
#endif
{
	// 描画用のCairo情報
#if GTK_MAJOR_VERSION <= 2
	//cairo_t *cairo_p = kaburagi_cairo_create((struct _GdkWindow*)widget->window);
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
#endif
	// コピーの拡大率
	gdouble zoom;

	// 背景データをコピー
	cairo_set_source_surface(cairo_p,
		layer->widget->thumb_surface, 0, 0);
	cairo_paint(cairo_p);

	// 拡大率を計算
	if(layer->width > layer->height)
	{
		zoom = ((gdouble)LAYER_THUMBNAIL_SIZE * layer->window->app->gui_scale) / (gdouble)layer->width;
	}
	else
	{
		zoom = ((gdouble)LAYER_THUMBNAIL_SIZE * layer->window->app->gui_scale) / (gdouble)layer->height;
	}

	// 拡大率をセットしてレイヤーのピクセルデータを拡大縮小してサムネイルに書き込む
	cairo_set_operator(cairo_p, CAIRO_OPERATOR_OVER);
	cairo_scale(cairo_p, zoom, zoom);
	cairo_set_source_surface(cairo_p, layer->surface_p, 0, 0);
	cairo_paint(cairo_p);

#if GTK_MAJOR_VERSION <= 2
	// Cairoを破棄
	cairo_destroy(cairo_p);
#endif
}

void LayerViewSetActiveLayer(LAYER* layer, GtkWidget* view)
{
	LAYER* layer_list = layer->window->layer;

	if((layer->window->app->flags & APPLICATION_IN_OPEN_OPERATION) == 0)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(layer->window->app->layer_window.layer_control.mode), layer->layer_mode);
		gtk_adjustment_set_value(layer->window->app->layer_window.layer_control.opacity, layer->alpha);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layer->window->app->layer_window.layer_control.mask_width_under),
			layer->flags & LAYER_MASKING_WITH_UNDER_LAYER);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layer->window->app->layer_window.layer_control.lock_opacity),
			layer->flags & LAYER_LOCK_OPACITY);
	}

	gtk_widget_set_sensitive(layer->window->app->layer_window.layer_control.mask_width_under,
		layer->prev != NULL);
	gtk_widget_set_sensitive(layer->window->app->layer_window.layer_control.merge_down,
		layer->prev != NULL);

	while(layer_list != NULL)
	{
		if(layer_list->widget == NULL)
		{
			LayerViewAddLayer(layer_list, layer_list->window->layer,
				layer_list->window->app->layer_window.view, layer_list->window->num_layer);
			gtk_box_reorder_child(GTK_BOX(layer_list->window->app->layer_window.view),
				layer_list->widget->box, GetLayerID(layer_list->window->layer, layer_list->prev, layer_list->window->num_layer));
			if(layer_list->layer_set != NULL && (layer_list->layer_set->flags & LAYER_SET_CLOSE) != 0)
			{
				gtk_widget_hide(layer_list->widget->box);
			}
		}

		if(layer_list == layer || layer_list == layer->window->active_layer_set)
		{
			GdkColor color;
			gdk_color_parse("#3399FF", &color);

			gtk_widget_modify_bg(layer_list->widget->eye->widget, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(layer_list->widget->pin->widget, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(layer_list->widget->thumbnail, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(layer_list->widget->box, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(layer_list->widget->name, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(layer_list->widget->mode, GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg(layer_list->widget->alpha, GTK_STATE_NORMAL, &color);

			if(layer->layer_set != NULL)
			{
				LAYER *target = layer->layer_set;
				if(target->widget == NULL)
				{
					LayerViewAddLayer(target, target->window->layer,
						target->window->app->layer_window.view, target->window->num_layer);
					gtk_box_reorder_child(GTK_BOX(target->window->app->layer_window.view), target->widget->box,
						GetLayerID(target->window->layer, target->prev, target->window->num_layer));
				}

				gtk_widget_modify_bg(target->widget->eye->widget, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->pin->widget, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->thumbnail, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->box, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->name, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->mode, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_bg(target->widget->alpha, GTK_STATE_NORMAL, &color);
			}
		}
		else
		{
			GtkStyle* style = gtk_widget_get_style(view);
			gtk_widget_modify_bg(layer_list->widget->eye->widget, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(layer_list->widget->pin->widget, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(layer_list->widget->thumbnail, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(layer_list->widget->box, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(layer_list->widget->name, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(layer_list->widget->mode, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
			gtk_widget_modify_bg(layer_list->widget->alpha, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
		}

		layer_list = layer_list->next;
	}
}

static gint LayerWidgetButtonPressCallBack(GtkWidget *widget, GdkEventButton* event, gpointer p)
{
	LAYER* layer = (LAYER*)p;
	if(event->button == 1)
	{
		if((event->state & GDK_CONTROL_MASK) != 0)
		{
			LAYER *before_active = layer->window->active_layer;
			layer->window->active_layer = layer;
			if((event->state & GDK_SHIFT_MASK) != 0)
			{
				LayerAlphaAddSelectionArea(layer->window);
			}
			else
			{
				LayerAlpha2SelectionArea(layer->window);
			}
			layer->window->active_layer = before_active;
		}
		else
		{
			ChangeActiveLayer(layer->window, layer);
			LayerViewSetActiveLayer(layer, layer->window->app->layer_window.view);

			if((layer->window->app->layer_window.flags & LAYER_WINDOW_IN_DRAG_OPERATION) == 0)
			{
				layer->widget->start_x = (gint)event->x;
				layer->widget->start_y = (gint)event->y;
			}
		}
	}
	else if(event->button == 3)
	{
		GtkWidget* menu = gtk_menu_new();
		GtkWidget* menu_item;

		ChangeActiveLayer(layer->window->app->draw_window[layer->window->app->active_window], layer);
		LayerViewSetActiveLayer(layer, layer->window->app->layer_window.view);

		menu_item = gtk_menu_item_new_with_label(
			layer->window->app->labels->layer_window.add_normal);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeColorLayer), layer->window->app);

		menu_item = gtk_menu_item_new_with_label(
			layer->window->app->labels->layer_window.add_vector);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeVectorLayer), layer->window->app);

		menu_item = gtk_menu_item_new_with_label(
			layer->window->app->labels->layer_window.add_layer_set);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeLayerSet), layer->window->app);

		menu_item = gtk_menu_item_new_with_label(
			layer->window->app->labels->layer_window.add_adjustment_layer);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeAdjustmentLayer), layer->window->app);

		if(GetHas3DLayer(layer->window->app) != FALSE)
		{
			menu_item = gtk_menu_item_new_with_label(
				layer->window->app->labels->layer_window.add_3d_modeling);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
			(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
				G_CALLBACK(ExecuteMake3DLayer), layer->window->app);
		}

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->menu.copy_layer);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteCopyLayer), layer->window->app);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->menu.delete_layer);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(DeleteActiveLayer), layer->window->app);

		if(layer->window->num_layer < 2)
		{
			gtk_widget_set_sensitive(menu_item, FALSE);
		}

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->layer_window.alpha_to_select);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(LayerAlpha2SelectionArea), layer->window);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->layer_window.alpha_add_select);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(LayerAlphaAddSelectionArea), layer->window);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
			NULL, event->button, event->time);

		gtk_widget_show_all(menu);
	}

	return FALSE;
}

gint LayerWidgetMotionNotify(GtkWidget* widget, GdkEventMotion* event, gpointer data)
{
	LAYER* layer = (LAYER*)data;

	if((layer->window->app->layer_window.flags & LAYER_WINDOW_IN_DRAG_OPERATION) == 0)
	{
		if(gtk_drag_check_threshold(widget, layer->widget->start_x, layer->widget->start_y,
			(gint)event->x, (gint)event->y) != FALSE
			&& (event->state & GDK_BUTTON1_MASK) != 0)
		{
			g_signal_connect(G_OBJECT(widget), "drag-data-get",
				G_CALLBACK(LayerWidgetDragDataSet), layer);
			g_signal_connect(G_OBJECT(widget), "drag-end",
				G_CALLBACK(LayerWidgetDragEnd), layer);

			layer->window->app->layer_window.flags |= LAYER_WINDOW_IN_DRAG_OPERATION;
			layer->window->app->layer_window.before_layer_set = layer->layer_set;
		}
	}

	return FALSE;
}

static gint LayerViewChangeName(GtkWidget* widget, gpointer p)
{
	const gchar* text = gtk_entry_get_text(GTK_ENTRY(widget));
	GtkWidget* label;
	LAYER* layer = (LAYER*)p;

	if((layer->widget->flags & LAYER_WINDOW_IN_CHANGE_NAME) == 0)
	{
		return FALSE;
	}
	layer->widget->flags &= ~(LAYER_WINDOW_IN_CHANGE_NAME);

	if(strcmp(text, layer->name) != 0 && *text != '\0'
		&& CorrectLayerName(layer->window->app->draw_window[layer->window->app->active_window]->layer, text) != 0)
	{
		gboolean has_parent_window = layer->window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW;
		AddLayerNameChangeHistory(layer->window->app, layer->name, text);
		MEM_FREE_FUNC(layer->name);
		layer->name = MEM_STRDUP_FUNC(text);
		if(has_parent_window != FALSE)
		{
			LAYER *parent_target = SearchLayer(layer->window->focal_window->layer, layer->name);
			if(parent_target != NULL)
			{
				MEM_FREE_FUNC(parent_target->name);
				parent_target->name = MEM_STRDUP_FUNC(text);
			}
		}
	}

	gtk_widget_destroy(widget);
	layer->widget->name = gtk_event_box_new();
	label = gtk_label_new(layer->name);
	gtk_container_add(GTK_CONTAINER(layer->widget->name), label);
	gtk_box_pack_start(GTK_BOX(layer->widget->parameters), layer->widget->name,
		FALSE, TRUE, 0);
	gtk_box_reorder_child(GTK_BOX(layer->widget->parameters),
		layer->widget->name, 0);
	gtk_widget_show_all(layer->widget->parameters);
	gtk_widget_add_events(layer->widget->name,
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK);
	(void)g_signal_connect(G_OBJECT(layer->widget->name), "button_press_event",
		G_CALLBACK(LayerWidgetNameClickedCallBack), layer);
	(void)g_signal_connect(G_OBJECT(layer->widget->name), "motion-notify-event",
		G_CALLBACK(LayerWidgetMotionNotify), layer);

	if(layer == layer->window->app->draw_window[layer->window->app->active_window]->active_layer)
	{
		GdkColor color;
		gdk_color_parse("#3399FF", &color);

		gtk_widget_modify_bg(layer->widget->name, GTK_STATE_NORMAL, &color);
	}

	return FALSE;
}

typedef struct _LAYER_GROUP_TEMPLATE_DETAIL_SETTING
{
	GtkWidget *name;
	GtkWidget *add;
	GtkWidget *type;
} LAYER_GROUP_TEMPLATE_DETAIL_SETTING;

static GtkWidget* LayerGroupSelectLayerType(
	eLAYER_TYPE type,
	APPLICATION* app
)
{
	GtkWidget *combo;

#if GTK_MAJOR_VERSION <= 2
	combo = gtk_combo_box_new_text();
#else
	combo = gtk_combo_box_text_new();
#endif

#if GTK_MAJOR_VERSION <= 2
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
		app->labels->layer_window.normal_layer);
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
		app->labels->layer_window.vector_layer);
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
		app->labels->layer_window.text_layer);
#else
	gtk_combo_box_text_append(GTK_COMBO_BOX(combo),
		app->labels->layer_window.normal_layer);
	gtk_combo_box_text_append(GTK_COMBO_BOX(combo),
		app->labels->layer_window.vector_layer);
	gtk_combo_box_text_append(GTK_COMBO_BOX(combo),
		app->labels->layer_window.text_layer);
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), type);

	return combo;
}

static GtkWidget* LayerGroupLayerSettingWidgetNew(
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING* setting,
	LAYER_GROUP_TEMPLATE_NODE* node,
	gboolean add,
	APPLICATION* app
)
{
	GtkWidget *box;
	GtkWidget *widget;
	char str[128];

	box = gtk_hbox_new(FALSE, 0);

	(void)sprintf(str, "%s : ", app->labels->unit.name);
	widget = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);

	widget = gtk_entry_new();
	if(node->name != NULL)
	{
		gtk_entry_append_text(GTK_ENTRY(widget), node->name);
	}
	setting->name = widget;
	gtk_box_pack_start(GTK_BOX(box), widget, TRUE, TRUE, 0);

	widget = gtk_check_button_new_with_label(app->labels->unit.add);
	if(add != FALSE)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
	}
	setting->add = widget;
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);

	widget = LayerGroupSelectLayerType(node->layer_type, app);
	setting->type = widget;
	gtk_box_pack_start(GTK_BOX(box), widget, TRUE, TRUE, 0);

	return box;
}

static GtkWidget* LayerGroupDetailSettingWidgetNew(
	LAYER_GROUP_TEMPLATE* target,
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING* setting,
	APPLICATION* app
)
{
	LAYER_GROUP_TEMPLATE_NODE **nodes;
	LAYER_GROUP_TEMPLATE_NODE *node;
	GtkWidget *box;
	GtkWidget *widget;
	int i;

	box = gtk_vbox_new(FALSE, 0);

	nodes = (LAYER_GROUP_TEMPLATE_NODE**)MEM_ALLOC_FUNC(sizeof(*nodes)*target->num_layers);
	i = 0;
	node = target->names;
	while(node != NULL)
	{
		nodes[target->num_layers-i-1] = node;
		node = node->next;
		i++;
	}

	for(i=0; i<target->num_layers; i++)
	{
		widget =
			LayerGroupLayerSettingWidgetNew(&setting[target->num_layers-i-1], nodes[i],
				FLAG_CHECK(target->add_flags, target->num_layers-i-1), app
		);
		gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	}

	MEM_FREE_FUNC(nodes);

	return box;
}

static void LayerGroupSelectTemplateChanged(GtkWidget* combo, APPLICATION* app)
{
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING *setting;
	GtkWidget *detail;
	GtkWidget *frame;
	GtkWidget *name_edit;
	GtkWidget *add_layer_set;
	GtkWidget *add_under_active_layer;
	gint active;

	active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	setting = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING*)g_object_get_data(G_OBJECT(combo), "setting-data");
	detail = GTK_WIDGET(g_object_get_data(G_OBJECT(combo), "detail-box"));
	frame = GTK_WIDGET(g_object_get_data(G_OBJECT(combo), "frame"));
	name_edit = GTK_WIDGET(g_object_get_data(G_OBJECT(combo), "name-edit"));
	add_layer_set = GTK_WIDGET(g_object_get_data(G_OBJECT(combo), "add-layer-set"));
	add_under_active_layer = GTK_WIDGET(g_object_get_data(G_OBJECT(combo), "add-under-active"));

	gtk_entry_set_text(GTK_ENTRY(name_edit), app->layer_group_templates.templates[active].group_name);

	setting = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING*)MEM_REALLOC_FUNC(
		setting, sizeof(*setting) * app->layer_group_templates.templates[active].num_layers);

	if(app->layer_group_templates.num_templates > 0)
	{
		gtk_entry_set_text(GTK_ENTRY(name_edit), app->layer_group_templates.templates[active].group_name);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_layer_set),
			app->layer_group_templates.templates[active].flags & LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_under_active_layer),
			app->layer_group_templates.templates[active].flags & LAYER_GROUP_TEMPLATE_FLAG_ADD_UNDER_ACTIVE_LAYER);
	}
	else
	{
		gtk_entry_set_text(GTK_ENTRY(name_edit), "");
	}

	if(detail != NULL)
	{
		gtk_widget_destroy(detail);
	}
	detail = LayerGroupDetailSettingWidgetNew(&app->layer_group_templates.templates[active],
		setting, app);

	g_object_set_data(G_OBJECT(combo), "detail-box", detail);
	g_object_set_data(G_OBJECT(combo), "setting-data", setting);

	gtk_container_add(GTK_CONTAINER(frame), detail);

	gtk_widget_show_all(frame);
}

static int LayerGroupTemplateIsValid(
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING* setting,
	int num_setting,
	int group_id,
	GtkWidget* group_name,
	GtkWidget* window,
	APPLICATION* app
)
{
	GtkWidget *dialog;
	const char *name1, *name2;
	int i, j;

	name1 = gtk_entry_get_text(GTK_ENTRY(group_name));
	if(name1 == NULL || name1[0] == '\0')
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, app->labels->layer_window.layer_group_must_have_name);
		gtk_widget_show_all(dialog);
		(void)gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return FALSE;
	}
	for(i=0; i<app->layer_group_templates.num_templates; i++)
	{
		if(i != group_id && StringCompareIgnoreCase(name1, app->layer_group_templates.templates[i].group_name) == 0)
		{
			dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, app->labels->layer_window.same_group_name, name1);
			gtk_widget_show_all(dialog);
			(void)gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return FALSE;
		}
	}

	for(i=0; i<num_setting; i++)
	{
		name1 = gtk_entry_get_text(GTK_ENTRY(setting[i].name));
		if(name1 == NULL || name1[0] == '\0')
		{
			dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, app->labels->layer_window.same_layer_name);
			gtk_widget_show_all(dialog);
			(void)gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return FALSE;
		}

		for(j=i+1; j<num_setting; j++)
		{
			name2 = gtk_entry_get_text(GTK_ENTRY(setting[j].name));
			if(name2 == NULL || StringCompareIgnoreCase(name1, name2) == 0)
			{
				dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, app->labels->layer_window.same_layer_name);
				gtk_widget_show_all(dialog);
				(void)gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
				return FALSE;
			}
		}
	}

	return TRUE;
}

static int LayerGroupIsValid(
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING* setting,
	int num_setting,
	GtkWidget* group_name,
	GtkWidget* window,
	APPLICATION* app
)
{
	GtkWidget *dialog;
	const char *name1, *name2;
	int num_layer = 0;
	int i, j;

	name1 = gtk_entry_get_text(GTK_ENTRY(group_name));
	if(name1 == NULL || name1[0] == '\0')
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, app->labels->layer_window.layer_group_must_have_name);
		gtk_widget_show_all(dialog);
		(void)gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return FALSE;
	}

	for(i=0; i<num_setting; i++)
	{
		name1 = gtk_entry_get_text(GTK_ENTRY(setting[i].name));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setting[i].add)) != FALSE)
		{
			if(name1 == NULL || name1[0] == '\0')
			{
				dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, app->labels->layer_window.same_layer_name);
				gtk_widget_show_all(dialog);
				(void)gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
				return FALSE;
			}

			for(j=i+1; j<num_setting; j++)
			{
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setting[j].add)) != FALSE)
				{
					name2 = gtk_entry_get_text(GTK_ENTRY(setting[j].name));
					if(name2 == NULL || StringCompareIgnoreCase(name1, name2) == 0)
					{
						dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, app->labels->layer_window.same_layer_name);
						gtk_widget_show_all(dialog);
						(void)gtk_dialog_run(GTK_DIALOG(dialog));
						gtk_widget_destroy(dialog);
						return FALSE;
					}
				}
			}

			num_layer++;
		}
	}

	if(num_layer == 0)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "One or more layers must be selected.");
		gtk_widget_show_all(dialog);
		(void)gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return FALSE;
	}

	return TRUE;
}

static void LayerGroupEditDeleteButtonClicked(GtkWidget* button, GtkWidget* box)
{
	int *num_setting;
	num_setting = (int*)g_object_get_data(G_OBJECT(button), "num_setting");
	if(*num_setting > 0)
	{
		(*num_setting)--;
	}
	gtk_widget_destroy(box);
}

static GtkWidget* LayerGroupEditDetailWidgetNew(
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING* setting,
	LAYER_GROUP_TEMPLATE_NODE* node,
	gboolean add_flag,
	int* num_settting,
	APPLICATION* app
)
{
	GtkWidget *box;
	GtkWidget *widget;
	char str[128];

	box = gtk_hbox_new(FALSE, 0);

	(void)sprintf(str, "%s : ", app->labels->unit.name);
	widget = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);

	widget = gtk_entry_new();
	if(node != NULL && node->name != NULL)
	{
		gtk_entry_append_text(GTK_ENTRY(widget), node->name);
	}
	setting->name = widget;
	gtk_box_pack_start(GTK_BOX(box), widget, TRUE, TRUE, 0);

	widget = gtk_check_button_new_with_label(app->labels->unit.add);
	setting->add = widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), add_flag);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);

	widget = LayerGroupSelectLayerType(
		(node != NULL) ? node->layer_type : TYPE_NORMAL_LAYER, app);
	setting->type = widget;
	gtk_box_pack_start(GTK_BOX(box), widget, TRUE, TRUE, 0);

	widget = gtk_button_new_with_label(app->labels->unit._delete);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(widget), "num_setting", num_settting);
	(void)g_signal_connect(G_OBJECT(widget), "clicked",
		G_CALLBACK(LayerGroupEditDeleteButtonClicked), box);

	return box;
}

static void LayerGroupAddLayerButtonClicked(GtkWidget* button, APPLICATION* app)
{
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING **setting;
	GtkWidget *box;
	GtkWidget *widget;
	int *num_setting;

	setting = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING**)g_object_get_data(
		G_OBJECT(button), "setting");
	num_setting = (int*)g_object_get_data(G_OBJECT(button), "num_setting");
	(*num_setting)++;
	*setting = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING*)MEM_REALLOC_FUNC(
		*setting, sizeof(**setting) * (*num_setting));
	num_setting = (int*)g_object_get_data(G_OBJECT(button), "num_setting");
	box = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "detail"));
	widget = LayerGroupEditDetailWidgetNew(&(*setting)[(*num_setting)-1],
		NULL, TRUE, num_setting, app);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	gtk_widget_show_all(widget);
}

static void LayerGroupAddButtonClicked(GtkWidget* button, APPLICATION* app)
{
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING *setting = NULL;
	GtkWidget *dialog;
	GtkWidget *combo;
	GtkWidget *box;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *detail;
	GtkWidget *name;
	GtkWidget *add_layer_set;
	GtkWidget *add_under_active_layer;
	GtkWidget *add_button;
	GtkWidget *label;
	char str[256];
	int num_setting = 0;
	int i;

	combo = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "select-template"));

	dialog = gtk_dialog_new_with_buttons("Add Layer Group Template",
		GTK_WINDOW(g_object_get_data(G_OBJECT(combo), "dialog")),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL
	);
	box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", app->labels->unit.name);
	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	name = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), name, TRUE, TRUE, 0);
	add_button = gtk_button_new_with_label(app->labels->layer_window.add_normal);
	gtk_box_pack_start(GTK_BOX(hbox), add_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);

	add_layer_set = gtk_check_button_new_with_label(app->labels->layer_window.add_layer_set);
	gtk_box_pack_start(GTK_BOX(box), add_layer_set, FALSE, FALSE, 0);

	add_under_active_layer = gtk_check_button_new_with_label(app->labels->layer_window.add_under_active_layer);
	gtk_box_pack_start(GTK_BOX(box), add_under_active_layer, FALSE, FALSE, 0);

	frame = gtk_frame_new(app->labels->unit.detail);
	detail = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), detail);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);

	g_object_set_data(G_OBJECT(add_button), "detail", detail);
	g_object_set_data(G_OBJECT(add_button), "setting", &setting);
	g_object_set_data(G_OBJECT(add_button), "num_setting", &num_setting);
	(void)g_signal_connect(G_OBJECT(add_button), "clicked",
		G_CALLBACK(LayerGroupAddLayerButtonClicked), app);

	gtk_widget_show_all(dialog);
	while(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		if(LayerGroupTemplateIsValid(setting, num_setting, -1,
			name, dialog, app) != FALSE)
		{
			LAYER_GROUP_TEMPLATE *new_template;
			LAYER_GROUP_TEMPLATE_NODE *node;
			app->layer_group_templates.num_templates++;
			app->layer_group_templates.templates =
				(LAYER_GROUP_TEMPLATE*)MEM_REALLOC_FUNC(app->layer_group_templates.templates,
					sizeof(*new_template) * app->layer_group_templates.num_templates
			);
			new_template = &app->layer_group_templates.templates[app->layer_group_templates.num_templates-1];
			(void)memset(new_template, 0, sizeof(*new_template));
			new_template->group_name = MEM_STRDUP_FUNC(gtk_entry_get_text(GTK_ENTRY(name)));
			new_template->add_flags = (uint8*)MEM_ALLOC_FUNC((num_setting+7)/8);
			(void)memset(new_template->add_flags, 0, (num_setting+7)/8);
			new_template->names = (LAYER_GROUP_TEMPLATE_NODE*)MEM_ALLOC_FUNC(sizeof(*new_template->names)*num_setting);
			new_template->num_layers = num_setting;
			(void)memset(new_template->names, 0, sizeof(*new_template->names)*num_setting);
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_layer_set)) != FALSE)
			{
				new_template->flags |= LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET;
			}
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_under_active_layer)) != FALSE)
			{
				new_template->flags |= LAYER_GROUP_TEMPLATE_FLAG_ADD_UNDER_ACTIVE_LAYER;
			}

			node = new_template->names;
			for(i=0; i<num_setting-1; i++)
			{
				node->next = &new_template->names[i+1];
				node = node->next;
			}
			node = new_template->names;
			for(i=0; i<num_setting; i++, node = node->next)
			{
				node->name = MEM_STRDUP_FUNC(gtk_entry_get_text(GTK_ENTRY(setting[num_setting-i-1].name)));
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setting[num_setting-i-1].add)) != FALSE)
				{
					FLAG_ON(new_template->add_flags, i);
				}
			}

#if GTK_MAJOR_VERSION <= 2
			gtk_combo_box_append_text(GTK_COMBO_BOX(combo), new_template->group_name);
#else
			gtk_combo_box_text_append(GTK_COMBO_BOX(comob), new_template->group_name);
#endif
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), app->layer_group_templates.num_templates-1);

			break;
		}
	}
	gtk_widget_destroy(dialog);
	MEM_FREE_FUNC(setting);
}

static void LayerGroupEditButtonClicked(GtkWidget* button, APPLICATION* app)
{
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING *setting = NULL;
	LAYER_GROUP_TEMPLATE_NODE *node;
	GtkWidget *dialog;
	GtkWidget *combo;
	GtkWidget *box;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *detail;
	GtkWidget *name;
	GtkWidget *add_layer_set;
	GtkWidget *add_under_active_layer;
	GtkWidget *add_button;
	GtkWidget *label;
	char str[256];
	int num_setting = 0;
	int active;
	int i;

	combo = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "select-template"));

	if(app->layer_group_templates.num_templates <= 0 ||
		(active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo))) < 0)
	{
		return;
	}

	dialog = gtk_dialog_new_with_buttons("Add Layer Group Template",
		GTK_WINDOW(g_object_get_data(G_OBJECT(combo), "dialog")),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL
	);
	box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", app->labels->unit.name);
	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	name = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), name, TRUE, TRUE, 0);
	add_button = gtk_button_new_with_label(app->labels->layer_window.add_normal);
	gtk_box_pack_start(GTK_BOX(hbox), add_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);

	add_layer_set = gtk_check_button_new_with_label(app->labels->layer_window.add_layer_set);
	gtk_box_pack_start(GTK_BOX(box), add_layer_set, FALSE, FALSE, 0);

	add_under_active_layer = gtk_check_button_new_with_label(app->labels->layer_window.add_under_active_layer);
	gtk_box_pack_start(GTK_BOX(box), add_under_active_layer, FALSE, FALSE, 0);

	setting = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING*)MEM_ALLOC_FUNC(
		sizeof(*setting)*app->layer_group_templates.templates[active].num_layers);
	frame = gtk_frame_new(app->labels->unit.detail);
	detail = gtk_vbox_new(FALSE, 0);
	node = app->layer_group_templates.templates[active].names;
	i = 0;
	while(node != NULL)
	{
		gtk_box_pack_start(GTK_BOX(detail), LayerGroupEditDetailWidgetNew(&setting[i], node,
			FLAG_CHECK(app->layer_group_templates.templates[active].add_flags, i), &num_setting, app), FALSE, FALSE, 0);
		node = node->next;
		i++;
	}
	gtk_container_add(GTK_CONTAINER(frame), detail);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);

	g_object_set_data(G_OBJECT(add_button), "detail", detail);
	g_object_set_data(G_OBJECT(add_button), "setting", &setting);
	g_object_set_data(G_OBJECT(add_button), "num_setting", &num_setting);
	(void)g_signal_connect(G_OBJECT(add_button), "clicked",
		G_CALLBACK(LayerGroupAddLayerButtonClicked), app);

	gtk_widget_show_all(dialog);
	while(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		if(LayerGroupTemplateIsValid(setting, num_setting, active,
			name, dialog, app) != FALSE)
		{
			LAYER_GROUP_TEMPLATE *target_template;
			target_template = &app->layer_group_templates.templates[active];
			target_template->flags = 0;
			MEM_FREE_FUNC(target_template->group_name);
			target_template->group_name = MEM_STRDUP_FUNC(gtk_entry_get_text(GTK_ENTRY(name)));
			target_template->add_flags = (uint8*)MEM_REALLOC_FUNC(target_template->add_flags, (num_setting+7)/8);
			(void)memset(target_template->add_flags, 0, (num_setting+7)/8);
			target_template->names = (LAYER_GROUP_TEMPLATE_NODE*)MEM_REALLOC_FUNC(target_template->names,
				sizeof(*target_template->names)*num_setting);
			target_template->num_layers = num_setting;
			(void)memset(target_template->names, 0, sizeof(*target_template->names)*num_setting);
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_layer_set)) != FALSE)
			{
				target_template->flags |= LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET;
			}
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_under_active_layer)) != FALSE)
			{
				target_template->flags |= LAYER_GROUP_TEMPLATE_FLAG_ADD_UNDER_ACTIVE_LAYER;
			}

			node = target_template->names;
			for(i=0; i<num_setting-1; i++)
			{
				node->next = &target_template->names[i+1];
				node = node->next;
			}
			node = target_template->names;
			for(i=0; i<num_setting; i++, node = node->next)
			{
				node->name = MEM_STRDUP_FUNC(gtk_entry_get_text(GTK_ENTRY(setting[num_setting-i-1].name)));
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setting[num_setting-i-1].add)) != FALSE)
				{
					FLAG_ON(target_template->add_flags, i);
				}
			}

			break;
		}
	}
	gtk_widget_destroy(dialog);
	MEM_FREE_FUNC(setting);
}

static void LayerGroupDeleteButtonClicked(GtkWidget* button, APPLICATION* app)
{
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING *setting = NULL;
	GtkWidget *combo;
	int delete_id;
	int i;

	combo = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "select-template"));

	if(app->layer_group_templates.num_templates <= 0 ||
		(delete_id = gtk_combo_box_get_active(GTK_COMBO_BOX(combo))) < 0)
	{
		return;
	}

	setting = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING*)g_object_get_data(G_OBJECT(combo), "setting-data");

#if GTK_MAJOR_VERSION <= 2
	gtk_combo_box_remove_text(GTK_COMBO_BOX(combo), delete_id);
#else
	gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(combo), delete_id);
#endif

	for(i=delete_id; i<app->layer_group_templates.num_templates-1; i++)
	{
		setting[i] = setting[i+1];
	}

	app->layer_group_templates.num_templates--;
}

/***********************************************
* ExecuteLayerGroupTemplateWindow関数          *
* レイヤーをまとめて作成する                   *
* 引数                                         *
* app	: アプリケーション全体を管理するデータ *
***********************************************/
void ExecuteLayerGroupTemplateWindow(APPLICATION* app)
{
	GtkWidget *window;
	GtkWidget *box;
	GtkWidget *select_template;
	GtkWidget *name_edit;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *detail;
	GtkWidget *add_button;
	GtkWidget *edit_button;
	GtkWidget *delete_button;
	GtkWidget *add_layer_set;
	GtkWidget *add_under_active_layer;
	LAYER_GROUP_TEMPLATE_DETAIL_SETTING *settings = NULL;
	char str[256];
	int i;

	window = gtk_dialog_new_with_buttons(app->labels->menu.new_layer_group,
		GTK_WINDOW(app->widgets->window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL
	);
	box = gtk_dialog_get_content_area(GTK_DIALOG(window));

	if(app->layer_group_templates.num_templates > 0)
	{
		settings = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING*)MEM_ALLOC_FUNC(
			sizeof(*settings) * app->layer_group_templates.num_templates);
		(void)memset(settings, 0, sizeof(*settings) * app->layer_group_templates.num_templates);
	}
	else
	{
		settings = NULL;
	}

	hbox = gtk_hbox_new(FALSE, 2);
#if GTK_MAJOR_VERSION <= 2
	select_template = gtk_combo_box_new_text();
#else
	select_template = gtk_combo_box_text_new();
#endif
	g_object_set_data(G_OBJECT(select_template), "dialog", window);
	gtk_box_pack_start(GTK_BOX(hbox), select_template, TRUE, FALSE, 2);
	for(i=0; i<app->layer_group_templates.num_templates; i++)
	{
#if GTK_MAJOR_VERSION <= 2
		gtk_combo_box_append_text(GTK_COMBO_BOX(select_template),
			app->layer_group_templates.templates[i].group_name);
#else
		gtk_combo_box_text_append(GTK_COMBO_BOX(select_template),
			app->layer_group_templates.templates[i].group_name);
#endif
	}
	(void)g_signal_connect(G_OBJECT(select_template), "changed",
		G_CALLBACK(LayerGroupSelectTemplateChanged), app);

	add_button = gtk_button_new_with_label(app->labels->unit.add);
	g_object_set_data(G_OBJECT(add_button), "select-template", select_template);
	(void)g_signal_connect(G_OBJECT(add_button), "clicked",
		G_CALLBACK(LayerGroupAddButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(hbox), add_button, FALSE, FALSE, 0);
	edit_button = gtk_button_new_with_label(app->labels->menu.edit);
	g_object_set_data(G_OBJECT(edit_button), "select-template", select_template);
	(void)g_signal_connect(G_OBJECT(edit_button), "clicked",
		G_CALLBACK(LayerGroupEditButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(hbox), edit_button, FALSE, FALSE, 0);
	delete_button = gtk_button_new_with_label(app->labels->unit._delete);
	g_object_set_data(G_OBJECT(delete_button), "select-template", select_template);
	(void)g_signal_connect(G_OBJECT(delete_button), "clicked",
		G_CALLBACK(LayerGroupDeleteButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(hbox), delete_button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(box), hbox, TRUE, FALSE, 1);

	add_layer_set = gtk_check_button_new_with_label(app->labels->layer_window.add_layer_set);
	if(app->layer_group_templates.num_templates > 0 && (app->layer_group_templates.templates->flags & LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET) != 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_layer_set), TRUE);
	}
	gtk_box_pack_start(GTK_BOX(box), add_layer_set, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(select_template), "add-layer-set", add_layer_set);

	add_under_active_layer = gtk_check_button_new_with_label(app->labels->layer_window.add_under_active_layer);
	if(app->layer_group_templates.num_templates > 0 && (app->layer_group_templates.templates->flags & LAYER_GROUP_TEMPLATE_FLAG_ADD_UNDER_ACTIVE_LAYER) != 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_under_active_layer), TRUE);
	}
	gtk_box_pack_start(GTK_BOX(box), add_under_active_layer, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(select_template), "add-under-active", add_under_active_layer);

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", "Group Name");
	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	name_edit = gtk_entry_new();
	if(app->layer_group_templates.num_templates > 0)
	{
		gtk_entry_set_text(GTK_ENTRY(name_edit), app->layer_group_templates.templates->names->name);
	}
	g_object_set_data(G_OBJECT(select_template), "name-edit", name_edit);
	gtk_box_pack_start(GTK_BOX(hbox), name_edit, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);

	frame = gtk_frame_new(app->labels->unit.detail);
	if(app->layer_group_templates.num_templates > 0)
	{
		settings = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING*)MEM_ALLOC_FUNC(
			sizeof(*settings) * app->layer_group_templates.templates->num_layers);
		detail = LayerGroupDetailSettingWidgetNew(
			app->layer_group_templates.templates, settings, app);
	}
	else
	{
		detail = NULL;
	}
	g_object_set_data(G_OBJECT(select_template), "detail-box", detail);
	g_object_set_data(G_OBJECT(select_template), "setting-data", settings);
	g_object_set_data(G_OBJECT(select_template), "frame", frame);
	if(detail != NULL)
	{
		gtk_container_add(GTK_CONTAINER(frame), detail);
	}
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);

	if(app->layer_group_templates.num_templates > 0)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(select_template), 0);
	}

	gtk_widget_show_all(window);
	while(gtk_dialog_run(GTK_DIALOG(window)) == GTK_RESPONSE_OK)
	{
		int active;
		int num_layers;
		active = gtk_combo_box_get_active(GTK_COMBO_BOX(select_template));
		num_layers = app->layer_group_templates.templates[active].num_layers;
		settings = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING*)g_object_get_data(G_OBJECT(select_template), "setting-data");
		if(LayerGroupIsValid(settings, num_layers, name_edit, window, app ) != FALSE)
		{
			DRAW_WINDOW *canvas = GetActiveDrawWindow(app);
			LAYER **added_layers;
			LAYER *prev;
			LAYER *next;
			int num_added;
			LAYER_GROUP_TEMPLATE group = {0};
			LAYER_GROUP_TEMPLATE_NODE *node;
			group.group_name = MEM_STRDUP_FUNC(gtk_entry_get_text(GTK_ENTRY(name_edit)));
			group.add_flags = (uint8*)MEM_ALLOC_FUNC((num_layers+7)/8);
			(void)memset(group.add_flags, 0, (num_layers+7)/8);
			group.names = node = (LAYER_GROUP_TEMPLATE_NODE*)MEM_ALLOC_FUNC(sizeof(*group.names) * num_layers);
			(void)memset(node, 0, sizeof(*node) * num_layers);
			for(i=0; i<num_layers; i++)
			{
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings[i].add)) != FALSE)
				{
					FLAG_ON(group.add_flags, i);
				}
			}
			for(i=0; i<num_layers-1; i++)
			{
				node->next = &group.names[i+1];
				node = node->next;
			}
			node = group.names;
			i = 0;
			while(node != NULL)
			{
				node->name = (char*)MEM_STRDUP_FUNC(gtk_entry_get_text(GTK_ENTRY(settings[i].name)));
				node->layer_type = (eLAYER_TYPE)gtk_combo_box_get_active(
					GTK_COMBO_BOX(settings[i].type));
				node = node->next;
				i++;
			}

			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_layer_set)) != FALSE)
			{
				group.flags |= LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET;
			}
			group.num_layers = num_layers;

			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_under_active_layer)) == FALSE)
			{
				prev = canvas->active_layer;
				next = prev->next;
			}
			else
			{
				prev = canvas->active_layer->prev;
				next = canvas->active_layer;
			}
			added_layers = AddLayerGroupTemplate(&group, group.add_flags,
				prev, next, &num_added);

			for(i=0; i<num_added; i++)
			{
				canvas->num_layer++;
				LayerViewAddLayer(added_layers[i], canvas->layer,
					app->layer_window.view, canvas->num_layer);
			}
			if((canvas->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
			{
				canvas->num_layer += num_layers;
			}
			AddLayerGroupHistory(added_layers, num_added, canvas->active_layer);

			node = group.names;
			while(node != NULL)
			{
				MEM_FREE_FUNC(node->name);
				node = node->next;
			}
			MEM_FREE_FUNC(group.names);
			MEM_FREE_FUNC(group.add_flags);
			MEM_FREE_FUNC(group.group_name);
			MEM_FREE_FUNC(added_layers);

			break;
		}
	}

	gtk_widget_destroy(window);
	settings = (LAYER_GROUP_TEMPLATE_DETAIL_SETTING*)g_object_get_data(G_OBJECT(select_template), "setting-data");
	MEM_FREE_FUNC(settings);
}

/*************************************************
* ClearLayerView関数                             *
* レイヤービューのウィジェットを全て削除する     *
* 引数                                           *
* layer_window	: レイヤービューを持つウィンドウ *
*************************************************/
void ClearLayerView(LAYER_WINDOW* layer_window)
{
	GList *view_list = gtk_container_get_children(GTK_CONTAINER(layer_window->view));
	GList *target = view_list;
	while(target != NULL)
	{
		gtk_widget_destroy(GTK_WIDGET(target->data));
		target = target->next;
	}
	g_list_free(view_list);
}

/*********************************************************
* LayerViewSetDrawWindow関数                             *
* レイヤービューにキャンバスの全てのレイヤーをセットする *
* 引数                                                   *
* layer_window	: レイヤービューを持つウィンドウ         *
* draw_window	: 描画領域                               *
*********************************************************/
void LayerViewSetDrawWindow(LAYER_WINDOW* layer_window, DRAW_WINDOW* draw_window)
{
	// アプリケーション全体を管理するデータ
	APPLICATION *app = draw_window->app;
	// レイヤービューにセットするレイヤー
	LAYER *layer;
	// ウィジェット表示処理用のリスト
	GList *child_widgets;
	GList *target;
	// カウンタ
	int counter;

	for(counter=0, layer=draw_window->layer; layer != NULL; layer = layer->next, counter++)
	{
		LayerViewAddLayer(layer, draw_window->layer,
			layer_window->view, counter+1);
	}

	child_widgets = gtk_container_get_children(GTK_CONTAINER(layer_window->view));
	target = child_widgets;
	for(layer=draw_window->layer; layer != NULL; layer = layer->next)
	{
		if(layer->layer_set == NULL || (layer->layer_set->flags & LAYER_SET_CLOSE) == 0)
		{
			gtk_widget_show_all(GTK_WIDGET(target->data));
		}
		target = target->next;
	}
	g_list_free(child_widgets);

	LayerViewSetActiveLayer(draw_window->active_layer, layer_window->view);
}

gint LayerViewEntryFocusOut(GtkWidget* widget, GdkEvent* event, gpointer p)
{
	return LayerViewChangeName(widget, p);
}

static gboolean LayerWidgetNameClickedCallBack(GtkWidget *widget, GdkEventButton* event, gpointer p)
{
	LAYER* layer = (LAYER*)p;

	if(event->button == 1)
	{
		if((event->state & GDK_CONTROL_MASK) != 0)
		{
			if(event->type == GDK_BUTTON_PRESS)
			{
				LAYER *before_active = layer->window->active_layer;
				layer->window->active_layer = layer;
				if((event->state & GDK_SHIFT_MASK) != 0)
				{
					LayerAlphaAddSelectionArea(layer->window);
				}
				else
				{
					LayerAlpha2SelectionArea(layer->window);
				}
				layer->window->active_layer = before_active;
			}
		}
		else
		{
			if(event->type == GDK_BUTTON_PRESS)
			{
				ChangeActiveLayer(layer->window->app->draw_window[layer->window->app->active_window], layer);
				LayerViewSetActiveLayer(layer, layer->window->app->layer_window.view);

				if((layer->window->app->layer_window.flags & LAYER_WINDOW_IN_DRAG_OPERATION) == 0)
				{
					layer->widget->start_x = (gint)event->x;
					layer->widget->start_y = (gint)event->y;
				}
			}
			else if(event->type == GDK_2BUTTON_PRESS)
			{
				GtkWidget* entry = gtk_entry_new();
				gtk_entry_set_text(GTK_ENTRY(entry), layer->name);
				gtk_entry_set_has_frame(GTK_ENTRY(entry), FALSE);
				gtk_widget_destroy(layer->widget->name);
				gtk_box_pack_start(GTK_BOX(layer->widget->parameters), entry, FALSE, TRUE, 0);
				gtk_box_reorder_child(GTK_BOX(layer->widget->parameters), entry, 0);

				gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);

				(void)g_signal_connect(G_OBJECT(entry), "activate",
					G_CALLBACK(LayerViewChangeName), layer);
				(void)g_signal_connect(G_OBJECT(entry), "focus-out-event",
					G_CALLBACK(LayerViewEntryFocusOut), layer);

				layer->widget->flags |= LAYER_WINDOW_IN_CHANGE_NAME;

				gtk_widget_show_all(layer->widget->parameters);
			}
		}
	}
	else if(event->button == 3)
	{
		GtkWidget* menu = gtk_menu_new();
		GtkWidget* menu_item;

		ChangeActiveLayer(layer->window->app->draw_window[layer->window->app->active_window], layer);
		LayerViewSetActiveLayer(layer, layer->window->app->layer_window.view);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->layer_window.add_normal);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeColorLayer), layer->window->app);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->layer_window.add_vector);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeVectorLayer), layer->window->app);

		menu_item = gtk_menu_item_new_with_label(
			layer->window->app->labels->layer_window.add_layer_set);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeLayerSet), layer->window->app);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->menu.delete_layer);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(DeleteActiveLayer), layer->window->app);

		if(layer->window->num_layer < 2)
		{
			gtk_widget_set_sensitive(menu_item, FALSE);
		}

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->layer_window.alpha_to_select);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(LayerAlpha2SelectionArea), layer->window);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->layer_window.alpha_add_select);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(LayerAlphaAddSelectionArea), layer->window);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
			NULL, event->button, event->time);

		gtk_widget_show_all(menu);
	}

	return FALSE;
}

static gint LayerViewChangeVisible(IMAGE_CHECK_BUTTON* button, void* data)
{
	LAYER* layer = (LAYER*)data;

	if(button->state == 0)
	{
		layer->flags |= LAYER_FLAG_INVISIBLE;
	}
	else
	{
		layer->flags &= ~(LAYER_FLAG_INVISIBLE);
	}

	if(layer->window->app->draw_window[layer->window->app->active_window] != NULL)
	{
		layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}

	return FALSE;
}

static gint LayerViewChangeChain(IMAGE_CHECK_BUTTON* button, void* data)
{
	LAYER* layer = (LAYER*)data;

	if(button->state == 0)
	{
		layer->flags &= ~(LAYER_CHAINED);
	}
	else
	{
		layer->flags |= LAYER_CHAINED;
	}

	return FALSE;
}

static void OnDestroyLayerWidget(LAYER* layer)
{
	MEM_FREE_FUNC(layer->widget);
	layer->widget = NULL;
}

void LayerViewAddLayer(LAYER* layer, LAYER* bottom, GtkWidget* view, uint16 num_layer)
{
	GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *name_label, *mode_label, *alpha_label;
	GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	LAYER* parent = layer->layer_set;
	int32 layer_id;
	int hierarchy = 0;
	char num_str[16];
	int i;

	GtkStyle *style = gtk_widget_get_style(view);

	(void)g_signal_connect_swapped(G_OBJECT(hbox), "destroy",
		G_CALLBACK(OnDestroyLayerWidget), layer);

	while(parent != NULL)
	{
		hierarchy++;
		parent = parent->layer_set;
	}

	if(layer->widget == NULL)
	{
		layer->widget = (LAYER_WIDGET*)MEM_ALLOC_FUNC(sizeof(*layer->widget));
	}
	(void)memset(layer->widget, 0, sizeof(*layer->widget));
	layer->widget->eye = CreateImageCheckButton(
		layer->window->app->layer_window.eye,
		LayerViewChangeVisible,
		layer
	);
	ImageCheckButtonSetState(layer->widget->eye, !(layer->flags & LAYER_FLAG_INVISIBLE));
	layer->widget->pin = CreateImageCheckButton(
		layer->window->app->layer_window.pin,
		LayerViewChangeChain,
		layer
	);
	ImageCheckButtonSetState(layer->widget->pin, layer->flags & LAYER_CHAINED);
	layer->widget->alignment = gtk_alignment_new(0, 0, 1, 1);
	layer->widget->box = gtk_event_box_new();
	layer->widget->parameters = gtk_vbox_new(TRUE, 0);
	layer->widget->thumbnail = gtk_drawing_area_new();
	gtk_widget_set_size_request(layer->widget->thumbnail,
		(int)(LAYER_THUMBNAIL_SIZE * layer->window->app->gui_scale),
		(int)(LAYER_THUMBNAIL_SIZE * layer->window->app->gui_scale)
	);
	layer->widget->thumb_surface = layer->window->app->layer_window.thumb_back;
	layer->widget->name = gtk_event_box_new();
	name_label = gtk_label_new(layer->name);
	gtk_container_add(GTK_CONTAINER(layer->widget->name), name_label);
	gtk_misc_set_alignment(GTK_MISC(name_label), 0, 0);
	layer->widget->mode = gtk_event_box_new();
	mode_label = gtk_label_new(layer->window->app->labels->layer_window.blend_modes[
		layer->layer_mode]);
	gtk_container_add(GTK_CONTAINER(layer->widget->mode), mode_label);
	gtk_misc_set_alignment(GTK_MISC(mode_label), 0, 0);
	layer->widget->alpha = gtk_event_box_new();
	(void)sprintf(num_str, "%d%%", layer->alpha);
	alpha_label = gtk_label_new(num_str);
	gtk_container_add(GTK_CONTAINER(layer->widget->alpha), alpha_label);
	gtk_misc_set_alignment(GTK_MISC(alpha_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(layer->widget->parameters), layer->widget->name,
		FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(layer->widget->parameters), layer->widget->mode,
		FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(layer->widget->parameters), layer->widget->alpha,
		FALSE, TRUE, 0);
	layer->widget->frame = gtk_frame_new(NULL);

	gtk_box_pack_start(GTK_BOX(view), layer->widget->box, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layer->widget->box), layer->widget->alignment);
	gtk_container_add(GTK_CONTAINER(layer->widget->alignment), layer->widget->frame);
	gtk_container_add(GTK_CONTAINER(layer->widget->frame), hbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	if(layer->layer_type == TYPE_LAYER_SET)
	{
		gtk_box_pack_start(GTK_BOX(hbox), CreateLayerSetChildButton(layer),
			FALSE, FALSE, 0);
	}

	gtk_container_add(GTK_CONTAINER(alignment), layer->widget->thumbnail);
	gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), layer->widget->parameters, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(vbox), layer->widget->eye->widget);
	gtk_container_add(GTK_CONTAINER(vbox), layer->widget->pin->widget);

	gtk_widget_show_all(layer->widget->box);
	if(layer->layer_set != NULL)
	{
		if((layer->layer_set->flags & LAYER_SET_CLOSE) != 0)
		{
			gtk_widget_hide(layer->widget->box);
		}
	}

	gtk_widget_add_events(layer->widget->name,
		GDK_BUTTON_PRESS_MASK);// | GDK_BUTTON1_MOTION_MASK);
	(void)g_signal_connect(G_OBJECT(layer->widget->name), "button_press_event",
		G_CALLBACK(LayerWidgetNameClickedCallBack), layer);
	gtk_widget_add_events(layer->widget->box,
		GDK_BUTTON_PRESS_MASK);// | GDK_BUTTON1_MOTION_MASK);
	(void)g_signal_connect(G_OBJECT(layer->widget->box), "button_press_event",
		G_CALLBACK(LayerWidgetButtonPressCallBack), layer);
	gtk_widget_add_events(layer->widget->thumbnail,
		GDK_BUTTON_PRESS_MASK);// | GDK_BUTTON1_MOTION_MASK);
	(void)g_signal_connect(G_OBJECT(layer->widget->thumbnail), "button_press_event",
		G_CALLBACK(LayerWidgetButtonPressCallBack), layer);
#if GTK_MAJOR_VERSION <= 2
	(void)g_signal_connect(G_OBJECT(layer->widget->thumbnail), "expose_event",
		G_CALLBACK(UpadateLayerThumbnail), layer);
#else
	(void)g_signal_connect(G_OBJECT(layer->widget->thumbnail), "draw",
		G_CALLBACK(UpadateLayerThumbnail), layer);
#endif
	gtk_widget_add_events(layer->widget->thumbnail,
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);

	gtk_alignment_set_padding(GTK_ALIGNMENT(layer->widget->alignment),
		0, 0, LAYER_SET_DISPLAY_OFFSET * hierarchy, 0);

	layer_id = GetLayerID(bottom, layer->prev, num_layer);
	if(layer_id != num_layer - 1)
	{
		gtk_box_reorder_child(GTK_BOX(view), layer->widget->box, layer_id);
	}

	if(layer->window->num_layer > 1)
	{
		for(i=0; i<layer->window->app->menus.num_disable_if_single_layer; i++)
		{
			if(layer->window->app->menus.disable_if_single_layer[i] != NULL)
			{
				gtk_widget_set_sensitive(layer->window->app->menus.disable_if_single_layer[i], TRUE);
			}
		}
	}
}

/***************************************
* UpdateLayerThumbnailWidget関数       *
* レイヤーのサムネイル画像を更新する   *
* 引数                                 *
* layer	: サムネイルを更新するレイヤー *
***************************************/
void UpdateLayerThumbnailWidget(LAYER* layer)
{
	gtk_widget_queue_draw(layer->widget->thumbnail);
}

/*****************************************
* ResetLayerView関数                     *
* レイヤー一覧ウィジェットをリセットする *
* 引数                                   *
* canvas	: 表示するキャンバス         *
*****************************************/
void ResetLayerView(DRAW_WINDOW* canvas)
{
	DRAW_WINDOW *window;
	LAYER *layer;
	int i;

	APPLICATION *app = canvas->app;

	ClearLayerView(&app->layer_window);

	//window = (canvas->focal_window == NULL) ? canvas : canvas->focal_window;
	window = canvas;
	layer = window->layer;

	for(i=0; i<window->num_layer && layer != NULL; i++)
	{
		LayerViewAddLayer(layer, window->layer,
			app->layer_window.view, i+1);
		layer = layer->next;
	}
	window->num_layer = (uint16)i;

	LayerViewSetActiveLayer(canvas->active_layer, canvas->app->layer_window.view);
}

#ifdef __cplusplus
}
#endif
