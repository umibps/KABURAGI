#ifndef _INCLUDED_FRACTAL_EDITOR_H_
#define _INCLUDED_FRACTAL_EDITOR_H_

#include <gtk/gtk.h>
#include "fractal.h"
#include "fractal_label.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FRACTAL_EDITOR_WIDGET
{
	FRACTAL *fractal;
	FRACTAL_LABEL *labels;
	GtkWidget *window;
	GtkWidget *triangle_points[3][2];
	GtkWidget *preview;
	GtkWidget *graph;
	GtkWidget *color_combo_box;
	GtkWidget *transform;
	GtkWidget *a, *b, *c, *d, *e, *f;
	GtkWidget *symmetry_order;
	GtkWidget *random_seed;
	GtkAdjustment *weight;
	GtkAdjustment *symmetry;
	GtkAdjustment *variation[NUM_FRACTAL_VALIATION];
	FLOAT_T main_triangles[FRACTAL_NUM_XFORMS+1][3][2];
	FLOAT_T old_triangle[3][2];
	FLOAT_T graph_zoom;
	uint8 reference_color[3];
	FLOAT_T x_length, y_length;
	FLOAT_T center_x, center_y;
	gdouble before_x, before_y;
	int triangle_caught;
	int corner_caught;
	int selected_triangle;
	gboolean update_immediately;
	unsigned int color_random_seed;
} FRACTAL_EDITOR_WIDGET;

GtkWidget* FractalEditorNew(
	FRACTAL* fractal,
	int preview_width,
	int preview_height,
	FRACTAL_LABEL* label_string,
	FRACTAL_EDITOR_WIDGET** editor_p
);

extern void ResizeFractalEditor(FRACTAL_EDITOR_WIDGET* editor, int new_width, int new_height);

extern void FractalEditorUpdateFlame(FRACTAL_EDITOR_WIDGET* editor);

extern void FractalEditorSetZoom(FRACTAL_EDITOR_WIDGET* editor, int width, int height);

extern void FractalEditorResetWidgets(FRACTAL_EDITOR_WIDGET* editor);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_FRACTAL_EDITOR_H_
