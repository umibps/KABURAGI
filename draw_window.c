// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <math.h>
#include "memory.h"
#include "draw_window.h"
#include "display.h"
#include "layer_window.h"
#include "application.h"
#include "widgets.h"
#include "selection_area.h"
#include "memory_stream.h"
#include "image_read_write.h"
#include "utils.h"

# include "MikuMikuGtk+/ui.h"

#include "./gui/GTK/input_gtk.h"
#include "./gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAW_AREA_FRAME_RATE 60

/*********************************************
* TimerCallBack�֐�                          *
* ����(1/60�b)�ŌĂяo�����R�[���o�b�N�֐� *
* ����                                       *
* window	: �Ή�����`��̈�               *
* �Ԃ�l                                     *
*	���TRUE                                 *
*********************************************/
gboolean TimerCallBack(DRAW_WINDOW* window)
{
	if((window->flags & (DRAW_WINDOW_UPDATE_ACTIVE_OVER | DRAW_WINDOW_UPDATE_ACTIVE_UNDER)) != 0)
	{
		gtk_widget_queue_draw(window->window);
	}
	else if(g_timer_elapsed(window->timer, NULL) >= (FLOAT_T)1/DRAW_AREA_FRAME_RATE)
	{
		g_timer_start(window->timer);

		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			gtk_widget_queue_draw(window->window);
		}
	}

	return TRUE;
}

#if GTK_MAJOR_VERSION >= 3
static gboolean FirstDrawCheckCallBack(DRAW_WINDOW* window)
{
	if((window->flags & DRAW_WINDOW_FIRST_DRAW) != 0)
	{
		return FALSE;
	}

	gtk_widget_set_size_request(window->window, window->disp_size + 1, window->disp_size + 1);
	gtk_widget_show(window->window);
	gtk_widget_queue_draw(window->window);
	
	gtk_widget_set_size_request(window->window, window->disp_size, window->disp_size);
	gtk_widget_show(window->window);
	gtk_widget_queue_draw(window->window);

	return TRUE;
}
#endif

static gboolean AutoSaveCallBack(DRAW_WINDOW* window)
{
	if(g_timer_elapsed(window->auto_save_timer, NULL) >= window->app->preference.auto_save_time)
	{
		AutoSave(window);
	}

	return TRUE;
}

/*****************************************
* AutoSave�֐�                           *
* �o�b�N�A�b�v�Ƃ��ăt�@�C����ۑ�����   *
* ����                                   *
* window	: �o�b�N�A�b�v�����`��̈� *
*****************************************/
void AutoSave(DRAW_WINDOW* window)
{
	FILE *fp;
	char file_name[4096];
	gchar *path, *system_path, *temp_path;

	(void)sprintf(file_name, "%d.kbt", GetWindowID(window, window->app));
	if(window->app->backup_directory_path[0] == '.'
		&& (window->app->backup_directory_path[1] == '/' || window->app->backup_directory_path[1] == '\\'))
	{
		path = g_build_filename(window->app->current_path, "kabtmp", NULL);
		temp_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);
		g_free(path);
		path = g_build_filename(window->app->current_path, file_name, NULL);
	}
	else
	{
		path = g_build_filename(window->app->backup_directory_path, "kabtmp", NULL);
		temp_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);
		g_free(path);
		path = g_build_filename(window->app->backup_directory_path, file_name, NULL);
	}
	system_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);

	fp = fopen(temp_path, "wb");

	if(fp != NULL)
	{
#if GTK_MAJOR_VERSION >= 3
		GdkWindow *status_window = gtk_widget_get_window(window->app->widgets->status_bar);
#endif
		guint context_id = gtk_statusbar_get_context_id(
			GTK_STATUSBAR(window->app->widgets->status_bar), "Execute Back Up");
		guint message_id = gtk_statusbar_push(GTK_STATUSBAR(window->app->widgets->status_bar),
			context_id, window->app->labels->status_bar.auto_save);
		GdkEvent *queued_event;
		while(gtk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event == NULL)
			{
				break;
			}

#if GTK_MAJOR_VERSION <= 2
			if(queued_event->any.window == window->app->widgets->status_bar->window)
#else
			if(queued_event->any.window == status_window)
#endif
			{
				gdk_event_free(queued_event);
				break;
			}
			else
			{
				gdk_event_free(queued_event);
			}
		}

		WriteOriginalFormat((void*)fp, (stream_func_t)fwrite, window, 0, 3);
#ifdef _DEBUG
		(void)printf("Execute Auto Save.\n");
#endif
		(void)fclose(fp);
		(void)remove(system_path);
		(void)rename(temp_path, system_path);
		(void)remove(temp_path);

		gtk_statusbar_remove(GTK_STATUSBAR(window->app->widgets->status_bar),
			context_id, message_id);
		while(gtk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event == NULL)
			{
				break;
			}

#if GTK_MAJOR_VERSION <= 2
			if(queued_event->any.window == window->app->widgets->status_bar->window)
#else
			if(queued_event->any.window == status_window)
#endif
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

	g_free(path);
	g_free(system_path);
	g_free(temp_path);

	g_timer_start(window->auto_save_timer);
}

/***********************************************
* OnCloseDrawWindow�֐�                        *
* �^�u��������Ƃ��̃R�[���o�b�N�֐�       *
* ����                                         *
* data	: �`��̈�̃f�[�^                     *
* page	: ����^�u��ID                       *
* �Ԃ�l                                       *
*	���鑀��̒��~:TRUE ���鑀�쑱�s:FALSE *
***********************************************/
gboolean OnCloseDrawWindow(void* data, gint page)
{
	// �`��̈�̏��ɃL���X�g
	DRAW_WINDOW* window = (DRAW_WINDOW*)data;
	// for���p�̃J�E���^
	int i, j;

	if((window->history.flags & HISTORY_UPDATED) != 0)
	{	// �ۑ����邩�ǂ����̃E�B���h�E��\��
		GtkWidget* dialog = gtk_dialog_new_with_buttons(
			NULL,
			GTK_WINDOW(window->app->widgets->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_SAVE,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_NO,
			GTK_RESPONSE_NO,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
		// �_�C�A���O�ɓ���郉�x��
		GtkWidget* label = gtk_label_new(window->app->labels->save.close_with_unsaved_chage);
		// �_�C�A���O�̑I������
		gint result;

		// �_�C�A���O�Ƀ��x��������
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
			label, FALSE, FALSE, 0);
		gtk_widget_show(label);

		// �_�C�A���O�����s
		result = gtk_dialog_run(GTK_DIALOG(dialog));

		// �_�C�A���O�����
		gtk_widget_destroy(dialog);

		// �_�C�A���O�̑I���ɂ���ď�����؂�ւ�
		switch(result)
		{
		case GTK_RESPONSE_ACCEPT:	// �ۑ����s
			{
				int16 before_id = window->app->active_window;
				window->app->active_window = (int16)GetWindowID(window, window->app);
				ExecuteSave(window->app);
				window->app->active_window = before_id;
			}
			break;
		case GTK_RESPONSE_REJECT:	// �L�����Z��
			// ���鑀����~�߂�
			return TRUE;
		}
	}

	// �A�v���P�[�V�����̕`��̈�z����ړ�
	for(i=0; i<window->app->window_num; i++)
	{
		if(window->app->draw_window[i] == window)
		{
			for(j=i+1; j<window->app->window_num; j++)
			{
				// �^�u�ɐݒ肳��Ă���y�[�W�����Đݒ�
				g_object_set_data(G_OBJECT(g_object_get_data(G_OBJECT(
					gtk_notebook_get_tab_label(GTK_NOTEBOOK(window->app->widgets->note_book),
						window->app->draw_window[j]->scroll)), "button_widget")), "page", GINT_TO_POINTER(j-1));
			}

			if(i <= window->app->active_window)
			{	// �A�N�e�B�u�ȕ`��̈��K�v�ɉ����ĕύX
				if(window->app->active_window > 0)
				{
					window->app->active_window--;
				}
			}

			(void)memmove(&window->app->draw_window[i],
				&window->app->draw_window[i+1],
				sizeof(window->app->draw_window[0])*(window->app->window_num-i)
			);
		}
	}
	// �`��̈�̐����X�V
	window->app->window_num--;

	// �`��̈悪�[���ɂȂ����ꍇ�Ƀ��j���[�𖳌���
	if(!window->app->window_num)
	{
		for(i = 0; i < window->app->menus.num_disable_if_no_open; i++)
		{
			gtk_widget_set_sensitive(window->app->menus.disable_if_no_open[i], FALSE);
		}

		UpdateWindowTitle(window->app);
	}

	// �`��̈�̍폜���s
	DeleteDrawWindow(&window);

	return FALSE;
}

/*****************************************************
* WindowResizeCallBack�֐�                           *
* �E�B���h�E�̃T�C�Y���ύX���ꂽ���̃R�[���o�b�N�֐� *
* ����                                               *
* widget	: �`��̈�̃E�B�W�F�b�g                 *
* event		: �T�C�Y�ύX�C�x���g�̓��e               *
* window	: �`��̈���Ǘ�����\���̂̃A�h���X     *
*****************************************************/
static void WindowResizeCallBack(
	GtkWidget *widget,
	GdkEvent *event,
	DRAW_WINDOW *window
)
{
	// �`��̈�̃T�C�Y�擾�p
	GtkAllocation allocation;

	// ���݂̕`��̈�X�N���[���̃T�C�Y���擾
	gtk_widget_get_allocation(window->scroll, &allocation);

	// ���݂̃X�N���[���̈ʒu���擾
	window->scroll_x = (int)gtk_range_get_value(GTK_RANGE(gtk_scrolled_window_get_hscrollbar(
		GTK_SCROLLED_WINDOW(window->scroll))));
	window->scroll_y = (int)gtk_range_get_value(GTK_RANGE(gtk_scrolled_window_get_vscrollbar(
		GTK_SCROLLED_WINDOW(window->scroll))));
}

/*********************************************************************
* ScrollCallBack�֐�                                                 *
* �`��̈�̃X�N���[�������삳�ꂽ�Ƃ��ɌĂяo�����R�[���o�b�N�֐� +
* ����                                                               *
* scroll	: �X�N���[���h�E�B���h�E                                 *
* window	: �`��̈�̏��                                         *
*********************************************************************/
static void ScrollCallBack(
	GtkScrolledWindow* scroll,
	DRAW_WINDOW* window
)
{	// �i�r�Q�[�V�����̓��e���X�V
	gtk_widget_queue_draw(window->app->navigation_window.draw_area);

	window->scroll_x = (int)gtk_adjustment_get_value(gtk_scrolled_window_get_hadjustment(
		GTK_SCROLLED_WINDOW(window->scroll)));
	window->scroll_y = (int)gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(window->scroll)));

	UpdateDrawWindowClippingArea(window);
}

/***************************************************************
* ScrollChangeSize�֐�                                         *
* �`��̈�̃T�C�Y���ύX���ꂽ���ɌĂяo�����R�[���o�b�N�֐� *
* ����                                                         *
* scroll		: �X�N���[���h�E�B���h�E                       *
* allocation	: �ݒ肳�ꂽ�T�C�Y                             *
* window		: �`��̈�̏��                               *
***************************************************************/
static void ScrollSizeChange(
	GtkScrolledWindow* scroll,
	GtkAllocation* allocation,
	DRAW_WINDOW* window
)
{
	UpdateDrawWindowClippingArea(window);
}


static gboolean DrawWindowConfigureEvent(GtkWidget* widget,GdkEventConfigure* event_info,DRAW_WINDOW* window)
{
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	if(GetHas3DLayer(window->app) != FALSE)
	{
		LAYER *layer = window->layer;

		if(window->focal_window != NULL)
		{
			return FALSE;
		}

		if((window->flags & DRAW_WINDOW_INITIALIZED) == 0)
		{
			if(ConfigureEvent(widget,event_info,window->first_project) == FALSE)
			{
				return FALSE;
			}
			window->flags |= DRAW_WINDOW_INITIALIZED;
		}

		while(layer != NULL)
		{
			if(layer->modeling_data != NULL)
			{
				MEMORY_STREAM stream ={(uint8*)layer->modeling_data,0,layer->modeling_data_size,1024};
				LoadProjectContextData(layer->layer_data.project,(void*)&stream,(size_t (*)(void*,size_t,size_t,void*))MemRead,
					(int (*)(void*,long,int))MemSeek);
				MEM_FREE_FUNC(layer->modeling_data);
				layer->modeling_data = NULL;
			}
			layer = layer->next;
		}

		if(window->active_layer == NULL)
		{
			return FALSE;
		}
		else if(window->active_layer->layer_type != TYPE_3D_LAYER
				&& (window->flags & DRAW_WINDOW_DISCONNECT_3D) != 0)
		{
			gtk_widget_hide(window->gl_area);
			DisconnectDrawWindowCallbacks(window->gl_area,window);
			gtk_widget_show(window->window);
			SetDrawWindowCallbacks(window->window,window);
			g_signal_handler_disconnect(window->window,window->callbacks.configure);
			window->callbacks.configure = 0;
			gtk_widget_queue_draw(window->window);

			window->flags &= ~(window->flags & DRAW_WINDOW_DISCONNECT_3D);
		}
	}
	else
#endif
	{
		g_signal_handler_disconnect(window->window,window->callbacks.configure);
		window->callbacks.configure = 0;
		window->flags |= DRAW_WINDOW_INITIALIZED;
	}

	return FALSE;
}

/*******************************************************
* SetDrawWindowCallbacks�֐�                           *
* �`��̈�̃R�[���o�b�N�֐��̐ݒ���s��               *
* ����                                                 *
* widget	: �R�[���o�b�N�֐����Z�b�g����E�B�W�F�b�g *
* window	: �`��̈�̏��                           *
*******************************************************/
void SetDrawWindowCallbacks(
	GtkWidget* widget,
	DRAW_WINDOW* window
)
{
	window->callbacks.configure = g_signal_connect(G_OBJECT(widget), "configure-event",
		G_CALLBACK(DrawWindowConfigureEvent), window);
#if GTK_MAJOR_VERSION <= 2
	window->callbacks.display = g_signal_connect(G_OBJECT(widget), "expose_event",
		G_CALLBACK(DisplayDrawWindow), window);
#else
	window->callbacks.display = g_signal_connect(G_OBJECT(widget), "draw",
		G_CALLBACK(DisplayDrawWindow), window);
#endif
	// ���͂̃R�[���o�b�N�֐����Z�b�g
	window->callbacks.mouse_button_press =g_signal_connect(G_OBJECT(widget), "button_press_event",
		G_CALLBACK(ButtonPressEvent), window);
	window->callbacks.mouse_move = g_signal_connect(G_OBJECT(widget), "motion_notify_event",
		G_CALLBACK(MotionNotifyEvent), window);
	window->callbacks.mouse_button_release = g_signal_connect(G_OBJECT(widget), "button_release_event",
		G_CALLBACK(ButtonReleaseEvent), window);
	window->callbacks.mouse_wheel = g_signal_connect(G_OBJECT(widget), "scroll_event",
		G_CALLBACK(MouseWheelEvent), window);
#if GTK_MAJOR_VERSION >= 3
	(void)g_signal_connect(G_OBJECT(widget), "touch-event",
		G_CALLBACK(TouchEvent), window);
#endif
	window->callbacks.enter = g_signal_connect(G_OBJECT(widget), "enter_notify_event",
		G_CALLBACK(EnterNotifyEvent), window);
	window->callbacks.leave = g_signal_connect(G_OBJECT(widget), "leave_notify_event",
		G_CALLBACK(LeaveNotifyEvent), window);

	if(widget == window->gl_area)
	{
		window->flags |= DRAW_WINDOW_DISCONNECT_3D;
	}
}

/***************************************************
* DisconnectDrawWindowCallbacks�֐�                *
* �`��̈�̃R�[���o�b�N�֐����~�߂�               *
* ����                                             *
* widget	: �R�[���o�b�N�֐����~�߂�E�B�W�F�b�g *
* window	: �`��̈�̏��                       *
***************************************************/
void DisconnectDrawWindowCallbacks(
	GtkWidget* widget,
	DRAW_WINDOW* window
)
{
	if(window->callbacks.display != 0)
	{
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.display);
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.mouse_button_press);
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.mouse_move);
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.mouse_button_release);
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.mouse_wheel);
		window->callbacks.display = 0;
	}

	if(window->callbacks.configure != 0)
	{
		g_signal_handler_disconnect(G_OBJECT(widget), window->callbacks.configure);
		window->callbacks.configure = 0;
	}
}

/***************************************************************
* CreateDrawWindow�֐�                                         *
* �`��̈���쐬����                                           *
* ����                                                         *
* width		: �L�����o�X�̕�                                   *
* height	: �L�����o�X�̍���                                 *
* channel	: �L�����o�X�̃`�����l����(RGB:3, RGBA:4)          *
* name		: �L�����o�X�̖��O                                 *
* note_book	: �`��̈�^�u�E�B�W�F�b�g                         *
* window_id	: �`��̈�z�񒆂�ID                               *
* app		: �A�v���P�[�V�����̏����Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                       *
*	�`��̈�̏����Ǘ�����\���̂̃A�h���X                   *
***************************************************************/
DRAW_WINDOW* CreateDrawWindow(
	int32 width,
	int32 height,
	uint8 channel,
	const gchar* name,
	GtkWidget* note_book,
	uint16 window_id,
	APPLICATION* app
)
{
	// �Ԃ�l
	DRAW_WINDOW* ret =
		(DRAW_WINDOW*)MEM_ALLOC_FUNC(sizeof(*ret));
	// �����z�u�p
	GtkWidget *alignment;
	// �^�u�̃��x��
	GtkWidget *tab_label;
	// �\���̈�̃T�C�Y
	size_t disp_size;
	// ��]�����p�̍s��f�[�^
	cairo_matrix_t matrix;
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// �L�����o�X�E�B�W�F�b�g
	GtkWidget *canvas;
	GtkWidget *canvas_box;

	// �V�K�쐬�`��̈�쐬���̃t���O�𗧂Ă�
	app->flags |= APPLICATION_IN_MAKE_NEW_DRAW_AREA;

	// 0������
	(void)memset(ret, 0, sizeof(*ret));

	// �I���W�i���̕��ƍ������L��
	ret->original_width = width;
	ret->original_height = height;

	// �����E����4�̔{����
	width += (4 - (width % 4)) % 4;
	height += (4 - (height % 4)) % 4;

	// �\���T�C�Y���v�Z
	disp_size = (size_t)(2 * sqrt((width/2)*(width/2)+(height/2)*(height/2)) + 1);

	// �l�̃Z�b�g
	ret->channel = channel;
	ret->file_name = (name == NULL) ? NULL : MEM_STRDUP_FUNC(name);
	ret->width = width;
	ret->height = height;
	ret->stride = width * channel;
	ret->pixel_buf_size = ret->stride * height;
	ret->zoom = 100;
	ret->zoom_rate = 1;
	ret->rev_zoom = 1;
	ret->app = app;
	ret->cursor_x = ret->cursor_y = -50000;

	// �w�i�̃s�N�Z�����������m��
	ret->back_ground = (uint8*)MEM_ALLOC_FUNC(sizeof(*ret->back_ground)*ret->pixel_buf_size);
	(void)memset(ret->back_ground, 0xff, sizeof(*ret->back_ground)*ret->pixel_buf_size);

	// �u���V�p�̃o�b�t�@���m��
	ret->brush_buffer = (uint8*)MEM_ALLOC_FUNC(sizeof(*ret->brush_buffer)*ret->pixel_buf_size);

	// �`��p�̃��C���[���쐬
	ret->disp_layer = CreateLayer(0, 0, width, height, channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, ret);
	ret->scaled_mixed = CreateLayer(0, 0, width, height, channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, ret);

	// ���C���[�����̃t���O�𗧂Ă�
	ret->flags = DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	// �`��̈���쐬
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	if(GetHas3DLayer(app) != FALSE)
	{
		ret->first_project = ProjectContextNew(app->modeling,
			width, height, (void**)&ret->gl_area);
		canvas_box = gtk_hbox_new(FALSE, 0);
		canvas = ret->gl_area;
		gtk_box_pack_start(GTK_BOX(canvas_box), ret->gl_area, TRUE, TRUE, 0);
		ret->window = gtk_drawing_area_new();
		gtk_box_pack_start(GTK_BOX(canvas_box), ret->window, TRUE, TRUE, 0);
	}
	else
#endif
	{
		canvas = ret->window = gtk_drawing_area_new();
	}

	// �C�x���g�̎�ނ��Z�b�g
	gtk_widget_set_events(ret->window,
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_SCROLL_MASK
		| GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_RELEASE_MASK
#if GTK_MAJOR_VERSION >= 3
			| GDK_TOUCH_MASK
#endif
			| GDK_POINTER_MOTION_HINT_MASK
	);
	// �`��̈������X�N���[�����쐬
	ret->scroll = gtk_scrolled_window_new(NULL, NULL);
	// �X�N���[�������T�C�Y���ꂽ���̃R�[���o�b�N�֐���ݒ�
	(void)g_signal_connect(G_OBJECT(ret->scroll), "size-allocate",
		G_CALLBACK(ScrollSizeChangeEvent), ret);
	// �T�C�Y��ݒ�
	gtk_widget_set_size_request(ret->window, (gint)disp_size, (gint)disp_size);
	// �����z�u�̐ݒ�
	alignment = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
	// �^�u�̃��x�����쐬
	tab_label = CreateNotebookLabel(ret->file_name, note_book, window_id,
		OnCloseDrawWindow, ret);
	// �X�N���[���\���̏������Z�b�g
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	// �X�N���[���h�E�B���h�E�ɕ`��̈��ǉ�
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ret->scroll), alignment);
	// �^�u�ɕ`��̈��ǉ�
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), ret->scroll, tab_label);
	// �X�N���[���ړ����̃R�[���o�b�N�֐����Z�b�g
	{
		GtkWidget* bar = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(ret->scroll));
		(void)g_signal_connect(G_OBJECT(bar), "value-changed", G_CALLBACK(ScrollCallBack), ret);
		bar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(ret->scroll));
		(void)g_signal_connect(G_OBJECT(bar), "value-changed", G_CALLBACK(ScrollCallBack), ret);
	}
	// �`��̈�T�C�Y�ύX���̃R�[���o�b�N�֐����Z�b�g
	(void)g_signal_connect(G_OBJECT(ret->scroll), "size-allocate",
		G_CALLBACK(ScrollSizeChange), ret);
	g_object_set_data(G_OBJECT(ret->window), "draw-window", ret);
	// �w�i�F��ݒ�
	if((app->flags & APPLICATION_SET_BACK_GROUND_COLOR) != 0)
	{
		GdkColor color = {0};
		color.red = app->preference.canvas_back_ground[0]
			| (app->preference.canvas_back_ground[0] << 8);
		color.green = app->preference.canvas_back_ground[1]
			| (app->preference.canvas_back_ground[1] << 8);
		color.blue = app->preference.canvas_back_ground[2]
			| (app->preference.canvas_back_ground[2] << 8);
		gtk_widget_modify_bg(ret->window, GTK_STATE_NORMAL, &color);
		gtk_widget_modify_bg(ret->scroll, GTK_STATE_NORMAL, &color);
	}

	// �`��p�̃R�[���o�b�N�֐����Z�b�g
	SetDrawWindowCallbacks(canvas, ret);

	// �`��̈��\��
	if(GetHas3DLayer(app) != FALSE)
	{
		gtk_container_add(GTK_CONTAINER(alignment), canvas_box);
		gtk_widget_show_all(ret->scroll);
		gtk_widget_hide(ret->window);
	}
	else
	{
		gtk_container_add(GTK_CONTAINER(alignment), ret->window);
		gtk_widget_show_all(ret->scroll);
	}

#if GTK_MAJOR_VERSION <= 2
	gtk_notebook_set_page(GTK_NOTEBOOK(note_book), window_id);
#else
	gtk_notebook_set_current_page(GTK_NOTEBOOK(note_book), window_id);
#endif

	// ��莞�Ԗ��ɍĕ`�悳���悤�ɃR�[���o�b�N�֐����Z�b�g
	//ret->timer_id = g_idle_add((GSourceFunc)TimerCallBack, ret);
	ret->timer_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000 / FRAME_RATE,
		(GSourceFunc)TimerCallBack, ret, NULL);
#if GTK_MAJOR_VERSION >= 3
	(void)g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000 / FRAME_RATE,
		(GSourceFunc)FirstDrawCheckCallBack, ret, NULL);
#endif
	ret->timer = g_timer_new();
	g_timer_start(ret->timer);

	// ���C���[�����p�̊֐��|�C���^���Z�b�g
	ret->layer_blend_functions = app->layer_blend_functions;
	ret->part_layer_blend_functions = app->part_layer_blend_functions;

	// �����ۑ��̃R�[���o�b�N�֐����Z�b�g
	if(app->preference.auto_save != 0)
	{
		//ret->auto_save_id = g_idle_add((GSourceFunc)AutoSaveCallBack, ret);
		ret->auto_save_id = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT_IDLE, AUTO_SAVE_INTERVAL,
			(GSourceFunc)AutoSaveCallBack, ret, NULL);
		//ret->auto_save_id = g_timeout_add_full(
		//	G_PRIORITY_DEFAULT_IDLE, 1000 / FRAME_RATE, (GSourceFunc)AutoSaveCallBack, ret, NULL);
		ret->auto_save_timer = g_timer_new();
		g_timer_start(ret->auto_save_timer);
	}

#if GTK_MAJOR_VERSION <= 2
	// �g���f�o�C�X��L����
	gtk_widget_set_extension_events(ret->window, GDK_EXTENSION_EVENTS_ALL);
#endif

	// ���C���[�̖��O���Z�b�g
	(void)sprintf(layer_name, "%s 1", app->labels->layer_window.new_layer);

	// �ŏ��̃��C���[���쐬����
	ret->active_layer = ret->layer = CreateLayer(0, 0, width, height,
		channel, TYPE_NORMAL_LAYER, NULL, NULL, layer_name, ret);
	ret->num_layer++;

	// ���C���[�E�B���h�E�ɍŏ��̃��C���[��\��
	LayerViewAddLayer(ret->active_layer, ret->active_layer->prev,
		app->layer_window.view, ret->num_layer);

	// ��Ɨp�̃��C���[���쐬
	ret->work_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// �A�N�e�B�u���C���[�ƍ�Ɨp���C���[���ꎞ�I�ɍ������郌�C���[
	ret->temp_layer = CreateLayer(0, 0, width, height, 5, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// ���C���[��������������
	ret->mixed_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	(void)memcpy(ret->mixed_layer->pixels, ret->back_ground, ret->pixel_buf_size);
	// �������ʂɑ΂��Ċg��E�k����ݒ肷�邽�߂̃p�^�[��
	ret->mixed_pattern = cairo_pattern_create_for_surface(ret->mixed_layer->surface_p);
	cairo_pattern_set_filter(ret->mixed_pattern, CAIRO_FILTER_FAST);

	// �I��̈�̃��C���[���쐬
	ret->selection = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// ��ԏ�ŃG�t�F�N�g�\�����s�����C���[���쐬
	ret->effect = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// �e�N�X�`���p�̃��C���[���쐬
	ret->texture = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// ���̃��C���[�Ń}�X�L���O�A�o�P�c�c�[���ł̃}�X�L���O�p
	ret->mask = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	ret->mask_temp = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	// �u���V�`�掞�̕s�����x�ݒ�p�T�[�t�F�[�X�A�C���[�W
	ret->alpha_surface = cairo_image_surface_create_for_data(ret->mask->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->alpha_cairo = cairo_create(ret->alpha_surface);
	ret->alpha_temp = cairo_image_surface_create_for_data(ret->temp_layer->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->alpha_temp_cairo = cairo_create(ret->alpha_temp);
	ret->gray_mask_temp = cairo_image_surface_create_for_data(ret->mask_temp->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->gray_mask_cairo = cairo_create(ret->gray_mask_temp);

	// �\���p�Ɋg��E�k��������̈ꎟ�L��������
	ret->disp_temp = CreateDispTempLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER, ret);

	// ��]�\���p�̒l���v�Z
	ret->cos_value = 1;
	ret->trans_x = - ret->half_size + ((width / 2) * ret->cos_value + (height / 2) * ret->sin_value);
	ret->trans_y = - ret->half_size - ((width / 2) * ret->sin_value - (height / 2) * ret->cos_value);
	cairo_matrix_init_rotate(&matrix, 0);
	cairo_matrix_translate(&matrix, ret->trans_x, ret->trans_y);
	ret->rotate = cairo_pattern_create_for_surface(ret->disp_layer->surface_p);
	cairo_pattern_set_filter(ret->rotate, CAIRO_FILTER_FAST);
	cairo_pattern_set_matrix(ret->rotate, &matrix);

	// �J�[�\�����W�␳�p�̒l���v�Z
	ret->add_cursor_x = - (ret->half_size - ret->disp_layer->width / 2) + ret->half_size;
	ret->add_cursor_y = - (ret->half_size - ret->disp_layer->height / 2) + ret->half_size;
	ret->rev_add_cursor_x = ret->disp_layer->width/2 + (ret->half_size - ret->disp_layer->width/2);
	ret->rev_add_cursor_y = ret->disp_layer->height/2 + (ret->half_size - ret->disp_layer->height/2);

	// �A�N�e�B�u���C���[��艺�����������摜�̕ۑ��p
	ret->under_active = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	(void)memcpy(ret->under_active->pixels, ret->back_ground, ret->pixel_buf_size);

	// �`��̈�̐V�K�쐬�쐬���̃t���O���~�낷
	app->flags &= ~(APPLICATION_IN_MAKE_NEW_DRAW_AREA);

	// �e�N�X�`����ݒ�
	FillTextureLayer(ret->texture, &app->textures);

	return ret;
}

/***************************************************************
* CreateTempDrawWindow�֐�                                     *
* �ꎞ�I�ȕ`��̈���쐬����                                   *
* ����                                                         *
* width		: �L�����o�X�̕�                                   *
* height	: �L�����o�X�̍���                                 *
* channel	: �L�����o�X�̃`�����l����(RGB:3, RGBA:4)          *
* name		: �L�����o�X�̖��O                                 *
* note_book	: �`��̈�^�u�E�B�W�F�b�g                         *
* window_id	: �`��̈�z�񒆂�ID                               *
* app		: �A�v���P�[�V�����̏����Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                       *
*	�`��̈�̏����Ǘ�����\���̂̃A�h���X                   *
***************************************************************/
DRAW_WINDOW* CreateTempDrawWindow(
	int32 width,
	int32 height,
	uint8 channel,
	const gchar* name,
	GtkWidget* note_book,
	uint16 window_id,
	APPLICATION* app
)
{
	// �Ԃ�l
	DRAW_WINDOW* ret =
		(DRAW_WINDOW*)MEM_ALLOC_FUNC(sizeof(*ret));
	// �\���̈�̃T�C�Y
	size_t disp_size;
	// ��]�����p�̍s��f�[�^
	cairo_matrix_t matrix;

	// �V�K�쐬�`��̈�쐬���̃t���O�𗧂Ă�
	app->flags |= APPLICATION_IN_MAKE_NEW_DRAW_AREA;

	// 0������
	(void)memset(ret, 0, sizeof(*ret));

	// �I���W�i���̕��ƍ������L��
	ret->original_width = width;
	ret->original_height = height;

	// �����E����4�̔{����
	width += (4 - (width % 4)) % 4;
	height += (4 - (height % 4)) % 4;

	// �\���T�C�Y���v�Z
	disp_size = (size_t)(2 * sqrt((width/2)*(width/2)+(height/2)*(height/2)) + 1);

	// �l�̃Z�b�g
	ret->channel = channel;
	ret->file_name = (name == NULL) ? NULL : MEM_STRDUP_FUNC(name);
	ret->width = width;
	ret->height = height;
	ret->stride = width * channel;
	ret->pixel_buf_size = ret->stride * height;
	ret->zoom = 100;
	ret->zoom_rate = 1;
	ret->rev_zoom = 1;
	ret->app = app;

	// �w�i�̃s�N�Z�����������m��
	ret->back_ground = (uint8*)MEM_ALLOC_FUNC(sizeof(*ret->back_ground)*ret->pixel_buf_size);
	(void)memset(ret->back_ground, 0xff, sizeof(*ret->back_ground)*ret->pixel_buf_size);

	// �u���V�p�̃o�b�t�@���m��
	ret->brush_buffer = (uint8*)MEM_ALLOC_FUNC(sizeof(*ret->brush_buffer)*ret->pixel_buf_size);

	// �`��p�̃��C���[���쐬
	ret->disp_layer = CreateLayer(0, 0, width, height, channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, ret);
	ret->scaled_mixed = CreateLayer(0, 0, width, height, channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, ret);

	// ���C���[�����̃t���O�𗧂Ă�
	ret->flags = DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

#if 0 // GTK_MAJOR_VERSION <= 2
	// �g���f�o�C�X��L����
	gtk_widget_set_extension_events(ret->window, GDK_EXTENSION_EVENTS_ALL);
#endif

	// ��Ɨp�̃��C���[���쐬
	ret->work_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// �A�N�e�B�u���C���[�ƍ�Ɨp���C���[���ꎞ�I�ɍ������郌�C���[
	ret->temp_layer = CreateLayer(0, 0, width, height, 5, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// ���C���[��������������
	ret->mixed_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	(void)memcpy(ret->mixed_layer->pixels, ret->back_ground, ret->pixel_buf_size);
	// �������ʂɑ΂��Ċg��E�k����ݒ肷�邽�߂̃p�^�[��
	ret->mixed_pattern = cairo_pattern_create_for_surface(ret->mixed_layer->surface_p);
	cairo_pattern_set_filter(ret->mixed_pattern, CAIRO_FILTER_FAST);

	// �I��̈�̃��C���[���쐬
	ret->selection = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// ��ԏ�ŃG�t�F�N�g�\�����s�����C���[���쐬
	ret->effect = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// �e�N�X�`���p�̃��C���[���쐬
	ret->texture = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);

	// ���̃��C���[�Ń}�X�L���O�A�o�P�c�c�[���ł̃}�X�L���O�p
	ret->mask = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	ret->mask_temp = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	// �u���V�`�掞�̕s�����x�ݒ�p�T�[�t�F�[�X�A�C���[�W
	ret->alpha_surface = cairo_image_surface_create_for_data(ret->mask->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->alpha_cairo = cairo_create(ret->alpha_surface);
	ret->alpha_temp = cairo_image_surface_create_for_data(ret->temp_layer->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->alpha_temp_cairo = cairo_create(ret->alpha_temp);
	ret->gray_mask_temp = cairo_image_surface_create_for_data(ret->mask_temp->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	ret->gray_mask_cairo = cairo_create(ret->gray_mask_temp);

	// �\���p�Ɋg��E�k��������̈ꎟ�L��������
	ret->disp_temp = CreateDispTempLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER, ret);

	// ��]�\���p�̒l���v�Z
	ret->cos_value = 1;
	ret->trans_x = - ret->half_size + ((width / 2) * ret->cos_value + (height / 2) * ret->sin_value);
	ret->trans_y = - ret->half_size - ((width / 2) * ret->sin_value - (height / 2) * ret->cos_value);
	cairo_matrix_init_rotate(&matrix, 0);
	cairo_matrix_translate(&matrix, ret->trans_x, ret->trans_y);
	ret->rotate = cairo_pattern_create_for_surface(ret->disp_layer->surface_p);
	cairo_pattern_set_filter(ret->rotate, CAIRO_FILTER_FAST);
	cairo_pattern_set_matrix(ret->rotate, &matrix);

	// �J�[�\�����W�␳�p�̒l���v�Z
	ret->add_cursor_x = - (ret->half_size - ret->disp_layer->width / 2) + ret->half_size;
	ret->add_cursor_y = - (ret->half_size - ret->disp_layer->height / 2) + ret->half_size;
	ret->rev_add_cursor_x = ret->disp_layer->width/2 + (ret->half_size - ret->disp_layer->width/2);
	ret->rev_add_cursor_y = ret->disp_layer->height/2 + (ret->half_size - ret->disp_layer->height/2);

	// ���C���[�����p�̊֐��|�C���^���Z�b�g
	ret->layer_blend_functions = app->layer_blend_functions;
	ret->part_layer_blend_functions = app->part_layer_blend_functions;

	// �A�N�e�B�u���C���[��艺�����������摜�̕ۑ��p
	ret->under_active = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, ret);
	(void)memcpy(ret->under_active->pixels, ret->back_ground, ret->pixel_buf_size);

	return ret;
}

/*********************************
* Change2FocalMode�֐�           *
* �Ǐ��L�����o�X���[�h�Ɉڍs���� *
* ����                           *
* parent_window	: �e�L�����o�X   *
*********************************/
void Change2FocalMode(DRAW_WINDOW* parent_window)
{
	// �A�v���P�[�V�����S�̂��Ǘ�����f�[�^
	APPLICATION *app = parent_window->app;
	// �Ǐ��L�����o�X
	DRAW_WINDOW *focal_window;
	// ���C���[�o�^�p
	LAYER *prev_layer = NULL;
	LAYER *src_layer;
	LAYER *target;
	// �L�����o�X�̃X�N���[���o�[�E�B�W�F�b�g
	GtkWidget *scroll_bar;
	// �Ǐ��L�����o�X�̃T�C�Y
	int focal_width, focal_height, focal_stride;
	// �R�s�[�J�n���W
	int start_x, start_y;
	// for���p�̃J�E���^
	int y;

	// �I��͈͂�������ΏI��
	if((parent_window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{
		return;
	}

	// �������̃R�[���o�b�N�֐����Ă΂�Ȃ��悤�ɐ�Ɏ~�߂�
	DisconnectDrawWindowCallbacks(parent_window->window, parent_window);

	// ���݂̃X�N���[���o�[�̈ʒu���L������
	scroll_bar = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(parent_window->scroll));
	parent_window->focal_x = (int16)gtk_range_get_value(GTK_RANGE(scroll_bar));
	scroll_bar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(parent_window->scroll));
	parent_window->focal_y = (int16)gtk_range_get_value(GTK_RANGE(scroll_bar));

	// �I��͈͕����Ƀt�H�[�J�X����
	start_x = parent_window->selection_area.min_x;
	start_y = parent_window->selection_area.min_y;
	focal_width = parent_window->selection_area.max_x - parent_window->selection_area.min_x;
	focal_height = parent_window->selection_area.max_y - parent_window->selection_area.min_y;
	focal_stride = focal_width * parent_window->channel;

	// �L�����o�X����
	focal_window = parent_window->focal_window = CreateTempDrawWindow(focal_width, focal_height,
		parent_window->channel, parent_window->file_name, NULL, 0, app);
	focal_window->focal_window = parent_window;
	focal_window->scroll = parent_window->scroll;

	if(GetHas3DLayer(parent_window->app) != FALSE)
	{
		focal_window->first_project = parent_window->first_project;
	}
	focal_window->flags |= DRAW_WINDOW_IS_FOCAL_WINDOW;

	// �e�L�����o�X�̃��C���[�����R�s�[����
	for(src_layer = parent_window->layer; src_layer != NULL; src_layer = src_layer->next)
	{
		prev_layer = CreateLayer(0, 0, focal_window->width, focal_window->height, focal_window->channel,
			(eLAYER_TYPE)src_layer->layer_type, prev_layer, NULL, src_layer->name, focal_window);
		prev_layer->layer_mode = src_layer->layer_mode;
		prev_layer->flags = src_layer->flags;
		prev_layer->alpha = src_layer->alpha;

		for(y=0; y<focal_height; y++)
		{
			(void)memcpy(&prev_layer->pixels[y*prev_layer->stride],
				&src_layer->pixels[(start_y+y)*src_layer->stride+start_x*src_layer->channel], focal_stride);
			if(src_layer->layer_type == TYPE_LAYER_SET)
			{
				LAYER *child = prev_layer->prev;
				target = src_layer->prev;
				while(target->layer_set == src_layer)
				{
					child->layer_set = prev_layer;
					child = child->prev;
					target = target->prev;
				}

				if(parent_window->active_layer_set == src_layer)
				{
					focal_window->active_layer_set = prev_layer;
				}
			}
			if(src_layer == parent_window->active_layer)
			{
				focal_window->active_layer = prev_layer;
			}
		}
		focal_window->num_layer++;
	}
	// ��ԉ��̃��C���[��V�����L�����o�X�ɐݒ�
	while(prev_layer->prev != NULL)
	{
		prev_layer = prev_layer->prev;
	}
	focal_window->layer = prev_layer;

	// ���C���[�r���[�����Z�b�g
	ClearLayerView(&app->layer_window);
	LayerViewSetDrawWindow(&app->layer_window, focal_window);

	// �L�����o�X�ւ̃R�[���o�b�N�֐���ύX
	focal_window->window = parent_window->window;
	SetDrawWindowCallbacks(focal_window->window, focal_window);

	// �E�B�W�F�b�g�̃T�C�Y��ύX
	focal_window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	DrawWindowChangeZoom(focal_window, focal_window->zoom);

	// �i�r�Q�[�V�����̕\����؂�ւ�
	ChangeNavigationDrawWindow(&app->navigation_window, focal_window);
	FillTextureLayer(focal_window->texture, &app->textures);
}

/*************************************************************************
* DrawWindowConfigureEventOnReturnFromFocalMode�֐�                      *
* ���[�y���[�h����߂�����A�E�B�W�F�b�g���\�����ꂽ�ۂ̃R�[���o�b�N�֐� * 
* ����                                                                   *
* widget		: �\�����ꂽ�E�B�W�F�b�g                                 *
* event_info	: �C�x���g�̏ڍ׏��                                     *
* window		: �L�����o�X�̏��                                       *
* �߂�l                                                                 *
*	���FALSE                                                            *
*************************************************************************/
static gboolean DrawWindowConfigureEventOnReturnFromFocalMode(GtkWidget* widget,GdkEventConfigure* event_info,DRAW_WINDOW* window)
{
	GtkWidget *scroll_bar;
	g_signal_handler_disconnect(window->window,window->callbacks.configure);
	window->callbacks.configure = 0;
	scroll_bar = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(window->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar),window->focal_x);
	scroll_bar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(window->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar),window->focal_y);

	return FALSE;
}

/*********************************
* ReturnFromFocalMode�֐�        *
* �Ǐ��L�����o�X���[�h����߂�   *
* ����                           *
* parent_window	: �e�L�����o�X   *
*********************************/
void ReturnFromFocalMode(DRAW_WINDOW* parent_window)
{
	// �A�v���P�[�V�����S�̂��Ǘ�����f�[�^
	APPLICATION *app = parent_window->app;
	// �Ǐ��L�����o�X
	DRAW_WINDOW *focal_window = parent_window->focal_window;
	// ���C���[���ړ��p
	LAYER *prev_layer = NULL;
	LAYER *src_layer;
	LAYER *dst_layer;
	LAYER *target;
	LAYER *check_layer;
	// �Ǐ��L�����o�X�̃T�C�Y
	int focal_width, focal_height, focal_stride;
	// �R�s�[�J�n���W
	int start_x, start_y;
	// for���p�̃J�E���^
	int y;

	// �Ǐ��L�����o�X��������ΏI��
	if(focal_window == NULL)
	{
		return;
	}

	// �������ɃR�[���o�b�N�֐����Ă΂�Ȃ��悤�ɐ�Ɏ~�߂�
	DisconnectDrawWindowCallbacks(focal_window->window, focal_window);

	// �I��͈͕����Ƀs�N�Z���f�[�^��߂�
	start_x = parent_window->selection_area.min_x;
	start_y = parent_window->selection_area.min_y;
	focal_width = parent_window->selection_area.max_x - parent_window->selection_area.min_x;
	focal_height = parent_window->selection_area.max_y - parent_window->selection_area.min_y;
	focal_stride = focal_width * parent_window->channel;
	src_layer = focal_window->layer;
	dst_layer = parent_window->layer;
	parent_window->num_layer = 0;
	while(src_layer != NULL && dst_layer != NULL)
	{
		dst_layer->layer_mode = src_layer->layer_mode;
		dst_layer->flags = src_layer->flags;
		dst_layer->alpha = src_layer->alpha;
		switch(src_layer->layer_type)
		{
		case TYPE_NORMAL_LAYER:
			for(y=0; y<focal_height; y++)
			{
				(void)memcpy(&dst_layer->pixels[(start_y+y)*dst_layer->stride + start_x*dst_layer->channel],
					&src_layer->pixels[y*src_layer->stride], focal_stride);
			}
			break;
		case TYPE_LAYER_SET:
			target = dst_layer->prev;
			check_layer = src_layer->prev;
			while(check_layer != NULL && check_layer->layer_set == src_layer)
			{
				target->layer_set = dst_layer;
				target = target->prev;
				check_layer = check_layer->prev;
			}
			if(focal_window->active_layer_set == src_layer)
			{
				parent_window->active_layer_set = dst_layer;
			}
			break;
		case TYPE_ADJUSTMENT_LAYER:
			src_layer->layer_data.adjustment_layer_p->update(src_layer->layer_data.adjustment_layer_p,
				src_layer->prev, parent_window->mixed_layer, 0, 0, src_layer->width, src_layer->height);
			break;
		}
		dst_layer = dst_layer->next;
		src_layer = src_layer->next;
		parent_window->num_layer++;

		if(src_layer == focal_window->active_layer)
		{
			parent_window->active_layer = dst_layer;
		}
	}

	// ���C���[�r���[�����Z�b�g
	ClearLayerView(&app->layer_window);
	LayerViewSetDrawWindow(&app->layer_window, parent_window);

	// �R�[���o�b�N�֐���ݒ肵����
	SetDrawWindowCallbacks(parent_window->window, parent_window);

	// �E�B�W�F�b�g�̃T�C�Y��ύX
	parent_window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	DrawWindowChangeZoom(parent_window, parent_window->zoom);

	DeleteDrawWindow(&parent_window->focal_window);

	// �i�r�Q�[�V�����̕\����؂�ւ�
	ChangeNavigationDrawWindow(&app->navigation_window, parent_window);
	FillTextureLayer(parent_window->texture, &app->textures);

	// �E�B�W�F�b�g��\�����Ă���g�嗦��\���ʒu�𒲐�
	gtk_widget_show_all(parent_window->window);
	DrawWindowChangeZoom(parent_window, parent_window->zoom);
	parent_window->callbacks.configure = g_signal_connect(
		G_OBJECT(parent_window->window), "configure-event", G_CALLBACK(DrawWindowConfigureEventOnReturnFromFocalMode), parent_window);
}

/***************************************
* DeleteDrawWindow�֐�                 *
* �`��̈�̏����폜                 *
* ����                                 *
* window	: �`��̈�̏��̃A�h���X *
***************************************/
void DeleteDrawWindow(DRAW_WINDOW** window)
{
	// �폜���郌�C���[�Ǝ��ɍ폜���郌�C���[
	LAYER* delete_layer, *next_delete;
	// for���p�̃J�E���^
	int i;

	// ���ԂŌĂ΂��R�[���o�b�N�֐����~
	if((*window)->timer_id != 0)
	{
		(void)g_source_remove((*window)->timer_id);
	}
	if((*window)->auto_save_id != 0)
	{
		(void)g_source_remove((*window)->auto_save_id);
	}
	if((*window)->timer != NULL)
	{
		g_timer_destroy((*window)->timer);
	}

	// �u���V�`��p�̃T�[�t�F�[�X�A�C���[�W���폜
	cairo_destroy((*window)->alpha_cairo);
	cairo_surface_destroy((*window)->alpha_surface);
	cairo_destroy((*window)->alpha_temp_cairo);
	cairo_surface_destroy((*window)->alpha_temp);
	cairo_destroy((*window)->gray_mask_cairo);
	cairo_surface_destroy((*window)->gray_mask_temp);

	// �t�@�C�����A�t�@�C���p�X���폜
	MEM_FREE_FUNC((*window)->file_name);
	MEM_FREE_FUNC((*window)->file_path);

	// �w�i�̃s�N�Z���f�[�^���J��
	MEM_FREE_FUNC((*window)->back_ground);

	// ���C���[��S�č폜
	delete_layer = (*window)->layer;
	while(delete_layer != NULL)
	{
		next_delete = delete_layer->next;
		DeleteLayer(&delete_layer);
		delete_layer = next_delete;
		(*window)->layer = next_delete;
	}
	DeleteLayer(&(*window)->mixed_layer);
	DeleteLayer(&(*window)->temp_layer);
	DeleteLayer(&(*window)->selection);
	DeleteLayer(&(*window)->under_active);
	DeleteLayer(&(*window)->mask);
	DeleteLayer(&(*window)->mask_temp);
	DeleteLayer(&(*window)->texture);
	DeleteLayer(&(*window)->work_layer);

#ifdef OLD_SELECTION_AREA
	// �I��͈͂̏����J��
	for(i=0; i<(*window)->selection_area.num_area; i++)
	{
		MEM_FREE_FUNC((*window)->selection_area.area_data[i].points);
	}
	MEM_FREE_FUNC((*window)->selection_area.area_data);
#else
	if((*window)->selection_area.pixels != NULL)
	{
		MEM_FREE_FUNC((*window)->selection_area.pixels);
		cairo_surface_destroy((*window)->selection_area.surface_p);
	}
#endif

	// �����f�[�^�̏����J��
	for(i=0; i<HISTORY_BUFFER_SIZE; i++)
	{
		MEM_FREE_FUNC((*window)->history.history[i].data);
	}

	MEM_FREE_FUNC(*window);
	*window = NULL;
}

/*****************************************************
* SwapDrawWindowFromMemoryStream�֐�                 *
* �������[��̕`��̈�f�[�^�Ɠ���ւ���             *
* ����                                               *
* window	: �`��̈�̏��                         *
* stream	: �������[��̕`��̈�̃f�[�^�X�g���[�� *
*****************************************************/
void SwapDrawWindowFromMemoryStream(DRAW_WINDOW* window, MEMORY_STREAM_PTR stream)
{
	// �폜���郌�C���[�Ǝ��ɍ폜���郌�C���[
	LAYER* delete_layer, *next_delete;
	// �O�̃A�N�e�B�u���C���[�̖��O���L��
	char active_name[MAX_LAYER_NAME_LENGTH];
#ifdef OLD_SELECTION_AREA
	// for���p�̃J�E���^
	int i;
#endif

	// �A�N�e�B�u�ȃ��C���[�̖��O���L��
	(void)strcpy(active_name, window->active_layer->name);

	// �w�i�̃s�N�Z���f�[�^���J��
	MEM_FREE_FUNC(window->back_ground);

	// ���C���[��S�č폜
	delete_layer = window->layer;
	while(delete_layer != NULL)
	{
		next_delete = delete_layer->next;
		DeleteLayer(&delete_layer);
		delete_layer = next_delete;
	}
	DeleteLayer(&window->texture);
	DeleteLayer(&window->mixed_layer);
	DeleteLayer(&window->temp_layer);
	DeleteLayer(&window->selection);
	DeleteLayer(&window->under_active);
	DeleteLayer(&window->mask);
	DeleteLayer(&window->mask_temp);
	DeleteLayer(&window->work_layer);

	// �u���V����̃T�[�t�F�[�X�A�C���[�W����x�폜
	cairo_destroy(window->alpha_cairo);
	cairo_surface_destroy(window->alpha_surface);
	cairo_destroy(window->alpha_temp_cairo);
	cairo_surface_destroy(window->alpha_temp);
	cairo_destroy(window->gray_mask_cairo);
	cairo_surface_destroy(window->gray_mask_temp);

	// ���C���[�r���[�̃E�B�W�F�b�g��S�č폜
		// ���C���[�r���[�̃��X�g
	{
		GList* view_list = gtk_container_get_children(
			GTK_CONTAINER(window->app->layer_window.view));
		while(view_list != NULL)
		{
			gtk_widget_destroy(GTK_WIDGET(view_list->data));
			view_list = view_list->next;
		}

		g_list_free(view_list);
	}

	// �I��͈͂̏����J��
#ifdef OLD_SELECTION_AREA
	for(i=0; i<window->selection_area.num_area; i++)
	{
		MEM_FREE_FUNC(window->selection_area.area_data[i].points);
	}
	MEM_FREE_FUNC(window->selection_area.area_data);
#else
	if(window->selection_area.pixels != NULL)
	{
		MEM_FREE_FUNC(window->selection_area.pixels);
		cairo_surface_destroy(window->selection_area.surface_p);
	}
#endif

	// �O�̏�Ԃ���f�[�^�𕜌�
	ReadOriginalFormatMemoryStream(window, stream);

	// �\���p�̃o�b�t�@���X�V
	DrawWindowChangeZoom(window, window->zoom);

	// �������ʂɑ΂��Ċg��E�k����ݒ肷�邽�߂̃p�^�[���쐬������
	window->mixed_pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);
	cairo_pattern_set_filter(window->mixed_pattern, CAIRO_FILTER_FAST);

	// �i�r�Q�[�V�����̕\���ݒ�
	ChangeNavigationDrawWindow(&window->app->navigation_window, window);
	// �e�N�X�`���p�̃��C���[���X�V
	FillTextureLayer(window->texture, &window->app->textures);

	// �A�N�e�B�u���C���[���Đݒ�
	window->active_layer = SearchLayer(window->layer, active_name);
	// �A�N�e�B�u���C���[�̕\�����X�V
	ChangeActiveLayer(window, window->active_layer);
}

/***********************************************************
* GetWindowID�֐�                                          *
* �`��̈��ID���擾����                                   *
* ����                                                     *
* window	: �`��̈�̏��                               *
* app		: 	�A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                   *
*	�`��̈��ID (�s���ȕ`��̈�Ȃ��-1)                  *
***********************************************************/
int GetWindowID(DRAW_WINDOW* window, APPLICATION* app)
{
	// for���p�̃J�E���^
	int i;

	for(i=0; i<app->window_num; i++)
	{
		if(window == app->draw_window[i])
		{
			return i;
		}
	}

	return -1;
}

/*********************************************************
* GetWindowTitle�֐�                                     *
* �`��̈�̏��Ɋ�Â��E�C���h�E�^�C�g�����擾����     *
* ����                                                   *
* window	: �`��̈�̏��                             *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	�E�C���h�E�^�C�g��                                   *
*********************************************************/
gchar *GetWindowTitle(DRAW_WINDOW* window, APPLICATION* app)
{
	gchar *profile_desc = NULL;
	gchar *title;
	cmsHPROFILE h_profile = window->input_icc ? window->input_icc : app->input_icc;

	wchar_t *buf = NULL;
	guint32 buf_size;

	// TODO : lang�t�@�C����LanguageCode��CountryCode�����Ƃ��Ăق���
	if((buf_size = cmsGetProfileInfo(h_profile, cmsInfoDescription, "ja", "JP", NULL, 0)))
	{
		gssize count;

		buf = (wchar_t *)g_malloc(buf_size);

		cmsGetProfileInfo(h_profile, cmsInfoDescription, "ja", "JP", buf, buf_size);

		profile_desc = g_convert((const gchar*)buf, buf_size, "UTF-8", "UTF-16LE", NULL, &count, NULL);

		g_free(buf);
	}
	else
	{
		profile_desc = g_strdup("*unknown profile name*");
	}

	title = g_strdup_printf("%s (%s%s)", window->file_name ? window->file_name : app->labels->make_new.name, !(window->input_icc) ? "untagged : " : "", profile_desc);

	g_free(profile_desc);

	return title;
}

/***************************************
* ResizeDispTemp�֐�                   *
* �\���p�̈ꎞ�ۑ��̃o�b�t�@��ύX     *
* ����                                 *
* window		: �`��̈�̏��       *
* new_width		: �`��̈�̐V������   *
* new_height	: �`��̈�̐V�������� *
***************************************/
void ResizeDispTemp(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
)
{
	// CAIRO�ɐݒ肷��t�H�[�}�b�g���
	cairo_format_t format = (window->channel == 4) ?
		CAIRO_FORMAT_ARGB32 : (window->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;
	// ��]�����p�̍s��f�[�^
	cairo_matrix_t matrix;

	window->disp_size = (int)(2 * sqrt((new_width/2)*(new_width/2)+(new_height/2)*(new_height/2)) + 1);
	window->disp_stride = window->disp_size * 4;
	window->half_size = window->disp_size * 0.5;
	window->trans_x = - window->half_size + ((new_width / 2) * window->cos_value + (new_height / 2) * window->sin_value);
	window->trans_y = - window->half_size - ((new_width / 2) * window->sin_value - (new_height / 2) * window->cos_value);

	window->add_cursor_x = - (window->half_size - window->disp_layer->width / 2) + window->half_size;
	window->add_cursor_y = - (window->half_size - window->disp_layer->height / 2) + window->half_size;
	window->rev_add_cursor_x = window->disp_layer->width/2 + (window->half_size - window->disp_layer->width/2);
	window->rev_add_cursor_y = window->disp_layer->height/2 + (window->half_size - window->disp_layer->height/2);

	cairo_surface_destroy(window->disp_temp->surface_p);
	cairo_destroy(window->disp_temp->cairo_p);

	window->disp_temp->width = new_width;
	window->disp_temp->height = new_height;
	window->disp_temp->stride = new_width * window->disp_temp->channel;

	window->disp_temp->pixels = (uint8*)MEM_REALLOC_FUNC(window->disp_temp->pixels,
		window->disp_stride * window->disp_size);
	window->disp_temp->surface_p = cairo_image_surface_create_for_data(
		window->disp_temp->pixels, format, new_width, new_height, window->disp_temp->stride);
	window->disp_temp->cairo_p = cairo_create(window->disp_temp->surface_p);

	cairo_matrix_init_rotate(&matrix, window->angle);
	cairo_matrix_translate(&matrix, window->trans_x, window->trans_y);

	if(window->rotate != NULL)
	{
		cairo_pattern_destroy(window->rotate);
	}
	window->rotate = cairo_pattern_create_for_surface(window->disp_layer->surface_p);
	cairo_pattern_set_filter(window->rotate, CAIRO_FILTER_FAST);
	cairo_pattern_set_matrix(window->rotate, &matrix);

	UpdateDrawWindowClippingArea(window);
}

/*********************************
* DrawWindowChangeZoom�֐�       *
* �`��̈�̊g��k������ύX���� *
* ����                           *
* window	: �`��̈�̏��     *
* zoom		: �V�����g��k����   *
*********************************/
void DrawWindowChangeZoom(
	DRAW_WINDOW* window,
	int16 zoom
)
{
	// �g�嗦�ݒ�p�̍s��f�[�^
	cairo_matrix_t matrix;

	// �V�����g�嗦�ōs��f�[�^��������
	cairo_matrix_init_scale(&matrix, 1/(zoom*0.01), 1/(zoom*0.01));
	// �����摜�f�[�^�̃p�^�[���ɐݒ�
	cairo_pattern_set_matrix(window->mixed_pattern, &matrix);

	// �V�����g��k�������Z�b�g
	window->zoom = zoom;
	window->zoom_rate = zoom * 0.01;
	window->rev_zoom = 1 / window->zoom_rate;

	// �\���p�A�G�t�F�N�g�p�A�u���V�\���p�̃��C���[�̃o�b�t�@���Ċm��
	ResizeLayerBuffer(window->disp_layer, (int32)(window->width*zoom*0.01), (int32)(window->height*zoom*0.01));
	ResizeLayerBuffer(window->scaled_mixed, (int32)(window->width*zoom*0.01), (int32)(window->height*zoom*0.01));
	ResizeLayerBuffer(window->effect, (int32)(window->width*zoom*0.01), (int32)(window->height*zoom*0.01));
	ResizeDispTemp(window, (int32)(window->width*zoom*0.01), (int32)(window->height*zoom*0.01));

	// �E�B�W�F�b�g�̃T�C�Y���C��
	gtk_widget_set_size_request(window->window, window->disp_size, window->disp_size);
	gtk_widget_show(window->window);

	// �i�r�Q�[�V�����̕\�����X�V
	if(window->app->navigation_window.draw_area != NULL)
	{
		gtk_widget_queue_draw(window->app->navigation_window.draw_area);
	}

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

#ifndef OLD_SELECTION_AREA
	if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
	{
		(void)UpdateSelectionArea(&window->selection_area, window->selection, window->disp_temp);
	}
#endif
}

/*****************************************
* FlipDrawWindowHorizontally�֐�         *
* �`��̈�𐅕����]����                 *
* ����                                   *
* window	: �������]����`��̈�̏�� *
*****************************************/
void FlipDrawWindowHorizontally(DRAW_WINDOW* window)
{
	// �������]���郌�C���[
	LAYER* target = window->layer;
	int i;

	while(target != NULL)
	{
		// ���C���[�̃s�N�Z���f�[�^�𐅕����]
		FlipLayerHorizontally(target, window->temp_layer);

		// �x�N�g�����C���[�Ȃ��
		if(target->layer_type == TYPE_VECTOR_LAYER)
		{	// �x�N�g���f�[�^�̍��W�𐅕����]
			VECTOR_LINE* line = ((VECTOR_LINE*)(target->layer_data.vector_layer_p->base))->base_data.next;

			while(line != NULL)
			{
				if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					for(i=0; i<line->num_points; i++)
					{
						line->points[i].x = window->width - line->points[i].x;
					}
				}

				line = line->base_data.next;
			}

			// �x�N�g�������X�^���C�Y
			target->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ALL;
			RasterizeVectorLayer(window, target, target->layer_data.vector_layer_p);
		}

		target = target->next;
	}

	// �`����e���X�V����
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

/*****************************************
* FlipDrawWindowVertically�֐�           *
* �`��̈�𐂒����]����                 *
* ����                                   *
* window	: �������]����`��̈�̏�� *
*****************************************/
void FlipDrawWindowVertically(DRAW_WINDOW* window)
{
	// �������]���郌�C���[
	LAYER* target = window->layer;
	int i;

	// �S�Ẵ��C���[�𐂒����]����
	while(target != NULL)
	{	// ���C���[�̃s�N�Z���f�[�^�𐂒����]
		FlipLayerVertically(target, window->temp_layer);

		// �x�N�g�����C���[�Ȃ��
		if(target->layer_type == TYPE_VECTOR_LAYER)
		{	// �x�N�g���̍��W�f�[�^�𐂒����]����
			VECTOR_LINE* line = ((VECTOR_LINE*)(target->layer_data.vector_layer_p->base))->base_data.next;

			while(line != NULL)
			{
				for(i=0; i<line->num_points; i++)
				{
					line->points[i].y = window->height - line->points[i].y;
				}

				line = line->base_data.next;
			}

			// �x�N�g�������X�^���C�Y
			target->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ALL;
			RasterizeVectorLayer(window, target, target->layer_data.vector_layer_p);
		}

		target = target->next;
	}

	// �`����e���X�V����
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

/***************************************
* LayerAlpha2SelectionArea�֐�         *
* ���C���[�̕s����������I��͈͂ɂ��� *
* ����                                 *
* window	: �`��̈�̏��           *
***************************************/
void LayerAlpha2SelectionArea(DRAW_WINDOW* window)
{
	int i, j;	// for���p�̃J�E���^

	// ���C���[�̃s�N�Z���f�[�^���̃��l��I��͈͂ɂ���
	for(i=0; i<window->active_layer->height; i++)
	{
		for(j=0; j<window->active_layer->width; j++)
		{
			window->selection->pixels[(window->active_layer->y+i)*window->width+j] =
				window->active_layer->pixels[i*window->active_layer->stride+j*4+3];
		}
	}

	// �I��͈͂��X�V����
	if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == 0)
	{	// �I��͈͖�
		window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
	}
	else
	{	// �I��͈͗L
		window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
	}
}

/*****************************************
* LayerAlphaAddSelectionArea�֐�         *
* ���C���[�̕s����������I��͈͂ɉ����� *
* ����                                   *
* window	: �`��̈�̏��             *
*****************************************/
void LayerAlphaAddSelectionArea(DRAW_WINDOW* window)
{
	int i, j;	// for���p�̃J�E���^

	// ���C���[�̃s�N�Z���f�[�^���̃��l��I��͈͂ɂ���
	for(i=0; i<window->active_layer->height; i++)
	{
		for(j=0; j<window->active_layer->width; j++)
		{
			if(window->selection->pixels[(window->active_layer->y+i)*window->width+j]
				< window->active_layer->pixels[i*window->active_layer->stride+j*4+3])
			{
				window->selection->pixels[(window->active_layer->y+i)*window->width+j] =
					window->active_layer->pixels[i*window->active_layer->stride+j*4+3];
			}
		}
	}

	// �I��͈͂��X�V����
	if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == 0)
	{	// �I��͈͖�
		window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
	}
	else
	{	// �I��͈͗L
		window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
	}
}

/*****************************
* MergeAllLayer�֐�          *
* �S�Ẵ��C���[����������   *
* ����                       *
* window	: �`��̈�̏�� *
*****************************/
void MergeAllLayer(DRAW_WINDOW* window)
{
	// �S�Ẵ��C���[�������������C���[
	LAYER* merge = MixLayerForSave(window);
	// �폜���郌�C���[
	LAYER* delete_layer = window->layer;
	// ���ɍ폜���郌�C���[
	LAYER* next_delete;
	// ��ԉ��̃��C���[�ɓK�p���閼�O
	char* layer_name = MEM_STRDUP_FUNC(window->layer->name);

	ChangeActiveLayer(window, window->layer);

	// ���̃��C���[���͑S�č폜
	while(delete_layer != NULL)
	{
		next_delete = delete_layer->next;
		DeleteLayer(&delete_layer);
		window->layer = next_delete;
		delete_layer = next_delete;
	}

	// ��ԉ��̃��C���[��V���ɍ쐬
	window->layer = CreateLayer(0, 0, window->width, window->height,
		window->channel, TYPE_NORMAL_LAYER, NULL, NULL, layer_name, window);
	window->num_layer = 1;

	// �s�N�Z���f�[�^���R�s�[
	(void)memcpy(window->layer->pixels, merge->pixels, window->pixel_buf_size);

	// �A�N�e�B�u���C���[��ύX���ĕ\��
	window->active_layer = window->layer;
	LayerViewAddLayer(window->layer, window->layer, window->app->layer_window.view, 1);
	LayerViewSetActiveLayer(window->layer, window->app->layer_window.view);

	MEM_FREE_FUNC(layer_name);
	DeleteLayer(&merge);
}

/**********************************************
* CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY�\���� *
* �𑜓x�ύX�̗����f�[�^                      *
**********************************************/
typedef struct _CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY
{
	int32 new_width, new_height;	// �ύX��̉𑜓x
	int32 x, y;						// ���C���[�̍��W
	size_t before_data_size;		// �ύX�O�̃f�[�^�T�C�Y
	uint8 *before_data;				// �ύX�O�̃f�[�^
} CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY;

/*************************************
* ChangeDrawWindowResolutionUndo�֐� *
* �𑜓x�ύX�����ɖ߂�               *
* ����                               *
* window	: �`��̈�̏��         *
* p			: �����f�[�^             *
*************************************/
static void ChangeDrawWindowResolutionUndo(DRAW_WINDOW* window, void* p)
{
	// �ύX�O�̏�ԃf�[�^�̃o�C�g��
	size_t before_data_size;
	// �����f�[�^���o�C�g�P�ʂɃL���X�g
	uint8 *byte_data = (uint8*)p;
	// �O�̏�Ԃ�ǂݍ��ނ��߂̃X�g���[��
	MEMORY_STREAM stream;

	// �ύX�O�̏�ԃf�[�^�̃o�C�g����ǂݍ���
	(void)memcpy(
		&before_data_size,
		&byte_data[offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data_size)],
		sizeof(before_data_size)
	);

	// �X�g���[���̐ݒ�
	stream.data_point = 0;
	stream.block_size = 1;
	stream.buff_ptr = &byte_data[offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data)];
	stream.data_size = before_data_size;

	// �ύX�O�̏�Ԃ̕`��̈�Ɠ���ւ���
	SwapDrawWindowFromMemoryStream(window, &stream);
}

/*************************************
* ChangeDrawWindowResolutionRedo�֐� *
* �𑜓x�ύX����蒼��               *
* ����                               *
* window	: �`��̈�̏��         *
* p			: �����f�[�^             *
*************************************/
static void ChangeDrawWindowResolutionRedo(DRAW_WINDOW* window, void* p)
{
	// �����f�[�^
	CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY history_data;

	// �ύX��̕��A�����ǂݍ��ނ��߂Ƀf�[�^�R�s�[
	(void)memcpy(&history_data, p,
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data_size));

	// �𑜓x�ύX�����s
	ChangeDrawWindowResolution(window,
		history_data.new_width, history_data.new_height);
}

/*************************************************
* AddChangeDrawWindowResolutionHistory�֐�       *
* �𑜓x�ύX�̗����f�[�^��ǉ�����               *
* ����                                           *
* window		: �𑜓x��ύX����`��̈�̏�� *
* new_width		: �V������                       *
* new_height	: �V��������                     *
*************************************************/
void AddChangeDrawWindowResolutionHistory(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
)
{
	// �����f�[�^
	CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY history_data =
		{new_width, new_height, 0, 0, 0, NULL};
	// �f�[�^�o�^�p�X�g���[��
	MEMORY_STREAM_PTR history_stream;
	// ���݂̏�Ԃ��L�����邽�߂̃X�g���[��
	MEMORY_STREAM_PTR layers_data;
	// �X�g���[���̃T�C�Y
	size_t stream_size;

	// ���C���[�̐�*�s�N�Z���f�[�^ + 8k�����������m�ۂ��Ă���
	stream_size = 8192 +
		window->num_layer * window->width * window->height * window->channel;

	// �X�g���[���쐬
	layers_data = CreateMemoryStream(stream_size);
	// ���݂̏�Ԃ��������X�g���[���ɏ����o��
	WriteOriginalFormat((void*)layers_data,
		(stream_func_t)MemWrite, window, 0, window->app->preference.compress);

	// ���݂̏�Ԃ̃f�[�^�T�C�Y���L������
	history_data.before_data_size = layers_data->data_point;

	// �����f�[�^���쐬����
	stream_size = layers_data->data_point +
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data);
	history_stream = CreateMemoryStream(stream_size);
	(void)MemWrite(&history_data, 1,
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data), history_stream);
	(void)MemWrite(layers_data->buff_ptr, 1, history_data.before_data_size, history_stream);

	AddHistory(&window->history, window->app->labels->menu.change_resolution,
		history_stream->buff_ptr, (uint32)stream_size, ChangeDrawWindowResolutionUndo, ChangeDrawWindowResolutionRedo);

	(void)DeleteMemoryStream(layers_data);
	(void)DeleteMemoryStream(history_stream);
}

/*******************************************
* ChangeDrawWindowResolution�֐�           *
* �𑜓x��ύX����                         *
* ����                                     *
* window		: �𑜓x��ύX����`��̈� *
* new_width		: �V������                 *
* new_height	: �V��������               *
*******************************************/
void ChangeDrawWindowResolution(DRAW_WINDOW* window, int32 new_width, int32 new_height)
{
	// �g��k�����s�����C���[
	LAYER *layer = window->layer;
	// ���C���[���W�ύX�p�̊g��k����
	gdouble zoom_x = (gdouble)new_width / (gdouble)window->width,
		zoom_y = (gdouble)new_height / (gdouble)window->height;
	// ���݂̃A�N�e�B�u���C���[�̖��O
	char active_name[MAX_LAYER_NAME_LENGTH];

	// ���݂̃A�N�e�B�u���C���[�̖��O���L��
	(void)strcpy(active_name, window->active_layer->name);

	// �w�i�̃s�N�Z���f�[�^���X�V
	window->back_ground = (uint8*)MEM_REALLOC_FUNC(
		window->back_ground, new_width*new_height*window->channel);
	(void)memset(window->back_ground, 0xff, new_width*new_height*window->channel);

	// ��ƃ��C���[�A�ꎞ�ۑ����C���[�̃T�C�Y�ύX
	DeleteLayer(&window->temp_layer);
	window->temp_layer = CreateLayer(0, 0, new_width, new_height, 5,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
	ResizeLayerBuffer(window->mask, new_width, new_height);
	ResizeLayerBuffer(window->mask_temp, new_width, new_height);
	ResizeLayerBuffer(window->work_layer, new_width, new_height);
	ResizeLayerBuffer(window->mixed_layer, new_width, new_height);
	ResizeLayerBuffer(window->under_active, new_width, new_height);
	ResizeLayerBuffer(window->selection, new_width, new_height);
	ResizeLayerBuffer(window->texture, new_width, new_height);

	// �V�������ƍ�����`��̈�̏��ɃZ�b�g
	window->width = new_width, window->height = new_height;
	// �s�N�Z���f�[�^�̃o�C�g���A1�s���̃o�C�g�����v�Z
	window->stride = new_width * window->channel;
	window->pixel_buf_size = window->stride * new_height;

	// �������ʂɑ΂��Ċg��E�k����ݒ肷�邽�߂̃p�^�[���쐬������
	window->mixed_pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);
	cairo_pattern_set_filter(window->mixed_pattern, CAIRO_FILTER_FAST);

	// �i�r�Q�[�V�����̕\���ݒ�
	ChangeNavigationDrawWindow(&window->app->navigation_window, window);
	// �e�N�X�`���p�̃��C���[���X�V
	FillTextureLayer(window->texture, &window->app->textures);

	// �S�Ẵ��C���[�����T�C�Y
	while(layer != NULL)
	{
		ResizeLayer(layer, new_width, new_height);

		layer = layer->next;
	}

	// �\���p�̃��C���[�����T�C�Y
	DrawWindowChangeZoom(window, window->zoom);

	// �A�N�e�B�u���C���[�����ɖ߂�
	ChangeActiveLayer(window, SearchLayer(window->layer, active_name));

	// ���C���[������������
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

/***********************************
* ChangeDrawWindowSizeUndo�֐�     *
* �L�����o�X�T�C�Y�̕ύX�����ɖ߂� *
* ����                             *
* window	: �`��̈�̏��       *
* p			: �����f�[�^           *
***********************************/
// ����͉𑜓x�̕ύX�Ɠ���Ȃ̂Ń}�N���őΏ�
#define ChangeDrawWindowSizeUndo ChangeDrawWindowResolutionUndo

/***********************************
* ChangeDrawWindowSizeRedo�֐�     *
* �L�����o�X�T�C�X�̕ύX����蒼�� *
* ����                             *
* window	: �`��̈�̏��       *
* p			: �����f�[�^           *
***********************************/
static void ChangeDrawWindowSizeRedo(DRAW_WINDOW* window, void* p)
{
	// �����f�[�^
	CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY history_data;

	// �ύX��̕��A�����ǂݍ��ނ��߂Ƀf�[�^�R�s�[
	(void)memcpy(&history_data, p,
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data_size));

	// �L�����o�X�T�C�Y�̕ύX�����s
	ChangeDrawWindowResolution(window,
		history_data.new_width, history_data.new_height);
}

/*************************************************
* AddChangeDrawWindowSizeHistory�֐�             *
* �L�����o�X�T�C�Y�ύX�̗����f�[�^��ǉ�����     *
* ����                                           *
* window		: �𑜓x��ύX����`��̈�̏�� *
* new_width		: �V������                       *
* new_height	: �V��������                     *
*************************************************/
void AddChangeDrawWindowSizeHistory(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
)
{
	// �����f�[�^
	CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY history_data =
		{new_width, new_height, 0, 0, 0, NULL};
	// �f�[�^�o�^�p�X�g���[��
	MEMORY_STREAM_PTR history_stream;
	// ���݂̏�Ԃ��L�����邽�߂̃X�g���[��
	MEMORY_STREAM_PTR layers_data;
	// �X�g���[���̃T�C�Y
	size_t stream_size;

	// ���C���[�̐�*�s�N�Z���f�[�^ + 8k�����������m�ۂ��Ă���
	stream_size = 8192 +
		window->num_layer * window->width * window->height * window->channel;

	// �X�g���[���쐬
	layers_data = CreateMemoryStream(stream_size);
	// ���݂̏�Ԃ��������X�g���[���ɏ����o��
	WriteOriginalFormat((void*)layers_data,
		(stream_func_t)MemWrite, window, 0, window->app->preference.compress);

	// ���݂̏�Ԃ̃f�[�^�T�C�Y���L������
	history_data.before_data_size = layers_data->data_point;

	// �����f�[�^���쐬����
	stream_size = layers_data->data_point +
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data);
	history_stream = CreateMemoryStream(stream_size);
	(void)MemWrite(&history_data, 1,
		offsetof(CHANGE_DRAW_WINDOW_RESOLUTION_HISTORY, before_data), history_stream);
	(void)MemWrite(layers_data->buff_ptr, 1, history_data.before_data_size, history_stream);

	AddHistory(&window->history, window->app->labels->menu.change_canvas_size,
		history_stream->buff_ptr, (uint32)stream_size, ChangeDrawWindowSizeUndo, ChangeDrawWindowSizeRedo);

	(void)DeleteMemoryStream(layers_data);
	(void)DeleteMemoryStream(history_stream);
}

/*******************************************
* ChangeDrawWindowSize�֐�                 *
* �`��̈�̃T�C�Y��ύX����               *
* ����                                     *
* window		: �𑜓x��ύX����`��̈� *
* new_width		: �V������                 *
* new_height	: �V��������               *
*******************************************/
void ChangeDrawWindowSize(DRAW_WINDOW* window, int32 new_width, int32 new_height)
{
	// �T�C�Y�ύX���s�����C���[
	LAYER *layer = window->layer;
	// ���݂̃A�N�e�B�u���C���[�̖��O
	char active_name[MAX_LAYER_NAME_LENGTH];

	// ���݂̃A�N�e�B�u���C���[�̖��O���L��
	(void)strcpy(active_name, window->active_layer->name);

	// �w�i�̃s�N�Z���f�[�^���X�V
	window->back_ground = (uint8*)MEM_REALLOC_FUNC(
		window->back_ground, new_width*new_height*window->channel);
	(void)memset(window->back_ground, 0xff, new_width*new_height*window->channel);

	// ��ƃ��C���[�A�ꎞ�ۑ����C���[�̃T�C�Y�ύX
	DeleteLayer(&window->temp_layer);
	window->temp_layer = CreateLayer(0, 0, new_width, new_height, 5,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
	ResizeLayerBuffer(window->mask, new_width, new_height);
	ResizeLayerBuffer(window->mask_temp, new_width, new_height);
	ResizeLayerBuffer(window->work_layer, new_width, new_height);
	ResizeLayerBuffer(window->mixed_layer, new_width, new_height);
	ResizeLayerBuffer(window->under_active, new_width, new_height);
	ResizeLayerBuffer(window->selection, new_width, new_height);

	// �V�������ƍ�����`��̈�̏��ɃZ�b�g
	window->width = new_width, window->height = new_height;
	// �s�N�Z���f�[�^�̃o�C�g���A1�s���̃o�C�g�����v�Z
	window->stride = new_width * window->channel;
	window->pixel_buf_size = window->stride * new_height;

	// �������ʂɑ΂��Ċg��E�k����ݒ肷�邽�߂̃p�^�[���쐬������
	window->mixed_pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);
	cairo_pattern_set_filter(window->mixed_pattern, CAIRO_FILTER_FAST);

	// �i�r�Q�[�V�����̕\���ݒ�
	ChangeNavigationDrawWindow(&window->app->navigation_window, window);
	// �e�N�X�`���p�̃��C���[���X�V
	FillTextureLayer(window->texture, &window->app->textures);

	// �S�Ẵ��C���[�����T�C�Y
	while(layer != NULL)
	{
		ChangeLayerSize(layer, new_width, new_height);

		layer = layer->next;
	}

	// �\���p�̃��C���[�����T�C�Y
	DrawWindowChangeZoom(window, window->zoom);

	// �A�N�e�B�u���C���[�����ɖ߂�
	ChangeActiveLayer(window, SearchLayer(window->layer, active_name));

	// ���C���[������������
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

/*****************************************
* UpdateDrawWindowClippingArea           *
* ��ʍX�V���ɃN���b�s���O����̈���X�V *
* ����                                   *
* window	: �`��̈�̏��             *
*****************************************/
void UpdateDrawWindowClippingArea(DRAW_WINDOW* window)
{
	GtkAllocation allocation;
	int half_width = window->disp_layer->width / 2;
	int half_height = window->disp_layer->height / 2;
	int add_x = half_width - window->disp_size / 2;
	int add_y = half_height - window->disp_size / 2;
	int right, bottom;

	gtk_widget_get_allocation(window->scroll, &allocation);
	right = window->scroll_x + allocation.width;
	bottom = window->scroll_y + allocation.height;

	window->update_clip_area[0][0] =
		(int)((window->scroll_x + add_x - half_width) * window->cos_value
			- (window->scroll_y + add_y - half_height) * window->sin_value) + half_width;
	window->update_clip_area[0][1] =
		(int)((window->scroll_x + add_x - half_width) * window->sin_value
			+ (window->scroll_y + add_y - half_height) * window->cos_value) + half_height;
	window->update_clip_area[1][0] =
		(int)((window->scroll_x + add_x - half_width) * window->cos_value
			- (bottom + add_y - half_height) * window->sin_value) + half_width;
	window->update_clip_area[1][1] =
		(int)((window->scroll_x + add_x - half_width) * window->sin_value
			+ (bottom + add_y - half_height) * window->cos_value) + half_height;
	window->update_clip_area[2][0] =
		(int)((right + add_x - half_width) * window->cos_value
			- (bottom + add_y - half_height) * window->sin_value) + half_width;
	window->update_clip_area[2][1] =
		(int)((right + add_x - half_width) * window->sin_value
			+ (bottom + add_y - half_height) * window->cos_value) + half_height;
	window->update_clip_area[3][0] =
		(int)((right + add_x - half_width) * window->cos_value
			- (window->scroll_y + add_y - half_height) * window->sin_value) + half_width;
	window->update_clip_area[3][1] =
		(int)((right + add_x - half_width) * window->sin_value
			+ (window->scroll_y + add_y - half_height) * window->cos_value) + half_height;

	gtk_widget_queue_draw(window->window);
}

/*************************************************
* ClipUpdateArea�֐�                             *
* ��ʂ̃X�N���[���ɓ����Ă��镔���ŃN���b�s���O *
* ����                                           *
* window	: �`��̈�̏��                     *
* cairo_p	: Cairo���                          *
*************************************************/
void ClipUpdateArea(DRAW_WINDOW* window, cairo_t* cairo_p)
{
	cairo_move_to(cairo_p, window->update_clip_area[0][0], window->update_clip_area[0][1]);
	cairo_line_to(cairo_p, window->update_clip_area[1][0], window->update_clip_area[1][1]);
	cairo_line_to(cairo_p, window->update_clip_area[2][0], window->update_clip_area[2][1]);
	cairo_line_to(cairo_p, window->update_clip_area[3][0], window->update_clip_area[3][1]);
	cairo_close_path(cairo_p);
	cairo_clip(cairo_p);
}

/*******************************************************************
* DrawWindowSetIccProfile�֐�                                      *
* �L�����o�X��ICC�v���t�@�C�������蓖�Ă�                          *
* ����                                                             *
* window	: �`��̈�̏��(icc_profile_data�Ƀf�[�^���蓖�čς�) *
* data_size	: ICC�v���t�@�C���̃f�[�^�̃o�C�g��                    *
* ask_set	: �\�t�g�v���[�t�\����K�p���邩��q�˂邩�ۂ�         *
*******************************************************************/
void DrawWindowSetIccProfile(DRAW_WINDOW* window, int32 data_size, gboolean ask_set)
{
	APPLICATION *app = window->app;
	cmsHPROFILE *monitor_profile;

	if(ask_set != FALSE)
	{
		GtkWidget *dialog = gtk_dialog_new_with_buttons(
			"",
			GTK_WINDOW(window->app->widgets->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_YES, GTK_RESPONSE_YES,
			GTK_STOCK_NO, GTK_RESPONSE_NO,
			NULL
		);
		GtkWidget *label;
		char str[2048];
		char label_str[2048] = "";
		char *copy_start;
		char *p;
		char *next;
		int result;

		(void)strcpy(str, app->labels->make_new.adopt_icc_profile);
		copy_start = p = str;
		next = (char*)g_utf8_next_char(p);
		while(*next != '\0')
		{
			if((uint8)*p == 0x5c && *(p+1) == 'n')
			{
				*p = '\n';
				*(p+1) = '\0';
				(void)strcat(label_str, copy_start);
				copy_start = (char*)g_utf8_next_char(next);
			}
			else if((next - p) >= 2 && (uint8)*p == 0xc2
				&& (uint8)(*(p+1)) == 0xa5 && (uint8)*next == 'n')
			{
				*p = '\n';
				*(p+1) = '\0';
				(void)strcat(label_str, copy_start);
				copy_start = (char*)g_utf8_next_char(next);
			}

			p = next;
			next = (char*)g_utf8_next_char(next);
		}
		(void)strcat(label_str, copy_start);

		label = gtk_label_new(label_str);

		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label, FALSE, FALSE, 2);

		gtk_widget_show_all(dialog);

		result = gtk_dialog_run(GTK_DIALOG(dialog));

		gtk_widget_destroy(dialog);

		if(result != GTK_RESPONSE_YES)
		{
			return;
		}
	}

	window->input_icc = cmsOpenProfileFromMem(window->icc_profile_data, data_size);
	window->icc_profile_size = data_size;

	if(window->input_icc != NULL)
	{
		monitor_profile = GetPrimaryMonitorProfile();

		if(app->output_icc != NULL)
		{
			cmsBool bpc[] = {TRUE, TRUE, TRUE, TRUE};
			cmsHPROFILE h_profiles[] = {window->input_icc, app->output_icc, app->output_icc, monitor_profile};
			cmsUInt32Number intents[] = { INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC };
			cmsFloat64Number adaptation_states[] = {0, 0, 0, 0};

			window->icc_transform = cmsCreateExtendedTransform(cmsGetProfileContextID(h_profiles[1]), 4, h_profiles,
				bpc, intents, adaptation_states, NULL, 0, TYPE_BGRA_8, TYPE_BGRA_8, 0);
		}
		else
		{
			window->icc_transform = cmsCreateTransform(window->input_icc, TYPE_BGRA_8,
				monitor_profile, TYPE_BGRA_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_BLACKPOINTCOMPENSATION);
		}

		// �i�f�B�X�v���C�t�B���^��؂�ւ��āj�\�����X�V
		{
			GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_ICC_PROFILE]);
			gtk_check_menu_item_set_active(item, FALSE);
			gtk_check_menu_item_set_active(item, TRUE);
		}

		cmsCloseProfile(monitor_profile);
	}

	UpdateWindowTitle(app);
}

void ScrollSizeChangeEvent(GtkWidget* scroll, GdkRectangle* size, DRAW_WINDOW* window)
{
	LAYER *layer = window->active_layer;

	if(layer->layer_type == TYPE_3D_LAYER
		&& (window->flags & DRAW_WINDOW_EDITTING_3D_MODEL) != 0)
	{
		int width = size->width;
		int height = size->height;
		int new_width, new_height;

		if(width > SCROLLED_WINDOW_MARGIN)
		{
			width -= SCROLLED_WINDOW_MARGIN;
		}
		if(height > SCROLLED_WINDOW_MARGIN)
		{
			height -= SCROLLED_WINDOW_MARGIN;
		}
		new_width = width;
		new_height = new_width * window->height / window->width;
		if(new_height > height)
		{
			new_height = height;
			new_width = new_height * window->width / window->height;
		}
		gtk_widget_set_size_request(layer->window->window, new_width, new_height);
		gtk_widget_show_all(layer->window->window);
	}
}

void UpdateDrawWindow(DRAW_WINDOW* window)
{
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	gtk_widget_queue_draw(window->window);
}

#ifdef __cplusplus
}
#endif
