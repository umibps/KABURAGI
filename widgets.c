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
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
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
#if GTK_MAJOR_VERSION <= 2
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
	(void)g_signal_connect(G_OBJECT(button->widget), "button_press_event",
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
	const char* font_file,
	FLOAT_T scale
)
{
#define DISPLAY_FONT_HEIGHT 8.5
	// �\��t����C���[�W
	cairo_surface_t *image;
	// �����`��p
	cairo_t *cairo_p;
	// �摜�̕��ƍ���
	int32 width;
	int32 height;
	// �摜�̈�񕪂̃o�C�g��
	int32 stride;
	// �����\����Y���W
	gdouble draw_y = 9.0 * scale;
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

	pixbuf = gdk_pixbuf_new_from_file(image_file_path, NULL);
	if(scale != 1.0)
	{
		GdkPixbuf *old = pixbuf;
		pixbuf = gdk_pixbuf_scale_simple(old, (int)(gdk_pixbuf_get_width(old) * scale),
			(int)(gdk_pixbuf_get_height(old) * scale), GDK_INTERP_NEAREST);
		g_object_unref(old);
	}
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);
	stride = gdk_pixbuf_get_rowstride(pixbuf);
	image = cairo_image_surface_create_for_data(gdk_pixbuf_get_pixels(pixbuf),
		(stride / width) == 4 ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, width, height, stride);
	cairo_p = cairo_create(image);

	// �������`��
	if(label != NULL && *label != '\0')
	{
		// �t�H���g�T�C�Y�Z�b�g
		cairo_select_font_face(cairo_p, font_file, CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cairo_p, DISPLAY_FONT_HEIGHT * scale);

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
				draw_y += DISPLAY_FONT_HEIGHT * scale;
			}
			else if((next - now) >= 2 && (uint8)*now == 0xc2
				&& (uint8)(*(now+1)) == 0xa5 && (uint8)*next == 'n')
			{
				*now = '\0';
				cairo_move_to(cairo_p, 0.0, draw_y);
				cairo_show_text(cairo_p, show_text);
				show_text = now = g_utf8_next_char(next);
				next = g_utf8_next_char(now);
				draw_y += DISPLAY_FONT_HEIGHT * scale;
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

#if 0 //defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	{
		// Windows�Ȃ��BGR��RGB�̕ϊ����s��
		uint8* pix = cairo_image_surface_get_data(image);
		uint8 b;
		int32 channel = stride / width;
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

static void IccProfileChangerDialogProfileChanged(IccButton *button, GtkToggleButton *toggle)
{
	gpointer p;

	gtk_toggle_button_set_active(toggle, TRUE);

	p = g_object_get_data(G_OBJECT(toggle), "data");
	g_free(p);
	g_object_set_data(G_OBJECT(toggle), "data", icc_button_get_filename(button));
}

static void IccProfileChangerDialogSelectionChanged(GObject *radio, GObject *dialog)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio)))
	{
		g_object_set_data(dialog, "data", radio);

#ifdef _DEBUG
		g_printerr("Selection : %s\n",  g_object_get_data(radio, "data"));
#endif
	}
}

static void IccProfileChangerDialogResponse(GObject *dialog, gint response_id, gpointer user_data)
{
	if(response_id == GTK_RESPONSE_OK)
	{
		g_object_set_data(dialog, "file", g_strdup(g_object_get_data(G_OBJECT(g_object_get_data(G_OBJECT(dialog), "data")), "data")));
	}
	g_free(g_object_get_data(G_OBJECT(g_object_get_data(G_OBJECT(dialog), "radio1")), "data"));
	g_free(g_object_get_data(G_OBJECT(g_object_get_data(G_OBJECT(dialog), "radio2")), "data"));
	g_free(g_object_get_data(G_OBJECT(g_object_get_data(G_OBJECT(dialog), "radio3")), "data"));
}

/********************************************************
* IccProfileChangerDialogNew�֐�                        *
* ICC�v���t�@�C����I������_�C�A���O�E�B�W�F�b�g���쐬 *
* ����                                                  *
* parent			     : �e�E�C���h�E                 *
* workspace_profile_path : ��Ɨp�v���t�@�C���̃p�X     *
* �Ԃ�l                                                *
*	�_�C�A���O�����p�̃E�B�W�F�b�g                      *
********************************************************/
GtkWidget* IccProfileChangerDialogNew(GtkWindow *parent, gchar *workspace_profile_path)
{
	GtkWidget *dialog, *content_area, *box, *radio1, *radio2 = NULL, *radio3, *button;

	dialog = gtk_dialog_new_with_buttons("Change ICC profile", parent, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_widget_set_size_request(dialog, 400, -1);
	g_signal_connect(dialog, "response", G_CALLBACK(IccProfileChangerDialogResponse), dialog);
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 4);

	radio1 = gtk_radio_button_new_with_label(NULL, "Don't color manage this canvas");
	gtk_misc_set_alignment(GTK_MISC(gtk_bin_get_child(GTK_BIN(radio1))), 0, 0.5);
	gtk_label_set_width_chars(GTK_LABEL(gtk_bin_get_child(GTK_BIN(radio1))), 64);
	gtk_label_set_ellipsize(GTK_LABEL(gtk_bin_get_child(GTK_BIN(radio1))), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start(GTK_BOX(content_area), radio1, FALSE, FALSE, 2);

	if(workspace_profile_path)
	{
		wchar_t *buf;
		guint32 buf_size;
		gssize count;
		gchar *desc = NULL, *label_text;
		cmsHPROFILE h_profile = cmsOpenProfileFromFile(workspace_profile_path, "rb");
		
		if(h_profile != NULL)
		{
			buf_size = cmsGetProfileInfo(h_profile, cmsInfoDescription, "ja", "JP", NULL, 0);
			buf = (wchar_t *)g_malloc(buf_size);
			cmsGetProfileInfo(h_profile, cmsInfoDescription, "ja", "JP", buf, buf_size);

			desc = g_convert((gchar *)buf, buf_size, "UTF-8", "UTF-16LE", NULL, &count, NULL);

			cmsCloseProfile(h_profile);
		}

		label_text = g_strdup_printf("Workspace%s%s", desc ? " : " : "", desc ? desc : "");

		radio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1), label_text);
		gtk_misc_set_alignment(GTK_MISC(gtk_bin_get_child(GTK_BIN(radio2))), 0, .5);
		gtk_label_set_width_chars(GTK_LABEL(gtk_bin_get_child(GTK_BIN(radio2))), 64);
		gtk_label_set_ellipsize(GTK_LABEL(gtk_bin_get_child(GTK_BIN(radio2))), PANGO_ELLIPSIZE_END);
		g_object_set_data(G_OBJECT(radio2), "data", g_strdup(workspace_profile_path));
		gtk_box_pack_start(GTK_BOX(content_area), radio2, FALSE, FALSE, 2);
	}

	radio3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1), "Profile : ");
	button = icc_button_new();
	icc_button_set_enable_empty(ICC_BUTTON(button), FALSE);
	icc_button_set_mask (ICC_BUTTON(button),
		ICC_BUTTON_CLASS_INPUT | ICC_BUTTON_CLASS_OUTPUT | ICC_BUTTON_CLASS_DISPLAY,
		ICC_BUTTON_COLORSPACE_XYZ | ICC_BUTTON_COLORSPACE_LAB,
		ICC_BUTTON_COLORSPACE_RGB);
	g_object_set_data(G_OBJECT(radio3), "data", icc_button_get_filename(ICC_BUTTON(button)));
	(void)g_signal_connect(button, "changed", G_CALLBACK(IccProfileChangerDialogProfileChanged), radio3);
	box = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(box), radio3, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), box, FALSE, FALSE, 2);

	(void)g_signal_connect(radio1, "toggled", G_CALLBACK(IccProfileChangerDialogSelectionChanged), dialog);
	(void)g_signal_connect(radio2, "toggled", G_CALLBACK(IccProfileChangerDialogSelectionChanged), dialog);
	(void)g_signal_connect(radio3, "toggled", G_CALLBACK(IccProfileChangerDialogSelectionChanged), dialog);

	g_object_set_data(G_OBJECT(dialog), "radio1", radio1);
	g_object_set_data(G_OBJECT(dialog), "radio2", radio2);
	g_object_set_data(G_OBJECT(dialog), "radio3", radio3);

	gtk_widget_show_all(dialog);

	return dialog;
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

typedef struct _TOGGLE_RADIO_BUTTON_DATA
{
	gboolean ui_disabled;
	GtkWidget *current;
	GList *button_list;
	void (*callback_func)(GtkWidget* button, void* data);
	void *callback_data;
} TOGGLE_RADIO_BUTTON_DATA;

static void OnDestroyToggleRadioButton(TOGGLE_RADIO_BUTTON_DATA* data)
{
	g_list_free(data->button_list);
	MEM_FREE_FUNC(data);
}

static void OnToggledToggleRadioButton(GtkWidget* button, TOGGLE_RADIO_BUTTON_DATA* data)
{
	if(data->ui_disabled != FALSE)
	{
		return;
	}

	data->ui_disabled = TRUE;
	if(button == data->current && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		data->ui_disabled = FALSE;
		return;
	}

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE
		&& data->callback_func != NULL)
	{
		GList *list = data->button_list;
		while(list != NULL)
		{
			if(list->data != button)
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(list->data), FALSE);
			}
			list = list->next;
		}

		if(data->callback_func != NULL)
		{
			data->callback_func(button, data->callback_data);
		}
	}
	data->ui_disabled = FALSE;
}

/***************************************************************
* ToggleRadioButtonNew�֐�                                     *
* ���W�I�A�N�V������ݒ肵���g�O���{�^���E�B�W�F�b�g���쐬     *
* ����                                                         *
* list			: �{�^�����X�g�̓�����GList                    *
*				  �擪�̃{�^����NULL�̓�����list��n��         *
*				  g_list_free���Ȃ�����                        *
* callback_func	: �{�^�����L����ԂɂȂ������̃R�[���o�b�N�֐� *
* callback_data	: �R�[���o�b�N�֐��ɓn���f�[�^                 *
* �Ԃ�l                                                       *
*	�{�^���E�B�W�F�b�g                                         *
***************************************************************/
GtkWidget* ToggleRadioButtonNew(
	GList** list,
	void (*callback_func)(GtkWidget* button, void* data),
	void* callback_data
)
{
	TOGGLE_RADIO_BUTTON_DATA *data;
	GtkWidget *button;
	GList *button_list;

	if(list == NULL)
	{
		return NULL;
	}
	else if(*list == NULL)
	{
		data = (TOGGLE_RADIO_BUTTON_DATA*)MEM_ALLOC_FUNC(sizeof(*data));
		(void)memset(data, 0, sizeof(*data));
		*list = data->button_list = g_list_alloc();
		(*list)->data = (gpointer)(button = gtk_toggle_button_new());
		g_object_set_data(G_OBJECT(button), "widget_data", data);
		(void)g_signal_connect_swapped(G_OBJECT(button), "destroy",
			G_CALLBACK(OnDestroyToggleRadioButton), data);
		data->callback_func = callback_func;
		data->callback_data = callback_data;
		(void)g_signal_connect(G_OBJECT(button), "toggled",
			G_CALLBACK(OnToggledToggleRadioButton), data);

		return button;
	}

	data = (TOGGLE_RADIO_BUTTON_DATA*)g_object_get_data(G_OBJECT((*list)->data), "widget_data");
	button_list = *list;
	while(button_list->next != NULL)
	{
		button_list = button_list->next;
	}
	(void)g_list_append(button_list, (gpointer)(button = gtk_toggle_button_new()));
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(OnToggledToggleRadioButton), data);

	return button;
}

#ifdef __cplusplus
}
#endif
