/*
* layer.c
* レイヤーの追加、削除、順序変更等を定義
*/

// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <zlib.h>
#include "layer.h"
#include "memory.h"
#include "application.h"
#include "history.h"
#include "tool_box.h"
#include "memory_stream.h"
#include "image_read_write.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************
* CreateLayer関数                                    *
* 引数                                               *
* x				: レイヤー左上のX座標                *
* y				: レイヤー左上のY座標                *
* width			: レイヤーの幅                       *
* height		: レイヤーの高さ                     *
* channel		: レイヤーのチャンネル数             *
* layer_type	: レイヤーのタイプ                   *
* prev_layer	: 追加するレイヤーの前のレイヤー     *
* next_layer	: 追加するレイヤーの次のレイヤー     *
* name			: 追加するレイヤーの名前             *
* window		: 追加するレイヤーを管理する描画領域 *
* 返り値                                             *
*	初期化された構造体のアドレス                     *
*****************************************************/
LAYER* CreateLayer(
	int32 x,
	int32 y,
	int32 width,
	int32 height,
	int8 channel,
	eLAYER_TYPE layer_type,
	LAYER* prev_layer,
	LAYER* next_layer,
	const char *name,
	struct _DRAW_WINDOW* window
)
{
	// 返り値
	LAYER* ret = (LAYER*)MEM_ALLOC_FUNC(sizeof(*ret));
	// レイヤーのフォーマット
	cairo_format_t format =
		(channel > 3) ? CAIRO_FORMAT_ARGB32
			: (channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;

	// 0初期化
	(void)memset(ret, 0, sizeof(*ret));

	// 値のセット
	ret->name = (name == NULL) ? NULL : MEM_STRDUP_FUNC(name);
	ret->channel = (channel <= 4) ? channel : 4;
	ret->layer_type = (uint16)layer_type;
	ret->x = x, ret->y = y;
	ret->width = width, ret->height = height, ret->stride = width * ret->channel;
	ret->pixels = (uint8*)MEM_ALLOC_FUNC(sizeof(uint8)*width*height*channel);
	ret->alpha = 100;
	ret->window = window;

	// ピクセルを初期化
	(void)memset(ret->pixels, 0x0, sizeof(uint8)*(ret->stride*height));

	// cairoの生成
	ret->surface_p = cairo_image_surface_create_for_data(
		ret->pixels, format, width, height, ret->stride
	);
	ret->cairo_p = cairo_create(ret->surface_p);

	if(layer_type == TYPE_VECTOR_LAYER)
	{	// ベクトルレイヤーなら
		VECTOR_LAYER_RECTANGLE rect = {0, 0, width, height};
		// 線をレイヤーにするための準備
		ret->layer_data.vector_layer_p =
			(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*ret->layer_data.vector_layer_p));
		(void)memset(ret->layer_data.vector_layer_p, 0, sizeof(*ret->layer_data.vector_layer_p));
		// ラインを合成したレイヤー
		ret->layer_data.vector_layer_p->mix = CreateVectorLineLayer(ret, NULL, &rect);

		// 一番下に空のレイヤーを作成する
		ret->layer_data.vector_layer_p->base =
			(VECTOR_DATA*)CreateVectorLine(NULL, NULL);
		(void)memset(ret->layer_data.vector_layer_p->base, 0, sizeof(VECTOR_LINE));
		ret->layer_data.vector_layer_p->base->line.base_data.layer =
			CreateVectorLineLayer(ret, NULL, &rect);
		ret->layer_data.vector_layer_p->top_data = ret->layer_data.vector_layer_p->base;
	}
	else if(layer_type == TYPE_LAYER_SET)
	{
		ret->layer_data.layer_set_p =
			(LAYER_SET*)MEM_ALLOC_FUNC(sizeof(*ret->layer_data.layer_set_p));
		(void)memset(ret->layer_data.layer_set_p, 0, sizeof(*ret->layer_data.layer_set_p));
		ret->layer_data.layer_set_p->active_under =
			CreateLayer(0, 0, width, height, channel, TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
	}
	else if(layer_type == TYPE_3D_LAYER)
	{
		ret->layer_data.project = window->first_project;
	}

	// 前後のレイヤーが存在するなら関係をセット
	if(prev_layer != NULL)
	{
		ret->prev = prev_layer;
		ret->layer_set = prev_layer->layer_set;
		prev_layer->next = ret;
	}
	if(next_layer != NULL)
	{
		ret->next = next_layer;
		next_layer->prev = ret;
	}

	return ret;
}

/*************************************************************
* AddLayer関数                                               *
* 描画領域にレイヤーを追加する                               *
* 引数                                                       *
* canvas		: レイヤーを追加する描画領域                 *
* layer_type	: 追加するレイヤーのタイプ(通常、ベクトル等) *
* layer_name	: 追加するレイヤーの名前                     *
* prev_layer	: 追加したレイヤーの下にくるレイヤー         *
* 返り値                                                     *
*	成功:レイヤー構造体のアドレス	失敗:NULL                *
*************************************************************/
LAYER* AddLayer(
	DRAW_WINDOW* window,
	eLAYER_TYPE layer_type,
	const char* layer_name,
	LAYER* prev_layer
)
{
	LAYER *new_layer;
	LAYER *prev = prev_layer;
	LAYER *next;

	if(layer_name == NULL)
	{
		return NULL;
	}

	if(*layer_name == '\0')
	{
		return NULL;
	}

	if(CorrectLayerName(window->layer, layer_name) == FALSE)
	{
		return NULL;
	}

	if(prev == NULL)
	{
		next = window->layer;
	}
	else
	{
		next = prev->next;
	}

	new_layer = CreateLayer(0, 0, window->width, window->height, window->channel,
		layer_type, prev, next, layer_name, window);
	window->num_layer++;

	LayerViewAddLayer(new_layer, window->layer, window->app->layer_window.view, window->num_layer);

	return new_layer;
}

/*********************************************************
* RemoveLayer関数                                        *
* レイヤーを削除する(キャンバスに登録されているもののみ) *
* 引数                                                   *
* target	: 削除するレイヤー                           *
*********************************************************/
void RemoveLayer(LAYER* target)
{
	DRAW_WINDOW *window;
	LAYER *layer = target;

	if(target == NULL || target->name == NULL || target->window == NULL)
	{
		return;
	}

	window = target->window;
	if(target->window->num_layer <= 1)
	{
		return;
	}

	DeleteLayer(&layer);
	window->num_layer--;

	UpdateDrawWindow(window);
}

/*****************************************************
* CreateDispTempLayer関数                            *
* 表示用一時保存レイヤーを作成                       *
* 引数                                               *
* x				: レイヤー左上のX座標                *
* y				: レイヤー左上のY座標                *
* width			: レイヤーの幅                       *
* height		: レイヤーの高さ                     *
* channel		: レイヤーのチャンネル数             *
* layer_type	: レイヤーのタイプ                   *
* window		: 追加するレイヤーを管理する描画領域 *
* 返り値                                             *
*	初期化された構造体のアドレス                     *
*****************************************************/
LAYER* CreateDispTempLayer(
	int32 x,
	int32 y,
	int32 width,
	int32 height,
	int8 channel,
	eLAYER_TYPE layer_type,
	struct _DRAW_WINDOW* window
)
{
	// 返り値
	LAYER* ret = (LAYER*)MEM_ALLOC_FUNC(sizeof(*ret));
	// レイヤーのフォーマット
	cairo_format_t format =
		(channel > 3) ? CAIRO_FORMAT_ARGB32
			: (channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;

	// 表示用の描画領域サイズを計算
	window->disp_size = (int)(2 * sqrt((width/2)*(width/2)+(height/2)*(height/2)) + 1);
	window->disp_stride = window->disp_size * 4;
	window->half_size = window->disp_size * 0.5;

	// 0初期化
	(void)memset(ret, 0, sizeof(*ret));

	// 値のセット
	ret->channel = (channel <= 4) ? channel : 4;
	ret->layer_type = (uint16)layer_type;
	ret->x = x, ret->y = y;
	ret->width = width, ret->height = height, ret->stride = width * ret->channel;
	ret->pixels = (uint8*)MEM_ALLOC_FUNC(sizeof(uint8)*window->disp_stride*window->disp_size);
	ret->alpha = 100;
	ret->window = window;

	// ピクセルを初期化
	(void)memset(ret->pixels, 0x0, sizeof(uint8)*window->disp_stride*window->disp_size);

	// cairoの生成
	ret->surface_p = cairo_image_surface_create_for_data(
		ret->pixels, format, width, height, ret->stride
	);
	ret->cairo_p = cairo_create(ret->surface_p);

	return ret;
}

/*********************************************
* DeleteLayer関数                            *
* レイヤーを削除する                         *
* 引数                                       *
* layer	: 削除するレイヤーポインタのアドレス *
*********************************************/
void DeleteLayer(LAYER** layer)
{
	if((*layer)->layer_type == TYPE_VECTOR_LAYER)
	{
		DeleteVectorLayer(&(*layer)->layer_data.vector_layer_p);
	}
	else if((*layer)->layer_type == TYPE_TEXT_LAYER)
	{
		DeleteTextLayer(&(*layer)->layer_data.text_layer_p);
	}
	else if((*layer)->layer_type == TYPE_LAYER_SET)
	{
		DeleteLayerSet(*layer, (*layer)->window);
	}

	if((*layer)->next != NULL)
	{
		(*layer)->next->prev = (*layer)->prev;
	}
	if((*layer)->prev != NULL)
	{
		(*layer)->prev->next = (*layer)->next;
	}

	if((*layer)->widget != NULL)
	{
		MEM_FREE_FUNC((*layer)->widget->eye);
		MEM_FREE_FUNC((*layer)->widget->pin);
		gtk_widget_destroy((*layer)->widget->box);
		MEM_FREE_FUNC((*layer)->widget);
	}

	cairo_destroy((*layer)->cairo_p);
	cairo_surface_destroy((*layer)->surface_p);
	MEM_FREE_FUNC((*layer)->name);

	if((*layer)->widget != NULL)
	{
		(*layer)->window->num_layer--;
		(*layer)->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}

	(void)memset((*layer)->window->work_layer->pixels, 0,
		(*layer)->window->pixel_buf_size);

	MEM_FREE_FUNC((*layer)->pixels);

	MEM_FREE_FUNC(*layer);

	*layer = NULL;
}

/*********************************************
* DeleteTempLayer関数                        *
* 一時的に作成したレイヤーを削除する         *
* 引数                                       *
* layer	: 削除するレイヤーポインタのアドレス *
*********************************************/
void DeleteTempLayer(LAYER** layer)
{
	if((*layer)->layer_type == TYPE_VECTOR_LAYER)
	{
		DeleteVectorLayer(&(*layer)->layer_data.vector_layer_p);
	}
	else if((*layer)->layer_type == TYPE_TEXT_LAYER)
	{
		DeleteTextLayer(&(*layer)->layer_data.text_layer_p);
	}
	else if((*layer)->layer_type == TYPE_LAYER_SET)
	{
		DeleteLayerSet(*layer, (*layer)->window);
	}

	if((*layer)->next != NULL)
	{
		(*layer)->next->prev = (*layer)->prev;
	}
	if((*layer)->prev != NULL)
	{
		(*layer)->prev->next = (*layer)->next;
	}

	cairo_destroy((*layer)->cairo_p);
	cairo_surface_destroy((*layer)->surface_p);
	MEM_FREE_FUNC((*layer)->name);

	if((*layer)->widget != NULL)
	{
		(*layer)->window->num_layer--;
		(*layer)->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}

	MEM_FREE_FUNC((*layer)->pixels);

	MEM_FREE_FUNC(*layer);

	*layer = NULL;
}

/*********************************
* DELETE_LAYER_HISTORY構造体     *
* レイヤー削除の履歴データを格納 *
*********************************/
typedef struct _DELETE_LAYER_HISTORY
{
	uint16 layer_type;			// レイヤータイプ
	uint16 layer_mode;			// レイヤーの合成モード
	int32 x, y;					// レイヤー左上の座標
	// レイヤーの幅、高さ、一行分のバイト数
	int32 width, height, stride;
	uint32 flags;				// レイヤーの表示・非表示等のフラグ
	int8 channel;				// レイヤーのチャンネル数
	int8 alpha;					// レイヤーの不透明度
	uint16 name_length;			// レイヤーの名前の長さ
	gchar* name;				// レイヤーの名前
	uint16 prev_name_length;	// 前のレイヤーの文字数
	gchar* prev_name;			// 前のレイヤーの名前
	size_t iamge_data_size;		// 画像データのバイト数
	uint8* image_data;			// 画像データ
} DELETE_LAYER_HISTORY;

/*****************************
* DeleteLayerUndo関数        *
* レイヤー削除操作のアンドゥ *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void DeleteLayerUndo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	DELETE_LAYER_HISTORY history;
	// 履歴データ読み込み用ストリーム
	MEMORY_STREAM stream;
	// レイヤーのピクセルデータ
	uint8* pixels;
	// レイヤーの幅、高さ、一行分のバイト数
	gint32 width, height, stride;
	// 履歴データバイト数
	size_t data_size;
	// 復活させるレイヤー
	LAYER* layer;
	// 前のレイヤー
	LAYER* prev;

	// 履歴データを0初期化
	(void)memset(&history, 0, sizeof(history));

	// 履歴データのバイト数を取得
	(void)memcpy(&data_size, p, sizeof(data_size));

	// ストリームを初期化
	stream.block_size = 1;
	stream.data_size = data_size;
	stream.buff_ptr = (uint8*)p;
	stream.data_point = sizeof(data_size);

	// レイヤー情報を読み込む
	(void)MemRead(&history, 1,
		offsetof(DELETE_LAYER_HISTORY, name_length), &stream);
	// レイヤーの名前を読み込む
	(void)MemRead(&history.name_length,
		sizeof(history.name_length), 1, &stream);
	history.name = (gchar*)MEM_ALLOC_FUNC(history.name_length);
	(void)MemRead(history.name, 1, history.name_length, &stream);
	// 前のレイヤーの名前を読み込む
	(void)MemRead(&history.prev_name_length,
		sizeof(history.prev_name_length), 1, &stream);
	// 前のレイヤーの有無を確認
		// ついでに前のレイヤーのポインタをセットする
	if(history.prev_name_length != 0)
	{
		history.prev_name = (gchar*)MEM_ALLOC_FUNC(history.prev_name_length);
		(void)MemRead(history.prev_name, 1,
			history.prev_name_length, &stream);
		prev = SearchLayer(window->layer, history.prev_name);
	}
	else
	{
		prev = NULL;
	}

	// 復活させるレイヤー作成
	layer = CreateLayer(
		history.x, history.y, history.width, history.height, history.channel,
		history.layer_type, prev, (prev == NULL) ? window->layer : prev->next,
		history.name, window
	);

	// レイヤーのピクセルデータを読み込む
	(void)MemSeek(&stream, sizeof(size_t), SEEK_CUR);
	pixels = ReadPNGStream(
		&stream,
		(size_t (*)(void*, size_t, size_t, void*))MemRead,
		&width, &height, &stride
	);

	// レイヤーのピクセルメモリにコピー
	(void)memcpy(layer->pixels, pixels, stride*height);

	MEM_FREE_FUNC(history.name);
	MEM_FREE_FUNC(history.prev_name);
	MEM_FREE_FUNC(pixels);

	// 描画領域を更新
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	// レイヤーの数を更新
	window->num_layer++;

	// レイヤービューにレイヤーを追加
	LayerViewAddLayer(layer, window->layer,
		window->app->layer_window.view, window->num_layer);
}

/*****************************
* DeleteLayerRedo関数        *
* レイヤー削除操作のリドゥ   *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void DeleteLayerRedo(DRAW_WINDOW* window, void* p)
{
	// 削除レイヤー
	LAYER* delete_layer;
	// 削除レイヤー名読み込み用
	MEMORY_STREAM stream;
	// 削除レイヤーの名前
	gchar* name;
	// 削除レイヤー名の長さ
	uint16 name_length;

	// ストリームを初期化
	stream.block_size = 1;
	(void)memcpy(&stream.data_size, p, sizeof(stream.data_size));
	stream.data_point =
		offsetof(DELETE_LAYER_HISTORY, name_length) + sizeof(size_t);
	stream.buff_ptr = (uint8*)p;

	// 削除レイヤー名の長さを取得
	(void)MemRead(&name_length, sizeof(name_length), 1, &stream);
	name = (gchar*)MEM_ALLOC_FUNC(name_length);
	(void)MemRead(name, 1, name_length, &stream);

	delete_layer = SearchLayer(window->layer, name);
	DeleteLayer(&delete_layer);

	MEM_FREE_FUNC(name);
}

/*********************************
* AddDeleteLayerHistory関数      *
* レイヤーの削除履歴データを作成 *
* 引数                           *
* window	: 描画領域のデータ   *
* target	: 削除するレイヤー   *
*********************************/
void AddDeleteLayerHistory(
	DRAW_WINDOW* window,
	LAYER* target
)
{
// データの圧縮レベル
#define DATA_COMPRESSION_LEVEL 8
	// 履歴データ
	DELETE_LAYER_HISTORY history;
	// PNGデータを作成するためのストリーム
	MEMORY_STREAM_PTR stream = NULL;
	// 履歴データを作成するためのストリーム
	MEMORY_STREAM_PTR history_data =
		CreateMemoryStream(target->stride*target->height*4);
	// 前のレイヤーの名前の長さ
	uint16 name_length;
	// 履歴データのバイト数
	size_t data_size;

	// データのバイト数を記憶する部分を空ける
	(void)MemSeek(history_data, sizeof(size_t), SEEK_SET);

	// レイヤーのデータを履歴にセット
	history.layer_type = target->layer_type;
	history.layer_mode = target->layer_mode;
	history.x = target->x;
	history.y = target->y;
	history.width = target->width;
	history.height = target->height;
	history.stride = target->stride;
	history.flags = target->flags;
	history.channel = target->channel;
	history.alpha = target->alpha;
	// ストリームへ書き込む
	(void)MemWrite(&history, 1,
		offsetof(DELETE_LAYER_HISTORY, name_length), history_data);

	// レイヤーの名前を書き込む
	name_length = (uint16)strlen(target->name) + 1;
	(void)MemWrite(&name_length, sizeof(name_length), 1, history_data);
	(void)MemWrite(target->name, 1, name_length, history_data);

	// 前のレイヤーの名前を書き込む
	if(target->prev == NULL)
	{
		name_length = 0;
		(void)MemWrite(&name_length, sizeof(name_length), 1, history_data);
	}
	else
	{
		name_length = (uint16)strlen(target->name) + 1;
		(void)MemWrite(&name_length, sizeof(name_length), 1, history_data);
		(void)MemWrite(target->prev->name, 1, name_length, history_data);
	}

	if(target->layer_type == TYPE_NORMAL_LAYER)
	{
		stream =
			CreateMemoryStream(target->stride*target->height*2);

		// 削除するレイヤーのピクセルデータからPNGデータ作成
		WritePNGStream(
			(void*)stream,
			(size_t (*)(void*, size_t, size_t, void*))MemWrite,
			NULL,
			target->pixels,
			target->width,
			target->height,
			target->stride,
			target->channel,
			0,
			DATA_COMPRESSION_LEVEL
		);

		// PNGデータを書き込む
		(void)MemWrite(&stream->data_point, sizeof(stream->data_point), 1, history_data);
		(void)MemWrite(stream->buff_ptr, 1, stream->data_point, history_data);
	}
	else if(target->layer_type == TYPE_VECTOR_LAYER)
	{	// ベクトル情報を書き込む
		VECTOR_LINE* line = target->layer_data.vector_layer_p->base->line.base_data.next;
		size_t stream_size = 0;
		VECTOR_LINE_BASE_DATA base;

		while(line != NULL)
		{
			if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
			{
				stream_size += sizeof(VECTOR_LINE_BASE_DATA) + sizeof(VECTOR_POINT)*line->num_points;
			}
			else if(line->base_data.vector_type == VECTOR_TYPE_SQUARE)
			{
				stream_size += sizeof(VECTOR_SQUARE);
			}
			else if(line->base_data.vector_type == VECTOR_TYPE_ECLIPSE)
			{
				stream_size += sizeof(VECTOR_ECLIPSE);
			}
			line = line->base_data.next;
		}

		if(stream_size != 0)
		{
			stream = CreateMemoryStream(stream_size);
			line = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next;
			while(line != NULL)
			{
				if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					base.vector_type = line->base_data.vector_type;
					base.num_points = line->num_points;
					base.blur = line->blur;
					base.outline_hardness = line->outline_hardness;

					(void)MemWrite(&base, sizeof(base), 1, stream);
					(void)MemWrite(line->points, sizeof(*line->points), line->num_points, stream);
				}
				else
				{
					WriteVectorShapeData((VECTOR_DATA*)line, stream);
				}

				line = line->base_data.next;
			}
		
			// 履歴データに追加する
			(void)MemWrite(stream->buff_ptr, 1, stream->data_point, history_data);
		}
	}

	// データバイト数を書き込む
	data_size = history_data->data_point;
	(void)MemSeek(history_data, 0, SEEK_SET);
	(void)MemWrite(&data_size, sizeof(data_size), 1,
		history_data);

	// 履歴に追加
	AddHistory(&window->history, window->app->labels->menu.delete_layer,
		history_data->buff_ptr, (uint32)data_size, DeleteLayerUndo, DeleteLayerRedo
	);

	(void)DeleteMemoryStream(stream);
	(void)DeleteMemoryStream(history_data);
}

/***********************************************************
* ResizeLayerBuffer関数                                    *
* レイヤーのピクセルデータのサイズを変更する               *
* 引数                                                     *
* target		: ピクセルデータのサイズを変更するレイヤー *
* new_width		: 新しいレイヤーの幅                       *
* new_height	: 新しいレイヤーの高さ                     *
***********************************************************/
void ResizeLayerBuffer(
	LAYER* target,
	int32 new_width,
	int32 new_height
)
{
	// CAIROに設定するフォーマット情報
	cairo_format_t format = (target->channel == 4) ?
		CAIRO_FORMAT_ARGB32 : (target->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;
	// レイヤーに新しい幅、高さ、一行分のバイト数を設定
	target->width = new_width;
	target->height = new_height;
	target->stride = new_width * target->channel;

	// CAIROを一度破棄する
	cairo_surface_destroy(target->surface_p);
	cairo_destroy(target->cairo_p);

	// ピクセルデータとCAIROおよびそのサーフェースを再度作成
	target->pixels = (uint8*)MEM_REALLOC_FUNC(target->pixels, target->stride*target->height);
	target->surface_p = cairo_image_surface_create_for_data(
		target->pixels, format, new_width, new_height, target->stride);
	target->cairo_p = cairo_create(target->surface_p);
}

/*******************************************
* ResizeLayer関数                          *
* レイヤーのサイズを変更する(拡大縮小有)   *
* 引数                                     *
* target		: サイズを変更するレイヤー *
* new_width		: 新しいレイヤーの幅       *
* new_height	: 新しいレイヤーの高さ     *
*******************************************/
void ResizeLayer(
	LAYER* target,
	int32 new_width,
	int32 new_height
)
{
	// CAIROを作成し直すのでローカルで一度アドレスを受ける
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	uint8 *pixels;
	// CAIROに設定するフォーマット情報
	cairo_format_t format = (target->channel == 4) ?
		CAIRO_FORMAT_ARGB32 : (target->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;
	// サイズ変更時の幅・高さに対する拡大縮小率
	gdouble zoom_x = (gdouble)new_width / (gdouble)target->width,
		zoom_y = (gdouble)new_height / (gdouble)target->height;

	// ピクセルデータのメモリ領域を確保
	pixels = (uint8*)MEM_ALLOC_FUNC(
		sizeof(*pixels)*new_width*new_height*target->channel);
	// 0初期化
	(void)memset(pixels, 0, sizeof(*pixels)*new_width*new_height*target->channel);

	// CAIROを作成
	surface_p = cairo_image_surface_create_for_data(pixels,
		format, new_width, new_height, new_width * target->channel);
	cairo_p = cairo_create(surface_p);

	// レイヤーの種類で処理切り替え
	switch(target->layer_type)
	{
	case TYPE_NORMAL_LAYER:	// 通常レイヤー
		// ピクセルデータを拡大or縮小して写す
		cairo_scale(cairo_p, zoom_x, zoom_y);
		cairo_set_source_surface(cairo_p, target->surface_p, 0, 0);
		cairo_paint(cairo_p);
		// 拡大縮小率を元に戻す
		cairo_scale(cairo_p, 1/zoom_x, 1/zoom_y);

		// 変更が終わったのでピクセルデータとCAIROの情報を更新
		(void)MEM_FREE_FUNC(target->pixels);
		target->pixels = pixels;
		cairo_surface_destroy(target->surface_p);
		target->surface_p = surface_p;
		cairo_destroy(target->cairo_p);
		target->cairo_p = cairo_p;

		break;
	case TYPE_VECTOR_LAYER:	// ベクトルレイヤー
		{
			// 座標を修正するベクトルデータ
			VECTOR_LINE *line = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next;
			// 一番下のラインはメモリ再確保
			VECTOR_LINE_LAYER *line_layer = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.layer;
			int i;	// for文用のカウンタ

			// 先にレイヤーのピクセルデータとCAIRO情報を更新
			(void)MEM_FREE_FUNC(target->pixels);
			target->pixels = pixels;
			cairo_surface_destroy(target->surface_p);
			target->surface_p = surface_p;
			cairo_destroy(target->cairo_p);
			target->cairo_p = cairo_p;

			// 全てのラインの座標を修正
			while(line != NULL)
			{
				if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					for(i=0; i<line->num_points; i++)
					{
						line->points[i].x *= zoom_x;
						line->points[i].y *= zoom_y;
						line->points[i].size *= (zoom_x + zoom_y) * 0.5;
					}
				}
				else if(line->base_data.vector_type == VECTOR_TYPE_SQUARE)
				{
					VECTOR_SQUARE *square = (VECTOR_SQUARE*)line;
					square->left *= zoom_x;
					square->top *= zoom_y;;
					square->width *= zoom_x;
					square->height *= zoom_y;
					square->line_width *= (zoom_x + zoom_y) * 0.5;
				}
				else if(line->base_data.vector_type == VECTOR_TYPE_ECLIPSE)
				{
					VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)line;
					eclipse->x *= zoom_x;
					eclipse->y *= zoom_y;
					eclipse->radius *= (zoom_x + zoom_y) * 0.5;
					eclipse->line_width *= (zoom_x + zoom_y) * 0.5;
				}

				// 次のラインへ
				line = line->base_data.next;
			}

			// 一番下のレイヤーのラインレイヤーを作成し直す
			line_layer->width = new_width, line_layer->height = new_height;
			line_layer->stride = new_width * target->channel;
			line_layer->pixels = (uint8*)MEM_REALLOC_FUNC(
				line_layer->pixels, new_width*new_height*target->channel);
			(void)memset(line_layer->pixels, 0,
				new_width*new_height*target->channel);
			cairo_surface_destroy(line_layer->surface_p);
			cairo_destroy(line_layer->cairo_p);
			line_layer->surface_p = cairo_image_surface_create_for_data(
				line_layer->pixels, format, new_width, new_height, new_width*target->channel);
			line_layer->cairo_p = cairo_create(line_layer->surface_p);

			// ラインを合成したレイヤーも作り直し
			line_layer = target->layer_data.vector_layer_p->mix;
			line_layer->width = new_width, line_layer->height = new_height;
			line_layer->stride = new_width * target->channel;
			line_layer->pixels = (uint8*)MEM_REALLOC_FUNC(
				line_layer->pixels, new_width*new_height*target->channel);
			(void)memset(line_layer->pixels, 0,
				new_width*new_height*target->channel);
			cairo_surface_destroy(line_layer->surface_p);
			cairo_destroy(line_layer->cairo_p);
			line_layer->surface_p = cairo_image_surface_create_for_data(
				line_layer->pixels, format, new_width, new_height, new_width*target->channel);
			line_layer->cairo_p = cairo_create(line_layer->surface_p);

			target->width = new_width;
			target->height = new_height;
			target->stride = new_width * target->channel;

			// ラインを全てラスタライズ
			ChangeActiveLayer(target->window, target);
			target->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ALL | VECTOR_LAYER_FIX_LINE;
			RasterizeVectorLayer(target->window, target, target->layer_data.vector_layer_p);
		}
		break;
	case TYPE_TEXT_LAYER:	// テキストレイヤー
		{
			// 文字の拡大縮小率に適用する値
			gdouble zoom = (zoom_x > zoom_y) ? zoom_x : zoom_y;

			// 先にレイヤーのピクセルデータとCAIRO情報を更新
			(void)MEM_FREE_FUNC(target->pixels);
			target->pixels = pixels;
			cairo_surface_destroy(target->surface_p);
			target->surface_p = surface_p;
			cairo_destroy(target->cairo_p);
			target->cairo_p = cairo_p;

			// 文字を表示する座標・範囲を変更
			target->layer_data.text_layer_p->x *= zoom_x;
			target->layer_data.text_layer_p->y *= zoom_y;
			target->layer_data.text_layer_p->width *= zoom_x;
			target->layer_data.text_layer_p->height *= zoom_y;

			// フォントサイズを変更
			target->layer_data.text_layer_p->font_size *= zoom;

			// 文字情報をレンダリング
			RenderTextLayer(target->window, target, target->layer_data.text_layer_p);
		}
		break;
	case TYPE_LAYER_SET:
		// ピクセルデータを拡大or縮小して写す
		cairo_scale(cairo_p, zoom_x, zoom_y);
		cairo_set_source_surface(cairo_p, target->surface_p, 0, 0);
		cairo_paint(cairo_p);
		// 拡大縮小率を元に戻す
		cairo_scale(cairo_p, 1/zoom_x, 1/zoom_y);

		// 変更が終わったのでピクセルデータとCAIROの情報を更新
		(void)MEM_FREE_FUNC(target->pixels);
		target->pixels = pixels;
		cairo_surface_destroy(target->surface_p);
		target->surface_p = surface_p;
		cairo_destroy(target->cairo_p);
		target->cairo_p = cairo_p;

		break;
	}	// レイヤーの種類で処理切り替え
		// switch(target->layer_type)

	// レイヤーの幅、高さ、1行分のバイト数を設定
	target->width = new_width;
	target->height = new_height;
	target->stride = new_width * target->channel;
}

/*******************************************
* ChangeLayerSize関数                      *
* レイヤーのサイズを変更する(拡大縮小無)   *
* 引数                                     *
* target		: サイズを変更するレイヤー *
* new_width		: 新しいレイヤーの幅       *
* new_height	: 新しいレイヤーの高さ     *
*******************************************/
void ChangeLayerSize(
	LAYER* target,
	int32 new_width,
	int32 new_height
)
{
	// CAIROを作成し直すのでローカルで一度アドレスを受ける
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	uint8 *pixels;
	// CAIROに設定するフォーマット情報
	cairo_format_t format = (target->channel == 4) ?
		CAIRO_FORMAT_ARGB32 : (target->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;

	// ピクセルデータのメモリ領域を確保
	pixels = (uint8*)MEM_ALLOC_FUNC(
		sizeof(*pixels)*new_width*new_height*target->channel);
	// 0初期化
	(void)memset(pixels, 0, sizeof(*pixels)*new_width*new_height*target->channel);

	// CAIROを作成
	surface_p = cairo_image_surface_create_for_data(pixels,
		format, new_width, new_height, new_width * target->channel);
	cairo_p = cairo_create(surface_p);

	// レイヤーの種類で処理切り替え
	switch(target->layer_type)
	{
	case TYPE_NORMAL_LAYER:	// 通常レイヤー
		// ピクセルデータを写す
		cairo_set_source_surface(cairo_p, target->surface_p, 0, 0);
		cairo_paint(cairo_p);

		// 変更が終わったのでピクセルデータとCAIROの情報を更新
		(void)MEM_FREE_FUNC(target->pixels);
		target->pixels = pixels;
		cairo_surface_destroy(target->surface_p);
		target->surface_p = surface_p;
		cairo_destroy(target->cairo_p);
		target->cairo_p = cairo_p;

		break;
	case TYPE_VECTOR_LAYER:	// ベクトルレイヤー
		{
			// 座標を修正するベクトルデータ
			VECTOR_LINE *line = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next;
			// 一番下のラインはメモリ再確保
			VECTOR_LINE_LAYER *line_layer = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.layer;

			// 先にレイヤーのピクセルデータとCAIRO情報を更新
			(void)MEM_FREE_FUNC(target->pixels);
			target->pixels = pixels;
			cairo_surface_destroy(target->surface_p);
			target->surface_p = surface_p;
			cairo_destroy(target->cairo_p);
			target->cairo_p = cairo_p;

			// 一番下のレイヤーのラインレイヤーを作成し直す
			line_layer->width = new_width, line_layer->height = new_height;
			line_layer->stride = new_width * target->channel;
			line_layer->pixels = (uint8*)MEM_REALLOC_FUNC(
				line_layer->pixels, new_width*new_height*target->channel);
			(void)memset(line_layer->pixels, 0,
				new_width*new_height*target->channel);
			cairo_surface_destroy(line_layer->surface_p);
			cairo_destroy(line_layer->cairo_p);
			line_layer->surface_p = cairo_image_surface_create_for_data(
				line_layer->pixels, format, new_width, new_height, new_width*target->channel);
			line_layer->cairo_p = cairo_create(line_layer->surface_p);

			// ラインを合成したレイヤーも作り直し
			line_layer = target->layer_data.vector_layer_p->mix;
			line_layer->width = new_width, line_layer->height = new_height;
			line_layer->stride = new_width * target->channel;
			line_layer->pixels = (uint8*)MEM_REALLOC_FUNC(
				line_layer->pixels, new_width*new_height*target->channel);
			(void)memset(line_layer->pixels, 0,
				new_width*new_height*target->channel);
			cairo_surface_destroy(line_layer->surface_p);
			cairo_destroy(line_layer->cairo_p);
			line_layer->surface_p = cairo_image_surface_create_for_data(
				line_layer->pixels, format, new_width, new_height, new_width*target->channel);
			line_layer->cairo_p = cairo_create(line_layer->surface_p);

			// ラインを全てラスタライズ
			ChangeActiveLayer(target->window, target);
			target->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ALL;
			RasterizeVectorLayer(target->window, target, target->layer_data.vector_layer_p);
		}
		break;
	case TYPE_TEXT_LAYER:	// テキストレイヤー
		{
			// 先にレイヤーのピクセルデータとCAIRO情報を更新
			(void)MEM_FREE_FUNC(target->pixels);
			target->pixels = pixels;
			cairo_surface_destroy(target->surface_p);
			target->surface_p = surface_p;
			cairo_destroy(target->cairo_p);
			target->cairo_p = cairo_p;

			// 文字情報をレンダリング
			RenderTextLayer(target->window, target, target->layer_data.text_layer_p);
		}
		break;
	case TYPE_LAYER_SET:
		{
		}
		break;
	}	// レイヤーの種類で処理切り替え
		// switch(target->layer_type)

	// レイヤーの幅、高さ、1行分のバイト数を設定
	target->width = new_width;
	target->height = new_height;
	target->stride = new_width * target->channel;
}

/***********************************
* ADD_NEW_LAYER_HISTORY_DATA構造体 *
* 新しいレイヤー追加時の履歴データ *
***********************************/
typedef struct _ADD_NEW_LAYER_HISTORY_DATA
{
	uint16 layer_type;
	uint16 name_length;
	int32 x, y;
	int32 width, height;
	uint8 channel;
	uint16 prev_name_length;
	char* layer_name, *prev_name;
} ADD_NEW_LAYER_HISTORY_DATA;

static void AddNewLayerUndo(DRAW_WINDOW* window, void* p)
{
	ADD_NEW_LAYER_HISTORY_DATA data;
	uint8* buff = (uint8*)p;
	LAYER* delete_layer;

	(void)memcpy(&data, buff, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	buff += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	data.layer_name = (char*)buff;

	delete_layer = SearchLayer(window->layer, data.layer_name);
	if(delete_layer == window->active_layer)
	{
		LAYER *new_active = (delete_layer->prev == NULL)
			? delete_layer->next : delete_layer->prev;	
		ChangeActiveLayer(window, new_active);
			LayerViewSetActiveLayer(new_active, window->app->layer_window.view);
	}

	DeleteLayer(&delete_layer);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

static void AddNewLayerRedo(DRAW_WINDOW* window, void* p)
{
	ADD_NEW_LAYER_HISTORY_DATA data;
	uint8* buff = (uint8*)p;
	LAYER *prev, *next, *new_layer;

	(void)memcpy(&data, buff, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	buff += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	data.layer_name = (char*)buff;
	buff += data.name_length;
	data.prev_name = (char*)buff;

	if(*data.prev_name == '\0')
	{
		prev = NULL;
		next = window->layer;
	}
	else
	{
		prev = SearchLayer(window->layer, data.prev_name);
		next = prev->next;
	}

	new_layer = CreateLayer(
		data.x, data.y, data.width, data.height, data.channel,
		data.layer_type, prev, next, data.layer_name, window
	);

	window->num_layer++;
	LayerViewAddLayer(new_layer, window->layer,
		window->app->layer_window.view, window->num_layer);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

void AddNewLayerHistory(
	const LAYER* new_layer,
	uint16 layer_type
)
{
	ADD_NEW_LAYER_HISTORY_DATA data;
	gchar *history_name;
	uint8 *buff, *write;
	size_t data_size;

	data.layer_type = layer_type;
	data.x = new_layer->x, data.y = new_layer->y;
	data.width = new_layer->width, data.height = new_layer->height;
	data.channel = new_layer->channel;
	data.name_length = (uint16)strlen(new_layer->name) + 1;
	if(new_layer->prev == NULL)
	{
		data.prev_name_length = 1;
	}
	else
	{
		data.prev_name_length = (uint16)strlen(new_layer->prev->name) + 1;
	}
	data_size = offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name)
		+ data.name_length + data.prev_name_length;

	buff = write = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memcpy(write, &data, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	write += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	(void)memcpy(write, new_layer->name, data.name_length);
	write += data.name_length;
	if(new_layer->prev == NULL)
	{
		*write = 0;
	}
	else
	{
		(void)memcpy(write, new_layer->prev->name, data.prev_name_length);
	}

	switch(layer_type)
	{
	case TYPE_NORMAL_LAYER:
		history_name = new_layer->window->app->labels->layer_window.add_normal;
		break;
	case TYPE_VECTOR_LAYER:
		history_name = new_layer->window->app->labels->layer_window.add_vector;
		break;
	case TYPE_3D_LAYER:
		history_name = new_layer->window->app->labels->layer_window.add_3d_modeling;
		break;
	default:
		history_name = new_layer->window->app->labels->layer_window.add_layer_set;
	}

	AddHistory(
		&new_layer->window->app->draw_window[new_layer->window->app->active_window]->history,
		history_name, buff, (uint32)data_size, AddNewLayerUndo, AddNewLayerRedo
	);

	MEM_FREE_FUNC(buff);
}

/**********************************************
* ADD_NEW_LAYER_WITH_IMAGE_HISTORY_DATA構造体 *
* 画像データ付きでのレイヤー追加履歴データ    *
**********************************************/
typedef struct _ADD_NEW_LAYER_WITH_IMAGE_HISTORY_DATA
{
	ADD_NEW_LAYER_HISTORY_DATA layer_history;
	size_t image_data_size;
	uint8* image_data;
} ADD_NEW_LAYER_WITH_IMAGE_HISTORY_DATA;

/*******************************************
* AddNewLayerWithImageRedo関数             *
* 画像データ付きでのレイヤー追加のやり直し *
* 引数                                     *
* window	: 描画領域の情報               *
* p			: 履歴データのアドレス         *
*******************************************/
static void AddNewLayerWithImageRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_NEW_LAYER_HISTORY_DATA data;
	// 履歴データをバイト単位で扱うためにキャスト
	uint8* buff = (uint8*)p;
	// 前後のレイヤーと追加するレイヤーへのポインタ
	LAYER *prev, *next, *new_layer;
	// 画像データ展開用のストリーム
	MEMORY_STREAM stream;
	// 画像データのピクセルデータ
	uint8* pixels;
	// 画像データの情報
	int32 width, height, stride;
	// for文用のカウンタ
	int i;

	// レイヤーの情報を読み込み
	(void)memcpy(&data, buff, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	buff += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	data.layer_name = (char*)buff;
	buff += data.name_length;
	data.prev_name = (char*)buff;

	// 前のレイヤーを探す
	if(*data.prev_name == '\0')
	{
		prev = NULL;
		next = window->layer;
	}
	else
	{
		prev = SearchLayer(window->layer, data.prev_name);
		next = prev->next;
	}

	// レイヤー追加
	new_layer = CreateLayer(
		data.x, data.y, data.width, data.height, data.channel,
		data.layer_type, prev, next, data.layer_name, window
	);

	// 画像データを展開
	buff += data.prev_name_length;
	(void)memcpy(&stream.data_size, buff, sizeof(stream.data_size));
	buff += sizeof(stream.data_point);
	stream.buff_ptr = buff;
	stream.data_point = 0;
	stream.block_size = 1;
	pixels = ReadPNGStream(
		(void*)&stream, (stream_func_t)MemRead, &width, &height, &stride
	);

	// 追加したレイヤーに画像データをコピー
	for(i=0; i<height; i++)
	{
		(void)memcpy(&new_layer->pixels[i*new_layer->stride],
			&pixels[i*stride], stride);
	}

	window->num_layer++;
	LayerViewAddLayer(new_layer, window->layer,
		window->app->layer_window.view, window->num_layer);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	MEM_FREE_FUNC(pixels);
}

/*********************************************
* AddNewLayerWithImageHistory関数            *
* 画像データ付きでのレイヤー追加の履歴追加   *
* 引数                                       *
* new_layer		: 追加したレイヤー           *
* pixels		: 画像データのピクセルデータ *
* width			: 画像データの幅             *
* height		: 画像データの高さ           *
* stride		: 画像データ一行分のバイト数 *
* channel		: 画像データのチャンネル数   *
* history_name	: 履歴の名前                 *
*********************************************/
void AddNewLayerWithImageHistory(
	const LAYER* new_layer,
	uint8* pixels,
	int32 width,
	int32 height,
	int32 stride,
	uint8 channel,
	const char* history_name
)
{
// データの圧縮レベル
#define DATA_COMPRESSION_LEVEL 8
	// 履歴データ
	ADD_NEW_LAYER_WITH_IMAGE_HISTORY_DATA data;
	// データ圧縮用ストリーム
	MEMORY_STREAM_PTR image;
	// 履歴データのバッファと書き込み先のアドレス
	uint8 *buff, *write;
	// 履歴データのバイト数
	size_t data_size;

	// レイヤーのデータをセット
	data.layer_history.layer_type = TYPE_NORMAL_LAYER;
	data.layer_history.x = new_layer->x, data.layer_history.y = new_layer->y;
	data.layer_history.width = new_layer->width, data.layer_history.height = new_layer->height;
	data.layer_history.channel = new_layer->channel;
	data.layer_history.name_length = (uint16)strlen(new_layer->name) + 1;

	// ピクセルデータを圧縮
	image = CreateMemoryStream(height*stride*2);
	WritePNGStream(image, (stream_func_t)MemWrite, NULL,
		pixels, width, height, stride, channel, 0, DATA_COMPRESSION_LEVEL);

	// 前のレイヤーの名前の長さをセット
	if(new_layer->prev == NULL)
	{
		data.layer_history.prev_name_length = 1;
	}
	else
	{
		data.layer_history.prev_name_length = (uint16)strlen(new_layer->prev->name) + 1;
	}

	// 履歴データのバイト数を計算
	data_size = offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name)
		+ data.layer_history.name_length + data.layer_history.prev_name_length
			+ sizeof(image->data_point)	+ image->data_point;

	// 履歴データ作成
	buff = write = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memcpy(write, &data, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	write += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	(void)memcpy(write, new_layer->name, data.layer_history.name_length);
	write += data.layer_history.name_length;
	if(new_layer->prev == NULL)
	{
		*write = 0;
	}
	else
	{
		(void)memcpy(write, new_layer->prev->name, data.layer_history.prev_name_length);
	}
	write += data.layer_history.prev_name_length;
	(void)memcpy(write, &image->data_point, sizeof(image->data_point));
	write += sizeof(image->data_point);
	(void)memcpy(write, image->buff_ptr, image->data_point);

	// アンドゥ、リドゥ用の関数ポインタをセットして履歴データ追加
	AddHistory(
		&new_layer->window->history,
		history_name,
		buff, (uint32)data_size, AddNewLayerUndo, AddNewLayerWithImageRedo
	);

	// メモリ開放
	DeleteMemoryStream(image);
	MEM_FREE_FUNC(buff);
}

int CorrectLayerName(const LAYER* bottom_layer, const char* name)
{
	const LAYER *layer = bottom_layer;

	do
	{
		if(strcmp(layer->name, name) == 0)
		{
			return 0;
		}
		layer = layer->next;
	} while(layer != NULL);

	return 1;
}

LAYER* SearchLayer(LAYER* bottom_layer, const gchar* name)
{
	LAYER* layer = bottom_layer;

	while(layer != NULL)
	{
		if(strcmp(layer->name, name) == 0)
		{
			return layer;
		}
		layer = layer->next;
	}

	return NULL;
}

int32 GetLayerID(const LAYER* bottom, const LAYER* prev, uint16 num_layer)
{
	const LAYER *layer = bottom;
	int32 id = 1;

	if(prev == NULL)
	{
		return num_layer - 1;
	}

	while(layer != prev)
	{
		layer = layer->next;
		id++;
	}

	return num_layer - id - 1;
}

typedef struct _LAYER_NAME_CHANGE_HISTORY
{
	uint16 before_length, after_length;
	gchar *before_name, *after_name;
} LAYER_NAME_CHANGE_HISTORY;

static void LayerNameChangeUndo(DRAW_WINDOW* window, void* data)
{
	LAYER_NAME_CHANGE_HISTORY history;
	uint8 *p = (uint8*)data;
	LAYER* layer;
	GList* label;

	(void)memcpy(&history.before_length, p, sizeof(history.before_length));
	p += sizeof(history.before_length);
	(void)memcpy(&history.after_length, p, sizeof(history.before_length));
	p += sizeof(history.before_length);
	history.before_name = (gchar*)p;
	p += history.before_length;
	history.after_name = (gchar*)p;

	layer = SearchLayer(window->layer, history.after_name);

	MEM_FREE_FUNC(layer->name);
	layer->name = MEM_STRDUP_FUNC(history.before_name);

	label = gtk_container_get_children(GTK_CONTAINER(layer->widget->name));

	gtk_label_set_text(GTK_LABEL(label->data), layer->name);

	g_list_free(label);
}

static void LayerNameChangeRedo(DRAW_WINDOW* window, void* data)
{
	LAYER_NAME_CHANGE_HISTORY history;
	uint8 *p = (uint8*)data;
	LAYER* layer;
	GList* label;

	(void)memcpy(&history.before_length, p, sizeof(history.before_length));
	p += sizeof(history.before_length);
	(void)memcpy(&history.after_length, p, sizeof(history.before_length));
	p += sizeof(history.before_length);
	history.before_name = (gchar*)p;
	p += history.before_length;
	history.after_name = (gchar*)p;

	layer = SearchLayer(window->layer, history.before_name);

	MEM_FREE_FUNC(layer->name);
	layer->name = MEM_STRDUP_FUNC(history.after_name);

	label = gtk_container_get_children(GTK_CONTAINER(layer->widget->name));

	gtk_label_set_text(GTK_LABEL(label->data), layer->name);
}

void AddLayerNameChangeHistory(
	struct _APPLICATION* app,
	const gchar* before_name,
	const gchar* after_name
)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	int32 data_size;
	uint16 before_len = (uint16)strlen(before_name)+1,
		after_len = (uint16)strlen(after_name)+1;
	uint8* data, *p;
	
	data_size = sizeof(before_len)*2 + before_len + after_len;

	p = data = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memcpy(p, &before_len, sizeof(before_len));
	p += sizeof(before_len);
	(void)memcpy(p, &after_len, sizeof(after_len));
	p += sizeof(after_len);
	(void)memcpy(p, before_name, before_len);
	p += before_len;
	(void)memcpy(p, after_name, after_len);

	AddHistory(
		&window->history,
		app->labels->layer_window.rename,
		data,
		data_size,
		LayerNameChangeUndo,
		LayerNameChangeRedo
	);

	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		AddHistory(
			&app->draw_window[app->active_window]->history,
			app->labels->layer_window.rename,
			data,
			data_size,
			LayerNameChangeUndo,
			LayerNameChangeRedo
		);
	}

	MEM_FREE_FUNC(data);
}

void ChangeActiveLayer(DRAW_WINDOW* window, LAYER* layer)
{
	if((window->app->current_tool != layer->layer_type || layer->layer_type == TYPE_TEXT_LAYER)
		&& window->transform == NULL)
	{
		// メニューを有効にするフラグ
		gboolean enable_menu = TRUE;
		// for文用のカウンタ
		int i;

		if(layer->layer_set != NULL)
		{
			window->active_layer_set = layer->layer_set;
		}

		// 現在のツールを変更
		window->app->current_tool = layer->layer_type;

		gtk_widget_destroy(layer->window->app->tool_window.brush_table);
		switch(layer->layer_type)
		{
		case TYPE_NORMAL_LAYER:
			CreateBrushTable(layer->window->app, &layer->window->app->tool_window, window->app->tool_window.brushes);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layer->window->app->tool_window.active_brush[window->app->input]->button), TRUE);

			if((window->app->tool_window.flags & TOOL_USING_BRUSH) != 0)
			{
				gtk_widget_destroy(layer->window->app->tool_window.detail_ui);
				layer->window->app->tool_window.detail_ui = layer->window->app->tool_window.active_brush[window->app->input]->create_detail_ui(
					layer->window->app, layer->window->app->tool_window.active_brush[window->app->input]);
				gtk_scrolled_window_add_with_viewport(
					GTK_SCROLLED_WINDOW(layer->window->app->tool_window.detail_ui_scroll), layer->window->app->tool_window.detail_ui);
			}

			enable_menu = FALSE;
			break;
		case TYPE_VECTOR_LAYER:
			CreateVectorBrushTable(layer->window->app, &layer->window->app->tool_window, window->app->tool_window.vector_brushes);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layer->window->app->tool_window.active_vector_brush[window->app->input]->button), TRUE);

			if((window->app->tool_window.flags & TOOL_USING_BRUSH) != 0)
			{
				gtk_widget_destroy(layer->window->app->tool_window.detail_ui);
				layer->window->app->tool_window.detail_ui = layer->window->app->tool_window.active_vector_brush[window->app->input]->create_detail_ui(
					layer->window->app, layer->window->app->tool_window.active_vector_brush[window->app->input]->brush_data);
				gtk_scrolled_window_add_with_viewport(
					GTK_SCROLLED_WINDOW(layer->window->app->tool_window.detail_ui_scroll), layer->window->app->tool_window.detail_ui);
			}
			break;
		case TYPE_TEXT_LAYER:
			{
				layer->layer_data.text_layer_p->text_field = layer->window->app->tool_window.brush_table
					= gtk_text_view_new();
				(void)g_signal_connect(G_OBJECT(layer->layer_data.text_layer_p->text_field), "focus-in-event",
					G_CALLBACK(TextFieldFocusIn), layer->window->app);
				(void)g_signal_connect(G_OBJECT(layer->layer_data.text_layer_p->text_field), "focus-out-event",
					G_CALLBACK(TextFieldFocusOut), layer->window->app);
				(void)g_signal_connect(G_OBJECT(layer->layer_data.text_layer_p->text_field), "destroy",
					G_CALLBACK(OnDestroyTextField), layer->window->app);
				layer->layer_data.text_layer_p->buffer =
					gtk_text_view_get_buffer(GTK_TEXT_VIEW(layer->layer_data.text_layer_p->text_field));
				if(layer->layer_data.text_layer_p->text != NULL)
				{
					gtk_text_buffer_set_text(layer->layer_data.text_layer_p->buffer, layer->layer_data.text_layer_p->text, -1);
				}
				(void)g_signal_connect(G_OBJECT(layer->layer_data.text_layer_p->buffer), "changed",
					G_CALLBACK(OnChangeTextCallBack), layer);
				gtk_widget_destroy(window->app->tool_window.detail_ui);
				layer->window->app->tool_window.detail_ui = CreateTextLayerDetailUI(
					window->app, layer, layer->layer_data.text_layer_p);
				gtk_scrolled_window_add_with_viewport(
					GTK_SCROLLED_WINDOW(layer->window->app->tool_window.detail_ui_scroll),
					layer->window->app->tool_window.detail_ui
				);
			}
			break;
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
		case TYPE_3D_LAYER:
			CreateChange3DLayerUI(layer->window->app, &layer->window->app->tool_window);
			break;
#endif
		}

		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(layer->window->app->tool_window.brush_scroll),
			layer->window->app->tool_window.brush_table);
		gtk_widget_show_all(layer->window->app->tool_window.brush_table);
		gtk_widget_show_all(layer->window->app->tool_window.detail_ui);

		// 通常のレイヤーならラスタライズ等のメニューは無効に
		for(i=0; i<window->app->menus.num_disable_if_normal_layer; i++)
		{
			gtk_widget_set_sensitive(window->app->menus.disable_if_normal_layer[i], enable_menu);
		}

		// 通常レイヤーならば不透明度保護の機能ON
		gtk_widget_set_sensitive(window->app->layer_window.layer_control.lock_opacity, !enable_menu);
	}

	// 一番下のレイヤーなら「下のレイヤーと結合メニュー」、「下へ移動」を機能OFF
	gtk_widget_set_sensitive(window->app->menus.merge_down_menu,(window->layer != layer));
	gtk_widget_set_sensitive(window->app->layer_window.layer_control.down, (window->layer != layer));

	// 一番上のレイヤーならなら「上へ移動」を機能OFF
	gtk_widget_set_sensitive(window->app->layer_window.layer_control.up, layer->next != NULL);

	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

	window->active_layer = layer;
	window->active_layer_set = layer->layer_set;
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

typedef struct _LAYER_MERGE_DOWN_HISTORY
{
	uint16 layer_type;				// レイヤータイプ
	uint16 layer_mode;				// レイヤーの合成モード
	int32 x, y;						// レイヤー左上の座標
	// レイヤーの幅、高さ、一行分のバイト数
	int32 width, height, stride;
	uint32 flags;					// レイヤーの表示・非表示等のフラグ
	int8 channel;					// レイヤーのチャンネル数
	int8 alpha;						// レイヤーの不透明度
	uint16 name_length;				// レイヤーの名前の長さ
	uint16 prev_name_length;		// 前のレイヤーの文字数
	size_t image_data_size;			// 画像データのバイト数
	size_t prev_image_data_size;	// 合成先のレイヤーの画像データバイト数
	gchar* name;					// レイヤーの名前
	gchar* prev_name;				// 前のレイヤーの名前
	uint8* image_data;				// 画像データ
	uint8* prev_image_data;			// 合成先の画像データ
} LAYER_MERGE_DOWN_HISTORY;

/*****************************
* LayerMeargeDownUndo関数    *
* レイヤー結合操作のアンドゥ *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void LayerMergeDownUndo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	LAYER_MERGE_DOWN_HISTORY history;
	// 履歴データ読み込み用ストリーム
	MEMORY_STREAM stream;
	// レイヤーのピクセルデータ
	uint8* pixels;
	// レイヤーの幅、高さ、一行分のバイト数
	gint32 width, height, stride;
	// 履歴データバイト数
	size_t data_size;
	// 復活させるレイヤー
	LAYER* layer;
	// 前のレイヤー
	LAYER* prev;
	// 現在のデータ参照位置
	size_t data_point;

	// 履歴データを0初期化
	(void)memset(&history, 0, sizeof(history));

	// 履歴データのバイト数を読み込む
	(void)memcpy(&data_size, p, sizeof(data_size));

	// ストリームを初期化
	stream.block_size = 1;
	stream.data_size = data_size;
	stream.buff_ptr = (uint8*)p;
	stream.data_point = sizeof(data_size);

	// レイヤー情報を読み込む
	(void)MemRead(&history, 1,
		offsetof(LAYER_MERGE_DOWN_HISTORY, name), &stream);
	// レイヤーの名前、前のレイヤーの名前をセット
	history.name = (char*)&stream.buff_ptr[stream.data_point];
	(void)MemSeek(&stream, history.name_length, SEEK_CUR);
	history.prev_name = (char*)&stream.buff_ptr[stream.data_point];
	(void)MemSeek(&stream, history.prev_name_length, SEEK_CUR);

	// 前のレイヤーのポインタをセットする
	prev = SearchLayer(window->layer, history.prev_name);

	// 復活させるレイヤー作成
	layer = CreateLayer(
		history.x, history.y, history.width, history.height, history.channel,
		history.layer_type, prev, prev->next,
		history.name, window
	);
	layer->alpha = history.alpha;
	layer->layer_mode = history.layer_mode;

	data_point = stream.data_point;
	// レイヤーのピクセルデータを読み込む
	pixels = ReadPNGStream(
		&stream,
		(size_t (*)(void*, size_t, size_t, void*))MemRead,
		&width, &height, &stride
	);

	// レイヤーのピクセルメモリにコピー
	(void)memcpy(layer->pixels, pixels, stride*height);

	MEM_FREE_FUNC(pixels);

	// PNG読み込みで最後までシークされているのを戻す
	stream.data_point = data_point + history.image_data_size;
	// 結合前のレイヤーデータを読み込んで
	pixels = ReadPNGStream(&stream, (stream_func_t)MemRead,
		&width, &height, &stride);

	// ピクセルデータをコピー
	(void)memcpy(prev->pixels, pixels, height*stride);

	MEM_FREE_FUNC(pixels);

	// 描画領域を更新
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	// レイヤーの数を更新
	window->num_layer++;

	// レイヤービューにレイヤーを追加
	LayerViewAddLayer(layer, window->layer,
		window->app->layer_window.view, window->num_layer);
}

/*****************************
* LayerMeargeDownRedo関数    *
* レイヤー結合操作のリドゥ   *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void LayerMergeDownRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	LAYER_MERGE_DOWN_HISTORY history;
	// 履歴データ読み込み用ストリーム
	MEMORY_STREAM stream;
	// 結合するターゲット
	LAYER* target;
	// 履歴データのバイト数
	size_t data_size;

	// 履歴データのバイト数を読み込む
	(void)memcpy(&data_size, p, sizeof(data_size));

	// ストリームを初期化
	stream.block_size = 1;
	stream.data_size = data_size;
	stream.buff_ptr = (uint8*)p;
	stream.data_point = sizeof(data_size);

	// レイヤー情報を読み込む
	(void)MemRead(&history, 1,
		offsetof(LAYER_MERGE_DOWN_HISTORY, name), &stream);
	// レイヤーの名前、前のレイヤーの名前をセット
	history.name = (char*)&stream.buff_ptr[stream.data_point];

	// 結合するターゲットをサーチ
	target = SearchLayer(window->layer, history.name);

	// 結合を実行
	LayerMergeDown(target);
}

/*******************************************
* AddLayerMergeDownHistory関数             *
* 下のレイヤーと結合の履歴データを追加する *
* 引数                                     *
* window	: 描画領域の情報               *
* target	: 結合するレイヤー             *
*******************************************/
void AddLayerMergeDownHistory(
	DRAW_WINDOW* window,
	LAYER* target
)
{
// データの圧縮レベル
#ifndef DATA_COMPRESSION_LEVEL
#define DATA_COMPRESSION_LEVEL Z_DEFAULT_COMPRESSION
#endif
	// 履歴データ
	LAYER_MERGE_DOWN_HISTORY history;
	// 履歴データを作成するためのストリーム
	MEMORY_STREAM_PTR history_data =
		CreateMemoryStream(target->stride*target->height*4);
	// 履歴データのバイト数
	size_t data_size;

	(void)MemSeek(history_data, sizeof(data_size), SEEK_SET);

	// レイヤーのデータを履歴にセット
	history.layer_type = target->layer_type;
	history.layer_mode = target->layer_mode;
	history.x = target->x;
	history.y = target->y;
	history.width = target->width;
	history.height = target->height;
	history.stride = target->stride;
	history.flags = target->flags;
	history.channel = target->channel;
	history.alpha = target->alpha;
	history.name_length = (uint16)strlen(target->name) + 1;
	history.prev_name_length = (uint16)strlen(target->prev->name) + 1;

	// 先にレイヤーの名前とピクセルデータを登録する
	(void)MemSeek(
		history_data,
		offsetof(LAYER_MERGE_DOWN_HISTORY, name),
		SEEK_CUR
	);
	(void)MemWrite(target->name, 1, history.name_length, history_data);
	(void)MemWrite(target->prev->name, 1, history.prev_name_length, history_data);

	data_size = history_data->data_point;
	WritePNGStream(history_data, (stream_func_t)MemWrite, NULL, target->pixels,
		target->width, target->height, target->stride, target->channel, 0, DATA_COMPRESSION_LEVEL);
	history.image_data_size = history_data->data_point - data_size;

	data_size = history_data->data_point;
	WritePNGStream(history_data, (stream_func_t)MemWrite, NULL, target->prev->pixels,
		target->prev->width, target->prev->height, target->prev->stride, target->prev->channel,
		0, DATA_COMPRESSION_LEVEL);
	history.prev_image_data_size = history_data->data_point - data_size;
	
	data_size = offsetof(LAYER_MERGE_DOWN_HISTORY, name) + history.name_length + history.prev_name_length
		+ history.image_data_size + history.prev_image_data_size;

	(void)MemSeek(history_data, 0, SEEK_SET);
	(void)MemWrite(&data_size, sizeof(data_size), 1, history_data);
	(void)MemWrite(&history, 1, offsetof(LAYER_MERGE_DOWN_HISTORY, name), history_data);

	AddHistory(&window->history, window->app->labels->menu.merge_down_layer, history_data->buff_ptr,
		(uint32)data_size, LayerMergeDownUndo, LayerMergeDownRedo);

	DeleteMemoryStream(history_data);
}

/*******************************
* LayerMergeDown関数           *
* 下のレイヤーと結合する       *
* 引数                         *
* target	: 結合するレイヤー *
*******************************/
void LayerMergeDown(LAYER* target)
{
	if(target->layer_type == TYPE_NORMAL_LAYER)
	{
		switch(target->layer_mode)
		{
		case LAYER_BLEND_OVERLAY:
			{
				DRAW_WINDOW *window = target->window;
				// 見えたままに合成するために一度合成し直す
				LAYER *temp_layer = CreateLayer(0, 0, target->width, target->height, target->channel,
					TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
				LAYER *new_prev = CreateLayer(0, 0, target->width, target->height, target->channel,
					TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
				LAYER *blend = window->layer;
				FLOAT_T rc, ra, dc, da, sa;
				uint8 alpha;
				int i;

				// 結合するレイヤーまで合成
				(void)memcpy(new_prev->pixels, window->back_ground, window->pixel_buf_size);
				(void)memcpy(temp_layer->pixels, window->back_ground, window->pixel_buf_size);
				while(blend != target)
				{
					window->layer_blend_functions[blend->layer_mode](blend, temp_layer);

					blend = blend->next;

					if(blend == target->prev)
					{
						(void)memcpy(new_prev->pixels, temp_layer->pixels, window->pixel_buf_size);
						for(i=0; i<target->width * target->height; i++)
						{
							new_prev->pixels[i*4+3] = blend->pixels[i*4+3];
						}
					}
				}
				window->layer_blend_functions[blend->layer_mode](blend, temp_layer);

				(void)memcpy(window->temp_layer->pixels, target->prev->pixels, window->pixel_buf_size);
				window->layer_blend_functions[LAYER_BLEND_NORMAL](target, window->temp_layer);

				for(i=0; i<target->width * target->height; i++)
				{
					if(window->temp_layer->pixels[i*4+3] > 0)
					{
						alpha = window->temp_layer->pixels[i*4+3];
						ra = 1;
						da = new_prev->pixels[i*4+3] * DIV_PIXEL;
						sa = alpha * DIV_PIXEL;
						dc = new_prev->pixels[i*4] * DIV_PIXEL;
						rc = temp_layer->pixels[i*4] * DIV_PIXEL;
						target->prev->pixels[i*4] = (uint8)MINIMUM(alpha, ((ra*rc - da*dc*(1-sa)) / sa) * alpha);
						dc = new_prev->pixels[i*4+1] * DIV_PIXEL;
						rc = temp_layer->pixels[i*4+1] * DIV_PIXEL;
						target->prev->pixels[i*4+1] = (uint8)MINIMUM(alpha, ((ra*rc - da*dc*(1-sa)) / sa) * alpha);
						dc = new_prev->pixels[i*4+2] * DIV_PIXEL;
						rc = temp_layer->pixels[i*4+2] * DIV_PIXEL;
						target->prev->pixels[i*4+2] = (uint8)MINIMUM(alpha, ((ra*rc - da*dc*(1-sa)) / sa) * alpha);
						target->prev->pixels[i*4+3] = window->temp_layer->pixels[i*4+3];
					}
				}

				DeleteLayer(&temp_layer);
			}

			break;
		default:
			target->window->layer_blend_functions[target->layer_mode](target, target->prev);
		}

		DeleteLayer(&target);
	}
	else if(target->layer_type == TYPE_VECTOR_LAYER)
	{
		LAYER *prev = target->prev;

		if(target->prev->layer_type == TYPE_VECTOR_LAYER)
		{
			prev->layer_data.vector_layer_p->num_lines += target->layer_data.vector_layer_p->num_lines;
			if(((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next != NULL)
			{
				prev->layer_data.vector_layer_p->top_data->line.base_data.next
					= ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next;
				((VECTOR_LINE*)((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next)->base_data.prev
					= prev->layer_data.vector_layer_p->top_data;
				prev->layer_data.vector_layer_p->top_data = target->layer_data.vector_layer_p->top_data;
			}

			MEM_FREE_FUNC(target->layer_data.vector_layer_p);

			prev->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ALL;

			RasterizeVectorLayer(target->window, prev, prev->layer_data.vector_layer_p);
		}

		target->layer_type = TYPE_NORMAL_LAYER;

		DeleteLayer(&target);
	}
}

void ChangeLayerOrder(LAYER* change_layer, LAYER* new_prev, LAYER** bottom)
{
	if(change_layer->prev != NULL)
	{
		change_layer->prev->next = change_layer->next;
	}
	if(change_layer->next != NULL)
	{
		change_layer->next->prev = change_layer->prev;
	}

	if(new_prev != NULL)
	{
		if(new_prev->next != NULL)
		{
			new_prev->next->prev = change_layer;
		}
		change_layer->next = new_prev->next;
		new_prev->next = change_layer;
		
		if(change_layer == *bottom)
		{
			*bottom = new_prev;
		}
	}
	else
	{
		(*bottom)->prev = change_layer;
		change_layer->next = *bottom;
		*bottom = change_layer;
	}

	change_layer->prev = new_prev;

	// 局所キャンバスモードなら親キャンバスの順序も変更
	if((change_layer->window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		LAYER *parent_new_prev = NULL;
		if(new_prev != NULL)
		{
			parent_new_prev = SearchLayer(change_layer->window->focal_window->layer, new_prev->name);
		}
		ChangeLayerOrder(SearchLayer(change_layer->window->focal_window->layer, change_layer->name),
			parent_new_prev, &change_layer->window->focal_window->layer);
	}
}

typedef struct _CHANGE_LAYER_ORDER_HISTORY
{
	uint16 layer_name_length;
	uint16 before_prev_name_length, after_prev_name_length;
	uint16 before_parent_name_length, after_parent_name_length;
	gchar *layer_name, *before_prev_name, *after_prev_name,
		*before_parent_name, *after_parent_name;
} CHANGE_LAYER_ORDER_HISTORY;

static void ChangeLayerOrderUndo(DRAW_WINDOW* window, void *data)
{
	CHANGE_LAYER_ORDER_HISTORY history;
	LAYER *new_prev = NULL, *change_layer;
	LAYER *parent;
	LAYER *target = NULL;
	LAYER *next_target;
	uint8 *buff = (uint8*)data;

	(void)memcpy(&history, buff, offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name));
	buff += offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name);
	history.layer_name = (gchar*)buff;
	buff += history.layer_name_length;
	history.before_prev_name = (gchar*)buff;
	buff += history.before_prev_name_length + history.after_prev_name_length
		+ history.before_parent_name_length;
	history.before_parent_name = (gchar*)buff;

	change_layer = SearchLayer(window->layer, history.layer_name);
	if(*history.before_prev_name != '\0')
	{
		new_prev = SearchLayer(window->layer, history.before_prev_name);
	}

	if(*history.before_parent_name != '\0')
	{
		int hierarchy = 1;;
		parent = SearchLayer(window->layer, history.before_parent_name);
		change_layer->layer_set = parent;
		while(parent->layer_set != NULL)
		{
			hierarchy++;
			parent = parent->layer_set;
		}
	}

	if(change_layer->layer_type == TYPE_LAYER_SET)
	{
		target =  change_layer->prev;
	}
	ChangeLayerOrder(change_layer, new_prev, &window->layer);
	while(target != NULL && target->layer_set == change_layer)
	{
		next_target = target->prev;
		ChangeLayerOrder(target, new_prev, &window->layer);
		target = next_target;
	}

	ClearLayerView(&window->app->layer_window);
	LayerViewSetDrawWindow(&window->app->layer_window, window);
}

static void ChangeLayerOrderRedo(DRAW_WINDOW* window, void *data)
{
	CHANGE_LAYER_ORDER_HISTORY history;
	LAYER *new_prev = NULL, *change_layer;
	LAYER *parent;
	LAYER *target = NULL;
	LAYER *next_target;
	uint8 *buff = (uint8*)data;

	(void)memcpy(&history, buff, offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name));
	buff += offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name);
	history.layer_name = (gchar*)buff;
	buff += history.layer_name_length;
	buff += history.before_prev_name_length;
	history.after_prev_name = (gchar*)buff;
	buff += history.after_prev_name_length + history.before_parent_name_length;
	history.after_parent_name = (gchar*)buff;

	change_layer = SearchLayer(window->layer, history.layer_name);
	if(*history.after_prev_name != '\0')
	{
		new_prev = SearchLayer(window->layer, history.after_prev_name);
	}

	if(*history.after_parent_name != '\0')
	{
		int hierarchy = 1;
		parent = SearchLayer(window->layer, history.after_parent_name);
		change_layer->layer_set = parent;
		while(parent->layer_set != NULL)
		{
			hierarchy++;
			parent = parent->layer_set;
		}
	}

	if(change_layer->layer_type == TYPE_LAYER_SET)
	{
		target = change_layer->prev;
	}
	ChangeLayerOrder(change_layer, new_prev, &window->layer);
	while(target != NULL && target->layer_set == change_layer)
	{
		next_target = target->prev;
		ChangeLayerOrder(target, new_prev, &window->layer);
		target = next_target;
	}

	ClearLayerView(&window->app->layer_window);
	LayerViewSetDrawWindow(&window->app->layer_window, window);
}

/*************************************************
* AddChangeLayerOrderHistory関数                 *
* レイヤーの順序変更の履歴データを追加           *
* 引数                                           *
* change_layer	: 順序変更したレイヤー           *
* before_prev	: 順序変更前の前のレイヤー       *
* after_prev	: 順序変更後の前のレイヤー       *
* before_parent	: 順序変更前の所属レイヤーセット *
*************************************************/
void AddChangeLayerOrderHistory(
	const LAYER* change_layer,
	const LAYER* before_prev,
	const LAYER* after_prev,
	const LAYER* before_parent
)
{
	CHANGE_LAYER_ORDER_HISTORY history;
	size_t data_size = 0;
	uint8 *buff, *data;
	data_size += history.layer_name_length = (uint16)strlen(change_layer->name) + 1;
	if(before_prev != NULL)
	{
		history.before_prev_name_length = (uint16)strlen(before_prev->name) + 1;
	}
	else
	{
		history.before_prev_name_length = 1;
	}
	data_size += history.before_prev_name_length;
	if(after_prev != NULL)
	{
		history.after_prev_name_length = (uint16)strlen(after_prev->name) + 1;
	}
	else
	{
		history.after_prev_name_length = 1;
	}
	data_size += history.after_prev_name_length;
	if(before_parent != NULL)
	{
		history.before_parent_name_length = (uint16)strlen(before_parent->name) + 1;
	}
	else
	{
		history.before_parent_name_length = 1;
	}
	data_size += history.before_parent_name_length;
	if(change_layer->layer_set != NULL)
	{
		history.after_parent_name_length = (uint16)strlen(change_layer->layer_set->name) + 1;
	}
	else
	{
		history.after_parent_name_length = 1;
	}
	data_size += history.after_parent_name_length;
	data_size += offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name);

	data = buff = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memset(buff, 0, data_size);
	(void)memcpy(buff, &history, offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name));
	buff += offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name);
	(void)strcpy(buff, change_layer->name);
	buff += history.layer_name_length;
	if(before_prev != NULL)
	{
		(void)strcpy(buff, before_prev->name);
	}
	buff += history.before_prev_name_length;
	if(after_prev != NULL)
	{
		(void)strcpy(buff, after_prev->name);
	}
	buff += history.after_prev_name_length;
	if(before_parent != NULL)
	{
		(void)strcpy(buff, before_parent->name);
	}

	AddHistory(
		&change_layer->window->app->draw_window[
			change_layer->window->app->active_window]->history,
		change_layer->window->app->labels->layer_window.reorder,
		data,
		(uint32)data_size,
		ChangeLayerOrderUndo,
		ChangeLayerOrderRedo
	);

	MEM_FREE_FUNC(data);

	// 局所キャンバスモードなら親キャンバスの履歴を作成
	if((change_layer->window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		LAYER *parent_before_prev = NULL;
		LAYER *parent_after_prev = NULL;
		LAYER *parent_before_parent = NULL;
		if(before_prev != NULL)
		{
			parent_before_prev = SearchLayer(change_layer->window->focal_window->layer, before_prev->name);
		}
		if(after_prev != NULL)
		{
			parent_after_prev = SearchLayer(change_layer->window->focal_window->layer, after_prev->name);
		}
		if(before_parent != NULL)
		{
			parent_before_parent = SearchLayer(change_layer->window->focal_window->layer, before_parent->name);
		}
		AddChangeLayerOrderHistory(SearchLayer(change_layer->window->focal_window->layer, change_layer->name),
			parent_before_prev, parent_after_prev, parent_before_parent);
	}
}

void FillLayerColor(LAYER* target, uint8 color[3])
{
	if(target->layer_type != TYPE_NORMAL_LAYER)
	{	// 通常レイヤー以外なら終了
		return;
	}

	cairo_set_source_rgb(target->cairo_p, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL);
	cairo_rectangle(target->cairo_p, 0, 0, target->width, target->height);
	if((target->window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{
		if((target->flags & LAYER_LOCK_OPACITY) == 0)
		{
			cairo_fill(target->cairo_p);
		}
		else
		{
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_ATOP);
			cairo_fill(target->cairo_p);
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_OVER);
		}
	}
	else
	{
		if((target->flags & LAYER_LOCK_OPACITY) == 0)
		{
			cairo_mask_surface(target->cairo_p, target->window->selection->surface_p, 0, 0);
		}
		else
		{
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_ATOP);
			cairo_mask_surface(target->cairo_p, target->window->selection->surface_p, 0, 0);
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_OVER);
		}
	}

	target->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

/*********************************************************
* FillLayerPattern関数                                   *
* レイヤーをパターンで塗り潰す                           *
* 引数                                                   *
* target	: 塗り潰しを行うレイヤー                     *
* patterns	: パターンを管理する構造体のアドレス         *
* app		: アプリケーションを管理する構造体のアドレス *
* color		: パターンがグレースケールのときに使う色     *
*********************************************************/
void FillLayerPattern(
	LAYER* target,
	PATTERNS* patterns,
	APPLICATION* app,
	uint8 color[3]
)
{
	// 描画領域の情報
	DRAW_WINDOW* window = app->draw_window[app->active_window];
	// 塗り潰しに使用するパターンのサーフェース
	cairo_surface_t* pattern_surface = CreatePatternSurface(patterns,
		app->tool_window.color_chooser->rgb,app->tool_window.color_chooser->back_rgb,  0, PATTERN_MODE_SATURATION, 1);
	// 塗り潰しパターン
	cairo_pattern_t* pattern;
	// 拡大縮小率
	gdouble zoom = 1 / (app->patterns.scale * 0.01);
	// パターンに拡大縮小率をセットするための行列
	cairo_matrix_t matrix;

	if(pattern_surface == NULL)
	{	// パターンサーフェース作成に失敗したら終了
		return;
	}

	if(target->layer_type != TYPE_NORMAL_LAYER)
	{	// 通常レイヤー以外ならば塗りつぶし中止
		goto func_end;
	}

	// サーフェースからパターン作成
	pattern = cairo_pattern_create_for_surface(pattern_surface);
	cairo_matrix_init_scale(&matrix, zoom, zoom);
	cairo_pattern_set_matrix(pattern, &matrix);
	cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

	if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{	// 選択範囲なし
		cairo_set_source(target->cairo_p, pattern);
		cairo_rectangle(target->cairo_p, 0, 0,
			target->width, target->height);

		if((target->flags & LAYER_LOCK_OPACITY) == 0)
		{
			cairo_fill(target->cairo_p);
		}
		else
		{
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_ATOP);
			cairo_fill(target->cairo_p);
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_OVER);
		}
	}
	else
	{	// 選択範囲あり
		cairo_set_source(window->temp_layer->cairo_p, pattern);
		(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
		cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_rectangle(window->temp_layer->cairo_p, 0, 0, window->width, window->height);
		cairo_fill(window->temp_layer->cairo_p);
		cairo_set_source_surface(target->cairo_p, window->temp_layer->surface_p, 0, 0);

		if((target->flags & LAYER_LOCK_OPACITY) == 0)
		{
			cairo_mask_surface(target->cairo_p, window->selection->surface_p, 0, 0);
		}
		else
		{
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_ATOP);
			cairo_mask_surface(target->cairo_p, window->selection->surface_p, 0, 0);
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_OVER);
		}
	}

func_end:
	cairo_pattern_destroy(pattern);
	cairo_surface_destroy(pattern_surface);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

void FlipLayerHorizontally(LAYER* target, LAYER* temp)
{
	int x, y;

	(void)memcpy(temp->pixels, target->pixels, target->height*target->stride);

	for(y=0; y<target->height; y++)
	{
		for(x=0; x<target->width; x++)
		{
			(void)memcpy(&target->pixels[y*target->stride+x*target->channel],
				&temp->pixels[y*target->stride+(target->width-x-1)*target->channel], target->channel);
		}
	}
}

void FlipLayerVertically(LAYER* target, LAYER* temp)
{
	int y;

	(void)memcpy(temp->pixels, target->pixels, target->height*target->stride);

	for(y=0; y<target->height; y++)
	{
		(void)memcpy(&target->pixels[y*target->stride],
			&temp->pixels[(target->height-y-1)*target->stride], target->stride);
	}
}

/***************************************
* RasterizeLayer関数                   *
* レイヤーをラスタライズ               *
* 引数                                 *
* target	: ラスタライズするレイヤー *
***************************************/
void RasterizeLayer(LAYER* target)
{
	// for文用のカウンタ
	int i;

	// 通常のレイヤーでは無効なメニューを設定
	for(i=0; i<target->window->app->menus.num_disable_if_normal_layer; i++)
	{
		gtk_widget_set_sensitive(target->window->app->menus.disable_if_normal_layer[i], FALSE);
	}

	// ベクトルデータを破棄
	if(target->layer_type == TYPE_VECTOR_LAYER)
	{
		DeleteVectorLayer(&target->layer_data.vector_layer_p);
	}
	else if(target->layer_type == TYPE_TEXT_LAYER)
	{
		DeleteTextLayer(&target->layer_data.text_layer_p);
	}

	// レイヤーのタイプ変更
	target->layer_type = TYPE_NORMAL_LAYER;

	// ラスタライズするレイヤーがアクティブレイヤーならば
		// 詳細UIを切り替え
	gtk_widget_destroy(target->window->app->tool_window.brush_table);
	CreateBrushTable(target->window->app, &target->window->app->tool_window,
		target->window->app->tool_window.brushes);
	// 共通ツールを使っていない場合は詳細ツール設定UIを作成
	if((target->window->app->tool_window.flags & TOOL_USING_BRUSH) != 0)
	{
		gtk_widget_destroy(target->window->app->tool_window.detail_ui);
		target->window->app->tool_window.detail_ui =
			target->window->app->tool_window.active_brush[target->window->app->input]->create_detail_ui(
				target->window->app, target->window->app->tool_window.active_brush[target->window->app->input]);
		// スクロールドウィンドウに入れる
		gtk_scrolled_window_add_with_viewport(
			GTK_SCROLLED_WINDOW(target->window->app->tool_window.detail_ui_scroll), target->window->app->tool_window.detail_ui);
		gtk_widget_show_all(target->window->app->tool_window.detail_ui);
	}

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(target->window->app->tool_window.brush_scroll),
		target->window->app->tool_window.brush_table);
	gtk_widget_show_all(target->window->app->tool_window.brush_table);

	(void)memset(target->window->work_layer->pixels, 0, target->window->pixel_buf_size);
}

/*****************************
* CreateLayerCopy関数        *
* レイヤーのコピーを作成する *
* 引数                       *
* src	: コピー元のレイヤー *
* 返り値                     *
*	作成したレイヤー         *
*****************************/
LAYER* CreateLayerCopy(LAYER* src)
{
	// 返り値
	LAYER* ret;
	// レイヤーの名前
	char layer_name[4096];
	// カウンタ
	int i;

	// レイヤー名を作成
	(void)sprintf(layer_name, "%s %s", src->window->app->labels->menu.copy, src->name);
	i = 1;
	while(CorrectLayerName(src->window->layer, layer_name) == 0)
	{
		(void)sprintf(layer_name, "%s %s (%d)", src->window->app->labels->menu.copy, src->name, i);
		i++;
	}

	// レイヤー作成
	ret = CreateLayer(
		src->x, src->y, src->width, src->height, src->channel, src->layer_type,
			src, src->next, layer_name, src->window);
	// レイヤーのフラグをコピー
	ret->flags = src->flags;

	// ピクセルデータをコピー
	(void)memcpy(ret->pixels, src->pixels, src->stride*src->height);

	if(src->layer_type == TYPE_VECTOR_LAYER)
	{	// ベクトルレイヤーならベクトルデータをコピー
		VECTOR_LINE *line = (VECTOR_LINE*)ret->layer_data.vector_layer_p->base,
			*src_line = (VECTOR_LINE*)((VECTOR_LINE*)src->layer_data.vector_layer_p->base)->base_data.next;
		while(src_line != NULL)
		{
			if(src_line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
			{
				line = CreateVectorLine(line, NULL);
				line->base_data.vector_type = src_line->base_data.vector_type;
				line->num_points = src_line->num_points;
				line->buff_size = src_line->buff_size;
				line->blur = src_line->blur;
				line->outline_hardness = src_line->outline_hardness;
			}
			else if(src_line->base_data.vector_type == VECTOR_TYPE_SQUARE)
			{
				VECTOR_SQUARE *square = (VECTOR_SQUARE*)CreateVectorShape(&line->base_data, NULL, VECTOR_TYPE_SQUARE);
				*square = *((VECTOR_SQUARE*)src_line);
				line = (VECTOR_LINE*)square;
			}
			else if(src_line->base_data.vector_type == VECTOR_TYPE_ECLIPSE)
			{
				VECTOR_ECLIPSE *square = (VECTOR_ECLIPSE*)CreateVectorShape(&line->base_data, NULL, VECTOR_TYPE_ECLIPSE);
				*square = *((VECTOR_ECLIPSE*)src_line);
				line = (VECTOR_LINE*)square;
			}

			line->base_data.layer = (VECTOR_LINE_LAYER*)MEM_ALLOC_FUNC(sizeof(*line->base_data.layer));
			line->base_data.layer->x = src_line->base_data.layer->x;
			line->base_data.layer->y = src_line->base_data.layer->y;
			line->base_data.layer->width = src_line->base_data.layer->width;
			line->base_data.layer->height = src_line->base_data.layer->height;
			line->base_data.layer->stride = src_line->base_data.layer->stride;
			line->base_data.layer->pixels = (uint8*)MEM_ALLOC_FUNC(line->base_data.layer->stride*line->base_data.layer->height);
			(void)memcpy(line->base_data.layer->pixels, src_line->base_data.layer->pixels,
				line->base_data.layer->stride*line->base_data.layer->height);
			line->base_data.layer->surface_p = cairo_image_surface_create_for_data(
				line->base_data.layer->pixels, CAIRO_FORMAT_ARGB32, line->base_data.layer->width,
				line->base_data.layer->height, line->base_data.layer->stride
			);
			line->base_data.layer->cairo_p = cairo_create(line->base_data.layer->surface_p);

			if(src_line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
			{
				line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*line->points)*src_line->buff_size);
				(void)memcpy(line->points, src_line->points, sizeof(*line->points)*src_line->num_points);
			}

			src_line = (VECTOR_LINE*)src_line->base_data.next;
		}

		ret->layer_data.vector_layer_p->num_lines = src->layer_data.vector_layer_p->num_lines;
		ret->layer_data.vector_layer_p->top_data = (void*)line;
		(void)memcpy(ret->layer_data.vector_layer_p->mix->pixels,
			src->layer_data.vector_layer_p->mix->pixels, src->stride*src->height);
	}
	else if(src->layer_type == TYPE_TEXT_LAYER)
	{	// テキストレイヤーならテキストデータをコピー
		ret->layer_data.text_layer_p = CreateTextLayer(
			src->window,
			src->layer_data.text_layer_p->x, src->layer_data.text_layer_p->y,
			src->layer_data.text_layer_p->width, src->layer_data.text_layer_p->height,
			src->layer_data.text_layer_p->base_size, src->layer_data.text_layer_p->font_size,
			src->layer_data.text_layer_p->font_id,
			src->layer_data.text_layer_p->color,
			src->layer_data.text_layer_p->balloon_type,
			src->layer_data.text_layer_p->back_color,
			src->layer_data.text_layer_p->line_color,
			src->layer_data.text_layer_p->line_width,
			&src->layer_data.text_layer_p->balloon_data,
			src->layer_data.text_layer_p->flags
		);
		ret->layer_data.text_layer_p->text = MEM_STRDUP_FUNC(
			src->layer_data.text_layer_p->text);
	}
	else
	{
		ret->layer_type = TYPE_NORMAL_LAYER;
	}

	return ret;
}

/*************************************************
* GetLayerChain関数                              *
* ピン留めされたレイヤー配列を取得する           *
* 引数                                           *
* window	: 描画領域の情報                     *
* num_layer	: レイヤー数を格納する変数のアドレス *
* 返り値                                         *
*	レイヤー配列                                 *
*************************************************/
LAYER** GetLayerChain(DRAW_WINDOW* window, uint16* num_layer)
{
	// 返り値
	LAYER** ret =
		(LAYER**)MEM_ALLOC_FUNC(sizeof(*ret)*LAYER_CHAIN_BUFFER_SIZE);
	// バッファサイズ
	unsigned int buffer_size = LAYER_CHAIN_BUFFER_SIZE;
	// チェックするレイヤー
	LAYER* layer = window->layer;
	// レイヤーの数
	unsigned int num = 0;

	// ピン留めされたレイヤーを探す
	while(layer != NULL)
	{	// アクティブなレイヤーかピン留めされたレイヤーなら
		if(layer == window->active_layer || (layer->flags & LAYER_CHAINED) != 0)
		{	// 配列に追加
			ret[num] = layer;
			num++;
		}

		// バッファが不足していたら再確保
		if(num >= buffer_size)
		{
			buffer_size += LAYER_CHAIN_BUFFER_SIZE;
			ret = MEM_REALLOC_FUNC(ret, buffer_size * sizeof(*ret));
		}

		layer = layer->next;
	}

	*num_layer = (uint16)num;

	return ret;
}

/***********************************************
* DeleteSelectAreaPixels関数                   *
* 選択範囲のピクセルデータを削除する           *
* 引数                                         *
* window	: 描画領域の情報                   *
* target	: ピクセルデータを削除するレイヤー *
* selection	: 選択範囲を管理するレイヤー       *
***********************************************/
void DeleteSelectAreaPixels(
	DRAW_WINDOW* window,
	LAYER* target,
	LAYER* selection
)
{
	// for文用のカウンタ
	int i;

	// 一時保存のレイヤーに選択範囲を写す
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	for(i=0; i<selection->width*selection->height; i++)
	{
		window->temp_layer->pixels[i*4+3] = selection->pixels[i];
	}

	// レイヤー合成でピクセルデータの削除を実行
	window->layer_blend_functions[LAYER_BLEND_ALPHA_MINUS](window->temp_layer, target);
}

typedef struct _DELETE_SELECT_AREA_PIXELS_HISTORY
{
	// 消去したピクセルデータの最大最小の座標
	int32 min_x, min_y, max_x, max_y;
	// レイヤー名の長さ
	uint16 layer_name_length;
	// レイヤー名
	char* layer_name;
	// ピクセルデータ
	uint8* pixels;
} DELETE_SELECT_AREA_PIXELS_HISTORY;

/*******************************************
* DeletePixelsUndoRedo関数                 *
* ピクセルデータ削除操作のリドゥ、アンドゥ *
* 引数                                     *
* window	: 描画領域の情報               *
* p			: 履歴データ                   *
*******************************************/
static void DeletePixelsUndoRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データの型にキャスト
	DELETE_SELECT_AREA_PIXELS_HISTORY data;
	// 操作が行われたレイヤー
	LAYER* target;
	// バイト単位でデータを扱うためのキャスト
	uint8* byte_data = (uint8*)p;
	// ピクセルデータを変更する幅、高さ、一行分のバイト数
	int32 width, height, stride;
	// データ差分変更用のバッファ
	uint8* before_data;
	// for文用のカウンタ
	int i;

	// 履歴の基本情報をコピー
	(void)memcpy(&data, p, offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name));

	// レイヤー名を取得
	data.layer_name = (char*)&byte_data[offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name)];

	// ピクセルデータを取得
	data.pixels = &byte_data[offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name)+data.layer_name_length];

	// レイヤーを検索
	target = SearchLayer(window->layer, data.layer_name);

	// ピクセルデータの幅、高さ、一行分のバイト数を計算
	width = data.max_x - data.min_x;
	height = data.max_y - data.min_y;
	stride = width * target->channel;

	// 現在のピクセルデータを記憶
	before_data = (uint8*)MEM_ALLOC_FUNC(height*stride);
	for(i=0; i<height; i++)
	{
		(void)memcpy(&before_data[i*stride],
			&target->pixels[(i+data.min_y)*target->stride+data.min_x*target->channel], stride);
	}

	// 履歴データからピクセルデータをコピー
	for(i=0; i<height; i++)
	{
		(void)memcpy(&target->pixels[(i+data.min_y)*target->stride+data.min_x*target->channel],
			&data.pixels[i*stride], stride);
	}

	// 記憶したデータに履歴データを変更する
	for(i=0; i<height; i++)
	{
		(void)memcpy(&data.pixels[i*stride], &before_data[i*stride], stride);
	}

	MEM_FREE_FUNC(before_data);
}

/*******************************************************
* AddDeletePixelsHistory関数                           *
* ピクセルデータ消去操作の履歴データを追加する         *
* 引数                                                 *
* tool_name	: ピクセルデータ消去に使用したツールの名前 *
* window	: 描画領域の情報                           *
* min_x		: ピクセルデータのX座標最小値              *
* min_y		: ピクセルデータのY座標最小値              *
* max_x		: ピクセルデータのX座標最大値              *
* max_y		: ピクセルデータのY座標最大値              *
*******************************************************/
void AddDeletePixelsHistory(
	const char* tool_name,
	DRAW_WINDOW* window,
	LAYER* target,
	int32 min_x,
	int32 min_y,
	int32 max_x,
	int32 max_y
)
{
	// 履歴データ作成用
	DELETE_SELECT_AREA_PIXELS_HISTORY data;
	// 実際に履歴配列に追加するデータのストリーム
	MEMORY_STREAM_PTR stream;
	// 履歴に残すピクセルデータの幅と高さと一行分のバイト数
	int32 width, height, stride;
	// 履歴データのサイズ
	size_t data_size;
	// for文用のカウンタ
	int i;

	// 履歴の基本情報をセット
	data.min_x = min_x - 1;
	data.min_y = min_y - 1;
	data.max_x = max_x + 1;
	data.max_y = max_y + 1;
	data.layer_name_length = (uint16)strlen(target->name) + 1;

	// 描画領域外へ出ていたら修正
	if(data.min_x < 0)
	{
		data.min_x = 0;
	}
	if(data.min_y < 0)
	{
		data.min_y = 0;
	}
	if(data.max_x >= window->width - 1)
	{
		data.max_x = window->width - 1;
	}
	if(data.max_y = window->height - 1)
	{
		data.max_y = window->height - 1;
	}

	// 選択範囲のデータを履歴データに残す
	width = data.max_x - data.min_x;
	height = data.max_y - data.min_y;
	stride = width * target->channel;

	// 履歴データ用のストリームを作成して書き込む
	data_size = offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name)
		+ data.layer_name_length + stride*height;
	stream = CreateMemoryStream(data_size);

	(void)MemWrite(&data, 1, offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name), stream);
	(void)MemWrite(target->name, 1, data.layer_name_length, stream);

	for(i=0; i<height; i++)
	{
		(void)MemWrite(&target->pixels[(min_y+i)*target->stride+min_x*target->channel], 1,
			stride, stream);
	}

	// 履歴データを履歴配列に追加
	AddHistory(&window->history, tool_name, stream->buff_ptr, (uint32)data_size,
		DeletePixelsUndoRedo, DeletePixelsUndoRedo);

	(void)DeleteMemoryStream(stream);
}

/*****************************************************
* CalcLayerAverageRGBA関数                           *
* レイヤーの平均ピクセルデータ値を計算               *
* 引数                                               *
* target	: 平均ピクセルデータ値を計算するレイヤー *
* 返り値                                             *
*	対象レイヤーの平均ピクセルデータ値               *
*****************************************************/
RGBA_DATA CalcLayerAverageRGBA(LAYER* target)
{
	RGBA_DATA average;
	uint64 sum_r = 0, sum_g = 0,
		sum_b = 0, sum_a = 0;
	unsigned int index;
	int i;

	for(i=0; i<target->width*target->height; i++)
	{
		index = i * 4;
		sum_r += target->pixels[index];
		sum_g += target->pixels[index+1];
		sum_b += target->pixels[index+2];
		sum_a += target->pixels[index+3];
	}

	average.r = (uint8)(sum_r / index);
	average.g = (uint8)(sum_g / index);
	average.b = (uint8)(sum_b / index);
	average.a = (uint8)(sum_a / index);

	return average;
}

/*****************************************************
* CalcLayerAverageRGBAwithAlpha関数                  *
* α値を考慮してレイヤーの平均ピクセルデータ値を計算 *
* 引数                                               *
* target	: 平均ピクセルデータ値を計算するレイヤー *
* 返り値                                             *
*	対象レイヤーの平均ピクセルデータ値               *
*****************************************************/
RGBA_DATA CalcLayerAverageRGBAwithAlpha(LAYER* target)
{
	RGBA_DATA average;
	uint64 sum_r = 0, sum_g = 0,
		sum_b = 0, sum_a = 0;
	unsigned int index;
	int i;

	for(i=0; i<target->width*target->height; i++)
	{
		index = i * 4;
		sum_r += target->pixels[index] * target->pixels[index+3];
		sum_g += target->pixels[index+1] * target->pixels[index+3];
		sum_b += target->pixels[index+2] * target->pixels[index+3];
		sum_a += target->pixels[index+3];
	}

	average.r = (uint8)(sum_r / sum_a);
	average.g = (uint8)(sum_g / sum_a);
	average.b = (uint8)(sum_b / sum_a);
	average.a = (uint8)(sum_a / index);

	return average;
}

/*******************************************************
* FillTextureLayer関数                                 *
* テクスチャレイヤーを選択されたパラメーターで塗り潰す *
* 引数                                                 *
* layer		: テクスチャレイヤー                       *
* textures	: テクスチャのパラメーター                 *
*******************************************************/
void FillTextureLayer(LAYER* layer, TEXTURES* textures)
{
	// レイヤーをリセット
	(void)memset(layer->pixels, 0, layer->width*layer->height);

	// 使用するテクスチャが指定されていたら
	if(textures->active_texture > 0)
	{
		cairo_surface_t *surface_p;
		cairo_pattern_t *pattern;
		cairo_matrix_t matrix;
		FLOAT_T zoom = 1 / textures->scale;
		FLOAT_T rate = (1 - textures->strength * 0.01);
		FLOAT_T angle = textures->angle * G_PI / 180;
		int i;
		// ピクセルデータの複製を作る
		uint8 *pixel_copy = (uint8*)MEM_ALLOC_FUNC(
			textures->texture[textures->active_texture-1].stride*textures->texture[textures->active_texture-1].height);
		(void)memcpy(pixel_copy, textures->texture[textures->active_texture-1].pixels,
			textures->texture[textures->active_texture-1].stride*textures->texture[textures->active_texture-1].height);

		for(i=0; i<textures->texture[textures->active_texture-1].stride*textures->texture[textures->active_texture-1].height; i++)
		{
			pixel_copy[i] = (uint8)(pixel_copy[i] + (0xff - pixel_copy[i]) * rate + 0.8);
		}

		// テクスチャパターンを作成
		surface_p = cairo_image_surface_create_for_data(pixel_copy, CAIRO_FORMAT_A8,
			textures->texture[textures->active_texture-1].width, textures->texture[textures->active_texture-1].height,
				textures->texture[textures->active_texture-1].stride);
		pattern = cairo_pattern_create_for_surface(surface_p);
		cairo_matrix_init_scale(&matrix, zoom ,zoom);
		cairo_matrix_rotate(&matrix, angle);
		cairo_pattern_set_matrix(pattern, &matrix);
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

		// 塗り潰しを実行
		cairo_set_operator(layer->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source(layer->cairo_p, pattern);
		cairo_rectangle(layer->cairo_p, 0, 0, layer->width, layer->height);
		cairo_fill(layer->cairo_p);

		cairo_pattern_destroy(pattern);
		cairo_surface_destroy(surface_p);
		MEM_FREE_FUNC(pixel_copy);
	}
}

/******************************************
* GetAverageColor関数                     *
* 指定座標周辺の平均色を取得              *
* 引数                                    *
* target	: 色を取得するレイヤー        *
* x			: X座標                       *
* y			: Y座標                       *
* size		: 色を取得する範囲            *
* color		: 取得した色を格納(4バイト分) *
******************************************/
void GetAverageColor(LAYER* target, int x, int y, int size, uint8 color[4])
{
	unsigned int sum_color[4] = {0, 0, 0, 1};
	uint8 *ref;
	int start_x, start_y;
	int width, height;
	int count_x, count_y;
	int count = 0;

	start_x = x - size,	start_y = y - size;
	width = height = size * 2 + 1;

	if(start_x < 0)
	{
		start_x = 0;
	}
	if(start_x + width > target->width)
	{
		width = target->width - start_x;
	}

	if(start_y < 0)
	{
		start_y = 0;
	}
	if(start_y + height > target->height)
	{
		height = target->height - start_y;
	}

	for(count_y=0; count_y<height; count_y++)
	{
		ref = &target->pixels[(start_y+count_y)*target->stride+start_x*4];
		for(count_x=0; count_x<width; count_x++, ref+=4)
		{
			sum_color[0] += ref[0] * ref[3];
			sum_color[1] += ref[1] * ref[3];
			sum_color[2] += ref[2] * ref[3];
			sum_color[3] += ref[3];
			count++;
		}
	}

	color[0] = (uint8)(sum_color[0] / sum_color[3]);
	color[1] = (uint8)(sum_color[1] / sum_color[3]);
	color[2] = (uint8)(sum_color[2] / sum_color[3]);
	color[3] = (uint8)(sum_color[3] / count);
}

/***************************************************
* GetBlendedUnderLayer関数                         *
* 対象より下のレイヤーを合成したレイヤーを取得する *
* 引数                                             *
* target			: 対象のレイヤー               *
* window			: 描画領域の情報               *
* use_back_ground	: 背景色を使用するかどうか     *
* 返り値                                           *
*	合成したレイヤー                               *
***************************************************/
LAYER* GetBlendedUnderLayer(LAYER* target, DRAW_WINDOW* window, int use_back_ground)
{
	LAYER *ret = CreateLayer(0, 0, window->width, window->height, window->channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
	LAYER *src = window->layer;

	if(use_back_ground != 0)
	{
		(void)memcpy(ret->pixels, window->back_ground, window->pixel_buf_size);
	}

	while(src != NULL && src != target)
	{
		if((src->flags & LAYER_FLAG_INVISIBLE) == 0)
		{
			window->layer_blend_functions[src->layer_mode](src, ret);
		}
		src = src->next;
	}

	return ret;
}

#ifdef __cplusplus
}
#endif
