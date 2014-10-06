#ifndef _INCLUDED_INPUT_H_
#define _INCLUDED_INPUT_H_

#include <gtk/gtk.h>
#include "configure.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************
* ButtonPressEvent関数                         *
* マウスクリックの処理                         *
* 引数                                         *
* widget	: マウスクリックされたウィジェット *
* event		: マウスの情報                     *
* window	: 描画領域の情報                   *
* 返り値                                       *
*	常にTRUE                                   *
***********************************************/
extern gboolean ButtonPressEvent(GtkWidget *widget, GdkEventButton *event, struct _DRAW_WINDOW* window);

/*************************************
* MotionNotifyEvent関数              *
* マウスオーバーの処理               *
* 引数                               *
* widget	: 描画領域のウィジェット *
* event		: マウスの情報           *
* window	: 描画領域の情報         *
* 返り値                             *
*	常にTRUE                         *
*************************************/
extern gboolean MotionNotifyEvent(GtkWidget *widget, GdkEventMotion *event, DRAW_WINDOW* window);

extern gboolean ButtonReleaseEvent(GtkWidget *widget, GdkEventButton *event, DRAW_WINDOW* window);

extern gboolean MouseWheelEvent(GtkWidget*widget, GdkEventScroll* event_info, DRAW_WINDOW* window);

#if GTK_MAJOR_VERSION >= 3
/***************************************
* TouchEvent関数                       *
* タッチ操作時のコールバック関数       *
* 引数                                 *
* widget		: 描画領域ウィジェット *
* event_info	: イベントの内容       *
* window		: 描画領域の情報       *
* 返り値                               *
*	常にFALSE                          *
***************************************/
extern gboolean TouchEvent(GtkWidget* widget, GdkEventTouch* event_info, DRAW_WINDOW* window);
#endif

/***************************************************************
* KeyPressEvent関数                                            *
* キーボードが押された時に呼び出されるコールバック関数         *
* 引数                                                         *
* widget	: キーボードが押された時にアクティブなウィジェット *
* event		: キーボードの情報                                 *
* data		: アプリケーションの情報                           *
* 返り値                                                       *
*	常にFALSE                                                  *
***************************************************************/
extern gboolean KeyPressEvent(
	GtkWidget *widget,
	GdkEventKey *event,
	void *data
);

/*********************************************************
* EnterNotifyEvent関数                                   *
* 描画領域内にマウスカーソルが入った際に呼び出される関数 *
* 引数                                                   *
* widget		: 描画領域のウィジェット                 *
* event_info	: イベントの情報                         *
* window		: 描画領域の情報                         *
* 返り値                                                 *
*	常にFALSE                                            *
*********************************************************/
extern gboolean EnterNotifyEvent(GtkWidget*widget, GdkEventCrossing* event_info, DRAW_WINDOW* window);

/*************************************************************
* LeaveNotifyEvent関数                                       *
* 描画領域外にマウスカーソルが出て行った際に呼び出される関数 *
* 引数                                                       *
* widget		: 描画領域のウィジェット                     *
* event_info	: イベントの情報                             *
* window		: 描画領域の情報                             *
*************************************************************/
extern gboolean LeaveNotifyEvent(GtkWidget* widget, GdkEventCrossing* event_info, DRAW_WINDOW* window);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_INPUT_H_
