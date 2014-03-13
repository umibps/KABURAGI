#ifndef _INCLUDED_UI_H_
#define _INCLUDED_UI_H_

#include <gtk/gtk.h>
#include "ght_hash_table.h"

typedef struct _WIDGETS
{
	GtkWidget *drawing_area;
	GtkWidget *main_window;
	GtkWidget *bone_tree_view;
	int ui_disabled;
} WIDGETS;

extern void InitializeWidgets(WIDGETS* widgets, int widget_width, int widget_height, void* project);

extern gboolean MouseButtonPressEvent(GtkWidget* widget, GdkEventButton* event_info, void* project_context);

extern gboolean MouseMotionEvent(GtkWidget* widget, GdkEventMotion* event_info, void* project_context);

extern gboolean MouseButtonReleaseEvent(GtkWidget* widget, GdkEventButton* event_info, void* project_context);

extern gboolean ProjectDisplayEvent(GtkWidget* widget, GdkEventExpose* event_info, void* project_context);

extern gboolean MouseWheelScrollEvent(GtkWidget* widget, GdkEventScroll* event_info, void* project_context);

extern void BoneTreeViewSetBones(GtkWidget *tree_view, void* model_interface, void* project_context);

extern gboolean ConfigureEvent(
	GtkWidget* widget,
	void* event_info,
	void* project_context
);

#endif	// #ifndef _INCLUDED_UI_H_
