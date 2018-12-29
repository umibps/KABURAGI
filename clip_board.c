// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "application.h"
#include "display.h"

#include "./gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	// �A�v���P�[�V�����̏��ɃL���X�g
	APPLICATION* app = (APPLICATION*)data;
	// �s�N�Z���f�[�^
	uint8* pixels;
	// �摜�f�[�^�̕��A�����A��s���̃o�C�g��
	int32 width, height, stride;
	// �s�N�Z���f�[�^�̔j���v��
	gboolean delete_pixbuf = FALSE;
	// for���p�̃J�E���^
	int i;

	// �s�N�Z���o�b�t�@�쐬�Ɏ��s���Ă���Ƃ��͏I��
	if(pixbuf == NULL)
	{
		return;
	}

	// ���`�����l�����Ȃ��ꍇ�͒ǉ�
	if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
	{
		pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
		delete_pixbuf = TRUE;
	}

	// �摜�̕��ƍ������擾
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);

	// �s�N�Z���Ɖ摜��񕪂̃o�C�g�����擾
	pixels = gdk_pixbuf_get_pixels(pixbuf);
	stride = gdk_pixbuf_get_rowstride(pixbuf);
	
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

	// �`��̈悪�Ȃ��Ƃ��͐V�K�쐬
	if(app->window_num == 0)
	{
		// �`��̈��V���ɒǉ�
		app->active_window = app->window_num;
		app->draw_window[app->window_num] = CreateDrawWindow(
			width, height, 4, app->labels->menu.clip_board, app->widgets->note_book, app->window_num, app);

		// �摜�f�[�^����ԉ��̃��C���[�ɃR�s�[
		for(i=0; i<height; i++)
		{
			(void)memcpy(&app->draw_window[app->active_window]->active_layer->pixels[
				app->draw_window[app->active_window]->active_layer->stride*i],
					&pixels[i*stride], app->draw_window[app->active_window]->active_layer->stride
			);
		}

		// ��ԉ��̃��C���[���A�N�e�B�u�\����
		LayerViewSetActiveLayer(
			app->draw_window[app->active_window]->active_layer, app->layer_window.view
		);

		// �����ɂ��Ă����ꕔ�̃��j���[��L����
		for(i=0; i<app->menus.num_disable_if_no_open; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
		}
		gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
		gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

		// �`��̈�̃J�E���^���X�V
		app->window_num++;

		// �E�B���h�E�̃^�C�g�����摜����
		UpdateWindowTitle(app);
	}
	else
	{	// �`��̈�ɐV���Ƀ��C���[���쐬���ăR�s�[
			// �\��t����`��̈�
		DRAW_WINDOW *window = GetActiveDrawWindow(app);
		// �ǉ����郌�C���[
		LAYER *layer;
		// ���C���[�̖��O
		char layer_name[256];
		// �R�s�[���̈�s���̃o�C�g��
		int32 original_stride = stride;
		// �R�s�[��̍��W
		int32 x = 0, y = 0;

		// �I��͈͂�����Ƃ��͍��W��ݒ�
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			x = window->selection_area.min_x;
			y = window->selection_area.min_y;
		}

		// ���C���[�̖��O���쐬
		i = 1;
		(void)strcpy(layer_name, app->labels->layer_window.pasted_layer);
		while(CorrectLayerName(window->layer, layer_name) == 0)
		{
			(void)sprintf(layer_name, "%s (%d)", app->labels->layer_window.pasted_layer, i);
			i++;
		}

		// �A�N�e�B�u�ȕ`��̈�Ƀ��C���[��ǉ�
		layer = CreateLayer(0, 0, window->width,
			window->height, 4, TYPE_NORMAL_LAYER,
			window->active_layer,
			window->active_layer->next,
			layer_name,
			window
		);
		window->num_layer++;
		if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
		{
			DRAW_WINDOW *parent = app->draw_window[app->active_window];
			LAYER *parent_prev = SearchLayer(parent->layer, window->active_layer->name);
			LAYER *parent_next = (parent_prev == NULL) ? parent->layer : parent_prev->next;
			LAYER *parent_new = CreateLayer(0, 0, parent->width, parent->height, parent->channel,
				(eLAYER_TYPE)layer->layer_type, parent_prev, parent_next, layer->name, parent);
			parent->num_layer++;
			AddNewLayerHistory(parent_new, parent_new->layer_type);
		}

		// �摜�f�[�^��ǉ��������C���[�ɃR�s�[
		if(x + width > layer->width)
		{
			width = layer->width - x;
			stride = width * 4;
		}
		if(y + height > layer->height)
		{
			height = layer->height - y;
		}
		for(i=0; i<height; i++)
		{
			(void)memcpy(&layer->pixels[layer->stride*(i+y)+(x*4)],
					&pixels[i*stride], stride
			);
		}

		// �����f�[�^��ǉ�
		AddNewLayerWithImageHistory(layer, pixels, width, height,
			original_stride, 4, app->labels->menu.open_as_layer);

		// �ǉ��������C���[���A�N�e�B�u��
		LayerViewAddLayer(layer, window->layer,
			app->layer_window.view, window->num_layer);
		ChangeActiveLayer(app->draw_window[app->active_window], layer);
		LayerViewSetActiveLayer(
			window->active_layer, app->layer_window.view
		);
	}

	if(delete_pixbuf != FALSE)
	{
		// �s�N�Z���o�b�t�@���J��
		g_object_unref(pixbuf);
	}
}

/*****************************************************
* ExecutePaste�֐�                                   *
* �\��t�������s����                                 *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecutePaste(APPLICATION* app)
{
	// �N���b�v�{�[�h�ɑ΂��ăf�[�^��v��
	gtk_clipboard_request_image(gtk_clipboard_get(GDK_NONE),
		ClipBoardImageRecieveCallBack, app);
}

/*****************************************************
* ExecuteCopy�֐�                                    *
* �R�s�[�����s                                       *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteCopy(APPLICATION* app)
{
	// �R�s�[����s�N�Z���o�b�t�@
	GdkPixbuf* pixbuf;
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW* window = GetActiveDrawWindow(app);
	// �R�s�[��̃s�N�Z���f�[�^
	uint8* pixels;
	// �R�s�[�J�n���W
	int x, y;
	// �R�s�[���镝�A����
	int width, height;
	// �R�s�[�����s���̃o�C�g��
	int stride;
	// for���p�̃J�E���^
	int i;

	// �R�s�[������W�A���A����������
	x = window->selection_area.min_x;
	y = window->selection_area.min_y;
	width = window->selection_area.max_x - x;
	height = window->selection_area.max_y - y;
	stride = width * window->channel;

	// �I��͈͂̃s�N�Z���݂̂��c��
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(window->temp_layer->cairo_p, window->active_layer->surface_p, 0, 0);
	cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);

	// �s�N�Z���o�b�t�@���쐬
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, window->channel == 4, 8, width, height);

	// �s�N�Z���f�[�^���擾
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	// �f�[�^���ʂ�
	for(i=0; i<height; i++)
	{
		(void)memcpy(
			&pixels[i*stride],
			&window->active_layer->pixels[(y+i)*window->active_layer->stride+x*window->active_layer->channel],
			stride
		);
	}
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

	// �N���b�v�{�[�h�փf�[�^���R�s�[
	gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), pixbuf);

	// �s�N�Z���o�b�t�@���J��
	g_object_unref(pixbuf);
}

/*****************************************************
* ExecuteCopyVisible�֐�                             *
* �����R�s�[�����s                                 *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteCopyVisible(APPLICATION* app)
{
	// �R�s�[����s�N�Z���o�b�t�@
	GdkPixbuf* pixbuf;
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW* window = GetActiveDrawWindow(app);
	// �����������C���[
	LAYER *mixed = MixLayerForSave(window);
	// �R�s�[��̃s�N�Z���f�[�^
	uint8* pixels;
	// �R�s�[�J�n���W
	int x, y;
	// �R�s�[���镝�A����
	int width, height;
	// �R�s�[�����s���̃o�C�g��
	int stride;
	// for���p�̃J�E���^
	int i;

	// �R�s�[������W�A���A����������
	x = window->selection_area.min_x;
	y = window->selection_area.min_y;
	width = window->selection_area.max_x - x;
	height = window->selection_area.max_y - y;
	stride = width * window->channel;

	// �I��͈͂̃s�N�Z���݂̂��c��
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(window->temp_layer->cairo_p, mixed->surface_p, 0, 0);
	cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);

	// �s�N�Z���o�b�t�@���쐬
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, window->channel == 4, 8, width, height);

	// �s�N�Z���f�[�^���擾
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	// �f�[�^���ʂ�
	for(i=0; i<height; i++)
	{
		(void)memcpy(
			&pixels[i*stride],
			&window->active_layer->pixels[(y+i)*window->active_layer->stride+x*window->active_layer->channel],
			stride
		);
	}
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

	// �N���b�v�{�[�h�փf�[�^���R�s�[
	gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), pixbuf);

	// �s�N�Z���o�b�t�@���J��
	g_object_unref(pixbuf);

	// �����������C���[�͊��ɕs�v
	DeleteLayer(&mixed);
}

/*****************************************************
* ExecuteCut�֐�                                     *
* �؂�������s                                     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteCut(APPLICATION* app)
{
	// �R�s�[����s�N�Z���o�b�t�@
	GdkPixbuf* pixbuf;
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW* window = GetActiveDrawWindow(app);
	// �R�s�[��̃s�N�Z���f�[�^
	uint8* pixels;
	// �R�s�[�J�n���W
	int x, y;
	// �R�s�[���镝�A����
	int width, height;
	// �R�s�[�����s���̃o�C�g��
	int stride;
	// for���p�̃J�E���^
	int i;

	// �R�s�[������W�A���A����������
	x = window->selection_area.min_x;
	y = window->selection_area.min_y;
	width = window->selection_area.max_x - x;
	height = window->selection_area.max_y - y;
	stride = width * window->channel;

	// �I��͈͂̃s�N�Z���݂̂��c��
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(window->temp_layer->cairo_p, window->active_layer->surface_p, 0, 0);
	cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);

	// �s�N�Z���o�b�t�@���쐬
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, window->channel == 4, 8, width, height);

	// �s�N�Z���f�[�^���擾
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	// �f�[�^���ʂ�
	for(i=0; i<height; i++)
	{
		(void)memcpy(
			&pixels[i*stride],
			&window->active_layer->pixels[(y+i)*window->active_layer->stride+x*window->active_layer->channel],
			stride
		);
	}
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

	// �N���b�v�{�[�h�փf�[�^���R�s�[
	gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), pixbuf);

	// �s�N�Z���o�b�t�@���J��
	g_object_unref(pixbuf);

	// �����f�[�^��ǉ�
	AddDeletePixelsHistory(app->labels->menu.cut, window, window->active_layer,
		window->selection_area.min_x, window->selection_area.min_y,
		window->selection_area.max_x, window->selection_area.min_y
	);

	// �I��͈͂̃s�N�Z���f�[�^���폜
	DeleteSelectAreaPixels(window, window->active_layer, window->selection);

	// ��ʂ��X�V
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

#ifdef __cplusplus
}
#endif
