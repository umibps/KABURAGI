#ifndef _INCLUDED_LABEL_H_
#define _INCLUDED_LABEL_H_

typedef struct _LABEL
{
	struct
	{
		char *file, *open, *file_open, *dir_open;
	} menu;

	struct
	{
		char *title, *artist, *album, *duration;
	} list_view;
} LABEL;

/*
* LoadLabel�֐�
* �A�v���P�[�V�����Ŏg�����x���̕����f�[�^��ǂݍ���
* label				: ���x���̕����f�[�^���L������\����
* land_file_path	: �����f�[�^�̓������t�@�C���̃p�X
*/
extern void LoadLabel(LABEL* label, const char* lang_file_path);

#endif	// #ifndef _INCLUDED_LABEL_H_
