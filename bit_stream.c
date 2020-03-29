#include <string.h>
#include "bit_stream.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************
* CreateBitStream�֐�               *
* 1�r�b�g�P�ʂœǂݍ��ރf�[�^�̐��� *
* �Ԃ�l                            *
*	�쐬�����f�[�^                  *
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
* DeleteBitStream�֐�                                         *
* 1�r�b�g�P�ʂœǂݍ��ރf�[�^�̍폜                           *
* ����                                                        *
* stream	: 1�r�b�g�P�ʂœǂݍ��ނ��߂̃X�g���[���Ǘ��f�[�^ *
* �Ԃ�l                                                      *
*	���0                                                     *
**************************************************************/
int DeleteBitStream(BIT_STREAM* stream)
{
	MEM_FREE_FUNC(stream->bytes);
	MEM_FREE_FUNC(stream);

	return 0;
}

/**************************************************************
* BitsRead�֐�                                                *
* �w�肳�ꂽ�r�b�g���ǂݍ���                                  *
* ����                                                        *
* stream	: 1�r�b�g�P�ʂœǂݍ��ނ��߂̃X�g���[���Ǘ��f�[�^ *
* read_bits	: �ǂݍ��ރr�b�g��(0�`32)                         *
* �Ԃ�l                                                      *
*	�ǂݍ��񂾃r�b�g�����̒l                                  *
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
