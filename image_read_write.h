#ifndef _INCLDUED_IMAGE_READ_WRITE_H_
#define _INCLDUED_IMAGE_READ_WRITE_H_

#include <stdio.h>
#include <gtk/gtk.h>
#include "application.h"
#include "memory_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eIMAGE_RW_FLAGS
{
	IMAGE_COMPRESS_PNG = 0x01	// PNG圧縮
} eIMAGE_RW_FRAGS;

typedef enum _eIMAGE_DATA_TYPE
{
	IMAGE_DATA_RGB,
	IMAGE_DATA_RGBA,
	IMAGE_DATA_CMYK
} eIMAGE_DATA_TYPE;

// 関数のプロトタイプ宣言
/***********************************************************
* ReadPNGStream関数                                        *
* PNGイメージデータを読み込む                              *
* 引数                                                     *
* stream	: データストリーム                             *
* read_func	: 読み込み用の関数ポインタ                     *
* width		: イメージの幅を格納するアドレス               *
* height	: イメージの高さを格納するアドレス             *
* stride	: イメージの一行分のバイト数を格納するアドレス *
* 返り値                                                   *
*	ピクセルデータ                                         *
***********************************************************/
extern uint8* ReadPNGStream(
	void* stream,
	stream_func read_func,
	gint32* width,
	gint32* height,
	gint32* stride
);

/**************************************************************************
* ReadPNGHeader関数                                                       *
* PNGイメージのヘッダ情報を読み込む                                       *
* 引数                                                                    *
* stream	: データストリーム                                            *
* read_func	: 読み込み用の関数ポインタ                                    *
* width		: イメージの幅を格納するアドレス                              *
* height	: イメージの高さを格納するアドレス                            *
* stride	: イメージの一行分のバイト数を格納するアドレス                *
* dpi		: イメージの解像度(DPI)を格納するアドレス                     *
* icc_profile_name	: ICCプロファイルの名前を受けるポインタ               *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ             *
* icc_profile_size	: ICCプロファイルのデータのバイト数を格納するアドレス *
**************************************************************************/
extern void ReadPNGHeader(
	void* stream,
	stream_func read_func,
	gint32* width,
	gint32* height,
	gint32* stride,
	int32* dpi,
	char** icc_profile_name,
	uint8** icc_profile_data,
	uint32* icc_profile_size
);

/**************************************************************************
* ReadPNGDetailData関数                                                   *
* PNGイメージデータを詳細情報付きで読み込む                               *
* 引数                                                                    *
* stream	: データストリーム                                            *
* read_func	: 読み込み用の関数ポインタ                                    *
* width		: イメージの幅を格納するアドレス                              *
* height	: イメージの高さを格納するアドレス                            *
* stride	: イメージの一行分のバイト数を格納するアドレス                *
* dpi		: イメージの解像度(DPI)を格納するアドレス                     *
* icc_profile_name	: ICCプロファイルの名前を受けるポインタ               *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ             *
* icc_profile_size	: ICCプロファイルのデータのバイト数を格納するアドレス *
* 返り値                                                                  *
*	ピクセルデータ                                                        *
**************************************************************************/
extern uint8* ReadPNGDetailData(
	void* stream,
	stream_func read_func,
	gint32* width,
	gint32* height,
	gint32* stride,
	int32* dpi,
	char** icc_profile_name,
	uint8** icc_profile_data,
	uint32* icc_profile_size
);

/*********************************************
* WritePNGStream関数                         *
* PNGのデータをストリームに書き込む          *
* 引数                                       *
* stream		: データストリーム           *
* write_func	: 書き込み用の関数ポインタ   *
* pixels		: ピクセルデータ             *
* width			: 画像の幅                   *
* height		: 画像の高さ                 *
* stride		: 画像データ一行分のバイト数 *
* channel		: 画像のチャンネル数         *
* interlace		: インターレースの有無       *
* compression	: 圧縮レベル                 *
*********************************************/
extern void WritePNGStream(
	void* stream,
	stream_func write_func,
	void (*flush_func)(void*),
	uint8* pixels,
	int32 width,
	int32 height,
	int32 stride,
	uint8 channel,
	int32 interlace,
	int32 compression
);

/********************************************************
* WritePNGDetailData関数                                *
* PNGのデータを詳細情報付きでストリームに書き込む       *
* 引数                                                  *
* stream			: データストリーム                  *
* write_func		: 書き込み用の関数ポインタ          *
* pixels			: ピクセルデータ                    *
* width				: 画像の幅                          *
* height			: 画像の高さ                        *
* stride			: 画像データ一行分のバイト数        *
* channel			: 画像のチャンネル数                *
* interlace			: インターレースの有無              *
* compression		: 圧縮レベル                        *
* dpi				: 解像度(DPI)                       *
* icc_profile_name	: ICCプロファイルの名前             *
* icc_profile_data	: ICCプロファイルのデータ           *
* icc_profile_data	: ICCプロファイルのデータのバイト数 *
********************************************************/
extern void WritePNGDetailData(
	void* stream,
	stream_func write_func,
	void (*flush_func)(void*),
	uint8* pixels,
	int32 width,
	int32 height,
	int32 stride,
	uint8 channel,
	int32 interlace,
	int32 compression,
	int32 dpi,
	char* icc_profile_name,
	uint8* icc_profile_data,
	uint32 icc_profile_size
);

/********************************************************************
* ReadJpegHeader関数                                                *
* JPEGのヘッダ情報を読み込む                                        *
* 引数                                                              *
* system_file_path	: OSのファイルシステムに則したファイルパス      *
* width				: 画像の幅データの読み込み先                    *
* height			: 画像の高さデータの読み込み先                  *
* resolution		: 画像の解像度データの読み込み先                *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ       *
* icc_profile_size	: ICCプロファイルのデータのバイト数の読み込み先 *
********************************************************************/
extern void ReadJpegHeader(
	const char* system_file_path,
	int* width,
	int* height,
	int* resolution,
	uint8** icc_profile_data,
	int32* icc_profile_size
);

/****************************************************************
* WriteJpegFile関数                                             *
* JPEG形式で画像データを書き出す                                *
* 引数                                                          *
* system_file_path	: OSのファイルシステムに則したファイルパス  *
* pixels			: ピクセルデータ                            *
* width				: 画像データの幅                            *
* height			: 画像データの高さ                          *
* channel			: 画像のチャンネル数                        *
* data_type			: ピクセルデータの形式(RGB or CMYK)         *
* quality			: JPEG画像の画質                            *
* optimize_coding	: 最適化エンコードの有無                    *
* icc_profile_data	: 埋め込むICCプロファイルのデータ           *
*						(埋め込まない時はNULL)                  *
* icc_profile_size	: 埋め込むICCプロファイルのデータのバイト数 *
* resolution		: 解像度(dpi)                               *
****************************************************************/
extern void WriteJpegFile(
	const char* system_file_path,
	uint8* pixels,
	int width,
	int height,
	int channel,
	eIMAGE_DATA_TYPE data_type,
	int quality,
	int optimize_coding,
	void* icc_profile_data,
	int icc_profile_size,
	int resolution
);

/***********************************************************
* ReadOriginalFormat関数                                   *
* 独自形式のデータを読み込む                               *
* 引数                                                     *
* stram			: 読み込み元のアドレス                     *
* read_func		: 読み込み用の関数ポインタ                 *
* stream_size	: データの総バイト数                       *
* app			: アプリケーションを管理する構造体アドレス *
* data_name		: ファイル名                               *
* 返り値                                                   *
*	描画領域のデータ                                       *
***********************************************************/
extern DRAW_WINDOW* ReadOriginalFormat(
	void* stream,
	stream_func read_func,
	size_t stream_size,
	APPLICATION* app,
	const char* data_name
);

/***********************************************************
* ReadOriginalFormatMixedData関数                          *
* 独自形式の合成データを作成して返す                       *
* 引数                                                     *
* stram			: 読み込み元のアドレス                     *
* read_func		: 読み込み用の関数ポインタ                 *
* stream_size	: データの総バイト数                       *
* app			: アプリケーションを管理する構造体アドレス *
* data_name		: ファイル名                               *
* 返り値                                                   *
*	合成したレイヤー                                       *
***********************************************************/
extern LAYER* ReadOriginalFormatMixedData(
	void* stream,
	stream_func read_func,
	size_t stream_size,
	APPLICATION* app,
	const char* data_name
);

/*******************************************************************
* ReadOriginalFormatMemoryStream関数                               *
* メモリストリームの独自フォーマットのデータを読み込む(アンドゥ用) *
* 引数                                                             *
* window	: データの読み込み先                                   *
* stream	: メモリストリーム                                     *
*******************************************************************/
extern void ReadOriginalFormatMemoryStream(
	DRAW_WINDOW* window,
	MEMORY_STREAM_PTR stream
);

/*******************************************
* WriteOriginalFormat関数                  *
* 独自形式のデータを生成する               *
* 引数                                     *
* stream		: 書き込み先のストリーム   *
* write_func	: 書き込み用の関数ポインタ *
* window		: 描画領域の情報           *
* add_thumbnail	: サムネイルの有無         *
* compress		: 圧縮率                   *
*******************************************/
extern void WriteOriginalFormat(
	void* stream,
	stream_func write_func,
	DRAW_WINDOW* window,
	int add_thumbnail,
	int compress
);

/*************************************************
* WritePhotoShopDocument関数                     *
* PSD形式で書きだす                              *
* 引数                                           *
* stream		: 書き込み先のストリーム         *
* write_func	: 書き込み用の関数ポインタ       *
* seek_func		: シーク用の関数ポインタ         *
* tell_func		: シーク位置取得用の関数ポインタ *
* window		: 描画領域の情報                 *
*************************************************/
extern void WritePhotoShopDocument(
	void* stream,
	stream_func write_func,
	seek_func seek_func,
	long (*tell_func)(void*),
	DRAW_WINDOW* window
);

/**************************************************************
* ReadTiffTagData関数                                         *
* TIFFファイルのタグ情報を読み込む                            *
* 引数                                                        *
* system_file_path	: 実行OSでのファイルパス                  *
* resolution		: 解像度(DPI)を格納するアドレス           *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ *
* icc_profile_size	: ICCプロファイルのデータのバイト数       *
**************************************************************/
extern void ReadTiffTagData(
	const char* system_file_path,
	int* resolution,
	uint8** icc_profile_data,
	int32* icc_profile_size
);

/*******************************************************
* WriteTiff関数                                        *
* TIFF形式で画像を書き出す                             *
* 引数                                                 *
* system_file_path	: OSのファイルシステムに即したパス *
* window			: 画像を書き出す描画領域の情報     *
* write_aplha		: 透明情報を書き出すか否か         *
* compress			: 圧縮を行うか否か                 *
* profile_data		: ICCプロファイルのデータ          *
*					  (書き出さない場合はNULL)         *
* profile_data_size	: ICCプロファイルのデータバイト数  *
*******************************************************/
extern void WriteTiff(
	const char* system_file_path,
	DRAW_WINDOW* window,
	gboolean write_alpha,
	gboolean compress,
	void* profile_data,
	guint32 profile_data_size
);

/*******************************************************
* SetFileChooserPreview関数                            *
* ファイル選択ダイアログにプレビューウィジェットを設定 *
* 引数                                                 *
* file_chooser	: ファイル選択ダイアログウィジェット   *
*******************************************************/
extern void SetFileChooserPreview(GtkWidget *file_chooser);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLDUED_IMAGE_READ_WRITE_H_
