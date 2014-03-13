#ifndef _INCLUDED_BEZIER_H_
#define _INCLUDED_BEZIER_H_

#include "vector.h"
#include "draw_window.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************
* BEZIER_POINT構造体       *
* ベジェ曲線での制御点座標 *
***************************/
typedef struct _BEZIER_POINT
{
	FLOAT_T x, y;
} BEZIER_POINT;

// 関数のプロトタイプ宣言
/******************************************************
* MakeBezier3EdgeControlPoint関数                     *
* 3点の座標から3次ベジェ曲線の端点での座標を決定する  *
* 引数                                                *
* src		: ベジェ曲線が通る座標                    *
* control	: 2次ベジェ曲線の制御点を記憶するアドレス *
******************************************************/
extern void MakeBezier3EdgeControlPoint(BEZIER_POINT* src, BEZIER_POINT* control);

/*************************************************
* CalcBezier2関数                                *
* 2次ベジェ曲線の媒介変数tにおける座標を計算する *
* 引数                                           *
* points	: ベジェ曲線の制御点                 *
* t			: 媒介変数t                          *
* dest		: 座標データを記憶するアドレス       *
*************************************************/
extern void CalcBezier2(BEZIER_POINT* points, FLOAT_T t, BEZIER_POINT* dest);

extern void MakeBezier3ControlPoints(BEZIER_POINT* src, BEZIER_POINT* controls);

extern void CalcBezier3(BEZIER_POINT* points, FLOAT_T t, BEZIER_POINT* dest);

extern void StrokeBezierLine(
	DRAW_WINDOW* window,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
);

extern void StrokeBezierLineClose(
	DRAW_WINDOW* window,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_BEZIER_H_
