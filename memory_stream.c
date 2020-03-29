#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "memory_stream.h"

// C++�ŃR���p�C������ۂ̃G���[���
#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************
* CreateMemoryStream�֐�                                 *
* �������̓ǂݏ������Ǘ�����\���̂̃������̊m�ۂƏ����� *
* ����                                                   *
* buff_size	: �m�ۂ���o�b�t�@�̃T�C�Y                   *
* �Ԃ�l                                                 *
*	���������ꂽ�\���̂̃A�h���X                         *
*********************************************************/
MEMORY_STREAM_PTR CreateMemoryStream(
	size_t buff_size
)
{
	// �Ԃ�l
	MEMORY_STREAM_PTR ret =
		(MEMORY_STREAM_PTR)MEM_ALLOC_FUNC(sizeof(MEMORY_STREAM));

	// �������m�ې����̃`�F�b�N
	if(ret == NULL)
	{
#ifdef _DEBUG
		(void)printf("Memory allocate error.\n(In CreateMemoryStream)\n");
#endif
		return NULL;
	}

	// �o�b�t�@�̃������̊m��
	ret->buff_ptr = (unsigned char*)MEM_ALLOC_FUNC(buff_size+1);

	// �������m�ې����̃`�F�b�N
	if(ret->buff_ptr == NULL)
	{
#ifdef _DEBUG
		(void)printf("Memory allocate error.\n(In CreateMemoryStream)\n");
#endif
		MEM_FREE_FUNC(ret);

		return NULL;
	}

	// �o�b�t�@��0�N���A���Ă���
	(void)memset(ret->buff_ptr, 0, buff_size+1);

	// �e��ϐ��̐ݒ�
	ret->data_point = 0;
	ret->data_size = buff_size;
	ret->block_size = buff_size;

	return ret;
}

/*****************************************************
* DeleteMemoryStream�֐�                             *
* �������̓ǂݏ������Ǘ�����\���̂̃��������J��     *
* ����                                               *
* mem	: ��������ǂݏ������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                             *
*	���0                                            *
*****************************************************/
int DeleteMemoryStream(MEMORY_STREAM_PTR mem)
{
	if(mem != NULL)
	{
		MEM_FREE_FUNC(mem->buff_ptr);
		MEM_FREE_FUNC(mem);
	}

	return 0;
}

/***************************************************************
* MemRead�֐�                                                  *
* ����������f�[�^��ǂݍ���                                   *
* ����                                                         *
* dst			: �f�[�^�̓ǂݍ��ݐ�                           *
* block_size	: �f�[�^��ǂݍ��ލۂ�1�̃u���b�N�̃T�C�Y    *
* block_num		: �ǂݍ��ރu���b�N�̌�                       *
* mem			: �ǂݍ��݌��̃��������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                       *
*	�ǂݍ��񂾃o�C�g��                                         *
***************************************************************/
size_t MemRead(
	void* dst,
	size_t block_size,
	size_t block_num,
	MEMORY_STREAM_PTR mem
)
{
	// �ǂݍ��݂�v�����ꂽ�o�C�g��
	size_t required_size = block_size * block_num;

	// ���ۂɓǂݍ��񂾃o�C�g���A�u���b�N��
	size_t read_size, read_block;

	// �v�����ꂽ�o�C�g���ǂݍ���ł��f�[�^�̏I�[�ɓ��B���Ȃ��Ȃ�
	if(required_size + mem->data_point <= mem->data_size)
	{
		// �v�����ꂽ�o�C�g�����ǂݍ���(�R�s�[����)
		read_size = required_size;
		read_block = block_num;
	}
	else
	{
		// �f�[�^�̏I�[�ɓ��B���Ă���̂�
		// �ǂݍ��ރo�C�g�����v�Z���Ă���ǂݍ���
		read_size = mem->data_size - mem->data_point;
		read_block = read_size / block_size;
	}

	(void)memcpy(dst, &mem->buff_ptr[mem->data_point], read_size);

	// �f�[�^�̎Q�ƈʒu���X�V
	mem->data_point += read_size;

	return read_block;
}

/***************************************************************
* MemWrite�֐�                                                 *
* �������փf�[�^����������                                     *
* ����                                                         *
* src                                                          *
* block_size	: �f�[�^���������ލۂ�1�̃u���b�N�̃T�C�Y    *
* block_num		: �������ރu���b�N�̐�                         *
* mem			: �������ݐ�̃��������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                       *
*	�������񂾃o�C�g��                                         *
***************************************************************/
size_t MemWrite(
	const void* src,
	size_t block_size,
	size_t block_num,
	MEMORY_STREAM_PTR mem
)
{
	// �������݂�v�����ꂽ�o�C�g��
	size_t required_size = block_size * block_num;

	// ���ۂɏ������񂾃o�C�g���A�u���b�N��
	size_t write_size;

	// �v�����ꂽ�o�C�g����������ł��o�b�t�@�̏I�[�ɓ��B���Ȃ��Ȃ�
	if(required_size + mem->data_point <= mem->data_size)
	{
		// �v�����ꂽ�o�C�g������������(�R�s�[����)
		write_size = required_size;
	}
	else
	{
		// �o�b�t�@�̏I�[�ɓ��B���Ă��܂��̂�
		// �o�b�t�@���m�ۂ��Ȃ���
		unsigned char* temp = (unsigned char*)MEM_ALLOC_FUNC(
			mem->data_size + mem->block_size + (required_size / mem->block_size) * mem->block_size);
		(void)memcpy(temp, mem->buff_ptr, mem->data_size);
		(void)memset(&temp[mem->data_size], 0, mem->block_size);
		MEM_FREE_FUNC(mem->buff_ptr);
		mem->buff_ptr = temp;
		mem->data_size += mem->block_size + (required_size / mem->block_size) * mem->block_size;
		write_size = required_size;
	}

	(void)memcpy(&mem->buff_ptr[mem->data_point], src, write_size);

	// �f�[�^�̎Q�ƈʒu���X�V
	mem->data_point += write_size;

	return block_num;
}

/***************************************************************
* MemSeek�֐�                                                  *
* �V�[�N���s��                                                 *
* ����                                                         *
* mem		: �V�[�N���s�����������Ǘ����Ă���\���̂̃A�h���X *
* offset	: �ړ�����o�C�g��                                 *
* origin	: �ړ����J�n����ʒu(fseek�Ɠ���)                  *
* �Ԃ�l                                                       *
*	����I��(0)�A�ُ�I��(0�ȊO)                               *
***************************************************************/
int MemSeek(MEMORY_STREAM_PTR mem, long offset, int origin)
{
	switch(origin)
	{
	case SEEK_SET:
		if(offset >= 0 && (size_t)offset <= mem->data_size)
		{
			mem->data_point = offset;
			return 0;
		}
		else
		{
			return 1;
		}
	case SEEK_CUR:
		if(mem->data_point + offset <= mem->data_size)
		{
			mem->data_point += offset;
			return 0;
		}
		else
		{
			return 1;
		}
	case SEEK_END:
		{
			if(offset <= 0)
			{
				mem->data_point = mem->data_size + offset;
				return 0;
			}
			else
			{
				return 1;
			}
		}
	}	// switch(origin)

	return 1;
}

/***************************************************************
* MemSeek64�֐�                                                *
* �V�[�N���s��(64�r�b�g���b�v��)                               *
* ����                                                         *
* mem		: �V�[�N���s�����������Ǘ����Ă���\���̂̃A�h���X *
* offset	: �ړ�����o�C�g��                                 *
* origin	: �ړ����J�n����ʒu(fseek�Ɠ���)                  *
* �Ԃ�l                                                       *
*	����I��(0)�A�ُ�I��(0�ȊO)                               *
***************************************************************/
int MemSeek64(MEMORY_STREAM_PTR mem, long long int offset, int origin)
{
	return MemSeek(mem, (long)offset, origin);
}

/*****************************************************
* MemTell�֐�                                        *
* �f�[�^�̎Q�ƈʒu��Ԃ�                             *
* mem	: ��������ǂݏ������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                             *
*	�f�[�^�̎Q�ƈʒu                                 *
*****************************************************/
long MemTell(MEMORY_STREAM_PTR mem)
{
	return (long)mem->data_point;
}

/***************************************************************
* MemGets�֐�                                                  *
* 1�s�ǂݍ���                                                  *
* ����                                                         *
* string	: �ǂݍ��ޕ�������i�[����A�h���X                 *
* n			: �ǂݍ��ލő�̕�����                             *
* mem		: �V�[�N���s�����������Ǘ����Ă���\���̂̃A�h���X *
* �Ԃ�l                                                       *
*	����(string)�A���s(NULL)                                   *
***************************************************************/
char* MemGets(char* string, int n, MEMORY_STREAM_PTR mem)
{
	int i;	// for���p�̃J�E���^

	// �f�[�^�̏I�[�ɒB���Ă��Ȃ����𔻒f
	if(mem->data_point == mem->data_size)
	{
		return NULL;
	}

	// ���s��T��
	for(i=0; i<n; i++)
	{
		// ���s��������
		if(mem->buff_ptr[mem->data_point+i] == '\n' || mem->buff_ptr[mem->data_point+i] == '\r')
		{
			break;
		}

		// �f�[�^�̏I�[�ɒB����
		if(mem->data_point + i == mem->data_size)
		{
			(void)memcpy(string, &mem->buff_ptr[mem->data_point], i);
			return NULL;
		}
	}

	// ����������t�ɂȂ���
	if(i == n)
	{
		(void)memcpy(string, &mem->buff_ptr[mem->data_point], i-1);
		mem->data_point += i-1;	// �f�[�^�̎Q�ƈʒu���X�V
		string[i-1] = '\0';	// // �Ō�̕������k�������ɂ��Ă���
		return NULL;
	}

	(void)memcpy(string, &mem->buff_ptr[mem->data_point], i);
	mem->data_point += i;	// �f�[�^�̎Q�ƈʒu���X�V
	if(mem->buff_ptr[mem->data_point] == '\r')
	{
		mem->data_point++;
	}

	if(mem->buff_ptr[mem->data_point] == '\n')
	{
		mem->data_point++;
	}
	string[i] = '\0';	// �Ō�̕������k�������ɂ��Ă���

	return string;
}

// C++�ŃR���p�C������ۂ̃G���[���
#ifdef __cplusplus
}
#endif
