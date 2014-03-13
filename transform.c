// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <math.h>
#include "transform.h"
#include "utils.h"
#include "memory_stream.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************
* CreateTransformData関数            *
* 変形処理用のデータを作成する       *
* 引数                               *
* window	: 変形処理を行う描画領域 *
*************************************/
TRANSFORM_DATA *CreateTransformData(DRAW_WINDOW* window)
{
	TRANSFORM_DATA *ret = (TRANSFORM_DATA*)MEM_ALLOC_FUNC(sizeof(*ret));
	cairo_surface_t *before_surface;
	cairo_pattern_t *before_pattern;
	cairo_matrix_t matrix;
	// コピーするピクセルデータの一行分のバイト数
	int copy_stride;
	unsigned int i;
	int j;

	(void)memset(ret, 0, sizeof(*ret));

	// 選択範囲があれば
	if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
	{	// 選択範囲の左上を変形開始位置に
		ret->x = window->selection_area.min_x;
		ret->y = window->selection_area.min_y;
		// 選択範囲を変形領域へ
		ret->width = window->selection_area.max_x - window->selection_area.min_x + 1;
		ret->height = window->selection_area.max_y - window->selection_area.min_y + 1;

		if(ret->x + ret->width > window->width)
		{
			ret->width = window->width - ret->x;
		}

		if(ret->y + ret->height > window->height)
		{
			ret->height = window->height - ret->y;
		}
	}
	else
	{	// 選択範囲がない場合は描画領域全体を変形領域へ
		ret->x = ret->y = 0;
		ret->width = window->width, ret->height = window->height;
	}

	ret->stride = ret->width * 4;

	// チェックの付いているレイヤーを探す
	ret->layers = GetLayerChain(window, &ret->num_layers);

	// 変形領域のピクセルデータを記憶
	ret->source_pixels = (uint8**)MEM_ALLOC_FUNC(sizeof(*ret->source_pixels)*ret->num_layers);
	ret->before_pixels = (uint8**)MEM_ALLOC_FUNC(sizeof(*ret->before_pixels)*ret->num_layers);
	cairo_matrix_init_translate(&matrix, - ret->x, - ret->y);
	for(i=0; i<ret->num_layers; i++)
	{
		copy_stride = ret->width * ret->layers[i]->channel;
		ret->source_pixels[i] = (uint8*)MEM_ALLOC_FUNC(copy_stride*ret->height);
		ret->before_pixels[i] = (uint8*)MEM_ALLOC_FUNC(ret->layers[i]->stride*ret->layers[i]->height);
		
		(void)memcpy(ret->before_pixels[i], ret->layers[i]->pixels,
			ret->layers[i]->stride*ret->layers[i]->height);
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{
			for(j=0; j<ret->height; j++)
			{
				(void)memcpy(
					&ret->source_pixels[i][j*copy_stride],
					&ret->layers[i]->pixels[
						(ret->y+j)*ret->layers[i]->stride+ret->x*ret->layers[i]->channel],
					copy_stride
				);
				// 元に位置にあったピクセルデータを消去
				(void)memset(&ret->before_pixels[i][
						(ret->y+j)*ret->layers[i]->stride+ret->x*ret->layers[i]->channel], 0, copy_stride);
			}
		}
		else
		{
			(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
			cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
			cairo_set_source_surface(window->temp_layer->cairo_p, ret->layers[i]->surface_p, 0, 0);
			cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);
			for(j=0; j<ret->height; j++)
			{
				(void)memcpy(
					&ret->source_pixels[i][j*copy_stride],
					&window->temp_layer->pixels[
						(ret->y+j)*window->temp_layer->stride+ret->x*4],
					copy_stride
				);
			}
			// 元の位置にあったピクセルデータを消去
			DeleteSelectAreaPixels(window, ret->layers[i], window->selection);

			(void)memcpy(ret->before_pixels[i], ret->layers[i]->pixels, window->pixel_buf_size);

			before_surface = cairo_image_surface_create_for_data(ret->source_pixels[i],
				CAIRO_FORMAT_ARGB32, ret->width, ret->height, ret->width * 4);
			before_pattern = cairo_pattern_create_for_surface(before_surface);
			cairo_pattern_set_matrix(before_pattern, &matrix);

			cairo_set_source(ret->layers[i]->cairo_p, before_pattern);
			cairo_paint(ret->layers[i]->cairo_p);

			cairo_pattern_destroy(before_pattern);
			cairo_surface_destroy(before_surface);
		}
	}

	// アフィン変換用のデータを初期化
	ret->move_x = ret->x, ret->move_y = ret->y;
	ret->angle = 0;

	// 最初の変換座標は元の位置に
	ret->trans[0][0] = ret->x, ret->trans[0][1] = ret->y;
	ret->trans[1][0] = ret->x, ret->trans[1][1] = ret->y + ret->height * 0.5;
	ret->trans[2][0] = ret->x, ret->trans[2][1] = ret->y + ret->height;
	ret->trans[3][0] = ret->x + ret->width * 0.5, ret->trans[3][1] = ret->trans[2][1];
	ret->trans[4][0] = ret->x + ret->width, ret->trans[4][1] = ret->trans[2][1];
	ret->trans[5][0] = ret->trans[4][0], ret->trans[5][1] = ret->trans[1][1];
	ret->trans[6][0] = ret->trans[4][0], ret->trans[6][1] = ret->y;
	ret->trans[7][0] = ret->trans[3][0], ret->trans[7][1] = ret->y;

	return ret;
}

/***************************************
* DeleteTransformData関数              *
* 変形処理用のデータを開放             *
* 引数                                 *
* transform	: 開放するデータのアドレス *
***************************************/
void DeleteTransformData(TRANSFORM_DATA** transform)
{
	unsigned int i;

	for(i=0; i<(*transform)->num_layers; i++)
	{
		MEM_FREE_FUNC((*transform)->source_pixels[i]);
	}
	MEM_FREE_FUNC((*transform)->source_pixels);
	MEM_FREE_FUNC((*transform)->layers);

	*transform = NULL;
}

/*********************************************
* CalcProjectionParameter関数                *
* 射影変換のパラメーターを計算する           *
* 引数                                       *
* src			: 返還前の座標配列           *
* dst			: 変換後の座標配列           *
* parameter		: パラメーターを格納する配列 *
*********************************************/
void CalcProjectionParameter(
	FLOAT_T (*src)[2],
	FLOAT_T (*dst)[2],
	FLOAT_T parameter[9]
)
{
	int i, j;
	FLOAT_T *matrix[9];
	FLOAT_T a[9][9] = {
		{src[0][0], src[0][1], 1, 0, 0, 0, -src[0][0]*dst[0][0], -src[0][1]*dst[0][0], 0},
		{0, 0, 0, src[0][0], src[0][1], 1, -src[0][0]*dst[0][1], -src[0][1]*dst[0][1], 0},
		{src[1][0], src[1][1], 1, 0, 0, 0, -src[1][0]*dst[1][0], -src[1][1]*dst[1][0], 0},
		{0, 0, 0, src[1][0], src[1][1], 1, -src[1][0]*dst[1][1], -src[1][1]*dst[1][1], 0},
		{src[2][0], src[2][1], 1, 0, 0, 0, -src[2][0]*dst[2][0], -src[2][1]*dst[2][0], 0},
		{0, 0, 0, src[2][0], src[2][1], 1, -src[2][0]*dst[2][1], -src[2][1]*dst[2][1], 0},
		{src[3][0], src[3][1], 1, 0, 0, 0, -src[3][0]*dst[3][0], -src[3][1]*dst[3][0], 0},
		{0, 0, 0, src[3][0], src[3][1], 1, -src[3][0]*dst[3][1], -src[3][1]*dst[3][1], 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 1}
	};
	FLOAT_T v[9] = {dst[0][0], dst[0][1], dst[1][0], dst[1][1],
		dst[2][0], dst[2][1], dst[3][0], dst[3][1], 0};

	// 逆行列
	for(i=0; i<9; i++)
	{
		matrix[i] = a[i];
	}
	InvertMatrix(matrix, 9);

	(void)memset(parameter, 0, sizeof(*parameter)*8);
	for(i=0; i<9; i++)
	{
		for(j=0; j<9; j++)
		{
			parameter[i] += a[i][j] * v[j];
		}
	}
}

/*******************************************
* Projection関数                           *
* 射影変換を行う                           *
* 引数                                     *
* src		: 変換前の座標が入っている配列 *
* dst		: 変換後の座標を格納する配列   *
* parameter	: 射影変換用のパラメーター     *
*******************************************/
void Projection(FLOAT_T src[2], FLOAT_T dst[2], FLOAT_T parameter[9])
{
	dst[0] = (src[0]*parameter[0] + src[1]*parameter[1] + parameter[2])
			/ (src[0]*parameter[6] + src[1]*parameter[7] + 1);
	dst[1] = (src[0]*parameter[3] + src[1]*parameter[4] + parameter[5])
			/ (src[0]*parameter[6] + src[1]*parameter[7] + 1);
}

// 自由変形用パラメータ
typedef struct _TRANSPUT
{
	FLOAT_T length;			// 描画ラインの長さ
	FLOAT_T dst[2];			// 描画座標
	FLOAT_T add[2];			// 座標の増分
	FLOAT_T src_point[2];	// 現在の元画像の座標
	FLOAT_T src_add[2];		// 元画像の座標の増分
	int out_add[2];			// 描画位置補正
} TRANSPUT;

/*
* TransformDrawLine関数
* 引数
* point
* transform
* tranput
* length
* current_layer	: 現在操作性しているレイヤー
*/
static void TransformDrawLine(
	int* point,
	TRANSFORM_DATA* transform,
	TRANSPUT* transput,
	int length,
	int current_layer
)
{
	DRAW_WINDOW *window = transform->layers[0]->window;
	int src[2];
	int src_index, dst_index;
	int i;

	src[0] = (int)transput->src_point[0];
	src[1] = (int)transput->src_point[1];

	for(i=0; i<=length; i++)
	{
		point[0] = (int)transput->dst[0];
		point[1] = (int)transput->dst[1];

		if(point[0] >= 0 && point[0] < window->width
			&& point[1] >= 0 && point[1] < window->height)
		{
			if(src[0] >= 0 && src[1] >= 0
				&& src[0] < transform->width && src[1] < transform->height)
			{
				src_index = src[1] * transform->stride + src[0]*4;
				dst_index = point[1]*window->temp_layer->stride + point[0]*4;
				if(window->temp_layer->pixels[dst_index+3]
					< transform->source_pixels[current_layer][src_index+3])
				{
					window->temp_layer->pixels[dst_index+0]
						= transform->source_pixels[current_layer][src_index+0];
					window->temp_layer->pixels[dst_index+1]
						= transform->source_pixels[current_layer][src_index+1];
					window->temp_layer->pixels[dst_index+2]
						= transform->source_pixels[current_layer][src_index+2];
					window->temp_layer->pixels[dst_index+3]
						= transform->source_pixels[current_layer][src_index+3];
				}
			}
		}

		transput->dst[0] += transput->add[0];
		transput->dst[1] += transput->add[1];

		transput->src_point[0] += transput->src_add[0];
		src[0] = (int)transput->src_point[0];
	}
}

/*
* TransformOutput関数
* 自由変形の描画
* 引数
*/
static void TransformOutput(TRANSFORM_DATA* transform, int current_layer)
{
	DRAW_WINDOW *window = transform->layers[0]->window;
	TRANSPUT output_point;
	int dst_points[4][2];
	int point[2];
	FLOAT_T line_points[2][2];
	FLOAT_T line_add[2][2];
	FLOAT_T left_length, right_length;
	FLOAT_T line_length;
	FLOAT_T ave_x = 0, ave_y = 0;
	int src_x_start;
	int loop_length;
	int i, j;

	for(i=0; i<8; i++)
	{
		ave_x += transform->trans[i][0];
		ave_y += transform->trans[i][1];
	}
	ave_x *= 0.125,	ave_y *= 0.125;

	for(i=0; i<4; i++)
	{
		(void)memset(&output_point, 0, sizeof(output_point));
		(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);

		switch(i)
		{
		case 0:
			src_x_start = 0;
			dst_points[0][0] = (int)(transform->trans[0][0]);
			dst_points[0][1] = (int)(transform->trans[0][1]);
			dst_points[1][0] = (int)(transform->trans[1][0]);
			dst_points[1][1] = (int)(transform->trans[1][1]);
			dst_points[2][0] = (int)ave_x;
			dst_points[2][1] = (int)ave_y;
			dst_points[3][0] = (int)(transform->trans[7][0]);
			dst_points[3][1] = (int)(transform->trans[7][1]);

			break;
		case 1:
			output_point.src_point[1] = transform->height / 2;
			src_x_start = 0;
			dst_points[0][0] = (int)(transform->trans[1][0]);
			dst_points[0][1] = (int)(transform->trans[1][1]);
			dst_points[1][0] = (int)(transform->trans[2][0]);
			dst_points[1][1] = (int)(transform->trans[2][1]);
			dst_points[2][0] = (int)(transform->trans[3][0]);
			dst_points[2][1] = (int)(transform->trans[3][1]);
			dst_points[3][0] = (int)ave_x;
			dst_points[3][1] = (int)ave_y;

			break;
		case 2:
			src_x_start = (int)(output_point.src_point[0] = transform->width / 2);
			output_point.src_point[1] = transform->height / 2;
			dst_points[0][0] = (int)ave_x;
			dst_points[0][1] = (int)ave_y;
			dst_points[1][0] = (int)(transform->trans[3][0]);
			dst_points[1][1] = (int)(transform->trans[3][1]);
			dst_points[2][0] = (int)(transform->trans[4][0]);
			dst_points[2][1] = (int)(transform->trans[4][1]);
			dst_points[3][0] = (int)(transform->trans[5][0]);
			dst_points[3][1] = (int)(transform->trans[5][1]);

			break;
		case 3:
			src_x_start = (int)(output_point.src_point[0] = transform->width / 2);
			dst_points[0][0] = (int)(transform->trans[7][0]);
			dst_points[0][1] = (int)(transform->trans[7][1]);
			dst_points[1][0] = (int)ave_x;
			dst_points[1][1] = (int)ave_y;
			dst_points[2][0] = (int)(transform->trans[5][0]);
			dst_points[2][1] = (int)(transform->trans[5][1]);
			dst_points[3][0] = (int)(transform->trans[6][0]);
			dst_points[3][1] = (int)(transform->trans[6][1]);

			break;
		}

		output_point.out_add[0] = (int)transform->move_x;
		output_point.out_add[1] = (int)transform->move_y;

		left_length = sqrt(
			(dst_points[0][0]-dst_points[1][0])*(dst_points[0][0]-dst_points[1][0])+(dst_points[0][1]-dst_points[1][1])*(dst_points[0][1]-dst_points[1][1]));
		right_length = sqrt(
			(dst_points[2][0]-dst_points[3][0])*(dst_points[2][0]-dst_points[3][0])+(dst_points[2][1]-dst_points[3][1])*(dst_points[2][1]-dst_points[3][1]));

		line_add[0][0] = (dst_points[1][0]-dst_points[0][0]) / left_length;
		line_add[1][0] = (dst_points[2][0]-dst_points[3][0]) / right_length;

		line_points[0][0] = dst_points[0][0];
		line_points[0][1] = dst_points[0][1];
		line_points[1][0] = dst_points[3][0];
		line_points[1][1] = dst_points[3][1];

		if(left_length > right_length)
		{
			loop_length = (int)left_length;
			output_point.src_add[1] = (transform->height / 2) / left_length;
			line_add[0][1] = (dst_points[1][1]-dst_points[0][1]) / left_length;
			line_add[1][1] = (dst_points[2][1]-dst_points[3][1]) / left_length;
		}
		else
		{
			loop_length = (int)right_length;
			output_point.src_add[1] = (transform->height / 2) / right_length;
			line_add[0][1] = (dst_points[1][1]-dst_points[0][1]) / right_length;
			line_add[1][1] = (dst_points[2][1]-dst_points[3][1]) / right_length;
		}

		for(j=0; j<=loop_length; j++)
		{
			line_length = sqrt(
				(line_points[0][0]-line_points[1][0])*(line_points[0][0]-line_points[1][0]) + (line_points[0][1]-line_points[1][1])*(line_points[0][1]-line_points[1][1]));

			output_point.dst[0] = line_points[0][0];
			output_point.dst[1] = line_points[0][1];

			output_point.add[0] = ((line_points[1][0] - line_points[0][0]) / line_length) * 0.5;
			output_point.add[1] = ((line_points[1][1] - line_points[0][1]) / line_length) * 0.5;

			output_point.src_add[0] = ((transform->width / 2) / line_length) * 0.5;

			TransformDrawLine(point, transform, &output_point, (int)line_length*2, current_layer);

			line_points[0][0] += line_add[0][0];
			line_points[0][1] += line_add[0][1];
			line_points[1][0] += line_add[1][0];
			line_points[1][1] += line_add[1][1];

			output_point.src_point[0] = src_x_start;
			output_point.src_point[1] += output_point.src_add[1];
		}

		cairo_save(window->mask_temp->cairo_p);

		cairo_move_to(window->mask_temp->cairo_p, dst_points[0][0], dst_points[0][1]);
		cairo_line_to(window->mask_temp->cairo_p, dst_points[1][0], dst_points[1][1]);
		cairo_line_to(window->mask_temp->cairo_p, dst_points[2][0], dst_points[2][1]);
		cairo_line_to(window->mask_temp->cairo_p, dst_points[3][0], dst_points[3][1]);
		cairo_close_path(window->mask_temp->cairo_p);
		cairo_clip(window->mask_temp->cairo_p);

		cairo_set_source_surface(window->mask_temp->cairo_p, window->temp_layer->surface_p, 0, 0);
		cairo_paint(window->mask_temp->cairo_p);

		cairo_restore(window->mask_temp->cairo_p);
	}
}

/*********************************
* Transform関数                  *
* 自由変形を実行                 *
* 引数                           *
* transform	: 変形処理用のデータ *
*********************************/
void Transform(TRANSFORM_DATA* transform)
{
	DRAW_WINDOW *window = transform->layers[0]->window;
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	unsigned int i;

	(void)memset(window->brush_buffer, 0, window->pixel_buf_size);
	surface_p = cairo_image_surface_create_for_data(window->brush_buffer,
		CAIRO_FORMAT_A8, window->width, window->height, window->width);
	cairo_p = cairo_create(surface_p);
	cairo_move_to(cairo_p, transform->trans[0][0], transform->trans[0][1]);
	for(i=1; i<8; i++)
	{
		cairo_line_to(cairo_p, transform->trans[i][0], transform->trans[i][1]);
	}
	cairo_close_path(cairo_p);
	cairo_fill(cairo_p);

	for(i=0; i<transform->num_layers; i++)
	{
		(void)memset(window->mask_temp->pixels, 0, window->pixel_buf_size);
		(void)memcpy(transform->layers[i]->pixels, transform->before_pixels[i], window->pixel_buf_size);
		TransformOutput(transform, i);

		cairo_set_operator(transform->layers[i]->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(transform->layers[i]->cairo_p, window->mask_temp->surface_p, 0, 0);
		cairo_mask_surface(transform->layers[i]->cairo_p, surface_p, 0, 0);
	}

	cairo_surface_destroy(surface_p);
	cairo_destroy(cairo_p);
	(void)memset(window->brush_buffer, 0, window->pixel_buf_size);
}

/***********************************************
* ProjectionTransform関数                      *
* 射影変形処理を実行                           *
* 引数                                         *
* transform			: 変形処理用のデータ       *
* projection_param	: 射影変換用のパラメーター *
***********************************************/
void ProjectionTransform(TRANSFORM_DATA* transform, FLOAT_T projection_param[4][9])
{
	DRAW_WINDOW *window = transform->layers[0]->window;
	FLOAT_T src[2], dst[2];
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	int x, y;
	int stride = transform->width * 4;
	int start_x, end_x, start_y, end_y;
	unsigned int i;
	int j, k, l;

	(void)memset(window->brush_buffer, 0, window->pixel_buf_size);
	surface_p = cairo_image_surface_create_for_data(window->brush_buffer,
		CAIRO_FORMAT_A8, window->width, window->height, window->width);
	cairo_p = cairo_create(surface_p);
	cairo_move_to(cairo_p, transform->trans[0][0], transform->trans[0][1]);
	for(i=1; i<8; i++)
	{
		cairo_line_to(cairo_p, transform->trans[i][0], transform->trans[i][1]);
	}
	cairo_close_path(cairo_p);
	cairo_fill(cairo_p);

	for(i=0; i<transform->num_layers; i++)
	{
		(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
		(void)memcpy(transform->layers[i]->pixels, transform->before_pixels[i],
			transform->layers[i]->stride * transform->layers[i]->height);
		if((transform->layers[i]->window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			DeleteSelectAreaPixels(transform->layers[i]->window, transform->layers[i],
				transform->layers[i]->window->selection);
		}

		for(j=0; j<4; j++)
		{
			start_x = transform->min_x[j];
			end_x = transform->max_x[j];
			start_y = transform->min_y[j];
			end_y = transform->max_y[j];

			if(start_x < 0)
			{
				start_x = 0;
			}
			if(end_x > transform->layers[i]->width)
			{
				end_x = transform->layers[i]->width;
			}
			if(start_y < 0)
			{
				start_y = 0;
			}
			if(end_y > transform->layers[i]->height)
			{
				end_y = transform->layers[i]->height;
			}

			for(k=transform->min_y[j]; k<transform->max_y[j]; k++)
			{
				for(l=transform->min_x[j]; l<transform->max_x[j]; l++)
				{
					src[0] = l, src[1] = k;
					Projection(src, dst, projection_param[j]);

					x = (int)(dst[0] + 0.5) - transform->x;
					y = (int)(dst[1] + 0.5) - transform->y;

					if(x >= 0 && x < transform->width
						&& y >= 0 && y < transform->height)
					{
						window->temp_layer->pixels[window->temp_layer->stride*k + l*4]
							= transform->source_pixels[i][y*stride+x*4];
						window->temp_layer->pixels[window->temp_layer->stride*k + l*4 + 1]
							= transform->source_pixels[i][y*stride+x*4 + 1];
						window->temp_layer->pixels[window->temp_layer->stride*k + l*4 + 2]
							= transform->source_pixels[i][y*stride+x*4 + 2];
						window->temp_layer->pixels[window->temp_layer->stride*k + l*4 + 3]
							= transform->source_pixels[i][y*stride+x*4 + 3];
					}
				}
			}
		}

		cairo_set_operator(transform->layers[i]->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(transform->layers[i]->cairo_p, window->temp_layer->surface_p, 0, 0);
		cairo_mask_surface(transform->layers[i]->cairo_p, surface_p, 0, 0);
	}

	cairo_surface_destroy(surface_p);
	cairo_destroy(cairo_p);
	(void)memset(window->brush_buffer, 0, window->pixel_buf_size);
}

void TransformButtonPress(TRANSFORM_DATA* transform, gdouble x, gdouble y)
{
	FLOAT_T distance, min_distance = 12;
	FLOAT_T ave_x = 0, ave_y = 0;
	int i;

	transform->trans_point = TRANSFORM_POINT_NONE;
	transform->trans_mode = TRANSFORM_ROTATE;
	for(i=0; i<8; i++)
	{
		ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
		distance = sqrt((x-transform->trans[i][0])*(x-transform->trans[i][0])
			+(y-transform->trans[i][1])*(y-transform->trans[i][1]));
		if(distance < min_distance)
		{
			min_distance = distance;
			transform->trans_point = i;

			transform->trans_mode = TRANSFORM_SCALE;
		}
	}

	ave_x /= 8, ave_y /= 8;
	if(sqrt((x-ave_x)*(x-ave_x)+(y-ave_y)*(y-ave_y)) < 12)
	{
		transform->trans_mode = TRANSFORM_MOVE;
	}

	transform->before_angle = atan2(ave_y-y, x-ave_x);

	transform->before_x = x, transform->before_y = y;
}

void ProjectionTransformMotionNotify(
	DRAW_WINDOW* window,
	TRANSFORM_DATA* transform,
	gdouble x,
	gdouble y,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		FLOAT_T ave_x = 0, ave_y = 0;

		if(transform->selected_mode != TRANSFORM_FREE
			&& transform->trans_mode != TRANSFORM_MOVE)
		{
			transform->trans_mode = transform->selected_mode;
		}

		switch(transform->trans_mode)
		{
		case TRANSFORM_SCALE:
			{
				gdouble diff_x = x - transform->before_x, diff_y = y - transform->before_y;
				FLOAT_T sin_value = sin(transform->angle), cos_value = cos(transform->angle);
				FLOAT_T before_x, before_y;
				int i;

				before_x = diff_x, before_y = diff_y;
				diff_x = before_x * cos_value - diff_y * sin_value;
				diff_y = before_x * sin_value + diff_y * cos_value;

				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}
				ave_x /= 8, ave_y /= 8;

				for(i=0; i<8; i++)
				{
					before_x = transform->trans[i][0], before_y = transform->trans[i][1];
					transform->trans[i][0] = cos_value * before_x - sin_value * before_y + ave_x
						- ave_x * cos_value + ave_y * sin_value;
					transform->trans[i][1] = sin_value * before_x + cos_value * before_y
						+ ave_y - ave_x * sin_value - ave_y * cos_value;
				}

				switch(transform->trans_point)
				{
				case TRANSFORM_LEFT_UP:
					{
						transform->move_x += x - transform->before_x;
						transform->move_y += y - transform->before_y;

						transform->trans[0][0] += diff_x;
						transform->trans[1][0] += diff_x;
						transform->trans[2][0] += diff_x;
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[7][0] += diff_x * 0.5;

						transform->trans[0][1] += diff_y;
						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[5][1] += diff_y * 0.5;
						transform->trans[6][1] += diff_y;
						transform->trans[7][1] += diff_y;
					}
					break;
				case TRANSFORM_LEFT:
					{
						transform->move_x += x - transform->before_x;

						transform->trans[0][0] += diff_x;
						transform->trans[1][0] += diff_x;
						transform->trans[2][0] += diff_x;
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[7][0] += diff_x * 0.5;
					}
					break;
				case TRANSFORM_LEFT_DOWN:
					{
						transform->move_x += x - transform->before_x;

						transform->trans[0][0] += diff_x;
						transform->trans[1][0] += diff_x;
						transform->trans[2][0] += diff_x;
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[7][0] += diff_x * 0.5;

						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[2][1] += diff_y;
						transform->trans[3][1] += diff_y;
						transform->trans[4][1] += diff_y;
						transform->trans[5][1] += diff_y * 0.5;
					}
					break;
				case TRANSFORM_DOWN:
					{
						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[2][1] += diff_y;
						transform->trans[3][1] += diff_y;
						transform->trans[4][1] += diff_y;
						transform->trans[5][1] += diff_y * 0.5;
					}
					break;
				case TRANSFORM_RIGHT_DOWN:
					{
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[4][0] += diff_x;
						transform->trans[5][0] += diff_x;
						transform->trans[6][0] += diff_x;
						transform->trans[7][0] += diff_x * 0.5;

						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[2][1] += diff_y;
						transform->trans[3][1] += diff_y;
						transform->trans[4][1] += diff_y;
						transform->trans[5][1] += diff_y * 0.5;
					}
					break;
				case TRANSFORM_RIGHT:
					{
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[4][0] += diff_x;
						transform->trans[5][0] += diff_x;
						transform->trans[6][0] += diff_x;
						transform->trans[7][0] += diff_x * 0.5;
					}
					break;
				case TRANSFORM_RIGHT_UP:
					{
						transform->move_y += y - transform->before_y;
						
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[4][0] += diff_x;
						transform->trans[5][0] += diff_x;
						transform->trans[6][0] += diff_x;
						transform->trans[7][0] += diff_x * 0.5;

						transform->trans[0][1] += diff_y;
						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[5][1] += diff_y * 0.5;
						transform->trans[6][1] += diff_y;
						transform->trans[7][1] += diff_y;
					}
					break;
				case TRANSFORM_UP:
					{
						transform->move_y += y - transform->before_y;

						transform->trans[0][1] += diff_y;
						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[5][1] += diff_y * 0.5;
						transform->trans[6][1] += diff_y;
						transform->trans[7][1] += diff_y;
					}
					break;
				}

				ave_x = 0, ave_y = 0;
				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}
				ave_x /= 8, ave_y /= 8;

				for(i=0; i<8; i++)
				{
					before_x = transform->trans[i][0], before_y = transform->trans[i][1];
					transform->trans[i][0] = cos_value * before_x + sin_value * before_y + ave_x
						- ave_x * cos_value - ave_y * sin_value;
					transform->trans[i][1] = - sin_value * before_x + cos_value * before_y
						+ ave_y + ave_x * (sin_value) - ave_y * cos_value;
				}
			}
			break;
		case TRANSFORM_FREE_SHAPE:
			{
				gdouble diff_x = x - transform->before_x, diff_y = y - transform->before_y;
				FLOAT_T sin_value = sin(transform->angle), cos_value = cos(transform->angle);
				FLOAT_T before_x, before_y;
				int i;

				before_x = diff_x, before_y = diff_y;
				diff_x = before_x * cos_value - diff_y * sin_value;
				diff_y = before_x * sin_value + diff_y * cos_value;

				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}
				ave_x /= 8, ave_y /= 8;

				for(i=0; i<8; i++)
				{
					before_x = transform->trans[i][0], before_y = transform->trans[i][1];
					transform->trans[i][0] = cos_value * before_x - sin_value * before_y + ave_x
						- ave_x * cos_value + ave_y * sin_value;
					transform->trans[i][1] = sin_value * before_x + cos_value * before_y
						+ ave_y - ave_x * sin_value - ave_y * cos_value;
				}

				transform->move_x += x - transform->before_x;
				transform->move_y += y - transform->before_y;
				transform->trans[transform->trans_point][0] += diff_x;
				transform->trans[transform->trans_point][1] += diff_y;
			}
			break;
		case TRANSFORM_ROTATE:
			{
				FLOAT_T angle, change_angle;
				FLOAT_T sin_value, cos_value;
				int i;

				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}

				ave_x /= 8, ave_y /= 8;
				angle = atan2(ave_y-y, x-ave_x);
				change_angle = angle - transform->angle;
				transform->angle += change_angle;
				transform->before_angle = angle;

				sin_value = sin(- change_angle), cos_value = cos(- change_angle);
				for(i=0; i<8; i++)
				{
					FLOAT_T before_x = transform->trans[i][0], before_y = transform->trans[i][1];
					transform->trans[i][0] = cos_value * before_x - sin_value * before_y + ave_x
						- ave_x * cos_value + ave_y * sin_value;
					transform->trans[i][1] = sin_value * before_x + cos_value * before_y
						+ ave_y - ave_x * sin_value - ave_y * cos_value;
				}
			}
			break;
		case TRANSFORM_MOVE:
			{
				FLOAT_T move_x = x - transform->before_x;
				FLOAT_T move_y = y - transform->before_y;
				int i;

				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}
				ave_x /= 8, ave_y /= 8;

				for(i=0; i<8; i++)
				{
					transform->trans[i][0] += move_x;
					transform->trans[i][1] += move_y;
				}
				transform->move_x += move_x;
				transform->move_y += move_y;
			}
			break;
		}

		// 変形を実行
		{
			// 元の座標から変換後のピクセルの対応を求める
			FLOAT_T start[4][2], parameter[4][9];
			FLOAT_T dst[4][2];
			int i;

			start[0][0] = transform->x, start[0][1] = transform->y;
			start[1][0] = transform->x, start[1][1] = transform->y + transform->height/2;
			start[2][0] = transform->x + transform->width/2, start[2][1] = transform->y + transform->height/2;
			start[3][0] = transform->x + transform->width/2, start[3][1] = transform->y;
			dst[0][0] = transform->trans[0][0], dst[0][1] = transform->trans[0][1];
			dst[1][0] = transform->trans[1][0], dst[1][1] = transform->trans[1][1];
			//dst[2][0] = start[2][0], dst[2][1] = start[2][1];
			dst[2][0] = ave_x, dst[2][1] = ave_y;
			dst[3][0] = transform->trans[7][0], dst[3][1] = transform->trans[7][1];
			CalcProjectionParameter(dst, start, parameter[0]);

			transform->min_x[0] = transform->max_x[0] = (int)dst[0][0];
			transform->min_y[0] = transform->max_y[0] = (int)dst[0][1];
			for(i=1; i<4; i++)
			{
				if(transform->min_x[0] > (int)dst[i][0])
				{
					transform->min_x[0] = (int)dst[i][0];
				}
				if(transform->max_x[0] < (int)dst[i][0]+1)
				{
					transform->max_x[0] = (int)dst[i][0]+1;
				}
				if(transform->min_y[0] > (int)dst[i][1])
				{
					transform->min_y[0] = (int)dst[i][1];
				}
				if(transform->max_y[0] < (int)dst[i][1]+1)
				{
					transform->max_y[0] = (int)dst[i][1]+1;
				}
			}

			start[0][0] = transform->x, start[0][1] = transform->y + transform->height/2;
			start[1][0] = transform->x, start[1][1] = transform->y + transform->height;
			start[2][0] = transform->x + transform->width/2, start[2][1] = transform->y + transform->height;
			start[3][0] = transform->x + transform->width/2, start[3][1] = transform->y + transform->height/2;
			dst[0][0] = transform->trans[1][0], dst[0][1] = transform->trans[1][1];
			dst[1][0] = transform->trans[2][0], dst[1][1] = transform->trans[2][1];
			dst[2][0] = transform->trans[3][0], dst[2][1] = transform->trans[3][1];
			//dst[3][0] = start[3][0], dst[3][1] = start[3][1];
			dst[3][0] = ave_x, dst[3][1] = ave_y;
			CalcProjectionParameter(dst, start, parameter[1]);

			transform->min_x[1] = transform->max_x[1] = (int)dst[0][0];
			transform->min_y[1] = transform->max_y[1] = (int)dst[0][1];
			for(i=1; i<4; i++)
			{
				if(transform->min_x[1] > (int)dst[i][0])
				{
					transform->min_x[1] = (int)dst[i][0];
				}
				if(transform->max_x[1] < (int)dst[i][0]+1)
				{
					transform->max_x[1] = (int)dst[i][0]+1;
				}
				if(transform->min_y[1] > (int)dst[i][1])
				{
					transform->min_y[1] = (int)dst[i][1];
				}
				if(transform->max_y[1] < (int)dst[i][1]+1)
				{
					transform->max_y[1] = (int)dst[i][1]+1;
				}
			}

			start[0][0] = transform->x + transform->width/2, start[0][1] = transform->y + transform->height/2;
			start[1][0] = transform->x + transform->width/2, start[1][1] = transform->y + transform->height;
			start[2][0] = transform->x + transform->width, start[2][1] = transform->y + transform->height;
			start[3][0] = transform->x + transform->width, start[3][1] = transform->y + transform->height/2;
			//dst[0][0] = start[0][0], dst[0][1] = start[0][1];
			dst[0][0] = ave_x, dst[0][1] = ave_y;
			dst[1][0] = transform->trans[3][0], dst[1][1] = transform->trans[3][1];
			dst[2][0] = transform->trans[4][0], dst[2][1] = transform->trans[4][1];
			dst[3][0] = transform->trans[5][0], dst[3][1] = transform->trans[5][1];
			CalcProjectionParameter(dst, start, parameter[2]);

			transform->min_x[2] = transform->max_x[2] = (int)dst[0][0];
			transform->min_y[2] = transform->max_y[2] = (int)dst[0][1];
			for(i=1; i<4; i++)
			{
				if(transform->min_x[2] > (int)dst[i][0])
				{
					transform->min_x[2] = (int)dst[i][0];
				}
				if(transform->max_x[2] < (int)dst[i][0]+1)
				{
					transform->max_x[2] = (int)dst[i][0]+1;
				}
				if(transform->min_y[2] > (int)dst[i][1])
				{
					transform->min_y[2] = (int)dst[i][1];
				}
				if(transform->max_y[2] < (int)dst[i][1])
				{
					transform->max_y[2] = (int)dst[i][1]+1;
				}
			}

			start[0][0] = transform->x + transform->width/2, start[0][1] = transform->y;
			start[1][0] = transform->x + transform->width/2, start[1][1] = transform->y + transform->height/2;
			start[2][0] = transform->x + transform->width, start[2][1] = transform->y + transform->height/2;
			start[3][0] = transform->x + transform->width, start[3][1] = transform->y;
			dst[0][0] = transform->trans[7][0], dst[0][1] = transform->trans[7][1];
			//dst[1][0] = start[1][0], dst[1][1] = start[1][1];
			dst[1][0] = ave_x, dst[1][1] = ave_y;
			dst[2][0] = transform->trans[5][0], dst[2][1] = transform->trans[5][1];
			dst[3][0] = transform->trans[6][0], dst[3][1] = transform->trans[6][1];
			CalcProjectionParameter(dst, start, parameter[3]);

			transform->min_x[3] = transform->max_x[3] = (int)dst[0][0];
			transform->min_y[3] = transform->max_y[3] = (int)dst[0][1];
			for(i=1; i<4; i++)
			{
				if(transform->min_x[3] > (int)dst[i][0])
				{
					transform->min_x[3] = (int)dst[i][0];
				}
				if(transform->max_x[3] < (int)dst[i][0]+1)
				{
					transform->max_x[3] = (int)dst[i][0]+1;
				}
				if(transform->min_y[3] > (int)dst[i][1])
				{
					transform->min_y[3] = (int)dst[i][1];
				}
				if(transform->max_y[3] < (int)dst[i][1]+1)
				{
					transform->max_y[3] = (int)dst[i][1]+1;
				}
			}

			ProjectionTransform(transform, parameter);
		}

		transform->before_x = x, transform->before_y = y;

		if(transform->layers[0] == window->active_layer)
		{
			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		}
		else
		{
			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
		}
	}
}

void TransformMotionNotify(
	DRAW_WINDOW* window,
	TRANSFORM_DATA* transform,
	gdouble x,
	gdouble y,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		FLOAT_T ave_x = 0, ave_y = 0;

		if(transform->selected_mode != TRANSFORM_FREE
			&& transform->trans_mode != TRANSFORM_MOVE)
		{
			transform->trans_mode = transform->selected_mode;
		}

		switch(transform->trans_mode)
		{
		case TRANSFORM_SCALE:
			{
				gdouble diff_x = x - transform->before_x, diff_y = y - transform->before_y;
				FLOAT_T sin_value = sin(transform->angle), cos_value = cos(transform->angle);
				FLOAT_T before_x, before_y;
				int i;

				before_x = diff_x, before_y = diff_y;
				diff_x = before_x * cos_value - diff_y * sin_value;
				diff_y = before_x * sin_value + diff_y * cos_value;

				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}
				ave_x /= 8, ave_y /= 8;

				for(i=0; i<8; i++)
				{
					before_x = transform->trans[i][0], before_y = transform->trans[i][1];
					transform->trans[i][0] = cos_value * before_x - sin_value * before_y + ave_x
						- ave_x * cos_value + ave_y * sin_value;
					transform->trans[i][1] = sin_value * before_x + cos_value * before_y
						+ ave_y - ave_x * sin_value - ave_y * cos_value;
				}

				switch(transform->trans_point)
				{
				case TRANSFORM_LEFT_UP:
					{
						transform->move_x += x - transform->before_x;
						transform->move_y += y - transform->before_y;

						transform->trans[0][0] += diff_x;
						transform->trans[1][0] += diff_x;
						transform->trans[2][0] += diff_x;
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[7][0] += diff_x * 0.5;

						transform->trans[0][1] += diff_y;
						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[5][1] += diff_y * 0.5;
						transform->trans[6][1] += diff_y;
						transform->trans[7][1] += diff_y;
					}
					break;
				case TRANSFORM_LEFT:
					{
						transform->move_x += x - transform->before_x;

						transform->trans[0][0] += diff_x;
						transform->trans[1][0] += diff_x;
						transform->trans[2][0] += diff_x;
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[7][0] += diff_x * 0.5;
					}
					break;
				case TRANSFORM_LEFT_DOWN:
					{
						transform->move_x += x - transform->before_x;

						transform->trans[0][0] += diff_x;
						transform->trans[1][0] += diff_x;
						transform->trans[2][0] += diff_x;
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[7][0] += diff_x * 0.5;

						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[2][1] += diff_y;
						transform->trans[3][1] += diff_y;
						transform->trans[4][1] += diff_y;
						transform->trans[5][1] += diff_y * 0.5;
					}
					break;
				case TRANSFORM_DOWN:
					{
						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[2][1] += diff_y;
						transform->trans[3][1] += diff_y;
						transform->trans[4][1] += diff_y;
						transform->trans[5][1] += diff_y * 0.5;
					}
					break;
				case TRANSFORM_RIGHT_DOWN:
					{
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[4][0] += diff_x;
						transform->trans[5][0] += diff_x;
						transform->trans[6][0] += diff_x;
						transform->trans[7][0] += diff_x * 0.5;

						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[2][1] += diff_y;
						transform->trans[3][1] += diff_y;
						transform->trans[4][1] += diff_y;
						transform->trans[5][1] += diff_y * 0.5;
					}
					break;
				case TRANSFORM_RIGHT:
					{
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[4][0] += diff_x;
						transform->trans[5][0] += diff_x;
						transform->trans[6][0] += diff_x;
						transform->trans[7][0] += diff_x * 0.5;
					}
					break;
				case TRANSFORM_RIGHT_UP:
					{
						transform->move_y += y - transform->before_y;
						
						transform->trans[3][0] += diff_x * 0.5;
						transform->trans[4][0] += diff_x;
						transform->trans[5][0] += diff_x;
						transform->trans[6][0] += diff_x;
						transform->trans[7][0] += diff_x * 0.5;

						transform->trans[0][1] += diff_y;
						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[5][1] += diff_y * 0.5;
						transform->trans[6][1] += diff_y;
						transform->trans[7][1] += diff_y;
					}
					break;
				case TRANSFORM_UP:
					{
						transform->move_y += y - transform->before_y;

						transform->trans[0][1] += diff_y;
						transform->trans[1][1] += diff_y * 0.5;
						transform->trans[5][1] += diff_y * 0.5;
						transform->trans[6][1] += diff_y;
						transform->trans[7][1] += diff_y;
					}
					break;
				}

				ave_x = 0, ave_y = 0;
				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}
				ave_x /= 8, ave_y /= 8;

				for(i=0; i<8; i++)
				{
					before_x = transform->trans[i][0], before_y = transform->trans[i][1];
					transform->trans[i][0] = cos_value * before_x + sin_value * before_y + ave_x
						- ave_x * cos_value - ave_y * sin_value;
					transform->trans[i][1] = - sin_value * before_x + cos_value * before_y
						+ ave_y + ave_x * (sin_value) - ave_y * cos_value;
				}
			}
			break;
		case TRANSFORM_FREE_SHAPE:
			{
				gdouble diff_x = x - transform->before_x, diff_y = y - transform->before_y;
				FLOAT_T sin_value = sin(transform->angle), cos_value = cos(transform->angle);
				FLOAT_T before_x, before_y;
				int i;

				before_x = diff_x, before_y = diff_y;
				diff_x = before_x * cos_value - diff_y * sin_value;
				diff_y = before_x * sin_value + diff_y * cos_value;

				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}
				ave_x /= 8, ave_y /= 8;

				for(i=0; i<8; i++)
				{
					before_x = transform->trans[i][0], before_y = transform->trans[i][1];
					transform->trans[i][0] = cos_value * before_x - sin_value * before_y + ave_x
						- ave_x * cos_value + ave_y * sin_value;
					transform->trans[i][1] = sin_value * before_x + cos_value * before_y
						+ ave_y - ave_x * sin_value - ave_y * cos_value;
				}

				transform->move_x += x - transform->before_x;
				transform->move_y += y - transform->before_y;
				transform->trans[transform->trans_point][0] += diff_x;
				transform->trans[transform->trans_point][1] += diff_y;
			}
			break;
		case TRANSFORM_ROTATE:
			{
				FLOAT_T angle, change_angle;
				FLOAT_T sin_value, cos_value;
				int i;

				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}

				ave_x /= 8, ave_y /= 8;
				angle = atan2(ave_y-y, x-ave_x);
				change_angle = angle - transform->angle;
				transform->angle += change_angle;
				transform->before_angle = angle;

				sin_value = sin(- change_angle), cos_value = cos(- change_angle);
				for(i=0; i<8; i++)
				{
					FLOAT_T before_x = transform->trans[i][0], before_y = transform->trans[i][1];
					transform->trans[i][0] = cos_value * before_x - sin_value * before_y + ave_x
						- ave_x * cos_value + ave_y * sin_value;
					transform->trans[i][1] = sin_value * before_x + cos_value * before_y
						+ ave_y - ave_x * sin_value - ave_y * cos_value;
				}
			}
			break;
		case TRANSFORM_MOVE:
			{
				FLOAT_T move_x = x - transform->before_x;
				FLOAT_T move_y = y - transform->before_y;
				int i;

				for(i=0; i<8; i++)
				{
					ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
				}
				ave_x /= 8, ave_y /= 8;

				for(i=0; i<8; i++)
				{
					transform->trans[i][0] += move_x;
					transform->trans[i][1] += move_y;
				}
				transform->move_x += move_x;
				transform->move_y += move_y;
			}
			break;
		}

		// 変形を実行
		{
			// 元の座標から変換後のピクセルの対応を求める
			FLOAT_T start[4][2], parameter[4][9];
			FLOAT_T dst[4][2];
			int i;

			start[0][0] = transform->x, start[0][1] = transform->y;
			start[1][0] = transform->x, start[1][1] = transform->y + transform->height/2;
			start[2][0] = transform->x + transform->width/2, start[2][1] = transform->y + transform->height/2;
			start[3][0] = transform->x + transform->width/2, start[3][1] = transform->y;
			dst[0][0] = transform->trans[0][0], dst[0][1] = transform->trans[0][1];
			dst[1][0] = transform->trans[1][0], dst[1][1] = transform->trans[1][1];
			//dst[2][0] = start[2][0], dst[2][1] = start[2][1];
			dst[2][0] = ave_x, dst[2][1] = ave_y;
			dst[3][0] = transform->trans[7][0], dst[3][1] = transform->trans[7][1];

			transform->min_x[0] = transform->max_x[0] = (int)dst[0][0];
			transform->min_y[0] = transform->max_y[0] = (int)dst[0][1];
			for(i=1; i<4; i++)
			{
				if(transform->min_x[0] > (int)dst[i][0])
				{
					transform->min_x[0] = (int)dst[i][0];
				}
				if(transform->max_x[0] < (int)dst[i][0]+1)
				{
					transform->max_x[0] = (int)dst[i][0]+1;
				}
				if(transform->min_y[0] > (int)dst[i][1])
				{
					transform->min_y[0] = (int)dst[i][1];
				}
				if(transform->max_y[0] < (int)dst[i][1]+1)
				{
					transform->max_y[0] = (int)dst[i][1]+1;
				}
			}

			start[0][0] = transform->x, start[0][1] = transform->y + transform->height/2;
			start[1][0] = transform->x, start[1][1] = transform->y + transform->height;
			start[2][0] = transform->x + transform->width/2, start[2][1] = transform->y + transform->height;
			start[3][0] = transform->x + transform->width/2, start[3][1] = transform->y + transform->height/2;
			dst[0][0] = transform->trans[1][0], dst[0][1] = transform->trans[1][1];
			dst[1][0] = transform->trans[2][0], dst[1][1] = transform->trans[2][1];
			dst[2][0] = transform->trans[3][0], dst[2][1] = transform->trans[3][1];
			//dst[3][0] = start[3][0], dst[3][1] = start[3][1];
			dst[3][0] = ave_x, dst[3][1] = ave_y;

			transform->min_x[1] = transform->max_x[1] = (int)dst[0][0];
			transform->min_y[1] = transform->max_y[1] = (int)dst[0][1];
			for(i=1; i<4; i++)
			{
				if(transform->min_x[1] > (int)dst[i][0])
				{
					transform->min_x[1] = (int)dst[i][0];
				}
				if(transform->max_x[1] < (int)dst[i][0]+1)
				{
					transform->max_x[1] = (int)dst[i][0]+1;
				}
				if(transform->min_y[1] > (int)dst[i][1])
				{
					transform->min_y[1] = (int)dst[i][1];
				}
				if(transform->max_y[1] < (int)dst[i][1]+1)
				{
					transform->max_y[1] = (int)dst[i][1]+1;
				}
			}

			start[0][0] = transform->x + transform->width/2, start[0][1] = transform->y + transform->height/2;
			start[1][0] = transform->x + transform->width/2, start[1][1] = transform->y + transform->height;
			start[2][0] = transform->x + transform->width, start[2][1] = transform->y + transform->height;
			start[3][0] = transform->x + transform->width, start[3][1] = transform->y + transform->height/2;
			//dst[0][0] = start[0][0], dst[0][1] = start[0][1];
			dst[0][0] = ave_x, dst[0][1] = ave_y;
			dst[1][0] = transform->trans[3][0], dst[1][1] = transform->trans[3][1];
			dst[2][0] = transform->trans[4][0], dst[2][1] = transform->trans[4][1];
			dst[3][0] = transform->trans[5][0], dst[3][1] = transform->trans[5][1];

			transform->min_x[2] = transform->max_x[2] = (int)dst[0][0];
			transform->min_y[2] = transform->max_y[2] = (int)dst[0][1];
			for(i=1; i<4; i++)
			{
				if(transform->min_x[2] > (int)dst[i][0])
				{
					transform->min_x[2] = (int)dst[i][0];
				}
				if(transform->max_x[2] < (int)dst[i][0]+1)
				{
					transform->max_x[2] = (int)dst[i][0]+1;
				}
				if(transform->min_y[2] > (int)dst[i][1])
				{
					transform->min_y[2] = (int)dst[i][1];
				}
				if(transform->max_y[2] < (int)dst[i][1])
				{
					transform->max_y[2] = (int)dst[i][1]+1;
				}
			}

			start[0][0] = transform->x + transform->width/2, start[0][1] = transform->y;
			start[1][0] = transform->x + transform->width/2, start[1][1] = transform->y + transform->height/2;
			start[2][0] = transform->x + transform->width, start[2][1] = transform->y + transform->height/2;
			start[3][0] = transform->x + transform->width, start[3][1] = transform->y;
			dst[0][0] = transform->trans[7][0], dst[0][1] = transform->trans[7][1];
			//dst[1][0] = start[1][0], dst[1][1] = start[1][1];
			dst[1][0] = ave_x, dst[1][1] = ave_y;
			dst[2][0] = transform->trans[5][0], dst[2][1] = transform->trans[5][1];
			dst[3][0] = transform->trans[6][0], dst[3][1] = transform->trans[6][1];
			CalcProjectionParameter(dst, start, parameter[3]);

			transform->min_x[3] = transform->max_x[3] = (int)dst[0][0];
			transform->min_y[3] = transform->max_y[3] = (int)dst[0][1];
			for(i=1; i<4; i++)
			{
				if(transform->min_x[3] > (int)dst[i][0])
				{
					transform->min_x[3] = (int)dst[i][0];
				}
				if(transform->max_x[3] < (int)dst[i][0]+1)
				{
					transform->max_x[3] = (int)dst[i][0]+1;
				}
				if(transform->min_y[3] > (int)dst[i][1])
				{
					transform->min_y[3] = (int)dst[i][1];
				}
				if(transform->max_y[3] < (int)dst[i][1]+1)
				{
					transform->max_y[3] = (int)dst[i][1]+1;
				}
			}

			Transform(transform);
		}

		transform->before_x = x, transform->before_y = y;

		if(transform->layers[0] == window->active_layer)
		{
			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		}
		else
		{
			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
		}
	}
}

/***********************************************
* TransformMotionNotifyCallBack関数            *
* 変形処理時のマウスドラッグのコールバック関数 *
* 引数                                         *
* window	: 描画領域の情報                   *
* transform	: 変形処理の情報                   *
* x			: マウスのX座標                    *
* y			: マウスのY座標                    *
* state		: マウス・キーボードの状態         *
***********************************************/
void TransformMotionNotifyCallBack(
	DRAW_WINDOW* window,
	TRANSFORM_DATA* transform,
	gdouble x,
	gdouble y,
	void* state
)
{
	if(transform->trans_type == TRANSFORM_PROJECTION)
	{
		ProjectionTransformMotionNotify(window, transform, x, y, state);
	}
	else
	{
		TransformMotionNotify(window, transform, x, y, state);
	}
}

typedef struct _TRANSFORM_HISTORY_DATA
{
	uint16 num_layer;
	uint16 *layer_name_length;
	char **layer_names;
	uint8 **pixels;
} TRANSFORM_HISTORY_DATA;

void TransformUndoRedo(DRAW_WINDOW *window, void* p)
{
	TRANSFORM_HISTORY_DATA data;
	LAYER *layer;
	uint8 *byte_data = (uint8*)p;
	unsigned int i;

	(void)memcpy(&data.num_layer, byte_data, sizeof(data.num_layer));
	byte_data += sizeof(data.num_layer);
	data.layer_name_length = (uint16*)byte_data;
	byte_data += sizeof(*data.layer_name_length) * data.num_layer;
	data.layer_names = (char**)MEM_ALLOC_FUNC(sizeof(*data.layer_names)*data.num_layer);
	for(i=0; i<data.num_layer; i++)
	{
		data.layer_names[i] = (char*)byte_data;
		byte_data += data.layer_name_length[i];
	}
	data.pixels = (uint8**)MEM_ALLOC_FUNC(sizeof(*data.pixels)*data.num_layer);
	for(i=0; i<data.num_layer; i++)
	{
		data.pixels[i] = byte_data;
		byte_data += window->pixel_buf_size;
	}

	for(i=0; i<data.num_layer; i++)
	{
		layer = window->layer;
		while(strcmp(data.layer_names[i], layer->name) != 0)
		{
			layer = layer->next;
		}

		(void)memcpy(window->temp_layer->pixels, layer->pixels, window->pixel_buf_size);
		(void)memcpy(layer->pixels, data.pixels[i], window->pixel_buf_size);
		(void)memcpy(data.pixels[i], window->temp_layer->pixels, window->pixel_buf_size);
	}

	MEM_FREE_FUNC(data.layer_names);
	MEM_FREE_FUNC(data.pixels);
}

void AddTransformHistory(DRAW_WINDOW *window)
{
	TRANSFORM_DATA *transform = window->transform;
	MEMORY_STREAM_PTR stream = CreateMemoryStream(
		transform->num_layers * window->pixel_buf_size + 8096);
	int stride = transform->width * 4;
	uint16 name_length;
	int y;
	unsigned int i;

	(void)MemWrite(&transform->num_layers, sizeof(transform->num_layers), 1, stream);
	for(i=0; i<transform->num_layers; i++)
	{
		name_length = (uint16)strlen(transform->layers[i]->name) + 1;
		(void)MemWrite(&name_length, sizeof(name_length), 1, stream);
	}
	for(i=0; i<transform->num_layers; i++)
	{
		(void)MemWrite(transform->layers[i]->name,
			1, strlen(transform->layers[i]->name) + 1, stream);
	}
	for(i=0; i<transform->num_layers; i++)
	{
		(void)memcpy(window->temp_layer->pixels, transform->before_pixels[i],
			transform->layers[i]->stride * transform->layers[i]->height);
		for(y=0; y<transform->height; y++)
		{
			(void)memcpy(
				&window->temp_layer->pixels[(transform->y+y)*transform->layers[i]->stride + transform->x*4],
				&transform->source_pixels[i][y*stride], stride
			);
		}
		(void)MemWrite(window->temp_layer->pixels, 1, window->pixel_buf_size, stream);
	}

	AddHistory(&window->history, window->app->labels->menu.transform, stream->buff_ptr,
		stream->data_point, TransformUndoRedo, TransformUndoRedo);

	DeleteMemoryStream(stream);
}

/*****************************************************
* ExecuteTransform関数                               *
* アプリケーションメニューからの変形処理の呼び出し   *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteTransform(APPLICATION* app)
{
	app->draw_window[app->active_window]->transform =
		CreateTransformData(app->draw_window[app->active_window]);
	app->draw_window[app->active_window]->transform->trans_type = TRANSFORM_PATTERN;

	gtk_widget_destroy(app->tool_window.detail_ui);
	app->tool_window.detail_ui = CreateTransformDetailUI(app);
	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
	gtk_widget_show_all(app->tool_window.detail_ui);
	gtk_widget_set_sensitive(app->tool_window.common_tool_table, FALSE);
	gtk_widget_set_sensitive(app->tool_window.brush_table, FALSE);
}

/*******************************************************
* ExecuteProjection関数                                *
* アプリケーションメニューからの射影変換処理の呼び出し *
* 引数                                                 *
* app	: アプリケーションを管理する構造体のアドレス   *
*******************************************************/
void ExecuteProjection(APPLICATION* app)
{
	app->draw_window[app->active_window]->transform =
		CreateTransformData(app->draw_window[app->active_window]);
	app->draw_window[app->active_window]->transform->trans_type = TRANSFORM_PROJECTION;

	gtk_widget_destroy(app->tool_window.detail_ui);
	app->tool_window.detail_ui = CreateTransformDetailUI(app);
	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
	gtk_widget_show_all(app->tool_window.detail_ui);
	gtk_widget_set_sensitive(app->tool_window.common_tool_table, FALSE);
	gtk_widget_set_sensitive(app->tool_window.brush_table, FALSE);
}

void DisplayTransform(DRAW_WINDOW *window)
{
	FLOAT_T ave_x = 0, ave_y = 0;
	FLOAT_T zoom = window->zoom * 0.01;
	cairo_line_cap_t before_line_type;
	int i;

	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);

	for(i=0; i<8; i++)
	{
		cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
		cairo_arc(window->disp_temp->cairo_p,
			window->transform->trans[i][0] * zoom, window->transform->trans[i][1] * zoom,
			3, 0, 2*G_PI);
		ave_x += window->transform->trans[i][0], ave_y += window->transform->trans[i][1];
		cairo_stroke(window->disp_temp->cairo_p);
	}
	ave_x *= 0.125, ave_y *= 0.125;

	cairo_arc(window->disp_temp->cairo_p, ave_x * zoom, ave_y * zoom,
		12 * zoom, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);
	before_line_type = cairo_get_line_cap(window->disp_temp->cairo_p);
	cairo_set_line_cap(window->disp_temp->cairo_p, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_width(window->disp_temp->cairo_p, 2.5);
	cairo_move_to(window->disp_temp->cairo_p, (ave_x - 9) * zoom, ave_y * zoom);
	cairo_line_to(window->disp_temp->cairo_p, (ave_x + 9) * zoom, ave_y * zoom);
	cairo_stroke(window->disp_temp->cairo_p);
	cairo_move_to(window->disp_temp->cairo_p, (ave_x + 9) * zoom, ave_y * zoom);
	cairo_line_to(window->disp_temp->cairo_p, (ave_x + 9 - 3.5) * zoom, (ave_y - 3.5) * zoom);
	cairo_stroke(window->disp_temp->cairo_p);
	cairo_move_to(window->disp_temp->cairo_p, (ave_x + 9) * zoom, ave_y * zoom);
	cairo_line_to(window->disp_temp->cairo_p, (ave_x + 9 - 3.5) * zoom, (ave_y + 3.5) * zoom);
	cairo_stroke(window->disp_temp->cairo_p);
	cairo_move_to(window->disp_temp->cairo_p, (ave_x - 9) * zoom, ave_y * zoom);
	cairo_line_to(window->disp_temp->cairo_p, (ave_x - 9 + 3.5) * zoom, (ave_y - 3.5) * zoom);
	cairo_stroke(window->disp_temp->cairo_p);
	cairo_move_to(window->disp_temp->cairo_p, (ave_x - 9) * zoom, ave_y * zoom);
	cairo_line_to(window->disp_temp->cairo_p, (ave_x - 9 + 3.5) * zoom, (ave_y + 3.5) * zoom);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_move_to(window->disp_temp->cairo_p, ave_x * zoom, (ave_y - 9) * zoom);
	cairo_line_to(window->disp_temp->cairo_p, ave_x * zoom, (ave_y + 9) * zoom);
	cairo_stroke(window->disp_temp->cairo_p);
	cairo_move_to(window->disp_temp->cairo_p, ave_x * zoom, (ave_y - 9) * zoom);
	cairo_line_to(window->disp_temp->cairo_p, (ave_x - 3.5) * zoom, (ave_y - 9 + 3.5) * zoom);
	cairo_stroke(window->disp_temp->cairo_p);
	cairo_move_to(window->disp_temp->cairo_p, ave_x * zoom, (ave_y - 9) * zoom);
	cairo_line_to(window->disp_temp->cairo_p, (ave_x + 3.5) * zoom, (ave_y - 9 + 3.5) * zoom);
	cairo_stroke(window->disp_temp->cairo_p);
	cairo_move_to(window->disp_temp->cairo_p, ave_x * zoom, (ave_y + 9) * zoom);
	cairo_line_to(window->disp_temp->cairo_p, (ave_x - 3.5) * zoom, (ave_y + 9 - 3.5) * zoom);
	cairo_stroke(window->disp_temp->cairo_p);
	cairo_move_to(window->disp_temp->cairo_p, ave_x * zoom, (ave_y + 9) * zoom);
	cairo_line_to(window->disp_temp->cairo_p, (ave_x + 3.5) * zoom, (ave_y + 9 - 3.5) * zoom);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_set_line_cap(window->disp_temp->cairo_p, before_line_type);

	g_layer_blend_funcs[LAYER_BLEND_DIFFERENCE](window->disp_temp, window->disp_layer);
}

static void ChangeTransformMode(GtkWidget* radio_button, APPLICATION* app)
{
	app->draw_window[app->active_window]->transform->selected_mode =
		(uint8)GPOINTER_TO_UINT(g_object_get_data(
			G_OBJECT(radio_button), "trans_mode"));
}

static void ClickedTransformButtonOK(GtkWidget* button, APPLICATION* app)
{
	AddTransformHistory(app->draw_window[app->active_window]);

	gtk_widget_destroy(app->tool_window.detail_ui);
	switch(app->draw_window[app->active_window]->active_layer->layer_type)
	{
	case TYPE_NORMAL_LAYER:
		if((app->tool_window.flags & TOOL_USING_BRUSH) != 0)
		{
			app->tool_window.detail_ui = app->tool_window.active_brush[app->input]->create_detail_ui(
					app, app->tool_window.active_brush[app->input]);
		}
		else
		{
			app->tool_window.detail_ui = app->tool_window.active_common_tool->create_detail_ui(
				app, app->tool_window.active_common_tool->tool_data);
		}
		break;
	case TYPE_VECTOR_LAYER:
		if((app->tool_window.flags & TOOL_USING_BRUSH) != 0)
		{
			app->tool_window.detail_ui = app->tool_window.active_vector_brush[app->input]->create_detail_ui(
				app, app->tool_window.active_vector_brush[app->input]->brush_data);
		}
		else
		{
			app->tool_window.detail_ui = app->tool_window.active_common_tool->create_detail_ui(
				app, app->tool_window.active_common_tool->tool_data);
		}
		break;
	case TYPE_TEXT_LAYER:
		app->tool_window.detail_ui = CreateTextLayerDetailUI(
			app, app->draw_window[app->active_window]->active_layer,
			app->draw_window[app->active_window]->active_layer->layer_data.text_layer_p
		);
		break;
	}

	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
	gtk_widget_show_all(app->tool_window.detail_ui);

	gtk_widget_set_sensitive(app->tool_window.common_tool_table, TRUE);
	gtk_widget_set_sensitive(app->tool_window.brush_table, TRUE);

	DeleteTransformData(&app->draw_window[app->active_window]->transform);
}

static void ClickedTransformButtonCancel(GtkWidget* button, APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];
	cairo_t *restore_cairo;
	cairo_surface_t *restore_surface;
	int stride = window->transform->width * 4;
	unsigned int i;
	int j, k;

	for(i=0; i<window->transform->num_layers; i++)
	{
		cairo_surface_t *before_surface;

		if((window->transform->flags & TRANSFORM_REVERSE_HORIZONTALLY) != 0)
		{
			for(j=0; j<window->transform->height; j++)
			{
				for(k=0; k<window->transform->width; k++)
				{
					window->mask_temp->pixels[k*4+0] =
						window->transform->source_pixels[i][j*stride+(window->transform->width-k-1)*4+0];
					window->mask_temp->pixels[k*4+1] =
						window->transform->source_pixels[i][j*stride+(window->transform->width-k-1)*4+1];
					window->mask_temp->pixels[k*4+2] =
						window->transform->source_pixels[i][j*stride+(window->transform->width-k-1)*4+2];
					window->mask_temp->pixels[k*4+3] =
						window->transform->source_pixels[i][j*stride+(window->transform->width-k-1)*4+3];
				}
				(void)memcpy(&window->transform->source_pixels[i][j*stride], window->mask_temp->pixels, stride);
			}
		}

		if((window->transform->flags & TRANSFORM_REVERSE_VERTICALLY) != 0)
		{
			for(j=0; j<window->transform->height; j++)
			{
				(void)memcpy(&window->mask_temp->pixels[j*stride],
					&window->transform->source_pixels[i][(window->transform->height-j-1)*stride], stride);
			}
			(void)memcpy(window->transform->source_pixels[i], window->mask_temp->pixels, stride * window->transform->height);
		}

		before_surface = cairo_image_surface_create_for_data(window->transform->source_pixels[i],
			CAIRO_FORMAT_ARGB32, window->transform->width, window->transform->height, window->transform->width * 4);
		restore_surface = cairo_surface_create_for_rectangle(window->transform->layers[i]->surface_p,
			window->transform->x, window->transform->y, window->transform->width, window->transform->height);
		restore_cairo = cairo_create(restore_surface);

		(void)memcpy(window->transform->layers[i]->pixels, window->transform->before_pixels[i],
			window->transform->layers[i]->stride * window->transform->layers[i]->height);
		cairo_set_source_surface(restore_cairo, before_surface, 0, 0);
		cairo_paint(restore_cairo);

		cairo_destroy(restore_cairo);
		cairo_surface_destroy(before_surface);
	}

	gtk_widget_destroy(app->tool_window.detail_ui);
	switch(app->draw_window[app->active_window]->active_layer->layer_type)
	{
	case TYPE_NORMAL_LAYER:
		if((app->tool_window.flags & TOOL_USING_BRUSH) != 0)
		{
			app->tool_window.detail_ui = app->tool_window.active_brush[app->input]->create_detail_ui(
					app, app->tool_window.active_brush[app->input]);
		}
		else
		{
			app->tool_window.detail_ui = app->tool_window.active_common_tool->create_detail_ui(
				app, app->tool_window.active_common_tool->tool_data);
		}
		break;
	case TYPE_VECTOR_LAYER:
		if((app->tool_window.flags & TOOL_USING_BRUSH) != 0)
		{
			app->tool_window.detail_ui = app->tool_window.active_vector_brush[app->input]->create_detail_ui(
				app, app->tool_window.active_vector_brush[app->input]->brush_data);
		}
		else
		{
			app->tool_window.detail_ui = app->tool_window.active_common_tool->create_detail_ui(
				app, app->tool_window.active_common_tool->tool_data);
		}
		break;
	case TYPE_TEXT_LAYER:
		app->tool_window.detail_ui = CreateTextLayerDetailUI(
			app, app->draw_window[app->active_window]->active_layer,
			app->draw_window[app->active_window]->active_layer->layer_data.text_layer_p
		);
		break;
	}

	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
	gtk_widget_show_all(app->tool_window.detail_ui);

	gtk_widget_set_sensitive(app->tool_window.common_tool_table, TRUE);
	gtk_widget_set_sensitive(app->tool_window.brush_table, TRUE);

	DeleteTransformData(&app->draw_window[app->active_window]->transform);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

static void ClickedTransformReverseHorizontallyButton(GtkWidget* button, APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];
	TRANSFORM_DATA *transform = window->transform;
	int i, j, k;

	transform->flags ^= TRANSFORM_REVERSE_HORIZONTALLY;

	for(i=0; i<transform->num_layers; i++)
	{
		for(j=0; j<transform->height; j++)
		{
			for(k=0; k<transform->width; k++)
			{
				window->temp_layer->pixels[k*4] =
					transform->source_pixels[i][transform->width*4*j+((transform->width-k-1)*4)];
				window->temp_layer->pixels[k*4+1] =
					transform->source_pixels[i][transform->width*4*j+((transform->width-k-1)*4)+1];
				window->temp_layer->pixels[k*4+2] =
					transform->source_pixels[i][transform->width*4*j+((transform->width-k-1)*4)+2];
				window->temp_layer->pixels[k*4+3] =
					transform->source_pixels[i][transform->width*4*j+((transform->width-k-1)*4)+3];
			}
			(void)memcpy(&transform->source_pixels[i][transform->width*4*j],
				window->temp_layer->pixels, transform->width*4);
		}
	}

	// 変形を実行
	{
		// 元の座標から変換後のピクセルの対応を求める
		FLOAT_T start[4][2], parameter[4][9];
		FLOAT_T dst[4][2];
		FLOAT_T ave_x = 0, ave_y = 0;
		int i;

		for(i=0; i<8; i++)
		{
			ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
		}
		ave_x /= 8, ave_y /= 8;

		start[0][0] = transform->x, start[0][1] = transform->y;
		start[1][0] = transform->x, start[1][1] = transform->y + transform->height/2;
		start[2][0] = transform->x + transform->width/2, start[2][1] = transform->y + transform->height/2;
		start[3][0] = transform->x + transform->width/2, start[3][1] = transform->y;
		dst[0][0] = transform->trans[0][0], dst[0][1] = transform->trans[0][1];
		dst[1][0] = transform->trans[1][0], dst[1][1] = transform->trans[1][1];
		//dst[2][0] = start[2][0], dst[2][1] = start[2][1];
		dst[2][0] = ave_x, dst[2][1] = ave_y;
		dst[3][0] = transform->trans[7][0], dst[3][1] = transform->trans[7][1];
		CalcProjectionParameter(dst, start, parameter[0]);

		transform->min_x[0] = transform->max_x[0] = (int)dst[0][0];
		transform->min_y[0] = transform->max_y[0] = (int)dst[0][1];
		for(i=1; i<4; i++)
		{
			if(transform->min_x[0] > (int)dst[i][0])
			{
				transform->min_x[0] = (int)dst[i][0];
			}
			if(transform->max_x[0] < (int)dst[i][0]+1)
			{
				transform->max_x[0] = (int)dst[i][0]+1;
			}
			if(transform->min_y[0] > (int)dst[i][1])
			{
				transform->min_y[0] = (int)dst[i][1];
			}
			if(transform->max_y[0] < (int)dst[i][1]+1)
			{
				transform->max_y[0] = (int)dst[i][1]+1;
			}
		}

		start[0][0] = transform->x, start[0][1] = transform->y + transform->height/2;
		start[1][0] = transform->x, start[1][1] = transform->y + transform->height;
		start[2][0] = transform->x + transform->width/2, start[2][1] = transform->y + transform->height;
		start[3][0] = transform->x + transform->width/2, start[3][1] = transform->y + transform->height/2;
		dst[0][0] = transform->trans[1][0], dst[0][1] = transform->trans[1][1];
		dst[1][0] = transform->trans[2][0], dst[1][1] = transform->trans[2][1];
		dst[2][0] = transform->trans[3][0], dst[2][1] = transform->trans[3][1];
		//dst[3][0] = start[3][0], dst[3][1] = start[3][1];
		dst[3][0] = ave_x, dst[3][1] = ave_y;
		CalcProjectionParameter(dst, start, parameter[1]);

		transform->min_x[1] = transform->max_x[1] = (int)dst[0][0];
		transform->min_y[1] = transform->max_y[1] = (int)dst[0][1];
		for(i=1; i<4; i++)
		{
			if(transform->min_x[1] > (int)dst[i][0])
			{
				transform->min_x[1] = (int)dst[i][0];
			}
			if(transform->max_x[1] < (int)dst[i][0]+1)
			{
				transform->max_x[1] = (int)dst[i][0]+1;
			}
			if(transform->min_y[1] > (int)dst[i][1])
			{
				transform->min_y[1] = (int)dst[i][1];
			}
			if(transform->max_y[1] < (int)dst[i][1]+1)
			{
				transform->max_y[1] = (int)dst[i][1]+1;
			}
		}

		start[0][0] = transform->x + transform->width/2, start[0][1] = transform->y + transform->height/2;
		start[1][0] = transform->x + transform->width/2, start[1][1] = transform->y + transform->height;
		start[2][0] = transform->x + transform->width, start[2][1] = transform->y + transform->height;
		start[3][0] = transform->x + transform->width, start[3][1] = transform->y + transform->height/2;
		//dst[0][0] = start[0][0], dst[0][1] = start[0][1];
		dst[0][0] = ave_x, dst[0][1] = ave_y;
		dst[1][0] = transform->trans[3][0], dst[1][1] = transform->trans[3][1];
		dst[2][0] = transform->trans[4][0], dst[2][1] = transform->trans[4][1];
		dst[3][0] = transform->trans[5][0], dst[3][1] = transform->trans[5][1];
		CalcProjectionParameter(dst, start, parameter[2]);

		transform->min_x[2] = transform->max_x[2] = (int)dst[0][0];
		transform->min_y[2] = transform->max_y[2] = (int)dst[0][1];
		for(i=1; i<4; i++)
		{
			if(transform->min_x[2] > (int)dst[i][0])
			{
				transform->min_x[2] = (int)dst[i][0];
			}
			if(transform->max_x[2] < (int)dst[i][0]+1)
			{
				transform->max_x[2] = (int)dst[i][0]+1;
			}
			if(transform->min_y[2] > (int)dst[i][1])
			{
				transform->min_y[2] = (int)dst[i][1];
			}
			if(transform->max_y[2] < (int)dst[i][1])
			{
				transform->max_y[2] = (int)dst[i][1]+1;
			}
		}

		start[0][0] = transform->x + transform->width/2, start[0][1] = transform->y;
		start[1][0] = transform->x + transform->width/2, start[1][1] = transform->y + transform->height/2;
		start[2][0] = transform->x + transform->width, start[2][1] = transform->y + transform->height/2;
		start[3][0] = transform->x + transform->width, start[3][1] = transform->y;
		dst[0][0] = transform->trans[7][0], dst[0][1] = transform->trans[7][1];
		//dst[1][0] = start[1][0], dst[1][1] = start[1][1];
		dst[1][0] = ave_x, dst[1][1] = ave_y;
		dst[2][0] = transform->trans[5][0], dst[2][1] = transform->trans[5][1];
		dst[3][0] = transform->trans[6][0], dst[3][1] = transform->trans[6][1];
		CalcProjectionParameter(dst, start, parameter[3]);

		transform->min_x[3] = transform->max_x[3] = (int)dst[0][0];
		transform->min_y[3] = transform->max_y[3] = (int)dst[0][1];
		for(i=1; i<4; i++)
		{
			if(transform->min_x[3] > (int)dst[i][0])
			{
				transform->min_x[3] = (int)dst[i][0];
			}
			if(transform->max_x[3] < (int)dst[i][0]+1)
			{
				transform->max_x[3] = (int)dst[i][0]+1;
			}
			if(transform->min_y[3] > (int)dst[i][1])
			{
				transform->min_y[3] = (int)dst[i][1];
			}
			if(transform->max_y[3] < (int)dst[i][1]+1)
			{
				transform->max_y[3] = (int)dst[i][1]+1;
			}
		}

		if(transform->trans_type == TRANSFORM_PROJECTION)
		{
			ProjectionTransform(transform, parameter);
		}
		else
		{
			Transform(transform);
		}
	}

	if(transform->layers[0] == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}
}

static void ClickedTransformReverseVerticallyButton(GtkWidget* button, APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];
	TRANSFORM_DATA *transform = window->transform;
	int i, j;

	transform->flags ^= TRANSFORM_REVERSE_VERTICALLY;

	for(i=0; i<transform->num_layers; i++)
	{
		(void)memcpy(window->temp_layer->pixels, transform->source_pixels[i],
			transform->height*transform->width*4);
		for(j=0; j<transform->height; j++)
		{
			(void)memcpy(&transform->source_pixels[i][transform->width*4*j],
				&window->temp_layer->pixels[(transform->height-j-1)*transform->width*4], transform->width*4);
		}
	}

	// 変形を実行
	{
		// 元の座標から変換後のピクセルの対応を求める
		FLOAT_T start[4][2], parameter[4][9];
		FLOAT_T dst[4][2];
		FLOAT_T ave_x = 0, ave_y = 0;
		int i;

		for(i=0; i<8; i++)
		{
			ave_x += transform->trans[i][0], ave_y += transform->trans[i][1];
		}
		ave_x /= 8, ave_y /= 8;

		start[0][0] = transform->x, start[0][1] = transform->y;
		start[1][0] = transform->x, start[1][1] = transform->y + transform->height/2;
		start[2][0] = transform->x + transform->width/2, start[2][1] = transform->y + transform->height/2;
		start[3][0] = transform->x + transform->width/2, start[3][1] = transform->y;
		dst[0][0] = transform->trans[0][0], dst[0][1] = transform->trans[0][1];
		dst[1][0] = transform->trans[1][0], dst[1][1] = transform->trans[1][1];
		//dst[2][0] = start[2][0], dst[2][1] = start[2][1];
		dst[2][0] = ave_x, dst[2][1] = ave_y;
		dst[3][0] = transform->trans[7][0], dst[3][1] = transform->trans[7][1];
		CalcProjectionParameter(dst, start, parameter[0]);

		transform->min_x[0] = transform->max_x[0] = (int)dst[0][0];
		transform->min_y[0] = transform->max_y[0] = (int)dst[0][1];
		for(i=1; i<4; i++)
		{
			if(transform->min_x[0] > (int)dst[i][0])
			{
				transform->min_x[0] = (int)dst[i][0];
			}
			if(transform->max_x[0] < (int)dst[i][0]+1)
			{
				transform->max_x[0] = (int)dst[i][0]+1;
			}
			if(transform->min_y[0] > (int)dst[i][1])
			{
				transform->min_y[0] = (int)dst[i][1];
			}
			if(transform->max_y[0] < (int)dst[i][1]+1)
			{
				transform->max_y[0] = (int)dst[i][1]+1;
			}
		}

		start[0][0] = transform->x, start[0][1] = transform->y + transform->height/2;
		start[1][0] = transform->x, start[1][1] = transform->y + transform->height;
		start[2][0] = transform->x + transform->width/2, start[2][1] = transform->y + transform->height;
		start[3][0] = transform->x + transform->width/2, start[3][1] = transform->y + transform->height/2;
		dst[0][0] = transform->trans[1][0], dst[0][1] = transform->trans[1][1];
		dst[1][0] = transform->trans[2][0], dst[1][1] = transform->trans[2][1];
		dst[2][0] = transform->trans[3][0], dst[2][1] = transform->trans[3][1];
		//dst[3][0] = start[3][0], dst[3][1] = start[3][1];
		dst[3][0] = ave_x, dst[3][1] = ave_y;
		CalcProjectionParameter(dst, start, parameter[1]);

		transform->min_x[1] = transform->max_x[1] = (int)dst[0][0];
		transform->min_y[1] = transform->max_y[1] = (int)dst[0][1];
		for(i=1; i<4; i++)
		{
			if(transform->min_x[1] > (int)dst[i][0])
			{
				transform->min_x[1] = (int)dst[i][0];
			}
			if(transform->max_x[1] < (int)dst[i][0]+1)
			{
				transform->max_x[1] = (int)dst[i][0]+1;
			}
			if(transform->min_y[1] > (int)dst[i][1])
			{
				transform->min_y[1] = (int)dst[i][1];
			}
			if(transform->max_y[1] < (int)dst[i][1]+1)
			{
				transform->max_y[1] = (int)dst[i][1]+1;
			}
		}

		start[0][0] = transform->x + transform->width/2, start[0][1] = transform->y + transform->height/2;
		start[1][0] = transform->x + transform->width/2, start[1][1] = transform->y + transform->height;
		start[2][0] = transform->x + transform->width, start[2][1] = transform->y + transform->height;
		start[3][0] = transform->x + transform->width, start[3][1] = transform->y + transform->height/2;
		//dst[0][0] = start[0][0], dst[0][1] = start[0][1];
		dst[0][0] = ave_x, dst[0][1] = ave_y;
		dst[1][0] = transform->trans[3][0], dst[1][1] = transform->trans[3][1];
		dst[2][0] = transform->trans[4][0], dst[2][1] = transform->trans[4][1];
		dst[3][0] = transform->trans[5][0], dst[3][1] = transform->trans[5][1];
		CalcProjectionParameter(dst, start, parameter[2]);

		transform->min_x[2] = transform->max_x[2] = (int)dst[0][0];
		transform->min_y[2] = transform->max_y[2] = (int)dst[0][1];
		for(i=1; i<4; i++)
		{
			if(transform->min_x[2] > (int)dst[i][0])
			{
				transform->min_x[2] = (int)dst[i][0];
			}
			if(transform->max_x[2] < (int)dst[i][0]+1)
			{
				transform->max_x[2] = (int)dst[i][0]+1;
			}
			if(transform->min_y[2] > (int)dst[i][1])
			{
				transform->min_y[2] = (int)dst[i][1];
			}
			if(transform->max_y[2] < (int)dst[i][1])
			{
				transform->max_y[2] = (int)dst[i][1]+1;
			}
		}

		start[0][0] = transform->x + transform->width/2, start[0][1] = transform->y;
		start[1][0] = transform->x + transform->width/2, start[1][1] = transform->y + transform->height/2;
		start[2][0] = transform->x + transform->width, start[2][1] = transform->y + transform->height/2;
		start[3][0] = transform->x + transform->width, start[3][1] = transform->y;
		dst[0][0] = transform->trans[7][0], dst[0][1] = transform->trans[7][1];
		//dst[1][0] = start[1][0], dst[1][1] = start[1][1];
		dst[1][0] = ave_x, dst[1][1] = ave_y;
		dst[2][0] = transform->trans[5][0], dst[2][1] = transform->trans[5][1];
		dst[3][0] = transform->trans[6][0], dst[3][1] = transform->trans[6][1];
		CalcProjectionParameter(dst, start, parameter[3]);

		transform->min_x[3] = transform->max_x[3] = (int)dst[0][0];
		transform->min_y[3] = transform->max_y[3] = (int)dst[0][1];
		for(i=1; i<4; i++)
		{
			if(transform->min_x[3] > (int)dst[i][0])
			{
				transform->min_x[3] = (int)dst[i][0];
			}
			if(transform->max_x[3] < (int)dst[i][0]+1)
			{
				transform->max_x[3] = (int)dst[i][0]+1;
			}
			if(transform->min_y[3] > (int)dst[i][1])
			{
				transform->min_y[3] = (int)dst[i][1];
			}
			if(transform->max_y[3] < (int)dst[i][1]+1)
			{
				transform->max_y[3] = (int)dst[i][1]+1;
			}
		}

		if(transform->trans_type == TRANSFORM_PROJECTION)
		{
			ProjectionTransform(transform, parameter);
		}
		else
		{
			Transform(transform);
		}
	}

	if(transform->layers[0] == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}
}

/*****************************************************
* CreateTransformDetailUI関数                        *
* 変形処理時のUIウィジェットを作成                   *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
* 返り値                                             *
*	作成したUIウィジェット                           *
*****************************************************/
GtkWidget* CreateTransformDetailUI(APPLICATION* app)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox;
	GtkWidget *buttons[4];
	GtkWidget *separator;
	GtkWidget *label;
	char str[256];

	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.transform_free);
	g_object_set_data(G_OBJECT(buttons[0]), "trans_mode", GINT_TO_POINTER(0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[0]), TRUE);
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(ChangeTransformMode), app);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
		GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.transform_scale);
	g_object_set_data(G_OBJECT(buttons[1]), "trans_mode", GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(ChangeTransformMode), app);
	buttons[2] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
		GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.transform_free_shape);
	g_object_set_data(G_OBJECT(buttons[2]), "trans_mode", GINT_TO_POINTER(2));
	g_signal_connect(G_OBJECT(buttons[2]), "toggled", G_CALLBACK(ChangeTransformMode), app);
	buttons[3] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
		GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.transform_rotate);
	g_object_set_data(G_OBJECT(buttons[3]), "trans_mode", GINT_TO_POINTER(3));
	g_signal_connect(G_OBJECT(buttons[3]), "toggled", G_CALLBACK(ChangeTransformMode), app);

	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[2], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[3], FALSE, FALSE, 0);

	buttons[0] = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(buttons[0]), "clicked",
		G_CALLBACK(ClickedTransformButtonOK), app);
	buttons[1] = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(buttons[1]), "clicked",
		G_CALLBACK(ClickedTransformButtonCancel), app);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), buttons[1], FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);

	(void)sprintf(str, "%s : ", app->labels->tool_box.reverse);
	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 3);

	buttons[0] = gtk_check_button_new_with_label(app->labels->tool_box.reverse_horizontally);
	g_signal_connect(G_OBJECT(buttons[0]), "toggled",
		G_CALLBACK(ClickedTransformReverseHorizontallyButton), app);
	gtk_box_pack_start(GTK_BOX(hbox), buttons[0], FALSE, FALSE, 0);

	buttons[1] = gtk_check_button_new_with_label(app->labels->tool_box.reverse_vertically);
	g_signal_connect(G_OBJECT(buttons[1]), "toggled",
		G_CALLBACK(ClickedTransformReverseVerticallyButton), app);
	gtk_box_pack_start(GTK_BOX(hbox), buttons[1], FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	return vbox;
}

#ifdef __cplusplus
}
#endif
