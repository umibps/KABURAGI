#ifndef _INCLUDED_UTILS_H_
#define _INCLUDED_UTILS_H_

#include "types.h"
#include "ght_hash_table.h"

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

typedef struct _FLOAT_ARRAY
{
	FLOAT_T *buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
} FLOAT_ARRAY;

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
* StringCompareIgnoreCase�֐�         *
* �啶��/�������𖳎����ĕ�������r *
* ����                                *
* str1	: ��r������1                 *
* str2	: ��r������2                 *
* �Ԃ�l                              *
*	������̍�(��������Ȃ�0)         *
**************************************/
EXTERN int StringCompareIgnoreCase(const char* str1, const char* str2);

/**************************************
* memset32�֐�                        *
* 32bit�����Ȃ������Ńo�b�t�@�𖄂߂� *
* ����                                *
* buff	: ���߂�Ώۂ̃o�b�t�@        *
* value	: ���߂�l                    *
* size	: ���߂�o�C�g��              *
**************************************/
EXTERN void memset32(void* buff, uint32 value, size_t size);

/**************************************
* memset64�֐�                        *
* 64bit�����Ȃ������Ńo�b�t�@�𖄂߂� *
* ����                                *
* buff	: ���߂�Ώۂ̃o�b�t�@        *
* value	: ���߂�l                    *
* size	: ���߂�o�C�g��              *
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

EXTERN FLOAT_ARRAY* FloatArrayNew(size_t block_size);
EXTERN void FloatArrayAppend(FLOAT_ARRAY* farray, FLOAT_T data);
EXTERN void FloatArrayDestroy(FLOAT_ARRAY** farray);
EXTERN void FloatArrayResize(FLOAT_ARRAY* float_array, size_t new_size);

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
* StringStringIgnoreCase�֐�                                  *
* �啶��/�������𖳎����Ĉ���1�̕����񂩂����2�̕������T�� *
* str		: �����Ώۂ̕�����                                *
* search	: �������镶����                                  *
* �Ԃ�l                                                      *
*	�����񔭌�:���������ʒu�̃|�C���^	������Ȃ�:NULL     *
**************************************************************/
EXTERN char* StringStringIgnoreCase(const char* str, const char* search);

/*******************************************
* StringRemoveTargetString�֐�             *
* �����񂩂�Ώۂ̕�������폜����         *
* ����                                     *
* str		: �Ώۂ̕�������폜���镶���� *
* target	: �폜���镶����               *
* output	: �o�̓o�b�t�@                 *
*******************************************/
EXTERN void StringRemoveTargetString(char* str, const char* target, char* output);

/*************************************
* GetNextUtf8Character�֐�           *
* ����UTF8�̕������擾����           *
* ����                               *
* str	: ���̕������擾������������ *
* �Ԃ�l                             *
*	���̕����̃A�h���X               *
*************************************/
EXTERN const char* GetNextUtf8Character(const char* str);

/*********************************
* GetFileExtention�֐�           *
* �t�@�C��������g���q���擾���� *
* ����                           *
* file_name	: �t�@�C����         *
* �Ԃ�l                         *
*	�g���q�̕�����               *
*********************************/
EXTERN char* GetFileExtention(char* file_name);

/***************************************
* StringReplace�֐�                    *
* �w�肵���������u������             *
* ����                                 *
* str			: ���������镶����     *
* replace_from	: �u���������镶���� *
* replace_to	: �u�������镶����     *
***************************************/
EXTERN void StringRepalce(
	char* str,
	char* replace_from,
	char* replace_to
);

/***********************************************
* InvertMatrix�֐�                             *
* �t�s����v�Z����                             *
* ����                                         *
* a	: �v�Z�Ώۂ̍s��(�t�s��f�[�^�͂����ɓ���) *
* n	: �s��̎���                               *
***********************************************/
EXTERN void InvertMatrix(FLOAT_T **a, int n);

/*****************************************
* IsPointInTriangle�֐�                  *
* �O�p�`���ɍ��W�����邩���ׂ�           *
* ����                                   *
* check		: ���ׂ���W                 *
* triangle1	: �O�p�`�̒��_����1          *
* triangle2	: �O�p�`�̒��_����2          *
* triangle3	: �O�p�`�̒��_����3          *
* �Ԃ�l                                 *
*	�O�p�`���ɂ���:TRUE	�O�p�`�̊O:FALSE *
*****************************************/
EXTERN int IsPointInTriangle(FLOAT_T check[2], FLOAT_T triangle1[2], FLOAT_T triangle2[2], FLOAT_T triangle3[2]);

/*********************************
* FLAG_CHECK�}�N���֐�           *
* �t���O��ON/OFF�𔻒肷��       *
* ����                           *
* FLAGS	: �t���O�z��             *
* ID	: �`�F�b�N����t���O��ID *
* �Ԃ�l                         *
*	ON:0�ȊO OFF:0               *
*********************************/
#define FLAG_CHECK(FLAGS, ID) (FLAGS[((ID)/8)] & (1 <<((ID)%8)))

/*****************************
* FLAG_ON�}�N���֐�          *
* �t���O��ON�ɂ���           *
* ����                       *
* FLAGS	: �t���O�z��         *
* ID	: ON�ɂ���t���O��ID *
*****************************/
#define FLAG_ON(FLAGS, ID) FLAGS[((ID)/8)] |= (1 << ((ID)%8))

/******************************
* FLAG_OFF�}�N���֐�          *
* �t���O��OFF�ɂ���           *
* ����                        *
* FLAGS	: �t���O�z��          *
* ID	: OFF�ɂ���t���O��ID *
*****************************/
#define FLAG_OFF(FLAGS, ID) FLAGS[((ID)/8)] &= ~(1 << ((ID)%8))

/*******************************************************
* InflateData�֐�                                      *
* ZIP���k���ꂽ�f�[�^���f�R�[�h����                    *
* ����                                                 *
* data				: ���̓f�[�^                       *
* out_buffer		: �o�͐�̃o�b�t�@                 *
* in_size			: ���̓f�[�^�̃o�C�g��             *
* out_buffer_size	: �o�͐�̃o�b�t�@�̃T�C�Y         *
* out_size			: �o�͂����o�C�g���̊i�[��(NULL��) *
* �Ԃ�l                                               *
*	����I��:0�A���s:0�ȊO                             *
*******************************************************/
EXTERN int InflateData(
	uint8* data,
	uint8* out_buffer,
	size_t in_size,
	size_t out_buffer_size,
	size_t* out_size
);

/***************************************************
* DeflateData�֐�                                  *
* ZIP���k���s��                                    *
* ����                                             *
* data					: ���̓f�[�^               *
* out_buffer			: �o�͐�̃o�b�t�@         *
* target_data_size		: ���̓f�[�^�̃o�C�g��     *
* out_buffer_size		: �o�͐�̃o�b�t�@�̃T�C�Y *
* compressed_data_size	: ���k��̃o�C�g���i�[��   *
* compress_level		: ���k���x��(0�`9)         *
* �Ԃ�l                                           *
*	����I��:0�A���s:0�ȊO                         *
***************************************************/
EXTERN int DeflateData(
	uint8* data,
	uint8* out_buffer,
	size_t target_data_size,
	size_t out_buffer_size,
	size_t* compressed_data_size,
	int compress_level
);

/*****************************************
* GetStringHash�֐�                      *
* �����񂩂�n�b�V���l���v�Z����         *
* ����                                   *
* key	: �n�b�V���e�[�u�������L�[�f�[�^ *
* �Ԃ�l                                 *
*	�n�b�V���l                           *
*****************************************/
EXTERN ght_uint32_t GetStringHash(ght_hash_key_t* key);

/***************************************************
* GetStringHashIgnoreCase�֐�                      *
* �����񂩂�n�b�V���l���v�Z����(�啶������������) *
* ����                                             *
* key	: �n�b�V���e�[�u�������L�[�f�[�^           *
* �Ԃ�l                                           *
*	�n�b�V���l                                     *
***************************************************/
EXTERN ght_uint32_t GetStringHashIgnoreCase(ght_hash_key_t* key);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_UTILS_H_
