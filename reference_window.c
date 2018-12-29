// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "reference_window.h"
#include "utils.h"
#include "application.h"
#include "image_read_write.h"
#include "menu.h"
#include "widgets.h"
#include "memory.h"

#include "./gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eDROP_LIST
{
	DROP_URI = 20,
	NUM_DROP_LIST = 1
} eDROP_LIST;

// �֐��̃v���g�^�C�v�錾
static void ReferenceWindowOpenFile(GtkWidget *menu_item, REFERENCE_WINDOW* reference);

/***************************************************
* ReferenceWindowPasteImage�֐�                    *
* �N���b�v�{�[�h����摜�f�[�^���擾���ĕ\���ɒǉ� *
* ����                                             *
* menu_item	: ���j���[�A�C�e���E�B�W�F�b�g         *
* reference	: �Q�l�p�摜�\���E�B���h�E�̏��       *
***************************************************/
static void ReferenceWindowPasteImage(GtkWidget* menu_item, REFERENCE_WINDOW* reference);

/*********************************************
* ReferenceWindowCloseImage�֐�              *
* �摜�f�[�^�����                         *
* ����                                       *
* menu_item	: ���j���[�A�C�e���E�B�W�F�b�g   *
* reference	: �Q�l�p�摜�\���E�B���h�E�̏�� *
*********************************************/
static void ReferenceWindowCloseImage(GtkWidget* menu_item, REFERENCE_WINDOW* reference);

/*******************************************************
* OnDeleteReferenceWindow�֐�                          *
* �Q�l�p�摜�\���E�B���h�E�������鎞�ɌĂяo����� *
* ����                                                 *
* window		: �Q�l�p�摜�\���E�B���h�E�E�B�W�F�b�g *
* event_info	: �폜�C�x���g�̏��                   *
* reference		: �Q�l�p�摜�\���E�B���h�E�̏��       *
* �Ԃ�l                                               *
*	���FALSE                                          *
*******************************************************/
static gboolean OnDeleteReferenceWindow(
	GtkWidget* window,
	GdkEvent* event_info,
	REFERENCE_WINDOW* reference
)
{
	gtk_window_get_position(GTK_WINDOW(window),
		&reference->window_x, &reference->window_y);
	gtk_window_get_size(GTK_WINDOW(window),
		&reference->window_width, &reference->window_height);

	reference->app->flags |= APPLICATION_IN_DELETE_EVENT;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(reference->menu_item), FALSE);
	reference->app->flags &= ~(APPLICATION_IN_DELETE_EVENT);

	MEM_FREE_FUNC(reference->data);
	reference->data = NULL;

	return FALSE;
}

/*******************************************************
* OnDeleteReferenceWindow�֐�                          *
* �Q�l�p�摜�\���E�B���h�E�j�����ɌĂяo�����         *
* ����                                                 *
* window		: �Q�l�p�摜�\���E�B���h�E�E�B�W�F�b�g *
* reference		: �Q�l�p�摜�\���E�B���h�E�̏��       *
*******************************************************/
static void OnDestroyReferenceWindow(GtkWidget* window, REFERENCE_WINDOW* reference)
{
	if(reference->data != NULL)
	{
		reference->app->flags |= APPLICATION_IN_DELETE_EVENT;
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(reference->menu_item), FALSE);
		reference->app->flags &= ~(APPLICATION_IN_DELETE_EVENT);

		MEM_FREE_FUNC(reference->data);
		reference->data = NULL;
	}
}

/***************************************************************
* OnCloseReferenceImage�֐�                                    *
* �Q�l�p�摜�̃^�u�������鎞�ɌĂяo�����R�[���o�b�N�֐� *
* ����                                                         *
* data	: �Q�l�p�摜�̃f�[�^                                   *
* page	: ����^�u��ID                                       *
* �Ԃ�l                                                       *
*	���FALSE                                                  *
***************************************************************/
static gboolean OnCloseReferenceImage(void* data, gint page)
{
	REFERENCE_WINDOW *reference = (REFERENCE_WINDOW*)data;
	int i;

	for(i=page+1; i<reference->data->num_image; i++)
	{
		// �^�u�ɐݒ肳��Ă���y�[�W�ԍ����Đݒ�
		g_object_set_data(G_OBJECT(g_object_get_data(G_OBJECT(
			gtk_notebook_get_tab_label(GTK_NOTEBOOK(reference->data->note_book),
				reference->data->images[i]->scroll)), "button_widget")), "page", GINT_TO_POINTER(i-1));
	}

	(void)memmove(&reference->data->images[page], &reference->data->images[page+1],
		sizeof(reference->data->images[0]) * (reference->data->num_image - page));

	if(reference->data->active_image >= page)
	{
		reference->data->active_image--;
	}

	if(reference->data->active_image < 0)
	{
		reference->data->active_image = 0;
	}

	reference->data->num_image--;
	if(reference->data->num_image <= 0)
	{
		gtk_widget_set_sensitive(reference->data->scale, FALSE);
	}

	return FALSE;
}

/*************************************************************************
* OnDestroyReferenceImageDrawArea�֐�                                    *
* �Q�l�p�\���摜�E�B�W�F�b�g���j������鎞�ɌĂяo�����R�[���o�b�N�֐� *
* ����                                                                   *
* widget	: �Q�l�p�摜�\���E�B�W�F�b�g                                 *
* image		: �摜�f�[�^                                                 *
*************************************************************************/
static void OnDestroyReferenceImageDrawArea(GtkWidget* widget, REFERENCE_IMAGE* image)
{
	cairo_surface_destroy(image->surface_p);
	MEM_FREE_FUNC(image->pixels);
	image->pixels = NULL;
}

/***************************************************
* DisplayReferenceImage�֐�                        *
* �Q�l�p�摜�̕`�揈��                             *
* ����                                             *
* widget		: �Q�l�p�摜��`�悷��E�B�W�F�b�g *
* event_info	: �`��C�x���g�̏��               *
* image			: �Q�l�p�摜�̃f�[�^               *
* �Ԃ�l                                           *
*	���TRUE                                       *
***************************************************/
#if GTK_MAJOR_VERSION <= 2
static gboolean DisplayReferenceImage(
	GtkWidget* widget,
	GdkEventExpose* event_info,
	REFERENCE_IMAGE* image
)
#else
static gboolean DisplayReferenceImage(
	GtkWidget* widget,
	cairo_t* cairo_p,
	REFERENCE_IMAGE* image
)
#endif
{
#if GTK_MAJOR_VERSION <= 2
	//cairo_t *cairo_p = kaburagi_cairo_create((struct _GdkWindow*)widget->window);
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
	gdk_cairo_region(cairo_p, event_info->region);
	cairo_clip(cairo_p);
#endif
	cairo_scale(cairo_p, image->zoom, image->zoom);
	cairo_set_source_surface(cairo_p, image->surface_p, 0, 0);
	cairo_paint(cairo_p);
#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

/*******************************************
* ExecuteAddColor2Pallete�֐�              *
* �p���b�g�ɐF��ǉ�����                   *
* ����                                     *
* menu_item	: ���j���[�A�C�e���E�B�W�F�b�g *
* chooser	: �F�I���E�B�W�F�b�g�̏��     *
*******************************************/
static void ExecuteAddColor2Pallete(GtkWidget* menu_item, COLOR_CHOOSER* chooser)
{
	guint color_value = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menu_item), "color_data"));
	uint8 *color = (uint8*)&color_value;
	RegisterColorPallete(chooser, color);
}

/***************************************************
* ReferenceImageButtonPress�֐�                    *
* �Q�l�p�摜��ł̃}�E�X�N���b�N���̏���           *
* ����                                             *
* widget		: �Q�l�p�摜��`�悷��E�B�W�F�b�g *
* event_info	: �}�E�X�N���b�N�C�x���g�̏��     *
* image			: �Q�l�p�摜�̃f�[�^               *
* �Ԃ�l                                           *
*	���TRUE                                       *
***************************************************/
static gboolean ReferenceImageButtonPress(
	GtkWidget* widget,
	GdkEventButton* event_info,
	REFERENCE_IMAGE* image
)
{
	uint8 color[4] = {0};
	guint *color_value = (guint*)color;
	int x = (int)(event_info->x * image->rev_zoom);
	int y = (int)(event_info->y * image->rev_zoom);
	int index = y * image->stride + x * image->channel;

	color[0] = image->pixels[index+0];
	color[1] = image->pixels[index+1];
	color[2] = image->pixels[index+2];

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	{
		uint8 r = color[2];
		color[2] = color[0];
		color[0] = r;
	}
#endif

	if(event_info->button == 1)
	{
		image->app->tool_window.color_chooser->rgb[0] = color[0];
		image->app->tool_window.color_chooser->rgb[1] = color[1];
		image->app->tool_window.color_chooser->rgb[2] = color[2];

		RGB2HSV_Pixel(color, &image->app->tool_window.color_chooser->hsv);

		SetColorChooserPoint(image->app->tool_window.color_chooser,
			&image->app->tool_window.color_chooser->hsv, TRUE);

		if((event_info->state & GDK_CONTROL_MASK) != 0)
		{
			RegisterColorPallete(image->app->tool_window.color_chooser, color);
		}
	}
	else if(event_info->button == 3)
	{
		GtkWidget *menu = gtk_menu_new();
		GtkWidget *menu_item;

		menu_item = gtk_menu_item_new_with_label(image->app->labels->tool_box.pallete_add);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteAddColor2Pallete), image->app->tool_window.color_chooser);
		g_object_set_data(G_OBJECT(menu_item), "color_data", GUINT_TO_POINTER(*color_value));

		menu_item = gtk_menu_item_new_with_label(image->app->labels->menu.open);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ReferenceWindowOpenFile), &image->app->reference_window);

		menu_item = gtk_menu_item_new_with_label(image->app->labels->menu.paste);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ReferenceWindowPasteImage), &image->app->reference_window);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(image->app->labels->menu.close);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ReferenceWindowCloseImage), &image->app->reference_window);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(image->app->labels->window.cancel);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event_info->button, event_info->time);
		gtk_widget_show_all(menu);
	}

	return TRUE;
}

/*********************************************
* AddReferenceImage�֐�                      *
* �Q�l�p�摜�̕\����ǉ�                     *
* ����                                       *
* reference	: �Q�l�p�摜�\���E�B���h�E�̏�� *
* file_name	: �ǉ�����t�@�C����             *
* pixels	: �摜�̃s�N�Z���f�[�^           *
* width		: �摜�̕�                       *
* height	: �摜�̍���                     *
* stride	: �摜��1�s���̃o�C�g��          *
* channel	: �摜�̃`�����l����             *
*********************************************/
static void AddReferenceImage(
	REFERENCE_WINDOW* reference,
	const char* file_name,
	uint8* pixels,
	int width,
	int height,
	int stride,
	int channel
)
{
	GtkWidget *tab_label;
	GtkWidget *alignment;
	cairo_format_t format = (stride / width == 4) ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24;

	REFERENCE_IMAGE *image;
	image = reference->data->images[reference->data->num_image] =
		(REFERENCE_IMAGE*)MEM_ALLOC_FUNC(sizeof(**reference->data->images));
	image->pixels = pixels;
	image->width = width;
	image->height = height;
	image->stride = stride;
	image->channel = (uint8)channel;
	image->surface_p = cairo_image_surface_create_for_data(image->pixels,
		format, image->width, image->height, image->stride);
	image->zoom = image->rev_zoom = 1;
	image->app = reference->app;

	image->draw_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(image->draw_area, image->width, image->height);
	gtk_widget_set_events(image->draw_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
		| GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
	(void)g_signal_connect(G_OBJECT(image->draw_area), "destroy",
		G_CALLBACK(OnDestroyReferenceImageDrawArea), image);
#if GTK_MAJOR_VERSION <= 2
	(void)g_signal_connect(G_OBJECT(image->draw_area), "expose_event",
		G_CALLBACK(DisplayReferenceImage), image);
#else
	(void)g_signal_connect(G_OBJECT(image->draw_area), "draw",
		G_CALLBACK(DisplayReferenceImage), image);
#endif
	(void)g_signal_connect(G_OBJECT(image->draw_area), "button_press_event",
		G_CALLBACK(ReferenceImageButtonPress), image);
	alignment = gtk_alignment_new(0.5f, 0.5f, 0.0f, 0.0f);
	gtk_container_add(GTK_CONTAINER(alignment), image->draw_area);

	image->scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(image->scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(image->scroll), alignment);

	tab_label = CreateNotebookLabel(file_name, reference->data->note_book, reference->data->num_image,
		OnCloseReferenceImage, reference);
	gtk_notebook_append_page(GTK_NOTEBOOK(reference->data->note_book), image->scroll, tab_label);

	gtk_widget_show_all(image->scroll);
	gtk_widget_set_sensitive(reference->data->scale, TRUE);
	reference->data->active_image = reference->data->num_image;
#if GTK_MAJOR_VERSION <= 2
	gtk_notebook_set_page(GTK_NOTEBOOK(reference->data->note_book), reference->data->active_image);
#else
	gtk_notebook_set_current_page(GTK_NOTEBOOK(reference->data->note_book), reference->data->active_image);
#endif

	reference->data->num_image++;
}

/*********************************************
* OpenAsReferenceImage�֐�                   *
* �Q�l�p�摜���J��                           *
* ����                                       *
* file_path	: �t�@�C���̃p�X                 *
* reference	: �Q�l�p�摜�\���E�B���h�E�̏�� *
*********************************************/
void OpenAsReferenceImage(char* file_path, REFERENCE_WINDOW* reference)
{
	// �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X
	APPLICATION *app = reference->app;
	// �s�N�Z���f�[�^�擾�p
	GdkPixbuf *pixbuf;
	// �g���q����p
	gchar *str;
	// �t�@�C����
	gchar *file_name;

	// �g���q�擾
	str = file_path + strlen(file_path) - 1;
	while(*str != '.' && str > file_path)
	{
		str--;
	}

	// �t�@�C�����擾
	file_name = file_path + strlen(file_path) - 1;
	while(*file_name != '/' && *file_name != '\\' && file_name > file_path)
	{
		file_name--;
	}
	if(file_name != file_path)
	{
		file_name++;
	}

	if(StringCompareIgnoreCase(str, ".kab") == 0)
	{
		// �V�X�e���̕����R�[�h�֕ϊ�
		gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		FILE *fp = fopen(system_path, "rb");
		// �t�@�C���T�C�Y
		size_t data_size;
		// �Ǎ������b�Z�[�W��ID
		guint context_id, message_id;
		// �\���C�x���g�����p
		GdkEvent *queued_event;
		// �����������C���[
		LAYER *mixed;

		g_free(system_path);

		if(fp == NULL)
		{
			return;
		}

		// �Ǎ����̃��b�Z�[�W��\��
		context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(app->widgets->status_bar), "Loading");
		message_id = gtk_statusbar_push(GTK_STATUSBAR(app->widgets->status_bar),
			context_id, app->labels->window.loading);
		// �C�x���g���񂵂ă��b�Z�[�W��\��
#if GTK_MAJOR_VERSION <= 2
		gdk_window_process_updates(app->widgets->status_bar->window, TRUE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->widgets->status_bar), TRUE);
#endif

		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if GTK_MAJOR_VERSION <= 2
				if(queued_event->any.window == app->widgets->status_bar->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->widgets->status_bar)
#endif
					&& queued_event->any.type == GDK_EXPOSE)
				{
					gdk_event_free(queued_event);
					break;
				}
				else
				{
					gdk_event_free(queued_event);
				}
			}
		}

		(void)fseek(fp, 0, SEEK_END);
		data_size = (size_t)ftell(fp);
		rewind(fp);

		mixed = ReadOriginalFormatMixedData(fp, (stream_func_t)fread, data_size, app, file_name);
		if(mixed != NULL)
		{
			uint8 *pixels = (uint8*)MEM_ALLOC_FUNC(mixed->stride * mixed->height);
			(void)memcpy(pixels, mixed->pixels, mixed->stride * mixed->height);
			AddReferenceImage(reference, file_name, pixels,
				mixed->width, mixed->height, mixed->stride, mixed->channel);

			cairo_destroy(mixed->cairo_p);
			cairo_surface_destroy(mixed->surface_p);
			MEM_FREE_FUNC(mixed->name);

			MEM_FREE_FUNC(mixed->pixels);

			MEM_FREE_FUNC(mixed);
		}
	}
	else
	{
		GError *error = NULL;
		pixbuf = gdk_pixbuf_new_from_file(file_path, &error);

		// �t�@�C���I�[�v������
		if(pixbuf != NULL)
		{
			int width = gdk_pixbuf_get_width(pixbuf);
			int height = gdk_pixbuf_get_height(pixbuf);
			int stride;
			int channel;
			uint8 *pixels;
			uint8 *src_pixels;
			int dst_stride;
			int i, j;

			if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
			{
				GdkPixbuf *old_buf = pixbuf;
				pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
				g_object_unref(old_buf);
			}


			stride = gdk_pixbuf_get_rowstride(pixbuf);
			channel = stride / width;
			dst_stride = cairo_format_stride_for_width(
				(channel == 4) ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, width);

			src_pixels = gdk_pixbuf_get_pixels(pixbuf);
			pixels = (uint8*)MEM_ALLOC_FUNC(dst_stride * height);
			(void)memset(pixels, 0, dst_stride * height);
			for(i=0; i<height; i++)
			{
				for(j=0; j<width; j++)
				{
					pixels[i*dst_stride+j*4+0] =
						src_pixels[i*stride+j*4+2];
					pixels[i*dst_stride+j*4+1] =
						src_pixels[i*stride+j*4+1];
					pixels[i*dst_stride+j*4+2] =
						src_pixels[i*stride+j*4+0];
					pixels[i*dst_stride+j*4+3] =
						src_pixels[i*stride+j*4+3];
				}
			}

			AddReferenceImage(
				reference,
				file_name,
				pixels,
				width,
				height,
				dst_stride,
				channel
			);

			g_object_unref(pixbuf);
		}

		g_error_free(error);
	}
}

/*********************************************
* ReferenceWindowCloseImage�֐�              *
* �摜�f�[�^�����                         *
* ����                                       *
* menu_item	: ���j���[�A�C�e���E�B�W�F�b�g   *
* reference	: �Q�l�p�摜�\���E�B���h�E�̏�� *
*********************************************/
static void ReferenceWindowCloseImage(GtkWidget* menu_item, REFERENCE_WINDOW* reference)
{
	if(reference->data != NULL)
	{
		if(reference->data->num_image > 0)
		{
			int close_page = reference->data->active_image;

			(void)OnCloseReferenceImage((void*)reference, reference->data->active_image);
			gtk_notebook_remove_page(GTK_NOTEBOOK(reference->data->note_book), close_page);
		}
	}
}

/**********************************************************************
* DragDataRecieveCallBack�֐�                                         *
* �t�@�C�����h���b�O&�h���b�v���ꂽ�Ƃ��ɌĂяo�����R�[���o�b�N�֐� *
* ����                                                                *
* widget			: �h���b�v��̃E�B�W�F�b�g                        *
* context			: �h���b�O&�h���b�v�̏��                         *
* x					: �h���b�v���ꂽ�Ƃ��̃}�E�X��X���W               *
* y					: �h���b�v���ꂽ�Ƃ��̃}�E�X��Y���W               *
* selection_data	: �h���b�O&�h���b�v�̃f�[�^                       *
* target_type		: �h���b�v���ꂽ�f�[�^�̃^�C�v                    *
* time_stamp		: �h���b�v���ꂽ����                              *
* reference			: �Q�l�p�摜�\���E�B���h�E�̏��                  *
**********************************************************************/
static void DragDataRecieveCallBack(
	GtkWidget* widget,
	GdkDragContext* context,
	gint x,
	gint y,
	GtkSelectionData* selection_data,
	guint target_type,
	guint time_stamp,
	REFERENCE_WINDOW* reference
)
{
	if((selection_data != NULL) && (gtk_selection_data_get_length(selection_data) >= 0))
	{
		if(target_type == DROP_URI)
		{
			gchar *uri = (gchar*)gtk_selection_data_get_data(selection_data);
			gchar *file_path = g_filename_from_uri(uri, NULL, NULL);
			gchar *c = file_path;

			while(*c != '\0')
			{
				if(*c == '\r')
				{
					*c = '\0';
					break;
				}
				c = g_utf8_next_char(c);
			}

			OpenAsReferenceImage(file_path, reference);

			g_free(file_path);
		}

		gtk_drag_finish(context, TRUE, TRUE, time_stamp);
	}
}

static void ReferenceWindowOpenFile(GtkWidget *menu_item, REFERENCE_WINDOW* reference)
{
	// �t�@�C�����J���_�C�A���O
	GtkWidget* chooser = gtk_file_chooser_dialog_new(
		reference->app->labels->menu.open,
		GTK_WINDOW(reference->data->window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL
	);

	// �v���r���[���Z�b�g
	SetFileChooserPreview(chooser);

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
	{	// �t�@�C�����J���ꂽ
		gchar* file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		
		OpenAsReferenceImage(file_path, reference);

		g_free(file_path);
	}

	// �t�@�C���I���_�C�A���O�����
	gtk_widget_destroy(chooser);
}

/***********************************************************
* ClipBoardImageRecieveCallBack�֐�                        *
* �N���b�v�{�[�h�̉摜�f�[�^���󂯂������̃R�[���o�b�N�֐� *
* ����                                                     *
* clipboard	: �N���b�v�{�[�h�̏��                         *
* pixbuf	: �s�N�Z���o�b�t�@                             *
* data		: �A�v���P�[�V�������Ǘ�����\���̂�           *
***********************************************************/
static void ClipBoardImageRecieveCallBack(
	GtkClipboard *clipboard,
	GdkPixbuf* pixbuf,
	gpointer data
)
{
	REFERENCE_WINDOW *reference = (REFERENCE_WINDOW*)data;
	gboolean delete_pixbuf = FALSE;

	if(pixbuf != NULL)
	{
		int width = gdk_pixbuf_get_width(pixbuf);
		int height = gdk_pixbuf_get_height(pixbuf);
		int stride;
		int channel;
		uint8 *pixels;
		uint8 *src_pixels;
		int dst_stride;
		int i, j;

		if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
		{
			pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
			delete_pixbuf = TRUE;
		}

		stride = gdk_pixbuf_get_rowstride(pixbuf);
		channel = stride / width;
		dst_stride = cairo_format_stride_for_width(
			(channel == 4) ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, width);

		src_pixels = gdk_pixbuf_get_pixels(pixbuf);
		pixels = (uint8*)MEM_ALLOC_FUNC(dst_stride * height);
		(void)memset(pixels, 0, dst_stride * height);
		for(i=0; i<height; i++)
		{
			for(j=0; j<width; j++)
			{
				pixels[i*dst_stride+j*4+0] =
					src_pixels[i*stride+j*4+2];
				pixels[i*dst_stride+j*4+1] =
					src_pixels[i*stride+j*4+1];
				pixels[i*dst_stride+j*4+2] =
					src_pixels[i*stride+j*4+0];
				pixels[i*dst_stride+j*4+3] =
					src_pixels[i*stride+j*4+3];
			}
		}

		AddReferenceImage(
			reference,
			reference->app->labels->unit.clip_board,
			pixels,
			width,
			height,
			dst_stride,
			channel
		);

		if(delete_pixbuf != FALSE)
		{
			g_object_unref(pixbuf);
		}
	}
}

/***************************************************
* ReferenceWindowPasteImage�֐�                    *
* �N���b�v�{�[�h����摜�f�[�^���擾���ĕ\���ɒǉ� *
* ����                                             *
* menu_item	: ���j���[�A�C�e���E�B�W�F�b�g         *
* reference	: �Q�l�p�摜�\���E�B���h�E�̏��       *
***************************************************/
static void ReferenceWindowPasteImage(GtkWidget* menu_item, REFERENCE_WINDOW* reference)
{
	// �N���b�v�{�[�h�ɑ΂��ăf�[�^��v��
	gtk_clipboard_request_image(gtk_clipboard_get(GDK_NONE),
		ClipBoardImageRecieveCallBack, reference);
}

/*****************************************************
* ReferenceWindowButtonPress�֐�                     *
* �Q�l�p�摜�\���E�B���h�E��ł̃}�E�X�N���b�N�̏��� *
* ����                                               *
* window		: �E�B���h�E�E�B�W�F�b�g             *
* event_info	: �}�E�X�N���b�N�̏��               *
* reference		: �Q�l�p�摜�\���E�B���h�E�̏��     *
* �Ԃ�l                                             *
*	���TRUE                                         *
*****************************************************/
static gboolean ReferenceWindowButtonPress(
	GtkWidget* window,
	GdkEventButton* event_info,
	REFERENCE_WINDOW* reference
)
{
	if(event_info->button == 3)
	{
		GtkWidget *menu = gtk_menu_new();
		GtkWidget *menu_item;

		menu_item = gtk_menu_item_new_with_label(reference->app->labels->menu.open);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ReferenceWindowOpenFile), reference);

		menu_item = gtk_menu_item_new_with_label(reference->app->labels->menu.paste);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ReferenceWindowPasteImage), reference);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(reference->app->labels->menu.close);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ReferenceWindowCloseImage), reference);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(reference->app->labels->window.cancel);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event_info->button, event_info->time);
		gtk_widget_show_all(menu);
	}

	return TRUE;
}

/*********************************************************
* ReferenceWindowMenuNew�֐�                             *
* �Q�l�p�摜�\���E�B���h�E�̃��j���[���쐬               *
* ����                                                   *
* reference	: �Q�l�p�摜�\���E�B���h�E�̏��             *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	���j���[�o�[�E�B�W�F�b�g                             *
*********************************************************/
static GtkWidget* ReferenceWindowMenuNew(REFERENCE_WINDOW* reference, APPLICATION* app)
{
	GtkWidget *menu_bar = gtk_menu_bar_new();
	GtkWidget *menu;
	GtkWidget *menu_item;
	GtkAccelGroup *accel_group;
	char buff[1024];

	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(reference->data->window), accel_group);

	// �u�t�@�C���v���j���[
	(void)sprintf(buff, "_%s(_F)", app->labels->menu.file);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'F', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// �u�t�@�C���v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u�J���v���j���[
	(void)sprintf(buff, "%s", app->labels->menu.open);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'O', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ReferenceWindowOpenFile), reference);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u����v���j���[
	(void)sprintf(buff, "%s", app->labels->menu.close);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'W', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ReferenceWindowCloseImage), reference);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�E�B���h�E�����v���j���[
	(void)sprintf(buff, "%s", app->labels->window.close_window);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
#if GTK_MAJOR_VERSION <= 2
		GDK_Escape, 0, GTK_ACCEL_VISIBLE);
#else
		GDK_KEY_Escape, 0, GTK_ACCEL_VISIBLE);
#endif
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(gtk_widget_destroy), reference->data->window);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�ҏW�v���j���[
	(void)sprintf(buff, "_%s(_E)", app->labels->menu.edit);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'E', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// �u�ҏW�v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u�\��t���v���j���[
	(void)sprintf(buff, "%s", app->labels->menu.paste);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'V', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ReferenceWindowPasteImage), reference);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	return menu_bar;
}

/*********************************************
* ReferenceWindowChangeZoom�֐�              *
* �Q�l�p�摜�̕\���{����ύX                 *
* ����                                       *
* slider	: �X���C�_�̃A�W���X�^           *
* reference	: �Q�l�p�摜�\���E�B���h�E�̏�� *
*********************************************/
void ReferenceWindowChangeZoom(GtkAdjustment* slider, REFERENCE_WINDOW* reference)
{
	if(reference->data != NULL)
	{
		REFERENCE_IMAGE *image = reference->data->images[reference->data->active_image];
		if(image != NULL)
		{
			image->zoom = gtk_adjustment_get_value(slider) * 0.01;
			image->rev_zoom = 1 / image->zoom;

			gtk_widget_set_size_request(image->draw_area,
				(gint)(image->width * image->zoom), (gint)(image->height * image->zoom));
			gtk_widget_queue_draw(image->draw_area);
		}
	}
}

/*****************************************************
* OnChangeCurrentTab�֐�                             *
* �^�u���؂�ւ�������̊֐�                         *
* ����                                               *
* note_book			: �^�u�E�B�W�F�b�g               *
* note_book_pate	: �^�u�̏��                     *
* page				: �^�u��ID                       *
* reference			: �Q�l�p�摜�\���E�B���h�E�̏�� *
*****************************************************/
static void OnChangeCurrentTab(
	GtkNotebook* note_book,
	GtkWidget* note_book_page,
	gint page,
	REFERENCE_WINDOW* reference
)
{
	reference->data->active_image = (int16)page;
	gtk_adjustment_set_value(reference->data->scale_adjust,
		reference->data->images[reference->data->active_image]->zoom * 100);
}

/*********************************************************
* InitializeReferenceWindow�֐�                          *
* �Q�l�p�摜�\���E�B���h�E�̏�����                       *
* ����                                                   *
* reference	: �Q�l�p�摜�\���E�B���h�E�̏��             *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
void InitializeReferenceWindow(REFERENCE_WINDOW* reference, struct _APPLICATION* app)
{
	GtkWidget *vbox;
	GtkWidget *menu_bar;
	GtkTargetEntry target_list[] = {{"text/uri-list", 0, DROP_URI}};

	reference->data = (REFERENCE_WINDOW_DATA*)MEM_ALLOC_FUNC(sizeof(*reference->data));
	(void)memset(reference->data, 0, sizeof(*reference->data));
	reference->app = app;

	reference->data->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_events(reference->data->window, GDK_BUTTON_PRESS_MASK);
	gtk_window_move(GTK_WINDOW(reference->data->window), reference->window_x, reference->window_y);
	gtk_window_resize(GTK_WINDOW(reference->data->window), reference->window_width, reference->window_height);
	(void)g_signal_connect(G_OBJECT(reference->data->window), "delete_event",
		G_CALLBACK(OnDeleteReferenceWindow), reference);
	(void)g_signal_connect(G_OBJECT(reference->data->window), "destroy",
		G_CALLBACK(OnDestroyReferenceWindow), reference);
	(void)g_signal_connect(G_OBJECT(reference->data->window), "button_press_event",
		G_CALLBACK(ReferenceWindowButtonPress), reference);
	gtk_window_set_title(GTK_WINDOW(reference->data->window), app->labels->window.reference);
	gtk_window_set_type_hint(GTK_WINDOW(reference->data->window), GDK_WINDOW_TYPE_HINT_UTILITY);
	gtk_window_set_transient_for(GTK_WINDOW(reference->data->window), GTK_WINDOW(app->widgets->window));
	// �^�X�N�o�[�ɂ͕\�����Ȃ�
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(reference->data->window), TRUE);

	// �t�@�C���̃h���b�v��ݒ�
	gtk_drag_dest_set(reference->data->window, GTK_DEST_DEFAULT_ALL, target_list,
		NUM_DROP_LIST, GDK_ACTION_COPY);
	gtk_drag_dest_add_uri_targets(reference->data->window);
	(void)g_signal_connect(G_OBJECT(reference->data->window), "drag-data-received",
		G_CALLBACK(DragDataRecieveCallBack), reference);

	vbox = gtk_vbox_new(FALSE, 0);
	menu_bar = ReferenceWindowMenuNew(reference, app);
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
	reference->data->note_book = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(reference->data->note_book), GTK_POS_BOTTOM);
	(void)g_signal_connect(G_OBJECT(reference->data->note_book), "switch_page",
		G_CALLBACK(OnChangeCurrentTab), reference);
	gtk_box_pack_start(GTK_BOX(vbox), reference->data->note_book, TRUE, TRUE, 0);

	reference->data->scale_adjust = GTK_ADJUSTMENT(gtk_adjustment_new(
		100, 10, 400, 10, 10, 0));
	(void)g_signal_connect(G_OBJECT(reference->data->scale_adjust), "value_changed",
		G_CALLBACK(ReferenceWindowChangeZoom), reference);
	reference->data->scale = SpinScaleNew(reference->data->scale_adjust, app->labels->menu.zoom, 0);
	gtk_widget_set_sensitive(reference->data->scale, FALSE);
	gtk_box_pack_end(GTK_BOX(vbox), reference->data->scale, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(reference->data->window), vbox);
	gtk_widget_show_all(reference->data->window);
}

/***************************************************************
* DisplayReferenceWindowMenuActivate�֐�                       *
* �Q�l�p�摜�\���E�B���h�E��\�����郁�j���[�̃R�[���o�b�N�֐� *
* ����                                                         *
* menu_item	: ���j���[�A�C�e���E�B�W�F�b�g                     *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X       *
***************************************************************/
void DisplayReferenceWindowMenuActivate(GtkWidget* menu_item, struct _APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_DELETE_EVENT) != 0)
	{
		return;
	}

	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)) == FALSE)
	{
		if(app->reference_window.data != NULL)
		{
			gtk_widget_destroy(app->reference_window.data->window);
		}
	}
	else
	{
		if(app->reference_window.data == NULL)
		{
			InitializeReferenceWindow(&app->reference_window, app);
		}
	}
}

#ifdef __cplusplus
}
#endif
