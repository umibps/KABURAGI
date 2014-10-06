#ifndef _INCLUDED_UTILS_H_
#define _INCLUDED_UTILS_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SWAP_LE_BE(val)	((uint32) ( \
	(((uint32) (val) & (uint32) 0x000000ffU) << 24) | \
	(((uint32) (val) & (uint32) 0x0000ff00U) <<  8) | \
	(((uint32) (val) & (uint32) 0x00ff0000U) >>  8) | \
	(((uint32) (val) & (uint32) 0xff000000U) >> 24)))

#define UINT32_FROM_BE(value) SWAP_LE_BE(value)

typedef struct _BYTE_ARRAY
{
	uint8 *buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
} BYTE_ARRAY;

typedef struct _WORD_ARRAY
{
	uint16 *buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
} WORD_ARRAY;

typedef struct _UINT32_ARRAY
{
	uint32 *buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
} UINT32_ARRAY;

typedef struct _POINTER_ARRAY
{
	void **buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
} POINTER_ARRAY;

typedef struct _STRUCT_ARRAY
{
	uint8 *buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
	size_t data_size;
} STRUCT_ARRAY;

/**************************************
* StringCompareIgnoreCase関数         *
* 大文字/小文字を無視して文字列を比較 *
* 引数                                *
* str1	: 比較文字列1                 *
* str2	: 比較文字列2                 *
* 返り値                              *
*	文字列の差(同文字列なら0)         *
**************************************/
EXTERN int StringCompareIgnoreCase(const char* str1, const char* str2);

/**************************************
* memset32関数                        *
* 32bit符号なし整数でバッファを埋める *
* 引数                                *
* buff	: 埋める対象のバッファ        *
* value	: 埋める値                    *
* size	: 埋めるバイト数              *
**************************************/
EXTERN void memset32(void* buff, uint32 value, size_t size);

/**************************************
* memset64関数                        *
* 64bit符号なし整数でバッファを埋める *
* 引数                                *
* buff	: 埋める対象のバッファ        *
* value	: 埋める値                    *
* size	: 埋めるバイト数              *
**************************************/
EXTERN void memset64(void* buff, uint64 value, size_t size);

EXTERN BYTE_ARRAY* ByteArrayNew(size_t block_size);
EXTERN void ByteArrayAppend(BYTE_ARRAY* barray, uint8 data);
EXTERN void ByteArrayDesteroy(BYTE_ARRAY** barray);
EXTERN void ByteArrayResize(BYTE_ARRAY* barray, size_t new_size);

EXTERN WORD_ARRAY* WordArrayNew(size_t block_size);
EXTERN void WordArrayAppend(WORD_ARRAY* warray, uint16 data);
EXTERN void WordArrayDestroy(WORD_ARRAY** warray);
EXTERN void WordArrayResize(WORD_ARRAY* warray, size_t new_size);

EXTERN UINT32_ARRAY* Uint32ArrayNew(size_t block_size);
EXTERN void Uint32ArrayAppend(UINT32_ARRAY* uarray, uint32 data);
EXTERN void Uint32ArrayDestroy(UINT32_ARRAY** uarray);
EXTERN void Uint32ArrayResize(UINT32_ARRAY* uint32_array, size_t new_size);

EXTERN POINTER_ARRAY* PointerArrayNew(size_t block_size);
EXTERN void PointerArrayRelease(
	POINTER_ARRAY* pointer_array,
	void (*destroy_func)(void*)
);
EXTERN void PointerArrayDestroy(
	POINTER_ARRAY** pointer_array,
	void (*destroy_func)(void*)
);
void PointerArrayAppend(POINTER_ARRAY* pointer_array, void* data);
EXTERN void PointerArrayRemoveByIndex(
	POINTER_ARRAY* pointer_array,
	size_t index,
	void (*destroy_func)(void*)
);
EXTERN void PointerArrayRemoveByData(
	POINTER_ARRAY* pointer_array,
	void* data,
	void (*destroy_func)(void*)
);
EXTERN int PointerArrayDoesCointainData(POINTER_ARRAY* pointer_array, void* data);

EXTERN STRUCT_ARRAY* StructArrayNew(size_t data_size, size_t block_size);
EXTERN void StructArrayDestroy(
	STRUCT_ARRAY** struct_array,
	void (*destroy_func)(void*)
);
EXTERN void StructArrayAppend(STRUCT_ARRAY* struct_array, void* data);
EXTERN void* StructArrayReserve(STRUCT_ARRAY* struct_array);
EXTERN void StructArrayResize(STRUCT_ARRAY* struct_array, size_t new_size);
EXTERN void StructArrayRemoveByIndex(
	STRUCT_ARRAY* struct_array,
	size_t index,
	void (*destroy_func)(void*)
);

#ifdef _MSC_VER
EXTERN int strncasecmp(const char* s1, const char* s2, size_t n);
#endif

/**************************************************************
* StringStringIgnoreCase関数                                  *
* 大文字/小文字を無視して引数1の文字列から引数2の文字列を探す *
* str		: 検索対象の文字列                                *
* search	: 検索する文字列                                  *
* 返り値                                                      *
*	文字列発見:発見した位置のポインタ	見つからない:NULL     *
**************************************************************/
EXTERN char* StringStringIgnoreCase(const char* str, const char* search);

/*********************************
* GetFileExtention関数           *
* ファイル名から拡張子を取得する *
* 引数                           *
* file_name	: ファイル名         *
* 返り値                         *
*	拡張子の文字列               *
*********************************/
EXTERN char* GetFileExtention(char* file_name);

/***************************************
* StringReplace関数                    *
* 指定した文字列を置換する             *
* 引数                                 *
* str			: 処理をする文字列     *
* replace_from	: 置き換えられる文字列 *
* replace_to	: 置き換える文字列     *
***************************************/
EXTERN void StringRepalce(
	char* str,
	char* replace_from,
	char* replace_to
);

EXTERN int ForFontFamilySearchCompare(const char* str, PangoFontFamily** font);

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

/***********************************************
* InvertMatrix関数                             *
* 逆行列を計算する                             *
* 引数                                         *
* a	: 計算対象の行列(逆行列データはここに入る) *
* n	: 行列の次数                               *
***********************************************/
EXTERN void InvertMatrix(FLOAT_T **a, int n);

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

EXTERN void AdjustmentChangeValueCallBackInt(GtkAdjustment* adjustment, int* store);
EXTERN void AdjustmentChangeValueCallBackUint(GtkAdjustment* adjustment, unsigned int* store);
EXTERN void AdjustmentChangeValueCallBackInt8(GtkAdjustment* adjustment, int8* store);
EXTERN void AdjustmentChangeValueCallBackUint8(GtkAdjustment* adjustment, uint8* store);
EXTERN void AdjustmentChangeValueCallBackInt16(GtkAdjustment* adjustment, int16* store);
EXTERN void AdjustmentChangeValueCallBackUint16(GtkAdjustment* adjustment, uint16* store);
EXTERN void AdjustmentChangeValueCallBackInt32(GtkAdjustment* adjustment, int32* store);
EXTERN void AdjustmentChangeValueCallBackUint32(GtkAdjustment* adjustment, uint32* store);
EXTERN void AdjustmentChangeValueCallBackDouble(GtkAdjustment* adjustment, gdouble* value);

EXTERN void CheckButtonSetFlagsCallBack(GtkWidget* button, guint32* flags, guint32 flag_value);

/*******************************************************
* InflateData関数                                      *
* ZIP圧縮されたデータをデコードする                    *
* 引数                                                 *
* data				: 入力データ                       *
* out_buffer		: 出力先のバッファ                 *
* in_size			: 入力データのバイト数             *
* out_buffer_size	: 出力先のバッファのサイズ         *
* out_size			: 出力したバイト数の格納先(NULL可) *
* 返り値                                               *
*	正常終了:0、失敗:0以外                             *
*******************************************************/
EXTERN int InflateData(
	uint8* data,
	uint8* out_buffer,
	size_t in_size,
	size_t out_buffer_size,
	size_t* out_size
);

/***************************************************
* DeflateData関数                                  *
* ZIP圧縮を行う                                    *
* 引数                                             *
* data					: 入力データ               *
* out_buffer			: 出力先のバッファ         *
* target_data_size		: 入力データのバイト数     *
* out_buffer_size		: 出力先のバッファのサイズ *
* compressed_data_size	: 圧縮後のバイト数格納先   *
* compress_level		: 圧縮レベル(0～9)         *
* 返り値                                           *
*	正常終了:0、失敗:0以外                         *
***************************************************/
EXTERN int DeflateData(
	uint8* data,
	uint8* out_buffer,
	size_t target_data_size,
	size_t out_buffer_size,
	size_t* compressed_data_size,
	int compress_level
);

EXTERN void UpdateWidget(GtkWidget* widget);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_UTILS_H_
