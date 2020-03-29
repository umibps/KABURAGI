#include <math.h>
#include "smoother.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************
* Smooth�֐�                                *
* ��u���␳���s��                          *
* ����                                      *
* smoother	: ��u���␳�̃f�[�^            *
* x			: ���݂�x���W�B�␳��̒l������ *
* y			: ���݂�y���W�B�␳��̒l������ *
* this_time	: ��u���␳�����J�n����        *
* zoom_rate	: �`��̈�̊g��k����          *
********************************************/
void Smooth(
	SMOOTHER* smoother,
	FLOAT_T *x,
	FLOAT_T *y,
	guint this_time,
	FLOAT_T zoom_rate
)
{
// �����f�[�^��ǉ�����臒l
#define IGNORE_THRESHOULD 0.0001f
// sqrt(2*PI)
#define SQRT_2_PI 1.7724538509055160272981674833411f

	// �����f�[�^�ɍ��W�f�[�^��ǉ�
	smoother->buffer[smoother->index].x = *x;
	smoother->buffer[smoother->index].y = *y;

	// �f�[�^�_������Ε␳���s��
	if(smoother->num_data > 0)
	{
		// �v�Z����
		FLOAT_T result_x = 0.0, result_y = 0.0;
		// �O��̏�������̋���
		FLOAT_T distance =
			((*x - smoother->before_point.x)*(*x - smoother->before_point.x)
			+ (*y - smoother->before_point.y)*(*y - smoother->before_point.y)) * zoom_rate;
		// �}�E�X�J�[�\���̈ړ����x
		FLOAT_T velocity;
		// �K�E�X�֐���K�p���銄��
		FLOAT_T weight = 0.0, weight2 = smoother->rate * smoother->rate;
		// ���x�̍��v�l
		FLOAT_T velocity_sum = 0.0;
		FLOAT_T scale_sum = 0.0;
		FLOAT_T rate;
		// wieight��0�łȂ����Ƃ̃t���O
		int none_0_wieight = 0;
		// �z��̃C���f�b�N�X
		int index;
		// for���p�̃J�E���^
		int i;

		if(weight2 != 0.0)
		{
			weight = 1.0 / (SQRT_2_PI * smoother->rate);
			none_0_wieight++;
		}
		
		distance = sqrt(distance);
		// �O�񂩂�̋�����臒l�ȏ�Ȃ�Η����f�[�^�ǉ�
		if(distance >= IGNORE_THRESHOULD)
		{
			velocity = distance * 10.0;
			smoother->velocity[smoother->index] = velocity;
			smoother->before_point = smoother->buffer[smoother->index];

			// �C���f�b�N�X�X�V�B�͈͂��I�[�o�[������͂��߂ɖ߂�
			smoother->index++;
			if(smoother->index >= smoother->num_use)
			{
				smoother->index = 0;
			}

			if(smoother->num_data < smoother->num_use)
			{
				smoother->num_data++;
			}
		}

		// ���x�x�[�X�̃K�E�V�A���d�ݕt���Ŏ�u���␳���s
		index = smoother->index-1;
		for(i=0; i<smoother->num_data; i++, index--)
		{
			rate = 0.0;
			if(index < 0)
			{
				index = smoother->num_use-1;
			}

			if(none_0_wieight != 0)
			{
				velocity_sum += smoother->velocity[index];
				rate = weight * exp(-velocity_sum*velocity_sum / (2.0*weight2));

				if(i == 0 && rate == 0.0)
				{
					rate = 1.0;
				}
			}
			else
			{
				rate = (i == 0) ? 1.0 : 0.0;
			}

			scale_sum += rate;
			result_x += rate * smoother->buffer[index].x;
			result_y += rate * smoother->buffer[index].y;
		}

		// 臒l�ȏ�Ȃ�␳���s(3/1
		if(scale_sum > 0.01)
		{
			result_x /= scale_sum;
			result_y /= scale_sum;
			*x = result_x, *y = result_y;
		}
	}
	else
	{
		smoother->before_point = smoother->buffer[smoother->index];
		smoother->velocity[smoother->index] = 0.0;
		smoother->index++;
		smoother->num_data = 1;
	}

	// ����̎��Ԃ��L��
	smoother->last_time = this_time;
}

/*********************************************
* AddAverageSmoothPoint�֐�                  *
* ���W���ω��ɂ���u���␳�Ƀf�[�^�_��ǉ� *
* ����                                       *
* smoother	: ��u���␳�̃f�[�^             *
* x			: �}�E�X��X���W                  *
* y			: �}�E�X��Y���W                  *
* pressure	: �M��                           *
* zoom_rate	: �`��̈�̊g��k����           *
* �Ԃ�l                                     *
*	�u���V�̕`����s��:TRUE	�s��Ȃ�:FALSE   *
*********************************************/
gboolean AddAverageSmoothPoint(
	SMOOTHER* smoother,
	FLOAT_T* x,
	FLOAT_T* y,
	FLOAT_T* pressure,
	FLOAT_T zoom_rate
)
{
	int ref_index = smoother->index % smoother->num_use;

	if(smoother->index == 0)
	{
		smoother->buffer[0].x = *x;
		smoother->buffer[0].y = *y;
		smoother->velocity[0] = *pressure;
		smoother->before_point.x = *x;
		smoother->before_point.y = *y;
		smoother->sum.x = *x;
		smoother->sum.y = *y;
		smoother->index++;
		smoother->last_time = 0;
	}
	else
	{
		if(fabs(smoother->before_point.x - *x) +
			 fabs(smoother->before_point.y - *y) < 0.8 * zoom_rate)
		{
			int before_index = ref_index - 1;
			if(before_index < 0)
			{
				before_index = smoother->num_use - 1;
				if(before_index < 0)
				{
					return FALSE;
				}
			}

			if(smoother->velocity[before_index] < *pressure)
			{
				smoother->velocity[before_index] = *pressure;
			}
			
			return FALSE;
		}

		if(smoother->index >= smoother->num_use)
		{
			FLOAT_T add_x = *x, add_y = *y, add_pressure = *pressure;
			*x = smoother->sum.x / smoother->num_use;
			*y = smoother->sum.y / smoother->num_use;
			*pressure = smoother->velocity[ref_index];
			smoother->sum.x -= smoother->buffer[ref_index].x;
			smoother->sum.y -= smoother->buffer[ref_index].y;

			smoother->sum.x += add_x;
			smoother->sum.y += add_y;
			smoother->before_point.x = smoother->buffer[ref_index].x = add_x;
			smoother->before_point.y = smoother->buffer[ref_index].y = add_y;
			smoother->velocity[ref_index] = add_pressure;

			smoother->index++;

			return TRUE;
		}
		else
		{
			smoother->sum.x += *x;
			smoother->sum.y += *y;
			smoother->before_point.x = smoother->buffer[ref_index].x = *x;
			smoother->before_point.y = smoother->buffer[ref_index].y = *y;
			smoother->velocity[ref_index] = *pressure;

			smoother->index++;
		}
	}

	return FALSE;
}

/****************************************************
* AverageSmoothFlush�֐�                            *
* ���ω��ɂ���u���␳�̎c��o�b�t�@��1���o�� *
* ����                                              *
* smoother	: ��u���␳�̏��                      *
* x			: �}�E�X��X���W                         *
* y			: �}�E�X��Y���W                         *
* pressure	: �M��                                  *
* �Ԃ�l                                            *
*	�o�b�t�@�̎c�薳��:TRUE	�c��L��:FALSE          *
****************************************************/
gboolean AverageSmoothFlush(
	SMOOTHER* smoother,
	FLOAT_T* x,
	FLOAT_T* y,
	FLOAT_T* pressure
)
{
	int ref_index;

	if(smoother->last_time == 0)
	{
		smoother->last_time++;
		if(smoother->index >= smoother->num_use)
		{
			smoother->num_data = smoother->num_use;
		}
		else
		{
			smoother->num_data = smoother->index;
			smoother->index = 0;
		}
	}

	ref_index = smoother->index % smoother->num_use;
	*x = smoother->sum.x / smoother->num_data;
	*y = smoother->sum.y / smoother->num_data;
	*pressure = smoother->velocity[ref_index];
	smoother->sum.x -= smoother->buffer[ref_index].x;
	smoother->sum.y -= smoother->buffer[ref_index].y;
	smoother->index++;
	smoother->num_data--;

	if(smoother->num_data <= 0)
	{
		*x = smoother->buffer[ref_index].x;
		*y = smoother->buffer[ref_index].y;

		return TRUE;
	}

	return FALSE;
}

/*********************************
* MotionQueueAppendItem�֐�      *
* �}�E�X�J�[�\���̍��W��ǉ����� *
* ����                           *
* queue		: ���W�Ǘ��̃f�[�^   *
* x			: �ǉ�����X���W      *
* y			: �ǉ�����Y���W      *
* pressure	: �ǉ�����M��       *
* state		: �V�t�g�L�[���̏�� *
*********************************/
void MotionQueueAppendItem(
	MOTION_QUEUE* queue,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T pressure,
	unsigned int state
)
{
	int index;

	queue->last_queued_x = x;
	queue->last_queued_y = y;

	if(queue->num_items >= queue->max_items)
	{
		index = (queue->start_index + queue->max_items - 1) % queue->max_items;
	}
	else
	{
		index = (queue->start_index + queue->num_items) % queue->max_items;
		queue->num_items++;
	}

	queue->queue[index].x = x;
	queue->queue[index].y = y;
	queue->queue[index].pressure = pressure;
	queue->queue[index].state = state;
}

#ifdef __cplusplus
}
#endif
