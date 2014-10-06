#include <string.h>
#include <math.h>
#include <float.h>
#include "application.h"
#include "color.h"
#include "fractal_point.h"
#include "fractal_color_map.h"
#include "fractal.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
# define IS_NAN(X) _isnan((X))
# define IS_INF(X) (!(_finite((X))))
#else
# define IS_NAN(X) isnan((X))
# define IS_INF(X) isinf((X))
#endif

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#define ROUND(X) (int)((X) + 0.5)
#define ROUND6(X) ((FLOAT_T)(((int)((X) * 1000000)) * 0.000001))
#define TRUNC(X) (int)((X))

#define PROP_TABLE_SIZE 1024

void InitializeFractalPointXForm(FRACTAL_POINT_XFORM* form)
{
	(void)memset(form, 0, sizeof(*form));

	form->vars[0] = 1;
	form->c[0][0] = 1;
	form->c[1][1] = 1;
}

void InitializeFractalPoint(FRACTAL_POINT* point, int width, int height)
{
	int i;

	(void)memset(point, 0, sizeof(*point));

	for(i=0; i<FRACTAL_NUM_XFORMS; i++)
	{
		InitializeFractalPointXForm(&point->xform[i]);
	}

	point->pulse[0][1] = 60;
	point->pulse[1][1] = 60;

	point->wiggle[0][1] = 60;
	point->wiggle[1][1] = 60;

	point->pixels_per_unit = 50;

	point->width = width;
	point->height = height;

	point->spatial_oversample = 1;
	point->spatial_filter_radius = 0.5;

	point->gamma = 1;
	point->vibrancy = 1;
	point->contrast = 1;
	point->brightness = 1;

	point->sample_density = 50;
	point->num_batches = 1;

	point->white_level = 200;

	point->var_flags = 262143;
}

void ReleaseFractalPoint(FRACTAL_POINT* point)
{
	MEM_FREE_FUNC(point->name);
	MEM_FREE_FUNC(point->nick);
	MEM_FREE_FUNC(point->url);

	MEM_FREE_FUNC(point->prop_table);
}

void ResizeFractalPoint(FRACTAL_POINT* point, int width, int height)
{
	if(width >= height)
	{
		point->pixels_per_unit = point->pixels_per_unit / (point->width / (FLOAT_T)width);
	}
	else
	{
		point->pixels_per_unit = point->pixels_per_unit / (point->height / (FLOAT_T)height);
	}
	point->width = width;
	point->height = height;
}

void CopyFractalPoint(FRACTAL_POINT* dst, const FRACTAL_POINT* src)
{
	ReleaseFractalPoint(dst);
	*dst = *src;
	dst->prop_table =(int*)MEM_ALLOC_FUNC(
		sizeof(*dst->prop_table) * PROP_TABLE_SIZE);
	(void)memcpy(dst->prop_table, src->prop_table, sizeof(*dst->prop_table) * PROP_TABLE_SIZE);
}

void FractalPointPreparePropTable(FRACTAL_POINT* point)
{
	FLOAT_T prop_sum;
	FLOAT_T loop_value = 0;
	FLOAT_T tot_value = 0;
	int i, j;

	if(point->prop_table == NULL)
	{
		point->prop_table = (int*)MEM_ALLOC_FUNC(
			sizeof(*point->prop_table) * PROP_TABLE_SIZE);
	}

	for(i=0; i<FRACTAL_NUM_XFORMS; i++)
	{
		tot_value += point->xform[i].density;
	}

	for(i=0; i<PROP_TABLE_SIZE; i++)
	{
		prop_sum = 0;
		j = -1;
		do
		{
			j++;
			prop_sum += point->xform[j].density;
		} while(prop_sum <= loop_value && j < FRACTAL_NUM_XFORMS);
		point->prop_table[i] = j;
		loop_value += tot_value / 1024;
	}
}

void FractalPointIterate(
	FRACTAL_POINT* point,
	int num_r_points,
	FLOAT_T (*points)[3]
)
{
	FRACTAL_POINT_XFORM *xform;
	FLOAT_T sin_value, cos_value;
	FLOAT_T px, py, pc;
	FLOAT_T dx, dy;
	FLOAT_T tx, ty;
	FLOAT_T nx, ny;
	FLOAT_T r;
	FLOAT_T s, v, a;
	FLOAT_T n0, n1;
	FLOAT_T m0, m1;
	int i;

	px = 2.0 * FRACTAL_FLOAT_RAND() - 1;
	py = 2.0 * FRACTAL_FLOAT_RAND() - 1;
	pc = FRACTAL_FLOAT_RAND();

	FractalPointPreparePropTable(point);

	for(i=-100; i<num_r_points; i++)
	{
		xform = &point->xform[point->prop_table[FRACTAL_RAND() % 1024]];
		s = point->symmetry;
		pc = (pc + xform->color) * 0.5 * (1 - s) + s * pc;

		tx = xform->c[0][0] * px + xform->c[1][0] * py + xform->c[2][0];
		ty = xform->c[0][1] * px + xform->c[1][1] * py + xform->c[2][1];

		px = 0,	py = 0;

		if(xform->vars[FRACTAL_VALIATION_LINEAR] > 0)
		{
			px += xform->vars[FRACTAL_VALIATION_LINEAR] * tx;
			py += xform->vars[FRACTAL_VALIATION_LINEAR] * ty;
		}

		if(xform->vars[FRACTAL_VALIATION_SINUSOIDAL] > 0)
		{
			px += xform->vars[FRACTAL_VALIATION_SINUSOIDAL] * sin(tx);
			py += xform->vars[FRACTAL_VALIATION_SINUSOIDAL] * sin(ty);
		}

		if(xform->vars[FRACTAL_VALIATION_SPHERICAL] > 0)
		{
			r = tx * tx + ty * ty + 1e-6;
			px += xform->vars[FRACTAL_VALIATION_SPHERICAL] * tx / r;
			py += xform->vars[FRACTAL_VALIATION_SPHERICAL] * ty / r;
		}

		if(xform->vars[FRACTAL_VALIATION_SWIRL] > 0)
		{
			r = tx * tx + ty * ty;
			sin_value = sin(r);
			cos_value = cos(r);
			px += xform->vars[FRACTAL_VALIATION_SWIRL] * (sin_value * tx - cos_value * ty);
			py += xform->vars[FRACTAL_VALIATION_SWIRL] * (cos_value * tx + sin_value * ty);
		}

		if(xform->vars[FRACTAL_VALIATION_HORSESHOE] > 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			sin_value = sin(a);
			cos_value = cos(a);
			px += xform->vars[FRACTAL_VALIATION_HORSESHOE] * (sin_value * tx - cos_value * ty);
			py += xform->vars[FRACTAL_VALIATION_HORSESHOE] * (cos_value * tx + sin_value * ty);
		}

		if(xform->vars[FRACTAL_VALIATION_POLAR] > 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(ty, ty) / M_PI;
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty) - 1;
			px += xform->vars[FRACTAL_VALIATION_POLAR] * a;
			py += xform->vars[FRACTAL_VALIATION_POLAR] * r;
		}

		if(xform->vars[FRACTAL_VALIATION_HANDKERCHIEF] > 0)
		{
			nx = ty,	ny = ty;
			if(nx < 0 && nx > -1e100)
			{
				nx *= 2;
			}
			if(ny < 0)
			{
				ny /= 2;
			}
			px += xform->vars[FRACTAL_VALIATION_HANDKERCHIEF] * nx;
			py += xform->vars[FRACTAL_VALIATION_HANDKERCHIEF] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_HEART] > 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);

			px += xform->vars[FRACTAL_VALIATION_HEART] * (sin(a * r) * r);
			py -= xform->vars[FRACTAL_VALIATION_HEART] * (cos(a * r) * r);
		}

		if(xform->vars[FRACTAL_VALIATION_DISC] > 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			px += xform->vars[FRACTAL_VALIATION_DISC] * (sin(r) * a);
			py += xform->vars[FRACTAL_VALIATION_DISC] * (cos(r) * a);
		}

		if(xform->vars[FRACTAL_VALIATION_SPIRAL] > 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = pow(tx * tx + ty * ty, 0.5) + 1e-6;
			px += xform->vars[FRACTAL_VALIATION_SPIRAL] * ((cos(a) + sin(r)) / r);
			py += xform->vars[FRACTAL_VALIATION_SPIRAL] * ((sin(a) - cos(r)) / r);
		}

		if(xform->vars[FRACTAL_VALIATION_HYPERBOLIC] > 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = pow(tx * tx + ty * ty, 0.25) + 1e-6;
			px += xform->vars[FRACTAL_VALIATION_HYPERBOLIC] * (sin(a) / r);
			py -= xform->vars[FRACTAL_VALIATION_HYPERBOLIC] * (cos(a) * r);
		}

		v = xform->vars[FRACTAL_VALIATION_SQUARE];
		if(v > 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			px += v * sin(a) * cos(r);
			py += v * cos(a) * sin(r);
		}

		v = xform->vars[FRACTAL_VALIATION_EX];
		if(v > 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			n0 = sin(a + r);
			n1 = cos(a - r);
			m0 = n0 * n0 * n0 * r;
			m1 = n1 * n1 * n1 * r;
			px += v * (m0 + m1);
			py += v * (m0 - m1);
		}

		if(xform->vars[FRACTAL_VALIATION_JULIA] > 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			px += xform->vars[FRACTAL_VALIATION_JULIA] * (sin(a + r) * r);
			py -= xform->vars[FRACTAL_VALIATION_JULIA] * (cos(a - r) * r);
		}

		if(xform->vars[FRACTAL_VALIATION_BENT] > 0)
		{
			nx = tx,	ny = ty;
			if(nx < 0 && nx > -1e100)
			{
				nx *= 2;
			}
			if(ny < 0)
			{
				ny /= 2;
			}
			px += xform->vars[FRACTAL_VALIATION_BENT] * nx;
			py += xform->vars[FRACTAL_VALIATION_BENT] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_WAVES] != 0)
		{
			dx = xform->c[2][0],	dy = xform->c[2][1];
			nx = tx + xform->c[1][0] * sin(ty / ((dx * dx) + FRACTAL_EPS));
			ny = ty + xform->c[1][1] * sin(tx / ((dy * dy) + FRACTAL_EPS));
			px += xform->vars[FRACTAL_VALIATION_WAVES] * nx;
			py += xform->vars[FRACTAL_VALIATION_WAVES] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_FISHEYE] != 0)
		{
			r = sqrt(tx * tx + ty * ty);
			a = atan2(tx, ty);
			r = 2 * r / (r + 1);
			nx = r * cos(a);
			ny = r * sin(a);
			px += xform->vars[FRACTAL_VALIATION_FISHEYE] * nx;
			py += xform->vars[FRACTAL_VALIATION_FISHEYE] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_POPCORN] != 0)
		{
			nx = tx + xform->c[1][0] * sin(ty * tan(3 * ty) + FRACTAL_EPS);
			ny = ty + xform->c[1][1] * sin(tx * tan(3 * tx) + FRACTAL_EPS);
			px += xform->vars[FRACTAL_VALIATION_POPCORN] * nx;
			py += xform->vars[FRACTAL_VALIATION_POPCORN] * ny;
		}

		if(i >= 0)
		{
			if(px >= px && IS_NAN(px) == 0 && IS_INF(px) == 0)
			{
				points[i][0] = px;
			}
			if(py >= py && IS_NAN(py) == 0 && IS_INF(py) == 0)
			{
				points[i][1] = py;
			}
			
			if(pc >= pc && IS_NAN(pc) == 0 && IS_INF(pc) == 0)
			{
				points[i][2] = pc;
			}
		}
	}
}

void FractalPointIterateFloat(
	FRACTAL_POINT* point,
	int num_r_points,
	FLOAT_T (*points)[3]
)
{
	FRACTAL_POINT_XFORM *xform;
	FLOAT_T px, py, pc;
	FLOAT_T dx, dy;
	FLOAT_T tx, ty;
	FLOAT_T c1, c2;
	FLOAT_T nx, ny;
	FLOAT_T r, r2;
	FLOAT_T s, a;
	FLOAT_T n0, n1;
	FLOAT_T m0, m1;
	int i;

	px = 2.0 * FRACTAL_FLOAT_RAND() - 1;
	py = 2.0 * FRACTAL_FLOAT_RAND() - 1;
	pc = FRACTAL_FLOAT_RAND();

	FractalPointPreparePropTable(point);

	for(i=-100; i<num_r_points; i++)
	{
		xform = &point->xform[point->prop_table[
			FRACTAL_RAND() % 1024]];
		s = point->symmetry;
		pc = (pc + xform->color) * 0.5 * (1 - s) + s * pc;

		tx = xform->c[0][0] * px + xform->c[1][0] * py + xform->c[2][0];
		ty = xform->c[0][1] * px + xform->c[1][1] * py + xform->c[2][1];

		px = 0,	py = 0;

		if(xform->vars[FRACTAL_VALIATION_LINEAR] != 0)
		{
			nx = tx,	ny = ty;
			px += xform->vars[FRACTAL_VALIATION_LINEAR] * nx;
			py += xform->vars[FRACTAL_VALIATION_LINEAR] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_SINUSOIDAL] != 0)
		{
			nx = sin(tx),	ny = sin(ty);
			px += xform->vars[FRACTAL_VALIATION_SINUSOIDAL] * nx;
			py += xform->vars[FRACTAL_VALIATION_SINUSOIDAL] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_SPHERICAL] != 0)
		{
			r2 = tx * tx + ty * ty + 1e-6;
			nx = tx / r2,	ny = ty / r2;
			px += xform->vars[FRACTAL_VALIATION_SPHERICAL] * nx;
			py += xform->vars[FRACTAL_VALIATION_SPHERICAL] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_SWIRL] != 0)
		{
			r2 = tx * tx + ty * ty;
			c1 = sin(r);
			c2 = cos(r);
			nx = c1 * tx - c2 * ty;
			ny = c2 * tx + c1 * ty;
			px += xform->vars[FRACTAL_VALIATION_SWIRL] * nx;
			py += xform->vars[FRACTAL_VALIATION_SWIRL] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_HORSESHOE] != 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			c1 = sin(a);
			c2 = cos(a);
			nx = c1 * tx - c2 * ty;
			ny = c2 * tx + c1 * ty;
			px += xform->vars[FRACTAL_VALIATION_HORSESHOE] * nx;
			py += xform->vars[FRACTAL_VALIATION_HORSESHOE] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_POLAR] != 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				nx = atan2(ty, ty) / M_PI;
			}
			else
			{
				nx = 0;
			}
			ny = sqrt(tx * tx + ty * ty) - 1;
			px += xform->vars[FRACTAL_VALIATION_POLAR] * nx;
			py += xform->vars[FRACTAL_VALIATION_POLAR] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_HANDKERCHIEF] != 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			px += xform->vars[FRACTAL_VALIATION_HANDKERCHIEF] * sin(a + r) * r;
			py += xform->vars[FRACTAL_VALIATION_HANDKERCHIEF] * cos(a - r) * r;
		}

		if(xform->vars[FRACTAL_VALIATION_HEART] != 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			a *= r;
			px += xform->vars[FRACTAL_VALIATION_HEART] * sin(a) * r;
			py += xform->vars[FRACTAL_VALIATION_HEART] * cos(a) * -r;
		}

		if(xform->vars[FRACTAL_VALIATION_DISC] != 0)
		{
			nx = tx * M_PI,	ny = ty * M_PI;
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			px += xform->vars[FRACTAL_VALIATION_DISC] * sin(r) * a / M_PI;
			py += xform->vars[FRACTAL_VALIATION_DISC] * cos(r) * a / M_PI;
		}

		if(xform->vars[FRACTAL_VALIATION_SPIRAL] != 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty) + 1e-6;
			px += xform->vars[FRACTAL_VALIATION_SPIRAL] * ((cos(a) + sin(r)) / r);
			py += xform->vars[FRACTAL_VALIATION_SPIRAL] * ((sin(a) - cos(r)) / r);
		}

		if(xform->vars[FRACTAL_VALIATION_HYPERBOLIC] != 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			px += xform->vars[FRACTAL_VALIATION_HYPERBOLIC] * sin(a) * cos(r);
			py += xform->vars[FRACTAL_VALIATION_HYPERBOLIC] * cos(a) * sin(r);
		}

		if(xform->vars[FRACTAL_VALIATION_SQUARE] != 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			px += xform->vars[FRACTAL_VALIATION_SQUARE] * sin(a) * cos(r);
			py += xform->vars[FRACTAL_VALIATION_SQUARE] * cos(a) * sin(r);
		}

		if(xform->vars[FRACTAL_VALIATION_EX] != 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = sqrt(tx * tx + ty * ty);
			n0 = sin(a + r);
			n1 = cos(a - r);
			m0 = n0 * n0 * n0 * r;
			m1 = n1 * n1 * n1 * r;
			px += xform->vars[FRACTAL_VALIATION_EX] * (m0 + m1);
			py += xform->vars[FRACTAL_VALIATION_EX] * (m0 - m1);
		}

		if(xform->vars[FRACTAL_VALIATION_JULIA] != 0)
		{
			if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
			{
				a = atan2(tx, ty);
			}
			else
			{
				a = 0;
			}
			r = pow(tx * tx + ty * ty, 0.25);
			a += TRUNC(FRACTAL_FLOAT_RAND() * 2) * M_PI;
			nx = r * cos(a),	ny = r * sin(a);
			px += xform->vars[FRACTAL_VALIATION_JULIA] * nx;
			py += xform->vars[FRACTAL_VALIATION_JULIA] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_BENT] != 0)
		{
			nx = tx,	ny = ty;
			if(nx < 0 && nx > -1e100)
			{
				nx *= 2;
			}
			if(ny < 0)
			{
				ny /= 2;
			}
			px += xform->vars[FRACTAL_VALIATION_BENT] * nx;
			py += xform->vars[FRACTAL_VALIATION_BENT] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_WAVES] != 0)
		{
			dx = xform->c[2][0],	dy = xform->c[2][1];
			nx = tx + xform->c[1][0] * sin(ty / ((dx * dx) + FRACTAL_EPS));
			ny = ty + xform->c[1][1] * sin(tx / ((dy * dy) + FRACTAL_EPS));
			px += xform->vars[FRACTAL_VALIATION_WAVES] * nx;
			py += xform->vars[FRACTAL_VALIATION_WAVES] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_FISHEYE] != 0)
		{
			r = sqrt(tx * tx + ty * ty);
			a = atan2(tx, ty);
			r = 2 * r / (r + 1);
			nx = r * cos(a);
			ny = r * sin(a);
			px += xform->vars[FRACTAL_VALIATION_FISHEYE] * nx;
			py += xform->vars[FRACTAL_VALIATION_FISHEYE] * ny;
		}

		if(xform->vars[FRACTAL_VALIATION_POPCORN] != 0)
		{
			dx = tan(3 * ty);
			dy = tan(3 * tx);
			nx = tx + xform->c[2][0] * sin(dx);
			ny = ty + xform->c[2][1] * sin(dy);
			px += xform->vars[FRACTAL_VALIATION_POPCORN] * nx;
			py += xform->vars[FRACTAL_VALIATION_POPCORN] * ny;
		}

		if(i >= 0)
		{
			points[i][0] = px;
			points[i][1] = py;
			points[i][2] = pc;
		}
	}
}

void FractalPointCalculateBoundBox(FRACTAL_POINT* point, int compatibility)
{
	FLOAT_T (*points)[3];
	FLOAT_T delta_x;
	FLOAT_T min_x, max_x;
	FLOAT_T delta_y;
	FLOAT_T min_y, max_y;
	int count_min_x, count_max_x;
	int count_min_y, count_max_y;
	int limit_out_side_points;
	int i, j;

	points = (FLOAT_T (*)[3])MEM_ALLOC_FUNC(sizeof(*points) * FRACTAL_SUB_BATCH_SIZE);

	switch(compatibility)
	{
	case FRACTAL_COMPABILITY_TYPE_INTEGER:
		FractalPointIterate(point, FRACTAL_SUB_BATCH_SIZE, points);
		break;
	case FRACTAL_COMPABILITY_TYPE_FLOAT:
		FractalPointIterateFloat(point, FRACTAL_SUB_BATCH_SIZE, points);
		break;
	}

	limit_out_side_points = ROUND(0.05 * FRACTAL_SUB_BATCH_SIZE);

	min_x = 1e10;
	max_x = -1e10;
	min_y = 1e10;
	max_y = -1e10;
	for(i=0; i<FRACTAL_SUB_BATCH_SIZE; i++)
	{
		min_x = MINIMUM(min_x, points[i][0]);
		max_x = MAXIMUM(max_x, points[i][0]);
		min_y = MINIMUM(min_y, points[i][1]);
		max_y = MAXIMUM(max_y, points[i][1]);
	}

	delta_x = (max_x - min_x) * 0.25;
	max_x = (max_x + min_x) / 2;
	min_x = max_x;

	delta_y = (max_y - min_y) * 0.25;
	max_y = (max_y + min_y) / 2;
	min_y = max_y;

	for(j=0; j<=10; j++)
	{
		count_min_x = 0;
		count_max_x = 0;
		count_min_y = 0;
		count_max_y = 0;
		for(i=0; i<FRACTAL_SUB_BATCH_SIZE; i++)
		{
			if(points[i][0] < min_x)
			{
				count_min_x++;
			}
			if(points[i][0] > max_x)
			{
				count_max_x++;
			}
			if(points[i][1] < min_y)
			{
				count_min_y++;
			}
			if(points[i][1] > max_y)
			{
				count_max_y++;
			}

			if(count_min_x < limit_out_side_points)
			{
				min_x = min_x + delta_x;
			}
			else
			{
				min_x = min_x - delta_x;
			}

			if(count_max_x < limit_out_side_points)
			{
				max_x = max_x - delta_x;
			}
			else
			{
				max_x = max_x + delta_x;
			}

			delta_x = delta_x / 2;

			if(count_min_y < limit_out_side_points)
			{
				min_y = min_y + delta_y;
			}
			else
			{
				min_y = min_y - delta_y;
			}

			if(count_max_y < limit_out_side_points)
			{
				max_y = max_y - delta_y;
			}
			else
			{
				max_y = max_y + delta_y;
			}

			delta_y = delta_y / 2;
		}
		point->center[0] = (min_x + max_x) / 2;
		point->center[1] = (min_y + max_y) / 2;
		if((max_x - min_x) > 0.001 && ((max_y - min_y) > 0.001))
		{
			point->pixels_per_unit = 0.7 * MINIMUM(
				point->width / (max_x - min_x), point->height / (max_y - min_y));
		}
		else
		{
			point->pixels_per_unit = 10;
		}
	}

	MEM_FREE_FUNC(points);
}

void FractalPointRandomize(
	FRACTAL_POINT* point,
	int minimum,
	int maximum,
	int calculate,
	int compatibility
)
{
	const int xform_distrib[] = {2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 7, 8};
	const int var_distrib[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0,
		1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
	const mixed_var_distrib[] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5,
		6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
	int num_r_xforms;
	int v;
	int rv;
	int i, j;

	point->hue_rotation = 1;
	point->color_map_index = FRACTAL_RAND() % 84;
	FractalColorMapGetHueRotationMap(point->color_map_index,
		point->hue_rotation, point->color_map);
	point->time_value = 0;

	if(minimum < maximum)
	{
		num_r_xforms = (FRACTAL_RAND() % (maximum - minimum)) + minimum;
	}
	else
	{
		num_r_xforms = minimum;
	}

	do
	{
		rv = var_distrib[FRACTAL_RAND() % 37];
	} while(rv >= 0 && (point->var_flags & (1 << rv)) == 0);

	for(i=0; i<FRACTAL_NUM_XFORMS; i++)
	{
		point->xform[i].density = 0;
	}

	for(i=0; i<num_r_xforms; i++)
	{
		point->xform[i].density = 1.0 / num_r_xforms;
		point->xform[i].color = i / (FLOAT_T)(num_r_xforms - 1);

		point->xform[i].c[0][0] = 2 * FRACTAL_FLOAT_RAND() - 1;
		point->xform[i].c[0][1] = 2 * FRACTAL_FLOAT_RAND() - 1;
		point->xform[i].c[1][0] = 2 * FRACTAL_FLOAT_RAND() - 1;
		point->xform[i].c[1][1] = 2 * FRACTAL_FLOAT_RAND() - 1;
		point->xform[i].c[2][0] = 4 * FRACTAL_FLOAT_RAND() - 2;
		point->xform[i].c[2][1] = 4 * FRACTAL_FLOAT_RAND() - 2;

		for(j=0; j<FRACTAL_NUM_VARS; j++)
		{
			point->xform[i].vars[j] = 0;
		}

		if(rv < 0)
		{
			do
			{
				v = mixed_var_distrib[FRACTAL_RAND() % 27];
			} while((point->var_flags & (1 << v)) == 0);
			point->xform[i].vars[v] = 1;
		}
		else
		{
			point->xform[i].vars[rv] = 1;
		}
	}

	if(calculate)
	{
		FractalPointCalculateBoundBox(point, compatibility);
	}
}

void FractalPointClear(FRACTAL_POINT* point)
{
	int i, j;

	point->symmetry = 0;
	for(i=0; i<FRACTAL_NUM_XFORMS; i++)
	{
		point->xform[i].density = 0;
		point->xform[i].symmetry = 0;
		point->xform[i].color = 0;
		point->xform[i].vars[0] = 1;
		for(j=1; j<FRACTAL_NUM_VARS; j++)
		{
			point->xform[i].vars[j] = 0;
		}
	}

	point->zoom = 0;
}

int FractalPointGetNumXForms(FRACTAL_POINT* point)
{
	int ret = FRACTAL_NUM_XFORMS;
	int i;

	for(i=0; i<FRACTAL_NUM_XFORMS; i++)
	{
		if(point->xform[i].density == 0)
		{
			ret = i;
			break;
		}
	}

	return ret;
}

void FractalPointEqualizeWeights(FRACTAL_POINT* point)
{
	int t;
	int i;

	t = FractalPointGetNumXForms(point);
	for(i=0; i<t; i++)
	{
		point->xform[i].density = 1.0 / t;
	}
}

void FractalPointNormalizeWeights(FRACTAL_POINT* point)
{
	FLOAT_T td = 0;
	int loop;
	int i;

	loop = FractalPointGetNumXForms(point);
	for(i=0; i<loop; i++)
	{
		td += point->xform[i].density;
	}
	if(td < 0.001)
	{
		FractalPointEqualizeWeights(point);
	}
	else
	{
		for(i=0; i<loop; i++)
		{
			point->xform[i].density /= td;
		}
	}
}

void FractalPointSetVariation(FRACTAL_POINT* point, eFRACTAL_VALIATION variation)
{
	const int xform_distrib[] = {2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 7, 8};
	const int var_distrib[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0,
		1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
	const mixed_var_distrib[] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5,
		6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
	int v;
	int rv;
	int i, j;

	do
	{
		rv = var_distrib[FRACTAL_RAND() % 37];
	} while(rv >= 0 && (point->var_flags & (1 << rv)) == 0);

	for(i=0; i<FRACTAL_NUM_XFORMS; i++)
	{
		for(j=0; j<FRACTAL_NUM_VARS; j++)
		{
			point->xform[i].vars[j] = 0;
		}

		if(variation == FRACTAL_VALIATION_RANDOM)
		{
			if(rv < 0)
			{
				do
				{
					v = mixed_var_distrib[FRACTAL_RAND() % 27];
				} while((point->var_flags & (1 << v)) == 0);
				point->xform[i].vars[v] = 1;
			}
			else
			{
				point->xform[i].vars[rv] = 1;
			}
		}
		else
		{
			point->xform[i].vars[variation] = 1;
		}
	}
}

#define COPY_MATRIX3x3(DEST, SRC) \
	(DEST)[0] = (SRC)[0], \
	(DEST)[1] = (SRC)[1], \
	(DEST)[2] = (SRC)[2], \
	(DEST)[3] = (SRC)[3], \
	(DEST)[4] = (SRC)[4], \
	(DEST)[5] = (SRC)[5], \
	(DEST)[6] = (SRC)[6], \
	(DEST)[7] = (SRC)[7], \
	(DEST)[8] = (SRC)[8]

#define IDENTITY_MATRIX3x3 {1,0,0,0,1,0,0,0,1}

static void MulMatrix3x3(FLOAT_T matrix1[9], const FLOAT_T matrix2[9])
{
	FLOAT_T ret[9];
	ret[0] = matrix1[0] * matrix2[0] + matrix1[1] * matrix2[3] + matrix1[2] * matrix2[6];
	ret[1] = matrix1[0] * matrix2[1] + matrix1[1] * matrix2[4] + matrix1[2] * matrix2[7];
	ret[2] = matrix1[0] * matrix2[2] + matrix1[1] * matrix2[5] + matrix1[2] * matrix2[8];
	ret[3] = matrix1[3] * matrix2[0] + matrix1[4] * matrix2[3] + matrix1[5] * matrix2[6];
	ret[4] = matrix1[3] * matrix2[1] + matrix1[4] * matrix2[4] + matrix1[5] * matrix2[7];
	ret[5] = matrix1[3] * matrix2[2] + matrix1[4] * matrix2[5] + matrix1[5] * matrix2[8];
	ret[6] = matrix1[6] * matrix2[0] + matrix1[7] * matrix2[3] + matrix1[8] * matrix2[6];
	ret[7] = matrix1[6] * matrix2[1] + matrix1[7] * matrix2[4] + matrix1[8] * matrix2[7];
	ret[8] = matrix1[6] * matrix2[2] + matrix1[7] * matrix2[5] + matrix1[8] * matrix2[8];

	COPY_MATRIX3x3(matrix1, ret);
}

void FractalXFormTranslate(
	FRACTAL_POINT_XFORM* xform,
	FLOAT_T x,
	FLOAT_T y
)
{
	FLOAT_T m1[] = IDENTITY_MATRIX3x3;
	FLOAT_T matrix[] = IDENTITY_MATRIX3x3;

	m1[2] = x;
	m1[5] = y;
	matrix[0] = xform->c[0][0];
	matrix[1] = xform->c[0][1];
	matrix[3] = xform->c[1][0];
	matrix[4] = xform->c[1][1];
	matrix[2] = xform->c[2][0];
	matrix[5] = xform->c[2][1];
	MulMatrix3x3(matrix, m1);
	xform->c[0][0] = matrix[0];
	xform->c[0][1] = matrix[1];
	xform->c[1][0] = matrix[3];
	xform->c[1][1] = matrix[4];
	xform->c[2][0] = matrix[2];
	xform->c[2][1] = matrix[5];
}

void FractalXFormRotate(FRACTAL_POINT_XFORM* xform, FLOAT_T degrees)
{
	FLOAT_T r = degrees * M_PI / 180.0;
	FLOAT_T m1[] = IDENTITY_MATRIX3x3;
	FLOAT_T matrix[] = IDENTITY_MATRIX3x3;
	FLOAT_T sin_value, cos_value;

	sin_value = sin(r);
	cos_value = cos(r);
	m1[0] = cos_value;
	m1[1] = - sin_value;
	m1[3] = sin_value;
	m1[4] = cos_value;

	matrix[0] = xform->c[0][0];
	matrix[1] = xform->c[0][1];
	matrix[3] = xform->c[1][0];
	matrix[4] = xform->c[1][1];
	matrix[2] = xform->c[2][0];
	matrix[5] = xform->c[2][1];
	MulMatrix3x3(matrix, m1);
	xform->c[0][0] = matrix[0];
	xform->c[0][1] = matrix[1];
	xform->c[1][0] = matrix[3];
	xform->c[1][1] = matrix[4];
	xform->c[2][0] = matrix[2];
	xform->c[2][1] = matrix[5];
}

void FractalXFormScale(FRACTAL_POINT_XFORM* xform, FLOAT_T scale)
{
	FLOAT_T m1[] = IDENTITY_MATRIX3x3;
	FLOAT_T matrix[] = IDENTITY_MATRIX3x3;

	m1[0] = scale;
	m1[4] = scale;

	matrix[0] = xform->c[0][0];
	matrix[1] = xform->c[0][1];
	matrix[3] = xform->c[1][0];
	matrix[4] = xform->c[1][1];
	matrix[2] = xform->c[2][0];
	matrix[5] = xform->c[2][1];
	MulMatrix3x3(matrix, m1);
	xform->c[0][0] = matrix[0];
	xform->c[0][1] = matrix[1];
	xform->c[1][0] = matrix[3];
	xform->c[1][1] = matrix[4];
	xform->c[2][0] = matrix[2];
	xform->c[2][1] = matrix[5];
}

void FractalXFormMultiply(
	FRACTAL_POINT_XFORM* xform,
	FLOAT_T a,
	FLOAT_T b,
	FLOAT_T c,
	FLOAT_T d
)
{
	FLOAT_T m1[] = IDENTITY_MATRIX3x3;
	FLOAT_T matrix[] = IDENTITY_MATRIX3x3;

	m1[0] = a;
	m1[1] = b;
	m1[3] = c;
	m1[4] = d;
	matrix[0] = xform->c[0][0];
	matrix[1] = xform->c[0][1];
	matrix[3] = xform->c[1][0];
	matrix[4] = xform->c[1][1];
	matrix[2] = xform->c[2][0];
	matrix[5] = xform->c[2][1];
	MulMatrix3x3(matrix, m1);
	xform->c[0][0] = matrix[0];
	xform->c[0][1] = matrix[1];
	xform->c[1][0] = matrix[3];
	xform->c[1][1] = matrix[4];
	xform->c[2][0] = matrix[2];
	xform->c[2][1] = matrix[5];
}

int AddSymmetry2FractalPoint(
	FRACTAL_POINT* point,
	int symmetry
)
{
	const int sym_distrib[] = {-4, -3, -2, -2, -2,
		-1, -1, -1, 2, 2, 2, 3, 3, 4,4 };
	FLOAT_T a;
	int rand_value;
	int i, j, k;
	int ret = 0;

	if(symmetry == 0)
	{
		rand_value = FRACTAL_RAND();
		if((rand_value & 1) == 0)
		{
			symmetry = sym_distrib[rand_value % 14];
		}
		else if((rand_value % 32) != 0)
		{
			symmetry = FRACTAL_RAND() % 13 - 6;
		}
		else
		{
			symmetry = FRACTAL_RAND() % 51 - 25;
		}
	}

	if(symmetry == 0 || symmetry == 1)
	{
		return 0;
	}

	for(i=0; i<FRACTAL_NUM_XFORMS; i++)
	{
		if(point->xform[i].density == 0.0)
		{
			break;
		}
	}

	if(i == FRACTAL_NUM_XFORMS)
	{
		return 0;
	}
	point->symmetry = symmetry;

	if(symmetry < 0)
	{
		point->xform[i].density = 1;
		point->xform[i].symmetry = 1;
		point->xform[i].vars[0] = 1;
		for(j=1; j<FRACTAL_NUM_VARS; j++)
		{
			point->xform[i].vars[j] = 0;
		}
		point->xform[i].color = 1;
		point->xform[i].c[0][0] = -1.0;
		point->xform[i].c[0][1] = 0.0;
		point->xform[i].c[1][0] = 0.0;
		point->xform[i].c[1][1] = 1.0;
		point->xform[i].c[2][0] = 0.0;
		point->xform[i].c[2][1] = 0.0;

		i++;
		ret++;
		symmetry = - symmetry;
	}

	a = 2.0 * M_PI / symmetry;

	for(k=1; (k < symmetry) && (i < FRACTAL_NUM_XFORMS); k++)
	{
		point->xform[i].density = 1.0;
		point->xform[i].vars[0] = 1.0;
		point->xform[i].symmetry = 1;
		for(j=1; j<FRACTAL_NUM_VARS; j++)
		{
			point->xform[i].vars[j] = 0;
		}
		if(symmetry < 3)
		{
			point->xform[i].color = 0;
		}
		else
		{
			point->xform[i].color = (k - 1) / (FLOAT_T)(symmetry - 2);
		}

		if(point->xform[i].color > 1)
		{
			point->xform[i].color = point->xform[i].color - (int)(point->xform[i].color);
		}

		point->xform[i].c[0][0] = cos(k * a);
		point->xform[i].c[0][1] = sin(k * a);
		point->xform[i].c[1][0] = - point->xform[i].c[0][1];
		point->xform[i].c[1][1] = point->xform[i].c[0][0];
		point->xform[i].c[2][0] = 0.0;
		point->xform[i].c[2][1] = 0.0;

		i++;
		ret++;
	}

	return ret;
}

FLOAT_T FractalPointSolve3(
	FLOAT_T x1,
	FLOAT_T x2,
	FLOAT_T x1h,
	FLOAT_T y1,
	FLOAT_T y2,
	FLOAT_T y1h,
	FLOAT_T z1,
	FLOAT_T z2,
	FLOAT_T z1h,
	FLOAT_T* a,
	FLOAT_T* b,
	FLOAT_T* e
)
{
#define DET(A, B, C, D) ((A) * (D) - (B) * (C))
	FLOAT_T det = x1 * DET(y2, 1.0, z2, 1.0) - x2 * DET(y1, 1.0, z1, 1.0)
		+ DET(y1, y2, z1, z2);
	FLOAT_T div;
	FLOAT_T local_a, local_b, local_e;

	if(det == 0.0)
	{
		return 0.0;
	}
	div  = 1.0 / det;

	local_a = (x1h * DET(y2, 1.0, z2, 1.0) - x2 * DET(y1h, 1.0, z1h, 1.0)
		+ DET(y1h, y2, z1h, z2)) * div;
	local_b = (x1 * DET(y1h, 1.0, z1h, 1.0) - x1h * DET(y1, 1.0, z1, 1.0)
		+ DET(y1, y1h, z1, z1h)) * div;
	local_e = (x1 * DET(y2, y1h, z2, z1h) - x2 * DET(y1, y1h, z1, z1h)
		+ x1h * DET(y1, y2, z2, z1h)) * div;
	*a = ROUND6(local_a);
	*b = ROUND6(local_b);
	*e = ROUND6(local_e);

	return det;
}

void FractalPointGetXForms(
	FRACTAL_POINT* point,
	FLOAT_T (*triangles)[3][2],
	int t
)
{
	int i;
	for(i=0; i<t; i++)
	{
		FractalPointSolve3(triangles[0][0][0], -triangles[0][0][1], triangles[i+1][0][0],
			triangles[0][1][0], - triangles[0][1][1], triangles[i+1][1][0],
			triangles[0][2][0], - triangles[0][2][1], triangles[i+1][2][0],
			&point->xform[i].c[0][0], &point->xform[i].c[1][0], &point->xform[i].c[2][1]);

		FractalPointSolve3(triangles[0][0][0], - triangles[0][0][1], - triangles[i+1][0][1],
			triangles[0][1][0], - triangles[0][1][1], - triangles[i+1][1][1],
			triangles[0][2][0], - triangles[0][2][1], - triangles[i+1][2][1],
			&point->xform[i].c[0][1], &point->xform[i].c[1][1], &point->xform[i].c[2][1]);
	}
}

void FractalPointInterpolate(
	FRACTAL_POINT* dst,
	FRACTAL_POINT* point1,
	FRACTAL_POINT* point2,
	FLOAT_T tm
)
{
	FLOAT_T c0, c1;
	int i, j;

	if(point2->time_value - point1->time_value > 1e-6)
	{
		c0 = (point2->time_value - tm) / (point2->time_value - point1->time_value);
		c1 = 1 - c0;
	}
	else
	{
		c0 = 1;
		c1 = 0;
	}

	dst->time_value = tm;

	if(point1->color_map_inter == 0)
	{
		HSV hsv1, hsv2;
		HSV t_hsv;
		for(i=0; i<256; i++)
		{
			RGB2HSV_Pixel(point1->color_map[i], &hsv1);
			RGB2HSV_Pixel(point2->color_map[i], &hsv2);
			t_hsv.h = (int16)(c0 * hsv1.h + c1 * hsv2.h);
			if(t_hsv.h >= 360)
			{
				t_hsv.h -= (t_hsv.h / 360) * 360;
			}
			else if(t_hsv.h < 0)
			{
				t_hsv.h += (t_hsv.h / 360 + 1) * 360;
			}
			t_hsv.s = (uint8)(c0 * hsv1.s + c1 * hsv2.s);
			t_hsv.v = (uint8)(c0 * hsv1.v + c1 * hsv2.v);
			HSV2RGB_Pixel(&t_hsv, dst->color_map[i]);
		}
	}

	dst->color_map_index = -1;
	dst->brightness = c0 * point1->brightness + c1 * point2->brightness;
	dst->contrast = c0 * point1->contrast + c1 * point2->contrast;
	dst->gamma = c0 * point1->gamma + c1 * point2->gamma;
	dst->width = point1->width;
	dst->height = point1->height;
	dst->spatial_oversample = ROUND(
		c0 * point1->spatial_oversample + c1 * point2->spatial_oversample);
	dst->spatial_filter_radius = c0 * point1->spatial_filter_radius + c1 * point2->spatial_filter_radius;
	dst->sample_density = c0 * point1->sample_density + c1 * point2->sample_density;
	dst->center[0] = c0 * point1->center[0] + c1 * point2->center[0];
	dst->center[1] = c0 * point1->center[1] + c1 * point2->center[1];
	dst->pixels_per_unit = c0 * point1->pixels_per_unit + c1 * point2->pixels_per_unit;
	dst->zoom = c0 * point1->zoom + c1 * point2->zoom;
	dst->num_batches = ROUND(c0 * point1->num_batches + c1 * point2->num_batches);
	dst->white_level = ROUND(c0 * point1->white_level + c1 * point2->white_level);

	for(i=0; i<=3; i++)
	{
		dst->pulse[i/2][i%2] = c0 * point1->pulse[i/2][i%2] + c1 * point2->pulse[i/2][i%2];
		dst->wiggle[i/2][i%2] = c0 * point1->wiggle[i/2][i%2] + c1 * point2->wiggle[i/2][i%2];
	}

	for(i=0; i<FRACTAL_NUM_XFORMS; i++)
	{
		dst->xform[i].density = c0 * point1->xform[i].density + c1 * point2->xform[i].density;
		dst->xform[i].color = c0 * point1->xform[i].color + c1 * point2->xform[i].color;
		dst->xform[i].symmetry = c0 * point1->xform[i].symmetry + c1 * point2->xform[i].symmetry;
		for(j=0; j<FRACTAL_NUM_VARS; j++)
		{
			dst->xform[i].vars[j] = c0 * point1->xform[i].vars[j] + c1 * point2->xform[i].vars[j];
		}

		for(j=0; j<3; j++)
		{
			dst->xform[i].c[j][0] = c0 * point1->xform[i].c[j][0] + c1 * point2->xform[i].c[j][0];
			dst->xform[i].c[j][1] = c0 * point1->xform[i].c[j][1] + c1 * point2->xform[i].c[j][1];
		}
	}
}

#ifdef __cplusplus
}
#endif
