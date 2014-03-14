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
		char *bone;
		char *load_model;
		char *enable_physics;
		char *display_grid;
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
