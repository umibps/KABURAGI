#ifndef _INCLUDED_UI_LABEL_H_
#define _INCLUDED_UI_LABEL_H_

typedef struct _UI_LABEL
{
	struct
	{
		char *file;
		char *add_model_accessory;
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
		char *load_model;
		char *load_pose;
		char *control_model;
		char *no_select;
		char *reset;
		char *apply_center_position;
		char *scale;
		char *color;
		char *opacity;
		char *distance;
		char *field_of_view;
		char *model_connect_to;
		char *enable_physics;
		char *display_grid;
		char *render_edge_only;
		struct
		{
			char *select;
			char *move;
			char *rotate;
		} edit_mode;
	} control;
} UI_LABEL;

extern UI_LABEL LoadDefaultUiLabel(void);

#endif	// #ifndef _INCLUDED_UI_LABEL_H_
