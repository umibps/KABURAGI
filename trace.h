#ifndef _INCLUDED_TRACE_H_
#define _INCLUDED_TRACE_H_

#include "types.h"

// �r�b�g�}�b�v�f�[�^�g���[�X�̃��[�h
typedef enum _eBITMAP_TRACE_MODE
{
	BITMAP_TRACE_MODE_GRAY_SCALE,
	BITMAP_TRACE_MODE_BRIGHTNESS,
	BITMAP_TRACE_MODE_COLOR
} eBITMAP_TRACE_MODE;

typedef enum _eBITMAP_TRACE_TURN_POLICY
{
	BITMAP_TRACE_TURN_POLICY_BLACK,
	BITMAP_TRACE_TURN_POLICY_WHITE,
	BITMAP_TRACE_TURN_POLICY_LEFT,
	BITMAP_TRACE_TURN_POLICY_RIGHT,
	BITMAP_TRACE_TURN_POLICY_MINORITY,
	BITMAP_TRACE_TURN_POLICY_MAJORITY,
	BITMAP_TRACE_TURN_POLICY_RANDOM
} eBITMAP_TRACE_TURN_POLICY;

typedef enum _eBITMAP_TRACE_TARGET
{
	BITMAP_TRACE_TARGET_LAYER,
	BITMAP_TRACE_TARGET_CANVAS,
	NUM_BITMAP_TRACE_TARGET
} eBITMAP_TRACE_TARGET;

typedef enum _eBITMAP_TRACE_FALGS
{
	BITMAP_TRACE_FALGS_USE_SPECIFY_LINE_COLOR = 0x01
} eBITMAP_TRACE_FALGS;

typedef struct _BITMAP_TRACER
{
	void *path;
	uint8* trace_pixels;
	int width;
	int height;
	int trace_mode;
	int minimum_path_size;
	int minimum_point_distance;
	int optimize_curve;
	unsigned int flags;
	FLOAT_T optimize_tolerance;
	FLOAT_T alpha_max;
	uint8 trace_target;
	uint8 line_color[4];
	int (*threshold)(struct _BITMAP_TRACER*, uint8*, int, int);
	int (*sign)(struct _BITMAP_TRACER*, uint8*, int, int);
	void (*clear)(struct _BITMAP_TRACER*, uint8*, int, int);
	int (*blur)(struct _BITMAP_TRACER*, uint8*, int, int);
	void (*progress_update)(void*, FLOAT_T);
	eBITMAP_TRACE_TURN_POLICY trace_policy;
	void *progress_data;

	struct
	{
		int black_white_threshold;
		int opacity_threshold;
	} threshold_data;
} BITMAP_TRACER;

// �֐��̃v���g�^�C�v�錾
/*****************************************************
* InitializeBitmapTracer�֐�                         *
* �r�b�g�}�b�v�f�[�^���g���[�X����f�[�^������������ *
* ����                                               *
* tracer	: �g���[�T�[�̃f�[�^                     *
* pixels	: �r�b�g�}�b�v�f�[�^                     *
* width		: �r�b�g�}�b�v�f�[�^�̕�                 *
* height	: �r�b�g�}�b�v�f�[�^�̍���               *
*****************************************************/
EXTERN void InitializeBitmapTracer(BITMAP_TRACER* tracer, uint8* pixels, int width, int height);

/***************************************************
* ReleaseBitmapTracer�֐�                          *
* �r�b�g�}�b�v�f�[�^���g���[�X����f�[�^���J������ *
* ����                                             *
* tracer	: �g���[�T�[�̃f�[�^                   *
***************************************************/
EXTERN void ReleaseBitmapTracer(BITMAP_TRACER* tracer);

/***********************************************
* TraceBitmap�֐�                              *
* �r�b�g�}�b�v�f�[�^���x�N�g���f�[�^�ɕϊ����� *
* ����                                         *
* tracer	: �g���[�T�[�̃f�[�^               *
* �Ԃ�l                                       *
*	����I��:TRUE	�ُ�I��:FALSE             *
***********************************************/
EXTERN int TraceBitmap(BITMAP_TRACER* tracer);

/***********************************************
* AdoptTraceBitmapResult�֐�                   *
* �g���[�X�������ʂ��x�N�g�����C���[�ɓK�p���� *
* ����                                         *
* tracer	: �g���[�T�[�̃f�[�^               *
* layer		: �K�p��̃x�N�g�����C���[         *
***********************************************/
EXTERN void AdoptTraceBitmapResult(BITMAP_TRACER* tracer, void* layer);

#endif	// #ifndef _INCLUDED_TRACE_H_
