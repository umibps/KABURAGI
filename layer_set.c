// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include "layer.h"
#include "layer_window.h"
#include "draw_window.h"
#include "application.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************
* DeleteLayerSet関数                       *
* レイヤーセットの削除を行う               *
* 引数                                     *
* layer_set	: 削除するレイヤーセット       *
* window	: レイヤーセットを持つ描画領域 *
*******************************************/
void DeleteLayerSet(LAYER* layer_set, DRAW_WINDOW* window)
{
	LAYER *layer = window->layer;
	LAYER *new_layer_set = layer_set->layer_set;
	int hierarchy = 0;

	while(new_layer_set != NULL)
	{
		hierarchy++;
	}

	while(layer != layer_set)
	{
		if(layer->layer_set == layer_set)
		{
			gtk_alignment_set_padding(GTK_ALIGNMENT(layer->widget->alignment),
				0, 0, LAYER_SET_DISPLAY_OFFSET * hierarchy, 0);
			layer->layer_set = new_layer_set;
		}

		layer = layer->next;
	}

	DeleteLayer(&layer_set->layer_data.layer_set_p->active_under);
	MEM_FREE_FUNC(layer_set->layer_data.layer_set_p);
}

typedef struct _CHANGE_LAYER_SET_HISTORY
{
	uint16 layer_name_length;
	uint16 before_parent_name_length;
	uint16 after_parent_name_length;
	char *layer_name;
	char *before_parent_name;
	char *after_parent_name;
} CHANGE_LAYER_SET_HISTORY;

static void ChangeLayerSetUndo(DRAW_WINDOW* window, void* data)
{
	CHANGE_LAYER_SET_HISTORY history;
	LAYER *change_layer;
	uint8 *buff = (uint8*)data;

	(void)memcpy(&history, buff, offsetof(CHANGE_LAYER_SET_HISTORY, layer_name));
	buff += offsetof(CHANGE_LAYER_SET_HISTORY, layer_name);
	history.layer_name = (char*)buff;
	buff += history.layer_name_length;
	history.before_parent_name = (char*)buff;

	change_layer = SearchLayer(window->layer, history.layer_name);
	if(*history.before_parent_name != '\0')
	{
		LAYER *parent = SearchLayer(window->layer, history.before_parent_name);
		int hierarchy = 1;

		change_layer->layer_set = parent;
		while(parent->layer_set != NULL)
		{
			hierarchy++;
			parent = parent->layer_set;
		}

		gtk_alignment_set_padding(GTK_ALIGNMENT(change_layer->widget->alignment),
			0, 0, LAYER_SET_DISPLAY_OFFSET * hierarchy, 0);
	}
	else
	{
		gtk_alignment_set_padding(GTK_ALIGNMENT(change_layer->widget->alignment),
			0, 0, 0, 0);
	}
}

static void ChangeLayerSetRedo(DRAW_WINDOW* window, void* data)
{
	CHANGE_LAYER_SET_HISTORY history;
	LAYER *change_layer;
	uint8 *buff = (uint8*)data;

	(void)memcpy(&history, buff, offsetof(CHANGE_LAYER_SET_HISTORY, layer_name));
	buff += offsetof(CHANGE_LAYER_SET_HISTORY, layer_name);
	history.layer_name = (char*)buff;
	buff += history.layer_name_length + history.before_parent_name_length;
	history.after_parent_name = (char*)buff;

	change_layer = SearchLayer(window->layer, history.layer_name);
	if(*history.after_parent_name != '\0')
	{
		LAYER *parent = SearchLayer(window->layer, history.after_parent_name);
		int hierarchy = 1;

		change_layer->layer_set = parent;
		while(parent->layer_set != NULL)
		{
			hierarchy++;
			parent = parent->layer_set;
		}

		gtk_alignment_set_padding(GTK_ALIGNMENT(change_layer->widget->alignment),
			0, 0, LAYER_SET_DISPLAY_OFFSET * hierarchy, 0);
	}
	else
	{
		gtk_alignment_set_padding(GTK_ALIGNMENT(change_layer->widget->alignment),
			0, 0, 0, 0);
	}
}

/***********************************************************
* AddChangeLyaerSetHistory関数                             *
* レイヤーの所属レイヤーセット変更の履歴データを追加       *
* 引数                                                     *
* change_layer	: 所属レイヤーセットを変更するレイヤー     *
* before_parent	: レイヤーセット変更前の所属レイヤーセット *
* after_parent	: レイヤーセット変更後の所属レイヤーセット *
***********************************************************/
void AddChangeLayerSetHistory(
	const LAYER* change_layer,
	const LAYER* before_parent,
	const LAYER* after_parent
)
{
	CHANGE_LAYER_SET_HISTORY history;
	size_t data_size = 0;
	uint8 *buff, *data;

	data_size += history.layer_name_length = (uint16)strlen(change_layer->name) + 1;
	if(before_parent != NULL)
	{
		history.before_parent_name_length = (uint16)strlen(before_parent->name) + 1;
	}
	else
	{
		history.before_parent_name_length = 1;
	}
	data_size += history.before_parent_name_length;
	if(after_parent != NULL)
	{
		history.after_parent_name_length = (uint16)strlen(after_parent->name) + 1;
	}
	else
	{
		history.after_parent_name_length = 1;
	}
	data_size += history.after_parent_name_length;
	data_size += offsetof(CHANGE_LAYER_SET_HISTORY, layer_name);

	data = buff = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memset(buff, 0, data_size);
	(void)memcpy(buff, &history, offsetof(CHANGE_LAYER_SET_HISTORY, layer_name));
	buff += offsetof(CHANGE_LAYER_SET_HISTORY, layer_name);
	(void)strcpy(buff, change_layer->name);
	buff += history.layer_name_length;
	if(before_parent != NULL)
	{
		(void)strcpy(buff, before_parent->name);
	}
	buff += history.before_parent_name_length;
	if(after_parent != NULL)
	{
		(void)strcpy(buff, after_parent->name);
	}

	AddHistory(&change_layer->window->history, change_layer->window->app->labels->layer_window.reorder,
		data, data_size, ChangeLayerSetUndo, ChangeLayerSetRedo);

	MEM_FREE_FUNC(data);
}

/***************************************************
* LayerSetShowChildren関数                         *
* レイヤーセットの子レイヤーを表示する             *
* 引数                                             *
* layer_set	: 子を表示するレイヤーセット           *
* prev		: 関数終了後の次にチェックするレイヤー *
***************************************************/
void LayerSetShowChildren(LAYER* layer_set, LAYER **prev)
{
	LAYER *layer = *prev;

	while(layer != NULL)
	{
		if(layer->layer_set == layer_set)
		{	// 表示ON
			gtk_widget_show_all(layer->widget->box);
			if(layer->layer_type == TYPE_LAYER_SET)
			{	// 子レイヤにレイヤーセットがあればそちらも表示
				LayerSetShowChildren(layer, &layer);
			}
			else
			{
				layer = layer->prev;
			}
		}
		else
		{	// レイヤーセット外に出たので終了
			break;
		}
	}

	*prev = layer;
}

/***************************************************
* LayerSetHideChildren関数                         *
* レイヤーセットの子レイヤーを非表示する           *
* 引数                                             *
* layer_set	: 子を表示するレイヤーセット           *
* prev		: 関数終了後の次にチェックするレイヤー *
***************************************************/
void LayerSetHideChildren(LAYER* layer_set, LAYER **prev)
{
	LAYER *layer = *prev;

	while(layer != NULL)
	{
		if(layer->layer_set == layer_set)
		{	// 表示OFF
			gtk_widget_hide(layer->widget->box);
			if(layer->layer_type == TYPE_LAYER_SET)
			{	// 子レイヤにレイヤーセットがあればそちらも表示
				LayerSetHideChildren(layer, &layer);
			}
			else
			{
				layer = layer->prev;
			}
		}
		else
		{	// レイヤーセット外に出たので終了
			break;
		}
	}

	*prev = layer;
}

static void LayerSetChildButtonCallBack(GtkWidget *button, LAYER* layer_set)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		LAYER *prev = layer_set->prev;

		layer_set->flags |= LAYER_SET_CLOSE;
		LayerSetHideChildren(layer_set, &prev);

		gtk_image_set_from_pixbuf(GTK_IMAGE(layer_set->layer_data.layer_set_p->button_image),
			layer_set->window->app->layer_window.close);
	}
	else
	{
		LAYER *prev = layer_set->prev;

		layer_set->flags &= ~(LAYER_SET_CLOSE);
		LayerSetShowChildren(layer_set, &prev);

		gtk_image_set_from_pixbuf(GTK_IMAGE(layer_set->layer_data.layer_set_p->button_image),
			layer_set->window->app->layer_window.open);
	}
}

/*****************************************************
* CreateLayerSetChildButton関数                      *
* レイヤーセットの子レイヤー表示・非表示ボタンを作成 *
* 引数                                               *
* layer_set	: ボタン作成対象のレイヤーセット         *
* 返り値                                             *
*	作成したボタンウィジェット                       *
*****************************************************/
GtkWidget* CreateLayerSetChildButton(LAYER* layer_set)
{
	layer_set->layer_data.layer_set_p->button_image = gtk_image_new();
	layer_set->layer_data.layer_set_p->show_child_button = gtk_toggle_button_new();

	gtk_image_set_from_pixbuf(GTK_IMAGE(layer_set->layer_data.layer_set_p->button_image),
		layer_set->window->app->layer_window.open);
	gtk_container_add(GTK_CONTAINER(layer_set->layer_data.layer_set_p->show_child_button),
		layer_set->layer_data.layer_set_p->button_image);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(layer_set->layer_data.layer_set_p->show_child_button),
			(layer_set->flags & LAYER_SET_CLOSE) == 0);

	g_signal_connect(G_OBJECT(layer_set->layer_data.layer_set_p->show_child_button),
		"toggled", G_CALLBACK(LayerSetChildButtonCallBack), layer_set);

	return layer_set->layer_data.layer_set_p->show_child_button;
}

/*************************************************
* MixLayerSet関数                                *
* レイヤーセット内を合成                         *
* 引数                                           *
* bottom	: レイヤーセットの一番下のレイヤー   *
* next		: 合成後に次に合成するレイヤー       *
* window	: 描画領域を管理する構造体のアドレス *
*************************************************/
void MixLayerSet(LAYER* bottom, LAYER** next, DRAW_WINDOW* window)
{
	// レイヤーセットのピクセルデータのバイト数
	size_t pixel_bytes = bottom->layer_set->stride*bottom->layer_set->height;
	// 所属レイヤーセット
	LAYER *layer_set = bottom->layer_set;
	// レイヤー合成用
	LAYER *layer = bottom;
	LAYER *blend_layer;
	int blend_mode;

	// レイヤーセットのピクセルデータをリセット
	(void)memset(layer_set->pixels, 0, pixel_bytes);

	if(layer_set == window->active_layer_set)
	{
		(void)memcpy(window->under_active->pixels, window->mixed_layer->pixels, window->pixel_buf_size);
	}

	// 所属レイヤーセットまでループ
	while(1)
	{
		blend_layer = layer;
		blend_mode = layer->layer_mode;

		// アクティブレイヤーなら
		if(blend_layer == window->active_layer)
		{
			// 現在の合成結果を記憶
			(void)memcpy(blend_layer->layer_set->layer_data.layer_set_p->active_under->pixels,
				blend_layer->layer_set->pixels, window->pixel_buf_size);

			// 可視判定
			if((blend_layer->flags & LAYER_FLAG_INVISIBLE) == 0)
			{
				if(layer->layer_type == TYPE_NORMAL_LAYER)
				{	// 通常レイヤーは
						// 作業レイヤーとアクティブレイヤーを一度合成してから下のレイヤーと合成
					(void)memcpy(window->temp_layer->pixels, layer->pixels, layer->stride*layer->height);
					window->layer_blend_functions[window->work_layer->layer_mode](window->work_layer, window->temp_layer);
					blend_layer = window->temp_layer;
					blend_layer->alpha = layer->alpha;
					blend_layer->flags = layer->flags;
					blend_layer->prev = layer->prev;
				}
				else if(layer->layer_type == TYPE_VECTOR_LAYER)
				{	// ベクトルレイヤーは
						// レイヤーのラスタライズ処理を行なってから作業レイヤーと下のレイヤーを合成
					RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
					window->layer_blend_functions[window->work_layer->layer_mode](window->work_layer, layer);
				}
				else if(layer->layer_type == TYPE_TEXT_LAYER)
				{	// テキストレイヤーは
						// テキストの内容をラスタライズ処理してから下のレイヤーと合成
					RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
				}

				while(layer->next != NULL && layer->next->layer_type == TYPE_ADJUSTMENT_LAYER)
				{
					if((layer->next->flags & LAYER_FLAG_INVISIBLE) != 0)
					{
						layer->next->layer_data.adjustment_layer_p->filter_func(
							layer->layer_data.adjustment_layer_p, layer->pixels, layer->next->pixels,
								layer->width * layer->height, layer);
					}
					layer->next->layer_data.adjustment_layer_p->update(
						layer->layer_data.adjustment_layer_p, layer, window->mixed_layer,
							0, 0, layer->width, layer->height);
					blend_layer = layer->next;
					layer = layer->next;
				}

				// 合成する対象と方法が確定したので合成を実行する
				window->layer_blend_functions[blend_mode](blend_layer, layer->layer_set);
				// 合成したらデータを元に戻す
				window->temp_layer->alpha = 100;
				window->temp_layer->flags = 0;
				window->temp_layer->prev = NULL;
				cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
			}	// 可視判定
					// if((blend_layer->flags & LAYER_FLAG_INVISIBLE) == 0)

			// サムネイル更新
			gtk_widget_queue_draw(layer->widget->thumbnail);
		}	// アクティブレイヤーなら
				// if(blend_layer == window->active_layer)
		else
		{
			// 可視判定
			if((blend_layer->flags & LAYER_FLAG_INVISIBLE) == 0)
			{
				window->layer_blend_functions[blend_mode](blend_layer, layer_set);
			}
		}

		layer = layer->next;

		if(layer->layer_set != layer_set)
		{
			// 所属レイヤーセットに到達したら
			if(layer == layer_set || layer->layer_set == NULL)
			{
				break;
			}

			MixLayerSet(layer, &layer, window);
		}
	}	// while(1)
			// 所属レイヤーセットまでループ

	// サムネイル更新
	gtk_widget_queue_draw(layer->widget->thumbnail);

	*next = layer;
}

/***************************************************************
* MixLayerSetActiveOver関数                                    *
* レイヤーセット中のアクティブレイヤー以上のレイヤーを合成する *
* 引数                                                         *
* start		: アクティブレイヤー                               *
* next		: 合成後の次に合成するレイヤー                     *
* window	: 描画領域を管理する構造体のアドレス               *
***************************************************************/
void MixLayerSetActiveOver(LAYER* start, LAYER** next, DRAW_WINDOW* window)
{
	// レイヤーセットのピクセルデータのバイト数
	size_t pixel_bytes = start->layer_set->stride*start->layer_set->height;
	// 所属レイヤーセット
	LAYER *layer_set = start->layer_set;
	// レイヤー合成用
	LAYER *layer = start;
	LAYER *blend_layer;
	int blend_mode;

	// アクティブなレイヤーより下の合成結果をコピー
	(void)memcpy(layer->layer_set->pixels,
		layer->layer_set->layer_data.layer_set_p->active_under->pixels, pixel_bytes);

	// 所属レイヤーセットまでループ
	while(1)
	{
		blend_layer = layer;
		blend_mode = layer->layer_mode;

		// 可視判定
		if((blend_layer->flags & LAYER_FLAG_INVISIBLE) == 0)
		{	// アクティブレイヤーなら
			if(blend_layer == window->active_layer)
			{
				if(layer->layer_type == TYPE_NORMAL_LAYER)
				{	// 通常レイヤーは
						// 作業レイヤーとアクティブレイヤーを一度合成してから下のレイヤーと合成
					(void)memcpy(window->temp_layer->pixels, layer->pixels, layer->stride*layer->height);
					window->layer_blend_functions[window->work_layer->layer_mode](window->work_layer, window->temp_layer);
					blend_layer = window->temp_layer;
					blend_layer->alpha = layer->alpha;
					blend_layer->flags = layer->flags;
					blend_layer->prev = layer->prev;
				}
				else if(layer->layer_type == TYPE_VECTOR_LAYER)
				{	// ベクトルレイヤーは
						// レイヤーのラスタライズ処理を行なってから作業レイヤーと下のレイヤーを合成
					RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
					window->layer_blend_functions[window->work_layer->layer_mode](window->work_layer, layer);
				}
				else if(layer->layer_type == TYPE_TEXT_LAYER)
				{	// テキストレイヤーは
						// テキストの内容をラスタライズ処理してから下のレイヤーと合成
					RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
				}

				while(layer->next != NULL && layer->next->layer_type == TYPE_ADJUSTMENT_LAYER)
				{
					if((layer->next->flags & LAYER_FLAG_INVISIBLE) != 0)
					{
						layer->next->layer_data.adjustment_layer_p->filter_func(
							layer->layer_data.adjustment_layer_p, layer->pixels, layer->next->pixels,
								layer->width * layer->height, layer);
					}
					layer->next->layer_data.adjustment_layer_p->update(
						layer->layer_data.adjustment_layer_p, layer, window->mixed_layer,
							0, 0, layer->width, layer->height);
					blend_layer = layer->next;
					layer = layer->next;
				}

				// サムネイル更新
				gtk_widget_queue_draw(layer->widget->thumbnail);
			}	// アクティブレイヤーなら
					// if(blend_layer == window->active_layer)

			// 合成する対象と方法が確定したので合成を実行する
			window->layer_blend_functions[blend_mode](blend_layer, layer_set);
			// 合成したらデータを元に戻す
			window->temp_layer->alpha = 100;
			window->temp_layer->flags = 0;
			window->temp_layer->prev = NULL;
			cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
		}	// 可視判定
				// if((blend_layer->flags & LAYER_FLAG_INVISIBLE) == 0)

		layer = layer->next;

		if(layer->layer_set != layer_set)
		{
			// 所属レイヤーセットに到達したら
			if(layer == layer_set)
			{
				if(layer_set->layer_set == NULL)
				{
					break;
				}
				layer_set = layer_set->layer_set;
			}
		}
	}	// while(1)
			// 所属レイヤーセットまでループ

	// サムネイル更新
	gtk_widget_queue_draw(layer->widget->thumbnail);

	*next = layer;
}

#ifdef __cplusplus
}
#endif
