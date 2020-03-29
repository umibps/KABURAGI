#ifndef _INCLUDED_INPUT_GTK_H_
#define _INCLUDED_INPUT_GTK_H_

#include <gtk/gtk.h>
#include "../../configure.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************
* ButtonPressEvent�֐�                         *
* �}�E�X�N���b�N�̏���                         *
* ����                                         *
* widget	: �}�E�X�N���b�N���ꂽ�E�B�W�F�b�g *
* event		: �}�E�X�̏��                     *
* window	: �`��̈�̏��                   *
* �Ԃ�l                                       *
*	���TRUE                                   *
***********************************************/
extern gboolean ButtonPressEvent(GtkWidget *widget, GdkEventButton *event, struct _DRAW_WINDOW* window);

/*************************************
* MotionNotifyEvent�֐�              *
* �}�E�X�I�[�o�[�̏���               *
* ����                               *
* widget	: �`��̈�̃E�B�W�F�b�g *
* event		: �}�E�X�̏��           *
* window	: �`��̈�̏��         *
* �Ԃ�l                             *
*	���TRUE                         *
*************************************/
extern gboolean MotionNotifyEvent(GtkWidget *widget, GdkEventMotion *event, DRAW_WINDOW* window);

extern gboolean ButtonReleaseEvent(GtkWidget *widget, GdkEventButton *event, DRAW_WINDOW* window);

extern gboolean MouseWheelEvent(GtkWidget*widget, GdkEventScroll* event_info, DRAW_WINDOW* window);

#if GTK_MAJOR_VERSION >= 3
/***************************************
* TouchEvent�֐�                       *
* �^�b�`���쎞�̃R�[���o�b�N�֐�       *
* ����                                 *
* widget		: �`��̈�E�B�W�F�b�g *
* event_info	: �C�x���g�̓��e       *
* window		: �`��̈�̏��       *
* �Ԃ�l                               *
*	���FALSE                          *
***************************************/
extern gboolean TouchEvent(GtkWidget* widget, GdkEventTouch* event_info, DRAW_WINDOW* window);
#endif

/***************************************************************
* KeyPressEvent�֐�                                            *
* �L�[�{�[�h�������ꂽ���ɌĂяo�����R�[���o�b�N�֐�         *
* ����                                                         *
* widget	: �L�[�{�[�h�������ꂽ���ɃA�N�e�B�u�ȃE�B�W�F�b�g *
* event		: �L�[�{�[�h�̏��                                 *
* data		: �A�v���P�[�V�����̏��                           *
* �Ԃ�l                                                       *
*	���FALSE                                                  *
***************************************************************/
extern gboolean KeyPressEvent(
	GtkWidget *widget,
	GdkEventKey *event,
	void *data
);

/*********************************************************
* EnterNotifyEvent�֐�                                   *
* �`��̈���Ƀ}�E�X�J�[�\�����������ۂɌĂяo�����֐� *
* ����                                                   *
* widget		: �`��̈�̃E�B�W�F�b�g                 *
* event_info	: �C�x���g�̏��                         *
* window		: �`��̈�̏��                         *
* �Ԃ�l                                                 *
*	���FALSE                                            *
*********************************************************/
extern gboolean EnterNotifyEvent(GtkWidget*widget, GdkEventCrossing* event_info, DRAW_WINDOW* window);

/*************************************************************
* LeaveNotifyEvent�֐�                                       *
* �`��̈�O�Ƀ}�E�X�J�[�\�����o�čs�����ۂɌĂяo�����֐� *
* ����                                                       *
* widget		: �`��̈�̃E�B�W�F�b�g                     *
* event_info	: �C�x���g�̏��                             *
* window		: �`��̈�̏��                             *
*************************************************************/
extern gboolean LeaveNotifyEvent(GtkWidget* widget, GdkEventCrossing* event_info, DRAW_WINDOW* window);

/***********************************************
* CustomButtonPressEvent�֐�                   *
* �}�E�X�N���b�N�̏����J�X�^����               *
* ����                                         *
* widget	: �}�E�X�N���b�N���ꂽ�E�B�W�F�b�g *
* event		: �}�E�X�̏��                     *
* window	: �`��̈�̏��                   *
* �Ԃ�l                                       *
*	���TRUE                                   *
***********************************************/
extern gboolean CustomButtonPressEvent(GtkWidget *widget, GdkEventButton *event, DRAW_WINDOW* window);

/*************************************
* CustomMotionNotifyEvent�֐�        *
* �}�E�X�I�[�o�[�̏����J�X�^����     *
* ����                               *
* widget	: �`��̈�̃E�B�W�F�b�g *
* event		: �}�E�X�̏��           *
* window	: �`��̈�̏��         *
* �Ԃ�l                             *
*	���TRUE                         *
*************************************/
extern gboolean CustomMotionNotifyEvent(GtkWidget *widget, GdkEventMotion *event, DRAW_WINDOW* window);

extern gboolean CustomButtonReleaseEvent(GtkWidget *widget, GdkEventButton *event, DRAW_WINDOW* window);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_INPUT_GTK_H_
