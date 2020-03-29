#include <string.h>
#include "bit_stream.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************
* CreateBitStream関数               *
* 1ビット単位で読み込むデータの生成 *
* 返り値                            *
*	作成したデータ                  *
************************************/
BIT_STREAM* CreateBitStream(
	uint8* bytes,
	size_t data_size
)
{
	BIT_STREAM *ret = (BIT_STREAM*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->bytes = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memcpy(ret->bytes, bytes, data_size);
	ret->num_bytes = data_size;

	return ret;
}

/**************************************************************
* DeleteBitStream関数                                         *
* 1ビット単位で読み込むデータの削除                           *
* 引数                                                        *
* stream	: 1ビット単位で読み込むためのストリーム管理データ *
* 返り値                                                      *
*	常に0                                                     *
**************************************************************/
int DeleteBitStream(BIT_STREAM* stream)
{
	MEM_FREE_FUNC(stream->bytes);
	MEM_FREE_FUNC(stream);

	return 0;
}

/**************************************************************
* BitsRead関数                                                *
* 指定されたビット数読み込む                                  *
* 引数                                                        *
* stream	: 1ビット単位で読み込むためのストリーム管理データ *
* read_bits	: 読み込むビット数(0～32)                         *
* 返り値                                                      *
*	読み込んだビット数分の値                                  *
**************************************************************/
uint32 BitsRead(BIT_STREAM* stream, int read_bits)
{
	uint32 ret = 0;
	int i;

	if(read_bits <= 0)
	{
		return 0;
	}
	else if(read_bits > 32)
	{
		read_bits = 32;
	}

	if(stream->bit_point + read_bits > stream->num_bytes * 8)
	{
		read_bits = (int)(stream->num_bytes * 8 - stream->bit_point);
	}

	for(i=0; i<read_bits; i++)
	{
		ret |= stream->bytes[stream->bit_point / 8] & (1 << (stream->bit_point % 8));
		stream->bit_point++;
	}

	return ret;
}

#ifdef __cplusplus
}
#endif
