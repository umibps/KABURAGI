#ifndef _INCLUDED_VECTOR_BRUSH_CORE_H_
#define _INCLUDED_VECTOR_BRUSH_CORE_H_

#include <gtk/gtk.h>
#include "types.h"
#include "brush_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*vector_bruch_core_func)(struct _DRAW_WINDOW* window, gdouble x, gdouble y,
	gdouble pressure, struct _VECTOR_BRUSH_CORE *core, void* state);

typedef struct _VECTOR_BRUSH_CORE
{
	struct _APPLICATION *app;
	void* brush_data;
	uint8 (*color)[3];
	uint8 (*back_color)[3];

	gchar* name;
	char* image_file_path;

	vector_bruch_core_func press_func, motion_func, release_func;
	void (*key_press_func)(struct _DRAW_WINDOW* window, GdkEventKey* key, void* data);
	void (*draw_cursor)(struct _DRAW_WINDOW* window, gdouble x, gdouble y, void* data);
	brush_update_func button_update, motion_update;
	GtkWidget* (*create_detail_ui)(struct _APPLICATION* app, void* data);
	void (*color_change)(const uint8 color[3], void* data);
	GtkWidget* button;

	size_t detail_data_size;

	uint16 brush_type;
	gchar hot_key;
} VECTOR_BRUSH_CORE;

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_VECTOR_BRUSH_CORE_H_
