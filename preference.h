#ifndef _INCLUDED_PREFERENCE_H_
#define _INCLUDED_PREFERENCE_H_

#include "ini_file.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************
* PREFERENCE構造体 *
* 環境設定を格納   *
*******************/
typedef struct _PREFERENCE
{
	// 現在設定中の項目
	int16 current_setting;
	// ZIP圧縮率
	int8 compress;
	// 自動保存の有無
	int8 auto_save;
	// テーマファイル名
	char *theme;
	// 自動保存の間隔
	int32 auto_save_time;
	// キャンバスの背景色
	uint8 canvas_back_ground[3];
} PREFERENCE;

// 関数のプロトタイプ宣言
/*************************************************************
* ReadPreference関数                                         *
* 環境設定を読み込む                                         *
* 引数                                                       *
* file			: 初期化ファイル読み込み用の構造体のアドレス *
* preference	: 環境設定を管理する構造体のアドレス         *
*************************************************************/
extern void ReadPreference(INI_FILE_PTR file, PREFERENCE* preference);

/*************************************************************
* WritePreference関数                                        *
* 環境設定を書き込む                                         *
* 引数                                                       *
* file			: 初期化ファイル書き込み用の構造体のアドレス *
* preference	: 環境設定を管理する構造体のアドレス         *
*************************************************************/
extern void WritePreference(INI_FILE_PTR file, PREFERENCE* preference);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_PREFERENCE_H_
