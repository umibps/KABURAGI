// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <gtk/gtk.h>
#include "ini_file.h"
#include "label.h"

/*
* LoadLabel�֐�
* �A�v���P�[�V�����Ŏg�����x���̕����f�[�^��ǂݍ���
* label				: ���x���̕����f�[�^���L������\����
* land_file_path	: �����f�[�^�̓������t�@�C���̃p�X
*/
void LoadLabel(LABEL* label, const char* lang_file_path)
{
	// .ini�`���̃t�@�C���ǂݍ��ݗp
	INI_FILE *file;
	// �t�@�C���A�N�Z�X�p
	FILE *fp;
	// �t�@�C���̃o�C�g��
	size_t data_size;
	// �t�@�C���̕����R�[�h
	char code[512];
	// ���x���̕�����
	char str[2048];
	// ������̒���
	size_t length;

	// �t�@�C���I�[�v��
	fp = fopen(lang_file_path, "rb");
	if(fp == NULL)
	{
		return;
	}

	// �o�C�g�����擾
	(void)fseek(fp, 0, SEEK_END);
	data_size = ftell(fp);
	// �V�[�N�ʒu�����ɖ߂�
	rewind(fp);

	// .ini�`���t�@�C���ǂݍ��݃f�[�^�쐬
	file = CreateIniFile((void*)fp, (size_t (*)(void*, size_t, size_t, void*))fread,
		data_size, INI_READ);

	// �����R�[�h���擾
	(void)IniFileGetString(file, "CODE", "CODE", code, 512);

	// ���j���[
	length = IniFileGetString(file, "MENU", "FILE", str, 2048);
	label->menu.file = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "MENU", "OPEN", str, 2048);
	label->menu.open = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "MENU", "FILE_OPEN", str, 2048);
	label->menu.file_open = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "MENU", "DIRECTORY_OPEN", str, 2048);
	label->menu.dir_open = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);

	// �t�@�C�����X�g�r���[
	length = IniFileGetString(file, "LIST_VIEW", "TITLE", str, 2048);
	label->list_view.title = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "LIST_VIEW", "ARTIST", str, 2048);
	label->list_view.artist = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "LIST_VIEW", "ALBUM", str, 2048);
	label->list_view.album = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "LIST_VIEW", "DURATION", str, 2048);
	label->list_view.duration = g_convert(str, length, "UTF-8", code, NULL, NULL, NULL);

	(void)fclose(fp);
	file->delete_func(file);
}
