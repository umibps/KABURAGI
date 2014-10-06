// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <time.h>
#include <gtk/gtk.h>
#include "application.h"
#include "fractal.h"
#include "fractal_label.h"
#include "fractal_color_map.h"
#include "fractal_editor.h"
#include "spin_scale.h"
#include "color.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REVERSE_THRESHOLD 25

#define ROUND(X) (int)((X) + 0.5)
#define TRUNC(X) (int)((X))

static void GetColorFromColorMap(uint8 color_map[256][3], FLOAT_T index, uint8 color[3])
{
	int int_index = TRUNC(index * 255);
	color[0] = color_map[int_index][0];
	color[1] = color_map[int_index][1];
	color[2] = color_map[int_index][2];
}

void FractalEditorUpdateFlame(FRACTAL_EDITOR_WIDGET* editor)
{
	FractalPointGetXForms(&editor->fractal->control_point,
		editor->main_triangles, editor->fractal->transforms);
	if((editor->fractal->flags & FRACTAL_FLAG_CHECK_PRESERVE) == 0)
	{
		FractalPointComputeWeights(&editor->fractal->control_point,
			editor->main_triangles, editor->fractal->transforms);
		editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;
		gtk_adjustment_set_value(editor->weight,
			editor->fractal->control_point.xform[editor->selected_triangle].density * 100);
		editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);
	}

	FractalRender(editor->fractal, editor->fractal->random_start);

	gtk_widget_queue_draw(editor->preview);
}

void ResizeFractalEditor(FRACTAL_EDITOR_WIDGET* editor, int new_width, int new_height)
{
	ResizeFractal(editor->fractal, new_width, new_height);
	FractalEditorSetZoom(editor, new_width, new_height);

	FractalPointGetXForms(&editor->fractal->control_point,
		editor->main_triangles, editor->fractal->transforms);
	if((editor->fractal->flags & FRACTAL_FLAG_CHECK_PRESERVE) == 0)
	{
		FractalPointComputeWeights(&editor->fractal->control_point,
			editor->main_triangles, editor->fractal->transforms);
	}

	FractalRender(editor->fractal, editor->fractal->random_start);
}

void FractalEditorAutoZoom(FRACTAL_EDITOR_WIDGET* editor)
{
	GtkAllocation allocation;
	FLOAT_T x_min_z, y_min_z, x_max_z, y_max_z;
	int i, j;

	gtk_widget_get_allocation(editor->graph, &allocation);

	x_min_z = y_min_z = x_max_z = y_max_z = 0;
	for(i=0; i<=editor->fractal->transforms; i++)
	{
		for(j=0; j<3; j++)
		{
			if(editor->main_triangles[i][j][0] < x_min_z)
			{
				x_min_z = editor->main_triangles[i][j][0];
			}
			if(editor->main_triangles[i][j][1] < y_min_z)
			{
				y_min_z = editor->main_triangles[i][j][1];
			}
			if(editor->main_triangles[i][j][0] > x_max_z)
			{
				x_max_z = editor->main_triangles[i][j][0];
			}
			if(editor->main_triangles[i][j][1] > y_max_z)
			{
				x_max_z = editor->main_triangles[i][j][1];
			}
		}
	}

	editor->x_length = x_max_z - x_min_z;
	editor->y_length = y_max_z - y_min_z;
	editor->center_x = x_min_z + editor->x_length / 2;
	editor->center_y = y_min_z + editor->y_length / 2;
	if(editor->x_length >= editor->y_length)
	{
		editor->graph_zoom = allocation.width / 60 / editor->x_length;
	}
	else
	{
		editor->graph_zoom = allocation.height / 60 / editor->y_length;
	}
}

void FractalEditorSetZoom(FRACTAL_EDITOR_WIDGET* editor, int width, int height)
{
	FLOAT_T x_min_z, y_min_z, x_max_z, y_max_z;
	int i, j;

	x_min_z = y_min_z = x_max_z = y_max_z = 0;
	for(i=0; i<=editor->fractal->transforms; i++)
	{
		for(j=0; j<3; j++)
		{
			if(editor->main_triangles[i][j][0] < x_min_z)
			{
				x_min_z = editor->main_triangles[i][j][0];
			}
			if(editor->main_triangles[i][j][1] < y_min_z)
			{
				y_min_z = editor->main_triangles[i][j][1];
			}
			if(editor->main_triangles[i][j][0] > x_max_z)
			{
				x_max_z = editor->main_triangles[i][j][0];
			}
			if(editor->main_triangles[i][j][1] > y_max_z)
			{
				x_max_z = editor->main_triangles[i][j][1];
			}
		}
	}

	editor->x_length = x_max_z - x_min_z;
	editor->y_length = y_max_z - y_min_z;
	editor->center_x = x_min_z + editor->x_length / 2;
	editor->center_y = y_min_z + editor->y_length / 2;
	if(editor->x_length >= editor->y_length)
	{
		editor->graph_zoom = width / 60 / editor->x_length;
	}
	else
	{
		editor->graph_zoom = height / 60 / editor->y_length;
	}
}

static void TriangleCenterOfGravity(
	FLOAT_T ax,
	FLOAT_T ay,
	FLOAT_T bx,
	FLOAT_T by,
	FLOAT_T cx,
	FLOAT_T cy,
	FLOAT_T* x,
	FLOAT_T* y
)
{
	*x = (ax + bx + cx) / 3.0;
	*y = (ay + by + cy) / 3.0;
}

static void AutoZoom(FRACTAL_EDITOR_WIDGET* editor)
{
	FractalEditorAutoZoom(editor);
	gtk_widget_queue_draw(editor->graph);
}

static void SetTrianglePoints(FRACTAL_EDITOR_WIDGET* editor)
{
	int i;
	for(i=0; i<3; i++)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[i][0]),
			editor->main_triangles[editor->selected_triangle][i][0]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[i][1]),
			editor->main_triangles[editor->selected_triangle][i][1]);
	}
}

static void AddTriangle(FRACTAL_EDITOR_WIDGET* editor)
{
	char str[1024];
	int before_select = editor->selected_triangle;
	int i;

	editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;

	editor->fractal->transforms++;
	(void)sprintf(str, "%d", editor->fractal->transforms);
#if GTK_MAJOR_VERSION <= 2
	gtk_combo_box_append_text(GTK_COMBO_BOX(editor->transform), str);
#else
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(editor->transform), str);
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(editor->transform), editor->fractal->transforms-1);
	editor->selected_triangle = editor->fractal->transforms - 1;

	for(i=0; i<3; i++)
	{
		editor->main_triangles[editor->selected_triangle][i][0]
			= editor->main_triangles[before_select][i][0];
		editor->main_triangles[editor->selected_triangle][i][1]
			= editor->main_triangles[before_select][i][1];
	}

	editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);

	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}

	gtk_widget_queue_draw(editor->graph);
}

static void DeleteTriangle(FRACTAL_EDITOR_WIDGET* editor)
{
	int target = editor->selected_triangle;
	int i, j;

	editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;

#if GTK_MAJOR_VERSION <= 2
	gtk_combo_box_remove_text(GTK_COMBO_BOX(editor->transform),
		editor->fractal->transforms - 1);
#else
	gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(editor->transform),
		editor->fractal->transforms - 1);
#endif
	for(i=target; i<editor->fractal->transforms; i++)
	{
		for(j=0; j<3; j++)
		{
			editor->main_triangles[i][j][0] =
				editor->main_triangles[i+1][j][0];
			editor->main_triangles[i][j][1] =
				editor->main_triangles[i+1][j][1];
		}
	}
	editor->fractal->transforms--;
	if(editor->selected_triangle == editor->fractal->transforms)
	{
		editor->selected_triangle--;
	}

	SetTrianglePoints(editor);

	editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);

	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}

	gtk_widget_queue_draw(editor->graph);
}

static void FlipTriangleHorizontally(FRACTAL_EDITOR_WIDGET* editor)
{
	int i;
	for(i=0; i<3; i++)
	{
		editor->main_triangles[editor->selected_triangle][i][0]
			= - editor->main_triangles[editor->selected_triangle][i][0];
	}

	editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;

	SetTrianglePoints(editor);

	editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);

	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}

	gtk_widget_queue_draw(editor->graph);
}

static void FlipTriangleVertically(FRACTAL_EDITOR_WIDGET* editor)
{
	int i;
	for(i=0; i<3; i++)
	{
		editor->main_triangles[editor->selected_triangle][i][1]
			= - editor->main_triangles[editor->selected_triangle][i][1];
	}

	editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;

	SetTrianglePoints(editor);

	editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);

	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}

	gtk_widget_queue_draw(editor->graph);
}

static void FlipAllTriangleHorizontally(FRACTAL_EDITOR_WIDGET* editor)
{
	int i, j;

	for(i=0; i<editor->fractal->transforms; i++)
	{
		for(j=0; j<3; j++)
		{
			editor->main_triangles[editor->selected_triangle][i][0]
				= - editor->main_triangles[editor->selected_triangle][i][0];
		}
	}

	editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;

	SetTrianglePoints(editor);

	editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);

	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}

	gtk_widget_queue_draw(editor->graph);
}

static void FlipAllTriangleVertically(FRACTAL_EDITOR_WIDGET* editor)
{
	int i, j;

	for(i=0; i<editor->fractal->transforms; i++)
	{
		for(j=0; j<3; j++)
		{
			editor->main_triangles[editor->selected_triangle][i][1]
				= - editor->main_triangles[editor->selected_triangle][i][1];
		}
	}

	editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;

	SetTrianglePoints(editor);

	editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);

	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}

	gtk_widget_queue_draw(editor->graph);
}

static gboolean FractalEditorDrawGraph(
	GtkWidget* widget,
#if GTK_MAJOR_VERSION <= 2
	GdkEventExpose* event_info,
#else
	cairo_t* cairo_p,
#endif
	FRACTAL_EDITOR_WIDGET* editor
)
{
#if GTK_MAJOR_VERSION <= 2
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
#endif
	const double dash[] = {5, 5};
	GtkAllocation allocation;
	double color[3];
	uint8 byte_color[3];
	double font_size;
	char text[8];
	FLOAT_T ix, iy;
	FLOAT_T sc;
	FLOAT_T ax, ay, bx, by, cx, cy;
	int i;

	gtk_widget_get_allocation(widget, &allocation);
	ix = allocation.width / 2;
	iy = allocation.height / 2;
	sc = editor->graph_zoom * 50;
	font_size = (allocation.width <= allocation.height) ?
		allocation.width * 0.05 : allocation.height * 0.05;

	if((editor->fractal->flags & FRACTAL_FLAG_CHECK_FLAME_BACK) != 0)
	{
		color[0] = editor->fractal->control_point.back_bround[0] * DIV_PIXEL;
		color[1] = editor->fractal->control_point.back_bround[1] * DIV_PIXEL;
		color[2] = editor->fractal->control_point.back_bround[2] * DIV_PIXEL;
	}
	else
	{
		color[0] = editor->fractal->back_ground[0] * DIV_PIXEL;
		color[1] = editor->fractal->back_ground[1] * DIV_PIXEL;
		color[2] = editor->fractal->back_ground[2] * DIV_PIXEL;
	}
	cairo_rectangle(cairo_p, 0, 0, allocation.width, allocation.height);
	cairo_set_source_rgb(cairo_p, color[0], color[1], color[2]);
	cairo_paint(cairo_p);

	cairo_set_line_width(cairo_p, 1);
	cairo_set_dash(cairo_p, dash, 2, 0);
	cairo_set_source_rgb(cairo_p, editor->reference_color[0] * DIV_PIXEL,
		editor->reference_color[1] * DIV_PIXEL, editor->reference_color[2] * DIV_PIXEL);
	cairo_move_to(cairo_p, ix + editor->main_triangles[0][0][0] * sc - editor->center_x * sc,
		iy + (editor->center_y + - editor->main_triangles[0][0][1]) * sc);
	cairo_line_to(cairo_p, ix + editor->main_triangles[0][1][0] * sc - editor->center_x * sc,
		iy + (editor->center_y + - editor->main_triangles[0][1][1]) * sc);
	cairo_line_to(cairo_p, ix + editor->main_triangles[0][2][0] * sc - editor->center_x * sc,
		iy + (editor->center_y + - editor->main_triangles[0][2][1]) * sc);
	cairo_line_to(cairo_p, ix + editor->main_triangles[0][0][0] * sc - editor->center_x * sc,
		iy + (editor->center_y + - editor->main_triangles[0][0][1]) * sc);
	cairo_stroke(cairo_p);

	cairo_set_font_size(cairo_p, font_size);
	ax = ix + editor->main_triangles[0][0][0] * sc - editor->center_x * sc;
	ay = iy + (editor->center_y + - editor->main_triangles[0][0][1]) * sc;
	cairo_move_to(cairo_p, ax, ay);
	cairo_show_text(cairo_p, "A");
	bx = ix + editor->main_triangles[0][1][0] * sc - editor->center_x * sc;
	by = iy + (editor->center_y + - editor->main_triangles[0][1][1]) * sc;
	cairo_move_to(cairo_p, bx, by);
	cairo_show_text(cairo_p, "B");
	cx = ix + editor->main_triangles[0][2][0] * sc - editor->center_x * sc;
	cy = iy + (editor->center_y + - editor->main_triangles[0][2][1]) * sc;
	cairo_move_to(cairo_p, cx, cy);
	cairo_show_text(cairo_p, "C");
	TriangleCenterOfGravity(ax, ay, bx, by, cx, cy, &ax, &ay);
	cairo_move_to(cairo_p, ax, ay);
	cairo_show_text(cairo_p, "1");

	for(i=1; i<editor->fractal->transforms; i++)
	{
		ax = ix + editor->main_triangles[i][0][0] * sc - editor->center_x * sc;
		ay = iy + (editor->center_y + - editor->main_triangles[i][0][1]) * sc;
		bx = ix + editor->main_triangles[i][1][0] * sc - editor->center_x * sc;
		by = iy + (editor->center_y + - editor->main_triangles[i][1][1]) * sc;
		cx = ix + editor->main_triangles[i][2][0] * sc - editor->center_x * sc;
		cy = iy + (editor->center_y + - editor->main_triangles[i][2][1]) * sc;
		if((editor->fractal->flags & FRACTAL_FLAG_USE_XFORM_COLOR) != 0)
		{
			GetColorFromColorMap(editor->fractal->control_point.color_map,
				editor->fractal->control_point.xform[i].color, byte_color);
			color[0] = byte_color[0] * DIV_PIXEL;
			color[1] = byte_color[1] * DIV_PIXEL;
			color[2] = byte_color[2] * DIV_PIXEL;
		}
		else
		{
			switch(i-1)
			{
			case 0:
				color[0] = 255;
				color[1] = 255;
				color[2] = 0;
				break;
			case 1:
				color[0] = 0;
				color[1] = 128;
				color[2] = 255;
				break;
			default:
				GetColor((eCOLOR)(i % (NUM_COLOR-1) + 1), byte_color);
				color[0] = byte_color[0];
				color[1] = byte_color[1];
				color[2] = byte_color[2];
			}
		}

		if(i == editor->selected_triangle)
		{
			cairo_set_dash(cairo_p, NULL, 0, 0);
		}
		else
		{
			cairo_set_dash(cairo_p, dash, 2, 0);
		}
		cairo_set_source_rgb(cairo_p, color[0] * DIV_PIXEL,
			color[1] * DIV_PIXEL, color[2] * DIV_PIXEL);

		cairo_move_to(cairo_p, ax, ay);
		cairo_line_to(cairo_p, bx, by);
		cairo_line_to(cairo_p, cx, cy);
		cairo_line_to(cairo_p, ax, ay);
		cairo_stroke(cairo_p);

		cairo_set_dash(cairo_p, NULL, 0, 0);
		cairo_arc(cairo_p, ax, ay, 4, 0, G_PI * 2);
		cairo_stroke(cairo_p);
		cairo_arc(cairo_p, bx, by, 4, 0, G_PI * 2);
		cairo_stroke(cairo_p);
		cairo_arc(cairo_p, cx, cy, 4, 0, G_PI * 2);
		cairo_stroke(cairo_p);

		cairo_move_to(cairo_p, ax, ay);
		cairo_show_text(cairo_p, "A");
		cairo_move_to(cairo_p, bx, by);
		cairo_show_text(cairo_p, "B");
		cairo_move_to(cairo_p, cx, cy);
		cairo_show_text(cairo_p, "C");

		TriangleCenterOfGravity(ax, ay, bx, by, cx, cy, &ax, &ay);
		cairo_move_to(cairo_p, ax, ay);
		(void)sprintf(text, "%d", i+1);
		cairo_show_text(cairo_p, text);
	}

#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

static gboolean FractalEditorGraphConfigure(
	GtkWidget* widget,
	GdkEventConfigure* event_info,
	FRACTAL_EDITOR_WIDGET* editor
)
{
	FractalEditorAutoZoom(editor);

	return FALSE;
}

static void FractalEditorScale(
	FRACTAL_EDITOR_WIDGET* editor,
	FLOAT_T* fx,
	FLOAT_T* fy,
	FLOAT_T x,
	FLOAT_T y,
	int width,
	int height
)
{
	FLOAT_T sc = editor->graph_zoom * 50;
	*fx = (x - (width * 0.5)) / sc + editor->center_x;
	*fy = - ((y - (height * 0.5)) / sc - editor->center_y);
}

static int FractalEditorInsideTriangle(
	FRACTAL_EDITOR_WIDGET* editor,
	FLOAT_T x,
	FLOAT_T y
)
{
	int i, j, k;

	j = 2;
	for(k=0; k<editor->fractal->transforms; k++)
	{
		for(i=0; i<3; i++)
		{
			if(((editor->main_triangles[k][i][1] <= y
				&& y < editor->main_triangles[k][j][1])
				||
				(editor->main_triangles[k][j][1] <= y
				&& y < editor->main_triangles[k][i][1]))
				&& x < (editor->main_triangles[k][j][0] - editor->main_triangles[k][i][0])
					* (y - editor->main_triangles[k][i][1]) / (editor->main_triangles[k][j][1]
					- editor->main_triangles[k][i][1]) + editor->main_triangles[k][i][0]
			)
			{
				return k;
			}
			j = i;
		}
	}

	return -1;
}

static int FractalEditorInTriangle(
	FRACTAL_EDITOR_WIDGET* editor,
	FLOAT_T x,
	FLOAT_T y
)
{
	FLOAT_T d;
	int in;
	int i, j;

	in = FractalEditorInsideTriangle(editor, x, y);
	if(in >= 0)
	{
		return in;
	}

	for(i=0; i<editor->fractal->transforms; i++)
	{
		for(j=0; j<3; j++)
		{
			d = FractalDistance(x, y, editor->main_triangles[i][j][0],
				editor->main_triangles[i][j][1]);
			if(d * editor->graph_zoom * 50 < 4)
			{
				in = i;
			}
		}
	}

	return in;
}

static gboolean FractalEditorGraphButtonPress(
	GtkWidget* widget,
	GdkEventButton* event_info,
	FRACTAL_EDITOR_WIDGET* editor
)
{
	GtkAllocation allocation;
	FLOAT_T fx, fy;
	FLOAT_T d;
	int i, j;

	gtk_widget_get_allocation(widget, &allocation);

	editor->before_x = event_info->x;
	editor->before_y = event_info->y;

	editor->triangle_caught = 0;
	editor->corner_caught = 0;

	FractalEditorScale(editor, &fx, &fy,
		event_info->x, event_info->y, allocation.width, allocation.height);

	if(event_info->button == 1)
	{
		if((editor->fractal->flags & FRACTAL_FLAG_SELECT_LOCK) != 0)
		{
			for(j=0; j<3; j++)
			{
				d = FractalDistance(fx, fy, editor->main_triangles[editor->selected_triangle][j][0],
					editor->main_triangles[editor->selected_triangle][j][1]);
				if(d * editor->graph_zoom * 50 < 4)
				{
					editor->triangle_caught = 1;
					editor->corner_caught = j + 1;
					editor->before_x = fx;
					editor->before_y = fy;
					break;
				}
			}
		}
		else
		{
			for(i=0; i<editor->fractal->transforms; i++)
			{
				for(j=0; j<3; j++)
				{
					d = FractalDistance(fx, fy, editor->main_triangles[i][j][0],
						editor->main_triangles[i][j][1]);
					if(d * editor->graph_zoom * 50 < 4)
					{
						editor->triangle_caught = i + 1;
						editor->corner_caught = j + 1;
						gtk_combo_box_set_active(GTK_COMBO_BOX(editor->transform), i);
						editor->before_x = event_info->x;
						editor->before_y = fx;
					}
				}
			}
		}

		if(editor->corner_caught == 0)
		{
			editor->triangle_caught = FractalEditorInsideTriangle(
				editor, fx, fy) + 1;
			if(editor->triangle_caught > 0)
			{
				editor->before_x = fx;
				editor->before_y = fy;
				for(i=0; i<3; i++)
				{
					editor->old_triangle[i][0] = editor->main_triangles[
						editor->triangle_caught-1][i][0];
					editor->old_triangle[i][1] = editor->main_triangles[
						editor->triangle_caught-1][i][1];
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(editor->transform),
					editor->triangle_caught - 1);
			}
		}
	}

	gtk_widget_queue_draw(widget);

	return TRUE;
}

static gboolean FractalEditorGraphMouseMove(
	GtkWidget* widget,
	GdkEventMotion* event_info,
	FRACTAL_EDITOR_WIDGET* editor
)
{
	FLOAT_T vx, vy;
	FLOAT_T fx, fy;
	FLOAT_T x, y;
	GtkAllocation allocation;
	GdkModifierType state;
	int i;

	if(event_info->is_hint != 0)
	{
		gint get_x, get_y;
		gdk_window_get_pointer(event_info->window, &get_x, &get_y, &state);
		x = get_x, y = get_y;
	}
	else
	{
		state = event_info->state;
		x = event_info->x;
		y = event_info->y;
	}
	gtk_widget_get_allocation(widget, &allocation);

	FractalEditorScale(editor, &fx, &fy,
		x, y, allocation.width, allocation.height);

	if(FractalEditorInTriangle(editor, x, y) >= -0)
	{
		GdkCursor *cursor = gdk_cursor_new(GDK_HAND1);
		gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
	}
	else
	{
		gdk_window_set_cursor(gtk_widget_get_window(widget), NULL);
	}

	if(editor->corner_caught > 0)
	{
		int corner = editor->corner_caught - 1;
		int triangle = editor->triangle_caught - 1;
		editor->main_triangles[triangle][corner][0] = fx;
		editor->main_triangles[triangle][corner][1] = fy;
		editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(
			editor->triangle_points[corner][0]), fx);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(
			editor->triangle_points[corner][1]), fy);
		editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);

		gtk_widget_queue_draw(widget);
	}
	else if(editor->triangle_caught > 0)
	{
		int triangle = editor->triangle_caught - 1;
		vx = editor->before_x - fx;
		vy = editor->before_y - fy;
		editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;
		for(i=0; i<3; i++)
		{
			editor->main_triangles[triangle][i][0] =
				editor->old_triangle[i][0] - vx;
			editor->main_triangles[triangle][i][1] =
				editor->old_triangle[i][1] - vy;
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[i][0]), editor->main_triangles[triangle][i][0]);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[i][1]), editor->main_triangles[triangle][i][1]);
		}
		editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);

		gtk_widget_queue_draw(widget);
	}

	return TRUE;
}

static gboolean FractalEditorGraphButtonRelease(
	GtkWidget* widget,
	GdkEventButton* event_info,
	FRACTAL_EDITOR_WIDGET* editor
)
{
	if(event_info->button == 1)
	{
		if(editor->triangle_caught > 0)
		{
			if(editor->update_immediately != FALSE)
			{
				FractalEditorUpdateFlame(editor);
			}
			editor->triangle_caught = 0;
			editor->corner_caught = 0;
		}
	}
	else if(event_info->button == 3)
	{
		GtkWidget *menu = gtk_menu_new();
		GtkWidget *menu_item;

		menu_item = gtk_menu_item_new_with_label(editor->labels->auto_zoom);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(AutoZoom), editor);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(editor->labels->add);
		gtk_widget_set_sensitive(menu_item,
			editor->fractal->transforms < editor->fractal->max_transforms);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(AddTriangle), editor);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(editor->labels->del);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(DeleteTriangle), editor);
		gtk_widget_set_sensitive(menu_item,
			editor->fractal->transforms > editor->fractal->min_transforms);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(editor->labels->flip_horizontally);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(FlipTriangleHorizontally), editor);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(editor->labels->flip_vertically);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(FlipTriangleVertically), editor);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(editor->labels->flip_horizon_all);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(FlipAllTriangleHorizontally), editor);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(editor->labels->flip_vertical_all);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(FlipAllTriangleVertically), editor);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
			NULL, event_info->button, event_info->time);

		gtk_widget_show_all(menu);
	}

	return TRUE;
}

static void OnFractalEditorDestroy(FRACTAL_EDITOR_WIDGET* editor)
{
	MEM_FREE_FUNC(editor);
}

static gboolean DisplayFractalPreview(
	GtkWidget* widget,
#if GTK_MAJOR_VERSION <= 2
	GdkEventExpose* event_info,
#else
	cairo_t* cairo_p,
#endif
	FRACTAL* fractal
)
{
#if GTK_MAJOR_VERSION <= 2
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
#endif
	GtkAllocation allocation;
	cairo_surface_t *surface_p;

	gtk_widget_get_allocation(widget, &allocation);
	if(allocation.width != fractal->image_width
		|| allocation.height != fractal->image_height)
	{
		if(allocation.width == 0 || allocation.height == 0)
		{
			return FALSE;
		}
		ResizeFractal(fractal, allocation.width, allocation.height);
		FractalRender(fractal, fractal->random_start);
	}

	cairo_set_operator(cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgb(cairo_p, 0, 0, 0);
	cairo_rectangle(cairo_p, 0, 0, allocation.width, allocation.height);
	cairo_fill(cairo_p);

	surface_p = cairo_image_surface_create_for_data((uint8*)fractal->pixels,
		CAIRO_FORMAT_ARGB32, fractal->image_width, fractal->image_height, fractal->image_width * 4);
	cairo_set_source_surface(cairo_p, surface_p, 0, 0);
	cairo_paint(cairo_p);

#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

static void ChangeSelectedTriangle(GtkWidget* combo, FRACTAL_EDITOR_WIDGET* editor)
{
	int i;

	if((editor->fractal->flags & FRACTAL_FLAG_UI_DISABLED) != 0)
	{
		return;
	}

	editor->selected_triangle = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[0][0]),
		editor->main_triangles[editor->selected_triangle][0][0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[0][1]),
		editor->main_triangles[editor->selected_triangle][0][1]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[1][0]),
		editor->main_triangles[editor->selected_triangle][2][0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[1][1]),
		editor->main_triangles[editor->selected_triangle][0][1]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[2][0]),
		editor->main_triangles[editor->selected_triangle][2][0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[2][1]),
		editor->main_triangles[editor->selected_triangle][2][1]);
	for(i=0; i<FRACTAL_NUM_VARS; i++)
	{
		gtk_adjustment_set_value(editor->variation[i],
			editor->fractal->control_point.xform[editor->selected_triangle].vars[i] * 100);
	}
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->a),
		editor->fractal->control_point.xform[editor->selected_triangle].c[0][0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->b),
		editor->fractal->control_point.xform[editor->selected_triangle].c[1][0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->c),
		editor->fractal->control_point.xform[editor->selected_triangle].c[0][1]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->d),
		editor->fractal->control_point.xform[editor->selected_triangle].c[1][1]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->e),
		editor->fractal->control_point.xform[editor->selected_triangle].c[2][0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->f),
		editor->fractal->control_point.xform[editor->selected_triangle].c[2][1]);
	editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);

	gtk_widget_queue_draw(editor->graph);
}

static void ChangeTrianglePoint(GtkWidget* spin_button, FRACTAL_EDITOR_WIDGET* editor)
{
	int point_index, is_y;
	if((editor->fractal->flags & FRACTAL_FLAG_UI_DISABLED) != 0)
	{
		return;
	}

	point_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "point-index"));
	is_y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "is_y"));

	editor->main_triangles[editor->selected_triangle][point_index][is_y] =
		gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button));

	gtk_widget_queue_draw(editor->graph);

	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void RandomResetFractalTrianglePoint(FRACTAL_EDITOR_WIDGET* editor)
{
	if((editor->fractal->flags & FRACTAL_FLAG_USE_TIME_SEED) != 0)
	{
		int seed = (int)time(NULL);
		editor->fractal->random_start = (editor->fractal->random_start == seed) ? seed+1 : seed;
	}
	else
	{
		editor->fractal->random_start = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editor->random_seed));
	}

	editor->selected_triangle = 0;
	FractalRandomizeControlPoint(editor->fractal,
		&editor->fractal->control_point, editor->fractal->min_transforms, editor->fractal->max_transforms, 0);
	FractalTrianglesFromControlPoint(
		editor->fractal, &editor->fractal->control_point, editor->main_triangles);
	FractalEditorAutoZoom(editor);
	FractalEditorResetWidgets(editor);
}

static void ChangeRandomSeedMode(GtkWidget* button, FRACTAL_EDITOR_WIDGET* editor)
{
	int active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	if(active == FALSE)
	{
		editor->fractal->flags &= ~(FRACTAL_FLAG_USE_TIME_SEED);
	}
	else
	{
		editor->fractal->flags |= FRACTAL_FLAG_USE_TIME_SEED;
	}

	gtk_widget_set_sensitive(editor->random_seed, !active);
}

static void ChangeFractalControlPoint(GtkWidget* spin_button, FRACTAL_EDITOR_WIDGET* editor)
{
	int x, y;

	if((editor->fractal->flags & FRACTAL_FLAG_UI_DISABLED) != 0)
	{
		return;
	}

	x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "x-index"));
	y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "y-index"));

	editor->fractal->control_point.xform[editor->selected_triangle].c[x][y]
		= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button));

	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalColor(GtkWidget* combo, FRACTAL_EDITOR_WIDGET* editor)
{
	FRACTAL *fractal = editor->fractal;
	int color_index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	if((editor->fractal->flags & FRACTAL_FLAG_UI_DISABLED) != 0)
	{
		return;
	}

	editor->fractal->control_point.color_map_index = color_index;
	FractalGetColorMap(color_index, fractal->control_point.color_map);

	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeRandomColorClicked(FRACTAL_EDITOR_WIDGET* editor)
{
	FRACTAL_SRAND(editor->color_random_seed++);
	gtk_combo_box_set_active(GTK_COMBO_BOX(editor->color_combo_box),
		FRACTAL_RAND() % FRACTAL_NUM_COLOR_MAP);
}

static GtkWidget* SelectColorWidgetNew(
	FRACTAL_EDITOR_WIDGET* editor,
	FRACTAL_LABEL* labels
)
{
#define PIXBUF_WIDTH 256 * 2
#define PIXBUF_HEIGHT 16
	FRACTAL *fractal = editor->fractal;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *combo;
	GtkWidget *control;
	GtkCellRenderer *renderer;
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *pixbuf;
	uint8 *pixels;
	uint8 color_map[256][3];
	int stride;
	int i, j, k;

	store = gtk_list_store_new(1, GDK_TYPE_PIXBUF);
	editor->color_combo_box =
		combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", 0, NULL);

	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

	for(i=0; i<FRACTAL_NUM_COLOR_MAP; i++)
	{
		src_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE,
			8, 256, PIXBUF_HEIGHT);
		stride = gdk_pixbuf_get_rowstride(src_pixbuf);
		pixels = gdk_pixbuf_get_pixels(src_pixbuf);

		FractalGetColorMap(i, color_map);
		for(j=0; j<256; j++)
		{
			for(k=0; k<PIXBUF_HEIGHT; k++)
			{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				pixels[k*stride + j*3 + 0] = color_map[j][2];
				pixels[k*stride + j*3 + 1] = color_map[j][1];
				pixels[k*stride + j*3 + 2] = color_map[j][0];
#else
				pixels[k*stride + j*3 + 0] = color_map[j][0];
				pixels[k*stride + j*3 + 1] = color_map[j][1];
				pixels[k*stride + j*3 + 2] = color_map[j][2];
#endif
			}
		}

		pixbuf = gdk_pixbuf_scale_simple(src_pixbuf, PIXBUF_WIDTH,
			PIXBUF_HEIGHT, GDK_INTERP_NEAREST);

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pixbuf, -1);

		g_object_unref(src_pixbuf);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), fractal->color_map_index);
	(void)g_signal_connect(G_OBJECT(combo), "changed",
		G_CALLBACK(ChangeFractalColor), editor);

	control = gtk_button_new_with_label(labels->random);
	gtk_box_pack_start(GTK_BOX(vbox), control, FALSE, FALSE, 0);
	(void)g_signal_connect_swapped(G_OBJECT(control), "clicked",
		G_CALLBACK(ChangeRandomColorClicked), editor);

	g_object_unref(store);

	return vbox;
}

static void ChangeFractalVariation(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(slider), "var-index"));
	editor->fractal->control_point.xform[editor->selected_triangle].vars[index] =
		gtk_adjustment_get_value(slider) * 0.01;
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangePreserveWeightsMode(GtkWidget* button, FRACTAL_EDITOR_WIDGET* editor)
{
	if((editor->fractal->flags & FRACTAL_FLAG_UI_DISABLED) != 0)
	{
		return;
	}

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		editor->fractal->flags &= ~(FRACTAL_FLAG_CHECK_PRESERVE);
	}
	else
	{
		editor->fractal->flags |= FRACTAL_FLAG_CHECK_PRESERVE;
	}
}

static void ChangeFractalWeight(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	if((editor->fractal->flags & FRACTAL_FLAG_UI_DISABLED) != 0)
	{
		return;
	}

	editor->fractal->control_point.xform[editor->selected_triangle].density =
		gtk_adjustment_get_value(slider) * 0.01;
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalSymmetry(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	if((editor->fractal->flags & FRACTAL_FLAG_UI_DISABLED) != 0)
	{
		return;
	}

	editor->fractal->control_point.xform[editor->selected_triangle].symmetry =
		gtk_adjustment_get_value(slider) * 0.01;
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalGamma(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	editor->fractal->gamma = editor->fractal->control_point.gamma
		= gtk_adjustment_get_value(slider) * 0.1;
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalBrightness(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	editor->fractal->control_point.brightness
		= gtk_adjustment_get_value(slider) / 20;
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalVibrancy(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	editor->fractal->control_point.vibrancy
		= gtk_adjustment_get_value(slider) * 0.01;
	editor->fractal->vibrancy = editor->fractal->control_point.vibrancy;
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalZoom(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	editor->fractal->control_point.zoom = gtk_adjustment_get_value(slider) * 0.01;
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalCenter(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(slider), "c-index"));
	editor->fractal->control_point.center[index] = gtk_adjustment_get_value(slider) * 0.01;
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static GtkWidget* FractalAdjustWidgetNew(
	FRACTAL_EDITOR_WIDGET* editor,
	FRACTAL_LABEL* labels
)
{
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *box;
	GtkWidget *control;
	GtkWidget *layout;
	GtkAdjustment *adjustment;

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), vbox);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	layout = gtk_frame_new(labels->rendering);
	box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layout), box);
	gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 2);

	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		editor->fractal->control_point.gamma * 10, 1, 50, 0.01, 0.01, 0));
	control = SpinScaleNew(adjustment, labels->gamma, 2);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(ChangeFractalGamma), editor);
	gtk_box_pack_start(GTK_BOX(box), control, FALSE, FALSE, 0);

	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		editor->fractal->control_point.brightness * 20, 0, 100, 1, 1, 0));
	control = SpinScaleNew(adjustment, labels->brightness, 0);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(ChangeFractalBrightness), editor);
	gtk_box_pack_start(GTK_BOX(box), control, FALSE, FALSE, 0);

	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		editor->fractal->control_point.vibrancy * 100, 0, 100, 1, 1, 0));
	control = SpinScaleNew(adjustment, labels->vibrancy, 0);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(ChangeFractalVibrancy), editor);
	gtk_box_pack_start(GTK_BOX(box), control, FALSE, FALSE, 0);

	layout = gtk_frame_new(labels->camera);
	box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layout), box);
	gtk_box_pack_start(GTK_BOX(vbox), layout, FALSE, FALSE, 2);

	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		editor->fractal->control_point.zoom * 100, 0, 100, 1, 1, 0));
	control = SpinScaleNew(adjustment, labels->zoom, 0);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(ChangeFractalZoom), editor);
	gtk_box_pack_start(GTK_BOX(box), control, FALSE, FALSE, 0);

	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		editor->fractal->control_point.center[0] * 100, -100, 100, 1, 1, 0));
	control = SpinScaleNew(adjustment, "X", 2);
	g_object_set_data(G_OBJECT(adjustment), "c-index", GINT_TO_POINTER(0));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(ChangeFractalCenter), editor);
	gtk_box_pack_start(GTK_BOX(box), control, FALSE, FALSE, 0);

	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		editor->fractal->control_point.center[1] * 100, -100, 100, 1, 1, 0));
	control = SpinScaleNew(adjustment, "Y", 2);
	g_object_set_data(G_OBJECT(adjustment), "c-index", GINT_TO_POINTER(1));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(ChangeFractalCenter), editor);
	gtk_box_pack_start(GTK_BOX(box), control, FALSE, FALSE, 0);

	return scroll;
}

static void ChangeFractalOversample(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	editor->fractal->oversample = (int)gtk_adjustment_get_value(slider);
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalFilterRadius(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	editor->fractal->control_point.spatial_filter_radius = gtk_adjustment_get_value(slider) * 0.01;
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalSymmetryType(GtkWidget* combo, FRACTAL_EDITOR_WIDGET* editor)
{
	editor->fractal->symmetry_type = gtk_combo_box_get_active(
		GTK_COMBO_BOX(combo));
	gtk_widget_set_sensitive(editor->symmetry_order,
		editor->fractal->symmetry_order >= FRACTAL_SYMMETRY_TYPE_ROTATIONAL);
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void ChangeFractalSymmetryOrder(GtkAdjustment* slider, FRACTAL_EDITOR_WIDGET* editor)
{
	editor->fractal->symmetry_order = (int)gtk_adjustment_get_value(slider);
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

typedef struct _FRACTAL_MUTATION_WIDGET
{
	FRACTAL_MUTATION mutation;
	FRACTAL_EDITOR_WIDGET *editor;
	GtkWidget *canvas[9];
	int random_seed;
} FRACTAL_MUTATION_WIDGET;

static void UpdateFractalMutation(FRACTAL_MUTATION_WIDGET* mutator)
{
	int i;

	FractalMutationInterpolate(&mutator->mutation);
	for(i=0; i<sizeof(mutator->canvas)/sizeof(*mutator->canvas); i++)
	{
		FractalRender(&mutator->mutation.mutants[i], mutator->random_seed);
	}

	for(i=0; i<sizeof(mutator->canvas)/sizeof(*mutator->canvas); i++)
	{
		gtk_widget_queue_draw(mutator->canvas[i]);
	}
}

static gboolean FractalMutationDisplay(
	GtkWidget* widget,
#if GTK_MAJOR_VERSION <= 2
	GdkEventExpose* event_info,
#else
	cairo_t* cairo_p,
#endif
	FRACTAL_MUTATION_WIDGET* mutator
)
{
#if GTK_MAJOR_VERSION <= 2
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
#endif
	cairo_surface_t *surface_p;
	int index = GPOINTER_TO_INT(g_object_get_data(
		G_OBJECT(widget), "canvas_index"));

	cairo_set_source_rgb(
		cairo_p,
		mutator->editor->fractal->back_ground[0],
		mutator->editor->fractal->back_ground[1],
		mutator->editor->fractal->back_ground[2]
	);
	cairo_rectangle(cairo_p, 0, 0,
		mutator->mutation.mutants[index].image_width, mutator->mutation.mutants[index].image_height);
	cairo_paint(cairo_p);

	surface_p = cairo_image_surface_create_for_data((uint8*)mutator->mutation.mutants[index].pixels,
		CAIRO_FORMAT_ARGB32, mutator->mutation.mutants[index].image_width, mutator->mutation.mutants[index].image_height,
		mutator->mutation.mutants[index].image_width * 4
	);
	cairo_set_source_surface(cairo_p, surface_p, 0, 0);
	cairo_paint(cairo_p);

	cairo_surface_destroy(surface_p);

#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

static gboolean FractalMutationButtonPress(
	GtkWidget* widget,
	GdkEventButton* event_info,
	FRACTAL_MUTATION_WIDGET* mutator
)
{
	FRACTAL_POINT temp_point;
	int index = GPOINTER_TO_INT(g_object_get_data(
		G_OBJECT(widget), "canvas_index"));
	int i, j;

	if(index == 0)
	{
		mutator->mutation.random_seed++;
	}
	else
	{
		temp_point = mutator->mutation.control_points[0];
		temp_point.time_value = 0;
		CopyFractalPoint(&mutator->mutation.control_points[0],
			&mutator->mutation.control_points[index]);
		CopyFractal(&mutator->mutation.mutants[0], &mutator->mutation.mutants[index]);

		for(i=1; i<sizeof(mutator->canvas)/sizeof(*mutator->canvas); i++)
		{
			CopyFractalPoint(&mutator->mutation.control_points[i], &mutator->mutation.control_points[0]);
		}
		for(i=1; i<sizeof(mutator->canvas)/sizeof(*mutator->canvas); i++)
		{
			CopyFractal(&mutator->mutation.mutants[i], &mutator->mutation.mutants[0]);
		}

		if((mutator->mutation.flags & FRACTAL_MUTATION_FLAG_MAINTAIN_SYMMETRY) != 0)
		{
			for(i=0; i<mutator->mutation.mutants[0].transforms; i++)
			{
				if(temp_point.xform[i].symmetry == 1)
				{
					mutator->mutation.control_points[0].xform[i].symmetry = 1;
					mutator->mutation.control_points[0].xform[i].color = temp_point.xform[i].color;
					mutator->mutation.control_points[0].xform[i].density = temp_point.xform[i].density;
					mutator->mutation.control_points[0].xform[i].c[0][0] = temp_point.xform[i].c[0][0];
					mutator->mutation.control_points[0].xform[i].c[0][1] = temp_point.xform[i].c[0][1];
					mutator->mutation.control_points[0].xform[i].c[1][0] = temp_point.xform[i].c[1][0];
					mutator->mutation.control_points[0].xform[i].c[1][1] = temp_point.xform[i].c[1][1];
					mutator->mutation.control_points[0].xform[i].c[2][0] = temp_point.xform[i].c[2][0];
					mutator->mutation.control_points[0].xform[i].c[2][1] = temp_point.xform[i].c[2][1];
					for(j=0; j<FRACTAL_NUM_VARS; j++)
					{
						mutator->mutation.control_points[0].xform[i].vars[j] = temp_point.xform[i].vars[j];
					}
				}
			}
		}
	}
	
	FractalMutationRandomSet(&mutator->mutation);
	UpdateFractalMutation(mutator);

	return TRUE;
}

void ChangeFractalMutationSpeed(GtkAdjustment* slider, FRACTAL_MUTATION_WIDGET* mutator)
{
	mutator->mutation.time_value = gtk_adjustment_get_value(slider);
	UpdateFractalMutation(mutator);
}

void ChangeFractalMutationType(GtkWidget* combo, FRACTAL_MUTATION_WIDGET* mutator)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) - 1;
	int i;

	if(index < 0)
	{
		index = FRACTAL_VALIATION_RANDOM;
	}

	for(i=1; i<sizeof(mutator->canvas)/sizeof(*mutator->canvas); i++)
	{
		FractalPointSetVariation(&mutator->mutation.control_points[i],
			(eFRACTAL_VALIATION)index);
	}
	UpdateFractalMutation(mutator);
}

void ExecuteFractalMutation(FRACTAL_EDITOR_WIDGET* editor)
{
#define PREVIEW_SIZE 250
	const int indices[] = {1, 2, 3, 4, 0, 5, 6, 7, 8};
	FRACTAL_MUTATION_WIDGET mutator;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(
		editor->labels->mutation, GTK_WINDOW(editor->window), GTK_DIALOG_MODAL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL
	);
	GtkWidget *frame;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *box;
	GtkWidget *layout;
	GtkWidget *control;
	GtkWidget *label;
	GtkAdjustment *adjustment;
	char str[1024];
	int width, height;
	int i, j;

	if(editor->fractal->image_width >= editor->fractal->image_height)
	{
		width = PREVIEW_SIZE;
		height = (PREVIEW_SIZE * editor->fractal->image_height) / editor->fractal->image_width;
	}
	else
	{
		height = PREVIEW_SIZE;
		width = (PREVIEW_SIZE * editor->fractal->image_width) / editor->fractal->image_height;
	}

	mutator.editor = editor;

	InitializeFractalMutation(&mutator.mutation, editor->fractal,
		9, editor->fractal->random_start);

	for(i=0; i<9; i++)
	{
		ResizeFractal(&mutator.mutation.mutants[i], width, height);
		FractalRender(&mutator.mutation.mutants[i], mutator.mutation.mutants[0].random_start);
		ResizeFractalPoint(&mutator.mutation.control_points[i], width, height);
	}

	// 変形後の図形描画テーブル
	frame = gtk_frame_new(editor->labels->directions);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 2);
	layout = gtk_table_new(3, 3, TRUE);
	gtk_container_add(GTK_CONTAINER(frame), layout);
	for(i=0; i<3; i++)
	{
		for(j=0; j<3; j++)
		{
			int index = indices[i*3+j];
			mutator.canvas[index] = control = gtk_drawing_area_new();
			gtk_widget_set_size_request(control, width, height);
			gtk_widget_set_events(control, GDK_EXPOSURE_MASK | GDK_BUTTON1_MASK);
			g_object_set_data(G_OBJECT(control), "canvas_index", GINT_TO_POINTER(index));
#if GTK_MAJOR_VERSION <= 2
			(void)g_signal_connect(G_OBJECT(control), "expose_event",
				G_CALLBACK(FractalMutationDisplay), &mutator);
#else
			(void)g_signal_connect(G_OBJECT(control), "draw",
				G_CALLBACK(FractalMutationDisplay), &mutator);
#endif
			(void)g_signal_connect(G_OBJECT(control), "button_press_event",
				G_CALLBACK(FractalMutationButtonPress), &mutator);
			gtk_table_attach(GTK_TABLE(layout), control, j, j+1, i, i+1,
				GTK_FILL, GTK_FILL, 2, 2);
		}
	}

	// スピードと傾向
	frame = gtk_frame_new(editor->labels->controls);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), box);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(mutator.mutation.time_value,
		1, 50, 1, 1, 0));
	control = SpinScaleNew(adjustment, editor->labels->speed, 0);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(ChangeFractalMutationSpeed), &mutator);
	gtk_box_pack_start(GTK_BOX(box), control, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION <= 2
	control = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->random);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->linear);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->sinusoidal);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->spherical);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->swirl);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->horseshoe);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->polar);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->handkerchief);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->heart);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->disc);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->spiral);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->hyperbolic);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->diamond);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->ex);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->julia);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->bent);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->waves);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->fisheye);
	gtk_combo_box_append_text(GTK_COMBO_BOX(control), editor->labels->popcorn);
#else
	control = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->random);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->linear);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->sinusoidal);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->spherical);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->swirl);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->horseshoe);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->polar);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->handkerchief);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->heart);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->disc);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->spiral);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->hyperbolic);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->diamond);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->ex);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->julia);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->bent);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->waves);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->fisheye);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), editor->labels->popcorn);
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(control), 0);
	(void)g_signal_connect(G_OBJECT(control), "changed",
		G_CALLBACK(ChangeFractalMutationType), &mutator);
	layout = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", editor->labels->controls);
	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), layout, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		vbox, TRUE, TRUE, 0);

	UpdateFractalMutation(&mutator);

	gtk_widget_show_all(dialog);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		ResizeFractal(&mutator.mutation.mutants[0],
			editor->fractal->image_width, editor->fractal->image_height);
		CopyFractal(editor->fractal, &mutator.mutation.mutants[0]);
		FractalRender(editor->fractal, editor->fractal->random_start);
		FractalTrianglesFromControlPoint(
			editor->fractal, &editor->fractal->control_point, editor->main_triangles);
		FractalEditorAutoZoom(editor);
		FractalEditorUpdateFlame(editor);
		gtk_widget_queue_draw(editor->graph);
	}

	gtk_widget_destroy(dialog);

	ReleaseFractalMutation(&mutator.mutation);

#undef PIREVIEW_SIZE
}

static void ChangeFractalEditorUpdatePriviewMode(GtkWidget* button, FRACTAL_EDITOR_WIDGET* editor)
{
	if((editor->fractal->flags & FRACTAL_FLAG_UI_DISABLED) != 0)
	{
		return;
	}

	editor->update_immediately = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	if(editor->update_immediately != FALSE)
	{
		FractalEditorUpdateFlame(editor);
	}
}

static void OnClickedUpdateButton(GtkWidget* button, FRACTAL_EDITOR_WIDGET* editor)
{
	FractalEditorUpdateFlame(editor);
}

GtkWidget* FractalEditorNew(
	FRACTAL* fractal,
	int preview_width,
	int preview_height,
	FRACTAL_LABEL* label_string,
	FRACTAL_EDITOR_WIDGET** editor_p
)
{
#define DEFAULT_GRAPH_SIZE (400)
#define CONTROL_RANGE (3)
#define SCROLL_SIZE (350)
	FRACTAL_EDITOR_WIDGET *editor;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
	GtkAdjustment *adjustment;
	char str[4096];
	int i;

	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	editor = (FRACTAL_EDITOR_WIDGET*)MEM_ALLOC_FUNC(sizeof(*editor));
	(void)memset(editor, 0, sizeof(*editor));
	editor->fractal = fractal;
	editor->labels = label_string;
	editor->graph_zoom = 1;
	editor->reference_color[0] = 255 - (uint8)(fractal->back_ground[0] * 255);
	editor->reference_color[1] = 255 - (uint8)(fractal->back_ground[1] * 255);
	editor->reference_color[2] = 255 - (uint8)(fractal->back_ground[2] * 255);
	if(editor->reference_color[0] >= 128 - REVERSE_THRESHOLD
		&& editor->reference_color[0] <= 128 + REVERSE_THRESHOLD
		&& editor->reference_color[1] >= 128 - REVERSE_THRESHOLD
		&& editor->reference_color[1] <= 128 + REVERSE_THRESHOLD
		&& editor->reference_color[2] >= 128 - REVERSE_THRESHOLD
		&& editor->reference_color[2] <= 128 + REVERSE_THRESHOLD
	)
	{
		editor->reference_color[0] = 0;
		editor->reference_color[1] = 0;
		editor->reference_color[2] = 0;
	}
	editor->update_immediately = TRUE;

	FractalTrianglesFromControlPoint(
		fractal, &fractal->control_point, editor->main_triangles);

	// 三角形の調整ウィジェット作成
	{
		GtkWidget *graph = gtk_drawing_area_new();
		editor->graph = graph;

		gtk_widget_set_events(graph, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_RELEASE_MASK
#if GTK_MAJOR_VERSION >= 3
			| GDK_TOUCH_MASK
#endif
		);
		gtk_widget_set_size_request(graph, DEFAULT_GRAPH_SIZE, DEFAULT_GRAPH_SIZE);
		(void)g_signal_connect(G_OBJECT(graph), "expose_event",
			G_CALLBACK(FractalEditorDrawGraph), editor);
		(void)g_signal_connect(G_OBJECT(graph), "configure_event",
			G_CALLBACK(FractalEditorGraphConfigure), editor);
		(void)g_signal_connect(G_OBJECT(graph), "button_press_event",
			G_CALLBACK(FractalEditorGraphButtonPress), editor);
		(void)g_signal_connect(G_OBJECT(graph), "motion_notify_event",
			G_CALLBACK(FractalEditorGraphMouseMove), editor);
		(void)g_signal_connect(G_OBJECT(graph), "button_release_event",
			G_CALLBACK(FractalEditorGraphButtonRelease), editor);
		gtk_box_pack_start(GTK_BOX(hbox), graph, TRUE, TRUE, 2);
	}

	// プレビューとパラメータ設定のウィジェット作成
	{
		GtkWidget *box = gtk_vbox_new(FALSE, 0);
		GtkWidget *control;
		GtkWidget *page_box;
		GtkWidget *frame_box;
		GtkWidget *layout;
		GtkWidget *preview;
		GtkWidget *note_book;
		GtkWidget *transform;
		GtkWidget *label;

		gtk_box_pack_end(GTK_BOX(hbox), box, TRUE, TRUE, 0);

		// プレビューウィジェット
		editor->preview = preview = gtk_drawing_area_new();
		gtk_widget_set_events(preview, GDK_EXPOSURE_MASK);
		gtk_widget_set_size_request(preview, preview_width, preview_height);
#if GTK_MAJOR_VERSION <= 2
		(void)g_signal_connect(G_OBJECT(preview), "expose_event",
			G_CALLBACK(DisplayFractalPreview), fractal);
#else
		(void)g_signal_connect(G_OBJECT(preview), "draw",
			G_CALLBACK(DisplayFractalPreview), fractal);
#endif
		gtk_box_pack_start(GTK_BOX(box), preview, FALSE, FALSE, 0);

		// 三角形選択ウィジェット
		layout = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(box), layout, FALSE, FALSE, 0);
		(void)sprintf(str, "%s : ", label_string->transform);
		label = gtk_label_new(str);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION <= 2
		transform = gtk_combo_box_new_text();
		for(i=0; i<fractal->transforms; i++)
		{
			(void)sprintf(str, "%d", i+1);
			gtk_combo_box_append_text(GTK_COMBO_BOX(transform), str);
		}
#else
		transform = gtk_combo_box_text_new();
		for(i=0; i<fractal->transforms; i++)
		{
			(void)sprintf(str, "%d", i+1);
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(transform), str);
		}
#endif
		editor->transform = transform;
		gtk_combo_box_set_active(GTK_COMBO_BOX(transform), 0);
		(void)g_signal_connect(G_OBJECT(transform), "changed",
			G_CALLBACK(ChangeSelectedTriangle), editor);
		gtk_box_pack_start(GTK_BOX(layout), transform, TRUE, TRUE, 0);

		// パラメータ設定
		note_book = gtk_notebook_new();
		gtk_box_pack_start(GTK_BOX(box), note_book, FALSE, FALSE, 0);

		// 三角形の座標設定
		label = gtk_label_new(label_string->triangle);
		page_box = gtk_vbox_new(FALSE, 0);
		layout = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(layout), page_box);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(layout),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request(layout, SCROLL_SIZE, SCROLL_SIZE);
		(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), layout, label);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("Ax : ");
		editor->triangle_points[0][0] = control
			= gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.01);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control), editor->main_triangles[0][0][0]);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);
		g_object_set_data(G_OBJECT(control), "point-index", GINT_TO_POINTER(0));
		g_object_set_data(G_OBJECT(control), "is_y", GINT_TO_POINTER(0));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeTrianglePoint), editor);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("Ay : ");
		editor->triangle_points[0][1] = control
			= gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.01);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control), editor->main_triangles[0][0][1]);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);
		g_object_set_data(G_OBJECT(control), "point-index", GINT_TO_POINTER(0));
		g_object_set_data(G_OBJECT(control), "is_y", GINT_TO_POINTER(1));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeTrianglePoint), editor);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("Bx : ");
		editor->triangle_points[1][0] = control
			= gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.01);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control), editor->main_triangles[0][1][0]);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);
		g_object_set_data(G_OBJECT(control), "point-index", GINT_TO_POINTER(1));
		g_object_set_data(G_OBJECT(control), "is_y", GINT_TO_POINTER(0));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeTrianglePoint), editor);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("By : ");
		editor->triangle_points[1][1] = control
			= gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.01);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control), editor->main_triangles[0][1][1]);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);
		g_object_set_data(G_OBJECT(control), "point-index", GINT_TO_POINTER(1));
		g_object_set_data(G_OBJECT(control), "is_y", GINT_TO_POINTER(1));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeTrianglePoint), editor);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("Cx : ");
		editor->triangle_points[2][0] = control
			= gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.01);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control), editor->main_triangles[0][2][0]);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);
		g_object_set_data(G_OBJECT(control), "point-index", GINT_TO_POINTER(2));
		g_object_set_data(G_OBJECT(control), "is_y", GINT_TO_POINTER(0));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeTrianglePoint), editor);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("Cy : ");
		editor->triangle_points[2][1] = control
			= gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.01);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control), editor->main_triangles[0][2][1]);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);
		g_object_set_data(G_OBJECT(control), "point-index", GINT_TO_POINTER(2));
		g_object_set_data(G_OBJECT(control), "is_y", GINT_TO_POINTER(1));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeTrianglePoint), editor);

		control = gtk_check_button_new_with_label(label_string->preserve_weight);
		(void)g_signal_connect(G_OBJECT(control), "toggled",
			G_CALLBACK(ChangePreserveWeightsMode), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		control = gtk_button_new_with_label(label_string->reset);
		(void)g_signal_connect_swapped(G_OBJECT(control), "clicked",
			G_CALLBACK(RandomResetFractalTrianglePoint), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		layout = gtk_hbox_new(FALSE, 0);
		(void)sprintf(str, "%s : ", label_string->seed);
		label = gtk_label_new(str);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		editor->random_seed = control = gtk_spin_button_new_with_range(0, INT_MAX, 1);
		gtk_widget_set_sensitive(control, (fractal->flags & FRACTAL_FLAG_USE_TIME_SEED) == 0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control), editor->fractal->random_start);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);

		control = gtk_check_button_new_with_label(label_string->new_seed);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control),
			fractal->flags & FRACTAL_FLAG_USE_TIME_SEED);
		(void)g_signal_connect(G_OBJECT(control), "toggled",
			G_CALLBACK(ChangeRandomSeedMode), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		// 変形
		label = gtk_label_new(label_string->transform);
		page_box = gtk_vbox_new(FALSE, 0);
		layout = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(layout), page_box);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(layout),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request(layout, SCROLL_SIZE, SCROLL_SIZE);
		(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), layout, label);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("a : ");
		editor->a = control = gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control),
			fractal->control_point.xform[0].c[0][0]);
		g_object_set_data(G_OBJECT(control), "x-index", GINT_TO_POINTER(0));
		g_object_set_data(G_OBJECT(control), "y-index", GINT_TO_POINTER(0));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeFractalControlPoint), editor);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("b : ");
		editor->b = control = gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control),
			fractal->control_point.xform[0].c[1][0]);
		g_object_set_data(G_OBJECT(control), "x-index", GINT_TO_POINTER(1));
		g_object_set_data(G_OBJECT(control), "y-index", GINT_TO_POINTER(0));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeFractalControlPoint), editor);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("c : ");
		editor->c = control = gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control),
			fractal->control_point.xform[0].c[0][1]);
		g_object_set_data(G_OBJECT(control), "x-index", GINT_TO_POINTER(0));
		g_object_set_data(G_OBJECT(control), "y-index", GINT_TO_POINTER(1));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeFractalControlPoint), editor);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("d : ");
		editor->d = control = gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control),
			fractal->control_point.xform[0].c[1][1]);
		g_object_set_data(G_OBJECT(control), "x-index", GINT_TO_POINTER(1));
		g_object_set_data(G_OBJECT(control), "y-index", GINT_TO_POINTER(1));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeFractalControlPoint), editor);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("e : ");
		editor->e = control = gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control),
			fractal->control_point.xform[0].c[2][0]);
		g_object_set_data(G_OBJECT(control), "x-index", GINT_TO_POINTER(2));
		g_object_set_data(G_OBJECT(control), "y-index", GINT_TO_POINTER(0));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeFractalControlPoint), editor);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);

		layout = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("f : ");
		editor->f = control = gtk_spin_button_new_with_range(- CONTROL_RANGE, CONTROL_RANGE, 0.1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(control), 6);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(control),
			fractal->control_point.xform[0].c[2][1]);
		g_object_set_data(G_OBJECT(control), "x-index", GINT_TO_POINTER(2));
		g_object_set_data(G_OBJECT(control), "y-index", GINT_TO_POINTER(1));
		(void)g_signal_connect(G_OBJECT(control), "value_changed",
			G_CALLBACK(ChangeFractalControlPoint), editor);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 0);

		editor->weight = adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].density * 100,
			0, 100, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->weight, 0);
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalWeight), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].symmetry * 100,
			0, 100, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->symmetry, 0);
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalSymmetry), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		// 色選択
		label = gtk_label_new(label_string->colors);
		page_box = SelectColorWidgetNew(editor, label_string);
		(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), page_box, label);

		// バリエーション1
		(void)sprintf(str, "%s 1", label_string->variations);
		label = gtk_label_new(str);
		layout = gtk_scrolled_window_new(NULL, NULL);
		page_box = gtk_vbox_new(FALSE, 0);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(layout), page_box);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(layout),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request(layout, SCROLL_SIZE, SCROLL_SIZE);
		(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), layout, label);

		editor->variation[FRACTAL_VALIATION_LINEAR] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_LINEAR] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->linear, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_LINEAR));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_SINUSOIDAL] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_SINUSOIDAL] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->sinusoidal, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_SINUSOIDAL));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_SPHERICAL] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_SPHERICAL] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->spherical, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_SPHERICAL));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_SWIRL] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_SWIRL] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->swirl, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_SWIRL));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_HORSESHOE] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_HORSESHOE] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->horseshoe, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_HORSESHOE));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_POLAR] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_POLAR] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->polar, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_POLAR));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_HANDKERCHIEF] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_HANDKERCHIEF] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->handkerchief, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_HANDKERCHIEF));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_HEART] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_HEART] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->heart, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_HEART));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_DISC] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_DISC] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->disc, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_DISC));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		// バリエーション2
		(void)sprintf(str, "%s 2", label_string->variations);
		label = gtk_label_new(str);
		layout = gtk_scrolled_window_new(NULL, NULL);
		page_box = gtk_vbox_new(FALSE, 0);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(layout), page_box);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(layout),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request(layout, SCROLL_SIZE, SCROLL_SIZE);
		(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), layout, label);

		editor->variation[FRACTAL_VALIATION_SPIRAL] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_SPIRAL] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->spiral, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_SPIRAL));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_HYPERBOLIC] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_HYPERBOLIC] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->hyperbolic, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_HYPERBOLIC));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_SQUARE] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_SQUARE] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->diamond, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_SQUARE));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_EX] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_EX] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->ex, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_EX));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_JULIA] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_JULIA] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->julia, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_JULIA));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_BENT] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_BENT] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->bent, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_BENT));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_WAVES] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_WAVES] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->waves, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_WAVES));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_FISHEYE] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_FISHEYE] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->fisheye, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_FISHEYE));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		editor->variation[FRACTAL_VALIATION_POPCORN] = adjustment
			= GTK_ADJUSTMENT(gtk_adjustment_new(fractal->control_point.xform[0].vars[
				FRACTAL_VALIATION_POPCORN] * 100, -200, 200, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->popcorn, 0);
		g_object_set_data(G_OBJECT(adjustment), "var-index", GINT_TO_POINTER(
			FRACTAL_VALIATION_POPCORN));
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalVariation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		// 画像の調整
		label = gtk_label_new(label_string->adjust);
		page_box = FractalAdjustWidgetNew(editor, label_string);
		(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), page_box, label);

		// オプション
		label = gtk_label_new(label_string->option);
		page_box = gtk_vbox_new(FALSE, 0);
		layout = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(layout),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(layout), page_box);
		(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), layout, label);

		layout = gtk_frame_new(label_string->rendering);
		frame_box = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(layout), frame_box);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 2);

		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(fractal->oversample,
			1, 4, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->oversample, 0);
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalOversample), editor);
		gtk_box_pack_start(GTK_BOX(frame_box), control, FALSE, FALSE, 0);

		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			fractal->control_point.spatial_filter_radius * 100, 10, 100, 1, 1, 0));
		control = SpinScaleNew(adjustment, label_string->filter_radius, 0);
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalOversample), editor);
		gtk_box_pack_start(GTK_BOX(frame_box), control, FALSE, FALSE, 0);

		layout = gtk_frame_new(label_string->forced_symmetry);
		frame_box = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(layout), frame_box);
		gtk_box_pack_start(GTK_BOX(page_box), layout, FALSE, FALSE, 2);

#if GTK_MAJOR_VERSION <= 2
		control = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), label_string->none);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), label_string->bilateral);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), label_string->rotational);
		gtk_combo_box_append_text(GTK_COMBO_BOX(control), label_string->dihedral);
#else
		control = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), label_string->none);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), label_string->bilateral);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), label_string->rotational);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control), label_string->dihedral);
#endif
		(void)g_signal_connect(G_OBJECT(control), "changed",
			G_CALLBACK(ChangeFractalSymmetryType), editor);
		layout = gtk_hbox_new(FALSE, 0);
		(void)sprintf(str, "%s : ", label_string->type);
		label = gtk_label_new(str);
		gtk_combo_box_set_active(GTK_COMBO_BOX(control), fractal->symmetry_type);
		gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(layout), control, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(frame_box), layout, FALSE, FALSE, 0);

		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(fractal->symmetry_order,
			2, 100, 1, 1, 0));
		editor->symmetry_order = control = SpinScaleNew(adjustment, label_string->order, 0);
		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeFractalSymmetryOrder), editor);
		gtk_widget_set_sensitive(control, fractal->symmetry_type >= FRACTAL_SYMMETRY_TYPE_ROTATIONAL);
		gtk_box_pack_start(GTK_BOX(frame_box), control, FALSE, FALSE, 0);

		control = gtk_button_new_with_label(label_string->mutation);
		(void)g_signal_connect_swapped(G_OBJECT(control), "clicked",
			G_CALLBACK(ExecuteFractalMutation), editor);
		gtk_box_pack_start(GTK_BOX(page_box), control, FALSE, FALSE, 0);

		control = gtk_check_button_new_with_label(label_string->update_immediately);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control), editor->update_immediately);
		(void)g_signal_connect(G_OBJECT(control), "toggled",
			G_CALLBACK(ChangeFractalEditorUpdatePriviewMode), editor);
		gtk_box_pack_start(GTK_BOX(box), control, FALSE, FALSE, 0);


		control = gtk_button_new_with_label(label_string->update);
		(void)g_signal_connect(G_OBJECT(control), "clicked",
			G_CALLBACK(OnClickedUpdateButton), editor);
		gtk_box_pack_start(GTK_BOX(box), control, FALSE, FALSE, 0);
	}

	(void)g_signal_connect_swapped(G_OBJECT(vbox), "destroy",
		G_CALLBACK(OnFractalEditorDestroy), editor);

	if(editor_p != NULL)
	{
		*editor_p = editor;
	}

	return vbox;
}

void FractalEditorResetWidgets(FRACTAL_EDITOR_WIDGET* editor)
{
	GtkTreeModel *tree_model;
	char str[1024];
	int reset_flags = (editor->fractal->flags & FRACTAL_FLAG_UI_DISABLED) == 0;
	int i;

	editor->fractal->flags |= FRACTAL_FLAG_UI_DISABLED;

	tree_model = gtk_combo_box_get_model(GTK_COMBO_BOX(editor->transform));
	gtk_list_store_clear(GTK_LIST_STORE(tree_model));
	for(i=0; i<editor->fractal->transforms; i++)
	{
		(void)sprintf(str, "%d", i+1);
#if GTK_MAJOR_VERSION <= 2
		gtk_combo_box_append_text(GTK_COMBO_BOX(editor->transform), str);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(editor->transform), str);
#endif
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(editor->transform), editor->selected_triangle);

	for(i=0; i<3; i++)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[i][0]),
			editor->main_triangles[editor->selected_triangle][i][0]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->triangle_points[i][1]),
			editor->main_triangles[editor->selected_triangle][i][1]);
	}

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->a),
		editor->fractal->control_point.xform[editor->selected_triangle].c[0][0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->b),
		editor->fractal->control_point.xform[editor->selected_triangle].c[1][0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->c),
		editor->fractal->control_point.xform[editor->selected_triangle].c[0][1]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->d),
		editor->fractal->control_point.xform[editor->selected_triangle].c[1][1]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->e),
		editor->fractal->control_point.xform[editor->selected_triangle].c[2][0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->f),
		editor->fractal->control_point.xform[editor->selected_triangle].c[2][1]);

	for(i=0; i<FRACTAL_NUM_VARS; i++)
	{
		gtk_adjustment_set_value(editor->variation[i],
			editor->fractal->control_point.xform[editor->selected_triangle].vars[i] * 100);
	}

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor->random_seed), editor->fractal->random_start);

	if(reset_flags != FALSE)
	{
		editor->fractal->flags &= ~(FRACTAL_FLAG_UI_DISABLED);
	}

	gtk_widget_queue_draw(editor->graph);
	FractalEditorUpdateFlame(editor);
}

#ifdef __cplusplus
}
#endif
