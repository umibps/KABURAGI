#include <string.h>
#include "layer.h"
#include "memory.h"
#include "draw_window.h"

#ifdef __cplusplus
extern "C" {
#endif

void BlendNormal_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_OVER);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
	/*
	int i, j, k;

	for(i=0; i<src->height && i+src->y<dst->height; i++)
	{
		for(j=0; j<src->width && j+src->x<dst->width; j++)
		{
			if(src->pixels[i*src->stride+j*src->channel+3] != 0)
			{
				if(src->pixels[i*src->stride+j*src->channel+3] == 0xff)
				{
					(void)memcpy(&dst->pixels[(i+src->y)*dst->stride+j*dst->channel],
						&src->pixels[i*src->stride+j*src->channel], dst->channel);
				}
				else
				{
					for(k=0; k<src->channel; k++)
					{
						dst->pixels[(i+src->y)*dst->stride+j*dst->channel+k] =
							(uint32)(((int)src->pixels[i*src->stride+j*src->channel+k]
							- (int)dst->pixels[(i+src->y)*dst->stride+(j+src->x)*dst->channel+k])
								* src->pixels[i*src->stride+j*src->channel+3] >> 8)
								+ dst->pixels[(i+src->y)*dst->stride+(j+src->x)*dst->channel+k];
					}
				}
			}
		}
	}
	*/
}

void BlendAdd_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_ADD);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendMultiply_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_MULTIPLY);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendScreen_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_SCREEN);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendOverLay_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_OVERLAY);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}
void BlendLighten_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_LIGHTEN);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendDarken_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_DARKEN);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendDodge_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_COLOR_DODGE);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendBurn_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_COLOR_BURN);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendHardLight_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_HARD_LIGHT);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendSoftLight_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_SOFT_LIGHT);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendDifference_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_DIFFERENCE);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendExclusion_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_EXCLUSION);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendHslHue_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_HSL_HUE);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendHslSaturation_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_HSL_SATURATION);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendHslColor_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_HSL_COLOR);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendHslLuminosity_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_HSL_LUMINOSITY);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendBinalize_c(LAYER* src, LAYER* dst)
{
	int i;

	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_OVER);

	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		for(i=0; i<src->width * src->height; i++)
		{
			if(src->window->mask_temp->pixels[i*4+3] >= 128)
			{
				src->window->mask_temp->pixels[i*4+0] =
					(src->pixels[i*4+0] >= 128) ? 255 : 0;
				src->window->mask_temp->pixels[i*4+1] =
					(src->pixels[i*4+1] >= 128) ? 255 : 0;
				src->window->mask_temp->pixels[i*4+2] =
					(src->pixels[i*4+2] >= 128) ? 255 : 0;
				src->window->mask_temp->pixels[i*4+3] = 255;
			}
			else
			{
				src->window->mask_temp->pixels[i*4+0] = 0;
				src->window->mask_temp->pixels[i*4+1] = 0;
				src->window->mask_temp->pixels[i*4+2] = 0;
				src->window->mask_temp->pixels[i*4+3] = 0;
			}
		}
		cairo_set_source_surface(src->window->mask->cairo_p, src->window->mask_temp->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		for(i=0; i<src->width * src->height; i++)
		{
			if(src->window->mask_temp->pixels[i*4+3] >= 128)
			{
				src->window->mask_temp->pixels[i*4+0] =
					(src->pixels[i*4+0] >= 128) ? 255 : 0;
				src->window->mask_temp->pixels[i*4+1] =
					(src->pixels[i*4+1] >= 128) ? 255 : 0;
				src->window->mask_temp->pixels[i*4+2] =
					(src->pixels[i*4+2] >= 128) ? 255 : 0;
				src->window->mask_temp->pixels[i*4+3] =
					(src->pixels[i*4+3] >= 128) ? 255 : 0;
			}
		}
		cairo_set_source_surface(dst->cairo_p, src->window->mask_temp->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendColorReverse_c(LAYER* src, LAYER* dst)
{
	int i, j;

	for(i=0; i<src->height && i+src->y<dst->height; i++)
	{
		for(j=0; j<src->width && j+src->x<dst->width; j++)
		{
			if(src->pixels[i*src->stride+j*src->channel+3] != 0)
			{
				if(src->pixels[i*src->stride+j*src->channel+3] >= 0x80)
				{
					dst->pixels[(i+src->y)*dst->stride+j*dst->channel] = 0xff - dst->pixels[(i+src->y)*dst->stride+j*dst->channel];
					dst->pixels[(i+src->y)*dst->stride+j*dst->channel+1] = 0xff - dst->pixels[(i+src->y)*dst->stride+j*dst->channel+1];
					dst->pixels[(i+src->y)*dst->stride+j*dst->channel+2] = 0xff - dst->pixels[(i+src->y)*dst->stride+j*dst->channel+2];
				}
			}
		}
	}
}

void BlendGrater_c(LAYER* src, LAYER* dst)
{
	int i, j, k;

	for(i=0; i<src->height && i+src->y<dst->height; i++)
	{
		for(j=0; j<src->width && j+src->x<dst->width; j++)
		{
			if(src->pixels[i*src->stride+j*src->channel+3] > dst->pixels[(i+src->y)*dst->stride+(j+src->x)*dst->channel+3])
			{
				for(k=0; k<src->channel; k++)
				{
					dst->pixels[(i+src->y)*dst->stride+j*dst->channel+k] =
						(uint32)(((int)src->pixels[i*src->stride+j*src->channel+k]
						- (int)dst->pixels[(i+src->y)*dst->stride+(j+src->x)*dst->channel+k])
							* src->pixels[i*src->stride+j*src->channel+3] >> 8)
							+ dst->pixels[(i+src->y)*dst->stride+(j+src->x)*dst->channel+k];
				}
			}
		}
	}
}

void BlendAlphaMinus_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_DEST_OUT);
	cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
	cairo_paint_with_alpha(dst->cairo_p, src->alpha * 0.01);
}

void BlendSource_c(LAYER* src, LAYER* dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_SOURCE);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(src->window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(src->window->mask->cairo_p, src->surface_p,
			src->x, src->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(src->window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(src->window->mask_temp->cairo_p,
				src->window->work_layer->surface_p, 0, 0);
			cairo_paint(src->window->mask_temp->cairo_p);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(dst->cairo_p, src->window->mask->surface_p, 0, 0);
		cairo_mask_surface(dst->cairo_p, mask_source->surface_p, 0, 0);
	}
	else
	{
		cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
		cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

void BlendAtop_c(LAYER* src, LAYER *dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_ATOP);
	cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
	cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
}

void BlendOver_c(LAYER* src, LAYER *dst)
{
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
	cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
}

void BlendSourceOver_c(LAYER* src, LAYER* dst)
{
	uint8 *pixel = src->pixels;
	uint8 *dst_pixel = dst->pixels;
	uint8 alpha;
	int i, j;

	for(i=0; i<src->height; i++, pixel+=4, dst_pixel+=4)
	{
		pixel = &src->pixels[i*src->stride];
		dst_pixel = &dst->pixels[i*dst->stride];
		for(j=0; j<src->width; j++, pixel+=4,dst_pixel+=4)
		{
			alpha = pixel[3];
			dst_pixel[0] = ((0xff-alpha+1)*dst_pixel[0] + alpha*pixel[0]) >> 8;
			dst_pixel[1] = ((0xff-alpha+1)*dst_pixel[1] + alpha*pixel[1]) >> 8;
			dst_pixel[2] = ((0xff-alpha+1)*dst_pixel[2] + alpha*pixel[2]) >> 8;
			dst_pixel[3] = ((0xff-alpha+1)*dst_pixel[3] + alpha*pixel[3]) >> 8;
			/*
		dst_pixel[0] = ((0xff-alpha+1)*dst_pixel[0]+alpha*pixel[0]) >> 8;
		dst_pixel[1] = ((0xff-alpha+1)*dst_pixel[1]+alpha*pixel[1]) >> 8;
		dst_pixel[2] = ((0xff-alpha+1)*dst_pixel[2]+alpha*pixel[2]) >> 8;
		dst_pixel[3] = ((0xff-alpha+1)*dst_pixel[3]+alpha*pixel[3]) >> 8;
		*/
		}
	}
	/*
	cairo_set_operator(dst->cairo_p, CAIRO_OPERATOR_OVER);
	(void)memset(dst->pixels, 0, dst->stride*dst->height);
	cairo_set_source_surface(dst->cairo_p, src->surface_p, src->x, src->y);
	cairo_paint_with_alpha(dst->cairo_p, src->alpha * (FLOAT_T)0.01);
	*/
}

void (*g_layer_blend_funcs[])(LAYER* src, LAYER* dst) =
{
	BlendNormal_c,
	BlendAdd_c,
	BlendMultiply_c,
	BlendScreen_c,
	BlendOverLay_c,
	BlendLighten_c,
	BlendDarken_c,
	BlendDodge_c,
	BlendBurn_c,
	BlendHardLight_c,
	BlendSoftLight_c,
	BlendDifference_c,
	BlendExclusion_c,
	BlendHslHue_c,
	BlendHslSaturation_c,
	BlendHslColor_c,
	BlendHslLuminosity_c,
	BlendBinalize_c,
	BlendColorReverse_c,
	BlendGrater_c,
	BlendAlphaMinus_c,
	BlendSource_c,
	BlendAtop_c,
	BlendSourceOver_c,
	//BlendOver_c
	BlendNormal_c
};

static void PartBlendNormal_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_OVER);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendAdd_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_ADD);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendMultiply_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_MULTIPLY);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendScreen_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_SCREEN);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendOverLay_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_OVERLAY);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendLighten_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_LIGHTEN);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendDarken_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_DARKEN);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendDodge_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_COLOR_DODGE);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendBurn_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_COLOR_BURN);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendHardLight_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_HARD_LIGHT);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendSoftLight_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_SOFT_LIGHT);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendDifference_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_DIFFERENCE);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendExclusion_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_EXCLUSION);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendHslHue_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_HSL_HUE);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendHslSaturation_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_HSL_SATURATION);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendHslColor_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_HSL_COLOR);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendHslLuminosity_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_HSL_LUMINOSITY);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
			src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
		cairo_t *update_temp = cairo_create(temp_surface);

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, temp_surface, 0, 0);

		cairo_surface_destroy(temp_surface);
		cairo_destroy(update_temp);
		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendBinalize_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	int start_x, start_y;
	int width, height;
	int i, j;

	start_x = (int)update->x;
	if(start_x < 0)
	{
		start_x = 0;
	}
	else if(start_x >= src->width)
	{
		return;
	}
	width = (int)(update->x + update->width + 1);
	if(width > src->width)
	{
		width = src->width;
	}
	else if(width <= 0)
	{
		return;
	}
	width = width - start_x;

	start_y = (int)update->y;
	if(start_y < 0)
	{
		start_y = 0;
	}
	else if(start_y >= src->height)
	{
		return;
	}
	height = (int)(update->y + update->height + 1);
	if(height > src->height)
	{
		height = src->height;
	}
	else if(height <= 0)
	{
		return;
	}
	height = height - start_y;

	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_OVER);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;

		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
			src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
		cairo_t *update_temp = cairo_create(temp_surface);

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		for(i=0; i<height; i++)
		{
			for(j=0; j<width; j++)
			{
				if(src->pixels[(start_y+i)*src->stride+(start_x+j)*4+3] >= 128)
				{
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+0]
						= (src->pixels[(start_y+i)*src->stride+(start_x+j)*4+0] >= 128) ? 255 : 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+1]
						= (src->pixels[(start_y+i)*src->stride+(start_x+j)*4+1] >= 128) ? 255 : 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+2]
						= (src->pixels[(start_y+i)*src->stride+(start_x+j)*4+2] >= 128) ? 255 : 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+3] = 255;
				}
				else
				{
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+0] = 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+1] = 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+2] = 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+3] = 0;
				}
			}
		}

		cairo_set_source_surface(update_cairo, src->window->mask_temp->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(src->window->mask->cairo_p, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_source = src->window->mask_temp;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, temp_surface, 0, 0);

		cairo_surface_destroy(temp_surface);
		cairo_destroy(update_temp);
		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		for(i=0; i<height; i++)
		{
			for(j=0; j<width; j++)
			{
				if(src->pixels[(start_y+i)*src->stride+(start_x+j)*4+3] >= 128)
				{
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+0]
						= (src->pixels[(start_y+i)*src->stride+(start_x+j)*4+0] >= 128) ? 255 : 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+1]
						= (src->pixels[(start_y+i)*src->stride+(start_x+j)*4+1] >= 128) ? 255 : 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+2]
						= (src->pixels[(start_y+i)*src->stride+(start_x+j)*4+2] >= 128) ? 255 : 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+3] = 255;
				}
				else
				{
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+0] = 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+1] = 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+2] = 0;
					src->window->mask_temp->pixels[(start_y+i)*src->stride+(start_x+j)*4+3] = 0;
				}
			}
		}
		cairo_set_source_surface(update->cairo_p, src->window->mask_temp->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void DummyPartBlend(LAYER* src, UPDATE_RECTANGLE* update)
{
}

#define PartBlendColorReverse_c DummyPartBlend
#define PartBlendGrater_c DummyPartBlend

static void PartBlendAlphaMinus_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_DEST_OUT);
	cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
	cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
}

static void PartBlendSource_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_SOURCE);
	if((src->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
	{
		LAYER* mask_source = src->prev;
		cairo_surface_t *update_surface = cairo_surface_create_for_rectangle(
			src->window->mask->surface_p, update->x, update->y, update->width, update->height);
		cairo_surface_t *temp_surface = NULL, *mask_surface = update_surface;
		cairo_t *update_cairo = cairo_create(update_surface);
		cairo_t *update_temp;

		(void)memset(src->window->mask->pixels, 0, src->stride*src->height);
		cairo_set_operator(update_cairo, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(update_cairo, src->surface_p,
			- update->x, - update->y);
		cairo_paint_with_alpha(update_cairo, src->alpha * (FLOAT_T)0.01);

		while((mask_source->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0)
		{
			mask_source = mask_source->prev;
		}
		if(mask_source == src->window->active_layer)
		{
			mask_surface = temp_surface = cairo_surface_create_for_rectangle(
				src->window->mask_temp->surface_p, update->x, update->y, update->width, update->height);
			update_temp = cairo_create(temp_surface);
			cairo_set_operator(update_temp, CAIRO_OPERATOR_OVER);
			(void)memcpy(src->window->mask_temp->pixels, mask_source->pixels,
				mask_source->stride*mask_source->height);
			cairo_set_source_surface(update_temp,
				src->window->work_layer->surface_p, - update->x, - update->y);
			cairo_paint(update_temp);
			mask_surface = temp_surface;
		}

		cairo_set_source_surface(update->cairo_p, update_surface, 0, 0);
		cairo_mask_surface(update->cairo_p, mask_source->surface_p, - update->x, - update->y);

		if(temp_surface != NULL)
		{
			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update_cairo);
	}
	else
	{
		cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
		cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
	}
}

static void PartBlendAtop_c(LAYER* src, UPDATE_RECTANGLE* update)
{
	cairo_set_operator(update->cairo_p, CAIRO_OPERATOR_ATOP);
	cairo_set_source_surface(update->cairo_p, src->surface_p, - update->x, - update->y);
	cairo_paint_with_alpha(update->cairo_p, src->alpha * (FLOAT_T)0.01);
}

#define PartBlendSourceOver_c DummyPartBlend

void (*g_part_layer_blend_funcs[])(LAYER* src, UPDATE_RECTANGLE* update) =
{
	PartBlendNormal_c,
	PartBlendAdd_c,
	PartBlendMultiply_c,
	PartBlendScreen_c,
	PartBlendOverLay_c,
	PartBlendLighten_c,
	PartBlendDarken_c,
	PartBlendDodge_c,
	PartBlendBurn_c,
	PartBlendHardLight_c,
	PartBlendSoftLight_c,
	PartBlendDifference_c,
	PartBlendExclusion_c,
	PartBlendHslHue_c,
	PartBlendHslSaturation_c,
	PartBlendHslColor_c,
	PartBlendHslLuminosity_c,
	PartBlendBinalize_c,
	PartBlendColorReverse_c,
	PartBlendGrater_c,
	PartBlendAlphaMinus_c,
	PartBlendSource_c,
	PartBlendAtop_c,
	PartBlendSourceOver_c,
	//BlendOver_c
	PartBlendNormal_c
};

#ifdef __cplusplus
}
#endif
