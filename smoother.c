#include <math.h>
#include "smoother.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************
* Smooth関数                                *
* 手ブレ補正を行う                          *
* 引数                                      *
* smoother	: 手ブレ補正のデータ            *
* x			: 現在のx座標。補正後の値が入る *
* y			: 現在のy座標。補正後の値が入る *
* this_time	: 手ブレ補正処理開始時間        *
* zoom_rate	: 描画領域の拡大縮小率          *
********************************************/
void Smooth(
	SMOOTHER* smoother,
	FLOAT_T *x,
	FLOAT_T *y,
	guint this_time,
	FLOAT_T zoom_rate
)
{
// 履歴データを追加する閾値
#define IGNORE_THRESHOULD 0.0001f
// sqrt(2*PI)
#define SQRT_2_PI 1.7724538509055160272981674833411f

	// 履歴データに座標データを追加
	smoother->buffer[smoother->index].x = *x;
	smoother->buffer[smoother->index].y = *y;

	// データ点があれば補正を行う
	if(smoother->num_data > 0)
	{
		// 計算結果
		FLOAT_T result_x = 0.0, result_y = 0.0;
		// 前回の処理からの距離
		FLOAT_T distance =
			((*x - smoother->before_point.x)*(*x - smoother->before_point.x)
			+ (*y - smoother->before_point.y)*(*y - smoother->before_point.y)) * zoom_rate;
		// マウスカーソルの移動速度
		FLOAT_T velocity;
		// ガウス関数を適用する割合
		FLOAT_T weight = 0.0, weight2 = smoother->rate * smoother->rate;
		// 速度の合計値
		FLOAT_T velocity_sum = 0.0;
		FLOAT_T scale_sum = 0.0;
		FLOAT_T rate;
		// wieightが0でないことのフラグ
		int none_0_wieight = 0;
		// 配列のインデックス
		int index;
		// for文用のカウンタ
		int i;

		if(weight2 != 0.0)
		{
			weight = 1.0 / (SQRT_2_PI * smoother->rate);
			none_0_wieight++;
		}
		
		distance = sqrt(distance);
		// 前回からの距離が閾値以上ならば履歴データ追加
		if(distance >= IGNORE_THRESHOULD)
		{
			velocity = distance * 10.0;
			smoother->velocity[smoother->index] = velocity;
			smoother->before_point = smoother->buffer[smoother->index];

			// インデックス更新。範囲をオーバーしたらはじめに戻す
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

		// 速度ベースのガウシアン重み付けで手ブレ補正実行
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

		// 閾値以上なら補正実行(3/1
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

	// 今回の時間を記憶
	smoother->last_time = this_time;
}

/*********************************************
* AddAverageSmoothPoint関数                  *
* 座標平均化による手ブレ補正にデータ点を追加 *
* 引数                                       *
* smoother	: 手ブレ補正のデータ             *
* x			: マウスのX座標                  *
* y			: マウスのY座標                  *
* pressure	: 筆圧                           *
* zoom_rate	: 描画領域の拡大縮小率           *
* 返り値                                     *
*	ブラシの描画を行う:TRUE	行わない:FALSE   *
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
* AverageSmoothFlush関数                            *
* 平均化による手ブレ補正の残りバッファを1つ取り出す *
* 引数                                              *
* smoother	: 手ブレ補正の情報                      *
* x			: マウスのX座標                         *
* y			: マウスのY座標                         *
* pressure	: 筆圧                                  *
* 返り値                                            *
*	バッファの残り無し:TRUE	残り有り:FALSE          *
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
* MotionQueueAppendItem関数      *
* マウスカーソルの座標を追加する *
* 引数                           *
* queue		: 座標管理のデータ   *
* x			: 追加するX座標      *
* y			: 追加するY座標      *
* pressure	: 追加する筆圧       *
* state		: シフトキー等の情報 *
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
