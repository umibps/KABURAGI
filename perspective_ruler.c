#include <math.h>
#include "perspective_ruler.h"

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
* GetPerspenctiveRulerPoint�֐�       *
* �p�[�X��K�̕␳����W���擾����    *
* ����                                *
* ruler	: �p�[�X��K�ݒ�              *
* in_x	: �␳�OX���W                 *
* in_y	: �␳�OY���W                 *
* out_x	: �␳��X���W������A�h���X *
* out_y	: �␳��Y���W������A�h���X *
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
