// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "application.h"
#include "draw_window.h"
#include "clip_board.h"
#include "ini_file.h"
#include "menu.h"
#include "display.h"
#include "memory.h"
#include "filter.h"
#include "transform.h"
#include "printer.h"
#include "plug_in.h"

#include "./gui/GTK/input_gtk.h"
#include "./gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

static void ExecuteNew(APPLICATION *app);
void Change2LoupeMode(APPLICATION* app);
/*********************************************************
* ExecuteDisplayReverseHorizontally�֐�                  *
* �\�������E���]                                         *
* ����                                                   *
* menu_item	: ���j���[�A�C�e���E�B�W�F�b�g               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteDisplayReverseHorizontally(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* ExecuteChangeDisplayPreview�֐�                        *
* �v���r���[�̕\��/��\����؂�ւ���                    *
* ����                                                   *
* menu_item	: �\���؂�ւ����j���[�A�C�e���E�B�W�F�b�g   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteChangeDisplayPreview(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* ExecuteSwitchFullScreen�֐�                            *
* �t���X�N���[���̐؂�ւ����s��                         *
* ����                                                   *
* menu_item	: �\���؂�ւ����j���[�A�C�e���E�B�W�F�b�g   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteSwitchFullScreen(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* ExecuteMoveToolWindowTopLeft�֐�                       *
* �c�[���{�b�N�X�E�B���h�E������Ɉړ�����               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteMoveToolWindowTopLeft(APPLICATION* app);

/*********************************************************
* ExecuteMoveLayerWindowTopLeft�֐�                      *
* ���C���[�E�B���h�E������Ɉړ�����                     *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteMoveLayerWindowTopLeft(APPLICATION* app);

/*********************************************************
* ExecuteMoveNavigationWindowTopLeft�֐�                 *
* �i�r�Q�[�V�����E�B���h�E������Ɉړ�����               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteMoveNavigationWindowTopLeft(APPLICATION* app);

/*************************************************
* ParentItemSelected�֐�                         *
* �e���j���[�A�C�e�����I�����ꂽ���ɌĂяo����� *
* ����                                           *
* widget	: �e���j���[�A�C�e��                 *
* data		: �_�~�[                             *
*************************************************/
void ParentItemSelected(GtkWidget* widget, gpointer* data)
{
	gtk_menu_item_select(GTK_MENU_ITEM(widget));
}

/*********************************************************
* OnDestroyMainMenu�֐�                                  *
* ���C�����j���[���폜���ꂽ�Ƃ��ɌĂяo�����           *
* ����                                                   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void OnDestroyMainMenu(APPLICATION* app)
{
	app->menus.disable_if_single_layer[app->layer_window.layer_control.delete_menu_index] = NULL;
	app->menus.disable_if_single_layer[app->layer_window.layer_control.merge_down_menu_index] = NULL;
	app->menus.disable_if_single_layer[app->layer_window.layer_control.flatten_menu_index] = NULL;
}

/*********************************************************
* GetMainMenu�֐�                                        *
* ���C�����j���[���쐬����                               *
* ����                                                   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* window	: ���C���E�B���h�E�̃E�B�W�F�b�g             *
* path		:                                            *
* �Ԃ�l                                                 *
*	���j���[�o�[�E�B�W�F�b�g                             *
*********************************************************/
GtkWidget* GetMainMenu(
	APPLICATION* app,
	GtkWidget* window,
	const char* path
)
{
	// ���j���[�o�[�A�T�u���j���[�ƃ��j���[�A�C�e��
	GtkWidget* menu_bar, *menu, *menu_item, *sub_menu;
	// ���W�I���j���[�̐擪�A�C�e��
	GtkWidget *radio_top;
	// �T�u���j���[����؂邽�߂̃Z�p���[�^
	GtkWidget* separator = gtk_separator_menu_item_new();
	// �V���[�g�J�b�g�L�[�p
	GtkAccelGroup* accel_group;
	// �\��������쐬�p�̃o�b�t�@
	char buff[2048];
	int i;	// for���p�̃J�E���^

	// ���j���[�o�[��蒼���ɔ����ăE�B�W�F�b�g�̐����L�����Ă���
	app->menus.menu_start_disable_if_no_open = app->menus.num_disable_if_no_open;
	app->menus.menu_start_disable_if_no_select = app->menus.num_disable_if_no_select;
	app->menus.menu_start_disable_if_single_layer = app->menus.num_disable_if_single_layer;
	app->menus.menu_start_disable_if_normal_layer = app->menus.num_disable_if_normal_layer;

	// �V���[�g�J�b�g�L�[�o�^�̏���
	if(app->widgets->hot_key == NULL)
	{
		app->widgets->hot_key = accel_group = gtk_accel_group_new();
	}
	else
	{
		accel_group = app->widgets->hot_key;
	}
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	// ���j���[�o�[���쐬
	menu_bar = gtk_menu_bar_new();
	// ���j���[�o�[�폜���̃R�[���o�b�N�֐����Z�b�g
	(void)g_signal_connect_swapped(G_OBJECT(menu_bar), "destroy",
		G_CALLBACK(OnDestroyMainMenu), app);

	// �u�t�@�C���v���j���[
	(void)sprintf(buff, "_%s(_F)", app->labels->menu.file);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'F', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// �u�t�@�C���v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u�V�K�쐬�v
	(void)sprintf(buff, "%s", app->labels->menu.make_new);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'N', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteNew), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�J���v
	(void)sprintf(buff, "%s", app->labels->menu.open);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'O', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteOpenFile), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u���C���[�Ƃ��ĊJ���v
	(void)sprintf(buff, "%s", app->labels->menu.open_as_layer);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'O', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteOpenFileAsLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u�㏑���ۑ��v
	(void)sprintf(buff, "%s", app->labels->menu.save);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'S', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSave), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���O��t���ĕۑ��v
	(void)sprintf(buff, "%s", app->labels->menu.save_as);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'S', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSaveAs), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u����v
	(void)sprintf(buff, "%s", "Print");
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecutePrint), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u����v
	(void)sprintf(buff, "%s", app->labels->menu.close);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'W', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteClose), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�I���v
	(void)sprintf(buff, "%s", app->labels->menu.quit);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
#if GTK_MAJOR_VERSION <= 2
		GDK_Escape, 0, GTK_ACCEL_VISIBLE);
#else
		GDK_KEY_Escape, 0, GTK_ACCEL_VISIBLE);
#endif
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(OnQuitApplication), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�ҏW�v���j���[
	(void)sprintf(buff, "_%s(_E)", app->labels->menu.edit);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'E', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// �u�ҏW�v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u���ɖ߂��v
	(void)sprintf(buff, "%s", app->labels->menu.undo);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'Z', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteUndo), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u��蒼���v
	(void)sprintf(buff, "%s", app->labels->menu.redo);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'Y', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteRedo), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u�R�s�[�v
	(void)sprintf(buff, "%s", app->labels->menu.copy);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'C', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteCopy), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// �u�������R�s�[�v
	(void)sprintf(buff, "%s", app->labels->menu.copy_visible);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'C', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteCopyVisible), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// �u�؂���v
	(void)sprintf(buff, "%s", app->labels->menu.cut);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'X', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteCut), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// �u�폜�v
	(void)sprintf(buff, "%s", app->labels->unit._delete);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		GDK_KEY_Delete, 0, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteDelete), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// �u�\��t���v
	(void)sprintf(buff, "%s", app->labels->menu.paste);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'V', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecutePaste), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u�ό`�v
	(void)sprintf(buff, "%s", app->labels->menu.transform);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'T', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteTransform), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�ˉe�ϊ��v
	(void)sprintf(buff, "%s", app->labels->menu.projection);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'T', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteProjection), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u���ݒ�v
	(void)sprintf(buff, "%s", app->labels->preference.title);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSetPreference), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�L�����o�X�v���j���[
	(void)sprintf(buff, "_%s(_C)", app->labels->menu.canvas);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'C', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�L�����o�X�v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u�𑜓x�̕ύX�v
	(void)sprintf(buff, "%s", app->labels->menu.change_resolution);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeResolution), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�L�����o�X�T�C�Y�̕ύX�v
	(void)sprintf(buff, "%s", app->labels->menu.change_canvas_size);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeCanvasSize), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�������]�v
	(void)sprintf(buff, "%s", app->labels->menu.flip_canvas_horizontally);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FlipImageHorizontally), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�������]�v
	(void)sprintf(buff, "%s", app->labels->menu.flip_canvas_vertically);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FlipImageVertically), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�w�i�F��؂�ւ��v
	(void)sprintf(buff, "%s", app->labels->menu.switch_bg_color);
	app->menus.change_back_ground_menu = 
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
			menu_item = gtk_check_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(SwitchSecondBackColor), app);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'B', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u2�߂̔w�i�F��ύX�v
	(void)sprintf(buff, "%s", app->labels->menu.change_2nd_bg_color);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(Change2ndBackColor), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�L�����o�X��ICC�v���t�@�C����ύX�v
	(void)sprintf(buff, "%s", app->labels->menu.canvas_icc);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeCanvasIccProfile), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���C���[�v���j���[
	(void)sprintf(buff, "_%s(_L)", app->labels->menu.layer);	
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'L', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���C���[�v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u�V�K�ʏ탌�C���[�v
	(void)sprintf(buff, "%s", app->labels->menu.new_color);	
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'N', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMakeColorLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�V�K�x�N�g�����C���[�v
	(void)sprintf(buff, "%s", app->labels->menu.new_vector);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'P', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMakeVectorLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�V�K���C���[�Z�b�g�v
	(void)sprintf(buff, "%s", app->labels->menu.new_layer_set);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'G', GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMakeLayerSet), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�V�K�������C���[�v
	(void)sprintf(buff, "%s", app->labels->menu.new_adjustment_layer);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMakeAdjustmentLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u3D���C���[�v
	if(GetHas3DLayer(app) != FALSE)
	{
		(void)sprintf(buff, "%s", app->labels->menu.new_3d_modeling);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
			menu_item = gtk_menu_item_new_with_mnemonic(buff);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteMake3DLayer), app);
		app->menus.num_disable_if_no_open++;
	}

	// �u���C���[�O���[�v�쐬�v
	(void)sprintf(buff, "%s", app->labels->menu.new_layer_group);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteLayerGroupTemplateWindow), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���C���[�𕡐��v
	(void)sprintf(buff, "%s", app->labels->menu.copy_layer);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteCopyLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���������C���[�Ɂv
	(void)sprintf(buff, "%s", app->labels->menu.visible2layer);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteVisible2Layer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u���C���[���폜�v
	(void)sprintf(buff, "%s", app->labels->menu.delete_layer);
	app->layer_window.layer_control.delete_menu_index = app->menus.num_disable_if_single_layer;
	app->menus.disable_if_single_layer[app->menus.num_disable_if_single_layer] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(DeleteActiveLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_single_layer++;

	// �u���̃��C���[�ƌ����v
	(void)sprintf(buff, "%s", app->labels->menu.merge_down_layer);
	app->layer_window.layer_control.merge_down_menu_index = app->menus.num_disable_if_single_layer;
	app->menus.merge_down_menu =
		app->menus.disable_if_single_layer[app->menus.num_disable_if_single_layer] =
			menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(MergeDownActiveLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_single_layer++;

	// �u�摜�𓝍��v
	(void)sprintf(buff, "%s", app->labels->menu.merge_all_layer);
	app->layer_window.layer_control.flatten_menu_index = app->menus.num_disable_if_single_layer;
	app->menus.disable_if_single_layer[app->menus.num_disable_if_single_layer] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FlattenImage), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_single_layer++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u�`��F�œh��ׂ��v
	(void)sprintf(buff, "%s", app->labels->menu.fill_layer_fg_color);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'F', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FillForeGroundColor), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�p�^�[���œh��ׂ��v
	(void)sprintf(buff, "%s", app->labels->menu.fill_layer_pattern);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(FillPattern), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���C���[�����X�^���C�Y�v
	(void)sprintf(buff, "%s", app->labels->menu.rasterize_layer);
	app->menus.disable_if_normal_layer[app->menus.num_disable_if_normal_layer] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(RasterizeActiveLayer), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_widget_set_sensitive(menu_item, FALSE);
	app->menus.num_disable_if_normal_layer++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u�s����������I��͈͂Ɂv
	(void)sprintf(buff, "%s", app->labels->layer_window.alpha_to_select);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ActiveLayerAlpha2SelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�s����������I��͈͂ɉ�����v
	(void)sprintf(buff, "%s", app->labels->layer_window.alpha_add_select);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ActiveLayerAlphaAddSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�I��͈́v���j���[
	(void)sprintf(buff, "_%s(_S)", app->labels->menu.select);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'S', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�I��͈́v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u�I���������v
	(void)sprintf(buff, "%s", app->labels->menu.select_none);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'D', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(UnSetSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// �u�I���𔽓]�v
	(void)sprintf(buff, "%s", app->labels->menu.select_invert);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'I', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(InvertSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u�I��͈͂��g��v
	(void)sprintf(buff, "%s", app->labels->menu.selection_extend);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExtendSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// �u�I��͈͂��k���v
	(void)sprintf(buff, "%s", app->labels->menu.selection_reduct);
	app->menus.disable_if_no_select[app->menus.num_disable_if_no_select] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ReductSelectionArea), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_select++;

	// �u�I��͈͂�ҏW�v
	(void)sprintf(buff, "%s", app->labels->window.edit_selection);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		app->menu_data.edit_selection = menu_item = gtk_check_menu_item_new_with_label(buff);
#if GTK_MAJOR_VERSION <= 2
	gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
#endif
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'Q', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ChangeEditSelectionMode), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//--------------------------------------------------------//
	separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	// �u�S�đI���v
	(void)sprintf(buff, "%s", app->labels->menu.select_all);	
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'A', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSelectAll), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�t�B���^�[�v���j���[
	(void)sprintf(buff, "_%s(_T)", app->labels->menu.filters);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);	
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'T', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�t�B���^�[�v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u���邳�R���g���X�g�v
	(void)sprintf(buff, "%s", app->labels->menu.bright_contrast);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeBrightContrastFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�F���E�ʓx�v
	(void)sprintf(buff, "%s", app->labels->menu.hue_saturtion);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeHueSaturationFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���x���␳�v
	(void)sprintf(buff, "%s", app->labels->menu.color_levels);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteColorLevelAdjust), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�g�[���J�[�u�v
	(void)sprintf(buff, "%s", app->labels->menu.tone_curve);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteToneCurve), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�ڂ����v
	(void)sprintf(buff, "%s", app->labels->menu.blur);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteBlurFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���[�V�����ڂ����v
	(void)sprintf(buff, "%s", app->labels->menu.motion_blur);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMotionBlurFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�K�E�V�A���ڂ����v
	(void)sprintf(buff, "%s", app->labels->menu.gaussian_blur);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteGaussianBlurFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�P�x�𓧖��x�Ɂv
	(void)sprintf(buff, "%s", app->labels->menu.luminosity2opacity);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteLuminosity2OpacityFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�P�x�𓧖��x�Ɂv
	(void)sprintf(buff, "%s", app->labels->menu.color2alpha);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteColor2AlphaFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���̃��C���[�Œ��F�v
	(void)sprintf(buff, "%s", app->labels->menu.colorize_with_under);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteColorizeWithUnderFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�O���f�[�V�����}�b�v�v
	(void)sprintf(buff, "%s", app->labels->menu.gradation_map);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteGradationMapFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�s�N�Z�����g���[�X�v
	(void)sprintf(buff, "%s", app->labels->menu.trace_pixels);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteTraceBitmap), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�x�N�g�����C���[�ŉ��̃��C���[��h��ׂ��v
	(void)sprintf(buff, "%s", app->labels->menu.fill_with_vector);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteFillWithVectorLineFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���h��v���j���[
	(void)sprintf(buff, "%s",app->labels->menu.render);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���h��v���j���[�̉��ɃT�u���j���[�쐬
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// �u�_�v
	(void)sprintf(buff, "%s", app->labels->menu.cloud);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecutePerlinNoiseFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�t���N�^���v
	(void)sprintf(buff, "%s", app->labels->menu.fractal);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteFractal), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//----------------------------------------------------------//

	// �u�r���[�v���j���[
	(void)sprintf(buff, "_%s(_V)", app->labels->menu.view);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'V', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�r���[�v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u�g��E�k���v���j���[
	(void)sprintf(buff, "%s", app->labels->menu.zoom);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�g��E�k���v���j���[�̉��ɃT�u���j���[�쐬
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// �u�g��v
	(void)sprintf(buff, "%s", app->labels->menu.zoom_in);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'+', 0, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteZoomIn), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�k���v
	(void)sprintf(buff, "%s", app->labels->menu.zoom_out);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'-', 0, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteZoomOut), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u���{�\���v
	(void)sprintf(buff, "%s", app->labels->menu.zoom_reset);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteZoomReset), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u��]�v
	(void)sprintf(buff, "%s", app->labels->menu.rotate);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_label(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u��]�v���j���[�̉��ɃT�u���j���[�쐬
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// �u���v���v
	(void)sprintf(buff, "%s", app->labels->tool_box.clockwise);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'R', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteRotateClockwise), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�����v���v
	(void)sprintf(buff, "%s", app->labels->tool_box.counter_clockwise);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'L', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteRotateCounterClockwise), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u��]�����Z�b�g�v
	(void)sprintf(buff, "%s", app->labels->menu.reset_rotate);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteRotateReset), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�\���𔽓]�v
	(void)sprintf(buff, "%s", app->labels->menu.reverse_horizontally);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		app->menu_data.reverse_horizontally = menu_item = gtk_check_menu_item_new_with_mnemonic(buff);
#if GTK_MAJOR_VERSION <= 2
	gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
#endif
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'H', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteDisplayReverseHorizontally), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�\���t�B���^�[�v
	(void)sprintf(buff, "%s", app->labels->menu.display_filter);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_label(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�\���t�B���^�[�v���j���[�̉��ɃT�u���j���[�쐬
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// �u�����v
	(void)sprintf(buff, "%s", app->labels->menu.no_filter);
	app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_NO_CONVERT] =
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		radio_top = gtk_radio_menu_item_new_with_mnemonic(NULL, buff);
	(void)g_signal_connect(G_OBJECT(radio_top), "activate",
		G_CALLBACK(NoDisplayFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), radio_top);
	app->menus.num_disable_if_no_open++;

	// �u�O���[�X�P�[���v
	(void)sprintf(buff, "%s", app->labels->menu.gray_scale);
	app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_GRAY_SCALE] =
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_radio_menu_item_new_with_mnemonic(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(radio_top)), buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(GrayScaleDisplayFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�O���[�X�P�[��(YIQ)�v
	(void)sprintf(buff, "%s", app->labels->menu.gray_scale_yiq);
	app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_GRAY_SCALE_YIQ] =
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_radio_menu_item_new_with_mnemonic(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(radio_top)), buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(GrayScaleDisplayFilterYIQ), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;
	
	// �uICC�v���t�@�C���v
	(void)sprintf(buff, "%s", "ICC PROFILE");
	app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_ICC_PROFILE] =
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_radio_menu_item_new_with_mnemonic(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(radio_top)), buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(IccProfileDisplayFilter), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	app->menus.num_disable_if_no_open++;

	//-----------------------------------------------------------//
	// �u�E�B���h�E�v���j���[
	(void)sprintf(buff, "_%s(_W)", app->labels->window.window);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'W', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// �u�E�B���h�E�v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u�c�[���{�b�N�X�v
	(void)sprintf(buff, "%s", app->labels->tool_box.title);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�c�[���{�b�N�X�v���j���[�̉��ɃT�u���j���[�쐬
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// �u�E�B���h�E�v
	(void)sprintf(buff, "%s", app->labels->window.window);
	app->tool_window.menu_item = menu_item = gtk_radio_menu_item_new_with_label(NULL, buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->tool_window.flags & TOOL_DOCKED) == 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_WINDOW));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeToolWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// ���ɔz�u
	(void)sprintf(buff, "%s", app->labels->window.place_left);
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->tool_window.flags & TOOL_DOCKED) != 0 && (app->tool_window.flags & TOOL_PLACE_RIGHT) == 0);
	(void)g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_LEFT));
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeToolWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// �E�ɔz�u
	(void)sprintf(buff, "%s", app->labels->window.place_right);
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->tool_window.flags & TOOL_DOCKED) != 0 && (app->tool_window.flags & TOOL_PLACE_RIGHT) != 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_RIGHT));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeToolWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	/*
	// ������|�b�v�A�b�v
	(void)sprintf(buff, "%s", "pop up left");
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->tool_window.flags & TOOL_DOCKED) != 0 && (app->tool_window.flags & TOOL_PLACE_RIGHT) != 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_POPUP_LEFT));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeToolWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// �E����|�b�v�A�b�v
	(void)sprintf(buff, "%s", "pop up left");
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->tool_window.flags & TOOL_DOCKED) != 0 && (app->tool_window.flags & TOOL_PLACE_RIGHT) != 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_POPUP_RIGHT));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeToolWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
	*/

	// �E�B���h�E������Ɉړ�
	(void)sprintf(buff, "%s", app->labels->window.move_top_left);
	menu_item = gtk_menu_item_new_with_label(buff);
	g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMoveToolWindowTopLeft), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// �u���C���[�r���[ & �i�r�Q�[�V�����v
	(void)sprintf(buff, "%s & %s", app->labels->layer_window.title, app->labels->navigation.title);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u���C���[�r���[ & �i�r�Q�[�V�����v���j���[�̉��ɃT�u���j���[�쐬
	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	// �u�E�B���h�E�v
	(void)sprintf(buff, "%s", app->labels->window.window);
	app->tool_window.menu_item = menu_item = gtk_radio_menu_item_new_with_label(NULL, buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->layer_window.flags & LAYER_WINDOW_DOCKED) == 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_WINDOW));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeNavigationLayerWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// ���ɔz�u
	(void)sprintf(buff, "%s", app->labels->window.place_left);
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->layer_window.flags & LAYER_WINDOW_DOCKED) != 0 && (app->layer_window.flags & LAYER_WINDOW_PLACE_RIGHT) == 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_LEFT));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeNavigationLayerWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// �E�ɔz�u
	(void)sprintf(buff, "%s", app->labels->window.place_right);
	menu_item = gtk_radio_menu_item_new_with_label(
		gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(app->tool_window.menu_item)), buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
		(app->layer_window.flags & LAYER_WINDOW_DOCKED) != 0 && (app->layer_window.flags & LAYER_WINDOW_PLACE_RIGHT) != 0);
	g_object_set_data(G_OBJECT(menu_item), "change_mode", GINT_TO_POINTER(UTILITY_PLACE_RIGHT));
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeNavigationLayerWindowPlace), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// ���C���[�r���[������Ɉړ�
	(void)sprintf(buff, "%s (%s)", app->labels->window.move_top_left, app->labels->layer_window.title);
	menu_item = gtk_menu_item_new_with_label(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMoveLayerWindowTopLeft), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// �i�r�Q�[�V����������Ɉړ�
	(void)sprintf(buff, "%s (%s)", app->labels->window.move_top_left, app->labels->navigation.title);
	menu_item = gtk_menu_item_new_with_label(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteMoveNavigationWindowTopLeft), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);

	// �u�v���r���[�v�E�B���h�E
	(void)sprintf(buff, "%s", app->labels->unit.preview);
	app->preview_window.menu_item = menu_item = gtk_check_menu_item_new_with_label(buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteChangeDisplayPreview), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�Q�l�p�摜�v�E�B���h�E
	(void)sprintf(buff, "%s", app->labels->window.reference);
	app->reference_window.menu_item = menu_item = gtk_check_menu_item_new_with_label(buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), FALSE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(DisplayReferenceWindowMenuActivate), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�t���X�N���[���v
	(void)sprintf(buff, "%s", app->labels->window.fullscreen);
	menu_item = gtk_check_menu_item_new_with_label(buff);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), app->flags & APPLICATION_FULL_SCREEN);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteSwitchFullScreen), app);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
#if GTK_MAJOR_VERSION <= 2
		GDK_Return, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
#else
		GDK_KEY_Return, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
#endif
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	//-----------------------------------------------------------//

	// �u�v���O�C���v���j���[
	menu_item = PlugInMenuItemNew(app);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'P', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	//-----------------------------------------------------------//
	// �u�X�N���v�g�v���j���[
	(void)sprintf(buff, "_%s(_S)", app->labels->menu.script);
	app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] =
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'S', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	app->menus.num_disable_if_no_open++;

	// �u�X�N���v�g�v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �ǂݍ��񂾃X�N���v�g�̖��O�����j���[�ɒǉ�
	for(i=0; i<app->scripts.num_script; i++)
	{
		(void)sprintf(buff, "%s", app->scripts.file_names[i]);
		menu_item = gtk_menu_item_new_with_mnemonic(buff);
		(void)g_signal_connect(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ExecuteScript), app);
		g_object_set_data(G_OBJECT(menu_item), "script_id", GINT_TO_POINTER(i));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	}

	//-----------------------------------------------------------//
	// �u�w���v�v���j���[
	(void)sprintf(buff, "_%s(_H)", app->labels->menu.help);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ParentItemSelected), NULL);
	gtk_widget_add_accelerator(menu_item, "activate", accel_group,
		'H', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	// �u�w���v�v���j���[�̉��ɃT�u���j���[�쐬
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u�o�[�W�������v
	(void)sprintf(buff, "%s", app->labels->menu.version);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(DisplayVersion), app);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �L�����o�X�̐���0�̂Ƃ��ɖ����ȃ��j���[��ݒ�
	for(i=0; i<app->menus.num_disable_if_no_open; i++)
	{
		gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], FALSE);
	}

	// �I��͈͂��Ȃ��Ƃ��ɖ����ȃ��j���[��ݒ�
	for(i=0; i<app->menus.num_disable_if_no_select; i++)
	{
		gtk_widget_set_sensitive(app->menus.disable_if_no_select[i], FALSE);
	}

	return menu_bar;
}

/*************************************
* ADD_MAKE_NEW_DATA�\����            *
* �V�K�쐬���̃v���Z�b�g�f�[�^�ǉ��p *
*************************************/
typedef struct _ADD_MAKE_NEW_DATA
{
	APPLICATION *app;
	GtkWidget *dialog;
	GtkWidget *name;
	GtkWidget *width;
	GtkWidget *height;
	GtkWidget *resolution;
	GtkWidget *combo;
} ADD_MAKE_NEW_DATA;

/*********************************************************
* AddmakeNewData�֐�                                     *
* �V�K�쐬���̃v���Z�b�g�f�[�^��ǉ�����                 *
* ����                                                   *
* button	: �V�K�쐬�p�{�^���E�B�W�F�b�g               *
* make_new	: �V�K�쐬���̃v���Z�b�g�f�[�^�ǉ��p�̃f�[�^ *
*********************************************************/
static void AddMakeNewData(GtkWidget* button, ADD_MAKE_NEW_DATA* make_new)
{
	APPLICATION *app = make_new->app;
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *width;
	GtkWidget *height;
	GtkWidget *resolution;
	GtkWidget *hbox;
	GtkWidget *label;
	char str[256];

	dialog = 
		gtk_dialog_new_with_buttons(
			"",
			GTK_WINDOW(make_new->dialog),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			NULL
		);

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", app->labels->unit.name);
	label = gtk_label_new(str);
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", app->labels->make_new.width);
	label = gtk_label_new(str);
	width = gtk_spin_button_new_with_range(1, LONG_MAX, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(width),
		gtk_spin_button_get_value(GTK_SPIN_BUTTON(make_new->width)));
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(width), 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), width, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", app->labels->make_new.height);
	label = gtk_label_new(str);
	height = gtk_spin_button_new_with_range(1, LONG_MAX, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(height),
		gtk_spin_button_get_value(GTK_SPIN_BUTTON(make_new->height)));
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(height), 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), height, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", "dpi");
	label = gtk_label_new(str);
	resolution = gtk_spin_button_new_with_range(1, LONG_MAX, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(resolution),
		gtk_spin_button_get_value(GTK_SPIN_BUTTON(make_new->resolution)));
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(resolution), 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), resolution, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		hbox, FALSE, FALSE, 0);

	gtk_widget_show_all(dialog);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		const char *name = gtk_entry_get_text(GTK_ENTRY(entry));

		if(name != NULL)
		{
			int index;
			app->menu_data.num_make_new_data++;
			index = app->menu_data.num_make_new_data;
			app->menu_data.make_new = (MAKE_NEW_DATA*)MEM_REALLOC_FUNC(
				app->menu_data.make_new, sizeof(*app->menu_data.make_new) * (app->menu_data.num_make_new_data+1));
			app->menu_data.make_new[index].name = MEM_STRDUP_FUNC(name);
			app->menu_data.make_new[index].width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(width));
			app->menu_data.make_new[index].height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(height));
			app->menu_data.make_new[index].resolution = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(resolution));

#if GTK_MAJOR_VERSION <= 2
			gtk_combo_box_append_text(GTK_COMBO_BOX(make_new->name), name);
#else
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(make_new->name), name);
#endif
			gtk_combo_box_set_active(GTK_COMBO_BOX(make_new->name), index-1);
		}
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(make_new->width), gtk_spin_button_get_value(GTK_SPIN_BUTTON(width)));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(make_new->height), gtk_spin_button_get_value(GTK_SPIN_BUTTON(height)));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(make_new->resolution), gtk_spin_button_get_value(GTK_SPIN_BUTTON(resolution)));
	}
	gtk_widget_destroy(dialog);
}

/*************************************************
* SetFromMakeNewPreset�֐�                       *
* �V�K�쐬�̃f�[�^���v���Z�b�g����ݒ肷��       *
* ����                                           *
* combo		: �v���Z�b�g�f�[�^�I��p�E�B�W�F�b�g *
* make_new	: �v���Z�b�g�f�[�^                   *
*************************************************/
static void SetFromMakeNewPreset(GtkWidget* combo, ADD_MAKE_NEW_DATA* make_new)
{
	APPLICATION *app = make_new->app;
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	if(index >= 0)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(make_new->width), app->menu_data.make_new[index+1].width);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(make_new->height), app->menu_data.make_new[index+1].height);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(make_new->resolution), app->menu_data.make_new[index+1].resolution);
	}
}

/*******************************
* SwapHeightAndWidth�֐�       *
* �c�������ւ���             *
* ����                         *
* make_new	: �V�K�쐬�p�f�[�^ *
*******************************/
static void SwapHeightAndWidth(ADD_MAKE_NEW_DATA* make_new)
{
	int width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(make_new->width));
	int height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(make_new->height));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(make_new->width), height);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(make_new->height), width);
}

/*****************************************************
* ExecuteNew�֐�                                     *
* �V�K�쐬�����s                                     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
static void ExecuteNew(APPLICATION* app)
{
	// �V�K�쐬���̃f�[�^�p
	ADD_MAKE_NEW_DATA add_make_new = {0};
	// �摜�̕��ƍ��������߂�_�C�A���O�{�b�N�X���쐬
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->make_new.title,
			GTK_WINDOW(app->widgets->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
	// �E�B�W�F�b�g�̔z�u�p
	GtkWidget *table, *label;
	GtkWidget *second_color;
	GtkWidget *control;
	GtkAdjustment *adjust;
	// 2�߂̔w�i�F
	GdkColor second_back_rgb;
	// OK, �L�����Z���̌��ʂ��󂯂�
	gint ret;
	// ���x���p������
	char str[256];
	// �쐬����摜�̕��ƍ���
	int32 width, height;
	// for���p�̃J�E���^
	int i;

	add_make_new.dialog = dialog;
	add_make_new.app = app;

	table = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s : ", app->labels->make_new.preset);
	label = gtk_label_new(str);
#if GTK_MAJOR_VERSION <= 2
	add_make_new.name = gtk_combo_box_new_text();
	for(i=0; i<app->menu_data.num_make_new_data; i++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(add_make_new.name), app->menu_data.make_new[i+1].name);
	}
#else
	add_make_new.name = gtk_combo_box_text_new();
	for(i=0; i<app->menu_data.num_make_new_data; i++)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(add_make_new.name), app->menu_data.make_new[i+1].name);
	}
#endif
	(void)g_signal_connect(G_OBJECT(add_make_new.name), "changed", G_CALLBACK(SetFromMakeNewPreset), &add_make_new);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), add_make_new.name, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, FALSE, FALSE, 0);

	table = gtk_table_new(4, 3, FALSE);

	// �_�C�A���O�{�b�N�X�ɕ��̃��x���ƒl�ݒ�p�̃E�B�W�F�b�g��o�^
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);
	label = gtk_label_new(app->labels->make_new.width);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->menu_data.make_new->width, 1, LONG_MAX, 1, 10, 0));
	add_make_new.width = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), add_make_new.width, 1, 2, 0, 1);

	// �_�C�A���O�{�b�N�X�ɍ����̃��x���ƒl�ݒ�p�̃E�B�W�F�b�g��o�^
	label = gtk_label_new(app->labels->make_new.height);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->menu_data.make_new->height, 1, LONG_MAX, 1, 10, 0));
	add_make_new.height = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), add_make_new.height, 1, 2, 1, 2);

	// �𑜓x�ݒ�p�̃E�B�W�F�b�g��o�^
	label = gtk_label_new(app->labels->unit.resolution);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->menu_data.make_new->resolution,
		1, 1200, 1, 1, 0));
	add_make_new.resolution = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), add_make_new.resolution, 1, 2, 2, 3);
	label = gtk_label_new("dpi");
	gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 2, 3);

	// 2�߂̔w�i�F�ݒ�p�̃E�B�W�F�b�g��o�^
	label = gtk_label_new(app->labels->make_new.second_bg_color);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
	second_back_rgb.red = (app->menu_data.second_bg_color[0] << 8) | app->menu_data.second_bg_color[0];
	second_back_rgb.green = (app->menu_data.second_bg_color[1] << 8) | app->menu_data.second_bg_color[1];
	second_back_rgb.blue = (app->menu_data.second_bg_color[2] << 8) | app->menu_data.second_bg_color[2];
	second_color = gtk_color_button_new_with_color(&second_back_rgb);
	gtk_color_button_set_title(GTK_COLOR_BUTTON(second_color), app->labels->make_new.second_bg_color);
	gtk_table_attach_defaults(GTK_TABLE(table), second_color, 1, 2, 3, 4);

	// �c������ւ��{�^��
	control = gtk_button_new_with_label(app->labels->make_new.swap_height_and_width);
	(void)g_signal_connect_swapped(G_OBJECT(control), "clicked",
		G_CALLBACK(SwapHeightAndWidth), &add_make_new);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), control, FALSE, FALSE, 0);

	// �v���Z�b�g�ǉ��{�^��
	control = gtk_button_new_with_label(app->labels->make_new.add_preset);
	(void)g_signal_connect(G_OBJECT(control), "clicked", G_CALLBACK(AddMakeNewData), &add_make_new);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), control, FALSE, FALSE, 0);

	// �_�C�A���O�{�b�N�X��\��
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	// ���[�U�[���uOK�v�A�u�L�����Z���v���N���b�N�܂ő҂�
	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	// �uOK�v�������ꂽ��
	if(ret == GTK_RESPONSE_ACCEPT)
	{	// ���͂��ꂽ���ƍ����̒l���擾
		width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(add_make_new.width));
		height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(add_make_new.height));

		app->menu_data.make_new->width = width;
		app->menu_data.make_new->height = height;
		app->menu_data.make_new->resolution =
			(uint16)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(add_make_new.resolution));

		// �`��̈���쐬
		app->draw_window[app->window_num] =
			CreateDrawWindow(width, height, 4, app->labels->make_new.name,
			app->widgets->note_book, app->window_num, app);
		app->active_window = app->window_num;
		app->draw_window[app->active_window]->resolution =
			(uint16)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(add_make_new.resolution));
		gtk_color_button_get_color(GTK_COLOR_BUTTON(second_color), &second_back_rgb);
		app->menu_data.second_bg_color[0] = second_back_rgb.red / 256;
		app->menu_data.second_bg_color[1] = second_back_rgb.green / 256;
		app->menu_data.second_bg_color[2] = second_back_rgb.blue / 256;
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
		app->draw_window[app->active_window]->second_back_ground[2] = app->menu_data.second_bg_color[0];
		app->draw_window[app->active_window]->second_back_ground[1] = app->menu_data.second_bg_color[1];
		app->draw_window[app->active_window]->second_back_ground[0] = app->menu_data.second_bg_color[2];
#else
		app->draw_window[app->active_window]->second_back_ground[0] = app->menu_data.second_bg_color[0];
		app->draw_window[app->active_window]->second_back_ground[1] = app->menu_data.second_bg_color[1];
		app->draw_window[app->active_window]->second_back_ground[2] = app->menu_data.second_bg_color[2];
#endif
		app->window_num++;

		ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

		// �A�N�e�B�u���C���[�̕\��
		LayerViewSetActiveLayer(
			app->draw_window[app->active_window]->active_layer,
			app->layer_window.view
		);

		// �E�B���h�E�̃^�C�g���o�[���u�V�K�쐬�v��
		UpdateWindowTitle(app);
		
		// �����ɂ��Ă����ꕔ�̃��j���[��L����
		for(i=0; i<app->menus.num_disable_if_no_open; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
		}
		gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
		gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);
	}

	// �_�C�A���O�{�b�N�X�����
	gtk_widget_destroy(dialog);
}

/*****************************************************
* ExecuteMakeColorLayer�֐�                          *
* �ʏ탌�C���[�쐬�����s                             *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteMakeColorLayer(APPLICATION* app)
{
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// �쐬�������C���[�̃A�h���X
	LAYER* layer;
	// �V�K�쐬���郌�C���[�̖��O
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// ���C���[�̖��O����p�̃J�E���^
	int counter = 1;

	//AUTO_SAVE(app->draw_window[app->active_window]);

	// �u���C���[���v�́��ɓ���l������
		// (���C���[���̏d���������)
	do
	{
		(void)sprintf(layer_name, "%s %d", app->labels->layer_window.new_layer, counter);
		counter++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// ���݂̃A�N�e�B�u���C���[���ʏ탌�C���[�ȊO�Ȃ�΃c�[���ύX
	if(window->active_layer->layer_type != TYPE_NORMAL_LAYER)
	{
		gtk_widget_destroy(app->tool_window.brush_table);
		CreateBrushTable(app, &app->tool_window, app->tool_window.brushes);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(app->tool_window.brush_scroll),
			app->tool_window.brush_table);
		gtk_widget_show_all(app->tool_window.brush_table);
	}

	// ���C���[�쐬
	layer =CreateLayer(
		0,
		0,
		window->width,
		window->height,
		window->channel,
		TYPE_NORMAL_LAYER,
		window->active_layer,
		window->active_layer->next,
		layer_name,
		window
	);
	// ��ƃ��C���[�̃s�N�Z���f�[�^��������
	(void)memset(window->work_layer->pixels, 0, layer->stride*layer->height);

	// �Ǐ��L�����o�X���[�h�Ȃ�e�L�����o�X�ł����C���[�쐬
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

	// ���C���[�̐����X�V
	window->num_layer++;

	// ���C���[�E�B���h�E�ɒǉ����ăA�N�e�B�u���C���[��
	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);

	// �u�V�K���C���[�v�̗�����o�^
	AddNewLayerHistory(layer, layer->layer_type);
}

/*****************************************************
* ExecuteMakeVectorLayer�֐�                         *
* �V�K�x�N�g�����C���[�쐬�����s                     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteMakeVectorLayer(APPLICATION *app)
{
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// �V�K�쐬�������C���[�̃A�h���X���󂯎��
	LAYER* layer;
	// �쐬���郌�C���[�̖��O
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// ���C���[������p�̃J�E���^
	int counter = 1;

	AUTO_SAVE(app->draw_window[app->active_window]);

	// �u�x�N�g�����v�́��ɓ��鐔�l������
		// (���C���[���̏d���������)
	do
	{
		(void)sprintf(layer_name, "%s %d", app->labels->layer_window.new_vector, counter);
		counter++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// �A�N�e�B�u���C���[���x�N�g�����C���[�łȂ���΃c�[���ύX
	if(window->active_layer->layer_type != TYPE_VECTOR_LAYER)
	{
		gtk_widget_destroy(app->tool_window.brush_table);
		CreateVectorBrushTable(app, &app->tool_window, app->tool_window.vector_brushes);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(app->tool_window.brush_scroll),
			app->tool_window.brush_table);
		gtk_widget_show_all(app->tool_window.brush_table);

		gtk_widget_destroy(app->tool_window.detail_ui);
		app->tool_window.detail_ui =
			app->tool_window.active_vector_brush[app->input]->create_detail_ui(app, app->tool_window.active_vector_brush[app->input]->brush_data);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
		gtk_widget_show_all(app->tool_window.detail_ui);
	}

	// ���C���[�쐬
	layer = CreateLayer(
		0,
		0,
		window->width,
		window->height,
		window->channel,
		TYPE_VECTOR_LAYER,
		window->active_layer,
		window->active_layer->next,
		layer_name,
		window
	);

	// �Ǐ��L�����o�X���[�h�Ȃ�e�L�����o�X�ł����C���[�쐬
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
	// ���C���[�����X�V
	window->num_layer++;

	// ���C���[�E�B���h�E�ɓo�^���ăA�N�e�B�u���C���[��
	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);

	// �u�V�K�x�N�g�����C���[�v�̗�����o�^
	AddNewLayerHistory(layer, layer->layer_type);
}

/*****************************************************
* ExecuteMakeLayerSet�֐�                            *
* �V�K���C���[�Z�b�g�쐬�����s                       *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteMakeLayerSet(APPLICATION *app)
{
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// �V�K�쐬�������C���[�̃A�h���X���󂯎��
	LAYER* layer;
	// �쐬���郌�C���[�̖��O
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// ���C���[������p�̃J�E���^
	int counter = 1;

	AUTO_SAVE(app->draw_window[app->active_window]);

	// �u���C���[�Z�b�g���v�́��ɓ��鐔�l������
		// (���C���[���̏d���������)
	do
	{
		(void)sprintf(layer_name, "%s %d", app->labels->layer_window.new_layer_set, counter);
		counter++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// ���C���[�쐬
	layer = CreateLayer(
		0,
		0,
		window->width,
		window->height,
		window->channel,
		TYPE_LAYER_SET,
		window->active_layer,
		window->active_layer->next,
		layer_name,
		window
	);

	// �Ǐ��L�����o�X���[�h�Ȃ�e�L�����o�X�ł����C���[�쐬
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

	// ���C���[�����X�V
	app->draw_window[app->active_window]->num_layer++;

	// ���C���[�E�B���h�E�ɓo�^���ăA�N�e�B�u���C���[��
	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);

	// �u�V�K�x�N�g�����C���[�v�̗�����o�^
	AddNewLayerHistory(layer, layer->layer_type);
}

/*****************************************************
* ExecuteMakeAdjustmentLayer�֐�                     *
* �������C���[�쐬�����s                             *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteMakeAdjustmentLayer(APPLICATION *app)
{
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// �V�K�쐬�������C���[�̃A�h���X���󂯎��
	LAYER* layer;
	// �쐬���郌�C���[�̖��O
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// ���C���[������p�̃J�E���^
	int counter = 1;

	AUTO_SAVE(app->draw_window[app->active_window]);

	// �u���C���[�Z�b�g���v�́��ɓ��鐔�l������
		// (���C���[���̏d���������)
	do
	{
		(void)sprintf(layer_name, "%s %d", app->labels->layer_window.new_adjsutment_layer, counter);
		counter++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// ���C���[�쐬
	layer = CreateLayer(
		0,
		0,
		window->width,
		window->height,
		window->channel,
		TYPE_ADJUSTMENT_LAYER,
		window->active_layer,
		window->active_layer->next,
		layer_name,
		window
	);

	// �Ǐ��L�����o�X���[�h�Ȃ�e�L�����o�X�ł����C���[�쐬
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

	// ���C���[�����X�V
	app->draw_window[app->active_window]->num_layer++;

	// ���C���[�E�B���h�E�ɓo�^���ăA�N�e�B�u���C���[��
	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);

	// �u�V�K�x�N�g�����C���[�v�̗�����o�^
	AddNewLayerHistory(layer, layer->layer_type);
}

/*****************************************************
* ExecuteMake3DLayer�֐�                             *
* 3D���f�����O���C���[�쐬�����s                     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteMake3DLayer(APPLICATION* app)
{
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// �V�K�쐬�������C���[�̃A�h���X���󂯎��
	LAYER* layer;
	// �쐬���郌�C���[�̖��O
	char layer_name[MAX_LAYER_NAME_LENGTH];
	// ���C���[������p�̃J�E���^
	int counter = 1;

	AUTO_SAVE(app->draw_window[app->active_window]);

	// �u���C���[�Z�b�g���v�́��ɓ��鐔�l������
		// (���C���[���̏d���������)
	do
	{
		(void)sprintf(layer_name, "%s %d", app->labels->layer_window.new_3d_modeling, counter);
		counter++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// ���C���[�쐬
	layer = CreateLayer(
		0,
		0,
		window->width,
		window->height,
		window->channel,
		TYPE_3D_LAYER,
		window->active_layer,
		window->active_layer->next,
		layer_name,
		window
	);

	// �Ǐ��L�����o�X���[�h�Ȃ�e�L�����o�X�ł����C���[�쐬
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

	// ���C���[�����X�V
	window->num_layer++;

	// ���C���[�E�B���h�E�ɓo�^���ăA�N�e�B�u���C���[��
	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);

	// �u�V�K�x�N�g�����C���[�v�̗�����o�^
	AddNewLayerHistory(layer, layer->layer_type);
}

/*****************************************************
* ExecuteUpLayer�֐�                                 *
* ���C���[�̏�����1��ɕύX����                    *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteUpLayer(APPLICATION* app)
{
	DRAW_WINDOW *window;
	LAYER *parent;
	LAYER *before_prev, *after_prev, *before_parent;
	int hierarchy = 0;

	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);
	if(window->active_layer->next == NULL)
	{
		return;
	}

	before_prev = window->active_layer->prev;
	after_prev = window->active_layer->next;
	before_parent = window->active_layer->layer_set;

	if(window->active_layer->layer_type == TYPE_LAYER_SET)
	{
		LAYER *target = window->active_layer->prev;
		LAYER *new_prev = window->active_layer->next;
		LAYER *bottom_child = window->active_layer->prev;
		LAYER *next_target;

		while(bottom_child != NULL && bottom_child->layer_set == window->active_layer)
		{
			before_prev = bottom_child = bottom_child->prev;
		}
		if(bottom_child == NULL && new_prev->layer_type == TYPE_LAYER_SET)
		{
			return;
		}

		window->active_layer->layer_set = window->active_layer->next->layer_set;

		ChangeLayerOrder(window->active_layer, window->active_layer->next,
			&window->layer);

		while(target != NULL && target->layer_set == window->active_layer)
		{
			next_target = target->prev;
			ChangeLayerOrder(target, new_prev, &window->layer);
			target = next_target;
		}

		ClearLayerView(&app->layer_window);
		LayerViewSetDrawWindow(&app->layer_window, window);
	}
	else
	{
		window->active_layer->layer_set = window->active_layer->next->layer_set;

		ChangeLayerOrder(window->active_layer, window->active_layer->next,
			&window->layer);

		gtk_box_reorder_child(
			GTK_BOX(app->layer_window.view),
			window->active_layer->widget->box,
			GetLayerID(window->layer, window->active_layer->prev, window->num_layer)
		);
	}

	AddChangeLayerOrderHistory(window->active_layer, before_prev, after_prev, before_parent);

	parent = window->active_layer->layer_set;
	while(parent != NULL)
	{
		hierarchy++;
		parent = parent->layer_set;
	}

	gtk_alignment_set_padding(GTK_ALIGNMENT(window->active_layer->widget->alignment),
		0, 0, hierarchy * LAYER_SET_DISPLAY_OFFSET, 0);

	ChangeActiveLayer(window, window->active_layer);
}

/*****************************************************
* ExecuteDownLayer�֐�                               *
* ���C���[�̏�����1���ɕύX����                    *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteDownLayer(APPLICATION* app)
{
	DRAW_WINDOW *window;
	LAYER *before_prev, *after_prev, *before_parent;
	int hierarchy = 0;

	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);
	if(window->active_layer->prev == NULL)
	{
		return;
	}

	before_prev = window->active_layer->prev;
	after_prev = window->active_layer->prev->prev;
	before_parent = window->active_layer->layer_set;

	if(window->active_layer->layer_type == TYPE_LAYER_SET)
	{
		LAYER *target = window->active_layer->prev;
		LAYER *new_prev = window->active_layer->prev;
		LAYER *next_target;

		while(new_prev != NULL && new_prev->layer_set == window->active_layer)
		{
			new_prev = new_prev->prev;
		}

		if(new_prev != NULL)
		{
			if(new_prev->layer_type == TYPE_LAYER_SET)
			{
				window->active_layer->layer_set = new_prev;
			}
			else
			{
				window->active_layer->layer_set = new_prev->layer_set;
			}
		}
		else
		{
			window->active_layer->layer_set = NULL;
		}

		ChangeLayerOrder(window->active_layer, window->active_layer->prev->prev,
			&window->layer);

		while(target != NULL && target->layer_set == window->active_layer)
		{
			next_target = target->prev;
			ChangeLayerOrder(target, new_prev, &window->layer);
			target = next_target;
		}
	}
	else
	{
		if(window->active_layer->prev->layer_type == TYPE_LAYER_SET)
		{
			window->active_layer->layer_set = window->active_layer->prev;
		}
		else
		{
			window->active_layer->layer_set = window->active_layer->prev->layer_set;
		}

		ChangeLayerOrder(window->active_layer, window->active_layer->prev->prev,
			&window->layer);
	}

	AddChangeLayerOrderHistory(window->active_layer, before_prev, after_prev, before_parent);

	ResetLayerView(window);

	ChangeActiveLayer(window, window->active_layer);
}

/*****************************************************
* ExecuteZoomIn�֐�                                  *
* �g������s                                         *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteZoomIn(APPLICATION *app)
{
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// �A�N�e�B�u�ȕ`��̈�̊g�嗦�ύX
	window->zoom += 10;
	// �ő�l�𒴂��Ă�����C��
	if(window->zoom > MAX_ZOOM)
	{
		window->zoom = MAX_ZOOM;
	}

	// �i�r�Q�[�V�����̊g��k�����X���C�_�𓮂�����
		// �`��̈�̃��T�C�Y�����s��
	gtk_adjustment_set_value(app->navigation_window.zoom_slider,
		window->zoom);
}

/*****************************************************
* ExecuteZoomOut�֐�                                 *
* �k�������s                                         *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteZoomOut(APPLICATION *app)
{
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// �A�N�e�B�u�ȕ`��̈�̊g�嗦��ύX
	window->zoom -= 10;
	// �ŏ��l�������Ă�����C��
	if(window->zoom < MIN_ZOOM)
	{
		window->zoom = MIN_ZOOM;
	}

	// �i�r�Q�[�V�����̊g��k�����X���C�_�𓮂�����
		// �`��̈�̃��T�C�Y�����s��
	gtk_adjustment_set_value(app->navigation_window.zoom_slider,
		window->zoom);
}

/*****************************************************
* ExecuteZoomReset�֐�                               *
* ���{�\�������s                                     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteZoomReset(APPLICATION* app)
{
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// �`��̈悪�Ȃ���ΏI��
	if(app->window_num == 0)
	{
		return;
	}

	// �A�N�e�B�u�ȕ`��̈�̊g�嗦�����Z�b�g
	window->zoom = 100;

	// �i�r�Q�[�V�����̊g��k�����X���C�_�𓮂�����
		// �`��̈�̃��T�C�Y�����s��
	gtk_adjustment_set_value(app->navigation_window.zoom_slider,
		window->zoom);
}

#define ROTATE_STEP 15
/*****************************************************
* ExecuteRotateClockwise�֐�                         *
* �\�������v���ɉ�]                               *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteRotateClockwise(APPLICATION* app)
{
	// �`��̈�̏��
	DRAW_WINDOW *window;
	// �V�����p�x
	FLOAT_T angle;

	// �`��̈悪�Ȃ���ΏI��
	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);

	// ���݂̊p�x���擾
	angle = gtk_adjustment_get_value(app->navigation_window.rotate_slider) + ROTATE_STEP;
	// 180�x�𒴂��Ă�����C��
	if(angle > 180)
	{
		angle -= 360;
	}

	// �p�x��ݒ�
	gtk_adjustment_set_value(app->navigation_window.rotate_slider, angle);
}

/*****************************************************
* ExecuteRotateCounterClockwise�֐�                  *
* �\���𔽎��v���ɉ�]                             *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteRotateCounterClockwise(APPLICATION* app)
{
	// �`��̈�̏��
	DRAW_WINDOW *window;
	// �V�����p�x
	FLOAT_T angle;

	// �`��̈悪�Ȃ���ΏI��
	if(app->window_num == 0)
	{
		return;
	}

	window = GetActiveDrawWindow(app);

	// ���݂̊p�x���擾
	angle = gtk_adjustment_get_value(app->navigation_window.rotate_slider) - ROTATE_STEP;
	// -180�x�𒴂��Ă�����C��
	if(angle < -180)
	{
		angle += 360;
	}

	// �p�x��ݒ�
	gtk_adjustment_set_value(app->navigation_window.rotate_slider, angle);
}

/*****************************************************
* ExecuteRotateReset�֐�                             *
* ��]�p�x�����Z�b�g����                             *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteRotateReset(APPLICATION* app)
{
	// ��]�p�x��0��ݒ�
	gtk_adjustment_set_value(app->navigation_window.rotate_slider, 0);
}

/*********************************************************
* ExecuteDisplayReverseHorizontally�֐�                  *
* �\�������E���]                                         *
* ����                                                   *
* menu_item	: ���j���[�A�C�e���E�B�W�F�b�g               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteDisplayReverseHorizontally(GtkWidget* menu_item, APPLICATION* app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	GtkAdjustment *scroll_x_adjust;
	GtkAllocation allocation;
	double x, max_x;

	gboolean state = gtk_check_menu_item_get_active(
		GTK_CHECK_MENU_ITEM(menu_item));

	if((app->flags & APPLICATION_IN_REVERSE_OPERATION) == 0)
	{
		app->flags |= APPLICATION_IN_REVERSE_OPERATION;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->widgets->reverse_button), state);
		app->flags &= ~(APPLICATION_IN_REVERSE_OPERATION);
	}

	if(state == FALSE)
	{
		window->flags &= ~(DRAW_WINDOW_DISPLAY_HORIZON_REVERSE);
	}
	else
	{
		window->flags |= DRAW_WINDOW_DISPLAY_HORIZON_REVERSE;
	}

	ChangeNavigationDrawWindow(&app->navigation_window, window);

#if GTK_MAJOR_VERSION >= 3
	gtk_widget_get_allocation(window->scroll, &allocation);
#else
	allocation = window->scroll->allocation;
#endif
	scroll_x_adjust = gtk_scrolled_window_get_hadjustment(
		GTK_SCROLLED_WINDOW(window->scroll));
	x = gtk_adjustment_get_value(scroll_x_adjust);
	max_x = gtk_adjustment_get_upper(scroll_x_adjust);
	gtk_adjustment_set_value(scroll_x_adjust, max_x - x - allocation.width);

	// ��ʍX�V���邱�ƂŃi�r�Q�[�V�����ƃv���r���[�̓��e���X�V
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

/*********************************************************
* ExecuteChangeToolWindowPlace�֐�                       *
* �c�[���{�b�N�X�̈ʒu��ύX����                         *
* ����                                                   *
* menu_item	: ���j���[�A�C�e���E�B�W�F�b�g               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteChangeToolWindowPlace(GtkWidget* menu_item, APPLICATION* app)
{
	HSV hsv, back_hsv;
	uint8 fg_color[3], bg_color[3];
	GtkWidget *box;

	if((app->flags & APPLICATION_INITIALIZED) == 0
		|| (app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) != 0)
	{
		return;
	}

	// �y�[���̍��W���L��
	if(gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane)) != NULL)
	{
		app->left_pane_position = gtk_paned_get_position(GTK_PANED(app->widgets->left_pane));
	}
	if(gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane)) != NULL)
	{
		app->right_pane_position = gtk_paned_get_position(GTK_PANED(app->widgets->right_pane));
	}

	// ���݂̐F���L��
	hsv = app->tool_window.color_chooser->hsv;
	back_hsv = app->tool_window.color_chooser->back_hsv;
	(void)memcpy(fg_color, app->tool_window.color_chooser->rgb, 3);
	(void)memcpy(bg_color, app->tool_window.color_chooser->back_rgb, 3);

	if(menu_item != NULL)
	{
		app->tool_window.place = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu_item), "change_mode"));
	}

	switch(app->tool_window.place)
	{
	case UTILITY_PLACE_WINDOW:
		if(app->tool_window.window == NULL
			&& (app->flags & APPLICATION_IN_DELETE_EVENT) == 0)
		{
			if(app->tool_window.ui != NULL)
			{
				gtk_widget_destroy(app->tool_window.ui);
			}

			app->tool_window.flags &= ~(TOOL_DOCKED | TOOL_POP_UP);
			app->tool_window.window = CreateToolBoxWindow(app, app->widgets->window);

			gtk_widget_show_all(app->tool_window.window);
		}
		break;
	case UTILITY_PLACE_LEFT:
		if(app->tool_window.window != NULL)
		{
			gtk_widget_destroy(app->tool_window.window);
		}

		if(app->tool_window.ui != NULL)
		{
			gtk_widget_destroy(app->tool_window.ui);
		}

		app->tool_window.flags |= TOOL_DOCKED;
		app->tool_window.flags &= ~(TOOL_PLACE_RIGHT | TOOL_POP_UP);

		app->tool_window.window = CreateToolBoxWindow(app, app->widgets->window);
		gtk_widget_show_all(app->tool_window.ui);

		break;
	case UTILITY_PLACE_RIGHT:
		if(app->tool_window.window != NULL)
		{
			gtk_widget_destroy(app->tool_window.window);
		}

		if(app->tool_window.ui != NULL)
		{
			gtk_widget_destroy(app->tool_window.ui);
		}

		app->tool_window.flags |= TOOL_DOCKED;
		app->tool_window.flags |= TOOL_PLACE_RIGHT;
		app->tool_window.flags &= ~(TOOL_POP_UP);

		app->tool_window.window = CreateToolBoxWindow(app, app->widgets->window);
		gtk_widget_show_all(app->tool_window.ui);

		break;
	case UTILITY_PLACE_POPUP_LEFT:
		if(app->tool_window.window == NULL
			&& (app->flags & APPLICATION_IN_DELETE_EVENT) == 0)
		{
			if(app->tool_window.ui != NULL)
			{
				gtk_widget_destroy(app->tool_window.ui);
			}

			app->tool_window.flags |= TOOL_POP_UP;
			app->tool_window.flags &= ~(TOOL_DOCKED | TOOL_PLACE_RIGHT);
			app->tool_window.window = CreateToolBoxWindow(app, app->widgets->window);

			gtk_widget_show_all(app->tool_window.window);
		}
		break;
	case UTILITY_PLACE_POPUP_RIGHT:
		if(app->tool_window.window == NULL
			&& (app->flags & APPLICATION_IN_DELETE_EVENT) == 0)
		{
			if(app->tool_window.ui != NULL)
			{
				gtk_widget_destroy(app->tool_window.ui);
			}

			app->tool_window.flags |= TOOL_POP_UP | TOOL_PLACE_RIGHT;
			app->tool_window.flags &= ~(TOOL_DOCKED);
			app->tool_window.window = CreateToolBoxWindow(app, app->widgets->window);

			gtk_widget_show_all(app->tool_window.window);
		}
		break;
	}

	if((app->tool_window.flags & TOOL_USING_BRUSH) == 0)
	{
		app->tool_window.detail_ui = app->tool_window.active_common_tool->create_detail_ui(
			app, app->tool_window.active_common_tool->tool_data);
		gtk_scrolled_window_add_with_viewport(
			GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
		gtk_widget_show_all(app->tool_window.detail_ui);
	}
	else
	{
		if(app->window_num == 0 || app->current_tool == TYPE_NORMAL_LAYER)
		{
			app->tool_window.detail_ui = app->tool_window.active_brush[app->input]->create_detail_ui(
				app, app->tool_window.active_brush[app->input]);
			gtk_scrolled_window_add_with_viewport(
				GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
			gtk_widget_show_all(app->tool_window.detail_ui);
		}
		else
		{
			app->current_tool = TYPE_NORMAL_LAYER;
			if(menu_item != NULL)
			{
				ChangeActiveLayer(app->draw_window[app->active_window], app->draw_window[app->active_window]->active_layer);
			}
		}
	}

	(void)memcpy(app->tool_window.color_chooser->back_rgb, bg_color, 3);
	app->tool_window.color_chooser->back_hsv = back_hsv;
	(void)memcpy(app->tool_window.color_chooser->rgb, fg_color, 3);
	SetColorChooserPoint(app->tool_window.color_chooser, &hsv, TRUE);

	box = gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane));
	if(box != NULL)
	{
		GList *child = gtk_container_get_children(GTK_CONTAINER(box));
		if(child == NULL)
		{
			gtk_widget_destroy(box);
		}
		g_list_free(child);
	}

	box = gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane));
	if(box != NULL)
	{
		GList *child = gtk_container_get_children(GTK_CONTAINER(box));
		if(child == NULL)
		{
			gtk_widget_destroy(box);
		}
		g_list_free(child);
	}
}

/*********************************************************
* ExecuteChangeNavigationLayerWindowPlace�֐�            *
* �i�r�Q�[�V�����ƃ��C���[�r���[�̈ʒu��ύX����         *
* ����                                                   *
* menu_item	: �ʒu�ύX���j���[�A�C�e���E�B�W�F�b�g       *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteChangeNavigationLayerWindowPlace(GtkWidget* menu_item, APPLICATION* app)
{
	// �E�B���h�E�ƃh�b�L���O���ɃE�B�W�F�b�g������p�b�L���O�{�b�N�X
	GtkWidget *box;

	if((app->flags & APPLICATION_INITIALIZED) == 0
		|| (app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) != 0)
	{
		return;
	}

	// �y�[���̍��W���L��
	if(gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane)) != NULL)
	{
		app->left_pane_position = gtk_paned_get_position(GTK_PANED(app->widgets->left_pane));
	}
	if(gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane)) != NULL)
	{
		app->right_pane_position = gtk_paned_get_position(GTK_PANED(app->widgets->right_pane));
	}

	if(menu_item != NULL)
	{
		app->layer_window.place = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu_item), "change_mode"));
	}

	switch(app->layer_window.place)
	{
	case UTILITY_PLACE_WINDOW:
		app->layer_window.flags &= ~(LAYER_WINDOW_DOCKED | LAYER_WINDOW_PLACE_RIGHT | LAYER_WINDOW_POP_UP);
		if(app->layer_window.window == NULL)
		{
			if(app->widgets->navi_layer_pane != NULL)
			{
				app->layer_window.pane_position = gtk_paned_get_position(
					GTK_PANED(app->widgets->navi_layer_pane));
				gtk_widget_destroy(app->widgets->navi_layer_pane);
				app->widgets->navi_layer_pane = NULL;
			}

			app->layer_window.window = CreateLayerWindow(app, app->widgets->window, &app->layer_window.view);
			gtk_widget_show_all(app->layer_window.window);
		}

		if(app->navigation_window.window == NULL)
		{
			InitializeNavigation(&app->navigation_window, app, NULL);
		}

		box = gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		break;
	case UTILITY_PLACE_LEFT:
		app->layer_window.flags |= LAYER_WINDOW_DOCKED;
		app->layer_window.flags &= ~(LAYER_WINDOW_PLACE_RIGHT | LAYER_WINDOW_POP_UP);

		if(app->layer_window.window != NULL)
		{
			gtk_widget_destroy(app->layer_window.window);
		}

		if(app->navigation_window.window != NULL)
		{
			gtk_widget_destroy(app->navigation_window.window);
		}

		if(app->widgets->navi_layer_pane != NULL)
		{
			app->layer_window.pane_position = gtk_paned_get_position(
					GTK_PANED(app->widgets->navi_layer_pane));
			gtk_widget_destroy(app->widgets->navi_layer_pane);
		}

		app->widgets->navi_layer_pane = gtk_vpaned_new();
		gtk_paned_set_position(GTK_PANED(app->widgets->navi_layer_pane), app->layer_window.pane_position);
		InitializeNavigation(&app->navigation_window, app, app->widgets->navi_layer_pane);
		app->layer_window.window = CreateLayerWindow(app, app->widgets->window, &app->layer_window.view);

		box = gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane));
		if(box == NULL)
		{
			box = gtk_hbox_new(FALSE, 0);
			gtk_paned_pack1(GTK_PANED(app->widgets->left_pane), box, TRUE, FALSE);
			gtk_widget_show_all(box);
		}
		gtk_box_pack_start(GTK_BOX(box), app->widgets->navi_layer_pane, TRUE, TRUE, 0);
		gtk_box_reorder_child(GTK_BOX(box), app->widgets->navi_layer_pane, 0);

		gtk_widget_show_all(app->widgets->navi_layer_pane);

		box = gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		break;
	case UTILITY_PLACE_RIGHT:
		app->layer_window.flags |= LAYER_WINDOW_DOCKED | LAYER_WINDOW_PLACE_RIGHT;
		app->layer_window.flags &= ~(LAYER_WINDOW_POP_UP);

		if(app->layer_window.window != NULL)
		{
			gtk_widget_destroy(app->layer_window.window);
		}

		if(app->navigation_window.window != NULL)
		{
			gtk_widget_destroy(app->navigation_window.window);
		}

		if(app->widgets->navi_layer_pane != NULL)
		{
			app->layer_window.pane_position = gtk_paned_get_position(
					GTK_PANED(app->widgets->navi_layer_pane));
			gtk_widget_destroy(app->widgets->navi_layer_pane);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane));
		if(box == NULL)
		{
			box = gtk_hbox_new(FALSE, 0);
			gtk_paned_pack2(GTK_PANED(app->widgets->right_pane), box, TRUE, FALSE);
			gtk_widget_show_all(box);
		}

		app->widgets->navi_layer_pane = gtk_vpaned_new();
		gtk_paned_set_position(GTK_PANED(app->widgets->navi_layer_pane), app->layer_window.pane_position);
		InitializeNavigation(&app->navigation_window, app, app->widgets->navi_layer_pane);
		app->layer_window.window = CreateLayerWindow(app, app->widgets->window, &app->layer_window.view);

		gtk_box_pack_end(GTK_BOX(box), app->widgets->navi_layer_pane, TRUE, TRUE, 0);

		gtk_widget_show_all(app->widgets->navi_layer_pane);

		box = gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		break;
	case UTILITY_PLACE_POPUP_LEFT:
		app->layer_window.flags |= LAYER_WINDOW_POP_UP;
		app->layer_window.flags &= ~(LAYER_WINDOW_DOCKED | LAYER_WINDOW_PLACE_RIGHT);
		if(app->layer_window.window == NULL)
		{
			if(app->widgets->navi_layer_pane != NULL)
			{
				app->layer_window.pane_position = gtk_paned_get_position(
					GTK_PANED(app->widgets->navi_layer_pane));
				gtk_widget_destroy(app->widgets->navi_layer_pane);
				app->widgets->navi_layer_pane = NULL;
			}

			app->layer_window.window = CreateLayerWindow(app, app->widgets->window, &app->layer_window.view);
			gtk_widget_show_all(app->layer_window.window);
		}

		if(app->navigation_window.window == NULL)
		{
			InitializeNavigation(&app->navigation_window, app, NULL);
		}

		box = gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		break;
	case UTILITY_PLACE_POPUP_RIGHT:
		app->layer_window.flags |= LAYER_WINDOW_POP_UP | LAYER_WINDOW_PLACE_RIGHT;
		app->layer_window.flags &= ~(LAYER_WINDOW_DOCKED);
		if(app->layer_window.window == NULL)
		{
			if(app->widgets->navi_layer_pane != NULL)
			{
				app->layer_window.pane_position = gtk_paned_get_position(
					GTK_PANED(app->widgets->navi_layer_pane));
				gtk_widget_destroy(app->widgets->navi_layer_pane);
				app->widgets->navi_layer_pane = NULL;
			}

			app->layer_window.window = CreateLayerWindow(app, app->widgets->window, &app->layer_window.view);
			gtk_widget_show_all(app->layer_window.window);
		}

		if(app->navigation_window.window == NULL)
		{
			InitializeNavigation(&app->navigation_window, app, NULL);
		}

		box = gtk_paned_get_child1(GTK_PANED(app->widgets->left_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		box = gtk_paned_get_child2(GTK_PANED(app->widgets->right_pane));
		if(box != NULL)
		{
			GList *child = gtk_container_get_children(GTK_CONTAINER(box));
			if(child == NULL)
			{
				gtk_widget_destroy(box);
			}
			g_list_free(child);
		}

		break;
	}
}

/*********************************************************
* ExecuteChangeDisplayPreview�֐�                        *
* �v���r���[�̕\��/��\����؂�ւ���                    *
* ����                                                   *
* menu_item	: �\���؂�ւ����j���[�A�C�e���E�B�W�F�b�g   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteChangeDisplayPreview(GtkWidget* menu_item, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_DELETE_EVENT) != 0
		|| (app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) != 0)
	{
		return;
	}

	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)) == FALSE)
	{
		if(app->preview_window.window != NULL)
		{
			gtk_widget_destroy(app->preview_window.window);
			app->preview_window.window = NULL;
		}
	}
	else
	{
		if(app->preview_window.window == NULL)
		{
			InitializePreviewWindow(&app->preview_window, app);
			gtk_widget_show_all(app->preview_window.window);
		}
	}
}

/*********************************************************
* ExecuteSwitchFullScreen�֐�                            *
* �t���X�N���[���̐؂�ւ����s��                         *
* ����                                                   *
* menu_item	: �\���؂�ւ����j���[�A�C�e���E�B�W�F�b�g   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteSwitchFullScreen(GtkWidget* menu_item, APPLICATION* app)
{
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)) == FALSE)
	{
		gtk_window_unfullscreen(GTK_WINDOW(app->widgets->window));
	}
	else
	{
		gtk_window_fullscreen(GTK_WINDOW(app->widgets->window));
	}
}

/*********************************************************
* ExecuteMoveToolWindowTopLeft�֐�                       *
* �c�[���{�b�N�X�E�B���h�E������Ɉړ�����               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteMoveToolWindowTopLeft(APPLICATION* app)
{
	if(app->tool_window.window != NULL)
	{
		gtk_window_move(GTK_WINDOW(app->tool_window.window), 0, 0);
	}
}

/*********************************************************
* ExecuteMoveLayerWindowTopLeft�֐�                      *
* ���C���[�E�B���h�E������Ɉړ�����                     *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteMoveLayerWindowTopLeft(APPLICATION* app)
{
	if(app->layer_window.window != NULL)
	{
		gtk_window_move(GTK_WINDOW(app->layer_window.window), 0, 0);
	}
}

/*********************************************************
* ExecuteMoveNavigationWindowTopLeft�֐�                 *
* �i�r�Q�[�V�����E�B���h�E������Ɉړ�����               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
static void ExecuteMoveNavigationWindowTopLeft(APPLICATION* app)
{
	if(app->navigation_window.window != NULL)
	{
		gtk_window_move(GTK_WINDOW(app->navigation_window.window), 0, 0);
	}
}

#ifdef __cplusplus
}
#endif
