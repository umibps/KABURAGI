#include <math.h>
#include "perspective_ruler.h"

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
void SetPerspectiveRulerClickPoint(PERSPECTIVE_RULER* ruler, FLOAT_T x, FLOAT_T y)
{
	ruler->click_point[0] = x,	ruler->click_point[1] = y;
	switch(ruler->type)
	{
	case PERSPECTIVE_RULER_TYPE_CIRCLE:
		ruler->angle = atan2(y - ruler->start_point[ruler->active_point][0][1],
			x - ruler->start_point[ruler->active_point][0][0]);
		ruler->sin_value = sin(ruler->angle);
		ruler->cos_value = cos(ruler->angle);
		break;
	case PERSPECTIVE_RULER_TYPE_PARALLEL_LINES:
		ruler->angle = atan2(ruler->start_point[ruler->active_point][1][1] - ruler->start_point[ruler->active_point][0][1],
			ruler->start_point[ruler->active_point][1][0] - ruler->start_point[ruler->active_point][0][0]);
		ruler->sin_value = sin(ruler->angle);
		ruler->cos_value = cos(ruler->angle);
		break;
	}
}

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
void GetPerspectiveRulerPoint(PERSPECTIVE_RULER* ruler, FLOAT_T in_x, FLOAT_T in_y, FLOAT_T* out_x, FLOAT_T* out_y)
{
	FLOAT_T distance;
	distance = sqrt((in_x - ruler->click_point[0]) * (in_x - ruler->click_point[0])
		+ (in_y - ruler->click_point[1]) * (in_y - ruler->click_point[1]));
	switch(ruler->type)
	{
	case PERSPECTIVE_RULER_TYPE_CIRCLE:
		*out_x = ruler->click_point[0] + distance * ruler->cos_value;
		*out_y = ruler->click_point[1] + distance * ruler->sin_value;
		break;
	case PERSPECTIVE_RULER_TYPE_PARALLEL_LINES:
		*out_x = ruler->click_point[0] + distance * ruler->cos_value;
		*out_y = ruler->click_point[1] + distance * ruler->sin_value;
		break;
	}
}

#ifdef __cplusplus
}
#endif
