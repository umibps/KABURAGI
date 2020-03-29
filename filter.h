#ifndef _INCLUDED_FILTER_H_
#define _INCLUDED_FILTER_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************
* ExecuteBlurFilter�֐�                              *
* �ڂ����t�B���^�����s                               *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteBlurFilter(APPLICATION* app);

/*********************************************************
* ApplyBlurFilter�֐�                                    *
* �ڂ����t�B���^�[��K�p����                             *
* ����                                                   *
* target	: �ڂ����t�B���^�[��K�p���郌�C���[         *
* size		: �ڂ����t�B���^�[�ŕ��ϐF���v�Z�����`�͈� *
*********************************************************/
EXTERN void ApplyBlurFilter(LAYER* target, int size);

/*****************************************************
* ExecuteMotionBlurFilter�֐�                        *
* ���[�V�����ڂ����t�B���^�����s                     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteMotionBlurFilter(APPLICATION* app);

/*****************************************************
* ExecuteGaussianBlurFilter�֐�                      *
* �K�E�V�A���ڂ����t�B���^�����s                     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteGaussianBlurFilter(APPLICATION* app);

/*****************************************************
* ExecuteChangeBrightContrastFilter�֐�              *
* ���邳�R���g���X�g�t�B���^�����s                   *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteChangeBrightContrastFilter(APPLICATION* app);

/*****************************************************
* ExecuteChangeHueSaturationFilter�֐�               *
* �F���E�ʓx�t�B���^�����s                           *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteChangeHueSaturationFilter(APPLICATION* app);

/*****************************************************
* ExecuteColorLevelAdjust�֐�                        *
* �F�̃��x���␳�t�B���^�[�����s                     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteColorLevelAdjust(APPLICATION* app);

/*****************************************************
* ExecuteToneCurve�֐�                               *
* �g�[���J�[�u�t�B���^�[�����s                       *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteToneCurve(APPLICATION* app);

/*****************************************************
* ExecuteLuminosity2OpacityFilter�֐�                *
* �P�x�𓧖��x�֕ϊ������s                           *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteLuminosity2OpacityFilter(APPLICATION* app);

/*****************************************************
* ExecuteColor2AlphaFilter�֐�                       *
* �w��F�𓧖��x�֕ϊ������s                         *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteColor2AlphaFilter(APPLICATION* app);

/*****************************************************
* ExecuteColorizeWithUnderFilter�֐�                 *
* ���̃��C���[�Œ��F�����s                           *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteColorizeWithUnderFilter(APPLICATION* app);

/*****************************************************
* ExecuteGradationMapFilter�֐�                      *
* �O���f�[�V�����}�b�v�����s                         *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteGradationMapFilter(APPLICATION* app);

/*****************************************************
* ExecuteFillWithVectorLineFilter�֐�                *
* �x�N�g�����C���[�𗘗p���ĉ��̃��C���[��h��ׂ�   *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteFillWithVectorLineFilter(APPLICATION* app);

/*****************************************************
* ExecutePerlinNoiseFilter�֐�                       *
* �p�[�����m�C�Y�ŉ��h������s                       *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecutePerlinNoiseFilter(APPLICATION* app);

/*****************************************************
* ExecuteFractal�֐�                                 *
* �t���N�^���}�`�ŉ��h����s                         *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteFractal(APPLICATION* app);

/*****************************************************
* ExecuteTraceBitmap�֐�                             *
* �s�N�Z���f�[�^�̃x�N�g�����C���[�ւ̕ϊ������s     *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
EXTERN void ExecuteTraceBitmap(APPLICATION* app);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_FILTER_H_
