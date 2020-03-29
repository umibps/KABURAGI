// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <errno.h>
#include "application.h"
#include "menu.h"
#include "utils.h"
#include "memory.h"

// �h���b�O&�h���b�v�̔���p
#define DROP_URI 10

#define INITIALIZE_FILE_PATH "./application.ini"

/*
* OnDeleteMainWindow�֐�
* ���C���E�B���h�E��������O�ɌĂяo�����
* ����
* window		: ���C���E�B���h�E�E�B�W�F�b�g
* event_info	: �E�B���h�E�����C�x���g�̏��
* app			: �A�v���P�[�V�������Ǘ�����\����
* �Ԃ�l
*	���̂܂܃E�B���h�E�����:FALSE	�E�B���h�E����Ȃ�:TRUE
*/
static gboolean OnDeleteMainWindow(GtkWidget* window, GdkEvent* event_info, APPLICATION* app)
{
	// �������f�[�^�������o��
	WriteInitialize(app, INITIALIZE_FILE_PATH);

	// �v���O���������S�I��
	gtk_main_quit();

	return FALSE;
}

/*
* DragDataRecieveCallBack�֐�
* �h���b�O&�h���b�v���ꂽ���ɌĂяo�����
* ����
* widget			: �h���b�O&�h���b�v���ꂽ�E�B�W�F�b�g
* conext			: �h���b�O&�h���b�v�����̃R���e�L�X�g
* x					: �h���b�v���ꂽ���̃}�E�X��X���W
* y					: �h���b�v���ꂽ���̃}�E�X��Y���W
* selection_data	: �t�@�C���p�X�������Ă���
* target_type		: �h���b�v���ꂽ�f�[�^�̃^�C�v
* time_stamp		: �h���b�v�C�x���g�̔�����������
* app				: �A�v���P�[�V�������Ǘ�����\����
*/
static void DragDataRecieveCallBack(
	GtkWidget* widget,
	GdkDragContext* context,
	gint x,
	gint y,
	GtkSelectionData* selection_data,
	guint target_type,
	guint time_stamp,
	APPLICATION* app
)
{
	// �h���b�v���ꂽ�f�[�^�̐��������m�F
	if((selection_data != NULL) && (gtk_selection_data_get_length(selection_data) >= 0))
	{	// �h���b�v���ꂽ�f�[�^�̓t�@�C���p�X
		if(target_type == DROP_URI)
		{
			// �f�B���N�g���I�[�v���p
			GDir *dir;
			// ���^�f�[�^�擾�p
			SOUND_PROPERTY *property_data;
			FILE *fp;
			// �h���b�v���ꂽ�f�[�^����URI�̌`���Ŏ��o��
			gchar **uris;
			gchar **uri;
			// UTF-8�̃p�X
			gchar *file_path;
			// �V�X�e���̃p�X
			gchar *system_path;
			// '\'��'/'�֕ϊ��p
			gchar *str;

			// URI�`��������URI�ɕϊ�
			uris = g_uri_list_extract_uris(selection_data->data);
			// ���ꂼ���URI��UTF-8��OS�̕����R�[�h�ɕϊ�����
				// �t�@�C�����I�[�v���A���^�f�[�^���擾
			for(uri = uris; *uri != NULL; uri++)
			{
				file_path = g_filename_from_uri(*uri, NULL, NULL);
				// '\'��'/'�֕ϊ�
				str = file_path;
				while(*str != '\0')
				{
					if(*str == '\\')
					{
						*str = '/';
					}

					str = g_utf8_next_char(str);
				}

				// �f�B���N�g���I�[�v��
				dir = g_dir_open(file_path, 0, NULL);
				// �I�[�v���Ɏ��s������t�@�C���I�[�v��������
				if(dir == NULL)
				{
					// �V�X�e���̕����R�[�֕ϊ�
					system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);

					// �t�@�C���p�X���L��
					MEM_FREE_FUNC(app->play_data.file_path);

					// �t�@�C�����I�[�v������
					fp = fopen(system_path, "rb");
					if(fp != NULL)
					{	// �I�[�v�����������烁�^�f�[�^���擾
						property_data = GetSoundMetaData(
								(void*)fp, (size_t (*)(void*, size_t, size_t, void*))fread,
								(int (*)(void*, long, int))fseek, (long (*)(void*))ftell);
						if(property_data != NULL)
						{
							property_data->path = MEM_STRDUP_FUNC(system_path);
							AppendSoundPropertyData(app->widgets.list_view, property_data);
							ListAppendSoundData(app, property_data);
						}
						(void)fclose(fp);
					}

					g_free(system_path);
				}	// // �I�[�v���Ɏ��s������t�@�C���I�[�v��������
					// if(dir == NULL)
				else
				{	// ���������̂Ńf�B���N�g�����̑S�ẴA�C�e�����`�F�b�N
					g_dir_close(dir);

					GetDirectoryContents(app, file_path);
				}

				g_free(file_path);
			}

			g_strfreev(uris);
		}

		// �h���b�O&�h���b�v�̃f�[�^���폜
		gtk_drag_finish(context, TRUE, TRUE, time_stamp);
	}
}

/*
* PlayButtonClicked�֐�
* �Đ��{�^�����N���b�N���ꂽ���ɌĂяo�����
* ����
* button	: �{�^���E�B�W�F�b�g
* app		: �A�v���P�[�V�������Ǘ�����\����
*/
static void PlayButtonClicked(GtkWidget* button, APPLICATION* app)
{
	// �������[�v����t���O���`�F�b�N
	if((app->flags & APPLICATION_FLAG_IN_PLAY_CONTROL) != 0)
	{
		return;
	}

	// �������[�v����t���O�𗧂ĂĂ��珈���J�n
	app->flags |= APPLICATION_FLAG_IN_PLAY_CONTROL;

	// �{�^���������ꂽ��ԂɂȂ�����Đ��J�n
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		app->flags &= ~(APPLICATION_FLAG_PAUSE);
	}

	// �������I�������̂Ŗ������[�v����t���O���~�낷
	app->flags &= ~(APPLICATION_FLAG_IN_PLAY_CONTROL);
}

/*
* PauseButtonClicked�֐�
* �ꎞ��~�{�^�����N���b�N���ꂽ���ɌĂяo�����
* button	: �{�^���E�B�W�F�b�g
* app		: �A�v���P�[�V�������Ǘ�����\����
*/
static void PauseButtonClicked(GtkWidget* button, APPLICATION* app)
{
	// �������[�v����t���O���`�F�b�N
	if((app->flags & APPLICATION_FLAG_IN_PLAY_CONTROL) != 0)
	{
		return;
	}

	// �������[�v����t���O�𗧂ĂĂ��珈���J�n
	app->flags |= APPLICATION_FLAG_IN_PLAY_CONTROL;

	// �{�^���������ꂽ
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		// �����Đ����Ă��Ȃ���΃{�^���̏�Ԃ�
			// �ύX����
		if(app->play_data.audio_player == NULL)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
		}
		else
		{	// �ꎞ��~�̃t���O�𗧂Ă�
			app->flags |= APPLICATION_FLAG_PAUSE;
			// �ꎞ��~�����s
			alSourcePause(app->play_data.audio_player->source);
		}
	}
	else
	{	// �{�^���̏�Ԃ��߂���
		app->flags &= ~(APPLICATION_FLAG_PAUSE);
	}

	// �������I�������̂Ŗ������[�v����t���O���~�낷
	app->flags &= ~(APPLICATION_FLAG_IN_PLAY_CONTROL);
}

/*
* ChangePlayModeButtonClicked�֐�
* �Đ����[�h�ݒ�p�{�^���������ꂽ���ɌĂяo�����
* ����
* button	: �{�^���E�B�W�F�b�g
* app		: �A�v���P�[�V�������Ǘ�����\����
*/
static void ChangePlayModeButtonClicked(GtkWidget* button, APPLICATION* app)
{
	switch(app->play_data.play_mode)
	{
	case APPLICATION_PLAYER_STOP_AT_END:	// 1�ȍĐ��ŏI��
		app->play_data.play_mode = APPLICATION_PLAYER_ONE_LOOP;
		gtk_image_set_from_file(GTK_IMAGE(app->widgets.play_mode_image), "./image/loop_play.png");
		break;
	case APPLICATION_PLAYER_ONE_LOOP:	// 1�ȃ��[�v
		app->play_data.play_mode = APPLICATION_PLAYER_STOP_AT_LIST_END;
		gtk_image_set_from_file(GTK_IMAGE(app->widgets.play_mode_image), "./image/list_once.png");
		break;
	case APPLICATION_PLAYER_STOP_AT_LIST_END:	// ���X�g��1��
		app->play_data.play_mode = APPLICATION_PLAYER_LIST_LOOP;
		gtk_image_set_from_file(GTK_IMAGE(app->widgets.play_mode_image), "./image/list_loop.png");
		break;
	case APPLICATION_PLAYER_LIST_LOOP:	// ���X�g�����[�v
		app->play_data.play_mode = APPLICATION_PLAYER_STOP_AT_END;
		gtk_image_set_from_file(GTK_IMAGE(app->widgets.play_mode_image), "./image/play_once.png");
		break;
	}
}

/*
* VolumeChanged�֐�
* �{�����[���R���g���[�������삳�ꂽ���ɌĂяo�����
* ����
* control	: �{�����[���R���g���[���E�B�W�F�b�g
* volume	: �{�����[���̒l
* app		: �A�v���P�[�V�������Ǘ�����\����
*/
static void VolumeControl(GtkWidget* control, gdouble volume, APPLICATION* app)
{
	app->play_data.volume = volume;
	if(app->play_data.audio_player != NULL)
	{
		alSourcef(app->play_data.audio_player->source, AL_GAIN, (ALfloat)volume);
	}
}

/*
* Initialize�֐�
* �A�v���P�[�V�����̏���������
* ����
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃f�[�^
*/
void Initialize(APPLICATION* app)
{
#define BUTTON_IMAGE_SIZE 32
	// �������f�[�^
	APPLICATION_INITIALIZE initialize = {0};
	// ���C�A�E�g�p
	GtkWidget *vbox;
	GtkWidget *hbox;
	// �Đ��R���g���[���p�̃{�^��
	GtkWidget *button;
	// �{�^���ɉ摜�\���p
	GtkWidget *image;
	// �`����s�p
	uint8 *pixels;
	GdkPixbuf *pixbuf;
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	// �t�@�C���h���b�v�Ή��p
	GtkTargetEntry target_list[] = {{"text/uri-list", 0, DROP_URI}};

	// �������f�[�^��ǂݍ���
	ReadInitializeFile(app, &initialize, INITIALIZE_FILE_PATH);

	// ���x���f�[�^��ǂݍ���
	LoadLabel(app->label, "./lang/japanese.lang");

	// ���C���E�B���h�E���쐬
	app->widgets.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	// �E�B���h�E�̍��W��ݒ�
	gtk_window_move(GTK_WINDOW(app->widgets.window), initialize.window_x, initialize.window_y);
	// �E�B���h�E�̃T�C�Y��ݒ�
	gtk_widget_set_size_request(app->widgets.window, initialize.window_width, initialize.window_height);

	// ���C���E�B���h�E�����鎞�̃C�x���g���L�q
	(void)g_signal_connect(G_OBJECT(app->widgets.window), "delete-event",
		G_CALLBACK(OnDeleteMainWindow), app);

	// �t�@�C���̃h���b�v��ݒ�
	gtk_drag_dest_set(
		app->widgets.window,
		GTK_DEST_DEFAULT_ALL,
		target_list,
		1,
		GDK_ACTION_COPY
	);
	gtk_drag_dest_add_uri_targets(app->widgets.window);
	(void)g_signal_connect(G_OBJECT(app->widgets.window), "drag-data-received",
		G_CALLBACK(DragDataRecieveCallBack), app);

	// �����ŌĂяo�����֐���ݒ�
	app->widgets.time_oud_id = g_timeout_add_full(G_PRIORITY_LOW, 1000/60, (GSourceFunc)TimeOutCallBack,
		app, NULL);

	// �R���g���[�����c�ɕ��ׂ邽�߂̃p�b�L���O�{�b�N�X
	vbox = gtk_vbox_new(FALSE, 0);
	// ���C���E�B���h�E�o�^
	gtk_container_add(GTK_CONTAINER(app->widgets.window), vbox);

	// ���j���[�o�[��o�^
	gtk_box_pack_start(GTK_BOX(vbox), MenuBarNew(app->label, app), FALSE, FALSE, 0);

	// �t�@�C�����X�g�r���[��o�^
	app->widgets.scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(app->widgets.scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	app->widgets.list_view = gtk_tree_view_new();
	// �\����������
	ModelInitialize(app->widgets.list_view, app->label);
	// �\�����Ȗ͗l��
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(app->widgets.list_view), TRUE);
	// �X�N���[�����Ƀ��X�g�r���[��\��
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(app->widgets.scrolled_window), app->widgets.list_view);
	// �_�u���N���b�N���̃R�[���o�b�N�֐���ݒ�
	(void)g_signal_connect(G_OBJECT(app->widgets.list_view), "row-activated",
		G_CALLBACK(ListViewDoubleClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), app->widgets.scrolled_window, TRUE, TRUE, 0);

	// �Đ��A�ꎞ��~�{�^�����쐬
	hbox = gtk_hbox_new(FALSE, 3);
	// �`��p�̃s�N�Z���f�[�^���쐬
	pixels = (uint8*)MEM_ALLOC_FUNC(BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	(void)memset(pixels, 0, BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	// �C���[�W�E�B�W�F�b�g�ɃZ�b�g�o����`���ɕϊ�
	pixbuf = gdk_pixbuf_new_from_data(pixels, GDK_COLORSPACE_RGB, TRUE, 8, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE,
		BUTTON_IMAGE_SIZE * 4, NULL, NULL);
	// �`�惉�C�u�����Ƀs�N�Z���f�[�^��n��
		// �`��C���[�W�쐬
	surface_p = cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_ARGB32,
		BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE * 4);
	// �`��R���e�L�X�g���쐬
	cairo_p = cairo_create(surface_p);
	// �Đ��̎O�p���쐬
		// �F���w��
	cairo_set_source_rgb(cairo_p, 1, 0, 0);
	cairo_move_to(cairo_p, BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE * 0.5);
	cairo_fill(cairo_p);
	// �C���[�W�E�B�W�F�b�g���쐬
	image = gtk_image_new_from_pixbuf(pixbuf);
	// �{�^���쐬
	app->widgets.play_button = gtk_toggle_button_new();
	// �{�^���̒��ɃC���[�W������
	gtk_container_add(GTK_CONTAINER(app->widgets.play_button), image);
	(void)g_signal_connect(G_OBJECT(app->widgets.play_button), "toggled",
		G_CALLBACK(PlayButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(hbox), app->widgets.play_button, FALSE, FALSE, 0);
	// �R���e�L�X�g���폜
	cairo_surface_destroy(surface_p);
	cairo_destroy(cairo_p);

	// �ꎞ��~�̃C���[�W���쐬
		// �`��p�̃s�N�Z���f�[�^���쐬
	pixels = (uint8*)MEM_ALLOC_FUNC(BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	(void)memset(pixels, 0, BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	// �C���[�W�E�B�W�F�b�g�ɃZ�b�g�o����`���ɕϊ�
	pixbuf = gdk_pixbuf_new_from_data(pixels, GDK_COLORSPACE_RGB, TRUE, 8, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE,
		BUTTON_IMAGE_SIZE * 4, NULL, NULL);
	// �`�惉�C�u�����Ƀs�N�Z���f�[�^��n��
		// �`��C���[�W�쐬
	surface_p = cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_ARGB32,
		BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE * 4);
	// �`��R���e�L�X�g���쐬
	cairo_p = cairo_create(surface_p);
	pixbuf = gdk_pixbuf_new_from_data(pixels, GDK_COLORSPACE_RGB, TRUE, 8, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE,
		BUTTON_IMAGE_SIZE * 4, NULL, NULL);
	(void)memset(pixels, 0, BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	cairo_set_source_rgb(cairo_p, 0, 0, 0);
	cairo_move_to(cairo_p, BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.35, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.35, BUTTON_IMAGE_SIZE * 0.1);
	cairo_fill(cairo_p);
	cairo_set_source_rgb(cairo_p, 0, 0, 0);
	cairo_move_to(cairo_p, BUTTON_IMAGE_SIZE * 0.65, BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.65, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE * 0.1);
	cairo_fill(cairo_p);
	image = gtk_image_new_from_pixbuf(pixbuf);
	app->widgets.pause_button = gtk_toggle_button_new();
	(void)g_signal_connect(G_OBJECT(app->widgets.pause_button), "toggled",
		G_CALLBACK(PauseButtonClicked), app);
	gtk_container_add(GTK_CONTAINER(app->widgets.pause_button), image);
	gtk_box_pack_start(GTK_BOX(hbox), app->widgets.pause_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	cairo_surface_destroy(surface_p);
	cairo_destroy(cairo_p);
	// �Đ����[�h�̃{�^�����쐬
	app->widgets.play_mode_image = gtk_image_new_from_file("./image/play_once.png");
	button = gtk_button_new();
	(void)g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(ChangePlayModeButtonClicked), app);
	gtk_container_add(GTK_CONTAINER(button), app->widgets.play_mode_image);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 10);

	// �{�����[���R���g���[�����쐬
	app->widgets.volume_control = gtk_volume_button_new();
	// �E�B���h�E�ɒǉ�
	gtk_box_pack_end(GTK_BOX(vbox), app->widgets.volume_control, FALSE, FALSE, 0);
	// �{�����[����ݒ�
	gtk_scale_button_set_value(GTK_SCALE_BUTTON(app->widgets.volume_control), app->play_data.volume);
	// �{�����[�����쎞�̃R�[���o�b�N�֐����Z�b�g
	(void)g_signal_connect(G_OBJECT(app->widgets.volume_control), "value-changed",
		G_CALLBACK(VolumeControl), app);

	// ���C���E�B���h�E��\��
	gtk_widget_show_all(app->widgets.window);
}
