#ifndef _INCLUDED_COLOR_GTK_H_
#define _INCLUDED_COLOR_GTK_H_

#include <gtk/gtk.h>
#include "../../color.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _COLOR_CHOOSER
{
	GtkWidget* widget, *choose_area, *color_box, *pallete;
	GtkWidget *chooser_box;
	GtkWidget *pallete_widget;
	GtkWidget *circle_button, *pallete_button;
	cairo_surface_t *color_box_surface;
	uint8 *color_box_pixel_data;
	cairo_surface_t *circle_surface;
	uint8 *color_circle_data;
	uint8 *chooser_pixel_data;
	cairo_surface_t *chooser_surface;
	cairo_t *chooser_cairo;
	uint8 *pallete_pixels;
	cairo_surface_t *pallete_surface;
	void (*filter_func)(
		uint8* source, uint8* destination, int size, void* filter_data);
	void *filter_data;
	int32 widget_width;
	int32 widget_height;
	HSV hsv, back_hsv;
	uint8 rgb[3], back_rgb[3];
	uint8 color_history[MAX_COLOR_HISTORY_NUM][3];
	uint8 (*pallete_data)[3];
	uint8 *pallete_flags;
	uint8 line_width;
	uint8 flags;
	uint8 num_color_history;
	uint16 selected_pallete;
	FLOAT_T sv_x, sv_y;
	FLOAT_T ui_scale;
	void (*color_change_callback)(
		GtkWidget* widget, const uint8 color[3], void* data);
	void* data;
} COLOR_CHOOSER;

EXTERN void DrawColorCircle(
	COLOR_CHOOSER* chooser,
	FLOAT_T cx,
	FLOAT_T cy,
	FLOAT_T r
);

EXTERN COLOR_CHOOSER *CreateColorChooser(
	int32 width,
	int32 height,
	uint8 line_width,
	int32 start_rad,
	uint8 (*pallete)[3],
	uint8 *pallete_use,
	const gchar* base_path,
	struct _APPLICATION_LABELS* labels,
	FLOAT_T ui_scale
);

EXTERN void SetColorChooserPoint(COLOR_CHOOSER* chooser, HSV* set_hsv, gboolean add_history);

EXTERN void UpdateColorBox(COLOR_CHOOSER* chooser);

EXTERN void DestroyColorChooser(COLOR_CHOOSER* chooser);

/*********************************************************
* SetColorChangeCallBack�֐�                             *
* �F���ύX���ꂽ�Ƃ��ɌĂяo�����R�[���o�b�N�֐���ݒ� *
* ����                                                   *
* chooser	: �F�I��p�̃f�[�^                           *
* function	: �R�[���o�b�N�֐�                           *
* data		: �R�[���o�b�N�֐��Ŏg�p����f�[�^           *
*********************************************************/
EXTERN void SetColorChangeCallBack(
	COLOR_CHOOSER* chooser,
	void (*function)(GtkWidget* widget, const uint8 color[3], void* data),
	void *data
);


/***********************************
* LoadPalleteAdd�֐�               *
* �p���b�g�f�[�^��ǉ��ǂݍ���     *
* ����                             *
* chooser	: �F�I���̏��         *
* file_path	: �f�[�^�t�@�C���̃p�X *
* �Ԃ�l                           *
*	�ǂݍ��񂾃f�[�^�̐�           *
***********************************/
EXTERN int LoadPalleteAdd(
	COLOR_CHOOSER* chooser,
	const char* file_path
);

/*************************************
* LoadPalleteFile�֐�                *
* �p���b�g�̏���ǂݍ���           *
* chooser	: �F�I��p�̏��         *
* file_path	: �ǂݍ��ރt�@�C���̃p�X *
* �Ԃ�l                             *
*	����I��:0	���s:���̒l          *
*************************************/
EXTERN int LoadPalleteFile(COLOR_CHOOSER* chooser, const gchar* file_path);

/*************************************
* WritePalleteFile�֐�               *
* �p���b�g�̏�����������           *
* chooser	: �F�I��p�̏��         *
* file_path	: �������ރt�@�C���̃p�X *
* �Ԃ�l                             *
*	����I��:0	���s:���̒l          *
*************************************/
EXTERN int WritePalleteFile(COLOR_CHOOSER* chooser, const gchar* file_path);

/***************************************
* RegisterColorPallete�֐�             *
* �p���b�g�ɐF��ǉ�����               *
* ����                                 *
* chooser	: �F�I���E�B�W�F�b�g�̏�� *
* color		: �ǉ�����F               *
***************************************/
EXTERN void RegisterColorPallete(COLOR_CHOOSER* chooser, const uint8 color[3]);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_COLOR_GTK_H_
