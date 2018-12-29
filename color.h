#ifndef _INCLUDED_COLOR_H_
#define _INCLUDED_COLOR_H_

#include "lcms/lcms2.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PALLETE_BOX_SIZE 10
#define MAX_COLOR_HISTORY_NUM 10

typedef enum _eCOLOR
{
	COLOR_GLAY,
	COLOR_YELLOW,
	COLOR_PLUM,
	COLOR_RED,
	COLOR_LIME,
	COLOR_AQUA,
	COLOR_GREEN,
	COLOR_MAROON,
	COLOR_NAVY,
	COLOR_OLIVE,
	COLOR_PURPLE,
	COLOR_TEAL,
	COLOR_BLUE,
	NUM_COLOR
} eCOLOR;

typedef struct _HSV
{
	int16 h;
	uint8 s;
	uint8 v;
} HSV;

typedef struct _CMYK
{
	uint8 c;
	uint8 m;
	uint8 y;
	uint8 k;
} CMYK;

typedef struct _CMYK16
{
	uint16 c;
	uint16 m;
	uint16 y;
	uint16 k;
} CMYK16;

/************************************
* COLOR_HISTGRAM構造体              *
* RGBA、CMYK、HSVのヒストグラムの値 *
************************************/
typedef struct _COLOR_HISTGRAM
{
	unsigned int r[256];
	uint8 r_min, r_max;
	unsigned int g[256];
	uint8 g_min, g_max;
	unsigned int b[256];
	uint8 b_min, b_max;
	unsigned int a[256];
	uint8 a_min, a_max;
	unsigned int h[360];
	unsigned int s[256];
	uint8 s_min, s_max;
	unsigned int v[256];
	uint8 v_min, v_max;
	unsigned int c[256];
	uint8 c_min, c_max;
	unsigned int m[256];
	uint8 m_min, m_max;
	unsigned int y[256];
	uint8 y_min, y_max;
	unsigned int k[256];
	uint8 k_min, k_max;
} COLOR_HISTGRAM;

typedef enum _eCOLOR_CHOOSER_FLAGS
{
	COLOR_CHOOSER_MOVING_CIRCLE = 0x01,
	COLOR_CHOOSER_MOVING_SQUARE = 0x02,
	COLOR_CHOOSER_SHOW_CIRCLE = 0x04,
	COLOR_CHOOSER_SHOW_PALLETE = 0x08
} eCOLOR_CHOOSER_FLAGS;

EXTERN void RGB2HSV_Pixel(uint8 rgb[3], HSV* hsv);

EXTERN HSV* RGB2HSV(
	uint8 *pixels,
	int32 width,
	int32 height,
	int32 channel
);

EXTERN void HSV2RGB_Pixel(HSV* hsv, uint8 rgb[3]);

EXTERN void HSV2RGB(
	HSV* hsv,
	uint8 *pixels,
	int32 width,
	int32 height,
	int32 channel
);

EXTERN void RGB2CMYK_Pixel(uint8 rgb[3], CMYK* cmyk);

EXTERN void CMYK2RGB_Pixel(CMYK* cmyk, uint8 rgb[3]);

/*************************************************
* ReadACO関数                                    *
* Adobe Color Fileを読み込む                     *
* 引数                                           *
* src			: データへのポインタ             *
* read_func		: 読み込みに使用する関数ポインタ *
* rgb			: 読み込んだRGBデータの格納先    *
* buffer_size	: 最大読み込み数                 *
* 返り値                                         *
*	読み込みに成功したデータの数                 *
*************************************************/
EXTERN int ReadACO(
	void* src,
	stream_func_t read_func,
	uint8 (*rgb)[3],
	int buffer_size
);

/***********************************************
* WriteACOファイル                             *
* Adobe Color Fileに書き出す                   *
* 引数                                         *
* dst			: 書き出し先へのポインタ       *
* write_func	: 書き出し用の関数へのポインタ *
* rgb			: 書きだすRGBデータ配列        *
* write_num		: 書きだすデータの数           *
***********************************************/
EXTERN void WriteACO(
	void* dst,
	stream_func_t write_func,
	uint8 (*rgb)[3],
	int write_num
);

/***************************************
* LoadPallete関数                      *
* パレットデータを読み込む             *
* 引数                                 *
* file_path	: データファイルのパス     *
* rgb		: 読み込んだデータの格納先 *
* max_read	: 読み込みを行う最大数     *
* 返り値                               *
*	読み込んだデータの数               *
***************************************/
EXTERN int LoadPallete(
	const char* file_path,
	uint8 (*rgb)[3],
	int max_read
);

EXTERN cmsHPROFILE* CreateDefaultSrgbProfile(void);
EXTERN cmsHPROFILE* GetPrimaryMonitorProfile(void);

EXTERN void GetColor(eCOLOR color_index, uint8* color);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_COLOR_H_
