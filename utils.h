#ifndef _INCLUDED_UTILS_H_
#define _INCLUDED_UTILS_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************
* StringCompareIgnoreCase関数         *
* 大文字/小文字を無視して文字列を比較 *
* 引数                                *
* str1	: 比較文字列1                 *
* str2	: 比較文字列2                 *
* 返り値                              *
*	文字列の差(同文字列なら0)         *
**************************************/
extern int StringCompareIgnoreCase(const char* str1, const char* str2);

/**************************************
* memset32関数                        *
* 32bit符号なし整数でバッファを埋める *
* 引数                                *
* buff	: 埋める対象のバッファ        *
* value	: 埋める値                    *
* size	: 埋めるバイト数              *
**************************************/
extern void memset32(void* buff, uint32 value, size_t size);

/**************************************
* memset64関数                        *
* 64bit符号なし整数でバッファを埋める *
* 引数                                *
* buff	: 埋める対象のバッファ        *
* value	: 埋める値                    *
* size	: 埋めるバイト数              *
**************************************/
extern void memset64(void* buff, uint64 value, size_t size);

#ifdef _MSC_VER
extern int strncasecmp(const char* s1, const char* s2, size_t n);
#endif

/**************************************************************
* StringStringIgnoreCase関数                                  *
* 大文字/小文字を無視して引数1の文字列から引数2の文字列を探す *
* str		: 検索対象の文字列                                *
* search	: 検索する文字列                                  *
* 返り値                                                      *
*	文字列発見:発見した位置のポインタ	見つからない:NULL     *
**************************************************************/
extern char* StringStringIgnoreCase(const char* str, const char* search);

/***************************************
* StringReplace関数                    *
* 指定した文字列を置換する             *
* 引数                                 *
* str			: 処理をする文字列     *
* replace_from	: 置き換えられる文字列 *
* replace_to	: 置き換える文字列     *
***************************************/
extern void StringRepalce(
	char* str,
	char* replace_from,
	char* replace_to
);

extern int ForFontFamilySearchCompare(const char* str, PangoFontFamily** font);

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
extern size_t FileRead(void* dst, size_t block_size, size_t read_num, GFileInputStream* stream);

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
extern size_t FileWrite(void* src, size_t block_size, size_t read_num, GFileOutputStream* stream);

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
extern int FileSeek(void* stream, long offset, int origin);

/************************************************
* FileSeekTell関数                              *
* Gtkの関数を利用してファイルのシーク位置を返す *
* 引数                                          *
* stream	: シーク位置を調べるストリーム      *
* 返り値                                        *
*	シーク位置                                  *
************************************************/
extern long FileSeekTell(void* stream);

/***********************************************
* InvertMatrix関数                             *
* 逆行列を計算する                             *
* 引数                                         *
* a	: 計算対象の行列(逆行列データはここに入る) *
* n	: 行列の次数                               *
***********************************************/
extern void InvertMatrix(FLOAT_T **a, int n);

/*********************************
* FLAG_CHECKマクロ関数           *
* フラグのON/OFFを判定する       *
* 引数                           *
* FLAGS	: フラグ配列             *
* ID	: チェックするフラグのID *
* 返り値                         *
*	ON:0以外 OFF:0               *
*********************************/
#define FLAG_CHECK(FLAGS, ID) (FLAGS[((ID)/8)] & (1 <<((ID)%8)))

/*****************************
* FLAG_ONマクロ関数          *
* フラグをONにする           *
* 引数                       *
* FLAGS	: フラグ配列         *
* ID	: ONにするフラグのID *
*****************************/
#define FLAG_ON(FLAGS, ID) FLAGS[((ID)/8)] |= (1 << ((ID)%8))

/******************************
* FLAG_OFFマクロ関数          *
* フラグをOFFにする           *
* 引数                        *
* FLAGS	: フラグ配列          *
* ID	: OFFにするフラグのID *
*****************************/
#define FLAG_OFF(FLAGS, ID) FLAGS[((ID)/8)] &= ~(1 << ((ID)%8))

extern void AdjustmentChangeValueCallBackInt8(GtkAdjustment* adjustment, int8* store);
extern void AdjustmentChangeValueCallBackUint8(GtkAdjustment* adjustment, uint8* store);
extern void AdjustmentChangeValueCallBackInt16(GtkAdjustment* adjustment, int16* store);
extern void AdjustmentChangeValueCallBackUint16(GtkAdjustment* adjustment, uint16* store);
extern void AdjustmentChangeValueCallBackInt32(GtkAdjustment* adjustment, int32* store);
extern void AdjustmentChangeValueCallBackUint32(GtkAdjustment* adjustment, uint32* store);
extern void AdjustmentChangeVaueCallBackDouble(GtkAdjustment* adjustment, gdouble* value);

extern void CheckButtonSetFlagsCallBack(GtkWidget* button, guint32* flags, guint32 flag_value);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_UTILS_H_
