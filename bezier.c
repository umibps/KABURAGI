#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include "types.h"
#include "vector.h"
#include "draw_window.h"
#include "bezier.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SQRT sqrt
#define LOG log

/******************************************************
* MakeBezier3EdgeControlPoint関数                     *
* 3点の座標から3次ベジェ曲線の端点での座標を決定する  *
* 引数                                                *
* src		: ベジェ曲線が通る座標                    *
* control	: 2次ベジェ曲線の制御点を記憶するアドレス *
******************************************************/
void MakeBezier3EdgeControlPoint(BEZIER_POINT* src, BEZIER_POINT* control)
{
#define SMOOTH_VALUE 2
	FLOAT_T length_negative =
		SQRT((src[0].x-src[1].x)*(src[0].x-src[1].x)+(src[0].y-src[1].y)*(src[0].y-src[1].y));
	FLOAT_T length_positive =
		SQRT((src[1].x-src[2].x)*(src[1].x-src[2].x)+(src[1].y-src[2].y)*(src[1].y-src[2].y));
	if(length_negative <= 0 || length_positive <= 0)
	{
		control[0] = control[1] = src[1];
	}
	else
	{
		BEZIER_POINT m_negative = {(src[0].x+src[1].x)*0.5, (src[0].y+src[1].y)*0.5};
		BEZIER_POINT m_positive = {(src[1].x+src[2].x)*0.5, (src[1].y+src[2].y)*0.5};
		FLOAT_T rev_length = 1 / (length_negative + length_positive);
		FLOAT_T d_negative = length_negative * rev_length;
		FLOAT_T d_positive = length_positive * rev_length;
		BEZIER_POINT b =
			{d_negative*m_negative.x+d_positive*m_positive.x,
				d_negative*m_negative.y+d_positive*m_positive.y};

		m_negative.x = b.x - (b.x - m_negative.x)*d_negative*SMOOTH_VALUE;
		m_negative.y = b.y - (b.y - m_negative.y)*d_negative*SMOOTH_VALUE;
		m_positive.x = b.x + (m_positive.x - b.x)*d_positive*SMOOTH_VALUE;
		m_positive.y = b.y + (m_positive.y - b.y)*d_positive*SMOOTH_VALUE;

		b.x = src[1].x - b.x, b.y = src[1].y - b.y;

		control[0].x = m_negative.x + b.x, control[0].y = m_negative.y + b.y;
		control[1].x = m_positive.x + b.x, control[1].y = m_positive.y + b.y;
	}
#undef SMOOTH_VALUE
}

/*************************************************
* CalcBezier2関数                                *
* 2次ベジェ曲線の媒介変数tにおける座標を計算する *
* 引数                                           *
* points	: ベジェ曲線の制御点                 *
* t			: 媒介変数t                          *
* dest		: 座標データを記憶するアドレス       *
*************************************************/
void CalcBezier2(BEZIER_POINT* points, FLOAT_T t, BEZIER_POINT* dest)
{
	FLOAT_T t2 = t*t, mt = (1.0-t), mt2;
	mt2 = mt*mt;

	dest->x = points[0].x*mt2 + t*mt*2.0*points[1].x + points[2].x*t2;
	dest->y = points[0].y*mt2 + t*mt*2.0*points[1].y + points[2].y*t2;
}

/********************************
* CalcBezier2Length関数         *
* 2次ベジェ曲線の長さを計算する *
* 引数                          *
* points	: 制御点座標        *
* start		: 媒介変数tの開始値 *
* end		: 媒介変数tの終了値 *
* 返り値                        *
*	2次ベジェ曲線の長さ         *
********************************/
FLOAT_T CalcBezier2Length(BEZIER_POINT* points, FLOAT_T start, FLOAT_T end)
{
	// インターネットで調べた情報を元に実装。上手く動いていない?

	FLOAT_T xa = 2.0*(points[0].x-2.0*points[1].x+points[2].x),
		xb = -2.0*points[0].x+2.0*points[1].x;
	FLOAT_T ya = 2.0*(points[0].y-2.0*points[1].y+points[2].y),
		yb = -2.0*points[0].y+2.0*points[1].y;
	FLOAT_T a = xa*xa+ya*ya, b = 2.0*(xa*xb+ya*yb), c = xb*xb+yb*yb;
	FLOAT_T d = b*b-4.0*a*c;

	if(d != 0.0)
	{
		FLOAT_T s1 = 2.0 * (SQRT(a*a*start*start+b*start+c)+a*start) + b;
		FLOAT_T s2 = 2.0 * (SQRT(a*a*end*end+b*end+c)+a*end) + b;
		FLOAT_T s1_2 = s1*s1;
		FLOAT_T s2_2 = s2*s2;
		return (s2_2-s1_2-d*(4.0*log(fabs(s2/s1))+d*(1.0/(s2_2)-1.0/(s1_2))))/(32.0*a*SQRT(a));
	}
	else if(a != 0.0)
	{
		FLOAT_T s1 = 2.0*fabs(2.0*a*start+b)*(2*a*start+b);
		FLOAT_T s2 = 2.0*fabs(2.0*a*end+b)*(2*a*end+b);
		return (s2 - s1)/(8.0*a*SQRT(a));
	}

	return SQRT(c)/(end-start);
}

void StrokeBezier2Line(
	DRAW_WINDOW* window,
	VECTOR_LINE* line,
	VECTOR_POINT* vec_points,
	BEZIER_POINT* points,
	FLOAT_T start,
	FLOAT_T end,
	VECTOR_LAYER_RECTANGLE *rect
)
{
	BEZIER_POINT draw;
	cairo_pattern_t* brush;
	gdouble r, oct_r, d, div_d, t = start, dist = 0, div_end = 1/(end-start);
	gdouble ad, bd;
	gdouble red, green, blue, alpha;
	int32 min_x, min_y, max_x, max_y;
	int x, y;
	int index;

	d = SQRT((vec_points[0].x-vec_points[1].x)*(vec_points[0].x-vec_points[1].x)
		+ (vec_points[0].y-vec_points[1].y)*(vec_points[0].y-vec_points[1].y));
	if(end - start > 0.75)
	{
		d += SQRT((vec_points[1].x-vec_points[2].x)*(vec_points[1].x-vec_points[2].x)
			+ (vec_points[1].y-vec_points[2].y)*(vec_points[1].y-vec_points[2].y));
	}
	d *= 1.5;

	div_d = 1.0 / d;

	do
	{
		index = (int)(end - start - 0.5 + t);
		ad = (d - dist*div_end) * div_d, bd = dist*div_end * div_d;
		r = vec_points[index].size * vec_points[index].pressure * 0.01f * ad
			+ vec_points[index+1].size * vec_points[index+1].pressure * 0.01f * bd;
		if(r < 0.5)
		{
			r = 0.5;
		}
		oct_r = r * 0.125;
		if(oct_r < 0.5)
		{
			oct_r = 0.5;
		}

		CalcBezier2(points, t, &draw);
		dist += oct_r;
		t = dist / d + start;

		min_x = (int32)(draw.x - r - 1);
		min_y = (int32)(draw.y - r - 1);
		max_x = (int32)(draw.x + r + 1);
		max_y = (int32)(draw.y + r + 1);

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
		red = (vec_points[index].color[0] * ad) + (vec_points[index+1].color[0] * bd);
		green = (vec_points[index].color[1] * ad) + (vec_points[index+1].color[1] * bd);
		blue = (vec_points[index].color[2] * ad) + (vec_points[index+1].color[2] * bd);
		alpha = (vec_points[index].color[3] * ad) + (vec_points[index+1].color[3] * bd);

		brush = cairo_pattern_create_radial(draw.x, draw.y, 0.0, draw.x, draw.y, r);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
		cairo_pattern_add_color_stop_rgba(brush, 0.0, red, green, blue, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0-line->blur*DIV_PIXEL, red, green, blue, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0, red, green, blue,
			line->outline_hardness*0.01*alpha);

		cairo_set_source(window->temp_layer->cairo_p, brush);

		cairo_arc(window->temp_layer->cairo_p, draw.x, draw.y, r, 0, 2*G_PI);
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
	} while(t < end);
}

void MakeBezier3ControlPoints(BEZIER_POINT* src, BEZIER_POINT* controls)
{
#define SMOOTH_VALUE 0.75

	FLOAT_T xc1 = (src[0].x + src[1].x) * 0.5;
	FLOAT_T yc1 = (src[0].y + src[1].y) * 0.5;
	FLOAT_T xc2 = (src[1].x + src[2].x) * 0.5;
	FLOAT_T yc2 = (src[1].y + src[2].y) * 0.5;
	FLOAT_T xc3 = (src[2].x + src[3].x) * 0.5;
	FLOAT_T yc3 = (src[2].y + src[3].y) * 0.5;

	FLOAT_T len1 = SQRT((src[1].x-src[0].x) * (src[1].x-src[0].x)
					+ (src[1].y-src[0].y) * (src[1].y-src[0].y));
	FLOAT_T len2 = SQRT((src[2].x-src[1].x) * (src[2].x-src[1].x)
					+ (src[2].y-src[1].y) * (src[2].y-src[1].y));
	FLOAT_T len3 = SQRT((src[3].x-src[2].x) * (src[3].x-src[2].x)
					+ (src[3].y-src[2].y) * (src[3].y-src[2].y));

	FLOAT_T k1 = len1 / (len1 + len2), k2 = len2 / (len2 + len3);

	FLOAT_T xm1 = xc1 + (xc2 - xc1) * k1,
			ym1 = yc1 + (yc2 - yc1) * k1;

	FLOAT_T xm2 = xc2 + (xc3 - xc2) * k2,
			ym2 = yc2 + (yc3 - yc2) * k2;

	controls[0].x = xm1 + (xc2 - xm1) * SMOOTH_VALUE + src[1].x - xm1;
	controls[0].y = ym1 + (yc2 - ym1) * SMOOTH_VALUE + src[1].y - ym1;

	controls[1].x = xm2 + (xc2 - xm2) * SMOOTH_VALUE + src[2].x - xm2;
	controls[1].y = ym2 + (yc2 - ym2) * SMOOTH_VALUE + src[2].y - ym2;
#undef SMOOTH_VALUE
}

void CalcBezier3(BEZIER_POINT* points, FLOAT_T t, BEZIER_POINT* dest)
{
	FLOAT_T t2 = t*t, mt = 1.0-t, mt2;
	mt2 = mt*mt;

	dest->x = points[0].x*mt2*mt + 3.0*points[1].x*t*mt2 + 3.0*points[2].x*t2*mt + points[3].x*t2*t;
	dest->y = points[0].y*mt2*mt + 3.0*points[1].y*t*mt2 + 3.0*points[2].y*t2*mt + points[3].y*t2*t;
}

FLOAT_T CalcBezier3Length(BEZIER_POINT* points, FLOAT_T start, FLOAT_T end)
{
#define DIVISION 32
	FLOAT_T term1, term2, term3, term4, termC;
	FLOAT_T h = (end - start) / (2.0 * (FLOAT_T)DIVISION);
	FLOAT_T result = 0.0;
	FLOAT_T f[3];
	FLOAT_T t = start, t2, t3;
	FLOAT_T ft;
	int i;

	{
		FLOAT_T a1, a2, b1, b2, c1, c2;
		a1 = - points[0].x + 3.0*points[1].x - 3.0*points[2].x + points[3].x;
		a2 = - points[0].y + 3.0*points[1].y - 3.0*points[2].y + points[3].y;
		b1 = 3.0 * points[0].x - 6.0 * points[1].x + 3.0 * points[2].x;
		b2 = 3.0 * points[0].y - 6.0 * points[1].y + 3.0 * points[2].y;
		c1 = -3.0 * points[0].x + 3.0 * points[1].x;
		c2 = -3.0 * points[0].y + 3.0 * points[1].y;

		term1 = 4.0 * (b1*c1 + b2*b2);
		term2 = (6.0 * (a1*c1 + a2*c2) + 4.0 * (b1*b1 + b2*b2));
		term3 = 12.0 * (a1*b1 + a2*b2);
		term4 = 9.0 * (a1*a1 + a2*a2);
		termC = c1*c1 + c2*c2;
	}

	for(i=0; i<3; i++, t += h)
	{
		t2 = t*t;
		t3 = t2*t;
		ft = term4*t*t3 + term3*t3 + term2*t2 + term1*t + termC;
		if(ft < 0.0)
		{
			f[i] = 0.0;
		}
		else
		{
			f[i] = SQRT(ft);
		}
	}

	for(i=0; i<DIVISION; i++)
	{
		result += f[0] + f[1] + f[2];
		f[0] = f[2];
		t2 = t*t;
		t3 = t2*t;
		ft = term4*t*t3 + term3*t3 + term2*t2 + term1*t + termC;
		if(ft < 0.0)
		{
			f[1] = 0.0;
		}
		else
		{
			f[1] = SQRT(ft);
		}
		t += h;
		t2 = t*t;
		t3 = t2*t;
		ft = term4*t*t3 + term3*t3 + term2*t2 + term1*t + termC;
		if(ft < 0.0)
		{
			f[2] = 0.0;
		}
		else
		{
			f[2] = SQRT(ft);
		}
		t += h;
	}

	return (result * (h / 3.0));
}

void StrokeBezier3Line(
	DRAW_WINDOW* window,
	VECTOR_LINE* line,
	VECTOR_POINT* vec_points,
	BEZIER_POINT* points,
	VECTOR_LAYER_RECTANGLE* rect
)
{
	BEZIER_POINT draw;
	cairo_pattern_t* brush;
	gdouble r, oct_r, d, div_d, t = 0, dist = 0;
	gdouble red, green, blue, alpha;
	gdouble ad, bd;
	int32 min_x, min_y, max_x, max_y;
	int x, y;

	// d = CalcBezier3Length(points, 0, 1);
	d = SQRT((points[0].x-points[1].x)*(points[0].x-points[1].x)
		+ (points[0].y-points[1].y)*(points[0].y-points[1].y)) * 1.5;
	d += SQRT((points[1].x-points[2].x)*(points[1].x-points[2].x)
		+ (points[1].y-points[2].y)*(points[1].y-points[2].y)) * 1.5;
	d += SQRT((points[2].x-points[3].x)*(points[2].x-points[3].x)
		+ (points[2].y-points[3].y)*(points[2].y-points[3].y)) * 1.5;

	div_d = 1.0 / d;

	do
	{
		ad = (d - dist) * div_d, bd = dist * div_d;
		r = vec_points[0].size * vec_points[0].pressure * 0.01 * ad
			+ vec_points[1].size * vec_points[1].pressure * 0.01 * bd;
		if(r < 0.5)
		{
			r = 0.5;
		}
		oct_r = r * 0.125;
		if(oct_r < 0.5)
		{
			oct_r = 0.5;
		}

		CalcBezier3(points, t, &draw);
		dist += oct_r;
		t = dist / d;

		min_x = (int32)(draw.x - r - 1);
		min_y = (int32)(draw.y - r - 1);
		max_x = (int32)(draw.x + r + 1);
		max_y = (int32)(draw.y + r + 1);

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
		red = (vec_points[0].color[0] * ad) + (vec_points[1].color[0] * bd);
		green = (vec_points[0].color[1] * ad) + (vec_points[1].color[1] * bd);
		blue = (vec_points[0].color[2] * ad) + (vec_points[1].color[2] * bd);
		alpha = (vec_points[0].color[3] * ad) + (vec_points[1].color[3] * bd);

		brush = cairo_pattern_create_radial(draw.x, draw.y, 0.0, draw.x, draw.y, r);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
		cairo_pattern_add_color_stop_rgba(brush, 0.0, red, green, blue, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0-line->blur*0.01, red, green, blue, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0, red, green, blue, line->outline_hardness*0.01f*alpha);
		cairo_set_source(window->temp_layer->cairo_p, brush);

		cairo_arc(window->temp_layer->cairo_p, draw.x, draw.y, r, 0, 2*G_PI);
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
	} while(t < 1.0);
}

void StrokeBezierLine(
	DRAW_WINDOW* window,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
)
{
	BEZIER_POINT calc[4], inter[2];
	int i, j;

	rect->min_x = rect->max_x = line->points->x;
	rect->min_y = rect->min_y = line->points->y;

	for(i=0; i<3; i++)
	{
		calc[i].x = line->points[i].x;
		calc[i].y = line->points[i].y;
	}
	MakeBezier3EdgeControlPoint(calc, inter);
	calc[1].x = line->points[0].x, calc[1].y = line->points[0].y;
	calc[2] = inter[0];
	calc[3].x = line->points[1].x, calc[3].y = line->points[1].y;
	StrokeBezier3Line(window, line, line->points, calc, rect);

	for(i=0; i<line->num_points-3; i++)
	{
		for(j=0; j<4; j++)
		{
			calc[j].x = line->points[i+j].x;
			calc[j].y = line->points[i+j].y;
		}
		MakeBezier3ControlPoints(calc, inter);
		calc[0].x = line->points[i+1].x;
		calc[0].y = line->points[i+1].y;
		calc[1] = inter[0], calc[2] = inter[1];
		calc[3].x = line->points[i+2].x;
		calc[3].y = line->points[i+2].y;

		StrokeBezier3Line(window, line, &line->points[i+1], calc, rect);
	}

	for(j=0; j<3; j++)
	{
		calc[j].x = line->points[i+j].x;
		calc[j].y = line->points[i+j].y;
	}
	MakeBezier3EdgeControlPoint(calc, inter);

	calc[0].x = line->points[i+1].x, calc[0].y = line->points[i+1].y;
	calc[1] = inter[1];
	calc[2].x = line->points[i+2].x, calc[2].y = line->points[i+2].y;
	calc[3].x = line->points[i+2].x, calc[3].y = line->points[i+2].y;
	StrokeBezier3Line(window, line, &line->points[i+1], calc, rect);
}

void StrokeBezierLineClose(
	DRAW_WINDOW* window,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
)
{
	BEZIER_POINT calc[4], inter[2];
	int i, j;

	rect->min_x = rect->max_x = line->points->x;
	rect->min_y = rect->min_y = line->points->y;

	calc[0].x = line->points[line->num_points-1].x;
	calc[0].y = line->points[line->num_points-1].y;
	for(i=0; i<3; i++)
	{
		calc[i+1].x = line->points[i].x;
		calc[i+1].y = line->points[i].y;
	}
	MakeBezier3ControlPoints(calc, inter);
	calc[0].x = line->points[0].x, calc[0].y = line->points[0].y;
	calc[1] = inter[0], calc[2] = inter[1];
	calc[3].x = line->points[1].x, calc[3].y = line->points[1].y;
	StrokeBezier3Line(window, line, line->points, calc, rect);

	for(i=0; i<line->num_points-3; i++)
	{
		for(j=0; j<4; j++)
		{
			calc[j].x = line->points[i+j].x;
			calc[j].y = line->points[i+j].y;
		}
		MakeBezier3ControlPoints(calc, inter);
		calc[0].x = line->points[i+1].x;
		calc[0].y = line->points[i+1].y;
		calc[3].x = line->points[i+2].x;
		calc[3].y = line->points[i+2].y;
		calc[1] = inter[0], calc[2] = inter[1];

		StrokeBezier3Line(window, line, &line->points[i+1], calc, rect);
	}

	for(j=0; j<3; j++)
	{
		calc[j].x = line->points[i+j].x;
		calc[j].y = line->points[i+j].y;
	}
	calc[3].x = line->points[0].x, calc[3].y = line->points[0].y;
	MakeBezier3ControlPoints(calc, inter);
	calc[0].x = line->points[i+1].x, calc[0].y = line->points[i+1].y;
	calc[1] = inter[0], calc[2] = inter[1];
	calc[3].x = line->points[i+2].x, calc[3].y = line->points[i+2].y;
	StrokeBezier3Line(window, line, &line->points[i+1], calc, rect);

	calc[0].x = line->points[i+1].x, calc[0].y = line->points[i+1].y;
	calc[1].x = line->points[i+2].x, calc[1].y = line->points[i+2].y;
	calc[2].x = line->points[0].x, calc[2].y = line->points[0].y;
	calc[3].x = line->points[1].x, calc[3].y = line->points[1].y;
	MakeBezier3ControlPoints(calc, inter);
	calc[0].x = line->points[i+2].x, calc[0].y = line->points[i+2].y;
	calc[1] = inter[0], calc[2] = inter[1];
	calc[3].x = line->points[0].x, calc[3].y = line->points[0].y;
	StrokeBezier3Line(window, line, &line->points[i+1], calc, rect);
}

#ifdef __cplusplus
}
#endif
