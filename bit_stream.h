#ifndef _INCLUDED_BIT_STREAM_H_
#define _INCLUDED_BIT_STREAM_H_

#include <stdio.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BIT_STREAM
{
	uint8 *bytes;
	size_t bit_point;
	size_t num_bytes;
} BIT_STREAM;

// 関数のプロトタイプ宣言
/************************************
* CreateBitStream関数               *
* 1ビット単位で読み込むデータの生成 *
* 返り値                            *
*	作成したデータ                  *
************************************/
extern BIT_STREAM* CreateBitStream(
	uint8* bytes,
	size_t data_size
);

/**************************************************************
* DeleteBitStream関数                                         *
* 1ビット単位で読み込むデータの削除                           *
* 引数                                                        *
* stream	: 1ビット単位で読み込むためのストリーム管理データ *
* 返り値                                                      *
*	常に0                                                     *
**************************************************************/
extern int DeleteBitStream(BIT_STREAM* stream);

/**************************************************************
* BitsRead関数                                                *
* 指定されたビット数読み込む                                  *
* 引数                                                        *
* stream	: 1ビット単位で読み込むためのストリーム管理データ *
* read_bits	: 読み込むビット数(0～32)                         *
* 返り値                                                      *
*	読み込んだビット数分の値                                  *
**************************************************************/
extern uint32 BitsRead(BIT_STREAM* stream, int read_bits);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_BIT_STREAM_H_
