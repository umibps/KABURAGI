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
* ReleaseAdjustmentLayer関数     *
* 調整レイヤーのメモリを開放する *
* 引数                           *
* layer	: 調整レイヤー           *
*********************************/
EXTERN void ReleaseAdjustmentLayer(ADJUSTMENT_LAYER** layer);

/*****************************************
* ReadAdjustmentLayerData関数            *
* 調整レイヤーのデータを読み込む         *
* 引数                                   *
* stream	: 読み込み元のデータ         *
* read_func	: 読み込みに使う関数ポインタ *
* layer		: 読み込み先の調整レイヤー   *
*****************************************/
EXTERN void ReadAdjustmentLayerData(void* stream, stream_func_t read_func, ADJUSTMENT_LAYER* layer);

/*********************************************
* WriteAdjustmentLayerData関数               *
* 調整レイヤーのデータを書き出す             *
* 引数                                       *
* stream		: 書き出し先のポインタ       *
* write_func	: 書き出しに使う関数ポインタ *
* layer			: 書き出す調整レイヤー       *
*********************************************/
EXTERN void WriteAdjustmentLayerData(void* stream, stream_func_t write_func, ADJUSTMENT_LAYER* layer);

/********************************************************************************
* SetAdjustmentLayerType                                                        *
* 調整レイヤーのモードを設定                                                    *
* 引数                                                                          *
* layer			: 調整レイヤー                                                  *
* type			; 調整レイヤーのタイプ                                          *
********************************************************************************/
EXTERN void SetAdjustmentLayerType(
	ADJUSTMENT_LAYER* layer,
	eADJUSTMENT_LAYER_TYPE type
);

/****************************************************************************
* SetAdjustmentLayerTarget関数                                              *
* 調整レイヤーを適用する相手を設定                                          *
* layer		: 調整レイヤー                                                  *
* target	: 調整レイヤーが適用する相手(直下のレイヤー/下のレイヤーの統合) *
****************************************************************************/
EXTERN void SetAdjustmentLayerTarget(ADJUSTMENT_LAYER* layer, eADJUSTMENT_LAYER_TARGET target);

#endif	// #ifndef _INCLUDED_ADJUSTMENT_LAYER_H_
