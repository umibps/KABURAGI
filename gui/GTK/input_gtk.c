#include <ctype.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "../../application.h"
#include "../../brush_core.h"
#include "../../transform.h"

#include "input_gtk.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************
* ButtonPressEvent関数                         *
* マウスクリックの処理                         *
* 引数                                         *
* widget	: マウスクリックされたウィジェット *
* event		: マウスの情報                     *
* window	: 描画領域の情報                   *
* 返り値                                       *
*	常にTRUE                                   *
***********************************************/
gboolean ButtonPressEvent(GtkWidget *widget, GdkEventButton *event, DRAW_WINDOW* window)
{
	brush_update_func update_func = DefaultToolUpdate;
	void *update_data = NULL;
	gdouble pressure;
	gdouble rev_zoom = window->rev_zoom;
	FLOAT_T x, y;
#if GTK_MAJOR_VERSION >= 3
	GdkDevice *device;
	GdkInputSource source;
#endif

	if((window->state & GDK_BUTTON1_MASK) != 0
		&& event->type != GDK_2BUTTON_PRESS)
	{
		return TRUE;
	}

	// 画面更新用のデータを取得
	if(window->transform == NULL)
	{
		if((window->app->tool_window.flags & TOOL_USING_BRUSH) != 0)
		{
			if(window->active_layer->layer_type == TYPE_NORMAL_LAYER)
			{
				update_func = window->app->tool_window.active_brush[window->app->input]->button_update;
				update_data = window->app->tool_window.active_brush[window->app->input]->brush_data;
			}
			else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
			{
				update_func = window->app->tool_window.active_vector_brush[window->app->input]->button_update;
				update_data = window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			}
		}
		else
		{
			update_func = window->app->tool_window.active_common_tool->button_update;
			update_data = window->app->tool_window.active_common_tool->tool_data;
		}
	}

	// マウス以外なら筆圧を取得
#if GTK_MAJOR_VERSION <= 2
	if(event->device->source != GDK_SOURCE_MOUSE)
	{
		if(event->device->source == GDK_SOURCE_ERASER)
		{
			if(window->app->input == INPUT_PEN)
			{
				window->app->input = INPUT_ERASER;
				if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| ((window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0))
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_ERASER]->button), TRUE);
				}
				else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_ERASER]->button), TRUE);
				}
			}
		}
		else
		{
			if(window->app->input != INPUT_PEN)
			{
				window->app->input = INPUT_PEN;
				if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| ((window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0))
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
				}
				else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
				}
			}
		}
		gdk_event_get_axis((GdkEvent*)event, GDK_AXIS_PRESSURE, &pressure);
	}
	else
	{	// マウスなら筆圧はレイヤーのタイプに依存
		pressure = (window->active_layer->layer_type == TYPE_NORMAL_LAYER) ? 1.0 : 0.5;
	}
#else
	device = gdk_event_get_source_device((GdkEvent*)event);
	source = gdk_device_get_source(device);
	if(gdk_device_get_axis(device, event->axes, GDK_AXIS_PRESSURE, &pressure) == FALSE)
	{
		pressure = (window->active_layer->layer_type == TYPE_NORMAL_LAYER) ? 1.0 : 0.5;
	}
	else
	{
		if(source == GDK_SOURCE_ERASER)
		{
			if(window->app->input == INPUT_PEN)
			{
				window->app->input = INPUT_ERASER;
				if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| ((window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0))
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_ERASER]->button), TRUE);
				}
				else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_ERASER]->button), TRUE);
				}
			}
		}
		else
		{
			if(window->app->input != INPUT_PEN)
			{
				window->app->input = INPUT_PEN;
				if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| ((window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0))
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
				}
				else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
				}
			}
		}
	}
#endif

	// 筆圧が下限値未満なら修正
	if(pressure < MINIMUM_PRESSURE)
	{
		pressure = MINIMUM_PRESSURE;
	}

	// 回転分を計算
	window->cursor_x = (event->x - window->half_size) * window->cos_value
		- (event->y - window->half_size) * window->sin_value + window->add_cursor_x;
	window->cursor_y = (event->x - window->half_size) * window->sin_value
		+ (event->y - window->half_size) * window->cos_value + window->add_cursor_y;
	x = rev_zoom * window->cursor_x;
	y = rev_zoom * window->cursor_y;

	// 左右反転表示中ならばX座標を修正
	if((window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) != 0)
	{
		x = window->width - x;
		window->cursor_x = window->disp_layer->width - window->cursor_x;
	}

	// 手ブレ補正実行
	if(event->button == 1)
	{
		// クリックされたボタンを記憶
		window->state = event->state | GDK_BUTTON1_MASK;

		if(window->app->tool_window.smoother.num_use != 0)
		{
			if(window->app->tool_window.smoother.mode == SMOOTH_GAUSSIAN)
			{
				Smooth(&window->app->tool_window.smoother, &x, &y, event->time, window->rev_zoom);
			}
			else
			{
				window->app->tool_window.smoother.last_draw_point.x = event->x;
				window->app->tool_window.smoother.last_draw_point.y = event->y;
				(void)AddAverageSmoothPoint(&window->app->tool_window.smoother,
					&x, &y, &pressure, window->rev_zoom);
			}
		}
	}

	if(window->transform != NULL)
	{
		TransformButtonPress(window->transform, x, y);
		return TRUE;
	}

	if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
	{
		window->app->tool_window.active_common_tool->press_func(
			window, x, y, window->app->tool_window.active_common_tool, (void*)event);
	}
	else
	{
		if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
			|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
		{
			if((event->state & GDK_CONTROL_MASK) == 0 && event->button != 3)
			{
				if((event->state & GDK_SHIFT_MASK) == 0)
				{
					window->app->tool_window.active_brush[window->app->input]->press_func(
						window, x, y, pressure, window->app->tool_window.active_brush[window->app->input], (void*)event
					);
				}
				else if(event->button == 1)
				{
					// シフトキーが押されているので直線で処理
					FLOAT_T d;
					FLOAT_T dx = x - window->last_x;
					FLOAT_T dy = y - window->last_y;
					d = sqrt(dx*dx + dy*dy) * 2;
					dx = dx / d,	dy = dy / d;

					window->last_x += dx;
					window->last_y += dy;
					d -= 1;

					if((window->flags & DRAW_WINDOW_DRAWING_STRAIGHT) == 0)
					{
						window->app->tool_window.active_brush[window->app->input]->press_func(
							window, window->last_x, window->last_y, pressure, window->app->tool_window.active_brush[window->app->input], (void*)event);
						window->flags |= DRAW_WINDOW_DRAWING_STRAIGHT;
					}

					while(d >= 1)
					{
						window->last_x += dx;
						window->last_y += dy;

						window->app->tool_window.active_brush[window->app->input]->motion_func(
							window, window->last_x, window->last_y, pressure,
								window->app->tool_window.active_brush[window->app->input], (void*)(&window->state));

						d -= 1;
					}
					window->app->tool_window.active_brush[window->app->input]->motion_func(
						window, x, y, pressure, window->app->tool_window.active_brush[window->app->input], (void*)(&window->state));

					window->state &= ~(GDK_BUTTON1_MASK);
					window->last_x = x,	window->last_y = y;
					window->last_pressure = pressure;

					gtk_widget_queue_draw(window->window);
				}
			}
			else
			{
				window->app->tool_window.color_picker_core.press_func(
					window, x, y, &window->app->tool_window.color_picker_core, (void*)event
				);
				window->state &= ~(GDK_BUTTON1_MASK);
			}
		}
		else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
		{
			if((event->state & GDK_SHIFT_MASK) != 0)
			{
				window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
				window->app->tool_window.vector_control_core.press_func(
					window, x, y, pressure, &window->app->tool_window.vector_control_core, (void*)event
				);
			}
			else if((event->state & GDK_CONTROL_MASK) != 0)
			{
				window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
				window->app->tool_window.vector_control_core.press_func(
					window, x, y, pressure, &window->app->tool_window.vector_control_core, (void*)event
				);
			}
			else
			{
				window->app->tool_window.active_vector_brush[window->app->input]->press_func(
					window, x, y, pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)event
				);
			}
		}
		else
		{
			TextLayerButtonPressCallBack(window, x, y, event);
		}
	}

	// 再描画
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	update_func(window, event->x, event->y, update_data);

	window->before_cursor_x = event->x;
	window->before_cursor_y = event->y;

	return TRUE;
}

/*************************************
* MotionNotifyEvent関数              *
* マウスオーバーの処理               *
* 引数                               *
* widget	: 描画領域のウィジェット *
* event		: マウスの情報           *
* window	: 描画領域の情報         *
* 返り値                             *
*	常にTRUE                         *
*************************************/
gboolean MotionNotifyEvent(GtkWidget *widget, GdkEventMotion *event, DRAW_WINDOW* window)
{
	GdkModifierType state;
	brush_update_func update_func = DefaultToolUpdate;
	void *update_data = NULL;
	gdouble pressure;
	gdouble x, y, x0, y0;
	gdouble rev_zoom = window->rev_zoom;
	int update_window = 0;
#if GTK_MAJOR_VERSION >= 3
	GdkDevice *device;
	GdkInputSource source;
#endif
	// 座標を取得
	if(event->is_hint != 0)
	{
		gint get_x, get_y;
#if GTK_MAJOR_VERSION <= 2
		gdk_window_get_pointer(event->window, &get_x, &get_y, &state);
#else
		gdk_window_get_device_position(event->window, event->device,
			&get_x, &get_y, &state);
#endif
		x0 = get_x, y0 = get_y;
	}
	else
	{
		state = event->state;
		x0 = event->x;
		y0 = event->y;
	}

	// 回転分を計算
	window->cursor_x = (x0 - window->half_size) * window->cos_value
		- (y0 - window->half_size) * window->sin_value + window->add_cursor_x;
	window->cursor_y = (x0 - window->half_size) * window->sin_value
		+ (y0 - window->half_size) * window->cos_value + window->add_cursor_y;
	x = rev_zoom * window->cursor_x;
	y = rev_zoom * window->cursor_y;

	// 左右反転表示中ならばX座標を修正
	if((window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) != 0)
	{
		x = window->width - x;
		window->cursor_x = window->disp_layer->width - window->cursor_x;
	}

	// 入力デバイスの設定
#if GTK_MAJOR_VERSION <= 2
	if(event->device->source == GDK_SOURCE_ERASER)
#else
	device = gdk_event_get_device((GdkEvent*)event);
	source = gdk_device_get_source(device);
	if(source == GDK_SOURCE_ERASER)
#endif
	{
		if(window->app->input == INPUT_PEN)
		{
			window->app->input = INPUT_ERASER;
			gtk_widget_destroy(window->app->tool_window.detail_ui);
			if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
				|| ((window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0))
			{
				gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_ERASER]->button), TRUE);
				window->app->tool_window.detail_ui =
					window->app->tool_window.active_brush[INPUT_ERASER]->create_detail_ui(
						window->app, window->app->tool_window.active_brush[INPUT_ERASER]);
			}
			else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
			{
				gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_ERASER]->button), TRUE);
				window->app->tool_window.detail_ui =
					window->app->tool_window.active_vector_brush[INPUT_ERASER]->create_detail_ui(
						window->app, window->app->tool_window.active_vector_brush[INPUT_ERASER]);
			}
			// スクロールドウィンドウに入れる
			gtk_scrolled_window_add_with_viewport(
				GTK_SCROLLED_WINDOW(window->app->tool_window.detail_ui_scroll), window->app->tool_window.detail_ui);
			gtk_widget_show_all(window->app->tool_window.detail_ui);
		}
	}
	else
	{
		if(window->app->input != INPUT_PEN)
		{
			window->app->input = INPUT_PEN;
			gtk_widget_destroy(window->app->tool_window.detail_ui);
			if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
				|| ((window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0))
			{
				gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
				window->app->tool_window.detail_ui =
					window->app->tool_window.active_brush[INPUT_PEN]->create_detail_ui(
						window->app, window->app->tool_window.active_brush[INPUT_PEN]);
			}
			else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
			{
				gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(window->app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
				window->app->tool_window.detail_ui =
					window->app->tool_window.active_vector_brush[INPUT_PEN]->create_detail_ui(
						window->app, window->app->tool_window.active_vector_brush[INPUT_PEN]);
			}
			// スクロールドウィンドウに入れる
			gtk_scrolled_window_add_with_viewport(
				GTK_SCROLLED_WINDOW(window->app->tool_window.detail_ui_scroll), window->app->tool_window.detail_ui);
			gtk_widget_show_all(window->app->tool_window.detail_ui);
		}
	}

	// 画面更新用のデータを取得
	if(window->transform == NULL)
	{
		if((window->app->tool_window.flags & TOOL_USING_BRUSH) != 0)
		{
			if(window->active_layer->layer_type == TYPE_NORMAL_LAYER)
			{
				update_func = window->app->tool_window.active_brush[window->app->input]->motion_update;
				update_data = window->app->tool_window.active_brush[window->app->input]->brush_data;
			}
			else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
			{
				update_func = window->app->tool_window.active_vector_brush[window->app->input]->motion_update;
				update_data = window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			}
		}
		else
		{
			update_func = window->app->tool_window.active_common_tool->motion_update;
			update_data = window->app->tool_window.active_common_tool->tool_data;
		}
	}

	// マウス以外なら筆圧を取得
#if GTK_MAJOR_VERSION <= 2
	if(event->device->source != GDK_SOURCE_MOUSE)
	{
		gdk_event_get_axis((GdkEvent*)event, GDK_AXIS_PRESSURE, &pressure);
#else
	if(gdk_device_get_axis(device, event->axes, GDK_AXIS_PRESSURE, &pressure) != FALSE)
	{
#endif
		if((window->state & GDK_BUTTON1_MASK) == 0)
		{
			if(pressure > MINIMUM_PRESSURE)
			{
				GdkEventButton button = {0};
				button.button = 1;
				button.state = GDK_BUTTON1_MASK | state;
				button.device = event->device;

				window->state |= GDK_BUTTON1_MASK;
				window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

				// 手ブレ補正実行
				if(window->app->tool_window.smoother.num_use != 0
					&& (window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) == 0)
				{
					if(window->app->tool_window.smoother.mode == SMOOTH_GAUSSIAN)
					{
						Smooth(&window->app->tool_window.smoother, &x, &y, event->time, window->rev_zoom);
					}
					else
					{
						window->app->tool_window.smoother.last_draw_point.x = x0;
						window->app->tool_window.smoother.last_draw_point.y = y0;
						(void)AddAverageSmoothPoint(&window->app->tool_window.smoother,
							&x, &y, &pressure, window->rev_zoom);
					}
				}

				if(window->transform != NULL)
				{
					TransformButtonPress(window->transform, x, y);
					goto func_end;
				}

				if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
				{
					window->app->tool_window.active_common_tool->press_func(
						window, x, y, window->app->tool_window.active_common_tool, (void*)&button);
				}
				else
				{
					if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
						|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
					{
						if((state & GDK_CONTROL_MASK) == 0)
						{
							if((state & GDK_SHIFT_MASK) == 0)
							{
								window->app->tool_window.active_brush[window->app->input]->press_func(
									window, x, y, pressure, window->app->tool_window.active_brush[window->app->input], (void*)&button
								);
							}
							else
							{
								// シフトキーが押されているので直線で処理
								FLOAT_T d;
								FLOAT_T dx = x - window->last_x;
								FLOAT_T dy = y - window->last_y;
								d = sqrt(dx*dx + dy*dy);
								dx = dx / d,	dy = dy / d;

								window->last_x += dx;
								window->last_y += dy;
								d -= 1;

								if((window->flags & DRAW_WINDOW_DRAWING_STRAIGHT) == 0)
								{
									window->app->tool_window.active_brush[window->app->input]->press_func(
										window, window->last_x, window->last_y, pressure, window->app->tool_window.active_brush[window->app->input], (void*)event);
									window->flags |= DRAW_WINDOW_DRAWING_STRAIGHT;
								}

								while(d >= 1)
								{
									window->last_x += dx;
									window->last_y += dy;

									window->app->tool_window.active_brush[window->app->input]->motion_func(
										window, window->last_x, window->last_y, pressure,
											window->app->tool_window.active_brush[window->app->input], (void*)(&window->state));

									d -= 1;
								}
								window->app->tool_window.active_brush[window->app->input]->motion_func(
									window, x, y, pressure, window->app->tool_window.active_brush[window->app->input], (void*)(&window->state));

								window->state &= ~(GDK_BUTTON1_MASK);
								window->last_x = x,	window->last_y = y;
								window->last_pressure = pressure;

								gtk_widget_queue_draw(window->window);
							}
						}
						else
						{
							window->app->tool_window.color_picker_core.press_func(
								window, x, y, &window->app->tool_window.color_picker_core, (void*)&button
							);
						}
					}
					else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
					{
						if((event->state & GDK_SHIFT_MASK) != 0)
						{
							window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
							window->app->tool_window.vector_control_core.press_func(
								window, x, y, pressure, &window->app->tool_window.vector_control_core, (void*)&button
							);
						}
						else if((event->state & GDK_CONTROL_MASK) != 0)
						{
							window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
							window->app->tool_window.vector_control_core.press_func(
								window, x, y, pressure, &window->app->tool_window.vector_control_core, (void*)&button
							);
						}
						else
						{
							window->app->tool_window.active_vector_brush[window->app->input]->press_func(
								window, x, y, pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)&button
							);
						}
					}
					else
					{
						TextLayerButtonPressCallBack(window, x, y, &button);
					}
				}

				goto func_end;
			}
		}
		else
		{
			if(pressure <= RELEASE_PRESSURE)
			{
				GdkEventButton button = {0};
				button.button = 1;
				button.state = GDK_BUTTON1_MASK | state;
				button.device = event->device;

				window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

				if(window->app->tool_window.smoother.num_use > 0 &&
					window->app->tool_window.smoother.mode != SMOOTH_GAUSSIAN)
				{
					FLOAT_T motion_x, motion_y, motion_pressure;
					FLOAT_T update_x, update_y;
					gboolean finish;

					if(AddAverageSmoothPoint(&window->app->tool_window.smoother, &x, &y,
						&pressure, 0) != FALSE)
					{
						if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
							|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
						{
							window->app->tool_window.active_brush[window->app->input]->motion_func(
								window, x, y, pressure,
									window->app->tool_window.active_brush[window->app->input], (void*)(&window->state)
							);
						}
						else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
						{
							if((state & GDK_SHIFT_MASK) != 0)
							{
								/*
								window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
								window->app->tool_window.vector_control_core.motion_func(
									window, x, y, pressure, &window->app->tool_window.vector_control_core, (void*)(&window->state)
								);*/
							}
							else if((event->state & GDK_CONTROL_MASK) != 0 ||
								(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
							{
								/*
								window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
								window->app->tool_window.vector_control_core.motion_func(
									window, x, y, motion_pressure,
										&window->app->tool_window.vector_control_core, (void*)(&window->state)
								);*/
							}
							else
							{
								window->app->tool_window.active_vector_brush[window->app->input]->motion_func(
									window, x, y, pressure,
										window->app->tool_window.active_vector_brush[window->app->input], (void*)(&window->state)
								);
							}
						}
						else
						{
							TextLayerMotionCallBack(window, x, y, event->state);
						}

						window->before_cursor_x = window->app->tool_window.smoother.last_draw_point.x;
						window->before_cursor_y = window->app->tool_window.smoother.last_draw_point.y;
						update_x = ((x-window->width/2)*window->cos_value + (y-window->height/2)*window->sin_value) * window->zoom_rate
							+ window->rev_add_cursor_x;
						update_y = (- (x-window->width/2)*window->sin_value + (y-window->height/2)*window->cos_value) * window->zoom_rate
							+ window->rev_add_cursor_y;
						update_func(window, update_x, update_y, update_data);
						window->app->tool_window.smoother.last_draw_point.x = update_x;
						window->app->tool_window.smoother.last_draw_point.y = update_y;
					}

					do
					{
						finish = AverageSmoothFlush(&window->app->tool_window.smoother,
							&motion_x, &motion_y, &motion_pressure);

						if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
						{
							window->app->tool_window.active_common_tool->motion_func(
								window, motion_x, motion_y, window->app->tool_window.active_common_tool,
									(void*)(&window->state) );
						}
						else
						{
							if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
								|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
							{
								window->app->tool_window.active_brush[window->app->input]->motion_func(
									window, motion_x, motion_y, motion_pressure,
										window->app->tool_window.active_brush[window->app->input], (void*)(&window->state)
								);
							}
							else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
							{
								if((state & GDK_SHIFT_MASK) != 0)
								{
									/*
									window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
									window->app->tool_window.vector_control_core.motion_func(
										window, motion_x, motion_y, motion_pressure, &window->app->tool_window.vector_control_core, (void*)(&window->state)
									);*/
								}
								else if((event->state & GDK_CONTROL_MASK) != 0 ||
									(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
								{
									/*
									window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
									window->app->tool_window.vector_control_core.motion_func(
										window, motion_x, motion_y, motion_pressure,
											&window->app->tool_window.vector_control_core, (void*)(&window->state)
									);*/
								}
								else
								{
									window->app->tool_window.active_vector_brush[window->app->input]->motion_func(
										window, motion_x, motion_y, motion_pressure,
											window->app->tool_window.active_vector_brush[window->app->input], (void*)(&window->state)
									);
								}
							}
							else
							{
								TextLayerMotionCallBack(window, motion_x, motion_y, event->state);
							}
						}

						window->before_cursor_x = window->app->tool_window.smoother.last_draw_point.x;
						window->before_cursor_y = window->app->tool_window.smoother.last_draw_point.y;
						update_x = ((motion_x-window->width/2)*window->cos_value + (motion_y-window->height/2)*window->sin_value) * window->zoom_rate
							+ window->rev_add_cursor_x;
						update_y = (- (motion_x-window->width/2)*window->sin_value + (motion_y-window->height/2)*window->cos_value) * window->zoom_rate
							+ window->rev_add_cursor_y;
						update_func(window, update_x, update_y, update_data);
						window->app->tool_window.smoother.last_draw_point.x = update_x;
						window->app->tool_window.smoother.last_draw_point.y = update_y;
					} while(finish == FALSE);
				}

				INIT_SMOOTHER(window->app->tool_window.smoother);

				// ボタン状態もリセット
				window->state &= ~(GDK_BUTTON1_MASK);

				if(window->transform != NULL)
				{
					window->transform->trans_point = TRANSFORM_POINT_NONE;
					goto func_end;
				}

				if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
				{
					window->app->tool_window.active_common_tool->release_func(
						window, x, y,
						window->app->tool_window.active_common_tool, (void*)&button
					);
				}
				else
				{
					if(window->active_layer->layer_type == TYPE_NORMAL_LAYER)
					{
						window->app->tool_window.active_brush[window->app->input]->release_func(
							window, x, y, pressure,
							window->app->tool_window.active_brush[window->app->input], (void*)&button
						);
					}
					else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
					{
						if((event->state & GDK_SHIFT_MASK) != 0)
						{
							window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
							window->app->tool_window.vector_control_core.release_func(
									window, x, y,
								pressure, &window->app->tool_window.vector_control_core, (void*)&button
							);
						}
						else if((state & GDK_CONTROL_MASK) != 0 ||
							(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
						{
							window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
							window->app->tool_window.vector_control_core.release_func(
								window, x,y,
									pressure, &window->app->tool_window.vector_control_core, (void*)&button
						);
						}
						else
						{
							window->app->tool_window.active_vector_brush[window->app->input]->release_func(
								window, x, y,
								pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)&button
							);
						}
					}
					else if(window->active_layer->layer_type == TYPE_TEXT_LAYER)
					{
						TextLayerButtonReleaseCallBack(window, x, y, &button);
					}
				}
			}
		}
	}
	else
	{
		pressure = 1.0;

		if((state & GDK_BUTTON1_MASK) == 0 && (window->state & GDK_BUTTON1_MASK) != 0)
		{
			GdkEventButton button = {0};
			button.button = 1;
			button.state = GDK_BUTTON1_MASK | state;
			button.device = event->device;

			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

			if(window->app->tool_window.smoother.num_use > 0 &&
				window->app->tool_window.smoother.mode != SMOOTH_GAUSSIAN)
			{
				FLOAT_T motion_x, motion_y, motion_pressure;
				FLOAT_T update_x, update_y;
				gboolean finish;

				if(AddAverageSmoothPoint(&window->app->tool_window.smoother, &x, &y,
					&pressure, 0) != FALSE)
				{
					if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
						|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
					{
						window->app->tool_window.active_brush[window->app->input]->motion_func(
							window, x, y, pressure,
								window->app->tool_window.active_brush[window->app->input], (void*)(&window->state)
						);
					}
					else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
					{
						if((state & GDK_SHIFT_MASK) != 0)
						{
							/*
							window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
							window->app->tool_window.vector_control_core.motion_func(
								window, x, y, pressure, &window->app->tool_window.vector_control_core, (void*)(&window->state)
							);*/
						}
						else if((event->state & GDK_CONTROL_MASK) != 0 ||
							(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
						{
							/*
							window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
							window->app->tool_window.vector_control_core.motion_func(
								window, x, y, motion_pressure,
									&window->app->tool_window.vector_control_core, (void*)(&window->state)
							);*/
						}
						else
						{
							window->app->tool_window.active_vector_brush[window->app->input]->motion_func(
								window, x, y, pressure,
									window->app->tool_window.active_vector_brush[window->app->input], (void*)(&window->state)
							);
						}
					}
					else
					{
						TextLayerMotionCallBack(window, x, y, event->state);
					}

					window->before_cursor_x = window->app->tool_window.smoother.last_draw_point.x;
					window->before_cursor_y = window->app->tool_window.smoother.last_draw_point.y;
					update_x = ((x-window->width/2)*window->cos_value + (y-window->height/2)*window->sin_value) * window->zoom_rate
						+ window->rev_add_cursor_x;
					update_y = (- (x-window->width/2)*window->sin_value + (y-window->height/2)*window->cos_value) * window->zoom_rate
						+ window->rev_add_cursor_y;
					update_func(window, update_x, update_y, update_data);
					window->app->tool_window.smoother.last_draw_point.x = update_x;
					window->app->tool_window.smoother.last_draw_point.y = update_y;
				}

				do
				{
					finish = AverageSmoothFlush(&window->app->tool_window.smoother,
						&motion_x, &motion_y, &motion_pressure);

					if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
					{
						window->app->tool_window.active_common_tool->motion_func(
							window, motion_x, motion_y, window->app->tool_window.active_common_tool,
								(void*)(&window->state) );
					}
					else
					{
						if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
							|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
						{
							window->app->tool_window.active_brush[window->app->input]->motion_func(
								window, motion_x, motion_y, motion_pressure,
									window->app->tool_window.active_brush[window->app->input], (void*)(&window->state)
							);
						}
						else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
						{
							if((state & GDK_SHIFT_MASK) != 0)
							{
								/*
								window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
								window->app->tool_window.vector_control_core.motion_func(
									window, motion_x, motion_y, motion_pressure, &window->app->tool_window.vector_control_core, (void*)(&window->state)
								);
								*/
							}
							else if((event->state & GDK_CONTROL_MASK) != 0 ||
								(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
							{
								/*
								window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
								window->app->tool_window.vector_control_core.motion_func(
									window, motion_x, motion_y, motion_pressure,
										&window->app->tool_window.vector_control_core, (void*)(&window->state)
								);
								*/
							}
							else
							{
								window->app->tool_window.active_vector_brush[window->app->input]->motion_func(
									window, motion_x, motion_y, motion_pressure,
										window->app->tool_window.active_vector_brush[window->app->input], (void*)(&window->state)
								);
							}
						}
						else
						{
							TextLayerMotionCallBack(window, motion_x, motion_y, event->state);
						}
					}

					window->before_cursor_x = window->app->tool_window.smoother.last_draw_point.x;
					window->before_cursor_y = window->app->tool_window.smoother.last_draw_point.y;
					update_x = ((motion_x-window->width/2)*window->cos_value + (motion_y-window->height/2)*window->sin_value) * window->zoom_rate
						+ window->rev_add_cursor_x;
					update_y = (- (motion_x-window->width/2)*window->sin_value + (motion_y-window->height/2)*window->cos_value) * window->zoom_rate
						+ window->rev_add_cursor_y;
					update_func(window, update_x, update_y, update_data);
					window->app->tool_window.smoother.last_draw_point.x = update_x;
					window->app->tool_window.smoother.last_draw_point.y = update_y;
				} while(finish == FALSE);
			}

			INIT_SMOOTHER(window->app->tool_window.smoother);

			// ボタン状態もリセット
			window->state &= ~(GDK_BUTTON1_MASK);

			if(window->transform != NULL)
			{
				window->transform->trans_point = TRANSFORM_POINT_NONE;
				update_func(window, x0, y0, update_data);
				goto func_end;
			}

			if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
			{
				window->app->tool_window.active_common_tool->release_func(
					window, x, y,
					window->app->tool_window.active_common_tool, (void*)&button
				);
			}
			else
			{
				if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
				{
					window->app->tool_window.active_brush[window->app->input]->release_func(
						window, x, y, pressure,
						window->app->tool_window.active_brush[window->app->input], (void*)&button
					);
				}
				else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					if((event->state & GDK_SHIFT_MASK) != 0)
					{
						window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
						window->app->tool_window.vector_control_core.release_func(
							window, x, y,
						pressure, &window->app->tool_window.vector_control_core, (void*)&button
						);
					}
					else if((state & GDK_CONTROL_MASK) != 0 ||
						(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
					{
						window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
						window->app->tool_window.vector_control_core.release_func(
							window, x,y,
								pressure, &window->app->tool_window.vector_control_core, (void*)&button
						);
					}
					else
					{
						window->app->tool_window.active_vector_brush[window->app->input]->release_func(
							window, x, y,
							pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)&button
						);
					}
				}
			}

			update_func(window, x, y, update_data);
			window->last_x = x,	window->last_y = y;

			goto func_end;
		}
	}

	// 筆圧が下限値未満なら修正
	if(pressure < MINIMUM_PRESSURE)
	{
		pressure = MINIMUM_PRESSURE;
	}

	// 手ブレ補正実行
	if((window->state & GDK_BUTTON1_MASK) != 0)
	{
#define DISPLAY_UPDATE_DISTANCE 4
		if(fabs(window->before_x-x0)+fabs(window->before_y-y0) > DISPLAY_UPDATE_DISTANCE)
		{
			update_window++;
			window->before_x = x0, window->before_y = y0;
		}

		if(window->transform != NULL)
		{
			TransformMotionNotifyCallBack(window, window->transform, x, y, &window->state);
			update_func(window, x0, y0, update_data);
			goto func_end;
		}
		else
		{
			window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
			if(window->app->tool_window.smoother.num_use != 0
				&& (window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) == 0)
			{
				if(window->app->tool_window.smoother.mode == SMOOTH_GAUSSIAN)
				{
					Smooth(&window->app->tool_window.smoother, &x, &y, event->time, window->rev_zoom);
				}
				else
				{
					if(AddAverageSmoothPoint(&window->app->tool_window.smoother,
						&x, &y, &pressure, window->rev_zoom) == FALSE)
					{
						window->flags &= ~(DRAW_WINDOW_UPDATE_ACTIVE_OVER);
						goto func_end;
					}

					window->before_cursor_x = window->app->tool_window.smoother.last_draw_point.x;
					window->before_cursor_y = window->app->tool_window.smoother.last_draw_point.y;
					x0 = ((x-window->width/2)*window->cos_value + (y-window->height/2)*window->sin_value) * window->zoom_rate
						+ window->rev_add_cursor_x;
					y0 = (- (x-window->width/2)*window->sin_value + (y-window->height/2)*window->cos_value) * window->zoom_rate
						+ window->rev_add_cursor_y;
					window->app->tool_window.smoother.last_draw_point.x = x0;
					window->app->tool_window.smoother.last_draw_point.y = y0;
				}
			}
		}
	}
	else if(window->transform != NULL)
	{
		goto func_end;
	}

	if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
	{
		window->app->tool_window.active_common_tool->motion_func(
			window, x, y, window->app->tool_window.active_common_tool,
			(void*)(&window->state) );
	}
	else
	{
		if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
			|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
		{
			window->app->tool_window.active_brush[window->app->input]->motion_func(
				window, x, y, pressure, window->app->tool_window.active_brush[window->app->input], (void*)(&window->state)
			);
		}
		else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
		{
			if((state & GDK_SHIFT_MASK) != 0)
			{
				window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
				window->app->tool_window.vector_control_core.motion_func(
					window, x, y, pressure, &window->app->tool_window.vector_control_core, (void*)(&window->state)
				);
				update_func = window->app->tool_window.vector_control_core.motion_update;
			}
			else if((state & GDK_CONTROL_MASK) != 0 ||
				(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
			{
				window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
				window->app->tool_window.vector_control_core.motion_func(
					window, x, y, pressure, &window->app->tool_window.vector_control_core, (void*)(&window->state)
				);
				update_func = window->app->tool_window.vector_control_core.motion_update;
			}
			else
			{
				window->app->tool_window.active_vector_brush[window->app->input]->motion_func(
					window, x, y, pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)(&window->state)
				);
			}
		}
		else
		{
			TextLayerMotionCallBack(window, x, y, state);
		}
	}

	window->state = (state & (~(GDK_BUTTON1_MASK))) | (window->state & GDK_BUTTON1_MASK);

	update_func(window, x0, y0, update_data);

func_end:
	if(update_window != 0)
	{
		GdkEvent *queued_event = NULL;

		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
				if(queued_event->any.type == GDK_EXPOSE)
				{
					gdk_event_free(queued_event);
					break;
				}
				else
				{
					gdk_event_free(queued_event);
				}
			}
			else
			{
				break;
			}
		}
	}

	// ここでflushしないと色変更ツール使用時に読み込みエラーになる…　なんで? (12年11月18日)
	(void)fflush(stdout);

	window->before_cursor_x = x0;
	window->before_cursor_y = y0;

	if(event->is_hint != FALSE)
	{
		gdk_event_request_motions(event);
	}

	return TRUE;
}

gboolean ButtonReleaseEvent(GtkWidget *widget, GdkEventButton *event, DRAW_WINDOW* window)
{
	brush_update_func update_func = DefaultToolUpdate;
	void *update_data = NULL;
	gdouble pressure;
	gdouble x, y;
#if GTK_MAJOR_VERSION >= 3
	GdkDevice *device;
	GdkInputSource source;
#endif

	// 画面更新用のデータを取得
	if(window->transform == NULL)
	{
		if((window->app->tool_window.flags & TOOL_USING_BRUSH) != 0)
		{
			if(window->active_layer->layer_type == TYPE_NORMAL_LAYER)
			{
				update_func = window->app->tool_window.active_brush[window->app->input]->motion_update;
				update_data = window->app->tool_window.active_brush[window->app->input]->brush_data;
			}
			else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
			{
				update_func = window->app->tool_window.active_vector_brush[window->app->input]->motion_update;
				update_data = window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			}
		}
		else
		{
			update_func = window->app->tool_window.active_common_tool->motion_update;
			update_data = window->app->tool_window.active_common_tool->tool_data;
		}
	}

	if((window->state & GDK_BUTTON1_MASK) == 0)
	{
		return TRUE;
	}

	window->cursor_x = (event->x - window->half_size) * window->cos_value
		- (event->y - window->half_size) * window->sin_value + window->add_cursor_x;
	window->cursor_y = (event->x - window->half_size) * window->sin_value
		+ (event->y - window->half_size) * window->cos_value + window->add_cursor_y;
	x = window->rev_zoom * window->cursor_x;
	y = window->rev_zoom * window->cursor_y;

	// 左右反転表示中ならばX座標を修正
	if((window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) != 0)
	{
		x = window->width - x;
		window->cursor_x = window->disp_layer->width - window->cursor_x;
	}

	// マウス以外なら筆圧を取得
#if GTK_MAJOR_VERSION <= 2
	if(event->device->source != GDK_SOURCE_MOUSE)
	{
		gdk_event_get_axis((GdkEvent*)event, GDK_AXIS_PRESSURE, &pressure);
	}
	else
#else
	device = gdk_event_get_device((GdkEvent*)event);
	source = gdk_device_get_source(device);
	if(gdk_device_get_axis(device, event->axes, GDK_AXIS_PRESSURE, &pressure) == FALSE)
#endif
	{	// マウスなら筆圧は最大に
		pressure = 1.0;
	}

	// 筆圧が下限値未満なら修正
	if(pressure < MINIMUM_PRESSURE)
	{
		pressure = MINIMUM_PRESSURE;
	}

	// 手ブレ補正データの初期化
	if(event->button == 1)
	{
		if(window->app->tool_window.smoother.num_use > 0 &&
			window->app->tool_window.smoother.mode != SMOOTH_GAUSSIAN)
		{
			FLOAT_T motion_x, motion_y, motion_pressure;
			FLOAT_T update_x, update_y;
			gboolean finish;

			if(AddAverageSmoothPoint(&window->app->tool_window.smoother, &x, &y,
				&pressure, 0) != FALSE)
			{
				if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
				{
					window->app->tool_window.active_brush[window->app->input]->motion_func(
						window, x, y, pressure,
							window->app->tool_window.active_brush[window->app->input], (void*)(&window->state)
					);
				}
				else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					if((event->state & GDK_SHIFT_MASK) != 0)
					{
						/*
						window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
						window->app->tool_window.vector_control_core.motion_func(
							window, x, y, pressure, &window->app->tool_window.vector_control_core, (void*)(&window->state)
						);*/
					}
					else if((event->state & GDK_CONTROL_MASK) != 0 ||
						(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
					{
						/*
						window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
						window->app->tool_window.vector_control_core.motion_func(
							window, x, y, motion_pressure,
								&window->app->tool_window.vector_control_core, (void*)(&window->state)
						);*/
					}
					else
					{
						window->app->tool_window.active_vector_brush[window->app->input]->motion_func(
							window, x, y, pressure,
								window->app->tool_window.active_vector_brush[window->app->input], (void*)(&window->state)
						);
					}
				}
				else
				{
					TextLayerMotionCallBack(window, x, y, event->state);
				}

				window->before_cursor_x = window->app->tool_window.smoother.last_draw_point.x;
				window->before_cursor_y = window->app->tool_window.smoother.last_draw_point.y;
				update_x = ((x-window->width/2)*window->cos_value + (y-window->height/2)*window->sin_value) * window->zoom_rate
					+ window->rev_add_cursor_x;
				update_y = (- (x-window->width/2)*window->sin_value + (y-window->height/2)*window->cos_value) * window->zoom_rate
					+ window->rev_add_cursor_y;
				update_func(window, update_x, update_y, update_data);
				window->app->tool_window.smoother.last_draw_point.x = update_x;
				window->app->tool_window.smoother.last_draw_point.y = update_y;
			}

			do
			{
				finish = AverageSmoothFlush(&window->app->tool_window.smoother,
					&motion_x, &motion_y, &motion_pressure);

				if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
				{
					window->app->tool_window.active_common_tool->motion_func(
						window, motion_x, motion_y, window->app->tool_window.active_common_tool,
							(void*)(&window->state) );
				}
				else
				{
					if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
						|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
					{
						window->app->tool_window.active_brush[window->app->input]->motion_func(
							window, motion_x, motion_y, motion_pressure,
								window->app->tool_window.active_brush[window->app->input], (void*)(&window->state)
						);
					}
					else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
					{
						if((event->state & GDK_SHIFT_MASK) != 0)
						{
							/*
							window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
							window->app->tool_window.vector_control_core.motion_func(
								window, motion_x, motion_y, motion_pressure, &window->app->tool_window.vector_control_core, (void*)(&window->state)
							);
							*/
						}
						else if((event->state & GDK_CONTROL_MASK) != 0 ||
							(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
						{
							/*
							window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
							window->app->tool_window.vector_control_core.motion_func(
								window, motion_x, motion_y, motion_pressure,
									&window->app->tool_window.vector_control_core, (void*)(&window->state)
							);
							*/
						}
						else
						{
							window->app->tool_window.active_vector_brush[window->app->input]->motion_func(
								window, motion_x, motion_y, motion_pressure,
									window->app->tool_window.active_vector_brush[window->app->input], (void*)(&window->state)
							);
						}
					}
					else
					{
						TextLayerMotionCallBack(window, motion_x, motion_y, event->state);
					}
				}

				window->before_cursor_x = window->app->tool_window.smoother.last_draw_point.x;
				window->before_cursor_y = window->app->tool_window.smoother.last_draw_point.y;
				update_x = ((motion_x-window->width/2)*window->cos_value + (motion_y-window->height/2)*window->sin_value) * window->zoom_rate
					+ window->rev_add_cursor_x;
				update_y = (- (motion_x-window->width/2)*window->sin_value + (motion_y-window->height/2)*window->cos_value) * window->zoom_rate
					+ window->rev_add_cursor_y;
				update_func(window, update_x, update_y, update_data);
				window->app->tool_window.smoother.last_draw_point.x = update_x;
				window->app->tool_window.smoother.last_draw_point.y = update_y;
			} while(finish == FALSE);
		}

		INIT_SMOOTHER(window->app->tool_window.smoother);

		// ボタン状態をリセット
		window->state &= ~(GDK_BUTTON1_MASK);

		// 座標を記憶
		window->last_x = x,	window->last_y = y;
	}

	if(window->transform != NULL)
	{
		window->transform->trans_point = TRANSFORM_POINT_NONE;
		update_func(window, event->x, event->y, update_data);
		goto func_end;
	}

	if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
	{
		window->app->tool_window.active_common_tool->release_func(
			window, x, y,
			window->app->tool_window.active_common_tool, (void*)event
		);
	}
	else
	{
		if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
			|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
		{
			window->app->tool_window.active_brush[window->app->input]->release_func(
				window, x, y, pressure,
				window->app->tool_window.active_brush[window->app->input], (void*)event
			);
		}
		else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
		{
			if((event->state & GDK_SHIFT_MASK) != 0)
			{
				window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
				window->app->tool_window.vector_control_core.release_func(
					window, x, y,
					pressure, &window->app->tool_window.vector_control_core, (void*)event
				);
			}
			else if((event->state & GDK_CONTROL_MASK) != 0 ||
				(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
			{
				window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
				window->app->tool_window.vector_control_core.release_func(
					window, x,y,
					pressure, &window->app->tool_window.vector_control_core, (void*)event
				);
			}
			else
			{
				window->app->tool_window.active_vector_brush[window->app->input]->release_func(
					window, x, y,
					pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)event
				);
			}
		}
		else if(window->active_layer->layer_type == TYPE_TEXT_LAYER)
		{
			TextLayerButtonReleaseCallBack(window, x, y, event);
		}
	}

	update_func(window, event->x, event->y, update_data);

func_end:
	window->before_cursor_x = event->x;
	window->before_cursor_y = event->y;

	window->state = event->state & ~(GDK_BUTTON1_MASK);

	return TRUE;
}

gboolean MouseWheelEvent(GtkWidget*widget, GdkEventScroll* event_info, DRAW_WINDOW* window)
{
	if((event_info->state & GDK_CONTROL_MASK) != 0)
	{
		switch(event_info->direction)
		{
		case GDK_SCROLL_UP:
			window->zoom += 10;
			if(window->zoom > MAX_ZOOM)
			{
				window->zoom = MAX_ZOOM;
			}

			// ナビゲーションの拡大縮小率スライダを動かして
				// 描画領域のリサイズ等を行う
			gtk_adjustment_set_value(window->app->navigation_window.zoom_slider, window->zoom);
			return TRUE;
		case GDK_SCROLL_DOWN:
			window->zoom -= 10;
			if(window->zoom < MIN_ZOOM)
			{
				window->zoom = MIN_ZOOM;
			}

			// ナビゲーションの拡大縮小率スライダを動かして
				// 描画領域のリサイズ等を行う
			gtk_adjustment_set_value(window->app->navigation_window.zoom_slider, window->zoom);
			return TRUE;
		}
	}

	return FALSE;
}

#if GTK_MAJOR_VERSION >= 3
/***************************************
* TouchEvent関数                       *
* タッチ操作時のコールバック関数       *
* 引数                                 *
* widget		: 描画領域ウィジェット *
* event_info	: イベントの内容       *
* window		: 描画領域の情報       *
* 返り値                               *
*	常にFALSE                          *
***************************************/
gboolean TouchEvent(GtkWidget* widget, GdkEventTouch* event_info, DRAW_WINDOW* window)
{
	// ツールウィンドウ
	TOOL_WINDOW *tool_window = &window->app->tool_window;
	// 画面更新用関数へのポインタ
	brush_update_func update_func = DefaultToolUpdate;
	// 画面更新用のデータ
	void *update_data = NULL;
	// 描画領域上の座標
	FLOAT_T real_x, real_y;
	// 回転後の座標
	FLOAT_T x, y;
	// 筆圧
	gdouble pressure;
	// 詳細データコピー用の一時保存
	void *temp_brush_data;

	// 描画領域ウィジェット上での座標を記憶
	real_x = event_info->x,	real_y = event_info->y;
	// カーソルの位置と描画領域拡大率100%時の座標を計算
	window->cursor_x = (real_x - window->half_size) * window->cos_value
		- (real_y - window->half_size) * window->sin_value + window->add_cursor_x;
	window->cursor_y = (real_x - window->half_size) * window->sin_value
		+ (real_y - window->half_size) * window->cos_value + window->add_cursor_y;
	x = window->rev_zoom * window->cursor_x;
	y = window->rev_zoom * window->cursor_y;

	// 左右反転表示中ならX座標を修正
	if((window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE) != 0)
	{
		x = window->width - x;
		window->cursor_x = window->disp_layer->width - window->cursor_x;
	}

	// タッチイベントで描画するなら
	if((window->app->flags & APPLICATION_DRAW_WITH_TOUCH) != 0)
	{
		// タッチの開始、指の移動、終了を判定
		switch(event_info->type)
		{
		case GDK_TOUCH_BEGIN:	// タッチ開始
			{
				GdkEventButton button = {0};
				button.button = 1;
				button.state = GDK_BUTTON1_MASK | event_info->state;
				button.device = event_info->device;

				window->state |= GDK_BUTTON1_MASK;
				window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

				// 最大数以上のタッチイベントが発生したら終了
				if(tool_window->num_touch >= MAX_TOUCH)
				{
					return FALSE;
				}

				// 現在の指のデータアドレスを記憶
				tool_window->fingers[window->app->tool_window.num_touch] =
					event_info->sequence;

				// 筆圧に対応していたら取得
				if(gdk_device_get_axis(gdk_event_get_device((GdkEvent*)event_info),
					event_info->axes, GDK_AXIS_PRESSURE, &pressure) == FALSE)
				{	// 非対応
					pressure = (window->active_layer->layer_type == TYPE_NORMAL_LAYER) ? 1.0 : 0.5;
				}
				else
				{	// 対応時、最小値を下回っていたら修正
					if(pressure < MINIMUM_PRESSURE)
					{
						pressure = MINIMUM_PRESSURE;
					}
				}

				// 手ブレ補正を実行
				if(tool_window->smoother.num_use != 0)
				{
					if(tool_window->smoother.mode == SMOOTH_GAUSSIAN)
					{
						Smooth(&tool_window->touch_smoother[tool_window->num_touch],
							&x, &y, event_info->time, window->rev_zoom);
					}
					else
					{
						tool_window->touch_smoother[tool_window->num_touch].last_draw_point.x = real_x;
						tool_window->touch_smoother[tool_window->num_touch].last_draw_point.y = real_y;
						(void)AddAverageSmoothPoint(&tool_window->touch_smoother[tool_window->num_touch],
							&x, &y, &pressure, window->rev_zoom);
					}
				}

				if(window->transform != NULL)
				{
					TransformButtonPress(window->transform, x, y);
					return FALSE;
				}

				if((tool_window->flags & TOOL_USING_BRUSH) == 0)
				{
					if(tool_window->num_touch == 0)
					{
						tool_window->active_common_tool->press_func(
							window, x, y, tool_window->active_common_tool, (void*)&button);
						update_func = tool_window->active_common_tool->button_update;
						update_data = tool_window->active_common_tool->tool_data;
					}
					tool_window->num_touch++;
				}
				else
				{
					if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
						|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
					{
						if((event_info->state & GDK_CONTROL_MASK)== 0)
						{
							if((event_info->state & GDK_SHIFT_MASK) == 0)
							{
								temp_brush_data = tool_window->touch[tool_window->num_touch].brush_data;
								tool_window->touch[tool_window->num_touch] = *tool_window->active_brush[INPUT_PEN];
								tool_window->touch[tool_window->num_touch].brush_data = temp_brush_data;
								(void)memcpy(temp_brush_data, tool_window->active_brush[INPUT_PEN]->brush_data,
									tool_window->active_brush[INPUT_PEN]->detail_data_size);

								tool_window->active_brush[INPUT_PEN]->press_func(
									window, x, y, pressure, tool_window->active_brush[INPUT_PEN],
									tool_window->touch[tool_window->num_touch].brush_data
								);
								update_func = tool_window->active_brush[INPUT_PEN]->button_update;
								update_data = tool_window->touch[tool_window->num_touch].brush_data;

								tool_window->num_touch++;
							}
							else
							{
								if(tool_window->num_touch == 0)
								{
									// シフトキーが押されているので直線で処理
									FLOAT_T d;
									FLOAT_T dx = x - window->last_x;
									FLOAT_T dy = y - window->last_y;
									d = sqrt(dx*dx + dy*dy);
									dx = dx / d,	dy = dy / d;

									window->last_x += dx,	window->last_y += dy;
									d -= 1;

									if((window->flags & DRAW_WINDOW_DRAWING_STRAIGHT) == 0)
									{
										temp_brush_data = tool_window->touch[tool_window->num_touch].brush_data;
										tool_window->touch[tool_window->num_touch] = *tool_window->active_brush[INPUT_PEN];
										tool_window->touch[tool_window->num_touch].brush_data = temp_brush_data;
										(void)memcpy(temp_brush_data, tool_window->active_brush[INPUT_PEN]->brush_data,
											tool_window->active_brush[INPUT_PEN]->detail_data_size);

										tool_window->active_brush[INPUT_PEN]->press_func(
											window, x, y, pressure, tool_window->active_brush[INPUT_PEN],
											tool_window->touch[tool_window->num_touch].brush_data
										);

										window->flags |= DRAW_WINDOW_DRAWING_STRAIGHT;
									}

									while(d >= 1)
									{
										window->last_x += dx,	window->last_y += dy;

										tool_window->active_brush[INPUT_PEN]->motion_func(
											window, window->last_x, window->last_y, pressure,
											&tool_window->touch[tool_window->num_touch], (void*)(&window->state));

										d -= 1;
									}
									tool_window->active_brush[INPUT_PEN]->motion_func(
										window, x, y, pressure, &tool_window->touch[tool_window->num_touch], (void*)(&window->state));

									window->state &= ~(GDK_BUTTON1_MASK);
									window->last_x = x,	window->last_y = y;
									window->last_pressure = pressure;

									gtk_widget_queue_draw(window->window);
								}
							}
						}
						else
						{
							if(tool_window->num_touch == 0)
							{
								tool_window->color_picker_core.press_func(
									window, x, y, &tool_window->color_picker_core, (void*)&button);
							}
						}
					}
					else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
					{
						if(tool_window->num_touch == 0)
						{
							if((event_info->state & GDK_SHIFT_MASK) != 0)
							{
								tool_window->vector_control.mode = CONTROL_POINT_DELETE;
								tool_window->vector_control_core.press_func(
									window, x, y, pressure, &tool_window->vector_control_core, (void*)&button);
							}
							else if((event_info->state & GDK_CONTROL_MASK) != 0)
							{
								tool_window->vector_control.mode = CONTROL_POINT_MOVE;
								tool_window->vector_control_core.press_func(
									window, x, y, pressure, &tool_window->vector_control_core, (void*)&button);
							}
							else
							{
								tool_window->active_vector_brush[INPUT_PEN]->press_func(
									window, x, y, pressure, tool_window->active_vector_brush[INPUT_PEN], (void*)&button);
								update_func = tool_window->active_vector_brush[INPUT_PEN]->button_update;
								update_data = tool_window->active_vector_brush[INPUT_PEN]->brush_data;
							}

							tool_window->num_touch++;
						}
					}
					else
					{
						if(tool_window->num_touch == 0)
						{
							TextLayerButtonPressCallBack(window, x, y, &button);
							tool_window->num_touch++;
						}
					}
				}

				update_func(window, real_x, real_y, update_data);

				window->before_cursor_x = real_x;
				window->before_cursor_y = real_y;
			}
			break;
			// スライド操作
		case GDK_TOUCH_UPDATE:
			{
				int finger_id;
				int update_window = 0;

				// どの指のイベントか判定
				for(finger_id=0; finger_id<MAX_TOUCH; finger_id++)
				{
					if(tool_window->fingers[finger_id] == event_info->sequence)
					{
						break;
					}
				}
				if(finger_id == MAX_TOUCH)
				{
					return FALSE;
				}

				// 筆圧に対応していたら取得
				if(gdk_device_get_axis(gdk_event_get_device((GdkEvent*)event_info),
					event_info->axes, GDK_AXIS_PRESSURE, &pressure) == FALSE)
				{	// 非対応
					pressure = (window->active_layer->layer_type == TYPE_NORMAL_LAYER) ? 1.0 : 0.5;
				}
				else
				{	// 対応時、最小値を下回っていたら修正
					if(pressure < MINIMUM_PRESSURE)
					{
						pressure = MINIMUM_PRESSURE;
					}
				}

				if((window->state & GDK_BUTTON1_MASK) != 0)
				{
#define DISPLAY_UPDATE_DISTANCE 4
					if(fabs(window->before_x-real_x)+fabs(window->before_y-real_y)
						> DISPLAY_UPDATE_DISTANCE)
					{
						update_window++;
						window->before_x = real_x,	window->before_y = real_y;
					}

					if(window->transform != NULL)
					{
						TransformMotionNotifyCallBack(window, window->transform,
							x, y, (void*)(&window->state));
						update_func(window, real_x, real_y, update_data);
						goto motion_end;
					}
					else
					{
						window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
						if(tool_window->smoother.num_use != 0
							&& (tool_window->vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) == 0)
						{
							if(tool_window->smoother.mode == SMOOTH_GAUSSIAN)
							{
								Smooth(&tool_window->touch_smoother[finger_id], &x, &y,
									event_info->time, window->rev_zoom);
							}
							else
							{
								if(AddAverageSmoothPoint(&tool_window->touch_smoother[finger_id],
									&x, &y, &pressure, window->rev_zoom) == FALSE)
								{
									window->flags &= ~(DRAW_WINDOW_UPDATE_ACTIVE_OVER);
									goto motion_end;
								}

								window->before_cursor_x = tool_window->touch_smoother[finger_id].last_draw_point.x;
								window->before_cursor_y = tool_window->touch_smoother[finger_id].last_draw_point.y;
								tool_window->touch_smoother[finger_id].last_draw_point.x = real_x;
								tool_window->touch_smoother[finger_id].last_draw_point.y = real_y;
							}
						}
					}

					if((tool_window->flags & TOOL_USING_BRUSH) == 0)
					{
						tool_window->active_common_tool->motion_func(
							window, x, y, tool_window->active_common_tool, (void*)(&window->state));
						update_func = tool_window->active_common_tool->motion_update;
						update_data = tool_window->active_common_tool->tool_data;
					}
					else
					{
						if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
							|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
						{
							tool_window->active_brush[INPUT_PEN]->motion_func(
								window, x, y, pressure, &tool_window->touch[finger_id], (void*)(&window->state));
							update_func = tool_window->active_brush[INPUT_PEN]->motion_update;
							update_data = tool_window->touch[finger_id].brush_data;
						}
						else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
						{
							if((event_info->state & GDK_SHIFT_MASK) != 0)
							{
								tool_window->vector_control.mode = CONTROL_POINT_DELETE;
								tool_window->vector_control_core.motion_func(
									window, x, y, pressure, &tool_window->vector_control_core, (void*)(&window->state));
								update_func = tool_window->vector_control_core.motion_update;
								update_data = tool_window->vector_control_core.brush_data;
							}
							else if((event_info->state & GDK_CONTROL_MASK) != 0
								|| (tool_window->vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
							{
								tool_window->vector_control.mode = CONTROL_POINT_MOVE;
								tool_window->vector_control_core.motion_func(
									window, x, y, pressure, &tool_window->vector_control_core, (void*)(&window->state));
								update_func = tool_window->vector_control_core.motion_update;
								update_data = tool_window->vector_control_core.brush_data;
							}
							else
							{
								tool_window->active_vector_brush[INPUT_PEN]->motion_func(
									window, x, y, pressure, tool_window->active_vector_brush[INPUT_PEN], (void*)(&window->state));
								update_func = tool_window->active_vector_brush[INPUT_PEN]->motion_update;
								update_data = tool_window->active_vector_brush[INPUT_PEN]->brush_data;
							}
						}
						else
						{
							TextLayerMotionCallBack(window, x, y, window->state);
						}
					}
				}

				window->state = (event_info->state & (~(GDK_BUTTON1_MASK)))
					| (window->state & GDK_BUTTON1_MASK);

motion_end:
				if(update_window != 0)
				{
					GdkEvent *queued_event = NULL;

					while(gdk_events_pending() != FALSE)
					{
						queued_event = gdk_event_get();
						gtk_main_iteration();
						if(queued_event != NULL)
						{
							if(queued_event->any.type == GDK_EXPOSE)
							{
								gdk_event_free(queued_event);
								break;
							}
							else
							{
								gdk_event_free(queued_event);
							}
						}
						else
						{
							break;
						}
					}
				}

				window->before_cursor_x = real_x;
				window->before_cursor_y = real_y;
			}
			break;
		case GDK_TOUCH_END:
		case GDK_TOUCH_CANCEL:
			{
				int finger_id;
				GdkEventButton button = {0};
				button.button = 1;
				button.state = GDK_BUTTON1_MASK | event_info->state;
				button.device = event_info->device;

				// どの指のイベントか判定
				for(finger_id=0; finger_id<MAX_TOUCH; finger_id++)
				{
					if(tool_window->fingers[finger_id] == event_info->sequence)
					{
						break;
					}
				}
				if(finger_id == MAX_TOUCH)
				{
					return FALSE;
				}

				// 画面更新用のデータを取得
				if(window->transform == NULL)
				{
					if((tool_window->flags & TOOL_USING_BRUSH) != 0)
					{
						if(window->active_layer->layer_type == TYPE_NORMAL_LAYER)
						{
							update_func = tool_window->active_brush[INPUT_PEN]->motion_update;
							update_data = tool_window->active_brush[INPUT_PEN]->brush_data;
						}
						else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
						{
							update_func = tool_window->active_vector_brush[INPUT_PEN]->motion_update;
							update_data = tool_window->active_vector_brush[INPUT_PEN]->brush_data;
						}
					}
					else
					{
						update_func = tool_window->active_common_tool->motion_update;
						update_data = tool_window->active_common_tool->tool_data;
					}
				}

				// 手ブレ補正が平均法なら残り座標分を処理
				if(tool_window->smoother.num_use != 0
					&& tool_window->smoother.mode != SMOOTH_GAUSSIAN)
				{
					FLOAT_T motion_x, motion_y, motion_pressure;
					FLOAT_T update_x, update_y;
					gboolean finish;

					if(AddAverageSmoothPoint(&tool_window->touch_smoother[finger_id],
						&x, &y, &pressure, 0) != FALSE)
					{
						if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
							|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
						{
							tool_window->active_brush[INPUT_PEN]->motion_func(
								window, x, y, pressure, &tool_window->touch[finger_id], (void*)(&window->state));
						}
						else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
						{
							if((event_info->state & GDK_SHIFT_MASK) != 0)
							{
							}
							else if((event_info->state & GDK_CONTROL_MASK) != 0
								|| (tool_window->vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
							{
							}
							else
							{
								tool_window->active_vector_brush[INPUT_PEN]->motion_func(
									window, x, y, pressure, tool_window->active_vector_brush[INPUT_PEN], (void*)(&window->state));
							}
						}
						else
						{
							TextLayerMotionCallBack(window, x, y, event_info->state);
						}

						window->before_cursor_x = tool_window->touch_smoother[finger_id].last_draw_point.x;
						window->before_cursor_y = tool_window->touch_smoother[finger_id].last_draw_point.y;
						update_x = ((x-window->width/2)*window->cos_value + (y-window->height/2)*window->sin_value)
							* window->zoom_rate + window->rev_add_cursor_x;
						update_y = (- (x-window->width/2)*window->sin_value + (y-window->height/2)*window->cos_value)
							* window->zoom_rate + window->rev_add_cursor_y;
						update_func(window, update_x, update_y, update_data);
						tool_window->touch_smoother[finger_id].last_draw_point.x = update_x;
						tool_window->touch_smoother[finger_id].last_draw_point.y = update_y;
					}

					do
					{
						finish = AverageSmoothFlush(&tool_window->touch_smoother[finger_id],
							&motion_x, &motion_y, &motion_pressure);

						if((tool_window->flags & TOOL_USING_BRUSH) == 0)
						{
							tool_window->active_common_tool->motion_func(
								window, motion_x, motion_y, tool_window->active_common_tool, (void*)(&window->state));
						}
						else
						{
							if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
								|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
							{
								tool_window->active_brush[INPUT_PEN]->motion_func(
									window, motion_x, motion_y, motion_pressure, &tool_window->touch[finger_id], (void*)(&window->state));
							}
							else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
							{
								if((event_info->state & GDK_SHIFT_MASK) != 0)
								{
								}
								else if((event_info->state & GDK_CONTROL_MASK) != 0
									|| (tool_window->vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
								{
								}
								else
								{
									tool_window->active_vector_brush[INPUT_PEN]->motion_func(
										window, motion_x, motion_y, motion_pressure,
											tool_window->active_vector_brush[INPUT_PEN], (void*)(&window->state));
								}
							}
							else
							{
								TextLayerMotionCallBack(window, motion_x, motion_y, event_info->state);
							}
						}

						window->before_cursor_x = tool_window->touch_smoother[finger_id].last_draw_point.x;
						window->before_cursor_y = tool_window->touch_smoother[finger_id].last_draw_point.y;
						update_x = ((motion_x-window->width/2)*window->cos_value + (motion_y-window->height/2)*window->sin_value) * window->zoom_rate
							+ window->rev_add_cursor_x;
						update_y = (- (motion_x-window->width/2)*window->sin_value + (motion_y-window->height/2)*window->cos_value) * window->zoom_rate
							+ window->rev_add_cursor_y;
						update_func(window, update_x, update_y, update_data);
						tool_window->touch_smoother[finger_id].last_draw_point.x = update_x;
						tool_window->touch_smoother[finger_id].last_draw_point.y = update_y;
					} while(finish == FALSE);
				}

				INIT_SMOOTHER(tool_window->touch_smoother[finger_id]);

				window->last_x = x,	window->last_y = y;

				if(window->transform != NULL)
				{
					window->transform->trans_point = TRANSFORM_POINT_NONE;
					update_func(window, real_x, real_y, update_data);
					goto release_end;
				}

				if((tool_window->flags & TOOL_USING_BRUSH) == 0)
				{
					tool_window->active_common_tool->release_func(
						window, x, y, tool_window->active_common_tool, (void*)&button);
				}
				else
				{
					if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
						|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
					{
						tool_window->active_brush[INPUT_PEN]->release_func(
							window, x, y, pressure, &tool_window->touch[finger_id], (void*)&button);
					}
					else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
					{
						if((event_info->state & GDK_SHIFT_MASK) != 0)
						{
							tool_window->vector_control.mode = CONTROL_POINT_DELETE;
							tool_window->vector_control_core.release_func(
								window, x, y, pressure, &tool_window->vector_control_core, (void*)&button);
						}
						else if((event_info->state & GDK_CONTROL_MASK) != 0
							|| (tool_window->vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
						{
							tool_window->vector_control.mode = CONTROL_POINT_MOVE;
							tool_window->vector_control_core.release_func(
								window, x, y, pressure, &tool_window->vector_control_core, (void*)&button);
						}
						else
						{
							tool_window->active_vector_brush[INPUT_PEN]->release_func(
								window, x, y, pressure, tool_window->active_vector_brush[INPUT_PEN], (void*)&button);
						}
					}
				}

				update_func(window, real_x, real_y, update_data);

release_end:
				window->before_cursor_x = real_x;
				window->before_cursor_y = real_y;

				window->state = event_info->state & ~(GDK_BUTTON1_MASK);

				tool_window->fingers[finger_id] = NULL;
				if(tool_window->num_touch > 0)
				{
					tool_window->num_touch--;
				}
			}
			break;
		}
	}
	else
	{
		int finger_id;

		switch(event_info->type)
		{
		case GDK_TOUCH_BEGIN:
			{
				TOUCH_POINT *point;

				for(finger_id=0; finger_id<MAX_TOUCH; finger_id++)
				{
					if(tool_window->fingers[finger_id] == NULL)
					{
						tool_window->fingers[finger_id] = event_info->sequence;
						break;
					}
				}
				if(finger_id == MAX_TOUCH)
				{
					return FALSE;
				}

				point = (TOUCH_POINT*)tool_window->touch[finger_id].brush_data;
				point->x = event_info->x,	point->y = event_info->y;

				tool_window->num_touch++;

				if(tool_window->num_touch >= 2)
				{
					TOUCH_POINT *other_point;
					FLOAT_T *d;
					int other_finger;

					for(other_finger=0; other_finger<MAX_TOUCH; other_finger++)
					{
						if(other_finger != finger_id &&
							tool_window->fingers[other_finger] != NULL)
						{
							break;
						}
					}
					if(other_finger == MAX_TOUCH)
					{
						return FALSE;
					}

					other_point = (TOUCH_POINT*)tool_window->touch[other_finger].brush_data;

					d = (FLOAT_T*)tool_window->touch[5].brush_data;
					*d = sqrt((point->x-other_point->x)*(point->x-other_point->x)
						+ (point->y-other_point->y)*(point->y-other_point->y));
				}
			}
		case GDK_TOUCH_UPDATE:
			{
				for(finger_id=0; finger_id<MAX_TOUCH; finger_id++)
				{
					if(tool_window->fingers[finger_id] == event_info->sequence)
					{
						break;
					}
				}
				if(finger_id == MAX_TOUCH)
				{
					return FALSE;
				}

				if(tool_window->num_touch == 1)
				{
					TOUCH_POINT *point = (TOUCH_POINT*)tool_window->touch[finger_id].brush_data;
					GtkWidget *scroll_bar;
					FLOAT_T dx, dy;
					dx = event_info->x - point->x,	dy = event_info->y - point->y;

					scroll_bar = gtk_scrolled_window_get_hscrollbar(
						GTK_SCROLLED_WINDOW(window->scroll));
					gtk_range_set_value(GTK_RANGE(scroll_bar),
						gtk_range_get_value(GTK_RANGE(scroll_bar)) + dx);
					scroll_bar = gtk_scrolled_window_get_vscrollbar(
						GTK_SCROLLED_WINDOW(window->scroll));
					gtk_range_set_value(GTK_RANGE(scroll_bar),
						gtk_range_get_value(GTK_RANGE(scroll_bar)) + dy);
				}
				else if(tool_window->num_touch == 2)
				{
					TOUCH_POINT *point = (TOUCH_POINT*)tool_window->touch[finger_id].brush_data;
					TOUCH_POINT *other_point;
					FLOAT_T *before_distance = (FLOAT_T*)tool_window->touch[5].brush_data;
					FLOAT_T distance;
					int other_finger;

					for(other_finger=0; other_finger<MAX_TOUCH; other_finger++)
					{
						if(other_finger != finger_id &&
							tool_window->fingers[other_finger] != NULL)
						{
							break;
						}
					}
					if(other_finger == MAX_TOUCH)
					{
						return FALSE;
					}

					other_point = (TOUCH_POINT*)tool_window->touch[other_finger].brush_data;
					distance = sqrt((point->x-other_point->x)*(point->x-other_point->x)
						+ (point->y-other_point->y)*(point->y-other_point->y));
					if(fabs(*before_distance - distance) >= 1)
					{
						int new_zoom = (int)window->zoom + (int)(distance - *before_distance);
						if(new_zoom > MAX_ZOOM)
						{
							new_zoom = MAX_ZOOM;
						}
						else if(new_zoom < MIN_ZOOM)
						{
							new_zoom = MIN_ZOOM;
						}

						window->zoom = (uint16)new_zoom;

						gtk_adjustment_set_value(window->app->navigation_window.zoom_slider,
							new_zoom);

						*before_distance = distance;
					}
				}
			}
			break;
		case GDK_TOUCH_END:
		case GDK_TOUCH_CANCEL:
			for(finger_id=0; finger_id<MAX_TOUCH; finger_id++)
			{
				if(tool_window->fingers[finger_id] == event_info->sequence)
				{
					tool_window->fingers[finger_id] = NULL;
					if(tool_window->num_touch > 0)
					{
						tool_window->num_touch--;
					}
					return FALSE;
				}
			}
			break;
		}
	}

	return FALSE;
}
#endif

static gboolean BrushChainKeyTimeout(TOOL_WINDOW* tool)
{
	tool->flags &= ~(TOOL_BUTTON_STOPPED);
	return FALSE;
}

/***************************************************************
* KeyPressEvent関数                                            *
* キーボードが押された時に呼び出されるコールバック関数         *
* 引数                                                         *
* widget	: キーボードが押された時にアクティブなウィジェット *
* event		: キーボードの情報                                 *
* data		: アプリケーションの情報                           *
* 返り値                                                       *
*	常にFALSE                                                  *
***************************************************************/
gboolean KeyPressEvent(
	GtkWidget *widget,
	GdkEventKey *event,
	void *data
)
{
	APPLICATION* application = (APPLICATION*)data;
	DRAW_WINDOW* window = NULL;
	int x, y;
	int end_flag = 0;

	if(application->window_num == 0)
	{
		if(((event->state & ~(GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK))
			& (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) == 0)
		{
			for(y=0; y<COMMON_TOOL_TABLE_HEIGHT && end_flag == 0; y++)
			{
				for(x=0; x<COMMON_TOOL_TABLE_WIDTH && end_flag == 0; x++)
				{
					if(application->tool_window.common_tools[y][x].hot_key ==
						toupper((int)event->keyval))
					{
						gtk_toggle_button_set_active(
							GTK_TOGGLE_BUTTON(application->tool_window.common_tools[y][x].button), TRUE);
						end_flag++;
					}
				}
			}

			for(y=0; y<BRUSH_TABLE_HEIGHT && end_flag == 0; y++)
			{
				for(x=0; x<BRUSH_TABLE_WIDTH && end_flag == 0; x++)
				{
					if(application->tool_window.brushes[y][x].hot_key ==
						toupper((int)event->keyval))
					{
						gtk_toggle_button_set_active(
							GTK_TOGGLE_BUTTON(application->tool_window.brushes[y][x].button), TRUE);
						end_flag++;
					}
				}
			}
		}

		return FALSE;
	}

	window = GetActiveDrawWindow(application);

	if(window->active_layer->layer_type == TYPE_3D_LAYER)
	{
		return FALSE;
	}

	// レイヤーの名前変更中なら終了
	if((window->active_layer->widget->flags & LAYER_WINDOW_IN_CHANGE_NAME) != 0)
	{
		return FALSE;
	}

	if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
		|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
	{
		if(event->type == GDK_KEY_PRESS)
		{
			int i;

			if(window->app->tool_window.active_brush[window->app->input]->key_press_func != NULL)
			{
				window->app->tool_window.active_brush[window->app->input]->key_press_func(
					window, event,
					window->app->tool_window.active_brush[window->app->input]->brush_data
				);
				return FALSE;
			}

			if(application->tool_window.brush_chain.key == event->keyval)
			{
#define BRUSH_CHAIN_TIMEOUT_INTERVAL (700)
				BRUSH_CHAIN_ITEM *item = (BRUSH_CHAIN_ITEM*)application->tool_window.brush_chain.chains->buffer[application->tool_window.brush_chain.current];
				char *brush_name = NULL;

				for(i=0; i<(int)item->names->num_data; i++)
				{
					if(strcmp(application->tool_window.active_brush[INPUT_PEN]->name, (char*)item->names->buffer[i]) == 0)
					{
						brush_name = (char*)((i >= item->names->num_data - 1) ? item->names->buffer[0] : item->names->buffer[i+1]);
						for(y=0; y<BRUSH_TABLE_HEIGHT; y++)
						{
							for(x=0; x<BRUSH_TABLE_WIDTH; x++)
							{
								if(strcmp(application->tool_window.brushes[y][x].name, brush_name) == 0)
								{
									application->tool_window.active_brush[application->input] = &application->tool_window.brushes[y][x];
									SetActiveBrushTableButton(&application->tool_window, (void*)&application->tool_window.brushes[y][x]);
									application->tool_window.flags |= TOOL_USING_BRUSH;
									if(application->tool_window.detail_ui != NULL)
									{
										gtk_widget_destroy(application->tool_window.detail_ui);
									}
									application->tool_window.detail_ui = application->tool_window.brushes[y][x].create_detail_ui(application, &application->tool_window.brushes[y][x]);
									gtk_scrolled_window_add_with_viewport(
										GTK_SCROLLED_WINDOW(application->tool_window.detail_ui_scroll), application->tool_window.detail_ui);
									gtk_widget_show_all(application->tool_window.detail_ui);
									application->tool_window.flags |= TOOL_BUTTON_STOPPED;
									(void)g_timeout_add(BRUSH_CHAIN_TIMEOUT_INTERVAL, (GSourceFunc)BrushChainKeyTimeout, &application->tool_window);
									return TRUE;
								}
							}
						}
					}
				}

				if(brush_name == NULL)
				{
					for(y=0; y<BRUSH_TABLE_HEIGHT && brush_name == NULL; y++)
					{
						for(x=0; x<BRUSH_TABLE_WIDTH; x++)
						{
							if(strcmp(application->tool_window.brushes[y][x].name, (char*)item->names->buffer[0]) == 0)
							{
								application->tool_window.active_brush[application->input] = &application->tool_window.brushes[y][x];
								SetActiveBrushTableButton(&application->tool_window, (void*)&application->tool_window.brushes[y][x]);
								application->tool_window.flags |= TOOL_USING_BRUSH;
								if(application->tool_window.detail_ui != NULL)
								{
									gtk_widget_destroy(application->tool_window.detail_ui);
								}
								application->tool_window.detail_ui = application->tool_window.brushes[y][x].create_detail_ui(application, &application->tool_window.brushes[y][x]);
								gtk_scrolled_window_add_with_viewport(
									GTK_SCROLLED_WINDOW(application->tool_window.detail_ui_scroll), application->tool_window.detail_ui);
								gtk_widget_show_all(application->tool_window.detail_ui);
								application->tool_window.flags |= TOOL_BUTTON_STOPPED;
								(void)g_timeout_add(BRUSH_CHAIN_TIMEOUT_INTERVAL, (GSourceFunc)BrushChainKeyTimeout, &application->tool_window);
								return TRUE;
							}
						}
					}
				}
			}

			if(application->tool_window.brush_chain.change_key == event->keyval)
			{
				application->tool_window.brush_chain.current = (application->tool_window.brush_chain.current+1) % application->tool_window.brush_chain.chains->num_data;
			}

			if((event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) == 0)
			{
				if(((window->state & ~(GDK_BUTTON1_MASK | GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK))
					& (GDK_BUTTON1_MASK | GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) == 0)
				{
					for(y=0; y<COMMON_TOOL_TABLE_HEIGHT && end_flag == 0; y++)
					{
						for(x=0; x<COMMON_TOOL_TABLE_WIDTH && end_flag == 0; x++)
						{
							if(application->tool_window.common_tools[y][x].hot_key ==
								toupper((int)event->keyval))
							{
								gtk_toggle_button_set_active(
									GTK_TOGGLE_BUTTON(application->tool_window.common_tools[y][x].button), TRUE);
								end_flag++;
							}
						}
					}

					for(y=0; y<BRUSH_TABLE_HEIGHT && end_flag == 0; y++)
					{
						for(x=0; x<BRUSH_TABLE_WIDTH && end_flag == 0; x++)
						{
							if(application->tool_window.brushes[y][x].hot_key ==
								toupper((int)event->keyval))
							{
								gtk_toggle_button_set_active(
									GTK_TOGGLE_BUTTON(application->tool_window.brushes[y][x].button), TRUE);
								end_flag++;
							}
						}
					}
				}
			}
		}
		else if(event->type == GDK_KEY_RELEASE)
		{
#if GTK_MAJOR_VERSION <= 2
			if(event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R)
#else
			if(event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_R)
#endif
			{
				if((window->flags & DRAW_WINDOW_DRAWING_STRAIGHT) != 0)
				{
					GdkEventButton button = {0};
					button.button = 1;
					button.state = GDK_BUTTON1_MASK | window->state;

					window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;

					window->app->tool_window.active_brush[window->app->input]->release_func(
						window, window->last_x, window->last_y, window->last_pressure,
							window->app->tool_window.active_brush[window->app->input], (void*)&button);

					window->flags &= ~(DRAW_WINDOW_DRAWING_STRAIGHT);
					INIT_SMOOTHER(window->app->tool_window.smoother);
				}
			}
		}
	}
	else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
	{
		if(event->type == GDK_KEY_PRESS)
		{
			if(window->app->tool_window.active_vector_brush[window->app->input]->key_press_func != NULL)
			{
				window->app->tool_window.active_vector_brush[window->app->input]->key_press_func(
					window, event,
					window->app->tool_window.active_vector_brush[window->app->input]->brush_data
				);
			}

			if((event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) == 0)
			{
				if(((window->state & ~(GDK_BUTTON1_MASK | GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK))
					& (GDK_BUTTON1_MASK | GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) == 0)
				{
					for(y=0; y<COMMON_TOOL_TABLE_HEIGHT && end_flag == 0; y++)
					{
						for(x=0; x<COMMON_TOOL_TABLE_WIDTH && end_flag == 0; x++)
							{
							if(application->tool_window.common_tools[y][x].hot_key ==
								toupper((int)event->keyval))
							{
								gtk_toggle_button_set_active(
									GTK_TOGGLE_BUTTON(application->tool_window.common_tools[y][x].button), TRUE);
								end_flag++;
							}
						}
					}

					for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT && end_flag == 0; y++)
					{
						for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH && end_flag == 0; x++)
						{
							if(application->tool_window.vector_brushes[y][x].hot_key ==
								toupper((int)event->keyval))
							{
								gtk_toggle_button_set_active(
									GTK_TOGGLE_BUTTON(application->tool_window.vector_brushes[y][x].button), TRUE);
								end_flag++;
							}
						}
					}
				}
			}
		}
		else
		{
			if((window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
			{
				GdkEventButton button = {0};
				gdouble rev_zoom = 1.0 / (window->zoom * 0.01);
				button.button = 1;

				window->app->tool_window.vector_control_core.release_func(
					window, window->cursor_x * rev_zoom, window->cursor_y * rev_zoom, 0,
						&window->app->tool_window.vector_control_core, &button);
			}
		}
	}

	// 画面更新
	gtk_widget_queue_draw(window->window);

	window->state = (event->state & ~(GDK_BUTTON1_MASK)) | (window->state & GDK_BUTTON1_MASK);

	return FALSE;
}

/*********************************************************
* EnterNotifyEvent関数                                   *
* 描画領域内にマウスカーソルが入った際に呼び出される関数 *
* 引数                                                   *
* widget		: 描画領域のウィジェット                 *
* event_info	: イベントの情報                         *
* window		: 描画領域の情報                         *
* 返り値                                                 *
*	常にFALSE                                            *
*********************************************************/
gboolean EnterNotifyEvent(GtkWidget*widget, GdkEventCrossing* event_info, DRAW_WINDOW* window)
{
#ifndef _WIN32
	// 入力デバイスの設定を変更
	GList *device_list = gdk_devices_list();
	int counter = 0;

	while(device_list != NULL)
	{
		if(window->app->set_input_modes[counter] != FALSE)
		{
			gdk_device_set_mode((GdkDevice*)device_list->data, GDK_MODE_SCREEN);
		}
		gdk_device_set_source((GdkDevice*)device_list->data, window->app->input_sources[counter]);

		counter++;
		device_list = device_list->next;
	}
#else
# if GDK_MAJOR_VERSION <= 2
	GList *device_list;
	GdkDevice *device;

	device_list = gdk_devices_list();
	while(device_list != NULL)
	{
		device = GDK_DEVICE(device_list->data);
		gdk_pointer_grab(widget->window, FALSE,
			GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK
				| GDK_BUTTON_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK, NULL, NULL, event_info->time
		);
		device_list = device_list->next;
	}
# else
	GdkDevice *device = gdk_device_manager_get_client_pointer(
		gdk_display_get_device_manager(gdk_display_get_default()));
	gdk_device_grab(device, gtk_widget_get_window(widget), GDK_OWNERSHIP_WINDOW, FALSE,
		GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_TOUCH_MASK,
				NULL, event_info->time);
# endif
#endif

	if((window->state & GDK_BUTTON1_MASK) != 0)
	{
		GdkEventButton event_info = {0};
		gdouble x, y;
		gdouble x0, y0;
		const gdouble pressure = 0;

		x0 = (window->before_cursor_x - window->half_size) * window->cos_value
			- (window->before_cursor_y - window->half_size) * window->sin_value + window->add_cursor_x;
		y0 = (window->before_cursor_x - window->half_size) * window->sin_value
			+ (window->before_cursor_y - window->half_size) * window->cos_value + window->add_cursor_y;
		x = window->rev_zoom * x0;
		y = window->rev_zoom * y0;

		event_info.button = 1;
		event_info.state = window->state;

		if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
		{
			window->app->tool_window.active_common_tool->release_func(
				window, x, y,
				window->app->tool_window.active_common_tool, (void*)&event_info
			);
		}
		else
		{
			if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
				|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
			{
				window->app->tool_window.active_brush[window->app->input]->release_func(
					window, x, y, pressure,
					window->app->tool_window.active_brush[window->app->input], (void*)&event_info
				);
			}
			else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
			{
				if((event_info.state & GDK_SHIFT_MASK) != 0)
				{
					window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
					window->app->tool_window.vector_control_core.release_func(
						window, x, y,
						pressure, &window->app->tool_window.vector_control_core, (void*)&event_info
					);
				}
				else if((event_info.state & GDK_CONTROL_MASK) != 0 ||
					(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
				{
					window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
					window->app->tool_window.vector_control_core.release_func(
						window, x,y,
						pressure, &window->app->tool_window.vector_control_core, (void*)&event_info
					);
				}
				else
				{
					window->app->tool_window.active_vector_brush[window->app->input]->release_func(
						window, x, y,
						pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)&event_info
					);
				}
			}
		}

		window->state &= ~(GDK_BUTTON1_MASK);
	}

	return TRUE;
}

/*************************************************************
* LeaveNotifyEvent関数                                       *
* 描画領域外にマウスカーソルが出て行った際に呼び出される関数 *
* 引数                                                       *
* widget		: 描画領域のウィジェット                     *
* event_info	: イベントの情報                             *
* window		: 描画領域の情報                             *
*************************************************************/
gboolean LeaveNotifyEvent(GtkWidget* widget, GdkEventCrossing* event_info, DRAW_WINDOW* window)
{
#ifndef _WIN32
	// 入力デバイスの設定を変更
	GList *device_list = gdk_devices_list();
	int counter = 0;
	while(device_list != NULL && counter < window->app->num_device)
	{
		if(window->app->set_input_modes[counter] != FALSE)
		{
			gdk_device_set_mode((GdkDevice*)device_list->data, GDK_MODE_DISABLED);
		}
		gdk_device_set_source((GdkDevice*)device_list->data, window->app->input_sources[counter]);

		device_list = device_list->next;
		counter++;
	}

	gdk_flush();
#else
# if GDK_MAJOR_VERSION <= 2
	gdk_pointer_ungrab(event_info->time);
# else
	{
		GList *device_list, *check_list;
		GdkDevice *device;
		gdk_window_set_support_multidevice(gtk_widget_get_window(widget), TRUE);
		device_list = gdk_device_manager_list_devices(
			gdk_display_get_device_manager(gdk_display_get_default()), GDK_DEVICE_TYPE_MASTER);
		check_list = device_list;
		while(check_list != NULL)
		{
			device = (GdkDevice*)check_list->data;
			gdk_device_ungrab(device, event_info->time);
			check_list = check_list->next;
		}
		g_list_free(device_list);
	}
# endif
#endif

	// マウスカーソルが描画されないように外へ
	window->cursor_x = -500000;
	window->cursor_y = -500000;

	gtk_widget_queue_draw(widget);

	return TRUE;
}

#ifdef __cplusplus
}
#endif
