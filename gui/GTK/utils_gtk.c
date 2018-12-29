#include "../../utils.h"
#include "utils_gtk.h"

#ifdef __cplusplus
extern "C" {
#endif


int ForFontFamilySearchCompare(const char* str, PangoFontFamily** font)
{
	return StringCompareIgnoreCase(str, pango_font_family_get_name(*font));
}

/*******************************************
* FileRead�֐�                             *
* Gtk�̊֐��𗘗p���ăt�@�C���ǂݍ���      *
* ����                                     *
* dst			: �ǂݍ��ݐ�̃A�h���X     *
* block_size	: �ǂݍ��ރu���b�N�̃T�C�Y *
* read_num		: �ǂݍ��ރu���b�N�̐�     *
* stream		: �ǂݍ��݌��̃X�g���[��   *
* �Ԃ�l                                   *
*	�ǂݍ��񂾃u���b�N��                   *
*******************************************/
size_t FileRead(void* dst, size_t block_size, size_t read_num, GFileInputStream* stream)
{
	return g_input_stream_read(
		G_INPUT_STREAM(stream), dst, block_size*read_num, NULL, NULL) / block_size;
}

/*******************************************
* FileWrite�֐�                            *
* Gtk�̊֐��𗘗p���ăt�@�C����������      *
* ����                                     *
* src			: �������݌��̃A�h���X     *
* block_size	: �������ރu���b�N�̃T�C�Y *
* read_num		: �������ރu���b�N�̐�     *
* stream		: �������ݐ�̃X�g���[��   *
* �Ԃ�l                                   *
*	�������񂾃u���b�N��                   *
*******************************************/
size_t FileWrite(void* src, size_t block_size, size_t read_num, GFileOutputStream* stream)
{
	return g_output_stream_write(
		G_OUTPUT_STREAM(stream), src, block_size*read_num, NULL, NULL) / block_size;
}

/********************************************
* FileSeek�֐�                              *
* Gtk�̊֐��𗘗p���ăt�@�C���V�[�N         *
* ����                                      *
* stream	: �V�[�N���s���X�g���[��        *
* offset	: �ړ��o�C�g��                  *
* origin	: �ړ��J�n�ʒu(fseek�֐��Ɠ���) *
* �Ԃ�l                                    *
*	����I��(0), �ُ�I��(0�ȊO)            *
********************************************/
int FileSeek(void* stream, long offset, int origin)
{
	GSeekType seek;

	switch(origin)
	{
	case SEEK_SET:
		seek = G_SEEK_SET;
		break;
	case SEEK_CUR:
		seek = G_SEEK_CUR;
		break;
	case SEEK_END:
		seek = G_SEEK_END;
		break;
	default:
		return -1;
	}

	return !(g_seekable_seek(G_SEEKABLE(stream), offset, seek, NULL, NULL));
}

/************************************************
* FileSeekTell�֐�                              *
* Gtk�̊֐��𗘗p���ăt�@�C���̃V�[�N�ʒu��Ԃ� *
* ����                                          *
* stream	: �V�[�N�ʒu�𒲂ׂ�X�g���[��      *
* �Ԃ�l                                        *
*	�V�[�N�ʒu                                  *
************************************************/
long FileSeekTell(void* stream)
{
	return (long)g_seekable_tell(G_SEEKABLE(stream));
}

void AdjustmentChangeValueCallBackInt(GtkAdjustment* adjustment, int* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (int)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackUint(GtkAdjustment* adjustment, unsigned int* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (unsigned int)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackInt8(GtkAdjustment* adjustment, int8* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (int8)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackUint8(GtkAdjustment* adjustment, uint8* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (uint8)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackInt16(GtkAdjustment* adjustment, int16* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (int16)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackUint16(GtkAdjustment* adjustment, uint16* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (uint16)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackInt32(GtkAdjustment* adjustment, int32* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (int32)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackUint32(GtkAdjustment* adjustment, uint32* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (uint32)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackDouble(GtkAdjustment* adjustment, gdouble* value)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*value = gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackDoubleRate(GtkAdjustment* adjustment, gdouble* value)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*value = gtk_adjustment_get_value(adjustment) * 0.01;

	if(func != NULL)
	{
		func(func_data);
	}
}

void SetAdjustmentChangeValueCallBack(
	GtkAdjustment* adjustment,
	void (*func)(void*),
	void* func_data
)
{
	g_object_set_data(G_OBJECT(adjustment), "changed_callback", func);
	g_object_set_data(G_OBJECT(adjustment), "callback_data", func_data);
}

static void CheckButtonChangeFlags(GtkWidget* button, guint32* flags)
{
	guint32 flag_value = (guint32)GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(button), "flag-value"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		*flags &= ~flag_value;
	}
	else
	{
		*flags |= flag_value;
	}
}

void UpdateWidget(GtkWidget* widget)
{
#define MAX_EVENTS 500
	GdkEvent *queued_event;
	int counter = 0;
	// �C�x���g���񂵂ă��b�Z�[�W��\��

	gdk_threads_enter();
#if GTK_MAJOR_VERSION <= 2
	gdk_window_process_updates(widget->window, TRUE);
#else
	gdk_window_process_updates(gtk_widget_get_window(widget), TRUE);
#endif

	g_usleep(1);

	gdk_threads_leave();

	while(gdk_events_pending() != FALSE && counter < MAX_EVENTS)
	{
		counter++;
		queued_event = gdk_event_get();
		gtk_main_iteration();
		if(queued_event != NULL)
		{
#if GTK_MAJOR_VERSION <= 2
			if(queued_event->any.window == widget->window
#else
			if(queued_event->any.window == gtk_widget_get_window(widget)
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
}

void CheckButtonSetFlagsCallBack(GtkWidget* button, unsigned int* flags, unsigned int flag_value)
{
	g_object_set_data(G_OBJECT(button), "flag-value", GUINT_TO_POINTER(flag_value));
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(CheckButtonChangeFlags), flags);
}

#ifdef __cplusplus
}
#endif
