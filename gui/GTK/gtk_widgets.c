#include <string.h>
#include "../../memory.h"
#include "gtk_widgets.h"
#include "../../application.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************
* CreateMainWindowWidgets関数                            *
* アプリケーションのメインウィンドウを作成する           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
* 返り値                                                 *
*	作成したメインウィンドウウィジェットデータのアドレス *
*********************************************************/
MAIN_WINDOW_WIDGETS_PTR CreateMainWindowWidgets(APPLICATION* app)
{
	MAIN_WINDOW_WIDGETS_PTR ret = (MAIN_WINDOW_WIDGETS_PTR)MEM_ALLOC_FUNC(sizeof(*ret));

	(void)memset(ret, 0, sizeof(*ret));

	return ret;
}

/*************************************************************************
* GetWriteMainWindowData関数                                             *
* 初期化ファイルに書き出すメインウィンドウウィジェットのデータを取得する *
* 引数                                                                   *
* write_data	: 初期化ファイルへの書き出し専用データのアドレス         *
* app			: アプリケーション全体を管理する構造体のアドレス         *
*************************************************************************/
void GetWriteMainWindowData(WRITE_APPLICATIOIN_DATA* write_data, APPLICATION* app)
{
	if((app->flags & (APPLICATION_FULL_SCREEN | APPLICATION_WINDOW_MAXIMIZE)) == 0)
	{
		gtk_window_get_position(GTK_WINDOW(app->widgets->window),
			&write_data->main_window_x, &write_data->main_window_y);
		gtk_window_get_size(GTK_WINDOW(app->widgets->window),
			&write_data->main_window_width, &write_data->main_window_height);
	}
	else
	{
		write_data->main_window_x = app->window_x,	write_data->main_window_y = app->window_y;
		write_data->main_window_width = app->window_width,	write_data->main_window_height = app->window_height;
	}

	if(gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane)) != NULL)
	{
		app->left_pane_position = gtk_paned_get_position(GTK_PANED(app->widgets->left_pane));
	}
	if(gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane)) != NULL)
	{
		app->right_pane_position = gtk_paned_get_position(GTK_PANED(app->widgets->right_pane));
	}

	if(app->widgets->navi_layer_pane != NULL)
	{
		app->layer_window.pane_position = gtk_paned_get_position(GTK_PANED(app->widgets->navi_layer_pane));
	}
}

#ifdef __cplusplus
}
#endif
