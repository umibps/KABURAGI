#ifndef _INCLUDED_UTILS_GTK_H_
#define _INCLUDED_UTILS_GTK_H_

#include <gtk/gtk.h>
#include "../../types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************
* FileRead関数                             *
* Gtkの関数を利用してファイル読み込み      *
* 引数                                     *
* dst			: 読み込み先のアドレス     *
* block_size	: 読み込むブロックのサイズ *
* read_num		: 読み込むブロックの数     *
* stream		: 読み込み元のストリーム   *
* 返り値                                   *
*	読み込んだブロック数                   *
*******************************************/
EXTERN size_t FileRead(void* dst, size_t block_size, size_t read_num, GFileInputStream* stream);

/*******************************************
* FileWrite関数                            *
* Gtkの関数を利用してファイル書き込み      *
* 引数                                     *
* src			: 書き込み元のアドレス     *
* block_size	: 書き込むブロックのサイズ *
* read_num		: 書き込むブロックの数     *
* stream		: 書き込み先のストリーム   *
* 返り値                                   *
*	書き込んだブロック数                   *
*******************************************/
EXTERN size_t FileWrite(void* src, size_t block_size, size_t read_num, GFileOutputStream* stream);

/********************************************
* FileSeek関数                              *
* Gtkの関数を利用してファイルシーク         *
* 引数                                      *
* stream	: シークを行うストリーム        *
* offset	: 移動バイト数                  *
* origin	: 移動開始位置(fseek関数と同等) *
* 返り値                                    *
*	正常終了(0), 異常終了(0以外)            *
********************************************/
EXTERN int FileSeek(void* stream, long offset, int origin);

/************************************************
* FileSeekTell関数                              *
* Gtkの関数を利用してファイルのシーク位置を返す *
* 引数                                          *
* stream	: シーク位置を調べるストリーム      *
* 返り値                                        *
*	シーク位置                                  *
************************************************/
EXTERN long FileSeekTell(void* stream);

EXTERN int ForFontFamilySearchCompare(const char* str, PangoFontFamily** font);

EXTERN void AdjustmentChangeValueCallBackInt(GtkAdjustment* adjustment, int* store);
EXTERN void AdjustmentChangeValueCallBackUint(GtkAdjustment* adjustment, unsigned int* store);
EXTERN void AdjustmentChangeValueCallBackInt8(GtkAdjustment* adjustment, int8* store);
EXTERN void AdjustmentChangeValueCallBackUint8(GtkAdjustment* adjustment, uint8* store);
EXTERN void AdjustmentChangeValueCallBackInt16(GtkAdjustment* adjustment, int16* store);
EXTERN void AdjustmentChangeValueCallBackUint16(GtkAdjustment* adjustment, uint16* store);
EXTERN void AdjustmentChangeValueCallBackInt32(GtkAdjustment* adjustment, int32* store);
EXTERN void AdjustmentChangeValueCallBackUint32(GtkAdjustment* adjustment, uint32* store);
EXTERN void AdjustmentChangeValueCallBackDouble(GtkAdjustment* adjustment, gdouble* value);
EXTERN void AdjustmentChangeValueCallBackDoubleRate(GtkAdjustment* adjustment, gdouble* value);
EXTERN void SetAdjustmentChangeValueCallBack(
	GtkAdjustment* adjustment,
	void (*func)(void*),
	void* func_data
);

EXTERN void CheckButtonSetFlagsCallBack(GtkWidget* button, unsigned int* flags, unsigned int flag_value);
EXTERN void UpdateWidget(GtkWidget* widget);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_UTILS_GTK_H_
