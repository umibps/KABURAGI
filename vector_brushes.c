// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
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

#include "gui/GTK/gtk_widgets.h"

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

		if(((GdkEventButton*)state)->type == GDK_2BUTTON_PRESS && ((VECTOR_BASE_DATA*)layer->top_data)->vector_type < NUM_VECTOR_LINE_TYPE
			&& (line->flags & POLY_LINE_START) != 0 && ((VECTOR_LINE*)layer->top_data)->num_points > 3)
		{
			VECTOR_LINE *top_line = (VECTOR_LINE*)layer->top_data;
			top_line->num_points -= 2;
			top_line->points[top_line->num_points-1].pressure =
				line->before_pressure * 200 * line->last_pressure * 0.01;
			line->flags &= ~(POLY_LINE_START);
			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
		}
		else
		{
			if((line->flags & POLY_LINE_START) == 0)
			{
				VECTOR_LINE *top_line;
				layer->flags |= VECTOR_LAYER_RASTERIZE_TOP;
				layer->top_data = (void*)(top_line = CreateVectorLine((VECTOR_LINE*)layer->top_data, NULL));
				if((line->flags & POLY_LINE_ANTI_ALIAS) != 0)
				{
					top_line->flags |= VECTOR_LINE_ANTI_ALIAS;
				}
				line->flags |= POLY_LINE_START;
				top_line->base_data.vector_type = line->line_type;
				top_line->blur = line->blur;
				top_line->outline_hardness = line->outline_hardness;
				top_line->points =
					(VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
				(void)memset(top_line->points, 0,
						sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
				top_line->buff_size = VECTOR_LINE_BUFFER_SIZE;
				top_line->points->x = x;
				top_line->points->y = y;
				top_line->points->size = line->r;
				top_line->points->pressure = pressure * 200 * line->first_pressure * 0.01;
				(void)memcpy(top_line->points->color,
					window->app->tool_window.color_chooser->rgb, 3);
				top_line->points->color[3] = line->flow;
				top_line->points[1].x = x;
				top_line->points[1].y = y;
				top_line->points[1].size = line->r;
				top_line->points[1].pressure = pressure * 200;
				(void)memcpy(top_line->points[1].color,
					window->app->tool_window.color_chooser->rgb, 3);
				top_line->points[1].color[3] = line->flow;
				top_line->num_points = 2;
				layer->num_lines++;

				g_timer_start(line->timer);

				AddTopLineControlPointHistory(window, window->active_layer, top_line,
					&top_line->points[1], core->name, TRUE);
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
					((VECTOR_LINE*)layer->top_data)->num_points--;
					line->flags &= ~(POLY_LINE_START);
					layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
				}
				else
				{
					VECTOR_LINE *top_line = (VECTOR_LINE*)layer->top_data;
					top_line->points[top_line->num_points].x = x;
					top_line->points[top_line->num_points].y = y;
					top_line->points[top_line->num_points].size = line->r;
					top_line->points[top_line->num_points].pressure = pressure * 200 * line->last_pressure * 0.01;
					(void)memcpy(top_line->points[top_line->num_points].color,
						window->app->tool_window.color_chooser->rgb, 3);
					top_line->points[top_line->num_points].color[3] = line->flow;

					if(top_line->num_points > 1)
					{
						top_line->points[top_line->num_points-1].pressure = line->before_pressure * 200;
					}

					AddTopLineControlPointHistory(window, window->active_layer,
						top_line, &top_line->points[top_line->num_points], core->name, FALSE);

					top_line->num_points++;
					top_line->points[top_line->num_points].pressure = pressure * 200;

					if(top_line->num_points >= top_line->buff_size-1)
					{
						top_line->buff_size += VECTOR_LINE_BUFFER_SIZE;
						top_line->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
							top_line->points, top_line->buff_size*sizeof(VECTOR_POINT));
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
		VECTOR_LINE *top_line = (VECTOR_LINE*)layer->top_data;

		if((line->flags & POLY_LINE_START) != 0 && top_line->num_points > 2)
		{
			top_line->num_points -= 1;
			top_line->points[top_line->num_points-1].pressure =
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
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;
		VECTOR_LINE *top_line = (VECTOR_LINE*)layer->top_data;
		layer->flags |= VECTOR_LAYER_RASTERIZE_TOP;
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		top_line->points[top_line->num_points-1].x = x;
		top_line->points[top_line->num_points-1].y = y;
	}
}

#define PolyLineReleaseCallBack VectorBrushDummyCallBack

static void PolyLineKeyPressCallBack(DRAW_WINDOW *window, GdkEventKey* key, void* data)
{
#if GTK_MAJOR_VERSION <= 2
	if(key->keyval == GDK_Return)
#else
	if(key->keyval == GDK_KEY_Return)
#endif
	{
		POLY_LINE_BRUSH* line = (POLY_LINE_BRUSH*)data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;

		if((line->flags & POLY_LINE_START) != 0 && ((VECTOR_BASE_DATA*)layer->top_data)->vector_type < NUM_VECTOR_LINE_TYPE
			&& ((VECTOR_LINE*)layer->top_data)->num_points > 2)
		{
			((VECTOR_LINE*)layer->top_data)->num_points -= 1;
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
	g_object_set_data(G_OBJECT(buttons[0]), "line_type", GINT_TO_POINTER(VECTOR_TYPE_STRAIGHT));
	buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.close_path);
	g_object_set_data(G_OBJECT(buttons[1]), "line_type", GINT_TO_POINTER(VECTOR_TYPE_STRAIGHT_CLOSE));
	if(line->line_type == VECTOR_TYPE_STRAIGHT)
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
			&& (line->flags & BEZIER_LINE_START) != 0 && ((VECTOR_LINE*)layer->top_data)->num_points > 3)
		{
			((VECTOR_LINE*)layer->top_data)->num_points -= 2;
			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].pressure =
				line->before_pressure * 200 * line->last_pressure * 0.01;
			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
			line->flags &= ~(BEZIER_LINE_START);
		}
		else
		{
			if((line->flags & BEZIER_LINE_START) == 0)
			{
				layer->flags |= VECTOR_LAYER_RASTERIZE_TOP;
				layer->top_data = (void*)CreateVectorLine((VECTOR_LINE*)layer->top_data, NULL);
				if((line->flags & BEZIER_LINE_ANTI_ALIAS) != 0)
				{
					((VECTOR_LINE*)layer->top_data)->flags |= VECTOR_LINE_ANTI_ALIAS;
				}
				line->flags |= BEZIER_LINE_START;
				((VECTOR_LINE*)layer->top_data)->base_data.vector_type = line->line_type;
				((VECTOR_LINE*)layer->top_data)->blur = line->blur;
				((VECTOR_LINE*)layer->top_data)->outline_hardness = line->outline_hardness;
				((VECTOR_LINE*)layer->top_data)->points =
					(VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
				((VECTOR_LINE*)layer->top_data)->buff_size = VECTOR_LINE_BUFFER_SIZE;
				(void)memset(((VECTOR_LINE*)layer->top_data)->points, 0,
						sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
				((VECTOR_LINE*)layer->top_data)->points->x = x;
				((VECTOR_LINE*)layer->top_data)->points->y = y;
				((VECTOR_LINE*)layer->top_data)->points->size = line->r;
				((VECTOR_LINE*)layer->top_data)->points->pressure = pressure * 200 * line->first_pressure * 0.01;
				(void)memcpy(((VECTOR_LINE*)layer->top_data)->points->color,
					window->app->tool_window.color_chooser->rgb, 3);
				((VECTOR_LINE*)layer->top_data)->points->color[3] = line->flow;
				((VECTOR_LINE*)layer->top_data)->points[1].x = x;
				((VECTOR_LINE*)layer->top_data)->points[1].y = y;
				((VECTOR_LINE*)layer->top_data)->points[1].size = line->r;
				((VECTOR_LINE*)layer->top_data)->points[1].pressure = pressure * 200;
				(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[1].color,
					window->app->tool_window.color_chooser->rgb, 3);
				((VECTOR_LINE*)layer->top_data)->points[1].color[3] = line->flow;
				((VECTOR_LINE*)layer->top_data)->num_points = 2;
				layer->num_lines++;

				AddTopLineControlPointHistory(window, window->active_layer, (VECTOR_LINE*)layer->top_data,
					&((VECTOR_LINE*)layer->top_data)->points[1], core->name, TRUE);

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
					((VECTOR_LINE*)layer->top_data)->num_points--;
					layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
					line->flags &= ~(BEZIER_LINE_START);
				}
				else
				{
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].x = x;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].y = y;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].size = line->r;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].pressure = pressure * 200 * line->last_pressure * 0.01;
					(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color,
						window->app->tool_window.color_chooser->rgb, 3);
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color[3] = line->flow;

					if(((VECTOR_LINE*)layer->top_data)->num_points > 1)
					{
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].pressure = line->before_pressure * 200;
					}

					AddTopLineControlPointHistory(window, window->active_layer,
						(VECTOR_LINE*)layer->top_data, &((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points], core->name, FALSE);

					((VECTOR_LINE*)layer->top_data)->num_points++;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].size = line->r;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].pressure = pressure * 200 * line->last_pressure * 0.01;

					if(((VECTOR_LINE*)layer->top_data)->num_points >= ((VECTOR_LINE*)layer->top_data)->buff_size-1)
					{
						((VECTOR_LINE*)layer->top_data)->buff_size += VECTOR_LINE_BUFFER_SIZE;
						((VECTOR_LINE*)layer->top_data)->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
							((VECTOR_LINE*)layer->top_data)->points, ((VECTOR_LINE*)layer->top_data)->buff_size*sizeof(VECTOR_POINT));
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

		if((line->flags & BEZIER_LINE_START) != 0 && ((VECTOR_LINE*)layer->top_data)->num_points > 2)
		{
			((VECTOR_LINE*)layer->top_data)->num_points -= 1;
			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].pressure =
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
		((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].x = x;
		((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].y = y;
	}
}

#define BezierLineReleaseCallBack VectorBrushDummyCallBack

static void BezierLineKeyPressCallBack(DRAW_WINDOW *window, GdkEventKey* key, void* data)
{
#if GTK_MAJOR_VERSION <= 2
	if(key->keyval == GDK_Return)
#else
	//if(key->keyval == GDK_KEY_Return)
#endif
	{
		BEZIER_LINE_BRUSH* line = (BEZIER_LINE_BRUSH*)data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;

		if((line->flags & BEZIER_LINE_START) != 0 && ((VECTOR_LINE*)layer->top_data)->num_points > 2)
		{
			((VECTOR_LINE*)layer->top_data)->num_points -= 1;
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
	g_object_set_data(G_OBJECT(buttons[0]), "line_type", GINT_TO_POINTER(VECTOR_TYPE_BEZIER_OPEN));
	buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.close_path);
	g_object_set_data(G_OBJECT(buttons[1]), "line_type", GINT_TO_POINTER(VECTOR_TYPE_BEZIER_CLOSE));
	if(line->line_type == VECTOR_TYPE_BEZIER_OPEN)
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

		layer->top_data = (void*)CreateVectorLine((VECTOR_LINE*)layer->top_data, NULL);
		((VECTOR_LINE*)layer->top_data)->base_data.vector_type = free_hand->line_type;
		if((free_hand->flags & FREE_HAND_ANTI_ALIAS) != 0)
		{
			((VECTOR_LINE*)layer->top_data)->flags |= VECTOR_LINE_ANTI_ALIAS;
		}
		((VECTOR_LINE*)layer->top_data)->points =
			(VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
		(void)memset(((VECTOR_LINE*)layer->top_data)->points, 0,
			sizeof(VECTOR_POINT)*VECTOR_LINE_BUFFER_SIZE);
		((VECTOR_LINE*)layer->top_data)->buff_size = VECTOR_LINE_BUFFER_SIZE;
		((VECTOR_LINE*)layer->top_data)->points->x = x;
		((VECTOR_LINE*)layer->top_data)->points->y = y;
		((VECTOR_LINE*)layer->top_data)->points->size = free_hand->r;
		((VECTOR_LINE*)layer->top_data)->points->pressure = pressure * 200;
		((VECTOR_LINE*)layer->top_data)->blur = free_hand->blur;
		((VECTOR_LINE*)layer->top_data)->outline_hardness = free_hand->outline_hardness;
		(void)memcpy(((VECTOR_LINE*)layer->top_data)->points->color, *core->color, 3);
		((VECTOR_LINE*)layer->top_data)->points->color[3] = free_hand->flow;
		((VECTOR_LINE*)layer->top_data)->num_points++;

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
				if(((VECTOR_LINE*)layer->top_data)->num_points == 1)
				{
					free_hand->before_arg = arg;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].x = x;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].y = y;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].pressure = pressure*200;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].size = free_hand->r;
					(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color,
						color, 3);
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color[3] = free_hand->flow;
					((VECTOR_LINE*)layer->top_data)->num_points++;
				}
				else
				{
					if(fabs(free_hand->before_arg-arg) >= free_hand->min_arg)
					{
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].x = x;
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].y = y;
						(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color,
							core->color, 3);
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].pressure = pressure * 200;
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].size = free_hand->r;
						(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color,
							color, 3);
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color[3] = free_hand->flow;
						((VECTOR_LINE*)layer->top_data)->num_points++;

						if(((VECTOR_LINE*)layer->top_data)->num_points >= ((VECTOR_LINE*)layer->top_data)->buff_size-1)
						{
							((VECTOR_LINE*)layer->top_data)->buff_size += VECTOR_LINE_BUFFER_SIZE;
							((VECTOR_LINE*)layer->top_data)->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
								((VECTOR_LINE*)layer->top_data)->points, ((VECTOR_LINE*)layer->top_data)->buff_size*sizeof(VECTOR_POINT));
						}

						free_hand->before_x = x;
						free_hand->before_y = y;
						free_hand->before_arg = arg;
					}
					else
					{
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].x = x;
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].y = y;
					}
				}
			}
		}
		else
		{
			if(((VECTOR_LINE*)layer->top_data)->num_points < 2)
			{
				if(d >= free_hand->min_distance)
				{
					free_hand->before_arg = arg;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].x = x;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].y = y;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].pressure = pressure*200;
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].size = free_hand->r;
					(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color,
						color, 3);
					((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color[3] = free_hand->flow;
					((VECTOR_LINE*)layer->top_data)->num_points++;
					free_hand->before_x = x;
					free_hand->before_y = y;
				}
			}
			else
			{
				if(d >= FREE_HAND_MINIMUM_DISTANCE)
				{
					dx = free_hand->before_x - ((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-2].x;
					dy = free_hand->before_y - ((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-2].y;
					arg2 = atan2(dy, dx);

					if(fabs(arg - arg2) > free_hand->min_arg)
					{
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].x = x;
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].y = y;
						(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color,
							core->color, 3);
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].pressure = pressure * 200;
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].size = free_hand->r;
						(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color,
							color, 3);
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color[3] = free_hand->flow;
						((VECTOR_LINE*)layer->top_data)->num_points++;

						if(((VECTOR_LINE*)layer->top_data)->num_points >= ((VECTOR_LINE*)layer->top_data)->buff_size-1)
						{
							((VECTOR_LINE*)layer->top_data)->buff_size += VECTOR_LINE_BUFFER_SIZE;
							((VECTOR_LINE*)layer->top_data)->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
								((VECTOR_LINE*)layer->top_data)->points, ((VECTOR_LINE*)layer->top_data)->buff_size*sizeof(VECTOR_POINT));
						}

						free_hand->before_x = x;
						free_hand->before_y = y;
						free_hand->before_arg = arg;
					}
					else
					{
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].x = x;
						((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].y = y;
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
	VECTOR_BASE_DATA *next_top;
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

	next_top = ((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->prev;
	DeleteVectorLine(((VECTOR_LINE**)(&(layer->layer_data.vector_layer_p->top_data))));
	layer->layer_data.vector_layer_p->top_data = (void*)next_top;

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

	add_line = (VECTOR_LINE*)(layer->layer_data.vector_layer_p->top_data =
		(void*)CreateVectorLine((VECTOR_LINE*)layer->layer_data.vector_layer_p->top_data, NULL));
	add_line->base_data.vector_type = data.line_data.base_data.vector_type;
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

	// �����������o��
	(void)MemWrite(add_line, sizeof(*add_line), 1, stream);
	// ���C���[�̖��O�������o��
	(void)MemWrite(&name_length, sizeof(name_length), 1, stream);
	(void)MemWrite(active_layer->name, 1, name_length, stream);
	// ���W�f�[�^�������o��
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

		if(((free_hand->flags & FREE_HAND_PRIOR_ARG) == 0) || ((VECTOR_LINE*)layer->top_data)->num_points == 1)
		{
			if((free_hand->flags & FREE_HAND_SIZE_WITH_PRESSURE) == 0)
			{
				pressure = 0.5;
			}

			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].x = x;
			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].y = y;
			(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color,
				core->color, 3);
			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color[3] = free_hand->flow;
			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].pressure = pressure * 200;
			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].size = free_hand->r;
			(void)memcpy(((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color,
				color, 3);
			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points].color[3] = free_hand->flow;
			((VECTOR_LINE*)layer->top_data)->num_points++;

			if(((VECTOR_LINE*)layer->top_data)->num_points >= ((VECTOR_LINE*)layer->top_data)->buff_size-1)
			{
				((VECTOR_LINE*)layer->top_data)->buff_size += VECTOR_LINE_BUFFER_SIZE;
				((VECTOR_LINE*)layer->top_data)->points = (VECTOR_POINT*)MEM_REALLOC_FUNC(
					((VECTOR_LINE*)layer->top_data)->points, ((VECTOR_LINE*)layer->top_data)->buff_size*sizeof(VECTOR_POINT));
			}

			for(i=0; i<((VECTOR_LINE*)layer->top_data)->num_points-1 && ((VECTOR_LINE*)layer->top_data)->num_points > 2; i++)
			{
				if((((VECTOR_LINE*)layer->top_data)->points[i].x-((VECTOR_LINE*)layer->top_data)->points[i+1].x)*(((VECTOR_LINE*)layer->top_data)->points[i].x-((VECTOR_LINE*)layer->top_data)->points[i+1].x)
					+ (((VECTOR_LINE*)layer->top_data)->points[i].y-((VECTOR_LINE*)layer->top_data)->points[i+1].y)*(((VECTOR_LINE*)layer->top_data)->points[i].y-((VECTOR_LINE*)layer->top_data)->points[i+1].y)
					<= free_hand->min_distance*free_hand->min_distance)
				{
					(void)memmove(&((VECTOR_LINE*)layer->top_data)->points[i+1], &((VECTOR_LINE*)layer->top_data)->points[i+2],
						sizeof(VECTOR_POINT)*(((VECTOR_LINE*)layer->top_data)->num_points-i-1));
					((VECTOR_LINE*)layer->top_data)->num_points--;
				}
			}
		}
		else
		{
			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].x = x;
			((VECTOR_LINE*)layer->top_data)->points[((VECTOR_LINE*)layer->top_data)->num_points-1].y = y;
		}

		AddFreeHandHistory(window, window->active_layer, (VECTOR_LINE*)layer->top_data, core->name);
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
	FREE_HAND_BRUSH *free_hand = (FREE_HAND_BRUSH*)data;
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
	g_object_set_data(G_OBJECT(buttons[0]), "line_type", GINT_TO_POINTER(VECTOR_TYPE_BEZIER_OPEN));
	buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.close_path);
	g_object_set_data(G_OBJECT(buttons[1]), "line_type", GINT_TO_POINTER(VECTOR_TYPE_BEZIER_CLOSE));
	if(free_hand->line_type == VECTOR_TYPE_BEZIER_OPEN)
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int line_index = 0;
	int point_index;

	while(line != change_line)
	{
		line = line->base_data.next;
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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
		line = CreateVectorLine((VECTOR_LINE*)line->base_data.prev, line);
		line->num_points = 2;
		line->points[0] = history_data->before_point_data;
		line->points[1] = history_data->change_data;
		line->base_data.vector_type = history_data->add_flag-2;
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
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int line_index = 0;
	int point_index;

	while(line != change_line)
	{
		line = line->base_data.next;
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int line_index = 0;

	while(line != change_line)
	{
		line = line->base_data.next;
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int line_index = 0;
	int point_index;

	while(line != change_line)
	{
		line = line->base_data.next;
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
	VECTOR_LINE* base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
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
						line = base->base_data.next;
						window->active_layer->layer_data.vector_layer_p->active_data = NULL;
						window->active_layer->layer_data.vector_layer_p->active_point = NULL;
						while(line != NULL && end_flag == 0)
						{
							if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
							{
								if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
									&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
								{
									layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);
				
									for(i=0; i<line->num_points; i++)
									{
										distance = (x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y);
										if(distance < POINT_SELECT_DISTANCE)
										{
											window->active_layer->layer_data.vector_layer_p->active_point =
												&line->points[i];
											window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
											control->flags |= CONTROL_POINT_TOOL_HAS_POINT;
											window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;

											if(distance < 4)
											{
												break;
											}
										}
									}

									if(window->active_layer->layer_data.vector_layer_p->active_data != NULL)
									{
										break;
									}

									start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
									end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

									if(start_x < 0)
									{
										start_x = 0;
									}
									else if(end_x >= line->base_data.layer->width)
									{
										end_x = line->base_data.layer->width - 1;
									}

									if(start_y < 0)
									{
										start_y = 0;
									}
									else if(end_y >= line->base_data.layer->height)
									{
										end_y = line->base_data.layer->height - 1;
									}

									hit = 0;
									for(i=start_y; i<end_y && hit == 0; i++)
									{
										for(j=start_x; j<end_x; j++)
										{
											if(line->base_data.layer->pixels[i*line->base_data.layer->stride+j*4+3] != 0)
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
										&& window->active_layer->layer_data.vector_layer_p->active_data == NULL)
									{
										window->active_layer->layer_data.vector_layer_p->active_data = (void*)add_point_line;

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

							line = line->base_data.next;
						}
					}	// if((control->flags & CONTROL_POINT_TOOL_HAS_LOCK) == 0)
					else
					{
						line = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;
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
					line = base->base_data.next;
					window->active_layer->layer_data.vector_layer_p->active_data = NULL;
					window->active_layer->layer_data.vector_layer_p->active_point = NULL;
					while(line != NULL && end_flag == 0)
					{
						if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
						{
							if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
								&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
							{
								layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

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
											if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
											{
												window->active_layer->layer_data.vector_layer_p->top_data =
													line->base_data.prev;
											}

											AddControlPointDeleteHistory(window, line, &line->points[0], &line->points[1], line->base_data.vector_type+2);
											DeleteVectorLine(&line);
											window->active_layer->layer_data.vector_layer_p->active_data = NULL;
										}
										else
										{
											AddControlPointDeleteHistory(window, line, &line->points[i], &control->change_point_data, 1);
											window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
											window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
										}
										window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
										end_flag++;
										break;
									}
								}
								break;
							}
						}

						line = line->base_data.next;
					}
				}
				else
				{
					line = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;
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
								if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
								{
									window->active_layer->layer_data.vector_layer_p->top_data =
										(void*)line->base_data.prev;
								}

								AddControlPointDeleteHistory(window, line, &line->points[0], &line->points[1], line->base_data.vector_type+2);
								DeleteVectorLine(&line);
								window->active_layer->layer_data.vector_layer_p->active_data = NULL;
							}
							else
							{
								AddControlPointDeleteHistory(window, line, &line->points[i], &control->change_point_data, 1);
								window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
								window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
							}
						}
					}
				}
			}
			break;
		case CONTROL_STROKE_MOVE:
			if((control->flags & CONTROL_POINT_TOOL_HAS_STROKE) == 0)
			{
				line = base->base_data.next;
				window->active_layer->layer_data.vector_layer_p->active_data = NULL;
				window->active_layer->layer_data.vector_layer_p->active_point = NULL;
				while(line != NULL)
				{
					if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
					{
						if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
							&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
						{
							layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

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
									window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
									break;
								}
							}

							if(window->active_layer->layer_data.vector_layer_p->active_data != NULL)
							{
								break;
							}

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->base_data.layer->width)
							{
								end_x = line->base_data.layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->base_data.layer->height)
							{
								end_y = line->base_data.layer->height - 1;
							}

							hit = 0;
							for(i=start_y; i<end_y && hit == 0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->base_data.layer->pixels[i*line->base_data.layer->stride+j*4+3] != 0)
									{
										hit++;
										break;
									}
								}
							}

							if(hit != 0
								&& window->active_layer->layer_data.vector_layer_p->active_data == NULL)
							{
								window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
								control->change_point_data = line->points[0];
							}
						}
					}

					line = line->base_data.next;
				}

				if(window->active_layer->layer_data.vector_layer_p->active_data != NULL)
				{
					control->flags |= CONTROL_POINT_TOOL_HAS_STROKE;
					control->before_x = x;
					control->before_y = y;
				}
			}
		}
	}
	else if(((GdkEventButton*)state)->button == 3)
	{	// �E�N���b�N�Ȃ�
		// ����_����c�[���̏ڍ׃f�[�^�ɃL���X�g
		CONTROL_POINT_TOOL *control = (CONTROL_POINT_TOOL*)core->brush_data;
		// ��ԉ��̃��C��
		VECTOR_LINE *base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
		// �����蔻����ۂ����Ƃ����郉�C���ւ̃|�C���^
		VECTOR_LINE *line;
		// 2�T�ڂ̃t���O
		int second_loop = 0;
		// ���C�����C���[�̍��W
		int layer_x, layer_y;
		// �����蔻��̊J�n�E�I�����W
		int start_x, end_x, start_y, end_y;
		// ���[�v�I���̃t���O
		int end_flag = 0;
		// �����蔻��̃t���O
		int hit = 0;
		int i, j;	// for���p�̃J�E���^

		switch(control->mode)
		{
		case CONTROL_POINT_SELECT:
		case CONTROL_POINT_CHANGE_PRESSURE:
		case CONTROL_POINT_DELETE:
			if(window->active_layer->layer_data.vector_layer_p->active_point != NULL)
			{
				VECTOR_POINT *before_active = window->active_layer->layer_data.vector_layer_p->active_point;

				// ���C�����C���[�̃s�N�Z�����l��0�łȂ���Γ�����
				line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)window->active_layer->layer_data.vector_layer_p->active_data)->next;
				// �E�N���b�N�őI�����ꂽ�ꍇ�͈�苗���}�E�X���ړ�����܂őI�����C����ύX���Ȃ�
				control->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// �}�E�X�̍��W���L��
				control->lock_x = x, control->lock_y = y;

				while(hit == 0)
				{
					if(line != NULL && line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
					{
						if(line->base_data.layer != NULL)
						{
							if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
								&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
							{
								for(i=0; i<line->num_points; i++)
								{	// �}�E�X�J�[�\���t�߂�
									if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
										<= POINT_SELECT_DISTANCE)
									{
										window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
										(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
										cairo_set_source_surface(window->work_layer->cairo_p, line->base_data.layer->surface_p,
											line->base_data.layer->x, line->base_data.layer->y);
										cairo_paint(window->work_layer->cairo_p);
										window->active_layer->layer_data.vector_layer_p->active_point = &line->points[i];
										hit++;
										break;
									}
								}
							}
						}

						line = line->base_data.next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->base_data.next;
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

				// ���C�����C���[�̃s�N�Z�����l��0�łȂ���Γ�����
				line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)window->active_layer->layer_data.vector_layer_p->active_data)->next;
				// �E�N���b�N�őI�����ꂽ�ꍇ�͈�苗���}�E�X���ړ�����܂őI�����C����ύX���Ȃ�
				control->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// �}�E�X�̍��W���L��
				control->lock_x = x, control->lock_y = y;

				while(hit == 0)
				{
					if(line != NULL && line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
					{
						if(line->base_data.layer != NULL)
						{
							if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
								&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
							{
								for(i=0; i<line->num_points; i++)
								{	// �}�E�X�J�[�\���t�߂�
									if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
										<= POINT_SELECT_DISTANCE)
									{
										window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
										(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
										cairo_set_source_surface(window->work_layer->cairo_p, line->base_data.layer->surface_p,
											line->base_data.layer->x, line->base_data.layer->y);
										cairo_paint(window->work_layer->cairo_p);
										window->active_layer->layer_data.vector_layer_p->active_point = &line->points[i];
										hit++;
										break;
									}
								}
							}
						}

						line = line->base_data.next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->base_data.next;
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
			else if(window->active_layer->layer_data.vector_layer_p->active_data != NULL)
			{
				VECTOR_LINE *before_active = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;

				// ���C�����C���[�̃s�N�Z�����l��0�łȂ���Γ�����
				line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)window->active_layer->layer_data.vector_layer_p->active_data)->next;
				// �E�N���b�N�őI�����ꂽ�ꍇ�͈�苗���}�E�X���ړ�����܂őI�����C����ύX���Ȃ�
				control->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// �}�E�X�̍��W���L��
				control->lock_x = x, control->lock_y = y;

				while(hit == 0)
				{	// �e���C���[�����̃��C���ɑ΂��ď������s
					if(line != NULL && line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
					{
						if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
							&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
						{
							layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->base_data.layer->width)
							{
								end_x = line->base_data.layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->base_data.layer->height)
							{
								end_y = line->base_data.layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->base_data.layer->pixels[i*line->base_data.layer->stride+j*4+3] != 0)
									{
										window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
										(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
										cairo_set_source_surface(window->work_layer->cairo_p, line->base_data.layer->surface_p,
											line->base_data.layer->x, line->base_data.layer->y);
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

						line = line->base_data.next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->base_data.next;
							second_loop++;
						}
						else
						{
							break;
						}
					}
				}

				if(window->active_layer->layer_data.vector_layer_p->active_data == NULL)
				{
					window->active_layer->layer_data.vector_layer_p->active_data =
						(void*)before_active;
				}
			}
			break;
		case CONTROL_STROKE_MOVE:
		case CONTROL_STROKE_COPY_MOVE:
		case CONTROL_STROKE_JOINT:
			if(window->active_layer->layer_data.vector_layer_p->active_data != NULL)
			{
				VECTOR_LINE *before_active = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;

				// ���C�����C���[�̃s�N�Z�����l��0�łȂ���Γ�����
				line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)window->active_layer->layer_data.vector_layer_p->active_data)->next;
				// �E�N���b�N�őI�����ꂽ�ꍇ�͈�苗���}�E�X���ړ�����܂őI�����C����ύX���Ȃ�
				control->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// �}�E�X�̍��W���L��
				control->lock_x = x, control->lock_y = y;

				while(hit == 0)
				{	// �e���C���[�����̃��C���ɑ΂��ď������s
					if(line != NULL && line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
					{
						if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
							&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
						{
							layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->base_data.layer->width)
							{
								end_x = line->base_data.layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->base_data.layer->height)
							{
								end_y = line->base_data.layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->base_data.layer->pixels[i*line->base_data.layer->stride+j*4+3] != 0)
									{
										window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
										(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
										cairo_set_source_surface(window->work_layer->cairo_p, line->base_data.layer->surface_p,
											line->base_data.layer->x, line->base_data.layer->y);
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

						line = line->base_data.next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->base_data.next;
							second_loop++;
						}
						else
						{
							break;
						}
					}
				}

				if(window->active_layer->layer_data.vector_layer_p->active_data == NULL)
				{
					window->active_layer->layer_data.vector_layer_p->active_data =
						(void*)before_active;
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
	VECTOR_LINE* base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
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

			line = base->base_data.next;
			window->active_layer->layer_data.vector_layer_p->active_data = NULL;
			window->active_layer->layer_data.vector_layer_p->active_point = NULL;
			while(line != NULL)
			{
				if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
						&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
					{
						layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);
						
						for(i=0; i<line->num_points; i++)
						{
							distance = (x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y);
							if(distance < POINT_SELECT_DISTANCE)
							{
								window->active_layer->layer_data.vector_layer_p->active_point =
									&line->points[i];
								window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
								
								if(distance < 4)
								{
									break;
								}
							}
						}

						if(line->base_data.layer->pixels[layer_y*line->base_data.layer->stride+layer_x*4+3] != 0
							&& window->active_layer->layer_data.vector_layer_p->active_data == NULL)
						{
							window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
							(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
							cairo_set_source_surface(window->work_layer->cairo_p, line->base_data.layer->surface_p,
								line->base_data.layer->x, line->base_data.layer->y);
							cairo_paint(window->work_layer->cairo_p);
							break;
						}
					}
				}

				line = line->base_data.next;
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
			line = base->base_data.next;
			window->active_layer->layer_data.vector_layer_p->active_data = NULL;
			window->active_layer->layer_data.vector_layer_p->active_point = NULL;
			while(line != NULL)
			{
				if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
						&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
					{
						layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);
						
						for(i=0; i<line->num_points; i++)
						{
							distance = (x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y);
							if(distance < POINT_SELECT_DISTANCE)
							{
								window->active_layer->layer_data.vector_layer_p->active_point =
									&line->points[i];
								window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;

								if(distance < 4)
								{
									break;
								}
							}
						}

						if(line->base_data.layer->pixels[layer_y*line->base_data.layer->stride+layer_x*4+3] != 0
							&& window->active_layer->layer_data.vector_layer_p->active_data == NULL)
						{
							window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
							(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
							cairo_set_source_surface(window->work_layer->cairo_p, line->base_data.layer->surface_p,
								line->base_data.layer->x, line->base_data.layer->y);
							cairo_paint(window->work_layer->cairo_p);
							break;
						}
					}
				}

				line = line->base_data.next;
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
		line = base->base_data.next;
		window->active_layer->layer_data.vector_layer_p->active_data = NULL;
		window->active_layer->layer_data.vector_layer_p->active_point = NULL;
		while(line != NULL)
		{
			if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
			{
				if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
					&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
				{
					layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);
					
					for(i=0; i<line->num_points; i++)
					{
						if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
							< POINT_SELECT_DISTANCE)
						{
							window->active_layer->layer_data.vector_layer_p->active_point =
								&line->points[i];
							window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
							break;
						}
					}

					if(line->base_data.layer->pixels[layer_y*line->base_data.layer->stride+layer_x*4+3] != 0
						&& window->active_layer->layer_data.vector_layer_p->active_data == NULL)
					{
						window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
						break;
					}
				}
			}

			line = line->base_data.next;
		}
		break;
	case CONTROL_STROKE_MOVE:
		if((control->flags & CONTROL_POINT_TOOL_HAS_STROKE) == 0)
		{
			line = base->base_data.next;
			window->active_layer->layer_data.vector_layer_p->active_data = NULL;
			window->active_layer->layer_data.vector_layer_p->active_point = NULL;
			while(line != NULL)
			{
				if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
						&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
					{
						layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);
						
						for(i=0; i<line->num_points; i++)
						{
							if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
								< POINT_SELECT_DISTANCE)
							{
								window->active_layer->layer_data.vector_layer_p->active_point =
									&line->points[i];
								window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
								break;
							}
						}

						if(line->base_data.layer->pixels[layer_y*line->base_data.layer->stride+layer_x*4+3] != 0
							&& window->active_layer->layer_data.vector_layer_p->active_data == NULL)
						{
							window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
							(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
							cairo_set_source_surface(window->work_layer->cairo_p, line->base_data.layer->surface_p,
								line->base_data.layer->x, line->base_data.layer->y);
							cairo_paint(window->work_layer->cairo_p);
							break;
						}
					}
				}

				line = line->base_data.next;
			}
		}
		else
		{
			VECTOR_LINE* line = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;
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
	CONTROL_POINT_TOOL *control = (CONTROL_POINT_TOOL*)core->brush_data;
	VECTOR_LINE *base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
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
				AddControlPointMoveHistory(window, (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data,
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
				AddControlPointPressureHistory(window, (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data,
					window->active_layer->layer_data.vector_layer_p->active_point, control->change_point_data.pressure);
			}
			break;
		case CONTROL_STROKE_MOVE:
			if((control->flags & CONTROL_POINT_TOOL_HAS_STROKE) != 0)
			{
				window->active_layer->layer_data.vector_layer_p->flags |=
					VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
				AddStrokeMoveHistory(window, (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data,
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

	CONTROL_POINT_TOOL *control = (CONTROL_POINT_TOOL*)data;
	VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;
	VECTOR_LINE *active_line = (VECTOR_LINE*)layer->active_data;
	gdouble rev_zoom = window->rev_zoom;

	if(active_line != NULL && active_line->base_data.vector_type >= NUM_VECTOR_LINE_TYPE)
	{
		active_line = NULL;
	}

	cairo_save(window->disp_temp->cairo_p);
	ClipUpdateArea(window, window->disp_layer->cairo_p);
	ClipUpdateArea(window, window->disp_temp->cairo_p);

	cairo_set_operator(window->disp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	if(active_line != NULL && active_line->base_data.layer != NULL)
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
		cairo_rectangle(window->disp_layer->cairo_p, active_line->base_data.layer->x,
			active_line->base_data.layer->y, active_line->base_data.layer->width, active_line->base_data.layer->height);
		cairo_mask_surface(window->disp_layer->cairo_p, window->disp_temp->surface_p, 0, 0);
		cairo_scale(window->disp_temp->cairo_p, rev_zoom, rev_zoom);
		(void)memset(window->disp_temp->pixels, 0, window->disp_temp->stride*window->disp_temp->height);
		cairo_new_path(window->disp_layer->cairo_p);
	}

	while(line != NULL)
	{
		if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
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
		}

		line = line->base_data.next;
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int line_index = 0;
	int i;

	while(line != change_line)
	{
		line = line->base_data.next;
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int line_index = 0;
	int point_index;

	while(line != change_line)
	{
		line = line->base_data.next;
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
* LineColorChangeBrushPressCallBack�֐�              *
* ���C���̐F�ύX�c�[���̃N���b�N���̃R�[���o�b�N�֐� *
* ����                                               *
* window	: �`��̈�̏��                         *
* x			: �}�E�X��X���W                          *
* y			: �}�E�X��Y���W                          *
* pressure	: �M��                                   *
* core		: �x�N�g���u���V�̊�{���               *
* state		: �}�E�X�C�x���g�̏��                   *
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
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{	// ���C����Ȃ�F�ύX

		// ���C���F�ύX�c�[���̏ڍ׃f�[�^�ɃL���X�g
		CHANGE_LINE_COLOR_TOOL *tool = (CHANGE_LINE_COLOR_TOOL*)core->brush_data;
		// ��ԉ��̃��C��
		VECTOR_LINE *base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
		// �����蔻����ۂ����Ƃ����郉�C���ւ̃|�C���^
		VECTOR_LINE *line;
		// ���C�����C���[�̍��W
		int layer_x, layer_y;
		// �����蔻��̊J�n�E�I�����W
		int start_x, end_x, start_y, end_y;
		// ���[�v�I���̃t���O
		int end_flag = 0;
		// for���p�̃J�E���^
		int i, j;
		// �����蔻��̃t���O
		int hit;

		// ���C�����C���[�̃s�N�Z�����l��0�łȂ���Γ�����
		line = base->base_data.next;
		window->active_layer->layer_data.vector_layer_p->active_data = NULL;
		window->active_layer->layer_data.vector_layer_p->active_point = NULL;
		
		switch(tool->mode)
		{
		case CHANGE_LINE_COLOR_MODE:	// ���C���F�ύX
			while(line != NULL)
			{	// �e���C���[�����̃��C���ɑ΂��ď������s

				if(window->active_layer->layer_data.vector_layer_p->active_data == NULL)
				{
					if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
					{
						if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
							&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
						{
							layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->base_data.layer->width)
							{
								end_x = line->base_data.layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->base_data.layer->height)
							{
								end_y = line->base_data.layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->base_data.layer->pixels[i*line->base_data.layer->stride+j*4+3] != 0
										&& (memcmp(line->points->color, *core->color, 3) != 0 || line->points->color[3] != tool->flow))
									{
										window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
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
								window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;

								// �����f�[�^��ǉ�
								AddChangeLineColorHistory(window, line, change_color, core->name);

								// �F�ύX
								for(i=0; i<line->num_points; i++)
								{
									line->points[i].color[0] = (*core->color)[0];
									line->points[i].color[1] = (*core->color)[1];
									line->points[i].color[2] = (*core->color)[2];
									line->points[i].color[3] = tool->flow;
								}

								// ���C�����C���[�̍�蒼��
								window->active_layer->layer_data.vector_layer_p->flags |=
									VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

								break;
							}
						}

						line = line->base_data.next;
					}
				}
				else
				{
					uint8 change_color[4] = {(*core->color)[0], (*core->color)[1], (*core->color)[2], tool->flow};
					line = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;

					// �����f�[�^��ǉ�
					AddChangeLineColorHistory(window, line, change_color, core->name);

					// �F�ύX
					for(i=0; i<line->num_points; i++)
					{
						line->points[i].color[0] = (*core->color)[0];
						line->points[i].color[1] = (*core->color)[1];
						line->points[i].color[2] = (*core->color)[2];
						line->points[i].color[3] = tool->flow;
					}

					// ���C�����C���[�̍�蒼��
					window->active_layer->layer_data.vector_layer_p->flags |=
						VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
				}

				tool->flags &= ~(CONTROL_POINT_TOOL_HAS_LOCK);
			}
			break;
		case CHANGE_POINT_COLOR_MODE:	// ����_�F�ύX
			while(line != NULL && end_flag == 0)
			{	// �e���C���[�����̃��C���ɑ΂��ď������s
				if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
						&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
					{
						for(i=0; i<line->num_points; i++)
						{	// �}�E�X�J�[�\���̈ʒu�ɐ���_������ΐF�ύX
							if((x-line->points[i].x)*(x-line->points[i].x)+(y-line->points[i].y)*(y-line->points[i].y)
								<= POINT_SELECT_DISTANCE && memcmp(line->points[i].color, *core->color, 3) != 0)
							{
								uint8 change_color[4] = {(*core->color)[0], (*core->color)[1], (*core->color)[2], tool->flow};
								// �����f�[�^��ǉ�
								AddChangePointColorHistory(window, line, &line->points[i], change_color, core->name);

								line->points[i].color[0] = (*core->color)[0];
								line->points[i].color[1] = (*core->color)[1];
								line->points[i].color[2] = (*core->color)[2];
								line->points[i].color[3] = tool->flow;

								window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
								// ���C�����C���[�̍�蒼��
								window->active_layer->layer_data.vector_layer_p->flags |=
									VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

								end_flag++;

								break;
							}
						}
					}
				}

				line = line->base_data.next;
			}
			break;
		}
	}
	else if(((GdkEventButton*)state)->button == 3)
	{	// �E�N���b�N�Ȃ�
		// ���C���F�ύX�c�[���̏ڍ׃f�[�^�ɃL���X�g
		CHANGE_LINE_COLOR_TOOL* tool = (CHANGE_LINE_COLOR_TOOL*)core->brush_data;
		// ��ԉ��̃��C��
		VECTOR_LINE* base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
		// �����蔻����ۂ����Ƃ����郉�C���ւ̃|�C���^
		VECTOR_LINE* line;
		// 2�T�ڂ̃t���O
		int second_loop = 0;
		// ���C�����C���[�̍��W
		int layer_x, layer_y;
		// �����蔻��̊J�n�E�I�����W
		int start_x, end_x, start_y, end_y;
		// ���[�v�I���̃t���O
		int end_flag = 0;
		// for���p�̃J�E���^
		int i, j;
		// �����蔻��̃t���O
		int hit = 0;

		switch(tool->mode)
		{
		case CHANGE_LINE_COLOR_MODE:	// ���C���F�ύX
			if(window->active_layer->layer_data.vector_layer_p->active_data != NULL)
			{
				VECTOR_LINE *before_active = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;

				// ���C�����C���[�̃s�N�Z�����l��0�łȂ���Γ�����
				line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)window->active_layer->layer_data.vector_layer_p->active_data)->next;
				// �E�N���b�N�őI�����ꂽ�ꍇ�͈�苗���}�E�X���ړ�����܂őI�����C����ύX���Ȃ�
				tool->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
				// �}�E�X�̍��W���L��
				tool->lock_x = x, tool->lock_y = y;

				while(hit == 0)
				{	// �e���C���[�����̃��C���ɑ΂��ď������s
					if(line != NULL && line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
					{
						if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
							&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
						{
							layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->base_data.layer->width)
							{
								end_x = line->base_data.layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->base_data.layer->height)
							{
								end_y = line->base_data.layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->base_data.layer->pixels[i*line->base_data.layer->stride+j*4+3] != 0
										&& memcmp(line->points->color, *core->color, 3) != 0)
									{
										window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
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

						line = line->base_data.next;
					}
					else
					{
						if(second_loop == 0)
						{
							line = base->base_data.next;
							second_loop++;
						}
						else
						{
							break;
						}
					}
				}

				if(window->active_layer->layer_data.vector_layer_p->active_data == NULL)
				{
					window->active_layer->layer_data.vector_layer_p->active_data =
						(void*)before_active;
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
	// ���C���F�ύX�c�[���̏ڍ׃f�[�^�ɃL���X�g
	CHANGE_LINE_COLOR_TOOL* tool = (CHANGE_LINE_COLOR_TOOL*)core->brush_data;
	// ��ԉ��̃��C��
	VECTOR_LINE* base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
	// �����蔻����ۂ����Ƃ����郉�C���ւ̃|�C���^
	VECTOR_LINE* line;
	// ���[�v�I���̃t���O
	int end_flag = 0;
	// for���p�̃J�E���^
	int i;

	// �F�ύX�������ł���Β��f
	if((window->active_layer->layer_data.vector_layer_p->flags & VECTOR_LAYER_FIX_LINE) != 0)
	{
		return;
	}

	// ����_����̋��������ȉ��Ȃ瓖����
	line = base->base_data.next;
	window->active_layer->layer_data.vector_layer_p->active_data = NULL;
	window->active_layer->layer_data.vector_layer_p->active_point = NULL;

	if(tool->mode == CHANGE_LINE_COLOR_MODE)
	{
		FLOAT_T distance = sqrt((x-tool->lock_x)*(x-tool->lock_x)+(y-tool->lock_y)*(y-tool->lock_y));
		// ���C�����C���[�̍��W
		int layer_x, layer_y;
		// �����蔻��̊J�n�E�I�����W
		int start_x, end_x, start_y, end_y;
		// �s�N�Z���̃C���f�b�N�X
		int index;
		// for���p�̃J�E���^
		int i, j;
		// �����蔻��̃t���O
		int hit;

		if((tool->flags & CONTROL_POINT_TOOL_HAS_LOCK) == 0
			|| distance >= CHANGE_LINE_DISTANCE)
		{
			tool->flags &= ~(CONTROL_POINT_TOOL_HAS_LOCK);

			while(line != NULL && end_flag == 0)
			{
				// �e���C���[�����̃��C���ɑ΂��ď������s
				if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
						&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
					{
						layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

						start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
						end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

						if(start_x < 0)
						{
							start_x = 0;
						}
						else if(end_x >= line->base_data.layer->width)
						{
							end_x = line->base_data.layer->width - 1;
						}

						if(start_y < 0)
						{
							start_y = 0;
						}
						else if(end_y >= line->base_data.layer->height)
						{
							end_y = line->base_data.layer->height - 1;
						}

						hit = 0, j = 0;
						for(i=start_y; i<end_y && j>=0; i++)
						{
							for(j=start_x; j<end_x; j++)
							{
								index = i*line->base_data.layer->stride+j*4+3;
								if(index < line->base_data.layer->stride*line->base_data.layer->height)
								{
									if(line->base_data.layer->pixels[index] != 0
										&& (memcmp(line->points->color, *core->color, 3) != 0 || line->points->color[3] != tool->flow))
									{
										window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
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
				}	// �e���C���[�����̃��C���ɑ΂��ď������s
					// if(line->layer != NULL)
				line = line->base_data.next;
			}	// while(line != NULL && end_flag == 0)
		}
	}
	else if(tool->mode == CHANGE_POINT_COLOR_MODE)
	{
		while(line != NULL && end_flag == 0)
		{	// �e���C���[�����̃��C���ɑ΂��ď������s
			if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
			{
				if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
					&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
				{
					for(i=0; i<line->num_points; i++)
					{	// �}�E�X�J�[�\���̈ʒu�ɐ���_������΋���
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

			line = line->base_data.next;
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
	CHANGE_LINE_COLOR_TOOL *color = (CHANGE_LINE_COLOR_TOOL*)data;

	cairo_save(window->disp_temp->cairo_p);
	ClipUpdateArea(window, window->disp_layer->cairo_p);
	ClipUpdateArea(window, window->disp_temp->cairo_p);

	if(color->mode == CHANGE_LINE_COLOR_MODE)
	{
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;
		VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;
		VECTOR_LINE *active_line = (VECTOR_LINE*)layer->active_data;
		gdouble rev_zoom = 1.0 / (window->zoom * 0.01);

		if(active_line != NULL && active_line->base_data.vector_type >= NUM_VECTOR_LINE_TYPE)
		{
			active_line = NULL;
		}

		if(active_line != NULL)
		{	
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
			cairo_set_source_rgb(window->disp_layer->cairo_p, 1, 0, 0);
#else
			cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 0, 1);
#endif
			cairo_scale(window->disp_temp->cairo_p, window->zoom*0.01, window->zoom*0.01);
			cairo_set_source_surface(window->disp_temp->cairo_p,
				active_line->base_data.layer->surface_p, active_line->base_data.layer->x, active_line->base_data.layer->y);
			cairo_paint(window->disp_temp->cairo_p);
			cairo_rectangle(window->disp_layer->cairo_p, active_line->base_data.layer->x,
				active_line->base_data.layer->y, active_line->base_data.layer->width, active_line->base_data.layer->height);
			cairo_mask_surface(window->disp_layer->cairo_p, window->disp_temp->surface_p, 0, 0);
			cairo_scale(window->disp_temp->cairo_p, rev_zoom, rev_zoom);
			(void)memset(window->disp_temp->pixels, 0, window->disp_temp->stride*window->disp_temp->height);
			cairo_new_path(window->disp_layer->cairo_p);
		}
	}
	else if(color->mode == CHANGE_POINT_COLOR_MODE)
	{
		unsigned int i;

		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;
		VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;
		gdouble rev_zoom = 1.0 / (window->zoom * 0.01);

		while(line != NULL)
		{
			if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
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
			}

			line = line->base_data.next;
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
	// �c�[���̏ڍ׃f�[�^�ɃL���X�g
	CHANGE_LINE_COLOR_TOOL *color = (CHANGE_LINE_COLOR_TOOL*)data;
	// �ڍאݒ�E�B�W�F�b�g������p�b�L���O�{�b�N�X
	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	// �X���C�_
	GtkWidget *scale;
	// �E�B�W�F�b�g����p�̃e�[�u��
	GtkWidget *table;
	// ���x��
	GtkWidget *label;
	// ���W�I�{�^��
	GtkWidget *radio_buttons[2];
	// �X���C�_�ɐݒ肷��A�W���X�^
	GtkAdjustment *scale_adjustment;
	// ���x���̃t�H���g�T�C�Y�ύX�p�̃}�[�N�A�b�v�o�b�t�@
	char mark_up_buff[256];

	// �Z�x�ύX�E�B�W�F�b�g
	table = gtk_table_new(1, 3, FALSE);
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		color->flow * DIV_PIXEL * 100, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeLineColorFlow), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.flow, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �F�ύX�Ώی���p���W�I�{�^��
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
	char *name = &(((char*)history_data)[offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name)]);
	VECTOR_LINE *line;
	uint8 *buff = (uint8*)history_data;
	gdouble *before_size, temp_size;
	uint8 *before_flow, temp_flow;
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->line_index; i++)
	{
		line = line->base_data.next;
	}
	layer->layer_data.vector_layer_p->active_data = (void*)line;
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
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	uint8 *buff = (uint8*)history_data;
	uint8 *before_flow;
	gdouble *before_size;
	int line_index = 0;
	int i;

	before_flow = &buff[offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name) + layer_name_length + 1];
	before_size = (gdouble*)&buff[offsetof(CHANGE_LINE_SIZE_HISTORY, layer_name) + layer_name_length + 1 + line->num_points];

	while(line != change_line)
	{
		line = line->base_data.next;
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
* LineSizeChangeBrushPressCallBack�֐�                 *
* ���C���̎�ޕύX�c�[���̃N���b�N���̃R�[���o�b�N�֐� *
* ����                                                 *
* window	: �`��̈�̏��                           *
* x			: �}�E�X��X���W                            *
* y			: �}�E�X��Y���W                            *
* pressure	: �M��                                     *
* core		: �x�N�g���u���V�̊�{���                 *
* state		: �}�E�X�C�x���g�̏��                     *
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
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{	// ���C����Ȃ�T�C�Y�ύX

		// �ݒ肷�郉�C���̃T�C�Y���
		CHANGE_LINE_SIZE_TOOL *size = (CHANGE_LINE_SIZE_TOOL*)core->brush_data;
		// ��ԉ��̃��C��
		VECTOR_LINE *base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
		// �����蔻����ۂ����Ƃ����郉�C���ւ̃|�C���^
		VECTOR_LINE *line;
		// ���C�����C���[�̍��W
		int layer_x, layer_y;
		// �����蔻��̊J�n�E�I�����W
		int start_x, end_x, start_y, end_y;
		// �����蔻��̃t���O
		int hit;
		// for���p�̃J�E���^
		int i, j;

		// ���C�����C���[�̃s�N�Z�����l��0�łȂ���Γ�����
		if(window->active_layer->layer_data.vector_layer_p->active_data == NULL)
		{
			line = base->base_data.next;
			window->active_layer->layer_data.vector_layer_p->active_data = NULL;
			window->active_layer->layer_data.vector_layer_p->active_point = NULL;

			while(line != NULL)
			{	// �e���C���[�����̃��C���ɑ΂��ď������s
				if(window->active_layer->layer_data.vector_layer_p->active_data == NULL)
				{
					if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
					{
						if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
							&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
						{
							layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

							start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
							end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(end_x >= line->base_data.layer->width)
							{
								end_x = line->base_data.layer->width - 1;
							}

							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(end_y >= line->base_data.layer->height)
							{
								end_y = line->base_data.layer->height - 1;
							}

							hit = 0, j = 0;
							for(i=start_y; i<end_y && j>=0; i++)
							{
								for(j=start_x; j<end_x; j++)
								{
									if(line->base_data.layer->pixels[i*line->base_data.layer->stride+j*4+3] != 0)
									{
										window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
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
								window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;

								// �����f�[�^��ǉ�
								AddChangeLineSizeHistory(window, line, core->name);

								// �T�C�Y�ύX
								line->blur = size->blur;
								line->outline_hardness = size->outline_hardness;
								for(i=0; i<line->num_points; i++)
								{
									line->points[i].size = size->r;
									line->points[i].color[3] = size->flow;
								}

								// ���C�����C���[�̍�蒼��
								window->active_layer->layer_data.vector_layer_p->flags |=
									VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
							}

							break;
						}
					}

					line = line->base_data.next;
				}
				else
				{
					// �����f�[�^��ǉ�
					line = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;
					AddChangeLineSizeHistory(window, line, core->name);

					// �T�C�Y�ύX
					line->blur = size->blur;
					line->outline_hardness = size->outline_hardness;
					for(i=0; i<line->num_points; i++)
					{
						line->points[i].size = size->r;
						line->points[i].color[3] = size->flow;
					}

					// ���C�����C���[�̍�蒼��
					window->active_layer->layer_data.vector_layer_p->flags |=
						VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
				}
			}
		}
		else
		{
			line = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;

			// �����f�[�^��ǉ�
			AddChangeLineSizeHistory(window, line, core->name);

			// �T�C�Y�ύX
			line->blur = size->blur;
			line->outline_hardness = size->outline_hardness;
			for(i=0; i<line->num_points; i++)
			{
				line->points[i].size = size->r;
				line->points[i].color[3] = size->flow;
			}

			// ���C�����C���[�̍�蒼��
			window->active_layer->layer_data.vector_layer_p->flags |=
				VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;
		}

		size->flags &= ~(CONTROL_POINT_TOOL_HAS_LOCK);
	}
	else if(((GdkEventButton*)state)->button == 3)
	{	// �E�N���b�N�Ȃ�
		// ���C���F�ύX�c�[���̏ڍ׃f�[�^�ɃL���X�g
		CHANGE_LINE_SIZE_TOOL *size = (CHANGE_LINE_SIZE_TOOL*)core->brush_data;
		// ��ԉ��̃��C��
		VECTOR_LINE *base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
		// �����蔻����ۂ����Ƃ����郉�C���ւ̃|�C���^
		VECTOR_LINE *line;
		// 2�T�ڂ̃t���O
		int second_loop = 0;
		// ���C�����C���[�̍��W
		int layer_x, layer_y;
		// �����蔻��̊J�n�E�I�����W
		int start_x, end_x, start_y, end_y;
		// ���[�v�I���̃t���O
		int end_flag = 0;
		// for���p�̃J�E���^
		int i, j;
		// �����蔻��̃t���O
		int hit = 0;

		if(window->active_layer->layer_data.vector_layer_p->active_data != NULL)
		{
			VECTOR_LINE *before_active = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->active_data;

			// ���C�����C���[�̃s�N�Z�����l��0�łȂ���Γ�����
			line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)window->active_layer->layer_data.vector_layer_p->active_data)->next;
			// �E�N���b�N�őI�����ꂽ�ꍇ�͈�苗���}�E�X���ړ�����܂őI�����C����ύX���Ȃ�
			size->flags |= CONTROL_POINT_TOOL_HAS_LOCK;
			// �}�E�X�̍��W���L��
			size->lock_x = x, size->lock_y = y;

			while(hit == 0)
			{	// �e���C���[�����̃��C���ɑ΂��ď������s
				if(line != NULL && line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
						&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
					{
						layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

						start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
						end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

						if(start_x < 0)
						{
							start_x = 0;
						}
						else if(end_x >= line->base_data.layer->width)
						{
							end_x = line->base_data.layer->width - 1;
						}

						if(start_y < 0)
						{
							start_y = 0;
						}
						else if(end_y >= line->base_data.layer->height)
						{
							end_y = line->base_data.layer->height - 1;
						}

						hit = 0, j = 0;
						for(i=start_y; i<end_y && j>=0; i++)
						{
							for(j=start_x; j<end_x; j++)
							{
								if(line->base_data.layer->pixels[i*line->base_data.layer->stride+j*4+3] != 0
									&& memcmp(line->points->color, *core->color, 3) != 0)
								{
									window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
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

					line = line->base_data.next;
				}
				else
				{
					if(second_loop == 0)
					{
						line = base->base_data.next;
						second_loop++;
					}
					else
					{
						break;
					}
				}
			}

			if(window->active_layer->layer_data.vector_layer_p->active_data == NULL)
			{
				window->active_layer->layer_data.vector_layer_p->active_data =
					(void*)before_active;
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
	// ���C���F�ύX�c�[���̏ڍ׃f�[�^�ɃL���X�g
	CHANGE_LINE_SIZE_TOOL *size = (CHANGE_LINE_SIZE_TOOL*)core->brush_data;
	// ��ԉ��̃��C��
	VECTOR_LINE *base = (VECTOR_LINE*)window->active_layer->layer_data.vector_layer_p->base;
	// �����蔻����ۂ����Ƃ����郉�C���ւ̃|�C���^
	VECTOR_LINE *line;
	// ���[�v�I���̃t���O
	int end_flag = 0;

	// �F�ύX�������ł���Β��f
	if((window->active_layer->layer_data.vector_layer_p->flags & VECTOR_LAYER_FIX_LINE) != 0)
	{
		return;
	}

	// ����_����̋��������ȉ��Ȃ瓖����
	line = base->base_data.next;
	window->active_layer->layer_data.vector_layer_p->active_data = NULL;
	window->active_layer->layer_data.vector_layer_p->active_point = NULL;

	{
		FLOAT_T distance = sqrt((x-size->lock_x)*(x-size->lock_x)+(y-size->lock_y)*(y-size->lock_y));
		// ���C�����C���[�̍��W
		int layer_x, layer_y;
		// �����蔻��̊J�n�E�I�����W
		int start_x, end_x, start_y, end_y;
		// for���p�̃J�E���^
		int i, j;
		// �����蔻��̃t���O
		int hit;

		if((size->flags & CONTROL_POINT_TOOL_HAS_LOCK) == 0
			|| distance >= CHANGE_LINE_DISTANCE)
		{
			size->flags &= ~(CONTROL_POINT_TOOL_HAS_LOCK);

			while(line != NULL && end_flag == 0)
			{
				// �e���C���[�����̃��C���ɑ΂��ď������s
				if(line->base_data.layer != NULL && line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					if(x > line->base_data.layer->x && x < line->base_data.layer->x+line->base_data.layer->width
						&& y > line->base_data.layer->y && y < line->base_data.layer->y+line->base_data.layer->height)
					{
						layer_x = (int)(x - line->base_data.layer->x), layer_y = (int)(y - line->base_data.layer->y);

						start_x = layer_x - STROKE_SELECT_DISTANCE, start_y = layer_y - STROKE_SELECT_DISTANCE;
						end_x = layer_x + STROKE_SELECT_DISTANCE, end_y = layer_y + STROKE_SELECT_DISTANCE;

						if(start_x < 0)
						{
							start_x = 0;
						}
						else if(end_x >= line->base_data.layer->width)
						{
							end_x = line->base_data.layer->width - 1;
						}

						if(start_y < 0)
						{
							start_y = 0;
						}
						if(end_y + start_y >= line->base_data.layer->height)
						{
							end_y = line->base_data.layer->height - start_y - 1;
						}

						hit = 0, j = 0;
						for(i=start_y; i<end_y && j>=0; i++)
						{
							for(j=start_x; j<end_x; j++)
							{
								if(line->base_data.layer->pixels[i*line->base_data.layer->stride+j*4+3] != 0)
								{
									window->active_layer->layer_data.vector_layer_p->active_data = (void*)line;
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
				}	// �e���C���[�����̃��C���ɑ΂��ď������s
					// if(line->layer != NULL)
				line = line->base_data.next;
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
	VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;
	VECTOR_LINE *active_line = (VECTOR_LINE*)layer->active_data;
	gdouble rev_zoom = 1.0 / (window->zoom * 0.01);

	if(active_line != NULL && active_line->base_data.vector_type >= NUM_VECTOR_LINE_TYPE)
	{
		active_line = NULL;
	}

	if(active_line != NULL)
	{	
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		cairo_set_source_rgb(window->disp_layer->cairo_p, 1, 0, 0);
#else
		cairo_set_source_rgb(window->disp_layer->cairo_p, 0, 0, 1);
#endif
		cairo_scale(window->disp_temp->cairo_p, window->zoom*0.01, window->zoom*0.01);
		cairo_set_source_surface(window->disp_temp->cairo_p,
			active_line->base_data.layer->surface_p, active_line->base_data.layer->x, active_line->base_data.layer->y);
		cairo_paint(window->disp_temp->cairo_p);
		cairo_rectangle(window->disp_layer->cairo_p, active_line->base_data.layer->x,
			active_line->base_data.layer->y, active_line->base_data.layer->width, active_line->base_data.layer->height);
		cairo_mask_surface(window->disp_layer->cairo_p, window->disp_temp->surface_p, 0, 0);
		cairo_scale(window->disp_temp->cairo_p, rev_zoom, rev_zoom);
		(void)memset(window->disp_temp->pixels, 0, window->disp_temp->stride*window->disp_temp->height);
		cairo_new_path(window->disp_layer->cairo_p);
	}
}

/*****************************************************************
* ChangeLineSizeScale�֐�                                        *
* ���C���T�C�Y�ύX�c�[���̃T�C�Y�X���C�_���쎞�̃R�[���o�b�N�֐� *
* ����                                                           *
* slider	: �X���C�_�̃A�W���X�^                               *
* size		: ���C���T�C�Y�ύX�c�[���̃f�[�^                     *
*****************************************************************/
static void ChangeLineSizeScale(
	GtkAdjustment* slider,
	CHANGE_LINE_SIZE_TOOL* size
)
{
	size->r = gtk_adjustment_get_value(slider)*0.5;
}

/*****************************************************************
* ChangeLineSizeBlur�֐�                                         *
* ���C���T�C�Y�ύX�c�[���̃{�P���X���C�_���쎞�̃R�[���o�b�N�֐� *
* ����                                                           *
* slider	: �X���C�_�̃A�W���X�^                               *
* size		: ���C���T�C�Y�ύX�c�[���̃f�[�^                     *
*****************************************************************/
static void ChangeLineSizeBlur(
	GtkAdjustment* slider,
	CHANGE_LINE_SIZE_TOOL* size
)
{
	size->blur = gtk_adjustment_get_value(slider);
}

/*********************************************************************
* ChangeLineSizeOutlineHarndess�֐�                                  *
* ���C���T�C�Y�ύX�c�[���̗֊s�̍d���X���C�_���쎞�̃R�[���o�b�N�֐� *
* ����                                                               *
* slider	: �X���C�_�̃A�W���X�^                                   *
* size		: ���C���T�C�Y�ύX�c�[���̃f�[�^                         *
*********************************************************************/
static void ChangeLineSizeOutlineHardness(
	GtkAdjustment* slider,
	CHANGE_LINE_SIZE_TOOL* size
)
{
	size->outline_hardness = gtk_adjustment_get_value(slider);
}

/***************************************************************
* ChangeLineSizeFlow�֐�                                       *
* ���C���T�C�Y�ύX�c�[���̔Z�x�X���C�_���쎞�̃R�[���o�b�N�֐� *
* ����                                                         *
* slider	: �X���C�_�̃A�W���X�^                             *
* size		: ���C���T�C�Y�ύX�c�[���̃f�[�^                   *
***************************************************************/
static void ChangeLineSizeFlow(
	GtkAdjustment* slider,
	CHANGE_LINE_SIZE_TOOL* size
)
{
	size->flow = (uint8)(gtk_adjustment_get_value(slider) * 2.55 + 0.5);
}

/*****************************************************
* CreateChangeLineSizeDetailUI�֐�                   *
* ���C���T�C�Y�ύX�c�[���̏ڍ�UI�𐶐�               *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* data	: �c�[���̏ڍ׃f�[�^                         *
*****************************************************/
static GtkWidget* CreateChangeLineSizeDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	// �ڍ׃f�[�^���L���X�g
	CHANGE_LINE_SIZE_TOOL *size = (CHANGE_LINE_SIZE_TOOL*)data;
	// �E�B�W�F�b�g����ׂ�p�b�L���O�{�b�N�X
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	// �X���C�_
	GtkWidget *scale;
	// �E�B�W�F�b�g���ׂ�e�[�u��
	GtkWidget *table;
	// �X���C�_�ɓK�p����A�W���X�^
	GtkAdjustment *scale_adjustment;

	// �T�C�Y�ύX�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(size->r*2, 1, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeLineSizeScale), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.brush_scale, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �{�P���ύX�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(size->blur, 0, 255, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeLineSizeBlur), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.blur, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �֊s�̍d���ύX�E�B�W�F�b�g
	table = gtk_table_new(1, 3, FALSE);
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(size->outline_hardness, 0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeLineSizeOutlineHardness), data);
	scale = SpinScaleNew(GTK_ADJUSTMENT(scale_adjustment),
		app->labels->tool_box.outline_hardness, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �Z�x�ύX�E�B�W�F�b�g
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
* VectorEraserPressCallBack�֐�                                *
* �x�N�g�������S���c�[���ł̃}�E�X�N���b�N���̃R�[���o�b�N�֐� *
* ����                                                         *
* window	: �`��̈�̏��                                   *
* x			: �}�E�X��X���W                                    *
* y			: �}�E�X��Y���W                                    *
* pressure	: �M��                                             *
* core		: �x�N�g���u���V�̊�{���                         *
* state		: �}�E�X�C�x���g�̏��                             *
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
	// ���N���b�N�Ȃ�
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
* VectorEraserMotionCallBack�֐�                               *
* �x�N�g�������S���c�[���ł̃}�E�X�h���b�O���̃R�[���o�b�N�֐� *
* ����                                                         *
* window	: �`��̈�̏��                                   *
* x			: �}�E�X��X���W                                    *
* y			: �}�E�X��Y���W                                    *
* pressure	: �M��                                             *
* core		: �x�N�g���u���V�̊�{���                         *
* state		: �}�E�X�C�x���g�̏��                             *
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
* VectorEraserReleaseCallBack�֐�                                        *
* �x�N�g�������S���c�[���ł̃}�E�X�̃{�^���������ꂽ���̃R�[���o�b�N�֐� *
* ����                                                                   *
* window	: �`��̈�̏��                                             *
* x			: �}�E�X��X���W                                              *
* y			: �}�E�X��Y���W                                              *
* pressure	: �M��                                                       *
* core		: �x�N�g���u���V�̊�{���                                   *
* state		: �}�E�X�C�x���g�̏��                                       *
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
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �x�N�g�������S���̏ڍ׃f�[�^�ɃL���X�g
		VECTOR_ERASER* eraser = (VECTOR_ERASER*)core->brush_data;
		// ����p�̃f�[�^
		MEMORY_STREAM_PTR divide_change = NULL, divide_add = NULL;
		MEMORY_STREAM_PTR src_points_stream;
		DIVIDE_LINE_CHANGE_DATA divide_line;
		DIVIDE_LINE_ADD_DATA add_line;
		int num_divide = 0;
		// �x�W�F�Ȑ��̐���_
		BEZIER_POINT bezier_points[4], controls[4];
		// �ύX�O�̐���_
		VECTOR_POINT *src_points;
		// �x�W�F�Ȑ��̍��W
		BEZIER_POINT check_point;
		// �x�N�g�����C���[���̃��C��
		VECTOR_LINE *line =
			(VECTOR_LINE*)((VECTOR_BASE_DATA*)window->active_layer->layer_data.vector_layer_p->base)->next;
		// �����ɂ��V���ɍ쐬���ꂽ���C��
		VECTOR_LINE *new_line = NULL;
		// 1�̃��C���ɑ΂��Ēǉ����ꂽ���C���̐�
		int this_add;
		// �ύX�̂�����̃f�[�^
		VECTOR_LINE *change_lines = (VECTOR_LINE*)MEM_ALLOC_FUNC(
			sizeof(*change_lines)*VECTOR_ERASER_BUFFER_SIZE);
		// �ύX�̂�����̐�
		uint32 num_change_line = 0;
		// �ύX�̂�����L���p�̃o�b�t�@�T�C�Y
		uint32 buffer_size = VECTOR_ERASER_BUFFER_SIZE;
		// �������̐��̃C���f�b�N�X
		uint32 line_index = 0;
		// �ύX������̃C���f�b�N�X
		uint32 *change_line_indexes = NULL;
		// ����_�Ԃ̋���
		gdouble d, dx, dy;
		// ����_�Ԃ̊p�x
		gdouble arg;
		// �x�W�F�Ȑ��Ŏg�p����t
		gdouble t;
		// �`�F�b�N����X���W�AY���W
		gdouble check_x, check_y;
		// �����O�̐���_�̐�
		int before_num_points;
		// �����O�̐���_
		VECTOR_POINT before_point;
		// �����O�̔z��̃C���f�b�N�X
		int before_index;
		// ���W�̐����l
		int int_x, int_y;
		// cos�Asin�̒l
		gdouble cos_x, sin_y;
		// �s�N�Z���̃C���f�b�N�X
		int index;
		// ���[�v�I���̃t���O
		int end_flag;
		// ���C�������̃t���O
		int in_devide;
		// for���p�̃J�E���^
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

		// �������̈�f�[�^���R�s�[
		for(i=0; i<window->width*window->height; i++)
		{
			window->brush_buffer[i] = window->work_layer->pixels[i*4+3];
		}

		// ���C���[���̑S�Ẵ��C�����`�F�b�N
		while(line != NULL)
		{
			// �̈悪���C���Əd�Ȃ邩���`�F�b�N
			if(line->base_data.layer != NULL && line->base_data.layer->x <= eraser->max_x
				&& line->base_data.layer->x + line->base_data.layer->width >= eraser->min_x
				&& line->base_data.layer->y <= eraser->max_y
				&& line->base_data.layer->y + line->base_data.layer->height >= eraser->min_y
				&& line->base_data.vector_type < NUM_VECTOR_LINE_TYPE
			)
			{
				// �x�N�g�������S���̃��[�h�ŏ����؂�ւ�
				switch(eraser->mode)
				{
				case VECTOR_ERASER_STROKE_DIVIDE:
					// ���C�����X�g���[�N���ď������̈�Əd�Ȃ����烉�C������
					in_devide = 0;
					end_flag = 0;
					this_add = 0;
					before_num_points = line->num_points;
					// ���݂̐���_�����R�s�[
					src_points_stream->data_point = 0;
					(void)MemWrite(line->points, 1,
						sizeof(*line->points)*line->num_points, src_points_stream);
					src_points = (VECTOR_POINT*)src_points_stream->buff_ptr;

					if(line->base_data.vector_type == VECTOR_TYPE_STRAIGHT
						|| line->num_points == 2)
					{	// ����
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

								// �������̈�Əd�Ȃ����̂Ń��C���핪��
								if(index >= 0 && index < window->height * window->width)
								{
									if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8 && in_devide % 2 == 0)
									{
										if(in_devide == 0)
										{
											num_divide++;

											// �ύX�O�̃��C�����𗚗��f�[�^�Ƃ��Ďc��
											divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
												+ sizeof(VECTOR_POINT)*(line->num_points + i + 2) - sizeof(divide_line.data_size);
											divide_line.added = 0;
											divide_line.before_num_points = line->num_points;
											divide_line.after_num_points = i + 2;
											divide_line.index = line_index;
											divide_line.line_type = line->base_data.vector_type;
											(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
											(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

											// �I�[�ύX�O�̐���_�����L��
											before_point = line->points[i+1];

											// ���݂̍��W���I�[�ɂ���
											line->points[i+1].x = check_x, line->points[i+1].y = check_y;
											line->num_points = i + 2;

											(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

											// ���C�������X�^���C�Y����t���O�𗧂Ă�
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
											add_line.line_type = new_line->base_data.vector_type;
											add_line.num_points = new_line->num_points;
											(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
											(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

											num_change_line++;
											new_line = NULL;
										}

										// ���C���������̃t���O�𗧂Ă�
										in_devide++;
									}
									else if(window->brush_buffer[int_y*window->width+int_x] < 0x08 && in_devide % 2 != 0)
									{
										// ���݂̍��W���n�_�ɂ��ĐV���ȃ��C���쐬
										new_line = CreateVectorLine(line, (VECTOR_LINE*)line->base_data.next);
										new_line->base_data.vector_type = line->base_data.vector_type;
										new_line->num_points = before_num_points - i;
										new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
											* VECTOR_LINE_BUFFER_SIZE;
										// ����_�̐ݒ�
										new_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*new_line->points)*new_line->buff_size);
										(void)memset(new_line->points, 0, sizeof(*new_line->points)*new_line->buff_size);
										new_line->points->x = check_x, new_line->points->y = check_y;
										new_line->points->pressure = src_points[i].pressure;
										new_line->points->size = src_points[i].size;
										(void)memcpy(new_line->points->color, src_points[i].color, 4);
										new_line->base_data.vector_type = line->base_data.vector_type;
										new_line->points[1] = before_point;
										this_add++;

										if(new_line->num_points > 2)
										{
											(void)memcpy(&new_line->points[2], &src_points[i+2],
												sizeof(*new_line->points)*(new_line->num_points-2));
										}
										// ���C�������X�^���C�Y����t���O�𗧂Ă�
										new_line->flags |= VECTOR_LINE_FIX;

										// ���݂̐���_�����L��
										end_flag = i;

										// ���C�������I��
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
								add_line.line_type = new_line->base_data.vector_type;
								add_line.num_points = new_line->num_points;
								(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
								(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

								new_line = NULL;
							}
						}
					}
					else if(0)//line->vector_type == VECTOR_TYPE_BEZIER_OPEN && line->num_points == 3)
					{	// �x�W�F�Ȑ�
						dx = src_points[0].x-src_points[1].x;
						dy = src_points[0].y-src_points[1].y;
						d = sqrt(dx*dx+dy*dy);
						dx = src_points[1].x-src_points[2].x;
						dy = src_points[1].y-src_points[2].y;
						d += sqrt(dx*dx+dy*dy);

						// ����_���W�v�Z�̏���
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = src_points[j].x;
							bezier_points[j].y = src_points[j].y;
						}

						// ����_���W�v�Z
						MakeBezier3EdgeControlPoint(bezier_points, controls);
						bezier_points[1].x = src_points[0].x,	bezier_points[1].y = src_points[0].y;
						bezier_points[2] = controls[0];
						bezier_points[3].x = src_points[1].x,	bezier_points[3].y = src_points[1].y;

						// 3���x�W�F�Ȑ��������ď������̈�Əd�Ȃ邩�𒲂ׂ�
						d = 1 / (d * 1.5);
						t = 0;
						while(t < 1)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;
							index = int_y*window->width+int_x;

							// �������̈�Əd�Ȃ����̂Ń��C������
							if(index >= 0 && index < window->width * window->height)
							{
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8 && in_devide % 2 == 0)
								{
									int point_index = (int)(t + 1.5);
									before_point = line->points[point_index];
									if(in_devide == 0)
									{
										num_divide++;

										// �ύX�O�̃��C�����𗚗��f�[�^�Ƃ��Ďc��
										divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
											+ sizeof(VECTOR_POINT)*(line->num_points+point_index+1)  - sizeof(divide_line.data_size);
										divide_line.added = 0;
										divide_line.before_num_points = line->num_points;
										divide_line.after_num_points = i + 2;
										divide_line.index = line_index;
										divide_line.line_type = line->base_data.vector_type;
										(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										line->points[point_index].x = check_point.x;
										line->points[point_index].y = check_point.y;
										line->num_points = point_index+1;

										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										// ���C�������X�^���C�Y����t���O�𗧂Ă�
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
										add_line.line_type = new_line->base_data.vector_type;
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
									// ���݂̍��W���n�_�ɂ��ĐV���ȃ��C���쐬
									int point_index = (int)(t + 1.5);
									new_line = CreateVectorLine(line, (VECTOR_LINE*)line->base_data.next);
									new_line->base_data.vector_type = line->base_data.vector_type;
									new_line->num_points = 4 - point_index;
									new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
										* VECTOR_LINE_BUFFER_SIZE;
									// ����_�̐ݒ�
									new_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*new_line->points)*new_line->buff_size);
									(void)memset(new_line->points, 0, sizeof(*new_line->points)*new_line->buff_size);
									new_line->points->x = check_point.x, new_line->points->y = check_point.y;
									new_line->points->pressure = line->points[point_index].pressure;
									new_line->points->size = line->points[point_index].size;
									new_line->points->vector_type = line->points[point_index].vector_type;
									(void)memcpy(&new_line->points[1], &src_points[i], sizeof(*src_points)*(before_num_points-i-1));
									(void)memcpy(new_line->points->color, line->points[point_index].color, 4);
									this_add++;

									// ���C�������X�^���C�Y����t���O�𗧂Ă�
									new_line->flags |= VECTOR_LINE_FIX;

									// ���݂̐���_�����L��
									end_flag = i;

									// �����I��
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
							add_line.line_type = new_line->base_data.vector_type;
							add_line.num_points = new_line->num_points;
							(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
							(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

							new_line = NULL;
						}
					}
					else if(line->base_data.vector_type == VECTOR_TYPE_BEZIER_OPEN)
					{
						// �b��I�ȋ����v�Z
						dx = src_points[0].x-src_points[1].x;
						dy = src_points[0].y-src_points[1].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						// ����_���W�v�Z�̏���
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = src_points[j].x;
							bezier_points[j].y = src_points[j].y;
						}

						// ����_���W�v�Z
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

							// �������̈�Əd�Ȃ����̂Ń��C������
							if(index >= 0 && index < window->width * window->height)
							{
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8
									&& in_devide % 2 == 0)
								{
									if(in_devide == 0)
									{
										num_divide++;

										// �ύX�O�̃��C�����𗚗��f�[�^�Ƃ��Ďc��
										divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
											+ sizeof(VECTOR_POINT)*(line->num_points + 2) - sizeof(divide_line.data_size);
										divide_line.added = 0;
										divide_line.before_num_points = line->num_points;
										divide_line.after_num_points = 2;
										divide_line.index = line_index;
										divide_line.line_type = line->base_data.vector_type;
										(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										before_point = line->points[1];
										line->points[1].x = check_point.x;
										line->points[1].y = check_point.y;
										line->num_points = 2;

										(void)MemWrite(line->points, 1, sizeof(*line->points)*2, divide_change);

										// ���C�������X�^���C�Y����t���O�𗧂Ă�
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
										add_line.line_type = new_line->base_data.vector_type;
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
									// ���݂̍��W���n�_�ɂ��ĐV���ȃ��C���쐬
									new_line = CreateVectorLine(line, (VECTOR_LINE*)line->base_data.next);
									new_line->base_data.vector_type = line->base_data.vector_type;
									new_line->num_points = before_num_points;
									new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
										* VECTOR_LINE_BUFFER_SIZE;
									// ����_�̐ݒ�
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

									// ���C�������X�^���C�Y����t���O�𗧂Ă�
									new_line->flags |= VECTOR_LINE_FIX;

									end_flag = i;

									in_devide++;
								}
							}

							t += d;
						}

						for(i=0; i<line->num_points-3; i++)
						{
							// 3���x�W�F�Ȑ�
								// ����_���W�v�Z�̏���
							for(j=0; j<4; j++)
							{
								bezier_points[j].x = src_points[i+j].x;
								bezier_points[j].y = src_points[i+j].y;
							}

							// ����_���W�v�Z
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

								// �������̈�Əd�Ȃ����̂Ń��C������
								if(index >= 0 && index < window->width * window->height)
								{
									if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8 && in_devide % 2 == 0)
									{
										if(in_devide == 0)
										{
											num_divide++;

											// �ύX�O�̃��C�����𗚗��f�[�^�Ƃ��Ďc��
											divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
												+ sizeof(VECTOR_POINT)*(line->num_points + i+3) - sizeof(divide_line.data_size);
											divide_line.added = 0;
											divide_line.before_num_points = line->num_points;
											divide_line.after_num_points = i+3;
											divide_line.index = line_index;
											divide_line.line_type = line->base_data.vector_type;
											(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
											(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

											before_point = src_points[i+2];
											line->points[i+2].x = check_point.x;
											line->points[i+2].y = check_point.y;
											line->num_points = i+3;

											(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

											// ���C�������X�^���C�Y����t���O�𗧂Ă�
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
											add_line.line_type = new_line->base_data.vector_type;
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
										// ���݂̍��W���n�_�ɂ��ĐV���ȃ��C���쐬
										new_line = CreateVectorLine(line, (VECTOR_LINE*)line->base_data.next);
										new_line->base_data.vector_type = line->base_data.vector_type;
										new_line->num_points = before_num_points - i - 1;
										new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
											* VECTOR_LINE_BUFFER_SIZE;
										// ����_�̐ݒ�
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

										// ���C�������X�^���C�Y����t���O�𗧂Ă�
										new_line->flags |= VECTOR_LINE_FIX;

										// ���݂̐���_�����L��
										end_flag = i;

										// �����I��
										in_devide++;
									}
								}

								t += d;
							}
						}

						// �I�_�̎��x�W�F�Ȑ�
							// �b��I�ȋ����v�Z
						dx = src_points[i+1].x-src_points[i+2].x;
						dy = src_points[i+1].y-src_points[i+2].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						// ����_���W�v�Z�̏���
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = src_points[i+j].x;
							bezier_points[j].y = src_points[i+j].y;
						}

						// ����_���W�v�Z
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
								// �������̈�Əd�Ȃ����̂Ń��C���폜
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8 && in_devide % 2 == 0)
								{
									if(in_devide == 0)
									{
										num_divide++;

										// �ύX�O�̃��C�����𗚗��f�[�^�Ƃ��Ďc��
										divide_line.data_size = offsetof(DIVIDE_LINE_CHANGE_DATA, before)
											+ sizeof(VECTOR_POINT)*(line->num_points + i+3) - sizeof(divide_line.data_size);
										divide_line.added = 0;
										divide_line.before_num_points = line->num_points;
										divide_line.after_num_points = i+3;
										divide_line.index = line_index;
										divide_line.line_type = line->base_data.vector_type;
										(void)MemWrite(&divide_line, 1, offsetof(DIVIDE_LINE_CHANGE_DATA, before), divide_change);
										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										before_point = line->points[i+2];
										line->points[i+2].x = check_point.x;
										line->points[i+2].y = check_point.y;
										line->num_points = i+3;

										(void)MemWrite(line->points, 1, sizeof(*line->points)*line->num_points, divide_change);

										// ���C�������X�^���C�Y����t���O�𗧂Ă�
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
										add_line.line_type = new_line->base_data.vector_type;
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
									// ���݂̍��W���n�_�ɂ��ĐV���ȃ��C���쐬
									new_line = CreateVectorLine(line, (VECTOR_LINE*)line->base_data.next);
									new_line->base_data.vector_type = line->base_data.vector_type;
									new_line->num_points = 2;
									new_line->buff_size = (new_line->num_points / VECTOR_LINE_BUFFER_SIZE + 1)
										* VECTOR_LINE_BUFFER_SIZE;
									// ����_�̐ݒ�
									new_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*new_line->points)*new_line->buff_size);
									(void)memset(new_line->points, 0, sizeof(*new_line->points)*new_line->buff_size);
									new_line->points->x = check_point.x, new_line->points->y = check_point.y;
									new_line->points->pressure = src_points[i+1].pressure;
									new_line->points->size = src_points[i+1].size;
									new_line->points->vector_type = src_points[i+1].vector_type;
									(void)memcpy(new_line->points->color, src_points[i+1].color, 4);
									this_add++;

									new_line->points[1] = before_point;

									// ���C�������X�^���C�Y����t���O�𗧂Ă�
									new_line->flags |= VECTOR_LINE_FIX;

									// ���݂̐���_�����L������
									end_flag = i;

									// �����I��
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
							add_line.line_type = new_line->base_data.vector_type;
							add_line.num_points = new_line->num_points;
							(void)MemWrite(&add_line, 1, offsetof(DIVIDE_LINE_ADD_DATA, after), divide_add);
							(void)MemWrite(new_line->points, 1, sizeof(*line->points)*line->num_points, divide_add);

							new_line = NULL;
						}
					}

					for(j=0; j<this_add; j++)
					{
						line = line->base_data.next;
					}

					break;
				case VECTOR_ERASER_STROKE_DELETE:
					// ���C�����X�g���[�N���ď������̈�Əd�Ȃ����烉�C������
					if(line->base_data.vector_type == VECTOR_TYPE_STRAIGHT
						|| line->num_points == 2)
					{	// ����
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

								// �������̈�Əd�Ȃ����̂Ń��C���폜
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE* prev_line = line->base_data.prev;
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

									if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
									{
										window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
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
					else if(line->base_data.vector_type == VECTOR_TYPE_BEZIER_OPEN && line->num_points == 3)
					{	// 2���x�W�F�Ȑ�
						dx = line->points[0].x-line->points[1].x;
						dy = line->points[0].y-line->points[1].y;
						d = sqrt(dx*dx+dy*dy);
						dx = line->points[1].x-line->points[2].x;
						dy = line->points[1].y-line->points[2].y;
						d += sqrt(dx*dx+dy*dy);

						// ����_���W�v�Z�̏���
						for(i=0; i<3; i++)
						{
							bezier_points[i].x = line->points[i].x;
							bezier_points[i].y = line->points[i].y;
						}

						// ����_���W�v�Z
						MakeBezier3EdgeControlPoint(bezier_points, controls);
						bezier_points[1].x = line->points[0].x;
						bezier_points[1].y = line->points[0].y;
						bezier_points[2] = controls[0];
						bezier_points[3].x = line->points[1].x, bezier_points[3].y = line->points[1].y;

						// 2���x�W�F�Ȑ��������ď������̈�Əd�Ȃ邩�𒲂ׂ�
						d = 1 / (d * 1.5);
						t = 0;
						while(t < 1)
						{
							CalcBezier3(bezier_points, t, &check_point);
							int_x = (int)check_point.x, int_y = (int)check_point.y;

							if(int_x >= 0 && int_x < window->width
								&& int_y >= 0 && int_y < window->height)
							{
								// �d�Ȃ����̂Ń��C���폜
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE* prev_line = (VECTOR_LINE*)line->base_data.prev;
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

									if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
									{
										window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
									}
									DeleteVectorLine(&line);
									line = prev_line;
									break;
								}
							}

							t += d;
						}
					}
					else if(line->base_data.vector_type == VECTOR_TYPE_BEZIER_OPEN)
					{	// 2����3���x�W�F�Ȑ��̍���
							// �n�_����2�_�ڂ܂ł�2���x�W�F�Ȑ�
						// �b��I�ȋ����v�Z
						dx = line->points[0].x-line->points[1].x;
						dy = line->points[0].y-line->points[1].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						// ����_���W�v�Z�̏���
						for(i=0; i<3; i++)
						{
							bezier_points[i].x = line->points[i].x;
							bezier_points[i].y = line->points[i].y;
						}

						// ����_���W�v�Z
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
								// �������̈�Əd�Ȃ����̂Ń��C���폜
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE* prev_line = (VECTOR_LINE*)line->base_data.prev;
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

									if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
									{
										window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
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
							// 3���x�W�F�Ȑ�
								// ����_���W�v�Z�̏���
							for(j=0; j<4; j++)
							{
								bezier_points[j].x = line->points[i+j].x;
								bezier_points[j].y = line->points[i+j].y;
							}

							// ����_���W�v�Z
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
									// �������̈�Əd�Ȃ����̂Ń��C���폜
									if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
									{
										VECTOR_LINE *prev_line = (VECTOR_LINE*)line->base_data.prev;
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

										if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
										{
											window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
										}
										DeleteVectorLine(&line);
										line = prev_line;

										goto next_line;
									}
								}

								t += d;
							}
						}

						// �I�_�ƈ��O��2���x�W�F�Ȑ�
							// �b��I�ȋ����v�Z
						dx = line->points[i+1].x-line->points[i+2].x;
						dy = line->points[i+1].y-line->points[i+2].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						// ����_���W�v�Z�̏���
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = line->points[i+j].x;
							bezier_points[j].y = line->points[i+j].y;
						}

						// ����_���W�v�Z
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

							// �������̈�Əd�Ȃ����̂Ń��C���폜
							if(index >= 0 && index < window->width * window->height
								&& window->brush_buffer[index] >= 0xf8)
							{
								VECTOR_LINE *prev_line = (VECTOR_LINE*)line->base_data.prev;
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

								if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
								{
									window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
								}
								DeleteVectorLine(&line);
								line = prev_line;

								break;
							}

							t += d;
						}
					}
					else if(line->base_data.vector_type == VECTOR_TYPE_STRAIGHT_CLOSE)
					{	// ��������
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

								// �������̈�Əd�Ȃ����̂Ń��C���폜
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE *prev_line = (VECTOR_LINE*)line->base_data.prev;
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

									if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
									{
										window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
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

								// �������̈�Əd�Ȃ����̂Ń��C���폜
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE *prev_line = line->base_data.prev;
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

									if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
									{
										window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
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
					else if(line->base_data.vector_type == VECTOR_TYPE_BEZIER_CLOSE)
					{	// 2����3���x�W�F�Ȑ��̍���
							// �n�_����2�_�ڂ܂ł�2���x�W�F�Ȑ�
						// �b��I�ȋ����v�Z
						dx = line->points[0].x-line->points[1].x;
						dy = line->points[0].y-line->points[1].y;
						d = sqrt(dx*dx+dy*dy) * 1.5;

						bezier_points[0].x = line->points[line->num_points-1].x;
						bezier_points[0].y = line->points[line->num_points-1].y;
						// ����_���W�v�Z�̏���
						for(i=0; i<3; i++)
						{
							bezier_points[i+1].x = line->points[i].x;
							bezier_points[i+1].y = line->points[i].y;
						}

						// ����_���W�v�Z
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
								// �������̈�Əd�Ȃ����̂Ń��C���폜
								if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
								{
									VECTOR_LINE *prev_line = line->base_data.prev;
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

									if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
									{
										window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
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
							// 3���x�W�F�Ȑ�
								// ����_���W�v�Z�̏���
							for(j=0; j<4; j++)
							{
								bezier_points[j].x = line->points[i+j].x;
								bezier_points[j].y = line->points[i+j].y;
							}

							// ����_���W�v�Z
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
									// �������̈�Əd�Ȃ����̂Ń��C���폜
									if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
									{
										VECTOR_LINE *prev_line = line->base_data.prev;
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

										if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
										{
											window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
										}
										DeleteVectorLine(&line);
										line = prev_line;

										goto next_line;
									}
								}

								t += d;
							}
						}


						// ����_���W�v�Z�̏���
						for(j=0; j<3; j++)
						{
							bezier_points[j].x = line->points[i+j].x;
							bezier_points[j].y = line->points[i+j].y;
						}
						bezier_points[3].x = line->points[0].x, bezier_points[3].y = line->points[0].y;
						// ����_���W�v�Z
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

							// �������̈�Əd�Ȃ����̂Ń��C���폜
							if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
							{
								VECTOR_LINE *prev_line = line->base_data.prev;
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

								if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
								{
									window->active_layer->layer_data.vector_layer_p->top_data = line->base_data.prev;
								}
								DeleteVectorLine(&line);
								line = prev_line;

								break;
							}

							t += d;
						}

						// ����_���W�v�Z�̏���
						bezier_points[0].x = line->points[i+1].x, bezier_points[0].y = line->points[i+1].y;
						bezier_points[1].x = line->points[i+2].x, bezier_points[1].y = line->points[i+2].y;
						bezier_points[2].x = line->points[0].x, bezier_points[2].y = line->points[0].y;
						bezier_points[3].x = line->points[1].x, bezier_points[3].y = line->points[1].y;
						// ����_���W�v�Z
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

							// �������̈�Əd�Ȃ����̂Ń��C���폜
							if(window->brush_buffer[int_y*window->width+int_x] >= 0xf8)
							{
								VECTOR_LINE *prev_line = line->base_data.prev;
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

								if((void*)line == window->active_layer->layer_data.vector_layer_p->top_data)
								{
									window->active_layer->layer_data.vector_layer_p->top_data = (void*)line->base_data.prev;
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
				line = line->base_data.next;
			}

			line_index++;
		}

		// �����f�[�^���쐬
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
			// �����f�[�^�p�̃o�b�t�@���J��
			for(k=0; k<num_change_line; k++)
			{
				MEM_FREE_FUNC(change_lines[k].points);
			}
			MEM_FREE_FUNC(change_lines);
			MEM_FREE_FUNC(change_line_indexes);

			break;
		}


		// ���C�����d�˒���
		window->active_layer->layer_data.vector_layer_p->flags |=
			VECTOR_LAYER_RASTERIZE_CHECKED;
		// ��Ɨp���C���[�̃��[�h�����ɖ߂�
		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		(void)memset(window->brush_buffer, 0, window->pixel_buf_size);
		(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
	}
}

/*******************************************
* VectorEraserDrawCursor�֐�               *
* �x�N�g�������S���̃J�[�\����`��         *
* ����                                     *
* window	: �`��̈�̏��               *
* x			: �J�[�\����X���W              *
* y			: �J�[�\����Y���W              *
* data		: �x�N�g�������S���̏ڍ׃f�[�^ *
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
* CreateVectorEraserDetailUI�֐�                     *
* �x�N�g�������S���̏ڍאݒ�UI���쐬                 *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* data	: �x�N�g�������S���̏ڍ׃f�[�^               *
* �Ԃ�l                                             *
*	�쐬�����ڍאݒ�E�B�W�F�b�g                     *
*****************************************************/
static GtkWidget* CreateVectorEraserDetailUI(APPLICATION* app, void* data)
{
#define UI_FONT_SIZE 8.0
	// �x�N�g�������S���̏ڍ׃f�[�^�ɃL���X�g
	VECTOR_ERASER* eraser = (VECTOR_ERASER*)data;
	// �Ԃ�l
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	// ���x���ƃT�C�Y�ݒ�p�̃X���C�_
	GtkWidget* label, *eraser_scale;
	// �E�B�W�F�b�g����p�̃e�[�u��
	GtkWidget* table;
	// ���[�h�ݒ�p�̃��W�I�{�^��
	GtkWidget* radio_buttons[VECTOR_ERASER_MODE_NUM];
	// �M���g�p�̃`�F�b�N�{�b�N�X
	GtkWidget* check_button;
	// �X���C�_�Ɏg�p����A�W���X�^
	GtkAdjustment* scale_adjustment = GTK_ADJUSTMENT(
		gtk_adjustment_new(eraser->r * 2, 1, 100, 1, 1, 0));
	// ���x���̃t�H���g�ύX�p�̃}�[�N�A�b�v�o�b�t�@
	char mark_up_buff[256];
	// for���p�̃J�E���^
	int i;

	// �T�C�Y�ݒ�̃E�B�W�F�b�g
	label = gtk_label_new("");
	eraser_scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ChangeVectorEraserScale), data);
	table = gtk_table_new(1, 3, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), eraser_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �����S�����[�h�I��p�̃��W�I�{�^��
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

	// �M���g�p�̃`�F�b�N�{�b�N�X
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

#define HIT_RANGE 15
#define SHAPE_ROTATE 10
#define SHAPE_NEW 11

static void VectorShapeSetPoints(VECTOR_BASE_DATA* data, VECTOR_SHAPE_BRUSH* shape)
{
	if(data->vector_type == VECTOR_TYPE_SQUARE)
	{
		VECTOR_SQUARE *square = (VECTOR_SQUARE*)data;
		FLOAT_T sin_value = sin(square->rotate);
		FLOAT_T cos_value = cos(square->rotate);
		FLOAT_T half_width = square->width * 0.5;
		FLOAT_T half_height = square->height * 0.5;

		shape->shape_points[8][0] = square->left + half_width;
		shape->shape_points[8][1] = square->top + half_height;
		shape->shape_points[0][0] = shape->shape_points[8][0] - half_width * cos_value + half_height * sin_value;
		shape->shape_points[0][1] = shape->shape_points[8][1] - half_width * sin_value - half_height * cos_value;
		shape->shape_points[1][0] = shape->shape_points[8][0] - half_width * cos_value;
		shape->shape_points[1][1] = shape->shape_points[8][1] - half_width * sin_value;
		shape->shape_points[2][0] = shape->shape_points[1][0] + (shape->shape_points[1][0] - shape->shape_points[0][0]);
		shape->shape_points[2][1] = shape->shape_points[1][1] + (shape->shape_points[1][1] - shape->shape_points[0][1]);
		shape->shape_points[3][0] = shape->shape_points[8][0] - half_height * sin_value;
		shape->shape_points[3][1] = shape->shape_points[8][1] + half_height * cos_value;
		shape->shape_points[4][0] = shape->shape_points[3][0] + (shape->shape_points[3][0] - shape->shape_points[2][0]);
		shape->shape_points[4][1] = shape->shape_points[3][1] + (shape->shape_points[3][1] - shape->shape_points[2][1]);
		shape->shape_points[5][0] = shape->shape_points[8][0] + half_width * cos_value;
		shape->shape_points[5][1] = shape->shape_points[8][1] + half_width * sin_value;
		shape->shape_points[6][0] = shape->shape_points[5][0] + (shape->shape_points[5][0] - shape->shape_points[4][0]);
		shape->shape_points[6][1] = shape->shape_points[5][1] + (shape->shape_points[5][1] - shape->shape_points[4][1]);
		shape->shape_points[7][0] = (shape->shape_points[0][0] + shape->shape_points[6][0]) * 0.5;
		shape->shape_points[7][1] = (shape->shape_points[0][1] + shape->shape_points[6][1]) * 0.5;
	}
	else if(data->vector_type == VECTOR_TYPE_RHOMBUS
		|| data->vector_type == VECTOR_TYPE_ECLIPSE)
	{
		VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)data;
		FLOAT_T sin_value = sin(eclipse->rotate);
		FLOAT_T cos_value = cos(eclipse->rotate);
		FLOAT_T half_width, half_height;
		if(eclipse->ratio >= 1.0)
		{
			half_width = eclipse->radius * eclipse->ratio;
			half_height = eclipse->radius;
		}
		else
		{
			half_width = eclipse->radius;
			half_height = eclipse->radius * (1.0 / eclipse->ratio);
		}

		shape->shape_points[8][0] = eclipse->x;
		shape->shape_points[8][1] = eclipse->y;
		shape->shape_points[0][0] = shape->shape_points[8][0] - half_width * cos_value + half_height * sin_value;
		shape->shape_points[0][1] = shape->shape_points[8][1] - half_width * sin_value - half_height * cos_value;
		shape->shape_points[1][0] = shape->shape_points[8][0] - half_width * cos_value;
		shape->shape_points[1][1] = shape->shape_points[8][1] - half_width * sin_value;
		shape->shape_points[2][0] = shape->shape_points[1][0] + (shape->shape_points[1][0] - shape->shape_points[0][0]);
		shape->shape_points[2][1] = shape->shape_points[1][1] + (shape->shape_points[1][1] - shape->shape_points[0][1]);
		shape->shape_points[3][0] = shape->shape_points[8][0] - half_height * sin_value;
		shape->shape_points[3][1] = shape->shape_points[8][1] + half_height * cos_value;
		shape->shape_points[4][0] = shape->shape_points[3][0] + (shape->shape_points[3][0] - shape->shape_points[2][0]);
		shape->shape_points[4][1] = shape->shape_points[3][1] + (shape->shape_points[3][1] - shape->shape_points[2][1]);
		shape->shape_points[5][0] = shape->shape_points[8][0] + half_width * cos_value;
		shape->shape_points[5][1] = shape->shape_points[8][1] + half_width * sin_value;
		shape->shape_points[6][0] = shape->shape_points[5][0] + (shape->shape_points[5][0] - shape->shape_points[4][0]);
		shape->shape_points[6][1] = shape->shape_points[5][1] + (shape->shape_points[5][1] - shape->shape_points[4][1]);
		shape->shape_points[7][0] = (shape->shape_points[0][0] + shape->shape_points[6][0]) * 0.5;
		shape->shape_points[7][1] = (shape->shape_points[0][1] + shape->shape_points[6][1]) * 0.5;
	}
}

typedef struct _VECTOR_SHAPE_CHANGE_HISTORY
{
	VECTOR_DATA vector_data;
	int32 vector_index;
	uint16 layer_name_length;
	char *layer_name;
} VECTOR_SHAPE_CHANGE_HISTORY;

static void VectorShapeChangeUndoRedo(DRAW_WINDOW* window, void* p)
{
	VECTOR_SHAPE_CHANGE_HISTORY *history_data =
		(VECTOR_SHAPE_CHANGE_HISTORY*)p;
	LAYER *layer = window->layer;
	VECTOR_BASE_DATA *shape;
	char *name = &(((char*)history_data)[offsetof(VECTOR_SHAPE_CHANGE_HISTORY, layer_name)]);
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	shape = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->vector_index; i++)
	{
		shape = shape->next;
	}

	if(history_data->vector_data.line.base_data.vector_type == VECTOR_TYPE_SQUARE)
	{
		VECTOR_SQUARE *square = (VECTOR_SQUARE*)shape;
		VECTOR_BASE_DATA before_base = square->base_data;
		VECTOR_SQUARE before_data = *square;

		*square = history_data->vector_data.square;
		square->base_data = before_base;
		history_data->vector_data.square = before_data;
	}
	else if(history_data->vector_data.line.base_data.vector_type == VECTOR_TYPE_RHOMBUS
		|| history_data->vector_data.line.base_data.vector_type == VECTOR_TYPE_ECLIPSE)
	{
		VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)shape;
		VECTOR_BASE_DATA before_base = eclipse->base_data;
		VECTOR_ECLIPSE before_data = *eclipse;

		*eclipse = history_data->vector_data.eclipse;
		eclipse->base_data = before_base;
		history_data->vector_data.eclipse = before_data;
	}

	layer->layer_data.vector_layer_p->active_data = (void*)shape;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

	if(layer == window->active_layer)
	{
		if(window->app->tool_window.active_vector_brush[window->app->input]->brush_type == TYPE_VECTOR_SHAPE_BRUSH)
		{
			VectorShapeSetPoints(shape,
				(VECTOR_SHAPE_BRUSH*)window->app->tool_window.active_vector_brush[window->app->input]->brush_data);
		}
	}
}

static void AddChangeVectorShapeHistory(
	DRAW_WINDOW* window,
	VECTOR_BASE_DATA* change_shape,
	VECTOR_BASE_DATA* before_shape,
	const char* tool_name
)
{
	uint8 *byte_data;
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name);
	VECTOR_SHAPE_CHANGE_HISTORY *history_data =
		(VECTOR_SHAPE_CHANGE_HISTORY*)MEM_ALLOC_FUNC(
			offsetof(VECTOR_SHAPE_CHANGE_HISTORY, layer_name) + layer_name_length + 4);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_BASE_DATA *shape = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int shape_index = 0;

	while(shape != change_shape)
	{
		shape = shape->next;
		shape_index++;
	}

	if(shape->vector_type == VECTOR_TYPE_SQUARE)
	{
		history_data->vector_data.square = *((VECTOR_SQUARE*)before_shape);
	}
	else if(shape->vector_type == VECTOR_TYPE_RHOMBUS
		|| shape->vector_type == VECTOR_TYPE_ECLIPSE)
	{
		history_data->vector_data.eclipse = *((VECTOR_ECLIPSE*)before_shape);
	}
	else
	{
		MEM_FREE_FUNC(history_data);
		return;
	}

	history_data->vector_index = (int32)shape_index;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(VECTOR_SHAPE_CHANGE_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);

	AddHistory(&window->history, tool_name,
		(void*)history_data, offsetof(VECTOR_SHAPE_CHANGE_HISTORY, layer_name) + layer_name_length + 4,
			VectorShapeChangeUndoRedo, VectorShapeChangeUndoRedo
	);
}

typedef struct _VECTOR_SCRIPT_CHANGE_HISTORY
{
	size_t buffer_length;
	int32 vector_index;
	uint16 layer_name_length;
	char *layer_name;
} VECTOR_SCRIPT_CHANGE_HISTORY;

static void VectorScriptChangeUndoRedo(DRAW_WINDOW* window, void* p)
{
	VECTOR_SCRIPT_CHANGE_HISTORY *history_data =
		(VECTOR_SCRIPT_CHANGE_HISTORY*)p;
	LAYER *layer = window->layer;
	VECTOR_SCRIPT *target;
	VECTOR_BASE_DATA *script;
	char *name = &(((char*)history_data)[offsetof(VECTOR_SCRIPT_CHANGE_HISTORY, layer_name)]);
	char *script_data;
	char *temp;
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	script = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	for(i=0; i<history_data->vector_index; i++)
	{
		script = script->next;
	}

	temp = (char*)MEM_ALLOC_FUNC(history_data->buffer_length);

	target = (VECTOR_SCRIPT*)script;
	(void)strcpy(temp, target->script_data);
	script_data = name + history_data->layer_name_length;
	MEM_FREE_FUNC(target->script_data);
	target->script_data = MEM_STRDUP_FUNC(script_data);
	(void)strcpy(script_data, temp);

	MEM_FREE_FUNC(temp);

	layer->layer_data.vector_layer_p->active_data = (void*)script;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void AddChangeVectorScriptHistory(
	DRAW_WINDOW* window,
	VECTOR_SCRIPT* change_script,
	const char* before_script_data,
	const char* tool_name
)
{
	uint8 *byte_data;
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name) + 1;
	size_t buffer_length;
	size_t before_length = strlen(before_script_data)+1,
		after_length = strlen(change_script->script_data)+1;
	VECTOR_SCRIPT_CHANGE_HISTORY *history_data;
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_BASE_DATA *script = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int script_index = 0;

	buffer_length = (before_length > after_length) ? before_length : after_length;
	history_data  =
		(VECTOR_SCRIPT_CHANGE_HISTORY*)MEM_ALLOC_FUNC(
			offsetof(VECTOR_SHAPE_CHANGE_HISTORY, layer_name) + layer_name_length + buffer_length);

	while(script != &change_script->base_data)
	{
		script = script->next;
		script_index++;
	}

	history_data->vector_index = (int32)script_index;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(VECTOR_SHAPE_CHANGE_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);
	byte_data += layer_name_length;
	(void)strcpy((char*)byte_data, before_script_data);

	AddHistory(&window->history, tool_name,
		(void*)history_data, offsetof(VECTOR_SCRIPT_CHANGE_HISTORY, layer_name) + layer_name_length + buffer_length,
			VectorScriptChangeUndoRedo, VectorScriptChangeUndoRedo
	);
}

static void VectorShapeEditScript(
	DRAW_WINDOW* window,
	VECTOR_SHAPE_BRUSH* shape,
	LAYER* target_layer,
	VECTOR_SCRIPT* target
)
{
	VECTOR_LAYER *layer = target_layer->layer_data.vector_layer_p;
	VECTOR_SCRIPT *script = target;
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *text_edit;
	GtkWidget *scroll;
	GtkAllocation allocation;
	gint result;
	char *before_script_data = NULL;

	dialog = gtk_dialog_new_with_buttons(
		window->app->labels->menu.script,
		GTK_WINDOW(window->app->widgets->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		"Open", GTK_RESPONSE_YES,
		GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL
	);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

	vbox = gtk_vbox_new(FALSE, 0);
	text_edit = gtk_text_view_new();
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scroll, shape->text_field_size[0], shape->text_field_size[1]);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), text_edit);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

	if(target != NULL && target->script_data != NULL)
	{
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_edit)),
			target->script_data, -1);
		before_script_data = MEM_STRDUP_FUNC(target->script_data);
	}

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		vbox, TRUE, TRUE, 3);

	gtk_widget_show_all(dialog);
	do
	{
		result = gtk_dialog_run(GTK_DIALOG(dialog));
		switch(result)
		{
		case GTK_RESPONSE_YES:
			{
				GtkWidget *chooser = gtk_file_chooser_dialog_new(
					window->app->labels->menu.open,
					GTK_WINDOW(window->app->widgets->window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL
				);
				GtkFileFilter *filter;

				filter = gtk_file_filter_new();
				gtk_file_filter_set_name(filter, "Lua Script File");
				gtk_file_filter_add_pattern(filter, "*.lua");
				gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

				if(shape->before_script_directory != NULL)
				{
					(void)gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), shape->before_script_directory);
				}
				else
				{
					(void)gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), window->app->current_path);
				}

				if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK)
				{
					char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
					char *system_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);
					FILE *fp = fopen(system_path, "rb");
					long file_size;
					char *data;
					char *p;
					int i;

					(void)fseek(fp, SEEK_END, 0);
					file_size = ftell(fp);
					rewind(fp);

					data = (char*)MEM_ALLOC_FUNC(file_size);
					(void)fread(data, 1, file_size, fp);
					(void)fclose(fp);

					for(i=0; i<file_size; i++)
					{
						if(data[i] == '\0')
						{
							break;
						}
					}
					if(i == file_size)
					{
						if(script == NULL)
						{
							script = 
								(VECTOR_SCRIPT*)CreateVectorScript((VECTOR_BASE_DATA*)layer->top_data, NULL, NULL);
							layer->top_data = (void*)script;
						}
						MEM_FREE_FUNC(script->script_data);
						script->script_data = MEM_STRDUP_FUNC(data);
					}
					MEM_FREE_FUNC(data);

					p = g_path_get_dirname(path);
					shape->before_script_directory = MEM_STRDUP_FUNC(p);
					g_free(p);

					g_free(path);
					g_free(system_path);
				}

				gtk_widget_destroy(chooser);

				layer->active_data = (void*)script;
				layer->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
				window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
				gtk_widget_queue_draw(window->window);
			}
			break;
		case GTK_RESPONSE_APPLY:
			{
				GtkTextIter start, end;
				GtkTextBuffer *buffer;
				char *p;
				if(script == NULL)
				{
					script = (VECTOR_SCRIPT*)CreateVectorScript((VECTOR_BASE_DATA*)layer->top_data, NULL, NULL);
					layer->top_data = (void*)script;
				}
				MEM_FREE_FUNC(script->script_data);
				buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_edit));
				gtk_text_buffer_get_start_iter(buffer, &start);
				gtk_text_buffer_get_end_iter(buffer, &end);
				script->script_data = MEM_STRDUP_FUNC(
					p = gtk_text_buffer_get_text(buffer, &start, &end, FALSE));
				g_free(p);

				layer->active_data = (void*)script;
				layer->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
				window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
				gtk_widget_queue_draw(window->window);
			}
			break;
		case GTK_RESPONSE_OK:
			{
				GtkTextIter start, end;
				GtkTextBuffer *buffer;
				char *p;
				if(script == NULL)
				{
					script = (VECTOR_SCRIPT*)CreateVectorScript((VECTOR_BASE_DATA*)layer->top_data, NULL, NULL);
					layer->top_data = (void*)script;
				}
				MEM_FREE_FUNC(script->script_data);
				buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_edit));
				gtk_text_buffer_get_start_iter(buffer, &start);
				gtk_text_buffer_get_end_iter(buffer, &end);
				script->script_data = MEM_STRDUP_FUNC(
					p = gtk_text_buffer_get_text(buffer, &start, &end, FALSE));
				g_free(p);

				layer->active_data = (void*)script;
				layer->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
				window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
				gtk_widget_queue_draw(window->window);

				if(target == NULL)
				{
					AddTopScriptHistory(window, window->active_layer,
						script, shape->brush_name);
				}
				else
				{
					AddChangeVectorScriptHistory(window, script,
						before_script_data, shape->brush_name);
				}
			}
			break;
		case GTK_RESPONSE_CANCEL:
			if(target == NULL)
			{
				if(script != NULL)
				{
					layer->top_data = ((VECTOR_BASE_DATA*)layer->top_data)->prev;
					DeleteVectorScript(&script);
				}
			}
			layer->active_data = NULL;
			layer->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
			gtk_widget_queue_draw(window->window);
			break;
		}
	} while(result == GTK_RESPONSE_YES || result == GTK_RESPONSE_APPLY);

	gtk_widget_get_allocation(scroll, &allocation);
	shape->text_field_size[0] = allocation.width;
	shape->text_field_size[1] = allocation.height;

	MEM_FREE_FUNC(before_script_data);
	gtk_widget_destroy(dialog);
}

static void VectorShapeChangeFloatValue(GtkWidget* spin_button, FLOAT_T* value)
{
	DRAW_WINDOW *window = (DRAW_WINDOW*)g_object_get_data(G_OBJECT(spin_button), "draw_window");
	*value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button));
	window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(window->window);
}

static void VectorShapeChangeRotate(GtkWidget* spin_button, FLOAT_T* value)
{
	DRAW_WINDOW *window = (DRAW_WINDOW*)g_object_get_data(G_OBJECT(spin_button), "draw_window");
	*value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button)) * G_PI / 180;
	window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(window->window);
}

static void VectorShapeChangeColor(GtkWidget* color_button, uint8* color)
{
	DRAW_WINDOW *window = (DRAW_WINDOW*)g_object_get_data(G_OBJECT(color_button), "draw_window");
	GdkColor color_value;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(color_button), &color_value);
	color[0] = color_value.red >> 8;
	color[1] = color_value.green >> 8;
	color[2] = color_value.blue >> 8;
	color[3] = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(color_button)) >> 8;
	window->active_layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(window->window);
}

static void VectorShapeManuallySetDialog(
	DRAW_WINDOW* window,
	VECTOR_SHAPE_BRUSH* shape,
	VECTOR_BASE_DATA* target
)
{
#define UI_FONT_SIZE 8.0
	APPLICATION *app = window->app;
	VECTOR_DATA before_data;
	VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *layout;
	GtkWidget *x, *y;
	GtkWidget *size[2];
	GtkWidget *rotate;
	GtkWidget *line_width;
	GtkWidget *line_color;
	GtkWidget *fill_color;
	GdkColor color;
	char str[128];

	uint8 shape_type = (target != NULL) ? target->vector_type : shape->shape_type;

	if(shape_type == VECTOR_TYPE_SCRIPT)
	{
		VectorShapeEditScript(window, shape, window->active_layer, (VECTOR_SCRIPT*)target);
		return;
	}

	dialog = gtk_dialog_new_with_buttons(
		"",
		GTK_WINDOW(app->widgets->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL
	);
	vbox = gtk_vbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox, FALSE, FALSE, 0);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

	if(shape_type == VECTOR_TYPE_SQUARE)
	{
		VECTOR_SQUARE *square = (VECTOR_SQUARE*)target;

		if(target != NULL)
		{
			before_data.square = *square;
		}

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">X : </span>", UI_FONT_SIZE);
		gtk_label_set_markup(GTK_LABEL(label), str);
		x = gtk_spin_button_new_with_range(- window->width, window->width * 2, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(x), (target != NULL) ? square->left : 0);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(x), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), x, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);
		
		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">Y : </span>", UI_FONT_SIZE);
		gtk_label_set_markup(GTK_LABEL(label), str);
		y = gtk_spin_button_new_with_range(- window->height, window->height * 2, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(y), (target != NULL) ? square->top : 0);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(y), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), y, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->make_new.width);
		gtk_label_set_markup(GTK_LABEL(label), str);
		size[0] = gtk_spin_button_new_with_range(0.1, window->width * 2, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(size[0]), (target != NULL) ? square->width : window->width * 0.1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(size[0]), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), size[0], FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->make_new.height);
		gtk_label_set_markup(GTK_LABEL(label), str);
		size[1] = gtk_spin_button_new_with_range(0.1, window->width * 2, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(size[1]), (target != NULL) ? square->height : window->height * 0.1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(size[1]), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), size[1], FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->menu.rotate);
		gtk_label_set_markup(GTK_LABEL(label), str);
		rotate = gtk_spin_button_new_with_range(0, 360, 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(rotate), (target != NULL) ? square->rotate * 180 / G_PI : 0);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(rotate), 0);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), rotate, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->tool_box.line_width);
		gtk_label_set_markup(GTK_LABEL(label), str);
		line_width = gtk_spin_button_new_with_range(0.1, window->width * 2, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(line_width), (target != NULL) ? square->line_width : 1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(line_width), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), line_width, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->tool_box.line_color);
		gtk_label_set_markup(GTK_LABEL(label), str);
		if(target != NULL)
		{
			color.red = square->line_color[0] | (square->line_color[0] << 8);
			color.green = square->line_color[1] | (square->line_color[1] << 8);
			color.blue = square->line_color[2] | (square->line_color[2] << 8);
		}
		else
		{
			color.red = shape->line_color[0] | (shape->line_color[0] << 8);
			color.green = shape->line_color[1] | (shape->line_color[1] << 8);
			color.blue = shape->line_color[2] | (shape->line_color[2] << 8);
		}
		line_color = gtk_color_button_new_with_color(&color);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(line_color), TRUE);
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(line_color),
			(target != NULL) ? (square->line_color[3] | (square->line_color[3] << 8)) : (shape->line_color[3] | (shape->line_color[3] << 8)));
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), line_color, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->tool_box.fill_color);
		gtk_label_set_markup(GTK_LABEL(label), str);
		if(target != NULL)
		{
			color.red = square->fill_color[0] | (square->fill_color[0] << 8);
			color.green = square->fill_color[1] | (square->fill_color[1] << 8);
			color.blue = square->fill_color[2] | (square->fill_color[2] << 8);
		}
		else
		{
			color.red = shape->fill_color[0] | (shape->fill_color[0] << 8);
			color.green = shape->fill_color[1] | (shape->fill_color[1] << 8);
			color.blue = shape->fill_color[2] | (shape->fill_color[2] << 8);
		}
		fill_color = gtk_color_button_new_with_color(&color);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(fill_color), TRUE);
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(fill_color),
			(target != NULL) ? (square->fill_color[3] | (square->fill_color[3] << 8)) : (shape->fill_color[3] | (shape->fill_color[3] << 8)));
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), fill_color, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		if(target != NULL)
		{
			g_object_set_data(G_OBJECT(x), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(x), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &square->left);

			g_object_set_data(G_OBJECT(y), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(y), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &square->top);

			g_object_set_data(G_OBJECT(size[0]), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(size[0]), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &square->width);

			g_object_set_data(G_OBJECT(size[1]), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(size[1]), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &square->height);

			g_object_set_data(G_OBJECT(rotate), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(rotate), "changed", G_CALLBACK(VectorShapeChangeRotate), &square->rotate);

			g_object_set_data(G_OBJECT(line_width), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(line_width), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &square->line_width);

			g_object_set_data(G_OBJECT(line_color), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(line_color), "color-set", G_CALLBACK(VectorShapeChangeColor), square->line_color);

			g_object_set_data(G_OBJECT(fill_color), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(fill_color), "color-set", G_CALLBACK(VectorShapeChangeColor), square->fill_color);
		}
	}
	else if(shape_type == VECTOR_TYPE_RHOMBUS
		|| shape_type == VECTOR_TYPE_ECLIPSE)
	{
		VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)target;

		if(target != NULL)
		{
			before_data.eclipse = *eclipse;
		}

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">X : </span>", UI_FONT_SIZE);
		gtk_label_set_markup(GTK_LABEL(label), str);
		x = gtk_spin_button_new_with_range(- window->width, window->width * 2, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(x), (target != NULL) ? eclipse->x : window->width * 0.5);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(x), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), x, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);
		
		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">Y : </span>", UI_FONT_SIZE);
		gtk_label_set_markup(GTK_LABEL(label), str);
		y = gtk_spin_button_new_with_range(- window->height, window->height * 2, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(y), (target != NULL) ? eclipse->y : window->height * 0.5);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(y), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), y, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->tool_box.scale);
		gtk_label_set_markup(GTK_LABEL(label), str);
		size[0] = gtk_spin_button_new_with_range(0.1, window->width * 2, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(size[0]), (target != NULL) ? eclipse->radius : window->width * 0.1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(size[0]), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), size[0], FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->tool_box.aspect_ratio);
		gtk_label_set_markup(GTK_LABEL(label), str);
		size[1] = gtk_spin_button_new_with_range(0.1, 10, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(size[1]), (target != NULL) ? eclipse->ratio : window->width / (FLOAT_T)window->height);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(size[1]), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), size[1], FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->menu.rotate);
		gtk_label_set_markup(GTK_LABEL(label), str);
		rotate = gtk_spin_button_new_with_range(0, 360, 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(rotate), (target != NULL) ? eclipse->rotate * 180 / G_PI : 0);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(rotate), 0);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), rotate, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->tool_box.line_width);
		gtk_label_set_markup(GTK_LABEL(label), str);
		line_width = gtk_spin_button_new_with_range(0.1, window->width * 2, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(line_width), (target != NULL) ? eclipse->line_width : 1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(line_width), 1);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), line_width, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->tool_box.line_color);
		gtk_label_set_markup(GTK_LABEL(label), str);
		if(target != NULL)
		{
			color.red = eclipse->line_color[0] | (eclipse->line_color[0] << 8);
			color.green = eclipse->line_color[1] | (eclipse->line_color[1] << 8);
			color.blue = eclipse->line_color[2] | (eclipse->line_color[2] << 8);
		}
		else
		{
			color.red = shape->line_color[0] | (shape->line_color[0] << 8);
			color.green = shape->line_color[1] | (shape->line_color[1] << 8);
			color.blue = shape->line_color[2] | (shape->line_color[2] << 8);
		}
		line_color = gtk_color_button_new_with_color(&color);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(line_color), TRUE);
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(line_color),
			(target != NULL) ? (eclipse->line_color[3] | (eclipse->line_color[3] << 8)) : (shape->line_color[3] | (shape->line_color[3] << 8)));
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), line_color, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>", UI_FONT_SIZE, app->labels->tool_box.fill_color);
		gtk_label_set_markup(GTK_LABEL(label), str);
		if(target != NULL)
		{
			color.red = eclipse->fill_color[0] | (eclipse->fill_color[0] << 8);
			color.green = eclipse->fill_color[1] | (eclipse->fill_color[1] << 8);
			color.blue = eclipse->fill_color[2] | (eclipse->fill_color[2] << 8);
		}
		else
		{
			color.red = shape->fill_color[0] | (shape->fill_color[0] << 8);
			color.green = shape->fill_color[1] | (shape->fill_color[1] << 8);
			color.blue = shape->fill_color[2] | (shape->fill_color[2] << 8);
		}
		fill_color = gtk_color_button_new_with_color(&color);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(fill_color), TRUE);
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(fill_color),
			(target != NULL) ? (eclipse->fill_color[3] | (eclipse->fill_color[3] << 8)) : (shape->fill_color[3] | (shape->fill_color[3] << 8)));
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), fill_color, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		if(target != NULL)
		{
			g_object_set_data(G_OBJECT(x), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(x), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &eclipse->x);

			g_object_set_data(G_OBJECT(y), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(y), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &eclipse->y);

			g_object_set_data(G_OBJECT(size[0]), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(size[0]), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &eclipse->radius);

			g_object_set_data(G_OBJECT(size[1]), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(size[1]), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &eclipse->ratio);

			g_object_set_data(G_OBJECT(rotate), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(rotate), "changed", G_CALLBACK(VectorShapeChangeRotate), &eclipse->rotate);

			g_object_set_data(G_OBJECT(line_width), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(line_width), "changed", G_CALLBACK(VectorShapeChangeFloatValue), &eclipse->line_width);

			g_object_set_data(G_OBJECT(line_color), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(line_color), "color-set", G_CALLBACK(VectorShapeChangeColor), eclipse->line_color);

			g_object_set_data(G_OBJECT(fill_color), "draw_window", window);
			(void)g_signal_connect(G_OBJECT(fill_color), "color-set", G_CALLBACK(VectorShapeChangeColor), eclipse->fill_color);
		}
	}
	else
	{
		gtk_widget_destroy(dialog);
		return;
	}

	gtk_widget_show_all(dialog);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		if(target == NULL)
		{
			if(shape_type == VECTOR_TYPE_SQUARE)
			{
				VECTOR_SQUARE *square = (VECTOR_SQUARE*)(
					layer->top_data = (void*)CreateVectorShape((VECTOR_BASE_DATA*)layer->top_data, NULL, shape->shape_type));
				square->left = gtk_spin_button_get_value(GTK_SPIN_BUTTON(x));
				square->top = gtk_spin_button_get_value(GTK_SPIN_BUTTON(y));
				square->width = gtk_spin_button_get_value(GTK_SPIN_BUTTON(size[0]));
				square->height = gtk_spin_button_get_value(GTK_SPIN_BUTTON(size[1]));
				square->rotate = gtk_spin_button_get_value(GTK_SPIN_BUTTON(rotate));
				square->line_width = gtk_spin_button_get_value(GTK_SPIN_BUTTON(line_width));
				gtk_color_button_get_color(GTK_COLOR_BUTTON(line_color), &color);
				square->line_color[0] = (uint8)(color.red >> 8);
				square->line_color[1] = (uint8)(color.green >> 8);
				square->line_color[2] = (uint8)(color.blue >> 8);
				square->line_color[3] = (uint8)(gtk_color_button_get_alpha(GTK_COLOR_BUTTON(line_color)) >> 8);
				gtk_color_button_get_color(GTK_COLOR_BUTTON(fill_color), &color);
				square->fill_color[0] = (uint8)(color.red >> 8);
				square->fill_color[1] = (uint8)(color.green >> 8);
				square->fill_color[2] = (uint8)(color.blue >> 8);
				square->fill_color[3] = (uint8)(gtk_color_button_get_alpha(GTK_COLOR_BUTTON(fill_color)) >> 8);
			}
			else if(shape_type == VECTOR_TYPE_RHOMBUS
				|| shape_type == VECTOR_TYPE_ECLIPSE)
			{
				VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)(
					layer->top_data = (void*)CreateVectorShape((VECTOR_BASE_DATA*)layer->top_data, NULL, shape->shape_type));
				eclipse->x = gtk_spin_button_get_value(GTK_SPIN_BUTTON(x));
				eclipse->y = gtk_spin_button_get_value(GTK_SPIN_BUTTON(y));
				eclipse->radius = gtk_spin_button_get_value(GTK_SPIN_BUTTON(size[0]));
				eclipse->ratio = gtk_spin_button_get_value(GTK_SPIN_BUTTON(size[1]));
				eclipse->rotate = gtk_spin_button_get_value(GTK_SPIN_BUTTON(rotate));
				eclipse->line_width = gtk_spin_button_get_value(GTK_SPIN_BUTTON(line_width));
				gtk_color_button_get_color(GTK_COLOR_BUTTON(line_color), &color);
				eclipse->line_color[0] = (uint8)(color.red >> 8);
				eclipse->line_color[1] = (uint8)(color.green >> 8);
				eclipse->line_color[2] = (uint8)(color.blue >> 8);
				eclipse->line_color[3] = (uint8)(gtk_color_button_get_alpha(GTK_COLOR_BUTTON(line_color)) >> 8);
				gtk_color_button_get_color(GTK_COLOR_BUTTON(fill_color), &color);
				eclipse->fill_color[0] = (uint8)(color.red >> 8);
				eclipse->fill_color[1] = (uint8)(color.green >> 8);
				eclipse->fill_color[2] = (uint8)(color.blue >> 8);
				eclipse->fill_color[3] = (uint8)(gtk_color_button_get_alpha(GTK_COLOR_BUTTON(fill_color)) >> 8);
			}

			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
			layer->active_data = layer->top_data;
			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
			gtk_widget_queue_draw(window->window);
		}
		else
		{
			AddChangeVectorShapeHistory(window, (VECTOR_BASE_DATA*)layer->active_data,
				(VECTOR_BASE_DATA*)&before_data, shape->brush_name);
			layer->flags |= (VECTOR_LAYER_RASTERIZE_ACTIVE | VECTOR_LAYER_FIX_LINE);
			layer->active_data = layer->top_data;
			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
			gtk_widget_queue_draw(window->window);
		}
	}
	else
	{
		if(target != NULL)
		{
			if(shape_type == VECTOR_TYPE_SQUARE)
			{
				VECTOR_SQUARE *square = (VECTOR_SQUARE*)target;
				*square = before_data.square;
			}
			else if(shape_type == VECTOR_TYPE_RHOMBUS
				|| shape_type == VECTOR_TYPE_ECLIPSE)
			{
				VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)target;
				*eclipse = before_data.eclipse;
			}
			layer->flags |= (VECTOR_LAYER_RASTERIZE_ACTIVE | VECTOR_LAYER_FIX_LINE);
			layer->active_data = layer->top_data;
			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
			gtk_widget_queue_draw(window->window);
		}
	}

	gtk_widget_destroy(dialog);

#undef UI_FONT_SIZE
}

static void VectorShapePressCallBack(
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
		VECTOR_SHAPE_BRUSH *shape = (VECTOR_SHAPE_BRUSH*)core->brush_data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;
		int i;
		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		shape->add_vector = FALSE;
		shape->changing_shape = FALSE;

		switch(shape->mode)
		{
		case VECTOR_SHAPE_BRUSH_MODE_ADD:
			shape->add_vector = TRUE;
			break;
		case VECTOR_SHAPE_BRUSH_MODE_TRANSFORM:
			if(layer->active_data != NULL)
			{
				if(shape->manually_set == FALSE)
				{
					shape->changing_shape = TRUE;
				}
				else
				{
					VectorShapeManuallySetDialog(window, shape,
						(VECTOR_BASE_DATA*)layer->active_data);
				}
			}
			break;
		case VECTOR_SHAPE_BRUSH_MODE_CHANGE_COLOR:
			if(layer->active_data != NULL)
			{
				if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_SQUARE)
				{
					VECTOR_SQUARE *square = (VECTOR_SQUARE*)layer->active_data;
					square->line_color[0] = shape->line_color[0];
					square->line_color[1] = shape->line_color[1];
					square->line_color[2] = shape->line_color[2];
					square->line_color[3] = shape->line_color[3];
					square->fill_color[0] = shape->fill_color[0];
					square->fill_color[1] = shape->fill_color[1];
					square->fill_color[2] = shape->fill_color[2];
					square->fill_color[3] = shape->fill_color[3];
				}
				else if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_RHOMBUS
					|| ((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_ECLIPSE)
				{
					VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)layer->active_data;
					eclipse->line_color[0] = shape->line_color[0];
					eclipse->line_color[1] = shape->line_color[1];
					eclipse->line_color[2] = shape->line_color[2];
					eclipse->line_color[3] = shape->line_color[3];
					eclipse->fill_color[0] = shape->fill_color[0];
					eclipse->fill_color[1] = shape->fill_color[1];
					eclipse->fill_color[2] = shape->fill_color[2];
					eclipse->fill_color[3] = shape->fill_color[3];
				}
				layer->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE | VECTOR_LAYER_FIX_LINE;
			}
			break;
		case VECTOR_SHAPE_BRUSH_MODE_DELETE:
			if(layer->active_data != NULL)
			{
				if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type > VECTOR_TYPE_SHAPE
					&& ((VECTOR_BASE_DATA*)layer->active_data)->vector_type > VECTOR_SHAPE_END)
				{
					AddDeleteVectorShapeHistory(window, window->active_layer,
						(VECTOR_BASE_DATA*)layer->active_data, core->name);
					DeleteVectorShape(((VECTOR_BASE_DATA**)(&(layer->active_data))));
				}

				layer->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
			}
			break;
		}

		if(shape->add_vector != FALSE)
		{
			shape->add_vector = TRUE;

			switch(shape->shape_type)
			{
			case VECTOR_TYPE_SQUARE:
				{
					VECTOR_SQUARE *square = (VECTOR_SQUARE*)(
						layer->top_data = (void*)CreateVectorShape((VECTOR_BASE_DATA*)layer->top_data, NULL, shape->shape_type));
					layer->active_data = layer->top_data;
					square->left = x;
					square->top = y;
					square->width = 0;
					square->height = 0;
					square->rotate = 0;
					square->line_width = shape->line_width;
					(void)memcpy(square->line_color, shape->line_color, sizeof(shape->line_color));
					(void)memcpy(square->fill_color, shape->fill_color, sizeof(shape->fill_color));
				}
				break;
			case VECTOR_TYPE_RHOMBUS:
			case VECTOR_TYPE_ECLIPSE:
				{
					VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)(
						layer->top_data = (void*)CreateVectorShape((VECTOR_BASE_DATA*)layer->top_data, NULL, shape->shape_type));
					layer->active_data = layer->top_data;
					eclipse->x = x;
					eclipse->y = y;
					eclipse->radius = 0;
					eclipse->ratio = 1;
					eclipse->rotate = 0;
					eclipse->line_width = shape->line_width;
					(void)memcpy(eclipse->line_color, shape->line_color, sizeof(shape->line_color));
					(void)memcpy(eclipse->fill_color, shape->fill_color, sizeof(shape->fill_color));
				}
				break;
			}

			for(i=0; i<9; i++)
			{
				shape->shape_points[i][0] = x;
				shape->shape_points[i][1] = y;
			}
			shape->control_point = SHAPE_NEW;
			shape->changing_shape = TRUE;
		}
		else if(shape->changing_shape != FALSE)
		{
			DeleteVectorLineLayer(&((VECTOR_BASE_DATA*)layer->active_data)->layer);
			layer->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
			RasterizeVectorLayer(window, window->active_layer, layer);
			(void)memcpy(layer->mix->pixels, window->active_layer->pixels, window->pixel_buf_size);
		}

		layer->flags |= VECTOR_LAYER_RASTERIZE_TOP;
	}

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void VectorShapeMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	VECTOR_BRUSH_CORE* core,
	void *state
)
{
	VECTOR_SHAPE_BRUSH* shape = (VECTOR_SHAPE_BRUSH*)core->brush_data;
	VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;
	FLOAT_T width, height;
	FLOAT_T before_rotate[2][2];
	FLOAT_T sin_value = 0, cos_value = 1;
	int i;

	if(layer->active_data != NULL)
	{
		if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_SQUARE)
		{
			sin_value = sin(((VECTOR_SQUARE*)layer->active_data)->rotate);
			cos_value = cos(((VECTOR_SQUARE*)layer->active_data)->rotate);
		}
		else if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_RHOMBUS
			|| ((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_ECLIPSE)
		{
			sin_value = sin(((VECTOR_ECLIPSE*)layer->active_data)->rotate);
			cos_value = cos(((VECTOR_ECLIPSE*)layer->active_data)->rotate);
		}
	}

	if(shape->changing_shape != FALSE)
	{
		FLOAT_T x_min, x_max;
		FLOAT_T y_min, y_max;

		switch(shape->control_point-1)
		{
		case 0:
			shape->shape_points[0][0] = x;
			shape->shape_points[0][1] = y;
			shape->shape_points[8][0] = (shape->shape_points[0][0] + shape->shape_points[4][0]) * 0.5;
			shape->shape_points[8][1] = (shape->shape_points[0][1] + shape->shape_points[4][1]) * 0.5;
			before_rotate[0][0] = (shape->shape_points[0][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[0][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[0][1] = (shape->shape_points[0][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[0][0] - shape->shape_points[8][0]) * sin_value;
			before_rotate[1][0] = (shape->shape_points[4][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[4][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[1][1] = (shape->shape_points[4][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[4][0] - shape->shape_points[8][0]) * sin_value;
			width = fabs(before_rotate[1][0] - before_rotate[0][0]);
			height = fabs(before_rotate[1][1] - before_rotate[0][1]);
			shape->shape_points[1][0] = shape->shape_points[8][0] - width * 0.5 * cos_value;
			shape->shape_points[1][1] = shape->shape_points[8][1] - width * 0.5 * sin_value;
			shape->shape_points[2][0] = shape->shape_points[1][0] + (shape->shape_points[1][0] - shape->shape_points[0][0]);
			shape->shape_points[2][1] = shape->shape_points[1][1] + (shape->shape_points[1][1] - shape->shape_points[0][1]);
			shape->shape_points[3][0] = shape->shape_points[8][0] - height * 0.5 * sin_value;
			shape->shape_points[3][1] = shape->shape_points[8][1] + height * 0.5 * cos_value;
			shape->shape_points[5][0] = shape->shape_points[8][0] + width * 0.5 * cos_value;
			shape->shape_points[5][1] = shape->shape_points[8][1] + width * 0.5 * sin_value;
			shape->shape_points[7][0] = shape->shape_points[8][0] + height * 0.5 * sin_value;
			shape->shape_points[7][1] = shape->shape_points[8][1] - height * 0.5 * cos_value;
			shape->shape_points[6][0] = shape->shape_points[7][0] + (shape->shape_points[7][0] - shape->shape_points[0][0]);
			shape->shape_points[6][1] = shape->shape_points[7][1] + (shape->shape_points[7][1] - shape->shape_points[0][1]);
			break;
		case 1:
			width = fabs((x - shape->shape_points[5][0]) * cos_value
				+ (y - shape->shape_points[5][1]) * sin_value);
			before_rotate[0][1] = (shape->shape_points[0][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[0][0] - shape->shape_points[8][0]) * sin_value;
			before_rotate[1][1] = (shape->shape_points[4][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[4][0] - shape->shape_points[8][0]) * sin_value;
			height = fabs(before_rotate[1][1] - before_rotate[0][1]);
			shape->shape_points[1][0] = shape->shape_points[5][0] - width * cos_value;
			shape->shape_points[1][1] = shape->shape_points[5][1] - width * sin_value;
			shape->shape_points[8][0] = (shape->shape_points[1][0] + shape->shape_points[5][0]) * 0.5;
			shape->shape_points[8][1] = (shape->shape_points[1][1] + shape->shape_points[5][1]) * 0.5;
			shape->shape_points[0][0] = shape->shape_points[8][0] - width * 0.5 * cos_value + height * 0.5 * sin_value;
			shape->shape_points[0][1] = shape->shape_points[8][1] - height * 0.5 * cos_value - width * 0.5 * sin_value;
			shape->shape_points[2][0] = shape->shape_points[1][0] + (shape->shape_points[1][0] - shape->shape_points[0][0]);
			shape->shape_points[2][1] = shape->shape_points[1][1] + (shape->shape_points[1][1] - shape->shape_points[0][1]);
			shape->shape_points[3][0] = (shape->shape_points[2][0] + shape->shape_points[4][0]) * 0.5;
			shape->shape_points[3][1] = (shape->shape_points[2][1] + shape->shape_points[4][1]) * 0.5;
			shape->shape_points[7][0] = (shape->shape_points[0][0] + shape->shape_points[6][0]) * 0.5;
			shape->shape_points[7][1] = (shape->shape_points[0][1] + shape->shape_points[6][1]) * 0.5;
			break;
		case 2:
			shape->shape_points[2][0] = x;
			shape->shape_points[2][1] = y;
			shape->shape_points[8][0] = (shape->shape_points[2][0] + shape->shape_points[6][0]) * 0.5;
			shape->shape_points[8][1] = (shape->shape_points[2][1] + shape->shape_points[6][1]) * 0.5;
			before_rotate[0][0] = (shape->shape_points[2][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[2][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[0][1] = (shape->shape_points[2][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[2][0] - shape->shape_points[8][0]) * sin_value;
			before_rotate[1][0] = (shape->shape_points[6][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[6][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[1][1] = (shape->shape_points[6][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[6][0] - shape->shape_points[8][0]) * sin_value;
			width = fabs(before_rotate[1][0] - before_rotate[0][0]);
			height = fabs(before_rotate[1][1] - before_rotate[0][1]);
			shape->shape_points[1][0] = shape->shape_points[8][0] - width * 0.5 * cos_value;
			shape->shape_points[1][1] = shape->shape_points[8][1] - width * 0.5 * sin_value;
			shape->shape_points[0][0] = shape->shape_points[1][0] + (shape->shape_points[1][0] - shape->shape_points[2][0]);
			shape->shape_points[0][1] = shape->shape_points[1][1] + (shape->shape_points[1][1] - shape->shape_points[2][1]);
			shape->shape_points[3][0] = shape->shape_points[8][0] - height * 0.5 * sin_value;
			shape->shape_points[3][1] = shape->shape_points[8][1] + height * 0.5 * cos_value;
			shape->shape_points[4][0] = shape->shape_points[3][0] + (shape->shape_points[3][0] - shape->shape_points[2][0]);
			shape->shape_points[4][1] = shape->shape_points[3][1] + (shape->shape_points[3][1] - shape->shape_points[2][1]);
			shape->shape_points[5][0] = shape->shape_points[8][0] + width * 0.5 * cos_value;
			shape->shape_points[5][1] = shape->shape_points[8][1] + width * 0.5 * sin_value;
			shape->shape_points[7][0] = (shape->shape_points[0][0] + shape->shape_points[6][0]) * 0.5;
			shape->shape_points[7][1] = (shape->shape_points[0][1] + shape->shape_points[6][1]) * 0.5;
			break;
		case 3:
			before_rotate[0][0] = (shape->shape_points[0][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[0][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[1][0] = (shape->shape_points[6][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[6][1] - shape->shape_points[8][1]) * sin_value;
			width = fabs(before_rotate[0][0] - before_rotate[1][0]);
			height = fabs((y - shape->shape_points[7][1] * cos_value
				- (x - shape->shape_points[7][0]) * sin_value));
			shape->shape_points[3][0] = shape->shape_points[7][0] - height * sin_value;
			shape->shape_points[3][1] = shape->shape_points[7][1] + height * cos_value;
			shape->shape_points[8][0] = (shape->shape_points[3][0] + shape->shape_points[7][0]) * 0.5;
			shape->shape_points[8][1] = (shape->shape_points[3][1] + shape->shape_points[7][1]) * 0.5;
			shape->shape_points[1][0] = shape->shape_points[8][0] - width * 0.5 * cos_value;
			shape->shape_points[1][1] = shape->shape_points[8][1] - width * 0.5 * sin_value;
			shape->shape_points[2][0] = shape->shape_points[1][0] + (shape->shape_points[1][0] - shape->shape_points[0][0]);
			shape->shape_points[2][1] = shape->shape_points[1][1] + (shape->shape_points[1][1] - shape->shape_points[0][1]);
			shape->shape_points[5][0] = shape->shape_points[8][0] + width * 0.5 * cos_value;
			shape->shape_points[5][1] = shape->shape_points[8][1] + width * 0.5 * sin_value;
			shape->shape_points[4][0] = shape->shape_points[5][0] + (shape->shape_points[5][0] - shape->shape_points[6][0]);
			shape->shape_points[4][1] = shape->shape_points[5][1] + (shape->shape_points[5][1] - shape->shape_points[6][1]);
			break;
		case 4:
			shape->shape_points[4][0] = x;
			shape->shape_points[4][1] = y;
			shape->shape_points[8][0] = (shape->shape_points[0][0] + shape->shape_points[4][0]) * 0.5;
			shape->shape_points[8][1] = (shape->shape_points[0][1] + shape->shape_points[4][1]) * 0.5;
			before_rotate[0][0] = (shape->shape_points[0][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[0][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[0][1] = (shape->shape_points[0][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[0][0] - shape->shape_points[8][0]) * sin_value;
			before_rotate[1][0] = (shape->shape_points[4][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[4][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[1][1] = (shape->shape_points[4][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[4][0] - shape->shape_points[8][0]) * sin_value;
			width = fabs(before_rotate[1][0] - before_rotate[0][0]);
			height = fabs(before_rotate[1][1] - before_rotate[0][1]);
			shape->shape_points[1][0] = shape->shape_points[8][0] - width * 0.5 * cos_value;
			shape->shape_points[1][1] = shape->shape_points[8][1] - width * 0.5 * sin_value;
			shape->shape_points[2][0] = shape->shape_points[1][0] + (shape->shape_points[1][0] - shape->shape_points[0][0]);
			shape->shape_points[2][1] = shape->shape_points[1][1] + (shape->shape_points[1][1] - shape->shape_points[0][1]);
			shape->shape_points[3][0] = shape->shape_points[8][0] - height * 0.5 * sin_value;
			shape->shape_points[3][1] = shape->shape_points[8][1] + height * 0.5 * cos_value;
			shape->shape_points[5][0] = shape->shape_points[8][0] + width * 0.5 * cos_value;
			shape->shape_points[5][1] = shape->shape_points[8][1] + width * 0.5 * sin_value;
			shape->shape_points[7][0] = shape->shape_points[8][0] + height * 0.5 * sin_value;
			shape->shape_points[7][1] = shape->shape_points[8][1] - height * 0.5 * cos_value;
			shape->shape_points[6][0] = shape->shape_points[7][0] + (shape->shape_points[7][0] - shape->shape_points[0][0]);
			shape->shape_points[6][1] = shape->shape_points[7][1] + (shape->shape_points[7][1] - shape->shape_points[0][1]);
			break;
		case 5:
			width = fabs((x - shape->shape_points[1][0]) * cos_value
				+ (y - shape->shape_points[1][1]) * sin_value);
			before_rotate[0][1] = (shape->shape_points[0][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[0][0] - shape->shape_points[8][0]) * sin_value;
			before_rotate[1][1] = (shape->shape_points[2][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[2][0] - shape->shape_points[8][0]) * sin_value;
			height = fabs(before_rotate[1][1] - before_rotate[0][1]);
			shape->shape_points[5][0] = shape->shape_points[1][0] + width * cos_value;
			shape->shape_points[5][1] = shape->shape_points[1][1] + width * sin_value;
			shape->shape_points[8][0] = (shape->shape_points[1][0] + shape->shape_points[5][0]) * 0.5;
			shape->shape_points[8][1] = (shape->shape_points[1][1] + shape->shape_points[5][1]) * 0.5;
			shape->shape_points[3][0] = shape->shape_points[8][0] - height * 0.5 * sin_value;
			shape->shape_points[3][1] = shape->shape_points[8][1] + height * 0.5 * cos_value;
			shape->shape_points[4][0] = shape->shape_points[3][0] + (shape->shape_points[3][0] - shape->shape_points[2][0]);
			shape->shape_points[4][1] = shape->shape_points[3][1] + (shape->shape_points[3][1] - shape->shape_points[2][1]);
			shape->shape_points[7][0] = shape->shape_points[8][0] + height * 0.5 * sin_value;
			shape->shape_points[7][1] = shape->shape_points[8][1] - height * 0.5 * cos_value;
			shape->shape_points[6][0] = shape->shape_points[7][0] + (shape->shape_points[7][0] - shape->shape_points[0][0]);
			shape->shape_points[6][1] = shape->shape_points[7][1] + (shape->shape_points[7][1] - shape->shape_points[0][1]);
			break;
		case 6:
			shape->shape_points[6][0] = x;
			shape->shape_points[6][1] = y;
			shape->shape_points[8][0] = (shape->shape_points[2][0] + shape->shape_points[6][0]) * 0.5;
			shape->shape_points[8][1] = (shape->shape_points[2][1] + shape->shape_points[6][1]) * 0.5;
			before_rotate[0][0] = (shape->shape_points[2][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[2][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[0][1] = (shape->shape_points[2][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[2][0] - shape->shape_points[8][0]) * sin_value;
			before_rotate[1][0] = (shape->shape_points[6][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[6][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[1][1] = (shape->shape_points[6][1] - shape->shape_points[8][1]) * cos_value
				- (shape->shape_points[6][0] - shape->shape_points[8][0]) * sin_value;
			width = fabs(before_rotate[1][0] - before_rotate[0][0]);
			height = fabs(before_rotate[1][1] - before_rotate[0][1]);
			shape->shape_points[1][0] = shape->shape_points[8][0] - width * 0.5 * cos_value;
			shape->shape_points[1][1] = shape->shape_points[8][1] - width * 0.5 * sin_value;
			shape->shape_points[0][0] = shape->shape_points[1][0] + (shape->shape_points[1][0] - shape->shape_points[2][0]);
			shape->shape_points[0][1] = shape->shape_points[1][1] + (shape->shape_points[1][1] - shape->shape_points[2][1]);
			shape->shape_points[3][0] = shape->shape_points[8][0] - height * 0.5 * sin_value;
			shape->shape_points[3][1] = shape->shape_points[8][1] + height * 0.5 * cos_value;
			shape->shape_points[4][0] = shape->shape_points[3][0] + (shape->shape_points[3][0] - shape->shape_points[2][0]);
			shape->shape_points[4][1] = shape->shape_points[3][1] + (shape->shape_points[3][1] - shape->shape_points[2][1]);
			shape->shape_points[5][0] = (shape->shape_points[4][0] + shape->shape_points[6][0]) * 0.5;
			shape->shape_points[5][1] = (shape->shape_points[4][1] + shape->shape_points[6][1]) * 0.5;
			shape->shape_points[7][0] = (shape->shape_points[0][0] + shape->shape_points[6][0]) * 0.5;
			shape->shape_points[7][1] = (shape->shape_points[0][1] + shape->shape_points[6][1]) * 0.5;
			break;
		case 7:
			before_rotate[0][0] = (shape->shape_points[2][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[2][1] - shape->shape_points[8][1]) * sin_value;
			before_rotate[1][0] = (shape->shape_points[4][0] - shape->shape_points[8][0]) * cos_value
				+ (shape->shape_points[4][1] - shape->shape_points[8][1]) * sin_value;
			width = fabs(before_rotate[0][0] - before_rotate[1][0]);
			height = fabs((y - shape->shape_points[3][1] * cos_value
				- (x - shape->shape_points[3][0]) * sin_value));
			shape->shape_points[7][0] = shape->shape_points[3][0] + height * sin_value;
			shape->shape_points[7][1] = shape->shape_points[3][1] - height * cos_value;
			shape->shape_points[8][0] = (shape->shape_points[3][0] + shape->shape_points[7][0]) * 0.5;
			shape->shape_points[8][1] = (shape->shape_points[3][1] + shape->shape_points[7][1]) * 0.5;
			shape->shape_points[1][0] = shape->shape_points[8][0] - width * 0.5 * cos_value;
			shape->shape_points[1][1] = shape->shape_points[8][1] - width * 0.5 * sin_value;
			shape->shape_points[0][0] = shape->shape_points[1][0] + (shape->shape_points[1][0] - shape->shape_points[2][0]);
			shape->shape_points[0][1] = shape->shape_points[1][1] + (shape->shape_points[1][1] - shape->shape_points[2][1]);
			shape->shape_points[5][0] = shape->shape_points[8][0] + width * 0.5 * cos_value;
			shape->shape_points[5][1] = shape->shape_points[8][1] + width * 0.5 * sin_value;
			shape->shape_points[6][0] = shape->shape_points[7][0] + (shape->shape_points[7][0] - shape->shape_points[0][0]);
			shape->shape_points[6][1] = shape->shape_points[7][1] + (shape->shape_points[7][1] - shape->shape_points[0][1]);
			break;
		case 8:
			{
				FLOAT_T diff[2] = {x-shape->shape_points[8][0], y-shape->shape_points[8][1]};
				for(i=0; i<8; i++)
				{
					shape->shape_points[i][0] += diff[0];
					shape->shape_points[i][1] += diff[1];
				}
				shape->shape_points[8][0] = (shape->shape_points[0][0] + shape->shape_points[4][0]) * 0.5;
				shape->shape_points[8][1] = (shape->shape_points[0][1] + shape->shape_points[4][1]) * 0.5;
			}
			break;
		}

		if(shape->control_point < 8)
		{
			shape->shape_points[8][0] = (shape->shape_points[0][0] + shape->shape_points[4][0]) * 0.5;
			shape->shape_points[8][1] = (shape->shape_points[0][1] + shape->shape_points[4][1]) * 0.5;
		}
		else if(shape->control_point == SHAPE_ROTATE)
		{
			if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_SQUARE)
			{
				((VECTOR_SQUARE*)layer->active_data)->rotate = - shape->before_shape.square.rotate
					+ atan2(y - shape->shape_points[8][1], x - shape->shape_points[8][0]);
			}
			else if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_RHOMBUS
				|| ((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_ECLIPSE)
			{
				((VECTOR_ECLIPSE*)layer->active_data)->rotate = - shape->before_shape.eclipse.rotate
					+ atan2(y - shape->shape_points[8][1], x - shape->shape_points[8][0]);
			}

			VectorShapeSetPoints((VECTOR_BASE_DATA*)layer->active_data, shape);
		}
		else if(shape->control_point == SHAPE_NEW)
		{
			shape->shape_points[0][0] = x;
			shape->shape_points[0][1] = y;
		}

		x_min = shape->shape_points[0][0], x_max = shape->shape_points[0][0];
		y_min = shape->shape_points[0][1], y_max = shape->shape_points[0][1];
		for(i=1; i<8; i++)
		{
			if(x_min > shape->shape_points[i][0])
			{
				x_min = shape->shape_points[i][0];
			}
			if(x_max < shape->shape_points[i][0])
			{
				x_max = shape->shape_points[i][0];
			}
			if(y_min > shape->shape_points[i][1])
			{
				y_min = shape->shape_points[i][1];
			}
			if(y_max < shape->shape_points[i][1])
			{
				y_max = shape->shape_points[i][1];
			}
		}

		switch(((VECTOR_BASE_DATA*)layer->active_data)->vector_type)
		{
		case VECTOR_TYPE_SQUARE:
			{
				VECTOR_SQUARE *square = (VECTOR_SQUARE*)layer->active_data;

				if(shape->control_point == SHAPE_NEW)
				{
					square->left = x_min;
					square->top = y_min;
					square->width = x_max - x_min;
					square->height = y_max - y_min;
				}
				else if(shape->control_point != SHAPE_ROTATE)
				{
					square->width = sqrt((shape->shape_points[0][0]-shape->shape_points[6][0])*(shape->shape_points[0][0]-shape->shape_points[6][0])
						+ (shape->shape_points[0][1]-shape->shape_points[6][1])*(shape->shape_points[0][1]-shape->shape_points[6][1]));
					square->height = sqrt((shape->shape_points[0][0]-shape->shape_points[2][0])*(shape->shape_points[0][0]-shape->shape_points[2][0])
						+ (shape->shape_points[0][1]-shape->shape_points[2][1])*(shape->shape_points[0][1]-shape->shape_points[2][1]));
					square->left = shape->shape_points[8][0] - square->width * 0.5;
					square->top = shape->shape_points[8][1] - square->height * 0.5;
				}
			}
			break;
		case VECTOR_TYPE_RHOMBUS:
		case VECTOR_TYPE_ECLIPSE:
			{
				VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)layer->active_data;

				if(shape->control_point == SHAPE_NEW)
				{
					width = x_max - x_min;
					height = y_max - y_min;

					eclipse->x = (x_max + x_min) * 0.5;
					eclipse->y = (y_max + y_min) * 0.5;
					eclipse->ratio = width / height;
					if(width >= height)
					{
						eclipse->radius = height * 0.5;
					}
					else
					{
						eclipse->radius = width * 0.5;
					}
				}
				else if(shape->control_point != SHAPE_ROTATE)
				{
					eclipse->x = (x_max + x_min) * 0.5;
					eclipse->y = (y_max + y_min) * 0.5;

					width = sqrt((shape->shape_points[0][0]-shape->shape_points[6][0])*(shape->shape_points[0][0]-shape->shape_points[6][0])
						+ (shape->shape_points[0][1]-shape->shape_points[6][1])*(shape->shape_points[0][1]-shape->shape_points[6][1]));
					height = sqrt((shape->shape_points[0][0]-shape->shape_points[2][0])*(shape->shape_points[0][0]-shape->shape_points[2][0])
						+ (shape->shape_points[0][1]-shape->shape_points[2][1])*(shape->shape_points[0][1]-shape->shape_points[2][1]));
					eclipse->ratio = width / height;
					if(width >= height)
					{
						eclipse->radius = height * 0.5;
					}
					else
					{
						eclipse->radius = width * 0.5;
					}
				}
			}
			break;
		}

		if(shape->control_point == SHAPE_NEW)
		{
			layer->flags |= VECTOR_LAYER_RASTERIZE_TOP;
		}
		else
		{
			layer->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
		}
	}
	else
	{
		VECTOR_BASE_DATA *vector = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->base)->next;
		layer->active_data = NULL;

		while(vector != NULL && layer->active_data == NULL)
		{
			if(vector->vector_type > VECTOR_TYPE_SHAPE
				&& vector->vector_type < VECTOR_SHAPE_END)
			{
				VectorShapeSetPoints(vector, shape);

				switch(vector->vector_type)
				{
				case VECTOR_TYPE_SQUARE:
					{
						VECTOR_SQUARE *square = (VECTOR_SQUARE*)vector;

						for(i=0; i<9; i++)
						{
							if((shape->shape_points[i][0] - x) * (shape->shape_points[i][0] - x) + (shape->shape_points[i][1] - y) * (shape->shape_points[i][1] - y)
								<= (HIT_RANGE * HIT_RANGE) * window->rev_zoom)
							{
								layer->active_data = (void*)vector;
								shape->control_point = (uint8)(i+1);
								break;
							}
						}

						if(layer->active_data == NULL)
						{
							FLOAT_T check[2] = {x, y};
							if(IsPointInTriangle(check, shape->shape_points[0], shape->shape_points[8], shape->shape_points[6]) != FALSE)
							{
								layer->active_data = (void*)vector;
								shape->control_point = SHAPE_ROTATE;
							}
							if(layer->active_data == NULL && IsPointInTriangle(check, shape->shape_points[0], shape->shape_points[8], shape->shape_points[2]) != FALSE)
							{
								layer->active_data = (void*)vector;
								shape->control_point = SHAPE_ROTATE;
							}
							if(layer->active_data == NULL && IsPointInTriangle(check, shape->shape_points[2], shape->shape_points[8], shape->shape_points[4]) != FALSE)
							{
								layer->active_data = (void*)vector;
								shape->control_point = SHAPE_ROTATE;
							}
							if(layer->active_data == NULL && IsPointInTriangle(check, shape->shape_points[4], shape->shape_points[8], shape->shape_points[6]) != FALSE)
							{
								layer->active_data = (void*)vector;
								shape->control_point = SHAPE_ROTATE;
							}
						}

						if(layer->active_data != NULL)
						{
							shape->before_shape.square = *square;
							shape->before_shape.square.rotate += atan2(y - shape->shape_points[8][1], x - shape->shape_points[8][0]);
						}
					}
					break;
				case VECTOR_TYPE_RHOMBUS:
				case VECTOR_TYPE_ECLIPSE:
					{
						VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)vector;

						if(eclipse->ratio > 1.0)
						{
							for(i=0; i<9; i++)
							{
								if((shape->shape_points[i][0] - x) * (shape->shape_points[i][0] - x) + (shape->shape_points[i][1] - y) * (shape->shape_points[i][1] - y)
									<= (HIT_RANGE * HIT_RANGE) * window->rev_zoom)
								{
									layer->active_data = (void*)vector;
									shape->control_point = (uint8)(i+1);
									break;
								}
							}

							if(layer->active_data == NULL)
							{
								if(fabs(x - eclipse->x) <= eclipse->radius * eclipse->ratio
									&& fabs(y - eclipse->y) <= eclipse->radius)
								{
									layer->active_data = (void*)vector;
									shape->control_point = SHAPE_ROTATE;
								}
							}
						}
						else
						{
							FLOAT_T ratio = 1.0 / eclipse->ratio;

							for(i=0; i<9; i++)
							{
								if((shape->shape_points[i][0] - x) * (shape->shape_points[i][0] - x) + (shape->shape_points[i][1] - y) * (shape->shape_points[i][1] - y)
									<= (HIT_RANGE * HIT_RANGE) * window->rev_zoom)
								{
									layer->active_data = (void*)vector;
									shape->control_point = (uint8)(i+1);
									break;
								}
							}

							if(layer->active_data == NULL)
							{
								if(fabs(x - eclipse->x) <= eclipse->radius
									&& fabs(y - eclipse->y) <= eclipse->radius * ratio)
								{
									layer->active_data = (void*)vector;
									shape->control_point = SHAPE_ROTATE;
								}
							}
						}

						if(layer->active_data != NULL)
						{
							shape->before_shape.eclipse = *eclipse;
						}
					}
					break;
				}
			}
			else if(vector->vector_type == VECTOR_TYPE_SCRIPT)
			{
				if(vector->layer != NULL)
				{
					if(x >= vector->layer->x && x < vector->layer->x + vector->layer->width
						&& y >= vector->layer->y && y < vector->layer->y + vector->layer->height)
					{
						if(vector->layer->pixels[(int)(y - vector->layer->y)*vector->layer->stride
							+ (int)(x - vector->layer->x)*4 + 3] > 0)
						{
							layer->active_data = (void*)vector;
						}
					}
				}
			}

			vector = (VECTOR_BASE_DATA*)vector->next;
		}
	}
}

static void VectorShapeReleaseCallBack(
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
		VECTOR_SHAPE_BRUSH *shape = (VECTOR_SHAPE_BRUSH*)core->brush_data;
		VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;

		if(shape->add_vector != FALSE)
		{
			layer->flags |= (VECTOR_LAYER_RASTERIZE_TOP | VECTOR_LAYER_FIX_LINE);
			shape->add_vector = FALSE;

			if(shape->shape_type == VECTOR_TYPE_SQUARE)
			{
				VECTOR_SQUARE *square = (VECTOR_SQUARE*)layer->top_data;
				AddTopSquareHistory(window, window->active_layer,
					square, core->name);

				shape->shape_points[0][0] = square->left;
				shape->shape_points[0][1] = square->top;
				shape->shape_points[1][0] = square->left;
				shape->shape_points[1][1] = square->top + square->height * 0.5;
				shape->shape_points[2][0] = square->left;
				shape->shape_points[2][1] = square->top + square->height;
				shape->shape_points[3][0] = square->left + square->width * 0.5;
				shape->shape_points[3][1] = square->top + square->height;
				shape->shape_points[4][0] = square->left + square->width;
				shape->shape_points[4][1] = square->top + square->height;
				shape->shape_points[5][0] = square->left + square->width;
				shape->shape_points[5][1] = square->top + square->height * 0.5;
				shape->shape_points[6][0] = square->left + square->width;
				shape->shape_points[6][1] = square->top;
				shape->shape_points[7][0] = square->left + square->width * 0.5;
				shape->shape_points[7][1] = square->top;
				shape->shape_points[8][0] = shape->shape_points[3][0];
				shape->shape_points[8][1] = shape->shape_points[1][1];
			}
			else
			{
				VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)layer->top_data;
				AddTopEclipseHistory(window, window->active_layer,
					eclipse, core->name);

				shape->shape_points[0][0] = eclipse->x - eclipse->radius * eclipse->ratio;
				shape->shape_points[0][1] = eclipse->y - eclipse->radius;
				shape->shape_points[1][0] = eclipse->x - eclipse->radius * eclipse->ratio;
				shape->shape_points[1][1] = eclipse->y;
				shape->shape_points[2][0] = eclipse->x - eclipse->radius * eclipse->ratio;
				shape->shape_points[2][1] = eclipse->y + eclipse->radius;
				shape->shape_points[3][0] = eclipse->x;
				shape->shape_points[3][1] = eclipse->y + eclipse->radius;
				shape->shape_points[4][0] = eclipse->x + eclipse->radius * eclipse->ratio;
				shape->shape_points[4][1] = eclipse->y + eclipse->radius;
				shape->shape_points[5][0] = eclipse->x + eclipse->radius * eclipse->ratio;
				shape->shape_points[5][1] = eclipse->y;
				shape->shape_points[6][0] = eclipse->x + eclipse->radius * eclipse->ratio;
				shape->shape_points[6][1] = eclipse->y - eclipse->radius;
				shape->shape_points[7][0] = eclipse->x;
				shape->shape_points[7][1] = eclipse->y - eclipse->radius;
				shape->shape_points[8][0] = eclipse->x;
				shape->shape_points[8][1] = eclipse->y;
			}
		}
		else if(shape->changing_shape != FALSE)
		{
			AddChangeVectorShapeHistory(window, (VECTOR_BASE_DATA*)layer->active_data,
				(VECTOR_BASE_DATA*)&shape->before_shape, core->name);
			layer->flags |= (VECTOR_LAYER_RASTERIZE_ACTIVE | VECTOR_LAYER_FIX_LINE);
		}

		shape->changing_shape = FALSE;
	}
}

static void VectorShapeDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	VECTOR_SHAPE_BRUSH *shape = (VECTOR_SHAPE_BRUSH*)data;
	VECTOR_BASE_DATA *base_data = (VECTOR_BASE_DATA*)window->active_layer->layer_data.vector_layer_p->top_data;
	VECTOR_LAYER *layer = window->active_layer->layer_data.vector_layer_p;

	if(shape->mode != VECTOR_SHAPE_BRUSH_MODE_ADD)
	{
		if(layer->active_data != NULL
			&& ((VECTOR_BASE_DATA*)layer->active_data)->vector_type > VECTOR_TYPE_SHAPE
			&& ((VECTOR_BASE_DATA*)layer->active_data)->vector_type < VECTOR_SHAPE_END
		)
		{
			FLOAT_T x_min, x_max, y_min, y_max;
			int i;

			cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
			cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);

			x_min = shape->shape_points[0][0] - HIT_RANGE;
			y_min = shape->shape_points[0][1] - HIT_RANGE;
			x_max = shape->shape_points[0][0] + HIT_RANGE;
			y_max = shape->shape_points[0][1] + HIT_RANGE;
			cairo_arc(window->disp_temp->cairo_p,
				shape->shape_points[0][0] * window->zoom_rate, shape->shape_points[0][1] * window->zoom_rate, HIT_RANGE, 0, 2*G_PI);
			cairo_stroke(window->disp_temp->cairo_p);

			for(i=1; i<9; i++)
			{
				if(x_min > shape->shape_points[i][0] - HIT_RANGE)
				{
					x_min = shape->shape_points[i][0] - HIT_RANGE;
				}
				if(x_max < shape->shape_points[i][0] + HIT_RANGE)
				{
					x_max = shape->shape_points[i][0] + HIT_RANGE;
				}
				if(y_min > shape->shape_points[i][1] - HIT_RANGE)
				{
					y_min = shape->shape_points[i][1] - HIT_RANGE;
				}
				if(y_max < shape->shape_points[i][1] + HIT_RANGE)
				{
					y_max = shape->shape_points[i][1] + HIT_RANGE;
				}
				cairo_arc(window->disp_temp->cairo_p,
					shape->shape_points[i][0] * window->zoom_rate, shape->shape_points[i][1] * window->zoom_rate, HIT_RANGE, 0, 2*G_PI);
				cairo_stroke(window->disp_temp->cairo_p);
			}

			cairo_rectangle(window->disp_layer->cairo_p, x_min, y_min, x_max - x_min, y_max - y_min);
			cairo_clip(window->disp_layer->cairo_p);
		}
	}
}

static void VectorShapeChangeType(GtkWidget* combo, VECTOR_SHAPE_BRUSH* shape)
{
	shape->shape_type = (uint8)(gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) + VECTOR_TYPE_SHAPE + 1);
}

static void VectorShapeChangeLineJointType(GtkWidget* combo, VECTOR_SHAPE_BRUSH* shape)
{
	shape->line_joint = (uint8)(gtk_combo_box_get_active(GTK_COMBO_BOX(combo)));
}

static void VectorShapeChangeLineWidth(GtkAdjustment* slider, VECTOR_SHAPE_BRUSH* shape)
{
	shape->line_width = gtk_adjustment_get_value(slider);
}

static void VectorShapeSetLineColor(GtkWidget* color_chooser, VECTOR_SHAPE_BRUSH* shape)
{
	GdkColor color;

	gtk_color_button_get_color(GTK_COLOR_BUTTON(color_chooser), &color);
	shape->line_color[0] = (uint8)(color.red >> 8);
	shape->line_color[1] = (uint8)(color.green >> 8);
	shape->line_color[2] = (uint8)(color.blue >> 8);
	shape->line_color[3] = (uint8)(gtk_color_button_get_alpha(GTK_COLOR_BUTTON(color_chooser)) >> 8);
}

static void VectorShapeSetFillColor(GtkWidget* color_chooser, VECTOR_SHAPE_BRUSH* shape)
{
	GdkColor color;

	gtk_color_button_get_color(GTK_COLOR_BUTTON(color_chooser), &color);
	shape->fill_color[0] = (uint8)(color.red >> 8);
	shape->fill_color[1] = (uint8)(color.green >> 8);
	shape->fill_color[2] = (uint8)(color.blue >> 8);
	shape->fill_color[3] = (uint8)(gtk_color_button_get_alpha(GTK_COLOR_BUTTON(color_chooser)) >> 8);
}

static void VectorShapeSetChangeLineWidthMode(GtkWidget* button, VECTOR_SHAPE_BRUSH* shape)
{
	shape->change_line_width = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE) ? 1 : 0;
}

static void VectorShapeSetManuallySetMode(GtkWidget* button, VECTOR_SHAPE_BRUSH* shape)
{
	shape->manually_set = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE) ? 1 : 0;
}

static void VectorShapeManuallySetButtonClicked(GtkWidget* button, VECTOR_SHAPE_BRUSH* shape)
{
	APPLICATION *app = g_object_get_data(G_OBJECT(button), "app");
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	if(window == NULL)
	{
		return;
	}
	VectorShapeManuallySetDialog(window, shape, NULL);
}

static GtkWidget* CreateVectorShapeModeDetailUI(APPLICATION* app, VECTOR_SHAPE_BRUSH* shape, int mode)
{
#define UI_FONT_SIZE 8.0
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	GtkAdjustment *adjustment;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *layout;
	GtkWidget *control;
	GtkWidget *label;
	GdkColor color;
	char str[128];

	switch(mode)
	{
	case VECTOR_SHAPE_BRUSH_MODE_ADD:
#if GTK_MAJOR_VERSION <= 2
		control = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->tool_box.shape.square);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->tool_box.shape.rhombus);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->tool_box.shape.eclipse);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->menu.script);
#else
		control = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->tool_box.shape.square);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->tool_box.shape.rhombus);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->tool_box.shape.eclipse);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->menu.script);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(control), shape->shape_type - VECTOR_TYPE_SHAPE - 1);
		gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);
		(void)g_signal_connect(G_OBJECT(control), "changed",
			G_CALLBACK(VectorShapeChangeType), shape);

		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(shape->line_width, 0.1, 100, 1, 1, 0));
		control = SpinScaleNew(adjustment, app->labels->tool_box.line_width, 1);
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(VectorShapeChangeLineWidth), shape);
		gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION <= 2
		control = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->tool_box.bevel);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->tool_box.round);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->tool_box.miter);
#else
		control = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->tool_box.bevel);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->tool_box.round);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->tool_box.miter);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(control), shape->line_joint);
		gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);
		(void)g_signal_connect(G_OBJECT(control), "changed",
			G_CALLBACK(VectorShapeChangeLineJointType), shape);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.line_color);
		gtk_label_set_markup(GTK_LABEL(label), str);
		color.red = app->tool_window.color_chooser->rgb[0]
			| (app->tool_window.color_chooser->rgb[0] << 8);
		color.green = app->tool_window.color_chooser->rgb[1]
			| (app->tool_window.color_chooser->rgb[1] << 8);
		color.blue = app->tool_window.color_chooser->rgb[2]
			| (app->tool_window.color_chooser->rgb[2] << 8);
		control = gtk_color_button_new_with_color(&color);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(control), TRUE);
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(control),
			(shape->line_color[3] << 8) | shape->line_color[3]);
		(void)g_signal_connect(G_OBJECT(control), "color-set",
			G_CALLBACK(VectorShapeSetLineColor), shape);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.fill_color);
		gtk_label_set_markup(GTK_LABEL(label), str);
		color.red = app->tool_window.color_chooser->back_rgb[0]
			| (app->tool_window.color_chooser->back_rgb[0] << 8);
		color.green = app->tool_window.color_chooser->back_rgb[1]
			| (app->tool_window.color_chooser->back_rgb[1] << 8);
		color.blue = app->tool_window.color_chooser->back_rgb[2]
			| (app->tool_window.color_chooser->back_rgb[2] << 8);
		control = gtk_color_button_new_with_color(&color);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(control), TRUE);
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(control),
			(shape->fill_color[3] << 8) | shape->fill_color[3]);
		(void)g_signal_connect(G_OBJECT(control), "color-set",
			G_CALLBACK(VectorShapeSetFillColor), shape);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		control = gtk_button_new_with_label(app->labels->tool_box.manually_set);
		g_object_set_data(G_OBJECT(control), "app", app);
		gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 2);
		(void)g_signal_connect(G_OBJECT(control), "clicked",
			G_CALLBACK(VectorShapeManuallySetButtonClicked), shape);

		break;
	case VECTOR_SHAPE_BRUSH_MODE_TRANSFORM:
		control = gtk_check_button_new_with_label(app->labels->tool_box.change_line_width);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control), shape->change_line_width);
		(void)g_signal_connect(G_OBJECT(control), "toggled",
			G_CALLBACK(VectorShapeSetChangeLineWidthMode), shape);

		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(shape->line_width, 0.1, 100, 1, 1, 0));
		control = SpinScaleNew(adjustment, app->labels->tool_box.line_width, 1);
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(VectorShapeChangeLineWidth), shape);
		gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION <= 2
		control = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->tool_box.bevel);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->tool_box.round);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), app->labels->tool_box.miter);
#else
		control = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->tool_box.bevel);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->tool_box.round);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), app->labels->tool_box.miter);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(control), shape->line_joint);
		gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);
		(void)g_signal_connect(G_OBJECT(control), "changed",
			G_CALLBACK(VectorShapeChangeLineJointType), shape);

		control = gtk_check_button_new_with_label(app->labels->tool_box.manually_set);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control), shape->manually_set);
		(void)g_signal_connect(G_OBJECT(control), "toggled",
			G_CALLBACK(VectorShapeSetManuallySetMode), shape);
		gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);

		break;
	case VECTOR_SHAPE_BRUSH_MODE_CHANGE_COLOR:
		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.line_color);
		gtk_label_set_markup(GTK_LABEL(label), str);
		color.red = app->tool_window.color_chooser->rgb[0]
			| (app->tool_window.color_chooser->rgb[0] << 8);
		color.green = app->tool_window.color_chooser->rgb[1]
			| (app->tool_window.color_chooser->rgb[1] << 8);
		color.blue = app->tool_window.color_chooser->rgb[2]
			| (app->tool_window.color_chooser->rgb[2] << 8);
		control = gtk_color_button_new_with_color(&color);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(control), TRUE);
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(control),
			(shape->line_color[3] << 8) | shape->line_color[3]);
		(void)g_signal_connect(G_OBJECT(control), "color-set",
			G_CALLBACK(VectorShapeSetLineColor), shape);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		label = gtk_label_new("");
		(void)sprintf(str, "<span font_desc=\"%f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.fill_color);
		gtk_label_set_markup(GTK_LABEL(label), str);
		color.red = app->tool_window.color_chooser->back_rgb[0]
			| (app->tool_window.color_chooser->back_rgb[0] << 8);
		color.green = app->tool_window.color_chooser->back_rgb[1]
			| (app->tool_window.color_chooser->back_rgb[1] << 8);
		color.blue = app->tool_window.color_chooser->back_rgb[2]
			| (app->tool_window.color_chooser->back_rgb[2] << 8);
		control = gtk_color_button_new_with_color(&color);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(control), TRUE);
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(control),
			(shape->fill_color[3] << 8) | shape->fill_color[3]);
		(void)g_signal_connect(G_OBJECT(control), "color-set",
			G_CALLBACK(VectorShapeSetFillColor), shape);
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 0);

		break;
	}

	return vbox;
#undef UI_FONT_SIZE
}

static void VectorShapeSetMode(GtkWidget* button, VECTOR_SHAPE_BRUSH* shape)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		shape->mode = (uint8)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "mode"));
		gtk_widget_destroy(shape->detail_widget);
		shape->detail_widget = CreateVectorShapeModeDetailUI(
			(APPLICATION*)g_object_get_data(G_OBJECT(button), "app"), shape, shape->mode);
		gtk_container_add(GTK_CONTAINER(shape->detail_frame), shape->detail_widget);
		gtk_widget_show_all(shape->detail_widget);
	}
}

static GtkWidget* CreateVectorShapeDetailUI(APPLICATION* app, void* data)
{
	VECTOR_SHAPE_BRUSH *shape = (VECTOR_SHAPE_BRUSH*)data;
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *buttons[4];

	buttons[VECTOR_SHAPE_BRUSH_MODE_ADD] = gtk_radio_button_new_with_label(
		NULL, app->labels->unit.add);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[VECTOR_SHAPE_BRUSH_MODE_ADD], FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_ADD]), "mode", GINT_TO_POINTER(VECTOR_SHAPE_BRUSH_MODE_ADD));
	g_object_set_data(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_ADD]), "app", app);
	buttons[VECTOR_SHAPE_BRUSH_MODE_TRANSFORM] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[VECTOR_SHAPE_BRUSH_MODE_ADD])), app->fractal_labels->transform);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[VECTOR_SHAPE_BRUSH_MODE_TRANSFORM], FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_TRANSFORM]), "mode", GINT_TO_POINTER(VECTOR_SHAPE_BRUSH_MODE_TRANSFORM));
	g_object_set_data(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_TRANSFORM]), "app", app);
	buttons[VECTOR_SHAPE_BRUSH_MODE_CHANGE_COLOR] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[VECTOR_SHAPE_BRUSH_MODE_ADD])),
			app->labels->layer_window.blend_modes[LAYER_BLEND_HSL_COLOR]);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[VECTOR_SHAPE_BRUSH_MODE_CHANGE_COLOR], FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_CHANGE_COLOR]), "mode", GINT_TO_POINTER(VECTOR_SHAPE_BRUSH_MODE_CHANGE_COLOR));
	g_object_set_data(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_CHANGE_COLOR]), "app", app);
	buttons[VECTOR_SHAPE_BRUSH_MODE_DELETE] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[VECTOR_SHAPE_BRUSH_MODE_ADD])), app->labels->unit._delete);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[VECTOR_SHAPE_BRUSH_MODE_DELETE], FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_DELETE]), "mode", GINT_TO_POINTER(VECTOR_SHAPE_BRUSH_MODE_DELETE));
	g_object_set_data(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_DELETE]), "app", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[shape->mode]), TRUE);
	(void)g_signal_connect(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_ADD]), "toggled",
		G_CALLBACK(VectorShapeSetMode), shape);
	(void)g_signal_connect(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_TRANSFORM]), "toggled",
		G_CALLBACK(VectorShapeSetMode), shape);
	(void)g_signal_connect(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_CHANGE_COLOR]), "toggled",
		G_CALLBACK(VectorShapeSetMode), shape);
	(void)g_signal_connect(G_OBJECT(buttons[VECTOR_SHAPE_BRUSH_MODE_DELETE]), "toggled",
		G_CALLBACK(VectorShapeSetMode), shape);

	shape->line_color[0] = app->tool_window.color_chooser->rgb[0];
	shape->line_color[1] = app->tool_window.color_chooser->rgb[1];
	shape->line_color[2] = app->tool_window.color_chooser->rgb[2];

	shape->fill_color[0] = app->tool_window.color_chooser->back_rgb[0];
	shape->fill_color[1] = app->tool_window.color_chooser->back_rgb[1];
	shape->fill_color[2] = app->tool_window.color_chooser->back_rgb[2];

	shape->detail_frame = gtk_frame_new(app->labels->unit.detail);
	shape->detail_widget = CreateVectorShapeModeDetailUI(app, shape, shape->mode);
	gtk_box_pack_start(GTK_BOX(vbox), shape->detail_frame, FALSE, FALSE, 3);
	gtk_container_add(GTK_CONTAINER(shape->detail_frame), shape->detail_widget);

	if(window->active_layer->layer_data.vector_layer_p != NULL)
	{
		VectorShapeSetPoints((VECTOR_BASE_DATA*)window->active_layer->layer_data.vector_layer_p->top_data, shape);
	}

	return vbox;
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
	else if(StringCompareIgnoreCase(brush_type, "VECTOR_SHAPE") == 0)
	{
		VECTOR_SHAPE_BRUSH *shape;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*shape));
		(void)memset(core->brush_data, 0, sizeof(*shape));
		shape = (VECTOR_SHAPE_BRUSH*)core->brush_data;
		core->detail_data_size = sizeof(*shape);

		shape->shape_type = (uint8)IniFileGetInteger(file, section_name, "SHAPE_TYPE");
		shape->line_width = IniFileGetDouble(file, section_name, "LINE_WIDTH");
		shape->line_color[3] = (uint8)IniFileGetInteger(file, section_name, "LINE_FLOW");
		shape->fill_color[3] = (uint8)IniFileGetInteger(file, section_name, "FILL_FLOW");
		shape->line_joint = (uint8)IniFileGetInteger(file, section_name, "LINE_JOINT");
		shape->text_field_size[0] = IniFileGetInteger(file, section_name, "EDITOR_WIDTH");
		shape->text_field_size[1] = IniFileGetInteger(file, section_name, "EDITOR_HEIGHT");
		shape->brush_name = core->name;

		if(shape->shape_type < VECTOR_TYPE_SHAPE)
		{
			shape->shape_type = VECTOR_TYPE_SQUARE;
		}

		core->press_func = VectorShapePressCallBack;
		core->motion_func = VectorShapeMotionCallBack;
		core->release_func = VectorShapeReleaseCallBack;
		core->draw_cursor = VectorShapeDrawCursor;
		core->create_detail_ui = CreateVectorShapeDetailUI;

		core->brush_type = TYPE_VECTOR_SHAPE_BRUSH;
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
* WriteVectorBrushData�֐�                               *
* �x�N�g�����C���[�p�̃u���V�e�[�u���̃f�[�^�������o��   *
* ����                                                   *
* window	: �c�[���{�b�N�X�E�B���h�E                   *
* file_path	: �����o���t�@�C���̃p�X                     *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	����I��:0	���s:���̒l                              *
*********************************************************/
int WriteVectorBrushData(
	TOOL_WINDOW* window,
	const char* file_path,
	APPLICATION* app
)
{
	FILE* fp;
	INI_FILE_PTR file;
	char brush_section_name[256];
	char brush_name[1024];
	char *write_str;
	int brush_id = 1;
	int x, y;

	fp = fopen(file_path, "wb");

	// �t�@�C���I�[�v���Ɏ��s
	if(fp == NULL)
	{
		return -1;
	}

	file = CreateIniFile(fp, NULL, 0, INI_WRITE);

	// �����R�[�h����������
	IniFileAddString(file, "CODE", "CODE_TYPE", window->vector_brush_code);

	// �t�H���g�t�@�C��������������
	IniFileAddString(file, "FONT", "FONT_FILE", window->font_file);

	// �u���V�e�[�u���̓��e�������o��
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
				case TYPE_VECTOR_SHAPE_BRUSH:
					{
						VECTOR_SHAPE_BRUSH *shape =
							(VECTOR_SHAPE_BRUSH*)window->vector_brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "VECTOR_SHAPE");
						(void)IniFileAddInteger(file, brush_section_name, "SHAPE_TYPE", shape->shape_type, 10);
						(void)IniFileAddDouble(file, brush_section_name, "LINE_WIDTH", shape->line_width, 1);
						(void)IniFileAddInteger(file, brush_section_name, "LINE_FLOW", shape->line_color[3], 10);
						(void)IniFileAddInteger(file, brush_section_name, "FILL_FLOW", shape->fill_color[3], 10);
						(void)IniFileAddInteger(file, brush_section_name, "LINE_JOINT", shape->line_joint, 10);
						(void)IniFileAddInteger(file, brush_section_name, "EDITOR_WIDTH", shape->text_field_size[0], 10);
						(void)IniFileAddInteger(file, brush_section_name, "EDITOR_HEIGHT", shape->text_field_size[1], 10);
					}
					break;
				}

				brush_id++;
			}	// if(window->vector_brushes[y][x].name != NULL)
		}	// for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH; x++)
	}	// �u���V�e�[�u���̓��e�������o��
		// for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT; y++)

	WriteIniFile(file, (size_t (*)(void*, size_t, size_t, void*))fwrite);
	file->delete_func(file);

	(void)fclose(fp);

	return 0;
}

#ifdef __cplusplus
}
#endif
