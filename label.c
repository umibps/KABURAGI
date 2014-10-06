// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <gtk/gtk.h>
#include "ini_file.h"
#include "label.h"

/*
* LoadLabel関数
* アプリケーションで使うラベルの文字データを読み込む
* label				: ラベルの文字データを記憶する構造体
* land_file_path	: 文字データの入ったファイルのパス
*/
void LoadLabel(LABEL* label, const char* lang_file_path)
{
	// .ini形式のファイル読み込み用
	INI_FILE *file;
	// ファイルアクセス用
	FILE *fp;
	// ファイルのバイト数
	size_t data_size;
	// ファイルの文字コード
	char code[512];
	// ラベルの文字列
	char str[2048];
	// 文字列の長さ
	size_t length;

	// ファイルオープン
	fp = fopen(lang_file_path, "rb");
	if(fp == NULL)
	{
		return;
	}

	// バイト数を取得
	(void)fseek(fp, 0, SEEK_END);
	data_size = ftell(fp);
	// シーク位置を元に戻す
	rewind(fp);

	// .ini形式ファイル読み込みデータ作成
	file = CreateIniFile((void*)fp, (size_t (*)(void*, size_t, size_t, void*))fread,
		data_size, INI_READ);

	// 文字コードを取得
	(void)IniFileGetString(file, "CODE", "CODE", code, 512);

	// メニュー
	length = IniFileGetString(file, "MENU", "FILE", str, 2048);
	label->menu.file = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "MENU", "OPEN", str, 2048);
	label->menu.open = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "MENU", "FILE_OPEN", str, 2048);
	label->menu.file_open = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "MENU", "DIRECTORY_OPEN", str, 2048);
	label->menu.dir_open = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);

	// ファイルリストビュー
	length = IniFileGetString(file, "LIST_VIEW", "TITLE", str, 2048);
	label->list_view.title = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "LIST_VIEW", "ARTIST", str, 2048);
	label->list_view.artist = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "LIST_VIEW", "ALBUM", str, 2048);
	label->list_view.album = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "LIST_VIEW", "DURATION", str, 2048);
	label->list_view.duration = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);

	(void)fclose(fp);
	file->delete_func(file);
}
