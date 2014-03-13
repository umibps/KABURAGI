#ifndef _INCLUDED_DISPLAY_FILTER_H_
#define _INCLUDED_DISPLAY_FILTER_H_

typedef enum _eDISPLAY_FUNC_TYPE
{
	DISPLAY_FUNC_TYPE_NO_CONVERT,
	DISPLAY_FUNC_TYPE_GRAY_SCALE,
	DISPLAY_FUNC_TYPE_GRAY_SCALE_YIQ,
	DISPLAY_FUNC_TYPE_ICC_PROFILE,
	NUM_DISPLAY_FUNC_TYPE
} eDISPLAY_FUNC_TYPE;

extern void (*g_display_filter_funcs[])(uint8* source, uint8* destination, int num_pixel, void* filter_data);

/***********************************
* DISPLAY_FILTER構造体             *
* 表示時に適用するフィルターの情報 *
***********************************/
typedef struct _DISPLAY_FILTER
{
	void (*filter_func)(
		uint8* source, uint8* destination, int size, void* filter_data);
	void *filter_data;
} DISPLAY_FILTER;

// 関数のプロトタイプ宣言
/*********************************************
* RGB2GrayScaleFilter関数                    *
* RGBからグレースケールへ変換する            *
* 引数                                       *
* source		:                            *
* destination	: 変換後のデータ格納先       *
* num_pixel		: ピクセル数                 *
* filter_data	: フィルターで使用するデータ *
*********************************************/
extern void RGB2GrayScaleFilter(uint8* source, uint8* destination, int num_pixel, void* filter_data);

/*********************************************
* RGBA2GrayScaleFilter関数                   *
* RGBAからグレースケールへ変換する           *
* 引数                                       *
* source		:                            *
* destination	: 変換後のデータ格納先       *
* num_pixel		: ピクセル数                 *
* filter_data	: フィルターで使用するデータ *
*********************************************/
extern void RGBA2GrayScaleFilter(uint8* source, uint8* destination, int num_pixel, void* filter_data);

#endif	// #ifndef _INCLUDED_DISPLAY_FILTER_H_
