// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "layer_window.h"
#include "application.h"
#include "widgets.h"
#include "memory.h"
#include "drag_and_drop.h"
#include "input.h"

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

	app->menus.disable_if_no_open[app->layer_window.change_bg_index] = NULL;
	app->menus.disable_if_no_open[app->layer_window.layer_control.opacity_index] = NULL;
	app->menus.disable_if_no_open[app->layer_window.layer_control.new_box_index] = NULL;
	app->menus.disable_if_single_layer[app->layer_window.layer_control.merge_index] = NULL;

	app->layer_window.change_bg_button = NULL;

	app->layer_window.window_x = x, app->layer_window.window_y = y;
	app->layer_window.window_width = width, app->layer_window.window_height = height;

	g_object_unref(app->layer_window.eye);
	g_object_unref(app->layer_window.pin);
	g_object_unref(app->layer_window.close);
	g_object_unref(app->layer_window.open);

	app->layer_window.window = NULL;

	return FALSE;
}

static void ChangeLayerBlendModeCallBack(GtkComboBox *combo_box, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_OPEN_OPERATION) == 0)
	{
		GList *container_list = gtk_container_get_children(GTK_CONTAINER(
			app->draw_window[app->active_window]->active_layer->widget->mode));
		GtkWidget* label = GTK_WIDGET(container_list->data);
		app->draw_window[app->active_window]->active_layer->layer_mode =
			gtk_combo_box_get_active(combo_box);
		gtk_label_set_label(GTK_LABEL(label),
			app->labels->layer_window.blend_modes[app->draw_window[app->active_window]->active_layer->layer_mode]);

		app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

		g_list_free(container_list);
	}
}

static void ChangeLayerOpacityCallBack(GtkAdjustment* slider, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_OPEN_OPERATION) == 0)
	{
		GList *container_list = gtk_container_get_children(GTK_CONTAINER(
			app->draw_window[app->active_window]->active_layer->widget->alpha));
		char buff[16];
		app->draw_window[app->active_window]->active_layer->alpha =
			(int8)gtk_adjustment_get_value(slider);
		gtk_adjustment_set_value(slider, app->draw_window[app->active_window]->active_layer->alpha);

		(void)sprintf(buff, "%d%%", app->draw_window[app->active_window]->active_layer->alpha);
		gtk_label_set_label(GTK_LABEL(container_list->data), buff);

		app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

		g_list_free(container_list);
	}
}

static void SetLockLayerOpacity(GtkCheckButton* button, APPLICATION* app)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		app->draw_window[app->active_window]->active_layer->flags &=
			~(LAYER_LOCK_OPACITY);
	}
	else
	{
		app->draw_window[app->active_window]->active_layer->flags |=
			LAYER_LOCK_OPACITY;
	}
}

static void SetLayerMaskingWithUnderLayer(GtkCheckButton* button, APPLICATION* app)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		app->draw_window[app->active_window]->active_layer->flags &=
			~(LAYER_MASKING_WITH_UNDER_LAYER);
	}
	else
	{
		app->draw_window[app->active_window]->active_layer->flags |=
			LAYER_MASKING_WITH_UNDER_LAYER;
	}

	app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

static void LayerViewChangeLayerOrder(
	LAYER_WINDOW* layer_view,
	DRAW_WINDOW* window,
	gint pointer_y
)
{
	LAYER *new_prev = layer_view->drag_prev, *layer = window->layer;
	LAYER *parent = window->active_layer->layer_set;
	int hierarchy = 0;

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

			ChangeLayerOrder(window->active_layer, new_prev,
				&window->layer);

			gtk_box_reorder_child(
				GTK_BOX(layer_view->view),
				window->active_layer->widget->box,
				GetLayerID(window->layer, window->active_layer->prev, window->num_layer)
			);

			while(target != NULL && target->layer_set == window->active_layer)
			{
				ChangeLayerOrder(target, new_prev, &window->layer);

				gtk_box_reorder_child(
					GTK_BOX(layer_view->view),
					target->widget->box,
					GetLayerID(window->layer, target->prev, window->num_layer)
				);

				target = target->prev;
			}
		}
		else
		{
			ChangeLayerOrder(window->active_layer, new_prev,
				&window->layer);

			gtk_box_reorder_child(
				GTK_BOX(layer_view->view),
				window->active_layer->widget->box,
				GetLayerID(window->layer,
					window->active_layer->prev, window->num_layer)
			);
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

	while(parent != NULL)
	{
		hierarchy++;
		parent = parent->layer_set;
	}
	gtk_alignment_set_padding(GTK_ALIGNMENT(window->active_layer->widget->alignment),
		0, 0, LAYER_SET_DISPLAY_OFFSET * hierarchy, 0);
}

gboolean DragTimeOutCallBack(APPLICATION* app)
{
	LAYER *new_prev = NULL, *layer = app->draw_window[app->active_window]->layer;
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
			GetLayerID(app->draw_window[app->active_window]->layer,
				new_prev, app->draw_window[app->active_window]->num_layer) + 1
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
	if(app->window_num == 0)
	{
		return FALSE;
	}

	app->layer_window.flags |= LAYER_WINDOW_IN_DRAG_OPERATION;
	app->layer_window.drag_prev = app->draw_window[app->active_window]->active_layer->prev;
	app->layer_window.drop_layer_set = NULL;
	app->layer_window.before_layer_set = app->draw_window[app->active_window]->active_layer->layer_set;

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
#if MAJOR_VERSION == 1
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
	gboolean drag_success = FALSE;
	gboolean delete_data = FALSE;

#if MAJOR_VERSION == 1
	if(selection != NULL && selection->length > 0)
#else
	if(selection != NULL && gtk_selection_data_get_length > 0)
#endif
	{
#if MAJOR_VERSION == 1
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
				LayerViewChangeLayerOrder(&app->layer_window,
					app->draw_window[app->active_window], y);
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

#if MAJOR_VERSION == 1
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
		gint y;

		gtk_widget_destroy(app->layer_window.drag_position_separator);
		app->layer_window.drag_position_separator = NULL;
		gtk_widget_get_pointer(app->layer_window.view, NULL, &y);
		//gdk_window_get_pointer(app->layer_window.view->window, NULL, &y, NULL);

		LayerViewChangeLayerOrder(&app->layer_window,
			app->draw_window[app->active_window], y);

		app->layer_window.flags &= ~(LAYER_WINDOW_IN_DRAG_OPERATION);

		(void)g_source_remove(app->layer_window.timer_id);
	}
}

static void OnClickedChangeBackGroundButton(GtkWidget* button, APPLICATION* app)
{
	char buff[256];
	gboolean button_state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	if((app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) == 0)
	{
		if(button_state == FALSE)
		{
			(void)memset(app->draw_window[app->active_window]->back_ground,
				0xff, app->draw_window[app->active_window]->pixel_buf_size);
			app->draw_window[app->active_window]->flags &= ~(DRAW_WINDOW_SECOND_BG);
		}
		else
		{
			int i;

			for(i=0; i<app->draw_window[app->active_window]->width * app->draw_window[app->active_window]->height; i++)
			{
				app->draw_window[app->active_window]->back_ground[i*4+0] = app->draw_window[app->active_window]->second_back_ground[0];
				app->draw_window[app->active_window]->back_ground[i*4+1] = app->draw_window[app->active_window]->second_back_ground[1];
				app->draw_window[app->active_window]->back_ground[i*4+2] = app->draw_window[app->active_window]->second_back_ground[2];
				app->draw_window[app->active_window]->back_ground[i*4+3] = 0xff;
			}

			app->draw_window[app->active_window]->flags |= DRAW_WINDOW_SECOND_BG;
		}

		app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
		gtk_widget_queue_draw(app->draw_window[app->active_window]->window);

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

GtkWidget *CreateLayerView(APPLICATION* app, LAYER_WINDOW *window, GtkWidget **box)
{
#define UI_FONT_SIZE 8.0
	//const GtkTargetEntry target = {LAYER_DRAG_STRING, GTK_TARGET_SAME_WIDGET, DRAG_ID_LAYER_WIDGET};
	// ���C���[�\���p�̃X�N���[���h�E�B���h�E
	GtkWidget *view = gtk_scrolled_window_new(NULL, NULL);
	// �������[�h�A�s�����x���Ƃ͐؂蕪����UI�쐬
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0), *hbox = gtk_hbox_new(FALSE, 0);
	// �������[�h�A�s�����x��\��
	GtkWidget *label;
	// �s�����x�̃X���C�_
	GtkWidget *opacity_slider;
	// �{�^���ɕ\������C���[�W
	GtkWidget *image;
	// �V�K���C���[���̃{�^��
	GtkWidget *button;
	// ���C���[�ړ��{�^���p�̉摜�y�щ�]�p
	GdkPixbuf *src, *image_buff;
	// �摜�t�@�C���ւ̃p�X(Mac�΍�)
	gchar *file_path;
	// ���x���̃t�H���g��ݒ肷�邽�߂Ƀ}�[�N�A�b�v�̕�������쐬
	char mark_up_buff[128];
	// for���p�̃J�E���^
	int i;

	// �������[�h�̃��x��
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->layer_window.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	// �������[�h��I������R���{�{�b�N�X���쐬
#if MAJOR_VERSION == 1
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
	// �R���{�{�b�N�X�̒l�ύX���̃R�[���o�b�N�֐���ݒ�
	g_signal_connect(G_OBJECT(window->layer_control.mode), "changed", G_CALLBACK(ChangeLayerBlendModeCallBack), app);

	// �E�B���h�E�쐬���͗L���ȃC���[�W���Ȃ��͂��Ȃ̂Ŗ����ɂ��Ă���
	gtk_widget_set_sensitive(window->layer_control.mode, app->window_num > 0);
	
	// ���x���ƃR���{�{�b�N�X�𐅕��ɕ��ׂē����
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), window->layer_control.mode, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	// �s�����x�̃��x��
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->layer_window.opacity);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	// �s�����x�ݒ�̃X���C�_���쐬
	window->layer_control.opacity = GTK_ADJUSTMENT(gtk_adjustment_new(100, 0, 100, 1, 1, 0));
	opacity_slider = gtk_hscale_new(window->layer_control.opacity);
	// �l�ύX���̃R�[���o�b�N�֐���ݒ�
	g_signal_connect(G_OBJECT(window->layer_control.opacity), "value_changed",
		G_CALLBACK(ChangeLayerOpacityCallBack), app);
	// ���l�\���͍�����
	gtk_scale_set_value_pos(GTK_SCALE(opacity_slider), GTK_POS_LEFT);
	// �[���\���Ȃ�
	gtk_scale_set_digits(GTK_SCALE(opacity_slider), 0);
	// ���x�̓��x���ƃX���C�_�𐅕��ɕ��ׂ�
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), opacity_slider, TRUE, TRUE, 0);

	// �E�B���h�E�쐬���͗L���ȃC���[�W���Ȃ��͂��Ȃ̂Ŗ����ɂ��Ă���
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

	// �s�����x�ی�
	app->layer_window.layer_control.lock_opacity =
		gtk_check_button_new_with_label(app->labels->layer_window.lock_opacity);
	g_signal_connect(G_OBJECT(app->layer_window.layer_control.lock_opacity),
		"toggled", G_CALLBACK(SetLockLayerOpacity), app);
	gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), app->layer_window.layer_control.lock_opacity, FALSE, FALSE, 0);

	// ���̃��C���[�Ń}�X�L���O�`�F�b�N�{�b�N�X
	app->layer_window.layer_control.mask_width_under =
		gtk_check_button_new_with_label(app->labels->layer_window.mask_with_under);
	g_signal_connect(G_OBJECT(app->layer_window.layer_control.mask_width_under),
		"toggled", G_CALLBACK(SetLayerMaskingWithUnderLayer), app);

	// �������C���[���Ȃ���Ζ���
	gtk_widget_set_sensitive(app->layer_window.layer_control.mask_width_under, FALSE);

	gtk_box_pack_start(GTK_BOX(vbox), app->layer_window.layer_control.mask_width_under, FALSE, FALSE, 0);

	// �V�K���C���[�֘A�̃{�^��
	window->layer_control.new_box = gtk_hbox_new(FALSE, 0);
	file_path = g_build_filename(app->current_path, "image/new_document.png", NULL);
	image = gtk_image_new_from_file(file_path);
	g_free(file_path);
	button = gtk_button_new();
	g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteMakeColorLayer), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	file_path = g_build_filename(app->current_path, "image/new_vector.png", NULL);
	image = gtk_image_new_from_file(file_path);
	g_free(file_path);
	button = gtk_button_new();
	g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteMakeVectorLayer), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	file_path = g_build_filename(app->current_path, "image/folder.png", NULL);
	image = gtk_image_new_from_file(file_path);
	g_free(file_path);
	button = gtk_button_new();
	g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteMakeLayerSet), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	file_path = g_build_filename(app->current_path, "image/arrow.png", NULL);
	src = gdk_pixbuf_new_from_file(file_path, NULL);
	image_buff = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_CLOCKWISE);
	g_free(file_path);
	g_object_unref(src);
	window->layer_control.up = button = gtk_button_new();
	image = gtk_image_new_from_pixbuf(image_buff);
	gtk_container_add(GTK_CONTAINER(button), image);
	g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteUpLayer), app);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	gtk_widget_set_sensitive(window->layer_control.up, FALSE);
	src = image_buff;
	window->layer_control.down = button = gtk_button_new();
	image_buff = gdk_pixbuf_flip(src, FALSE);
	image = gtk_image_new_from_pixbuf(image_buff);
	gtk_container_add(GTK_CONTAINER(button), image);
	g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteDownLayer), app);
	gtk_box_pack_start(GTK_BOX(window->layer_control.new_box), button, FALSE, TRUE, 0);
	gtk_widget_set_sensitive(window->layer_control.down, FALSE);
	g_object_unref(src);
	g_object_unref(image_buff);

	gtk_box_pack_start(GTK_BOX(vbox), window->layer_control.new_box, FALSE, FALSE, 0);

	// �E�B���h�E�쐬���͖�����
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

	// ���̃��C���[�ƌ����֘A�̃{�^��
	hbox = gtk_hbox_new(FALSE, 0);
	window->layer_control.merge_box = gtk_hbox_new(FALSE, 0);
	file_path = g_build_filename(app->current_path, "image/merge_down.png", NULL);
	image = gtk_image_new_from_file(file_path);
	g_free(file_path);
	window->layer_control.merge_down = button = gtk_button_new();
	g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(MergeDownActiveLayer), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.merge_box), button, FALSE, TRUE, 0);
	file_path = g_build_filename(app->current_path, "image/trash_box.png", NULL);
	image = gtk_image_new_from_file(file_path);
	g_free(file_path);
	window->layer_control.delete_layer = button = gtk_button_new();
	g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(DeleteActiveLayer), app);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(window->layer_control.merge_box), button, FALSE, TRUE, 0);

	// �w�i�F�؂�ւ��{�^��
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
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		window->layer_control.merge_index = (uint16)app->menus.num_disable_if_single_layer;
		app->menus.disable_if_single_layer[app->menus.num_disable_if_single_layer] =
			window->layer_control.merge_box;
		app->menus.num_disable_if_single_layer++;

		// �������C���[���Ȃ���Ζ���
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

	// ���C���[�\�������{�b�N�X��
	gtk_box_pack_start(GTK_BOX(vbox), view, TRUE, TRUE, 0);

	// ���C���[�\���p�X�N���[���E�B���h�E���L��
	window->scrolled_window = view;
	
	// ���C���[�͏c�ɕ��ׂ�
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
	g_signal_connect(G_OBJECT(*box), "drag-begin",
		G_CALLBACK(LayerWidgetDragBegin), app);
	g_signal_connect(G_OBJECT(*box), "drag-motion",
		G_CALLBACK(DragMotion), app);
	g_signal_connect(G_OBJECT(*box), "drag-end",
		G_CALLBACK(LayerWidgetDragEnd), app);
	g_signal_connect(G_OBJECT(*box), "drag-data-received",
		G_CALLBACK(LayerWidgetDragDataReceived), app);
	g_signal_connect(G_OBJECT(*box), "drag-data-get",
		G_CALLBACK(LayerWidgetDragDataSet), app);
	g_signal_connect(G_OBJECT(*box), "drag-drop",
		G_CALLBACK(LayerWidgetDropData), app);

	gtk_drag_dest_set_track_motion(*box, TRUE);

	// �E�B�W�F�b�g�̊Ԋu���Z�b�g
	gtk_container_set_border_width(GTK_CONTAINER(view), 0);
	// �X�N���[���o�[��\������������Z�b�g
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(view),
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC
	);
	// ���C���[�r���[���X�N���[���h�E�B���h�E�ɒǉ�
	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(view), *box);

	// �`�F�b�N�{�b�N�X�p�̉摜�����[�h
	file_path = g_build_filename(app->current_path, "image/eye.png", NULL);
	window->eye = gdk_pixbuf_new_from_file(file_path, NULL);
	g_free(file_path);
	file_path = g_build_filename(app->current_path, "image/pin.png", NULL);
	window->pin = gdk_pixbuf_new_from_file(file_path, NULL);
	g_free(file_path);

	// ���C���[�Z�b�g�p�̉摜�����[�h
	file_path = g_build_filename(app->current_path, "image/open.png", NULL);
	window->open = gdk_pixbuf_new_from_file(file_path, NULL);
	g_free(file_path);
	file_path = g_build_filename(app->current_path, "image/close.png", NULL);
	window->close = gdk_pixbuf_new_from_file(file_path, NULL);
	g_free(file_path);

	return vbox;
#undef UI_FONT_SIZE
}

GtkWidget *CreateLayerWindow(APPLICATION* app, GtkWidget *parent, GtkWidget** view)
{
	// ���C���[�r���[�̃E�B���h�E
	GtkWidget *window = NULL;
	// ���C���[�T���l�C���摜�̈�s���̃o�C�g��
	int stride = LAYER_THUMBNAIL_SIZE*4;
	// ���C���[�T���l�C���̈�s���̃s�N�Z���f�[�^
	uint8 line_pixel_data[2][LAYER_THUMBNAIL_SIZE*4];
	// ���C���[�T���l�C���p�̃s�N�Z���ւ̃|�C���^
	uint8 *pixels = app->layer_window.thumb_back_pixels;
	// for���p�̃J�E���^
	int i;

	if((app->layer_window.flags & LAYER_WINDOW_DOCKED) == 0)
	{
		// �E�B���h�E�쐬
		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		// �E�B���h�E�̈ʒu�ƃT�C�Y��ݒ�
		gtk_window_move(GTK_WINDOW(window), app->layer_window.window_x, app->layer_window.window_y);
		gtk_window_resize(GTK_WINDOW(window), app->layer_window.window_width, app->layer_window.window_height);
		// �E�B���h�E������Ƃ��̃R�[���o�b�N�֐����Z�b�g
		g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(OnCloseLayerWindow), app);
		// �e�E�B���h�E��o�^
		gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent));
		// �^�C�g�����Z�b�g
		gtk_window_set_title(GTK_WINDOW(window), app->labels->layer_window.title);
		// ����{�^���݂̂̃E�B���h�E��
		gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
		// �V���[�g�J�b�g�L�[��o�^
		gtk_window_add_accel_group(GTK_WINDOW(window), app->hot_key);
		// �^�X�N�o�[�ɂ͕\�����Ȃ�
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

		// �L�[�{�[�h�̃R�[���o�b�N�֐����Z�b�g
		g_signal_connect(G_OBJECT(window), "key-press-event",
			G_CALLBACK(KeyPressEvent), app);
		g_signal_connect(G_OBJECT(window), "key-release-event",
			G_CALLBACK(KeyPressEvent), app);
	}

	// ���C���[�̃T���l�C���̔w�i�쐬
	{
		int index;
		uint8 pixel_value;
		for(i=0; i<LAYER_THUMBNAIL_SIZE; i++)
		{
			pixel_value = (i % 8 < 4) ? 0xff : 0x88;
			line_pixel_data[0][i*4] = pixel_value;
			line_pixel_data[0][i*4+1] = pixel_value;
			line_pixel_data[0][i*4+2] = pixel_value;
			line_pixel_data[0][i*4+3] = 0xff;
		}
		for(i=0; i<LAYER_THUMBNAIL_SIZE; i++)
		{
			pixel_value = (i % 8 < 4) ? 0x88 : 0xff;
			line_pixel_data[1][i*4] = pixel_value;
			line_pixel_data[1][i*4+1] = pixel_value;
			line_pixel_data[1][i*4+2] = pixel_value;
			line_pixel_data[1][i*4+3] = 0xff;
		}

		for(i=0; i<LAYER_THUMBNAIL_SIZE; i++)
		{
			index = (i % 8 < 4) ? 0 : 1;
			(void)memcpy(&pixels[i*stride],
				line_pixel_data[index], stride);
		}
	}
	app->layer_window.thumb_back = cairo_image_surface_create_for_data(
		pixels, CAIRO_FORMAT_ARGB32, LAYER_THUMBNAIL_SIZE, LAYER_THUMBNAIL_SIZE, LAYER_THUMBNAIL_SIZE*4);

	app->layer_window.vbox = CreateLayerView(app, &app->layer_window, view);

#if MAJOR_VERSION == 1
	// �g���f�o�C�X��L����
	gtk_widget_set_extension_events(app->layer_window.vbox, GDK_EXTENSION_EVENTS_CURSOR);
#endif

	if((app->layer_window.flags & LAYER_WINDOW_DOCKED) == 0)
	{
		// ���C���[�p�̃c���[�r���[���쐬���ēo�^
		gtk_container_add(GTK_CONTAINER(window), app->layer_window.vbox);
	}
	else
	{
		gtk_paned_add2(GTK_PANED(app->navi_layer_pane), app->layer_window.vbox);
	}

	return window;
}

/***********************************************
* UpadateLayerThumbnail�֐�                    *
* ���C���[�̃T���l�C�����X�V����               *
* ����                                         *
* widget	: �T���l�C���̕`��̈�E�B�W�F�b�g *
* display	: �`��C�x���g�̏��               *
* layer		: �T���l�C�����X�V���郌�C���[     *
***********************************************/
#if MAJOR_VERSION == 1
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
	// �`��p��Cairo���
#if MAJOR_VERSION == 1
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
#endif
	// �R�s�[�̊g�嗦
	gdouble zoom;

	// �w�i�f�[�^���R�s�[
	cairo_set_source_surface(cairo_p,
		layer->widget->thumb_surface, 0, 0);
	cairo_paint(cairo_p);

	// �g�嗦���v�Z
	if(layer->width > layer->height)
	{
		zoom = (gdouble)LAYER_THUMBNAIL_SIZE / (gdouble)layer->width;
	}
	else
	{
		zoom = (gdouble)LAYER_THUMBNAIL_SIZE / (gdouble)layer->height;
	}

	// �g�嗦���Z�b�g���ă��C���[�̃s�N�Z���f�[�^���g��k�����ăT���l�C���ɏ�������
	cairo_set_operator(cairo_p, CAIRO_OPERATOR_OVER);
	cairo_scale(cairo_p, zoom, zoom);
	cairo_set_source_surface(cairo_p, layer->surface_p, 0, 0);
	cairo_paint(cairo_p);

#if MAJOR_VERSION == 1
	// Cairo��j��
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
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeColorLayer), layer->window->app);

		menu_item = gtk_menu_item_new_with_label(
			layer->window->app->labels->layer_window.add_vector);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeVectorLayer), layer->window->app);

		menu_item = gtk_menu_item_new_with_label(
			layer->window->app->labels->layer_window.add_layer_set);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeLayerSet), layer->window->app);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->menu.copy_layer);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteCopyLayer), layer->window->app);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->menu.delete_layer);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
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
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(LayerAlpha2SelectionArea), layer->window);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->layer_window.alpha_add_select);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
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
		AddLayerNameChangeHistory(layer->window->app, layer->name, text);
		MEM_FREE_FUNC(layer->name);
		layer->name = MEM_STRDUP_FUNC(text);
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
	g_signal_connect(G_OBJECT(layer->widget->name), "button_press_event",
		G_CALLBACK(LayerWidgetNameClickedCallBack), layer);
	g_signal_connect(G_OBJECT(layer->widget->name), "motion-notify-event",
		G_CALLBACK(LayerWidgetMotionNotify), layer);

	if(layer == layer->window->app->draw_window[layer->window->app->active_window]->active_layer)
	{
		GdkColor color;
		gdk_color_parse("#3399FF", &color);

		gtk_widget_modify_bg(layer->widget->name, GTK_STATE_NORMAL, &color);
	}

	return FALSE;
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

				g_signal_connect(G_OBJECT(entry), "activate",
					G_CALLBACK(LayerViewChangeName), layer);
				g_signal_connect(G_OBJECT(entry), "focus-out-event",
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
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeVectorLayer), layer->window->app);

		menu_item = gtk_menu_item_new_with_label(
			layer->window->app->labels->layer_window.add_layer_set);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMakeLayerSet), layer->window->app);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->menu.delete_layer);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
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
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(LayerAlpha2SelectionArea), layer->window);

		menu_item = gtk_menu_item_new_with_mnemonic(
			layer->window->app->labels->layer_window.alpha_add_select);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
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
		layer->window->app->draw_window[layer->window->app->active_window]->flags
			|= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
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

void LayerViewAddLayer(LAYER *layer, LAYER *bottom, GtkWidget *view, uint16 num_layer)
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

	while(parent != NULL)
	{
		hierarchy++;
		parent = parent->layer_set;
	}

	layer->widget = (LAYER_WIDGET*)MEM_ALLOC_FUNC(sizeof(*layer->widget));
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
		LAYER_THUMBNAIL_SIZE, LAYER_THUMBNAIL_SIZE);
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

	gtk_widget_add_events(layer->widget->name,
		GDK_BUTTON_PRESS_MASK);// | GDK_BUTTON1_MOTION_MASK);
	g_signal_connect(G_OBJECT(layer->widget->name), "button_press_event",
		G_CALLBACK(LayerWidgetNameClickedCallBack), layer);
	gtk_widget_add_events(layer->widget->box,
		GDK_BUTTON_PRESS_MASK);// | GDK_BUTTON1_MOTION_MASK);
	g_signal_connect(G_OBJECT(layer->widget->box), "button_press_event",
		G_CALLBACK(LayerWidgetButtonPressCallBack), layer);
	gtk_widget_add_events(layer->widget->thumbnail,
		GDK_BUTTON_PRESS_MASK);// | GDK_BUTTON1_MOTION_MASK);
	g_signal_connect(G_OBJECT(layer->widget->thumbnail), "button_press_event",
		G_CALLBACK(LayerWidgetButtonPressCallBack), layer);
#if MAJOR_VERSION == 1
	g_signal_connect(G_OBJECT(layer->widget->thumbnail), "expose_event",
		G_CALLBACK(UpadateLayerThumbnail), layer);
#else
	g_signal_connect(G_OBJECT(layer->widget->thumbnail), "draw",
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
			gtk_widget_set_sensitive(layer->window->app->menus.disable_if_single_layer[i], TRUE);
		}
	}
}

#ifdef __cplusplus
}
#endif