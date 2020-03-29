#ifdef _MSC_VER
# pragma comment(lib, "mscms.lib")
#endif

#define USE_LCMS2

#ifdef _WIN32
# define INCLUDE_WIN_DEFAULT_API
# include <windows.h>
#endif

#include "application.h"
#include "iccbutton.h"
#include "types.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************
* RGB2GrayScaleFilter�֐�                    *
* RGB����O���[�X�P�[���֕ϊ�����            *
* ����                                       *
* source		:                            *
* destination	: �ϊ���̃f�[�^�i�[��       *
* num_pixel		: �s�N�Z����                 *
* filter_data	: �t�B���^�[�Ŏg�p����f�[�^ *
*********************************************/
void RGB2GrayScaleFilter(uint8* source, uint8* destination, int num_pixel, void* filter_data)
{
	int i;
	uint8 gray_value;
	uint8 *src = source, *dst = destination;

	for(i=0; i<num_pixel; i++)
	{
		gray_value = (src[0] + src[1] + src[2]) / 3;
		dst[0] = dst[1] = dst[2] = gray_value;
	}
}

/*********************************************
* RGBA2GrayScaleFilter�֐�                   *
* RGBA����O���[�X�P�[���֕ϊ�����           *
* ����                                       *
* source		:                            *
* destination	: �ϊ���̃f�[�^�i�[��       *
* num_pixel		: �s�N�Z����                 *
* filter_data	: �t�B���^�[�Ŏg�p����f�[�^ *
*********************************************/
void RGBA2GrayScaleFilter(uint8* source, uint8* destination, int num_pixel, void* filter_data)
{
	int i;
	uint8 gray_value;
	uint8 *src = source, *dst = destination;

	for(i=0; i<num_pixel; i++, src+=4, dst+=4)
	{
		gray_value = (src[0] + src[1] + src[2]) / 3;
		dst[0] = dst[1] = dst[2] = gray_value;
		dst[3] = src[3];
	}
}

/***************************************************
* RGBA2GrayScaleFilterYIQ�֐�                      *
* RGBA����O���[�X�P�[���֕ϊ�����(YIQ�J���[���f�� *
* ����                                             *
* source		:                                  *
* destination	: �ϊ���̃f�[�^�i�[��             *
* num_pixel		: �s�N�Z����                       *
* filter_data	: �t�B���^�[�Ŏg�p����f�[�^       *
***************************************************/
void RGBA2GrayScaleFilterYIQ(uint8* source, uint8* destination, int num_pixel, void* filter_data)
{
	int i;
	uint8 gray_value;
	uint8 *src = source, *dst = destination;

	for(i=0; i<num_pixel; i++, src+=4, dst+=4)
	{
		gray_value = (76 * src[0] + 151 * src[1] + 29 * src[2]) >> 8;
		dst[0] = dst[1] = dst[2] = gray_value;
		dst[3] = src[3];
	}
}

/*****************************************************
* NoDisplayFilter�֐�                                *
* �\�����̃t�B���^�[���I�t                           *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void NoDisplayFilter(GtkWidget* menu, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) == 0 && app->draw_window[app->active_window] &&
		gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) != FALSE)
	{
		app->draw_window[app->active_window]->display_filter_mode
			= DISPLAY_FUNC_TYPE_NO_CONVERT;
		app->display_filter.filter_func = NULL;

		app->tool_window.color_chooser->filter_func = NULL;
		gtk_widget_queue_draw(app->tool_window.color_chooser->widget);
		UpdateColorBox(app->tool_window.color_chooser);
		gtk_widget_queue_draw(app->tool_window.color_chooser->pallete_widget);

		app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		app->flags &= ~(APPLICATION_DISPLAY_GRAY_SCALE | APPLICATION_DISPLAY_SOFT_PROOF);
		gtk_widget_queue_draw(app->draw_window[app->active_window]->window);
	}
}

/*****************************************************
* GrayScaleDisplayFilter�֐�                         *
* �\�����̃t�B���^�[���O���[�X�P�[���ϊ��̂��̂�     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void GrayScaleDisplayFilter(GtkWidget* menu, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) == 0 && app->draw_window[app->active_window] &&
		gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) != FALSE)
	{
		app->draw_window[app->active_window]->display_filter_mode
			= DISPLAY_FUNC_TYPE_GRAY_SCALE;
		app->display_filter.filter_func = RGBA2GrayScaleFilter;

		app->tool_window.color_chooser->filter_func = RGBA2GrayScaleFilter;
		gtk_widget_queue_draw(app->tool_window.color_chooser->widget);
		UpdateColorBox(app->tool_window.color_chooser);
		gtk_widget_queue_draw(app->tool_window.color_chooser->pallete_widget);

		app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		app->flags |= APPLICATION_DISPLAY_GRAY_SCALE;
		app->flags &= ~(APPLICATION_DISPLAY_SOFT_PROOF);
		gtk_widget_queue_draw(app->draw_window[app->active_window]->window);
	}
}

/******************************************************************
* GrayScaleDisplayFilterYIQ�֐�                                   *
* �\�����̃t�B���^�[���O���[�X�P�[���ϊ�(YIQ�J���[���f��)�̂��̂� *
* ����                                                            *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X              *
******************************************************************/
void GrayScaleDisplayFilterYIQ(GtkWidget* menu, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) == 0 && app->draw_window[app->active_window] &&
		gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) != FALSE)
	{
		app->draw_window[app->active_window]->display_filter_mode
			= DISPLAY_FUNC_TYPE_GRAY_SCALE_YIQ;
		app->display_filter.filter_func = RGBA2GrayScaleFilterYIQ;

		app->tool_window.color_chooser->filter_func = RGBA2GrayScaleFilterYIQ;
		gtk_widget_queue_draw(app->tool_window.color_chooser->widget);
		UpdateColorBox(app->tool_window.color_chooser);
		gtk_widget_queue_draw(app->tool_window.color_chooser->pallete_widget);

		app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		app->flags |= APPLICATION_DISPLAY_GRAY_SCALE;
		app->flags &= ~(APPLICATION_DISPLAY_SOFT_PROOF);
		gtk_widget_queue_draw(app->draw_window[app->active_window]->window);
	}
}

void AdaptIccProfileDisplayFilter(uint8* source, uint8* destination, int num_pixel, void* filter_data)
{
	APPLICATION *app = (APPLICATION*)filter_data;
	DRAW_WINDOW *window = app->draw_window[app->active_window];

	if(window != NULL && window->icc_transform != NULL)
	{
		cmsDoTransform(window->icc_transform, source, destination, num_pixel);
	}
	else
	{
		cmsDoTransform(app->icc_transform, source, destination, num_pixel);
	}
}

/*****************************************************
* IccProfileDisplayFilter�֐�                        *
* �\�����̃t�B���^�[��ICC�v���t�@�C���K�p�̂��̂�    *
* ����                                               *
* menu	: ���j���[�E�B�W�F�b�g                       *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void IccProfileDisplayFilter(GtkWidget* menu, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) == 0 && app->draw_window[app->active_window] &&
		gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) != FALSE)
	{
		app->draw_window[app->active_window]->display_filter_mode
			= DISPLAY_FUNC_TYPE_ICC_PROFILE;
		app->display_filter.filter_func = AdaptIccProfileDisplayFilter;
		app->display_filter.filter_data = (void*)app;

		app->tool_window.color_chooser->filter_func = AdaptIccProfileDisplayFilter;
		app->tool_window.color_chooser->filter_data = (void*)app;
		gtk_widget_queue_draw(app->tool_window.color_chooser->widget);
		UpdateColorBox(app->tool_window.color_chooser);
		gtk_widget_queue_draw(app->tool_window.color_chooser->pallete_widget);

		app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		app->flags |= APPLICATION_DISPLAY_SOFT_PROOF;
		app->flags &= ~(APPLICATION_DISPLAY_GRAY_SCALE);
		gtk_widget_queue_draw(app->draw_window[app->active_window]->window);
	}
}

void (*g_display_filter_funcs[])(uint8* source, uint8* destination, int num_pixel, void* filter_data) =
{
	NULL,
	RGBA2GrayScaleFilter,
	RGBA2GrayScaleFilterYIQ,
	AdaptIccProfileDisplayFilter
};

#ifdef __cplusplus
}
#endif
