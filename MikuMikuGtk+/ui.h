#ifndef _INCLUDED_UI_H_
#define _INCLUDED_UI_H_

#include <gtk/gtk.h>
#include "ght_hash_table.h"

typedef struct _APPLICATION_WIDGETS
{
	GtkWidget *main_window;
	GtkWidget *model_combo_box;
	GtkWidget *model_scale;
	GtkWidget *model_opacity;
	GtkWidget *model_position[3];
	GtkWidget *model_rotation[3];
	GtkWidget *edge_size;
	GtkWidget *camera_look_at[3];
	GtkWidget *camera_angle[3];
	GtkWidget *camera_distance;
	GtkWidget *camera_fov;
	GtkWidget *light_direction[3];
	GtkWidget *light_color;
	GtkWidget *connect_model;
	GtkWidget *connect_bone;
	GtkWidget *bone_tree_view;
	GtkWidget *morph_group_selector;
	GtkWidget *detail_morph_selector;
	GtkAdjustment *morph_weight;
	int ui_disabled;
} APPLICATION_WIDGETS;

typedef struct _PROJECT_WIDGETS
{
	GtkWidget *drawing_area;
} PROJECT_WIDGETS;

extern void InitializeProjectWidgets(PROJECT_WIDGETS* widgets, int widget_width, int widget_height, void* project);

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

extern GtkWidget* MakeMenuBar(void* application_context, GtkAccelGroup* hot_key);

#endif	// #ifndef _INCLUDED_UI_H_
