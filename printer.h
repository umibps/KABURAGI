#ifndef _INCLUDED_PRINTER_H_
#define _INCLUDED_PRINTER_H_

#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

// 関数のプロトタイプ宣言
/*****************************************************
* ExecutePrint関数                                   *
* 印刷を実行                                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecutePrint(APPLICATION* app);

#ifdef __cplusplus
}
#endif

#endif	