#ifndef _INCLUDED_DISPLAY_H_
#define _INCLUDED_DISPLAY_H_

#include <gtk/gtk.h>
#include "layer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************
* DisplayDrawWindow関数                  *
* 描画領域の画面更新処理                 *
* 引数                                   *
* widget		: 描画領域のウィジェット *
* event_info	: 描画更新の情報         *
* window		: 描画領域の情報         *
* 返り値                                 *
*	常にFALSE                            *
*****************************************/
extern gboolean DisplayDrawWindow(
	GtkWidget *widget,
	GdkEventExpose *event_info,
	struct _DRAW_WINDOW *window
);

/*****************************
* UpdateDrawWindow関数       *
* 描画領域の更新処理         *
* 引数                       *
* window	: 描画領域の情報 *
*****************************/
extern void UpdateDrawWindow(struct _DRAW_WINDOW *window);

/*******************************************************
* MixLayerForSave関数                                  *
* 保存するために背景ピクセルデータ無しでレイヤーを合成 *
* 引数                                                 *
* window	: 描画領域の情報                           *
* 返り値                                               *
*	合成したレイヤーのデータ                           *
*******************************************************/
extern LAYER* MixLayerForSave(DRAW_WINDOW* window);

/*******************************************************
* MixLayerForSaveWithBackGround関数                    *
* 保存するために背景ピクセルデータ有りでレイヤーを合成 *
* 引数                                                 *
* window	: 描画領域の情報                           *
* 返り値                                               *
*	合成したレイヤーのデータ                           *
*******************************************************/
extern LAYER* MixLayerForSaveWithBackGround(DRAW_WINDOW* window);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_DISPLAY_H_
