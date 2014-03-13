#ifndef _INCLUDED_UI_LABEL_H_
#define _INCLUDED_UI_LABEL_H_

typedef struct _UI_LABEL
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
} UI_LABEL;

extern UI_LABEL LoadDefaultUiLabel(void);

#endif	// #ifndef _INCLUDED_UI_LABEL_H_
