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
* RGB2GrayScaleFilter関数                    *
* RGBからグレースケールへ変換する            *
* 引数                                       *
* source		:                            *
* destination	: 変換後のデータ格納先       *
* num_pixel		: ピクセル数                 *
* filter_data	: フィルターで使用するデータ *
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
* RGBA2GrayScaleFilter関数                   *
* RGBAからグレースケールへ変換する           *
* 引数                                       *
* source		:                            *
* destination	: 変換後のデータ格納先       *
* num_pixel		: ピクセル数                 *
* filter_data	: フィルターで使用するデータ *
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
* RGBA2GrayScaleFilterYIQ関数                      *
* RGBAからグレースケールへ変換する(YIQカラーモデル *
* 引数                                             *
* source		:                                  *
* destination	: 変換後のデータ格納先             *
* num_pixel		: ピクセル数                       *
* filter_data	: フィルターで使用するデータ       *
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
* NoDisplayFilter関数                                *
* 表示時のフィルターをオフ                           *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
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
* GrayScaleDisplayFilter関数                         *
* 表示時のフィルターをグレースケール変換のものへ     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
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
* GrayScaleDisplayFilterYIQ関数                                   *
* 表示時のフィルターをグレースケール変換(YIQカラーモデル)のものへ *
* 引数                                                            *
* app	: アプリケーションを管理する構造体のアドレス              *
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
* IccProfileDisplayFilter関数                        *
* 表示時のフィルターをICCプロファイル適用のものへ    *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
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
