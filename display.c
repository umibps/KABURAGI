#include "configure.h"
#include <string.h>
#include <gtk/gtk.h>
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
# include <GL/glew.h>
# include <gtk/gtkgl.h>
#endif
#include "application.h"
#include "display.h"
#include "draw_window.h"
#include "transform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eUPDATE_MODE
{
	NO_UPDATE,
	UPDATE_ALL,
	UPDATE_PART
} eUPDATE_MODE;

/*****************************************
* DisplayDrawWindow関数                  *
* 描画領域の画面更新処理                 *
* 引数                                   *
* widget		: 描画領域のウィジェット *
* event_info	: 描画更新の情報         *
* window		: 描画領域の情報         *
* 返り値                                 *
*	常にFALSE                            *
*****************************************/
gboolean DisplayDrawWindow(
	GtkWidget *widget,
	GdkEventExpose *event_info,
	struct _DRAW_WINDOW *window
)
{
	// 合成するレイヤー
	LAYER *layer = NULL, *blend_layer;
	// マウスの情報取得用
	GdkModifierType state;
	// 拡大・縮小を元に戻すための値
	gdouble rev_zoom = 1.0 / (window->zoom * 0.01);
	// for文用のカウンタ
	gint x, y;
	// 画面更新のモード
	eUPDATE_MODE update_mode = UPDATE_ALL;
	// アクティブレイヤーより下の合成結果を更新するフラグ
	int update_active_under = 0;
	// 合成モード
	int blend_mode;

	// 画面表示用Cairo情報
	cairo_t *cairo_p;

	state = window->state;

	// 選択範囲の編集中でなければ
	if((window->flags & DRAW_WINDOW_EDIT_SELECTION) == 0)
	{
		// 描画領域の更新フラグの状態で分岐
		if((window->flags & DRAW_WINDOW_UPDATE_PART) == 0)
		{
			if((window->flags & DRAW_WINDOW_UPDATE_ACTIVE_UNDER) != 0)
			{	// 全レイヤーを合成
				(void)memcpy(window->mixed_layer->pixels, window->back_ground, window->pixel_buf_size);
				// 合成する最初のレイヤーは一番下のレイヤー
				layer = window->layer;
			}
			else if((window->flags & DRAW_WINDOW_UPDATE_ACTIVE_OVER) != 0)
			{	// アクティブレイヤーとその上のレイヤーを合成
					// 合成する最初のレイヤーはアクティブレイヤー
				if(window->active_layer == window->layer)
				{
					(void)memcpy(window->mixed_layer->pixels, window->back_ground, window->pixel_buf_size);
				}
				else
				{
					(void)memcpy(window->mixed_layer->pixels, window->under_active->pixels, window->pixel_buf_size);
				}
				layer = window->active_layer;
			}
			else
			{	// 更新のフラグが立っていないのでレイヤーの合成無し
				update_mode = NO_UPDATE;
			}
		}
		else
		{
			int stride;
			int end_y;
			if(window->update.x < 0)
			{
				window->update.x = 0;
			}
			stride = (int)((window->update.x + window->update.width < window->width) ?
				window->update.width * 4 : (window->width - window->update.x) * 4);
			if(stride > 0 && window->update.height > 0)
			{
				if(window->update.y < 0)
				{
					window->update.y = 0;
				}

				if(window->update.x + window->update.width > window->width)
				{
					window->update.width = window->width - window->update.x;
				}

				if(window->update.y + window->update.height <= window->height)
				{
					end_y = (int)window->update.height;
				}
				else
				{
					end_y = (int)(window->update.height = window->height - window->update.y);
				}

				if(end_y >= 0)
				{
					if(window->active_layer == window->layer)
					{	// アクティブレイヤーが一番下ならば背景のピクセルデータをコピー
						for(y=0; y<end_y; y++)
						{
							(void)memcpy(&window->mixed_layer->pixels[(y+(int)window->update.y)*window->mixed_layer->stride+(int)window->update.x*4],
								&window->back_ground[(y+(int)window->update.y)*window->mixed_layer->stride+(int)window->update.x*4], stride);
						}
					}
					else
					{	// そうでなければアクティブレイヤーより下の合成済みのデータをコピー
						for(y=0; y<end_y; y++)
						{
							(void)memcpy(&window->mixed_layer->pixels[(y+(int)window->update.y)*window->mixed_layer->stride+(int)window->update.x*4],
								&window->under_active->pixels[(y+(int)window->update.y)*window->mixed_layer->stride+(int)window->update.x*4], stride);
						}
					}
					layer = window->active_layer;
				}
			}
			else
			{
				window->flags &= ~(DRAW_WINDOW_UPDATE_PART);
				goto execute_update;
			}

			window->update.surface_p = cairo_surface_create_for_rectangle(
				window->mixed_layer->surface_p, window->update.x, window->update.y,
					window->update.width, window->update.height);
			window->update.cairo_p = cairo_create(window->update.surface_p);
			window->temp_update = window->update;
			window->temp_update.surface_p = cairo_surface_create_for_rectangle(
				window->temp_layer->surface_p, window->update.x, window->update.y,
					window->update.width, window->update.height);
			window->temp_update.cairo_p = cairo_create(window->temp_update.surface_p);

			update_mode = UPDATE_PART;
		}
	}
	else
	{
		window->flags &= ~(DRAW_WINDOW_UPDATE_PART);
		update_mode = NO_UPDATE;
	}

	if(update_mode == UPDATE_ALL)
	{
		// 一番上のレイヤーに辿り着くまでループ
		while(layer != NULL)
		{
			// レイヤーセット内のレイヤーであれば
			if(layer->layer_set != NULL)
			{	// 全更新ならば
				if((window->flags & DRAW_WINDOW_UPDATE_ACTIVE_UNDER) != 0)
				{	// レイヤーセット内を更新
					MixLayerSet(layer, &layer, window);
				}	// 全更新ならば
					// if((window->flags & DRAW_WINDOW_UPDATE_ACTIVE_UNDER) != 0)
				else if(layer->layer_set == window->active_layer_set)
				{
					MixLayerSetActiveOver(layer, &layer, window);
				}	// else if(layer->layer_set == window->active_layer_set)
				else
				{
					layer = layer->layer_set;
				}
			}	// if(layer->layer_set != NULL)

			// 合成レイヤーと合成方法を一度記憶して
			blend_layer = layer;
			blend_mode = layer->layer_mode;

			// 非表示レイヤーになっていないことを確認
			if((blend_layer->flags & LAYER_FLAG_INVISIBLE) == 0)
			{	// もし、合成するレイヤーがアクティブレイヤーなら
				if(layer == window->active_layer)
				{
					if(layer->layer_type == TYPE_NORMAL_LAYER)
					{	// 通常レイヤーは
							// 作業レイヤーとアクティブレイヤーを一度合成してから下のレイヤーと合成
						(void)memcpy(window->temp_layer->pixels, layer->pixels, layer->stride*layer->height);
						g_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, window->temp_layer);
						blend_layer = window->temp_layer;
						blend_layer->alpha = layer->alpha;
						blend_layer->flags = layer->flags;
						blend_layer->prev = layer->prev;
					}
					else if(layer->layer_type == TYPE_VECTOR_LAYER)
					{	// ベクトルレイヤーは
							// レイヤーのラスタライズ処理を行なってから作業レイヤーと下のレイヤーを合成
						RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
						if(window->work_layer->layer_mode != LAYER_BLEND_NORMAL)
						{
							g_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, layer);
						}
					}
					else if(layer->layer_type == TYPE_TEXT_LAYER)
					{	// テキストレイヤーは
							// テキストの内容をラスタライズ処理してから下のレイヤーと合成
						RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
					}

					// サムネイル更新
					if(layer->widget != NULL)
					{
						gtk_widget_queue_draw(layer->widget->thumbnail);
					}
				}

				// 合成する対象と方法が確定したので合成を実行する
				g_layer_blend_funcs[blend_mode](blend_layer, window->mixed_layer);
				// 合成したらデータを元に戻す
				window->temp_layer->alpha = 100;
				window->temp_layer->flags = 0;
				window->temp_layer->prev = NULL;
				cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
			}	// 非表示レイヤーになっていないことを確認
			// if((blend_layer->flags & LAYER_FLAG_INVISIBLE) == 0)

			// 次のレイヤーへ
			layer = layer->next;

			// 次に合成するレイヤーがアクティブレイヤーなら
			if(layer == window->active_layer)
			{	// アクティブレイヤーより下のレイヤーの合成データを更新
				(void)memcpy(window->under_active->pixels, window->mixed_layer->pixels, window->pixel_buf_size);
			}
		}	// 一番上のレイヤーに辿り着くまでループ
				// while(layer != NULL)
	}
	else
	{
		// 一番上のレイヤーに辿り着くまでループ
		while(layer != NULL)
		{
			// レイヤーセット内のレイヤーであれば
			if(layer->layer_set != NULL)
			{	// 全更新ならば
				if(layer->layer_set == window->active_layer_set)
				{
					MixLayerSetActiveOver(layer, &layer, window);
				}	// else if(layer->layer_set == window->active_layer_set)
				else
				{
					layer = layer->layer_set;
				}
			}	// if(layer->layer_set != NULL)

			// 合成レイヤーと合成方法を一度記憶して
			blend_layer = layer;
			blend_mode = layer->layer_mode;

			// 非表示レイヤーになっていないことを確認
			if((blend_layer->flags & LAYER_FLAG_INVISIBLE) == 0)
			{	// もし、合成するレイヤーがアクティブレイヤーなら
				if(layer == window->active_layer)
				{
					if(layer->layer_type == TYPE_NORMAL_LAYER)
					{	// 通常レイヤーは
							// 作業レイヤーとアクティブレイヤーを一度合成してから下のレイヤーと合成
						(void)memcpy(window->temp_layer->pixels, layer->pixels, layer->stride*layer->height);
						g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->temp_update);
						//g_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, window->temp_layer);
						blend_layer = window->temp_layer;
						blend_layer->alpha = layer->alpha;
						blend_layer->flags = layer->flags;
						blend_layer->prev = layer->prev;
					}
					else if(layer->layer_type == TYPE_VECTOR_LAYER)
					{	// ベクトルレイヤーは
							// レイヤーのラスタライズ処理を行なってから作業レイヤーと下のレイヤーを合成
						RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
						if(window->work_layer->layer_mode != LAYER_BLEND_NORMAL)
						{
							g_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, layer);
						}
					}
					else if(layer->layer_type == TYPE_TEXT_LAYER)
					{	// テキストレイヤーは
							// テキストの内容をラスタライズ処理してから下のレイヤーと合成
						RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
					}

					// サムネイル更新
					gtk_widget_queue_draw(layer->widget->thumbnail);
				}

				// 合成する対象と方法が確定したので合成を実行する
				g_part_layer_blend_funcs[blend_mode](blend_layer, &window->update);
				// 合成したらデータを元に戻す
				window->temp_layer->alpha = 100;
				window->temp_layer->flags = 0;
				window->temp_layer->prev = NULL;
				cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
			}	// 非表示レイヤーになっていないことを確認
			// if((blend_layer->flags & LAYER_FLAG_INVISIBLE) == 0)

			// 次のレイヤーへ
			layer = layer->next;

			// 次に合成するレイヤーがアクティブレイヤーなら
			if(layer == window->active_layer)
			{	// アクティブレイヤーより下のレイヤーの合成データを更新
				(void)memcpy(window->under_active->pixels, window->mixed_layer->pixels, window->pixel_buf_size);
			}
		}	// 一番上のレイヤーに辿り着くまでループ
				// while(layer != NULL)
	}

	if(update_mode == UPDATE_ALL)
	{
		if(window->app->display_filter.filter_func != NULL)
		{
			window->app->display_filter.filter_func(window->mixed_layer->pixels,
				window->mixed_layer->pixels, window->width*window->height, window->app->display_filter.filter_data);
		}

		// 現在の拡大縮小率で表示用のデータに合成したデータを転写
		cairo_set_operator(window->scaled_mixed->cairo_p, CAIRO_OPERATOR_SOURCE);
		cairo_set_source(window->scaled_mixed->cairo_p, window->mixed_pattern);
		cairo_paint(window->scaled_mixed->cairo_p);
	}
	else if(update_mode == UPDATE_PART)
	{
		FLOAT_T zoom = window->zoom_rate;

		if(window->app->display_filter.filter_func != NULL)
		{
			int start_x = (int)window->update.x;
			int start_y = (int)window->update.y;
			int width = (int)window->update.width;
			int height = (int)window->update.height;
			int stride = width * 4;
			for(y=0; y<height; y++)
			{
				(void)memcpy(&window->temp_layer->pixels[y*stride],
					&window->mixed_layer->pixels[(start_y+y)*window->mixed_layer->stride + start_x*4], stride);
			}
			window->app->display_filter.filter_func(window->temp_layer->pixels,
				window->temp_layer->pixels, width*height, window->app->display_filter.filter_data);
			for(y=0; y<height; y++)
			{
				(void)memcpy(&window->mixed_layer->pixels[(start_y+y)*window->mixed_layer->stride + start_x*4],
					&window->temp_layer->pixels[y*stride], stride);
			}
		}

		cairo_save(window->scaled_mixed->cairo_p);
		cairo_rectangle(window->scaled_mixed->cairo_p, (int)(window->update.x * zoom), (int)(window->update.y * zoom),
			(int)(window->update.width * zoom), (int)(window->update.height * zoom));
		cairo_clip(window->scaled_mixed->cairo_p);
		cairo_set_operator(window->scaled_mixed->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source(window->scaled_mixed->cairo_p, window->mixed_pattern);
		cairo_paint(window->scaled_mixed->cairo_p);
		cairo_restore(window->scaled_mixed->cairo_p);
		//ScaleNearest(window);

		cairo_surface_destroy(window->update.surface_p);
		cairo_destroy(window->update.cairo_p);
		cairo_surface_destroy(window->temp_update.surface_p);
		cairo_destroy(window->temp_update.cairo_p);

		window->flags &= ~(DRAW_WINDOW_UPDATE_PART);
	}

	(void)memcpy(window->disp_layer->pixels, window->scaled_mixed->pixels,
		window->scaled_mixed->stride * window->scaled_mixed->height);

	// マウスカーソルの描画処理
		// ブラシが持つカーソル表示用の関数を呼び出す
	// 変形処理中はカーソル表示はしない
	if(window->transform == NULL)
	{
		// クリッピング前の状態を記憶
		cairo_save(window->disp_layer->cairo_p);

		if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
		{	// 通常・ベクトルレイヤー両方で使えるツール使用中
			window->app->tool_window.active_common_tool->display_func(
				window, window->app->tool_window.active_common_tool);

			g_layer_blend_funcs[window->effect->layer_mode](window->effect, window->disp_layer);
			// 表示用に使用したデータを初期化
			window->effect->layer_mode = LAYER_BLEND_NORMAL;
			(void)memset(window->effect->pixels, 0, window->effect->stride*window->effect->height);
		}
		else
		{
			(void)memset(window->disp_temp->pixels, 0, window->disp_temp->stride*window->disp_temp->height);
			if(window->active_layer->layer_type == TYPE_NORMAL_LAYER || ((window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0))
			{	// 通常レイヤー
				window->app->tool_window.active_brush[window->app->input]->draw_cursor(
					window, window->cursor_x, window->cursor_y, window->app->tool_window.active_brush[window->app->input]->brush_data);
			}
			else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
			{	// ベクトルレイヤーの場合、ShiftキーあるいはCtrlキーが
					// 押されていたら制御点操作の表示方法にする
				if((state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) != 0)
				{
					window->app->tool_window.vector_control_core.draw_cursor(
						window, window->cursor_x, window->cursor_y, window->app->tool_window.vector_control_core.brush_data);
				}
				else
				{	// そうでなければブラシが持つカーソル表示関数を呼び出す
					window->app->tool_window.active_vector_brush[window->app->input]->draw_cursor(
						window, window->cursor_x, window->cursor_y, window->app->tool_window.active_vector_brush[window->app->input]->brush_data);
				}
			}
			else
			{	// テキストレイヤーならテキストの描画領域を表示
				DisplayTextLayerRange(window, window->active_layer->layer_data.text_layer_p);
			}
			// 作成したデータを表示データに合成
			g_layer_blend_funcs[LAYER_BLEND_DIFFERENCE](window->disp_temp, window->disp_layer);
		}	// マウスカーソルの描画処理
					// ブラシが持つカーソル表示用の関数を呼び出す
			// if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0) else

		// Cairoを元の状態に戻す
		cairo_restore(window->disp_layer->cairo_p);

		// 選択範囲の編集中であれば内容を描画
		if((window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
		{
			DisplayEditSelection(window);
		}
		else
		{
			// 選択範囲があれば表示
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
			{
				DrawSelectionArea(&window->selection_area, window);
				g_layer_blend_funcs[LAYER_BLEND_NORMAL](window->effect, window->disp_layer);
				(void)memset(window->effect->pixels, 0, window->effect->stride*window->effect->height);
			}
		}
	}	// 変形処理中はカーソル表示はしない
		// if(window->transform == NULL)
	else
	{
		(void)memset(window->disp_temp->pixels, 0, window->disp_temp->stride*window->disp_temp->height);
		DisplayTransform(window);
	}

	// 左右反転表示中なら表示内容を左右反転
	if((window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) != 0)
	{
		uint8 *ref, *src;

		for(y=0; y<window->disp_layer->height; y++)
		{
			ref = window->disp_temp->pixels;
			src = &window->disp_layer->pixels[(y+1)*window->disp_layer->stride-4];
			for(x=0; x<window->disp_layer->width; x++, ref += 4, src -= 4)
			{
				ref[0] = src[0], ref[1] = src[1], ref[2] = src[2], ref[3] = src[3];
			}
			(void)memcpy(src+4, window->disp_temp->pixels, window->disp_layer->stride);
		}
	}

execute_update:

	// 回転処理 & 表示
# if GTK_MAJOR_VERSION <= 2
	cairo_p = gdk_cairo_create(window->window->window);
	gdk_cairo_region(cairo_p, event_info->region);
	cairo_clip(cairo_p);
# else
	cairo_p = (cairo_t*)event_info;
# endif
	cairo_set_source(cairo_p, window->rotate);
	cairo_paint(cairo_p);
# if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
# endif

	// 画面更新が終わったのでフラグを下ろす
	window->flags &= ~(DRAW_WINDOW_UPDATE_ACTIVE_UNDER | DRAW_WINDOW_UPDATE_ACTIVE_OVER);

	// ナビゲーションおよびプレビューの内容を更新
	if(update_mode != NO_UPDATE)
	{
		if(window->app->navigation_window.draw_area != NULL)
		{
			gtk_widget_queue_draw(window->app->navigation_window.draw_area);
		}

		if(window->app->preview_window.window != NULL)
		{
			gtk_widget_queue_draw(window->app->preview_window.image);
		}
	}

	return TRUE;
}

/*******************************************************
* MixLayerForSave関数                                  *
* 保存するために背景ピクセルデータ無しでレイヤーを合成 *
* 引数                                                 *
* window	: 描画領域の情報                           *
* 返り値                                               *
*	合成したレイヤーのデータ                           *
*******************************************************/
LAYER* MixLayerForSave(DRAW_WINDOW* window)
{
	// レイヤーのメモリを確保して
	LAYER* ret = CreateLayer(
		0, 0, window->width, window->height, 4,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, window
	);
	LAYER* src = window->layer;

	// 非表示でない全てのレイヤーを合成
	while(src != NULL)
	{
		if((src->flags & LAYER_FLAG_INVISIBLE) == 0 && src->layer_type != TYPE_LAYER_SET)
		{
			if(!(src->layer_set != NULL && (src->layer_set->flags & LAYER_FLAG_INVISIBLE) != 0))
			{
				g_layer_blend_funcs[src->layer_mode](src, ret);
			}
		}

		src = src->next;
	}

	return ret;
}

/*******************************************************
* MixLayerForSaveWithBackGround関数                    *
* 保存するために背景ピクセルデータ有りでレイヤーを合成 *
* 引数                                                 *
* window	: 描画領域の情報                           *
* 返り値                                               *
*	合成したレイヤーのデータ                           *
*******************************************************/
LAYER* MixLayerForSaveWithBackGround(DRAW_WINDOW* window)
{
	// レイヤーのメモリを確保して
	LAYER* ret = CreateLayer(
		0, 0, window->width, window->height, 4,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, window
	);
	LAYER* src = window->layer;

	(void)memcpy(ret->pixels, window->back_ground, window->pixel_buf_size);

	// 非表示でない全てのレイヤーを合成
	while(src != NULL)
	{
		if((src->flags & LAYER_FLAG_INVISIBLE) == 0 && src->layer_type != TYPE_LAYER_SET)
		{
			if(!(src->layer_set != NULL && (src->layer_set->flags & LAYER_FLAG_INVISIBLE) != 0))
			{
				g_layer_blend_funcs[src->layer_mode](src, ret);
			}
		}

		src = src->next;
	}

	return ret;
}

#ifdef __cplusplus
}
#endif
