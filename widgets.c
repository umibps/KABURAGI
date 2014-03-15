// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <ctype.h>
#include "configure.h"
#include "widgets.h"
#include "memory.h"

#define USE_LCMS2

#ifdef _WIN32
# define INCLUDE_WIN_DEFAULT_API
# include <windows.h>
#endif

#include "iccbutton.h"

#ifdef __cplusplus
extern "C" {
#endif

// �`�F�b�N�{�b�N�X�̘g�̕�
#define CHECK_BUTTON_BORDER_WIDTH 2

static gint OnImageCheckButtonClicked(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	IMAGE_CHECK_BUTTON* button = (IMAGE_CHECK_BUTTON*)data;
	GdkPixbuf* pixbuf;
	gint width, height;
	uint8* pixels;
	int i, j;

	button->state = !button->state;

	pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(button->image));
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);
	pixels = gdk_pixbuf_get_pixels(pixbuf);
	(void)memset(pixels, 0, height * width * 3);
	for(i=0; i<CHECK_BUTTON_BORDER_WIDTH; i++)
	{
		for(j=0; j<width; j++)
		{
			pixels[i*width*4+j*4+3] = 0xff;
		}
	}
	for(i=height-CHECK_BUTTON_BORDER_WIDTH; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			pixels[i*width*4+j*4+3] = 0xff;
		}
	}
	for(i=CHECK_BUTTON_BORDER_WIDTH; i<height-CHECK_BUTTON_BORDER_WIDTH; i++)
	{
		(void)memset(&pixels[i*width*4+CHECK_BUTTON_BORDER_WIDTH*4], 0xff,
			(width - CHECK_BUTTON_BORDER_WIDTH*2) * 4);
		for(j=0; j<CHECK_BUTTON_BORDER_WIDTH; j++)
		{
			pixels[i*width*4+j*4+3] = 0xff;
		}
		for(j=width-CHECK_BUTTON_BORDER_WIDTH; j<width; j++)
		{
			pixels[i*width*4+j*4+3] = 0xff;
		}
	}

	if(button->state != 0)
	{
		cairo_surface_t* surface_p = cairo_image_surface_create_for_data(
			pixels, CAIRO_FORMAT_ARGB32, width, height, width*4);
		cairo_t* cairo_p = cairo_create(surface_p);
		
		gdk_cairo_set_source_pixbuf(cairo_p, button->pixbuf, 0, 0);
		cairo_paint(cairo_p);

		cairo_destroy(cairo_p);
		cairo_surface_destroy(surface_p);

		// Windows�Ȃ�RGB��BGR
#if 1 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
		{
			uint8 r;
			for(i=0; i<width*height; i++)
			{
				r = pixels[i*4];
				pixels[i*4] = pixels[i*4+2];
				pixels[i*4+2] = r;
			}
		}
#endif
	}

	if(button->func != NULL)
	{
		(void)button->func(button, button->data);
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(button->image), pixbuf);

	// �C���[�W�̍ĕ`��
#if MAJOR_VERSION == 1
	if(button->image->window != NULL)
	{
		gdk_window_process_updates(button->image->window, FALSE);
	}
#else
	if(gtk_widget_get_window(button->image) != NULL)
	{
		gdk_window_process_updates(gtk_widget_get_window(button->image), FALSE);
	}
#endif

	return TRUE;
}

IMAGE_CHECK_BUTTON* CreateImageCheckButton(
	GdkPixbuf* pixbuf,
	gint (*func)(IMAGE_CHECK_BUTTON* button, void* data),
	void *data
)
{
	// �Ԃ�l
	IMAGE_CHECK_BUTTON* button =
		(IMAGE_CHECK_BUTTON*)MEM_ALLOC_FUNC(sizeof(*button));
	// �`�F�b�N�{�b�N�X�̔w�i
	GdkPixbuf* back;
	// �`�F�b�N�{�b�N�X�̃s�N�Z���f�[�^
	uint8* pixels;
	// �E�B�W�F�b�g�̕��ƍ���
	gint width, height;
	// for���p�̃J�E���^
	int i, j;

	// 0������
	(void)memset(button, 0, sizeof(*button));

	// �e��l�̃Z�b�g
	button->pixbuf = pixbuf;
	button->func = func;
	button->data = data;

	// �t���[�����ɃC�x���g�{�b�N�X��ݒu
		// ���̒��ɃC���[�W������
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);
	back = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	pixels = gdk_pixbuf_get_pixels(back);
	(void)memset(pixels, 0, height * width * 4);
	for(i=0; i<CHECK_BUTTON_BORDER_WIDTH; i++)
	{
		for(j=0; j<width; j++)
		{
			pixels[i*width*4+j*4+3] = 0xff;
		}
	}
	for(i=height-CHECK_BUTTON_BORDER_WIDTH; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			pixels[i*width*4+j*4+3] = 0xff;
		}
	}
	for(i=CHECK_BUTTON_BORDER_WIDTH; i<height-CHECK_BUTTON_BORDER_WIDTH; i++)
	{
		(void)memset(&pixels[i*width*4+CHECK_BUTTON_BORDER_WIDTH*4], 0xff,
			(width - CHECK_BUTTON_BORDER_WIDTH*2) * 4);
		for(j=0; j<CHECK_BUTTON_BORDER_WIDTH; j++)
		{
			pixels[i*width*4+j*4+3] = 0xff;
		}
		for(j=width-CHECK_BUTTON_BORDER_WIDTH; j<width; j++)
		{
			pixels[i*width*4+j*4+3] = 0xff;
		}
	}
	button->image = gtk_image_new_from_pixbuf(back);
	button->widget = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(button->widget), button->image);
	gtk_widget_set_size_request(button->image,
		gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
	gtk_widget_set_size_request(button->widget,
		gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));

	// �C�x���g�{�b�N�X�Ƀ}�E�X�N���b�N�̃C�x���g��ǉ�
	g_signal_connect(G_OBJECT(button->widget), "button_press_event",
		G_CALLBACK(OnImageCheckButtonClicked), button);
	gtk_widget_add_events(button->widget, GDK_BUTTON_PRESS_MASK);

	return button;
}

void ImageCheckButtonSetState(IMAGE_CHECK_BUTTON* button, int8 state)
{
	if(button->func != NULL && ((button->state == 0 && state != 0)
		|| (button->state != 0) && state == 0))
	{
		OnImageCheckButtonClicked(button->widget, NULL, button);
	}
}

static void TabColoseButtonCallback(GtkWidget* widget, gpointer data)
{
	GtkNotebook* note_book = (GtkNotebook*)data;
	gint page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "page"));
	gboolean (*destroy_func)(void* data, gint page) =
		(gboolean (*)(void*, gint))g_object_get_data(G_OBJECT(widget), "destroy_function");
	gpointer destroy_data = g_object_get_data(G_OBJECT(widget), "destroy_data");

	if(destroy_func != NULL)
	{
		if(destroy_func(destroy_data, page) != FALSE)
		{
			return;
		}
	}

	gtk_notebook_remove_page(note_book, page);
}

GtkWidget* CreateNotebookLabel(
	const gchar* text,
	GtkWidget* note_book,
	gint page,
	gboolean (*destroy_func)(void* data, gint page),
	gpointer data
)
{
	GtkWidget *hbox = gtk_hbox_new(FALSE, 3),
		*label = gtk_label_new(text),
		*button = gtk_button_new(),
		*image = gtk_image_new(),
		*resize_image;
	GdkPixbuf* image_buf, *resize_buf;

	g_object_set_data(G_OBJECT(hbox), "button_widget", button);
	g_object_set_data(G_OBJECT(button), "page", GINT_TO_POINTER(page));
	g_object_set_data(G_OBJECT(button), "destroy_function", (gpointer)destroy_func);
	g_object_set_data(G_OBJECT(button), "destroy_data", data);
	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(TabColoseButtonCallback), note_book);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	image_buf = gtk_widget_render_icon(image, GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU, NULL);
	resize_buf = gdk_pixbuf_scale_simple(image_buf, 8, 8, GDK_INTERP_BILINEAR);
	resize_image = gtk_image_new_from_pixbuf(resize_buf);
	gtk_container_add(GTK_CONTAINER(button), resize_image);

	gtk_widget_show_all(hbox);

	g_object_ref_sink(image);
	g_object_unref(image);
	g_object_unref(image_buf);
	g_object_unref(resize_buf);

	return hbox;
}


GtkWidget *CreateImageButton(
	const char* image_file_path,
	const gchar* label,
	const char* font_file
)
{
#define DISPLAY_FONT_HEIGHT 8.5
	// �\��t����C���[�W
	cairo_surface_t *image =
		cairo_image_surface_create_from_png(image_file_path);
	// �����`��p
	cairo_t *cairo_p = cairo_create(image);
	// �摜�̃t�H�[�}�b�g
	cairo_format_t format = cairo_image_surface_get_format(image);
	// �摜�̕��ƍ���
	int32 width = cairo_image_surface_get_width(image);
	int32 height = cairo_image_surface_get_height(image);
	// �摜�̈�񕪂̃o�C�g��
	int32 stride = width * ((format == CAIRO_FORMAT_ARGB32) ? 4 : 3);
	// �����\����Y���W
	gdouble draw_y = 9.0;
	GtkWidget *image_widget;
	// �s�N�Z���o�b�t�@
	GdkPixbuf *pixbuf;
	// �Ԃ�l
	GtkWidget *button;
	// ���s�����p�̃o�b�t�@
	gchar* str = (label == NULL) ? NULL : MEM_STRDUP_FUNC(label);
	gchar* show_text = str;
	// ���s�����p
	gchar *now, *next;

	// �������`��
	if(label != NULL && *label != '\0')
	{
		// �t�H���g�T�C�Y�Z�b�g
		cairo_select_font_face(cairo_p, font_file, CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cairo_p, DISPLAY_FONT_HEIGHT);

		// ���s���������Ȃ���\��
		now = show_text = str;
		next = g_utf8_next_char(show_text);
		while(*now != '\0' && *next != '\0')
		{
			if((uint8)*now == 0x5c && *next == 'n')
			{
				*now = '\0';
				cairo_move_to(cairo_p, 0.0, draw_y);
				cairo_show_text(cairo_p, show_text);
				show_text = now = g_utf8_next_char(next);
				next = g_utf8_next_char(now);
				draw_y += DISPLAY_FONT_HEIGHT;
			}
			else if((next - now) >= 2 && (uint8)*now == 0xc2
				&& (uint8)(*(now+1)) == 0xa5 && (uint8)*next == 'n')
			{
				*now = '\0';
				cairo_move_to(cairo_p, 0.0, draw_y);
				cairo_show_text(cairo_p, show_text);
				show_text = now = g_utf8_next_char(next);
				next = g_utf8_next_char(now);
				draw_y += DISPLAY_FONT_HEIGHT;
			}
			else
			{
				now = next;
				next = g_utf8_next_char(next);
			}
		}
		cairo_move_to(cairo_p, 0.0, draw_y);
		cairo_show_text(cairo_p, show_text);
	}

	// �C���[�W����s�N�Z���o�b�t�@�𐶐�
	pixbuf = gdk_pixbuf_new_from_data(
		cairo_image_surface_get_data(image),
		GDK_COLORSPACE_RGB,
		(format == CAIRO_FORMAT_ARGB32) ? TRUE : FALSE,
		8,
		width,
		height,
		stride,
		NULL,
		NULL
	);

#if 1 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
	{
		// Windows�Ȃ��BGR��RGB�̕ϊ����s��
		uint8* pix = cairo_image_surface_get_data(image);
		uint8 b;
		int32 channel = (format == CAIRO_FORMAT_ARGB32) ? 4 : 3;
		int32 i, j;

		for(i=0; i<height; i++)
		{
			for(j=0; j<width; j++)
			{
				b = pix[i*stride+j*channel];
				pix[i*stride+j*channel] = pix[i*stride+j*channel+2];
				pix[i*stride+j*channel+2] = b;
			}
		}
	}
#endif	// #if defined(DISPLAY_BGR) && DISPLAY_BGR != 0

	image_widget = gtk_image_new_from_pixbuf(pixbuf);

	button = gtk_toggle_button_new();

	// �{�^���ɃC���[�W�f�[�^���ڂ���
	gtk_container_add(GTK_CONTAINER(button), image_widget);

	cairo_destroy(cairo_p);

	MEM_FREE_FUNC(str);

	return button;
}

static void IccProfileChanged(IccButton* button, char** icc_profile_path)
{
	g_free(*icc_profile_path);

	*icc_profile_path = icc_button_get_filename(button);
}

GtkWidget* IccProfileChooser(char** icc_path, ICC_PROFILE_USAGE usage)
{
	GtkWidget *button = icc_button_new();
	icc_button_set_mask (ICC_BUTTON(button),
                             ICC_BUTTON_CLASS_INPUT | ICC_BUTTON_CLASS_OUTPUT | ICC_BUTTON_CLASS_DISPLAY,
                             ICC_BUTTON_COLORSPACE_XYZ | ICC_BUTTON_COLORSPACE_LAB,
                             ((usage & 3) << 2) | ((usage & 4) << 3));

	if(icc_path != NULL)
	{
		if(*icc_path != NULL)
		{
			(void)icc_button_set_filename(ICC_BUTTON(button), *icc_path, TRUE);
		}
	}

	(void)g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(IccProfileChanged), icc_path);

	return button;
}

/**************************************************************************
* IccProfileChooserDialogNew�֐�                                          *
* ICC�v���t�@�C����I������_�C�A���O�E�B�W�F�b�g���쐬                   *
* ����                                                                    *
* usage			: �ݒ�ł���ICC�v���t�@�C���̎�� ICC_PROFILE_USAGE���Q�� *
* profile_path	: �����l�y�ѐݒ肳�ꂽICC�v���t�@�C���̃p�X               *
* �Ԃ�l                                                                  *
*	�_�C�A���O�����p�̃E�B�W�F�b�g                                        *
**************************************************************************/
GtkWidget* IccProfileChooserDialogNew(int usage, char** profile_path)
{
	GtkWidget *button;

	button = icc_button_new();
	icc_button_set_mask (ICC_BUTTON(button),
							ICC_BUTTON_CLASS_INPUT | ICC_BUTTON_CLASS_OUTPUT | ICC_BUTTON_CLASS_DISPLAY,
							ICC_BUTTON_COLORSPACE_XYZ | ICC_BUTTON_COLORSPACE_LAB,
							((usage & 3) << 2) | ((usage & 4) << 3));

	if(profile_path != NULL)
	{
		if(*profile_path != NULL)
		{
			(void)icc_button_set_filename(ICC_BUTTON(button), *profile_path, TRUE);
		}
	}

	(void)g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(IccProfileChanged), profile_path);

	return button;
}

extern void icc_button_run_dialog (GtkWidget *widget, gpointer data);

/****************************************************
* IccProfileChooserDialogRun�֐�                    *
* ICC�v���t�@�C����I������_�C�A���O��\�����Ď��s *
* ����                                              *
* dialog	: �_�C�A���O�\���p�̃E�B�W�F�b�g        *
****************************************************/
void IccProfileChooserDialogRun(GtkWidget* dialog)
{
	icc_button_run_dialog(NULL, dialog);
}

#ifdef __cplusplus
}
#endif