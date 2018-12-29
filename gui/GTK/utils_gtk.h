#ifndef _INCLUDED_UTILS_GTK_H_
#define _INCLUDED_UTILS_GTK_H_

#include <gtk/gtk.h>
#include "../../types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************
* FileRead�֐�                             *
* Gtk�̊֐��𗘗p���ăt�@�C���ǂݍ���      *
* ����                                     *
* dst			: �ǂݍ��ݐ�̃A�h���X     *
* block_size	: �ǂݍ��ރu���b�N�̃T�C�Y *
* read_num		: �ǂݍ��ރu���b�N�̐�     *
* stream		: �ǂݍ��݌��̃X�g���[��   *
* �Ԃ�l                                   *
*	�ǂݍ��񂾃u���b�N��                   *
*******************************************/
EXTERN size_t FileRead(void* dst, size_t block_size, size_t read_num, GFileInputStream* stream);

/*******************************************
* FileWrite�֐�                            *
* Gtk�̊֐��𗘗p���ăt�@�C����������      *
* ����                                     *
* src			: �������݌��̃A�h���X     *
* block_size	: �������ރu���b�N�̃T�C�Y *
* read_num		: �������ރu���b�N�̐�     *
* stream		: �������ݐ�̃X�g���[��   *
* �Ԃ�l                                   *
*	�������񂾃u���b�N��                   *
*******************************************/
EXTERN size_t FileWrite(void* src, size_t block_size, size_t read_num, GFileOutputStream* stream);

/********************************************
* FileSeek�֐�                              *
* Gtk�̊֐��𗘗p���ăt�@�C���V�[�N         *
* ����                                      *
* stream	: �V�[�N���s���X�g���[��        *
* offset	: �ړ��o�C�g��                  *
* origin	: �ړ��J�n�ʒu(fseek�֐��Ɠ���) *
* �Ԃ�l                                    *
*	����I��(0), �ُ�I��(0�ȊO)            *
********************************************/
EXTERN int FileSeek(void* stream, long offset, int origin);

/************************************************
* FileSeekTell�֐�                              *
* Gtk�̊֐��𗘗p���ăt�@�C���̃V�[�N�ʒu��Ԃ� *
* ����                                          *
* stream	: �V�[�N�ʒu�𒲂ׂ�X�g���[��      *
* �Ԃ�l                                        *
*	�V�[�N�ʒu                                  *
************************************************/
EXTERN long FileSeekTell(void* stream);

EXTERN int ForFontFamilySearchCompare(const char* str, PangoFontFamily** font);

EXTERN void AdjustmentChangeValueCallBackInt(GtkAdjustment* adjustment, int* store);
EXTERN void AdjustmentChangeValueCallBackUint(GtkAdjustment* adjustment, unsigned int* store);
EXTERN void AdjustmentChangeValueCallBackInt8(GtkAdjustment* adjustment, int8* store);
EXTERN void AdjustmentChangeValueCallBackUint8(GtkAdjustment* adjustment, uint8* store);
EXTERN void AdjustmentChangeValueCallBackInt16(GtkAdjustment* adjustment, int16* store);
EXTERN void AdjustmentChangeValueCallBackUint16(GtkAdjustment* adjustment, uint16* store);
EXTERN void AdjustmentChangeValueCallBackInt32(GtkAdjustment* adjustment, int32* store);
EXTERN void AdjustmentChangeValueCallBackUint32(GtkAdjustment* adjustment, uint32* store);
EXTERN void AdjustmentChangeValueCallBackDouble(GtkAdjustment* adjustment, gdouble* value);
EXTERN void AdjustmentChangeValueCallBackDoubleRate(GtkAdjustment* adjustment, gdouble* value);
EXTERN void SetAdjustmentChangeValueCallBack(
	GtkAdjustment* adjustment,
	void (*func)(void*),
	void* func_data
);

EXTERN void CheckButtonSetFlagsCallBack(GtkWidget* button, unsigned int* flags, unsigned int flag_value);
EXTERN void UpdateWidget(GtkWidget* widget);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_UTILS_GTK_H_
