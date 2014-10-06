#ifndef _INCLUDED_LABEL_H_
#define _INCLUDED_LABEL_H_

typedef struct _LABEL
{
	struct
	{
		char *file, *open, *file_open, *dir_open;
	} menu;

	struct
	{
		char *title, *artist, *album, *duration;
	} list_view;
} LABEL;

/*
* LoadLabel関数
* アプリケーションで使うラベルの文字データを読み込む
* label				: ラベルの文字データを記憶する構造体
* land_file_path	: 文字データの入ったファイルのパス
*/
extern void LoadLabel(LABEL* label, const char* lang_file_path);

#endif	// #ifndef _INCLUDED_LABEL_H_
