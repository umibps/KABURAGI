// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include "layer.h"
#include "common_tools.h"
#include "selection_area.h"
#include "application.h"
#include "transform.h"
#include "utils.h"
#include "widgets.h"
#include "display.h"
#include "memory.h"
#include "anti_alias.h"

#ifdef __cplusplus
extern "C" {
#endif

static void CommonToolDummyCallback(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
}

static void CommonToolDummyDisplayCursor(DRAW_WINDOW* window, COMMON_TOOL_CORE* core)
{
}

static void ColorPickerPopupMenuItemClicked(GtkWidget* menu_item, COLOR_CHOOSER* chooser)
{
	int color_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu_item), "color_index"));
	uint8 *color = chooser->color_history[color_index];
	HSV hsv;
	uint8 temp_color[3] = {color[0], color[1], color[2]};

	chooser->color_history[color_index][0] =
		chooser->color_history[0][0];
	chooser->color_history[color_index][1] =
		chooser->color_history[0][1];
	chooser->color_history[color_index][2] =
		chooser->color_history[0][2];
	chooser->color_history[0][0] = temp_color[0];
	chooser->color_history[0][1] = temp_color[1];
	chooser->color_history[0][2] = temp_color[2];

#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
	temp_color[0] = chooser->color_history[0][2];
	temp_color[1] = chooser->color_history[0][1];
	temp_color[2] = chooser->color_history[0][0];
	RGB2HSV_Pixel(temp_color, &hsv);
#else
	RGB2HSV_Pixel(chooser->color_history[0], &hsv);
#endif

	chooser->rgb[0] = chooser->color_history[0][0];
	chooser->rgb[1] = chooser->color_history[0][1];
	chooser->rgb[2] = chooser->color_history[0][2];
	SetColorChooserPoint(chooser, &hsv, FALSE);
}

static void ColorPickerButtonPress(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void * state
)
{
	if(((GdkEventButton*)state)->button == 3
		&& (((GdkEventButton*)state)->state & GDK_CONTROL_MASK) != 0)
	{
#define PICKER_POPUP_ICON_SIZE 32
#define PICKER_POPUP_TABLE_WIDTH 5
		GtkWidget *menu;
		GtkWidget *menu_item;
		GtkWidget *image;
		GdkPixbuf *pixbuf;
		uint8 *pixels;
		int stride;
		int pos_x = 0, pos_y = 0;
		int i, j, k;

		if(window->app->tool_window.color_chooser->num_color_history == 0)
		{
			return;
		}

		menu = gtk_menu_new();

		for(i=0; i<window->app->tool_window.color_chooser->num_color_history; i++)
		{
			pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
				PICKER_POPUP_ICON_SIZE, PICKER_POPUP_ICON_SIZE);
			pixels = gdk_pixbuf_get_pixels(pixbuf);
			stride = gdk_pixbuf_get_rowstride(pixbuf);

			for(j=0; j<PICKER_POPUP_ICON_SIZE; j++)
			{
				for(k=0; k<PICKER_POPUP_ICON_SIZE; k++)
				{
					pixels[j*stride+k*3] = window->app->tool_window.color_chooser->color_history[i][0];
					pixels[j*stride+k*3+1] = window->app->tool_window.color_chooser->color_history[i][1];
					pixels[j*stride+k*3+2] = window->app->tool_window.color_chooser->color_history[i][2];
				}
			}

			menu_item = gtk_menu_item_new();
			g_object_set_data(G_OBJECT(menu_item), "color_index", GINT_TO_POINTER(i));
			(void)g_signal_connect(G_OBJECT(menu_item), "activate",
				G_CALLBACK(ColorPickerPopupMenuItemClicked), window->app->tool_window.color_chooser);
			image = gtk_image_new_from_pixbuf(pixbuf);
			gtk_container_add(GTK_CONTAINER(menu_item), image);
			gtk_menu_attach(GTK_MENU(menu), menu_item, pos_x, pos_x+1, pos_y, pos_y+1);

			pos_x++;
			if(pos_x == PICKER_POPUP_TABLE_WIDTH)
			{
				pos_x = 0;
				pos_y++;
			}

			g_object_unref(pixbuf);
		}

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
			((GdkEventButton*)state)->button, gtk_get_current_event_time());
		gtk_widget_show_all(menu);

		return;
	}

	// �`��̈�O�̃N���b�N�Ȃ�ΏI��
	if(x < 0 || x > window->width || y < 0 || y > window->height)
	{
		return;
	}

	if(((GdkEventButton*)state)->button == 1 || ((GdkEventButton*)state)->button == 3)
	{
		COLOR_PICKER* picker = (COLOR_PICKER*)core->tool_data;
		uint8 color[4];

		switch(picker->mode)
		{
		case COLOR_PICKER_SOURCE_ACTIVE_LAYER:
			(void)memcpy(color, &window->active_layer->pixels[
				(int)y*window->active_layer->stride+(int)x*4], 4);
			if(color[3] == 0)
			{
				return;
			}
			break;
		case COLOR_PICKER_SOURCE_CANVAS:
			(void)memcpy(color, &window->mixed_layer->pixels[
				(int)y*window->mixed_layer->stride+(int)x*4], 4);
			if(color[3] == 0)
			{
				(void)memset(color, 0xff, 3);
			}
			break;
		}

		(void)memcpy(window->app->tool_window.color_chooser->rgb, color, 3);
		
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
		{
			uint8 r = window->app->tool_window.color_chooser->rgb[2];
			window->app->tool_window.color_chooser->rgb[2] =
				window->app->tool_window.color_chooser->rgb[0];
			window->app->tool_window.color_chooser->rgb[0] = r;

			r = color[2];
			color[2] = color[0];
			color[0] = r;
		}
#endif
		RGB2HSV_Pixel(color, &window->app->tool_window.color_chooser->hsv);

		SetColorChooserPoint(window->app->tool_window.color_chooser,
			&window->app->tool_window.color_chooser->hsv, TRUE);
	}
}

#define ColorPickerMotionCallBack CommonToolDummyCallback
#define ColorPickerReleaseCallBack CommonToolDummyCallback
#define ColorPickerDisplayCursor CommonToolDummyDisplayCursor

static void ChangeColorPickerMode(GtkWidget* widget, COLOR_PICKER* picker)
{
	picker->mode = (uint8)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "picker-mode"));
	picker->app->tool_window.color_picker.mode = picker->mode;
}

static GtkWidget* CreateColorPickerDetailUI(APPLICATION* app, void *data)
{
	COLOR_PICKER* picker = (COLOR_PICKER*)data;
	GtkWidget* buttons[2];
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	int i;

	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.active_layer);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.canvas);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[picker->mode]), TRUE);
	for(i=0; i<sizeof(buttons)/sizeof(*buttons); i++)
	{
		gtk_box_pack_start(GTK_BOX(vbox), buttons[i], FALSE, TRUE, 0);
		g_signal_connect(G_OBJECT(buttons[i]), "toggled",
			G_CALLBACK(ChangeColorPickerMode), data);
		g_object_set_data(G_OBJECT(buttons[i]), "picker-mode", GINT_TO_POINTER(i));
	}

	return vbox;
}

static void SelectToolsTransformButtonClicked(GtkWidget* button, APPLICATION* app)
{
	if(app->window_num == 0)
	{
		return;
	}

	if((app->draw_window[app->active_window]->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{
		return;
	}

	ExecuteTransform(app);
}

static void SelectToolsSelectNoneButtonClicked(GtkWidget* button, APPLICATION* app)
{
	if(app->window_num == 0)
	{
		return;
	}

	if((app->draw_window[app->active_window]->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{
		return;
	}

	UnSetSelectionArea(app);
}

static void SelectRectangleButtonPress(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		SELECT_RECTANGLE* select = (SELECT_RECTANGLE*)core->tool_data;
		select->start_x = (int32)x;
		select->start_y = (int32)y;
		select->flags &= ~(SELECT_RECTANGLE_STARTED);

		if((((GdkEventButton*)state)->state & GDK_SHIFT_MASK) != 0)
		{
			select->flags |= SELECT_RECTANGLE_SHIFT_MASK;
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
			{
				select->flags |= SELECT_RECTANGLE_HAS_BEFORE_AREA;
			}
		}
		else
		{
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
			{
				select->flags |= SELECT_RECTANGLE_HAS_BEFORE_DATA;
				select->before_point.min_x = window->selection_area.min_x;
				select->before_point.min_y = window->selection_area.min_y;
				select->before_point.max_x = window->selection_area.max_x;
				select->before_point.max_y = window->selection_area.max_y;
			}

			window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
		}
	}
}

static void SelectRectangleMotion(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		SELECT_RECTANGLE* select = (SELECT_RECTANGLE*)core->tool_data;
		gdouble d = sqrt((select->start_x - x)*(select->start_x - x)
			+(select->start_y - y)*(select->start_y - y));

		if((select->flags & SELECT_RECTANGLE_STARTED) != 0
			|| d >= select->select_start)
		{
			select->x = x, select->y = y;
			select->flags |= SELECT_RECTANGLE_STARTED;
		}
	}
}

static void SelectRectangleRelease(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	SELECT_RECTANGLE* select = (SELECT_RECTANGLE*)core->tool_data;

	if(((GdkEventButton*)state)->button == 1
		&& (select->flags & SELECT_RECTANGLE_STARTED) != 0)
	{
		int32 min_x, max_x, min_y, max_y;
		int32 width, height;
		int32 i;

		if(select->start_x < x)
		{
			min_x = (int32)select->start_x;
			max_x = (int32)x;
		}
		else
		{
			min_x = (int32)x;
			max_x = (int32)select->start_x;
		}

		if(select->start_y < y)
		{
			min_y = (int32)select->start_y;
			max_y = (int32)y;
		}
		else
		{
			min_y = (int32)y;
			max_y = (int32)select->start_y;
		}

		if((select->flags & SELECT_RECTANGLE_HAS_BEFORE_AREA) == 0)
		{
			window->selection_area.min_x = min_x;
			window->selection_area.min_y = min_y;
			window->selection_area.max_x = max_x;
			window->selection_area.max_y = max_y;
		}
		else
		{
			window->selection_area.min_x =
				(window->selection_area.min_x < min_x) ? window->selection_area.min_x : min_x;
			window->selection_area.min_y =
				(window->selection_area.min_y < min_y) ? window->selection_area.min_y : min_y;
			window->selection_area.max_x =
				(window->selection_area.max_x > max_x) ? window->selection_area.max_x : max_x;
			window->selection_area.max_y =
				(window->selection_area.max_y > max_y) ? window->selection_area.max_y : max_y;
		}

		if(min_x < 0)
		{
			min_x = 0;
		}
		else if(min_x >= window->width)
		{
			return;
		}
		if(max_x > window->width)
		{
			max_x = window->width;
		}
		if(min_y < 0)
		{
			min_y = 0;
		}
		else if(min_y >= window->height)
		{
			return;
		}
		if(max_y > window->height)
		{
			max_y = window->height;
		}

		width = max_x - min_x, height = max_y - min_y;

		(void)memcpy(window->temp_layer->pixels, window->selection->pixels, window->width*window->height);

		if((select->flags & SELECT_RECTANGLE_SHIFT_MASK) == 0)
		{
			(void)memset(window->selection->pixels, 0, window->selection->width*window->selection->height);
		}

		for(i=0; i<height; i++)
		{
			(void)memset(
				&window->selection->pixels[(min_y+i)*window->selection->stride+min_x],
				0xff, width);
		}

		if((select->flags & SELECT_RECTANGLE_HAS_BEFORE_DATA) != 0)
		{
			if(min_x > select->before_point.min_x)
			{
				min_x = select->before_point.min_x;
			}
			if(max_x < select->before_point.max_x)
			{
				max_x = select->before_point.max_x;
			}
			if(min_y > select->before_point.min_y)
			{
				min_y = select->before_point.min_y;
			}
			if(max_y < select->before_point.max_y)
			{
				max_y = select->before_point.max_y;
			}
		}

		AddSelectionAreaChangeHistory(
			window, core->name, min_x, min_y, max_x, max_y);

		if((window->flags & DRAW_WINDOW_EDIT_SELECTION) == 0)
		{
			if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == FALSE)
			{
				window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
			}
			else
			{
				window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
			}
		}

		select->flags = 0;
	}
}

static void SelectRectangleStartChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	SELECT_RECTANGLE* select = (SELECT_RECTANGLE*)data;
	select->select_start = (int32)gtk_adjustment_get_value(slider);
	gtk_adjustment_set_value(slider, select->select_start);
}

static void SelectRectangleDisplay(DRAW_WINDOW* window, COMMON_TOOL_CORE* core)
{
	SELECT_RECTANGLE* select = (SELECT_RECTANGLE*)core->tool_data;
	window->effect->layer_mode = LAYER_BLEND_DIFFERENCE;

	if((select->flags & SELECT_RECTANGLE_STARTED) != 0)
	{
		cairo_set_source_rgb(window->effect->cairo_p, 1, 1, 1);
		cairo_set_line_width(window->effect->cairo_p, 1);
		cairo_rectangle(window->effect->cairo_p, select->start_x*window->zoom_rate,
			select->start_y*window->zoom_rate, (select->x - select->start_x)*window->zoom_rate,
			(select->y - select->start_y)*window->zoom_rate);
		cairo_stroke(window->effect->cairo_p);
	}
}

static GtkWidget* CreateSelectRectangleDetailUI(APPLICATION* app, void* data)
{
	SELECT_RECTANGLE* select = (SELECT_RECTANGLE*)data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *button;
	GtkWidget* select_start;
	GtkAdjustment* start_adjust =
		GTK_ADJUSTMENT(gtk_adjustment_new(select->select_start, 0, 32, 1, 1, 0));

	g_signal_connect(G_OBJECT(start_adjust), "value_changed",
		G_CALLBACK(SelectRectangleStartChange), data);
	select_start = SpinScaleNew(start_adjust,
		app->labels->tool_box.select_move_start, 1);
	gtk_box_pack_start(GTK_BOX(vbox), select_start, FALSE, TRUE, 0);

	button = gtk_button_new_with_label(app->labels->menu.transform);
	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(SelectToolsTransformButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	button = gtk_button_new_with_label(app->labels->menu.select_none);
	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(SelectToolsSelectNoneButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	return vbox;
}

static void SelectEclipseGetSelectedArea(
	DRAW_WINDOW* window,
	SELECT_ECLIPSE* select,
	LAYER* selection
)
{
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	FLOAT_T center_x, center_y;
	FLOAT_T width, height;
	FLOAT_T r;
	FLOAT_T scale_x = 1, scale_y = 1;
	int stride;

	stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, window->width);
	surface_p = cairo_image_surface_create_for_data(selection->pixels,
		CAIRO_FORMAT_A8, window->width, window->height, stride);
	(void)memset(selection->pixels, 0, window->height * stride);
	cairo_p = cairo_create(surface_p);

	center_x = (select->selected_area.max_x + select->selected_area.min_x) * 0.5;
	center_y = (select->selected_area.max_y + select->selected_area.min_y) * 0.5;
	width = select->selected_area.max_x - select->selected_area.min_x;
	height = select->selected_area.max_y - select->selected_area.min_y;
	if(width > height)
	{
		scale_y = height / width;
		r = center_x - select->selected_area.min_x;
	}
	else
	{
		scale_x = width / height;
		r = center_y - select->selected_area.min_y;
	}
	cairo_translate(cairo_p, center_x, center_y);
	cairo_scale(cairo_p, scale_x, scale_y);
	cairo_arc(cairo_p, 0, 0, r, 0, 2 * G_PI);
	cairo_fill(cairo_p);

	cairo_destroy(cairo_p);
	cairo_surface_destroy(surface_p);
}

static void SelectEclipseButtonPress(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		SELECT_ECLIPSE* select = (SELECT_ECLIPSE*)core->tool_data;
		select->start_x = x;
		select->start_y = y;

		if((select->flags & SELECT_ECLIPSE_HAS_EDITABLE_AREA) != 0)
		{
			if(x < select->selected_area.min_x
				|| x > select->selected_area.max_x
				|| y < select->selected_area.min_y
				|| y > select->selected_area.max_y
			)
			{
				int32 max_x, min_x, max_y, min_y;

				select->flags &= ~(SELECT_ECLIPSE_HAS_EDITABLE_AREA);

				(void)memcpy(window->temp_layer->pixels, window->selection->pixels,
					window->width * window->height);
				if((select->flags & SELECT_ECLIPSE_SHIFT_MASK) == 0)
				{
					if(select->selected_area.min_x < window->selection_area.min_x)
					{
						min_x = (int32)select->selected_area.min_x;
					}
					else
					{
						min_x = (int32)window->selection_area.min_x;
					}

					if(select->selected_area.max_x > window->selection_area.max_x)
					{
						max_x = (int32)select->selected_area.max_x;
					}
					else
					{
						max_x = (int32)window->selection_area.max_x;
					}

					if(select->selected_area.min_y < window->selection_area.min_y)
					{
						min_y = (int32)select->selected_area.min_y;
					}
					else
					{
						min_y = (int32)window->selection_area.min_y;
					}

					if(select->selected_area.max_y > window->selection_area.max_y)
					{
						max_y = (int32)select->selected_area.max_y;
					}
					else
					{
						max_y = (int32)window->selection_area.max_y;
					}

					(void)memset(window->selection->pixels, 0,
						window->width * window->height);
				}
					// if((select->flags & SELECT_ECLIPSE_SHIFT_MASK) == 0)
				else
				{
					max_x = (int32)select->selected_area.max_x;
					min_x = (int32)select->selected_area.min_x;
					max_y = (int32)select->selected_area.max_y;
					min_y = (int32)select->selected_area.min_y;
				}
				SelectEclipseGetSelectedArea(window, select, window->selection);
				AddSelectionAreaChangeHistory(window, core->name,
					min_x, min_y, max_x, max_y);
				if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == 0)
				{
					window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
				}
				else
				{
					window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
				}
				select->flags &= ~(SELECT_ECLIPSE_STARTED);
				select->move_point = SELECT_ECLIPSE_MOVE_NONE;
			}
			else
			{
				FLOAT_T center_x, center_y;
				center_x = (select->selected_area.max_x + select->selected_area.min_x) * 0.5;
				center_y = (select->selected_area.max_y + select->selected_area.min_y) * 0.5;

				if(sqrt((center_x-x)*(center_x-x)+(center_y-y)*(center_y-y)) <= SELECT_ECLIPSE_MOVE_DISTANCE * window->rev_zoom)
				{
					select->move_point = SELECT_ECLIPSE_MOVE;
				}
				else
				{
					if(x < center_x)
					{
						if(y < center_y)
						{
							select->move_point = SELECT_ECLIPSE_MOVE_LEFT_UP;
						}
						else
						{
							select->move_point = SELECT_ECLIPSE_MOVE_LEFT_DOWN;
						}
					}
					else
					{
						if(y < center_y)
						{
							select->move_point = SELECT_ECLIPSE_MOVE_RIGHT_UP;
						}
						else
						{
							select->move_point = SELECT_ECLIPSE_MOVE_RIGHT_DOWN;
						}
					}
				}
			}
		}

		if((((GdkEventButton*)state)->state & GDK_SHIFT_MASK) != 0)
		{
			select->flags |= SELECT_ECLIPSE_SHIFT_MASK;
			/*if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
			{
				select->flags |= SELECT_RECTANGLE_HAS_BEFORE_AREA;
			}*/
		}
		else
		{
			select->flags &= ~(SELECT_ECLIPSE_SHIFT_MASK);

			window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
		}

		select->before_x = x,	select->before_y = y;
	}
}

static void SelectEclipseMotion(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		SELECT_ECLIPSE* select = (SELECT_ECLIPSE*)core->tool_data;
		gdouble d = sqrt((select->start_x - x)*(select->start_x - x)
			+(select->start_y - y)*(select->start_y - y));

		if(((select->flags & SELECT_ECLIPSE_STARTED) != 0
			|| d >= select->select_start))
		{
			select->flags |= SELECT_ECLIPSE_STARTED;
		}

		if((select->flags & SELECT_ECLIPSE_HAS_EDITABLE_AREA) != 0)
		{
			FLOAT_T dx, dy;
			dx = x - select->before_x,	dy = y - select->before_y;
			switch(select->move_point)
			{
			case SELECT_ECLIPSE_MOVE:
				select->selected_area.min_x += dx;
				select->selected_area.min_y += dy;
				select->selected_area.max_x += dx;
				select->selected_area.max_y += dy;
				break;
			case SELECT_ECLIPSE_MOVE_LEFT_UP:
				select->selected_area.min_x += dx;
				select->selected_area.min_y += dy;
				break;
			case SELECT_ECLIPSE_MOVE_LEFT_DOWN:
				select->selected_area.min_x += dx;
				select->selected_area.max_y += dy;
				break;
			case SELECT_ECLIPSE_MOVE_RIGHT_UP:
				select->selected_area.min_y += dy;
				select->selected_area.max_x += dx;
				break;
			case SELECT_ECLIPSE_MOVE_RIGHT_DOWN:
				select->selected_area.max_x += dx;
				select->selected_area.max_y += dy;
				break;
			}
		}
		else if((select->flags & SELECT_ECLIPSE_STARTED) != 0)
		{
			FLOAT_T dx, dy;
			dx = x - select->start_x;
			dy = y - select->start_y;
			if(dx > 0)
			{
				select->selected_area.min_x = select->start_x;
				select->selected_area.max_x = x;
			}
			else
			{
				select->selected_area.min_x = x;
				select->selected_area.max_x = select->start_x;
			}
			if(dy > 0)
			{
				select->selected_area.min_y = select->start_y;
				select->selected_area.max_y = y;
			}
			else
			{
				select->selected_area.min_y = y;
				select->selected_area.max_y = select->start_y;
			}
		}

		select->before_x = x,	select->before_y = y;
		gtk_widget_queue_draw(window->window);
	}
}

static SelectEclipseButtonRelease(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	SELECT_ECLIPSE *select = (SELECT_ECLIPSE*)core->tool_data;

	if((window->flags & DRAW_WINDOW_TOOL_CHANGING) != 0
		&& (select->flags & SELECT_ECLIPSE_HAS_EDITABLE_AREA) != 0)
	{
		int32 max_x, min_x, max_y, min_y;

		select->flags &= ~(SELECT_ECLIPSE_HAS_EDITABLE_AREA);

		(void)memcpy(window->temp_layer->pixels, window->selection->pixels,
			window->width * window->height);
		if((select->flags & SELECT_ECLIPSE_SHIFT_MASK) == 0)
		{
			if(select->selected_area.min_x < window->selection_area.min_x)
			{
				min_x = (int32)select->selected_area.min_x;
			}
			else
			{
				min_x = (int32)window->selection_area.min_x;
			}

			if(select->selected_area.max_x > window->selection_area.max_x)
			{
				max_x = (int32)select->selected_area.max_x;
			}
			else
			{
				max_x = (int32)window->selection_area.max_x;
			}

			if(select->selected_area.min_y < window->selection_area.min_y)
			{
				min_y = (int32)select->selected_area.min_y;
			}
			else
			{
				min_y = (int32)window->selection_area.min_y;
			}

			if(select->selected_area.max_y > window->selection_area.max_y)
			{
				max_y = (int32)select->selected_area.max_y;
			}
			else
			{
				max_y = (int32)window->selection_area.max_y;
			}

			(void)memset(window->selection->pixels, 0,
				window->width * window->height);
		}
			// if((select->flags & SELECT_ECLIPSE_SHIFT_MASK) == 0)
		else
		{
			max_x = (int32)select->selected_area.max_x;
			min_x = (int32)select->selected_area.min_x;
			max_y = (int32)select->selected_area.max_y;
			min_y = (int32)select->selected_area.min_y;
		}
		SelectEclipseGetSelectedArea(window, select, window->selection);
		AddSelectionAreaChangeHistory(window, core->name,
			min_x, min_y, max_x, max_y);
		if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == 0)
		{
			window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
		}
		else
		{
			window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
		}
		select->flags &= ~(SELECT_ECLIPSE_STARTED);
		select->move_point = SELECT_ECLIPSE_MOVE_NONE;
	}
	else
	{
		if(((GdkEventButton*)state)->button == 1
			&& (select->flags & SELECT_ECLIPSE_STARTED) != 0)
		{
			select->flags &= ~(SELECT_ECLIPSE_STARTED);
			select->flags |= SELECT_ECLIPSE_HAS_EDITABLE_AREA;
			select->move_point = SELECT_ECLIPSE_MOVE_NONE;
		}
	}
}

static void SelectEclipseStartChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	SELECT_RECTANGLE* select = (SELECT_RECTANGLE*)data;
	select->select_start = (int32)gtk_adjustment_get_value(slider);
	gtk_adjustment_set_value(slider, select->select_start);
}

static void SelectEclipseDisplay(DRAW_WINDOW* window, COMMON_TOOL_CORE* core)
{
	SELECT_ECLIPSE* select = (SELECT_ECLIPSE*)core->tool_data;
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	window->effect->layer_mode = LAYER_BLEND_DIFFERENCE;

	surface_p = cairo_image_surface_create_for_data(window->effect->pixels,
		CAIRO_FORMAT_ARGB32, window->effect->width, window->effect->height, window->effect->width * 4);
	cairo_p = cairo_create(surface_p);

	if((select->flags & SELECT_ECLIPSE_HAS_EDITABLE_AREA) != 0)
	{
		FLOAT_T center_x, center_y;
		FLOAT_T r;
		FLOAT_T width, height;
		FLOAT_T x_scale = window->zoom_rate, y_scale = window->zoom_rate;

		width = select->selected_area.max_x - select->selected_area.min_x;
		height = select->selected_area.max_y - select->selected_area.min_y;

		cairo_set_source_rgb(cairo_p, 1, 1, 1);
		cairo_rectangle(
			cairo_p,
			select->selected_area.min_x * window->zoom_rate,
			select->selected_area.min_y * window->zoom_rate,
			width * window->zoom_rate,
			height * window->zoom_rate
		);
		cairo_set_line_width(cairo_p, 1);
		cairo_stroke(cairo_p);

		center_x = (select->selected_area.max_x + select->selected_area.min_x) * 0.5;
		center_y = (select->selected_area.max_y + select->selected_area.min_y) * 0.5;
		if(width > height)
		{
			r = center_x - select->selected_area.min_x;
			y_scale *= (height / width);
		}
		else
		{
			r = center_y - select->selected_area.min_y;
			x_scale *= (width / height);
		}
		center_x *= window->zoom_rate,	center_y *= window->zoom_rate;
		cairo_save(cairo_p);
		cairo_translate(cairo_p, center_x, center_y);
		cairo_scale(cairo_p, x_scale, y_scale);
		cairo_arc(cairo_p, 0, 0, r, 0, G_PI * 2);
		cairo_restore(cairo_p);
		cairo_set_source_rgb(cairo_p, 1, 1, 1);
		cairo_set_line_width(cairo_p, 1);
		cairo_stroke(cairo_p);
	}
	else if((select->flags & SELECT_ECLIPSE_STARTED) != 0)
	{
		FLOAT_T center_x, center_y;
		FLOAT_T r;
		FLOAT_T width, height;
		FLOAT_T x_scale = window->zoom_rate, y_scale = window->zoom_rate;

		center_x = (select->selected_area.max_x + select->selected_area.min_x) * 0.5;
		center_y = (select->selected_area.max_y + select->selected_area.min_y) * 0.5;
		width = select->selected_area.max_x - select->selected_area.min_x;
		height = select->selected_area.max_y - select->selected_area.min_y;
		cairo_save(cairo_p);
		if(width > height)
		{
			r = center_x - select->selected_area.min_x;
			y_scale *= (height / width);
		}
		else
		{
			r = center_y - select->selected_area.min_y;
			x_scale *= (width / height);
		}
		center_x *= window->zoom_rate,	center_y *= window->zoom_rate;
		cairo_translate(cairo_p, center_x, center_y);
		cairo_scale(cairo_p, x_scale, y_scale);
		cairo_arc(cairo_p, 0, 0, r, 0, G_PI * 2);
		cairo_set_source_rgb(cairo_p, 1, 1, 1);
		cairo_set_line_width(cairo_p, 1);
		cairo_stroke(cairo_p);
	}

	cairo_destroy(cairo_p);
	cairo_surface_destroy(surface_p);
}

static GtkWidget* CreateSelectEclipseDetailUI(APPLICATION* app, void* data)
{
	SELECT_ECLIPSE* select = (SELECT_ECLIPSE*)data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *button;
	GtkWidget* select_start;
	GtkAdjustment* start_adjust =
		GTK_ADJUSTMENT(gtk_adjustment_new(select->select_start, 0, 32, 1, 1, 0));

	g_signal_connect(G_OBJECT(start_adjust), "value_changed",
		G_CALLBACK(SelectRectangleStartChange), data);
	select_start = SpinScaleNew(start_adjust,
		app->labels->tool_box.select_move_start, 1);
	gtk_box_pack_start(GTK_BOX(vbox), select_start, FALSE, TRUE, 0);

	button = gtk_button_new_with_label(app->labels->menu.transform);
	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(SelectToolsTransformButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	button = gtk_button_new_with_label(app->labels->menu.select_none);
	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(SelectToolsSelectNoneButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	return vbox;
}

static void FreeSelectButtonPress(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		FREE_SELECT *select = (FREE_SELECT*)core->tool_data;
		select->flags |= FREE_SELECT_STARTED;
		(void)memset(window->brush_buffer, 0, window->pixel_buf_size);

		if(select->cairo_p != NULL)
		{
			cairo_destroy(select->cairo_p);
		}
		if(select->surface_p != NULL)
		{
			cairo_surface_destroy(select->surface_p);
		}

		select->surface_p = cairo_image_surface_create_for_data(
			window->brush_buffer, CAIRO_FORMAT_A8, window->width, window->height, window->width);
		select->cairo_p = cairo_create(select->surface_p);

		select->select_size.min_x = select->select_size.max_x = (int32)x;
		select->select_size.min_y = select->select_size.max_y = (int32)y;

		cairo_move_to(select->cairo_p, x, y);

		if((((GdkEventButton*)state)->state & GDK_SHIFT_MASK) != 0)
		{
			select->flags |= FREE_SELECT_SHIFT_MASK;
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
			{
				select->flags |= FREE_SELECT_HAS_BEFORE_AREA;
			}
		}
		else
		{
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
			{
				select->flags |= FREE_SELECT_HAS_BEFORE_DATA;
				select->before_point.min_x = window->selection_area.min_x;
				select->before_point.min_y = window->selection_area.min_y;
				select->before_point.max_x = window->selection_area.max_x;
				select->before_point.max_y = window->selection_area.max_y;
			}

			window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
		}
	}
}

static void FreeSelectMotion(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		FREE_SELECT *select = (FREE_SELECT*)core->tool_data;

		if((select->flags & FREE_SELECT_STARTED) != 0)
		{
			cairo_line_to(select->cairo_p, x, y);

			if(x < select->select_size.min_x)
			{
				select->select_size.min_x = (int32)x;
			}
			else if(x > select->select_size.max_x)
			{
				select->select_size.max_x = (int32)x;
			}

			if(y < select->select_size.min_y)
			{
				select->select_size.min_y = (int32)y;
			}
			else if(y > select->select_size.max_y)
			{
				select->select_size.max_y = (int32)y;
			}
		}
	}
}

static void FreeSelectButtonRelease(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	FREE_SELECT *select = (FREE_SELECT*)core->tool_data;

	if(((GdkEventButton*)state)->button == 1
		&& (select->flags & FREE_SELECT_STARTED) != 0)
	{
		int32 start_x, start_y;
		int32 width, height;
		int i, j;

		cairo_move_to(select->cairo_p, x, y);

		cairo_close_path(select->cairo_p);
		cairo_fill(select->cairo_p);

		if(x < select->select_size.min_x)
		{
			select->select_size.min_x = (int32)x;
		}
		else if(x > select->select_size.max_x)
		{
			select->select_size.max_x = (int32)x;
		}

		if(y < select->select_size.min_y)
		{
			select->select_size.min_y = (int32)y;
		}
		else if(y > select->select_size.max_y)
		{
			select->select_size.max_y = (int32)y;
		}

		if(select->select_size.min_x > window->width
			|| select->select_size.max_x < 0
			|| select->select_size.min_y > window->height
			|| select->select_size.max_y < 0
		)
		{
			return;
		}

		select->select_size.min_x--;
		select->select_size.min_y--;
		select->select_size.max_x++;
		select->select_size.max_y++;

		if(select->select_size.min_x < 0)
		{
			select->select_size.min_x = 0;
		}
		if(select->select_size.min_y < 0)
		{
			select->select_size.min_y = 0;
		}
		if(select->select_size.max_x > window->width)
		{
			select->select_size.max_x = window->width;
		}
		if(select->select_size.max_y > window->height)
		{
			select->select_size.max_y = window->height;
		}

		start_x = select->select_size.min_x;
		start_y = select->select_size.min_y;
		width = select->select_size.max_x - select->select_size.min_x;
		height = select->select_size.max_y - select->select_size.max_y;

		(void)memcpy(window->temp_layer->pixels, window->selection->pixels,
			window->width * window->height);

		if((select->flags & FREE_SELECT_HAS_BEFORE_AREA) == 0)
		{
			window->selection_area.min_x = select->select_size.min_x;
			window->selection_area.min_y = select->select_size.min_y;
			window->selection_area.max_x = select->select_size.max_x;
			window->selection_area.max_y = select->select_size.max_y;
		}
		else
		{
			if(window->selection_area.min_x > select->select_size.min_x)
			{
				window->selection_area.min_x = select->select_size.min_x;
			}
			if(window->selection_area.min_y > select->select_size.min_y)
			{
				window->selection_area.min_y = select->select_size.min_y;
			}
			if(window->selection_area.max_x < select->select_size.max_x)
			{
				window->selection_area.max_x = select->select_size.max_x;
			}
			if(window->selection_area.max_y < select->select_size.max_y)
			{
				window->selection_area.max_y = select->select_size.max_y;
			}
		}

		if((select->flags & FREE_SELECT_SHIFT_MASK) == 0)
		{
			(void)memcpy(window->selection->pixels, window->brush_buffer,
				window->width * window->height);
		}
		else
		{
			for(i=0; i<height; i++)
			{
				for(j=0; j<width; j++)
				{
					window->selection->pixels[(i+start_y)*window->width+(j+start_x)] =
						(window->selection->pixels[(i+start_y)*window->width+(j+start_x)] > window->brush_buffer[(i+start_y)*window->width+(j+start_x)])
							? window->selection->pixels[(i+start_y)*window->width+(j+start_x)] : window->brush_buffer[(i+start_y)*window->width+(j+start_x)];
				}
			}
		}

		AddSelectionAreaChangeHistory(window, core->name,
			select->select_size.min_x, select->select_size.min_y,
			select->select_size.max_x, select->select_size.max_y
		);

		if((window->flags & DRAW_WINDOW_EDIT_SELECTION) == 0)
		{
			if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == FALSE)
			{
				window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
			}
			else
			{
				window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
			}
		}

		cairo_surface_destroy(select->surface_p);
		select->surface_p = NULL;
		cairo_destroy(select->cairo_p);
		select->cairo_p = NULL;

		select->flags = 0;
	}
}

static void FreeSelectDisplay(DRAW_WINDOW* window, COMMON_TOOL_CORE* core)
{
	FREE_SELECT *select = (FREE_SELECT*)core->tool_data;

	if((select->flags & FREE_SELECT_STARTED) != 0)
	{
		gdouble zoom = window->zoom * 0.01;
		gdouble rev_zoom = 1 / zoom;

		cairo_scale(window->effect->cairo_p, zoom, zoom);
		cairo_set_source_rgb(window->effect->cairo_p, 1, 01, 1);
		cairo_append_path(window->effect->cairo_p,
			cairo_copy_path(select->cairo_p));
		cairo_stroke(window->effect->cairo_p);
		cairo_scale(window->effect->cairo_p, rev_zoom, rev_zoom);
		window->effect->layer_mode = LAYER_BLEND_DIFFERENCE;
	}
}

static GtkWidget* CreateFreeSelectDetailUI(APPLICATION* app, void* data)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *button;

	button = gtk_button_new_with_label(app->labels->menu.transform);
	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(SelectToolsTransformButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	button = gtk_button_new_with_label(app->labels->menu.select_none);
	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(SelectToolsSelectNoneButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	return vbox;
}

void FuzzySelectButtonPress(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	// �`��̈�O�̃N���b�N�Ȃ�ΏI��
	if(x < 0 || x > window->width || y < 0 || y > window->height)
	{
		return;
	}

	if(((GdkEventButton*)state)->button == 1)
	{
		FUZZY_SELECT* fuzzy = (FUZZY_SELECT*)core->tool_data;
		LAYER* target;
		uint8* buff = &window->temp_layer->pixels[window->width*window->height];
		int32 min_x, min_y, max_x, max_y;
		int32 before_min_x, before_min_y, before_max_x, before_max_y;
		int32 flags = 0;
		uint8 channel = (fuzzy->select_mode == FUZZY_SELECT_RGB) ? 3 :
			(fuzzy->select_mode == FUZZY_SELECT_ALPHA) ? 1 : 4;
		int i;

		switch(fuzzy->select_target)
		{
		case FUZZY_SELECT_ACTIVE_LAYER:
			target = window->active_layer;
			break;
		case FUZZY_SELECT_CANVAS:
			target = MixLayerForSave(window);
			break;
		default:
			target = NULL;
		}

		if((((GdkEventButton*)state)->state & GDK_SHIFT_MASK) == 0)
		{
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
			{
				flags |= FUZZY_SELECT_HAS_BEFORE_AREA;
				(void)memcpy(window->temp_layer->pixels, window->selection->pixels, window->width*window->height);
				before_min_x = window->selection_area.min_x;
				before_min_y = window->selection_area.min_y;
				before_max_x = window->selection_area.max_x;
				before_max_y = window->selection_area.max_y;
			}

			(void)memset(window->selection->pixels, 0, window->selection->width*window->selection->height);
			(void)memset(buff, 0, window->selection->width*window->selection->height*2);
		}
		else
		{
			(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
		}

		if(fuzzy->select_mode != FUZZY_SELECT_ALPHA)
		{
			DetectSameColorArea(
				target,
				buff,
				&window->temp_layer->pixels[window->width*window->height*2],
				(int32)x, (int32)y,
				&target->pixels[(int32)y*target->stride + (int32)x*target->channel],
				channel,
				fuzzy->threshold, &min_x, &min_y, &max_x, &max_y,
				(eSELECT_FUZZY_DIRECTION)fuzzy->select_direction
			);
		}
		else
		{
			int8 before_channel = window->mask_temp->channel;

			for(i=0; i<window->width*window->height; i++)
			{
				window->mask_temp->pixels[i] =
					target->pixels[i*4+3];
			}

			window->mask_temp->stride = window->mask_temp->width;
			window->mask_temp->channel = 1;
			DetectSameColorArea(
				window->mask_temp,
				buff,
				&window->temp_layer->pixels[window->width*window->height*2],
				(int32)x, (int32)y,
				&target->pixels[(int32)y*target->stride + (int32)x*target->channel+3],
				channel,
				fuzzy->threshold, &min_x, &min_y, &max_x, &max_y,
				(eSELECT_FUZZY_DIRECTION)fuzzy->select_direction
			);
			window->mask_temp->channel = before_channel;
			window->mask_temp->stride = window->mask_temp->width * before_channel;
		}

		if(fuzzy->extend > 0)
		{
			int8 before_channel = window->mask->channel;

			window->mask->channel = 1;
			(void)memcpy(window->mask->pixels, &window->temp_layer->pixels[window->width*window->height],
				window->width * window->height);
			for(i=0; i<fuzzy->extend; i++)
			{
				ExtendSelectionAreaOneStep(window->mask, window->mask_temp);
				(void)memcpy(window->mask->pixels, window->mask_temp->pixels,
					window->width * window->height);
			}
			(void)memcpy(&window->temp_layer->pixels[window->width*window->height],
				window->mask->pixels, window->width * window->height);
			window->mask->channel = before_channel;
		}
		else if(fuzzy->extend < 0)
		{
			int8 before_channel = window->mask->channel;
			int loop = - fuzzy->extend;

			window->mask->channel = 1;
			(void)memcpy(window->mask->pixels, &window->temp_layer->pixels[window->width*window->height],
				window->width * window->height);
			for(i=0; i<loop; i++)
			{
				ReductSelectionAreaOneStep(window->mask, window->mask_temp);
				(void)memcpy(window->mask->pixels, window->mask_temp->pixels,
					window->width * window->height);
			}
			(void)memcpy(&window->temp_layer->pixels[window->width*window->height],
				window->mask->pixels, window->width * window->height);
			window->mask->channel = before_channel;
		}

		if((fuzzy->flags & FUZZY_SELECT_ANTI_ALIAS) != 0)
		{
			(void)memcpy(&window->temp_layer->pixels[window->width*window->height*3],
				buff, window->width*window->height);
			AntiAlias(
				&window->temp_layer->pixels[window->width*window->height*3],
				buff,
				window->width,
				window->height,
				window->width,
				1
			);
		}

		if(((((GdkEventButton*)state)->state & GDK_SHIFT_MASK) != 0)
			|| (window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{
			(void)memcpy(window->temp_layer->pixels, window->selection->pixels, window->width*window->height);
			for(i=0; i<window->width*window->height; i++)
			{
				if(window->selection->pixels[i] < buff[i])
				{
					window->selection->pixels[i] = buff[i];
				}
			}
		}
		else
		{
			(void)memcpy(window->selection->pixels, buff, window->selection->width*window->selection->height);
		}

		if((((GdkEventButton*)state)->state & GDK_SHIFT_MASK) != 0)
		{
			window->selection_area.min_x =
				(window->selection_area.min_x < min_x) ? window->selection_area.min_x : min_x;
			window->selection_area.min_y =
				(window->selection_area.min_y < min_y) ? window->selection_area.min_y : min_y;
			window->selection_area.max_x =
				(window->selection_area.max_x > max_x) ? window->selection_area.max_x : max_x;
			window->selection_area.max_y =
				(window->selection_area.max_y > max_y) ? window->selection_area.max_y : max_y;
		}
		else
		{
			window->selection_area.min_x = min_x;
			window->selection_area.min_y = min_y;
			window->selection_area.max_x = max_x;
			window->selection_area.max_y = max_y;
		}

		if((flags & FUZZY_SELECT_HAS_BEFORE_AREA) != 0)
		{
			min_x = (min_x < before_min_x) ? min_x : before_min_x;
			min_y = (min_y < before_min_y) ? min_y : before_min_y;
			max_x = (max_x > before_max_x) ? max_x : before_max_x;
			max_y = (max_y > before_max_y) ? max_y : before_max_y;
		}

		AddSelectionAreaChangeHistory(
			window, core->name, min_x, min_y, max_x, max_y);

		if(fuzzy->select_target == FUZZY_SELECT_CANVAS)
		{
			DeleteLayer(&target);
		}

		if((window->flags & DRAW_WINDOW_EDIT_SELECTION) == 0)
		{
			if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == FALSE)
			{
				window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
			}
			else
			{
				window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
			}
		}
	}
}

#define FuzzySelectMotion CommonToolDummyCallback

#define FuzzySelectRelease CommonToolDummyCallback

#define FuzzySelectDisplay CommonToolDummyDisplayCursor

static void FuzzySelectSetModeRGB(GtkWidget* widget, gpointer data)
{
	((FUZZY_SELECT*)data)->select_mode = FUZZY_SELECT_RGB;
}

static void FuzzySelectSetModeRGBA(GtkWidget* widget, gpointer data)
{
	((FUZZY_SELECT*)data)->select_mode = FUZZY_SELECT_RGBA;
}

static void FuzzySelectSetModeAlpha(GtkWidget* widget, gpointer data)
{
	((FUZZY_SELECT*)data)->select_mode = FUZZY_SELECT_ALPHA;
}

static void FuzzySelectChangeThreshold(GtkWidget* widget, gpointer data)
{
	FUZZY_SELECT* fuzzy = (FUZZY_SELECT*)data;
	fuzzy->threshold = (uint16)gtk_adjustment_get_value(GTK_ADJUSTMENT(widget));
	gtk_adjustment_set_value(GTK_ADJUSTMENT(widget), fuzzy->threshold);
}

static void FuzzySelectSetTargetActiveLayer(GtkWidget* widget, gpointer data)
{
	((FUZZY_SELECT*)data)->select_target = FUZZY_SELECT_ACTIVE_LAYER;
}

static void FuzzySelectSetTargetCanvas(GtkWidget* widget, gpointer data)
{
	((FUZZY_SELECT*)data)->select_target = FUZZY_SELECT_CANVAS;
}

static void FuzzySelectSetDetectDirection(GtkWidget* widget, gpointer data)
{
	((FUZZY_SELECT*)data)->select_direction =
		(uint8)g_object_get_data(G_OBJECT(widget), "direction");
}

static void FuzzySelectChangeExtend(GtkAdjustment* slider, FUZZY_SELECT* fuzzy)
{
	fuzzy->extend = (int16)gtk_adjustment_get_value(slider);
}

static void FuzzySelectChangeAntiAlias(GtkToggleButton* button, FUZZY_SELECT* fuzzy)
{
	if(gtk_toggle_button_get_active(button) == FALSE)
	{
		fuzzy->flags &= ~(FUZZY_SELECT_ANTI_ALIAS);
	}
	else
	{
		fuzzy->flags |= FUZZY_SELECT_ANTI_ALIAS;
	}
}

static GtkWidget* CreateFuzzySelectDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	FUZZY_SELECT* fuzzy = (FUZZY_SELECT*)data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* table;
	GtkWidget* label;
	GtkWidget* buttons[3];
	GtkWidget* threshold_scale;
	GtkAdjustment* threshold_adjustment;
	gchar mark_up_buff[256];

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.select.target);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.rgb);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.rgba);
	buttons[2] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.alpha);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[fuzzy->select_mode]), TRUE);
	(void)g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(FuzzySelectSetModeRGB), data);
	(void)g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(FuzzySelectSetModeRGBA), data);
	(void)g_signal_connect(G_OBJECT(buttons[2]), "toggled", G_CALLBACK(FuzzySelectSetModeAlpha), data);

	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[2], FALSE, TRUE, 0);

	threshold_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		fuzzy->threshold, 0, 255, 1, 1, 0));
	(void)g_signal_connect(G_OBJECT(threshold_adjustment), "value_changed",
		G_CALLBACK(FuzzySelectChangeThreshold), data);
	threshold_scale = SpinScaleNew(threshold_adjustment,
		app->labels->tool_box.select.threshold, 0);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), threshold_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.select.area);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.active_layer);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.canvas);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[fuzzy->select_target]), TRUE);
	(void)g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(FuzzySelectSetTargetActiveLayer), data);
	(void)g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(FuzzySelectSetTargetCanvas), data);

	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.select.detect_area);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.area_normal);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.area_large);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[fuzzy->select_direction]), TRUE);
	g_object_set_data(G_OBJECT(buttons[0]), "direction", GINT_TO_POINTER(FUZZY_SELECT_DIRECTION_QUAD));
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(FuzzySelectSetDetectDirection), data);
	g_object_set_data(G_OBJECT(buttons[1]), "direction", GINT_TO_POINTER(FUZZY_SELECT_DIRECTION_OCT));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(FuzzySelectSetDetectDirection), data);

	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);

	threshold_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		fuzzy->extend, -50, 50, 1, 1, 0));
	g_signal_connect(G_OBJECT(threshold_adjustment), "value_changed",
		G_CALLBACK(FuzzySelectChangeExtend), data);
	threshold_scale = SpinScaleNew(threshold_adjustment,
		app->labels->tool_box.extend, 0);
	gtk_box_pack_start(GTK_BOX(vbox), threshold_scale, FALSE, TRUE, 0);

	buttons[0] = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[0]), fuzzy->flags & FUZZY_SELECT_ANTI_ALIAS);
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(FuzzySelectChangeAntiAlias), data);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 3);

	buttons[0] = gtk_button_new_with_label(app->labels->menu.transform);
	g_signal_connect(G_OBJECT(buttons[0]), "clicked",
		G_CALLBACK(SelectToolsTransformButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);

	buttons[0] = gtk_button_new_with_label(app->labels->menu.select_none);
	g_signal_connect(G_OBJECT(buttons[0]), "clicked",
		G_CALLBACK(SelectToolsSelectNoneButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);

	return vbox;
#undef UI_FONT_SIZE
}

/***********************************************
* SelectByColorButtonPress�֐�                 *
* �F��I���̃}�E�X�N���b�N���̃R�[���o�b�N�֐� *
* ����                                         *
* window	: �`��̈�̏��                   *
* x			: �}�E�X��X���W                    *
* y			: �}�E�X��Y���W                    *
* core		: �c�[���̊�{���                 *
* state		: �}�E�X�̏��                     *
***********************************************/
static void SelectByColorButtonPress(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
#define HAS_BEFORE_SELECTION_AREA 1
	// �`��̈�O�̃N���b�N�Ȃ�I��
	if(x < 0 || x > window->width || y < 0 || y > window->height)
	{
		return;
	}

	if(((GdkEventButton*)state)->button == 1)
	{
		// �c�[���̏ڍ׃f�[�^�ɃL���X�g
		SELECT_BY_COLOR* select = (SELECT_BY_COLOR*)core->tool_data;
		// �I��͈͌���Ɏg�p���郌�C���[
		LAYER* target;
		// �I��͈͂��L������o�b�t�@
		uint8* buff = &window->temp_layer->pixels[window->width*window->height];
		// �I��͈͂̍ŏ��ő�̍��W
		int32 min_x, min_y, max_x, max_y;
		// �I��͈͕ύX�O�̍ŏ��ő�̍��W
		int32 before_min_x, before_min_y, before_max_x, before_max_y;
		// �c�[���g�p�O�ɑI��͈͂������Ă���t���O
		int flags = 0;
		// �I��͈͂����肷��`�����l����
		int channel = (select->select_mode == SELECT_BY_RGB) ? 3 : 4;
		// for���p�̃J�E���^
		int i;

		// �I��͈͌���Ɏg�p���郌�C���[��ݒ�
		switch(select->select_target)
		{
		case SELECT_BY_COLOR_ACTIVE_LAYER:
			target = window->active_layer;
			break;
		case SELECT_BY_COLOR_CANVAS:
			target = window->mixed_layer;
			break;
		default:
			target = NULL;
		}

		// �V�t�g�L�[��������Ă��Ȃ���ΈȑO�̑I��͈͂�����
		if((((GdkEventButton*)state)->state & GDK_SHIFT_MASK) == 0)
		{
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
			{	// �ȑO�̑I��͈͂�����Η����f�[�^�쐬�̂��߂�
					// �I��͈͂ƍ��W�̍ŏ��ő�l���L������
				flags = HAS_BEFORE_SELECTION_AREA;
				(void)memcpy(window->temp_layer->pixels, window->selection->pixels,
					window->width*window->height);
				before_min_x = window->selection_area.min_x;
				before_min_y = window->selection_area.min_y;
				before_max_x = window->selection_area.max_x;
				before_max_y = window->selection_area.max_y;
			}

			// ���݂̑I��͈͂ƑI��͈͍쐬�p�̃o�b�t�@��������
			(void)memset(window->selection->pixels, 0,
				window->selection->width*window->selection->height);
			(void)memset(buff, 0,
				window->selection->width*window->selection->height);
		}
		else
		{	// �I��͈͒ǉ����͗����f�[�^�쐬�ɓ���ȑ���s�v
			(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
		}

		// �I��͈͍쐬
		(void)AddSelectionAreaByColor(
			target, buff, &target->pixels[(int32)y*target->stride+(int32)x*target->channel],
				channel, select->threshold, &min_x, &min_y, &max_x, &max_y
		);

		// �쐬�����I��͈͂��R�s�[
		if(((((GdkEventButton*)state)->state & GDK_SHIFT_MASK) != 0)
			|| (window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{	// �ȑO�̑I��͈͂�����Α傫�������c��
			(void)memcpy(window->temp_layer->pixels, window->selection->pixels, window->width*window->height);
			for(i=0; i<window->width*window->height; i++)
			{
				if(window->selection->pixels[i] < buff[i])
				{
					window->selection->pixels[i] = buff[i];
				}
			}
		}
		else
		{
			(void)memcpy(window->selection->pixels, buff, window->selection->width*window->selection->height);
		}

		// �X�V���ꂽ�I��͈͂̍ŏ��ő�̍��W������
		if((((GdkEventButton*)state)->state & GDK_SHIFT_MASK) != 0)
		{
			window->selection_area.min_x =
				(window->selection_area.min_x < min_x) ? window->selection_area.min_x : min_x;
			window->selection_area.min_y =
				(window->selection_area.min_y < min_y) ? window->selection_area.min_y : min_y;
			window->selection_area.max_x =
				(window->selection_area.max_x > max_x) ? window->selection_area.max_x : max_x;
			window->selection_area.max_y =
				(window->selection_area.max_y > max_y) ? window->selection_area.max_y : max_y;
		}
		else
		{
			window->selection_area.min_x = min_x;
			window->selection_area.min_y = min_y;
			window->selection_area.max_x = max_x;
			window->selection_area.max_y = max_y;
		}

		if((flags & HAS_BEFORE_SELECTION_AREA) != 0)
		{
			min_x = (min_x < before_min_x) ? min_x : before_min_x;
			min_y = (min_y < before_min_y) ? min_y : before_min_y;
			max_x = (max_x > before_max_x) ? max_x : before_max_x;
			max_y = (max_y > before_max_y) ? max_y : before_max_y;
		}

		// ������ǉ�
		AddSelectionAreaChangeHistory(
			window, core->name, min_x, min_y, max_x, max_y);

		// �I��͈͕\�����X�V
		if((window->flags & DRAW_WINDOW_EDIT_SELECTION) == 0)
		{
			if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == FALSE)
			{
				window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
			}
			else
			{
				window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
			}
		}
	}
#undef HAS_BEFORE_SELECTION_AREA
}

#define SelectByColorMotion CommonToolDummyCallback

#define SelectByColorRelease CommonToolDummyCallback

#define SelectByColorDisplay CommonToolDummyDisplayCursor

static void SelectByColorSetMode(GtkWidget* widget, SELECT_BY_COLOR* select)
{
	select->select_mode = (uint8)GPOINTER_TO_INT(g_object_get_data(
		G_OBJECT(widget), "select-mode"));
}

static void SelectByColorChangeThreshold(GtkAdjustment* slider, SELECT_BY_COLOR* select)
{
	select->threshold = (uint16)gtk_adjustment_get_value(slider);
	gtk_adjustment_set_value(slider, select->threshold);
}

static void SelectByColorSetTarget(GtkWidget* widget, SELECT_BY_COLOR* select)
{
	select->select_target = (uint8)GPOINTER_TO_INT(g_object_get_data(
		G_OBJECT(widget), "select-target"));
}

static GtkWidget* CreateSelectByColorDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	// 臒l�I���c�[���̏ڍ׃f�[�^�ɃL���X�g
	SELECT_BY_COLOR* select = (SELECT_BY_COLOR*)data;
	// �ڍ�UI������p�b�L���O�{�b�N�X
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	// �E�B�W�F�b�g�𐮗񂷂邽�߂̃e�[�u��
	GtkWidget* table;
	// ���[�h�A�^�[�Q�b�g��I�����郉�W�I�{�^����
		// �X���C�_�ɂ��郉�x��
	GtkWidget* buttons[2];
	// 臒l����p�̃X���C�_
	GtkWidget* threshold_scale;
	// �X���C�_�Ɏg�p����A�W���X�^
	GtkAdjustment* threshold_adjustment;

	// ���[�h�I��p�̃��W�I�̍쐬�ƃf�[�^�A�R�[���o�b�N�֐����Z�b�g
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.rgb);
	g_object_set_data(G_OBJECT(buttons[0]), "select-mode", GINT_TO_POINTER(0));
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(SelectByColorSetMode), data);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.rgba);
	g_object_set_data(G_OBJECT(buttons[1]), "select-mode", GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(SelectByColorSetMode), data);
	// ���݂̃��[�h���Z�b�g
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[select->select_mode]), TRUE);

	// �{�b�N�X�ɒǉ�
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);

	// 臒l�����p�̃X���C�_�쐬
	threshold_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		select->threshold, 0, 255, 1, 1, 0));
	g_signal_connect(G_OBJECT(threshold_adjustment), "value_changed",
		G_CALLBACK(SelectByColorChangeThreshold), data);
	threshold_scale = SpinScaleNew(threshold_adjustment,
		app->labels->tool_box.select.threshold, 0);
	// �e�[�u���ɐ���
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), threshold_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �I��͈͌���̃^�[�Q�b�g����p���W�I�{�^���̍쐬
		// �f�[�^�ƃR�[���o�b�N�֐����Z�b�g
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.active_layer);
	g_object_set_data(G_OBJECT(buttons[0]), "select-target", GINT_TO_POINTER(0));
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(SelectByColorSetTarget), data);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.canvas);
	g_object_set_data(G_OBJECT(buttons[1]), "select-target", GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(SelectByColorSetTarget), data);
	// ���݂̃^�[�Q�b�g���Z�b�g
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[select->select_target]), TRUE);

	// �{�b�N�X�ɒǉ�
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);

	buttons[0] = gtk_button_new_with_label(app->labels->menu.transform);
	g_signal_connect(G_OBJECT(buttons[0]), "clicked",
		G_CALLBACK(SelectToolsTransformButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);

	buttons[0] = gtk_button_new_with_label(app->labels->menu.select_none);
	g_signal_connect(G_OBJECT(buttons[0]), "clicked",
		G_CALLBACK(SelectToolsSelectNoneButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);

	return vbox;
#undef UI_FONT_SIZE
}

/*****************************************************
* HandToolButtonPress�֐�                            *
* ��̂Ђ�c�[���̃}�E�X�N���b�N���̃R�[���o�b�N�֐� *
* ����                                               *
* window	: �`��̈�̏��                         *
* x			: �}�E�X��X���W                          *
* y			: �}�E�X��Y���W                          *
* core		: �c�[���̊�{���                       *
* state		: �}�E�X�̏��                           *
*****************************************************/
static void HandToolButtonPress(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		HAND_TOOL *hand = (HAND_TOOL*)core->tool_data;

		hand->before_x = ((x-window->width/2)*window->cos_value + (y - window->height/2)*window->sin_value)
			* window->zoom_rate + window->rev_add_cursor_x;
		hand->before_y = (- (x-window->width/2)*window->sin_value + (y-window->height/2)*window->cos_value)
			* window->zoom_rate + window->rev_add_cursor_y;
	}
}

/*********************************************************
* HandToolMotion�֐�                                     *
* ��̂Ђ�c�[���̃}�E�X�J�[�\���ړ����̃R�[���o�b�N�֐� *
* ����                                                   *
* window	: �`��̈�̏��                             *
* x			: �}�E�X��X���W                              *
* y			: �}�E�X��Y���W                              *
* core		: �c�[���̊�{���                           *
* state		: �}�E�X�̏��                               *
*********************************************************/
static void HandToolMotion(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	COMMON_TOOL_CORE* core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		HAND_TOOL *hand = (HAND_TOOL*)core->tool_data;
		GtkWidget *scroll_bar;
		gdouble real_x, real_y;
		gdouble now_x, now_y;

		real_x = ((x-window->width/2)*window->cos_value + (y - window->height/2)*window->sin_value)
			* window->zoom_rate + window->rev_add_cursor_x;
		real_y = (- (x-window->width/2)*window->sin_value + (y-window->height/2)*window->cos_value)
			* window->zoom_rate + window->rev_add_cursor_y;

		// �`��̈�̃X�N���[���̍��W��ύX
		scroll_bar = gtk_scrolled_window_get_hscrollbar(
			GTK_SCROLLED_WINDOW(window->scroll));
		now_x = gtk_range_get_value(GTK_RANGE(scroll_bar));
		gtk_range_set_value(GTK_RANGE(scroll_bar), now_x + (hand->before_x - real_x));

		scroll_bar = gtk_scrolled_window_get_vscrollbar(
			GTK_SCROLLED_WINDOW(window->scroll));
		now_y = gtk_range_get_value(GTK_RANGE(scroll_bar));
		gtk_range_set_value(GTK_RANGE(scroll_bar), now_y + (hand->before_y - real_y));

		hand->before_x = real_x;
		hand->before_y = real_y;
	}
}

#define HandToolButtonRelease CommonToolDummyCallback

#define HandToolDisplayCursor CommonToolDummyDisplayCursor

static GtkWidget* CreateHandToolDetailUI(APPLICATION* app, void* data)
{
	return gtk_label_new("There is no settings.");
}

/*****************************************************************
* LoupeButtonToggled�֐�                                         *
* ���[�y���[�h�؂�ւ��{�^�����N���b�N���ꂽ���̃R�[���o�b�N�֐� *
* ����                                                           *
* button	: �{�^���E�B�W�F�b�g                                 *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X         *
*****************************************************************/
void LoupeButtonToggled(GtkWidget* button, APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];

	if(app->window_num == 0 || (window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{
		app->flags |= APPLICATION_IN_SWITCH_DRAW_WINDOW;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
		app->flags &= ~(APPLICATION_IN_SWITCH_DRAW_WINDOW);
		return;
	}

	if((app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) != 0)
	{
		return;
	}

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		Change2LoupeMode(app);
	}
	else
	{
		ReturnFromLoupeMode(app);
	}
}

void LoadCommonToolDetailData(
	COMMON_TOOL_CORE* core,
	INI_FILE_PTR file,
	const char* section_name,
	const char* tool_type,
	struct _APPLICATION* app
)
{
	if(StringCompareIgnoreCase(tool_type, "SELECT_RECTANGLE") == 0)
	{
		SELECT_RECTANGLE* select;
		core->tool_data = MEM_ALLOC_FUNC(sizeof(*select));
		(void)memset(core->tool_data, 0, sizeof(*select));
		select = (SELECT_RECTANGLE*)core->tool_data;
		select->select_start = IniFileGetInteger(file, section_name, "START");
		core->press_func = SelectRectangleButtonPress;
		core->motion_func = SelectRectangleMotion;
		core->release_func = SelectRectangleRelease;
		core->display_func = SelectRectangleDisplay;
		core->create_detail_ui = CreateSelectRectangleDetailUI;

		core->tool_type = TYPE_SELECT_RECTANGLE;
	}
	else if(StringCompareIgnoreCase(tool_type, "SELECT_ECLIPSE") == 0)
	{
		SELECT_ECLIPSE* select;
		core->tool_data = MEM_ALLOC_FUNC(sizeof(*select));
		(void)memset(core->tool_data, 0, sizeof(*select));
		select = (SELECT_ECLIPSE*)core->tool_data;
		select->select_start = IniFileGetInteger(file, section_name, "START");
		core->press_func = SelectEclipseButtonPress;
		core->motion_func = SelectEclipseMotion;
		core->release_func = SelectEclipseButtonRelease;
		core->display_func = SelectEclipseDisplay;
		core->create_detail_ui = CreateSelectEclipseDetailUI;

		core->tool_type = TYPE_SELECT_ECLIPSE;
		core->flags |= COMMON_TOOL_DO_RELEASE_FUNC_AT_CHANGI_TOOL;
	}
	else if(StringCompareIgnoreCase(tool_type, "FREE_SELECT") == 0)
	{
		FREE_SELECT *select;
		core->tool_data = MEM_ALLOC_FUNC(sizeof(*select));
		(void)memset(core->tool_data, 0, sizeof(*select));

		core->press_func = FreeSelectButtonPress;
		core->motion_func = FreeSelectMotion;
		core->release_func = FreeSelectButtonRelease;
		core->display_func = FreeSelectDisplay;
		core->create_detail_ui = CreateFreeSelectDetailUI;

		core->tool_type = TYPE_FREE_SELECT;
	}
	else if(StringCompareIgnoreCase(tool_type, "FUZZY_SELECT") == 0)
	{
		FUZZY_SELECT* fuzzy;
		core->tool_data = MEM_ALLOC_FUNC(sizeof(*fuzzy));
		(void)memset(core->tool_data, 0, sizeof(*fuzzy));
		fuzzy = (FUZZY_SELECT*)core->tool_data;
		fuzzy->threshold = (uint16)IniFileGetInteger(file, section_name, "THRESHOLD");
		fuzzy->select_mode = (uint8)IniFileGetInteger(file, section_name, "MODE");
		fuzzy->select_target = (uint8)IniFileGetInteger(file, section_name, "TARGET");
		fuzzy->extend = (int16)IniFileGetInteger(file, section_name, "EXTEND");
		core->press_func = FuzzySelectButtonPress;
		core->motion_func = FuzzySelectMotion;
		core->release_func = FuzzySelectRelease;
		core->display_func = FuzzySelectDisplay;
		core->create_detail_ui = CreateFuzzySelectDetailUI;

		core->tool_type = TYPE_FUZZY_SELECT;
	}
	else if(StringCompareIgnoreCase(tool_type, "SELECT_BY_COLOR") == 0)
	{
		SELECT_BY_COLOR* select;
		core->tool_data = MEM_ALLOC_FUNC(sizeof(*select));
		(void)memset(core->tool_data, 0, sizeof(*select));
		select = (SELECT_BY_COLOR*)core->tool_data;
		select->threshold = (uint16)IniFileGetInteger(file, section_name, "THRESHOLD");
		select->select_mode = (uint8)IniFileGetInteger(file, section_name, "MODE");
		select->select_target = (uint8)IniFileGetInteger(file, section_name, "TARGET");
		core->press_func = SelectByColorButtonPress;
		core->motion_func = SelectByColorMotion;
		core->release_func = SelectByColorRelease;
		core->display_func = SelectByColorDisplay;
		core->create_detail_ui = CreateSelectByColorDetailUI;

		core->tool_type = TYPE_SELECT_BY_COLOR;
	}
	else if(StringCompareIgnoreCase(tool_type, "COLOR_PICKER") == 0)
	{
		COLOR_PICKER* picker;
		core->tool_data = MEM_ALLOC_FUNC(sizeof(*picker));
		(void)memset(core->tool_data, 0, sizeof(*picker));
		picker = (COLOR_PICKER*)core->tool_data;

		picker->mode = (uint8)IniFileGetInteger(file, section_name, "MODE");
		picker->app = app;
		app->tool_window.color_picker.mode = picker->mode;

		core->press_func = ColorPickerButtonPress;
		core->motion_func = ColorPickerMotionCallBack;
		core->release_func = ColorPickerReleaseCallBack;
		core->display_func = ColorPickerDisplayCursor;
		core->create_detail_ui = CreateColorPickerDetailUI;

		core->tool_type = TYPE_COLOR_PICKER;
	}
	else if(StringCompareIgnoreCase(tool_type, "HAND_TOOL") == 0)
	{
		HAND_TOOL *hand;
		core->tool_data = MEM_ALLOC_FUNC(sizeof(*hand));
		(void)memset(core->tool_data, 0, sizeof(*hand));
		hand = (HAND_TOOL*)core->tool_data;

		core->press_func = HandToolButtonPress;
		core->motion_func = HandToolMotion;
		core->release_func = HandToolButtonRelease;
		core->display_func = HandToolDisplayCursor;
		core->create_detail_ui = CreateHandToolDetailUI;

		core->tool_type = TYPE_HAND_TOOL;
	}
	else if(StringCompareIgnoreCase(tool_type, "LOUPE_TOOL") == 0)
	{
		core->tool_type = TYPE_LOUPE_TOOL;
	}

	if(core->button_update == NULL)
	{
		core->button_update = DefaultToolUpdate;
	}

	if(core->motion_update == NULL)
	{
		core->motion_update = DefaultToolUpdate;
	}
}

/*********************************************************
* WriteCommonToolData�֐�                                *
* ���ʃc�[���̃f�[�^�������o��                           *
* ����                                                   *
* window	: �c�[���{�b�N�X                             *
* file_path	: �����o���t�@�C���̃p�X                     *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	����I��:0 ���s:���̒l                               *
*********************************************************/
int WriteCommonToolData(
	TOOL_WINDOW* window,
	const char* file_path,
	APPLICATION* app
)
{
	FILE *fp;
	INI_FILE_PTR file;
	char tool_section_name[256];
	char tool_name[1024];
	char *write_str;
	int tool_id = 1;
	char hot_key[2] = {0};
	int x, y;

	fp = fopen(file_path, "wb");

	// �t�@�C���I�[�v���Ɏ��s
	if(fp == NULL)
	{
		return -1;
	}

	file = CreateIniFile(fp, NULL, 0, INI_WRITE);

	(void)IniFileAddString(file, "CODE", "CODE_TYPE", window->common_tool_code);

	// �c�[���e�[�u���̓��e�������o��
	for(y=0; y<COMMON_TOOL_TABLE_HEIGHT; y++)
	{
		for(x=0; x<COMMON_TOOL_TABLE_WIDTH; x++)
		{
			if(window->common_tools[y][x].name != NULL)
			{
				(void)sprintf(tool_section_name, "TOOL%d", tool_id);

				(void)strcpy(tool_name, window->common_tools[y][x].name);
				StringRepalce(tool_name, "\n", "\\n");
				write_str = g_convert(tool_name, -1, window->common_tool_code, "UTF-8", NULL, NULL, NULL);

				(void)IniFileAddString(file, tool_section_name, "NAME", write_str);
				g_free(write_str);
				(void)IniFileAddString(file, tool_section_name,
					"IMAGE", window->common_tools[y][x].image_file_path);
				(void)IniFileAddInteger(file, tool_section_name, "X", x, 10);
				(void)IniFileAddInteger(file, tool_section_name, "Y", y, 10);
				if(window->common_tools[y][x].hot_key != '\0')
				{
					hot_key[0] = window->common_tools[y][x].hot_key;
					(void)IniFileAddString(file, tool_section_name, "HOT_KEY", hot_key);
				}

				switch(window->common_tools[y][x].tool_type)
				{
				case TYPE_COLOR_PICKER:
					{
						COLOR_PICKER *picker = (COLOR_PICKER*)window->common_tools[y][x].tool_data;
						(void)IniFileAddString(file, tool_section_name, "TYPE", "COLOR_PICKER");
						(void)IniFileAddInteger(file, tool_section_name, "MODE", picker->mode, 10);
					}
					break;
				case TYPE_SELECT_RECTANGLE:
					{
						SELECT_RECTANGLE *select = (SELECT_RECTANGLE*)window->common_tools[y][x].tool_data;
						(void)IniFileAddString(file, tool_section_name, "TYPE", "SELECT_RECTANGLE");
						(void)IniFileAddInteger(file, tool_section_name,
							"START", select->select_start, 10);
					}
					break;
				case TYPE_SELECT_ECLIPSE:
					{
						SELECT_ECLIPSE *select = (SELECT_ECLIPSE*)window->common_tools[y][x].tool_data;
						(void)IniFileAddString(file, tool_section_name, "TYPE", "SELECT_ECLIPSE");
						(void)IniFileAddInteger(file, tool_section_name,
							"START", select->select_start, 10);
					}
					break;
				case TYPE_FREE_SELECT:
					{
						(void)IniFileAddString(file, tool_section_name, "TYPE", "FREE_SELECT");
					}
					break;
				case TYPE_FUZZY_SELECT:
					{
						FUZZY_SELECT *fuzzy = (FUZZY_SELECT*)window->common_tools[y][x].tool_data;
						(void)IniFileAddString(file, tool_section_name, "TYPE", "FUZZY_SELECT");
						(void)IniFileAddInteger(file, tool_section_name, "THRESHOLD",
							fuzzy->threshold, 10);
						(void)IniFileAddInteger(file, tool_section_name, "MODE",
							fuzzy->select_mode, 10);
						(void)IniFileAddInteger(file, tool_section_name, "TARGET",
							fuzzy->select_target, 10);
						(void)IniFileAddInteger(file, tool_section_name, "EXTEND",
							fuzzy->extend, 10);
					}
					break;
				case TYPE_SELECT_BY_COLOR:
					{
						SELECT_BY_COLOR *select = (SELECT_BY_COLOR*)window->common_tools[y][x].tool_data;
						(void)IniFileAddString(file, tool_section_name, "TYPE", "SELECT_BY_COLOR");
						(void)IniFileAddInteger(file, tool_section_name, "THRESHOLD",
							select->threshold, 10);
						(void)IniFileAddInteger(file, tool_section_name, "MODE",
							select->select_mode, 10);
						(void)IniFileAddInteger(file, tool_section_name, "TARGET",
							select->select_target, 10);
					}
					break;
				case TYPE_HAND_TOOL:
					{
						HAND_TOOL *hand = (HAND_TOOL*)window->common_tools[y][x].tool_data;
						(void)IniFileAddString(file, tool_section_name, "TYPE", "HAND_TOOL");
					}
					break;
				case TYPE_LOUPE_TOOL:
					(void)IniFileAddString(file, tool_section_name, "TYPE", "LOUPE_TOOL");
					break;
				}

				tool_id++;
			}	// if(window->common_tools[y][x].name != NULL)
		}	// for(x=0; x<COMMON_TOOL_TABLE_WIDTH; x++)
	}	// �c�[���e�[�u���̓��e�������o��
		// for(y=0; y<COMMON_TOOL_TABLE_HEIGHT; y++)

	WriteIniFile(file, (size_t (*)(void*, size_t, size_t, void*))fwrite);
	file->delete_func(file);
	(void)fclose(fp);

	return 0;
}

/*****************************************
* SetColorPicker�֐�                     *
* �X�|�C�g�c�[���̃f�[�^�Z�b�g           *
* ����                                   *
* core		: ���ʃc�[���̊�{���       *
* picker	: �X�|�C�g�c�[���̏ڍ׃f�[�^ *
*****************************************/
void SetColorPicker(
	COMMON_TOOL_CORE* core,
	COLOR_PICKER* picker
)
{
	// �R�[���o�b�N�֐����Z�b�g
	core->press_func = ColorPickerButtonPress;
	core->motion_func = ColorPickerMotionCallBack;
	core->release_func = ColorPickerReleaseCallBack;
	core->display_func = ColorPickerDisplayCursor;

	core->tool_data = (void*)picker;
}

#ifdef __cplusplus
}
#endif
