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
EXTERN void ExecutePaste(APPLICATION* app);

/*****************************************************
* ExecuteCopy関数                                    *
* コピーを実行                                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteCopy(APPLICATION* app);

/*****************************************************
* ExecuteCopyVisible関数                             *
* 可視部コピーを実行                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteCopyVisible(APPLICATION* app);

/*****************************************************
* ExecuteCut関数                                     *
* 切り取りを実行                                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteCut(APPLICATION* app);

/*****************************************************
* ExecuteDelete関数                                  *
* 削除を実行                                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteDelete(APPLICATION* app);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_CLIP_BOARD_H_
