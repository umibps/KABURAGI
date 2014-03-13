#ifndef _INCLUDED_CLIP_BOARD_H_
#define _INCLUDED_CLIP_BOARD_H_

#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************
* ExecutePaste関数                                   *
* 貼り付けを実行する                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecutePaste(APPLICATION* app);

/*****************************************************
* ExecuteCopy関数                                    *
* コピーを実行                                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteCopy(APPLICATION* app);

/*****************************************************
* ExecuteCopyVisible関数                             *
* 可視部コピーを実行                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteCopyVisible(APPLICATION* app);

/*****************************************************
* ExecuteCut関数                                     *
* 切り取りを実行                                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteCut(APPLICATION* app);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_CLIP_BOARD_H_
