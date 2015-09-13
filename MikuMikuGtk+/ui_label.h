#ifndef _INCLUDED_UI_LABEL_H_
#define _INCLUDED_UI_LABEL_H_

typedef struct _UI_LABEL
{
	struct
	{
		char *file;
		char *load_project;
		char *save_project;
		char *save_project_as;
		char *add_model_accessory;
		char *add_cuboid;
		char *add_cone;
		char *edit;
		char *undo;
		char *redo;
	} menu;

	struct
	{
		char *environment;
		char *camera;
		char *light;
		char *position;
		char *rotation;
		char *direction;
		char *model;
		char *bone;
		char *morph;
		char *load_model;
		char *load_pose;
		char *control_model;
		char *no_select;
		char *reset;
		char *apply_center_position;
		char *scale;
		char *color;
		char *opacity;
		char *width;
		char *height;
		char *depth;
		char *weight;
		char *edge_size;
		char *num_lines;
		char *line_width;
		char *distance;
		char *field_of_view;
		char *model_connect_to;
		char *enable_physics;
		char *display_grid;
		char *delete_current_model;
		char *render_shadow;
		char *render_edge_only;
		char *upper_size;
		char *lower_size;
		char *cuboid;
		char *add_cuboid;
		char *cone;
		char *add_cone;
		char *back_ground_color;
		char *line_color;
		char *surface_color;
		struct
		{
			char *select;
			char *move;
			char *rotate;
		} edit_mode;
		struct
		{
			char *eye;
			char *lip;
			char *eye_blow;
			char *other;
		} morph_group;
	} control;
} UI_LABEL;

#ifdef __cplusplus
extern "C" {
#endif

extern UI_LABEL LoadDefaultUiLabel(void);

extern UI_LABEL* GetUILabel(void* application_context);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_UI_LABEL_H_
