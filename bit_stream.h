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

// �֐��̃v���g�^�C�v�錾
/************************************
* CreateBitStream�֐�               *
* 1�r�b�g�P�ʂœǂݍ��ރf�[�^�̐��� *
* �Ԃ�l                            *
*	�쐬�����f�[�^                  *
************************************/
extern BIT_STREAM* CreateBitStream(
	uint8* bytes,
	size_t data_size
);

/**************************************************************
* DeleteBitStream�֐�                                         *
* 1�r�b�g�P�ʂœǂݍ��ރf�[�^�̍폜                           *
* ����                                                        *
* stream	: 1�r�b�g�P�ʂœǂݍ��ނ��߂̃X�g���[���Ǘ��f�[�^ *
* �Ԃ�l                                                      *
*	���0                                                     *
**************************************************************/
extern int DeleteBitStream(BIT_STREAM* stream);

/**************************************************************
* BitsRead�֐�                                                *
* �w�肳�ꂽ�r�b�g���ǂݍ���                                  *
* ����                                                        *
* stream	: 1�r�b�g�P�ʂœǂݍ��ނ��߂̃X�g���[���Ǘ��f�[�^ *
* read_bits	: �ǂݍ��ރr�b�g��(0�`32)                         *
* �Ԃ�l                                                      *
*	�ǂݍ��񂾃r�b�g�����̒l                                  *
**************************************************************/
extern uint32 BitsRead(BIT_STREAM* stream, int read_bits);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_BIT_STREAM_H_
