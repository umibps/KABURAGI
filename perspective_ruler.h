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
* SetPerspectiveRulerClickPoint関数       *
* 左クリック時にパース定規の設定を行う    *
* 引数                                    *
* ruler	: パース定規設定                  *
* x		: 左クリックしたキャンバスのX座標 *
* y		: 左クリックしたキャンバスのY座標 *
******************************************/
EXTERN void SetPerspectiveRulerClickPoint(PERSPECTIVE_RULER* ruler, FLOAT_T x, FLOAT_T y);

/**************************************
* GetPerspenctiveRulerPoint関数       *
* パース定規の補正後座標を取得する    *
* 引数                                *
* ruler	: パース定規設定              *
* in_x	: 補正前X座標                 *
* in_y	: 補正前Y座標                 *
* out_x	: 補正後X座標を入れるアドレス *
* out_y	: 補正後Y座標を入れるアドレス *
**************************************/
EXTERN void GetPerspectiveRulerPoint(PERSPECTIVE_RULER* ruler, FLOAT_T in_x, FLOAT_T in_y, FLOAT_T* out_x, FLOAT_T* out_y);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_PERSPECTIVE_RULER_H_
