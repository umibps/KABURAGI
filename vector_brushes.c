// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "configure.h"
#include "application.h"
#include "vector_brushes.h"
#include "vector.h"
#include "utils.h"
#include "bezier.h"
#include "widgets.h"
#include "memory_stream.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

static void VectorBrushDummyCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
}

static void VectorBrushDummyDrawCurosr(
	DRAW_WINDOW* window,
	gint x,
	gint y,
	void* data
)
{
}

static void PolyLinePressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		POLY_LINE_BRUSH* line = (POLY_LINE_BRUSH*)core->brush_data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;
		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;

		if((line->flags & POLY_LINE_SIZE_WITH_PRESSURE) == 0)
		{
			pressure = 0.5;
		}

		if(((GdkEventButton*)state)->type == GDK_2BUTTON_PRESS
			&& (line->flags & POLY_LINE_START) != 0 && layer->top_line->num_points > 3)
		{
			layer->top_line->num_points -= 2;
			layer->top_line->points[layer->top_line->num_points-1].pressure =
				line->before_pressure * 200 * line->last_pressure * 0.01;
			line->flags &= ~(POLY_LINE_START);
			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
		}
		else
		{
			if((line->flags & POLY_LINE_START) == 0)
			{
				layer->flags |= VECTOR_LAYER_RASTERIZE_TOP;
				layer->top_line = CreateVectorLine(layer->top_line, NULL);
				if((line->flags & POLY_LINE_ANTI_ALIAS) != 0)
				{
					layer->top_line->flags |= VECTOR_LINE_ANTI_ALIAS;
				}
				line->flags |= POLY_LINE_START;
				layer->top_line->vector_type = line->line_type;
				layer->top_line->blur = line->blur;
				layer->top_line->outline_hardness = line->outline_hardness;
				layer->top_line->points =
					(VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
				(void)memset(layer->top_line->points, 0,
						sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
				layer->top_line->buff_size = VECTOR_LINE_BUFFER_SIZE;
				layer->top_line->points->x = x;
				layer->top_line->points->y = y;
				layer->top_line->points->size = line->r;
				layer->top_line->points->pressure = pressure * 200 * line->first_pressure * 0.01;
				(void)memcpy(layer->top_line->points->color,
					window->app->tool_window.color_chooser->rgb, 3);
				layer->top_line->points->color[3] = line->flow;
				layer->top_line->points[1].x = x;
				layer->top_line->points[1].y = y;
				layer->top_line->points[1].size = line->r;
				layer->top_line->points[1].pressure = pressure * 200;
				(void)memcpy(layer->top_line->points[1].color,
					window->app->tool_window.color_chooser->rgb, 3);
				layer->top_line->points[1].color[3] = line->flow;
				layer->top_line->num_points = 2;
				layer->num_lines++;

				g_timer_start(line->timer);

				AddTopLineControlPointHistory(window, window->active_layer, layer->top_line,
					&layer->top_line->points[1], core->name, TRUE);
			}
			else
			{
				g_timer_stop(line->timer);
#if GTK_MAJOR_VERSION <= 2
				if(g_timer_elapsed(line->timer, NULL) < 0.4f && ((GdkEventButton*)state)->device->source != GDK_SOURCE_MOUSE)
#else
				if(g_timer_elapsed(line->timer, NULL) < 0.4f
					&& gdk_device_get_source(((GdkEventButton*)state)->device) != GDK_SOURCE_MOUSE)
#endif
				{
					layer->top_line->num_points--;
					line->flags &= ~(POLY_LINE_START);
					layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
				}
				else
				{
					layer->top_line->points[layer->top_line->num_points].x = x;
					layer->top_line->points[layer->top_line->num_points].y = y;
					layer->top_line->points[layer->top_line->num_points].size = line->r;
					layer->top_line->points[layer->top_line->num_points].pressure = pressure * 200 * line->last_pressure * 0.01;
					(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
						window->app->tool_window.color_chooser->rgb, 3);
					layer->top_line->points[layer->top_line->num_points].color[3] = line->flow;

					if(layer->top_line->num_points > 1)
					{
						layer->top_line->points[layer->top_line->num_points-1].pressure = line->before_pressure * 200;
					}

					AddTopLineControlPointHistory(window, window->active_layer,
						layer->top_line, &layer->top_line->points[layer->top_line->num_points], core->name, FALSE);

					layer->top_line->num_points++;
					layer->top_line->points[layer->top_line->num_points].pressure = pressure * 200;

					if(layer->top_line->num_points >= layer->top_line->buff_size-1)
					{
						layer->top_line->buff_size += VECTOR_LINE_BUFFER_SIZE;
						layer->top_line->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
							layer->top_line->points, layer->top_line->buff_size*sizeof(VECTOR_POINT));
					}

					window->work_layer->layer_mode = LAYER_BLEND_OVER;
					g_timer_start(line->timer);
				}
			}
		}

		line->before_pressure = pressure;
	}
	else if(((GdkEventButton*)state)->button == 3)
	{
		POLY_LINE_BRUSH* line = (POLY_LINE_BRUSH*)core->brush_data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;

		if((line->flags & POLY_LINE_START) != 0 && layer->top_line->num_points > 2)
		{
			layer->top_line->num_points -= 1;
			layer->top_line->points[layer->top_line->num_points-1].pressure =
				line->before_pressure * 200 * line->last_pressure * 0.01;
			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
			line->flags &= ~(POLY_LINE_START);
			layer->num_lines++;
		}
	}

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void PolyLineMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	POLY_LINE_BRUSH* line = (POLY_LINE_BRUSH*)core->brush_data;

	if((line->flags & POLY_LINE_START) != 0)
	{
		VECTOR_LAYER* layer = window->active_layer->layer_data.vector_layer_p;
		layer->flags |= VECTOR_LAYER_RASTERIZE_TOP;
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		layer->top_line->points[layer->top_line->num_points-1].x = x;
		layer->top_line->points[layer->top_line->num_points-1].y = y;
	}
}

#define PolyLineReleaseCallBack VectorBrushDummyCallBack

static void PolyLineKeyPressCallBack(DRAW_WINDOW *window, GdkEventKey* key, void* data)
{
	if(key->keyval == GDK_Return)
	//if(key->keyval == GDK_KEY_Return)
	{
		POLY_LINE_BRUSH* line = (POLY_LINE_BRUSH*)data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;

		if((line->flags & POLY_LINE_START) != 0 && layer->top_line->num_points > 2)
		{
			layer->top_line->num_points -= 1;
			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
			line->flags &= ~(POLY_LINE_START);
			layer->num_lines++;
		}
	}
}

static void PolyLineDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	POLY_LINE_BRUSH* line = (POLY_LINE_BRUSH*)data;
	gdouble r = line->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		r * 2 + 3, r * 2 + 3);
	cairo_clip(window->disp_layer->cairo_p);
}

static void PolyLineScaleChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	((POLY_LINE_BRUSH*)data)->r = gtk_adjustment_get_value(slider)*0.5;
}

static void PolyLineBlurChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	((POLY_LINE_BRUSH*)data)->blur = gtk_adjustment_get_value(slider);
}

static void PolyLineOutlineHardnessChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	((POLY_LINE_BRUSH*)data)->outline_hardness = gtk_adjustment_get_value(slider);
}

static void PolyLineFirstPressureChange(GtkAdjustment* slider, POLY_LINE_BRUSH* line)
{
	line->first_pressure = (uint8)(gtk_adjustment_get_value(slider));
}

static void PolyLineLastPressureChange(GtkAdjustment* slider, POLY_LINE_BRUSH* line)
{
	line->last_pressure = (uint8)(gtk_adjustment_get_value(slider));
}

static void PolyLineFlowChange(GtkAdjustment* slider, POLY_LINE_BRUSH* line)
{
	line->flow = (uint8)(gtk_adjustment_get_value(slider) * 2.55 + 0.5);
}

static void PolyLinePressureScaleChange(
	GtkWidget* widget,
	POLY_LINE_BRUSH* line
)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		line->flags &= ~(POLY_LINE_SIZE_WITH_PRESSURE);
	}
	else
	{
		line->flags |= POLY_LINE_SIZE_WITH_PRESSURE;
	}
}

static void PolyLineSetAntiAlias(
	GtkWidget* widget,
	POLY_LINE_BRUSH* line
)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		line->flags &= ~(POLY_LINE_ANTI_ALIAS);
	}
	else
	{
		line->flags |= POLY_LINE_ANTI_ALIAS;
	}
}

static void PolyLineSetLineType(GtkWidget* button, POLY_LINE_BRUSH* line)
{
	uint8 line_type = (uint8)GPOINTER_TO_INT(g_object_get_data(
		G_OBJECT(button), "line_type"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		line->line_type = line_type;
	}
}

static GtkWidget* CreatePolyLineDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	POLY_LINE_BRUSH* line = (POLY_LINE_BRUSH*)data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* hbox;
	GtkWidget* base_scale;
	GtkWidget* scale;
	GtkWidget* table;
	GtkWidget* label;
	GtkWidget* check_button;
	GtkWidget *buttons[2];
	GtkAdjustment* scale_adjustment;
	char mark_up_buff[256];

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
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
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), line->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(line->base_scale)
	{
	case 0:
		scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(line->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			line->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(line->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PolyLineScaleChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.brush_scale, 1);

	g_object_set_data(G_OBJECT(base_scale), "scale", scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &line->base_scale);

	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->blur, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PolyLineBlurChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.blur, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->outline_hardness, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PolyLineOutlineHardnessChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.outline_hardness, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->flow/2.55, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PolyLineFlowChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.flow, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->first_pressure, 0, 200, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PolyLineFirstPressureChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.enter, 0);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->last_pressure, 0, 200, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PolyLineLastPressureChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.out, 0);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(PolyLinePressureScaleChange), data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		line->flags & POLY_LINE_SIZE_WITH_PRESSURE);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(PolyLineSetAntiAlias), data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		line->flags & POLY_LINE_ANTI_ALIAS);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.open_path);
	g_object_set_data(G_OBJECT(buttons[0]), "line_type", GINT_TO_POINTER(VECTOR_LINE_STRAIGHT));
	buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.close_path);
	g_object_set_data(G_OBJECT(buttons[1]), "line_type", GINT_TO_POINTER(VECTOR_LINE_STRAIGHT_CLOSE));
	if(line->line_type == VECTOR_LINE_STRAIGHT)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[0]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[1]), TRUE);
	}
	g_signal_connect(G_OBJECT(buttons[0]), "toggled",
		G_CALLBACK(PolyLineSetLineType), data);
	g_signal_connect(G_OBJECT(buttons[1]), "toggled",
		G_CALLBACK(PolyLineSetLineType), data);
	table = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), buttons[1], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	return vbox;
}

static void BezierLinePressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		BEZIER_LINE_BRUSH* line = (BEZIER_LINE_BRUSH*)core->brush_data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;

		if((line->flags & BEZIER_LINE_SIZE_WITH_PRESSURE) == 0)
		{
			pressure = 0.5;
		}

		if(((GdkEventButton*)state)->type == GDK_2BUTTON_PRESS
			&& (line->flags & BEZIER_LINE_START) != 0 && layer->top_line->num_points > 3)
		{
			layer->top_line->num_points -= 2;
			layer->top_line->points[layer->top_line->num_points-1].pressure =
				line->before_pressure * 200 * line->last_pressure * 0.01;
			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
			line->flags &= ~(BEZIER_LINE_START);
		}
		else
		{
			if((line->flags & BEZIER_LINE_START) == 0)
			{
				layer->flags |= VECTOR_LAYER_RASTERIZE_TOP;
				layer->top_line = CreateVectorLine(layer->top_line, NULL);
				if((line->flags & BEZIER_LINE_ANTI_ALIAS) != 0)
				{
					layer->top_line->flags |= VECTOR_LINE_ANTI_ALIAS;
				}
				line->flags |= BEZIER_LINE_START;
				layer->top_line->vector_type = line->line_type;
				layer->top_line->blur = line->blur;
				layer->top_line->outline_hardness = line->outline_hardness;
				layer->top_line->points =
					(VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
				layer->top_line->buff_size = VECTOR_LINE_BUFFER_SIZE;
				(void)memset(layer->top_line->points, 0,
						sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
				layer->top_line->points->x = x;
				layer->top_line->points->y = y;
				layer->top_line->points->size = line->r;
				layer->top_line->points->pressure = pressure * 200 * line->first_pressure * 0.01;
				(void)memcpy(layer->top_line->points->color,
					window->app->tool_window.color_chooser->rgb, 3);
				layer->top_line->points->color[3] = line->flow;
				layer->top_line->points[1].x = x;
				layer->top_line->points[1].y = y;
				layer->top_line->points[1].size = line->r;
				layer->top_line->points[1].pressure = pressure * 200;
				(void)memcpy(layer->top_line->points[1].color,
					window->app->tool_window.color_chooser->rgb, 3);
				layer->top_line->points[1].color[3] = line->flow;
				layer->top_line->num_points = 2;
				layer->num_lines++;

				AddTopLineControlPointHistory(window, window->active_layer, layer->top_line,
					&layer->top_line->points[1], core->name, TRUE);

				g_timer_start(line->timer);
			}
			else
			{
				g_timer_stop(line->timer);
#if GTK_MAJOR_VERSION <= 2
				if(g_timer_elapsed(line->timer, NULL) < 0.4f && ((GdkEventButton*)state)->device->source != GDK_SOURCE_MOUSE)
#else
				if(g_timer_elapsed(line->timer, NULL) < 0.4f
					&& gdk_device_get_source(((GdkEventButton*)state)->device) != GDK_SOURCE_MOUSE)
#endif
				{
					layer->top_line->num_points--;
					layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
					line->flags &= ~(BEZIER_LINE_START);
				}
				else
				{
					layer->top_line->points[layer->top_line->num_points].x = x;
					layer->top_line->points[layer->top_line->num_points].y = y;
					layer->top_line->points[layer->top_line->num_points].size = line->r;
					layer->top_line->points[layer->top_line->num_points].pressure = pressure * 200 * line->last_pressure * 0.01;
					(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
						window->app->tool_window.color_chooser->rgb, 3);
					layer->top_line->points[layer->top_line->num_points].color[3] = line->flow;

					if(layer->top_line->num_points > 1)
					{
						layer->top_line->points[layer->top_line->num_points-1].pressure = line->before_pressure * 200;
					}

					AddTopLineControlPointHistory(window, window->active_layer,
						layer->top_line, &layer->top_line->points[layer->top_line->num_points], core->name, FALSE);

					layer->top_line->num_points++;
					layer->top_line->points[layer->top_line->num_points].size = line->r;
					layer->top_line->points[layer->top_line->num_points].pressure = pressure * 200 * line->last_pressure * 0.01;

					if(layer->top_line->num_points >= layer->top_line->buff_size-1)
					{
						layer->top_line->buff_size += VECTOR_LINE_BUFFER_SIZE;
						layer->top_line->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
							layer->top_line->points, layer->top_line->buff_size*sizeof(VECTOR_POINT));
					}

					g_timer_start(line->timer);

					window->work_layer->layer_mode = LAYER_BLEND_OVER;
				}
			}
		}

		line->before_pressure = pressure;
	}
	else if(((GdkEventButton*)state)->button == 3)
	{
		BEZIER_LINE_BRUSH* line = (BEZIER_LINE_BRUSH*)core->brush_data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;

		if((line->flags & BEZIER_LINE_START) != 0 && layer->top_line->num_points > 2)
		{
			layer->top_line->num_points -= 1;
			layer->top_line->points[layer->top_line->num_points-1].pressure =
				line->before_pressure * 200 * line->last_pressure * 0.01;
			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
			line->flags &= ~(BEZIER_LINE_START);
			layer->num_lines++;
		}
	}

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void BezierLineMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	BEZIER_LINE_BRUSH* line = (BEZIER_LINE_BRUSH*)core->brush_data;
	window->work_layer->layer_mode = LAYER_BLEND_NORMAL;

	if((line->flags & BEZIER_LINE_START) != 0)
	{
		VECTOR_LAYER* layer = window->active_layer->layer_data.vector_layer_p;
		layer->flags |= VECTOR_LAYER_RASTERIZE_TOP;
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		layer->top_line->points[layer->top_line->num_points-1].x = x;
		layer->top_line->points[layer->top_line->num_points-1].y = y;
	}
}

#define BezierLineReleaseCallBack VectorBrushDummyCallBack

static void BezierLineKeyPressCallBack(DRAW_WINDOW *window, GdkEventKey* key, void* data)
{
	if(key->keyval == GDK_Return)
	//if(key->keyval == GDK_KEY_Return)
	{
		BEZIER_LINE_BRUSH* line = (BEZIER_LINE_BRUSH*)data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;

		if((line->flags & BEZIER_LINE_START) != 0 && layer->top_line->num_points > 2)
		{
			layer->top_line->num_points -= 1;
			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
			line->flags &= ~(BEZIER_LINE_START);
			layer->num_lines++;
		}
	}
}

static void BezierLineDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	BEZIER_LINE_BRUSH* line = (BEZIER_LINE_BRUSH*)data;
	gdouble r = line->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		r * 2 + 3, r * 2 + 3);
	cairo_clip(window->disp_layer->cairo_p);
}

static void BezierLineScaleChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	((BEZIER_LINE_BRUSH*)data)->r = gtk_adjustment_get_value(slider)*0.5;
}

static void BezierLineBlurChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	((BEZIER_LINE_BRUSH*)data)->blur = gtk_adjustment_get_value(slider);
}

static void BezierLineOutlineHardnessChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	((BEZIER_LINE_BRUSH*)data)->outline_hardness = gtk_adjustment_get_value(slider);
}

static void BezierLineFlowChange(GtkAdjustment* slider, BEZIER_LINE_BRUSH* line)
{
	line->flow = (uint8)(gtk_adjustment_get_value(slider) * 2.55 + 0.5);
}

static void BezierLineFirstPressureChange(GtkAdjustment* slider, BEZIER_LINE_BRUSH* line)
{
	line->first_pressure = (uint8)(gtk_adjustment_get_value(slider));
}

static void BezierLineLastPressureChange(GtkAdjustment* slider, BEZIER_LINE_BRUSH* line)
{
	line->last_pressure = (uint8)(gtk_adjustment_get_value(slider));
}

static void BezierLinePressureScaleChange(
	GtkWidget* widget,
	BEZIER_LINE_BRUSH* line
)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		line->flags &= ~(BEZIER_LINE_SIZE_WITH_PRESSURE);
	}
	else
	{
		line->flags |= BEZIER_LINE_SIZE_WITH_PRESSURE;
	}
}

static void BezierLineSetAntiAlias(
	GtkWidget* widget,
	BEZIER_LINE_BRUSH* line
)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		line->flags &= ~(BEZIER_LINE_ANTI_ALIAS);
	}
	else
	{
		line->flags |= BEZIER_LINE_ANTI_ALIAS;
	}
}

static void BezierLineSetLineType(GtkWidget* button, BEZIER_LINE_BRUSH* line)
{
	uint8 line_type = (uint8)GPOINTER_TO_INT(g_object_get_data(
		G_OBJECT(button), "line_type"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		line->line_type = line_type;
	}
}

static GtkWidget* CreateBezierLineDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	BEZIER_LINE_BRUSH* line = (BEZIER_LINE_BRUSH*)data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* hbox;
	GtkWidget* base_scale;
	GtkWidget* scale;
	GtkWidget* table;
	GtkWidget* label;
	GtkWidget *buttons[2];
	GtkWidget* check_button;
	GtkAdjustment* scale_adjustment;
	char mark_up_buff[256];

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
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
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), line->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(line->base_scale)
	{
	case 0:
		scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(line->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			line->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(line->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BezierLineScaleChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.brush_scale, 1);

	g_object_set_data(G_OBJECT(base_scale), "scale", scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &line->base_scale);

	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->blur, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BezierLineBlurChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.blur, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->outline_hardness, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BezierLineOutlineHardnessChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.outline_hardness, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->flow/2.55, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BezierLineFlowChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.flow, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->first_pressure, 0, 200, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BezierLineFirstPressureChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.enter, 0);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(line->last_pressure, 0, 200, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BezierLineLastPressureChange), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.out, 0);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(BezierLinePressureScaleChange), data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		line->flags & BEZIER_LINE_SIZE_WITH_PRESSURE);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(BezierLineSetAntiAlias), data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		line->flags & BEZIER_LINE_ANTI_ALIAS);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.open_path);
	g_object_set_data(G_OBJECT(buttons[0]), "line_type", GINT_TO_POINTER(VECTOR_LINE_BEZIER_OPEN));
	buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.close_path);
	g_object_set_data(G_OBJECT(buttons[1]), "line_type", GINT_TO_POINTER(VECTOR_LINE_BEZIER_CLOSE));
	if(line->line_type == VECTOR_LINE_BEZIER_OPEN)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[0]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[1]), TRUE);
	}
	g_signal_connect(G_OBJECT(buttons[0]), "toggled",
		G_CALLBACK(BezierLineSetLineType), data);
	g_signal_connect(G_OBJECT(buttons[1]), "toggled",
		G_CALLBACK(BezierLineSetLineType), data);
	table = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), buttons[1], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	return vbox;
}

static void FreeHandPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		FREE_HAND_BRUSH* free_hand = (FREE_HAND_BRUSH*)core->brush_data;
		VECTOR_LAYER* layer = (VECTOR_LAYER*)window->active_layer->layer_data.vector_layer_p;
		cairo_pattern_t *brush;
		gdouble alpha;

		window->work_layer->layer_mode = LAYER_BLEND_OVER;
		layer->flags = 0;

		free_hand->flags |= FREE_HAND_STARTED;
#if GTK_MAJOR_VERSION <= 2
		free_hand->source = ((GdkEventButton*)state)->device->source;
#else
		free_hand->source = gdk_device_get_source(((GdkEventButton*)state)->device);
#endif

#if GTK_MAJOR_VERSION <= 2
		if(((GdkEventButton*)state)->device->source != GDK_SOURCE_MOUSE)
#else
		if(gdk_device_get_source(((GdkEventButton*)state)->device) != GDK_SOURCE_MOUSE)
#endif
		{
			pressure *= 0.5;
		}

#if GTK_MAJOR_VERSION <= 2
		if(((GdkEventButton*)state)->device->source == GDK_SOURCE_MOUSE)
#else
		if(gdk_device_get_source(((GdkEventButton*)state)->device) == GDK_SOURCE_MOUSE)
#endif
		{
			pressure = 0.5;
		}
		else if((free_hand->flags & FREE_HAND_SIZE_WITH_PRESSURE) == 0)
		{
			pressure = 0.5;
		}

		alpha = free_hand->flow * DIV_PIXEL;
		//alpha = free_hand->flow * DIV_PIXEL * pressure;

		layer->top_line = CreateVectorLine(layer->top_line, NULL);
		layer->top_line->vector_type = free_hand->line_type;
		if((free_hand->flags & FREE_HAND_ANTI_ALIAS) != 0)
		{
			layer->top_line->flags |= VECTOR_LINE_ANTI_ALIAS;
		}
		layer->top_line->points =
			(VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
		(void)memset(layer->top_line->points, 0,
			sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
		layer->top_line->buff_size = VECTOR_LINE_BUFFER_SIZE;
		layer->top_line->points->x = x;
		layer->top_line->points->y = y;
		layer->top_line->points->size = free_hand->r;
		layer->top_line->points->pressure = pressure * 200;
		layer->top_line->blur = free_hand->blur;
		layer->top_line->outline_hardness = free_hand->outline_hardness;
		(void)memcpy(layer->top_line->points->color, *core->color, 3);
		layer->top_line->points->color[3] = free_hand->flow;
		layer->top_line->num_points++;

		free_hand->draw_before_x = free_hand->before_x = x;
		free_hand->draw_before_y = free_hand->before_y = y;

		brush = cairo_pattern_create_radial(x, y, 0, x, y,
			free_hand->r * 2 * pressure);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
		cairo_pattern_add_color_stop_rgba(brush, 0.0, (*core->color)[0]*DIV_PIXEL,
			(*core->color)[1]*DIV_PIXEL, (*core->color)[2]*DIV_PIXEL, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0-free_hand->blur*0.01f,
			(*core->color)[0]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL, (*core->color)[2]*DIV_PIXEL, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0, (*core->color)[0]*DIV_PIXEL,
			(*core->color)[1]*DIV_PIXEL, (*core->color)[2]*DIV_PIXEL, free_hand->outline_hardness*alpha);
		cairo_set_source(window->work_layer->cairo_p, brush);

		cairo_paint(window->work_layer->cairo_p);

		cairo_pattern_destroy(brush);
	}
}

static void FreeHandMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		VECTOR_LAYER* layer = window->active_layer->layer_data.vector_layer_p;
		FREE_HAND_BRUSH* free_hand = (FREE_HAND_BRUSH*)core->brush_data;
		cairo_pattern_t *brush;
		gdouble d, step, arg, arg2;
		gdouble dx, dy;
		gdouble alpha;
		gdouble draw_x = x, draw_y = y;
		int32 clear_x, clear_y, clear_width, clear_height;
		gdouble diff_x, diff_y;
		gdouble hard_ness = free_hand->outline_hardness*0.01f;
		gdouble r;
		uint8* color = *core->color;
		int i, j, k;

		if((free_hand->flags & FREE_HAND_STARTED) == 0)
		{
			return;
		}

		layer->flags = 0;

		dx = x - free_hand->before_x;
		dy = y - free_hand->before_y;
		d = sqrt(dx*dx+dy*dy);
		arg = atan2(dy, dx);

		pressure *= 0.5;

		if((free_hand->flags & FREE_HAND_SIZE_WITH_PRESSURE) == 0
			|| free_hand->source == GDK_SOURCE_MOUSE)
		{
			pressure = 0.5;
		}

		r = free_hand->r * pressure * 2;
		step = r * BRUSH_STEP;
		if(step < 0.5)
		{
			step = 0.5;
		}

		if((free_hand->flags & FREE_HAND_PRIOR_ARG) == 0)
		{
			if(d >= free_hand->min_distance)
			{
				if(layer->top_line->num_points == 1)
				{
					free_hand->before_arg = arg;
					layer->top_line->points[layer->top_line->num_points].x = x;
					layer->top_line->points[layer->top_line->num_points].y = y;
					layer->top_line->points[layer->top_line->num_points].pressure = pressure*200;
					layer->top_line->points[layer->top_line->num_points].size = free_hand->r;
					(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
						color, 3);
					layer->top_line->points[layer->top_line->num_points].color[3] = free_hand->flow;
					layer->top_line->num_points++;
				}
				else
				{
					if(fabs(free_hand->before_arg-arg) >= free_hand->min_arg)
					{
						layer->top_line->points[layer->top_line->num_points].x = x;
						layer->top_line->points[layer->top_line->num_points].y = y;
						(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
							core->color, 3);
						layer->top_line->points[layer->top_line->num_points].pressure = pressure * 200;
						layer->top_line->points[layer->top_line->num_points].size = free_hand->r;
						(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
							color, 3);
						layer->top_line->points[layer->top_line->num_points].color[3] = free_hand->flow;
						layer->top_line->num_points++;

						if(layer->top_line->num_points >= layer->top_line->buff_size-1)
						{
							layer->top_line->buff_size += VECTOR_LINE_BUFFER_SIZE;
							layer->top_line->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
								layer->top_line->points, layer->top_line->buff_size*sizeof(VECTOR_POINT));
						}

						free_hand->before_x = x;
						free_hand->before_y = y;
						free_hand->before_arg = arg;
					}
					else
					{
						layer->top_line->points[layer->top_line->num_points-1].x = x;
						layer->top_line->points[layer->top_line->num_points-1].y = y;
					}
				}
			}
		}
		else
		{
			if(layer->top_line->num_points < 2)
			{
				if(d >= free_hand->min_distance)
				{
					free_hand->before_arg = arg;
					layer->top_line->points[layer->top_line->num_points].x = x;
					layer->top_line->points[layer->top_line->num_points].y = y;
					layer->top_line->points[layer->top_line->num_points].pressure = pressure*200;
					layer->top_line->points[layer->top_line->num_points].size = free_hand->r;
					(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
						color, 3);
					layer->top_line->points[layer->top_line->num_points].color[3] = free_hand->flow;
					layer->top_line->num_points++;
					free_hand->before_x = x;
					free_hand->before_y = y;
				}
			}
			else
			{
				if(d >= FREE_HAND_MINIMUM_DISTANCE)
				{
					dx = free_hand->before_x - layer->top_line->points[layer->top_line->num_points-2].x;
					dy = free_hand->before_y - layer->top_line->points[layer->top_line->num_points-2].y;
					arg2 = atan2(dy, dx);

					if(fabs(arg - arg2) > free_hand->min_arg)
					{
						layer->top_line->points[layer->top_line->num_points].x = x;
						layer->top_line->points[layer->top_line->num_points].y = y;
						(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
							core->color, 3);
						layer->top_line->points[layer->top_line->num_points].pressure = pressure * 200;
						layer->top_line->points[layer->top_line->num_points].size = free_hand->r;
						(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
							color, 3);
						layer->top_line->points[layer->top_line->num_points].color[3] = free_hand->flow;
						layer->top_line->num_points++;

						if(layer->top_line->num_points >= layer->top_line->buff_size-1)
						{
							layer->top_line->buff_size += VECTOR_LINE_BUFFER_SIZE;
							layer->top_line->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
								layer->top_line->points, layer->top_line->buff_size*sizeof(VECTOR_POINT));
						}

						free_hand->before_x = x;
						free_hand->before_y = y;
						free_hand->before_arg = arg;
					}
					else
					{
						layer->top_line->points[layer->top_line->num_points-1].x = x;
						layer->top_line->points[layer->top_line->num_points-1].y = y;
						free_hand->before_x = x;
						free_hand->before_y = y;
					}
				}
			}
		}

		draw_x = free_hand->draw_before_x;
		draw_y = free_hand->draw_before_y;
		dx = x - free_hand->draw_before_x;
		dy = y - free_hand->draw_before_y;
		d = sqrt(dx*dx+dy*dy);
		diff_x = (step * dx) / d;
		diff_y = (step * dy) / d;
		dx = d;

		do
		{
			clear_x = (int32)(draw_x - r - 1);
			clear_width = (int32)(draw_x + r + 1);
			clear_y = (int32)(draw_y - r - 1);
			clear_height = (int32)(draw_y + r + 1);

			if(clear_x < 0)
			{
				clear_x = 0;
			}
			else if(clear_x >= window->width)
			{
				goto skip_draw;
			}
			if(clear_y < 0)
			{
				clear_y = 0;
			}
			if(clear_height >= window->height)
			{
				clear_height = window->height;
			}
			clear_width = clear_width - clear_x;
			clear_height = clear_height - clear_y;

			if(clear_width <= 0 || clear_height <= 0)
			{
				goto skip_draw;
			}

			for(i=0; i<clear_height; i++)
			{
				(void)memset(&window->temp_layer->pixels[(i+clear_y)*window->work_layer->stride+clear_x*window->work_layer->channel],
					0x0, clear_width*window->work_layer->channel);
			}

			alpha = free_hand->flow * DIV_PIXEL;

			brush = cairo_pattern_create_radial(draw_x, draw_y, r * 0.0, draw_x, draw_y, r);
			cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
			cairo_pattern_add_color_stop_rgba(brush, 0.0, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1.0-free_hand->blur*0.01f,
				color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1.0, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL,
				free_hand->outline_hardness*0.01f*alpha);
			cairo_set_source(window->temp_layer->cairo_p, brush);

			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				cairo_paint(window->temp_layer->cairo_p);
			}
			else
			{
				cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);
			}
		
			for(i=0; i<clear_height; i++)
			{
				for(j=0; j<clear_width; j++)
				{
					if(window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3]
						> window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+3])
					{
						for(k=0; k<window->temp_layer->channel; k++)
						{
							window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k] =
								(uint32)(((int)window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+k]
								- (int)window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k])
									* window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3] >> 8)
									+ window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k];
						}
					}
				}
			}

			cairo_pattern_destroy(brush);

skip_draw:
			dx -= step;
			if(dx < 1)
			{
				break;
			}
			else if(dx >= step)
			{
				draw_x += diff_x, draw_y += diff_y;
			}
		} while(1);

		free_hand->draw_before_x = x;
		free_hand->draw_before_y = y;
	}
}

typedef struct _FREE_HAND_HISTORY_DATA
{
	VECTOR_LINE line_data;
	uint16 layer_name_length;
	char *layer_name;
	VECTOR_POINT *point_data;
} FREE_HAND_HISTORY_DATA;

static void FreeHandUndo(DRAW_WINDOW* window, void* p)
{
	FREE_HAND_HISTORY_DATA data;
	LAYER *layer = window->layer;
	VECTOR_LINE *next_top;
	uint8 *byte_data = (uint8*)p;
	(void)memcpy(&data.line_data, p, sizeof(data.line_data));
	byte_data += sizeof(data.line_data);
	(void)memcpy(&data.layer_name_length, byte_data, sizeof(data.layer_name_length));
	byte_data += sizeof(data.layer_name_length);
	data.layer_name = (char*)byte_data;
	
	while(strcmp(layer->name, data.layer_name) != 0)
	{
		layer = layer->next;
	}

	next_top = layer->layer_data.vector_layer_p->top_line->prev;
	DeleteVectorLine(&layer->layer_data.vector_layer_p->top_line);
	layer->layer_data.vector_layer_p->top_line = next_top;

	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
}

static void FreeHandRedo(DRAW_WINDOW* window, void* p)
{
	FREE_HAND_HISTORY_DATA data;
	LAYER *layer = window->layer;
	VECTOR_LINE *add_line;
	uint8 *byte_data = (uint8*)p;
	(void)memcpy(&data.line_data, p, sizeof(data.line_data));
	byte_data += sizeof(data.line_data);
	(void)memcpy(&data.layer_name_length, byte_data, sizeof(data.layer_name_length));
	byte_data += sizeof(data.layer_name_length);
	data.layer_name = (char*)byte_data;
	
	while(strcmp(layer->name, data.layer_name) != 0)
	{
		layer = layer->next;
	}

	byte_data += data.layer_name_length;
	(void)memcpy(&data.line_data, byte_data, sizeof(data.line_data));
	byte_data += sizeof(data.line_data);

	add_line = layer->layer_data.vector_layer_p->top_line =
		CreateVectorLine(layer->layer_data.vector_layer_p->top_line, NULL);
	add_line->vector_type = data.line_data.vector_type;
	add_line->flags = data.line_data.flags;
	add_line->num_points = data.line_data.num_points;
	add_line->buff_size = data.line_data.buff_size;
	add_line->blur = data.line_data.blur;
	add_line->outline_hardness = data.line_data.outline_hardness;
	add_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
		sizeof(*add_line->points)*add_line->buff_size);
	(void)memset(add_line->points, 0, sizeof(*add_line->points)*add_line->buff_size);
	(void)memcpy(add_line->points, byte_data, sizeof(*add_line->points)*add_line->num_points);

	layer->layer_data.vector_layer_p->flags |=
		VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE;
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void AddFreeHandHistory(
	DRAW_WINDOW* window,
	LAYER* active_layer,
	VECTOR_LINE* add_line,
	const char* tool_name
)
{
	MEMORY_STREAM_PTR stream = CreateMemoryStream(4096);
	uint16 name_length = (uint16)strlen(active_layer->name) + 1;

	// 線情報を書き出す
	(void)MemWrite(add_line, sizeof(*add_line), 1, stream);
	// レイヤーの名前を書き出す
	(void)MemWrite(&name_length, sizeof(name_length), 1, stream);
	(void)MemWrite(active_layer->name, 1, name_length, stream);
	// 座標データを書き出す
	(void)MemWrite(add_line, sizeof(*add_line), 1, stream);
	(void)MemWrite(add_line->points, sizeof(*add_line->points),
		add_line->num_points, stream);

	AddHistory(
		&window->history,
		tool_name,
		stream->buff_ptr,
		(uint32)stream->data_size,
		FreeHandUndo,
		FreeHandRedo
	);
	(void)DeleteMemoryStream(stream);
}

static void FreeHandReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		FREE_HAND_BRUSH *free_hand = (FREE_HAND_BRUSH*)core->brush_data;
		VECTOR_LAYER* layer = window->active_layer->layer_data.vector_layer_p;
		uint8* color = *core->color;
		int i;

		if((free_hand->flags & FREE_HAND_STARTED) == 0)
		{
			return;
		}

		if(((free_hand->flags & FREE_HAND_PRIOR_ARG) == 0) || layer->top_line->num_points == 1)
		{
			if((free_hand->flags & FREE_HAND_SIZE_WITH_PRESSURE) == 0)
			{
				pressure = 0.5;
			}

			layer->top_line->points[layer->top_line->num_points].x = x;
			layer->top_line->points[layer->top_line->num_points].y = y;
			(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
				core->color, 3);
			layer->top_line->points[layer->top_line->num_points].color[3] = free_hand->flow;
			layer->top_line->points[layer->top_line->num_points].pressure = pressure * 200;
			layer->top_line->points[layer->top_line->num_points].size = free_hand->r;
			(void)memcpy(layer->top_line->points[layer->top_line->num_points].color,
				color, 3);
			layer->top_line->points[layer->top_line->num_points].color[3] = free_hand->flow;
			layer->top_line->num_points++;

			if(layer->top_line->num_points >= layer->top_line->buff_size-1)
			{
				layer->top_line->buff_size += VECTOR_LINE_BUFFER_SIZE;
				layer->top_line->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
					layer->top_line->points, layer->top_line->buff_size*sizeof(VECTOR_POINT));
			}

			for(i=0; i<layer->top_line->num_points-1 && layer->top_line->num_points > 2; i++)
			{
				if((layer->top_line->points[i].x-layer->top_line->points[i+1].x)*(layer->top_line->points[i].x-layer->top_line->points[i+1].x)
					+ (layer->top_line->points[i].y-layer->top_line->points[i+1].y)*(layer->top_line->points[i].y-layer->top_line->points[i+1].y)
					<= free_hand->min_distance*free_hand->min_distance)
				{
					(void)memmove(&layer->top_line->points[i+1], &layer->top_line->points[i+2],
						sizeof(VECTOR_POINT)*(layer->top_line->num_points-i-1));
					layer->top_line->num_points--;
				}
			}
		}
		else
		{
			layer->top_line->points[layer->top_line->num_points-1].x = x;
			layer->top_line->points[layer->top_line->num_points-1].y = y;
		}

		AddFreeHandHistory(window, window->active_layer, layer->top_line, core->name);
		layer->flags |= VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE;
		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

		RasterizeVectorLayer(window, window->active_layer, window->active_layer->layer_data.vector_layer_p);
		(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

		free_hand->flags &= ~(FREE_HAND_STARTED);
	}
}

static void FreeHandDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	FREE_HAND_BRUSH* free_hand = (FREE_HAND_BRUSH*)data;
	gdouble r = free_hand->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		r * 2 + 3, r * 2 + 3);
	cairo_clip(window->disp_layer->cairo_p);
}

static void ChangeFreeHandScale(GtkAdjustment *slider, FREE_HAND_BRUSH* free_hand)
{
	free_hand->r = gtk_adjustment_get_value(slider) * 0.5;
}

static void ChangeFreeHandBlur(GtkAdjustment *slider, FREE_HAND_BRUSH* free_hand)
{
	free_hand->blur = gtk_adjustment_get_value(slider);
}

static void ChangeFreeHandOutlineHardness(GtkAdjustment* slider, FREE_HAND_BRUSH* free_hand)
{
	free_hand->outline_hardness = gtk_adjustment_get_value(slider);
}

static void ChangeFreeHandFlow(GtkAdjustment* slider, FREE_HAND_BRUSH* free_hand)
{
	free_hand->flow = (uint8)(gtk_adjustment_get_value(slider) * 2.55 + 0.5);
}

static void ChangeFreeHandMinDegree(GtkAdjustment* slider, FREE_HAND_BRUSH* free_hand)
{
	free_hand->min_degree = gtk_adjustment_get_value(slider);
	free_hand->min_arg = (free_hand->min_degree * G_PI) / 180;
}

static void ChangeFreeHandMinDistance(GtkAdjustment* slider, FREE_HAND_BRUSH* free_hand)
{
	free_hand->min_distance = gtk_adjustment_get_value(slider);
}

static void ChangeFreeHandPriorArg(
	GtkWidget* widget,
	FREE_HAND_BRUSH* free_hand
)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		free_hand->flags &= ~(FREE_HAND_PRIOR_ARG);
	}
	else
	{
		free_hand->flags |= FREE_HAND_PRIOR_ARG;
	}
}

static void ChangeFreeHandPressureSize(
	GtkWidget* widget,
	FREE_HAND_BRUSH* free_hand
)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		free_hand->flags &= ~(FREE_HAND_SIZE_WITH_PRESSURE);
	}
	else
	{
		free_hand->flags |= FREE_HAND_SIZE_WITH_PRESSURE;
	}
}

static void ChangeFreeHandSetAntiAlias(
	GtkWidget* widget,
	FREE_HAND_BRUSH* free_hand
)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		free_hand->flags &= ~(FREE_HAND_ANTI_ALIAS);
	}
	else
	{
		free_hand->flags |= FREE_HAND_ANTI_ALIAS;
	}
}

static void FreeHandSetLineType(GtkWidget* button, FREE_HAND_BRUSH* free_hand)
{
	uint8 line_type = (uint8)GPOINTER_TO_INT(g_object_get_data(
		G_OBJECT(button), "line_type"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		free_hand->line_type = line_type;
	}
}

static GtkWidget* CreateFreeHandDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	FREE_HAND_BRUSH* free_hand = (FREE_HAND_BRUSH*)data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* hbox;
	GtkWidget* base_scale;
	GtkWidget *buttons[2];
	GtkWidget* brush_scale, *table = gtk_table_new(6, 3, FALSE);
	GtkWidget* label, *check_button;
	GtkAdjustment *brush_scale_adjustment;
	char mark_up_buff[128];

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
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
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), free_hand->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(free_hand->base_scale)
	{
	case 0:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(free_hand->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			free_hand->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(free_hand->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(ChangeFreeHandScale), data);

	g_object_set_data(G_OBJECT(base_scale), "scale", brush_scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &free_hand->base_scale);

	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(free_hand->blur, 0, 255, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(BezierLineBlurChange), data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.blur, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 1, 2);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		free_hand->outline_hardness, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(ChangeFreeHandOutlineHardness), data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.outline_hardness, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 2, 3);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		free_hand->flow / 2.55, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(ChangeFreeHandFlow), data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.flow, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 3, 4);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		free_hand->min_degree, 3, 45, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(ChangeFreeHandMinDegree), data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.min_degree, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 4, 5);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		free_hand->min_distance, FREE_HAND_MINIMUM_DISTANCE, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(ChangeFreeHandMinDistance), data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.min_distance, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 5, 6);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.prior_angle);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		free_hand->flags & FREE_HAND_PRIOR_ARG);
	(void)g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(ChangeFreeHandPriorArg), free_hand);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(ChangeFreeHandPressureSize), data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		free_hand->flags & FREE_HAND_SIZE_WITH_PRESSURE);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(ChangeFreeHandSetAntiAlias), data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		free_hand->flags & FREE_HAND_ANTI_ALIAS);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.open_path);
	g_object_set_data(G_OBJECT(buttons[0]), "line_type", GINT_TO_POINTER(VECTOR_LINE_BEZIER_OPEN));
	buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.close_path);
	g_object_set_data(G_OBJECT(buttons[1]), "line_type", GINT_TO_POINTER(VECTOR_LINE_BEZIER_CLOSE));
	if(free_hand->line_type == VECTOR_LINE_BEZIER_OPEN)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[0]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[1]), TRUE);
	}
	g_signal_connect(G_OBJECT(buttons[0]), "toggled",
		G_CALLBACK(FreeHandSetLineType), data);
	g_signal_connect(G_OBJECT(buttons[1]), "toggled",
		G_CALLBACK(FreeHandSetLineType), data);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), buttons[1], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	return vbox;
}

typedef struct _CONTROL_POINT_MOVE_DELETE_HISTORY
{
	VECTOR_POINT before_point_data;
	VECTOR_POINT change_data;
	int32 line_index;
	int32 point_index;
	uint16 add_flag;
	uint16 layer_name_length;
	char *layer_name;
} CONTROL_POINT_MOVE_DELETE_HISTORY;

static void ControlPointMoveUndo(DRAW_WINDOW* window, void* p)
{
	CONTROL_POINT_MOVE_DELETE_HISTORY *history_data =
		(CONTROL_POINT_MOVE_DELETE_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CONTROL_POINT_MOVE_DELETE_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	if(history_data->add_flag == 0)
	{
		line->points[history_data->point_index] = history_data->before_point_data;
	}
	else
	{
		(void)memmove(&line->points[history_data->point_index],
			&line->points[history_data->point_index+1],
			sizeof(*line->points)*(line->num_points-history_data->point_index-1)
		);
		line->num_points--;
	}

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void ControlPointMoveRedo(DRAW_WINDOW* window, void* p)
{
	CONTROL_POINT_MOVE_DELETE_HISTORY *history_data =
		(CONTROL_POINT_MOVE_DELETE_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CONTROL_POINT_MOVE_DELETE_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	if(history_data->add_flag == 0)
	{
		line->points[history_data->point_index] = history_data->change_data;
	}
	else
	{
		(void)memmove(&line->points[history_data->point_index+1],
			&line->points[history_data->point_index],
			sizeof(*line->points)*(line->num_points-history_data->point_index)
		);
		line->points[history_data->point_index] = history_data->change_data;
		line->num_points++;
	}

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void AddControlPointMoveHistory(
	DRAW_WINDOW* window,
	VECTOR_LINE* change_line,
	VECTOR_POINT* change_point,
	VECTOR_POINT* before_point_data,
	uint16 add_flag
)
{
	uint8 *byte_data;
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name);
	CONTROL_POINT_MOVE_DELETE_HISTORY *history_data =
		(CONTROL_POINT_MOVE_DELETE_HISTORY*)MEM_ALLOC_FUNC(
			offsetof(CONTROL_POINT_MOVE_DELETE_HISTORY, layer_name) + layer_name_length + 4);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE *line = layer_data->base->next;
	int line_index = 0;
	int point_index;

	while(line != change_line)
	{
		line = line->next;
		line_index++;
	}

	for(point_index=0; point_index < change_line->num_points; point_index++)
	{
		if(&change_line->points[point_index] == change_point)
		{
			break;
		}
	}

	history_data->before_point_data = *before_point_data;
	history_data->change_data = *change_point;
	history_data->line_index = (int32)line_index;
	history_data->point_index = (int32)point_index;
	history_data->add_flag = add_flag;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(CONTROL_POINT_MOVE_DELETE_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);

	AddHistory(&window->history, window->app->labels->tool_box.control.move,
		(void*)history_data, offsetof(CONTROL_POINT_MOVE_DELETE_HISTORY, layer_name) + layer_name_length + 4,
		ControlPointMoveUndo, ControlPointMoveRedo
	);
}

static void ControlPointDeleteUndo(DRAW_WINDOW* window, void* p)
{
	CONTROL_POINT_MOVE_DELETE_HISTORY *history_data =
		(CONTROL_POINT_MOVE_DELETE_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CONTROL_POINT_MOVE_DELETE_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	if(history_data->add_flag == 1)
	{
		(void)memmove(&line->points[history_data->point_index+1],
			&line->points[history_data->point_index],
			sizeof(*line->points)*(line->num_points-history_data->point_index)
		);
		line->points[history_data->point_index] = history_data->change_data;
		line->num_points++;
	}
	else
	{
		line = CreateVectorLine(line->prev, line);
		line->num_points = 2;
		line->points[0] = history_data->before_point_data;
		line->points[1] = history_data->change_data;
		line->vector_type = history_data->add_flag-2;
	}

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void AddControlPointDeleteHistory(
	DRAW_WINDOW* window,
	VECTOR_LINE* change_line,
	VECTOR_POINT* change_point,
	VECTOR_POINT* before_change_data,
	uint16 add_flag
)
{
	uint8 *byte_data;
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name);
	CONTROL_POINT_MOVE_DELETE_HISTORY *history_data =
		(CONTROL_POINT_MOVE_DELETE_HISTORY*)MEM_ALLOC_FUNC(
			offsetof(CONTROL_POINT_MOVE_DELETE_HISTORY, layer_name) + layer_name_length + 4);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE *line = layer_data->base->next;
	int line_index = 0;
	int point_index;

	while(line != change_line)
	{
		line = line->next;
		line_index++;
	}

	for(point_index=0; point_index < change_line->num_points; point_index++)
	{
		if(&change_line->points[point_index] == change_point)
		{
			break;
		}
	}

	history_data->before_point_data = *before_change_data;
	history_data->change_data = *change_point;
	history_data->line_index = (int32)line_index;
	history_data->point_index = (int32)point_index;
	history_data->add_flag = add_flag;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(CONTROL_POINT_MOVE_DELETE_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);

	AddHistory(&window->history, window->app->labels->tool_box.control.move,
		(void*)history_data, offsetof(CONTROL_POINT_MOVE_DELETE_HISTORY, layer_name) + layer_name_length + 4,
		ControlPointDeleteUndo, ControlPointMoveUndo
	);
}

typedef struct _STROKE_MOVE_HISTORY
{
	FLOAT_T move_x, move_y;
	int32 line_index;
	uint16 layer_name_length;
	char *layer_name;
} STROKE_MOVE_HISTORY;

static void StrokeMoveUndoRedo(DRAW_WINDOW* window, void* p)
{
	STROKE_MOVE_HISTORY *history_data =
		(STROKE_MOVE_HISTORY*)p;
	LAYER *layer = window->layer;
	VECTOR_LINE *line;
	FLOAT_T diff_x, diff_y;
	char *name = &(((char*)p)[offsetof(STROKE_MOVE_HISTORY, layer_name)]);
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	diff_x = history_data->move_x - line->points[0].x;
	diff_y = history_data->move_y - line->points[0].y;

	history_data->move_x = line->points[0].x;
	history_data->move_y = line->points[0].y;

	for(i=0; i<line->num_points; i++)
	{
		line->points[i].x += diff_x;
		line->points[i].y += diff_y;
	}

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void AddStrokeMoveHistory(
	DRAW_WINDOW* window,
	VECTOR_LINE* change_line,
	VECTOR_POINT* before_point
)
{
	uint8 *byte_data;
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name);
	STROKE_MOVE_HISTORY *history_data =
		(STROKE_MOVE_HISTORY*)MEM_ALLOC_FUNC(
			offsetof(STROKE_MOVE_HISTORY, layer_name) + layer_name_length + 4);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE *line = layer_data->base->next;
	int line_index = 0;

	while(line != change_line)
	{
		line = line->next;
		line_index++;
	}

	history_data->move_x = before_point->x;
	history_data->move_y = before_point->y;
	history_data->line_index = (int32)line_index;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(STROKE_MOVE_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);

	AddHistory(&window->history, window->app->labels->tool_box.control.move_stroke,
		(void*)history_data, offsetof(STROKE_MOVE_HISTORY, layer_name) + layer_name_length + 4,
		(history_func)StrokeMoveUndoRedo, (history_func)StrokeMoveUndoRedo
	);
}

typedef struct _CONTROL_POINT_PRESSURE_HISTORY
{
	FLOAT_T before_pressure;
	FLOAT_T change_pressure;
	int32 line_index;
	int32 point_index;
	uint16 layer_name_length;
	char *layer_name;
} CONTROL_POINT_PRESSURE_HISTORY;

static void ControlPointPressureUndo(DRAW_WINDOW* window, void* p)
{
	CONTROL_POINT_PRESSURE_HISTORY *history_data =
		(CONTROL_POINT_PRESSURE_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CONTROL_POINT_PRESSURE_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	line->points[history_data->point_index].pressure = history_data->before_pressure;

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void ControlPointPressureRedo(DRAW_WINDOW* window, void* p)
{
	CONTROL_POINT_PRESSURE_HISTORY *history_data =
		(CONTROL_POINT_PRESSURE_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CONTROL_POINT_PRESSURE_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	line->points[history_data->point_index].pressure = history_data->change_pressure;

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void AddControlPointPressureHistory(
	DRAW_WINDOW* window,
	VECTOR_LINE* change_line,
	VECTOR_POINT* change_point,
	FLOAT_T before_pressure
)
{
	uint8 *byte_data;
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name);
	CONTROL_POINT_PRESSURE_HISTORY *history_data =
		(CONTROL_POINT_PRESSURE_HISTORY*)MEM_ALLOC_FUNC(
			offsetof(CONTROL_POINT_PRESSURE_HISTORY, layer_name) + layer_name_length + 4);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE *line = layer_data->base->next;
	int line_index = 0;
	int point_index;

	while(line != change_line)
	{
		line = line->next;
		line_index++;
	}

	for(point_index=0; point_index < change_line->num_points; point_index++)
	{
		if(&change_line->points[point_index] == change_point)
		{
			break;
		}
	}

	history_data->before_pressure = before_pressure;
	history_data->change_pressure = change_point->pressure;
	history_data->line_index = (int32)line_index;
	history_data->point_index = (int32)point_index;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(CONTROL_POINT_PRESSURE_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);

	AddHistory(&window->history, window->app->labels->tool_box.control.change_pressure,
		(void*)history_data, offsetof(CONTROL_POINT_PRESSURE_HISTORY, layer_name) + layer_name_length + 4,
		ControlPointPressureUndo, ControlPointPressureRedo
	);
}

#define POINT_SELECT_DISTANCE 16
#define STROKE_SELECT_DISTANCE 16

static void ControlPointToolPressCallBack
(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	CONTROL_POINT_TOOL* control = (CONTROL_POINT_TOOL*)core->brush_data;
	VECTOR_LINE* base = window->active_layer->layer_data.vector_layer_p->base;
	VECTOR_LINE* line;
	double distance;
	int end_flag = 0;
	int layer_x, layer_y;
	int start_x, end_x, start_y, end_y;
	int hit;
	int i, j;

	if(((GdkEventButton*)state)->button == 1)
	{
		switch(control->mode)
		{
		case CONTROL_POINT_MOVE:
		case CONTROL_POINT_CHANGE_PRESSURE:
			if((control->flags & CONTROL_POINT_TOOL_HAS_POINT) == 0)
			{
				if(window->active_layer->layer_data.vector_layer_p->active_point != NULL)
				{
					control->flags |= CONTROL_POINT_TOOL_HAS_POINT;
					control->change_point_data = *window->active_layer->layer_data.vector_layer_p->active_point;
					control->add_flag = 0;
				}	// if(window->active_layer->layer_data.vector_layer_p->active_point != NULL)
				else
				{
					VECTOR_LINE *add_point_line = NULL;
					int dx, dy;

					if((control->flags & CONTROL_POINT_TOOL_HAS_LOCK) == 0)
					{
						line = base->next;
						window->active_layer->layer_data.vector_layer_p->active_line = NULL;
						window->active_layer->layer_data.vector_layer_p->active_point = NULL;
						while(line != NULL && end_flag == 0)
						{
							if(line->layer != NULL)
							{
								if(x > line->layer->x && x < line->layer->x+line->layer->width
									&& y > line->layer->y && y < line->layer->y+line->layer->height)
								{
									layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);
				
									for(i=0; i<line->num_points; i++)
									{
										distance = (x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y);
										if(distance < POINT_SELECT_DISTANCE)
										{
											window->active_layer->layer_data.vector_layer_p->active_point =
												&line->points[i];
											window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
											control->flags |= CONTROL_POINT_TOOL_HAS_POINT;
											window->active_layer->layer_data.vector_layer_p->active_line = line;
		
											if(distance < 4)
											{
												break;
											}
										}
									}

									if(window->active_layer->layer_data.vector_layer_p->active_line != NULL)
									{
										break;
									}

									start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
									end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

									if(start_x < 0)
									{
										start_x = 0;
									}
									else if(end_x >= line->layer->width)
									{
										end_x = line->layer->width - 1;
									}

									if(start_y < 0)
									{
										start_y = 0;
									}
									else if(end_y >= line->layer->height)
									{
										end_y = line->layer->height - 1;
									}

									hit = 0;
									for(i=start_y; i<end_y && hit == 0; i++)
									{
										for(j=start_x; j<end_x; j++)
										{
											if(line->layer->pixels[i*line->layer->stride+j*4+3] != 0)
											{
												add_point_line = line;
												hit++;

												dx = abs(layer_x - j), dy = abs(layer_y - i);
												if(dx*dy < 4)
												{
													break;
												}
											}
										}
									}

									if(hit != 0
										&& window->active_layer->layer_data.vector_layer_p->active_line == NULL)
									{
										window->active_layer->layer_data.vector_layer_p->active_line = add_point_line;

										if(window->active_layer->layer_data.vector_layer_p->active_point == NULL
											&& control->mode == CONTROL_POINT_MOVE)
										{
											gdouble min_x, min_y, max_x, max_y;

											for(i=0; i<line->num_points-1; i++)
											{
												if(line->points[i].x < line->points[i+1].x)
												{
													min_x = line->points[i].x - dx;
													max_x = line->points[i+1].x + dx;
												}
												else
												{
													min_x = line->points[i+1].x - dx;
													max_x = line->points[i].x + dx;
												}

												if(line->points[i].y < line->points[i+1].y)
												{
													min_y = line->points[i].y - dy;
													max_y = line->points[i+1].y + dy;
												}
												else
												{
													min_y = line->points[i+1].y - dy;
													max_y = line->points[i].y + dy;
												}

												if(x >= min_x && x <= max_x && y >= min_y && y <= max_y)
												{
													(void)memmove(&line->points[i+2], &line->points[i+1], sizeof(*line->points)*(line->num_points-i-1));
													line->points[i+1].x = x, line->points[i+1].y = y;
													line->num_points++;
													if(line->num_points >= line->buff_size-1)
													{
														line->buff_size += VECTOR_LINE_BUFFER_SIZE;
														line->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
															line->points, sizeof(*line->points)*line->buff_size);
													}
													window->active_layer->layer_data.vector_layer_p->active_point = &line->points[i+1];
													window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
													control->flags |= CONTROL_POINT_TOOL_HAS_POINT;
													control->add_flag = 1;

													end_flag++;
													break;
												}
											}
										}
	
										break;
									}
								}
							}

							line = line->next;
						}
					}	// if((control->flags & CONTROL_POINT_TOOL_HAS_LOCK) == 0)
					else
					{
						line = window->active_layer->layer_data.vector_layer_p->active_line;
						if(control->mode == CONTROL_POINT_MOVE)
						{
							gdouble min_x, min_y, max_x, max_y;

							dx = abs((int)(control->lock_x-x));
							dy = abs((int)(control->lock_y-y));

							for(i=0; i<line->num_points-1; i++)
							{
								if(line->points[i].x < line->points[i+1].x)
								{
									min_x = line->points[i].x - dx;
									max_x = line->points[i+1].x + dx;
								}
								else
								{
									min_x = line->points[i+1].x - dx;
									max_x = line->points[i].x + dx;
								}

								if(line->points[i].y < line->points[i+1].y)
								{
									min_y = line->points[i].y - dy;
									max_y = line->points[i+1].y + dy;
								}
								else
								{
									min_y = line->points[i+1].y - dy;
									max_y = line->points[i].y + dy;
								}

								if(x >= min_x && x <= max_x && y >= min_y && y <= max_y)
								{
									(void)memmove(&line->points[i+2], &line->points[i+1], sizeof(*line->points)*(line->num_points-i-1));
									line->points[i+1].x = x, line->points[i+1].y = y;
									line->num_points++;
									if(line->num_points >= line->buff_size-1)
									{
										line->buff_size += VECTOR_LINE_BUFFER_SIZE;
										line->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
											line->points, sizeof(*line->points)*line->buff_size);
									}
									window->active_layer->layer_data.vector_layer_p->active_point = &line->points[i+1];
									window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
									control->flags |= CONTROL_POINT_TOOL_HAS_POINT;
									control->add_flag = 1;

									end_flag++;
									break;
								}
							}	// for(i=0; i<line->num_points-1; i++)
						}	// if(control->mode == CONTROL_POINT_MOVE)
					}	// if((control->flags & CONTROL_POINT_TOOL_HAS_LOCK) == 0) else
				}	// if(window->active_layer->layer_data.vector_layer_p->active_point != NULL) else
			}	// if((control->flags & CONTROL_POINT_TOOL_HAS_POINT) == 0)
			break;
		case CONTROL_POINT_DELETE:
			if((control->flags & CONTROL_POINT_TOOL_HAS_POINT) == 0)
			{
				if((control->flags & CONTROL_POINT_TOOL_HAS_LOCK) == 0)
				{
					line = base->next;
					window->active_layer->layer_data.vector_layer_p->active_line = NULL;
					window->active_layer->layer_data.vector_layer_p->active_point = NULL;
					while(line != NULL && end_flag == 0)
					{
						if(line->layer != NULL)
						{
							if(x > line->layer->x && x < line->layer->x+line->layer->width
								&& y > line->layer->y && y < line->layer->y+line->layer->height)
							{
								layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

								for(i=0; i<line->num_points; i++)
								{
									if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
										< POINT_SELECT_DISTANCE)
									{
										control->change_point_data = line->points[i];
										if(i < line->num_points - 1)
										{
											(void)memmove(&line->points[i], &line->points[i+1], (line->num_points-i)*sizeof(line->points[0]));
										}
										line->num_points--;
										if(line->num_points < 2)
										{
											if(line == window->active_layer->layer_data.vector_layer_p->top_line)
											{
												window->active_layer->layer_data.vector_layer_p->top_line =
													line->prev;
											}

											AddControlPointDeleteHistory(window, line, &line->points[0], &line->points[1], line->vector_type+2);
											DeleteVectorLine(&line);
											window->active_layer->layer_data.vector_layer_p->active_line = NULL;
										}
										else
										{
											AddControlPointDeleteHistory(window, line, &line->points[i], &control->change_point_data, 1);
											window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
											window->active_layer->layer_data.vector_layer_p->active_line = line;
										}
										window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
										end_flag++;
										break;
									}
								}
								break;
							}
						}

						line = line->next;
					}
				}
				else
				{
					line = window->active_layer->layer_data.vector_layer_p->active_line;
					for(i=0; i<line->num_points; i++)
					{
						if(&line->points[i] == window->active_layer->layer_data.vector_layer_p->active_point)
						{
							control->change_point_data = line->points[i];
							if(i < line->num_points - 1)
							{
								(void)memmove(&line->points[i], &line->points[i+1], (line->num_points-i)*sizeof(line->points[0]));
							}
							line->num_points--;
							if(line->num_points < 2)
							{
								if(line == window->active_layer->layer_data.vector_layer_p->top_line)
								{
									window->active_layer->layer_data.vector_layer_p->top_line =
										line->prev;
								}

								AddControlPointDeleteHistory(window, line, &line->points[0], &line->points[1], line->vector_type+2);
								DeleteVectorLine(&line);
								window->active_layer->layer_data.vector_layer_p->active_line = NULL;
							}
							else
							{
								AddControlPointDeleteHistory(window, line, &line->points[i], &control->change_point_data, 1);
								window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
								window->active_layer->layer_data.vector_layer_p->active_line = line;
							}
						}
					}
				}
			}
			break;
		case CONTROL_STROKE_MOVE:
			if((control->flags & CONTROL_POINT_TOOL_HAS_STROKE) == 0)
			{
				line = base->next;
				window->active_layer->layer_data.vector_layer_p->active_line = NULL;
				window->active_layer->layer_data.vector_layer_p->active_point = NULL;
				while(line != NULL)
				{
					if(line->layer != NULL)
					{
						if(x > line->layer->x && x < line->layer->x+line->layer->width
							&& y > line->layer->y && y < line->layer->y+line->layer->height)
						{
							layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

							for(i=0; i<line->num_points; i++)
							{
								if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
									< POINT_SELECT_DISTANCE)
								{
									window->active_layer->layer_data.vector_layer_p->active_point =
										&line->points[i];
									control->change_point_data = line->points[0];
									window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
									control->flags |= CONTROL_POINT_TOOL_HAS_POINT;
									window->active_layer->layer_data.vector_layer_p->active_line = line;
									break;
								}
							}

							if(window->active_layer->layer_data.vector_layer_p->active_line != NULL)
							{
								break;
							}

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->layer->width)
							{
								end_x = line->layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->layer->height)
							{
								end_y = line->layer->height - 1;
							}

							hit = 0;
							for(i=start_y; i<end_y && hit == 0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->layer->pixels[i*line->layer->stride+j*4+3] != 0)
									{
										hit++;
										break;
									}
								}
							}

							if(hit != 0
								&& window->active_layer->layer_data.vector_layer_p->active_line == NULL)
							{
								window->active_layer->layer_data.vector_layer_p->active_line = line;
								control->change_point_data = line->points[0];
							}
						}
					}

					line = line->next;
				}

				if(window->active_layer->layer_data.vector_layer_p->active_line != NULL)
				{
					control->flags |= CONTROL_POINT_TOOL_HAS_STROKE;
					control->before_x = x;
					control->before_y = y;
				}
			}
		}
	}
	else if(((GdkEventButton*)state)->button == 3)
	{	// 右クリックなら
		// 制御点操作ツールの詳細データにキャスト
		CONTROL_POINT_TOOL *control = (CONTROL_POINT_TOOL*)core->brush_data;
		// 一番下のライン
		VECTOR_LINE *base = window->active_layer->layer_data.vector_layer_p->base;
		// 当たり判定っぽいことをするラインへのポインタ
		VECTOR_LINE *line;
		// 2週目のフラグ
		int second_loop = 0;
		// ラインレイヤーの座標
		int layer_x, layer_y;
		// 当たり判定の開始・終了座標
		int start_x, end_x, start_y, end_y;
		// ループ終了のフラグ
		int end_flag = 0;
		// 当たり判定のフラグ
		int hit = 0;
		int i, j;	// for文用のカウンタ

		switch(control->mode)
		{
		case CONTROL_POINT_SELECT:
		case CONTROL_POINT_CHANGE_PRESSURE:
		case CONTROL_POINT_DELETE:
			if(window->active_layer->layer_data.vector_layer_p->active_point != NULL)
			{
				VECTOR_POINT *before_active = window->active_layer->layer_data.vector_layer_p->active_point;

				// ラインレイヤーのピクセルα値が0でなければ当たり
				line = window->active_layer->layer_data.vector_layer_p->active_line->next;
				// 右クリックで選択された場合は一定距離マウスが移動するまで選択ラインを変更しない
				control->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// マウスの座標を記憶
				control->lock_x = x, control->lock_y = y;

				while(hit == 0)
				{
					if(line != NULL && line->layer != NULL)
					{
						if(line->layer != NULL)
						{
							if(x > line->layer->x && x < line->layer->x+line->layer->width
								&& y > line->layer->y && y < line->layer->y+line->layer->height)
							{
								for(i=0; i<line->num_points; i++)
								{	// マウスカーソル付近に
									if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
										<= POINT_SELECT_DISTANCE)
									{
										window->active_layer->layer_data.vector_layer_p->active_line = line;
										(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
										cairo_set_source_surface(window->work_layer->cairo_p, line->layer->surface_p,
											line->layer->x, line->layer->y);
										cairo_paint(window->work_layer->cairo_p);
										window->active_layer->layer_data.vector_layer_p->active_point = &line->points[i];
										hit++;
										break;
									}
								}
							}
						}

						line = line->next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->next;
							second_loop++;
						}
						else
						{
							break;
						}
					}
				}

				if(window->active_layer->layer_data.vector_layer_p->active_point == NULL)
				{
					window->active_layer->layer_data.vector_layer_p->active_point = before_active;
				}
			}
			break;
		case CONTROL_POINT_MOVE:
			if(window->active_layer->layer_data.vector_layer_p->active_point != NULL)
			{
				VECTOR_POINT *before_active = window->active_layer->layer_data.vector_layer_p->active_point;

				// ラインレイヤーのピクセルα値が0でなければ当たり
				line = window->active_layer->layer_data.vector_layer_p->active_line->next;
				// 右クリックで選択された場合は一定距離マウスが移動するまで選択ラインを変更しない
				control->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// マウスの座標を記憶
				control->lock_x = x, control->lock_y = y;

				while(hit == 0)
				{
					if(line != NULL && line->layer != NULL)
					{
						if(line->layer != NULL)
						{
							if(x > line->layer->x && x < line->layer->x+line->layer->width
								&& y > line->layer->y && y < line->layer->y+line->layer->height)
							{
								for(i=0; i<line->num_points; i++)
								{	// マウスカーソル付近に
									if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
										<= POINT_SELECT_DISTANCE)
									{
										window->active_layer->layer_data.vector_layer_p->active_line = line;
										(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
										cairo_set_source_surface(window->work_layer->cairo_p, line->layer->surface_p,
											line->layer->x, line->layer->y);
										cairo_paint(window->work_layer->cairo_p);
										window->active_layer->layer_data.vector_layer_p->active_point = &line->points[i];
										hit++;
										break;
									}
								}
							}
						}

						line = line->next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->next;
							second_loop++;
						}
						else
						{
							break;
						}
					}
				}

				if(window->active_layer->layer_data.vector_layer_p->active_point == NULL)
				{
					window->active_layer->layer_data.vector_layer_p->active_point = before_active;
				}
			}
			else if(window->active_layer->layer_data.vector_layer_p->active_line != NULL)
			{
				VECTOR_LINE *before_active = window->active_layer->layer_data.vector_layer_p->active_line;

				// ラインレイヤーのピクセルα値が0でなければ当たり
				line = window->active_layer->layer_data.vector_layer_p->active_line->next;
				// 右クリックで選択された場合は一定距離マウスが移動するまで選択ラインを変更しない
				control->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// マウスの座標を記憶
				control->lock_x = x, control->lock_y = y;

				while(hit == 0)
				{	// 各レイヤー持ちのラインに対して処理実行
					if(line != NULL && line->layer != NULL)
					{
						if(x > line->layer->x && x < line->layer->x+line->layer->width
							&& y > line->layer->y && y < line->layer->y+line->layer->height)
						{
							layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->layer->width)
							{
								end_x = line->layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->layer->height)
							{
								end_y = line->layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->layer->pixels[i*line->layer->stride+j*4+3] != 0)
									{
										window->active_layer->layer_data.vector_layer_p->active_line = line;
										(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
										cairo_set_source_surface(window->work_layer->cairo_p, line->layer->surface_p,
											line->layer->x, line->layer->y);
										cairo_paint(window->work_layer->cairo_p);
										hit++;

										if(i*i+j*j < 4)
										{
											j = -2;
											break;
										}
									}
								}
							}

							if(hit != 0)
							{
								break;
							}
						}

						line = line->next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->next;
							second_loop++;
						}
						else
						{
							break;
						}
					}
				}

				if(window->active_layer->layer_data.vector_layer_p->active_line == NULL)
				{
					window->active_layer->layer_data.vector_layer_p->active_line =
						before_active;
				}
			}
			break;
		case CONTROL_STROKE_MOVE:
		case CONTROL_STROKE_COPY_MOVE:
		case CONTROL_STROKE_JOINT:
			if(window->active_layer->layer_data.vector_layer_p->active_line != NULL)
			{
				VECTOR_LINE *before_active = window->active_layer->layer_data.vector_layer_p->active_line;

				// ラインレイヤーのピクセルα値が0でなければ当たり
				line = window->active_layer->layer_data.vector_layer_p->active_line->next;
				// 右クリックで選択された場合は一定距離マウスが移動するまで選択ラインを変更しない
				control->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// マウスの座標を記憶
				control->lock_x = x, control->lock_y = y;

				while(hit == 0)
				{	// 各レイヤー持ちのラインに対して処理実行
					if(line != NULL && line->layer != NULL)
					{
						if(x > line->layer->x && x < line->layer->x+line->layer->width
							&& y > line->layer->y && y < line->layer->y+line->layer->height)
						{
							layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->layer->width)
							{
								end_x = line->layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->layer->height)
							{
								end_y = line->layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->layer->pixels[i*line->layer->stride+j*4+3] != 0)
									{
										window->active_layer->layer_data.vector_layer_p->active_line = line;
										(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
										cairo_set_source_surface(window->work_layer->cairo_p, line->layer->surface_p,
											line->layer->x, line->layer->y);
										cairo_paint(window->work_layer->cairo_p);
										hit++;

										if(i*i+j*j < 4)
										{
											j = -2;
											break;
										}
									}
								}
							}

							if(hit != 0)
							{
								break;
							}
						}

						line = line->next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->next;
							second_loop++;
						}
						else
						{
							break;
						}
					}
				}

				if(window->active_layer->layer_data.vector_layer_p->active_line == NULL)
				{
					window->active_layer->layer_data.vector_layer_p->active_line =
						before_active;
				}
			}
			break;
		}
	}
}

static void ControlPointToolMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
#define CHANGE_LINE_DISTANCE 20
	CONTROL_POINT_TOOL* control = (CONTROL_POINT_TOOL*)core->brush_data;
	VECTOR_LINE* base = window->active_layer->layer_data.vector_layer_p->base;
	VECTOR_LINE* line;
	double distance;
	int layer_x, layer_y;
	int i;

	if((control->flags & CONTROL_POINT_TOOL_HAS_LOCK) != 0)
	{
		if((control->lock_x-x)*(control->lock_x-x)+(control->lock_y-y)*(control->lock_y-y)
			<= CHANGE_LINE_DISTANCE)
		{
			return;
		}
	}
	control->flags &= ~(CONTROL_POINT_TOOL_HAS_LOCK);

	switch(control->mode)
	{
	case CONTROL_POINT_MOVE:
		if((control->flags & CONTROL_POINT_TOOL_HAS_POINT) == 0
			&& (window->active_layer->layer_data.vector_layer_p->flags & VECTOR_LAYER_FIX_LINE) == 0)
		{
			double distance;

			line = base->next;
			window->active_layer->layer_data.vector_layer_p->active_line = NULL;
			window->active_layer->layer_data.vector_layer_p->active_point = NULL;
			while(line != NULL)
			{
				if(line->layer != NULL)
				{
					if(x > line->layer->x && x < line->layer->x+line->layer->width
						&& y > line->layer->y && y < line->layer->y+line->layer->height)
					{
						layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);
						
						for(i=0; i<line->num_points; i++)
						{
							distance = (x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y);
							if(distance < POINT_SELECT_DISTANCE)
							{
								window->active_layer->layer_data.vector_layer_p->active_point =
									&line->points[i];
								window->active_layer->layer_data.vector_layer_p->active_line = line;
								
								if(distance < 4)
								{
									break;
								}
							}
						}

						if(line->layer->pixels[layer_y*line->layer->stride+layer_x*4+3] != 0
							&& window->active_layer->layer_data.vector_layer_p->active_line == NULL)
						{
							window->active_layer->layer_data.vector_layer_p->active_line = line;
							(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
							cairo_set_source_surface(window->work_layer->cairo_p, line->layer->surface_p,
								line->layer->x, line->layer->y);
							cairo_paint(window->work_layer->cairo_p);
							break;
						}
					}
				}

				line = line->next;
			}
		}
		else
		{
			if(window->active_layer->layer_data.vector_layer_p->active_point != NULL)
			{
				window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
				window->active_layer->layer_data.vector_layer_p->active_point->x = x;
				window->active_layer->layer_data.vector_layer_p->active_point->y = y;
			}
		}

		break;
	case CONTROL_POINT_CHANGE_PRESSURE:
		if((control->flags & CONTROL_POINT_TOOL_HAS_POINT) == 0
			&& (window->active_layer->layer_data.vector_layer_p->flags & VECTOR_LAYER_FIX_LINE) == 0)
		{
			line = base->next;
			window->active_layer->layer_data.vector_layer_p->active_line = NULL;
			window->active_layer->layer_data.vector_layer_p->active_point = NULL;
			while(line != NULL)
			{
				if(line->layer != NULL)
				{
					if(x > line->layer->x && x < line->layer->x+line->layer->width
						&& y > line->layer->y && y < line->layer->y+line->layer->height)
					{
						layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);
						
						for(i=0; i<line->num_points; i++)
						{
							distance = (x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y);
							if(distance < POINT_SELECT_DISTANCE)
							{
								window->active_layer->layer_data.vector_layer_p->active_point =
									&line->points[i];
								window->active_layer->layer_data.vector_layer_p->active_line = line;

								if(distance < 4)
								{
									break;
								}
							}
						}

						if(line->layer->pixels[layer_y*line->layer->stride+layer_x*4+3] != 0
							&& window->active_layer->layer_data.vector_layer_p->active_line == NULL)
						{
							window->active_layer->layer_data.vector_layer_p->active_line = line;
							(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
							cairo_set_source_surface(window->work_layer->cairo_p, line->layer->surface_p,
								line->layer->x, line->layer->y);
							cairo_paint(window->work_layer->cairo_p);
							break;
						}
					}
				}

				line = line->next;
			}
		}
		else
		{
			window->active_layer->layer_data.vector_layer_p->active_point->pressure =
				(x - window->active_layer->layer_data.vector_layer_p->active_point->x) * window->zoom * 0.01f + 100;
			if(window->active_layer->layer_data.vector_layer_p->active_point->pressure < 0.0)
			{
				window->active_layer->layer_data.vector_layer_p->active_point->pressure = 0.0;
			}
			else if(window->active_layer->layer_data.vector_layer_p->active_point->pressure > 200.0)
			{
				window->active_layer->layer_data.vector_layer_p->active_point->pressure = 200.0;
			}
			window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
		}

		break;
	case CONTROL_POINT_DELETE:
		line = base->next;
		window->active_layer->layer_data.vector_layer_p->active_line = NULL;
		window->active_layer->layer_data.vector_layer_p->active_point = NULL;
		while(line != NULL)
		{
			if(line->layer != NULL)
			{
				if(x > line->layer->x && x < line->layer->x+line->layer->width
					&& y > line->layer->y && y < line->layer->y+line->layer->height)
				{
					layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);
					
					for(i=0; i<line->num_points; i++)
					{
						if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
							< POINT_SELECT_DISTANCE)
						{
							window->active_layer->layer_data.vector_layer_p->active_point =
								&line->points[i];
							window->active_layer->layer_data.vector_layer_p->active_line = line;
							break;
						}
					}

					if(line->layer->pixels[layer_y*line->layer->stride+layer_x*4+3] != 0
						&& window->active_layer->layer_data.vector_layer_p->active_line == NULL)
					{
						window->active_layer->layer_data.vector_layer_p->active_line = line;
						break;
					}
				}
			}

			line = line->next;
		}
		break;
	case CONTROL_STROKE_MOVE:
		if((control->flags & CONTROL_POINT_TOOL_HAS_STROKE) == 0)
		{
			line = base->next;
			window->active_layer->layer_data.vector_layer_p->active_line = NULL;
			window->active_layer->layer_data.vector_layer_p->active_point = NULL;
			while(line != NULL)
			{
				if(line->layer != NULL)
				{
					if(x > line->layer->x && x < line->layer->x+line->layer->width
						&& y > line->layer->y && y < line->layer->y+line->layer->height)
					{
						layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);
						
						for(i=0; i<line->num_points; i++)
						{
							if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
								< POINT_SELECT_DISTANCE)
							{
								window->active_layer->layer_data.vector_layer_p->active_point =
									&line->points[i];
								window->active_layer->layer_data.vector_layer_p->active_line = line;
								break;
							}
						}

						if(line->layer->pixels[layer_y*line->layer->stride+layer_x*4+3] != 0
							&& window->active_layer->layer_data.vector_layer_p->active_line == NULL)
						{
							window->active_layer->layer_data.vector_layer_p->active_line = line;
							(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
							cairo_set_source_surface(window->work_layer->cairo_p, line->layer->surface_p,
								line->layer->x, line->layer->y);
							cairo_paint(window->work_layer->cairo_p);
							break;
						}
					}
				}

				line = line->next;
			}
		}
		else
		{
			VECTOR_LINE* line = window->active_layer->layer_data.vector_layer_p->active_line;
			gdouble dx = x - control->before_x;
			gdouble dy = y - control->before_y;

			for(i=0; i<line->num_points; i++)
			{
				line->points[i].x += dx;
				line->points[i].y += dy;
			}

			window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;

			control->before_x = x;
			control->before_y = y;
		}
		break;
	}
}

static void ControlPointToolReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	CONTROL_POINT_TOOL* control = (CONTROL_POINT_TOOL*)core->brush_data;
	VECTOR_LINE* base = window->active_layer->layer_data.vector_layer_p->base;
	//VECTOR_LINE* line;

	if(((GdkEventButton*)state)->button == 1)
	{
		switch(control->mode)
		{
		case CONTROL_POINT_MOVE:
			if((control->flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
			{
				window->active_layer->layer_data.vector_layer_p->flags |=
					VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
				AddControlPointMoveHistory(window, window->active_layer->layer_data.vector_layer_p->active_line,
					window->active_layer->layer_data.vector_layer_p->active_point, &control->change_point_data,
					(uint16)control->add_flag
				);
			}
			break;
		case CONTROL_POINT_CHANGE_PRESSURE:
			if((control->flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
			{
				window->active_layer->layer_data.vector_layer_p->flags |=
					VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
				AddControlPointPressureHistory(window, window->active_layer->layer_data.vector_layer_p->active_line,
					window->active_layer->layer_data.vector_layer_p->active_point, control->change_point_data.pressure);
			}
			break;
		case CONTROL_STROKE_MOVE:
			if((control->flags & CONTROL_POINT_TOOL_HAS_STROKE) != 0)
			{
				window->active_layer->layer_data.vector_layer_p->flags |=
					VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
				AddStrokeMoveHistory(window, window->active_layer->layer_data.vector_layer_p->active_line,
					&control->change_point_data);
			}
			break;
		}
	}

	RasterizeVectorLayer(window, window->active_layer, window->active_layer->layer_data.vector_layer_p);

	control->flags = 0;
}

static void ControlPointToolDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
#define DRAW_POINT_RADIUS 2
	char str[8];
	unsigned int i;

	CONTROL_POINT_TOOL* control = (CONTROL_POINT_TOOL*)data;
	VECTOR_LAYER* layer = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE* line = layer->base->next;
	gdouble rev_zoom = window->rev_zoom;

	cairo_save(window->disp_temp->cairo_p);
	ClipUpdateArea(window, window->disp_layer->cairo_p);
	ClipUpdateArea(window, window->disp_temp->cairo_p);

	cairo_set_operator(window->disp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	if(layer->active_line != NULL && layer->active_line->layer != NULL)
	{	
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		cairo_set_source_rgb(window->disp_layer->cairo_p, 1, 0, 0);
#else
		cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 0, 1);
#endif
		cairo_scale(window->disp_temp->cairo_p, window->zoom*0.01f, window->zoom*0.01f);
		cairo_set_source_surface(window->disp_temp->cairo_p, window->work_layer->surface_p,
			0, 0);
		cairo_paint(window->disp_temp->cairo_p);
		cairo_rectangle(window->disp_layer->cairo_p, layer->active_line->layer->x,
			layer->active_line->layer->y, layer->active_line->layer->width, layer->active_line->layer->height);
		cairo_mask_surface(window->disp_layer->cairo_p, window->disp_temp->surface_p, 0, 0);
		cairo_scale(window->disp_temp->cairo_p, rev_zoom, rev_zoom);
		(void)memset(window->disp_temp->pixels, 0, window->disp_temp->stride*window->disp_temp->height);
		cairo_new_path(window->disp_layer->cairo_p);
	}

	while(line != NULL)
	{
		cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 1, 0);
		cairo_arc(window->disp_layer->cairo_p, line->points->x*window->zoom_rate,
			line->points->y*window->zoom_rate, DRAW_POINT_RADIUS, 0, 2*G_PI);
		cairo_fill(window->disp_layer->cairo_p);

#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 1, 1);
#else
		cairo_set_source_rgb(window->disp_layer->cairo_p, 1, 1, 0);
#endif
		for(i=1; i<(unsigned int)(line->num_points-1); i++)
		{
			cairo_arc(window->disp_layer->cairo_p, line->points[i].x*window->zoom_rate,
				line->points[i].y*window->zoom_rate, DRAW_POINT_RADIUS, 0, 2*G_PI);
			cairo_fill(window->disp_layer->cairo_p);
		}

		cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 1, 0);
		cairo_arc(window->disp_layer->cairo_p, line->points[i].x*window->zoom_rate,
			line->points[i].y*window->zoom_rate, DRAW_POINT_RADIUS, 0, 2*G_PI);
		cairo_fill(window->disp_layer->cairo_p);

		line = line->next;
	}

	if(layer->active_point != NULL)
	{
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 0, 1);
#else
		cairo_set_source_rgb(window->disp_layer->cairo_p, 1, 0, 0);
#endif
		cairo_arc(window->disp_layer->cairo_p, layer->active_point->x*window->zoom_rate,
			layer->active_point->y*window->zoom_rate, DRAW_POINT_RADIUS, 0, 2*G_PI);
		cairo_fill(window->disp_layer->cairo_p);

		if(control->mode == CONTROL_POINT_CHANGE_PRESSURE)
		{
			int pressure = (int)layer->active_point->pressure;
			(void)sprintf(str, "%d%%", pressure);
			cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 0, 0);
			cairo_move_to(window->disp_layer->cairo_p,
				layer->active_point->x*window->zoom_rate, layer->active_point->y*window->zoom_rate);
			cairo_show_text(window->disp_layer->cairo_p, str);
		}
	}
#undef DRAW_POINT_RADIUS

	cairo_restore(window->disp_temp->cairo_p);
}

static void ControlPointToolModeChange(GtkWidget* widget, CONTROL_POINT_TOOL* control)
{
	control->mode = (uint8)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "control_point_mode"));
}

static GtkWidget* CreateControlPointToolDetailUI(APPLICATION* app, void* data)
{
	CONTROL_POINT_TOOL* control = (CONTROL_POINT_TOOL*)data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget* buttons[NUM_CONTROL_POINT_MODE];
	int i;

	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.control.select);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.control.move);
	buttons[2] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.control.change_pressure);
	buttons[3] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.control.delete_point);
	buttons[4] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.control.move_stroke);
	buttons[5] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.control.copy_stroke);
	buttons[6] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.control.joint_stroke);

	for(i=0; i<sizeof(buttons)/sizeof(*buttons); i++)
	{
		gtk_box_pack_start(GTK_BOX(vbox), buttons[i], FALSE, TRUE, 0);
		g_object_set_data(G_OBJECT(buttons[i]), "control_point_mode", GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(buttons[i]), "toggled", G_CALLBACK(ControlPointToolModeChange), data);
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[control->mode]), TRUE);

	return vbox;
}

typedef struct _CONTROL_POINT_CHANGE_COLOR_HISTORY
{
	uint8 change_color[4];
	int32 line_index;
	int32 point_index;
	uint16 layer_name_length;
	char *layer_name;
	uint8 (*before_color)[4];
} CONTROL_POINT_CHANGE_COLOR_HISTORY;

static void ChangeLineColorUndo(DRAW_WINDOW* window, void* p)
{
	CONTROL_POINT_CHANGE_COLOR_HISTORY *history_data =
		(CONTROL_POINT_CHANGE_COLOR_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	uint8 (*before_color)[4] = (uint8 (*)[4])(&((uint8*)history_data)[
		offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name) + history_data->layer_name_length+1]);
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	for(i=0; i<line->num_points; i++)
	{
		(void)memcpy(line->points[i].color, before_color[i], 4);
	}

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void ChangeLineColorRedo(DRAW_WINDOW* window, void* p)
{
	CONTROL_POINT_CHANGE_COLOR_HISTORY *history_data =
		(CONTROL_POINT_CHANGE_COLOR_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	for(i=0; i<line->num_points; i++)
	{
		(void)memcpy(line->points[i].color, history_data->change_color, 4);
	}

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void AddChangeLineColorHistory(
	DRAW_WINDOW* window,
	VECTOR_LINE* change_line,
	uint8 change_color[4],
	char* tool_name
)
{
	uint8 *byte_data;
	uint8 (*before_color)[4];
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name);
	size_t data_size = offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name) + layer_name_length
			+ change_line->num_points * 4 + 4;
	CONTROL_POINT_CHANGE_COLOR_HISTORY *history_data =
		(CONTROL_POINT_CHANGE_COLOR_HISTORY*)MEM_ALLOC_FUNC(data_size);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE *line = layer_data->base->next;
	int line_index = 0;
	int i;

	while(line != change_line)
	{
		line = line->next;
		line_index++;
	}

	(void)memcpy(history_data->change_color, change_color, 4);
	history_data->line_index = (int32)line_index;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);
	before_color = (uint8 (*)[4])(&byte_data[
		(offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name) + layer_name_length + 1)]);
	for(i=0; i<change_line->num_points; i++)
	{
		(void)memcpy(before_color[i], change_line->points[i].color, 4);
	}

	AddHistory(&window->history, tool_name, history_data, (uint32)data_size,
		ChangeLineColorUndo, ChangeLineColorRedo);
}

static void ChangePointColorUndo(DRAW_WINDOW* window, void* p)
{
	CONTROL_POINT_CHANGE_COLOR_HISTORY *history_data =
		(CONTROL_POINT_CHANGE_COLOR_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	uint8 (*before_color)[4] = (uint8 (*)[4])(&((uint8*)history_data)[
		offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name) + history_data->layer_name_length+1]);
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	(void)memcpy(line->points[history_data->point_index].color, *before_color, 4);

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void ChangePointColorRedo(DRAW_WINDOW* window, void* p)
{
	CONTROL_POINT_CHANGE_COLOR_HISTORY *history_data =
		(CONTROL_POINT_CHANGE_COLOR_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	uint8 (*before_color)[4] = (uint8 (*)[4])(&((uint8*)history_data)[
		offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name) + history_data->layer_name_length+1]);
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	(void)memcpy(line->points[history_data->point_index].color, history_data->change_color, 4);

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void AddChangePointColorHistory(
	DRAW_WINDOW* window,
	VECTOR_LINE* change_line,
	VECTOR_POINT* change_point,
	uint8 change_color[4],
	char* tool_name
)
{
	uint8 *byte_data;
	uint8 (*before_color)[4];
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name);
	size_t data_size = offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name) + layer_name_length
			+ 4 + 4;
	CONTROL_POINT_CHANGE_COLOR_HISTORY *history_data =
		(CONTROL_POINT_CHANGE_COLOR_HISTORY*)MEM_ALLOC_FUNC(data_size);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE *line = layer_data->base->next;
	int line_index = 0;
	int point_index;

	while(line != change_line)
	{
		line = line->next;
		line_index++;
	}

	for(point_index=0; point_index<line->num_points; point_index++)
	{
		if(&line->points[point_index] == change_point)
		{
			break;
		}
	}

	(void)memcpy(history_data->change_color, change_color, 4);
	history_data->line_index = (int32)line_index;
	history_data->point_index = point_index;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);
	before_color = (uint8 (*)[4])(&byte_data[
		(offsetof(CONTROL_POINT_CHANGE_COLOR_HISTORY, layer_name) + layer_name_length + 1)]);
	(void)memcpy(*before_color, change_point->color, 4);

	AddHistory(&window->history, tool_name, history_data, (uint32)data_size,
		ChangePointColorUndo, ChangePointColorRedo);
}

/*****************************************************
* LineColorChangeBrushPressCallBack関数              *
* ラインの色変更ツールのクリック時のコールバック関数 *
* 引数                                               *
* window	: 描画領域の情報                         *
* x			: マウスのX座標                          *
* y			: マウスのY座標                          *
* pressure	: 筆圧                                   *
* core		: ベクトルブラシの基本情報               *
* state		: マウスイベントの情報                   *
*****************************************************/
static void LineColorChangeBrushPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	// 左クリックなら
	if(((GdkEventButton*)state)->button == 1)
	{	// ライン上なら色変更

		// ライン色変更ツールの詳細データにキャスト
		CHANGE_LINE_COLOR_TOOL* tool = (CHANGE_LINE_COLOR_TOOL*)core->brush_data;
		// 一番下のライン
		VECTOR_LINE* base = window->active_layer->layer_data.vector_layer_p->base;
		// 当たり判定っぽいことをするラインへのポインタ
		VECTOR_LINE* line;
		// ラインレイヤーの座標
		int layer_x, layer_y;
		// 当たり判定の開始・終了座標
		int start_x, end_x, start_y, end_y;
		// ループ終了のフラグ
		int end_flag = 0;
		// for文用のカウンタ
		int i, j;
		// 当たり判定のフラグ
		int hit;

		// ラインレイヤーのピクセルα値が0でなければ当たり
		line = base->next;
		window->active_layer->layer_data.vector_layer_p->active_line = NULL;
		window->active_layer->layer_data.vector_layer_p->active_point = NULL;
		
		switch(tool->mode)
		{
		case CHANGE_LINE_COLOR_MODE:	// ライン色変更
			while(line != NULL)
			{	// 各レイヤー持ちのラインに対して処理実行

				if(window->active_layer->layer_data.vector_layer_p->active_line == NULL)
				{
					if(line->layer != NULL)
					{
						if(x > line->layer->x && x < line->layer->x+line->layer->width
							&& y > line->layer->y && y < line->layer->y+line->layer->height)
						{
							layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->layer->width)
							{
								end_x = line->layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->layer->height)
							{
								end_y = line->layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->layer->pixels[i*line->layer->stride+j*4+3] != 0
										&& (memcmp(line->points->color, *core->color, 3) != 0 || line->points->color[3] != tool->flow))
									{
										window->active_layer->layer_data.vector_layer_p->active_line = line;
										hit++;

										if(i*i+j*j < 4)
										{
											j = -2;
											break;
										}
									}
								}
							}

							if(hit != 0)
							{
								uint8 change_color[4] = {(*core->color)[0], (*core->color)[1], (*core->color)[2], tool->flow};
								window->active_layer->layer_data.vector_layer_p->active_line = line;

								// 履歴データを追加
								AddChangeLineColorHistory(window, line, change_color, core->name);

								// 色変更
								for(i=0; i<line->num_points; i++)
								{
									line->points[i].color[0] = (*core->color)[0];
									line->points[i].color[1] = (*core->color)[1];
									line->points[i].color[2] = (*core->color)[2];
									line->points[i].color[3] = tool->flow;
								}

								// ラインレイヤーの作り直し
								window->active_layer->layer_data.vector_layer_p->flags |=
									VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

								break;
							}
						}

						line = line->next;
					}
				}
				else
				{
					uint8 change_color[4] = {(*core->color)[0], (*core->color)[1], (*core->color)[2], tool->flow};
					line = window->active_layer->layer_data.vector_layer_p->active_line;

					// 履歴データを追加
					AddChangeLineColorHistory(window, line, change_color, core->name);

					// 色変更
					for(i=0; i<line->num_points; i++)
					{
						line->points[i].color[0] = (*core->color)[0];
						line->points[i].color[1] = (*core->color)[1];
						line->points[i].color[2] = (*core->color)[2];
						line->points[i].color[3] = tool->flow;
					}

					// ラインレイヤーの作り直し
					window->active_layer->layer_data.vector_layer_p->flags |=
						VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
				}

				tool->flags &= ~(CONTROL_POINT_TOOL_HAS_LOCK);
			}
			break;
		case CHANGE_POINT_COLOR_MODE:	// 制御点色変更
			while(line != NULL && end_flag == 0)
			{	// 各レイヤー持ちのラインに対して処理実行
				if(line->layer != NULL)
				{
					if(x > line->layer->x && x < line->layer->x+line->layer->width
						&& y > line->layer->y && y < line->layer->y+line->layer->height)
					{
						for(i=0; i<line->num_points; i++)
						{	// マウスカーソルの位置に制御点があれば色変更
							if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
								<= POINT_SELECT_DISTANCE && memcmp(line->points[i].color, *core->color, 3) != 0)
							{
								uint8 change_color[4] = {(*core->color)[0], (*core->color)[1], (*core->color)[2], tool->flow};
								// 履歴データを追加
								AddChangePointColorHistory(window, line, &line->points[i], change_color, core->name);

								line->points[i].color[0] = (*core->color)[0];
								line->points[i].color[1] = (*core->color)[1];
								line->points[i].color[2] = (*core->color)[2];
								line->points[i].color[3] = tool->flow;

								window->active_layer->layer_data.vector_layer_p->active_line = line;
								// ラインレイヤーの作り直し
								window->active_layer->layer_data.vector_layer_p->flags |=
									VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

								end_flag++;

								break;
							}
						}
					}
				}

				line = line->next;
			}
			break;
		}
	}
	else if(((GdkEventButton*)state)->button == 3)
	{	// 右クリックなら
		// ライン色変更ツールの詳細データにキャスト
		CHANGE_LINE_COLOR_TOOL* tool = (CHANGE_LINE_COLOR_TOOL*)core->brush_data;
		// 一番下のライン
		VECTOR_LINE* base = window->active_layer->layer_data.vector_layer_p->base;
		// 当たり判定っぽいことをするラインへのポインタ
		VECTOR_LINE* line;
		// 2週目のフラグ
		int second_loop = 0;
		// ラインレイヤーの座標
		int layer_x, layer_y;
		// 当たり判定の開始・終了座標
		int start_x, end_x, start_y, end_y;
		// ループ終了のフラグ
		int end_flag = 0;
		// for文用のカウンタ
		int i, j;
		// 当たり判定のフラグ
		int hit = 0;

		switch(tool->mode)
		{
		case CHANGE_LINE_COLOR_MODE:	// ライン色変更
			if(window->active_layer->layer_data.vector_layer_p->active_line != NULL)
			{
				VECTOR_LINE *before_active = window->active_layer->layer_data.vector_layer_p->active_line;

				// ラインレイヤーのピクセルα値が0でなければ当たり
				line = window->active_layer->layer_data.vector_layer_p->active_line->next;
				// 右クリックで選択された場合は一定距離マウスが移動するまで選択ラインを変更しない
				tool->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// マウスの座標を記憶
				tool->lock_x = x, tool->lock_y = y;

				while(hit == 0)
				{	// 各レイヤー持ちのラインに対して処理実行
					if(line != NULL && line->layer != NULL)
					{
						if(x > line->layer->x && x < line->layer->x+line->layer->width
							&& y > line->layer->y && y < line->layer->y+line->layer->height)
						{
							layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->layer->width)
							{
								end_x = line->layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->layer->height)
							{
								end_y = line->layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->layer->pixels[i*line->layer->stride+j*4+3] != 0
										&& memcmp(line->points->color, *core->color, 3) != 0)
									{
										window->active_layer->layer_data.vector_layer_p->active_line = line;
										hit++;

										if(i*i+j*j < 4)
										{
											j = -2;
											break;
										}
									}
								}
							}

							if(hit != 0)
							{
								break;
							}
						}

						line = line->next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->next;
							second_loop++;
						}
						else
						{
							break;
						}
					}
				}

				if(window->active_layer->layer_data.vector_layer_p->active_line == NULL)
				{
					window->active_layer->layer_data.vector_layer_p->active_line =
						before_active;
				}
			}
			break;
		}
	}
}

static void LineColorChangeBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
#define CHANGE_LINE_DISTANCE 20
	// ライン色変更ツールの詳細データにキャスト
	CHANGE_LINE_COLOR_TOOL* tool = (CHANGE_LINE_COLOR_TOOL*)core->brush_data;
	// 一番下のライン
	VECTOR_LINE* base = window->active_layer->layer_data.vector_layer_p->base;
	// 当たり判定っぽいことをするラインへのポインタ
	VECTOR_LINE* line;
	// ループ終了のフラグ
	int end_flag = 0;
	// for文用のカウンタ
	int i;

	// 色変更処理中であれば中断
	if((window->active_layer->layer_data.vector_layer_p->flags & VECTOR_LAYER_FIX_LINE) != 0)
	{
		return;
	}

	// 制御点からの距離が一定以下なら当たり
	line = base->next;
	window->active_layer->layer_data.vector_layer_p->active_line = NULL;
	window->active_layer->layer_data.vector_layer_p->active_point = NULL;

	if(tool->mode == CHANGE_LINE_COLOR_MODE)
	{
		FLOAT_T distance = sqrt((x-tool->lock_x)*(x-tool->lock_x)+(y-tool->lock_y)*(y-tool->lock_y));
		// ラインレイヤーの座標
		int layer_x, layer_y;
		// 当たり判定の開始・終了座標
		int start_x, end_x, start_y, end_y;
		// ピクセルのインデックス
		int index;
		// for文用のカウンタ
		int i, j;
		// 当たり判定のフラグ
		int hit;

		if((tool->flags & CONTROL_POINT_TOOL_HAS_LOCK) == 0
			|| distance >= CHANGE_LINE_DISTANCE)
		{
			tool->flags &= ~(CONTROL_POINT_TOOL_HAS_LOCK);

			while(line != NULL && end_flag == 0)
			{
				// 各レイヤー持ちのラインに対して処理実行
				if(line->layer != NULL)
				{
					if(x > line->layer->x && x < line->layer->x+line->layer->width
						&& y > line->layer->y && y < line->layer->y+line->layer->height)
					{
						layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

						start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
						end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

						if(start_x < 0)
						{
							start_x = 0;
						}
						else if(end_x >= line->layer->width)
						{
							end_x = line->layer->width - 1;
						}

						if(start_y < 0)
						{
							start_y = 0;
						}
						else if(end_y >= line->layer->height)
						{
							end_y = line->layer->height - 1;
						}

						hit = 0, j = 0;
						for(i=start_y; i<end_y && j>=0; i++)
						{
							for(j=start_x; j<end_x; j++)
							{
								index = i*line->layer->stride+j*4+3;
								if(index < line->layer->stride*line->layer->height)
								{
									if(line->layer->pixels[index] != 0
										&& (memcmp(line->points->color, *core->color, 3) != 0 || line->points->color[3] != tool->flow))
									{
										window->active_layer->layer_data.vector_layer_p->active_line = line;
										hit++;

										if(i*i+j*j < 4)
										{
											j = -2;
											break;
										}
									}
								}
							}
						}
					}
				}	// 各レイヤー持ちのラインに対して処理実行
					// if(line->layer != NULL)
				line = line->next;
			}	// while(line != NULL && end_flag == 0)
		}
	}
	else if(tool->mode == CHANGE_POINT_COLOR_MODE)
	{
		while(line != NULL && end_flag == 0)
		{	// 各レイヤー持ちのラインに対して処理実行
			if(line->layer != NULL)
			{
				if(x > line->layer->x && x < line->layer->x+line->layer->width
					&& y > line->layer->y && y < line->layer->y+line->layer->height)
				{
					for(i=0; i<line->num_points; i++)
					{	// マウスカーソルの位置に制御点があれば強調
						if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
							<= POINT_SELECT_DISTANCE)
						{
							window->active_layer->layer_data.vector_layer_p->active_point =
								&line->points[i];
							end_flag++;
							break;
						}
					}
				}
			}

			line = line->next;
		}
	}
#undef CHANGE_LINE_DISTANCE
}

#define LineColorChangeBrushReleaseCallBack VectorBrushDummyCallBack

static void LineColorChangeBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
#define DRAW_POINT_RADIUS 2
	CHANGE_LINE_COLOR_TOOL* color = (CHANGE_LINE_COLOR_TOOL*)data;

	cairo_save(window->disp_temp->cairo_p);
	ClipUpdateArea(window, window->disp_layer->cairo_p);
	ClipUpdateArea(window, window->disp_temp->cairo_p);

	if(color->mode == CHANGE_LINE_COLOR_MODE)
	{
		VECTOR_LAYER* layer = window->active_layer->layer_data.vector_layer_p;
		VECTOR_LINE* line = layer->base->next;
		gdouble rev_zoom = 1.0 / (window->zoom * 0.01);

		if(layer->active_line != NULL)
		{	
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
			cairo_set_source_rgb(window->disp_layer->cairo_p, 1, 0, 0);
#else
			cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 0, 1);
#endif
			cairo_scale(window->disp_temp->cairo_p, window->zoom*0.01, window->zoom*0.01);
			cairo_set_source_surface(window->disp_temp->cairo_p,
				layer->active_line->layer->surface_p, layer->active_line->layer->x, layer->active_line->layer->y);
			cairo_paint(window->disp_temp->cairo_p);
			cairo_rectangle(window->disp_layer->cairo_p, layer->active_line->layer->x,
				layer->active_line->layer->y, layer->active_line->layer->width, layer->active_line->layer->height);
			cairo_mask_surface(window->disp_layer->cairo_p, window->disp_temp->surface_p, 0, 0);
			cairo_scale(window->disp_temp->cairo_p, rev_zoom, rev_zoom);
			(void)memset(window->disp_temp->pixels, 0, window->disp_temp->stride*window->disp_temp->height);
			cairo_new_path(window->disp_layer->cairo_p);
		}
	}
	else if(color->mode == CHANGE_POINT_COLOR_MODE)
	{
		unsigned int i;

		VECTOR_LAYER* layer = window->active_layer->layer_data.vector_layer_p;
		VECTOR_LINE* line = layer->base->next;
		gdouble rev_zoom = 1.0 / (window->zoom * 0.01);

		while(line != NULL)
		{
			cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 1, 0);
			cairo_arc(window->disp_layer->cairo_p, line->points->x*(window->zoom)*0.01f,
				line->points->y*(window->zoom)*0.01f, DRAW_POINT_RADIUS, 0, 2*G_PI);
			cairo_fill(window->disp_layer->cairo_p);

#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
			cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 1, 1);
#else
			cairo_set_source_rgb(window->disp_layer->cairo_p, 1, 1, 0);
#endif
			for(i=1; i<(unsigned int)(line->num_points-1); i++)
			{
				cairo_arc(window->disp_layer->cairo_p, line->points[i].x*(window->zoom)*0.01f,
					line->points[i].y*(window->zoom)*0.01f, DRAW_POINT_RADIUS, 0, 2*G_PI);
				cairo_fill(window->disp_layer->cairo_p);
			}

			cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 1, 0);
			cairo_arc(window->disp_layer->cairo_p, line->points[i].x*(window->zoom)*0.01f,
				line->points[i].y*(window->zoom)*0.01f, DRAW_POINT_RADIUS, 0, 2*G_PI);
			cairo_fill(window->disp_layer->cairo_p);

			line = line->next;
		}

		if(layer->active_point != NULL)
		{
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
			cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 0, 1);
#else
			cairo_set_source_rgb(window->disp_layer->cairo_p, 1, 0, 0);
#endif
			cairo_arc(window->disp_layer->cairo_p, layer->active_point->x*(window->zoom)*0.01f,
				layer->active_point->y*window->zoom*0.01f, DRAW_POINT_RADIUS, 0, 2*G_PI);
			cairo_fill(window->disp_layer->cairo_p);
		}
	}
#undef DRAW_POINT_RADIUS

	cairo_restore(window->disp_temp->cairo_p);
}

static void ChangeLineColorFlow(GtkAdjustment* slider, CHANGE_LINE_COLOR_TOOL* color)
{
	color->flow = (uint8)(gtk_adjustment_get_value(slider) * 2.55 + 0.5);
}

static void ChangeLineColorMode(GtkWidget* widget, CHANGE_LINE_COLOR_TOOL* color)
{
	color->mode = (uint8)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "mode"));
}

static GtkWidget* CreateLineColorChangeBrushDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	// ツールの詳細データにキャスト
	CHANGE_LINE_COLOR_TOOL* color = (CHANGE_LINE_COLOR_TOOL*)data;
	// 詳細設定ウィジェットを入れるパッキングボックス
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	// スライダ
	GtkWidget* scale;
	// ウィジェット整列用のテーブル
	GtkWidget* table;
	// ラベル
	GtkWidget* label;
	// ラジオボタン
	GtkWidget* radio_buttons[2];
	// スライダに設定するアジャスタ
	GtkAdjustment* scale_adjustment;
	// ラベルのフォントサイズ変更用のマークアップバッファ
	char mark_up_buff[256];

	// 濃度変更ウィジェット
	table = gtk_table_new(1, 3, FALSE);
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		color->flow * DIV_PIXEL * 100, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeLineColorFlow), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.flow, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// 色変更対象決定用ラジオボタン
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.target);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	radio_buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.stroke);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.control_point
	);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[color->mode]), TRUE);
	g_object_set_data(G_OBJECT(radio_buttons[0]), "mode", GINT_TO_POINTER(0));
	g_signal_connect(G_OBJECT(radio_buttons[0]), "toggled", G_CALLBACK(ChangeLineColorMode), color);
	g_object_set_data(G_OBJECT(radio_buttons[1]), "mode", GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(radio_buttons[1]), "toggled", G_CALLBACK(ChangeLineColorMode), color);
	table = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[1], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	return vbox;
#undef UI_FONT_SIZE
}

typedef struct _CHANGE_LINE_SIZE_HISTORY
{
	int32 line_index;
	uint16 layer_name_length;
	char *layer_name;
	uint8 *before_flow;
	gdouble *before_size;
} CHANGE_LINE_SIZE_HISTORY;

static void ChangeLineSizeUndoRedo(DRAW_WINDOW* window, void* p)
{
	CHANGE_LINE_SIZE_HISTORY *history_data =
		(CHANGE_LINE_SIZE_HISTORY*)p;
	LAYER *layer = window->layer;
	char* name = &(((char*)history_data)[offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	uint8 *buff = (uint8*)history_data;
	gdouble *before_size, temp_size;
	uint8 *before_flow, temp_flow;
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = layer->layer_data.vector_layer_p->base->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->next;
	}
	layer->layer_data.vector_layer_p->active_line = line;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	before_flow = &buff[offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name) + history_data->layer_name_length + 1];
	before_size = (gdouble*)&buff[offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name) + history_data->layer_name_length + 1 + line->num_points];

	for(i=0; i<line->num_points; i++)
	{
		temp_flow = line->points[i].color[3];
		line->points[i].color[3] = before_flow[i];
		before_flow[i] = temp_flow;
		temp_size = line->points[i].size;
		line->points[i].size = before_size[i];
		before_size[i] = temp_size;
	}

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void AddChangeLineSizeHistory(
	DRAW_WINDOW* window,
	VECTOR_LINE* change_line,
	char* tool_name
)
{
	uint8 *byte_data;
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name);
	size_t data_size = offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name) + layer_name_length
		+ change_line->num_points * sizeof(gdouble) + change_line->num_points * sizeof(uint8) + 4;
	CHANGE_LINE_SIZE_HISTORY *history_data =
		(CHANGE_LINE_SIZE_HISTORY*)MEM_ALLOC_FUNC(data_size);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE *line = layer_data->base->next;
	uint8 *buff = (uint8*)history_data;
	uint8 *before_flow;
	gdouble *before_size;
	int line_index = 0;
	int i;

	before_flow = &buff[offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name) + layer_name_length + 1];
	before_size = (gdouble*)&buff[offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name) + layer_name_length + 1 + line->num_points];

	while(line != change_line)
	{
		line = line->next;
		line_index++;
	}

	history_data->line_index = (int32)line_index;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);

	for(i=0; i<change_line->num_points; i++)
	{
		before_flow[i] = change_line->points[i].color[3];
		before_size[i] = change_line->points[i].size;
	}

	AddHistory(&window->history, tool_name, history_data, (uint32)data_size,
		ChangeLineSizeUndoRedo, ChangeLineSizeUndoRedo);
}

/*******************************************************
* LineSizeChangeBrushPressCallBack関数                 *
* ラインの種類変更ツールのクリック時のコールバック関数 *
* 引数                                                 *
* window	: 描画領域の情報                           *
* x			: マウスのX座標                            *
* y			: マウスのY座標                            *
* pressure	: 筆圧                                     *
* core		: ベクトルブラシの基本情報                 *
* state		: マウスイベントの情報                     *
*******************************************************/
static void LineSizeChangeBrushPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	// 左クリックなら
	if(((GdkEventButton*)state)->button == 1)
	{	// ライン上ならサイズ変更

		// 設定するラインのサイズ情報
		CHANGE_LINE_SIZE_TOOL* size = (CHANGE_LINE_SIZE_TOOL*)core->brush_data;
		// 一番下のライン
		VECTOR_LINE* base = window->active_layer->layer_data.vector_layer_p->base;
		// 当たり判定っぽいことをするラインへのポインタ
		VECTOR_LINE* line;
		// ラインレイヤーの座標
		int layer_x, layer_y;
		// 当たり判定の開始・終了座標
		int start_x, end_x, start_y, end_y;
		// 当たり判定のフラグ
		int hit;
		// for文用のカウンタ
		int i, j;

		// ラインレイヤーのピクセルα値が0でなければ当たり
		if(window->active_layer->layer_data.vector_layer_p->active_line == NULL)
		{
			line = base->next;
			window->active_layer->layer_data.vector_layer_p->active_line = NULL;
			window->active_layer->layer_data.vector_layer_p->active_point = NULL;

			while(line != NULL)
			{	// 各レイヤー持ちのラインに対して処理実行
				if(window->active_layer->layer_data.vector_layer_p->active_line == NULL)
				{
					if(line->layer != NULL)
					{
						if(x > line->layer->x && x < line->layer->x+line->layer->width
							&& y > line->layer->y && y < line->layer->y+line->layer->height)
						{
							layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->layer->width)
							{
								end_x = line->layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->layer->height)
							{
								end_y = line->layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->layer->pixels[i*line->layer->stride+j*4+3] != 0)
									{
										window->active_layer->layer_data.vector_layer_p->active_line = line;
										hit++;

										if(i*i+j*j < 4)
										{
											j = -2;
											break;
										}
									}
								}
							}

							if(hit != 0)
							{
								window->active_layer->layer_data.vector_layer_p->active_line = line;

								// 履歴データを追加
								AddChangeLineSizeHistory(window, line, core->name);

								// サイズ変更
								line->blur = size->blur;
								line->outline_hardness = size->outline_hardness;
								for(i=0; i<line->num_points; i++)
								{
									line->points[i].size = size->r;
									line->points[i].color[3] = size->flow;
								}

								// ラインレイヤーの作り直し
								window->active_layer->layer_data.vector_layer_p->flags |=
									VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
							}

							break;
						}
					}

					line = line->next;
				}
				else
				{
					// 履歴データを追加
					line = window->active_layer->layer_data.vector_layer_p->active_line;
					AddChangeLineSizeHistory(window, line, core->name);

					// サイズ変更
					line->blur = size->blur;
					line->outline_hardness = size->outline_hardness;
					for(i=0; i<line->num_points; i++)
					{
						line->points[i].size = size->r;
						line->points[i].color[3] = size->flow;
					}

					// ラインレイヤーの作り直し
					window->active_layer->layer_data.vector_layer_p->flags |=
						VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
				}
			}
		}
		else
		{
			line = window->active_layer->layer_data.vector_layer_p->active_line;

			// 履歴データを追加
			AddChangeLineSizeHistory(window, line, core->name);

			// サイズ変更
			line->blur = size->blur;
			line->outline_hardness = size->outline_hardness;
			for(i=0; i<line->num_points; i++)
			{
				line->points[i].size = size->r;
				line->points[i].color[3] = size->flow;
			}

			// ラインレイヤーの作り直し
			window->active_layer->layer_data.vector_layer_p->flags |=
				VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
		}

		size->flags &= ~(CONTROL_POINT_TOOL_HAS_LOCK);
	}
	else if(((GdkEventButton*)state)->button == 3)
	{	// 右クリックなら
		// ライン色変更ツールの詳細データにキャスト
		CHANGE_LINE_SIZE_TOOL* size = (CHANGE_LINE_SIZE_TOOL*)core->brush_data;
		// 一番下のライン
		VECTOR_LINE* base = window->active_layer->layer_data.vector_layer_p->base;
		// 当たり判定っぽいことをするラインへのポインタ
		VECTOR_LINE* line;
		// 2週目のフラグ
		int second_loop = 0;
		// ラインレイヤーの座標
		int layer_x, layer_y;
		// 当たり判定の開始・終了座標
		int start_x, end_x, start_y, end_y;
		// ループ終了のフラグ
		int end_flag = 0;
		// for文用のカウンタ
		int i, j;
		// 当たり判定のフラグ
		int hit = 0;

		if(window->active_layer->layer_data.vector_layer_p->active_line != NULL)
		{
			VECTOR_LINE *before_active = window->active_layer->layer_data.vector_layer_p->active_line;

			// ラインレイヤーのピクセルα値が0でなければ当たり
			line = window->active_layer->layer_data.vector_layer_p->active_line->next;
			// 右クリックで選択された場合は一定距離マウスが移動するまで選択ラインを変更しない
			size->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
			// マウスの座標を記憶
			size->lock_x = x, size->lock_y = y;

			while(hit == 0)
			{	// 各レイヤー持ちのラインに対して処理実行
				if(line != NULL && line->layer != NULL)
				{
					if(x > line->layer->x && x < line->layer->x+line->layer->width
						&& y > line->layer->y && y < line->layer->y+line->layer->height)
					{
						layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

						start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
						end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

						if(start_x < 0)
						{
							start_x = 0;
						}
						else if(end_x >= line->layer->width)
						{
							end_x = line->layer->width - 1;
						}

						if(start_y < 0)
						{
							start_y = 0;
						}
						else if(end_y >= line->layer->height)
						{
							end_y = line->layer->height - 1;
						}

						hit = 0, j = 0;
						for(i=start_y; i<end_y && j>=0; i++)
						{
							for(j=start_x; j<end_x; j++)
							{
								if(line->layer->pixels[i*line->layer->stride+j*4+3] != 0
									&& memcmp(line->points->color, *core->color, 3) != 0)
								{
									window->active_layer->layer_data.vector_layer_p->active_line = line;
									hit++;

									if(i*i+j*j < 4)
									{
										j = -2;
										break;
									}
								}
							}
						}

						if(hit != 0)
						{
							break;
						}
					}

					line = line->next;
				}
				else
				{
					if(second_loop == 0)
					{
						line = base->next;
						second_loop++;
					}
					else
					{
						break;
					}
				}
			}

			if(window->active_layer->layer_data.vector_layer_p->active_line == NULL)
			{
				window->active_layer->layer_data.vector_layer_p->active_line =
					before_active;
			}
		}
	}
}

static void LineSizeChangeBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
#define CHANGE_LINE_DISTANCE 20
	// ライン色変更ツールの詳細データにキャスト
	CHANGE_LINE_SIZE_TOOL* size = (CHANGE_LINE_SIZE_TOOL*)core->brush_data;
	// 一番下のライン
	VECTOR_LINE* base = window->active_layer->layer_data.vector_layer_p->base;
	// 当たり判定っぽいことをするラインへのポインタ
	VECTOR_LINE* line;
	// ループ終了のフラグ
	int end_flag = 0;

	// 色変更処理中であれば中断
	if((window->active_layer->layer_data.vector_layer_p->flags & VECTOR_LAYER_FIX_LINE) != 0)
	{
		return;
	}

	// 制御点からの距離が一定以下なら当たり
	line = base->next;
	window->active_layer->layer_data.vector_layer_p->active_line = NULL;
	window->active_layer->layer_data.vector_layer_p->active_point = NULL;

	{
		FLOAT_T distance = sqrt((x-size->lock_x)*(x-size->lock_x)+(y-size->lock_y)*(y-size->lock_y));
		// ラインレイヤーの座標
		int layer_x, layer_y;
		// 当たり判定の開始・終了座標
		int start_x, end_x, start_y, end_y;
		// for文用のカウンタ
		int i, j;
		// 当たり判定のフラグ
		int hit;

		if((size->flags & CONTROL_POINT_TOOL_HAS_LOCK) == 0
			|| distance >= CHANGE_LINE_DISTANCE)
		{
			size->flags &= ~(CONTROL_POINT_TOOL_HAS_LOCK);

			while(line != NULL && end_flag == 0)
			{
				// 各レイヤー持ちのラインに対して処理実行
				if(line->layer != NULL)
				{
					if(x > line->layer->x && x < line->layer->x+line->layer->width
						&& y > line->layer->y && y < line->layer->y+line->layer->height)
					{
						layer_x = (int)(x - line->layer->x), layer_y = (int)(y - line->layer->y);

						start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
						end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

						if(start_x < 0)
						{
							start_x = 0;
						}
						else if(end_x >= line->layer->width)
						{
							end_x = line->layer->width - 1;
						}

						if(start_y < 0)
						{
							start_y = 0;
						}
						if(end_y + start_y >= line->layer->height)
						{
							end_y = line->layer->height - start_y - 1;
						}

						hit = 0, j = 0;
						for(i=start_y; i<end_y && j>=0; i++)
						{
							for(j=start_x; j<end_x; j++)
							{
								if(line->layer->pixels[i*line->layer->stride+j*4+3] != 0)
								{
									window->active_layer->layer_data.vector_layer_p->active_line = line;
									hit++;

									if(i*i+j*j < 4)
									{
										j = -2;
										break;
									}
								}
							}
						}
					}
				}	// 各レイヤー持ちのラインに対して処理実行
					// if(line->layer != NULL)
				line = line->next;
			}	// while(line != NULL && end_flag == 0)
		}
	}
#undef CHANGE_LINE_DISTANCE
}

#define LineSizeChangeBrushReleaseCallBack VectorBrushDummyCallBack

static void LineSizeChangeBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	VECTOR_LAYER* layer = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE* line = layer->base->next;
	gdouble rev_zoom = 1.0 / (window->zoom * 0.01);

	if(layer->active_line != NULL)
	{	
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		cairo_set_source_rgb(window->disp_layer->cairo_p, 1, 0, 0);
#else
		cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 0, 1);
#endif
		cairo_scale(window->disp_temp->cairo_p, window->zoom*0.01, window->zoom*0.01);
		cairo_set_source_surface(window->disp_temp->cairo_p,
			layer->active_line->layer->surface_p, layer->active_line->layer->x, layer->active_line->layer->y);
		cairo_paint(window->disp_temp->cairo_p);
		cairo_rectangle(window->disp_layer->cairo_p, layer->active_line->layer->x,
			layer->active_line->layer->y, layer->active_line->layer->width, layer->active_line->layer->height);
		cairo_mask_surface(window->disp_layer->cairo_p, window->disp_temp->surface_p, 0, 0);
		cairo_scale(window->disp_temp->cairo_p, rev_zoom, rev_zoom);
		(void)memset(window->disp_temp->pixels, 0, window->disp_temp->stride*window->disp_temp->height);
		cairo_new_path(window->disp_layer->cairo_p);
	}
}

/*****************************************************************
* ChangeLineSizeScale関数                                        *
* ラインサイズ変更ツールのサイズスライダ操作時のコールバック関数 *
* 引数                                                           *
* slider	: スライダのアジャスタ                               *
* size		: ラインサイズ変更ツールのデータ                     *
*****************************************************************/
static void ChangeLineSizeScale(
	GtkAdjustment* slider,
	CHANGE_LINE_SIZE_TOOL* size
)
{
	size->r = gtk_adjustment_get_value(slider)*0.5;
}

/*****************************************************************
* ChangeLineSizeBlur関数                                         *
* ラインサイズ変更ツールのボケ足スライダ操作時のコールバック関数 *
* 引数                                                           *
* slider	: スライダのアジャスタ                               *
* size		: ラインサイズ変更ツールのデータ                     *
*****************************************************************/
static void ChangeLineSizeBlur(
	GtkAdjustment* slider,
	CHANGE_LINE_SIZE_TOOL* size
)
{
	size->blur = gtk_adjustment_get_value(slider);
}

/*********************************************************************
* ChangeLineSizeOutlineHarndess関数                                  *
* ラインサイズ変更ツールの輪郭の硬さスライダ操作時のコールバック関数 *
* 引数                                                               *
* slider	: スライダのアジャスタ                                   *
* size		: ラインサイズ変更ツールのデータ                         *
*********************************************************************/
static void ChangeLineSizeOutlineHardness(
	GtkAdjustment* slider,
	CHANGE_LINE_SIZE_TOOL* size
)
{
	size->outline_hardness = gtk_adjustment_get_value(slider);
}

/***************************************************************
* ChangeLineSizeFlow関数                                       *
* ラインサイズ変更ツールの濃度スライダ操作時のコールバック関数 *
* 引数                                                         *
* slider	: スライダのアジャスタ                             *
* size		: ラインサイズ変更ツールのデータ                   *
***************************************************************/
static void ChangeLineSizeFlow(
	GtkAdjustment* slider,
	CHANGE_LINE_SIZE_TOOL* size
)
{
	size->flow = (uint8)(gtk_adjustment_get_value(slider) * 2.55 + 0.5);
}

/*****************************************************
* CreateChangeLineSizeDetailUI関数                   *
* ラインサイズ変更ツールの詳細UIを生成               *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
* data	: ツールの詳細データ                         *
*****************************************************/
static GtkWidget* CreateChangeLineSizeDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	// 詳細データをキャスト
	CHANGE_LINE_SIZE_TOOL* size = (CHANGE_LINE_SIZE_TOOL*)data;
	// ウィジェットを並べるパッキングボックス
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	// スライダ
	GtkWidget* scale;
	// ウィジェット並べるテーブル
	GtkWidget* table;
	// スライダに適用するアジャスタ
	GtkAdjustment* scale_adjustment;

	// サイズ変更ウィジェット
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(size->r*2, 1, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeLineSizeScale), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.brush_scale, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// ボケ足変更ウィジェット
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(size->blur, 0, 255, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeLineSizeBlur), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.blur, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// 輪郭の硬さ変更ウィジェット
	table = gtk_table_new(1, 3, FALSE);
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(size->outline_hardness, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeLineSizeOutlineHardness), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.outline_hardness, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// 濃度変更ウィジェット
	table = gtk_table_new(1, 3, FALSE);
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		size->flow / 2.55, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeLineSizeFlow), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.flow, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	return vbox;
}

/***************************************************************
* VectorEraserPressCallBack関数                                *
* ベクトル消しゴムツールでのマウスクリック時のコールバック関数 *
* 引数                                                         *
* window	: 描画領域の情報                                   *
* x			: マウスのX座標                                    *
* y			: マウスのY座標                                    *
* pressure	: 筆圧                                             *
* core		: ベクトルブラシの基本情報                         *
* state		: マウスイベントの情報                             *
***************************************************************/
static void VectorEraserPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	// 左クリックなら
	if(((GdkEventButton*)state)->button == 1)
	{
		VECTOR_ERASER* eraser = (VECTOR_ERASER*)core->brush_data;
		gdouble r;
		gdouble min_x, min_y, max_x, max_y;

		window->work_layer->layer_mode = LAYER_BLEND_ALPHA_MINUS;
		(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

		r = ((eraser->flags & VECTOR_ERASER_PRESSURE_SIZE) == 0) ?
			eraser->r : eraser->r * pressure;

		min_x = x - r, min_y = y - r;
		max_x = x + r, max_y = y + r;

		eraser->before_x = x, eraser->before_y = y;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		eraser->min_x = min_x, eraser->min_y = min_y;
		eraser->max_x = max_x, eraser->max_y = max_y;

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);

		cairo_set_source_rgb(window->work_layer->cairo_p, 0, 0, 0);
		cairo_arc(window->work_layer->cairo_p, x, y, r, 0, 2*G_PI);
		cairo_fill(window->work_layer->cairo_p);
	}
}

/***************************************************************
* VectorEraserMotionCallBack関数                               *
* ベクトル消しゴムツールでのマウスドラッグ時のコールバック関数 *
* 引数                                                         *
* window	: 描画領域の情報                                   *
* x			: マウスのX座標                                    *
* y			: マウスのY座標                                    *
* pressure	: 筆圧                                             *
* core		: ベクトルブラシの基本情報                         *
* state		: マウスイベントの情報                             *
***************************************************************/
static void VectorEraserMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		VECTOR_ERASER* eraser = (VECTOR_ERASER*)core->brush_data;
		gdouble r, d, arg;
		gdouble min_x, min_y, max_x, max_y;
		gdouble draw_x = x, draw_y = y;
		gdouble dx, dy, cos_x, sin_y;
		int32 clear_x, clear_width, clear_y, clear_height;
		int i, j, k;

		r = ((eraser->flags & VECTOR_ERASER_PRESSURE_SIZE) == 0) ?
			eraser->r : eraser->r * pressure;
		dx = x-eraser->before_x, dy = y-eraser->before_y;
		d = sqrt(dx*dx+dy*dy);
		arg = atan2(dy, dx);
		cos_x = cos(arg), sin_y = sin(arg);

		min_x = x - r, min_y = y - r;
		max_x = x + r, max_y = y + r;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		if(eraser->min_x > min_x)
		{
			eraser->min_x = min_x;
		}
		if(eraser->min_y > min_y)
		{
			eraser->min_y = min_y;
		}
		if(eraser->max_x < max_x)
		{
			eraser->max_x = max_x;
		}
		if(eraser->max_y < max_y)
		{
			eraser->max_y = max_y;
		}

		cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
		dx = d;
		do
		{
			clear_x = (int32)(draw_x - r - 1);
			clear_width = (int32)(draw_x + r + 1);
			clear_y = (int32)(draw_y - r - 1);
			clear_height = (int32)(draw_y + r + 1);

			if(clear_x < 0)
			{
				clear_x = 0;
			}
			if(clear_width >= window->width)
			{
				clear_width = window->width;
			}
			if(clear_y < 0)
			{
				clear_y = 0;
			}
			if(clear_height >= window->height)
			{
				clear_height = window->height;
			}
			clear_width = clear_width - clear_x;
			clear_height = clear_height - clear_y;

			if(clear_width <= 0 || clear_height <= 0)
			{
				goto skip_draw;
			}

			for(i=0; i<clear_height; i++)
			{
				(void)memset(&window->temp_layer->pixels[(i+clear_y)*window->work_layer->stride+clear_x*window->work_layer->channel],
					0x0, clear_width*window->work_layer->channel);
			}
			(void)memset(window->temp_layer->pixels, 0x0, window->temp_layer->height*window->temp_layer->stride);

			cairo_set_source_rgb(window->temp_layer->cairo_p, 0, 0, 0);
			cairo_arc(window->temp_layer->cairo_p, draw_x, draw_y, r, 0, 2*G_PI);
			cairo_fill(window->temp_layer->cairo_p);
		
			for(i=0; i<clear_height; i++)
			{
				for(j=0; j<clear_width; j++)
				{
					if(window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3]
						> window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+3])
					{
						for(k=0; k<window->temp_layer->channel; k++)
						{
							window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k] =
								(uint32)(((int)window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+k]
								- (int)window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k])
									* window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3] >> 8)
									+ window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k];
						}
					}
				}
			}

skip_draw:
			
			draw_x -= cos_x, draw_y -= sin_y;
			dx -= 1;
		} while(dx > 0);

		eraser->before_x = x, eraser->before_y = y;
	}
}

/*************************************************************************
* VectorEraserReleaseCallBack関数                                        *
* ベクトル消しゴムツールでのマウスのボタンが離された時のコールバック関数 *
* 引数                                                                   *
* window	: 描画領域の情報                                             *
* x			: マウスのX座標                                              *
* y			: マウスのY座標                                              *
* pressure	: 筆圧                                                       *
* core		: ベクトルブラシの基本情報                                   *
* state		: マウスイベントの情報                                       *
*************************************************************************/
static void VectorEraserReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
#define VECTOR_ERASER_BUFFER_SIZE 1024
	// 左クリックなら
	if(((GdkEventButton*)state)->button == 1)
	{
		// ベクトル消しゴムの詳細データにキャスト
		VECTOR_ERASER* eraser = (VECTOR_ERASER*)core->brush_data;
		// 履歴用のデータ
		MEMORY_STREAM_PTR divide_change = NULL, divide_add = NULL;
		MEMORY_STREAM_PTR src_points_stream;
		DIVIDE_LINE_CHANGE_DATA divide_line;
		DIVIDE_LINE_ADD_DATA add_line;
		int num_divide = 0;
		// ベジェ曲線の制御点
		BEZIER_POINT bezier_points[4], controls[4];
		// 変更前の制御点
		VECTOR_POINT *src_points;
		// ベジェ曲線の座標
		BEZIER_POINT check_point;
		// ベクトルレイヤー中のライン
		VECTOR_LINE *line = window->active_layer->layer_data.vector_layer_p->base->next;
		// 分割により新たに作成されたライン
		VECTOR_LINE *new_line = NULL;
		// 1つのラインに対して追加されたラインの数
		int this_add;
		// 変更のある線のデータ
		VECTOR_LINE *change_lines = (VECTOR_LINE*)MEM_ALLOC_FUNC(
			sizeof(*change_lines)*VECTOR_ERASER_BUFFER_SIZE);
		// 変更のある線の数
		uint32 num_change_line = 0;
		// 変更のある線記憶用のバッファサイズ
		uint32 buffer_size = VECTOR_ERASER_BUFFER_SIZE;
		// 検査中の線のインデックス
		uint32 line_index = 0;
		// 変更する線のインデックス
		uint32 *change_line_indexes = NULL;
		// 制御点間の距離
		gdouble d, dx, dy;
		// 制御点間の角度
		gdouble arg;
		// ベジェ曲線で使用するt
		gdouble t;
		// チェック中のX座標、Y座標
		gdouble check_x, check_y;
		// 分割前の制御点の数
		int before_num_points;
		// 分割前の制御点
		VECTOR_POINT before_point;
		// 分割前の配列のインデックス
		int before_index;
		// 座標の整数値
		int int_x, int_y;
		// cos、sinの値
		gdouble cos_x, sin_y;
		// ピクセルのインデックス
		int index;
		// ループ終了のフラグ
		int end_flag;
		// ライン分割のフラグ
		int in_devide;
		// for文用のカウンタ
		int i, j;
		unsigned int k;

		if(eraser->mode == VECTOR_ERASER_STROKE_DIVIDE)
		{
			divide_change = CreateMemoryStream(4096);
			divide_add = CreateMemoryStream(4096);
			src_points_stream = CreateMemoryStream(4096);
		}
		else
		{
			change_line_indexes = (uint32*)MEM_ALLOC_FUNC(
				sizeof(*change_line_indexes)*VECTOR_ERASER_BUFFER_SIZE);
		}

		// 消した領域データをコピー
		for(i=0; i<window->width*window->height; i++)
		{
			window->brush_buffer[i] = window->work_layer->pixels[i*4+3];
		}

		// レイヤー中の全てのラインをチェック
		while(line != NULL)
		{
			// 領域がラインと重なるかをチェック
			if(line->layer != NULL && line->layer->x <= eraser->max_x
				&& line->layer->x + line->layer->width >= eraser->min_x
				&& line->layer->y <= eraser->max_y
				&& line->layer->y + line->layer->height >= eraser->min_y
			)
			{
				// ベクトル消しゴムのモードで処理切り替え
				switch(eraser->mode)
				{
				case VECTOR_ERASER_STROKE_DIVIDE:
					// ラインをストロークして消した領域と重なったらライン分割
					in_devide = 0;
					end_flag = 0;
					this_add = 0;
					before_num_points = line->num_points;
					// 現在の制御点情報をコピー
					src_points_stream->data_point = 0;
					(void)MemWrite(line->points, 1,
						sizeof(*line->points)*line->num_points, src_points_stream);
					src_points = (VECTOR_POINT*)src_points_stream->buff_ptr;

					if(line->vector_type == VECTOR_LINE_STRAIGHT
						|| line->num_points == 2)
					{	// 直線
						for(i=0; i<before_num_points-1; i++)
						{
							dx = src_points[i].x - src_points[i+1].x;
							dy = src_points[i].y - src_points[i+1].y;
							d = sqrt(dx*dx+dy*dy);
							t = 0;
							arg = atan2(dy, dx);
							cos_x = cos(arg), sin_y = sin(arg);
							check_x = src_points[i].x, check_y = src_points[i].y;

							t = 0;
							do
							{
								int_x = (int)check_x, int_y = (int)check_y;
								index = int_y*window->width+int_x;

								// 消した領域と重なったのでライン削分割
								if(index >= 0 && index < window->height * window->width)
								{
									if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8 && in_devide % 2 == 0)
									{
										if(in_devide == 0)
										{
											num_divide++;

											// 変更前のライン情報を履歴データとして残す
											divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
												+ sizeof(VECTOR_POINT)*(line->num_points + i + 2) - sizeof(divide_line.data_size);
											divide_line.added = 0;
											divide_line.before_num_points = line->num_points;
											divide_line.after_num_points = i + 2;
											divide_line.index = line_index;
											divide_line.line_type = line->vector_type;
											(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
											(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

											// 終端変更前の制御点情報を記憶
											before_point = line->points[i+1];

											// 現在の座標を終端にする
											line->points[i+1].x = check_x, line->points[i+1].y = check_y;
											line->num_points = i + 2;

											(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

											// ラインをラスタライズするフラグを立てる
											line->flags |= VECTOR_LINE_FIX;
										}
										else if(new_line != NULL)
										{
											add_line.data_size = offsetof(DIVIDE_LINE_ADD_DATA, after)
												+ sizeof(*line->points)*(end_flag - i) - sizeof(add_line.data_size);
											new_line->num_points = before_num_points - end_flag;
											new_line->points[new_line->num_points-1].x = check_x;
											new_line->points[new_line->num_points-1].y = check_y;
											add_line.index = line_index;
											add_line.line_type = new_line->vector_type;
											add_line.num_points = new_line->num_points;
											(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
											(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

											num_change_line++;
											new_line = NULL;
										}

										// ライン分割中のフラグを立てる
										in_devide++;
									}
									else if(window->brush_buffer[int_y*window->width+int_x] < 0x08 && in_devide % 2 != 0)
									{
										// 現在の座標を始点にして新たなライン作成
										new_line = CreateVectorLine(line, line->next);
										new_line->vector_type = line->vector_type;
										new_line->num_points = before_num_points - i;
										new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
											* VECTOR_LINE_BUFFER_SIZE;
										// 制御点の設定
										new_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*new_line->points)*new_line->buff_size);
										(void)memset(new_line->points, 0, sizeof(*new_line->points)*new_line->buff_size);
										new_line->points->x = check_x, new_line->points->y = check_y;
										new_line->points->pressure = src_points[i].pressure;
										new_line->points->size = src_points[i].size;
										(void)memcpy(new_line->points->color, src_points[i].color, 4);
										new_line->vector_type = line->vector_type;
										new_line->points[1] = before_point;
										this_add++;

										if(new_line->num_points > 2)
										{
											(void)memcpy(&new_line->points[2], &src_points[i+2],
												sizeof(*new_line->points)*(new_line->num_points-2));
										}
										// ラインをラスタライズするフラグを立てる
										new_line->flags |= VECTOR_LINE_FIX;

										// 現在の制御点数を記憶
										end_flag = i;

										// ライン分割終了
										in_devide++;
									}
								}

								t += 1;
								check_x -= cos_x, check_y -= sin_y;
							} while(t < d);

							if(new_line != NULL)
							{
								num_change_line++;
								add_line.data_size = offsetof(DIVIDE_LINE_ADD_DATA, after)
									+ sizeof(*line->points)*(new_line->num_points) - sizeof(add_line.data_size);
								new_line->points[new_line->num_points-1].x = check_x;
								new_line->points[new_line->num_points-1].y = check_y;
								add_line.index = line_index;
								add_line.line_type = new_line->vector_type;
								add_line.num_points = new_line->num_points;
								(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
								(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

								new_line = NULL;
							}
						}
					}
					else if(0)//line->vector_type == VECTOR_LINE_BEZIER_OPEN && line->num_points == 3)
					{	// ベジェ曲線
						dx = src_points[0].x-src_points[1].x;
						dy = src_points[0].y-src_points[1].y;
						d = sqrt(dx*dx+dy*dy);
						dx = src_points[1].x-src_points[2].x;
						dy = src_points[1].y-src_points[2].y;
						d += sqrt(dx*dx+dy*dy);

						// 制御点座標計算の準備
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = src_points[j].x;
							bezier_points[j].y = src_points[j].y;
						}

						// 制御点座標計算
						MakeBezier3EdgeControlPoint(bezier_points, controls);
						bezier_points[1].x = src_points[0].x,	bezier_points[1].y = src_points[0].y;
						bezier_points[2] = controls[0];
						bezier_points[3].x = src_points[1].x,	bezier_points[3].y = src_points[1].y;

						// 3次ベジェ曲線を引いて消した領域と重なるかを調べる
						d = 1 / (d * 1.5);
						t = 0;
						while(t < 1)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;
							index = int_y*window->width+int_x;

							// 消した領域と重なったのでライン分割
							if(index >= 0 && index < window->width * window->height)
							{
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8 && in_devide % 2 == 0)
								{
									int point_index = (int)(t + 1.5);
									before_point = line->points[point_index];
									if(in_devide == 0)
									{
										num_divide++;

										// 変更前のライン情報を履歴データとして残す
										divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
											+ sizeof(VECTOR_POINT)*(line->num_points+point_index+1)  - sizeof(divide_line.data_size);
										divide_line.added = 0;
										divide_line.before_num_points = line->num_points;
										divide_line.after_num_points = i + 2;
										divide_line.index = line_index;
										divide_line.line_type = line->vector_type;
										(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										line->points[point_index].x = check_point.x;
										line->points[point_index].y = check_point.y;
										line->num_points = point_index+1;

										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										// ラインをラスタライズするフラグを立てる
										line->flags |= VECTOR_LINE_FIX;
									}
									else if(new_line != NULL)
									{
										add_line.data_size = offsetof(DIVIDE_LINE_ADD_DATA, after)
											+ sizeof(*line->points)*(end_flag - i) - sizeof(add_line.data_size);
										new_line->num_points = before_num_points - end_flag;
										new_line->points[new_line->num_points-1].x = check_x;
										new_line->points[new_line->num_points-1].y = check_y;
										add_line.index = line_index;
										add_line.line_type = new_line->vector_type;
										add_line.num_points = new_line->num_points;
										(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
										(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

										num_change_line++;
										new_line = NULL;
									}

									before_index = point_index;
									in_devide++;
								}
								else if(window->brush_buffer[int_y*window->width+int_x] < 0x08 && in_devide % 2 != 0)
								{
									// 現在の座標を始点にして新たなライン作成
									int point_index = (int)(t + 1.5);
									new_line = CreateVectorLine(line, line->next);
									new_line->vector_type = line->vector_type;
									new_line->num_points = 4 - point_index;
									new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
										* VECTOR_LINE_BUFFER_SIZE;
									// 制御点の設定
									new_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*new_line->points)*new_line->buff_size);
									(void)memset(new_line->points, 0, sizeof(*new_line->points)*new_line->buff_size);
									new_line->points->x = check_point.x, new_line->points->y = check_point.y;
									new_line->points->pressure = line->points[point_index].pressure;
									new_line->points->size = line->points[point_index].size;
									new_line->points->vector_type = line->points[point_index].vector_type;
									(void)memcpy(&new_line->points[1], &src_points[i], sizeof(*src_points)*(before_num_points-i-1));
									(void)memcpy(new_line->points->color, line->points[point_index].color, 4);
									this_add++;

									// ラインをラスタライズするフラグを立てる
									new_line->flags |= VECTOR_LINE_FIX;

									// 現在の制御点数を記憶
									end_flag = i;

									// 分割終了
									in_devide++;
								}
							}

							t += d;
						}

						if(new_line != NULL)
						{
							num_change_line++;
							add_line.data_size = offsetof(DIVIDE_LINE_ADD_DATA, after)
								+ sizeof(*line->points)*(new_line->num_points) - sizeof(add_line.data_size);
							new_line->points[new_line->num_points-1].x = check_x;
							new_line->points[new_line->num_points-1].y = check_y;
							add_line.index = line_index;
							add_line.line_type = new_line->vector_type;
							add_line.num_points = new_line->num_points;
							(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
							(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

							new_line = NULL;
						}
					}
					else if(line->vector_type == VECTOR_LINE_BEZIER_OPEN)
					{
						// 暫定的な距離計算
						dx = src_points[0].x-src_points[1].x;
						dy = src_points[0].y-src_points[1].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						// 制御点座標計算の準備
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = src_points[j].x;
							bezier_points[j].y = src_points[j].y;
						}

						// 制御点座標計算
						MakeBezier3EdgeControlPoint(bezier_points, controls);
						bezier_points[1].x = src_points[0].x;
						bezier_points[1].y = src_points[0].y;
						bezier_points[2] = controls[0];
						bezier_points[3].x = src_points[1].x, bezier_points[3].y = src_points[1].y;

						d = 1 / d;
						t = 0;
						while(t < 1)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;
							index = int_y*window->width + int_x;

							// 消した領域と重なったのでライン分割
							if(index >= 0 && index < window->width * window->height)
							{
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8
									&& in_devide % 2 == 0)
								{
									if(in_devide == 0)
									{
										num_divide++;

										// 変更前のライン情報を履歴データとして残す
										divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
											+ sizeof(VECTOR_POINT)*(line->num_points + 2) - sizeof(divide_line.data_size);
										divide_line.added = 0;
										divide_line.before_num_points = line->num_points;
										divide_line.after_num_points = 2;
										divide_line.index = line_index;
										divide_line.line_type = line->vector_type;
										(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										before_point = line->points[1];
										line->points[1].x = check_point.x;
										line->points[1].y = check_point.y;
										line->num_points = 2;

										(void)MemWrite(line->points, 1, sizeof(*line->points)*2, divide_change);

										// ラインをラスタライズするフラグを立てる
										line->flags |= VECTOR_LINE_FIX;

										before_index = 1;
									}
									else if(new_line != NULL)
									{
										add_line.data_size = offsetof(DIVIDE_LINE_ADD_DATA, after)
											+ sizeof(*line->points)*(end_flag - i) - sizeof(add_line.data_size);
										new_line->num_points = before_num_points - end_flag;
										new_line->points[new_line->num_points-1].x = check_x;
										new_line->points[new_line->num_points-1].y = check_y;
										add_line.index = line_index;
										add_line.line_type = new_line->vector_type;
										add_line.num_points = new_line->num_points;
										(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
										(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

										num_change_line++;
										new_line = NULL;
									}

									in_devide++;
								}
								else if(window->brush_buffer[int_y*window->width+int_x] < 0x08
									&& in_devide % 2 != 0)
								{
									// 現在の座標を始点にして新たなライン作成
									new_line = CreateVectorLine(line, line->next);
									new_line->vector_type = line->vector_type;
									new_line->num_points = before_num_points;
									new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
										* VECTOR_LINE_BUFFER_SIZE;
									// 制御点の設定
									new_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*new_line->points)*new_line->buff_size);
									(void)memset(new_line->points, 0, sizeof(*new_line->points)*new_line->buff_size);
									new_line->points->x = check_point.x, new_line->points->y = check_point.y;
									new_line->points->pressure = src_points[1].pressure;
									new_line->points->size = src_points[1].size;
									new_line->points->vector_type = src_points[1].vector_type;
									(void)memcpy(new_line->points->color, src_points[1].color, 4);
									this_add++;

									new_line->points[1] = before_point;

									(void)memcpy(&new_line->points[2], &src_points[2],
										sizeof(*new_line->points)*(new_line->num_points-2));

									// ラインをラスタライズするフラグを立てる
									new_line->flags |= VECTOR_LINE_FIX;

									end_flag = i;

									in_devide++;
								}
							}

							t += d;
						}

						for(i=0; i<line->num_points-3; i++)
						{
							// 3次ベジェ曲線
								// 制御点座標計算の準備
							for(j=0; j<4; j++)
							{
								bezier_points[j].x = src_points[i+j].x;
								bezier_points[j].y = src_points[i+j].y;
							}

							// 制御点座標計算
							MakeBezier3ControlPoints(bezier_points, controls);
							bezier_points[0].x = src_points[i+1].x;
							bezier_points[0].y = src_points[i+1].y;
							bezier_points[3].x = src_points[i+2].x;
							bezier_points[3].y = src_points[i+2].y;
							bezier_points[1] = controls[0], bezier_points[2] = controls[1];

							d = 0;
							for(j=0; j<3; j++)
							{
								dx = bezier_points[j].x-bezier_points[j+1].x;
								dy = bezier_points[j].y-bezier_points[j+1].y;
								d += sqrt(dx*dx+dy*dy) * 1.5;
							}

							d = 1 / d;
							t = 0;
							while(t < 1)
							{
								CalcBezier3(bezier_points, t, &check_point);
								int_x = (int)check_point.x, int_y = (int)check_point.y;
								index = int_y*window->width + int_x;

								// 消した領域と重なったのでライン分割
								if(index >= 0 && index < window->width * window->height)
								{
									if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8 && in_devide % 2 == 0)
									{
										if(in_devide == 0)
										{
											num_divide++;

											// 変更前のライン情報を履歴データとして残す
											divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
												+ sizeof(VECTOR_POINT)*(line->num_points + i+3) - sizeof(divide_line.data_size);
											divide_line.added = 0;
											divide_line.before_num_points = line->num_points;
											divide_line.after_num_points = i+3;
											divide_line.index = line_index;
											divide_line.line_type = line->vector_type;
											(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
											(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

											before_point = src_points[i+2];
											line->points[i+2].x = check_point.x;
											line->points[i+2].y = check_point.y;
											line->num_points = i+3;

											(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

											// ラインをラスタライズするフラグを立てる
											line->flags |= VECTOR_LINE_FIX;
										}
										else if(new_line != NULL)
										{
											add_line.data_size = offsetof(DIVIDE_LINE_ADD_DATA, after)
												+ sizeof(*line->points)*(end_flag - i) - sizeof(add_line.data_size);
											new_line->num_points = before_num_points - end_flag;
											new_line->points[new_line->num_points-1].x = check_x;
											new_line->points[new_line->num_points-1].y = check_y;
											add_line.index = line_index;
											add_line.line_type = new_line->vector_type;
											add_line.num_points = new_line->num_points;
											(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
											(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

											num_change_line++;
											new_line = NULL;
										}

										before_index = i;
										in_devide++;
									}
									else if(window->brush_buffer[int_y*window->width+int_x] < 0x08 && in_devide % 2 != 0)
									{
										// 現在の座標を始点にして新たなライン作成
										new_line = CreateVectorLine(line, line->next);
										new_line->vector_type = line->vector_type;
										new_line->num_points = before_num_points - i - 1;
										new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
											* VECTOR_LINE_BUFFER_SIZE;
										// 制御点の設定
										new_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*new_line->points)*new_line->buff_size);
										(void)memset(new_line->points, 0, sizeof(*new_line->points)*new_line->buff_size);
										new_line->points->x = check_point.x, new_line->points->y = check_point.y;
										new_line->points->pressure = src_points[i+1].pressure;
										new_line->points->size = src_points[i+1].size;
										new_line->points->vector_type = src_points[i+1].vector_type;
										(void)memcpy(new_line->points->color, src_points[i+1].color, 4);
										this_add++;

										new_line->points[1] = before_point;

										(void)memcpy(&new_line->points[2], &src_points[i+3],
											sizeof(*new_line->points)*(new_line->num_points-2));

										// ラインをラスタライズするフラグを立てる
										new_line->flags |= VECTOR_LINE_FIX;

										// 現在の制御点数を記憶
										end_flag = i;

										// 分割終了
										in_devide++;
									}
								}

								t += d;
							}
						}

						// 終点の次ベジェ曲線
							// 暫定的な距離計算
						dx = src_points[i+1].x-src_points[i+2].x;
						dy = src_points[i+1].y-src_points[i+2].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						// 制御点座標計算の準備
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = src_points[i+j].x;
							bezier_points[j].y = src_points[i+j].y;
						}

						// 制御点座標計算
						MakeBezier3EdgeControlPoint(bezier_points, controls);
						bezier_points[0].x = src_points[i+1].x;
						bezier_points[0].y = src_points[i+1].y;
						bezier_points[1] = controls[1];
						bezier_points[2].x = src_points[i+2].x, bezier_points[2].y = src_points[i+2].y;
						bezier_points[3].x = src_points[i+2].x, bezier_points[3].y = src_points[i+2].y;

						d = 1 / d;
						t = 0.5;
						while(t < 1)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;
							index = int_y*window->width + int_x;

							if(index >= 0 && index < window->width * window->height)
							{
								// 消した領域と重なったのでライン削除
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8 && in_devide % 2 == 0)
								{
									if(in_devide == 0)
									{
										num_divide++;

										// 変更前のライン情報を履歴データとして残す
										divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
											+ sizeof(VECTOR_POINT)*(line->num_points + i+3) - sizeof(divide_line.data_size);
										divide_line.added = 0;
										divide_line.before_num_points = line->num_points;
										divide_line.after_num_points = i+3;
										divide_line.index = line_index;
										divide_line.line_type = line->vector_type;
										(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										before_point = line->points[i+2];
										line->points[i+2].x = check_point.x;
										line->points[i+2].y = check_point.y;
										line->num_points = i+3;

										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										// ラインをラスタライズするフラグを立てる
										line->flags |= VECTOR_LINE_FIX;
									}
									else if(new_line != NULL)
									{
										add_line.data_size = offsetof(DIVIDE_LINE_ADD_DATA, after)
											+ sizeof(*line->points)*(end_flag - i) - sizeof(add_line.data_size);
										new_line->num_points = before_num_points - end_flag;
										new_line->points[new_line->num_points-1].x = check_x;
										new_line->points[new_line->num_points-1].y = check_y;
										add_line.index = line_index;
										add_line.line_type = new_line->vector_type;
										add_line.num_points = new_line->num_points;
										(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
										(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

										num_change_line++;
										new_line = NULL;
									}

									before_index = i+1;
									in_devide++;
								}
								else if(window->brush_buffer[int_y*window->width+int_x] < 0x08 && in_devide % 2 != 0)
								{
									// 現在の座標を始点にして新たなライン作成
									new_line = CreateVectorLine(line, line->next);
									new_line->vector_type = line->vector_type;
									new_line->num_points = 2;
									new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
										* VECTOR_LINE_BUFFER_SIZE;
									// 制御点の設定
									new_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*new_line->points)*new_line->buff_size);
									(void)memset(new_line->points, 0, sizeof(*new_line->points)*new_line->buff_size);
									new_line->points->x = check_point.x, new_line->points->y = check_point.y;
									new_line->points->pressure = src_points[i+1].pressure;
									new_line->points->size = src_points[i+1].size;
									new_line->points->vector_type = src_points[i+1].vector_type;
									(void)memcpy(new_line->points->color, src_points[i+1].color, 4);
									this_add++;

									new_line->points[1] = before_point;

									// ラインをラスタライズするフラグを立てる
									new_line->flags |= VECTOR_LINE_FIX;

									// 現在の制御点数を記憶する
									end_flag = i;

									// 分割終了
									in_devide++;
								}
							}

							t += d;
						}

						if(new_line != NULL)
						{
							num_change_line++;
							add_line.data_size = offsetof(DIVIDE_LINE_ADD_DATA, after)
								+ sizeof(*line->points)*(new_line->num_points) - sizeof(add_line.data_size);
							add_line.index = line_index;
							add_line.line_type = new_line->vector_type;
							add_line.num_points = new_line->num_points;
							(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
							(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

							new_line = NULL;
						}
					}

					for(j=0; j<this_add; j++)
					{
						line = line->next;
					}

					break;
				case VECTOR_ERASER_STROKE_DELETE:
					// ラインをストロークして消した領域と重なったらライン消し
					if(line->vector_type == VECTOR_LINE_STRAIGHT
						|| line->num_points == 2)
					{	// 直線
						end_flag = 0;
						for(i=0; i<line->num_points-1 && end_flag == 0; i++)
						{
							dx = line->points[i].x - line->points[i+1].x;
							dy = line->points[i].y - line->points[i+1].y;
							d = sqrt(dx*dx+dy*dy);
							t = 0;
							arg = atan2(dy, dx);
							cos_x = cos(arg), sin_y = sin(arg);
							check_x = line->points[i].x, check_y = line->points[i].y;
							
							t = 0;
							do
							{
								int_x = (int)check_x, int_y = (int)check_y;

								// 消した領域と重なったのでライン削除
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE* prev_line = line->prev;
									change_lines[num_change_line] = *line;
									change_line_indexes[num_change_line] = line_index;
									change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
										sizeof(*line->points)*line->num_points);
									for(k=0; k<line->num_points; k++)
									{
										change_lines[num_change_line].points[k] = line->points[k];
									}
									num_change_line++;
									if(num_change_line >= buffer_size - 1)
									{
										buffer_size += VECTOR_ERASER_BUFFER_SIZE;
										change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
											change_lines, sizeof(*change_lines)*buffer_size);
										change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
											change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
									}

									if(line == window->active_layer->layer_data.vector_layer_p->top_line)
									{
										window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
									}

									DeleteVectorLine(&line);
									line = prev_line;

									break;
								}
								t += 1;
								check_x -= cos_x, check_y -= sin_y;
							} while(t < d);
						}
					}
					else if(line->vector_type == VECTOR_LINE_BEZIER_OPEN && line->num_points == 3)
					{	// 2次ベジェ曲線
						dx = line->points[0].x-line->points[1].x;
						dy = line->points[0].y-line->points[1].y;
						d = sqrt(dx*dx+dy*dy);
						dx = line->points[1].x-line->points[2].x;
						dy = line->points[1].y-line->points[2].y;
						d += sqrt(dx*dx+dy*dy);

						// 制御点座標計算の準備
						for(i=0; i<3; i++)
						{
							bezier_points[i].x = line->points[i].x;
							bezier_points[i].y = line->points[i].y;
						}

						// 制御点座標計算
						MakeBezier3EdgeControlPoint(bezier_points, controls);
						bezier_points[1].x = line->points[0].x;
						bezier_points[1].y = line->points[0].y;
						bezier_points[2] = controls[0];
						bezier_points[3].x = line->points[1].x, bezier_points[3].y = line->points[1].y;

						// 2次ベジェ曲線を引いて消した領域と重なるかを調べる
						d = 1 / (d * 1.5);
						t = 0;
						while(t < 1)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;

							if(int_x >= 0 && int_x < window->width
								&& int_y >= 0 && int_y < window->height)
							{
								// 重なったのでライン削除
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE* prev_line = line->prev;
									change_lines[num_change_line] = *line;
									change_line_indexes[num_change_line] = line_index;
									change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
										sizeof(*line->points)*line->num_points);
									for(k=0; k<line->num_points; k++)
									{
										change_lines[num_change_line].points[k] = line->points[k];
									}
									num_change_line++;
									if(num_change_line >= buffer_size - 1)
									{
										buffer_size += VECTOR_ERASER_BUFFER_SIZE;
										change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
											change_lines, sizeof(*change_lines)*buffer_size);
										change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
											change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
									}

									if(line == window->active_layer->layer_data.vector_layer_p->top_line)
									{
										window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
									}
									DeleteVectorLine(&line);
									line = prev_line;
									break;
								}
							}

							t += d;
						}
					}
					else if(line->vector_type == VECTOR_LINE_BEZIER_OPEN)
					{	// 2次と3次ベジェ曲線の混合
							// 始点から2点目までは2次ベジェ曲線
						// 暫定的な距離計算
						dx = line->points[0].x-line->points[1].x;
						dy = line->points[0].y-line->points[1].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						// 制御点座標計算の準備
						for(i=0; i<3; i++)
						{
							bezier_points[i].x = line->points[i].x;
							bezier_points[i].y = line->points[i].y;
						}

						// 制御点座標計算
						MakeBezier3EdgeControlPoint(bezier_points, controls);
						bezier_points[1].x = line->points[0].x;
						bezier_points[1].y = line->points[0].y;
						bezier_points[2] = controls[0];
						bezier_points[3].x = line->points[1].x, bezier_points[3].y = line->points[1].y;

						d = 1 / d;
						t = 0;
						while(t < 0.5)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;

							if(int_x >= 0 && int_x < window->width
								&& int_y >= 0 && int_y < window->height)
							{
								// 消した領域と重なったのでライン削除
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE* prev_line = line->prev;
									change_lines[num_change_line] = *line;
									change_line_indexes[num_change_line] = num_change_line;
									change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
										sizeof(*line->points)*line->num_points);
									for(k=0; k<line->num_points; k++)
									{
										change_lines[num_change_line].points[k] = line->points[k];
									}
									num_change_line++;
									if(num_change_line >= buffer_size - 1)
									{
										buffer_size += VECTOR_ERASER_BUFFER_SIZE;
										change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
											change_lines, sizeof(*change_lines)*buffer_size);
										change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
											change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
									}

									if(line == window->active_layer->layer_data.vector_layer_p->top_line)
									{
										window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
									}
									DeleteVectorLine(&line);
									line = prev_line;

									goto next_line;
								}
							}

							t += d;
						}

						for(i=0; i<line->num_points-3; i++)
						{
							// 3次ベジェ曲線
								// 制御点座標計算の準備
							for(j=0; j<4; j++)
							{
								bezier_points[j].x = line->points[i+j].x;
								bezier_points[j].y = line->points[i+j].y;
							}

							// 制御点座標計算
							MakeBezier3ControlPoints(bezier_points, controls);
							bezier_points[0].x = line->points[i+1].x;
							bezier_points[0].y = line->points[i+1].y;
							bezier_points[3].x = line->points[i+2].x;
							bezier_points[3].y = line->points[i+2].y;
							bezier_points[1] = controls[0], bezier_points[2] = controls[1];

							d = 0;
							for(j=0; j<3; j++)
							{
								dx = bezier_points[j].x-bezier_points[j+1].x;
								dy = bezier_points[j].y-bezier_points[j+1].y;
								d += sqrt(dx*dx+dy*dy) * 1.5;
							}

							d = 1 / d;
							t = 0;
							while(t < 1)
							{
								CalcBezier3(bezier_points, t, &check_point);
								int_x = (int)check_point.x, int_y = (int)check_point.y;

								if(int_x >= 0 && int_x < window->width
									&& int_y >= 0 && int_y < window->height)
								{
									// 消した領域と重なったのでライン削除
									if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
									{
										VECTOR_LINE* prev_line = line->prev;
										change_lines[num_change_line] = *line;
										change_line_indexes[num_change_line] = line_index;
										change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
											sizeof(*line->points)*line->num_points);
										for(k=0; k<line->num_points; k++)
										{
											change_lines[num_change_line].points[k] = line->points[k];
										}
										num_change_line++;
										if(num_change_line >= buffer_size - 1)
										{
											buffer_size += VECTOR_ERASER_BUFFER_SIZE;
											change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
												change_lines, sizeof(*change_lines)*buffer_size);
											change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
												change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
										}

										if(line == window->active_layer->layer_data.vector_layer_p->top_line)
										{
											window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
										}
										DeleteVectorLine(&line);
										line = prev_line;

										goto next_line;
									}
								}

								t += d;
							}
						}

						// 終点と一個手前は2次ベジェ曲線
							// 暫定的な距離計算
						dx = line->points[i+1].x-line->points[i+2].x;
						dy = line->points[i+1].y-line->points[i+2].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						// 制御点座標計算の準備
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = line->points[i+j].x;
							bezier_points[j].y = line->points[i+j].y;
						}

						// 制御点座標計算
						MakeBezier3EdgeControlPoint(bezier_points, controls);
						bezier_points[i+1].x = line->points[0].x;
						bezier_points[i+1].y = line->points[0].y;
						bezier_points[2] = controls[1];
						bezier_points[3].x = line->points[i+2].x, bezier_points[3].y = line->points[i+2].y;

						d = 1 / d;
						t = 0.5;
						while(t < 1)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;
							index = int_y*window->width + int_x;

							// 消した領域と重なったのでライン削除
							if(index >= 0 && index < window->width * window->height
								&& window->brush_buffer[index] >= 0xf8)
							{
								VECTOR_LINE* prev_line = line->prev;
								change_lines[num_change_line] = *line;
								change_line_indexes[num_change_line] = line_index;
								change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
									sizeof(*line->points)*line->num_points);
								for(k=0; k<line->num_points; k++)
								{
									change_lines[num_change_line].points[k] = line->points[k];
								}
								num_change_line++;
								if(num_change_line >= buffer_size - 1)
								{
									buffer_size += VECTOR_ERASER_BUFFER_SIZE;
									change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
										change_lines, sizeof(*change_lines)*buffer_size);
									change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
										change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
								}

								if(line == window->active_layer->layer_data.vector_layer_p->top_line)
								{
									window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
								}
								DeleteVectorLine(&line);
								line = prev_line;

								break;
							}

							t += d;
						}
					}
					else if(line->vector_type == VECTOR_LINE_STRAIGHT_CLOSE)
					{	// 閉じた直線
						end_flag = 0;
						for(i=0; i<line->num_points-1 && end_flag == 0; i++)
						{
							dx = line->points[i].x - line->points[i+1].x;
							dy = line->points[i].y - line->points[i+1].y;
							d = sqrt(dx*dx+dy*dy);
							t = 0;
							arg = atan2(dy, dx);
							cos_x = cos(arg), sin_y = sin(arg);
							check_x = line->points[i].x, check_y = line->points[i].y;
							
							t = 0;
							do
							{
								int_x = (int)check_x, int_y = (int)check_y;

								// 消した領域と重なったのでライン削除
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE* prev_line = line->prev;
									change_lines[num_change_line] = *line;
									change_line_indexes[num_change_line] = line_index;
									change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
										sizeof(*line->points)*line->num_points);
									for(k=0; k<line->num_points; k++)
									{
										change_lines[num_change_line].points[k] = line->points[k];
									}
									num_change_line++;
									if(num_change_line >= buffer_size - 1)
									{
										buffer_size += VECTOR_ERASER_BUFFER_SIZE;
										change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
											change_lines, sizeof(*change_lines)*buffer_size);
										change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
											change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
									}

									if(line == window->active_layer->layer_data.vector_layer_p->top_line)
									{
										window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
									}

									DeleteVectorLine(&line);
									line = prev_line;

									break;
								}
								t += 1;
								check_x -= cos_x, check_y -= sin_y;
							} while(t < d);
						}
						if(end_flag == 0)
						{
							dx = line->points[i].x - line->points[0].x;
							dy = line->points[i].y - line->points[0].y;
							d = sqrt(dx*dx+dy*dy);
							t = 0;
							arg = atan2(dy, dx);
							cos_x = cos(arg), sin_y = sin(arg);
							check_x = line->points[i].x, check_y = line->points[i].y;
							
							t = 0;
							do
							{
								int_x = (int)check_x, int_y = (int)check_y;

								// 消した領域と重なったのでライン削除
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE* prev_line = line->prev;
									change_lines[num_change_line] = *line;
									change_line_indexes[num_change_line] = line_index;
									change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
										sizeof(*line->points)*line->num_points);
									for(k=0; k<line->num_points; k++)
									{
										change_lines[num_change_line].points[k] = line->points[k];
									}
									num_change_line++;
									if(num_change_line >= buffer_size - 1)
									{
										buffer_size += VECTOR_ERASER_BUFFER_SIZE;
										change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
											change_lines, sizeof(*change_lines)*buffer_size);
										change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
											change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
									}

									if(line == window->active_layer->layer_data.vector_layer_p->top_line)
									{
										window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
									}

									DeleteVectorLine(&line);
									line = prev_line;

									break;
								}
								t += 1;
								check_x -= cos_x, check_y -= sin_y;
							} while(t < d);
						}
					}
					else if(line->vector_type == VECTOR_LINE_BEZIER_CLOSE)
					{	// 2次と3次ベジェ曲線の混合
							// 始点から2点目までは2次ベジェ曲線
						// 暫定的な距離計算
						dx = line->points[0].x-line->points[1].x;
						dy = line->points[0].y-line->points[1].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						bezier_points[0].x = line->points[line->num_points-1].x;
						bezier_points[0].y = line->points[line->num_points-1].y;
						// 制御点座標計算の準備
						for(i=0; i<3; i++)
						{
							bezier_points[i+1].x = line->points[i].x;
							bezier_points[i+1].y = line->points[i].y;
						}

						// 制御点座標計算
						MakeBezier3ControlPoints(bezier_points, controls);
						bezier_points[0].x = line->points[0].x;
						bezier_points[0].y = line->points[0].y;
						bezier_points[1] = controls[0], bezier_points[2] = controls[1];
						bezier_points[3].x = line->points[1].x, bezier_points[3].y = line->points[1].y;

						d = 1 / d;
						t = 0;
						while(t < 0.5)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;

							if(int_x >= 0 && int_x < window->width
								&& int_y >= 0 && int_y < window->height)
							{
								// 消した領域と重なったのでライン削除
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE* prev_line = line->prev;
									change_lines[num_change_line] = *line;
									change_line_indexes[num_change_line] = line_index;
									change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
										sizeof(*line->points)*line->num_points);
									for(k=0; k<line->num_points; k++)
									{
										change_lines[num_change_line].points[k] = line->points[k];
									}
									num_change_line++;
									if(num_change_line >= buffer_size - 1)
									{
										buffer_size += VECTOR_ERASER_BUFFER_SIZE;
										change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
											change_lines, sizeof(*change_lines)*buffer_size);
										change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
											change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
									}

									if(line == window->active_layer->layer_data.vector_layer_p->top_line)
									{
										window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
									}
									DeleteVectorLine(&line);
									line = prev_line;

									goto next_line;
								}
							}

							t += d;
						}

						for(i=0; i<line->num_points-3; i++)
						{
							// 3次ベジェ曲線
								// 制御点座標計算の準備
							for(j=0; j<4; j++)
							{
								bezier_points[j].x = line->points[i+j].x;
								bezier_points[j].y = line->points[i+j].y;
							}

							// 制御点座標計算
							MakeBezier3ControlPoints(bezier_points, controls);
							bezier_points[0].x = line->points[i+1].x;
							bezier_points[0].y = line->points[i+1].y;
							bezier_points[3].x = line->points[i+2].x;
							bezier_points[3].y = line->points[i+2].y;
							bezier_points[1] = controls[0], bezier_points[2] = controls[1];

							d = 0;
							for(j=0; j<3; j++)
							{
								dx = bezier_points[j].x-bezier_points[j+1].x;
								dy = bezier_points[j].y-bezier_points[j+1].y;
								d += sqrt(dx*dx+dy*dy) * 1.5;
							}

							d = 1 / d;
							t = 0;
							while(t < 1)
							{
								CalcBezier3(bezier_points, t, &check_point);
								int_x = (int)check_point.x, int_y = (int)check_point.y;

								if(int_x >= 0 && int_x < window->width
									&& int_y >= 0 && int_y < window->height)
								{
									// 消した領域と重なったのでライン削除
									if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
									{
										VECTOR_LINE* prev_line = line->prev;
										change_lines[num_change_line] = *line;
										change_line_indexes[num_change_line] = line_index;
										change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
											sizeof(*line->points)*line->num_points);
										for(k=0; k<line->num_points; k++)
										{
											change_lines[num_change_line].points[k] = line->points[k];
										}
										num_change_line++;
										if(num_change_line >= buffer_size - 1)
										{
											buffer_size += VECTOR_ERASER_BUFFER_SIZE;
											change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
												change_lines, sizeof(*change_lines)*buffer_size);
											change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
												change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
										}

										if(line == window->active_layer->layer_data.vector_layer_p->top_line)
										{
											window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
										}
										DeleteVectorLine(&line);
										line = prev_line;

										goto next_line;
									}
								}

								t += d;
							}
						}


						// 制御点座標計算の準備
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = line->points[i+j].x;
							bezier_points[j].y = line->points[i+j].y;
						}
						bezier_points[3].x = line->points[0].x, bezier_points[3].y = line->points[0].y;
						// 制御点座標計算
						MakeBezier3ControlPoints(bezier_points, controls);
						bezier_points[0].x = line->points[i+1].x, bezier_points[0].y = line->points[i+1].y;
						bezier_points[1] = controls[0], bezier_points[2] = controls[1];
						bezier_points[3].x = line->points[i+2].x, bezier_points[3].y = line->points[i+2].y;

						d = 0;
						for(j=0; j<3; j++)
						{
							dx = bezier_points[j].x-bezier_points[j+1].x;
							dy = bezier_points[j].y-bezier_points[j+1].y;
							d += sqrt(dx*dx+dy*dy) * 1.5;
						}

						d = 1 / d;
						t = 0.5;
						while(t < 1)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;

							// 消した領域と重なったのでライン削除
							if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
							{
								VECTOR_LINE* prev_line = line->prev;
								change_lines[num_change_line] = *line;
								change_line_indexes[num_change_line] = line_index;
								change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
									sizeof(*line->points)*line->num_points);
								for(k=0; k<line->num_points; k++)
								{
									change_lines[num_change_line].points[k] = line->points[k];
								}
								num_change_line++;
								if(num_change_line >= buffer_size - 1)
								{
									buffer_size += VECTOR_ERASER_BUFFER_SIZE;
									change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
										change_lines, sizeof(*change_lines)*buffer_size);
									change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
										change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
								}

								if(line == window->active_layer->layer_data.vector_layer_p->top_line)
								{
									window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
								}
								DeleteVectorLine(&line);
								line = prev_line;

								break;
							}

							t += d;
						}

						// 制御点座標計算の準備
						bezier_points[0].x = line->points[i+1].x, bezier_points[0].y = line->points[i+1].y;
						bezier_points[1].x = line->points[i+2].x, bezier_points[1].y = line->points[i+2].y;
						bezier_points[2].x = line->points[0].x, bezier_points[2].y = line->points[0].y;
						bezier_points[3].x = line->points[1].x, bezier_points[3].y = line->points[1].y;
						// 制御点座標計算
						MakeBezier3ControlPoints(bezier_points, controls);
						bezier_points[0].x = line->points[i+2].x, bezier_points[0].y = line->points[i+2].y;
						bezier_points[1] = controls[0], bezier_points[2] = controls[1];
						bezier_points[3].x = line->points[0].x, bezier_points[3].y = line->points[0].y;

						d = 0;
						for(j=0; j<3; j++)
						{
							dx = bezier_points[j].x-bezier_points[j+1].x;
							dy = bezier_points[j].y-bezier_points[j+1].y;
							d += sqrt(dx*dx+dy*dy) * 1.5;
						}

						d = 1 / d;
						t = 0.5;
						while(t < 1)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;

							// 消した領域と重なったのでライン削除
							if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
							{
								VECTOR_LINE* prev_line = line->prev;
								change_lines[num_change_line] = *line;
								change_line_indexes[num_change_line] = line_index;
								change_lines[num_change_line].points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
									sizeof(*line->points)*line->num_points);
								for(k=0; k<line->num_points; k++)
								{
									change_lines[num_change_line].points[k] = line->points[k];
								}
								num_change_line++;
								if(num_change_line >= buffer_size - 1)
								{
									buffer_size += VECTOR_ERASER_BUFFER_SIZE;
									change_lines = (VECTOR_LINE*)MEM_REALLOC_FUNC(
										change_lines, sizeof(*change_lines)*buffer_size);
									change_line_indexes = (uint32*)MEM_REALLOC_FUNC(
										change_line_indexes, sizeof(*change_line_indexes)*buffer_size);
								}

								if(line == window->active_layer->layer_data.vector_layer_p->top_line)
								{
									window->active_layer->layer_data.vector_layer_p->top_line = line->prev;
								}
								DeleteVectorLine(&line);
								line = prev_line;

								break;
							}

							t += d;
						}
					}

					break;
				}
			}
next_line:
			if(line != NULL)
			{
				line = line->next;
			}

			line_index++;
		}

		// 履歴データを作成
		switch(eraser->mode)
		{
		case VECTOR_ERASER_STROKE_DIVIDE:
			if(num_divide > 0 || num_change_line > 0)
			{
				MEMORY_STREAM_PTR history_stream = CreateMemoryStream(4096);
				DIVIDE_LINE_DATA history_data;
				history_data.num_change = num_divide;
				history_data.num_add = num_change_line;
				history_data.change_size = (int)divide_change->data_point;
				history_data.layer_name_length = (int)strlen(window->active_layer->name) + 1;
				(void)MemWrite(&history_data, 1,
					offsetof(DIVIDE_LINE_DATA, layer_name), history_stream);
				(void)MemWrite(window->active_layer->name, 1,
					history_data.layer_name_length, history_stream);
				(void)MemWrite(divide_change->buff_ptr, 1,
					divide_change->data_point, history_stream);
				(void)MemWrite(divide_add->buff_ptr, 1,
					divide_add->data_point, history_stream);

				AddHistory(&window->history, core->name, history_stream->buff_ptr,
					history_stream->data_point, DivideLinesUndo, DivideLinesRedo);

				(void)DeleteMemoryStream(history_stream);
			}

			(void)DeleteMemoryStream(divide_change);
			(void)DeleteMemoryStream(divide_add);
			(void)DeleteMemoryStream(src_points_stream);

			break;
		case VECTOR_ERASER_STROKE_DELETE:
			AddDeleteLinesHistory(window, window->active_layer, change_lines,
				change_line_indexes, num_change_line, core->name
			);
			// 履歴データ用のバッファを開放
			for(k=0; k<num_change_line; k++)
			{
				MEM_FREE_FUNC(change_lines[k].points);
			}
			MEM_FREE_FUNC(change_lines);
			MEM_FREE_FUNC(change_line_indexes);

			break;
		}


		// ラインを重ね直す
		window->active_layer->layer_data.vector_layer_p->flags |=
			VECTOR_LAYER_RASTERIZE_CHECKED;
		// 作業用レイヤーのモードを元に戻す
		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		(void)memset(window->brush_buffer, 0, window->pixel_buf_size);
		(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
	}
}

/*******************************************
* VectorEraserDrawCursor関数               *
* ベクトル消しゴムのカーソルを描画         *
* 引数                                     *
* window	: 描画領域の情報               *
* x			: カーソルのX座標              *
* y			: カーソルのY座標              *
* data		: ベクトル消しゴムの詳細データ *
*******************************************/
static void VectorEraserDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	VECTOR_ERASER* eraser = (VECTOR_ERASER*)data;
	gdouble r = eraser->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		r * 2 + 3, r * 2 + 3);
	cairo_clip(window->disp_layer->cairo_p);
}

static void ChangeVectorEraserScale(GtkAdjustment* slider, VECTOR_ERASER* eraser)
{
	eraser->r = gtk_adjustment_get_value(slider) * 0.5;
}

static void SetVectorEraserMode(GtkWidget* button, VECTOR_ERASER* eraser)
{
	eraser->mode = (uint8)GPOINTER_TO_INT(g_object_get_data(
		G_OBJECT(button), "mode"));
}

static void ChangeVectorEraserUsePressureScale(GtkWidget* widget, VECTOR_ERASER* eraser)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		eraser->flags &= ~(VECTOR_ERASER_PRESSURE_SIZE);
	}
	else
	{
		eraser->flags |= VECTOR_ERASER_PRESSURE_SIZE;
	}
}

/*****************************************************
* CreateVectorEraserDetailUI関数                     *
* ベクトル消しゴムの詳細設定UIを作成                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
* data	: ベクトル消しゴムの詳細データ               *
* 返り値                                             *
*	作成した詳細設定ウィジェット                     *
*****************************************************/
static GtkWidget* CreateVectorEraserDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	// ベクトル消しゴムの詳細データにキャスト
	VECTOR_ERASER* eraser = (VECTOR_ERASER*)data;
	// 返り値
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	// ラベルとサイズ設定用のスライダ
	GtkWidget* label, *eraser_scale;
	// ウィジェット整列用のテーブル
	GtkWidget* table;
	// モード設定用のラジオボタン
	GtkWidget* radio_buttons[VECTOR_ERASER_MODE_NUM];
	// 筆圧使用のチェックボックス
	GtkWidget* check_button;
	// スライダに使用するアジャスタ
	GtkAdjustment* scale_adjustment = GTK_ADJUSTMENT(
		gtk_adjustment_new(eraser->r * 2, 1, 100, 1, 1, 0));
	// ラベルのフォント変更用のマークアップバッファ
	char mark_up_buff[256];
	// for文用のカウンタ
	int i;

	// サイズ設定のウィジェット
	label = gtk_label_new("");
	eraser_scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeVectorEraserScale), data);
	table = gtk_table_new(1, 3, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), eraser_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// 消しゴムモード選択用のラジオボタン
	radio_buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.devide_stroke);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.delete_stroke
	);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[eraser->mode]), TRUE);

	for(i=0; i<VECTOR_ERASER_MODE_NUM; i++)
	{
		g_object_set_data(G_OBJECT(radio_buttons[i]), "mode", GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(radio_buttons[i]), "toggled", G_CALLBACK(SetVectorEraserMode), data);
		gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[i], FALSE, TRUE, 0);
	}

	// 筆圧使用のチェックボックス
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(
		app->labels->tool_box.scale);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(check_button), eraser->flags & VECTOR_ERASER_PRESSURE_SIZE);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(ChangeVectorEraserUsePressureScale), data);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	return vbox;
#undef UI_FONT_SIZE
}

void LoadVectorBrushDetailData(
	VECTOR_BRUSH_CORE* core,
	INI_FILE_PTR file,
	const char* section_name,
	const char* brush_type
)
{
	if(StringCompareIgnoreCase(brush_type, "POLY_LINE") == 0)
	{
		POLY_LINE_BRUSH* line;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*line));
		(void)memset(core->brush_data, 0, sizeof(*line));
		line = (POLY_LINE_BRUSH*)core->brush_data;
		core->detail_data_size = sizeof(*line);

		core->press_func = PolyLinePressCallBack;
		core->motion_func = PolyLineMotionCallBack;
		core->release_func = PolyLineReleaseCallBack;
		core->key_press_func = PolyLineKeyPressCallBack;
		core->create_detail_ui = CreatePolyLineDetailUI;
		core->draw_cursor = PolyLineDrawCursor;

		line->r = IniFileGetInteger(file, section_name, "SIZE") * 0.5;
		line->line_type = IniFileGetInteger(file, section_name, "LINE_TYPE");
		line->blur = IniFileGetInteger(file, section_name, "BLUR");
		line->outline_hardness = IniFileGetInteger(file, section_name, "OUTLINE_HARDNESS");
		line->flow = (uint8)(IniFileGetInteger(file, section_name, "FLOW") * 2.55 + 0.5);
		line->first_pressure = (uint8)IniFileGetInteger(file, section_name, "FIRST_PRESSURE");
		line->last_pressure = (uint8)IniFileGetInteger(file, section_name, "LAST_PRESSURE");
		line->timer = g_timer_new();

		if(IniFileGetInteger(file, section_name, "PRESSURE_SIZE") != 0)
		{
			line->flags |= POLY_LINE_SIZE_WITH_PRESSURE;
		}

		if(IniFileGetInteger(file, section_name, "ANTI_ALIAS") != 0)
		{
			line->flags |= POLY_LINE_ANTI_ALIAS;
		}

		core->brush_type = TYPE_POLY_LINE_BRUSH;
	}
	else if(StringCompareIgnoreCase(brush_type, "BEZIER_LINE") == 0)
	{
		BEZIER_LINE_BRUSH* line;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*line));
		(void)memset(core->brush_data, 0, sizeof(*line));
		line = (BEZIER_LINE_BRUSH*)core->brush_data;
		core->detail_data_size = sizeof(*line);

		core->press_func = BezierLinePressCallBack;
		core->motion_func = BezierLineMotionCallBack;
		core->release_func = BezierLineReleaseCallBack;
		core->key_press_func = BezierLineKeyPressCallBack;
		core->create_detail_ui = CreateBezierLineDetailUI;
		core->draw_cursor = BezierLineDrawCursor;

		line->r = IniFileGetInteger(file, section_name, "SIZE") * 0.5;
		line->line_type = IniFileGetInteger(file, section_name, "LINE_TYPE");
		line->blur = IniFileGetInteger(file, section_name, "BLUR");
		line->outline_hardness = IniFileGetInteger(file, section_name, "OUTLINE_HARDNESS");
		line->flow = (uint8)(IniFileGetInteger(file, section_name, "FLOW") * 2.55 + 0.5);
		line->first_pressure = (uint8)IniFileGetInteger(file, section_name, "FIRST_PRESSURE");
		line->last_pressure = (uint8)IniFileGetInteger(file, section_name, "LAST_PRESSURE");
		line->timer = g_timer_new();

		if(IniFileGetInteger(file, section_name, "PRESSURE_SIZE") != 0)
		{
			line->flags |= BEZIER_LINE_SIZE_WITH_PRESSURE;
		}

		if(IniFileGetInteger(file, section_name, "ANTI_ALIAS") != 0)
		{
			line->flags |= BEZIER_LINE_ANTI_ALIAS;
		}

		core->brush_type = TYPE_BEZIER_LINE_BRUSH;
	}
	else if(StringCompareIgnoreCase(brush_type, "FREE_HAND") == 0)
	{
		FREE_HAND_BRUSH* free_hand;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*free_hand));
		(void)memset(core->brush_data, 0, sizeof(*free_hand));
		free_hand = (FREE_HAND_BRUSH*)core->brush_data;
		core->detail_data_size = sizeof(*free_hand);

		free_hand->r = IniFileGetInteger(file, section_name, "SIZE") * 0.5;
		free_hand->line_type = IniFileGetInteger(file, section_name, "LINE_TYPE");
		free_hand->blur = IniFileGetInteger(file, section_name, "BLUR");
		free_hand->outline_hardness = IniFileGetInteger(file, section_name, "OUTLINE_HARDNESS");
		free_hand->flow = (uint8)(IniFileGetInteger(file, section_name, "FLOW") * 2.55);
		free_hand->min_degree = IniFileGetInteger(file, section_name, "MINIMUM_DEGREE");
		if(free_hand->min_degree < 3)
		{
			free_hand->min_degree = 3;
		}
		free_hand->min_arg = (free_hand->min_degree * G_PI) / 180;
		free_hand->min_distance = IniFileGetInteger(file, section_name, "MINIMUM_DISTANCE");
		if(free_hand->min_distance < 1)
		{
			free_hand->min_distance = 1;
		}

		if(IniFileGetInteger(file, section_name, "PRESSURE_SIZE") != 0)
		{
			free_hand->flags |= FREE_HAND_SIZE_WITH_PRESSURE;
		}

		if(IniFileGetInteger(file, section_name, "ANTI_ALIAS") != 0)
		{
			free_hand->flags |= FREE_HAND_ANTI_ALIAS;
		}

		if(IniFileGetInteger(file, section_name, "PRIOR_ANGLE") != 0)
		{
			free_hand->flags |= FREE_HAND_PRIOR_ARG;
		}

		core->press_func = FreeHandPressCallBack;
		core->motion_func = FreeHandMotionCallBack;
		core->release_func = FreeHandReleaseCallBack;
		core->draw_cursor = FreeHandDrawCursor;
		core->create_detail_ui = CreateFreeHandDetailUI;

		core->brush_type = TYPE_FREE_HAND_BRUSH;
	}
	else if(StringCompareIgnoreCase(brush_type, "CONTROL_POINT") == 0)
	{
		CONTROL_POINT_TOOL* control;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*control));
		(void)memset(core->brush_data, 0, sizeof(*control));
		control = (CONTROL_POINT_TOOL*)core->brush_data;
		core->detail_data_size = sizeof(*control);

		control->mode = (uint8)IniFileGetInteger(file, section_name, "MODE");

		core->press_func = ControlPointToolPressCallBack;
		core->motion_func = ControlPointToolMotionCallBack;
		core->release_func = ControlPointToolReleaseCallBack;
		core->create_detail_ui = CreateControlPointToolDetailUI;
		core->draw_cursor = ControlPointToolDrawCursor;

		core->brush_type = TYPE_CONTROL_POINT;
	}
	else if(StringCompareIgnoreCase(brush_type, "CHANGE_LINE_COLOR") == 0)
	{
		CHANGE_LINE_COLOR_TOOL* color;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*color));
		(void)memset(core->brush_data, 0, sizeof(*color));
		color = (CHANGE_LINE_COLOR_TOOL*)core->brush_data;
		core->detail_data_size = sizeof(*color);

		color->flow = (uint8)(IniFileGetInteger(file, section_name, "FLOW") * 2.55);

		core->press_func = LineColorChangeBrushPressCallBack;
		core->motion_func = LineColorChangeBrushMotionCallBack;
		core->release_func = LineColorChangeBrushReleaseCallBack;
		core->draw_cursor = LineColorChangeBrushDrawCursor;
		core->create_detail_ui = CreateLineColorChangeBrushDetailUI;

		core->brush_type = TYPE_CHANGE_LINE_COLOR;
	}
	else if(StringCompareIgnoreCase(brush_type, "CHANGE_LINE_SIZE") == 0)
	{
		CHANGE_LINE_SIZE_TOOL* size;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*size));
		(void)memset(core->brush_data, 0, sizeof(*size));
		size = (CHANGE_LINE_SIZE_TOOL*)core->brush_data;
		core->detail_data_size = sizeof(*size);

		size->r = IniFileGetInteger(file, section_name, "SIZE") * 0.5;
		size->blur = IniFileGetInteger(file, section_name, "BLUR");
		size->outline_hardness = IniFileGetInteger(file, section_name, "OUTLINE_HARDNESS");
		size->flow = (uint8)(IniFileGetInteger(file, section_name, "FLOW") * 2.55);

		core->press_func = LineSizeChangeBrushPressCallBack;
		core->motion_func = LineSizeChangeBrushMotionCallBack;
		core->release_func = LineSizeChangeBrushReleaseCallBack;
		core->draw_cursor = LineSizeChangeBrushDrawCursor;
		core->create_detail_ui = CreateChangeLineSizeDetailUI;

		core->brush_type = TYPE_CHANGE_LINE_SIZE;
	}
	else if(StringCompareIgnoreCase(brush_type, "VECTOR_ERASER") == 0)
	{
		VECTOR_ERASER* eraser;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*eraser));
		(void)memset(core->brush_data, 0, sizeof(*eraser));
		eraser = (VECTOR_ERASER*)core->brush_data;
		core->detail_data_size = sizeof(*eraser);

		eraser->r = IniFileGetInteger(file, section_name, "SIZE") * 0.5;
		eraser->mode = (uint8)IniFileGetInteger(file, section_name, "MODE");

		if(IniFileGetInteger(file, section_name, "PRESSURE_SIZE") != 0)
		{
			eraser->flags |= VECTOR_ERASER_PRESSURE_SIZE;
		}

		core->press_func = VectorEraserPressCallBack;
		core->motion_func = VectorEraserMotionCallBack;
		core->release_func = VectorEraserReleaseCallBack;
		core->draw_cursor = VectorEraserDrawCursor;
		core->create_detail_ui = CreateVectorEraserDetailUI;

		core->brush_type = TYPE_VECTOR_ERASER;
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

void SetControlPointTool(
	VECTOR_BRUSH_CORE* core,
	CONTROL_POINT_TOOL* control
)
{
	core->brush_data = (void*)control;
	control->mode = CONTROL_POINT_MOVE;

	core->press_func = ControlPointToolPressCallBack;
	core->motion_func = ControlPointToolMotionCallBack;
	core->release_func = ControlPointToolReleaseCallBack;
	core->draw_cursor = ControlPointToolDrawCursor;
	core->button_update = DefaultToolUpdate;
	core->motion_update = DefaultToolUpdate;
}

/*********************************************************
* WriteVectorBrushData関数                               *
* ベクトルレイヤー用のブラシテーブルのデータを書き出す   *
* 引数                                                   *
* window	: ツールボックスウィンドウ                   *
* file_path	: 書き出すファイルのパス                     *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	正常終了:0	失敗:負の値                              *
*********************************************************/
int WriteVectorBrushData(
	TOOL_WINDOW* window,
	const char* file_path,
	APPLICATION* app
)
{
	GFile *fp = g_file_new_for_path(file_path);
	GFileOutputStream *stream =
		g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL);
	INI_FILE_PTR file;
	char brush_section_name[256];
	char brush_name[1024];
	char *write_str;
	int brush_id = 1;
	int x, y;

	// ファイルオープンに失敗したら上書きで試す
	if(stream == NULL)
	{
		stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);

		if(stream == NULL)
		{
			g_object_unref(fp);
			return -1;
		}
	}

	file = CreateIniFile(stream, NULL,0, INI_WRITE);

	// 文字コードを書き込む
	IniFileAddString(file, "CODE", "CODE_TYPE", window->vector_brush_code);

	// フォントファイル名を書き込む
	IniFileAddString(file, "FONT", "FONT_FILE", window->font_file);

	// ブラシテーブルの内容を書き出す
	for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT; y++)
	{
		for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH; x++)
		{
			if(window->vector_brushes[y][x].name != NULL)
			{
				(void)sprintf(brush_section_name, "BRUSH%d", brush_id);

				(void)strcpy(brush_name, window->vector_brushes[y][x].name);
				StringRepalce(brush_name, "\n", "\\n");
				write_str = g_convert(brush_name, -1, window->vector_brush_code, "UTF-8", NULL, NULL, NULL);

				(void)IniFileAddString(file, brush_section_name,
					"NAME", write_str);
				g_free(write_str);
				(void)IniFileAddString(file, brush_section_name,
					"IMAGE", window->vector_brushes[y][x].image_file_path);
				(void)IniFileAddInteger(file, brush_section_name,
					"X", x, 10);
				(void)IniFileAddInteger(file, brush_section_name,
					"Y", y, 10);

				switch(window->vector_brushes[y][x].brush_type)
				{
				case TYPE_POLY_LINE_BRUSH:
					{
						POLY_LINE_BRUSH *line = (POLY_LINE_BRUSH*)window->vector_brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "POLY_LINE");
						(void)IniFileAddInteger(file, brush_section_name, "SIZE",
							(int)(line->r * 2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR",
							(int)line->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)line->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW",
							(int)(((line->flow + 0.5) * 100) / 255), 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((line->flags & POLY_LINE_SIZE_WITH_PRESSURE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "ANTI_ALIAS",
							((line->flags & POLY_LINE_ANTI_ALIAS) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "LINE_TYPE",
							line->line_type, 10);
						(void)IniFileAddInteger(file, brush_section_name, "FIRST_PRESSURE",
							line->first_pressure, 10);
						(void)IniFileAddInteger(file, brush_section_name, "LAST_PRESSURE",
							line->last_pressure, 10);
					}
					break;
				case TYPE_BEZIER_LINE_BRUSH:
					{
						BEZIER_LINE_BRUSH *line = (BEZIER_LINE_BRUSH*)window->vector_brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "BEZIER_LINE");
						(void)IniFileAddInteger(file, brush_section_name, "SIZE",
							(int)(line->r * 2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR",
							(int)line->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)line->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW",
							(int)(((line->flow + 0.5) * 100) / 255), 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((line->flags & BEZIER_LINE_SIZE_WITH_PRESSURE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "ANTI_ALIAS",
							((line->flags & BEZIER_LINE_ANTI_ALIAS) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "LINE_TYPE",
							line->line_type, 10);
						(void)IniFileAddInteger(file, brush_section_name, "FIRST_PRESSURE",
							line->first_pressure, 10);
						(void)IniFileAddInteger(file, brush_section_name, "LAST_PRESSURE",
							line->last_pressure, 10);
					}
					break;
				case TYPE_FREE_HAND_BRUSH:
					{
						FREE_HAND_BRUSH *free_hand =
							(FREE_HAND_BRUSH*)window->vector_brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "FREE_HAND");
						(void)IniFileAddInteger(file, brush_section_name, "SIZE",
							(int)(free_hand->r * 2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR",
							(int)free_hand->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)free_hand->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW",
							(int)(((free_hand->flow + 0.5) * 100) / 255), 10);
						(void)IniFileAddInteger(file, brush_section_name, "MINIMUM_DEGREE",
							(int)free_hand->min_degree, 10);
						(void)IniFileAddInteger(file, brush_section_name, "MINIMUM_DISTANCE",
							(int)free_hand->min_distance, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							(int)((free_hand->flags & FREE_HAND_SIZE_WITH_PRESSURE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "ANTI_ALIAS",
							(int)((free_hand->flags & FREE_HAND_ANTI_ALIAS) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRIOR_ANGLE",
							(int)((free_hand->flags & FREE_HAND_PRIOR_ARG) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "LINE_TYPE",
							free_hand->line_type, 10);
					}
					break;
				case TYPE_CONTROL_POINT:
					{
						CONTROL_POINT_TOOL *control =
							(CONTROL_POINT_TOOL*)window->vector_brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "CONTROL_POINT");
						(void)IniFileAddInteger(file, brush_section_name, "MODE",
							control->mode, 10);
					}
					break;
				case TYPE_CHANGE_LINE_COLOR:
					{
						CHANGE_LINE_COLOR_TOOL *color =
							(CHANGE_LINE_COLOR_TOOL*)window->vector_brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "CHANGE_LINE_COLOR");
						(void)IniFileAddInteger(file, brush_section_name, "FLOW",
							(int)(((color->flow + 0.5) * 100) / 255), 10);
					}
					break;
				case TYPE_CHANGE_LINE_SIZE:
					{
						CHANGE_LINE_SIZE_TOOL *size =
							(CHANGE_LINE_SIZE_TOOL*)window->vector_brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "CHANGE_LINE_SIZE");
						(void)IniFileAddInteger(file, brush_section_name, "SIZE",
							(int)(size->r * 2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR", (int)size->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)size->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW",
							(int)(((size->flow + 0.5) * 100) / 255), 10);
					}
					break;
				case TYPE_VECTOR_ERASER:
					{
						VECTOR_ERASER *eraser =
							(VECTOR_ERASER*)window->vector_brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "VECTOR_ERASER");
						(void)IniFileAddInteger(file, brush_section_name, "SIZE",
							(int)(eraser->r * 2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "MODE", eraser->mode, 10);
					}
					break;
				}

				brush_id++;
			}	// if(window->vector_brushes[y][x].name != NULL)
		}	// for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH; x++)
	}	// ブラシテーブルの内容を書き出す
		// for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT; y++)

	WriteIniFile(file, (size_t (*)(void*, size_t, size_t, void*))FileWrite);
	file->delete_func(file);

	g_object_unref(fp);
	g_object_unref(stream);

	return 0;
}

#ifdef __cplusplus
}
#endif
