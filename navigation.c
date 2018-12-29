#include <string.h>
#include <math.h>
#include "configure.h"
#include "navigation.h"
#include "application.h"
#include "memory.h"
#include "widgets.h"

#include "./gui/GTK/input_gtk.h"
#include "./gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************
* ChangeNavigationRotate�֐�                     *
* �i�r�Q�[�V�����̉�]�p�x�ύX���ɌĂяo���֐�   *
* ����                                           *
* navigation	: �i�r�Q�[�V�����E�B���h�E�̏�� *
* draw_window	: �\������`��̈�               *
*************************************************/
void ChangeNavigationRotate(NAVIGATION_WINDOW* navigation, DRAW_WINDOW* window)
{
	cairo_matrix_t matrix;
	FLOAT_T zoom_x, zoom_y;
	FLOAT_T trans_x, trans_y;
	FLOAT_T half_size;
	int half_width, half_height;

	if(window->width > window->height)
	{
		half_size = window->width * ROOT2;
	}
	else
	{
		half_size = window->height * ROOT2;
	}

	half_width = navigation->width / 2;
	half_height = navigation->height / 2;

	zoom_x = ((gdouble)window->width) / navigation->width;
	zoom_y = ((gdouble)window->height) / navigation->height;


	if((window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) == 0)
	{
		trans_x = - half_width*zoom_x +
			(half_width * (1/ROOT2) * zoom_x * window->cos_value + half_height * (1/ROOT2) * zoom_x * window->sin_value);
		trans_x /= zoom_x;
		trans_y = - half_height*zoom_y - (
			half_width * (1/ROOT2) * zoom_y * window->sin_value - half_height * (1/ROOT2) * zoom_y * window->cos_value);
		trans_y /= zoom_y;

		cairo_matrix_init_scale(&matrix, zoom_x*ROOT2, zoom_y*ROOT2);
		cairo_matrix_rotate(&matrix, window->angle);
		cairo_matrix_translate(&matrix, trans_x, trans_y);
	}
	else
	{
		trans_x = - half_width*zoom_x +
			(- half_width * (1/ROOT2) * zoom_x * window->cos_value + half_height * (1/ROOT2) * zoom_x * window->sin_value);
		trans_x /= zoom_x;
		trans_y = - half_height*zoom_y - (
			- half_width * (1/ROOT2) * zoom_y * window->sin_value - half_height * (1/ROOT2) * zoom_y * window->cos_value);
		trans_y /= zoom_y;

		cairo_matrix_init_scale(&matrix, - zoom_x*ROOT2, zoom_y*ROOT2);
		cairo_matrix_rotate(&matrix, window->angle);
		cairo_matrix_translate(&matrix, trans_x, trans_y);
	}

	cairo_pattern_set_matrix(navigation->pattern, &matrix);
}

/*************************************************
* ChangeNavigationDrawWindow�֐�                 *
* �i�r�Q�[�V�����ŕ\������`��̈��ύX����     *
* ����                                           *
* navigation	: �i�r�Q�[�V�����E�B���h�E�̏�� *
* draw_window	: �\������`��̈�               *
*************************************************/
void ChangeNavigationDrawWindow(NAVIGATION_WINDOW* navigation, DRAW_WINDOW* window)
{
	GtkAllocation draw_allocation, navi_allocation;

	// �A�N�e�B�u�ȕ`��̈�̃E�B���h�E�T�C�Y���擾
	gtk_widget_get_allocation(window->scroll, &draw_allocation);
	// �i�r�Q�[�V�����̃T���l�C���̃T�C�Y���擾
	gtk_widget_get_allocation(navigation->draw_area, &navi_allocation);

	if(navigation->pattern != NULL)
	{
		cairo_pattern_destroy(navigation->pattern);
	}
	navigation->pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);

	navigation->point.width =
		(gdouble)draw_allocation.width / ((gdouble)window->disp_size);
	if(navigation->point.width > 1.0)
	{
		navigation->point.width = 1.0;
	}
	// �i�r�Q�[�V�������w�肵�Ă��鍂��
	navigation->point.height =
		(gdouble)draw_allocation.height / ((gdouble)window->disp_size);
	if(navigation->point.height > 1.0)
	{
		navigation->point.height = 1.0;
	}

	// �`��̈�̕��E�������L��
	navigation->draw_width = draw_allocation.width;
	navigation->draw_height = draw_allocation.height;

	// �\�����W�␳�p�̒l���v�Z
	navigation->point.ratio_x =
		(1.0 / window->disp_size) * navi_allocation.width;
	navigation->point.ratio_y =
		(1.0 / window->disp_size) * navi_allocation.height;

	// ��]�p�x���L��
	navigation->point.angle = window->angle;
	// �g��k�������L��
	navigation->point.zoom = window->zoom;

	// �i�r�Q�[�V�����̃p�^�[������]
	ChangeNavigationRotate(navigation, window);
}

/*************************************************************
* DisplayNavigation�֐�                                      *
* �i�r�Q�[�V�����E�B���h�E�̕`�揈��                         *
* ����                                                       *
* widget	: �i�r�Q�[�V�����E�B���h�E�̕`��̈�E�B�W�F�b�g *
* event		: �`��C�x���g�̏��                             *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X     *
* �Ԃ�l                                                     *
*	���TRUE                                                 *
*************************************************************/
#if GTK_MAJOR_VERSION <= 2
static gboolean DisplayNavigation(
	GtkWidget *widget,
	GdkEventExpose *event,
	APPLICATION *app
)
#else
static gboolean DisplayNavigation(
	GtkWidget *widget,
	cairo_t *cairo_p,
	APPLICATION *app
)
#endif
{
	// �\������`��̈�
	DRAW_WINDOW *window;
#if GTK_MAJOR_VERSION <= 2
	// ��ʕ\���p��Cairo���
	cairo_t *cairo_p;
#endif
	// ��ʕ\���p�̃s�N�Z���f�[�^��1�s���̃o�C�g��
	int32 stride;
	// �`��̈�ƃi�r�Q�[�V�����̃T���l�C���̃T�C�Y�擾�p
	GtkAllocation draw_allocation, navi_allocation;

	// �L�����o�X�̐���0�Ȃ�I��
	if(app->window_num == 0)
	{
		return TRUE;
	}

	window = GetActiveDrawWindow(app);

	// �`��̈�̃T�C�Y���擾
	gtk_widget_get_allocation(window->scroll, &draw_allocation);
	// �i�r�Q�[�V�����̃T���l�C���̃T�C�Y���擾
	gtk_widget_get_allocation(
		app->navigation_window.draw_area, &navi_allocation);
	// �\���摜�̃s�N�Z���f�[�^��1�s���̃o�C�g�����v�Z
	stride = navi_allocation.width * 4;

	// �i�r�Q�[�V�����̃T�C�Y���ύX����Ă�����
		// �s�N�Z���f�[�^�̃��������Ċm��
	if(app->navigation_window.width != navi_allocation.width
		|| app->navigation_window.height != navi_allocation.height)
	{
		app->navigation_window.pixels = (uint8*)MEM_REALLOC_FUNC(
			app->navigation_window.pixels, navi_allocation.height*stride);
		app->navigation_window.reverse_buff = (uint8*)MEM_REALLOC_FUNC(
			app->navigation_window.reverse_buff, stride);
		app->navigation_window.width = navi_allocation.width;
		app->navigation_window.height = navi_allocation.height;

		// �i�r�Q�[�V�������w�肵�Ă��镝
		app->navigation_window.point.width =
			(gdouble)draw_allocation.width / window->disp_size;
		if(app->navigation_window.point.width > 1.0)
		{
			app->navigation_window.point.width = 1.0;
		}
		// �i�r�Q�[�V�������w�肵�Ă��鍂��
		app->navigation_window.point.height =
			(gdouble)draw_allocation.height / window->disp_size;
		if(app->navigation_window.point.height > 1.0)
		{
			app->navigation_window.point.height = 1.0;
		}

		// �`��̈�̕��E�������L��
		app->navigation_window.draw_width = draw_allocation.width;
		app->navigation_window.draw_height = draw_allocation.height;

		// �\�����W�␳�p�̒l���v�Z
		app->navigation_window.point.ratio_x =
			(1.0 / (window->disp_size))
				* navi_allocation.width;
		app->navigation_window.point.ratio_y =
			(1.0 / (window->disp_size))
				* navi_allocation.height;

		// ��]�p�x���L��
		app->navigation_window.point.angle = window->angle;
		// �g��k�������L��
		app->navigation_window.point.zoom = window->zoom;

		// �\���p�^�[�����X�V
		ChangeNavigationDrawWindow(&app->navigation_window, window);
	}	// �i�r�Q�[�V�����̃T�C�Y���ύX����Ă�����
			// �s�N�Z���f�[�^�̃��������Ċm��
		// if(app->navigation_window.width != navi_allocation.width
			// || app->navigation_window.height != navi_allocation.height)
	else if(app->navigation_window.point.zoom != window->zoom
		|| app->navigation_window.point.angle != window->angle
		|| app->navigation_window.draw_width != draw_allocation.width
			|| app->navigation_window.draw_height != draw_allocation.height)
	{	// �g��k�����܂��̓E�B���h�E�T�C�Y���ύX����Ă�����
			// �i�r�Q�[�V�������w�肵�Ă��镝
		app->navigation_window.point.width =
			(gdouble)draw_allocation.width / ((gdouble)window->disp_size);
		if(app->navigation_window.point.width > 1.0)
		{
			app->navigation_window.point.width = 1.0;
		}
		// �i�r�Q�[�V�������w�肵�Ă��鍂��
		app->navigation_window.point.height =
			(gdouble)draw_allocation.height / ((gdouble)window->disp_size);
		if(app->navigation_window.point.height > 1.0)
		{
			app->navigation_window.point.height = 1.0;
		}

		// �`��̈�̕��E�������L��
		app->navigation_window.draw_width = draw_allocation.width;
		app->navigation_window.draw_height = draw_allocation.height;

		// �\�����W�␳�p�̒l���v�Z
		app->navigation_window.point.ratio_x =
			(1.0 / window->disp_size) * navi_allocation.width;
		app->navigation_window.point.ratio_y =
			(1.0 / window->disp_size) * navi_allocation.height;

		// ��]�p�x���L��
		app->navigation_window.point.angle = window->angle;
		// �g��k�������L��
		app->navigation_window.point.zoom = window->zoom;

		// �i�r�Q�[�V�����̃p�^�[������]
		ChangeNavigationRotate(&app->navigation_window, window);
	}

	// �摜���g��k�����ăi�r�Q�[�V�����̉摜��
#if GTK_MAJOR_VERSION <= 2
	//cairo_p = kaburagi_cairo_create((struct _GdkWindow*)app->navigation_window.draw_area->window);
	cairo_p = gdk_cairo_create(app->navigation_window.draw_area->window);
#endif
	cairo_set_operator(cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cairo_p, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_set_source(cairo_p, app->navigation_window.pattern);
	cairo_paint(cairo_p);
	cairo_set_operator(cairo_p, CAIRO_OPERATOR_OVER);

	// ���E���]�\�����Ȃ�\�����e�����E���]
	if((window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) != 0)
	{
		uint8 *ref, *src;
		int x, y;

		for(y=0; y<navi_allocation.height; y++)
		{
			ref = app->navigation_window.reverse_buff;
			src = &app->navigation_window.pixels[(y+1)*stride-4];

			for(x=0; x<navi_allocation.width; x++, ref += 4, src -= 4)
			{
				ref[0] = src[0], ref[1] = src[1], ref[2] = src[2], ref[3] = src[3];
			}
			(void)memcpy(src+4, app->navigation_window.reverse_buff, stride);
		}
	}

	// �`��̈悪��ʂɎ��܂�؂�Ȃ��Ƃ���
		// ���݂̕\���ʒu��\������
	if(draw_allocation.width <= window->disp_size || draw_allocation.height <= window->disp_size)
	{	// �\�����W
		gdouble x, y, width, height;

		// �X�N���[���o�[�̈ʒu����\�����W���v�Z
		x = gtk_range_get_value(GTK_RANGE(gtk_scrolled_window_get_hscrollbar(
			GTK_SCROLLED_WINDOW(window->scroll))));
		app->navigation_window.point.x = x;
		x *= app->navigation_window.point.ratio_x;
		width = app->navigation_window.point.width * navi_allocation.width;
		if(x + width > navi_allocation.width)
		{
			width = navi_allocation.width - x;
		}

		y = gtk_range_get_value(GTK_RANGE(gtk_scrolled_window_get_vscrollbar(
			GTK_SCROLLED_WINDOW(window->scroll))));
		app->navigation_window.point.y = y;
		y *= app->navigation_window.point.ratio_y;
		height = app->navigation_window.point.height * navi_allocation.height;
		if(y + height > navi_allocation.height)
		{
			height = navi_allocation.height - y;
		}

#if 0 // defined(DISPLAY_BGR) && DISPLAY_BGR != 0
		cairo_set_source_rgb(cairo_p, 0, 0, 1);
#else
		cairo_set_source_rgb(cairo_p, 1, 0, 0);
#endif

		cairo_rectangle(cairo_p, x, y, width, height);
		cairo_stroke_preserve(cairo_p);
		// �\�����Ă��Ȃ��͈͈͂Â�����
		cairo_set_source_rgba(cairo_p, 0, 0, 0, 0.5);
		cairo_rectangle(cairo_p, 0, 0,
			navi_allocation.width, navi_allocation.height);
		cairo_fill(cairo_p);
	}	// �`��̈悪��ʂɎ��܂�؂�Ȃ��Ƃ���
			// ���݂̕\���ʒu��\������
		// if(draw_allocation.width <= app->draw_window[app->active_window]->width * app->draw_window[app->active_window]->zoom * 0.01
			// || draw_allocation.height <= app->draw_window[app->active_window]->height * app->draw_window[app->active_window]->zoom * 0.01)

#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return TRUE;
}

/*********************************************************
* NavigationButtonPressCallBack�֐�                      *
* �i�r�Q�[�V�����E�B���h�E�ł̃N���b�N�ɑ΂��鏈��       *
* ����                                                   *
* widget	: �i�r�Q�[�V�����̕`��̈�                   *
* event		: �}�E�X�̏��                               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	���TRUE                                             *
*********************************************************/
static gboolean NavigationButtonPressCallBack(
	GtkWidget* widget,
	GdkEventButton* event,
	APPLICATION* app
)
{
	// �`��̈�̃X�N���[���o�[
	GtkWidget* scroll_bar;
	// �i�r�Q�[�V�������w�肵�Ă���͈͂�
		// ����̍��W
	gint x, y;
	// �i�r�Q�[�V�������w�肵�Ă���͈͂̕��E����
	gdouble width, height;
	// �\�����Ă���`��̈�
	DRAW_WINDOW *window;
	// �i�r�Q�[�V�����̍��W�w��p�̃T�C�Y
	gdouble size;

	// �L�����o�X�̐���0�Ȃ�I��
	if(app->window_num == 0)
	{
		return TRUE;
	}

	window = GetActiveDrawWindow(app);
	size = window->disp_size;

	// �i�r�Q�[�V�������w�肵�Ă���͈͂̕��E�������v�Z
	width = app->navigation_window.point.width * app->navigation_window.width;
	height = app->navigation_window.point.height * app->navigation_window.height;

	// �}�E�X�̍��W���擾
	x = (gint)event->x,	y = (gint)event->y;

	// �N���b�N���ꂽ���W���w��͈͂̒��S�ɂ���
	x = (gint)((x - (app->navigation_window.point.width * app->navigation_window.width) * 0.5)
		* (gdouble)size / (gdouble)app->navigation_window.width);
	y = (gint)((y - (app->navigation_window.point.height * app->navigation_window.height) * 0.5)
		* (gdouble)size / (gdouble)app->navigation_window.height);

	// �O�ɂ͂ݏo����C��
	if(x < 0)
	{
		x = 0;
	}
	else if(x + width > size)
	{
		x = (gint)(size - width);
	}

	if(y < 0)
	{
		y = 0;
	}
	else if(y + height > size)
	{
		y = (gint)(size - height);
	}

	// �`��̈�̃X�N���[���ɍ��W��ݒ�
	scroll_bar = gtk_scrolled_window_get_hscrollbar(
		GTK_SCROLLED_WINDOW(app->draw_window[app->active_window]->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar), x);
	scroll_bar = gtk_scrolled_window_get_vscrollbar(
		GTK_SCROLLED_WINDOW(app->draw_window[app->active_window]->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar), y);

	return TRUE;
}

/*********************************************************
* NavigationMotionNotifyCallBack�֐�                     *
* �i�r�Q�[�V�����E�B���h�E�ł̃N���b�N�ɑ΂��鏈��       *
* ����                                                   *
* widget	: �i�r�Q�[�V�����̕`��̈�                   *
* event		: �}�E�X�̏��                               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	���TRUE                                             *
*********************************************************/
static gboolean NavigationMotionNotifyCallBack(
	GtkWidget* widget,
	GdkEventMotion* event,
	APPLICATION* app
)
{
	// �`��̈�̃X�N���[���o�[
	GtkWidget* scroll_bar;
	// �i�r�Q�[�V�������w�肵�Ă���͈͂�
		// ����̍��W
	gint x, y;
	// �}�E�X�̏��
	GdkModifierType state;
	// �i�r�Q�[�V�������w�肵�Ă���͈͂̕��E����
	gdouble width, height;
	// �\�����Ă���`��̈�
	DRAW_WINDOW *window;
	// �i�r�Q�[�V�����̍��W�w��p�̃T�C�Y
	gdouble size;

	// �}�E�X�̍��W���擾
	if(event->is_hint != 0)
	{
		gdk_window_get_pointer(event->window, &x, &y, &state);
	}
	else
	{
		x = (gint)event->x,	y = (gint)event->y;
		state = event->state;
	}

	// �L�����o�X�̐���0�܂��͍��N���b�N����Ă��Ȃ���ΏI��
	if(app->window_num == 0 || (state & GDK_BUTTON1_MASK) == 0)
	{
		return TRUE;
	}

	window = GetActiveDrawWindow(app);
	size = window->disp_size;

	// �i�r�Q�[�V�������w�肵�Ă���͈͂̕��E�������v�Z
	width = app->navigation_window.point.width * app->navigation_window.width;
	height = app->navigation_window.point.height * app->navigation_window.height;

	// �N���b�N���ꂽ���W���w��͈͂̒��S�ɂ���
	x = (gint)((x - (app->navigation_window.point.width * app->navigation_window.width) * 0.5)
		* (gdouble)size / (gdouble)app->navigation_window.width);
	y = (gint)((y - (app->navigation_window.point.height * app->navigation_window.height) * 0.5)
		* (gdouble)size / (gdouble)app->navigation_window.height);

	// �O�ɂ͂ݏo����C��
	if(x < 0)
	{
		x = 0;
	}
	else if(x + width > app->draw_window[app->active_window]->disp_size)
	{
		x = (gint)(app->draw_window[app->active_window]->disp_size - width);
	}

	if(y < 0)
	{
		y = 0;
	}
	else if(y + height > app->draw_window[app->active_window]->disp_size)
	{
		y = (gint)(app->draw_window[app->active_window]->disp_size - height);
	}

	// �`��̈�̃X�N���[���ɍ��W��ݒ�
	scroll_bar = gtk_scrolled_window_get_hscrollbar(
		GTK_SCROLLED_WINDOW(app->draw_window[app->active_window]->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar), x);
	scroll_bar = gtk_scrolled_window_get_vscrollbar(
		GTK_SCROLLED_WINDOW(app->draw_window[app->active_window]->scroll));
	gtk_range_set_value(GTK_RANGE(scroll_bar), y);

	// ��ʍX�V
	gtk_widget_queue_draw(app->navigation_window.draw_area);

	return TRUE;
}

/***************************************************************************
* ChangeNavigationZoomSlider�֐�                                           *
* �i�r�Q�[�V�����E�B���h�E�̊g��k�����ύX�X���C�_���쎞�̃R�[���o�b�N�֐� *
* ����                                                                     *
* slider	: �i�r�Q�[�V�����E�B���h�E�̃X���C�_�p�̃A�W���X�^             *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X                   *
***************************************************************************/
static void ChangeNavigationZoomSlider(GtkAdjustment* slider, APPLICATION* app)
{
	DRAW_WINDOW *window;
	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);
	// �A�N�e�B�u�ȕ`��̈�Ɋg��k������ݒ�
	window->zoom = (uint16)gtk_adjustment_get_value(slider);

	DrawWindowChangeZoom(window, window->zoom);
}

/*************************************************************************
* ChangeNavigationRotateSlider�֐�                                       *
* �i�r�Q�[�V�����E�B���h�E�̉�]�p�x�ύX�X���C�_���쎞�̃R�[���o�b�N�֐� *
* ����                                                                   *
* slider	: �i�r�Q�[�V�����E�B���h�E�̃X���C�_�p�̃A�W���X�^           *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X                 *
*************************************************************************/
static void ChangeNavigationRotateSlider(GtkAdjustment* slider, APPLICATION* app)
{
	DRAW_WINDOW *window;
	FLOAT_T angle;
	cairo_matrix_t matrix;

	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);

	// �A�N�e�B�u�ȕ`��̈�ɉ�]�p�x��ݒ�
	angle = window->angle =
		- gtk_adjustment_get_value(slider) * G_PI / 180;

	// ��]�p��ݒ�
	window->sin_value = sin(angle);
	window->cos_value = cos(angle);

	window->trans_x = - window->half_size + ((window->disp_layer->width / 2) * window->cos_value + (window->disp_layer->height / 2) * window->sin_value);
	window->trans_y = - window->half_size - ((window->disp_layer->width / 2) * window->sin_value - (window->disp_layer->height / 2) * window->cos_value);
	
	window->add_cursor_x = - (window->half_size - window->disp_layer->width / 2) + window->half_size;
	window->add_cursor_y = - (window->half_size - window->disp_layer->height / 2) + window->half_size;

	cairo_matrix_init_rotate(&matrix, angle);
	cairo_matrix_translate(&matrix, window->trans_x, window->trans_y);
	cairo_pattern_set_matrix(window->rotate, &matrix);

	UpdateDrawWindowClippingArea(window);

	gtk_widget_queue_draw(app->navigation_window.draw_area);
}

/*************************************************************
* OnDeleteNavigationWindow�֐�                               *
* �i�r�Q�[�V�����E�B���h�E���폜�����Ƃ��ɌĂяo�����     *
* ����                                                       *
* window		: �i�r�Q�[�V�����E�B���h�E�E�B�W�F�b�g       *
* event_info	: �폜�C�x���g�̏��                         *
* app			: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                     *
*	���FALSE(�E�B���h�E�폜)                                *
*************************************************************/
static gboolean OnDeleteNavigationWindow(GtkWidget* window, GdkEvent* event_info, APPLICATION* app)
{
	gint x, y;
	gint width, height;

	gtk_window_get_position(GTK_WINDOW(window), &x, &y);
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);

	app->navigation_window.window_x = x, app->navigation_window.window_y = y;
	app->navigation_window.window_width = width, app->navigation_window.window_height = height;

	return FALSE;
}

/*************************************************************
* OnDestroyNavigationWidget�֐�                              *
* �i�r�Q�[�V�����̃E�B�W�F�b�g���폜�����Ƃ��ɌĂяo����� *
* ����                                                       *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X         *
*************************************************************/
static void OnDestroyNavigationWidget(APPLICATION* app)
{
	MEM_FREE_FUNC(app->navigation_window.pixels);
	app->navigation_window.pixels = NULL;
	MEM_FREE_FUNC(app->navigation_window.reverse_buff);
	app->navigation_window.reverse_buff = NULL;

	app->menus.disable_if_no_open[app->navigation_window.zoom_index] = NULL;
	app->menus.disable_if_no_open[app->navigation_window.rotate_index] = NULL;

	app->navigation_window.window = NULL;
	app->navigation_window.draw_area = NULL;
	app->navigation_window.vbox = NULL;
	app->widgets->navi_layer_pane = NULL;
	app->navigation_window.width = 0;
	app->navigation_window.height = 0;
}

/*********************************************************************
* InitializeNavigation�֐�                                           *
* �i�r�Q�[�V�����E�B���h�E��������                                   *
* ����                                                               *
* navigation	: �i�r�Q�[�V�����E�B���h�E���Ǘ�����\���̂̃A�h���X *
* app			: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X         *
* box			: �h�b�L���O����ꍇ�̓{�b�N�X�E�B�W�F�b�g���w��     *
*********************************************************************/
void InitializeNavigation(
	NAVIGATION_WINDOW* navigation,
	APPLICATION *app,
	GtkWidget* box
)
{
	// �E�B�W�F�b�g����p
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	// �{�^���E�B�W�F�b�g
	GtkWidget *button;

	if(box == NULL)
	{
		// �E�B���h�E���쐬
		navigation->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		// �E�B���h�E�̈ʒu�ƃT�C�Y��ݒ�
		gtk_window_move(GTK_WINDOW(navigation->window), navigation->window_x, navigation->window_y);
		gtk_window_resize(GTK_WINDOW(navigation->window),
			navigation->window_width, navigation->window_height);
		// ����{�^���݂̂̃E�B���h�E��
		gtk_window_set_type_hint(GTK_WINDOW(navigation->window), GDK_WINDOW_TYPE_HINT_UTILITY);
		// �T�C�Y���w��
		gtk_widget_set_size_request(navigation->window, 150, 150);
		// �E�B���h�E�^�C�g�����Z�b�g
		gtk_window_set_title(GTK_WINDOW(navigation->window),
			app->labels->navigation.title);
		// �e�E�B���h�E��o�^
		gtk_window_set_transient_for(
			GTK_WINDOW(navigation->window), GTK_WINDOW(app->widgets->window));
		// �^�X�N�o�[�ɂ͕\�����Ȃ�
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(navigation->window), TRUE);
		// �V���[�g�J�b�g�L�[��o�^
		gtk_window_add_accel_group(GTK_WINDOW(navigation->window), app->widgets->hot_key);
		// �E�B���h�E���폜����Ƃ��ɃC�x���g��ݒ�
		(void)g_signal_connect(G_OBJECT(navigation->window), "delete_event",
			G_CALLBACK(OnDeleteNavigationWindow), app);

		// �L�[�{�[�h�̃R�[���o�b�N�֐����Z�b�g
		(void)g_signal_connect(G_OBJECT(navigation->window), "key-press-event",
			G_CALLBACK(KeyPressEvent), app);
		(void)g_signal_connect(G_OBJECT(navigation->window), "key-release-event",
			G_CALLBACK(KeyPressEvent), app);
	}
	else
	{
		navigation->window = NULL;
		gtk_paned_add1(GTK_PANED(box), vbox);
	}
	// �E�B�W�F�b�g�폜���̃R�[���o�b�N�֐����Z�b�g
	(void)g_signal_connect_swapped(G_OBJECT(vbox), "destroy",
		G_CALLBACK(OnDestroyNavigationWidget), app);

	// �摜�̕\���̈���쐬
	navigation->draw_area = gtk_drawing_area_new();
	// �{�b�N�X�ɒǉ�
	gtk_box_pack_start(GTK_BOX(vbox), navigation->draw_area, TRUE, TRUE, 0);

	// �`��p�̃R�[���o�b�N�֐����Z�b�g
#if GTK_MAJOR_VERSION <= 2
	(void)g_signal_connect(G_OBJECT(navigation->draw_area), "expose_event",
		G_CALLBACK(DisplayNavigation), app);
#else
	(void)g_signal_connect(G_OBJECT(navigation->draw_area), "draw",
		G_CALLBACK(DisplayNavigation), app);
#endif
	// �}�E�X�N���b�N�̃R�[���o�b�N�֐����Z�b�g
	(void)g_signal_connect(G_OBJECT(navigation->draw_area), "button_press_event",
		G_CALLBACK(NavigationButtonPressCallBack), app);
	// �}�E�X�I�[�o�[�̃R�[���o�b�N�֐����Z�b�g
	(void)g_signal_connect(G_OBJECT(navigation->draw_area), "motion_notify_event",
		G_CALLBACK(NavigationMotionNotifyCallBack), app);

	// �C�x���g�̎�ނ��Z�b�g
	gtk_widget_add_events(navigation->draw_area,
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_HINT_MASK |
		GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);

	// �g��k�����ύX�A��]�p�x�ύX�p�̃X���C�_��ǉ�
	{
		GtkWidget *scale;
		GtkAdjustment *adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(100, MIN_ZOOM, MAX_ZOOM, 1, 10, 0));

		navigation->zoom_slider = adjustment;

		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeNavigationZoomSlider), app);
		scale = SpinScaleNew(adjustment, app->labels->menu.zoom, 0);
		gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, TRUE, 2);

		if((app->flags & APPLICATION_INITIALIZED) == 0)
		{
			navigation->zoom_index = app->menus.num_disable_if_no_open;
			app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = scale;
			gtk_widget_set_sensitive(scale, FALSE);
			app->menus.num_disable_if_no_open++;
		}
		else
		{
			app->menus.disable_if_no_open[navigation->zoom_index] = scale;
			gtk_widget_set_sensitive(scale, app->window_num > 0);
		}

		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0, -180, 180, 1, 15, 0));

		navigation->rotate_slider = adjustment;

		(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
			G_CALLBACK(ChangeNavigationRotateSlider), app);
		scale = SpinScaleNew(adjustment, app->labels->menu.rotate, 0);
		gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, TRUE, 2);

		if((app->flags & APPLICATION_INITIALIZED) == 0)
		{
			navigation->rotate_index = app->menus.num_disable_if_no_open;
			app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = scale;
			gtk_widget_set_sensitive(scale, FALSE);
			app->menus.num_disable_if_no_open++;
		}
		else
		{
			app->menus.disable_if_no_open[navigation->rotate_index] = scale;
			gtk_widget_set_sensitive(scale, app->window_num > 0);
		}
	}

	// ���{�\���{�^����ǉ�
	button = gtk_button_new_with_label(app->labels->menu.zoom_reset);
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteZoomReset), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	// ��]�p�x���Z�b�g�{�^����ǉ�
	button = gtk_button_new_with_label(app->labels->menu.reset_rotate);
	(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
		G_CALLBACK(ExecuteRotateReset), app);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	if(navigation->window != NULL)
	{
		gtk_container_add(GTK_CONTAINER(navigation->window), vbox);
		// �E�B���h�E�\��
		gtk_widget_show_all(navigation->window);
	}

	navigation->vbox = vbox;
}

#ifdef __cplusplus
}
#endif
