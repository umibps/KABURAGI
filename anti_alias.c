#include <string.h>
#include "types.h"
#include "anti_alias.h"
#include "layer.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************
* AntiAlias関数                             *
* アンチエイリアシング処理を実行            *
* 引数                                      *
* in_buff	: 入力データ                    *
* out_buff	: 出力データ                    *
* width		: 入出力データの幅              *
* height	: 入出力データの高さ            *
* stride	: 入出力データの1行分のバイト数 *
* channel	: 入出力データのチャンネル数    *
********************************************/
void AntiAlias(
	uint8* in_buff,
	uint8* out_buff,
	int width,
	int height,
	int stride,
	int channel
)
{
// アンチエイリアシングを行う1チャンネル分の閾値
#define ANTI_ALIAS_THRESHOLD (51*51)

	// アンチエイリアシングを行う閾値
	int threshold = ANTI_ALIAS_THRESHOLD * channel;
	// ピクセルデータ配列のインデックス
	int index, now_index;
	// 現在のピクセルと周囲のピクセルの色差
	int color_diff[4];
	// 色差の合計
	int sum_diff;
	// 周囲のピクセルの合計値
	int sum_color[4];
	int i, j, k;	// for文用のカウンタ

	// 1行目はそのままコピー
	(void)memcpy(out_buff, in_buff, stride);

	// 2行目から一番下手前の行まで処理
	for(i=1; i<height-1; i++)
	{
		// 一番左はそのままコピー
		index = i * stride;
		for(j=0; j<channel; j++)
		{
			out_buff[i+j] = in_buff[i+j];
		}

		for(j=1; j<width-1; j++)
		{	// 色差合計をクリア
			color_diff[3] = color_diff[2] = color_diff[1] = color_diff[0] = 0;
			// 周囲のピクセルの合計値をクリア
			sum_color[3] = sum_color[2] = sum_color[1] = sum_color[0] = 0;

			// 周囲8ピクセルとの色差と値の合計を計算
			now_index = i*stride + j * channel;
			index = (i-1)*stride + (j-1) * channel;
			for(k=0; k<channel; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[now_index+k];
				sum_color[k] += in_buff[index+k];
			}
			index = (i-1)*stride + j * channel;
			for(k=0; k<channel; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i-1)*stride + (j+1) * channel;
			for(k=0; k<channel; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = i*stride + (j-1) * channel;
			for(k=0; k<channel; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = i*stride + (j+1) * channel;
			for(k=0; k<channel; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + (j-1) * channel;
			for(k=0; k<channel; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + j * channel;
			for(k=0; k<channel; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + (j+1) * channel;
			for(k=0; k<channel; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}

			sum_diff = color_diff[0];
			for(k=1; k<channel; k++)
			{
				sum_diff += color_diff[k];
			}

			// 色差の合計が閾値以上ならばアンチエイリアシング実行
			if(sum_diff > threshold)
			{	// 設定する新たなデータ
				uint8 new_value;

				for(k=0; k<channel; k++)
				{
					new_value = (uint8)(sum_color[k] / 9);
					if(new_value > in_buff[now_index+k])
					{
						out_buff[now_index+k] = new_value;
					}
					else
					{
						out_buff[now_index+k] = in_buff[now_index+k];
					}
				}
			}
			else
			{
				for(k=0; k<channel; k++)
				{
					out_buff[now_index+k] = in_buff[now_index+k];
				}
			}
		}
	}	// 2行目から一番下手前の行まで処理
		// for(i=1; i<height-1; i++)

	// 一番下の行はそのままコピー
	(void)memcpy(&out_buff[i*stride], &in_buff[i*stride], stride);
#undef ANTI_ALIAS_THRESHOLD
}

/*********************************************************
* AntiAliasLayer関数                                     *
* レイヤーに対して範囲を指定してアンチエイリアスをかける *
* layer	: アンチエイリアスをかけるレイヤー               *
* rect	: アンチエイリアスをかける範囲                   *
*********************************************************/
void AntiAliasLayer(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect)
{
// アンチエイリアシングを行う1チャンネル分の閾値
#define ANTI_ALIAS_THRESHOLD (51*51)
	// ピクセルデータ配列のインデックス
	int index, now_index;
	// 現在のピクセルと周囲のピクセルの色差
	int color_diff[4];
	// 色差の合計
	int sum_diff;
	// 周囲のピクセルの合計値
	int sum_color[4];
	// アンチエイリアス開始・終了の座標
	int x, y, end_x, end_y;
	// 処理1行分のバイト数
	int stride;
	int i, j;	// for文用のカウンタ

	// 座標と範囲を設定
	end_x = rect->width;
	end_y = rect->height;
	if(rect->x < 0)
	{
		end_x += rect->x;
		x = 0;
	}
	else if(rect->x > layer->width)
	{
		return;
	}
	else
	{
		x = rect->x;
	}
	end_x = x + end_x;
	if(end_x > layer->width)
	{
		end_x = layer->width;
	}
	if(end_x - x < 3)
	{
		return;
	}

	if(rect->y < 0)
	{
		end_y += rect->y;
		y = 0;
	}
	else if(rect->y > layer->height)
	{
		return;
	}
	else
	{
		y = rect->y;
	}
	end_y = y + end_y;
	if(end_y > layer->height)
	{
		end_y = layer->height;
	}

	// 2行目から一番下手前の行まで処理
#ifdef _OPENMP
#pragma omp parallel for
#endif
	for(i=y+1; i<end_y-1; i++)
	{
		// 一番左はそのままコピー
		now_index = i * layer->stride + (x+1)*4;

		for(j=x+1; j<end_x-1; j++, now_index+=4)
		{

			// 周囲8ピクセルとの色差と値の合計を計算
			index = (i-1)*layer->stride + (j-1)*4;
			color_diff[0] = ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] = layer->pixels[now_index+0];
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] = ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] = layer->pixels[now_index+1];
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] = ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] = layer->pixels[now_index+2];
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] = ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] = layer->pixels[now_index+3];
			sum_color[3] += layer->pixels[index+3];

			index += 4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index += 4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index = i*layer->stride + (j-1)*4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index += 8;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index = (i+1)*layer->stride + (j-1)*4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index += 4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index += 4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			sum_diff = color_diff[0] + color_diff[1] + color_diff[2] + color_diff[3];

			// 色差の合計が閾値以上ならばアンチエイリアシング実行
			if(sum_diff > ANTI_ALIAS_THRESHOLD)
			{
				temp->pixels[now_index+0] = (uint8)(sum_color[0] / 9);
				temp->pixels[now_index+1] = (uint8)(sum_color[1] / 9);
				temp->pixels[now_index+2] = (uint8)(sum_color[2] / 9);
				temp->pixels[now_index+3] = (uint8)(sum_color[3] / 9);
				/*
				// 設定する新たなデータ
				uint8 new_value;

				new_value = (uint8)(sum_color[0] / 9);
				if(new_value > layer->pixels[now_index+0])
				{
					temp->pixels[now_index+0] = new_value;
				}
				else
				{
					temp->pixels[now_index+0] = layer->pixels[now_index+0];
				}
				new_value = (uint8)(sum_color[1] / 9);
				if(new_value > layer->pixels[now_index+1])
				{
					temp->pixels[now_index+1] = new_value;
				}
				else
				{
					temp->pixels[now_index+1] = layer->pixels[now_index+1];
				}
				new_value = (uint8)(sum_color[2] / 9);
				if(new_value > layer->pixels[now_index+2])
				{
					temp->pixels[now_index+2] = new_value;
				}
				else
				{
					temp->pixels[now_index+2] = layer->pixels[now_index+2];
				}
				new_value = (uint8)(sum_color[3] / 9);
				if(new_value > layer->pixels[now_index+3])
				{
					temp->pixels[now_index+3] = new_value;
				}
				else
				{
					temp->pixels[now_index+3] = layer->pixels[now_index+3];
				}
				*/
			}
			else
			{
				temp->pixels[now_index] = layer->pixels[now_index];
				temp->pixels[now_index+1] = layer->pixels[now_index+1];
				temp->pixels[now_index+2] = layer->pixels[now_index+2];
				temp->pixels[now_index+3] = layer->pixels[now_index+3];
			}
		}
	}	// 2行目から一番下手前の行まで処理
		// for(i=1; i<height-1; i++)

	stride = (end_x - x - 2) * 4;
	// 結果を返す
	for(i=y+1; i<end_y-1; i++)
	{
		(void)memcpy(&layer->pixels[i*layer->stride+(x+1)*4],
			&temp->pixels[i*layer->stride+(x+1)*4], stride);
	}
}

/*********************************************************************
* AntiAliasVectorLine関数                                            *
* ベクトルレイヤーの線に対して範囲を指定してアンチエイリアスをかける *
* layer	: アンチエイリアスをかけるレイヤー                           *
* rect	: アンチエイリアスをかける範囲                               *
*********************************************************************/
void AntiAliasVectorLine(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect)
{
// アンチエイリアシングを行う1チャンネル分の閾値
#define ANTI_ALIAS_THRESHOLD (51*51)
	// アンチエイリアス開始・終了の座標
	int x, y, end_x, end_y;
	// 処理1行分のバイト数
	int stride;
	int i;	// for文用のカウンタ

	// 座標と範囲を設定
	end_x = rect->width;
	end_y = rect->height;
	if(rect->x < 0)
	{
		end_x += rect->x;
		x = 0;
	}
	else if(rect->x > layer->width)
	{
		return;
	}
	else
	{
		x = rect->x;
	}
	end_x = x + end_x;
	if(end_x > layer->width)
	{
		end_x = layer->width;
	}
	if(end_x - x < 3)
	{
		return;
	}

	if(rect->y < 0)
	{
		end_y += rect->y;
		y = 0;
	}
	else if(rect->y > layer->height)
	{
		return;
	}
	else
	{
		y = rect->y;
	}
	end_y = y + end_y;
	if(end_y > layer->height)
	{
		end_y = layer->height;
	}

#ifdef _OPENMP
#pragma omp parallel for
#endif
	// 2行目から一番下手前の行まで処理
	for(i=y+1; i<end_y-1; i++)
	{
		// ピクセルデータ配列のインデックス
		int index, now_index;
		// 現在のピクセルと周囲のピクセルの色差
		int color_diff[4];
		// 色差の合計
		int sum_diff;
		// 周囲のピクセルの合計値
		int sum_color[4];
		int j;	// for文用のカウンタ

		// 一番左はそのままコピー
		now_index = i * layer->stride + (x+1)*4;

		for(j=x+1; j<end_x-1; j++, now_index+=4)
		{

			// 周囲8ピクセルとの色差と値の合計を計算
			index = (i-1)*layer->stride + (j-1)*4;
			color_diff[0] = ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] = layer->pixels[now_index+0];
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] = ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] = layer->pixels[now_index+1];
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] = ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] = layer->pixels[now_index+2];
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] = ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] = layer->pixels[now_index+3];
			sum_color[3] += layer->pixels[index+3];

			index += 4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index += 4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index = i*layer->stride + (j-1)*4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index += 8;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index = (i+1)*layer->stride + (j-1)*4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index += 4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			index += 4;
			color_diff[0] += ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0])
				* ((int)layer->pixels[now_index+0]-(int)layer->pixels[index+0]);
			sum_color[0] += layer->pixels[index+0];
			color_diff[1] += ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+1])
				* ((int)layer->pixels[now_index+1]-(int)layer->pixels[index+0]);
			sum_color[1] += layer->pixels[index+1];
			color_diff[2] += ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+2])
				* ((int)layer->pixels[now_index+2]-(int)layer->pixels[index+0]);
			sum_color[2] += layer->pixels[index+2];
			color_diff[3] += ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3])
				* ((int)layer->pixels[now_index+3]-(int)layer->pixels[index+3]);
			sum_color[3] += layer->pixels[index+3];

			sum_diff = color_diff[0] + color_diff[1] + color_diff[2] + color_diff[3];

			// 色差の合計が閾値以上ならばアンチエイリアシング実行
			if(sum_diff > ANTI_ALIAS_THRESHOLD)
			{	// 設定する新たなデータ
				uint8 new_value;

				new_value = (uint8)(sum_color[0] / 9);
				temp->pixels[now_index+0] = new_value;

				new_value = (uint8)(sum_color[1] / 9);
				temp->pixels[now_index+1] = new_value;
				
				new_value = (uint8)(sum_color[2] / 9);
				temp->pixels[now_index+2] = new_value;

				new_value = (uint8)(sum_color[3] / 9);
				temp->pixels[now_index+3] = new_value;
			}
			else
			{
				temp->pixels[now_index] = layer->pixels[now_index];
				temp->pixels[now_index+1] = layer->pixels[now_index+1];
				temp->pixels[now_index+2] = layer->pixels[now_index+2];
				temp->pixels[now_index+3] = layer->pixels[now_index+3];
			}
		}
	}	// 2行目から一番下手前の行まで処理
		// for(i=1; i<height-1; i++)

	stride = (end_x - x - 2) * 4;
	// 結果を返す
	for(i=y+1; i<end_y-1; i++)
	{
		(void)memcpy(&layer->pixels[i*layer->stride+(x+1)*4],
			&temp->pixels[i*layer->stride+(x+1)*4], stride);
	}
}

#ifdef __cplusplus
}
#endif
