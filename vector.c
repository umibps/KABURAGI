// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <math.h>
#include <zlib.h>
#include <gtk/gtk.h>
#include "application.h"
#include "draw_window.h"
#include "vector.h"
#include "vector_layer.h"
#include "bezier.h"
#include "anti_alias.h"
#include "memory_stream.h"
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "LuaCairo/lcairo.h"
#include "memory.h"

#include "gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

VECTOR_LINE_LAYER* CreateVectorLineLayer(
	LAYER* work,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
)
{
	VECTOR_LINE_LAYER* ret =
		(VECTOR_LINE_LAYER*)MEM_ALLOC_FUNC(sizeof(*ret));
	int width, height;
	int i;

	(void)memset(ret, 0, sizeof(*ret));

	if(rect->min_x < 0)
	{
		rect->min_x = 0;
	}
	if(rect->min_y < 0)
	{
		rect->min_y = 0;
	}

	width = (int32)(rect->max_x - rect->min_x)+1;
	height = (int32)(rect->max_y - rect->min_y)+1;

	if((int32)rect->min_x + width > work->width)
	{
		width = work->width - (int32)rect->min_x;
	}
	if((int32)rect->min_y + height > work->height)
	{
		height = work->height - (int32)rect->min_y;
	}

	ret->x = (int32)rect->min_x;
	ret->y = (int32)rect->min_y;
	ret->width = width;
	ret->height = height;
	ret->stride = width*4;
	ret->pixels = (uint8*)MEM_ALLOC_FUNC(height*ret->stride);

	if(ret->pixels == NULL)
	{
		MEM_FREE_FUNC(ret);
		return NULL;
	}

	for(i=0; i<height; i++)
	{
		(void)memcpy(&ret->pixels[ret->stride*i],
			&work->pixels[(ret->y+i)*work->stride+ret->x*4], width*4);
	}

	ret->surface_p = cairo_image_surface_create_for_data(
		ret->pixels, CAIRO_FORMAT_ARGB32, width, height, ret->stride);
	ret->cairo_p = cairo_create(ret->surface_p);

	return ret;
}

void DeleteVectorLineLayer(VECTOR_LINE_LAYER** layer)
{
	if(*layer == NULL)
	{
		return;
	}

	cairo_surface_destroy((*layer)->surface_p);
	cairo_destroy((*layer)->cairo_p);

	MEM_FREE_FUNC((*layer)->pixels);
	MEM_FREE_FUNC(*layer);

	*layer = NULL;
}

void StrokeStraightLine(
	DRAW_WINDOW* window,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
)
{
	gdouble r, half_r, d, div_d, arg, dist;
	int32 min_x, min_y, max_x, max_y;
	gdouble draw_x, draw_y;
	gdouble dx, dy, cos_x, sin_y;
	gdouble ad, bd;
	gdouble red, green, blue, alpha;
	cairo_pattern_t *brush;
	int i, x, y;

	cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	rect->min_x = rect->max_x = line->points->x;
	rect->min_y = rect->max_y = line->points->y;

	for(i=0; i<line->num_points-1; i++)
	{
		dx = line->points[i].x-line->points[i+1].x;
		dy = line->points[i].y-line->points[i+1].y;
		d = sqrt(dx*dx+dy*dy);

		if(d < 0.5)
		{
			continue;
		}

		div_d = 1.0 / d;
		dist = 0.0;
		arg = atan2(dy, dx);
		cos_x = cos(arg), sin_y = sin(arg);
		draw_x = line->points[i].x, draw_y = line->points[i].y;

		dx = d;

		do
		{
			ad = (d - dist) * div_d, bd = dist * div_d;
			r = line->points[i].size * line->points[i].pressure * 0.01f * ad
				+ line->points[i+1].size * line->points[i+1].pressure * 0.01f * bd;
			half_r = r * 0.125;
			if(half_r < 0.5)
			{
				half_r = 0.5;
			}
			dist += half_r;

			min_x = (int32)(draw_x - r - 1);
			min_y = (int32)(draw_y - r - 1);
			max_x = (int32)(draw_x + r + 1);
			max_y = (int32)(draw_y + r + 1);

			if(min_x < 0)
			{
				min_x = 0;
			}
			else if(min_x > window->width)
			{
				min_x = window->width;
			}
			if(min_y < 0)
			{
				min_y = 0;
			}
			else if(min_y > window->height)
			{
				min_y = window->height;
			}
			if(max_x > window->width)
			{
				max_x = window->width;
			}
			else if(max_x < 0)
			{
				max_x = 0;
			}
			if(max_y > window->height)
			{
				max_y = window->height;
			}
			else if(max_y < 0)
			{
				max_y = 0;
			}

			if(rect->min_x > min_x)
			{
				rect->min_x = min_x;
			}
			if(rect->min_y > min_y)
			{
				rect->min_y = min_y;
			}
			if(rect->max_x < max_x)
			{
				rect->max_x = max_x;
			}
			if(rect->max_y < max_y)
			{
				rect->max_y = max_y;
			}
			
			for(y=min_y; y<max_y; y++)
			{
				(void)memset(&window->temp_layer->pixels[y*window->temp_layer->stride+min_x*window->temp_layer->channel],
					0, (max_x - min_x)*window->temp_layer->channel);
			}

			ad *= DIV_PIXEL, bd *= DIV_PIXEL;
			red = (line->points[i].color[0] * ad) + (line->points[i+1].color[0] * bd);
			green = (line->points[i].color[1] * ad) + (line->points[i+1].color[1] * bd);
			blue = (line->points[i].color[2] * ad) + (line->points[i+1].color[2] * bd);
			alpha = (line->points[i].color[3] * ad) + (line->points[i+1].color[3] * bd);

			brush = cairo_pattern_create_radial(draw_x, draw_y, r * 0.0, draw_x, draw_y, r);
			cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
			cairo_pattern_add_color_stop_rgba(brush, 0.0, red, green, blue, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1.0-line->blur*0.01,
				red, green, blue, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1.0, red, green, blue,
				line->outline_hardness*0.01*alpha);
			cairo_set_source(window->temp_layer->cairo_p, brush);

			cairo_arc(window->temp_layer->cairo_p, draw_x, draw_y, r, 0, 2*G_PI);
			cairo_fill(window->temp_layer->cairo_p);

			for(y=min_y; y<max_y; y++)
			{
				for(x=min_x; x<max_x; x++)
				{
					if(window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >
						window->work_layer->pixels[y*window->work_layer->stride+x*window->channel+3])
					{
						window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel] =
							(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel]
							- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel])
								* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
								+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel];
						window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+1] =
							(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+1]
							- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+1])
								* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
								+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+1];
						window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+2] =
							(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+2]
							- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+2])
								* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
								+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+2];
						window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+3] =
							(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3]
							- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+3])
								* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
								+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+3];
					}
				}
			}

			cairo_pattern_destroy(brush);

			dx -= half_r;
			draw_x -= cos_x*half_r, draw_y -= sin_y*half_r;
		} while(dx >= half_r);
	}
}

void StrokeStraightCloseLine(
	DRAW_WINDOW* window,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
)
{
	gdouble r, half_r, d, div_d, arg, dist;
	int32 min_x, min_y, max_x, max_y;
	gdouble draw_x, draw_y;
	gdouble dx, dy, cos_x, sin_y;
	gdouble ad, bd;
	gdouble red, green, blue, alpha;
	cairo_pattern_t *brush;
	int i, x, y;

	cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
	rect->min_x = rect->max_x = line->points->x;
	rect->min_y = rect->max_y = line->points->y;

	for(i=0; i<line->num_points-1; i++)
	{
		dx = line->points[i].x-line->points[i+1].x;
		dy = line->points[i].y-line->points[i+1].y;
		d = sqrt(dx*dx+dy*dy);

		if(d < 0.5)
		{
			continue;
		}

		div_d = 1.0 / d;
		dist = 0.0;
		arg = atan2(dy, dx);
		cos_x = cos(arg), sin_y = sin(arg);
		draw_x = line->points[i].x, draw_y = line->points[i].y;

		dx = d;

		do
		{
			ad = (d - dist) * div_d, bd = dist * div_d;
			r = line->points[i].size * line->points[i].pressure * 0.01f * ad
				+ line->points[i+1].size * line->points[i+1].pressure * 0.01f * bd;
			half_r = r * 0.125;
			if(half_r < 0.5)
			{
				half_r = 0.5;
			}
			dist += half_r;

			min_x = (int32)(draw_x - r - 1);
			min_y = (int32)(draw_y - r - 1);
			max_x = (int32)(draw_x + r + 1);
			max_y = (int32)(draw_y + r + 1);

			if(min_x < 0)
			{
				min_x = 0;
			}
			else if(min_x > window->width)
			{
				min_x = window->width;
			}
			if(min_y < 0)
			{
				min_y = 0;
			}
			else if(min_y > window->height)
			{
				min_y = window->height;
			}
			if(max_x > window->width)
			{
				max_x = window->width;
			}
			else if(max_x < 0)
			{
				max_x = 0;
			}
			if(max_y > window->height)
			{
				max_y = window->height;
			}
			else if(max_y < 0)
			{
				max_y = 0;
			}

			if(rect->min_x > min_x)
			{
				rect->min_x = min_x;
			}
			if(rect->min_y > min_y)
			{
				rect->min_y = min_y;
			}
			if(rect->max_x < max_x)
			{
				rect->max_x = max_x;
			}
			if(rect->max_y < max_y)
			{
				rect->max_y = max_y;
			}
			
			for(y=min_y; y<max_y; y++)
			{
				(void)memset(&window->temp_layer->pixels[y*window->temp_layer->stride+min_x*window->temp_layer->channel],
					0, (max_x - min_x)*window->temp_layer->channel);
			}

			ad *= DIV_PIXEL, bd *= DIV_PIXEL;
			red = (line->points[i].color[0] * ad) + (line->points[i+1].color[0] * bd);
			green = (line->points[i].color[1] * ad) + (line->points[i+1].color[1] * bd);
			blue = (line->points[i].color[2] * ad) + (line->points[i+1].color[2] * bd);
			alpha = (line->points[i].color[3] * ad) + (line->points[i+1].color[3] * bd);

			brush = cairo_pattern_create_radial(draw_x, draw_y, r * 0.0, draw_x, draw_y, r);
			cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
			cairo_pattern_add_color_stop_rgba(brush, 0.0, red, green, blue, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1.0-line->blur*0.01,
				red, green, blue, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1.0, red, green, blue,
				line->outline_hardness*0.01f*alpha);
			cairo_set_source(window->temp_layer->cairo_p, brush);

			cairo_arc(window->temp_layer->cairo_p, draw_x, draw_y, r, 0, 2*G_PI);
			cairo_fill(window->temp_layer->cairo_p);

			for(y=min_y; y<max_y; y++)
			{
				for(x=min_x; x<max_x; x++)
				{
					if(window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >
						window->work_layer->pixels[y*window->work_layer->stride+x*window->channel+3])
					{
						window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel] =
							(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel]
							- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel])
								* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
								+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel];
						window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+1] =
							(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+1]
							- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+1])
								* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
								+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+1];
						window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+2] =
							(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+2]
							- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+2])
								* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
								+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+2];
						window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+3] =
							(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3]
							- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+3])
								* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
								+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+3];
					}
				}
			}

			cairo_pattern_destroy(brush);

			dx -= half_r;
			draw_x -= cos_x*half_r, draw_y -= sin_y*half_r;
		} while(dx >= half_r);
	}

	dx = line->points[i].x-line->points[0].x;
	dy = line->points[i].y-line->points[0].y;
	d = sqrt(dx*dx+dy*dy);

	if(d < 0.5)
	{
		return;
	}

	div_d = 1.0 / d;
	dist = 0.0;
	arg = atan2(dy, dx);
	cos_x = cos(arg), sin_y = sin(arg);
	draw_x = line->points[i].x, draw_y = line->points[i].y;

	dx = d;

	do
	{
		ad = (d - dist) * div_d, bd = dist * div_d;
		r = line->points[i].size * line->points[i].pressure * 0.01f * ad
			+ line->points[0].size * line->points[0].pressure * 0.01f * bd;
		half_r = r * 0.125;
		if(half_r < 0.5)
		{
			half_r = 0.5;
		}
		dist += half_r;

		min_x = (int32)(draw_x - r - 1);
		min_y = (int32)(draw_y - r - 1);
		max_x = (int32)(draw_x + r + 1);
		max_y = (int32)(draw_y + r + 1);

		if(min_x < 0)
		{
			min_x = 0;
		}
		else if(min_x > window->width)
		{
			min_x = window->width;
		}
		if(min_y < 0)
		{
			min_y = 0;
		}
		else if(min_y > window->height)
		{
			min_y = window->height;
		}
		if(max_x > window->width)
		{
			max_x = window->width;
		}
		else if(max_x < 0)
		{
			max_x = 0;
		}
		if(max_y > window->height)
		{
			max_y = window->height;
		}
		else if(max_y < 0)
		{
			max_y = 0;
		}

		if(rect->min_x > min_x)
		{
			rect->min_x = min_x;
		}
		if(rect->min_y > min_y)
		{
			rect->min_y = min_y;
		}
		if(rect->max_x < max_x)
		{
			rect->max_x = max_x;
		}
		if(rect->max_y < max_y)
		{
			rect->max_y = max_y;
		}

		for(y=min_y; y<max_y; y++)
		{
			(void)memset(&window->temp_layer->pixels[y*window->temp_layer->stride+min_x*window->temp_layer->channel],
				0, (max_x - min_x)*window->temp_layer->channel);
		}

		ad *= DIV_PIXEL, bd *= DIV_PIXEL;
		red = (line->points[i].color[0] * ad) + (line->points[0].color[0] * bd);
		green = (line->points[i].color[1] * ad) + (line->points[0].color[1] * bd);
		blue = (line->points[i].color[2] * ad) + (line->points[0].color[2] * bd);
		alpha = (line->points[i].color[3] * ad) + (line->points[0].color[3] * bd);

		brush = cairo_pattern_create_radial(draw_x, draw_y, r * 0.0, draw_x, draw_y, r);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
		cairo_pattern_add_color_stop_rgba(brush, 0.0, red, green, blue, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0-line->blur*0.01,
			red, green, blue, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0, red, green, blue,
			line->outline_hardness*0.01f*alpha);
		cairo_set_source(window->temp_layer->cairo_p, brush);

		cairo_arc(window->temp_layer->cairo_p, draw_x, draw_y, r, 0, 2*G_PI);
		cairo_fill(window->temp_layer->cairo_p);

		for(y=min_y; y<max_y; y++)
		{
			for(x=min_x; x<max_x; x++)
			{
				if(window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >
					window->work_layer->pixels[y*window->work_layer->stride+x*window->channel+3])
				{
					window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel] =
						(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel]
						- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel])
							* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
							+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel];
					window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+1] =
						(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+1]
						- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+1])
							* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
							+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+1];
					window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+2] =
						(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+2]
						- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+2])
							* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
							+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+2];
					window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+3] =
						(uint8)(((int)window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3]
						- (int)window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+3])
							* window->temp_layer->pixels[y*window->temp_layer->stride+x*window->temp_layer->channel+3] >> 8)
							+ window->work_layer->pixels[y*window->work_layer->stride+x*window->work_layer->channel+3];
				}
			}
		}

		cairo_pattern_destroy(brush);

		dx -= half_r;
		draw_x -= cos_x*half_r, draw_y -= sin_y*half_r;
	} while(dx >= half_r);
}

void RasterizeVectorSquare(
	DRAW_WINDOW* window,
	VECTOR_SQUARE* square,
	VECTOR_LAYER_RECTANGLE* rectangle
)
{
	cairo_t *cairo_p = cairo_create(window->work_layer->surface_p);
	FLOAT_T sin_value = sin(square->rotate);
	FLOAT_T cos_value = cos(square->rotate);

	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
	cairo_translate(cairo_p, square->left + square->width * 0.5, square->top + square->height * 0.5);
	cairo_rotate(cairo_p, square->rotate);
	cairo_translate(cairo_p, - square->width * 0.5, - square->height * 0.5);
	cairo_set_line_width(cairo_p, square->line_width);
	cairo_set_line_join(cairo_p, (cairo_line_join_t)square->line_joint);
	cairo_rectangle(cairo_p, 0,
		0, square->width, square->height);
	cairo_path_extents(cairo_p, &rectangle->min_x, &rectangle->min_y,
		&rectangle->max_x, &rectangle->max_y);

	rectangle->min_x += square->left - square->line_width;
	rectangle->max_x += square->left + square->line_width;
	rectangle->min_y += square->top - square->line_width;
	rectangle->max_y += square->top + square->line_width;

	if(square->line_color[3] > 0)
	{
		cairo_set_source_rgba(cairo_p,
#if 0 //defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			square->line_color[2] * DIV_PIXEL, square->line_color[1] * DIV_PIXEL, square->line_color[0] * DIV_PIXEL,
#else
			square->line_color[0] * DIV_PIXEL, square->line_color[1] * DIV_PIXEL, square->line_color[2] * DIV_PIXEL,
#endif
				square->line_color[3] * DIV_PIXEL
		);
		cairo_stroke_preserve(cairo_p);
	}
	if(square->fill_color[3] > 0)
	{
		cairo_set_source_rgba(cairo_p,
#if 0//defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			square->fill_color[2] * DIV_PIXEL, square->fill_color[1] * DIV_PIXEL, square->fill_color[0] * DIV_PIXEL,
#else
			square->fill_color[0] * DIV_PIXEL, square->fill_color[1] * DIV_PIXEL, square->fill_color[2] * DIV_PIXEL,
#endif
				square->fill_color[3] * DIV_PIXEL
		);
		cairo_fill(cairo_p);
	}

	cairo_destroy(cairo_p);
}

void RasterizeVectorRhombus(
	DRAW_WINDOW* window,
	VECTOR_ECLIPSE* rhombus,
	VECTOR_LAYER_RECTANGLE* rectangle
)
{
	cairo_t *cairo_p = cairo_create(window->work_layer->surface_p);
	double x_scale;
	double y_scale;
	double width, height;
	double sin_value = sin(- rhombus->rotate);
	double cos_value = cos(- rhombus->rotate);

	if(rhombus->ratio > 1.0)
	{
		x_scale = rhombus->ratio;
		y_scale = 1;
	}
	else
	{
		x_scale = 1;
		y_scale = 1.0 / rhombus->ratio;
	}

	width = rhombus->radius * x_scale;
	height = rhombus->radius * y_scale;

	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

	cairo_set_line_width(cairo_p, rhombus->line_width);
	cairo_set_line_join(cairo_p, (cairo_line_join_t)rhombus->line_joint);
	cairo_move_to(cairo_p, rhombus->x - sin_value * height, rhombus->y - cos_value * height);
	cairo_line_to(cairo_p, rhombus->x - cos_value * width, rhombus->y + sin_value * width);
	cairo_line_to(cairo_p, rhombus->x + sin_value * height, rhombus->y + cos_value * height);
	cairo_line_to(cairo_p, rhombus->x + cos_value * width, rhombus->y - sin_value * width);
	cairo_close_path(cairo_p);
	cairo_path_extents(cairo_p, &rectangle->min_x, &rectangle->min_y,
		&rectangle->max_x, &rectangle->max_y);

	rectangle->min_x += - rhombus->radius * x_scale - rhombus->line_width;
	rectangle->max_x += rhombus->radius * x_scale + rhombus->line_width;
	rectangle->min_y += - rhombus->radius * y_scale - rhombus->line_width;
	rectangle->max_y += rhombus->radius * y_scale + rhombus->line_width;

	if(rhombus->line_color[3] > 0)
	{
		cairo_set_source_rgba(cairo_p,
#if 0 //defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			rhombus->line_color[2] * DIV_PIXEL, rhombus->line_color[1] * DIV_PIXEL, rhombus->line_color[0] * DIV_PIXEL,
#else
			rhombus->line_color[0] * DIV_PIXEL, rhombus->line_color[1] * DIV_PIXEL, rhombus->line_color[2] * DIV_PIXEL,
#endif
				rhombus->line_color[3] * DIV_PIXEL
		);
		cairo_stroke_preserve(cairo_p);
	}
	if(rhombus->fill_color[3] > 0)
	{
		cairo_set_source_rgba(cairo_p,
#if 0 //defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			rhombus->fill_color[2] * DIV_PIXEL, rhombus->fill_color[1] * DIV_PIXEL, rhombus->fill_color[0] * DIV_PIXEL,
#else
			rhombus->fill_color[0] * DIV_PIXEL, rhombus->fill_color[1] * DIV_PIXEL, rhombus->fill_color[2] * DIV_PIXEL,
#endif
				rhombus->fill_color[3] * DIV_PIXEL
		);
		cairo_fill(cairo_p);
	}

	cairo_destroy(cairo_p);
}

void RasterizeVectorEclipse(
	DRAW_WINDOW* window,
	VECTOR_ECLIPSE* eclipse,
	VECTOR_LAYER_RECTANGLE* rectangle
)
{
	cairo_t *cairo_p = cairo_create(window->work_layer->surface_p);
	double x_scale;
	double y_scale;
	double sin_value = sin(eclipse->rotate);
	double cos_value = cos(eclipse->rotate);

	if(eclipse->radius < 0.01)
	{
		return;
	}

	if(eclipse->ratio > 1.0)
	{
		x_scale = eclipse->ratio;
		y_scale = 1;
	}
	else
	{
		x_scale = 1;
		y_scale = 1.0 / eclipse->ratio;
	}

	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
	cairo_translate(cairo_p, eclipse->x + eclipse->radius, eclipse->y + eclipse->radius);
	cairo_rotate(cairo_p, eclipse->rotate);
	cairo_translate(cairo_p, - eclipse->radius, - eclipse->radius);

	cairo_save(cairo_p);
	cairo_scale(cairo_p, x_scale, y_scale);
	cairo_arc(cairo_p, (1 - cos_value - sin_value) * eclipse->radius / x_scale,
		(1 - cos_value + sin_value) * eclipse->radius / y_scale, eclipse->radius, 0, 2 * G_PI);
	cairo_close_path(cairo_p);
	cairo_path_extents(cairo_p, &rectangle->min_x, &rectangle->min_y,
		&rectangle->max_x, &rectangle->max_y);

	rectangle->min_x += eclipse->x - eclipse->radius * x_scale - eclipse->line_width;
	rectangle->max_x += eclipse->x + eclipse->radius * x_scale + eclipse->line_width;
	rectangle->min_y += eclipse->y - eclipse->radius * y_scale - eclipse->line_width;
	rectangle->max_y += eclipse->y + eclipse->radius * y_scale + eclipse->line_width;

	if(eclipse->fill_color[3] > 0)
	{
		cairo_set_source_rgba(cairo_p,
#if 0 //defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			eclipse->fill_color[2] * DIV_PIXEL, eclipse->fill_color[1] * DIV_PIXEL, eclipse->fill_color[0] * DIV_PIXEL,
#else
			eclipse->fill_color[0] * DIV_PIXEL, eclipse->fill_color[1] * DIV_PIXEL, eclipse->fill_color[2] * DIV_PIXEL,
#endif
				eclipse->fill_color[3] * DIV_PIXEL
		);
		cairo_fill_preserve(cairo_p);
	}
	cairo_restore(cairo_p);
	cairo_set_line_width(cairo_p, eclipse->line_width);
	cairo_set_line_join(cairo_p, (cairo_line_join_t)eclipse->line_joint);
	if(eclipse->line_color[3] > 0)
	{
		cairo_set_source_rgba(cairo_p,
#if 0 //defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			eclipse->line_color[2] * DIV_PIXEL, eclipse->line_color[1] * DIV_PIXEL, eclipse->line_color[0] * DIV_PIXEL,
#else
			eclipse->line_color[0] * DIV_PIXEL, eclipse->line_color[1] * DIV_PIXEL, eclipse->line_color[2] * DIV_PIXEL,
#endif
				eclipse->line_color[3] * DIV_PIXEL
		);
		cairo_stroke(cairo_p);
	}

	cairo_destroy(cairo_p);
}

void RasterizeVectorScript(
	DRAW_WINDOW* window,
	VECTOR_SCRIPT* script,
	VECTOR_LAYER_RECTANGLE* rectangle
)
{
	struct lua_State *state;
	cairo_t *cairo_p;
	int result;
	int x, y;

	if(script->script_data == NULL
		|| (window->flags & DRAW_WINDOW_IN_RASTERIZING_VECTOR_SCRIPT) != 0)
	{
		return;
	}

	window->flags |= DRAW_WINDOW_IN_RASTERIZING_VECTOR_SCRIPT;

	state = luaL_newstate();
	luaL_openlibs(state);
	(void)luaopen_lcairo(state);

	cairo_p = cairo_create(window->work_layer->surface_p);
	lua_pushlightuserdata(state, cairo_p);
	lua_setglobal(state, "cr");

	lua_pushnumber(state, G_PI);
	lua_setglobal(state, "PI");

	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

	if(luaL_loadstring(state, script->script_data) != 0)
	{
		GtkWidget *dialog;
		char message[8192];
		(void)sprintf(message, "Failed to rasterize script vector.\n%s",
			lua_tostring(state, -1));
		dialog = gtk_message_dialog_new(
			GTK_WINDOW(window->app->widgets->window), GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, message);
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
		gtk_widget_show_all(dialog);
		(void)gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		rectangle->min_x = rectangle->max_x = rectangle->min_y = rectangle->max_y = 0;
		lua_close(state);
		cairo_destroy(cairo_p);
		window->flags &= ~(DRAW_WINDOW_IN_RASTERIZING_VECTOR_SCRIPT);
		return;
	}

	result = lua_pcall(state, 0, 0, 0);
	if(result == 0)
	{
		lua_getglobal(state, "main");
		if(lua_isfunction(state, -1) != 0)
		{
			result = lua_pcall(state, 0, LUA_MULTRET, 0);
		}
	}

	if(result != 0)
	{
		GtkWidget *dialog;
		char message[8192];
		(void)sprintf(message, "Failed to rasterize script vector.\n%s",
			lua_tostring(state, -1));
		dialog = gtk_message_dialog_new(
			GTK_WINDOW(window->app->widgets->window), GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, message);
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
		gtk_widget_show_all(dialog);
		(void)gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		rectangle->min_x = rectangle->max_x = rectangle->min_y = rectangle->max_y = 0;
		lua_close(state);
		cairo_destroy(cairo_p);
		window->flags &= ~(DRAW_WINDOW_IN_RASTERIZING_VECTOR_SCRIPT);
		return;
	}

	cairo_destroy(cairo_p);
	lua_close(state);

	rectangle->min_x = window->width;
	rectangle->min_y = window->height;
	rectangle->max_x = 0;
	rectangle->max_y = 0;
	for(y=0; y<window->height; y++)
	{
		for(x=0; x<window->width; x++)
		{
			if(window->work_layer->pixels[y*window->stride+x*4+3] > 0)
			{
				if(rectangle->max_x < x)
				{
					rectangle->max_x = x;
				}
				if(rectangle->min_x > x)
				{
					rectangle->min_x = 0;
				}
				if(rectangle->max_y < y)
				{
					rectangle->max_y = y;
				}
				if(rectangle->min_y > y)
				{
					rectangle->min_y = y;
				}
			}
		}
	}

	window->flags &= ~(DRAW_WINDOW_IN_RASTERIZING_VECTOR_SCRIPT);
}

void RasterizeVectorLine(
	DRAW_WINDOW* window,
	VECTOR_LAYER* layer,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
)
{
	switch(line->base_data.vector_type)
	{
	case VECTOR_TYPE_STRAIGHT:
		StrokeStraightLine(window, line, rect);
		break;
	case VECTOR_TYPE_BEZIER_OPEN:
		if(line->num_points < 3)
		{
			StrokeStraightLine(window, line, rect);
		}
		else
		{
			StrokeBezierLine(window, line, rect);
		}
		break;
	case VECTOR_TYPE_STRAIGHT_CLOSE:
		if(line->num_points < 3)
		{
			StrokeStraightLine(window, line, rect);
		}
		else
		{
			StrokeStraightCloseLine(window, line, rect);
		}
		break;
	case VECTOR_TYPE_BEZIER_CLOSE:
		if(line->num_points < 3)
		{
			StrokeStraightLine(window, line, rect);
		}
		else
		{
			StrokeBezierLineClose(window, line, rect);
		}
		break;
	case VECTOR_TYPE_SQUARE:
		RasterizeVectorSquare(window, (VECTOR_SQUARE*)line, rect);
		break;
	case VECTOR_TYPE_ECLIPSE:
		RasterizeVectorEclipse(window, (VECTOR_ECLIPSE*)line, rect);
		break;
	}

	if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
	{
		if((line->flags & VECTOR_LINE_ANTI_ALIAS) != 0)
		{
			ANTI_ALIAS_RECTANGLE range;

			range.x = (int)rect->min_x;
			range.y = (int)rect->min_y;
			range.width = (int)(rect->max_x - rect->min_x) + 1;
			range.height = (int)(rect->max_y - rect->min_y) + 1;

			AntiAliasVectorLine(window->work_layer, window->temp_layer, &range);
		}
	}
}

void ReadVectorShapeData(VECTOR_DATA* data, MEMORY_STREAM* stream)
{
	uint8 vector_type;

	(void)MemRead(&vector_type, sizeof(vector_type), 1, stream);

	if(vector_type == VECTOR_TYPE_SQUARE)
	{
		data->square.base_data.vector_type = vector_type;
		(void)MemRead(&data->square.flags, sizeof(data->square.flags), 1, stream);
		(void)MemRead(&data->square.line_joint, sizeof(data->square.line_joint), 1, stream);
		(void)MemRead(&data->square.left, sizeof(data->square.left), 1, stream);
		(void)MemRead(&data->square.top, sizeof(data->square.top), 1, stream);
		(void)MemRead(&data->square.width, sizeof(data->square.width), 1, stream);
		(void)MemRead(&data->square.height, sizeof(data->square.height), 1, stream);
		(void)MemRead(&data->square.rotate, sizeof(data->square.rotate), 1, stream);
		(void)MemRead(&data->square.line_width, sizeof(data->square.line_width), 1, stream);
		(void)MemRead(data->square.line_color, sizeof(data->square.line_color), 1, stream);
		(void)MemRead(data->square.fill_color, sizeof(data->square.fill_color), 1, stream);
	}
	else if(vector_type == VECTOR_TYPE_RHOMBUS
		|| vector_type == VECTOR_TYPE_ECLIPSE)
	{
		data->eclipse.base_data.vector_type = vector_type;
		(void)MemRead(&data->eclipse.flags, sizeof(data->eclipse.flags), 1, stream);
		(void)MemRead(&data->eclipse.line_joint, sizeof(data->eclipse.line_joint), 1, stream);
		(void)MemRead(&data->eclipse.x, sizeof(data->eclipse.x), 1, stream);
		(void)MemRead(&data->eclipse.y, sizeof(data->eclipse.y), 1, stream);
		(void)MemRead(&data->eclipse.radius, sizeof(data->eclipse.radius), 1, stream);
		(void)MemRead(&data->eclipse.ratio, sizeof(data->eclipse.ratio), 1, stream);
		(void)MemRead(&data->eclipse.rotate, sizeof(data->eclipse.rotate), 1, stream);
		(void)MemRead(&data->eclipse.line_width, sizeof(data->eclipse.line_width), 1, stream);
		(void)MemRead(data->eclipse.line_color, sizeof(data->eclipse.line_color), 1, stream);
		(void)MemRead(data->eclipse.fill_color, sizeof(data->eclipse.fill_color), 1, stream);
	}
}

void WriteVectorShapeData(VECTOR_DATA* data, MEMORY_STREAM* stream)
{
	switch(data->line.base_data.vector_type)
	{
	case VECTOR_TYPE_SQUARE:
		(void)MemWrite(&data->square.base_data.vector_type, sizeof(data->square.base_data.vector_type), 1, stream);
		(void)MemWrite(&data->square.flags, sizeof(data->square.flags), 1, stream);
		(void)MemWrite(&data->square.line_joint, sizeof(data->square.line_joint), 1, stream);
		(void)MemWrite(&data->square.left, sizeof(data->square.left), 1, stream);
		(void)MemWrite(&data->square.top, sizeof(data->square.top), 1, stream);
		(void)MemWrite(&data->square.width, sizeof(data->square.width), 1, stream);
		(void)MemWrite(&data->square.height, sizeof(data->square.height), 1, stream);
		(void)MemWrite(&data->square.rotate, sizeof(data->square.rotate), 1, stream);
		(void)MemWrite(&data->square.line_width, sizeof(data->square.line_width), 1, stream);
		(void)MemWrite(data->square.line_color, sizeof(data->square.line_color), 1, stream);
		(void)MemWrite(data->square.fill_color, sizeof(data->square.fill_color), 1, stream);
		break;
	case VECTOR_TYPE_RHOMBUS:
	case VECTOR_TYPE_ECLIPSE:
		(void)MemWrite(&data->eclipse.base_data.vector_type, sizeof(data->square.base_data.vector_type), 1, stream);
		(void)MemWrite(&data->eclipse.flags, sizeof(data->eclipse.flags), 1, stream);
		(void)MemWrite(&data->eclipse.line_joint, sizeof(data->eclipse.line_joint), 1, stream);
		(void)MemWrite(&data->eclipse.x, sizeof(data->eclipse.x), 1, stream);
		(void)MemWrite(&data->eclipse.y, sizeof(data->eclipse.y), 1, stream);
		(void)MemWrite(&data->eclipse.radius, sizeof(data->eclipse.radius), 1, stream);
		(void)MemWrite(&data->eclipse.ratio, sizeof(data->eclipse.ratio), 1, stream);
		(void)MemWrite(&data->eclipse.rotate, sizeof(data->eclipse.rotate), 1, stream);
		(void)MemWrite(&data->eclipse.line_width, sizeof(data->eclipse.line_width), 1, stream);
		(void)MemWrite(data->eclipse.line_color, sizeof(data->eclipse.line_color), 1, stream);
		(void)MemWrite(data->eclipse.fill_color, sizeof(data->eclipse.fill_color), 1, stream);
		break;
	}
}

void ReadVectorScriptData(VECTOR_SCRIPT* script, MEMORY_STREAM* stream)
{
	guint32 length;
	(void)MemRead(&script->base_data.vector_type, sizeof(script->base_data.vector_type), 1, stream);
	(void)MemRead(&length, sizeof(length), 1, stream);
	script->script_data = (char*)MEM_ALLOC_FUNC(length);
	(void)MemRead(script->script_data, 1, length, stream);
}

void WriteVectorScriptData(VECTOR_SCRIPT* script, MEMORY_STREAM* stream)
{
	guint32 length = (guint32)strlen(script->script_data);
	(void)MemWrite(&script->base_data.vector_type, sizeof(script->base_data.vector_type), 1, stream);
	(void)MemWrite(&length, sizeof(length), 1, stream);
	(void)MemWrite(script->script_data, 1, length, stream);
}

void BlendVectorLineLayer(VECTOR_LINE_LAYER* src, VECTOR_LINE_LAYER* dst)
{
	cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
	cairo_paint(dst->cairo_p);
}

VECTOR_LINE* CreateVectorLine(VECTOR_LINE* prev, VECTOR_LINE* next)
{
	VECTOR_LINE* ret = (VECTOR_LINE*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	if(prev != NULL)
	{
		prev->base_data.next = (void*)ret;
	}
	if(next != NULL)
	{
		next->base_data.prev = (void*)ret;
	}
	ret->base_data.prev = (void*)prev;
	ret->base_data.next = (void*)next;

	return ret;
}

void DeleteVectorLine(VECTOR_LINE** line)
{
	if((*line)->base_data.prev != NULL)
	{
		((VECTOR_BASE_DATA*)(*line)->base_data.prev)->next = (*line)->base_data.next;
	}
	if((*line)->base_data.next != NULL)
	{
		((VECTOR_BASE_DATA*)(*line)->base_data.next)->prev = (*line)->base_data.prev;
	}

	DeleteVectorLineLayer(&(*line)->base_data.layer);

	if((*line)->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
	{
		MEM_FREE_FUNC((*line)->points);
	}
	MEM_FREE_FUNC(*line);

	*line = NULL;
}

void DeleteVectorLayer(VECTOR_LAYER** layer)
{
	VECTOR_LINE* delete_line, *next_delete;
	delete_line = (VECTOR_LINE*)((*layer)->base);

	while(delete_line != NULL)
	{
		next_delete = (VECTOR_LINE*)delete_line->base_data.next;

		if(delete_line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
		{
			DeleteVectorLine(&delete_line);
		}
		else if(delete_line->base_data.vector_type < VECTOR_SHAPE_END)
		{
			DeleteVectorShape((VECTOR_BASE_DATA**)&delete_line);
		}
		else
		{
			DeleteVectorScript((VECTOR_SCRIPT**)&delete_line);
		}

		delete_line = next_delete;
	}
}

VECTOR_DATA* CreateVectorShape(VECTOR_BASE_DATA* prev, VECTOR_BASE_DATA* next, uint8 vector_type)
{
	VECTOR_DATA* ret;

	switch(vector_type)
	{
	case VECTOR_TYPE_SQUARE:
		ret = (VECTOR_DATA*)MEM_ALLOC_FUNC(sizeof(VECTOR_SQUARE));
		(void)memset(ret, 0, sizeof(VECTOR_SQUARE));
		break;
	case VECTOR_TYPE_RHOMBUS:
	case VECTOR_TYPE_ECLIPSE:
		ret = (VECTOR_DATA*)MEM_ALLOC_FUNC(sizeof(VECTOR_ECLIPSE));
		(void)memset(ret, 0, sizeof(VECTOR_ECLIPSE));
		break;
	default:
		return NULL;
	}

	if(prev != NULL)
	{
		prev->next = (void*)ret;
	}
	if(next != NULL)
	{
		next->prev = (void*)ret;
	}

	((VECTOR_BASE_DATA*)ret)->vector_type = vector_type;
	ret->line.base_data.prev = (void*)prev;
	ret->line.base_data.next = (void*)next;

	return ret;
}

VECTOR_DATA* CreateVectorScript(VECTOR_BASE_DATA* prev, VECTOR_BASE_DATA* next, const char* script)
{
	VECTOR_SCRIPT *ret = (VECTOR_SCRIPT*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	if(script != NULL)
	{
		ret->script_data = MEM_STRDUP_FUNC(script);
	}

	if(prev != NULL)
	{
		prev->next = (void*)ret;
	}
	if(next != NULL)
	{
		next->prev = (void*)ret;
	}
	ret->base_data.vector_type = VECTOR_TYPE_SCRIPT;
	ret->base_data.prev = (void*)prev;
	ret->base_data.next = (void*)next;

	return (VECTOR_DATA*)ret;
}

void DeleteVectorShape(VECTOR_BASE_DATA** shape)
{
	if((*shape)->prev != NULL)
	{
		((VECTOR_BASE_DATA*)(*shape)->prev)->next = (*shape)->next;
	}
	if((*shape)->next != NULL)
	{
		((VECTOR_BASE_DATA*)(*shape)->next)->prev = (*shape)->prev;
	}

	DeleteVectorLineLayer(&(*shape)->layer);

	MEM_FREE_FUNC(*shape);

	*shape = NULL;
}

void DeleteVectorScript(VECTOR_SCRIPT** script)
{
	if((*script)->base_data.prev != NULL)
	{
		((VECTOR_BASE_DATA*)(*script)->base_data.prev)->next = (*script)->base_data.next;
	}
	if((*script)->base_data.next != NULL)
	{
		((VECTOR_BASE_DATA*)(*script)->base_data.next)->prev = (*script)->base_data.prev;
	}

	DeleteVectorLineLayer(&(*script)->base_data.layer);

	MEM_FREE_FUNC((*script)->script_data);
	MEM_FREE_FUNC(*script);

	*script = NULL;
}

void RasterizeVectorLayer(
	struct _DRAW_WINDOW* window,
	struct _LAYER* target,
	VECTOR_LAYER* layer
)
{
	VECTOR_LAYER_RECTANGLE rect;

	cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);

	if((layer->flags & VECTOR_LAYER_RASTERIZE_TOP) != 0
		&& layer->top_data != NULL && layer->top_data != layer->base)
	{
		(void)memcpy(target->pixels, layer->mix->pixels, window->pixel_buf_size);
		(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

		if(((VECTOR_BASE_DATA*)layer->top_data)->vector_type < NUM_VECTOR_LINE_TYPE)
		{
			RasterizeVectorLine(window, layer, (VECTOR_LINE*)layer->top_data, &rect);
		}
		else if(((VECTOR_BASE_DATA*)layer->top_data)->vector_type == VECTOR_TYPE_SQUARE)
		{
			RasterizeVectorSquare(window, (VECTOR_SQUARE*)layer->top_data, &rect);
		}
		else if(((VECTOR_BASE_DATA*)layer->top_data)->vector_type == VECTOR_TYPE_RHOMBUS)
		{
			RasterizeVectorRhombus(window, (VECTOR_ECLIPSE*)layer->top_data, &rect);
		}
		else if(((VECTOR_BASE_DATA*)layer->top_data)->vector_type == VECTOR_TYPE_ECLIPSE)
		{
			RasterizeVectorEclipse(window, (VECTOR_ECLIPSE*)layer->top_data, &rect);
		}
		else
		{
			RasterizeVectorScript(window, (VECTOR_SCRIPT*)layer->top_data, &rect);
		}

		window->layer_blend_functions[LAYER_BLEND_NORMAL](window->work_layer, window->active_layer);

		if((layer->flags & VECTOR_LAYER_FIX_LINE) != 0)
		{
			((VECTOR_BASE_DATA*)layer->top_data)->layer = CreateVectorLineLayer(window->work_layer, (VECTOR_LINE*)layer->top_data, &rect);
			(void)memcpy(layer->mix->pixels, window->active_layer->pixels, window->pixel_buf_size);
			layer->active_data = NULL;
			(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
		}
	}
	else if((layer->flags & VECTOR_LAYER_RASTERIZE_ACTIVE) != 0)
	{
		VECTOR_LINE* dst = (VECTOR_LINE*)layer->base;
		VECTOR_LINE* src = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;

		(void)memset(dst->base_data.layer->pixels, 0, dst->base_data.layer->stride*dst->base_data.layer->height);
		(void)memset(layer->mix->pixels, 0, window->active_layer->stride*window->active_layer->height);

		while(src != NULL)
		{
			if((void*)src == layer->active_data)
			{
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type < NUM_VECTOR_LINE_TYPE)
				{
					RasterizeVectorLine(window, layer, (VECTOR_LINE*)layer->active_data, &rect);
				}
				else if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_SQUARE)
				{
					RasterizeVectorSquare(window, (VECTOR_SQUARE*)layer->active_data, &rect);
				}
				else if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_RHOMBUS)
				{
					RasterizeVectorRhombus(window, (VECTOR_ECLIPSE*)layer->active_data, &rect);
				}
				else if(((VECTOR_BASE_DATA*)layer->active_data)->vector_type == VECTOR_TYPE_ECLIPSE)
				{
					RasterizeVectorEclipse(window, (VECTOR_ECLIPSE*)layer->active_data, &rect);
				}
				else
				{
					RasterizeVectorScript(window, (VECTOR_SCRIPT*)layer->active_data, &rect);
				}

				if((layer->flags & VECTOR_LAYER_FIX_LINE) != 0)
				{
					DeleteVectorLineLayer(&src->base_data.layer);
					src->base_data.layer = CreateVectorLineLayer(window->work_layer, (VECTOR_LINE*)layer->active_data, &rect);
					BlendVectorLineLayer(src->base_data.layer, dst->base_data.layer);
				}
				else
				{
					cairo_set_source_surface(dst->base_data.layer->cairo_p, window->work_layer->surface_p, 0, 0);
					cairo_paint(dst->base_data.layer->cairo_p);
				}
			}
			else
			{
				if(src->base_data.layer != NULL)
				{
					BlendVectorLineLayer(src->base_data.layer, dst->base_data.layer);
				}
			}

			src = (VECTOR_LINE*)src->base_data.next;
		}

		(void)memcpy(window->active_layer->pixels, dst->base_data.layer->pixels, window->pixel_buf_size);
		if((layer->flags & VECTOR_LAYER_FIX_LINE) != 0)
		{
			(void)memcpy(layer->mix->pixels, window->active_layer->pixels, window->pixel_buf_size);
		}
	}
	else if((layer->flags & VECTOR_LAYER_RASTERIZE_CHECKED) != 0)
	{
		VECTOR_LINE* dst = (VECTOR_LINE*)layer->base;
		VECTOR_LINE* src = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;

		(void)memset(dst->base_data.layer->pixels, 0, dst->base_data.layer->stride*dst->base_data.layer->height);
		(void)memset(layer->mix->pixels, 0, window->active_layer->stride*window->active_layer->height);

		while(src != NULL)
		{
			if((src->flags & VECTOR_LINE_FIX) != 0)
			{
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				if(src->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					RasterizeVectorLine(window, layer, src, &rect);
				}
				else if(src->base_data.vector_type == VECTOR_TYPE_SQUARE)
				{
					RasterizeVectorSquare(window, (VECTOR_SQUARE*)src, &rect);
				}
				else if(src->base_data.vector_type == VECTOR_TYPE_RHOMBUS)
				{
					RasterizeVectorRhombus(window, (VECTOR_ECLIPSE*)src, &rect);
				}
				else if(src->base_data.vector_type == VECTOR_TYPE_ECLIPSE)
				{
					RasterizeVectorEclipse(window, (VECTOR_ECLIPSE*)src, &rect);
				}
				else
				{
					RasterizeVectorScript(window, (VECTOR_SCRIPT*)src, &rect);
				}

				DeleteVectorLineLayer(&src->base_data.layer);
				src->base_data.layer = CreateVectorLineLayer(window->work_layer, (VECTOR_LINE*)layer->active_data, &rect);
				BlendVectorLineLayer(src->base_data.layer, dst->base_data.layer);
				src->flags &= ~(VECTOR_LINE_FIX);
			}
			else
			{
				BlendVectorLineLayer(src->base_data.layer, dst->base_data.layer);
			}

			src = (VECTOR_LINE*)src->base_data.next;
		}

		(void)memcpy(target->pixels, dst->base_data.layer->pixels, window->pixel_buf_size);
		(void)memcpy(layer->mix->pixels, dst->base_data.layer->pixels, window->pixel_buf_size);
	}
	else if((layer->flags & VECTOR_LAYER_RASTERIZE_ALL) != 0)
	{
		VECTOR_LINE* rasterize = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;
		(void)memset(layer->mix->pixels, 0, window->active_layer->stride*window->active_layer->height);
		(void)memset(target->pixels, 0, target->stride*target->height);

		if(rasterize == NULL)
		{
			goto end;
		}

		while(rasterize->base_data.next != NULL)
		{
			(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

			if(rasterize->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
			{
				RasterizeVectorLine(window, layer, rasterize, &rect);
			}
			else if(rasterize->base_data.vector_type == VECTOR_TYPE_SQUARE)
			{
				RasterizeVectorSquare(window, (VECTOR_SQUARE*)rasterize, &rect);
			}
			else if(rasterize->base_data.vector_type == VECTOR_TYPE_RHOMBUS)
			{
				RasterizeVectorRhombus(window, (VECTOR_ECLIPSE*)rasterize, &rect);
			}
			else if(rasterize->base_data.vector_type == VECTOR_TYPE_ECLIPSE)
			{
				RasterizeVectorEclipse(window, (VECTOR_ECLIPSE*)rasterize, &rect);
			}
			else
			{
				RasterizeVectorScript(window, (VECTOR_SCRIPT*)rasterize, &rect);
			}

			if((layer->flags & VECTOR_LAYER_FIX_LINE) != 0)
			{
				DeleteVectorLineLayer(&rasterize->base_data.layer);
				rasterize->base_data.layer = CreateVectorLineLayer(window->work_layer, (VECTOR_LINE*)layer->active_data, &rect);
			}

			window->layer_blend_functions[LAYER_BLEND_NORMAL](window->work_layer, target);

			rasterize = (VECTOR_LINE*)rasterize->base_data.next;
		}

		(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

		if(rasterize->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
		{
			RasterizeVectorLine(window, layer, rasterize, &rect);
		}
		else if(rasterize->base_data.vector_type == VECTOR_TYPE_SQUARE)
		{
			RasterizeVectorSquare(window, (VECTOR_SQUARE*)rasterize, &rect);
		}
		else if(rasterize->base_data.vector_type == VECTOR_TYPE_RHOMBUS)
		{
			RasterizeVectorRhombus(window, (VECTOR_ECLIPSE*)rasterize, &rect);
		}
		else if(rasterize->base_data.vector_type == VECTOR_TYPE_ECLIPSE)
		{
			RasterizeVectorEclipse(window, (VECTOR_ECLIPSE*)rasterize, &rect);
		}

		if((layer->flags & VECTOR_LAYER_FIX_LINE) != 0)
		{
			DeleteVectorLineLayer(&rasterize->base_data.layer);
			rasterize->base_data.layer = CreateVectorLineLayer(window->work_layer, (VECTOR_LINE*)layer->active_data, &rect);
		}

		window->layer_blend_functions[LAYER_BLEND_NORMAL](window->work_layer, target);

		(void)memcpy(layer->mix->pixels, target->pixels, window->pixel_buf_size);
	}
	else
	{
		(void)memcpy(target->pixels, layer->mix->pixels, window->pixel_buf_size);
	}

end:
	layer->flags = 0;
}

/***********************************************
* ReadVectorLineData関数                       *
* ベクトルレイヤーのデータを読み込む           *
* 引数                                         *
* data		: ベクトルレイヤーのデータ(圧縮済) *
* target	: データを読み込むレイヤー         *
* 返り値                                       *
*	読み込んだバイト数                         *
***********************************************/
uint32 ReadVectorLineData(uint8* data, LAYER* target)
{
	// ベクトルレイヤーのデータ
	VECTOR_LAYER *layer = target->layer_data.vector_layer_p;
	// ベクトルレイヤーの基本情報読み込み用
	VECTOR_LINE_BASE_DATA line_base;
	// 線データ
	VECTOR_LINE *line;
	// データ読み込み用ストリーム
	MEMORY_STREAM read_stream;
	// ZIP圧縮展開用ストリーム
	MEMORY_STREAM *data_stream;
	// ZIP圧縮展開用データ
	z_stream decompress_stream;
	// 読み込んだ総バイト数
	guint32 data_size;
	// 4バイト読み込み用
	guint32 data32;
	// for文用のカウンタ
	unsigned int i, j;

	// データの総バイト数を読み込む
	(void)memcpy(&data_size, data, sizeof(data_size));

	// 読み込み用データの初期化
	read_stream.buff_ptr = data + sizeof(data_size);
	read_stream.data_point = 0;
	read_stream.data_size = data_size;

	// 展開後のバイト数を読み込む
	(void)MemRead(&data32, sizeof(data32), 1, &read_stream);
	// 展開後のデータを入れるストリーム作成
	data_stream = CreateMemoryStream(data32);

	// データを展開
		// ZIPデータ展開前の準備
	decompress_stream.zalloc = Z_NULL;
	decompress_stream.zfree = Z_NULL;
	decompress_stream.opaque = Z_NULL;
	decompress_stream.avail_in = 0;
	decompress_stream.next_in = Z_NULL;
	(void)inflateInit(&decompress_stream);
	// ZIPデータ展開
	decompress_stream.avail_in = (uInt)data_size;
	decompress_stream.next_in = &read_stream.buff_ptr[read_stream.data_point];
	decompress_stream.avail_out = (uInt)data32;
	decompress_stream.next_out = data_stream->buff_ptr;
	(void)inflate(&decompress_stream, Z_NO_FLUSH);
	(void)inflateEnd(&decompress_stream);

	// 線の数を読み込む
	(void)MemRead(&data32, sizeof(data32), 1, data_stream);
	layer->num_lines = data32;
	// フラグの情報を読み込む
	(void)MemRead(&data32, sizeof(data32), 1, data_stream);
	layer->flags = data32;

	for(i=0; i<layer->num_lines; i++)
	{
		(void)MemRead(&line_base.vector_type, sizeof(line_base.vector_type), 1, data_stream);
		if(line_base.vector_type < NUM_VECTOR_LINE_TYPE)
		{
			(void)MemRead(&line_base.flags, sizeof(line_base.flags), 1, data_stream);
			(void)MemRead(&line_base.num_points, sizeof(line_base.num_points), 1, data_stream);
			(void)MemRead(&line_base.blur, sizeof(line_base.blur), 1, data_stream);
			(void)MemRead(&line_base.outline_hardness, sizeof(line_base.outline_hardness), 1, data_stream);
			line = CreateVectorLine((VECTOR_LINE*)layer->top_data, NULL);
			line->base_data.vector_type = line_base.vector_type;
			line->flags = line_base.flags;
			line->num_points = line_base.num_points;
			line->blur = line_base.blur;
			line->outline_hardness = line_base.outline_hardness;
			// 制御点情報の読み込み
			line->buff_size =
				((line_base.num_points / VECTOR_LINE_BUFFER_SIZE) + 1) * VECTOR_LINE_BUFFER_SIZE;
			line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*line->points)*line->buff_size);
			(void)memset(line->points, 0, sizeof(*line->points)*line->buff_size);
			for(j=0; j<line->num_points; j++)
			{
				(void)MemRead(&line->points[j].vector_type, sizeof(line->points->vector_type), 1, data_stream);
				(void)MemRead(&line->points[j].flags, sizeof(line->points->flags), 1, data_stream);
				(void)MemRead(&line->points[j].color, sizeof(line->points->color), 1, data_stream);
				(void)MemRead(&line->points[j].pressure, sizeof(line->points->pressure), 1, data_stream);
				(void)MemRead(&line->points[j].size, sizeof(line->points->size), 1, data_stream);
				(void)MemRead(&line->points[j].x, sizeof(line->points->x), 1, data_stream);
				(void)MemRead(&line->points[j].y, sizeof(line->points->y), 1, data_stream);
			}

			if(line->points->size == 0.0)
			{
				DeleteVectorLine(&line);
				layer->num_lines--;
			}
			else
			{
				layer->top_data = (void*)line;
			}
		}
		else if(line_base.vector_type > VECTOR_TYPE_SHAPE
			&& line_base.vector_type < VECTOR_SHAPE_END)
		{
			VECTOR_DATA *data = CreateVectorShape((VECTOR_BASE_DATA*)layer->top_data, NULL, line_base.vector_type);
			data_stream->data_point--;
			ReadVectorShapeData(data, data_stream);
			layer->top_data = (void*)data;
		}
		else
		{
			VECTOR_SCRIPT *script =
				(VECTOR_SCRIPT*)CreateVectorScript((VECTOR_BASE_DATA*)layer->top_data, NULL, NULL);
			data_stream->data_point--;
			ReadVectorScriptData(script, data_stream);
			layer->top_data = (void*)script;
		}
	}

	(void)DeleteMemoryStream(data_stream);

	return data_size + sizeof(data_size);
}

/*********************************************************
* WriteVectorLineData関数                                *
* ベクトルレイヤーのデータを書き出す                     *
* 引数                                                   *
* target			: データを書き出すレイヤー           *
* dst				: 書き出し先のポインタ               *
* write_func		: 書き出しに使う関数ポインタ         *
* data_stream		: 圧縮前のデータを作成するストリーム *
* write_stream		: 書き出しデータを作成するストリーム *
* compress_level	: ZIP圧縮のレベル                    *
*********************************************************/
void WriteVectorLineData(
	LAYER* target,
	void* dst,
	stream_func_t write_func,
	MEMORY_STREAM* data_stream,
	MEMORY_STREAM* write_stream,
	int compress_level
)
{
	// ベクトルレイヤーデータの基本情報
	VECTOR_LINE_BASE_DATA line_base;
	// ZIP圧縮用
	z_stream compress_stream;
	// ベクトルレイヤーのデータバイト数
	guint32 vector_data_size;
	guint32 data_size;
	// ラインの数
	guint32 num_line = 0;
	// ラインデータ書き出し用
	VECTOR_LINE *line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)target->layer_data.vector_layer_p->base)->next;
	// 4バイト書き出し用
	guint32 data32;
	// for文用のカウンタ
	unsigned int i;

	// ベクトルの座標、サイズ、筆圧、色を一度メモリに溜める
		// ラインの数を書き込む領域を開けておく
	(void)MemSeek(data_stream, sizeof(guint32), SEEK_SET);
	// ベクトルレイヤーのフラグを書き出す
	data32 = target->layer_data.vector_layer_p->flags;
	(void)MemWrite(&data32, sizeof(data32), 1, data_stream);

	while(line != NULL)
	{
		if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
		{
			line_base.vector_type = line->base_data.vector_type;
			line_base.flags = line->flags;
			line_base.num_points = line->num_points;
			line_base.blur = line->blur;
			line_base.outline_hardness = line->outline_hardness;

			(void)MemWrite(&line_base.vector_type, sizeof(line_base.vector_type), 1, data_stream);
			(void)MemWrite(&line_base.flags, sizeof(line_base.flags), 1, data_stream);
			(void)MemWrite(&line_base.num_points, sizeof(line_base.num_points), 1, data_stream);
			(void)MemWrite(&line_base.blur, sizeof(line_base.blur), 1, data_stream);
			(void)MemWrite(&line_base.outline_hardness, sizeof(line_base.outline_hardness), 1, data_stream);
			for(i=0; i<line->num_points; i++)
			{
				(void)MemWrite(&line->points[i].vector_type, sizeof(line->points->vector_type), 1, data_stream);
				(void)MemWrite(&line->points[i].flags, sizeof(line->points->flags), 1, data_stream);
				(void)MemWrite(&line->points[i].color, sizeof(line->points->color), 1, data_stream);
				(void)MemWrite(&line->points[i].pressure, sizeof(line->points->pressure), 1, data_stream);
				(void)MemWrite(&line->points[i].size, sizeof(line->points->size), 1, data_stream);
				(void)MemWrite(&line->points[i].x, sizeof(line->points->x), 1, data_stream);
				(void)MemWrite(&line->points[i].y, sizeof(line->points->y), 1, data_stream);
			}
		}
		else if(line->base_data.vector_type > VECTOR_TYPE_SHAPE
			&& line->base_data.vector_type < VECTOR_SHAPE_END)
		{
			WriteVectorShapeData((VECTOR_DATA*)line, data_stream);
		}
		else
		{
			WriteVectorScriptData((VECTOR_SCRIPT*)line, data_stream);
		}

		line = (VECTOR_LINE*)line->base_data.next;
		num_line++;
	}

	// データサイズ記憶
	vector_data_size = (guint32)data_stream->data_point;
	// ラインの数書き出し
	(void)MemSeek(data_stream, 0, SEEK_SET);
	(void)MemWrite(&num_line, sizeof(num_line), 1, data_stream);

	// ストリームのサイズが足りなければ再確保
	if(write_stream->data_size < data_stream->data_size)
	{
		write_stream->data_size = data_stream->data_size;
		write_stream->buff_ptr =
			(uint8*)MEM_REALLOC_FUNC(write_stream->buff_ptr, write_stream->data_size);
	}

	// ベクトルデータをZIP圧縮する
		// 圧縮用ストリームのデータをセット
	compress_stream.zalloc = Z_NULL;
	compress_stream.zfree = Z_NULL;
	compress_stream.opaque = Z_NULL;
	(void)deflateInit(&compress_stream, compress_level);
	compress_stream.avail_in = (uInt)vector_data_size;
	compress_stream.next_in = data_stream->buff_ptr;
	compress_stream.avail_out = (uInt)write_stream->data_size;
	compress_stream.next_out = write_stream->buff_ptr;
	// 圧縮実行
	(void)deflate(&compress_stream, Z_FINISH);

	// 圧縮後のデータサイズを書き込む
	data_size = (guint32)(write_stream->data_size - compress_stream.avail_out
		+ sizeof(vector_data_size));
	(void)write_func(&data_size, sizeof(data_size), 1, dst);
	// 圧縮前のデータサイズを書き込む
	(void)write_func(&vector_data_size, sizeof(vector_data_size), 1, dst);
	// 圧縮したデータを書き込む
	(void)write_func(write_stream->buff_ptr, 1,
		data_size - sizeof(data_size), dst);

	// 圧縮用ストリームを開放
	(void)deflateEnd(&compress_stream);
}

/***********************************************
* IsVectorLineInSelectionArea関数              *
* 線が選択範囲内か否かを判定する               *
* 引数                                         *
* window			: 描画領域の情報           *
* selection_pixels	: 選択範囲のピクセルデータ *
* line				: 判定する線               *
* 返り値                                       *
*	選択範囲内:1	選択範囲外:0               *
***********************************************/
int IsVectorLineInSelectionArea(
	DRAW_WINDOW* window,
	uint8* selection_pixels,
	VECTOR_LINE* line
)
{
	FLOAT_T dx, dy;
	FLOAT_T length;
	FLOAT_T t;
	FLOAT_T dt;
	FLOAT_T x, y;
	int check_x, check_y;
	int stride = window->width;
	int i;

	if(line->num_points == 2)
	{
		dx = line->points[1].x - line->points[0].x;
		dy = line->points[1].y - line->points[0].y;
		length = sqrt(dx*dx + dy*dy);
		dx = dx / length,	dy = dy / length;
		x = line->points[0].x,	y = line->points[0].y;
		dt = 1 / length;

		for(t=0; t<1; t+=dt)
		{
			check_x = (int)x,	check_y = (int)y;
			if(check_x >= 0 && check_y >= 0
				&& check_x < window->width && check_y < window->height)
			{
				if(selection_pixels[check_y*stride + check_x] != 0)
				{
					return 1;
				}
			}
			x += dx;
			y += dy;
		}
	}
	else if(line->base_data.vector_type == VECTOR_TYPE_STRAIGHT)
	{
		for(i=0; i<line->num_points-1; i++)
		{
			dx = line->points[i+1].x - line->points[i].x;
			dy = line->points[i+1].y - line->points[i].y;
			length = sqrt(dx*dx + dy*dy);
			dx = dx / length,	dy = dy / length;
			x = line->points[i].x,	y = line->points[i].y;
			dt = 1 / length;

			for(t=0; t<1; t+=dt)
			{
				check_x = (int)x,	check_y = (int)y;
				if(check_x >= 0 && check_y >= 0
					&& check_x < window->width && check_y < window->height)
				{
					if(selection_pixels[check_y*stride + check_x] != 0)
					{
						return 1;
					}
				}
				x += dx;
				y += dy;
			}
		}

		dx = line->points[0].x - line->points[i].x;
		dy = line->points[0].y - line->points[i].y;
		length = sqrt(dx*dx + dy*dy);
		dx = dx / length,	dy = dy / length;
		x = line->points[i].x,	y = line->points[i].y;
		dt = 1 / length;

		for(t=0; t<1; t+=dt)
		{
			check_x = (int)x,	check_y = (int)y;
			if(check_x >= 0 && check_y >= 0
				&& check_x < window->width && check_y < window->height)
			{
				if(selection_pixels[check_y*stride + check_x] != 0)
				{
					return 1;
				}
			}
			x += dx;
			y += dy;
		}
	}
	else if(line->base_data.vector_type == VECTOR_TYPE_BEZIER_OPEN)
	{
		BEZIER_POINT calc[4], inter[2];
		BEZIER_POINT check;
		int j;

		for(j=0; j<3; j++)
		{
			calc[j].x = line->points[j].x;
			calc[j].y = line->points[j].y;
		}
		MakeBezier3EdgeControlPoint(calc, inter);
		calc[1].x = line->points[0].x,	calc[1].y = line->points[0].y;
		calc[2] = inter[0];
		calc[3].x = line->points[1].x,	calc[3].y = line->points[1].y;

		length = 0;
		for(j=1; j<3; j++)
		{
			length += sqrt((calc[j+1].x-calc[j].x)*(calc[j+1].x-calc[j].x)
				+ (calc[j+1].y-calc[j].y)*(calc[j+1].y-calc[j].y));
		}
		dt = 1 / length;

		for(t=0; t<1; t+=dt)
		{
			CalcBezier3(calc, t, &check);
			check_x = (int)check.x,	check_y = (int)check.y;
			if(check_x >= 0 && check_y >= 0
				&& check_x < window->width && check_y < window->height)
			{
				if(selection_pixels[check_y*stride + check_x] != 0)
				{
					return 1;
				}
			}
		}

		for(i=0; i<line->num_points-3; i++)
		{
			for(j=0; j<4; j++)
			{
				calc[j].x = line->points[i+j].x;
				calc[j].y = line->points[i+j].y;
			}
			MakeBezier3ControlPoints(calc, inter);
			calc[0].x = line->points[i+1].x,	calc[0].y = line->points[i+1].y;
			calc[1] = inter[0],	calc[2] = inter[1];
			calc[3].x = line->points[i+2].x,	calc[3].y = line->points[i+2].y;

			length = 0;
			for(j=0; j<3; j++)
			{
				length += sqrt((calc[j+1].x-calc[j].x)*(calc[j+1].x-calc[j].x)
					+ (calc[j+1].y-calc[j].y)*(calc[j+1].y-calc[j].y));
			}
			dt = 1 / length;

			for(t=0; t<1; t+=dt)
			{
				CalcBezier3(calc, t, &check);
				check_x = (int)check.x,	check_y = (int)check.y;
				if(check_x >= 0 && check_y >= 0
					&& check_x < window->width && check_y < window->height)
				{
					if(selection_pixels[check_y*stride + check_x] != 0)
					{
						return 1;
					}
				}
			}
		}

		for(j=0; j<3; j++)
		{
			calc[j].x = line->points[i+j].x;
			calc[j].y = line->points[i+j].y;
		}
		MakeBezier3EdgeControlPoint(calc, inter);

		calc[0].x = line->points[i+1].x,	calc[0].y = line->points[i+1].y;
		calc[1] = inter[1];
		calc[2].x = line->points[i+2].x,	calc[2].y = line->points[i+2].y;
		calc[3].x = line->points[i+2].x,	calc[3].y = line->points[i+2].y;

		length = 0;
		for(j=0; j<2; j++)
		{
			length += sqrt((calc[j+1].x-calc[j].x)*(calc[j+1].x-calc[j].x)
				+ (calc[j+1].y-calc[j].y)*(calc[j+1].y-calc[j].y));
		}
		dt = 1 / length;

		for(t=0; t<1; t+=dt)
		{
			CalcBezier3(calc, t, &check);
			check_x = (int)check.x,	check_y = (int)check.y;
			if(check_x >= 0 && check_y >= 0
				&& check_x < window->width && check_y < window->height)
			{
				if(selection_pixels[check_y*stride + check_x] != 0)
				{
					return 1;
				}
			}
		}
	}
	else if(line->base_data.vector_type == VECTOR_TYPE_STRAIGHT_CLOSE)
	{
		for(i=0; i<line->num_points-1; i++)
		{
			dx = line->points[i+1].x - line->points[i].x;
			dy = line->points[i+1].y - line->points[i].y;
			length = sqrt(dx*dx + dy*dy);
			dx = dx / length,	dy = dy / length;
			x = line->points[i].x,	y = line->points[i].y;
			dt = 1 / length;

			for(t=0; t<1; t+=dt)
			{
				check_x = (int)x,	check_y = (int)y;
				if(check_x >= 0 && check_y >= 0
					&& check_x < window->width && check_y < window->height)
				{
					if(selection_pixels[check_y*stride + check_x] != 0)
					{
						return 1;
					}
				}
				x += dx;
				y += dy;
			}
		}
	}
	else if(line->base_data.vector_type == VECTOR_TYPE_BEZIER_OPEN)
	{
		BEZIER_POINT calc[4], inter[2];
		BEZIER_POINT check;
		int j;

		calc[0].x = line->points[0].x,	calc[0].y = line->points[0].y;
		for(j=0; j<3; j++)
		{
			calc[j+1].x = line->points[j].x;
			calc[j+1].y = line->points[j].y;
		}
		MakeBezier3ControlPoints(calc, inter);
		calc[0].x = line->points[0].x,	calc[0].y = line->points[0].y;
		calc[1] = inter[0];
		calc[2] = inter[1];
		calc[3].x = line->points[1].x,	calc[3].y = line->points[1].y;

		length = 0;
		for(j=0; j<3; j++)
		{
			length += sqrt((calc[j+1].x-calc[j].x)*(calc[j+1].x-calc[j].x)
				+ (calc[j+1].y-calc[j].y)*(calc[j+1].y-calc[j].y));
		}
		dt = 1 / length;

		for(t=0; t<1; t+=dt)
		{
			CalcBezier3(calc, t, &check);
			check_x = (int)check.x,	check_y = (int)check.y;
			if(check_x >= 0 && check_y >= 0
				&& check_x < window->width && check_y < window->height)
			{
				if(selection_pixels[check_y*stride + check_x] != 0)
				{
					return 1;
				}
			}
		}

		for(i=0; i<line->num_points-3; i++)
		{
			for(j=0; j<4; j++)
			{
				calc[j].x = line->points[i+j].x;
				calc[j].y = line->points[i+j].y;
			}
			MakeBezier3ControlPoints(calc, inter);
			calc[0].x = line->points[i+1].x,	calc[0].y = line->points[i+1].y;
			calc[1] = inter[0],	calc[2] = inter[1];
			calc[3].x = line->points[i+2].x,	calc[3].y = line->points[i+2].y;

			length = 0;
			for(j=0; j<3; j++)
			{
				length += sqrt((calc[j+1].x-calc[j].x)*(calc[j+1].x-calc[j].x)
					+ (calc[j+1].y-calc[j].y)*(calc[j+1].y-calc[j].y));
			}
			dt = 1 / length;

			for(t=0; t<1; t+=dt)
			{
				CalcBezier3(calc, t, &check);
				check_x = (int)check.x,	check_y = (int)check.y;
				if(check_x >= 0 && check_y >= 0
					&& check_x < window->width && check_y < window->height)
				{
					if(selection_pixels[check_y*stride + check_x] != 0)
					{
						return 1;
					}
				}
			}
		}

		for(j=0; j<3; j++)
		{
			calc[j].x = line->points[i+j].x;
			calc[j].y = line->points[i+j].y;
		}
		calc[3].x = line->points[0].x,	calc[3].y = line->points[0].y;
		MakeBezier3EdgeControlPoint(calc, inter);

		calc[0].x = line->points[i+1].x,	calc[0].y = line->points[i+1].y;
		calc[1] = inter[0],	calc[2] = inter[1];
		calc[3].x = line->points[i+2].x,	calc[3].y = line->points[i+2].y;

		length = 0;
		for(j=0; j<3; j++)
		{
			length += sqrt((calc[j+1].x-calc[j].x)*(calc[j+1].x-calc[j].x)
				+ (calc[j+1].y-calc[j].y)*(calc[j+1].y-calc[j].y));
		}
		dt = 1 / length;

		for(t=0; t<1; t+=dt)
		{
			CalcBezier3(calc, t, &check);
			check_x = (int)check.x,	check_y = (int)check.y;
			if(check_x >= 0 && check_y >= 0
				&& check_x < window->width && check_y < window->height)
			{
				if(selection_pixels[check_y*stride + check_x] != 0)
				{
					return 1;
				}
			}
		}

		calc[0].x = line->points[i+1].x, calc[0].y = line->points[i+1].y;
		calc[1].x = line->points[i+2].x, calc[1].y = line->points[i+2].y;
		calc[2].x = line->points[0].x, calc[2].y = line->points[0].y;
		calc[3].x = line->points[1].x, calc[3].y = line->points[1].y;
		MakeBezier3ControlPoints(calc, inter);
		calc[0].x = line->points[i+2].x, calc[0].y = line->points[i+2].y;
		calc[1] = inter[0], calc[2] = inter[1];
		calc[3].x = line->points[0].x, calc[3].y = line->points[0].y;

		length = 0;
		for(j=0; j<3; j++)
		{
			length += sqrt((calc[j+1].x-calc[j].x)*(calc[j+1].x-calc[j].x)
				+ (calc[j+1].y-calc[j].y)*(calc[j+1].y-calc[j].y));
		}
		dt = 1 / length;

		for(t=0; t<1; t+=dt)
		{
			CalcBezier3(calc, t, &check);
			check_x = (int)check.x,	check_y = (int)check.y;
			if(check_x >= 0 && check_y >= 0
				&& check_x < window->width && check_y < window->height)
			{
				if(selection_pixels[check_y*stride + check_x] != 0)
				{
					return 1;
				}
			}
		}
	}

	return 0;
}

/**********************************
* ADD_CONTROL_POINT_HISTORY構造体 *
* 制御点追加の履歴データ          *
**********************************/
typedef struct ADD_CONTROL_POINT_HISTORY
{
	gdouble x, y;				// 追加した制御点の座標
	gdouble pressure;			// 追加した制御点の筆圧
	gdouble size;				// 追加した制御点のサイズ
	uint8 color[4];				// 追加した制御点の色
	uint32 line_id;				// 制御点を追加したラインのID
	uint32 point_id;			// 追加した制御点のID
	uint16 layer_name_length;	// 制御点を追加したレイヤーの名前の長さ
	char* layer_name;			// 制御点を追加したレイヤーの名前
} ADD_CONTROL_POINT_HISTORY;

/*****************************
* AddControlPointUndo関数    *
* 制御点の追加を元に戻す     *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void AddControlPointUndo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_CONTROL_POINT_HISTORY history;
	// 制御点を削除するレイヤー
	LAYER* layer;
	// 制御点を削除するライン
	VECTOR_LINE* line;
	// データをバイト単位で扱うためにキャスト
	int8* buff = (int8*)p;
	// カウンタ
	unsigned int i;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_CONTROL_POINT_HISTORY, layer_name));
	buff += offsetof(ADD_CONTROL_POINT_HISTORY, layer_name);
	history.layer_name = buff;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// 制御点を削除するラインを探す
	line = (VECTOR_LINE*)layer->layer_data.vector_layer_p->base;
	for(i=0; i<history.line_id; i++)
	{
		line = line->base_data.next;
	}

	if(line->num_points > history.point_id)
	{
		// 制御点情報を埋めて削除
		(void)memmove(&line->points[history.point_id],
			&line->points[history.point_id+1], sizeof(line->points[0])*(line->num_points-history.point_id));
	}

	line->num_points--;
}

/*****************************
* AddControlPointRedo関数    *
* 制御点の追加をやり直す     *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void AddControlPointRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_CONTROL_POINT_HISTORY history;
	// 制御点を追加するレイヤー
	LAYER* layer;
	// 制御点を追加するライン
	VECTOR_LINE* line;
	// データをバイト単位で扱うためにキャスト
	int8* buff = (int8*)p;
	// カウンタ
	unsigned int i;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_CONTROL_POINT_HISTORY, layer_name));
	buff += offsetof(ADD_CONTROL_POINT_HISTORY, layer_name);
	history.layer_name = buff;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// 制御点を追加するラインを探す
	line = (VECTOR_LINE*)layer->layer_data.vector_layer_p->base;
	for(i=0; i<history.line_id; i++)
	{
		line = line->base_data.next;
	}

	(void)memmove(&line->points[history.point_id+1], &line->points[history.point_id],
		sizeof(line->points[0])*(line->num_points-history.point_id));

	line->points[history.point_id].x = history.x;
	line->points[history.point_id].y = history.y;
	line->points[history.point_id].pressure = history.pressure;
	line->points[history.point_id].size = history.size;
	(void)memcpy(line->points[history.point_id].color, history.color, 4);

	line->num_points++;
}

/*******************************************
* AddControlPointHistory関数               *
* 制御点追加の履歴を作成する               *
* 引数                                     *
* window	: 描画領域の情報               *
* layer		: 制御点を追加したレイヤー     *
* line		: 制御点を追加したライン       *
* point		: 追加した制御点のアドレス     *
* tool_name	: 制御点を追加したツールの名前 *
*******************************************/
void AddControlPointHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_LINE* line,
	VECTOR_POINT* point,
	const char* tool_name
)
{
	// 履歴データ
	ADD_CONTROL_POINT_HISTORY* history;
	// レイヤー名の長さ
	uint16 name_length;
	// 履歴データのサイズ
	size_t data_size;
	// データコピー先のアドレス
	uint8* buff;
	// ラインのID計算用
	VECTOR_LINE* comp_line = (VECTOR_LINE*)layer->layer_data.vector_layer_p->base;
	// for文用のカウンタ
	unsigned int i;

	// レイヤー名の長さを取得
	name_length = (uint16)strlen(layer->name)+1;
	// 履歴データのバイト数を計算してメモリ確保
	data_size = offsetof(ADD_CONTROL_POINT_HISTORY, layer_name) + name_length;
	history = (ADD_CONTROL_POINT_HISTORY*)MEM_ALLOC_FUNC(data_size);
	// 制御点の情報を履歴データにセット
	history->x = point->x, history->y = point->y;
	history->pressure = point->pressure;
	history->size = point->size;
	history->layer_name_length = name_length;
	(void)memcpy(history->color, point->color, 3);

	// ラインのIDを計算
	i = 0;
	while(comp_line != line)
	{
		i++;
		comp_line = comp_line->base_data.next;
	}
	history->line_id = i;

	// 制御点のIDを計算
	for(i=0; i<=line->num_points; i++)
	{
		if(&line->points[i] == point)
		{
			history->point_id = i;
			break;
		}
	}

	// レイヤー名をコピー
	buff = (uint8*)history;
	buff += offsetof(ADD_CONTROL_POINT_HISTORY, layer_name);
	(void)memcpy(buff, layer->name, name_length);

	AddHistory(&window->history, tool_name, history, (uint32)data_size,
		AddControlPointUndo, AddControlPointRedo);
}

/***************************************************
* ADD_TOP_LINE_CONTROL_POINT_HISTORY構造体         *
* 折れ線ツールか曲線ツールでの制御点追加履歴データ *
***************************************************/
typedef struct _ADD_TOP_LINE_CONTROL_POINT_HISTORY
{
	gdouble x, y;				// 追加した制御点の座標
	gdouble pressure;			// 追加した制御点の筆圧
	gdouble size;				// 追加した制御点のサイズ
	uint8 color[4];				// 追加した制御点の色
	gdouble line_x, line_y;		// ラインの開始座標
	gdouble line_pressure;		// ラインの開始筆圧
	gdouble line_size;			// ラインの開始サイズ
	uint8 line_color[4];		// ラインの開始色
	uint8 add_line_flag;		// ラインを追加するフラグ
	uint8 vector_type;			// ラインの種類
	uint16 layer_name_length;	// 制御点を追加したレイヤーの名前の長さ
	char* layer_name;			// 制御点を追加したレイヤーの名前
} ADD_TOP_LINE_CONTROL_POINT_HISTORY;

/*****************************************************
* AddTopLineControlPointUndo関数                     *
* 折れ線ツールか曲線ツールでの制御点の追加を元に戻す *
* 引数                                               *
* window	: 描画領域の情報                         *
* p			: 履歴データ                             *
*****************************************************/
static void AddTopLineControlPointUndo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_TOP_LINE_CONTROL_POINT_HISTORY history;
	// 制御点を追加するレイヤー
	LAYER* layer;
	// 制御点を追加するライン
	VECTOR_LINE *line;
	// データをバイト単位で扱うためにキャスト
	int8 *buff = (int8*)p;
	// 変化したラインを固定するフラグ
	int fix_flag = TRUE;
	// 一番上のラインをフローティングするフラグ
	int float_top = FALSE;
	// ラインのタイプ
	uint8 line_type;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_TOP_LINE_CONTROL_POINT_HISTORY, layer_name));
	buff += offsetof(ADD_TOP_LINE_CONTROL_POINT_HISTORY, layer_name);
	history.layer_name = buff;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// ライン追加の履歴であればライン削除
	if(history.add_line_flag != FALSE)
	{
		line = (VECTOR_LINE*)layer->layer_data.vector_layer_p->top_data;
		line_type = line->base_data.vector_type;
		layer->layer_data.vector_layer_p->top_data =
			((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->prev;
		DeleteVectorLine(&line);
		fix_flag = FALSE;
	}
	else
	{	// 最後の点を削除
		line = (VECTOR_LINE*)layer->layer_data.vector_layer_p->top_data;
		line_type = line->base_data.vector_type;
		line->num_points--;
	}

	if(layer == window->active_layer)
	{
		if(line_type == VECTOR_TYPE_STRAIGHT
			&& window->app->tool_window.active_vector_brush[window->app->input]->brush_type == TYPE_POLY_LINE_BRUSH)
		{
			POLY_LINE_BRUSH* brush = (POLY_LINE_BRUSH*)window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			if((brush->flags & POLY_LINE_START) != 0)
			{
				if(history.add_line_flag != 0)
				{
					brush->flags &= ~(POLY_LINE_START);
					(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				}
				else if(line != NULL)
				{
					if(line->base_data.layer != NULL)
					{
						layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ACTIVE;
						layer->layer_data.vector_layer_p->active_data = (void*)line;
						brush->flags |= POLY_LINE_START;
						DeleteVectorLineLayer(&line->base_data.layer);
					}
				}
				fix_flag = FALSE;
			}
			else if(line != NULL)
			{
				if(line->base_data.layer != NULL)
				{
					brush->flags |= POLY_LINE_START;
					layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ACTIVE;
					layer->layer_data.vector_layer_p->active_data = (void*)line;
					brush->flags |= BEZIER_LINE_START;
					DeleteVectorLineLayer(&line->base_data.layer);
					line->num_points++;
					float_top = TRUE;
					//fix_flag = FALSE;
				}
			}
		}
		else if(line_type == VECTOR_TYPE_BEZIER_OPEN
			&& window->app->tool_window.active_vector_brush[window->app->input]->brush_type == TYPE_BEZIER_LINE_BRUSH)
		{
			BEZIER_LINE_BRUSH* brush = (BEZIER_LINE_BRUSH*)window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			if((brush->flags & BEZIER_LINE_START) != 0)
			{
				if(history.add_line_flag != 0)
				{
					brush->flags &= ~(BEZIER_LINE_START);
					(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				}
				fix_flag = FALSE;
			}
			else if(line != NULL)
			{
				if(line->base_data.layer != NULL)
				{
					brush->flags |= BEZIER_LINE_START;
					layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ACTIVE;
					layer->layer_data.vector_layer_p->active_data = (void*)line;
					brush->flags |= BEZIER_LINE_START;
					DeleteVectorLineLayer(&line->base_data.layer);
					line->num_points++;
					float_top = TRUE;
					// fix_flag = FALSE;
				}
			}
		}
	}

	// ラインをラスタライズ
	if(layer->layer_data.vector_layer_p->flags != VECTOR_LAYER_RASTERIZE_ACTIVE)
	{
		layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
		if(fix_flag != FALSE)
		{
			layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
		}
		RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
	}
	else
	{
		VECTOR_LINE *before_top = NULL;
		if(float_top != FALSE)
		{
			before_top = (VECTOR_LINE*)layer->layer_data.vector_layer_p->top_data;
			((VECTOR_BASE_DATA*)before_top->base_data.prev)->next = NULL;
			DeleteVectorLineLayer(&((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->layer);
		}
		if(fix_flag != FALSE)
		{
			layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
		}
		RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
		if(float_top != FALSE)
		{
			((VECTOR_BASE_DATA*)before_top->base_data.prev)->next = (void*)before_top;
		}
	}
	layer->layer_data.vector_layer_p->active_data = NULL;
	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

	// 表示を更新
	if(layer == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}

	layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
}

/*****************************************************
* AddTopLineControlPointRedo関数                     *
* 折れ線ツールか曲線ツールでの制御点の追加をやり直す *
* 引数                                               *
* window	: 描画領域の情報                         *
* p			: 履歴データ                             *
*****************************************************/
static void AddTopLineControlPointRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_TOP_LINE_CONTROL_POINT_HISTORY history;
	// 制御点を追加するレイヤー
	LAYER* layer;
	// 制御点を追加するライン
	VECTOR_LINE* line;
	// データをバイト単位で扱うためにキャスト
	int8* buff = (int8*)p;
	// 変化したラインを固定するフラグ
	int fix_flag = TRUE;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_TOP_LINE_CONTROL_POINT_HISTORY, layer_name));
	buff += offsetof(ADD_TOP_LINE_CONTROL_POINT_HISTORY, layer_name);
	history.layer_name = buff;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// ライン追加の履歴であればラインメモリ確保
	if(history.add_line_flag == 0)
	{
		line = (VECTOR_LINE*)layer->layer_data.vector_layer_p->top_data;
	}
	else
	{	// ライン追加
		line = CreateVectorLine((VECTOR_LINE*)layer->layer_data.vector_layer_p->top_data,
			(VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->next);
		// 制御点バッファ作成
		line->buff_size = VECTOR_LINE_BUFFER_SIZE;
		line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*line->points)*line->buff_size);
		// 開始点を設定
		line->points->x = history.line_x, line->points->y = history.line_y;
		line->points->pressure = history.line_pressure;
		line->points->size = history.line_size;
		(void)memcpy(line->points->color, history.line_color, 4);
		line->num_points = 1;
		// 一番上のライン変更
		layer->layer_data.vector_layer_p->top_data = (void*)line;
		line->base_data.vector_type = history.vector_type;
	}

	line->points[line->num_points].x = history.x;
	line->points[line->num_points].y = history.y;
	line->points[line->num_points].pressure = history.pressure;
	line->points[line->num_points].size = history.size;
	(void)memcpy(line->points[line->num_points].color, history.color, 4);

	if(layer == window->active_layer)
	{
		if(line->base_data.vector_type == VECTOR_TYPE_STRAIGHT
			&& window->app->tool_window.active_vector_brush[window->app->input]->brush_type == TYPE_POLY_LINE_BRUSH)
		{
			POLY_LINE_BRUSH* brush = (POLY_LINE_BRUSH*)window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			brush->flags |= POLY_LINE_START;
			fix_flag = FALSE;
		}
		else if(line->base_data.vector_type == VECTOR_TYPE_BEZIER_OPEN
			&& window->app->tool_window.active_vector_brush[window->app->input]->brush_type == TYPE_BEZIER_LINE_BRUSH)
		{
			BEZIER_LINE_BRUSH* brush = (BEZIER_LINE_BRUSH*)window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			brush->flags |= BEZIER_LINE_START;
			fix_flag = FALSE;
		}
	}

	line->num_points++;

	// ラインをラスタライズ
	layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
	if(fix_flag != FALSE)
	{
		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
	}
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

	// 表示を更新
	if(layer == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}
}

/***************************************************
* AddTopLineControlPointHistory関数                *
* 折れ線ツールか曲線ツールでの制御点追加履歴を追加 *
* 引数                                             *
* window		: 描画領域の情報                   *
* layer			: 制御点を追加したレイヤー         *
* line			: 制御点を追加したライン           *
* point			: 追加した制御点                   *
* tool_name		: 制御点を追加したツールの名前     *
* add_line_flag	: ライン追加をしたときのフラグ     *
***************************************************/
void AddTopLineControlPointHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_LINE* line,
	VECTOR_POINT* point,
	const char* tool_name,
	uint8 add_line_flag
)
{
	// 履歴データ
	ADD_TOP_LINE_CONTROL_POINT_HISTORY *history;
	// レイヤー名の長さ
	uint16 name_length;
	// 履歴データのサイズ
	size_t data_size;
	// データコピー先のアドレス
	uint8 *buff;

	// レイヤー名の長さを取得
	name_length = (uint16)strlen(layer->name)+1;
	// 履歴データのバイト数を計算してメモリ確保
	data_size = offsetof(ADD_TOP_LINE_CONTROL_POINT_HISTORY, layer_name) + name_length;
	history = (ADD_TOP_LINE_CONTROL_POINT_HISTORY*)MEM_ALLOC_FUNC(data_size);
	// 制御点の情報を履歴データにセット
	history->x = point->x, history->y = point->y;
	history->pressure = point->pressure;
	history->size = point->size;
	history->add_line_flag = add_line_flag;
	history->vector_type = line->base_data.vector_type;
	history->layer_name_length = name_length;
	(void)memcpy(history->color, point->color, 3);
	// ラインの開始位置の情報を履歴データにセット
	history->line_x = line->points->x;
	history->line_y = line->points->y;
	history->line_pressure = line->points->pressure;
	history->line_size = line->points->size;
	(void)memcpy(history->line_color, line->points->color, 4);

	// レイヤー名をコピー
	buff = (uint8*)history;
	buff += offsetof(ADD_TOP_LINE_CONTROL_POINT_HISTORY, layer_name);
	(void)memcpy(buff, layer->name, name_length);

	AddHistory(&window->history, tool_name, history, (uint32)data_size,
		AddTopLineControlPointUndo, AddTopLineControlPointRedo);
}

/*****************************************
* ADD_TOP_SQUARE_HISTORY構造体           *
* 形状作成ツールでの四角形追加履歴データ *
*****************************************/
typedef struct _ADD_TOP_SQUARE_HISTORY
{
	VECTOR_SQUARE square;
	uint16 layer_name_length;	// 四角形を追加したレイヤーの名前の長さ
	char* layer_name;			// 四角形を追加したレイヤーの名前
} ADD_TOP_SQUARE_HISTORY;

/*****************************
* AddTopSquareUndo関数       *
* 四角形追加を元に戻す       *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void AddTopSquareUndo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_TOP_SQUARE_HISTORY history;
	// 四角形の削除用
	VECTOR_SQUARE *square;
	// 四角形を追加したレイヤー
	LAYER *layer;
	// データをバイト単位で扱うためにキャスト
	int8 *buff = (int8*)p;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_TOP_SQUARE_HISTORY, layer_name));
	buff += offsetof(ADD_TOP_SQUARE_HISTORY, layer_name);
	history.layer_name = buff;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// 一番上の四角形を削除
	square = (VECTOR_SQUARE*)layer->layer_data.vector_layer_p->top_data;
	layer->layer_data.vector_layer_p->top_data = ((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->prev;
	DeleteVectorShape(((VECTOR_BASE_DATA**)(&square)));

	// 形状をラスタライズ
	if(layer->layer_data.vector_layer_p->flags != VECTOR_LAYER_RASTERIZE_ACTIVE)
	{
		layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
		RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
	}
	else
	{
		VECTOR_BASE_DATA *before_top = NULL;
		before_top = (VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data;
		layer->layer_data.vector_layer_p->top_data = ((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->prev;
		((VECTOR_BASE_DATA*)before_top->prev)->next = NULL;
		DeleteVectorLineLayer(&((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->layer);

		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
		RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
		((VECTOR_BASE_DATA*)before_top->prev)->next = (void*)before_top;
	}
	layer->layer_data.vector_layer_p->active_data = NULL;
	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

	// 表示を更新
	if(layer == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}

	layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
}

/*****************************
* AddTopSquareRedo関数       *
* 長方形の追加をやり直す     *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void AddTopSquareRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_TOP_SQUARE_HISTORY history;
	// 長方形を追加するレイヤー
	LAYER *layer;
	// 追加する長方形
	VECTOR_SQUARE *square;
	// 前後ベクトルデータの一時保存
	VECTOR_BASE_DATA before_base;
	// データをバイト単位で扱うためにキャスト
	int8 *buff = (int8*)p;
	// ベクトルデータを固定するフラグ
	gboolean fix_flag = TRUE;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_TOP_SQUARE_HISTORY, layer_name));
	buff += offsetof(ADD_TOP_SQUARE_HISTORY, layer_name);
	history.layer_name = buff;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// 四角形追加
	square = (VECTOR_SQUARE*)CreateVectorShape((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data,
			(VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->next, VECTOR_TYPE_SQUARE);
	before_base = square->base_data;
	*square = history.square;
	square->base_data = before_base;
	// 一番上のベクトル変更
	layer->layer_data.vector_layer_p->top_data = (void*)square;

	if(layer == window->active_layer)
	{
		if(window->app->tool_window.active_vector_brush[window->app->input]->brush_type == TYPE_VECTOR_SHAPE_BRUSH)
		{
			//POLY_LINE_BRUSH* brush = (POLY_LINE_BRUSH*)window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			//brush->flags |= POLY_LINE_START;
			fix_flag = FALSE;
		}
	}

	// ベクトルデータをラスタライズ
	layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
	if(fix_flag != FALSE)
	{
		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
	}
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

	// 表示を更新
	if(layer == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}
}

/***********************************************
* AddTopSquareHistory関数                      *
* 四角形追加履歴を追加                         *
* 引数                                         *
* window		: 描画領域の情報               *
* layer			: 四角形を追加したレイヤー     *
* square		: 追加した四角形               *
* tool_name		: 四角形を追加したツールの名前 *
***********************************************/
void AddTopSquareHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_SQUARE* square,
	const char* tool_name
)
{
	// 履歴データ
	ADD_TOP_SQUARE_HISTORY *history;
	// レイヤー名の長さ
	uint16 name_length;
	// 履歴データのサイズ
	size_t data_size;
	// データコピー先のアドレス
	uint8 *buff;

	// レイヤー名の長さを取得
	name_length = (uint16)strlen(layer->name)+1;
	// 履歴データのバイト数を計算してメモリ確保
	data_size = offsetof(ADD_TOP_SQUARE_HISTORY, layer_name) + name_length;
	history = (ADD_TOP_SQUARE_HISTORY*)MEM_ALLOC_FUNC(data_size);
	// 四角形の情報を履歴データにセット
	history->square = *square;

	// レイヤー名をコピー
	buff = (uint8*)history;
	buff += offsetof(ADD_TOP_SQUARE_HISTORY, layer_name);
	(void)memcpy(buff, layer->name, name_length);

	AddHistory(&window->history, tool_name, history, (uint32)data_size,
		AddTopSquareUndo, AddTopSquareRedo);
}

/***************************************
* ADD_TOP_ECLIPSE_HISTORY構造体        *
* 形状作成ツールでの楕円追加履歴データ *
***************************************/
typedef struct _ADD_TOP_ECLIPSE_HISTORY
{
	VECTOR_ECLIPSE eclipse;
	uint16 layer_name_length;	// 楕円を追加したレイヤーの名前の長さ
	char* layer_name;			// 楕円を追加したレイヤーの名前
} ADD_TOP_ECLIPSE_HISTORY;

/*****************************
* AddTopEclipseUndo関数      *
* 楕円追加を元に戻す         *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void AddTopEclipseUndo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_TOP_SQUARE_HISTORY history;
	// 楕円の削除用
	VECTOR_SQUARE *square;
	// 楕円を追加したレイヤー
	LAYER *layer;
	// データをバイト単位で扱うためにキャスト
	int8 *buff = (int8*)p;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_TOP_ECLIPSE_HISTORY, layer_name));
	buff += offsetof(ADD_TOP_ECLIPSE_HISTORY, layer_name);
	history.layer_name = buff;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// 一番上の楕円を削除
	square = (VECTOR_SQUARE*)layer->layer_data.vector_layer_p->top_data;
	layer->layer_data.vector_layer_p->top_data = ((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->prev;
	DeleteVectorShape(((VECTOR_BASE_DATA**)(&square)));

	// 形状をラスタライズ
	if(layer->layer_data.vector_layer_p->flags != VECTOR_LAYER_RASTERIZE_ACTIVE)
	{
		layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
		RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
	}
	else
	{
		VECTOR_BASE_DATA *before_top = NULL;
		before_top = (VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data;
		layer->layer_data.vector_layer_p->top_data = ((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->prev;
		((VECTOR_BASE_DATA*)before_top->prev)->next = NULL;
		DeleteVectorLineLayer(&((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->layer);

		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
		RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
		((VECTOR_BASE_DATA*)before_top->prev)->next = (void*)before_top;
	}
	layer->layer_data.vector_layer_p->active_data = NULL;
	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

	// 表示を更新
	if(layer == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}

	layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
}

/*****************************
* AddTopEclipseRedo関数      *
* 楕円の追加をやり直す       *
* 引数                       *
* window	: 描画領域の情報 *
* p			: 履歴データ     *
*****************************/
static void AddTopEclipseRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_TOP_ECLIPSE_HISTORY history;
	// 楕円を追加するレイヤー
	LAYER *layer;
	// 追加する楕円
	VECTOR_ECLIPSE *eclipse;
	// 前後ベクトルデータの一時保存
	VECTOR_BASE_DATA before_base;
	// データをバイト単位で扱うためにキャスト
	int8 *buff = (int8*)p;
	// ベクトルデータを固定するフラグ
	gboolean fix_flag = TRUE;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_TOP_ECLIPSE_HISTORY, layer_name));
	buff += offsetof(ADD_TOP_ECLIPSE_HISTORY, layer_name);
	history.layer_name = buff;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// 楕円追加
	eclipse = (VECTOR_ECLIPSE*)CreateVectorShape((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data,
		(VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->next, history.eclipse.base_data.vector_type);
	before_base = eclipse->base_data;
	*eclipse = history.eclipse;
	eclipse->base_data = before_base;
	// 一番上のベクトル変更
	layer->layer_data.vector_layer_p->top_data = (void*)eclipse;

	if(layer == window->active_layer)
	{
		if(window->app->tool_window.active_vector_brush[window->app->input]->brush_type == TYPE_VECTOR_SHAPE_BRUSH)
		{
			//POLY_LINE_BRUSH* brush = (POLY_LINE_BRUSH*)window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			//brush->flags |= POLY_LINE_START;
			fix_flag = FALSE;
		}
	}

	// ベクトルデータをラスタライズ
	layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
	if(fix_flag != FALSE)
	{
		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
	}
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

	// 表示を更新
	if(layer == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}
}


/*********************************************
* AddTopEclipseHistory関数                   *
* 楕円追加履歴を追加                         *
* 引数                                       *
* window		: 描画領域の情報             *
* layer			: 楕円を追加したレイヤー     *
* eclipse		: 追加した楕円               *
* tool_name		: 楕円を追加したツールの名前 *
*********************************************/
void AddTopEclipseHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_ECLIPSE* eclipse,
	const char* tool_name
)
{
	// 履歴データ
	ADD_TOP_ECLIPSE_HISTORY *history;
	// レイヤー名の長さ
	uint16 name_length;
	// 履歴データのサイズ
	size_t data_size;
	// データコピー先のアドレス
	uint8 *buff;

	// レイヤー名の長さを取得
	name_length = (uint16)strlen(layer->name)+1;
	// 履歴データのバイト数を計算してメモリ確保
	data_size = offsetof(ADD_TOP_ECLIPSE_HISTORY, layer_name) + name_length;
	history = (ADD_TOP_ECLIPSE_HISTORY*)MEM_ALLOC_FUNC(data_size);
	// 楕円の情報を履歴データにセット
	history->eclipse = *eclipse;

	// レイヤー名をコピー
	buff = (uint8*)history;
	buff += offsetof(ADD_TOP_ECLIPSE_HISTORY, layer_name);
	(void)memcpy(buff, layer->name, name_length);

	AddHistory(&window->history, tool_name, history, (uint32)data_size,
		AddTopEclipseUndo, AddTopEclipseRedo);
}

/*********************************************
* ADD_TOP_SCRIPT_HISTORY構造体               *
* 形状作成ツールでのスクリプト追加履歴データ *
*********************************************/
typedef struct _ADD_TOP_SCRIPT_HISTORY
{
	uint32 script_length;		// スクリプトデータの長さ
	uint16 layer_name_length;	// スクリプトを追加したレイヤーの名前の長さ
	char *layer_name;			// スクリプトを追加したレイヤーの名前
	char *script_data;			// スクリプトデータ
} ADD_TOP_SCRIPT_HISTORY;

/*************************************
* AddTopScriptUndo関数               *
* スクリプトのベクトル追加を元に戻す *
* 引数                               *
* window	: 描画領域の情報         *
* p			: 履歴データ             *
*************************************/
static void AddTopScriptUndo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_TOP_SCRIPT_HISTORY history;
	// スクリプトの削除用
	VECTOR_SCRIPT *script;
	// スクリプトを追加したレイヤー
	LAYER *layer;
	// データをバイト単位で扱うためにキャスト
	int8 *buff = (int8*)p;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_TOP_SCRIPT_HISTORY, layer_name));
	buff += offsetof(ADD_TOP_ECLIPSE_HISTORY, layer_name);
	history.layer_name = buff;
	history.script_data = buff + history.layer_name_length;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// 一番上のスクリプトを削除
	script = (VECTOR_SCRIPT*)layer->layer_data.vector_layer_p->top_data;
	layer->layer_data.vector_layer_p->top_data = ((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->prev;
	DeleteVectorScript(&script);

	// 形状をラスタライズ
	if(layer->layer_data.vector_layer_p->flags != VECTOR_LAYER_RASTERIZE_ACTIVE)
	{
		layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
		RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
	}
	else
	{
		VECTOR_BASE_DATA *before_top = NULL;
		before_top = (VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data;
		layer->layer_data.vector_layer_p->top_data = ((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->prev;
		((VECTOR_BASE_DATA*)before_top->prev)->next = NULL;
		DeleteVectorLineLayer(&((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->layer);

		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
		RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
		((VECTOR_BASE_DATA*)before_top->prev)->next = (void*)before_top;
	}
	layer->layer_data.vector_layer_p->active_data = NULL;
	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

	// 表示を更新
	if(layer == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}

	layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
}

/*************************************
* AddTopScriptRedo関数               *
* スクリプトのベクトル追加をやり直す *
* 引数                               *
* window	: 描画領域の情報         *
* p			: 履歴データ             *
*************************************/
static void AddTopScriptRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データ
	ADD_TOP_SCRIPT_HISTORY history;
	// 長方形を追加するレイヤー
	LAYER *layer;
	// 追加する楕円
	VECTOR_SCRIPT *script;
	// データをバイト単位で扱うためにキャスト
	int8 *buff = (int8*)p;
	// ベクトルデータを固定するフラグ
	gboolean fix_flag = TRUE;

	// データをコピー
	(void)memcpy(&history, p, offsetof(ADD_TOP_ECLIPSE_HISTORY, layer_name));
	buff += offsetof(ADD_TOP_ECLIPSE_HISTORY, layer_name);
	history.layer_name = buff;
	history.script_data = buff + history.layer_name_length;

	// レイヤーを名前で探す
	layer = SearchLayer(window->layer, history.layer_name);

	// スクリプト追加
	script = (VECTOR_SCRIPT*)CreateVectorScript((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data,
			(VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->top_data)->next, history.script_data);
	// 一番上のベクトル変更
	layer->layer_data.vector_layer_p->top_data = (void*)script;

	if(layer == window->active_layer)
	{
		if(window->app->tool_window.active_vector_brush[window->app->input]->brush_type == TYPE_VECTOR_SHAPE_BRUSH)
		{
			//POLY_LINE_BRUSH* brush = (POLY_LINE_BRUSH*)window->app->tool_window.active_vector_brush[window->app->input]->brush_data;
			//brush->flags |= POLY_LINE_START;
			fix_flag = FALSE;
		}
	}

	// ベクトルデータをラスタライズ
	layer->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_TOP;
	if(fix_flag != FALSE)
	{
		layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE;
	}
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

	// 表示を更新
	if(layer == window->active_layer)
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
	else
	{
		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}
}


/*********************************************
* AddTopScriptHistory関数                    *
* スクリプトのいベクトル追加履歴を追加       *
* 引数                                       *
* window		: 描画領域の情報             *
* layer			: 楕円を追加したレイヤー     *
* eclipse		: 追加した楕円               *
* tool_name		: 楕円を追加したツールの名前 *
*********************************************/
void AddTopScriptHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_SCRIPT* script,
	const char* tool_name
)
{
	// 履歴データ
	ADD_TOP_SCRIPT_HISTORY *history;
	// レイヤー名の長さ
	uint16 name_length;
	// スクリプトデータの長さ
	uint32 script_length = 0;
	// 履歴データのサイズ
	size_t data_size;
	// データコピー先のアドレス
	uint8 *buff;

	// レイヤー名の長さを取得
	name_length = (uint16)strlen(layer->name)+1;
	// スクリプトデータの長さを取得
	if(script->script_data != NULL)
	{
		script_length = (uint32)strlen(script->script_data)+1;
	}
	// 履歴データのバイト数を計算してメモリ確保
	data_size = offsetof(ADD_TOP_SCRIPT_HISTORY, layer_name) + name_length + script_length;
	history = (ADD_TOP_SCRIPT_HISTORY*)MEM_ALLOC_FUNC(data_size);

	// レイヤー名をコピー
	buff = (uint8*)history;
	buff += offsetof(ADD_TOP_SCRIPT_HISTORY, layer_name);
	(void)memcpy(buff, layer->name, name_length);
	// スクリプトデータをコピー
	buff += name_length;
	(void)memcpy(buff, script->script_data, script_length);

	AddHistory(&window->history, tool_name, history, (uint32)data_size,
		AddTopScriptUndo, AddTopScriptRedo);
}

typedef struct _VECTOR_SHAPE_DELETE_HISTORY
{
	VECTOR_DATA vector_data;
	int32 vector_index;
	uint16 layer_name_length;
	char *layer_name;
} VECTOR_SHAPE_DELETE_HISTORY;

static void DeleteVectorShapeUndo(DRAW_WINDOW* window, void* p)
{
	VECTOR_SHAPE_DELETE_HISTORY *history_data =
		(VECTOR_SHAPE_DELETE_HISTORY*)p;
	LAYER *layer = window->layer;
	VECTOR_BASE_DATA *shape;
	char *name = &(((char*)history_data)[offsetof(VECTOR_SHAPE_DELETE_HISTORY, layer_name)]);
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	shape = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base);
	for(i=0; i<history_data->vector_index; i++)
	{
		shape = shape->next;
	}

	if(history_data->vector_data.line.base_data.vector_type == VECTOR_TYPE_SQUARE)
	{
		VECTOR_SQUARE *square = (VECTOR_SQUARE*)CreateVectorShape(shape, (VECTOR_BASE_DATA*)shape->next,
			history_data->vector_data.line.base_data.vector_type);
		VECTOR_BASE_DATA before_base = square->base_data;

		*square = history_data->vector_data.square;
		square->base_data = before_base;
		shape = (VECTOR_BASE_DATA*)square;
	}
	else if(history_data->vector_data.line.base_data.vector_type == VECTOR_TYPE_RHOMBUS
		|| history_data->vector_data.line.base_data.vector_type == VECTOR_TYPE_ECLIPSE)
	{
		VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)CreateVectorShape(shape, (VECTOR_BASE_DATA*)shape->next,
			history_data->vector_data.line.base_data.vector_type);
		VECTOR_BASE_DATA before_base = eclipse->base_data;

		*eclipse = history_data->vector_data.eclipse;
		eclipse->base_data = before_base;
		shape = (VECTOR_BASE_DATA*)eclipse;
	}

	layer->layer_data.vector_layer_p->active_data = (void*)shape;
	if(shape->next == NULL)
	{
		layer->layer_data.vector_layer_p->top_data = (void*)shape;
	}
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

	layer->layer_data.vector_layer_p->active_data = (void*)NULL;
}

static void DeleteVectorShapeRedo(DRAW_WINDOW* window, void* p)
{
	VECTOR_SHAPE_DELETE_HISTORY *history_data =
		(VECTOR_SHAPE_DELETE_HISTORY*)p;
	LAYER *layer = window->layer;
	VECTOR_BASE_DATA *shape;
	char *name = &(((char*)history_data)[offsetof(VECTOR_SHAPE_DELETE_HISTORY, layer_name)]);
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	shape = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base);
	for(i=0; i<history_data->vector_index; i++)
	{
		shape = shape->next;
	}

	if(shape->next == NULL)
	{
		layer->layer_data.vector_layer_p->top_data = shape->prev;
	}
	DeleteVectorShape(&shape);

	layer->layer_data.vector_layer_p->active_data = (void*)NULL;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

/*************************************************
* AddDeleteVectorShapeHistory関数                *
* 四角形, 楕円削除の履歴を追加                   *
* 引数                                           *
* window		: 描画領域の情報                 *
* layer			: ベクトルを削除したレイヤー     *
* delete_shape	: 削除したベクトルデータ         *
* tool_name		: ベクトルデータを削除したツール *
*************************************************/
void AddDeleteVectorShapeHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_BASE_DATA* delete_shape,
	const char* tool_name
)
{
	uint8 *byte_data;
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name);
	VECTOR_SHAPE_DELETE_HISTORY *history_data =
		(VECTOR_SHAPE_DELETE_HISTORY*)MEM_ALLOC_FUNC(
			offsetof(VECTOR_SHAPE_DELETE_HISTORY, layer_name) + layer_name_length + 4);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_BASE_DATA *shape = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int shape_index = 0;

	while(shape != delete_shape)
	{
		shape = shape->next;
		shape_index++;
	}

	if(shape->vector_type == VECTOR_TYPE_SQUARE)
	{
		history_data->vector_data.square = *((VECTOR_SQUARE*)delete_shape);
	}
	else if(shape->vector_type == VECTOR_TYPE_RHOMBUS
		|| shape->vector_type == VECTOR_TYPE_ECLIPSE)
	{
		history_data->vector_data.eclipse = *((VECTOR_ECLIPSE*)delete_shape);
	}

	history_data->vector_index = (int32)shape_index;
	history_data->layer_name_length = layer_name_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(VECTOR_SHAPE_DELETE_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length + 1);

	AddHistory(&window->history, tool_name,
		(void*)history_data, offsetof(VECTOR_SHAPE_DELETE_HISTORY, layer_name) + layer_name_length + 4,
			DeleteVectorShapeUndo, DeleteVectorShapeRedo
	);
}

typedef struct _VECTOR_SCRIPT_DELETE_HISTORY
{
	uint32 script_length;
	int32 vector_index;
	uint16 layer_name_length;
	char *layer_name;
	char *script_data;
} VECTOR_SCRIPT_DELETE_HISTORY;

static void DeleteVectorScriptUndo(DRAW_WINDOW* window, void* p)
{
	VECTOR_SCRIPT_DELETE_HISTORY *history_data =
		(VECTOR_SCRIPT_DELETE_HISTORY*)p;
	LAYER *layer = window->layer;
	VECTOR_BASE_DATA *script;
	char *name = &(((char*)history_data)[offsetof(VECTOR_SCRIPT_DELETE_HISTORY, layer_name)]);
	char *script_data = &(((char*)history_data)[offsetof(VECTOR_SCRIPT_DELETE_HISTORY, layer_name)
		+ history_data->layer_name_length]);
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	script = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base);
	for(i=0; i<history_data->vector_index; i++)
	{
		script = script->next;
	}

	{
		VECTOR_SCRIPT *add = (VECTOR_SCRIPT*)CreateVectorScript(script, (VECTOR_BASE_DATA*)script->next,
			script_data);
		layer->layer_data.vector_layer_p->active_data = (void*)add;
		script = &add->base_data;
	}

	layer->layer_data.vector_layer_p->active_data = (void*)script;
	if(script->next == NULL)
	{
		layer->layer_data.vector_layer_p->top_data = (void*)script;
	}
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ACTIVE;

	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

	layer->layer_data.vector_layer_p->active_data = (void*)NULL;
}

static void DeleteVectorScriptRedo(DRAW_WINDOW* window, void* p)
{
	VECTOR_SCRIPT_DELETE_HISTORY *history_data =
		(VECTOR_SCRIPT_DELETE_HISTORY*)p;
	LAYER *layer = window->layer;
	VECTOR_BASE_DATA *script;
	char *name = &(((char*)history_data)[offsetof(VECTOR_SHAPE_DELETE_HISTORY, layer_name)]);
	int i;

	while(strcmp(layer->name, name) != 0)
	{
		layer = layer->next;
	}

	script = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base);
	for(i=0; i<history_data->vector_index; i++)
	{
		script = script->next;
	}

	if(script->next == NULL)
	{
		layer->layer_data.vector_layer_p->top_data = script->prev;
	}
	DeleteVectorScript((VECTOR_SCRIPT**)&script);

	layer->layer_data.vector_layer_p->active_data = (void*)NULL;
	layer->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ACTIVE;
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

/*************************************************
* AddDeleteVectorScriptHistory関数               *
* スクリプトのベクトル削除の履歴を追加           *
* 引数                                           *
* window		: 描画領域の情報                 *
* layer			: ベクトルを削除したレイヤー     *
* delete_script	: 削除したベクトルデータ         *
* tool_name		: ベクトルデータを削除したツール *
*************************************************/
void AddDeleteVectorScriptHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_SCRIPT* delete_script,
	const char* tool_name
)
{
	uint8 *byte_data;
	uint16 layer_name_length = (uint16)strlen(window->active_layer->name)+1;
	uint32 script_length = (uint32)strlen(delete_script->script_data)+1;
	VECTOR_SCRIPT_DELETE_HISTORY *history_data =
		(VECTOR_SCRIPT_DELETE_HISTORY*)MEM_ALLOC_FUNC(
			offsetof(VECTOR_SCRIPT_DELETE_HISTORY, layer_name) + layer_name_length + 4);
	VECTOR_LAYER *layer_data = window->active_layer->layer_data.vector_layer_p;
	VECTOR_BASE_DATA *script = (VECTOR_BASE_DATA*)((VECTOR_BASE_DATA*)layer_data->base)->next;
	int script_index = 0;

	while(script != &delete_script->base_data)
	{
		script = script->next;
		script_index++;
	}

	history_data->vector_index = (int32)script_index;
	history_data->layer_name_length = layer_name_length;
	history_data->script_length = script_length;
	byte_data = (uint8*)history_data;
	(void)memcpy(&byte_data[offsetof(VECTOR_SHAPE_DELETE_HISTORY, layer_name)],
		window->active_layer->name, layer_name_length);
	(void)memcpy(&byte_data[offsetof(VECTOR_SHAPE_DELETE_HISTORY, layer_name)+layer_name_length],
		delete_script->script_data, script_length);

	AddHistory(&window->history, tool_name,
		(void*)history_data, offsetof(VECTOR_SHAPE_DELETE_HISTORY, layer_name) + layer_name_length + 4,
			DeleteVectorScriptUndo, DeleteVectorScriptRedo
	);
}

void DivideLinesUndo(DRAW_WINDOW* window, void* p)
{
	DIVIDE_LINE_DATA data;
	LAYER *target = window->layer;
	VECTOR_LAYER *layer;
	VECTOR_LINE *line;
	uint8 *byte_data = (uint8*)p;
	uint32 update_flag = DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	int line_index = 0;
	int num_delete = 0;
	int num_change = 0;

	(void)memcpy(&data, p, offsetof(DIVIDE_LINE_DATA, layer_name));
	data.layer_name = (char*)(byte_data + offsetof(DIVIDE_LINE_DATA, layer_name));
	data.change = (DIVIDE_LINE_CHANGE_DATA*)&byte_data[
		offsetof(DIVIDE_LINE_DATA, layer_name) + data.layer_name_length];
	data.add = (DIVIDE_LINE_ADD_DATA*)&byte_data[
		offsetof(DIVIDE_LINE_DATA, layer_name) + data.layer_name_length + data.change_size];

	while(target != NULL)
	{
		if(target == window->active_layer)
		{
			update_flag = DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		}

		if(strcmp(target->name, data.layer_name) == 0)
		{
			layer = target->layer_data.vector_layer_p;
			break;
		}
		target = target->next;
	}

	line = ((VECTOR_BASE_DATA*)layer->base)->next;
	while(line != NULL && num_delete < data.num_add)
	{
		while(data.add->index == line_index)
		{
			VECTOR_LINE *next_line = (VECTOR_LINE*)line->base_data.next;
			uint8 *byte_p = (uint8*)data.add;
			DeleteVectorLine(&line);
			data.add = (DIVIDE_LINE_ADD_DATA*)&byte_p[data.add->data_size];
			num_delete++;
			line = next_line;
		}

		if(line != NULL)
		{
			line = (VECTOR_LINE*)line->base_data.next;
		}
		line_index++;
	}

	line_index = 0;
	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;
	while(line != NULL && num_change < data.num_change)
	{
		if(data.change->index == line_index)
		{
			uint8 *byte_p = (uint8*)data.change;
			line->flags |= VECTOR_LINE_FIX;
			line->base_data.vector_type = data.change->line_type;
			line->num_points = data.change->before_num_points;
			byte_p = &byte_p[offsetof(DIVIDE_LINE_CHANGE_DATA, before)];
			(void)memcpy(line->points, byte_p, sizeof(*line->points)*data.change->before_num_points);
			byte_p = (uint8*)data.change;
			data.change = (DIVIDE_LINE_CHANGE_DATA*)&byte_p[data.change->data_size];
			num_change++;
		}

		line = (VECTOR_LINE*)line->base_data.next;
		line_index++;
	}

	layer->flags |= VECTOR_LAYER_RASTERIZE_CHECKED;
	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
	RasterizeVectorLayer(window, target, layer);

	window->flags |= update_flag;
}

void DivideLinesRedo(DRAW_WINDOW* window, void* p)
{
	DIVIDE_LINE_DATA data;
	LAYER *target = window->layer;
	VECTOR_LAYER *layer;
	VECTOR_LINE *line;
	uint8 *byte_data = (uint8*)p;
	uint32 update_flag = DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	int line_index = 0;
	int num_add = 0;
	int num_change = 0;

	(void)memcpy(&data, p, offsetof(DIVIDE_LINE_DATA, layer_name));
	data.layer_name = (char*)&byte_data[offsetof(DIVIDE_LINE_DATA, layer_name)];
	data.change = (DIVIDE_LINE_CHANGE_DATA*)&byte_data[
		offsetof(DIVIDE_LINE_DATA, layer_name) + data.layer_name_length];
	data.add = (DIVIDE_LINE_ADD_DATA*)&byte_data[
		offsetof(DIVIDE_LINE_DATA, layer_name) + data.layer_name_length + data.change_size];

	while(target != NULL)
	{
		if(target == window->active_layer)
		{
			update_flag = DRAW_WINDOW_UPDATE_ACTIVE_OVER;
		}

		if(strcmp(target->name, data.layer_name) == 0)
		{
			layer = target->layer_data.vector_layer_p;
			break;
		}
		target = target->next;
	}

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;
	while(line != NULL && num_change < data.num_change)
	{
		while(data.add->index == line_index)
		{
			uint8 *byte_p = (uint8*)data.change;
			line->flags |= VECTOR_LINE_FIX;
			line->num_points = data.change->after_num_points;
			byte_p = &byte_p[offsetof(DIVIDE_LINE_CHANGE_DATA, before)];
			byte_p = &byte_p[sizeof(*line->points)*data.change->before_num_points];
			(void)memcpy(line->points, byte_p, sizeof(*line->points)*data.change->after_num_points);
			byte_p = (uint8*)data.change;
			data.change = (DIVIDE_LINE_CHANGE_DATA*)&byte_p[data.change->data_size];
			num_change++;
		}

		line = (VECTOR_LINE*)line->base_data.next;
		line_index++;
	}

	line_index = 0;
	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->base)->next;
	while(line != NULL && num_add < data.num_add)
	{
		while(data.change->index == line_index)
		{
			uint8 *byte_p = (uint8*)data.add;
			line = CreateVectorLine(line, (VECTOR_LINE*)line->base_data.next);
			line->flags = VECTOR_LINE_FIX;
			line->base_data.vector_type = data.add->line_type;
			line->num_points = data.add->num_points;
			
			line->buff_size =
				((line->num_points/VECTOR_LINE_BUFFER_SIZE)+1) * VECTOR_LINE_BUFFER_SIZE;
			line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
				sizeof(*line->points)*line->buff_size);

			byte_p = &byte_p[offsetof(DIVIDE_LINE_ADD_DATA, after)];
			(void)memcpy(line->points, byte_p, sizeof(*line->points)*data.add->num_points);
			byte_p = (uint8*)data.add;
			data.add = (DIVIDE_LINE_ADD_DATA*)&byte_p[data.add->data_size];
			num_add++;
		}

		line = (VECTOR_LINE*)line->base_data.next;
		line_index++;
	}

	layer->flags |= VECTOR_LAYER_RASTERIZE_CHECKED;
	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
	RasterizeVectorLayer(window, target, layer);

	window->flags |= update_flag;
}

typedef struct _DELETE_LINES_HISTORY_DATA
{
	uint16 layer_name_length;
	uint32 num_lines;
	char *layer_name;
	VECTOR_LINE *lines;
	VECTOR_POINT **points;
	uint32 *indexes;
} DELETE_LINES_HISTORY_DATA;

static void DeleteLinesUndo(DRAW_WINDOW* window, void* p)
{
	DELETE_LINES_HISTORY_DATA data;
	uint8 *byte_data = (uint8*)p;
	LAYER *layer = window->layer;
	VECTOR_LINE *line;
	VECTOR_LINE *ressurect_line;
	uint32 exec_lines = 0;
	uint32 i, j;

	(void)memcpy(&data.layer_name_length, byte_data,
		sizeof(data.layer_name_length));
	byte_data += sizeof(data.layer_name_length);
	(void)memcpy(&data.num_lines, byte_data,
		sizeof(data.num_lines));
	byte_data += sizeof(data.num_lines);
	data.layer_name = (char*)byte_data;
	byte_data += data.layer_name_length;
	data.lines = (VECTOR_LINE*)byte_data;
	byte_data += sizeof(*data.lines)*data.num_lines;
	data.points = (VECTOR_POINT**)MEM_ALLOC_FUNC(
		sizeof(*data.points)*data.num_lines);
	for(i=0; i<data.num_lines; i++)
	{
		data.points[i] = (VECTOR_POINT*)byte_data;
		byte_data += sizeof(**data.points) * data.lines[i].num_points;
	}
	data.indexes = (uint32*)byte_data;

	while(strcmp(layer->name, data.layer_name) != 0)
	{
		layer = layer->next;
	}

	line = (VECTOR_LINE*)layer->layer_data.vector_layer_p->base;
	i = 0;
	while(line != NULL)
	{
		if(i == data.indexes[exec_lines])
		{
			ressurect_line = CreateVectorLine(line, (VECTOR_LINE*)line->base_data.next);
			if((void*)line == layer->layer_data.vector_layer_p->top_data)
			{
				layer->layer_data.vector_layer_p->top_data = (void*)line;
			}
			ressurect_line->base_data.vector_type = data.lines[exec_lines].base_data.vector_type;
			ressurect_line->flags |= VECTOR_LINE_FIX;
			ressurect_line->num_points = data.lines[exec_lines].num_points;
			ressurect_line->buff_size = data.lines[exec_lines].buff_size;
			ressurect_line->blur = data.lines[exec_lines].blur;
			ressurect_line->outline_hardness = data.lines[exec_lines].outline_hardness;
			ressurect_line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
				sizeof(*ressurect_line->points)*ressurect_line->buff_size);
			(void)memset(ressurect_line->points, 0,
				sizeof(*ressurect_line->points)*ressurect_line->buff_size);
			for(j=0; j<ressurect_line->num_points; j++)
			{
				ressurect_line->points[j] = data.points[exec_lines][j];
			}

			exec_lines++;
			if(exec_lines >= data.num_lines)
			{
				break;
			}
		}

		line = (VECTOR_LINE*)line->base_data.next;
		i++;
	}

	layer->layer_data.vector_layer_p->flags |=
		VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_CHECKED;
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

	MEM_FREE_FUNC(data.points);
}

static void DeleteLinesRedo(DRAW_WINDOW* window, void* p)
{
	DELETE_LINES_HISTORY_DATA data;
	uint8 *byte_data = (uint8*)p;
	LAYER *layer = window->layer;
	VECTOR_LINE *line;
	uint32 exec_lines = 0;
	uint32 i;

	(void)memcpy(&data.layer_name_length, byte_data,
		sizeof(data.layer_name_length));
	byte_data += sizeof(data.layer_name_length);
	(void)memcpy(&data.num_lines, byte_data,
		sizeof(data.num_lines));
	byte_data += sizeof(data.num_lines);
	data.layer_name = (char*)byte_data;
	byte_data += data.layer_name_length;
	data.lines = (VECTOR_LINE*)byte_data;
	byte_data += sizeof(*data.lines)*data.num_lines;
	for(i=0; i<data.num_lines; i++)
	{
		byte_data += sizeof(**data.points) * data.lines[i].num_points;
	}
	data.indexes = (uint32*)byte_data;

	while(strcmp(layer->name, data.layer_name) != 0)
	{
		layer = layer->next;
	}

	line = (VECTOR_LINE*)((VECTOR_BASE_DATA*)layer->layer_data.vector_layer_p->base)->next;
	i = 0;
	while(line != NULL)
	{
		if(i == data.indexes[exec_lines])
		{
			VECTOR_LINE *prev_line = (VECTOR_LINE*)line->base_data.prev;
			DeleteVectorLine(&line);
			line = prev_line;

			exec_lines++;
			if(exec_lines >= data.num_lines)
			{
				break;
			}
		}

		line = (VECTOR_LINE*)line->base_data.next;
		i++;
	}

	layer->layer_data.vector_layer_p->flags |=
		VECTOR_LAYER_RASTERIZE_ACTIVE;
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
}

/***********************************************************
* AddDeleteLinesHistory関数                                *
* 複数の線を同時に削除した際の履歴データを追加             *
* 引数                                                     *
* window		: 描画領域の情報を管理する構造体のアドレス *
* active_layer	: アクティブなレイヤー                     *
* line_data		: 削除する線のデータ                       *
* line_indexes	: 削除する線の一番下から数えたインデックス *
* num_lines		: 削除する線の数                           *
* tool_name		: 削除に使用したツールの名前               *
***********************************************************/
void AddDeleteLinesHistory(
	DRAW_WINDOW* window,
	LAYER* active_layer,
	VECTOR_LINE* line_data,
	uint32* line_indexes,
	uint32 num_lines,
	const char* tool_name
)
{
	MEMORY_STREAM_PTR stream = CreateMemoryStream(8096);
	uint16 name_length = (uint16)strlen(active_layer->name) + 1;
	uint32 i;

	(void)MemWrite(&name_length, sizeof(name_length), 1, stream);
	(void)MemWrite(&num_lines, sizeof(num_lines),1, stream);
	(void)MemWrite(active_layer->name, 1, name_length, stream);
	(void)MemWrite(line_data, sizeof(*line_data), num_lines, stream);

	for(i=0; i<num_lines; i++)
	{
		(void)MemWrite(line_data[i].points, sizeof(*line_data[0].points),
			line_data[i].num_points, stream);
	}

	(void)MemWrite(line_indexes, sizeof(*line_indexes), num_lines, stream);
	AddHistory(&window->history, tool_name, stream->buff_ptr,
		stream->data_point, DeleteLinesUndo, DeleteLinesRedo);

	DeleteMemoryStream(stream);
}

#ifdef __cplusplus
}
#endif
