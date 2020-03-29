#ifndef _INCLUDED_ANTI_ALIAS_H_
#define _INCLUDED_ANTI_ALIAS_H_

#include "types.h"
#include "layer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************
* ANTI_ALIAS_RECTANGLE�\����   *
* �A���`�G�C���A�X�͈͎̔w��p *
*******************************/
typedef struct _ANTI_ALIAS_RECTANGLE
{
	int x, y;			// �A���`�G�C���A�X�������鍶��̍��W
	int width, height;	// �A���`�G�C���A�X��������͈͂̕��E����
} ANTI_ALIAS_RECTANGLE;

// �֐��̃v���g�^�C�v�錾
/***************************************************
* OldAntiAlias�֐�                                 *
* �A���`�G�C���A�V���O���������s                   *
* ����                                             *
* in_buff	: ���̓f�[�^                           *
* out_buff	: �o�̓f�[�^                           *
* width		: ���o�̓f�[�^�̕�                     *
* height	: ���o�̓f�[�^�̍���                   *
* stride	: ���o�̓f�[�^��1�s���̃o�C�g��        *
* app		: �A�v���P�[�V�����S�̂��Ǘ�����\���� *
***************************************************/
EXTERN void OldAntiAlias(
	uint8* in_buff,
	uint8* out_buff,
	int width,
	int height,
	int stride,
	void* app
);

/*******************************************************
* OldAntiAliasEditSelection�֐�                        *
* �N�C�b�N�}�X�N���̃A���`�G�C���A�V���O���������s     *
* ����                                                 *
* in_buff		: ���̓f�[�^                           *
* out_buff		: �o�̓f�[�^                           *
* width			: ���o�̓f�[�^�̕�                     *
* height		: ���o�̓f�[�^�̍���                   *
* stride		: ���o�̓f�[�^��1�s���̃o�C�g��        *
* application	: �A�v���P�[�V�����S�̂��Ǘ�����\���� *
*******************************************************/
EXTERN void OldAntiAliasEditSelection(
	uint8* in_buff,
	uint8* out_buff,
	int width,
	int height,
	int stride,
	void* application
);

/*************************************************************
* OldAntiAliasWithSelectionOrAlphaLock�֐�                   *
* �A���`�G�C���A�V���O���������s(�I��͈͂܂��͓����x�ی�L) *
* ����                                                       *
* selection_layer	: �I��͈�                               *
* target_layer		: �����x�ی�ɑΏۃ��C���[               *
* in_buff			: ���̓f�[�^                             *
* out_buff			: �o�̓f�[�^                             *
* start_x			: �A���`�G�C���A�X�������鍶���X���W    *
* start_y			: �A���`�G�C���A�X�������鍶���Y���W    *
* width				: ���o�̓f�[�^�̕�                       *
* height			: ���o�̓f�[�^�̍���                     *
* stride			: ���o�̓f�[�^��1�s���̃o�C�g��          *
* application		: �A�v���P�[�V�����S�̂��Ǘ�����\����   *
*************************************************************/
EXTERN void OldAntiAliasWithSelectionOrAlphaLock(
	LAYER* selection_layer,
	LAYER* target_layer,
	uint8* in_buff,
	uint8* out_buff,
	int start_x,
	int start_y,
	int width,
	int height,
	int stride,
	void* application
);

/*********************************************************************
* AntiAliasLayer�֐�                                                 *
* ���C���[�ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X��������             *
* layer			: �A���`�G�C���A�X�������郌�C���[                   *
* out			: ���ʂ����郌�C���[                               *
* rect			: �A���`�G�C���A�X��������͈�                       *
* applicatoion	: �A�v���P�[�V�����S�̂��Ǘ�����\���̊Ǘ�����\���� *
*********************************************************************/
EXTERN void AntiAliasLayer(LAYER *layer, LAYER* out, ANTI_ALIAS_RECTANGLE *rect, void* application);

/*************************************************************************************
* AntiAliasLayerWithSelectionOrAlphaLock�֐�                                         *
* ���C���[�ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X��������(�I��͈͂܂��͓����x�ی�L) *
* layer			: �A���`�G�C���A�X�������郌�C���[                                   *
* out			: ���ʂ����郌�C���[                                               *
* selection		: �I��͈�                                                           *
* target_layer	: �A�N�e�B�u�ȃ��C���[                                               *
* rect			: �A���`�G�C���A�X��������͈�                                       *
* applicatoion	: �A�v���P�[�V�����S�̂��Ǘ�����\���̊Ǘ�����\����                 *
*************************************************************************************/
EXTERN void AntiAliasLayerWithSelectionOrAlphaLock(
	LAYER *layer,
	LAYER* out,
	LAYER* selection,
	LAYER* target_layer,
	ANTI_ALIAS_RECTANGLE *rect,
	void* application
);

/*********************************************************************
* OldAntiAliasLayer�֐�                                              *
* ���C���[�ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X��������             *
* layer			: �A���`�G�C���A�X�������郌�C���[                   *
* rect			: �A���`�G�C���A�X��������͈�                       *
* applicatoion	: �A�v���P�[�V�����S�̂��Ǘ�����\���̊Ǘ�����\���� *
*********************************************************************/
EXTERN void OldAntiAliasLayer(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect, void* application);

/*************************************************************************************
* OldAntiAliasLayerWithSelectionOrAlphaLock�֐�                                      *
* ���C���[�ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X��������(�I��͈͂܂��͓����x�ی�L) *
* layer			: �A���`�G�C���A�X�������郌�C���[                                   *
* selection		: �I��͈�                                                           *
* target_layer	: �A�N�e�B�u�ȃ��C���[                                               *
* rect			: �A���`�G�C���A�X��������͈�                                       *
* applicatoion	: �A�v���P�[�V�����S�̂��Ǘ�����\���̊Ǘ�����\����                 *
*************************************************************************************/
EXTERN void OldAntiAliasLayerWithSelectionOrAlphaLock(
	LAYER *layer,
	LAYER* temp,
	LAYER* selection,
	LAYER* target_layer,
	ANTI_ALIAS_RECTANGLE *rect,
	void* application
);

/*********************************************************************
* AntiAliasVectorLine�֐�                                            *
* �x�N�g�����C���[�̐��ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X�������� *
* layer	: �A���`�G�C���A�X�������郌�C���[                           *
* rect	: �A���`�G�C���A�X��������͈�                               *
*********************************************************************/
EXTERN void AntiAliasVectorLine(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_ANTI_ALIAS_H_
