#ifndef _INCLUDED_GTK_WIDGETS_H_
#define _INCLUDED_GTK_WIDGETS_H_

#include <gtk/gtk.h>

typedef struct _MAIN_WINDOW_WIDGETS
{
	// �E�B���h�E�A���j���[�A�^�u������p�b�L���O�{�b�N�X�A�^�u
	GtkWidget *window, *vbox, *note_book;
	// ���j���[�o�[
	GtkWidget *menu_bar;
	// �X�e�[�^�X�o�[
	GtkWidget *status_bar;
	// �ۑ��A�t�B���^�[���̐i���󋵂�\���v���O���X�o�[
	GtkWidget *progress;
	// �V���O���E�B���h�E�p�Ƀc�[�������鍶�E�̃y�[��
	GtkWidget *left_pane, *right_pane;
	// �i�r�Q�[�V�����ƃ��C���[�r���[���h�b�L���O���邽�߂̃{�b�N�X
	GtkWidget *navi_layer_pane;
	// ���E���]�̃{�^���ƃ��x��
	GtkWidget *reverse_button, *reverse_label;
	// �I��͈͕ҏW�̃{�^��
	GtkWidget *edit_selection;
	// �g�p�e�N�X�`���̃��x��
	GtkWidget *texture_label;
	// �V���[�g�J�b�g�L�[
	GtkAccelGroup *hot_key;
} MAIN_WINDOW_WIDGETS;

#endif	// #ifndef _INCLUDED_GTK_WIDGETS_H_
