#include <string.h>
#include "../../memory.h"
#include "gtk_widgets.h"
#include "../../application.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************
* CreateMainWindowWidgets�֐�                            *
* �A�v���P�[�V�����̃��C���E�B���h�E���쐬����           *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	�쐬�������C���E�B���h�E�E�B�W�F�b�g�f�[�^�̃A�h���X *
*********************************************************/
MAIN_WINDOW_WIDGETS_PTR CreateMainWindowWidgets(APPLICATION* app)
{
	MAIN_WINDOW_WIDGETS_PTR ret = (MAIN_WINDOW_WIDGETS_PTR)MEM_ALLOC_FUNC(sizeof(*ret));

	(void)memset(ret, 0, sizeof(*ret));

	return ret;
}

/*************************************************************************
* GetWriteMainWindowData�֐�                                             *
* �������t�@�C���ɏ����o�����C���E�B���h�E�E�B�W�F�b�g�̃f�[�^���擾���� *
* ����                                                                   *
* write_data	: �������t�@�C���ւ̏����o����p�f�[�^�̃A�h���X         *
* app			: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X         *
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
