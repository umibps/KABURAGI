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
/*****************************************************************
* DecodeImageData関数                                            *
* メモリ中の画像データをデコードする                             *
* 引数                                                           *
* data				: 画像データ                                 *
* data_size			: 画像データのバイト数                       *
* file_type			: 画像のタイプを表す文字列(例:PNGなら".png") *
* width				: 画像の幅の格納先                           *
* height			: 画像の高さの格納先                         *
* channel			: 画像チャンネル数の格納先                   *
* resolution		: 画像解像度の格納先                         *
* icc_profile_name	: ICCプロファイルの名前の格納先              *
* icc_profile_data	: ICCプロファイルのデータの格納先            *
* icc_profile_size	: ICCプロファイルのデータサイズの格納先      *
* 返り値                                                         *
*	デコードしたピクセルデータ                                   *
*****************************************************************/
EXTERN uint8* DecodeImageData(
	uint8* data,
	size_t data_size,
	const char* file_type,
	int* width,
	int* height,
	int* channel,
	int* resolution,
	char** icc_profile_name,
	uint8** icc_profile_data,
	uint32* icc_profile_size
);

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
EXTERN uint8* ReadPNGStream(
	void* stream,
	stream_func_t read_func,
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
EXTERN void ReadPNGHeader(
	void* stream,
	stream_func_t read_func,
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
EXTERN uint8* ReadPNGDetailData(
	void* stream,
	stream_func_t read_func,
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
EXTERN void WritePNGStream(
	void* stream,
	stream_func_t write_func,
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
EXTERN void WritePNGDetailData(
	void* stream,
	stream_func_t write_func,
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
EXTERN void ReadJpegHeader(
	const char* system_file_path,
	int* width,
	int* height,
	int* resolution,
	uint8** icc_profile_data,
	int32* icc_profile_size
);

/********************************************************************
* ReadJpegStream関数                                                *
* JPEGのデータを読み込み、デコードする                              *
* 引数                                                              *
* stream			: 読み込み元                                    *
* read_func			: 読み込みに使う関数ポインタ                    *
* data_size			: JPEGのデータサイズ                            *
* width				: 画像の幅データの読み込み先                    *
* height			: 画像の高さデータの読み込み先                  *
* channel			: 画像データのチャンネル数                      *
* resolution		: 画像の解像度データの読み込み先                *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ       *
* icc_profile_size	: ICCプロファイルのデータのバイト数の読み込み先 *
* 返り値                                                            *
*	デコードしたピクセルデータ(失敗したらNULL)                      *
********************************************************************/
EXTERN uint8* ReadJpegStream(
	void* stream,
	stream_func_t read_func,
	size_t data_size,
	int* width,
	int* height,
	int* channel,
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
EXTERN void WriteJpegFile(
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
EXTERN DRAW_WINDOW* ReadOriginalFormat(
	void* stream,
	stream_func_t read_func,
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
EXTERN LAYER* ReadOriginalFormatMixedData(
	void* stream,
	stream_func_t read_func,
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
EXTERN void ReadOriginalFormatMemoryStream(
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
EXTERN void WriteOriginalFormat(
	void* stream,
	stream_func_t write_func,
	DRAW_WINDOW* window,
	int add_thumbnail,
	int compress
);

/*************************************************
* ReadPhotoShopDocument関数                      *
* PSD形式を読み込む                              *
* 引数                                           *
* stream		: 書き込み先のストリーム         *
* write_func	: 書き込み用の関数ポインタ       *
* seek_func		: シーク用の関数ポインタ         *
* tell_func		: シーク位置取得用の関数ポインタ *
* 返り値                                         *
*	正常終了時:一番下のレイヤー, 異常終了時:NULL *
*************************************************/
EXTERN LAYER* ReadPhotoShopDocument(
	void* stream,
	stream_func_t read_func,
	seek_func_t seek_func,
	long (*tell_func)(void*)
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
EXTERN void WritePhotoShopDocument(
	void* stream,
	stream_func_t write_func,
	seek_func_t seek_func,
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
EXTERN void ReadTiffTagData(
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
EXTERN void WriteTiff(
	const char* system_file_path,
	DRAW_WINDOW* window,
	gboolean write_alpha,
	gboolean compress,
	void* profile_data,
	guint32 profile_data_size
);

/**********************************************
* ReadTgaStream関数                           *
* TGAファイルの読み込み(必要に応じてデコード) *
* 引数                                        *
* stream	: 読み込み元                      *
* read_func	: 読み込みに使う関数ポインタ      *
* data_size	: 画像データのバイト数            *
* width		: 画像の幅                        *
* height	: 画像の高さ                      *
* channel	: 画像のチャンネル数              *
* 返り値                                      *
*	ピクセルデータ(失敗時はNULL)              *
**********************************************/
EXTERN uint8* ReadTgaStream(
	void* stream,
	stream_func_t read_func,
	size_t data_size,
	int* width,
	int* height,
	int* channel
);

/*******************************************************
* ReadBmpStream関数                                    *
* BMPファイルのピクセルデータを読み込む                *
* 引数                                                 *
* stream		: 画像データのストリーム               *
* read_func		: ストリーム読み込み用の関数ポインタ   *
* seek_func		: ストリームのシークに使う関数ポインタ *
* data_size		: 画像データのバイト数                 *
* width			: 画像の幅の格納先                     *
* height		: 画像の高さの格納先                   *
* channel		: 画像のチャンネル数の格納先           *
* resolution	: 画像の解像度(dpi)の格納先            *
* 返り値                                               *
*	ピクセルデータ(失敗時はNULL)                       *
*******************************************************/
EXTERN uint8* ReadBmpStream(
	void* stream,
	stream_func_t read_func,
	seek_func_t seek_func,
	size_t data_size,
	int* width,
	int* height,
	int* channel,
	int* resolution
);

/*****************************************
* ReadDdsStream関数                      *
* DDSファイルの読み込み                  *
* 引数                                   *
* stream	: 読み込み元                 *
* read_func	: 読み込みに使う関数ポインタ *
* data_size	: 画像データのバイト数       *
* width		: 画像の幅                   *
* height	: 画像の高さ                 *
* channel	: 画像のチャンネル数         *
* 返り値                                 *
*	ピクセルデータ(失敗時はNULL)         *
*****************************************/
EXTERN uint8* ReadDdsStream(
	void* stream,
	stream_func_t read_func,
	seek_func_t seek_func,
	size_t data_size,
	int* width,
	int* height,
	int* channel
);

/*******************************************************
* SetFileChooserPreview関数                            *
* ファイル選択ダイアログにプレビューウィジェットを設定 *
* 引数                                                 *
* file_chooser	: ファイル選択ダイアログウィジェット   *
*******************************************************/
EXTERN void SetFileChooserPreview(GtkWidget *file_chooser);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLDUED_IMAGE_READ_WRITE_H_
