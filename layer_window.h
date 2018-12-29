#ifndef _INCLUDED_LAYER_WINDOW_H_
#define _INCLUDED_LAYER_WINDOW_H_

#include <gtk/gtk.h>
#include "layer.h"
#include "draw_window.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LAYER_THUMBNAIL_SIZE 32

#define LAYER_SET_DISPLAY_OFFSET 20

typedef enum _eLAYER_WINDOW_FLAGS
{
	LAYER_WINDOW_IN_DRAG_OPERATION = 0x01,
	LAYER_WINDOW_DROP_UPPER = 0x02,
	LAYER_WINDOW_IN_CHANGE_NAME = 0x04,
	LAYER_WINDOW_DOCKED = 0x08,
	LAYER_WINDOW_PLACE_RIGHT = 0x10,
	LAYER_WINDOW_POP_UP = 0x20,
	LAYER_WINDOW_SCROLLBAR_PLACE_LEFT = 0x40
} eLAYER_WINDOW_FLAGS;

typedef struct _LAYER_WINDOW
{
	GtkWidget* window;
	int window_x, window_y;
	int window_width, window_height;
	int place;
	int pane_position;
	GtkWidget* view;
	GtkWidget* vbox;
	guint timer_id;
	LAYER *drag_prev;
	LAYER *drop_layer_set;
	LAYER *before_layer_set;
	GtkWidget *drag_position_separator;
	GtkWidget *scrolled_window;
	GtkWidget *change_bg_button;
	GtkWidget *change_bg_label;
	uint16 change_bg_index;
	struct
	{
		GtkWidget *mode, *mask_width_under,
			*lock_opacity, *new_box, *up, *down,
			*merge_box, *merge_down, *delete_layer,
			*new_adjustment;
		GtkAdjustment* opacity;
		uint16 opacity_index, new_box_index, merge_index,
			delete_menu_index, merge_down_menu_index, flatten_menu_index,
			new_adjustment_index;
	} layer_control;
	DRAW_WINDOW* active_draw;
	GdkPixbuf *eye, *pin;
	GdkPixbuf *open, *close;
	cairo_surface_t *thumb_back;
	uint8 *thumb_back_pixels;
	int flags;
} LAYER_WINDOW;

typedef struct _LAYER_WIDGET
{
	IMAGE_CHECK_BUTTON *eye, *pin;
	GtkWidget *box;
	GtkWidget *parameters, *name,
		*mode, *alpha, *alignment;
	GtkWidget *frame;
	GtkWidget *thumbnail;
	cairo_surface_t *thumb_surface;
	uint8 flags;
	gint start_x, start_y;
} LAYER_WIDGET;

// �֐��̃v���g�^�C�v�錾
EXTERN GtkWidget *CreateLayerWindow(struct _APPLICATION* app, GtkWidget *parent, GtkWidget** view);
EXTERN void LayerViewAddLayer(LAYER *layer, LAYER *bottom, GtkWidget *view, uint16 num_layer);
EXTERN void LayerViewSetActiveLayer(LAYER* layer, GtkWidget* view);

/*************************************************
* ClearLayerView�֐�                             *
* ���C���[�r���[�̃E�B�W�F�b�g��S�č폜����     *
* ����                                           *
* layer_window	: ���C���[�r���[�����E�B���h�E *
*************************************************/
EXTERN void ClearLayerView(LAYER_WINDOW* layer_window);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_LAYER_WINDOW_H_
