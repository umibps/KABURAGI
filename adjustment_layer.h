#ifndef _INCLUDED_ADJUSTMENT_LAYER_H_
#define _INCLUDED_ADJUSTMENT_LAYER_H_

#include "types.h"
#include "color.h"

typedef enum _eADJUSTMENT_LAYER_TYPE
{
	ADJUSTMENT_LAYER_TYPE_BRIGHT_CONTRAST,
	ADJUSTMENT_LAYER_TYPE_HUE_SATURATION,
	NUM_ADJUSTMENT_LAYER_TYPE
} eADJUSTMENT_LAYER_TYPE;

typedef enum _eADJUSTMENT_LAYER_TARGET
{
	ADJUSTMENT_LAYER_TARGET_UNDER_LAYER,
	ADJUSTMENT_LAYER_TARGET_UNDER_MIXED,
	NUM_ADJUSTMENT_LAYER_TARGET
} eADJUSTMENT_LAYER_TARGET;

typedef struct _ADJUSTMENT_LAYER
{
	uint8 type;
	uint8 target;
	struct _LAYER *target_layer;
	struct _LAYER *self;
	
	union
	{
		struct
		{
			int bright;
			int contrast;
			uint8 *pixels;
			uint64 sum_r, sum_g, sum_b, sum_a;
		} bright_contrast;
		
		struct
		{
			int h, s, v;
		} hue_saturation;
	} filter_data;

	void (*initialize)(struct _ADJUSTMENT_LAYER* layer, struct _LAYER* target,struct _LAYER* mixed);
	void (*release)(struct _ADJUSTMENT_LAYER* layer);
	void (*update)(struct _ADJUSTMENT_LAYER* layer, struct _LAYER* target, struct _LAYER* mixed,
		int start_x, int start_y, int width, int height);
	void (*filter_func)(struct _ADJUSTMENT_LAYER* layer, uint8* input, uint8* output,
		unsigned int length, struct _LAYER* target);
} ADJUSTMENT_LAYER;

/*********************************
* ReleaseAdjustmentLayer�֐�     *
* �������C���[�̃��������J������ *
* ����                           *
* layer	: �������C���[           *
*********************************/
EXTERN void ReleaseAdjustmentLayer(ADJUSTMENT_LAYER** layer);

/*****************************************
* ReadAdjustmentLayerData�֐�            *
* �������C���[�̃f�[�^��ǂݍ���         *
* ����                                   *
* stream	: �ǂݍ��݌��̃f�[�^         *
* read_func	: �ǂݍ��݂Ɏg���֐��|�C���^ *
* layer		: �ǂݍ��ݐ�̒������C���[   *
*****************************************/
EXTERN void ReadAdjustmentLayerData(void* stream, stream_func_t read_func, ADJUSTMENT_LAYER* layer);

/*********************************************
* WriteAdjustmentLayerData�֐�               *
* �������C���[�̃f�[�^�������o��             *
* ����                                       *
* stream		: �����o����̃|�C���^       *
* write_func	: �����o���Ɏg���֐��|�C���^ *
* layer			: �����o���������C���[       *
*********************************************/
EXTERN void WriteAdjustmentLayerData(void* stream, stream_func_t write_func, ADJUSTMENT_LAYER* layer);

/********************************************************************************
* SetAdjustmentLayerType                                                        *
* �������C���[�̃��[�h��ݒ�                                                    *
* ����                                                                          *
* layer			: �������C���[                                                  *
* type			; �������C���[�̃^�C�v                                          *
********************************************************************************/
EXTERN void SetAdjustmentLayerType(
	ADJUSTMENT_LAYER* layer,
	eADJUSTMENT_LAYER_TYPE type
);

/****************************************************************************
* SetAdjustmentLayerTarget�֐�                                              *
* �������C���[��K�p���鑊���ݒ�                                          *
* layer		: �������C���[                                                  *
* target	: �������C���[���K�p���鑊��(�����̃��C���[/���̃��C���[�̓���) *
****************************************************************************/
EXTERN void SetAdjustmentLayerTarget(ADJUSTMENT_LAYER* layer, eADJUSTMENT_LAYER_TARGET target);

#endif	// #ifndef _INCLUDED_ADJUSTMENT_LAYER_H_
