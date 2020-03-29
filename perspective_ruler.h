#ifndef _INCLUDED_PERSPECTIVE_RULER_H_
#define _INCLUDED_PERSPECTIVE_RULER_H_

#include "types.h"

typedef enum _ePERSPECTIVE_RULER_TYPE
{
	PERSPECTIVE_RULER_TYPE_CIRCLE,
	PERSPECTIVE_RULER_TYPE_PARALLEL_LINES
} ePERSPECTIVE_RULER_TYPE;

typedef struct _PERSPECTIVE_RULER
{
	ePERSPECTIVE_RULER_TYPE type;
	FLOAT_T start_point[3][2][2];
	FLOAT_T click_point[2];
	int active_point;
	FLOAT_T angle;
	FLOAT_T sin_value;
	FLOAT_T cos_value;
} PERSPECTIVE_RULER;

#ifdef __cplusplus
extern "C" {
#endif

/******************************************
* SetPerspectiveRulerClickPoint�֐�       *
* ���N���b�N���Ƀp�[�X��K�̐ݒ���s��    *
* ����                                    *
* ruler	: �p�[�X��K�ݒ�                  *
* x		: ���N���b�N�����L�����o�X��X���W *
* y		: ���N���b�N�����L�����o�X��Y���W *
******************************************/
EXTERN void SetPerspectiveRulerClickPoint(PERSPECTIVE_RULER* ruler, FLOAT_T x, FLOAT_T y);

/**************************************
* GetPerspenctiveRulerPoint�֐�       *
* �p�[�X��K�̕␳����W���擾����    *
* ����                                *
* ruler	: �p�[�X��K�ݒ�              *
* in_x	: �␳�OX���W                 *
* in_y	: �␳�OY���W                 *
* out_x	: �␳��X���W������A�h���X *
* out_y	: �␳��Y���W������A�h���X *
**************************************/
EXTERN void GetPerspectiveRulerPoint(PERSPECTIVE_RULER* ruler, FLOAT_T in_x, FLOAT_T in_y, FLOAT_T* out_x, FLOAT_T* out_y);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_PERSPECTIVE_RULER_H_
