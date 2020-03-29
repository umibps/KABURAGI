#include <string.h>
#include <math.h>
#include "memory.h"
#include "fractal.h"
#include "fractal_color_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ROUND(X) (int)((X) + 0.5)
#define TRUNC(X) (int)((X))
#define RGB(R, G, B) (((B) << 16) | ((G) << 8) | (R))
#ifndef MINIMUM
# define MINIMUM(A, B) (((A) <= (B)) ? (A) : (B))
#endif

#ifndef FALSE
# define FALSE (0)
#endif

#ifndef TRUE
# define TRUE (!(0))
#endif

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#define DEFAULT_SAMPLE_DENSITY 5
#define DEFAULT_GAMMA 4
#define DEFAULT_BRIGHTNESS 4
#define DEFAULT_VIBRANCY 1
#define DEFAULT_OVERSAMPLE 1
#define DEFAULT_FILTER_RADIUS 0.2
#define DEFAULT_SYMMETRY_ORDER 4
#define RAND_MIN_TRANSFORMS 1
#define RAND_MAX_TRANSFORMS 10
#define PREFILTER_WHITE (1 << 26)
#define FILTER_CUT_OFF (2.5)
#define BRIGHT_ADJUST (2.3)

void InitializeFractal(FRACTAL* fractal, int width, int height)
{
	(void)memset(fractal, 0, sizeof(*fractal));

	InitializeFractalPoint(&fractal->control_point, width, height);
	fractal->pixels = (uint8 (*)[4])MEM_ALLOC_FUNC(
		sizeof(*fractal->pixels) * width * height);
	(void)memset(fractal->pixels, 0, sizeof(*fractal->pixels) * width * height);
	fractal->image_width = width;
	fractal->image_height = height;
	fractal->symmetry_order = DEFAULT_SYMMETRY_ORDER;
	fractal->nr_slices = 1;
	fractal->min_transforms = RAND_MIN_TRANSFORMS;
	fractal->max_transforms = RAND_MAX_TRANSFORMS;
	fractal->flags |= FRACTAL_FLAG_USE_TIME_SEED;
}

void ReleaseFractal(FRACTAL* fractal)
{
	//int i;

	ReleaseFractalPoint(&fractal->control_point);

	//for(i=0; i<fractal->filter_width; i++)
	//{
	//	MEM_FREE_FUNC(fractal->filter[i]);
	//}
	if(fractal->filter != NULL)
	{
		MEM_FREE_FUNC(fractal->filter[0]);
		MEM_FREE_FUNC(fractal->filter);
	}

	MEM_FREE_FUNC(fractal->buckets);
	MEM_FREE_FUNC(fractal->accumulate);

	MEM_FREE_FUNC(fractal->pixels);
}

void ResizeFractal(FRACTAL* fractal, int width, int height)
{
	ResizeFractalPoint(&fractal->control_point, width, height);
	fractal->pixels = (uint8 (*)[4])MEM_REALLOC_FUNC(fractal->pixels,
		sizeof(*fractal->pixels) * width * height);
	fractal->image_width = width;
	fractal->image_height = height;
}

void CopyFractal(FRACTAL* dst, FRACTAL* src)
{
	int i, j;

	ReleaseFractal(dst);
	*dst = *src;
	dst->pixels = (uint8 (*)[4])MEM_ALLOC_FUNC(
		dst->image_width * dst->image_height * 4);
	(void)memcpy(dst->pixels, src->pixels,
		dst->image_width * dst->image_height * 4);
	dst->filter = (FLOAT_T**)MEM_ALLOC_FUNC(
		sizeof(*dst->filter) * dst->filter_width);
	dst->filter[0] = (FLOAT_T*)MEM_ALLOC_FUNC(
		sizeof(*dst->filter[0]) * dst->filter_width * dst->filter_width);
	for(i=0; i<dst->filter_width; i++)
	{
		dst->filter[i] = &dst->filter[0][dst->filter_width * i];
		for(j=0; j<dst->filter_width; j++)
		{
			dst->filter[i][j] = src->filter[i][j];
		}
	}

	dst->buckets = (int (*)[4])MEM_ALLOC_FUNC(
		sizeof(*dst->buckets) * dst->bucket_size);
	for(i=0; i<dst->bucket_size; i++)
	{
		dst->buckets[i][0] = src->buckets[i][0];
		dst->buckets[i][1] = src->buckets[i][1];
		dst->buckets[i][2] = src->buckets[i][2];
		dst->buckets[i][3] = src->buckets[i][3];
	}

	if(src->accumulate != NULL)
	{
		dst->accumulate = (int (*)[4])MEM_ALLOC_FUNC(
			sizeof(*dst->accumulate) * dst->bucket_size);
		for(i=0; i<dst->bucket_size; i++)
		{
			dst->accumulate[i][0] = src->accumulate[i][0];
			dst->accumulate[i][1] = src->accumulate[i][1];
			dst->accumulate[i][2] = src->accumulate[i][2];
			dst->accumulate[i][3] = src->accumulate[i][3];
		}
	}

	dst->control_point.prop_table = NULL;
	CopyFractalPoint(&dst->control_point, &src->control_point);
}

void FractalClearBuckets(FRACTAL* fractal)
{
	int i;

	if((fractal->flags & FRACTAL_FLAG_STOP) != 0)
	{
		return;
	}

	for(i=0; i<fractal->bucket_size; i++)
	{
		fractal->buckets[i][0] = 0;
		fractal->buckets[i][1] = 0;
		fractal->buckets[i][2] = 0;
		fractal->buckets[i][3] = 0;
	}
}

void FractalClearBuffers(FRACTAL* fractal)
{
	int i;

	if((fractal->flags & FRACTAL_FLAG_USE_ACCULATION_BUFFER) != 0)
	{
		for(i=0; i<fractal->bucket_size; i++)
		{
			fractal->buckets[i][0] = 0;
			fractal->buckets[i][1] = 0;
			fractal->buckets[i][2] = 0;
			fractal->buckets[i][3] = 0;

			fractal->accumulate[i][0] = 0;
			fractal->accumulate[i][1] = 0;
			fractal->accumulate[i][2] = 0;
			fractal->accumulate[i][3] = 0;
		}
	}
	else
	{
		FractalClearBuckets(fractal);
	}
}

void FractalMakeBitMap(FRACTAL* fractal)
{
	FLOAT_T ls;
	FLOAT_T alpha_value;
	int alpha, red, green, blue;
	int vib;
	int not_vib;
	int bucket_position;
	uint8 back_ground[4];
	FLOAT_T filter_value;
	int filter_position;
	FLOAT_T fp[4];
	uint8 (*row)[4];
	int ii, jj;
	int i, j;

	fractal->vibrancy /= fractal->vib_gam_n;
	fractal->gamma = fractal->vib_gam_n / fractal->gamma;

	vib = ROUND(fractal->vibrancy * 256.0);
	not_vib = 256 - vib;

	back_ground[0] = (uint8)ROUND((256 * fractal->back_ground[0]) / fractal->vib_gam_n);
	back_ground[1] = (uint8)ROUND((256 * fractal->back_ground[1]) / fractal->vib_gam_n);
	back_ground[2] = (uint8)ROUND((256 * fractal->back_ground[2]) / fractal->vib_gam_n);
	back_ground[3] = (uint8)ROUND(fractal->back_ground[3] / 256);
	if(back_ground[0] > back_ground[3])
	{
		back_ground[0] = back_ground[3];
	}
	if(back_ground[1] > back_ground[3])
	{
		back_ground[1] = back_ground[3];
	}
	if(back_ground[2] > back_ground[3])
	{
		back_ground[2] = back_ground[3];
	}

	bucket_position = 0;
	alpha = 0;
	ls = 0;
	for(i=0; i<fractal->image_height; i++)
	{
		if((fractal->flags & FRACTAL_FLAG_STOP) != 0)
		{
			return;
		}

		row = &fractal->pixels[i*fractal->image_width];
		for(j=0; i<fractal->image_width; j++)
		{
			if(fractal->filter_width > 1)
			{
				fp[0] = fp[1] = fp[2] = fp[3] = 0;

				for(ii=0; ii<fractal->filter_width; ii++)
				{
					for(jj=0; jj<fractal->filter_width; jj++)
					{
						if((fractal->flags & FRACTAL_FLAG_STOP) != 0)
						{
							return;
						}

						filter_value = fractal->filter[ii][jj];

						filter_position = bucket_position + ii * fractal->bucket_width + jj;
						fp[0] += filter_value * fractal->accumulate[filter_position][0];
						fp[1] += filter_value * fractal->accumulate[filter_position][1];
						fp[2] += filter_value * fractal->accumulate[filter_position][2];
						fp[3] += filter_value * fractal->accumulate[filter_position][3];
					}
				}
			}	// if(fractal->filter_width > 1)
			else
			{
				fp[0] = fractal->accumulate[bucket_position][0];
				fp[1] = fractal->accumulate[bucket_position][1];
				fp[2] = fractal->accumulate[bucket_position][2];
				fp[3] = fractal->accumulate[bucket_position][3];
			}

			bucket_position += fractal->oversample;
			fp[0] /= PREFILTER_WHITE;
			fp[1] /= PREFILTER_WHITE;
			fp[2] /= PREFILTER_WHITE;
			fp[3] /= PREFILTER_WHITE;

			if(fp[3] > 0.0)
			{
				alpha_value = pow(fp[3], fractal->gamma);
				ls = vib * alpha_value / fp[3];
				alpha = ROUND(alpha_value * 256);
				if(alpha < 0)
				{
					alpha = 0;
				}
				else if(alpha > 256)
				{
					alpha = 256;
				}
				alpha = 256 - alpha;
			}
			else
			{
				row[j][0] = back_ground[0];
				row[j][1] = back_ground[1];
				row[j][2] = back_ground[2];
				row[j][3] = back_ground[3];
				continue;
			}

			if(not_vib > 0)
			{
				red = ROUND(ls * fp[0] + not_vib * pow(fp[0], fractal->gamma));
			}
			else
			{
				red = ROUND(ls * fp[0]);
			}
			red += (alpha * back_ground[0]) << 8;
			if(red < 0)
			{
				red = 0;
			}
			else if(red > 255)
			{
				red = 255;
			}

			if(not_vib > 0)
			{
				green = ROUND(ls * fp[1] + not_vib * pow(fp[1], fractal->gamma));
			}
			else
			{
				green = ROUND(ls * fp[1]);
			}
			green += (alpha * back_ground[1]) << 8;
			if(green < 0)
			{
				green = 0;
			}
			else if(green > 255)
			{
				green = 255;
			}

			if(not_vib > 0)
			{
				blue = ROUND(ls * fp[2] + not_vib * pow(fp[2], fractal->gamma));
			}
			else
			{
				blue = ROUND(ls * fp[2]);
			}
			blue += (alpha * back_ground[2]) << 8;
			if(blue < 0)
			{
				blue = 0;
			}
			else if(blue > 255)
			{
				blue = 255;
			}

			row[j][0] = (uint8)blue;
			row[j][1] = (uint8)green;
			row[j][2] = (uint8)red;
		}

		bucket_position += 2 * fractal->gutter_width;
		bucket_position += (fractal->oversample - 1) * fractal->bucket_width;
	}
}

void FractalMakeBitMapFromBuckets(FRACTAL* fractal, int y_offset)
{
	FLOAT_T alpha_value;
	int alpha, red, green, blue;
	FLOAT_T ls;
	FLOAT_T fp[4];
	uint8 (*pixels)[4];
	uint8 back_ground[4];
	int vib;
	int not_vib;
	int bucket_position;
	FLOAT_T filter_value;
	int filter_position;
	FLOAT_T lsa[1025];
	FLOAT_T k1, k2;
	FLOAT_T area;
	int ii, jj;
	int i, j;

	if(fractal->control_point.gamma != 0)
	{
		fractal->gamma = 1.0 / fractal->control_point.gamma;
	}

	fractal->vibrancy = fractal->control_point.vibrancy;
	vib = ROUND(fractal->vibrancy * 256.0);
	not_vib = 256 - vib;

	back_ground[0] = (uint8)ROUND((256 * fractal->back_ground[0]) / fractal->vib_gam_n);
	back_ground[1] = (uint8)ROUND((256 * fractal->back_ground[1]) / fractal->vib_gam_n);
	back_ground[2] = (uint8)ROUND((256 * fractal->back_ground[2]) / fractal->vib_gam_n);
	back_ground[3] = (uint8)ROUND(fractal->back_ground[3] * 255);
	if(back_ground[0] > back_ground[3])
	{
		back_ground[0] = back_ground[3];
	}
	if(back_ground[1] > back_ground[3])
	{
		back_ground[1] = back_ground[3];
	}
	if(back_ground[2] > back_ground[3])
	{
		back_ground[2] = back_ground[3];
	}

	k1 = (fractal->control_point.contrast * BRIGHT_ADJUST
		* fractal->control_point.brightness * 268 * PREFILTER_WHITE) / 256.0;
	area = fractal->image_width * fractal->image_height / (fractal->pp_ux * fractal->pp_uy);
	k2 = (fractal->oversample * fractal->oversample)
		/ (fractal->control_point.contrast * area * fractal->control_point.white_level * fractal->sample_density);

	lsa[0] = 0;
	for(i=1; i<=1024; i++)
	{
		lsa[i] = (k1 * log10(1 + fractal->control_point.white_level * i * k2))
			/ (fractal->control_point.white_level * i);
	}

	if(fractal->filter_width > 1)
	{
		for(i=0; i<fractal->bucket_width * fractal->bucket_height; i++)
		{
			if(fractal->buckets[i][3] == 0)
			{
				continue;
			}

			ls = lsa[MINIMUM(1023, fractal->buckets[i][3])];

			fractal->buckets[i][0] = ROUND(fractal->buckets[i][0] * ls);
			fractal->buckets[i][1] = ROUND(fractal->buckets[i][1] * ls);
			fractal->buckets[i][2] = ROUND(fractal->buckets[i][2] * ls);
			fractal->buckets[i][3] = ROUND(fractal->buckets[i][3] * ls);
		}
	}

	ls = 0;
	alpha = 0;
	bucket_position = 0;
	for(i=0; i<fractal->image_height; i++)
	{
		pixels = &fractal->pixels[(y_offset + i) * fractal->image_width];
		for(j=0; j<fractal->image_width; j++)
		{
			if(fractal->filter_width > 1)
			{
				fp[0] = fp[1] = fp[2] = fp[3] = 0;
				for(ii=0; ii<fractal->filter_width; ii++)
				{
					for(jj=0; jj<fractal->filter_width; jj++)
					{
						filter_value = fractal->filter[ii][jj];
						filter_position = bucket_position + ii * fractal->bucket_width + jj;

						fp[0] += fractal->buckets[filter_position][0];
						fp[1] += fractal->buckets[filter_position][1];
						fp[2] += fractal->buckets[filter_position][2];
						fp[3] += fractal->buckets[filter_position][3];
					}
				}

				fp[0] /= PREFILTER_WHITE;
				fp[1] /= PREFILTER_WHITE;
				fp[2] /= PREFILTER_WHITE;
				fp[3] = fractal->control_point.white_level * fp[3] / PREFILTER_WHITE;
			}	// if(fractal->filter_width > 1)
			else
			{
				ls = lsa[MINIMUM(1023, fractal->buckets[bucket_position][3])] / PREFILTER_WHITE;

				fp[0] = ls * fractal->buckets[bucket_position][0];
				fp[1] = ls * fractal->buckets[bucket_position][1];
				fp[2] = ls * fractal->buckets[bucket_position][2];
				fp[3] = ls * fractal->buckets[bucket_position][3] * fractal->control_point.white_level;
			}

			bucket_position += fractal->oversample;

			if(fp[3] > 0.0)
			{
				alpha_value = pow(fp[3], fractal->gamma);
				ls = vib * alpha_value / fp[3];
				alpha = ROUND(alpha_value * 256);
				if(alpha < 0)
				{
					alpha = 0;
				}
				else if(alpha > 255)
				{
					alpha = 255;
				}
			}
			else
			{
				pixels[j][0] = back_ground[0];
				pixels[j][1] = back_ground[1];
				pixels[j][2] = back_ground[2];
				pixels[j][3] = back_ground[3];
				continue;
			}

			if(not_vib > 0)
			{
				red = ROUND(ls * fp[0] + not_vib * pow(fp[0], fractal->gamma));
			}
			else
			{
				red = ROUND(ls * fp[0]);
			}
			red += (alpha * back_ground[0]) << 8;
			if(red < 0)
			{
				red = 0;
			}
			else if(red > 255)
			{
				red = 255;
			}

			if(not_vib > 0)
			{
				green = ROUND(ls * fp[1] + not_vib * pow(fp[1], fractal->gamma));
			}
			else
			{
				green = ROUND(ls * fp[1]);
			}
			green += (alpha * back_ground[1]) << 8;
			if(green < 0)
			{
				green = 0;
			}
			else if(green > 255)
			{
				green = 255;
			}

			if(not_vib > 0)
			{
				blue = ROUND(ls * fp[2] + not_vib * pow(fp[2], fractal->gamma));
			}
			else
			{
				blue = ROUND(ls * fp[2]);
			}
			blue += (alpha * back_ground[2]) << 8;
			if(blue < 0)
			{
				blue = 0;
			}
			else if(blue > 255)
			{
				blue = 255;
			}

			pixels[j][0] = (uint8)red;
			pixels[j][1] = (uint8)green;
			pixels[j][2] = (uint8)blue;
			pixels[j][3] = (uint8)alpha;
		}

		bucket_position += 2 * fractal->gutter_width;
		bucket_position += (fractal->oversample - 1) * fractal->bucket_width;
	}
}

void FractalMakeCamera(FRACTAL* fractal)
{
	FLOAT_T scale;
	FLOAT_T t0, t1;
	FLOAT_T corner0, corner1;
	int shift;

	scale = pow(2, fractal->control_point.zoom);
	fractal->sample_density =
		fractal->control_point.sample_density * scale * scale;
	fractal->pp_ux = fractal->control_point.pixels_per_unit * scale;
	fractal->pp_uy = fractal->control_point.pixels_per_unit * scale;

	shift = 0;
	t0 = fractal->gutter_width / (fractal->oversample * fractal->pp_ux);
	t1 = fractal->gutter_width / (fractal->oversample * fractal->pp_uy);
	corner0 = fractal->control_point.center[0]
		- fractal->image_width / fractal->pp_ux / 2.0;
	corner1 = fractal->control_point.center[1]
		- fractal->image_height / fractal->pp_uy / 2.0;
	fractal->bounds[0] = corner0 - t0;
	fractal->bounds[1] = corner1 - t1 + shift;
	fractal->bounds[2] = corner0 + fractal->image_width / fractal->pp_ux + t0;
	fractal->bounds[3] = corner1 + fractal->image_height / fractal->pp_uy + t1;
	if(fabs(fractal->bounds[2] - fractal->bounds[0]) > 0.01)
	{
		fractal->size[0] = 1.0 / (fractal->bounds[2] - fractal->bounds[0]);
	}
	else
	{
		fractal->size[0] = 1;
	}

	if(fabs(fractal->bounds[3] - fractal->bounds[1]) > 0.01)
	{
		fractal->size[1] = 1.0 / (fractal->bounds[3] - fractal->bounds[1]);
	}
	else
	{
		fractal->size[1] = 1;
	}
}

void FractalMakeColorMap(FRACTAL* fractal)
{
	int i, j;

	for(i=0; i<256; i++)
	{
		for(j=0; j<3; j++)
		{
			fractal->color_map[i][j] =
				(uint8)((fractal->control_point.color_map[i][j] * fractal->control_point.white_level) / 256.0);
		}
		fractal->color_map[i][3] = (uint8)fractal->control_point.white_level;
	}
}

void FractalNormalizeFilter(FRACTAL* fractal)
{
	FLOAT_T t = 0;
	int i, j;

	for(i=0; i<fractal->filter_width; i++)
	{
		for(j=0; j<fractal->filter_width; j++)
		{
			t += fractal->filter[i][j];
		}
	}

	for(i=0; i<fractal->filter_width; i++)
	{
		for(j=0; j<fractal->filter_width; j++)
		{
			fractal->filter[i][j] /= t;
		}
	}
}

void FractalMakeFilter(FRACTAL* fractal)
{
	int i, j;

	fractal->oversample = fractal->control_point.spatial_oversample;
	fractal->filter_width = ROUND(2.0 * FILTER_CUT_OFF * fractal->oversample
		* fractal->control_point.spatial_filter_radius);

	if(((fractal->filter_width + fractal->oversample) & 1) != 0)
	{
		fractal->filter_width++;
	}

	fractal->filter = (FLOAT_T**)MEM_ALLOC_FUNC(
		sizeof(*fractal->filter) * fractal->filter_width);
	fractal->filter[0] = (FLOAT_T*)MEM_ALLOC_FUNC(
		sizeof(*fractal->filter[0]) * fractal->filter_width * fractal->filter_width);
	for(i=0; i<fractal->filter_width; i++)
	{
		fractal->filter[i] = &fractal->filter[0][i*fractal->filter_width];
		for(j=0; j<fractal->filter_width; j++)
		{
			fractal->filter[i][j] = exp(-2.0 * pow(((2.0 * i + 1.0) / fractal->filter_width -1.0)
				* FILTER_CUT_OFF , 2) * pow(((2.0 * j + 1.0) / fractal->filter_width - 1.0) * FILTER_CUT_OFF, 2));
		}
	}

	FractalNormalizeFilter(fractal);
}

void FractalFillAccumulation(FRACTAL* fractal, FLOAT_T filter)
{
	FLOAT_T k1, k2;
	FLOAT_T area;
	FLOAT_T ls;
	int i;

	fractal->vibrancy += fractal->control_point.vibrancy;
	fractal->gamma += fractal->control_point.gamma;
	fractal->vib_gam_n++;

	fractal->back_ground[0] += fractal->control_point.back_bround[0] / 256.0;
	fractal->back_ground[1] += fractal->control_point.back_bround[1] / 256.0;
	fractal->back_ground[2] += fractal->control_point.back_bround[2] / 256.0;

	k1 = (filter * fractal->control_point.contrast * BRIGHT_ADJUST
		* fractal->control_point.brightness * 268 * PREFILTER_WHITE) / 256.0;
	area = fractal->image_width * fractal->image_height / (fractal->pp_ux * fractal->pp_uy);
	k2 = (fractal->oversample * fractal->oversample * fractal->control_point.num_batches)
		/ (fractal->control_point.contrast * area * fractal->control_point.white_level * fractal->sample_density);

	for(i=0; i<fractal->bucket_width * fractal->bucket_height; i++)
	{
		if(fractal->buckets[i][3] == 0)
		{
			continue;
		}

		ls = (k1 * log10(1 + fractal->buckets[i][3] * k2)) / fractal->buckets[i][3];
		fractal->accumulate[i][0] += ROUND(fractal->buckets[i][0] * ls);
		fractal->accumulate[i][1] += ROUND(fractal->buckets[i][1] * ls);
		fractal->accumulate[i][2] += ROUND(fractal->buckets[i][2] * ls);
		fractal->accumulate[i][3] += ROUND(fractal->buckets[i][3] * ls);
	}
}

void FractalInitializeBuffers(FRACTAL* fractal)
{
	int before_bucket_size = fractal->bucket_size;
	fractal->gutter_width = (fractal->filter_width - fractal->oversample) / 2;
	fractal->bucket_height = fractal->oversample * fractal->image_height + 2 * fractal->gutter_width;
	fractal->bucket_width = fractal->oversample * fractal->image_width + 2 * fractal->gutter_width;
	fractal->bucket_size = fractal->bucket_width * fractal->bucket_height;

	if(before_bucket_size != fractal->bucket_size)
	{
		fractal->buckets = (int (*)[4])MEM_REALLOC_FUNC(fractal->buckets,
			sizeof(*fractal->buckets) * fractal->bucket_size);
		if((fractal->flags & FRACTAL_FLAG_USE_ACCULATION_BUFFER) != 0)
		{
			fractal->accumulate = (int (*)[4])MEM_REALLOC_FUNC(fractal->accumulate,
				sizeof(*fractal->accumulate) * fractal->bucket_size);
		}
	}
}

void FractalInitializeValues(FRACTAL* fractal)
{
	fractal->flags &= ~(FRACTAL_FLAG_STOP);
	fractal->image_height = fractal->control_point.height;
	fractal->image_width = fractal->control_point.width;

	if(fractal->control_point.num_batches > 1)
	{
		fractal->flags |= FRACTAL_FLAG_USE_ACCULATION_BUFFER;
	}

	FractalMakeFilter(fractal);
	FractalMakeCamera(fractal);

	FractalInitializeBuffers(fractal);

	FractalMakeColorMap(fractal);

	fractal->vibrancy = 0;
	fractal->gamma = 0;
	fractal->vib_gam_n = 0;
	fractal->back_ground[0] = 0;
	fractal->back_ground[1] = 0;
	fractal->back_ground[2] = 0;
}

void FractalSetPixels(FRACTAL* fractal)
{
	uint32 num_samples;
	uint32 num_r_batches;
	FLOAT_T (*points)[3];
	unsigned int color_index;
	int bucket_position;
	FLOAT_T px, py;
	FLOAT_T bws, bhs;
	FLOAT_T bx, by;
	int i, j;

	points = (FLOAT_T (*)[3])MEM_ALLOC_FUNC(
		sizeof(*points) * FRACTAL_SUB_BATCH_SIZE);

	bws = fractal->bucket_width * fractal->size[0];
	bhs = fractal->bucket_height * fractal->size[1];
	bx = fractal->bounds[0];
	by = fractal->bounds[1];

	num_samples = ROUND(fractal->sample_density * fractal->bucket_size
		/ (fractal->oversample * fractal->oversample));
	num_r_batches = ROUND(num_samples / (fractal->control_point.num_batches * FRACTAL_SUB_BATCH_SIZE));

	FRACTAL_SRAND(fractal->random_seed);
	for(i=0; i<(int)num_r_batches; i++)
	{
		switch(fractal->compatiblity)
		{
		case FRACTAL_COMPABILITY_TYPE_INTEGER:
			FractalPointIterate(&fractal->control_point, FRACTAL_SUB_BATCH_SIZE, points);
			break;
		case FRACTAL_COMPABILITY_TYPE_FLOAT:
			FractalPointIterateFloat(&fractal->control_point, FRACTAL_SUB_BATCH_SIZE, points);
			break;
		}

		for(j=FRACTAL_SUB_BATCH_SIZE-1; j>=0; j--)
		{
			px = points[j][0];
			py = points[j][1];

			if((px < bx || px > fractal->bounds[2]) || py < by || py > fractal->bounds[3])
			{
				continue;
			}

			color_index = ROUND(points[j][2] * 255);
			if(color_index > 255)
			{
				color_index = 0;
			}

			bucket_position = TRUNC(bws * (px - bx)) + TRUNC(bhs * (py - by)) * fractal->bucket_width;

			fractal->buckets[bucket_position][0] += fractal->color_map[color_index][0];
			fractal->buckets[bucket_position][1] += fractal->color_map[color_index][1];
			fractal->buckets[bucket_position][2] += fractal->color_map[color_index][2];
			fractal->buckets[bucket_position][3]++;
		}
	}

	MEM_FREE_FUNC(points);
}

void FractalRender(FRACTAL* fractal, int random_seed)
{
	fractal->random_start = fractal->random_seed = random_seed;
	FractalInitializeValues(fractal);
	FractalClearBuffers(fractal);
	FractalSetPixels(fractal);

	if((fractal->flags & FRACTAL_FLAG_USE_ACCULATION_BUFFER) != 0)
	{
		FractalFillAccumulation(fractal, 1);
		FractalMakeBitMap(fractal);
	}
	else
	{
		FractalMakeBitMapFromBuckets(fractal, 0);
	}
}

void FractalRandomVariation(FRACTAL* fractal, FRACTAL_POINT* point)
{
	int a, b;
	int i, j;

	fractal->random_seed++;
	FRACTAL_SRAND(fractal->random_seed);

	for(i=0; i<FRACTAL_NUM_XFORMS; i++)
	{
		for(j=0; j<FRACTAL_NUM_VARS; j++)
		{
			point->xform[i].vars[j] = 0;
		}

		do
		{
			a = FRACTAL_RAND() % FRACTAL_NUM_VARS;
		} while((point->var_flags & (1 << a)) == 0);

		do
		{
			b = FRACTAL_RAND() % FRACTAL_NUM_VARS;
		} while((point->var_flags & (1 << b)) == 0);

		if(a == b)
		{
			point->xform[i].vars[a] = 1;
		}
		else
		{
			point->xform[i].vars[a] = FRACTAL_FLOAT_RAND();
			point->xform[i].vars[b] = 1 - point->xform[i].vars[a];
		}
	}
}

void FractalSetVariation(FRACTAL* fractal, FRACTAL_POINT* point)
{
	int i, j;

	if(fractal->variation == FRACTAL_VALIATION_RANDOM)
	{
		FractalRandomVariation(fractal, point);
	}
	else
	{
		for(i=0; i<FRACTAL_NUM_XFORMS; i++)
		{
			for(j=0; j<FRACTAL_NUM_VARS; j++)
			{
				point->xform[i].vars[j] = 0;
			}
			point->xform[i].vars[fractal->variation] = 1;
		}
	}
}

int FractalTrianglesFromControlPoint(
	FRACTAL* fractal,
	FRACTAL_POINT* point,
	FLOAT_T (*triangles)[3][2]
)
{
	int xforms;
	FLOAT_T temp_x, temp_y;
	FLOAT_T x_set, y_set;
	FLOAT_T left = 0, top = 0, bottom = 0, right = 0;
	FLOAT_T a, b, c, d, e, f;
	int i, j;

	xforms = FractalPointGetNumXForms(point);
	if((fractal->flags & FRACTAL_FLAG_FIXED_REFERENCE) == 0)
	{
		for(i=0; i<xforms; i++)
		{
			a = point->xform[i].c[0][0];
			b = point->xform[i].c[0][1];
			c = point->xform[i].c[1][0];
			d = point->xform[i].c[1][1];
			e = point->xform[i].c[2][0];
			f = point->xform[i].c[2][1];
			x_set = 1,	y_set = 1;
			for(j=0; j<6; j++)
			{
				temp_x = x_set * a + y_set * c + e;
				temp_y = x_set * b + y_set * d + f;
				x_set = temp_x;
				y_set = temp_y;
			}

			if(i == 0)
			{
				left = right = x_set;
				top = bottom = y_set;
			}
			else
			{
				if(x_set < left)
				{
					left = x_set;
				}
				if(x_set > right)
				{
					right = x_set;
				}
				if(y_set < top)
				{
					top = y_set;
				}
				if(y_set > bottom)
				{
					bottom = y_set;
				}
			}
		}

		triangles[0][0][0] = left;
		triangles[0][0][1] = bottom;
		triangles[0][1][0] = right;
		triangles[0][1][1] = bottom;
		triangles[0][2][0] = right;
		triangles[0][2][1] = top;
	}
	else
	{
		triangles[0][0][0] = 0;
		triangles[0][0][1] = 0;
		triangles[0][1][0] = 1;
		triangles[0][1][1] = 0;
		triangles[0][2][0] = 1;
		triangles[0][2][1] = 1.5;
	}

	for(j=0; j<xforms; j++)
	{
		a = point->xform[j].c[0][0];
		b = point->xform[j].c[0][1];
		c = point->xform[j].c[1][0];
		d = point->xform[j].c[1][1];
		e = point->xform[j].c[2][0];
		f = point->xform[j].c[2][1];
		for(i=0; i<3; i++)
		{
			triangles[j+1][i][0] = triangles[0][i][0] * a
				+ triangles[0][i][1] * c + e;
			triangles[j+1][i][1] = triangles[0][i][0] * b
				+ triangles[0][i][1] * d + f;
		}
	}

	for(i=0; i<=xforms; i++)
	{
		for(j=0; j<3; j++)
		{
			triangles[i][j][1] = - triangles[i][j][1];
		}
	}

	return xforms;
}

int FractalBlowsUp(FRACTAL* fractal, FRACTAL_POINT* point, int num_r_points)
{
	FRACTAL_POINT_XFORM *xform;
	int ret = FALSE;
	FLOAT_T px, py;
	FLOAT_T pc;
	FLOAT_T dx, dy;
	FLOAT_T tx, ty;
	FLOAT_T nx, ny;
	FLOAT_T r, r2;
	FLOAT_T c1, c2;
	FLOAT_T a;
	FLOAT_T v;
	FLOAT_T n0, n1, m0, m1;
	int i;

	px = FRACTAL_FLOAT_RAND() * 2 - 1;
	py = FRACTAL_FLOAT_RAND() * 2 - 1;
	pc = FRACTAL_FLOAT_RAND();

	FractalPointPreparePropTable(point);

	for(i=-100; i<num_r_points; i++)
	{
		xform = &point->xform[point->prop_table[FRACTAL_RAND() % 1024]];
		pc = (pc + xform->color) / 2.0;

		tx = xform->c[0][0] * px + xform->c[1][0] * py + xform->c[2][0];
		ty = xform->c[0][1] * px + xform->c[1][1] * py + xform->c[2][1];

		px = 0,	py = 0;

		switch(fractal->compatiblity)
		{
		case FRACTAL_COMPABILITY_TYPE_INTEGER:
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
				c1 = sin(r),	c2 = cos(r);
				px += xform->vars[FRACTAL_VALIATION_SWIRL] * (c1 * tx - c2 * ty);
				py += xform->vars[FRACTAL_VALIATION_SWIRL] * (c2 * tx + c1 * ty);
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
				c1 = sin(a),	c2 = cos(a);
				px += xform->vars[FRACTAL_VALIATION_HORSESHOE] * (c1 * tx - c2 * ty);
				py += xform->vars[FRACTAL_VALIATION_HORSESHOE] * (c2 * tx + c1 * ty);
			}

			if(xform->vars[FRACTAL_VALIATION_POLAR] > 0)
			{
				if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
				{
					a = atan2(tx, ty);
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
				nx = tx,	ny = ty;
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

				px += xform->vars[FRACTAL_VALIATION_SPIRAL] * (sin(a) / r);
				py += xform->vars[FRACTAL_VALIATION_SPIRAL] * (cos(a) * r);
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
			if(v > 0.0)
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
			if(v > 0.0)
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
				nx = tx * xform->c[1][0] * sin(ty + tan(3 * ty) + FRACTAL_EPS);
				ny = ty * xform->c[1][1] * sin(tx + tan(3 * tx) + FRACTAL_EPS);
				px += xform->vars[FRACTAL_VALIATION_POPCORN] * nx;
				py += xform->vars[FRACTAL_VALIATION_POPCORN] * ny;
			}

			break;
		case FRACTAL_COMPABILITY_TYPE_FLOAT:
			if(xform->vars[FRACTAL_VALIATION_LINEAR] != 0)
			{
				nx = tx,	ny = ty;
				px += xform->vars[FRACTAL_VALIATION_LINEAR] * nx;
				py += xform->vars[FRACTAL_VALIATION_LINEAR] * ny;
			}

			if(xform->vars[FRACTAL_VALIATION_SINUSOIDAL] != 0)
			{
				px += xform->vars[FRACTAL_VALIATION_SINUSOIDAL] * sin(tx);
				py += xform->vars[FRACTAL_VALIATION_SINUSOIDAL] * sin(ty);
			}

			if(xform->vars[FRACTAL_VALIATION_SPHERICAL] != 0)
			{
				r2 = tx * tx + ty * ty + 1e-6;
				px += xform->vars[FRACTAL_VALIATION_SPHERICAL] * tx / r2;
				py += xform->vars[FRACTAL_VALIATION_SPHERICAL] * ty / r2;
			}

			if(xform->vars[FRACTAL_VALIATION_SWIRL] != 0)
			{
				r2 = tx * tx + ty * ty;
				c1 = sin(r2),	c2 = cos(r2);
				px += xform->vars[FRACTAL_VALIATION_SWIRL] * (c1 * tx - c2 * ty);
				py += xform->vars[FRACTAL_VALIATION_SWIRL] * (c2 * tx + c1 * ty);
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
				c1 = sin(a),	c2 = cos(a);
				px += xform->vars[FRACTAL_VALIATION_HORSESHOE] * (c1 * tx - c2 * ty);
				py += xform->vars[FRACTAL_VALIATION_HORSESHOE] * (c2 * tx + c1 * ty);
			}

			if(xform->vars[FRACTAL_VALIATION_POLAR] != 0)
			{
				if(tx < - FRACTAL_EPS || tx > FRACTAL_EPS || ty < - FRACTAL_EPS || ty > FRACTAL_EPS)
				{
					a = atan2(tx, ty);
				}
				else
				{
					a = 0;
				}
				r = sqrt(tx * tx + ty * ty) - 1;
				px += xform->vars[FRACTAL_VALIATION_POLAR] * a;
				py += xform->vars[FRACTAL_VALIATION_POLAR] * r;
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
				a = a * r;

				px += xform->vars[FRACTAL_VALIATION_HEART] * (sin(a) * r);
				py += xform->vars[FRACTAL_VALIATION_HEART] * (cos(a) * -r);
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
				r = sqrt(nx * nx + ny * ny);
				px += xform->vars[FRACTAL_VALIATION_DISC] * (sin(r) * a / M_PI);
				py += xform->vars[FRACTAL_VALIATION_DISC] * (cos(r) * a / M_PI);
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

				px += xform->vars[FRACTAL_VALIATION_SPIRAL] * (cos(a) + sin(r)) / r;
				py += xform->vars[FRACTAL_VALIATION_SPIRAL] * (sin(a) - cos(r)) / r;
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
				r = sqrt(tx * tx + ty * ty) + 1e-6;

				px += xform->vars[FRACTAL_VALIATION_HYPERBOLIC] * (sin(a) / r);
				py += xform->vars[FRACTAL_VALIATION_HYPERBOLIC] * (cos(a) * r);
			}

			if(xform->vars[FRACTAL_VALIATION_SQUARE] != 0.0)
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

			if(xform->vars[FRACTAL_VALIATION_EX] != 0.0)
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
				a = a + TRUNC(FRACTAL_FLOAT_RAND() * 2) * M_PI;
				nx = r * cos(a);
				ny = r * sin(a);
				px += xform->vars[FRACTAL_VALIATION_JULIA] * nx;
				py -= xform->vars[FRACTAL_VALIATION_JULIA] * ny;
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

			break;
		}
	}

	return ret;
}


void FractalRandomizeControlPoint(
	FRACTAL* fractal,
	FRACTAL_POINT* point,
	int minimum,
	int maximum,
	int algorithm
)
{
	int v_rand;
	int rnd;
	FLOAT_T triangles[FRACTAL_NUM_XFORMS+1][3][2];
	FLOAT_T r, s;
	FLOAT_T theta, phi;
	int skip;
	int i, j;

	fractal->random_seed = fractal->random_start;
	
	// switch(rand_gradient)
	point->color_map_index = FRACTAL_RAND() % 84;
	FractalColorMapGetHueRotationMap(point->color_map_index, 1, point->color_map);

	fractal->random_seed++;
	FRACTAL_SRAND(fractal->random_seed);
	if(minimum < maximum)
	{
		fractal->transforms = FRACTAL_RAND() % (maximum - (minimum - 1)) + minimum;
	}
	else
	{
		fractal->transforms = minimum;
	}

	do
	{
		fractal->random_seed++;
		FRACTAL_SRAND(fractal->random_seed);
		FractalPointClear(point);
		FractalPointRandomize(point, fractal->transforms, fractal->transforms,
			FALSE, fractal->compatiblity);
		FractalPointSetVariation(point, fractal->variation);

		switch(algorithm)
		{
		case 1:
			rnd = 0;
			break;
		case 2:
			rnd = 7;
			break;
		case 3:
			rnd = 9;
			break;
		default:
			if(fractal->variation == FRACTAL_VALIATION_LINEAR
				|| fractal->variation == FRACTAL_VALIATION_RANDOM)
			{
				rnd = FRACTAL_RAND() % 10;
			}
			else
			{
				rnd = 9;
			}
		}

		if(rnd <= 6)
		{
			for(i=0; i<fractal->transforms; i++)
			{
				if((FRACTAL_RAND() % 10) < 9)
				{
					point->xform[i].c[0][0] = 1;
				}
				else
				{
					point->xform[i].c[0][0] = -1;
				}
				point->xform[i].c[0][1] = 0;
				point->xform[i].c[1][0] = 0;
				point->xform[i].c[1][1] = 1;
				point->xform[i].c[2][0] = 0;
				point->xform[i].c[2][1] = 0;
				point->xform[i].color = 0;
				point->xform[i].symmetry = 0;
				point->xform[i].vars[0] = 1;
				for(j=1; j<FRACTAL_NUM_VARS; j++)
				{
					point->xform[i].vars[j] = 0;
				}
				FractalXFormTranslate(&point->xform[i],
					FRACTAL_FLOAT_RAND() * 2 - 1, FRACTAL_FLOAT_RAND() * 2 - 1);
				FractalXFormRotate(&point->xform[i], FRACTAL_FLOAT_RAND() * 360.0);
				if(i > 0)
				{
					FractalXFormScale(&point->xform[i], FRACTAL_FLOAT_RAND() * 0.8 + 0.2);
				}
				else
				{
					FractalXFormScale(&point->xform[i], FRACTAL_FLOAT_RAND() * 0.4 + 0.6);
				}
				if((FRACTAL_RAND() & 1) == 0)
				{
					FractalXFormMultiply(&point->xform[i], 1,
						FRACTAL_FLOAT_RAND() - 0.5, FRACTAL_FLOAT_RAND() - 0.5, 1);
				}
			}
			FractalSetVariation(fractal, point);
		}
		else if(rnd <= 8)
		{
			FLOAT_T theta_sin, theta_cos;
			FLOAT_T phi_cos;
			for(i=0; i<fractal->transforms; i++)
			{
				r = FRACTAL_FLOAT_RAND() * 2 - 1;
				if(0 <= r && r < 0.2)
				{
					r += 0.2;
				}
				else if(r > -0.2 && r <= 0)
				{
					r -= 0.2;
				}

				s = FRACTAL_FLOAT_RAND() * 2 - 1;
				if(0 <= s && s < 0.2)
				{
					s += 0.2;
				}
				else if(s > -0.2 && s <= 0)
				{
					s -= 0.2;
				}

				theta = M_PI * FRACTAL_FLOAT_RAND();
				phi = (2 + FRACTAL_FLOAT_RAND()) * M_PI / 4;
				theta_sin = sin(theta),	theta_cos = cos(theta);
				phi_cos = cos(phi);
				point->xform[i].c[0][0] = r * theta_cos;
				point->xform[i].c[1][0] = s * (theta_cos * phi_cos - theta_sin);
				point->xform[i].c[0][1] = r * theta_sin;
				point->xform[i].c[1][1] = s * (theta_sin * phi_cos + theta_cos);
				point->xform[i].c[2][0] = FRACTAL_FLOAT_RAND() * 2 - 1;
				point->xform[i].c[2][1] = FRACTAL_FLOAT_RAND() * 2 - 1;
			}

			for(i=0; i<FRACTAL_NUM_XFORMS; i++)
			{
				point->xform[i].density = 0;
			}
			for(i=0; i<fractal->transforms; i++)
			{
				point->xform[i].density = 1.0 / fractal->transforms;
			}
			FractalSetVariation(fractal, point);
		}
		else
		{
			for(i=0; i<FRACTAL_NUM_XFORMS; i++)
			{
				point->xform[i].density = 0;
			}
			for(i=0; i<fractal->transforms; i++)
			{
				point->xform[i].density = 1.0 / fractal->transforms;
			}
		}

		FractalTrianglesFromControlPoint(fractal, point, triangles);
		v_rand = FRACTAL_RAND() & 1;
		if(v_rand > 0)
		{
			FractalPointComputeWeights(point, triangles, fractal->transforms);
		}
		else
		{
			FractalPointEqualizeWeights(point);
		}

		for(i=0; i<fractal->transforms; i++)
		{
			point->xform[i].color = (FLOAT_T)i / (fractal->transforms - 1);
		}
		if(point->xform[0].density == 1)
		{
			continue;
		}
		switch(fractal->symmetry_type)
		{
		case FRACTAL_SYMMETRY_TYPE_BILATERAL:
			(void)AddSymmetry2FractalPoint(point, -1);
			break;
		case FRACTAL_SYMMETRY_TYPE_ROTATIONAL:
			(void)AddSymmetry2FractalPoint(point, fractal->symmetry_order);
			break;
		case FRACTAL_SYMMETRY_TYPE_ROTATIONAL_AND_REFLECTIVE:
			(void)AddSymmetry2FractalPoint(point, - fractal->symmetry_order);
			break;
		}

		skip = 0;
		for(i=0; i<fractal->transforms; i++)
		{
			if(FractalTransformAffine(triangles[i], triangles) == 0)
			{
				skip = !(0);
			}
		}
		if(skip != 0)
		{
			continue;
		}
	} while(FractalBlowsUp(fractal, point, 5000) != FALSE || point->xform[0].density == 0);

	point->brightness = DEFAULT_BRIGHTNESS;
	point->gamma = DEFAULT_GAMMA;
	point->vibrancy = DEFAULT_VIBRANCY;
	point->sample_density = DEFAULT_SAMPLE_DENSITY;
	point->spatial_oversample = DEFAULT_OVERSAMPLE;
	point->spatial_filter_radius = DEFAULT_FILTER_RADIUS;
	point->color_map_index = fractal->color_map_index;
	if((fractal->flags & FRACTAL_FLAG_KEEP_BACK_GROUND) == 0)
	{
		point->back_bround[0] = 0;
		point->back_bround[1] = 0;
		point->back_bround[2] = 0;
	}

	if(fractal->random_gradient == 0)
	{
	}
	else
	{
		for(i=0; i<256; i++)
		{
			point->color_map[i][0] = fractal->color_map[i][0];
			point->color_map[i][1] = fractal->color_map[i][1];
			point->color_map[i][2] = fractal->color_map[i][2];
		}
	}
	point->zoom = 0;
}

FLOAT_T FractalDistance(
	FLOAT_T x1,
	FLOAT_T y1,
	FLOAT_T x2,
	FLOAT_T y2
)
{
	FLOAT_T d2;
	d2 = (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
	if(d2 == 0)
	{
		return 0;
	}

	return sqrt(d2);
}

static FLOAT_T LineDistance(
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T x1,
	FLOAT_T y1,
	FLOAT_T x2,
	FLOAT_T y2
)
{
	FLOAT_T a, b, e, c;
	FLOAT_T ret = 0;
	if(x == x1 && y == y1)
	{
		a = 0;
	}
	else
	{
		a = sqrt((x-x1)*(x-x1) + (y-y1)*(y-y1));
	}
	if(x == x2 && y == y2)
	{
		b = 0;
	}
	else
	{
		b = sqrt((x-x2)*(x-x2) + (y-y2)*(y-y2));
	}
	if(x1 == x2 && y1 == y2)
	{
		e = 0;
	}
	else
	{
		e = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
	}
	if((a * a + e * e) < (b * b))
	{
		ret = a;
	}
	else if((b * b + e * e) < (a * a))
	{
		ret = b;
	}
	else if(e != 0)
	{
		c = (b *b - a * a - e * e) / (-2 * e);
		if((a * a - c * c) > 0)
		{
			ret = sqrt(a * a - c * c);
		}
	}
	else
	{
		ret = a;
	}

	return ret;
}

FLOAT_T FractalTriangleArea(FLOAT_T (*triangle)[2])
{
	FLOAT_T base;
	FLOAT_T height;

	base = FractalDistance(triangle[0][0], triangle[0][1],
		triangle[1][0], triangle[1][1]);
	height = LineDistance(triangle[2][0], triangle[2][1],
		triangle[1][0], triangle[1][1], triangle[0][0], triangle[0][1]);
	if(base < 1)
	{
		return height;
	}
	else if(height < 1)
	{
		return base;
	}

	return 0.5 * base * height;
}

void FractalPointComputeWeights(
	FRACTAL_POINT* point,
	FLOAT_T (*triangles)[3][2],
	int t
)
{
	FLOAT_T total_area = 0;
	int i;

	for(i=0; i<t; i++)
	{
		point->xform[i].density = FractalTriangleArea(triangles[i+1]);
		total_area += point->xform[i].density;
	}
	for(i=0; i<t; i++)
	{
		point->xform[i].density /= total_area;
	}
	FractalPointNormalizeWeights(point);
}

int FractalTransformAffine(
	FLOAT_T (*triangle)[2],
	FLOAT_T (*triangles)[3][2]
)
{
	FLOAT_T ra, rb, rc;
	FLOAT_T a, b, c;
	int ret = !(0);

	ra = FractalDistance(triangles[0][0][1], triangles[0][0][0],
		triangles[0][1][1], triangles[0][1][0]);
	rb = FractalDistance(triangles[0][1][1], triangles[0][1][0],
		triangles[0][2][1], triangles[0][2][0]);
	rc = FractalDistance(triangles[0][2][1], triangles[0][2][0],
		triangles[0][0][1], triangles[0][0][0]);
	a = FractalDistance(triangle[0][1], triangle[0][0], triangle[1][1], triangle[1][0]);
	b = FractalDistance(triangle[1][1], triangle[1][0], triangle[2][1], triangle[2][0]);
	c = FractalDistance(triangle[2][1], triangle[2][0], triangle[0][1], triangle[0][0]);
	if(a > ra)
	{
		ret = 0;
	}
	else if(b > rb)
	{
		ret = 0;
	}
	else if(c > rc)
	{
		ret = 0;
	}
	else if(a == ra && b == rb && c == rc)
	{
		ret = 0;
	}

	return ret;
}

void InitializeFractalMutation(
	FRACTAL_MUTATION* mutation,
	FRACTAL* fractal,
	int num_mutants,
	int random_seed
)
{
	int i;
	(void)memset(mutation, 0, sizeof(*mutation));
	if(num_mutants < 1)
	{
		num_mutants = 1;
	}
	mutation->num_mutants = num_mutants;
	mutation->max_transforms = fractal->max_transforms;
	mutation->min_transforms = fractal->min_transforms;
	mutation->mutants = (FRACTAL*)MEM_ALLOC_FUNC(
		sizeof(*mutation->mutants) * num_mutants);
	(void)memset(mutation->mutants, 0, sizeof(*mutation->mutants) * num_mutants);
	mutation->control_points = (FRACTAL_POINT*)MEM_ALLOC_FUNC(
		sizeof(*mutation->control_points) * num_mutants);
	(void)memset(mutation->control_points, 0, sizeof(*mutation->control_points) * num_mutants);

	for(i=0; i<num_mutants; i++)
	{
		CopyFractal(&mutation->mutants[i], fractal);
	}
	for(i=0; i<num_mutants; i++)
	{
		CopyFractalPoint(&mutation->control_points[i], &fractal->control_point);
	}

	mutation->time_value = 25;
	mutation->random_seed = random_seed;
	FractalMutationRandomSet(mutation);
}

void ReleaseFractalMutation(FRACTAL_MUTATION* mutation)
{
	int i;
	for(i=0; i<mutation->num_mutants; i++)
	{
		ReleaseFractalPoint(&mutation->control_points[i]);
	}
	for(i=0; i<mutation->num_mutants; i++)
	{
		ReleaseFractal(&mutation->mutants[i]);
	}
}

void FractalMutationInterpolate(FRACTAL_MUTATION* mutation)
{
	int i, j, k;
	for(i=1; i<mutation->num_mutants; i++)
	{
		mutation->control_points[0].time_value = 0;
		mutation->control_points[i].time_value = 1;
		FractalPointClear(&mutation->mutants[i].control_point);
		FractalPointInterpolate(&mutation->mutants[i].control_point, &mutation->control_points[0],
			&mutation->control_points[i], mutation->time_value * 0.01);
		mutation->mutants[i].control_point.color_map_index = mutation->control_points[0].color_map_index;
		(void)memcpy(mutation->mutants[i].control_point.color_map,
			mutation->control_points[0].color_map, sizeof(mutation->control_points[i].color_map));
		(void)memcpy(mutation->mutants[i].control_point.back_bround,
			mutation->control_points[0].back_bround, sizeof(mutation->control_points[i].back_bround));
		if((mutation->flags & FRACTAL_MUTATION_FLAG_MAINTAIN_SYMMETRY) != 0)
		{
			for(j=0; j<mutation->mutants->transforms; j++)
			{
				if(mutation->control_points[0].xform[j].symmetry == 1)
				{
					mutation->mutants[i].control_point.xform[j].symmetry = 1;
					mutation->mutants[i].control_point.xform[j].color = mutation->control_points[0].xform[j].color;
					mutation->mutants[i].control_point.xform[j].density = mutation->control_points[0].xform[j].density;
					mutation->mutants[i].control_point.xform[j].c[0][0] = mutation->control_points[0].xform[j].c[0][0];
					mutation->mutants[i].control_point.xform[j].c[0][1] = mutation->control_points[0].xform[j].c[0][1];
					mutation->mutants[i].control_point.xform[j].c[1][0] = mutation->control_points[0].xform[j].c[1][0];
					mutation->mutants[i].control_point.xform[j].c[1][1] = mutation->control_points[0].xform[j].c[1][1];
					mutation->mutants[i].control_point.xform[j].c[2][0] = mutation->control_points[0].xform[j].c[2][0];
					mutation->mutants[i].control_point.xform[j].c[2][1] = mutation->control_points[0].xform[j].c[2][1];
					for(k=0; k<FRACTAL_NUM_VARS; k++)
					{
						mutation->mutants[i].control_point.xform[j].vars[k] = mutation->control_points[0].xform[j].vars[k];
					}
				}
			}
		}
	}
}

void FractalMutationRandomSet(FRACTAL_MUTATION* mutation)
{
	int i;

	FRACTAL_SRAND(mutation->random_seed);
	for(i=1; i<mutation->num_mutants; i++)
	{
		FractalPointClear(&mutation->control_points[i]);
		if((mutation->flags & FRACTAL_MUTATION_FLAG_SAME_NUM_TRANSFORM) != 0)
		{
			FractalPointRandomize(&mutation->control_points[i], mutation->mutants[0].transforms,
				mutation->mutants[0].transforms, FALSE, mutation->mutants[0].compatiblity);
		}
		else
		{
			FractalPointRandomize(&mutation->control_points[i], mutation->min_transforms,
				mutation->max_transforms, FALSE, mutation->mutants[0].compatiblity);
		}
		FractalPointSetVariation(&mutation->control_points[i], mutation->trend);
	}

	FractalMutationInterpolate(mutation);
}

#ifdef __cplusplus
}
#endif
