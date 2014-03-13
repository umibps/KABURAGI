#ifndef _INCLUDED_LAYER_SET_H_
#define _INCLUDED_LAYER_SET_H_

#include <gtk/gtk.h>

typedef struct _LAYER_SET
{
	struct _LAYER *active_under;
	GtkWidget *show_child_button;
	GtkWidget *button_image;
} LAYER_SET;

#endif	// #ifndef _INCLUDED_LAYER_SET_H_
