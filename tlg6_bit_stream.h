//---------------------------------------------------------------------------
#ifndef TLG6BS_H
#define TLG6BS_H

#include <memory.h>
#include "memory_stream.h"
#include "golomb_table.h"

// TLG6.0 bitstream output implementation
typedef struct _TLG6BIT_STREAM
{
	int buffer_bit_pos;				// 出力バッファのビット位置
	long buffer_byte_pos;			// バッファのバイト位置
	MEMORY_STREAM_PTR out_stream;	// 出力ストリーム
	unsigned char* buffer;			// 出力バッファ
	long capacity;					// 容量
	GOLOMB_TABLE table;				// Golombテーブル
} TLG6BIT_STREAM;

extern TLG6BIT_STREAM* CreateTLG6BIT_STREAM(void);
extern void DeleteTLG6BIT_STREAM(TLG6BIT_STREAM** stream);
extern void TLG6BitStreamFlush(TLG6BIT_STREAM* stream);
extern void TLG6Put1Bit(TLG6BIT_STREAM* stream, int b);
extern void TLG6PutValue(TLG6BIT_STREAM* stream ,long v, int len);
extern void TLG6PutGamma(TLG6BIT_STREAM* stream, int v);
extern int GetGammaBitLength(int v);

#endif
//---------------------------------------------------------------------------
