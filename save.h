#ifndef _INCLDUED_SAVE_H_
#define _INCLDUED_SAVE_H_

#include "draw_window.h"

#ifdef __cplusplus
extern "C" {
#endif

// 関数のプロトタイプ宣言
/***************************************************
* IsSupportFileType関数                            *
* 書き出しをサポートしているファイルタイプかを判定 *
* 引数                                             *
* extention	: 拡張子の文字列                       *
* 返り値                                           *
*	サポートしている:1	サポートしていない:0       *
***************************************************/
extern int IsSupportFileType(const char* extention);

extern void SaveAsPng(struct _APPLICATION* app, DRAW_WINDOW* window, const gchar* file_name);
extern void SaveAsJpeg(struct _APPLICATION* app, DRAW_WINDOW* window, const gchar* file_name);

/*****************************************
* SaveAsOriginalFormat関数               *
* 独自形式でデータを書き出す             *
* 引数                                   *
* app		: アプリケーション全体の情報 *
* window	: 描画領域の情報             *
* file_name	: 書きだすファイルパス       *
*****************************************/
extern void SaveAsOriginalFormat(APPLICATION* app, DRAW_WINDOW* window, const gchar* file_name);

/*****************************************
* SaveAsPhotoShopDocument関数            *
* PSD形式でデータを書き出す              *
* 引数                                   *
* app		: アプリケーション全体の情報 *
* window	: 描画領域の情報             *
* file_name	: 書きだすファイルパス       *
*****************************************/
extern void SaveAsPhotoShopDocument(APPLICATION* app, DRAW_WINDOW* window, const gchar* file_name);

/*********************************************************
* Save関数                                               *
* 書き出し実行                                           *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* window	: 描画領域の情報                             *
* file_name	: 保存するファイルパス                       *
* file_type	: 保存するファイル形式の拡張子               *
* 返り値                                                 *
*	書き出したファイルのパス                             *
*********************************************************/
extern const char* Save(
	APPLICATION* app,
	DRAW_WINDOW* window,
	const gchar* file_name,
	const char* file_type
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLDUED_SAVE_H_
