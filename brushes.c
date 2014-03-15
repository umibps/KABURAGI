// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "application.h"
#include "brushes.h"
#include "ini_file.h"
#include "utils.h"
#include "display.h"
#include "memory.h"
#include "anti_alias.h"
#include "script.h"

#ifdef __cplusplus
extern "C" {
#endif

static void DummyBrushCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
}

static void DummyBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
}

static void DefaultReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		AddBrushHistory(core, window->active_layer);

		g_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, window->active_layer);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

static void DefaultEditSelectionReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[window->selection->layer_mode](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		window->selection->layer_mode = SELECTION_BLEND_NORMAL;
	}
}

/***********************************************************
* PencilPressCallBack�֐�                                  *
* ���M�c�[���g�p���̃}�E�X�N���b�N�ɑ΂���R�[���o�b�N�֐� *
* ����                                                     *
* window	: �`��̈�̏��                               *
* x			: X���W                                        *
* y			: Y���W                                        *
* pressure	: �M��                                         *
* core		: �u���V�̏��                                 *
* state		: �}�E�X�̏��                                 *
***********************************************************/
static void PencilPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ��
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		PENCIL* pen = (PENCIL*)core->brush_data;
		// �`��͈͂̃C���[�W���
		cairo_t *update;
		cairo_surface_t *update_surface;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// �u���V�̍��W�w��p
		cairo_matrix_t matrix;
		// �M���ɂ��u���V�k���p
		gdouble zoom;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �s�N�Z���f�[�^���Z�b�g���̍�����1�s���̃o�C�g��
		int height, stride;
		// �s�������ی쎞�̃}�X�N�p
		uint8 *mask;
		int i;	// for���p�̃J�E���^

		// ��ƃ��C���[�̍������@��ݒ�
		window->work_layer->layer_mode = pen->blend_mode;

		// �M���ŃT�C�Y�ύX���邩�t���O�����Ă���u���V�̔��a����
		if((pen->flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = pen->r;
			zoom = 1;
		}
		else
		{
			r = pen->r * pressure;
			zoom = 1/pressure;
		}
		// �M���ŕs�����x�ύX���邩�t���O�����Ă���s�����x����
		alpha = ((pen->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;

		// �}�E�X�̍��W�ƃu���V�̔��a����
			// �`�悷����W�̍ő�E�ŏ��l������
		min_x = x - r, min_y = y - r;
		max_x = x + r + 1, max_y = y + r + 1;

		// �X�V�͈͂̃T�[�t�F�[�X�쐬
		update_surface = cairo_surface_create_for_rectangle(
			window->work_layer->surface_p, min_x, min_y, r*2+1, r*2+1);
		update = cairo_create(update_surface);

		// ����̍��W���L��
		pen->before_x = x, pen->before_y = y;

		// �`��̈悩��͂ݏo����C��
		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		window->update.x = start_x = (int)min_x;
		window->update.y = start_y = (int)min_y;
		window->update.width = (int)max_x - window->update.x;
		stride = (int)window->update.width * 4;
		window->update.height = (int)max_y - window->update.y;
		height = (int)window->update.height;

		// ���݂̍��W�̍ő�E�ŏ��l���L�����Ă���
		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		// ��ƃ��C���[�̕`����e�̍������@��ʏ�ɂ��Ă���
		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);

		mask = window->mask_temp->pixels;
		if(window->app->textures.active_texture == 0)
		{
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(update, core->brush_pattern);
					cairo_paint_with_alpha(update, alpha);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0,0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);
					cairo_mask_surface(update,
						window->active_layer->surface_p, - x + r, - y + r);
				}
			}
			else
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);
					cairo_mask_surface(window->mask_temp->cairo_p,
						window->selection->surface_p, - x + r, - y + r);
				}
				else
				{
					cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
						window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
					cairo_t *update_temp = cairo_create(temp_surface);

					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);

					for(i=0; i<height; i++)
					{
						(void)memset(&window->temp_layer->pixels[
							(i+start_y)*window->temp_layer->stride+start_x*4],
							0, stride
						);
					}

					cairo_mask_surface(update,
						window->selection->surface_p, - x + r, - y + r);
					cairo_set_source_surface(update_temp,
						update_surface, 0, 0);
					cairo_mask_surface(update_temp,
						window->active_layer->surface_p, - x + r, - y + r);

					mask = window->temp_layer->pixels;

					cairo_surface_destroy(temp_surface);
					cairo_destroy(update_temp);
				}
			}
		}
		else
		{
			cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
				window->temp_layer->surface_p, min_x, min_y, r*2+1, r*2+1);
			cairo_t *update_temp = cairo_create(temp_surface);

			for(i=0; i<height; i++)
			{
				(void)memset(&window->temp_layer->pixels[
					(i+start_y)*window->temp_layer->stride+start_x*4],
					0, stride
				);
			}

			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(update_temp, core->brush_pattern);
					cairo_paint_with_alpha(update_temp, alpha);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update_temp, core->temp_pattern);
					cairo_mask_surface(update_temp,
						window->active_layer->surface_p, - x + r, - y + r);
				}

				cairo_set_source_surface(update, temp_surface, 0, 0);
				cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);

				mask = window->temp_layer->pixels;
			}
			else
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);
					cairo_mask_surface(update,
						window->selection->surface_p, - x + r, - y + r);

					for(i=0; i<height; i++)
					{
						(void)memset(&window->temp_layer->pixels[
							(i+start_y)*window->temp_layer->stride+start_x*4],
							0, stride
						);
					}

					cairo_set_source_surface(update_temp, update_surface, 0, 0);
					cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);

					for(i=0; i<height; i++)
					{
						(void)memset(&window->temp_layer->pixels[
							(i+start_y)*window->temp_layer->stride+start_x*4],
							0, stride
						);
					}

					cairo_mask_surface(update,
						window->selection->surface_p, - x + r, - y + r);
					cairo_set_source_surface(update_temp,
						update_surface, 0, 0);
					cairo_mask_surface(update_temp,
						window->active_layer->surface_p, - x + r, - y + r);

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[
							(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride
						);
					}

					cairo_set_source_surface(update, temp_surface, 0, 0);
					cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
				}
			}

			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		cairo_surface_destroy(update_surface);
		cairo_destroy(update);


		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}
}

#define PencilEditSelectionPressCallBack PencilPressCallBack

static void PencilMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		PENCIL* pen = (PENCIL*)core->brush_data;
		cairo_matrix_t matrix;
		// �`��͈͂̃C���[�W���
		cairo_t *update;
		cairo_surface_t *update_surface;
		gdouble r, step, alpha, d;
		gdouble min_x, min_y, max_x, max_y;
		gdouble draw_x = pen->before_x, draw_y = pen->before_y;
		gdouble dx, dy, diff_x, diff_y;
		gdouble zoom;
		int start_x, width, start_y, height;
		int stride;
		uint8* color = *core->color;
		uint8 *mask;
		uint8 *ref_pix, *mask_pix;
		int i, j;

		if((pen->flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = pen->r;
			zoom = 1;
		}
		else
		{
			r = pen->r * pressure;
			zoom = 1/pressure;
		}
		step = r * BRUSH_STEP;
		if(step < MIN_BRUSH_STEP)
		{
			dx = 0;
			goto skip_draw;
		}
		alpha = ((pen->flags & BRUSH_FLAG_FLOW) == 0) ?
			1 : pressure;
		dx = x-pen->before_x, dy = y-pen->before_y;
		d = sqrt(dx*dx+dy*dy);
		diff_x = step * dx/d, diff_y = step * dy/d;

		min_x = x - r*2, min_y = y - r*2;
		max_x = x + r*2, max_y = y + r*2;

		if((pen->flags & BRUSH_FLAG_ANTI_ALIAS) != 0)
		{
			max_x++;
			max_y++;
		}

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		if(window->update.x > min_x)
		{
			window->update.width += window->update.x - (int)min_x;
			window->update.x = (int)min_x;
		}
		if(window->update.width + window->update.x < max_x)
		{
			window->update.width += (int)max_x - window->update.width + window->update.x;
		}
		if(window->update.y > min_y)
		{
			window->update.height += window->update.y - (int)min_y;
			window->update.y = (int)min_y;
		}
		if(window->update.height + window->update.y < max_y)
		{
			window->update.height += (int)max_y - window->update.height + window->update.y;
		}

		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		dx = d;
		do
		{
			start_x = (int)(draw_x - r);
			start_y = (int)(draw_y - r);
			width = (int)(draw_x + r + 1);
			height = (int)(draw_y + r + 1);

			if(start_x < 0)
			{
				start_x = 0;
			}
			else if(start_x > window->work_layer->width)
			{
				goto skip_draw;
			}
			if(start_y < 0)
			{
				start_y = 0;
			}
			else if(start_y > window->work_layer->height)
			{
				goto skip_draw;
			}
			if(width > window->work_layer->width)
			{
				width = window->work_layer->width - start_x;
			}
			else
			{
				width = width - start_x;
			}
			if(height > window->work_layer->height)
			{
				height = window->work_layer->height - start_y;
			}
			else
			{
				height = height - start_y;
			}
			stride = width*4;

			if(width <= 0 || height <= 0)
			{
				goto skip_draw;
			}

			update_surface = cairo_surface_create_for_rectangle(
				window->mask_temp->surface_p, draw_x - r, draw_y - r,
					r*2+2, r*2+2);
			update = cairo_create(update_surface);

			window->flags |= DRAW_WINDOW_UPDATE_PART;

			for(i=0; i<height; i++)
			{
				(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
					0, stride);
			}

			cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);

			mask = window->mask_temp->pixels;
			if(window->app->textures.active_texture == 0)
			{
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - draw_x + r, - draw_y + r);
					}
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - draw_x + r, - draw_y + r);
					}
					else
					{
						cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
						cairo_t *update_temp = cairo_create(temp_surface);

						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_mask_surface(update,
							window->selection->surface_p, - draw_x + r, - draw_y + r);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - draw_x + r, - draw_y + r);

						mask = window->temp_layer->pixels;

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}
				}
			}
			else
			{
				cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
					window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
				cairo_t *update_temp = cairo_create(temp_surface);

				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - draw_x + r, - draw_y + r);
					}

					for(i=0; i<height; i++)
					{
						(void)memset(&window->temp_layer->pixels[
							(i+start_y)*window->temp_layer->stride+start_x*4],
							0, stride
						);
					}

					cairo_set_source_surface(update_temp, update_surface, 0, 0);
					cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

					mask = window->temp_layer->pixels;
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - draw_x + r, - draw_y + r);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_set_source_surface(update_temp, update_surface, 0, 0);
						cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

						mask = window->temp_layer->pixels;
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_mask_surface(update,
							window->selection->surface_p, - draw_x + r, - draw_y + r);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - draw_x + r, - draw_y + r);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->mask_temp->pixels[
								(i+start_y)*window->mask_temp->stride+start_x*4],
								0, stride
							);
						}

						cairo_set_source_surface(update, temp_surface, 0, 0);
						cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
					}
				}

				cairo_surface_destroy(temp_surface);
				cairo_destroy(update_temp);
			}

			cairo_surface_destroy(update_surface);
			cairo_destroy(update);

			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[
					(start_y+i)*window->work_layer->stride+start_x*4];
				mask_pix = &mask[(start_y+i)*window->work_layer->stride
					+start_x*4];

				for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
				{
					if(ref_pix[3] < mask_pix[3])
					{
						ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
							* mask_pix[3] >> 8) + ref_pix[0]);
						ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
							* mask_pix[3] >> 8) + ref_pix[1]);
						ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
							* mask_pix[3] >> 8) + ref_pix[2]);
						ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
							* mask_pix[3] >> 8) + ref_pix[3]);
					}
				}
			}

			// �A���`�G�C���A�X��K�p
			if((pen->flags & BRUSH_FLAG_ANTI_ALIAS) != 0)
			{
				ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
					width + 3, height + 3};
				AntiAliasLayer(window->work_layer, window->temp_layer, &range);
			}
skip_draw:
			dx -= step;
			if(dx < 1)
			{
				break;
			}
			else if(dx >= step)
			{
				draw_x += diff_x, draw_y += diff_y;
			}
			else
			{
				draw_x = x;
				draw_y = y;
			}
		} while(1);

		pen->before_x = x, pen->before_y = y;
		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}
}

#define PencilEditSelectionMotionCallBack PencilMotionCallBack

static void PencilReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		AddBrushHistory(core, window->active_layer);

		window->update.surface_p = cairo_surface_create_for_rectangle(
			window->active_layer->surface_p, window->update.x, window->update.y,
				window->update.width, window->update.height);
		window->update.cairo_p = cairo_create(window->update.surface_p);

		g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->update);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		cairo_surface_destroy(window->update.surface_p);
		cairo_destroy(window->update.cairo_p);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

#define PencilEditSelectionReleaseCallBack DefaultEditSelectionReleaseCallBack


static void PencilDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	PENCIL* pen = (PENCIL*)data;
	gdouble r = pen->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void PencilButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, PENCIL* pen)
{
	gdouble r = pen->r * window->zoom_rate + 3;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2 + 3), (gint)(r * 2 + 3));
}

static void PencilMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, PENCIL* pen)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = pen->r * window->zoom_rate  * 1.275 + BRUSH_UPDATE_MARGIN;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width + 2, (gint)height + 2);
}

static void PencilColorChange(const uint8 color[3], void* data)
{
	PENCIL *pen = (PENCIL*)data;
	BrushCoreSetCirclePattern(pen->core, pen->r, pen->outline_hardness * 0.01,
		0.5, pen->alpha * 0.01, color);
}

static void PencilScaleChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	PENCIL* pen = (PENCIL*)data;
	pen->r = gtk_adjustment_get_value(slider) * 0.5;
	BrushCoreSetCirclePattern(pen->core, pen->r, pen->outline_hardness * 0.01,
		0.5, pen->alpha * 0.01, *pen->core->color);
}

static void PencilFlowChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	PENCIL* pen = (PENCIL*)data;
	pen->alpha = gtk_adjustment_get_value(slider);
	BrushCoreSetCirclePattern(pen->core, pen->r, pen->outline_hardness * 0.01,
		0.5, pen->alpha * 0.01, *pen->core->color);
}

static void PencilPressureSizeChange(
	GtkWidget* widget,
	gpointer data
)
{
	PENCIL* pen = (PENCIL*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		pen->flags &= ~(BRUSH_FLAG_SIZE);
	}
	else
	{
		pen->flags |= BRUSH_FLAG_SIZE;
	}
}

static void PencilPressureFlowChange(
	GtkWidget* widget,
	gpointer data
)
{
	PENCIL* pen = (PENCIL*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		pen->flags &= ~(BRUSH_FLAG_FLOW);
	}
	else
	{
		pen->flags |= BRUSH_FLAG_FLOW;
	}
}

static void PencilSetAntiAlias(
	GtkWidget* widget,
	gpointer data
)
{
	PENCIL* pen = (PENCIL*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		pen->flags &= ~(BRUSH_FLAG_ANTI_ALIAS);
	}
	else
	{
		pen->flags |= BRUSH_FLAG_ANTI_ALIAS;
	}
}

static void PencilOutlineHardnessChange(
	GtkAdjustment* slider,
	PENCIL* pen
)
{
	pen->outline_hardness =
		gtk_adjustment_get_value(slider);
	BrushCoreSetCirclePattern(pen->core, pen->r, pen->outline_hardness * 0.01,
		0.5, pen->alpha * 0.01, *pen->core->color);
}

static void PencilSetBlendMode(GtkComboBox* combo, PENCIL* pen)
{
	pen->blend_mode = (uint16)gtk_combo_box_get_active(combo);
}

static GtkWidget* CreatePencilDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	PENCIL* pen = (PENCIL*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* hbox;
	GtkWidget* brush_scale;
	GtkWidget* table = gtk_table_new(1, 3, TRUE);
	GtkWidget* check_button;
	GtkWidget* label;
	GtkWidget *combo;
	GtkWidget* base_scale;
	GtkAdjustment* brush_scale_adjustment;
	char mark_up_buff[256];
	int i;

	pen->core = core;

	switch(pen->base_scale)
	{
	case 0:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(pen->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(pen->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(pen->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
		break;
	}

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

#if MAJOR_VERSION == 1
		base_scale = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[2]);
#else
		base_scale = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), pen->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(PencilScaleChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	g_object_set_data(G_OBJECT(base_scale), "scale", brush_scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &pen->base_scale);

	brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(pen->alpha, 0.0, 100.0, 1.0, 1.0, 0.0));
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.flow, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(PencilFlowChange), core->brush_data);
	table = gtk_table_new(1, 3, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	table = gtk_hbox_new(FALSE, 0);
	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(pen->outline_hardness,
		0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(PencilOutlineHardnessChange), core->brush_data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.outline_hardness, 1);
	gtk_box_pack_start(GTK_BOX(table), brush_scale, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
#if MAJOR_VERSION == 1
	combo = gtk_combo_box_new_text();
#else
	combo = gtk_combo_box_text_new();
#endif
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->layer_window.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
#if MAJOR_VERSION == 1
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), app->labels->layer_window.blend_modes[i]);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), app->labels->layer_window.blend_modes[i]);
#endif
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), pen->blend_mode);
	(void)g_signal_connect(G_OBJECT(combo), "changed",
		G_CALLBACK(PencilSetBlendMode), pen);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(PencilPressureSizeChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), pen->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(PencilPressureFlowChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), pen->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(PencilSetAntiAlias), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), pen->flags & BRUSH_FLAG_ANTI_ALIAS);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	BrushCoreSetCirclePattern(core, pen->r, pen->outline_hardness * 0.01,
		0.5, pen->alpha * 0.01, *core->color);

	return vbox;
}

/***********************************************************
* HardPenButtonPressCallBack�֐�                           *
* �d�M�y���g�p���̃}�E�X�N���b�N�ɑ΂���R�[���o�b�N�֐�   *
* ����                                                     *
* window	: �`��̈�̏��                               *
* x			: X���W                                        *
* y			: Y���W                                        *
* pressure	: �M��                                         *
* core		: �u���V�̏��                                 *
* state		: �}�E�X�̏��                                 *
***********************************************************/
static void HardPenButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ��
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		HARD_PEN* pen = (HARD_PEN*)core->brush_data;
		// �����`��p��Cairo�R���e�L�X�g
		cairo_t *update, *temp_update;
		// �����`��p�̃T�[�t�F�[�X
		cairo_surface_t *update_surface, *temp_surface;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���a1�ȉ��ɂȂ����ۂ̕s�����x
		gdouble alpha_rate = 1;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// �`��p�p�^�[��
		cairo_pattern_t *pattern;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �s�N�Z���f�[�^���Z�b�g���̕��E������1�s���̃o�C�g��
		int width, height, stride;
		// �s�������ی쎞�̃}�X�N�p
		uint8 *mask;
		// ��ƃ��C���[�ւ̕`�挋�ʓ]�ʗp
		uint8 *mask_pix, *ref;
		int i, j;	// for���p�̃J�E���^

		// ��ƃ��C���[�̍������@��ݒ�
		window->work_layer->layer_mode = pen->blend_mode;

		// �M���ŃT�C�Y�ύX���邩�t���O�����Ă���u���V�̔��a����
		if((pen->flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = pen->r;
		}
		else
		{
			r = pen->r * pressure;
		}
		// �M���ŕs�����x�ύX���邩�t���O�����Ă���s�����x����
		alpha = ((pen->flags & BRUSH_FLAG_FLOW) == 0) ? pen->alpha * 0.01 : pen->alpha * 0.01 * pressure;

		if(r < 1)
		{
			alpha_rate = r;
			alpha *= r;
			r = 1;
		}

		// �}�E�X�̍��W�ƃu���V�̔��a����
			// �`�悷����W�̍ő�E�ŏ��l������
		min_x = x - r, min_y = y - r;
		max_x = x + r + 1, max_y = y + r + 1;

		// Cairo�̃R���e�L�X�g�����L��
		cairo_save(window->temp_layer->cairo_p);
		cairo_save(window->mask_temp->cairo_p);

		// ����̍��W���L��
		pen->before_x = x, pen->before_y = y;

		// �`��̈悩��͂ݏo����C��
		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		if(min_x > window->width || min_y > window->height
			|| max_x < 0 || max_y < 0)
		{
			goto func_end;
		}

		window->update.x = start_x = (int)min_x;
		window->update.y = start_y = (int)min_y;
		width = (int)(window->update.width = (int)max_x - window->update.x);
		stride = (int)window->update.width * 4;
		window->update.height = (int)max_y - window->update.y;
		height = (int)window->update.height;

		// ���݂̍��W�̍ő�E�ŏ��l���L�����Ă���
		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		// ��ƃ��C���[�̕`����e�̍������@��ʏ�ɂ��Ă���
		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);

		// �`��G���A��0������
		for(i=0; i<height; i++)
		{
			(void)memset(&window->temp_layer->pixels[(start_y+i)*window->temp_layer->stride + start_x*4],
				0, stride);
		}
		for(i=0; i<height; i++)
		{
			(void)memset(&window->mask_temp->pixels[(start_y+i)*window->mask_temp->stride + start_x*4],
				0, stride);
		}

		// �`��p�~�`�p�^�[���쐬
		pattern = cairo_pattern_create_radial(r, r, 0,
			r, r, r);
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		cairo_pattern_add_color_stop_rgba(pattern, 0, (*core->color)[2]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL,
			(*core->color)[0]*DIV_PIXEL, alpha);
		cairo_pattern_add_color_stop_rgba(pattern, 1, (*core->color)[2]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL,
			(*core->color)[0]*DIV_PIXEL, alpha);
#else
		cairo_pattern_add_color_stop_rgba(pattern, 0, (*core->color)[0]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL,
			(*core->color)[2]*DIV_PIXEL, alpha);
		cairo_pattern_add_color_stop_rgba(pattern, 1, (*core->color)[0]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL,
			(*core->color)[2]*DIV_PIXEL, alpha);
#endif

		update_surface = cairo_surface_create_for_rectangle(window->mask_temp->surface_p,
			x - r, y - r, r * 2, r * 2);
		update = cairo_create(update_surface);
		temp_surface = cairo_surface_create_for_rectangle(window->temp_layer->surface_p,
			x - r, y - r, r * 2, r * 2);
		temp_update = cairo_create(temp_surface);

		mask = window->mask_temp->pixels;
		if(window->app->textures.active_texture == 0)
		{
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_set_source(update, pattern);
					cairo_paint(update);
				}
				else
				{
					cairo_set_source(update, pattern);
					cairo_mask_surface(update,
						window->active_layer->surface_p, - x + r, - y + r);
				}
			}
			else
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_set_source(update, pattern);
					cairo_mask_surface(update,
						window->selection->surface_p, - x + r, - y + r);
				}
				else
				{
					cairo_set_source(temp_update, pattern);
					cairo_mask_surface(temp_update,
						window->selection->surface_p, - x + r, - y + r);
					cairo_set_source_surface(update, temp_surface, 0, 0);
					cairo_mask_surface(update, window->active_layer->surface_p,
						- x + r, - y + r);
				}
			}
		}
		else
		{
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_set_source(update, core->brush_pattern);
					cairo_mask_surface(update, window->texture->surface_p,
						- x + r, - y + r);
				}
				else
				{
					cairo_set_source(temp_update, pattern);
					cairo_mask_surface(temp_update, window->active_layer->surface_p,
						- x + r, - y + r);
					cairo_set_source_surface(update, temp_surface,
						0, 0);
					cairo_mask_surface(update,
						window->texture->surface_p, - x + r, - y + r);
				}
			}
			else
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_set_source(temp_update, pattern);
					cairo_mask_surface(temp_update,
						window->selection->surface_p, - x + r, - y + r);

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[
							(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride
						);
					}

					cairo_set_source_surface(update, temp_surface, 0, 0);
					cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
				}
				else
				{
					cairo_set_source(update, pattern);
					cairo_mask_surface(update, window->selection->surface_p,
						- x + r, - y + r);
					cairo_set_source_surface(temp_update, update_surface, 0, 0);

					for(i=0; i<height; i++)
					{
						(void)memset(&window->temp_layer->pixels[
							(i+start_y)*window->temp_layer->stride+start_x*4],
							0, stride
						);
					}

					cairo_mask_surface(temp_update,
						window->active_layer->surface_p, - x + r, - y + r);

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[
							(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride
						);
					}

					cairo_set_source_surface(update, temp_surface, 0, 0);
					cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
				}
			}
		}

		cairo_destroy(update);
		cairo_surface_destroy(update_surface);
		cairo_destroy(temp_update);
		cairo_surface_destroy(temp_surface);

		if((pen->flags & BRUSH_FLAG_ANTI_ALIAS) == 0)
		{
			uint8 threshold;
			uint8 set_color[4];

			threshold = (uint8)(alpha * alpha_rate * 255 * 0.5);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			set_color[0] = (uint8)((*core->color)[2] * alpha);
			set_color[1] = (uint8)((*core->color)[1] * alpha);
			set_color[2] = (uint8)((*core->color)[0] * alpha);
#else
			set_color[0] = (uint8)((*core->color)[0] * alpha);
			set_color[1] = (uint8)((*core->color)[1] * alpha);
			set_color[2] = (uint8)((*core->color)[2] * alpha);
#endif
			set_color[3] = (uint8)(alpha * 255 + 0.3);
			for(i=0; i<height; i++)
			{
				for(j=0; j<width; j++)
				{
					if(mask[(start_y+i)*window->stride+(start_x+j)*4+3] > threshold)
					{
						mask[(start_y+i)*window->stride+(start_x+j)*4] = set_color[0];
						mask[(start_y+i)*window->stride+(start_x+j)*4+1] = set_color[1];
						mask[(start_y+i)*window->stride+(start_x+j)*4+2] = set_color[2];
						mask[(start_y+i)*window->stride+(start_x+j)*4+3] = set_color[3];
					}
					else
					{
						mask[(start_y+i)*window->stride+(start_x+j)*4] = 0;
						mask[(start_y+i)*window->stride+(start_x+j)*4+1] = 0;
						mask[(start_y+i)*window->stride+(start_x+j)*4+2] = 0;
						mask[(start_y+i)*window->stride+(start_x+j)*4+3] = 0;
					}
				}
			}
		}

		for(i=0; i<height; i++)
		{
			mask_pix = &mask[(start_y+i)*window->stride+start_x*4];
			ref = &window->work_layer->pixels[(start_y+i)*window->stride+start_x*4];
			for(j=0; j<width; mask_pix += 4, ref += 4, j++)
			{
				if(mask_pix[3] >= ref[3])
				{
					ref[0] = (uint8)(((int)mask_pix[0] - (int)ref[0]) * mask_pix[3] >> 8) + ref[0];
					ref[1] = (uint8)(((int)mask_pix[1] - (int)ref[1]) * mask_pix[3] >> 8) + ref[1];
					ref[2] = (uint8)(((int)mask_pix[2] - (int)ref[2]) * mask_pix[3] >> 8) + ref[2];
					ref[3] = (uint8)(((int)mask_pix[3] - (int)ref[3]) * mask_pix[3] >> 8) + ref[3];
				}
			}
		}
func_end:
		pen->before_x = x,	pen->before_y = y;
		pen->before_alpha = alpha;
		pen->before_r = r;

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}
}

#define HardPenEditSelectionPressCallBack HardPenButtonPressCallBack

static void HardPenMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		HARD_PEN* pen = (HARD_PEN*)core->brush_data;
		cairo_t *update, *temp_update;
		cairo_surface_t *update_surface, *temp_surface;
		cairo_pattern_t *pattern;
		gdouble r, step, alpha, d;
		gdouble set_alpha;
		gdouble alpha_rate = 1;
		gdouble min_x, min_y, max_x, max_y;
		gdouble draw_x = pen->before_x, draw_y = pen->before_y;
		gdouble dx, dy, diff_x, diff_y;
		int start_x, width, start_y, height;
		int stride;
		uint8* color = *core->color;
		uint8 *mask;
		// ��ƃ��C���[�ւ̕`�挋�ʓ]�ʗp
		uint8 *mask_pix, *ref;
		int i, j;

		if((pen->flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = pen->r;
		}
		else
		{
			r = pen->r * pressure;
		}
		step = r * 0.125;
		if(step < 0.125)
		{
			step = 0.125;
		}
		set_alpha = alpha = ((pen->flags & BRUSH_FLAG_FLOW) == 0) ?
			pen->alpha * 0.01 : pen->alpha * 0.01 * pressure;
		if(r < 1)
		{
			alpha_rate = r;
			r = 1;
		}
		dx = x-pen->before_x, dy = y-pen->before_y;
		d = sqrt(dx*dx+dy*dy);
		diff_x = step * dx/d, diff_y = step * dy/d;

		if(d < pen->before_r * 0.125)
		{
			return;
		}

		min_x = x - r*2, min_y = y - r*2;
		max_x = x + r*2, max_y = y + r*2;

		if((pen->flags & BRUSH_FLAG_ANTI_ALIAS) != 0)
		{
			max_x++;
			max_y++;
		}

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		if(min_x > window->width || min_y > window->height
			|| max_x < 0 || max_y < 0)
		{
			return;
		}

		if(window->update.x > min_x)
		{
			window->update.width += window->update.x - (int)min_x;
			window->update.x = (int)min_x;
		}
		if(window->update.width + window->update.x < max_x)
		{
			window->update.width += (int)max_x - window->update.width + window->update.x;
		}
		if(window->update.y > min_y)
		{
			window->update.height += window->update.y - (int)min_y;
			window->update.y = (int)min_y;
		}
		if(window->update.height + window->update.y < max_y)
		{
			window->update.height += (int)max_y - window->update.height + window->update.y;
		}

		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		dx = d;
		do
		{
			start_x = (int)(draw_x - r);
			start_y = (int)(draw_y - r);
			width = (int)(draw_x + r + 1);
			height = (int)(draw_y + r + 1);

			if(start_x < 0)
			{
				start_x = 0;
			}
			else if(start_x > window->work_layer->width)
			{
				goto skip_draw;
			}
			if(start_y < 0)
			{
				start_y = 0;
			}
			else if(start_y > window->work_layer->height)
			{
				goto skip_draw;
			}
			if(width > window->work_layer->width)
			{
				width = window->work_layer->width - start_x;
			}
			else
			{
				width = width - start_x;
			}
			if(height > window->work_layer->height)
			{
				height = window->work_layer->height - start_y;
			}
			else
			{
				height = height - start_y;
			}
			stride = width*4;

			if(width <= 0 || height <= 0)
			{
				goto skip_draw;
			}

			window->flags |= DRAW_WINDOW_UPDATE_PART;

			// �`��G���A��0������
			for(i=0; i<height; i++)
			{
				(void)memset(&window->temp_layer->pixels[(start_y+i)*window->temp_layer->stride + start_x*4],
					0, stride);
			}
			for(i=0; i<height; i++)
			{
				(void)memset(&window->mask_temp->pixels[(start_y+i)*window->mask_temp->stride + start_x*4],
					0, stride);
			}

			// �`��p�~�`�p�^�[���쐬
			pattern = cairo_pattern_create_radial(r, r, 0,
				r, r, r);
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);
			set_alpha = ((1 - dx/d) * alpha + dx/d * pen->before_alpha) * alpha_rate;
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
			cairo_pattern_add_color_stop_rgba(pattern, 0, (*core->color)[2]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL,
				(*core->color)[0]*DIV_PIXEL, set_alpha);
			cairo_pattern_add_color_stop_rgba(pattern, 1, (*core->color)[2]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL,
				(*core->color)[0]*DIV_PIXEL, set_alpha);
#else
			cairo_pattern_add_color_stop_rgba(pattern, 0, (*core->color)[0]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL,
				(*core->color)[2]*DIV_PIXEL, set_alpha);
			cairo_pattern_add_color_stop_rgba(pattern, 1, (*core->color)[0]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL,
				(*core->color)[2]*DIV_PIXEL, set_alpha);
#endif

			update_surface = cairo_surface_create_for_rectangle(window->mask_temp->surface_p,
				draw_x - r, draw_y - r, r * 2, r * 2);
			update = cairo_create(update_surface);
			temp_surface = cairo_surface_create_for_rectangle(window->temp_layer->surface_p,
				draw_x - r, draw_y - r, r * 2, r * 2);
			temp_update = cairo_create(temp_surface);

			mask = window->mask_temp->pixels;
			if(window->app->textures.active_texture == 0)
			{
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_set_source(update, pattern);
						cairo_paint(update);
					}
					else
					{
						cairo_set_source(update, pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - draw_x + r, - draw_y + r);
					}
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_set_source(update, pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - draw_x + r, - draw_y + r);
					}
					else
					{
						cairo_set_source(temp_update, pattern);
						cairo_mask_surface(temp_update,
							window->selection->surface_p, - draw_x + r, - draw_y + r);
						cairo_set_source_surface(update, temp_surface, 0, 0);
						cairo_mask_surface(update, window->active_layer->surface_p,
							- draw_x + r, - draw_y + r);
					}
				}
			}
			else
			{
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_set_source(update, core->brush_pattern);
						cairo_mask_surface(update, window->texture->surface_p,
							- draw_x + r, - draw_y + r);
					}
					else
					{
						cairo_set_source(temp_update, pattern);
						cairo_mask_surface(temp_update, window->active_layer->surface_p,
							- draw_x + r, - draw_y + r);
						cairo_set_source_surface(update, temp_surface,
							0, 0);
						cairo_mask_surface(update,
							window->texture->surface_p, - draw_x + r, - draw_y + r);
					}
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_set_source(temp_update, pattern);
						cairo_mask_surface(temp_update,
							window->selection->surface_p, - draw_x + r, - draw_y + r);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->mask_temp->pixels[
								(i+start_y)*window->mask_temp->stride+start_x*4],
								0, stride
							);
						}

						cairo_set_source_surface(update, temp_surface, 0, 0);
						cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
					}
					else
					{
						cairo_set_source(update, pattern);
						cairo_mask_surface(update, window->selection->surface_p,
							- draw_x + r, - draw_y + r);
						cairo_set_source_surface(temp_update, update_surface, 0, 0);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_mask_surface(temp_update,
							window->active_layer->surface_p, - draw_x + r, - draw_y + r);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->mask_temp->pixels[
								(i+start_y)*window->mask_temp->stride+start_x*4],
								0, stride
							);
						}

						cairo_set_source_surface(update, temp_surface, 0, 0);
						cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
					}
				}
			}

			cairo_destroy(update);
			cairo_surface_destroy(update_surface);
			cairo_destroy(temp_update);
			cairo_surface_destroy(temp_surface);

			if((pen->flags & BRUSH_FLAG_ANTI_ALIAS) == 0)
			{
				uint8 threshold;
				uint8 set_color[4];

				threshold = (uint8)(alpha * alpha_rate * 255 * 0.5);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				set_color[0] = (uint8)((*core->color)[2] * alpha);
				set_color[1] = (uint8)((*core->color)[1] * alpha);
				set_color[2] = (uint8)((*core->color)[0] * alpha);
#else
				set_color[0] = (uint8)((*core->color)[0] * alpha);
				set_color[1] = (uint8)((*core->color)[1] * alpha);
				set_color[2] = (uint8)((*core->color)[2] * alpha);
#endif
				set_color[3] = (uint8)(alpha * 255 + 0.3);
				for(i=0; i<height; i++)
				{
					for(j=0; j<width; j++)
					{
						if(mask[(start_y+i)*window->stride+(start_x+j)*4+3] > threshold)
						{
							mask[(start_y+i)*window->stride+(start_x+j)*4] = set_color[0];
							mask[(start_y+i)*window->stride+(start_x+j)*4+1] = set_color[1];
							mask[(start_y+i)*window->stride+(start_x+j)*4+2] = set_color[2];
							mask[(start_y+i)*window->stride+(start_x+j)*4+3] = set_color[3];
						}
						else
						{
							mask[(start_y+i)*window->stride+(start_x+j)*4] = 0;
							mask[(start_y+i)*window->stride+(start_x+j)*4+1] = 0;
							mask[(start_y+i)*window->stride+(start_x+j)*4+2] = 0;
							mask[(start_y+i)*window->stride+(start_x+j)*4+3] = 0;
						}
					}
				}
			}

			for(i=0; i<height; i++)
			{
				mask_pix = &mask[(start_y+i)*window->stride+start_x*4];
				ref = &window->work_layer->pixels[(start_y+i)*window->stride+start_x*4];
				for(j=0; j<width; mask_pix += 4, ref += 4, j++)
				{
					if(mask_pix[3] >= ref[3])
					{
						ref[0] = (uint8)(((int)mask_pix[0] - (int)ref[0]) * mask_pix[3] >> 8) + ref[0];
						ref[1] = (uint8)(((int)mask_pix[1] - (int)ref[1]) * mask_pix[3] >> 8) + ref[1];
						ref[2] = (uint8)(((int)mask_pix[2] - (int)ref[2]) * mask_pix[3] >> 8) + ref[2];
						ref[3] = (uint8)(((int)mask_pix[3] - (int)ref[3]) * mask_pix[3] >> 8) + ref[3];
					}
				}
			}

			pen->before_x = draw_x,	pen->before_y = draw_y;
skip_draw:
			dx -= step;
			if(dx < 1)
			{
				pen->before_alpha = set_alpha;
				pen->before_r = r;
				break;
			}
			else if(dx >= step)
			{
				draw_x += diff_x, draw_y += diff_y;
			}
			else
			{
				draw_x = x;
				draw_y = y;
			}
		} while(1);

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}
}

#define HardPenEditSelectionMotionCallBack HardPenMotionCallBack

static void HardPenButtonReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		AddBrushHistory(core, window->active_layer);

		window->update.surface_p = cairo_surface_create_for_rectangle(
			window->active_layer->surface_p, window->update.x, window->update.y,
				window->update.width, window->update.height);
		window->update.cairo_p = cairo_create(window->update.surface_p);

		g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->update);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		cairo_surface_destroy(window->update.surface_p);
		cairo_destroy(window->update.cairo_p);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

#define HardPenEditSelectionReleaseCallBack DefaultEditSelectionReleaseCallBack


static void HardPenDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	HARD_PEN* pen = (HARD_PEN*)data;
	gdouble r = pen->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void HardPenButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, HARD_PEN* pen)
{
	gdouble r = pen->r * window->zoom_rate + 3;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2 + 3), (gint)(r * 2 + 3));
}

static void HardPenMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, HARD_PEN* pen)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = pen->r * window->zoom_rate  * 1.275 + BRUSH_UPDATE_MARGIN;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width + 2, (gint)height + 2);
}

static void HardPenScaleChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	HARD_PEN* pen = (HARD_PEN*)data;
	pen->r = gtk_adjustment_get_value(slider) * 0.5;
}

static void HardPenFlowChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	HARD_PEN* pen = (HARD_PEN*)data;
	pen->alpha = gtk_adjustment_get_value(slider);
}

static void HardPenPressureSizeChange(
	GtkWidget* widget,
	gpointer data
)
{
	HARD_PEN* pen = (HARD_PEN*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		pen->flags &= ~(BRUSH_FLAG_SIZE);
	}
	else
	{
		pen->flags |= BRUSH_FLAG_SIZE;
	}
}

static void HardPenPressureFlowChange(
	GtkWidget* widget,
	gpointer data
)
{
	HARD_PEN* pen = (HARD_PEN*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		pen->flags &= ~(BRUSH_FLAG_FLOW);
	}
	else
	{
		pen->flags |= BRUSH_FLAG_FLOW;
	}
}

static void HardPenSetAntiAlias(
	GtkWidget* widget,
	gpointer data
)
{
	HARD_PEN* pen = (HARD_PEN*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		pen->flags &= ~(BRUSH_FLAG_ANTI_ALIAS);
	}
	else
	{
		pen->flags |= BRUSH_FLAG_ANTI_ALIAS;
	}
}

static void HardPenSetBlendMode(GtkComboBox* combo, HARD_PEN* pen)
{
	pen->blend_mode = (uint16)gtk_combo_box_get_active(combo);
}

static GtkWidget* CreateHardPenDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	HARD_PEN* pen = (HARD_PEN*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* hbox;
	GtkWidget* brush_scale;
	GtkWidget* table = gtk_table_new(1, 3, TRUE);
	GtkWidget* check_button;
	GtkWidget* label;
	GtkWidget *combo;
	GtkWidget* base_scale;
	GtkAdjustment* brush_scale_adjustment;
	char mark_up_buff[256];
	int i;

	pen->core = core;

	switch(pen->base_scale)
	{
	case 0:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(pen->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(pen->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(pen->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
		break;
	}

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

#if MAJOR_VERSION == 1
		base_scale = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[2]);
#else
		base_scale = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), pen->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(HardPenScaleChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	g_object_set_data(G_OBJECT(base_scale), "scale", brush_scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &pen->base_scale);

	brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(pen->alpha, 0.0, 100.0, 1.0, 1.0, 0.0));
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.flow, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(HardPenFlowChange), core->brush_data);
	table = gtk_table_new(1, 3, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
#if MAJOR_VERSION == 1
	combo = gtk_combo_box_new_text();
#else
	combo = gtk_combo_box_text_new();
#endif
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->layer_window.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
#if MAJOR_VERSION == 1
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), app->labels->layer_window.blend_modes[i]);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), app->labels->layer_window.blend_modes[i]);
#endif
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), pen->blend_mode);
	(void)g_signal_connect(G_OBJECT(combo), "changed",
		G_CALLBACK(HardPenSetBlendMode), pen);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(HardPenPressureSizeChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), pen->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(HardPenPressureFlowChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), pen->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(HardPenSetAntiAlias), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), pen->flags & BRUSH_FLAG_ANTI_ALIAS);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	return vbox;
}

static void AirBrushPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		AIR_BRUSH *brush = (AIR_BRUSH*)core->brush_data;
		// �X�V�͈͂̃C���[�W���
		cairo_surface_t *update_surface;
		cairo_t *update;
		// �u���V�̔��a�ƕs�����x
		FLOAT_T r, zoom, alpha;
		// ���W�̍ő�l�E�ŏ��l
		FLOAT_T min_x, min_y, max_x, max_y;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �Q�ƃs�N�Z��
		uint8 *mask;
		uint8 *ref_pix, *mask_pix;
		// �`��ʒu�w��p
		cairo_matrix_t matrix;
		int i, j;	// for���p�̃J�E���^

		window->work_layer->layer_mode = brush->blend_mode;

		if((brush->flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = brush->r;
			zoom = 1;
		}
		else
		{
			r = brush->r * pressure;
			zoom = 1/pressure;
		}

		brush->before_x = x, brush->before_y = y;
		brush->points[0][1] = x, brush->points[0][2] = y;
		brush->points[0][3] = pressure;
		brush->sum_distance = brush->travel = brush->finish_length = 0;
		brush->draw_start = r * brush->out * 0.01 * 4;
		brush->enter_length = r * brush->enter * 0.01 * 4;
		brush->enter_size = r * (1 - brush->enter * 0.01);
		brush->num_point = 0;
		brush->draw_finished = 0;
		brush->ref_point = 1;

		min_x = x - r, max_x = x + r;
		min_y = y - r, max_y = y + r;

		update_surface = cairo_surface_create_for_rectangle(
			window->mask_temp->surface_p, min_x, min_y, r*2+1, r*2+1);
		update = cairo_create(update_surface);

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		window->update.x = (int)min_x, window->update.y = (int)min_y;
		window->update.width = (int)max_x - window->update.x;
		window->update.height = (int)max_y - window->update.y;

		window->flags |= DRAW_WINDOW_UPDATE_PART;

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		if(brush->enter_length == 0)
		{
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;
			start_x = (int)min_x, start_y = (int)min_y;
			width = (int)(max_x - min_x);
			height = (int)(max_y - min_y);
			stride = width*4;

			if(max_x > window->width || max_y > window->height
				|| max_x < 0 || max_y < 0)
			{
				return;
			}

			for(i=0; i<height; i++)
			{
				(void)memset(&window->mask_temp->pixels[
					(i+start_y)*window->mask_temp->stride+start_x*4], 0, stride);
			}

			cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);

			mask = window->mask_temp->pixels;
			if(window->app->textures.active_texture == 0)
			{
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0,0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - x + r, - y + r);
					}
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);
					}
					else
					{
						cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
						cairo_t *update_temp = cairo_create(temp_surface);

						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - x + r, - y + r);

						mask = window->temp_layer->pixels;

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}
				}
			}
			else
			{
				cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
					window->temp_layer->surface_p, min_x, min_y, r*2+1, r*2+1);
				cairo_t *update_temp = cairo_create(temp_surface);

				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - x + r, - y + r);
					}

					for(i=0; i<height; i++)
					{
						(void)memset(&window->temp_layer->pixels[
							(i+start_y)*window->temp_layer->stride+start_x*4],
							0, stride
						);
					}

					cairo_set_source_surface(update_temp, update_surface, 0, 0);
					cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

					mask = window->temp_layer->pixels;
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_set_source_surface(update_temp, update_surface, 0, 0);
						cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

						mask = window->temp_layer->pixels;
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - x + r, - y + r);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->mask_temp->pixels[
								(i+start_y)*window->mask_temp->stride+start_x*4],
								0, stride
							);
						}

						cairo_set_source_surface(update, temp_surface, 0, 0);
						cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
					}
				}

				cairo_surface_destroy(temp_surface);
				cairo_destroy(update_temp);
			}

			cairo_surface_destroy(update_surface);
			cairo_destroy(update);

			/*
			for(i=0; i<height; i++)
			{
				(void)memcpy(
					&window->work_layer->pixels[(start_y+i)*window->work_layer->stride+start_x*4],
					&mask[(start_y+i)*window->work_layer->stride+start_x*4],
					stride
				);
			}
			*/
			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[
					(start_y+i)*window->work_layer->stride+start_x*4];
				mask_pix = &mask[(start_y+i)*window->work_layer->stride
					+start_x*4];

				for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
				{
					ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
						* mask_pix[3] >> 8) + ref_pix[0]);
					ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
						* mask_pix[3] >> 8) + ref_pix[1]);
					ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
						* mask_pix[3] >> 8) + ref_pix[2]);
					ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
						* mask_pix[3] >> 8) + ref_pix[3]);
				}
			}
		}	// if(brush->enter_length == 0)
	}	// ���N���b�N�Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

#define AirBrushEditSelectionPressCallBack AirBrushPressCallBack

static void AirBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^����������Ă�����
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		AIR_BRUSH *brush = (AIR_BRUSH*)core->brush_data;
		FLOAT_T distance;
		int index = brush->ref_point % BRUSH_POINT_BUFFER_SIZE;

		distance = sqrt((x-brush->before_x)*(x-brush->before_x)
			+(y-brush->before_y)*(y-brush->before_y));

		if(distance >= 1.0)
		{
			brush->before_x = x, brush->before_y = y;
			brush->sum_distance += distance;
			brush->travel += distance;
			brush->points[index][0] = distance;
			brush->points[index][1] = x, brush->points[index][2] = y;
			brush->points[index][3] = pressure;
			brush->ref_point++;

			if(brush->sum_distance >= brush->draw_start
				&& brush->sum_distance >= brush->enter_length)
			{
				// �X�V�͈͂̃C���[�W���
				cairo_surface_t *update_surface;
				cairo_t *update;
				// �u���V�̕M���ɂ��k���p
				gdouble zoom;
				// ����̐F�␳�p
				gdouble enter_alpha;
				// �u���V�̔��a�ƕs�����x
				gdouble r, alpha;
				// ���W�̍ő�E�ŏ��l
				gdouble min_x, min_y, max_x, max_y;
				// X�AY�����̈ړ���
				gdouble dx, dy;
				// �u���V�̌X���p
				gdouble diff_x, diff_y;
				// �u���V�̈ړ���
				gdouble d, step;
				// �`����s�����W
				gdouble draw_x = brush->before_x, draw_y = brush->before_y;
				// �`����W�w��p
				cairo_matrix_t matrix;
				// �s�N�Z���f�[�^�����Z�b�g������W
				int start_x, start_y;
				// �`�悷�镝�A�����A��s���̃o�C�g��
				int width, height, stride;
				// �Q�ƃs�N�Z��
				uint8 *ref_pix, *mask_pix, *mask;
				// �z��̃C���f�b�N�X�p
				int ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				int before_point;
				int i, j;	// for���p�̃J�E���^

				while(brush->sum_distance > brush->draw_start
					&& brush->draw_finished < brush->ref_point)
				{
					if(brush->finish_length < brush->enter_length)
					{
						enter_alpha = brush->finish_length / brush->enter_length;
						r = brush->enter_size + brush->finish_length * 0.25;
					}
					else
					{
						zoom = enter_alpha = 1, r = brush->r;
					}
					
					if((brush->flags & BRUSH_FLAG_SIZE) != 0)
					{
						r *= brush->points[ref_point][3];
					}

					
					alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
						? 1 : brush->points[ref_point][3];
					alpha *= enter_alpha;
					before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
					d = brush->points[ref_point][0];
					brush->sum_distance -= d;

					if(r > MIN_BRUSH_STEP)
					{
						zoom = brush->r / r;
						step = r * BRUSH_STEP;
						if(step < 0.5)
						{
							step = 0.5;
						}

						if(brush->draw_finished == 0)
						{
							draw_x = brush->points[0][1], draw_y = brush->points[0][2];
						}
						else
						{
							draw_x = brush->points[before_point][1];
							draw_y = brush->points[before_point][2];
						}
						dx = brush->points[ref_point][1] - draw_x;
						dy = brush->points[ref_point][2] - draw_y;
						diff_x = step*dx/d, diff_y = step*dy/d;

						min_x = brush->points[ref_point][1] - r - 1;
						max_x = brush->points[ref_point][1] + r + 1;
						min_y = brush->points[ref_point][2] - r - 1;
						max_y = brush->points[ref_point][2] + r + 1;

						if(min_x < 0.0)
						{
							min_x = 0.0;
						}
						if(core->min_x > min_x)
						{
							core->min_x = min_x;
						}
						if(min_y < 0.0)
						{
							min_y = 0.0;
						}
						if(core->min_y > min_y)
						{
							core->min_y = min_y;
						}
						if(max_x > window->work_layer->width)
						{
							max_x = window->work_layer->width;
						}
						if(core->max_x < max_x)
						{
							core->max_x = max_x;
						}
						if(max_y > window->work_layer->height)
						{
							min_y = window->work_layer->height;
						}
						if(core->max_y < max_y)
						{
							core->max_y = max_y;
						}

						if(window->update.x > min_x)
						{
							window->update.width += window->update.x - (int)min_x;
							window->update.x = (int)min_x;
						}
						if(window->update.width + window->update.x < max_x)
						{
							window->update.width += (int)max_x - window->update.width + window->update.x;
						}
						if(window->update.y > min_y)
						{
							window->update.height += window->update.y - (int)min_y;
							window->update.y = (int)min_y;
						}
						if(window->update.height + window->update.y < max_y)
						{
							window->update.height += (int)max_y - window->update.height + window->update.y;
						}

						dx = d;
						do
						{
							start_x = (int)(draw_x - r);
							start_y = (int)(draw_y - r);
							width = (int)(draw_x + r) + 1;
							height = (int)(draw_y + r) + 1;

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(start_x > window->work_layer->width)
							{
								goto skip_draw;
							}
							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(start_y > window->work_layer->height)
							{
								goto skip_draw;
							}
							if(width > window->work_layer->width)
							{
								width = window->work_layer->width - start_x;
							}
							else
							{
								width = width - start_x;
							}
							if(height > window->work_layer->height)
							{
								height = window->work_layer->height - start_y;
							}
							else
							{
								height = height - start_y;
							}
							stride = width*4;

							if(width <= 0 || height <= 0)
							{
								goto skip_draw;
							}

							update_surface = cairo_surface_create_for_rectangle(
								window->mask_temp->surface_p, draw_x - r, draw_y - r,
									r*2+1, r*2+1);
							update = cairo_create(update_surface);

							window->flags |= DRAW_WINDOW_UPDATE_PART;

							for(i=0; i<height; i++)
							{
								(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
									0, stride);
							}

							cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);

							mask = window->mask_temp->pixels;
							if(window->app->textures.active_texture == 0)
							{
								if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(update, core->brush_pattern);
										cairo_paint_with_alpha(update, alpha);
									}
									else
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
									}
								}
								else
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);
									}
									else
									{
										cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
											window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
										cairo_t *update_temp = cairo_create(temp_surface);

										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);

										for(i=0; i<height; i++)
										{
											(void)memset(&window->temp_layer->pixels[
												(i+start_y)*window->temp_layer->stride+start_x*4],
												0, stride
											);
										}

										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);
										cairo_set_source_surface(update_temp,
											update_surface, 0, 0);
										cairo_mask_surface(update_temp,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
	
										mask = window->temp_layer->pixels;

										cairo_surface_destroy(temp_surface);
										cairo_destroy(update_temp);
									}
								}
							}
							else
							{
								cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
									window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
								cairo_t *update_temp = cairo_create(temp_surface);

								if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(update, core->brush_pattern);
										cairo_paint_with_alpha(update, alpha);
									}
									else
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
									}

									for(i=0; i<height; i++)
									{
										(void)memset(&window->temp_layer->pixels[
											(i+start_y)*window->temp_layer->stride+start_x*4],
											0, stride
										);
									}

									cairo_set_source_surface(update_temp, update_surface, 0, 0);
									cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

									mask = window->temp_layer->pixels;
								}
								else
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);

										for(i=0; i<height; i++)
										{
											(void)memset(&window->temp_layer->pixels[
												(i+start_y)*window->temp_layer->stride+start_x*4],
												0, stride
											);
										}

										cairo_set_source_surface(update_temp, update_surface, 0, 0);
										cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

										mask = window->temp_layer->pixels;
									}
									else
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);

										for(i=0; i<height; i++)
										{
											(void)memset(&window->temp_layer->pixels[
												(i+start_y)*window->temp_layer->stride+start_x*4],
												0, stride
											);
										}

										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);
										cairo_set_source_surface(update_temp,
											update_surface, 0, 0);
										cairo_mask_surface(update_temp,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
	
										for(i=0; i<height; i++)
										{
											(void)memset(&window->mask_temp->pixels[
												(i+start_y)*window->mask_temp->stride+start_x*4],
												0, stride
											);
										}

										cairo_set_source_surface(update, temp_surface, 0, 0);
										cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
									}
								}

								cairo_surface_destroy(temp_surface);
								cairo_destroy(update_temp);
							}

							cairo_surface_destroy(update_surface);
							cairo_destroy(update);

							for(i=0; i<height; i++)
							{
								ref_pix = &window->work_layer->pixels[
									(start_y+i)*window->work_layer->stride+start_x*4];
								mask_pix = &mask[(start_y+i)*window->work_layer->stride
									+start_x*4];

								for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
								{
									if(ref_pix[3] < mask_pix[3])
									{
										ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
											* mask_pix[3] >> 8) + ref_pix[0]);
										ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
											* mask_pix[3] >> 8) + ref_pix[1]);
										ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
											* mask_pix[3] >> 8) + ref_pix[2]);
										ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
											* mask_pix[3] >> 8) + ref_pix[3]);
									}
								}
							}

							// �A���`�G�C���A�X��K�p
							if((brush->flags & BRUSH_FLAG_ANTI_ALIAS) != 0)
							{
								ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
									width + 3, height + 3};
								AntiAliasLayer(window->work_layer, window->temp_layer, &range);
							}
skip_draw:
							dx -= step;
							if(dx < 1)
							{
								break;
							}
							else if(dx >= step)
							{
								draw_x += diff_x, draw_y += diff_y;
							}
							else
							{
								draw_x = brush->points[ref_point][1];
								draw_y = brush->points[ref_point][2];
							}
						} while(1);
					}

					brush->finish_length += d;
					brush->travel += d;
					brush->draw_finished++;
					ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				}	// while(brush->sum_distance > brush->draw_start
						// && brush->draw_finished < brush->ref_point)
			}	// if(brush->sum_distance >= brush->draw_start
					// && brush->sum_distance >= brush->enter_length)
		}	// if(distance >= 1.0)
	}	// �}�E�X�̍��{�^����������Ă�����
		// if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
}

#define AirBrushEditSelectionMotionCallBack AirBrushMotionCallBack

static void AirBrushReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^���Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		AIR_BRUSH *brush = (AIR_BRUSH*)core->brush_data;
		// �X�V�͈͂̃C���[�W���
		cairo_surface_t *update_surface;
		cairo_t *update;
		// �u���V�̏k����
		gdouble zoom;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̈ړ���
		gdouble d, step;
		// �`����s�����W
		gdouble draw_x, draw_y;
		// ����A�������̐F�␳
		gdouble enter_alpha, out_alpha;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �`����W�w��p
		cairo_matrix_t matrix;
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask;
		// �z��̃C���f�b�N�X�p
		int ref_point;
		int before_point;
		int i, j;	// for���p�̃J�E���^

		while(brush->ref_point > brush->draw_finished)
		{
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;

			if(brush->finish_length < brush->enter_length)
			{
				enter_alpha = brush->finish_length / brush->enter_length;
				r = brush->enter_size + brush->finish_length * 0.25;
			}
			else
			{
				enter_alpha = 1;
				r = brush->r;
			}
			if(brush->sum_distance < brush->draw_start)
			{
				out_alpha = brush->sum_distance / brush->draw_start;
				r -= (brush->draw_start - brush->sum_distance) * 0.25;
			}
			else
			{
				out_alpha = 1;
			}

			if(r < 0.0)
			{
				brush->draw_finished++;
				continue;
			}

			if((brush->flags & BRUSH_FLAG_SIZE) != 0)
			{
				r *= brush->points[ref_point][3];
			}
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
				? 1 : brush->points[ref_point][3];
			alpha *= enter_alpha * out_alpha;
			before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
			if(r > MIN_BRUSH_STEP)
			{
				d = brush->points[ref_point][0];
				brush->sum_distance -= d;
				step = r * BRUSH_STEP;
				zoom = brush->r / r;

				if(step < 0.5)
				{
					step = 0.5;
				}
				if(brush->draw_finished == 0)
				{
					draw_x = brush->points[0][1], draw_y = brush->points[0][2];
				}
				else
				{
					draw_x = brush->points[before_point][1], draw_y = brush->points[before_point][2];
				}
				dx = brush->points[ref_point][1] - draw_x;
				dy = brush->points[ref_point][2] - draw_y;
				diff_x = step * dx / d, diff_y = step * dy / d;

				min_x = brush->points[ref_point][1] - r - 1;
				max_x = brush->points[ref_point][1] + r + 1;
				min_y = brush->points[ref_point][2] - r - 1;
				max_y = brush->points[ref_point][2] + r + 1;
				if(min_x < 0.0)
				{
					min_x = 0.0;
				}
				if(core->min_x > min_x)
				{
					core->min_x = min_x;
				}
				if(min_y < 0.0)
				{
					min_y = 0.0;
				}
				if(core->min_y > min_y)
				{
					core->min_y = min_y;
				}
				if(max_x > window->work_layer->width)
				{
					max_x = window->work_layer->width;
				}
				if(core->max_x < max_x)
				{
					core->max_x = max_x;
				}
				if(max_y > window->work_layer->height)
				{
					min_y = window->work_layer->height;
				}
				if(core->max_y < max_y)
				{
					core->max_y = max_y;
				}

				dx = d;
				do
				{
					start_x = (int)(draw_x - r);
					start_y = (int)(draw_y - r);
					width = (int)(draw_x + r);
					height = (int)(draw_y + r);

					if(start_x < 0)
					{
						start_x = 0;
					}
					else if(start_x > window->work_layer->width)
					{
						goto skip_draw;
					}
					if(start_y < 0)
					{
						start_y = 0;
					}
					else if(start_y > window->work_layer->height)
					{
						goto skip_draw;
					}
					if(width > window->work_layer->width)
					{
						width = window->work_layer->width - start_x;
					}
					else
					{
						width = width - start_x;
					}
					if(height > window->work_layer->height)
					{
						height = window->work_layer->height - start_y;
					}
					else
					{
						height = height - start_y;
					}
					stride = width*4;

					if(width <= 0 || height <= 0)
					{
						goto skip_draw;
					}

					update_surface = cairo_surface_create_for_rectangle(
						window->mask_temp->surface_p, draw_x - r, draw_y - r,
							r*2+1, r*2+1);
					update = cairo_create(update_surface);

					if(window->update.x > min_x)
					{
						window->update.width += window->update.x - (int)min_x;
						window->update.x = (int)min_x;
					}
					if(window->update.width + window->update.x < max_x)
					{
						window->update.width += (int)max_x - window->update.width + window->update.x;
					}
					if(window->update.y > min_y)
					{
						window->update.height += window->update.y - (int)min_y;
						window->update.y = (int)min_y;
					}
					if(window->update.height + window->update.y < max_y)
					{
						window->update.height += (int)max_y - window->update.height + window->update.y;
					}

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride);
					}

					cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);

					mask = window->mask_temp->pixels;
					if(window->app->textures.active_texture == 0)
					{
						if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(update, core->brush_pattern);
								cairo_paint_with_alpha(update, alpha);
							}
							else
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
							}
						}
						else
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);
							}
							else
							{
								cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
									window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
								cairo_t *update_temp = cairo_create(temp_surface);

								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);

								for(i=0; i<height; i++)
								{
									(void)memset(&window->temp_layer->pixels[
										(i+start_y)*window->temp_layer->stride+start_x*4],
										0, stride
									);
								}

								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);
								cairo_set_source_surface(update_temp,
									update_surface, 0, 0);
								cairo_mask_surface(update_temp,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
	
								mask = window->temp_layer->pixels;

								cairo_surface_destroy(temp_surface);
								cairo_destroy(update_temp);
							}
						}
					}
					else
					{
						cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
						cairo_t *update_temp = cairo_create(temp_surface);

						if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(update, core->brush_pattern);
								cairo_paint_with_alpha(update, alpha);
							}
							else
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
							}

							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}

							cairo_set_source_surface(update_temp, update_surface, 0, 0);
							cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

							mask = window->temp_layer->pixels;
						}
						else
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);

								for(i=0; i<height; i++)
								{
									(void)memset(&window->temp_layer->pixels[
										(i+start_y)*window->temp_layer->stride+start_x*4],
										0, stride
									);
								}

								cairo_set_source_surface(update_temp, update_surface, 0, 0);
								cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

								mask = window->temp_layer->pixels;
							}
							else
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);

								for(i=0; i<height; i++)
								{
									(void)memset(&window->temp_layer->pixels[
										(i+start_y)*window->temp_layer->stride+start_x*4],
										0, stride
									);
								}

								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);
								cairo_set_source_surface(update_temp,
									update_surface, 0, 0);
								cairo_mask_surface(update_temp,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
	
								for(i=0; i<height; i++)
								{
									(void)memset(&window->mask_temp->pixels[
										(i+start_y)*window->mask_temp->stride+start_x*4],
										0, stride
									);
								}

								cairo_set_source_surface(update, temp_surface, 0, 0);
								cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
							}
						}

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}

					cairo_surface_destroy(update_surface);
					cairo_destroy(update);

					for(i=0; i<height; i++)
					{
						ref_pix = &window->work_layer->pixels[
							(start_y+i)*window->work_layer->stride+start_x*4];
						mask_pix = &mask[(start_y+i)*window->work_layer->stride
							+start_x*4];

						for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
						{
							if(ref_pix[3] < mask_pix[3])
							{
								ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
									* mask_pix[3] >> 8) + ref_pix[0]);
								ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
									* mask_pix[3] >> 8) + ref_pix[1]);
								ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
									* mask_pix[3] >> 8) + ref_pix[2]);
								ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
									* mask_pix[3] >> 8) + ref_pix[3]);
							}
						}
					}

					// �A���`�G�C���A�X��K�p
					if((brush->flags & BRUSH_FLAG_ANTI_ALIAS) != 0)
					{
						ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
							width + 3, height + 3};
						AntiAliasLayer(window->work_layer, window->temp_layer, &range);
					}
skip_draw:
					dx -= step;
					if(dx < 1)
					{
						break;
					}
					else if(dx >= step)
					{
						draw_x += diff_x, draw_y += diff_y;
					}
					else
					{
						draw_x = brush->points[ref_point][1];
						draw_y = brush->points[ref_point][2];
					}
				} while(1);
			}

			brush->finish_length += d;
			brush->travel += d;
			brush->draw_finished++;
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
		}	// while(brush->ref_point > brush->draw_finished)

		AddBrushHistory(core, window->active_layer);

		window->update.surface_p = cairo_surface_create_for_rectangle(
			window->active_layer->surface_p, window->update.x, window->update.y,
				window->update.width, window->update.height);
		window->update.cairo_p = cairo_create(window->update.surface_p);
		g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->update);
		cairo_surface_destroy(window->update.surface_p);
		cairo_destroy(window->update.cairo_p);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}	// �}�E�X�̍��{�^���Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

static void AirBrushEditSelectionReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^���Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		AIR_BRUSH *brush = (AIR_BRUSH*)core->brush_data;
		// �u���V�̏k����
		gdouble zoom;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̈ړ���
		gdouble d, step;
		// �`����s�����W
		gdouble draw_x, draw_y;
		// ����A�������̐F�␳
		gdouble enter_alpha, out_alpha;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �`����W�w��p
		cairo_matrix_t matrix;
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask;
		// �z��̃C���f�b�N�X�p
		int ref_point;
		int before_point;
		int i, j;	// for���p�̃J�E���^

		while(brush->ref_point > brush->draw_finished)
		{
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;

			if(brush->finish_length < brush->enter_length)
			{
				enter_alpha = brush->finish_length / brush->enter_length;
				r = brush->enter_size + brush->finish_length * 0.25;
			}
			else
			{
				enter_alpha = 1;
				r = brush->r;
			}
			if(brush->sum_distance < brush->draw_start)
			{
				out_alpha = brush->sum_distance / brush->draw_start;
				r -= (brush->draw_start - brush->sum_distance) * 0.25;
			}
			else
			{
				out_alpha = 1;
			}

			if(r < 0.0)
			{
				brush->draw_finished++;
				continue;
			}

			if((brush->flags & BRUSH_FLAG_SIZE) != 0)
			{
				r *= brush->points[ref_point][3];
			}
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
				? 1 : brush->points[ref_point][3];
			alpha *= enter_alpha * out_alpha;
			before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
			if(r > MIN_BRUSH_STEP)
			{
				d = brush->points[ref_point][0];
				brush->sum_distance -= d;
				step = r * BRUSH_STEP;
				if(step < 0.5)
				{
					step = 0.5;
				}
				zoom = brush->r / r;
				if(brush->draw_finished == 0)
				{
					draw_x = brush->points[0][1], draw_y = brush->points[0][2];
				}
				else
				{
					draw_x = brush->points[before_point][1], draw_y = brush->points[before_point][2];
				}
				dx = brush->points[ref_point][1] - draw_x;
				dy = brush->points[ref_point][2] - draw_y;
				diff_x = step * dx / d, diff_y = step * dy / d;

				min_x = brush->points[ref_point][1] - r - 1;
				max_x = brush->points[ref_point][1] + r + 1;
				min_y = brush->points[ref_point][2] - r - 1;
				max_y = brush->points[ref_point][2] + r + 1;
				if(min_x < 0.0)
				{
					min_x = 0.0;
				}
				if(core->min_x > min_x)
				{
					core->min_x = min_x;
				}
				if(min_y < 0.0)
				{
					min_y = 0.0;
				}
				if(core->min_y > min_y)
				{
					core->min_y = min_y;
				}
				if(max_x > window->work_layer->width)
				{
					max_x = window->work_layer->width;
				}
				if(core->max_x < max_x)
				{
					core->max_x = max_x;
				}
				if(max_y > window->work_layer->height)
				{
					min_y = window->work_layer->height;
				}
				if(core->max_y < max_y)
				{
					core->max_y = max_y;
				}

				dx = d;
				do
				{
					start_x = (int)(draw_x - r);
					start_y = (int)(draw_y - r);
					width = (int)(draw_x + r);
					height = (int)(draw_y + r);

					if(start_x < 0)
					{
						start_x = 0;
					}
					else if(start_x > window->work_layer->width)
					{
						goto skip_draw;
					}
					if(start_y < 0)
					{
						start_y = 0;
					}
					else if(start_y > window->work_layer->height)
					{
						goto skip_draw;
					}
					if(width > window->work_layer->width)
					{
						width = window->work_layer->width - start_x;
					}
					else
					{
						width = width - start_x;
					}
					if(height > window->work_layer->height)
					{
						height = window->work_layer->height - start_y;
					}
					else
					{
						height = height - start_y;
					}
					stride = width*4;

					if(width <= 0 || height <= 0)
					{
						goto skip_draw;
					}

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride);
					}

					cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);

					mask = window->mask_temp->pixels;
					if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_matrix_translate(&matrix, - draw_x + r, - draw_y + r);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(window->mask_temp->cairo_p, core->brush_pattern);
							cairo_paint_with_alpha(window->mask_temp->cairo_p, alpha);
						}
						else
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(window->mask_temp->cairo_p, core->temp_pattern);
							cairo_mask_surface(window->mask_temp->cairo_p,
								window->active_layer->surface_p, 0, 0);
						}
					}
					else
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(window->mask_temp->cairo_p, core->temp_pattern);
							cairo_mask_surface(window->mask_temp->cairo_p,
								window->selection->surface_p, 0, 0);
						}
						else
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(window->mask_temp->cairo_p, core->temp_pattern);

							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}

							cairo_mask_surface(window->mask_temp->cairo_p,
								window->selection->surface_p, 0, 0);
							cairo_set_source_surface(window->temp_layer->cairo_p,
								window->mask_temp->surface_p, 0, 0);
							cairo_mask_surface(window->temp_layer->cairo_p,
								window->active_layer->surface_p, 0, 0);
	
							mask = window->temp_layer->pixels;
						}
					}

					for(i=0; i<height; i++)
					{
						ref_pix = &window->work_layer->pixels[
							(start_y+i)*window->work_layer->stride+start_x*4];
						mask_pix = &mask[(start_y+i)*window->work_layer->stride
							+start_x*4];

						for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
						{
							if(ref_pix[3] < mask_pix[3])
							{
								ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
									* mask_pix[3] >> 8) + ref_pix[0]);
								ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
									* mask_pix[3] >> 8) + ref_pix[1]);
								ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
									* mask_pix[3] >> 8) + ref_pix[2]);
								ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
									* mask_pix[3] >> 8) + ref_pix[3]);
							}
						}
					}

					// �A���`�G�C���A�X��K�p
					if((brush->flags & BRUSH_FLAG_ANTI_ALIAS) != 0)
					{
						ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
							width + 1, height + 1};
						AntiAliasLayer(window->work_layer, window->temp_layer, &range);
					}
skip_draw:
					dx -= step;
					if(dx < 1)
					{
						break;
					}
					else if(dx >= step)
					{
						draw_x += diff_x, draw_y += diff_y;
					}
					else
					{
						draw_x = brush->points[ref_point][1];
						draw_y = brush->points[ref_point][2];
					}
				} while(1);
			}

			brush->finish_length += d;
			brush->travel += d;
			brush->draw_finished++;
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
		}	// while(brush->ref_point > brush->draw_finished)

		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[window->selection->layer_mode](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}	// �}�E�X�̍��{�^���Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

static void AirBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	AIR_BRUSH* brush = (AIR_BRUSH*)data;
	gdouble r = brush->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void AirBrushButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, AIR_BRUSH* brush)
{
	gdouble r = brush->r * window->zoom_rate + 1;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2) + 2, (gint)(r * 2) + 2);
}

static void AirBrushMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, AIR_BRUSH* brush)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = brush->r * window->zoom_rate * 1.275 + BRUSH_UPDATE_MARGIN;
	gdouble enter = (brush->enter_length + brush->draw_start) * window->zoom_rate;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	start_x -= enter * 4;
	width += enter * 4 * 2 + 2;
	start_y -= enter * 4;
	height += enter * 4 * 2 + 2;

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width, (gint)height);
}

static void AirBrushColorChange(const uint8 color[3], void* data)
{
	AIR_BRUSH *brush = (AIR_BRUSH*)data;
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void AirBrushSetScale(GtkAdjustment* slider, AIR_BRUSH* brush)
{
	brush->r = gtk_adjustment_get_value(slider) * 0.5;
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void AirBrushSetFlow(GtkAdjustment* slider, AIR_BRUSH* brush)
{
	brush->opacity = gtk_adjustment_get_value(slider);
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void AirBrushSetBlur(GtkAdjustment* slider, AIR_BRUSH* brush)
{
	brush->blur = gtk_adjustment_get_value(slider);
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void AirBrushSetOutlineHardness(GtkAdjustment* slider, AIR_BRUSH* brush)
{
	brush->outline_hardness = gtk_adjustment_get_value(slider);
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void AirBrushSetEnterSize(GtkAdjustment* slider, AIR_BRUSH* brush)
{
	brush->enter = gtk_adjustment_get_value(slider);
}

static void AirBrushSetOutSize(GtkAdjustment* slider, AIR_BRUSH* brush)
{
	brush->out = gtk_adjustment_get_value(slider);
}

static void AirBrushSetPressureSize(GtkWidget* button, AIR_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_SIZE);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_SIZE;
	}
}

static void AirBrushSetPressureFlow(GtkWidget* button, AIR_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_FLOW);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_FLOW;
	}
}

static void AirBrushSetBlendMode(GtkComboBox* combo, AIR_BRUSH* brush)
{
	brush->blend_mode = (uint16)gtk_combo_box_get_active(combo);
}

static void AirBrushSetAntiAlias(GtkWidget* button, AIR_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_ANTI_ALIAS);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_ANTI_ALIAS;
	}
}

static GtkWidget* CreateAirBrushDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	AIR_BRUSH *brush = (AIR_BRUSH*)core->brush_data;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox;
	GtkWidget *brush_scale;
	GtkWidget *check_button;
	GtkWidget *combo;
	GtkWidget *label;
	GtkAdjustment *scale_adjustment;
	char mark_up_buff[256];
	int i;

	brush->core = core;

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		combo = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[2]);
#else
		combo = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), brush->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(brush->base_scale)
	{
	case 0:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(AirBrushSetScale), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.brush_scale, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(combo), "scale", brush_scale);
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(SetBrushBaseScale), &brush->base_scale);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->opacity, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(AirBrushSetFlow), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.flow, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->outline_hardness, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(AirBrushSetOutlineHardness), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.outline_hardness, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->blur, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(AirBrushSetBlur), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.blur, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->enter, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(AirBrushSetEnterSize), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.enter, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->out, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(AirBrushSetOutSize), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.out, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
#if MAJOR_VERSION == 1
	combo = gtk_combo_box_new_text();
#else
	combo = gtk_combo_box_text_new();
#endif
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->layer_window.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
#if MAJOR_VERSION == 1
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), app->labels->layer_window.blend_modes[i]);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), app->labels->layer_window.blend_modes[i]);
#endif
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), brush->blend_mode);
	(void)g_signal_connect(G_OBJECT(combo), "changed",
		G_CALLBACK(AirBrushSetBlendMode), brush);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(AirBrushSetPressureSize), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(AirBrushSetPressureFlow), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(AirBrushSetAntiAlias), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_ANTI_ALIAS);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);

	return vbox;
#undef UI_FONT_SIZE
}

static void OldAirBrushPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		OLD_AIR_BRUSH* pen = (OLD_AIR_BRUSH*)core->brush_data;
		gdouble r, alpha;
		gdouble min_x, min_y, max_x, max_y;
		uint8* color = *core->color;
		cairo_pattern_t *brush;

		if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
		{
			window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		}
		else
		{
			window->work_layer->layer_mode = LAYER_BLEND_ATOP;
		}

		r = ((pen->flags & BRUSH_FLAG_SIZE) == 0) ?
			pen->r : pen->r * pressure;
		alpha = ((pen->flags & BRUSH_FLAG_FLOW) == 0) ?
			pen->opacity * 0.01 : pen->opacity * 0.01 * pressure;

		min_x = x - r, min_y = y - r;
		max_x = x + r, max_y = y + r;

		pen->before_x = x, pen->before_y = y;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		brush = cairo_pattern_create_radial(x, y, r * 0.0, x, y, r);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
		cairo_pattern_add_color_stop_rgba(brush, 0.0, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1-pen->blur*0.01f, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha*pen->outline_hardness*0.01f);
		cairo_set_source(window->work_layer->cairo_p, brush);

		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{
			cairo_arc(window->work_layer->cairo_p, x, y, r, 0, 2*G_PI);
			cairo_fill(window->work_layer->cairo_p);
		}
		else
		{
			cairo_mask_surface(window->work_layer->cairo_p, window->selection->surface_p, 0, 0);
		}

		cairo_pattern_destroy(brush);
	}
}

#define OldAirBrushEditSelectionPressCallBack OldAirBrushPressCallBack

static void OldAirBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		OLD_AIR_BRUSH* pen = (OLD_AIR_BRUSH*)core->brush_data;
		gdouble r, half_r, alpha, d, arg;
		gdouble min_x, min_y, max_x, max_y;
		gdouble draw_x = x, draw_y = y;
		gdouble dx, dy, cos_x, sin_y;
		gdouble hardness = pen->outline_hardness * 0.01f;
		int32 clear_x, clear_width, clear_y, clear_height;
		uint8* color = *core->color;
		cairo_pattern_t *brush;
		uint8 alpha_c;
		int i, j, k;

		r = ((pen->flags & BRUSH_FLAG_SIZE) == 0) ?
			pen->r : pen->r * pressure;
		half_r = 1;//r * 0.125;
		alpha = ((pen->flags & BRUSH_FLAG_FLOW) == 0) ?
			pen->opacity * 0.01 : pen->opacity * 0.01 * pressure;
		alpha_c = (uint8)(alpha * 255);
		dx = x-pen->before_x, dy = y-pen->before_y;
		d = sqrt(dx*dx+dy*dy);
		arg = atan2(dy, dx);
		cos_x = cos(arg), sin_y = sin(arg);

		min_x = x - r*2, min_y = y - r*2;
		max_x = x + r*2, max_y = y + r*2;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		dx = d;
		do
		{
			clear_x = (int32)(draw_x - r - 1);
			clear_width = (int32)(draw_x + r + 1);
			clear_y = (int32)(draw_y - r - 1);
			clear_height = (int32)(draw_y + r + 1);

			if(clear_x < 0)
			{
				clear_x = 0;
			}
			else if(clear_x >= window->width)
			{
				goto skip_draw;
			}
			if(clear_y < 0)
			{
				clear_y = 0;
			}
			if(clear_height >= window->height)
			{
				clear_height = window->height;
			}
			clear_width = clear_width - clear_x;
			clear_height = clear_height - clear_y;

			if(clear_width <= 0 || clear_height <= 0)
			{
				goto skip_draw;
			}

			for(i=0; i<clear_height; i++)
			{
				(void)memset(&window->temp_layer->pixels[(i+clear_y)*window->work_layer->stride+clear_x*window->work_layer->channel],
					0x0, clear_width*window->work_layer->channel);
			}
			brush = cairo_pattern_create_radial(draw_x, draw_y, r * 0.0, draw_x, draw_y, r);
			cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
			cairo_pattern_add_color_stop_rgba(brush, 0.0, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1-pen->blur*0.01f, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1.0, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha*hardness);
			cairo_set_source(window->temp_layer->cairo_p, brush);

			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				cairo_arc(window->temp_layer->cairo_p, draw_x, draw_y, r, 0, 2*G_PI);
				cairo_fill(window->temp_layer->cairo_p);
			}
			else
			{
				cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);
			}
		
			for(i=0; i<clear_height; i++)
			{
				for(j=0; j<clear_width; j++)
				{
					if(window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3]
						> window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+3])
					{
						for(k=0; k<window->temp_layer->channel; k++)
						{
							window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k] =
								(uint32)(((int)window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+k]
								- (int)window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k])
									* window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3] >> 8)
									+ window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k];
						}
					}
				}
			}

			cairo_pattern_destroy(brush);

skip_draw:
			dx -= half_r;
			
			draw_x -= cos_x*half_r, draw_y -= sin_y*half_r;
		} while (dx >= half_r);

		pen->before_x = x, pen->before_y = y;
	}
}

#define OldAirBrushEditSelectionMotionCallBack OldAirBrushMotionCallBack

#define OldAirBrushReleaseCallBack DefaultReleaseCallBack
#define OldAirBrushEditSelectionReleaseCallBack DefaultEditSelectionReleaseCallBack

static void OldAirBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	OLD_AIR_BRUSH* pen = (OLD_AIR_BRUSH*)data;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, pen->r*window->zoom*0.01, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);
}

static void OldAirBrushScaleChange(
	GtkAdjustment* slider,
	OLD_AIR_BRUSH* brush
)
{
	brush->r = gtk_adjustment_get_value(slider) * 0.5;
}

static void OldAirBrushFlowChange(
	GtkAdjustment* slider,
	OLD_AIR_BRUSH* brush
)
{
	brush->opacity = gtk_adjustment_get_value(slider);
}

static void OldAirBrushOutlineHardnessChange(
	GtkAdjustment* slider,
	OLD_AIR_BRUSH* brush
)
{
	brush->outline_hardness = gtk_adjustment_get_value(slider);
}

static void OldAirBrushBlurChange(
	GtkAdjustment* slider,
	OLD_AIR_BRUSH* brush
)
{
	brush->blur = gtk_adjustment_get_value(slider);
}

static void OldAirBrushPressureSizeChange(
	GtkWidget* widget,
	OLD_AIR_BRUSH* brush
)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_SIZE);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_SIZE;
	}
}

static void OldAirBrushPressureFlowChange(
	GtkWidget* widget,
	OLD_AIR_BRUSH* brush
)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_FLOW);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_FLOW;
	}
}

static GtkWidget* CreateOldAirBrushDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	OLD_AIR_BRUSH* brush = (OLD_AIR_BRUSH*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* brush_scale;
	GtkWidget* table = gtk_table_new(4, 3, TRUE);
	GtkWidget* check_button;
	GtkWidget* label;
	GtkAdjustment* brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(brush->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
	char mark_up_buff[256];

	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(OldAirBrushScaleChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);

	brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(brush->opacity, 0.0, 100.0, 1.0, 1.0, 0.0));
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.flow, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(OldAirBrushFlowChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 1, 2);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->outline_hardness,
		0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(OldAirBrushOutlineHardnessChange), core->brush_data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.outline_hardness, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 2, 3);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->blur,
		0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(OldAirBrushBlurChange), core->brush_data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.blur, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 3, 4);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(OldAirBrushPressureSizeChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(OldAirBrushPressureFlowChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	return vbox;
}

static const cairo_operator_t g_layer_blend_operator[] =
{
	CAIRO_OPERATOR_OVER,
	CAIRO_OPERATOR_ADD,
	CAIRO_OPERATOR_MULTIPLY,
	CAIRO_OPERATOR_SCREEN,
	CAIRO_OPERATOR_OVERLAY,
	CAIRO_OPERATOR_LIGHTEN,
	CAIRO_OPERATOR_DARKEN,
	CAIRO_OPERATOR_COLOR_DODGE,
	CAIRO_OPERATOR_COLOR_BURN,
	CAIRO_OPERATOR_HARD_LIGHT,
	CAIRO_OPERATOR_SOFT_LIGHT,
	CAIRO_OPERATOR_DIFFERENCE,
	CAIRO_OPERATOR_EXCLUSION,
	CAIRO_OPERATOR_HSL_HUE,
	CAIRO_OPERATOR_HSL_SATURATION,
	CAIRO_OPERATOR_HSL_COLOR,
	CAIRO_OPERATOR_HSL_LUMINOSITY
};

static void BlendBrushButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		BLEND_BRUSH *brush = (BLEND_BRUSH*)core->brush_data;
		// �X�V�͈͂̃C���[�W���
		cairo_surface_t *update_surface;
		cairo_t *update;
		// �u���V�̔��a�ƕs�����x
		FLOAT_T r, zoom, alpha;
		// ���W�̍ő�l�E�ŏ��l
		FLOAT_T min_x, min_y, max_x, max_y;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �Q�ƃs�N�Z��
		uint8 *mask, *mask_pix;
		// �`��ʒu�w��p
		cairo_matrix_t matrix;
		// �ꎞ�����p
		cairo_surface_t *draw_surface;
		int i, j;	// for���p�̃J�E���^

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;

		if(brush->target == BLEND_BRUSH_TARGET_UNDER_LAYER && window->active_layer->prev != NULL)
		{
			(void)memcpy(window->brush_buffer, window->active_layer->prev->pixels, window->pixel_buf_size);
		}
		else
		{
			(void)memcpy(window->brush_buffer, window->mixed_layer->pixels, window->pixel_buf_size);
		}

		if((brush->flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = brush->r;
			zoom = 1;
		}
		else
		{
			r = brush->r * pressure;
			zoom = 1/pressure;
		}

		brush->before_x = x, brush->before_y = y;
		brush->points[0][1] = x, brush->points[0][2] = y;
		brush->points[0][3] = pressure;
		brush->sum_distance = brush->travel = brush->finish_length = 0;
		brush->draw_start = r * brush->out * 0.01 * 4;
		brush->enter_length = r * brush->enter * 0.01 * 4;
		brush->enter_size = r * (1 - brush->enter * 0.01);
		brush->num_point = 0;
		brush->draw_finished = 0;
		brush->ref_point = 1;

		min_x = x - r, max_x = x + r;
		min_y = y - r, max_y = y + r;

		update_surface = cairo_surface_create_for_rectangle(
			window->mask_temp->surface_p, min_x, min_y, r*2+1, r*2+1);
		update = cairo_create(update_surface);

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		window->update.x = (int)min_x, window->update.y = (int)min_y;
		window->update.width = (int)max_x - window->update.x;
		window->update.height = (int)max_y - window->update.y;

		window->flags |= DRAW_WINDOW_UPDATE_PART;

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		if(brush->enter_length == 0)
		{
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;
			start_x = (int)min_x, start_y = (int)min_y;
			width = (int)(max_x - min_x);
			height = (int)(max_y - min_y);
			stride = width*4;

			if(max_x > window->width || max_y > window->height
				|| max_x < 0 || max_y < 0)
			{
				return;
			}

			for(i=0; i<height; i++)
			{
				(void)memset(&window->mask_temp->pixels[
					(i+start_y)*window->mask_temp->stride+start_x*4], 0, stride);
			}

			cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);

			mask = window->mask_temp->pixels;
			if(window->app->textures.active_texture == 0)
			{
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0,0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - x + r, - y + r);
					}
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(window->mask_temp->cairo_p,
							window->selection->surface_p, - x + r, - y + r);
					}
					else
					{
						cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
						cairo_t *update_temp = cairo_create(temp_surface);

						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - x + r, - y + r);

						mask = window->temp_layer->pixels;

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}
				}
			}
			else
			{
				cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
					window->temp_layer->surface_p, min_x, min_y, r*2+1, r*2+1);
				cairo_t *update_temp = cairo_create(temp_surface);

				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - x + r, - y + r);
					}

					for(i=0; i<height; i++)
					{
						(void)memset(&window->temp_layer->pixels[
							(i+start_y)*window->temp_layer->stride+start_x*4],
							0, stride
						);
					}

					cairo_set_source_surface(update_temp, update_surface, 0, 0);
					cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

					mask = window->temp_layer->pixels;
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_set_source_surface(update_temp, update_surface, 0, 0);
						cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

						mask = window->temp_layer->pixels;
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->temp_layer->pixels[
								(i+start_y)*window->temp_layer->stride+start_x*4],
								0, stride
							);
						}

						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - x + r, - y + r);

						for(i=0; i<height; i++)
						{
							(void)memset(&window->mask_temp->pixels[
								(i+start_y)*window->mask_temp->stride+start_x*4],
								0, stride
							);
						}

						cairo_set_source_surface(update, temp_surface, 0, 0);
						cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
					}
				}

				cairo_surface_destroy(temp_surface);
				cairo_destroy(update_temp);
			}

			cairo_surface_destroy(update_surface);
			cairo_destroy(update);

			if(mask == window->mask_temp->pixels)
			{
				update_surface = cairo_surface_create_for_rectangle(
					window->mask_temp->surface_p, start_x, start_y, width, height);
			}
			else
			{
				update_surface = cairo_surface_create_for_rectangle(
					window->temp_layer->surface_p, start_x, start_y, width, height);
			}
			draw_surface = cairo_image_surface_create_for_data(window->mask->pixels,
				CAIRO_FORMAT_ARGB32, width, height, stride);
			for(i=0; i<height; i++)
			{
				(void)memcpy(&window->mask->pixels[i*stride],
					&window->brush_buffer[(start_y+i)*window->stride+start_x*4], stride);
			}

			update = cairo_create(draw_surface);
			cairo_set_operator(update, g_layer_blend_operator[brush->blend_mode]);
			cairo_set_source_surface(update, update_surface, 0, 0);
			cairo_paint(update);

			cairo_destroy(update);
			cairo_surface_destroy(draw_surface);
			cairo_surface_destroy(update_surface);

			for(i=0; i<height; i++)
			{
				mask_pix = &mask[(start_y+i)*window->work_layer->stride+start_x*4+3];
				for(j=0; j<width; j++, mask_pix+=4)
				{
					window->work_layer->pixels[(start_y+i)*window->work_layer->stride+(start_x+j)*4] =
						(uint8)((uint32)(((int)window->mask->pixels[i*stride+j*4]) * *mask_pix >> 8));
					window->work_layer->pixels[(start_y+i)*window->work_layer->stride+(start_x+j)*4+1] =
						(uint8)((uint32)(((int)window->mask->pixels[i*stride+j*4+1]) * *mask_pix >> 8));
					window->work_layer->pixels[(start_y+i)*window->work_layer->stride+(start_x+j)*4+2] =
						(uint8)((uint32)(((int)window->mask->pixels[i*stride+j*4+2]) * *mask_pix >> 8));
					window->work_layer->pixels[(start_y+i)*window->work_layer->stride+(start_x+j)*4+3] =
						*mask_pix;
				}
			}
		}	// if(brush->enter_length == 0)
	}	// ���N���b�N�Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

#define BlendBrushEditSelectionButtonPressCallBack BlendBrushButtonPressCallBack

static void BlendBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^����������Ă�����
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		BLEND_BRUSH *brush = (BLEND_BRUSH*)core->brush_data;
		FLOAT_T distance;
		int index = brush->ref_point % BRUSH_POINT_BUFFER_SIZE;

		distance = sqrt((x-brush->before_x)*(x-brush->before_x)
			+(y-brush->before_y)*(y-brush->before_y));

		if(distance >= 1.0)
		{
			brush->before_x = x, brush->before_y = y;
			brush->sum_distance += distance;
			brush->travel += distance;
			brush->points[index][0] = distance;
			brush->points[index][1] = x, brush->points[index][2] = y;
			brush->points[index][3] = pressure;
			brush->ref_point++;

			if(brush->sum_distance >= brush->draw_start
				&& brush->sum_distance >= brush->enter_length)
			{
				// �X�V�͈͂̃C���[�W���
				cairo_surface_t *update_surface;
				cairo_t *update;
				// �u���V�̕M���ɂ��k���p
				gdouble zoom;
				// ����̐F�␳�p
				gdouble enter_alpha;
				// �u���V�̔��a�ƕs�����x
				gdouble r, alpha;
				// ���W�̍ő�E�ŏ��l
				gdouble min_x, min_y, max_x, max_y;
				// X�AY�����̈ړ���
				gdouble dx, dy;
				// �u���V�̌X���p
				gdouble diff_x, diff_y;
				// �u���V�̈ړ���
				gdouble d, step;
				// �`����s�����W
				gdouble draw_x = brush->before_x, draw_y = brush->before_y;
				// �`����W�w��p
				cairo_matrix_t matrix;
				// �ꎞ�����p
				cairo_surface_t *draw_surface;
				// �s�N�Z���f�[�^�����Z�b�g������W
				int start_x, start_y;
				// �`�悷�镝�A�����A��s���̃o�C�g��
				int width, height, stride;
				// �Q�ƃs�N�Z��
				uint8 *ref_pix, *mask_pix, *mask, *alpha_pix;
				// �`��F�̃s�N�Z���l
				uint8 draw_value;
				// �z��̃C���f�b�N�X�p
				int ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				int before_point;
				int i, j;	// for���p�̃J�E���^

				while(brush->sum_distance > brush->draw_start
					&& brush->draw_finished < brush->ref_point)
				{
					if(brush->finish_length < brush->enter_length)
					{
						enter_alpha = brush->finish_length / brush->enter_length;
						r = brush->enter_size + brush->finish_length * 0.25;
					}
					else
					{
						zoom = enter_alpha = 1, r = brush->r;
					}
					
					if((brush->flags & BRUSH_FLAG_SIZE) != 0)
					{
						r *= brush->points[ref_point][3];
					}

					
					alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
						? 1 : brush->points[ref_point][3];
					alpha *= enter_alpha;
					before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
					d = brush->points[ref_point][0];
					brush->sum_distance -= d;

					if(r > MIN_BRUSH_STEP)
					{
						zoom = brush->r / r;
						step = r * BRUSH_STEP;
						if(step < 0.5)
						{
							step = 0.5;
						}

						if(brush->draw_finished == 0)
						{
							draw_x = brush->points[0][1], draw_y = brush->points[0][2];
						}
						else
						{
							draw_x = brush->points[before_point][1];
							draw_y = brush->points[before_point][2];
						}
						dx = brush->points[ref_point][1] - draw_x;
						dy = brush->points[ref_point][2] - draw_y;
						diff_x = step*dx/d, diff_y = step*dy/d;

						min_x = brush->points[ref_point][1] - r - 1;
						max_x = brush->points[ref_point][1] + r + 1;
						min_y = brush->points[ref_point][2] - r - 1;
						max_y = brush->points[ref_point][2] + r + 1;

						if(min_x < 0.0)
						{
							min_x = 0.0;
						}
						if(core->min_x > min_x)
						{
							core->min_x = min_x;
						}
						if(min_y < 0.0)
						{
							min_y = 0.0;
						}
						if(core->min_y > min_y)
						{
							core->min_y = min_y;
						}
						if(max_x > window->work_layer->width)
						{
							max_x = window->work_layer->width;
						}
						if(core->max_x < max_x)
						{
							core->max_x = max_x;
						}
						if(max_y > window->work_layer->height)
						{
							min_y = window->work_layer->height;
						}
						if(core->max_y < max_y)
						{
							core->max_y = max_y;
						}

						if(window->update.x > min_x)
						{
							window->update.width += window->update.x - (int)min_x;
							window->update.x = (int)min_x;
						}
						if(window->update.width + window->update.x < max_x)
						{
							window->update.width += (int)max_x - window->update.width + window->update.x;
						}
						if(window->update.y > min_y)
						{
							window->update.height += window->update.y - (int)min_y;
							window->update.y = (int)min_y;
						}
						if(window->update.height + window->update.y < max_y)
						{
							window->update.height += (int)max_y - window->update.height + window->update.y;
						}

						dx = d;
						do
						{
							start_x = (int)(draw_x - r);
							start_y = (int)(draw_y - r);
							width = (int)(draw_x + r);
							height = (int)(draw_y + r);

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(start_x > window->work_layer->width)
							{
								goto skip_draw;
							}
							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(start_y > window->work_layer->height)
							{
								goto skip_draw;
							}
							if(width > window->work_layer->width)
							{
								width = window->work_layer->width - start_x;
							}
							else
							{
								width = width - start_x;
							}
							if(height > window->work_layer->height)
							{
								height = window->work_layer->height - start_y;
							}
							else
							{
								height = height - start_y;
							}
							stride = width*4;

							if(width <= 0 || height <= 0)
							{
								goto skip_draw;
							}

							update_surface = cairo_surface_create_for_rectangle(
								window->mask_temp->surface_p, draw_x - r, draw_y - r,
									r*2+1, r*2+1);
							update = cairo_create(update_surface);

							window->flags |= DRAW_WINDOW_UPDATE_PART;

							for(i=0; i<height; i++)
							{
								(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
									0, stride);
							}

							cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);

							mask = window->mask_temp->pixels;
							if(window->app->textures.active_texture == 0)
							{
								if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(update, core->brush_pattern);
										cairo_paint_with_alpha(update, alpha);
									}
									else
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
									}
								}
								else
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);
									}
									else
									{
										cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
											window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
										cairo_t *update_temp = cairo_create(temp_surface);

										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);

										for(i=0; i<height; i++)
										{
											(void)memset(&window->temp_layer->pixels[
												(i+start_y)*window->temp_layer->stride+start_x*4],
												0, stride
											);
										}

										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);
										cairo_set_source_surface(update_temp,
											update_surface, 0, 0);
										cairo_mask_surface(update_temp,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
	
										mask = window->temp_layer->pixels;

										cairo_surface_destroy(temp_surface);
										cairo_destroy(update_temp);
									}
								}
							}
							else
							{
								cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
									window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
								cairo_t *update_temp = cairo_create(temp_surface);

								if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(update, core->brush_pattern);
										cairo_paint_with_alpha(update, alpha);
									}
									else
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
									}

									for(i=0; i<height; i++)
									{
										(void)memset(&window->temp_layer->pixels[
											(i+start_y)*window->temp_layer->stride+start_x*4],
											0, stride
										);
									}

									cairo_set_source_surface(update_temp, update_surface, 0, 0);
									cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

									mask = window->temp_layer->pixels;
								}
								else
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);

										for(i=0; i<height; i++)
										{
											(void)memset(&window->temp_layer->pixels[
												(i+start_y)*window->temp_layer->stride+start_x*4],
												0, stride
											);
										}

										cairo_set_source_surface(update_temp, update_surface, 0, 0);
										cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

										mask = window->temp_layer->pixels;
									}
									else
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);

										for(i=0; i<height; i++)
										{
											(void)memset(&window->temp_layer->pixels[
												(i+start_y)*window->temp_layer->stride+start_x*4],
												0, stride
											);
										}

										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);
										cairo_set_source_surface(update_temp,
											update_surface, 0, 0);
										cairo_mask_surface(update_temp,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
	
										for(i=0; i<height; i++)
										{
											(void)memset(&window->mask_temp->pixels[
												(i+start_y)*window->mask_temp->stride+start_x*4],
												0, stride
											);
										}

										cairo_set_source_surface(update, temp_surface, 0, 0);
										cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
									}
								}

								cairo_surface_destroy(temp_surface);
								cairo_destroy(update_temp);
							}

							cairo_surface_destroy(update_surface);
							cairo_destroy(update);

							if(mask == window->mask_temp->pixels)
							{
								update_surface = cairo_surface_create_for_rectangle(
									window->mask_temp->surface_p, start_x, start_y, width, height);
							}
							else
							{
								update_surface = cairo_surface_create_for_rectangle(
									window->temp_layer->surface_p, start_x, start_y, width, height);
							}
							draw_surface = cairo_image_surface_create_for_data(window->mask->pixels,
								CAIRO_FORMAT_ARGB32, width, height, stride);
							for(i=0; i<height; i++)
							{
								(void)memcpy(&window->mask->pixels[i*stride],
									&window->brush_buffer[(start_y+i)*window->stride+start_x*4], stride);
							}

							update = cairo_create(draw_surface);
							cairo_set_operator(update, g_layer_blend_operator[brush->blend_mode]);
							cairo_set_source_surface(update, update_surface, 0, 0);
							cairo_paint(update);

							cairo_destroy(update);
							cairo_surface_destroy(draw_surface);
							cairo_surface_destroy(update_surface);

							for(i=0; i<height; i++)
							{
								ref_pix = &window->work_layer->pixels[
									(start_y+i)*window->work_layer->stride+start_x*4];
								mask_pix = &window->mask->pixels[i*stride];
								alpha_pix = &mask[(start_y+i)*window->work_layer->stride
									+start_x*4+3];

								for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4, alpha_pix+=4)
								{
									if(ref_pix[3] < *alpha_pix)
									{
										draw_value = (uint8)((uint32)(((int)mask_pix[0]) * *alpha_pix >> 8));
										if(draw_value > ref_pix[0])
										{
											ref_pix[0] = (uint8)((uint32)(((int)draw_value-(int)ref_pix[0])
												* *alpha_pix >> 8) + ref_pix[0]);
										}
										draw_value = (uint8)((uint32)(((int)mask_pix[1]) * *alpha_pix >> 8));
										if(draw_value > ref_pix[1])
										{
											ref_pix[1] = (uint8)((uint32)(((int)draw_value-(int)ref_pix[1])
												* *alpha_pix >> 8) + ref_pix[1]);
										}
										draw_value = (uint8)((uint32)(((int)mask_pix[2]) * *alpha_pix >> 8));
										if(draw_value > ref_pix[2])
										{
											ref_pix[2] = (uint8)((uint32)(((int)draw_value-(int)ref_pix[2])
												* *alpha_pix >> 8) + ref_pix[2]);
										}
										ref_pix[3] = (uint8)((uint32)(((int)*alpha_pix-(int)ref_pix[3])
											* *alpha_pix >> 8) + ref_pix[3]);
									}
								}
							}

							// �A���`�G�C���A�X��K�p
							if((brush->flags & BRUSH_FLAG_ANTI_ALIAS) != 0)
							{
								ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
									width + 1, height + 1};
								AntiAliasLayer(window->work_layer, window->temp_layer, &range);
							}
skip_draw:
							dx -= step;
							if(dx < 1)
							{
								break;
							}
							else if(dx >= step)
							{
								draw_x += diff_x, draw_y += diff_y;
							}
							else
							{
								draw_x = brush->points[ref_point][1];
								draw_y = brush->points[ref_point][2];
							}
						} while(1);
					}

					brush->finish_length += d;
					brush->travel += d;
					brush->draw_finished++;
					ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				}	// while(brush->sum_distance > brush->draw_start
						// && brush->draw_finished < brush->ref_point)
			}	// if(brush->sum_distance >= brush->draw_start
					// && brush->sum_distance >= brush->enter_length)
		}	// if(distance >= 1.0)
	}	// �}�E�X�̍��{�^����������Ă�����
		// if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
}

#define BlendBrushEditSelectionMotionCallBack BlendBrushMotionCallBack

static void BlendBrushButtonReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^���Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		BLEND_BRUSH *brush = (BLEND_BRUSH*)core->brush_data;
		// �X�V�͈͂̃C���[�W���
		cairo_surface_t *update_surface;
		cairo_t *update;
		// �u���V�̏k����
		gdouble zoom;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̈ړ���
		gdouble d, step;
		// �`����s�����W
		gdouble draw_x, draw_y;
		// ����A�������̐F�␳
		gdouble enter_alpha, out_alpha;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �`����W�w��p
		cairo_matrix_t matrix;
		// �ꎞ�����p
		cairo_surface_t *draw_surface;
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask, *alpha_pix;
		// �`�悷��s�N�Z���l
		uint8 draw_value;
		// �z��̃C���f�b�N�X�p
		int ref_point;
		int before_point;
		int i, j;	// for���p�̃J�E���^

		while(brush->ref_point > brush->draw_finished)
		{
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;

			if(brush->finish_length < brush->enter_length)
			{
				enter_alpha = brush->finish_length / brush->enter_length;
				r = brush->enter_size + brush->finish_length * 0.25;
			}
			else
			{
				enter_alpha = 1;
				r = brush->r;
			}
			if(brush->sum_distance < brush->draw_start)
			{
				out_alpha = brush->sum_distance / brush->draw_start;
				r -= (brush->draw_start - brush->sum_distance) * 0.25;
			}
			else
			{
				out_alpha = 1;
			}

			if(r < 0.0)
			{
				brush->draw_finished++;
				continue;
			}

			if((brush->flags & BRUSH_FLAG_SIZE) != 0)
			{
				r *= brush->points[ref_point][3];
			}
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
				? 1 : brush->points[ref_point][3];
			alpha *= enter_alpha * out_alpha;
			before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
			if(r > MIN_BRUSH_STEP)
			{
				d = brush->points[ref_point][0];
				brush->sum_distance -= d;
				step = r * BRUSH_STEP;
				zoom = brush->r / r;

				if(step < 0.5)
				{
					step = 0.5;
				}
				if(brush->draw_finished == 0)
				{
					draw_x = brush->points[0][1], draw_y = brush->points[0][2];
				}
				else
				{
					draw_x = brush->points[before_point][1], draw_y = brush->points[before_point][2];
				}
				dx = brush->points[ref_point][1] - draw_x;
				dy = brush->points[ref_point][2] - draw_y;
				diff_x = step * dx / d, diff_y = step * dy / d;

				min_x = brush->points[ref_point][1] - r - 1;
				max_x = brush->points[ref_point][1] + r + 1;
				min_y = brush->points[ref_point][2] - r - 1;
				max_y = brush->points[ref_point][2] + r + 1;
				if(min_x < 0.0)
				{
					min_x = 0.0;
				}
				if(core->min_x > min_x)
				{
					core->min_x = min_x;
				}
				if(min_y < 0.0)
				{
					min_y = 0.0;
				}
				if(core->min_y > min_y)
				{
					core->min_y = min_y;
				}
				if(max_x > window->work_layer->width)
				{
					max_x = window->work_layer->width;
				}
				if(core->max_x < max_x)
				{
					core->max_x = max_x;
				}
				if(max_y > window->work_layer->height)
				{
					min_y = window->work_layer->height;
				}
				if(core->max_y < max_y)
				{
					core->max_y = max_y;
				}

				dx = d;
				do
				{
					start_x = (int)(draw_x - r);
					start_y = (int)(draw_y - r);
					width = (int)(draw_x + r);
					height = (int)(draw_y + r);

					if(start_x < 0)
					{
						start_x = 0;
					}
					else if(start_x > window->work_layer->width)
					{
						goto skip_draw;
					}
					if(start_y < 0)
					{
						start_y = 0;
					}
					else if(start_y > window->work_layer->height)
					{
						goto skip_draw;
					}
					if(width > window->work_layer->width)
					{
						width = window->work_layer->width - start_x;
					}
					else
					{
						width = width - start_x;
					}
					if(height > window->work_layer->height)
					{
						height = window->work_layer->height - start_y;
					}
					else
					{
						height = height - start_y;
					}
					stride = width*4;

					if(width <= 0 || height <= 0)
					{
						goto skip_draw;
					}

					update_surface = cairo_surface_create_for_rectangle(
						window->mask_temp->surface_p, draw_x - r, draw_y - r,
							r*2+1, r*2+1);
					update = cairo_create(update_surface);

					if(window->update.x > min_x)
					{
						window->update.width += window->update.x - (int)min_x;
						window->update.x = (int)min_x;
					}
					if(window->update.width + window->update.x < max_x)
					{
						window->update.width += (int)max_x - window->update.width + window->update.x;
					}
					if(window->update.y > min_y)
					{
						window->update.height += window->update.y - (int)min_y;
						window->update.y = (int)min_y;
					}
					if(window->update.height + window->update.y < max_y)
					{
						window->update.height += (int)max_y - window->update.height + window->update.y;
					}

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride);
					}

					cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);

					mask = window->mask_temp->pixels;
					if(window->app->textures.active_texture == 0)
					{
						if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(update, core->brush_pattern);
								cairo_paint_with_alpha(update, alpha);
							}
							else
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
							}
						}
						else
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);
							}
							else
							{
								cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
									window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
								cairo_t *update_temp = cairo_create(temp_surface);

								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);

								for(i=0; i<height; i++)
								{
									(void)memset(&window->temp_layer->pixels[
										(i+start_y)*window->temp_layer->stride+start_x*4],
										0, stride
									);
								}

								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);
								cairo_set_source_surface(update_temp,
									update_surface, 0, 0);
								cairo_mask_surface(update_temp,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
	
								mask = window->temp_layer->pixels;

								cairo_surface_destroy(temp_surface);
								cairo_destroy(update_temp);
							}
						}
					}
					else
					{
						cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, start_x, start_y, r*2+1, r*2+1);
						cairo_t *update_temp = cairo_create(temp_surface);

						if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(update, core->brush_pattern);
								cairo_paint_with_alpha(update, alpha);
							}
							else
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
							}

							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}

							cairo_set_source_surface(update_temp, update_surface, 0, 0);
							cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

							mask = window->temp_layer->pixels;
						}
						else
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);

								for(i=0; i<height; i++)
								{
									(void)memset(&window->temp_layer->pixels[
										(i+start_y)*window->temp_layer->stride+start_x*4],
										0, stride
									);
								}

								cairo_set_source_surface(update_temp, update_surface, 0, 0);
								cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

								mask = window->temp_layer->pixels;
							}
							else
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);

								for(i=0; i<height; i++)
								{
									(void)memset(&window->temp_layer->pixels[
										(i+start_y)*window->temp_layer->stride+start_x*4],
										0, stride
									);
								}

								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);
								cairo_set_source_surface(update_temp,
									update_surface, 0, 0);
								cairo_mask_surface(update_temp,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
	
								for(i=0; i<height; i++)
								{
									(void)memset(&window->mask_temp->pixels[
										(i+start_y)*window->mask_temp->stride+start_x*4],
										0, stride
									);
								}

								cairo_set_source_surface(update, temp_surface, 0, 0);
								cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
							}
						}

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}

					cairo_surface_destroy(update_surface);
					cairo_destroy(update);

					if(mask == window->mask_temp->pixels)
					{
						update_surface = cairo_surface_create_for_rectangle(
							window->mask_temp->surface_p, start_x, start_y, width, height);
					}
					else
					{
						update_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, start_x, start_y, width, height);
					}
					draw_surface = cairo_image_surface_create_for_data(window->mask->pixels,
						CAIRO_FORMAT_ARGB32, width, height, stride);
					for(i=0; i<height; i++)
					{
						(void)memcpy(&window->mask->pixels[i*stride],
							&window->brush_buffer[(start_y+i)*window->stride+start_x*4], stride);
					}

					update = cairo_create(draw_surface);
					cairo_set_operator(update, g_layer_blend_operator[brush->blend_mode]);
					cairo_set_source_surface(update, update_surface, 0, 0);
					cairo_paint(update);

					cairo_destroy(update);
					cairo_surface_destroy(draw_surface);
					cairo_surface_destroy(update_surface);

					for(i=0; i<height; i++)
					{
						ref_pix = &window->work_layer->pixels[
							(start_y+i)*window->work_layer->stride+start_x*4];
						mask_pix = &window->mask->pixels[i*stride];
						alpha_pix = &mask[(start_y+i)*window->work_layer->stride
							+start_x*4+3];

						for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4, alpha_pix+=4)
						{
							if(ref_pix[3] < *alpha_pix)
							{
								draw_value = (uint8)((uint32)(((int)mask_pix[0]) * *alpha_pix >> 8));
								if(draw_value > ref_pix[0])
								{
									ref_pix[0] = (uint8)((uint32)(((int)draw_value-(int)ref_pix[0])
										* *alpha_pix >> 8) + ref_pix[0]);
								}
								draw_value = (uint8)((uint32)(((int)mask_pix[1]) * *alpha_pix >> 8));
								if(draw_value > ref_pix[1])
								{
									ref_pix[1] = (uint8)((uint32)(((int)draw_value-(int)ref_pix[1])
										* *alpha_pix >> 8) + ref_pix[1]);
								}
								draw_value = (uint8)((uint32)(((int)mask_pix[2]) * *alpha_pix >> 8));
								if(draw_value > ref_pix[2])
								{
									ref_pix[2] = (uint8)((uint32)(((int)draw_value-(int)ref_pix[2])
										* *alpha_pix >> 8) + ref_pix[2]);
								}
								ref_pix[3] = (uint8)((uint32)(((int)*alpha_pix-(int)ref_pix[3])
									* *alpha_pix >> 8) + ref_pix[3]);
							}
						}
					}

					// �A���`�G�C���A�X��K�p
					if((brush->flags & BRUSH_FLAG_ANTI_ALIAS) != 0)
					{
						ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
							width + 1, height + 1};
						AntiAliasLayer(window->work_layer, window->temp_layer, &range);
					}
skip_draw:
					dx -= step;
					if(dx < 1)
					{
						break;
					}
					else if(dx >= step)
					{
						draw_x += diff_x, draw_y += diff_y;
					}
					else
					{
						draw_x = brush->points[ref_point][1];
						draw_y = brush->points[ref_point][2];
					}
				} while(1);
			}

			brush->finish_length += d;
			brush->travel += d;
			brush->draw_finished++;
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
		}	// while(brush->ref_point > brush->draw_finished)

		AddBrushHistory(core, window->active_layer);

		window->update.surface_p = cairo_surface_create_for_rectangle(
			window->active_layer->surface_p, window->update.x, window->update.y,
				window->update.width, window->update.height);
		window->update.cairo_p = cairo_create(window->update.surface_p);
		g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->update);
		cairo_surface_destroy(window->update.surface_p);
		cairo_destroy(window->update.cairo_p);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}	// �}�E�X�̍��{�^���Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

static void BlendBrushEditSelectionButtonReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^���Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		BLEND_BRUSH *brush = (BLEND_BRUSH*)core->brush_data;
		// �u���V�̏k����
		gdouble zoom;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̈ړ���
		gdouble d, step;
		// �`����s�����W
		gdouble draw_x, draw_y;
		// ����A�������̐F�␳
		gdouble enter_alpha, out_alpha;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �`����W�w��p
		cairo_matrix_t matrix;
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask;
		// �z��̃C���f�b�N�X�p
		int ref_point;
		int before_point;
		int i, j;	// for���p�̃J�E���^

		while(brush->ref_point > brush->draw_finished)
		{
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;

			if(brush->finish_length < brush->enter_length)
			{
				enter_alpha = brush->finish_length / brush->enter_length;
				r = brush->enter_size + brush->finish_length * 0.25;
			}
			else
			{
				enter_alpha = 1;
				r = brush->r;
			}
			if(brush->sum_distance < brush->draw_start)
			{
				out_alpha = brush->sum_distance / brush->draw_start;
				r -= (brush->draw_start - brush->sum_distance) * 0.25;
			}
			else
			{
				out_alpha = 1;
			}

			if(r < 0.0)
			{
				brush->draw_finished++;
				continue;
			}

			if((brush->flags & BRUSH_FLAG_SIZE) != 0)
			{
				r *= brush->points[ref_point][3];
			}
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
				? 1 : brush->points[ref_point][3];
			alpha *= enter_alpha * out_alpha;
			before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
			if(r > MIN_BRUSH_STEP)
			{
				d = brush->points[ref_point][0];
				brush->sum_distance -= d;
				step = r * BRUSH_STEP;
				if(step < 0.5)
				{
					step = 0.5;
				}
				zoom = brush->r / r;
				if(brush->draw_finished == 0)
				{
					draw_x = brush->points[0][1], draw_y = brush->points[0][2];
				}
				else
				{
					draw_x = brush->points[before_point][1], draw_y = brush->points[before_point][2];
				}
				dx = brush->points[ref_point][1] - draw_x;
				dy = brush->points[ref_point][2] - draw_y;
				diff_x = step * dx / d, diff_y = step * dy / d;

				min_x = brush->points[ref_point][1] - r - 1;
				max_x = brush->points[ref_point][1] + r + 1;
				min_y = brush->points[ref_point][2] - r - 1;
				max_y = brush->points[ref_point][2] + r + 1;
				if(min_x < 0.0)
				{
					min_x = 0.0;
				}
				if(core->min_x > min_x)
				{
					core->min_x = min_x;
				}
				if(min_y < 0.0)
				{
					min_y = 0.0;
				}
				if(core->min_y > min_y)
				{
					core->min_y = min_y;
				}
				if(max_x > window->work_layer->width)
				{
					max_x = window->work_layer->width;
				}
				if(core->max_x < max_x)
				{
					core->max_x = max_x;
				}
				if(max_y > window->work_layer->height)
				{
					min_y = window->work_layer->height;
				}
				if(core->max_y < max_y)
				{
					core->max_y = max_y;
				}

				dx = d;
				do
				{
					start_x = (int)(draw_x - r);
					start_y = (int)(draw_y - r);
					width = (int)(draw_x + r);
					height = (int)(draw_y + r);

					if(start_x < 0)
					{
						start_x = 0;
					}
					else if(start_x > window->work_layer->width)
					{
						goto skip_draw;
					}
					if(start_y < 0)
					{
						start_y = 0;
					}
					else if(start_y > window->work_layer->height)
					{
						goto skip_draw;
					}
					if(width > window->work_layer->width)
					{
						width = window->work_layer->width - start_x;
					}
					else
					{
						width = width - start_x;
					}
					if(height > window->work_layer->height)
					{
						height = window->work_layer->height - start_y;
					}
					else
					{
						height = height - start_y;
					}
					stride = width*4;

					if(width <= 0 || height <= 0)
					{
						goto skip_draw;
					}

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride);
					}

					cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);

					mask = window->mask_temp->pixels;
					if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_matrix_translate(&matrix, - draw_x + r, - draw_y + r);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(window->mask_temp->cairo_p, core->brush_pattern);
							cairo_paint_with_alpha(window->mask_temp->cairo_p, alpha);
						}
						else
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(window->mask_temp->cairo_p, core->temp_pattern);
							cairo_mask_surface(window->mask_temp->cairo_p,
								window->active_layer->surface_p, 0, 0);
						}
					}
					else
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(window->mask_temp->cairo_p, core->temp_pattern);
							cairo_mask_surface(window->mask_temp->cairo_p,
								window->selection->surface_p, 0, 0);
						}
						else
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(window->mask_temp->cairo_p, core->temp_pattern);

							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}

							cairo_mask_surface(window->mask_temp->cairo_p,
								window->selection->surface_p, 0, 0);
							cairo_set_source_surface(window->temp_layer->cairo_p,
								window->mask_temp->surface_p, 0, 0);
							cairo_mask_surface(window->temp_layer->cairo_p,
								window->active_layer->surface_p, 0, 0);
	
							mask = window->temp_layer->pixels;
						}
					}

					for(i=0; i<height; i++)
					{
						ref_pix = &window->work_layer->pixels[
							(start_y+i)*window->work_layer->stride+start_x*4];
						mask_pix = &mask[(start_y+i)*window->work_layer->stride
							+start_x*4];

						for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
						{
							if(ref_pix[3] < mask_pix[3])
							{
								ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
									* mask_pix[3] >> 8) + ref_pix[0]);
								ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
									* mask_pix[3] >> 8) + ref_pix[1]);
								ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
									* mask_pix[3] >> 8) + ref_pix[2]);
								ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
									* mask_pix[3] >> 8) + ref_pix[3]);
							}
						}
					}

					// �A���`�G�C���A�X��K�p
					if((brush->flags & BRUSH_FLAG_ANTI_ALIAS) != 0)
					{
						ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
							width + 1, height + 1};
						AntiAliasLayer(window->work_layer, window->temp_layer, &range);
					}
skip_draw:
					dx -= step;
					if(dx < 1)
					{
						break;
					}
					else if(dx >= step)
					{
						draw_x += diff_x, draw_y += diff_y;
					}
					else
					{
						draw_x = brush->points[ref_point][1];
						draw_y = brush->points[ref_point][2];
					}
				} while(1);
			}

			brush->finish_length += d;
			brush->travel += d;
			brush->draw_finished++;
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
		}	// while(brush->ref_point > brush->draw_finished)

		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[window->selection->layer_mode](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}	// �}�E�X�̍��{�^���Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

static void BlendBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	BLEND_BRUSH* brush = (BLEND_BRUSH*)data;
	gdouble r = brush->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void BlendBrushButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, BLEND_BRUSH* brush)
{
	gdouble r = brush->r * window->zoom_rate + 1;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2) + 2, (gint)(r * 2) + 2);
}

static void BlendBrushMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, BLEND_BRUSH* brush)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = brush->r * window->zoom_rate * 1.275 + BRUSH_UPDATE_MARGIN;
	gdouble enter = (brush->enter_length + brush->draw_start) * window->zoom_rate;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	start_x -= enter * 4;
	width += enter * 4 * 2 + 2;
	start_y -= enter * 4;
	height += enter * 4 * 2 + 2;

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width, (gint)height);
}

static void BlendBrushColorChange(const uint8 color[3], void* data)
{
	BLEND_BRUSH *brush = (BLEND_BRUSH*)data;
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void BlendBrushSetScale(GtkAdjustment* slider, BLEND_BRUSH* brush)
{
	brush->r = gtk_adjustment_get_value(slider) * 0.5;
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void BlendBrushSetFlow(GtkAdjustment* slider, BLEND_BRUSH* brush)
{
	brush->opacity = gtk_adjustment_get_value(slider);
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void BlendBrushSetBlur(GtkAdjustment* slider, BLEND_BRUSH* brush)
{
	brush->blur = gtk_adjustment_get_value(slider);
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void BlendBrushSetOutlineHardness(GtkAdjustment* slider, BLEND_BRUSH* brush)
{
	brush->outline_hardness = gtk_adjustment_get_value(slider);
	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);
}

static void BlendBrushSetEnterSize(GtkAdjustment* slider, BLEND_BRUSH* brush)
{
	brush->enter = gtk_adjustment_get_value(slider);
}

static void BlendBrushSetOutSize(GtkAdjustment* slider, BLEND_BRUSH* brush)
{
	brush->out = gtk_adjustment_get_value(slider);
}

static void BlendBrushSetPressureSize(GtkWidget* button, BLEND_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_SIZE);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_SIZE;
	}
}

static void BlendBrushSetPressureFlow(GtkWidget* button, BLEND_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_FLOW);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_FLOW;
	}
}

static void BlendBrushSetBlendMode(GtkComboBox* combo, BLEND_BRUSH* brush)
{
	brush->blend_mode = (uint16)gtk_combo_box_get_active(combo);
}

static void BlendBrushSetBlendTarget(GtkWidget* button, BLEND_BRUSH* brush)
{
	int target = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "target"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		brush->target = (uint8)target;
	}
}

static void BlendBrushSetAntiAlias(GtkWidget* button, BLEND_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_ANTI_ALIAS);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_ANTI_ALIAS;
	}
}

static GtkWidget* CreateBlendBrushDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	BLEND_BRUSH *brush = (BLEND_BRUSH*)core->brush_data;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox;
	GtkWidget *brush_scale;
	GtkWidget *check_button;
	GtkWidget *buttons[2];
	GtkWidget *combo;
	GtkWidget *label;
	GtkAdjustment *scale_adjustment;
	char mark_up_buff[256];
	int i;

	brush->core = core;

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		combo = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[2]);
#else
		combo = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), brush->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(brush->base_scale)
	{
	case 0:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BlendBrushSetScale), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.brush_scale, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(combo), "scale", brush_scale);
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(SetBrushBaseScale), &brush->base_scale);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->opacity, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BlendBrushSetFlow), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.flow, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->outline_hardness, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BlendBrushSetOutlineHardness), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.outline_hardness, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->blur, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BlendBrushSetBlur), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.blur, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->enter, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BlendBrushSetEnterSize), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.enter, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->out, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(BlendBrushSetOutSize), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.out, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
#if MAJOR_VERSION == 1
	combo = gtk_combo_box_new_text();
#else
	combo = gtk_combo_box_text_new();
#endif
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->layer_window.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
#if MAJOR_VERSION == 1
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), app->labels->layer_window.blend_modes[i]);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), app->labels->layer_window.blend_modes[i]);
#endif
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), brush->blend_mode);
	(void)g_signal_connect(G_OBJECT(combo), "changed",
		G_CALLBACK(BlendBrushSetBlendMode), brush);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.select.target);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.under_layer);
	g_object_set_data(G_OBJECT(buttons[0]), "target", GINT_TO_POINTER(BLEND_BRUSH_TARGET_UNDER_LAYER));
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(BlendBrushSetBlendTarget), brush);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
		GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.select.canvas);
	g_object_set_data(G_OBJECT(buttons[1]), "target", GINT_TO_POINTER(BLEND_BRUSH_TARGET_CANVAS));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(BlendBrushSetBlendTarget), brush);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[brush->target]), TRUE);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(BlendBrushSetPressureSize), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(BlendBrushSetPressureFlow), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(BlendBrushSetAntiAlias), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_ANTI_ALIAS);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	BrushCoreSetCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01, brush->blur * 0.01,
		brush->opacity * 0.01, *brush->core->color);

	return vbox;
#undef UI_FONT_SIZE
}

static void WaterColorBrushPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ��
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		WATER_COLOR_BRUSH *brush = (WATER_COLOR_BRUSH*)core->brush_data;
		// �X�V�͈͂̃C���[�W���
		cairo_t *update;
		cairo_surface_t *update_surface;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// �u���V�̊g�嗦
		gdouble zoom;
		// ���W�̍ő�l�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�E����
		int width, height;
		// �u���V�̈ʒu�ݒ�p
		cairo_matrix_t matrix;
		// �u���V�ʒu�̃s�N�Z���l���v
		unsigned int sum_color[6] = {0, 0, 0, 1, 0, 1};
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask;
		// �`�悷��F
		int color[4];
		// ���u�����h�p
		uint8 mask_value;
		int rev_alpha;
		uint8 blend_alpha;
		// ���F�
		uint8 mix = (uint8)(brush->mix * 2.55 + 0.5);
		int i, j;	// for���p�̃J�E���^

		(void)memcpy(window->work_layer->pixels, window->active_layer->pixels, window->pixel_buf_size);
		(void)memset(window->brush_buffer, 0, window->pixel_buf_size);

		window->work_layer->layer_mode = LAYER_BLEND_SOURCE;

		brush->before_x = brush->last_draw_x = x;
		brush->before_y = brush->last_draw_y = y;
		brush->points[0][1] = x, brush->points[0][2] = y;
		brush->points[0][3] = pressure;

		if((brush->flags & BRUSH_FLAG_SIZE) != 0)
		{
			r = brush->r * pressure;
			zoom = 1/pressure;
		}
		else
		{
			r = brush->r;
			zoom = 1;
		}

		brush->sum_distance = brush->travel = brush->finish_length = 0.0;
		brush->draw_start = r * brush->out * 0.01 * 4;
		brush->enter_length = r * brush->enter * 0.01 * 4;
		brush->enter_size = r * (1 - brush->enter * 0.01);
		brush->num_point = 0;
		brush->draw_finished = 0;
		brush->ref_point = 1;

		min_x = x - r, max_x = x + r;
		min_y = y - r, max_y = y + r;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		window->update.x = (int)min_x, window->update.y = (int)min_y;
		window->update.width = (int)max_x - window->update.x;
		window->update.height = (int)max_y - window->update.y;

		window->flags |= DRAW_WINDOW_UPDATE_PART;

		if(brush->enter_length == 0 && r > 0.5)
		{
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;
			start_x = (int)min_x, start_y = (int)min_y;
			width = (int)(max_x - min_x);
			height = (int)(max_y - min_y);

			update_surface = cairo_surface_create_for_rectangle(
				window->alpha_surface, x - r, y - r, r*2+1, r*2+1
			);
			update = cairo_create(update_surface);

			(void)memset(window->mask->pixels, 0, window->width*window->height);

			mask = window->mask->pixels;
			cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
			if(window->app->textures.active_texture == 0)
			{
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - x + r, - y + r);
					}
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);
					}
					else
					{
						cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
							window->alpha_temp, x - r, y - r, r*2+1, r*2+1);
						cairo_t *update_temp = cairo_create(temp_surface);

						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						(void)memset(window->temp_layer->pixels, 0,
							window->width*window->height);

						cairo_mask_surface(update,
							window->selection->surface_p, 0, 0);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - x + r, - y + r);

						mask = window->temp_layer->pixels;

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}
				}
			}
			else
			{
				cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
					window->alpha_temp, x - r, y - r, r*2+1, r*2+1);
				cairo_t *update_temp = cairo_create(temp_surface);

				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - x + r, - y + r);
					}

					cairo_set_source_surface(update_temp, update_surface, 0, 0);
					cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

					mask = window->temp_layer->pixels;
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);

						cairo_set_source_surface(update_temp, update_surface, 0, 0);
						cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

						mask = window->temp_layer->pixels;
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						(void)memset(window->temp_layer->pixels, 0,
							window->width*window->height);

						cairo_mask_surface(update,
							window->selection->surface_p, 0, 0);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - x + r, - y + r);

						cairo_set_operator(update, CAIRO_OPERATOR_SOURCE);
						cairo_set_source_surface(update, temp_surface, 0, 0);
						cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
					}
				}

				cairo_surface_destroy(temp_surface);
				cairo_destroy(update_temp);
			}

			cairo_surface_destroy(update_surface);
			cairo_destroy(update);

			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
				mask_pix = &mask[(i+start_y)*window->width+start_x];
				for(j=0; j<width; j++, ref_pix+=4, mask_pix++)
				{
					sum_color[0] += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
					sum_color[1] += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
					sum_color[2] += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
					sum_color[3] += ((ref_pix[3]+1) * *mask_pix) >> 8;
					sum_color[4] += ref_pix[3] * *mask_pix;
					sum_color[5] += *mask_pix;
				}
			}

			color[0] = (sum_color[0] + sum_color[3] / 2) / sum_color[3];
			if(color[0] > 0xff)
			{
				color[0] = 0xff;
			}
			color[1] = (sum_color[1] + sum_color[3] / 2) / sum_color[3];
			if(color[1] > 0xff)
			{
				color[1] = 0xff;
			}
			color[2] = (sum_color[2] + sum_color[3] / 2) / sum_color[3];
			if(color[2] > 0xff)
			{
				color[2] = 0xff;
			}
			color[3] = (uint8)(sum_color[4] / sum_color[5]);

			// �`�悷��F������
			rev_alpha = 0xff-mix+1;
			blend_alpha = (uint8)(brush->alpha * 2.55 + 0.5);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			color[0] = (rev_alpha*(*core->color)[2]+(mix+1)*color[0])>>8;
			color[1] = (rev_alpha*(*core->color)[1]+(mix+1)*color[1])>>8;
			color[2] = (rev_alpha*(*core->color)[0]+(mix+1)*color[2])>>8;
			color[3] = (rev_alpha*blend_alpha+(mix+1)*color[3])>>8;
#else
			color[0] = (rev_alpha*(*core->color)[0]+(mix+1)*color[0])>>8;
			color[1] = (rev_alpha*(*core->color)[1]+(mix+1)*color[1])>>8;
			color[2] = (rev_alpha*(*core->color)[2]+(mix+1)*color[2])>>8;
			color[3] = (rev_alpha*blend_alpha+(mix+1)*color[3])>>8;
#endif

			// ���݂̐F���L��
			brush->before_color[0] = color[0];
			brush->before_color[1] = color[1];
			brush->before_color[2] = color[2];
			brush->before_color[3] = color[3];

			// ��ƃ��C���[�ɕ`��
			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
				mask_pix = &mask[(i+start_y)*window->width+start_x];

				for(j=0; j<width; j++, ref_pix+=4, mask_pix++)
				{
					mask_value = *mask_pix;
					ref_pix[3] = ((mask_value+1)*color[3]+(0xff-mask_value+1)*ref_pix[3])>>8;

					ref_pix[0] = (uint8)(((mask_value+1)*color[0]+(0xff-mask_value+1)*ref_pix[0])>>8);
					ref_pix[1] = (uint8)(((mask_value+1)*color[1]+(0xff-mask_value+1)*ref_pix[1])>>8);
					ref_pix[2] = (uint8)(((mask_value+1)*color[2]+(0xff-mask_value+1)*ref_pix[2])>>8);
				}
			}

			(void)memcpy(window->brush_buffer, mask, window->width*window->height);

			for(i=0; i<height; i++)
			{
				(void)memcpy(&window->temp_layer->pixels[(i+start_y)*window->stride+start_x*4],
					&window->work_layer->pixels[(i+start_y)*window->stride+start_x*4], width*4);
			}
			update_surface = cairo_surface_create_for_rectangle(window->work_layer->surface_p,
				start_x, start_y, width, height);
			update = cairo_create(update_surface);
			cairo_set_source_surface(update, window->temp_layer->surface_p, start_x, start_y);
			cairo_mask_surface(update, window->temp_layer->surface_p, start_x, start_y);
			cairo_surface_destroy(update_surface);
			cairo_destroy(update);

			brush->draw_finished++;
		}
	}
}

static void WaterColorBrushEditSelectionPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ��
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		WATER_COLOR_BRUSH *brush = (WATER_COLOR_BRUSH*)core->brush_data;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// �u���V�̊g�嗦
		gdouble zoom;
		// �u���V�̍��W�w��p
		cairo_matrix_t matrix;
		// ���W�̍ő�l�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�E����
		int width, height;
		// �u���V�ʒu�̃s�N�Z���l���v
		unsigned int sum_color[2] = {1, 1};
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *comp_pix, *mask;
		// �`�悷��F
		uint8 color[2];
		int rev_alpha;
		uint8 blend_alpha;
		// ���F�
		uint8 mix = (uint8)(brush->mix * 2.55);
		int i, j;	// for���p�̃J�E���^

		(void)memcpy(window->work_layer->pixels, window->selection->pixels, window->width * window->height);
		(void)memset(window->brush_buffer, 0, window->width*window->height);

		window->selection->layer_mode = SELECTION_BLEND_COPY;

		brush->before_x = brush->last_draw_x = x;
		brush->before_y = brush->last_draw_y = y;
		brush->points[0][1] = x, brush->points[0][2] = y;
		brush->points[0][3] = pressure;

		if((brush->flags & BRUSH_FLAG_SIZE) != 0)
		{
			r = brush->r * pressure;
			zoom = 1/pressure;
		}
		else
		{
			r = brush->r;
			zoom = 1;
		}

		brush->sum_distance = brush->travel = brush->finish_length = 0.0;
		brush->draw_start = r * brush->out * 0.01 * 4;
		brush->enter_length = r * brush->enter * 0.01 * 4;
		brush->enter_size = r * (1 - brush->enter * 0.01);
		brush->num_point = 0;
		brush->draw_finished = 0;
		brush->ref_point = 1;

		min_x = x - r, max_x = x + r;
		min_y = y - r, max_y = y + r;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		if(brush->enter_length == 0)
		{
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;
			start_x = (int)min_x, start_y = (int)min_y;
			width = (int)(max_x - min_x);
			height = (int)(max_y - min_y);

			(void)memset(window->mask->pixels, 0, window->width*window->height);

			mask = window->mask->pixels;
			cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_matrix_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->brush_pattern);
					cairo_paint_with_alpha(window->alpha_cairo, alpha);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->temp_pattern);
					cairo_mask_surface(window->alpha_cairo,
						window->active_layer->surface_p, 0, 0);
				}
			}
			else
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->temp_pattern);
					cairo_mask_surface(window->alpha_cairo,
						window->selection->surface_p, 0, 0);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->temp_pattern);

					(void)memset(window->temp_layer->pixels, 0, window->width*window->height);

					cairo_mask_surface(window->alpha_cairo,
						window->selection->surface_p, 0, 0);
					cairo_set_source_surface(window->alpha_temp_cairo,
						window->alpha_surface, 0, 0);
					cairo_mask_surface(window->alpha_temp_cairo,
						window->active_layer->surface_p, 0, 0);

					mask = window->temp_layer->pixels;
				}
			}

			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
				mask_pix = &mask[(i+start_y)*window->width+start_x];
				for(j=0; j<width; j++, ref_pix++, mask_pix++)
				{
					sum_color[0] += ((ref_pix[0]+1) * *mask_pix) >> 8;
					sum_color[1] += *mask_pix;
				}
			}

			color[0] = (uint8)((sum_color[0] + (sum_color[1] / 255) / 2) / (sum_color[1] / 255.0));

			// �`�悷��F������
			rev_alpha = 0xff-mix+1;
			blend_alpha = (uint8)(brush->alpha * 2.55);
			color[0] = (rev_alpha*blend_alpha+mix*color[0])>>8;

			// ���݂̐F���L��
			brush->before_color[0] = color[0];

			// ��ƃ��C���[�ɕ`��
			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
				mask_pix = &mask[(i+start_y)*window->width+start_x];
				comp_pix = &window->brush_buffer[(i+start_y)*window->width+start_x];

				for(j=0; j<width; j++, ref_pix++, mask_pix++, comp_pix++)
				{
					*comp_pix = mask_pix[0];
					blend_alpha = ((mask_pix[0]+1)*(color[0])+(0xff-mask_pix[0]+1)*ref_pix[0])>>8;
					if(blend_alpha != 0)
					{
						ref_pix[0] = blend_alpha;
					}
				}
			}

			(void)memcpy(window->brush_buffer, mask, window->width*window->height);

			brush->draw_finished++;
		}
	}
}

static void WaterColorBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		WATER_COLOR_BRUSH *brush = (WATER_COLOR_BRUSH*)core->brush_data;
		FLOAT_T distance;
		int index = brush->ref_point % BRUSH_POINT_BUFFER_SIZE;

		distance = sqrt((x-brush->before_x)*(x-brush->before_x)+(y-brush->before_y)*(y-brush->before_y));

		if(distance >= 1.0)
		{
			brush->before_x = x, brush->before_y = y;
			brush->sum_distance += distance;
			brush->travel += distance;
			brush->points[index][0] = distance;
			brush->points[index][1] = x;
			brush->points[index][2] = y;
			brush->points[index][3] = pressure;
			brush->ref_point++;

			if(brush->sum_distance >= brush->draw_start
				&& brush->sum_distance >= brush->enter_length)
			{
				// ����̐F�␳�p
				gdouble enter_alpha;
				// �X�V�͈͂̃C���[�W���
				cairo_t *update;
				cairo_surface_t *update_surface;
				// �u���V�̔��a�ƕs�����x
				gdouble r, alpha;
				// �u���V�̊g�嗦
				gdouble zoom;
				// �u���V�̈ʒu�w��p
				cairo_matrix_t matrix;
				// ���W�̍ő�E�ŏ��l
				gdouble min_x, min_y, max_x, max_y;
				// X�AY�����̈ړ���
				gdouble dx, dy;
				// �u���V�̌X���p
				gdouble diff_x, diff_y;
				// �u���V�̈ړ���
				gdouble d, step;
				// �`����s�����W
				gdouble draw_x = brush->before_x, draw_y = brush->before_y;
				// �s�N�Z���f�[�^�����Z�b�g������W
				int start_x, start_y;
				// �`�悷�镝�A�����A��s���̃o�C�g��
				int width, height;
				// �u���V�ʒu�̃s�N�Z���l���v
				unsigned int sum_color[6];
				// �Q�ƃs�N�Z��
				uint8 *ref_pix, *mask_pix, *comp_pix, *alpha_pix, *mask, *memory_pix;
				// ���u�����h�p
				int rev_alpha;
				uint8 blend_alpha;
				// �o�b�t�@�X�V�p
				int clear_start_x, clear_start_y;
				int clear_width, clear_height;
				// �`�悷��F
				int color[4];
				// �O��̐F
				uint8 before_color[4];
				// �}�X�N�̃o�C�g�l
				uint8 mask_value;
				// ���F�
				uint8 mix = (uint8)(brush->mix * 2.555);
				// �F���ы
				uint8 extend = (uint8)(brush->extend * 2.555);
				// �z��̃C���f�b�N�X�p
				int ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				int before_point;
				int i, j;	// for���p�̃J�E���^

				while(brush->sum_distance > brush->draw_start
					&& brush->draw_finished < brush->ref_point)
				{
					if(brush->finish_length < brush->enter_length)
					{
						enter_alpha = brush->finish_length / brush->enter_length;
						r = brush->enter_size + brush->finish_length * 0.25;
					}
					else
					{
						enter_alpha = 1;
						r = brush->r;
					}
					if((brush->flags & BRUSH_FLAG_SIZE) != 0)
					{
						r *= brush->points[ref_point][3];
					}

					d = brush->points[ref_point][0];
					if(r > 0.5)
					{
						zoom = brush->r / r;
						step = r * BRUSH_STEP;
						if(step < 1)
						{
							step = 1;
						}
						alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
							? 1 : brush->points[ref_point][3];
						alpha *= enter_alpha;
						before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
						brush->sum_distance -= d;
						if(brush->draw_finished == 0)
						{
							draw_x = brush->points[0][1], draw_y = brush->points[0][2];
						}
						else
						{
							draw_x = brush->points[before_point][1], draw_y = brush->points[before_point][2];
						}
						dx = brush->points[ref_point][1] - draw_x;
						dy = brush->points[ref_point][2] - draw_y;
						diff_x = step * dx / d, diff_y = step * dy / d;

						min_x = brush->points[ref_point][1] - r - 1;
						max_x = brush->points[ref_point][1] + r + 1;
						min_y = brush->points[ref_point][2] - r - 1;
						max_y = brush->points[ref_point][2] + r + 1;
						if(min_x < 0.0)
						{
							min_x = 0.0;
						}
						if(core->min_x > min_x)
						{
							core->min_x = min_x;
						}
							if(min_y < 0.0)
						{
								min_y = 0.0;
						}
						if(core->min_y > min_y)
						{
							core->min_y = min_y;
						}
							if(max_x > window->work_layer->width)
						{
							max_x = window->work_layer->width;
						}
							if(core->max_x < max_x)
						{
							core->max_x = max_x;
						}
						if(max_y > window->work_layer->height)
						{
							min_y = window->work_layer->height;
						}
						if(core->max_y < max_y)
						{
							core->max_y = max_y;
						}

						if(window->update.x > min_x)
						{
							window->update.width += window->update.x - (int)min_x;
							window->update.x = (int)min_x;
						}
						if(window->update.width + window->update.x < max_x)
						{
							window->update.width += (int)max_x - window->update.width + window->update.x;
						}
						if(window->update.x + window->update.width > window->width)
						{
							window->update.width = window->width - window->update.x;
						}

						if(window->update.y > min_y)
						{
							window->update.height += window->update.y - (int)min_y;
							window->update.y = (int)min_y;
						}
						if(window->update.height + window->update.y < max_y)
						{
							window->update.height += (int)max_y - window->update.height + window->update.y;
						}
						if(window->update.y + window->update.height > window->height)
						{
							window->update.height = window->height - window->update.y;
						}

						dx = d;
						do
						{
							start_x = (int)(draw_x - r);
							start_y = (int)(draw_y - r);
							width = (int)(draw_x + r+1);
							height = (int)(draw_y + r+1);

							if(width < 0 || height < 0)
							{
								goto skip_draw;
							}

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(start_x > window->work_layer->width)
							{
								goto skip_draw;
							}
							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(start_y > window->work_layer->height)
							{
								goto skip_draw;
							}
							if(width > window->work_layer->width)
							{
								width = window->work_layer->width - start_x;
							}
							else
							{
								width = width - start_x;
							}
							if(height > window->work_layer->height)
							{
								height = window->work_layer->height - start_y;
							}
							else
							{
								height = height - start_y;
							}

							(void)memset(window->mask->pixels, 0, window->width*window->height);

							update_surface = cairo_surface_create_for_rectangle(
								window->alpha_surface, draw_x - r, draw_y - r, r*2+1, r*2+1);
							update = cairo_create(update_surface);

							mask = window->mask->pixels;
							cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
							if(window->app->textures.active_texture == 0)
							{
								if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(update, core->brush_pattern);
										cairo_paint_with_alpha(update, alpha);
									}
									else
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
									}
								}
								else
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);
									}
									else
									{
										cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
											window->alpha_temp, draw_x - r, draw_y - r, r*2+1, r*2+1);
										cairo_t *update_temp = cairo_create(temp_surface);

										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);

										(void)memset(window->temp_layer->pixels, 0,
											window->width*window->height);

										cairo_mask_surface(update,
											window->selection->surface_p, 0, 0);
										cairo_set_source_surface(update_temp,
											update_surface, 0, 0);
										cairo_mask_surface(update_temp,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);

										mask = window->temp_layer->pixels;

										cairo_surface_destroy(temp_surface);
										cairo_destroy(update_temp);
									}
								}
							}
							else
							{
								cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
									window->alpha_temp, draw_x - r, draw_y - r, r*2+1, r*2+1);
								cairo_t *update_temp = cairo_create(temp_surface);

								if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(update, core->brush_pattern);
										cairo_paint_with_alpha(update, alpha);
									}
									else
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);
									}

									cairo_set_source_surface(update_temp, update_surface, 0, 0);
									cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

									mask = window->temp_layer->pixels;
								}
								else
								{
									if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);
										cairo_mask_surface(update,
											window->selection->surface_p, - draw_x + r, - draw_y + r);

										cairo_set_source_surface(update_temp, update_surface, 0, 0);
										cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

										mask = window->temp_layer->pixels;
									}
									else
									{
										cairo_matrix_init_scale(&matrix, zoom, zoom);
										cairo_pattern_set_matrix(core->brush_pattern, &matrix);
										cairo_set_source(core->temp_cairo, core->brush_pattern);
										cairo_paint_with_alpha(core->temp_cairo, alpha);
										cairo_matrix_init_translate(&matrix, 0, 0);
										cairo_pattern_set_matrix(core->temp_pattern, &matrix);
										cairo_set_source(update, core->temp_pattern);

										(void)memset(window->temp_layer->pixels, 0,
											window->width*window->height);

										cairo_mask_surface(update,
											window->selection->surface_p, 0, 0);
										cairo_set_source_surface(update_temp,
											update_surface, 0, 0);
										cairo_mask_surface(update_temp,
											window->active_layer->surface_p, - draw_x + r, - draw_y + r);

										cairo_set_operator(update, CAIRO_OPERATOR_SOURCE);
										cairo_set_source_surface(update, temp_surface, 0, 0);
										cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
									}
								}

								cairo_surface_destroy(temp_surface);
								cairo_destroy(update_temp);
							}

							cairo_surface_destroy(update_surface);
							cairo_destroy(update);

							sum_color[0] = sum_color[1] = sum_color[2] = sum_color[4] = 0;
							sum_color[3] = sum_color[5] = 1;
							for(i=0; i<height; i++)
							{
								ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
								mask_pix = &mask[(i+start_y)*window->width+start_x];
								for(j=0; j<width; j++, ref_pix+=4, mask_pix++)
								{
									sum_color[0] += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
									sum_color[1] += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
									sum_color[2] += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
									sum_color[3] += ((ref_pix[3]+1) * *mask_pix) >> 8;
									sum_color[4] += *mask_pix * ref_pix[3];
									sum_color[5] += *mask_pix;
								}
							}
							color[0] = (sum_color[0] + sum_color[3] / 2) / sum_color[3];
							if(color[0] > 0xff)
							{
								color[0] = 0xff;
							}
							color[1] = (sum_color[1] + sum_color[3] / 2) / sum_color[3];
							if(color[1] > 0xff)
							{
								color[1] = 0xff;
							}
							color[2] = (sum_color[2] + sum_color[3] / 2) / sum_color[3];
							if(color[2] > 0xff)
							{
								color[2] = 0xff;
							}
							color[3] = (uint8)((sum_color[4]) / sum_color[5]);

							// �`�悷��F������
							rev_alpha = 0xff-mix+1;
							blend_alpha = (uint8)(brush->alpha * 2.55);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
							color[0] = (rev_alpha*(*core->color)[2]+(mix+1)*color[0])>>8;
							color[1] = (rev_alpha*(*core->color)[1]+(mix+1)*color[1])>>8;
							color[2] = (rev_alpha*(*core->color)[0]+(mix+1)*color[2])>>8;
							color[3] = (rev_alpha*blend_alpha+(mix+1)*color[3])>>8;
#else
							color[0] = (rev_alpha*(*core->color)[0]+(mix+1)*color[0])>>8;
							color[1] = (rev_alpha*(*core->color)[1]+(mix+1)*color[1])>>8;
							color[2] = (rev_alpha*(*core->color)[2]+(mix+1)*color[2])>>8;
							color[3] = (rev_alpha*blend_alpha+(mix+1)*color[3])>>8;
#endif

							before_color[0] = color[0];
							before_color[1] = color[1];
							before_color[2] = color[2];
							before_color[3] = color[3];

							if(extend != 0 && ref_point != 0)
							{
								rev_alpha = 0xff-extend+1;
								color[0] = (rev_alpha*color[0]+(extend+1)*brush->before_color[0])>>8;
								color[1] = (rev_alpha*color[1]+(extend+1)*brush->before_color[1])>>8;
								color[2] = (rev_alpha*color[2]+(extend+1)*brush->before_color[2])>>8;
								color[3] = (rev_alpha*color[3]+(extend+1)*brush->before_color[3])>>8;
							}

							// ���݂̐F���L��
							brush->before_color[0] = before_color[0];
							brush->before_color[1] = before_color[1];
							brush->before_color[2] = before_color[2];
							brush->before_color[3] = before_color[3];

#define CLEAR_MARGINE 3
							//	���u�����h�p�̃o�b�t�@���X�V
							if(draw_x > brush->last_draw_x)
							{
								clear_start_x = (int)(brush->last_draw_x - brush->r * CLEAR_MARGINE);
								clear_width = (int)(draw_x + brush->r * CLEAR_MARGINE);
							}
							else
							{
								clear_start_x = (int)(draw_x - brush->r * CLEAR_MARGINE);
								clear_width = (int)(brush->last_draw_x + brush->r * CLEAR_MARGINE);
							}

							if(clear_start_x < 0)
							{
								clear_start_x = 0;
							}
							if(clear_width > window->width)
							{
								clear_width = window->width;
							}
							clear_width = clear_width - clear_start_x;

							if(draw_y > brush->last_draw_y)
							{
								clear_start_y = (int)(brush->last_draw_y - brush->r * CLEAR_MARGINE);
								clear_height = (int)(draw_y - brush->r * CLEAR_MARGINE);
							}
							else
							{
								clear_start_y = (int)(draw_y - brush->r * CLEAR_MARGINE);
								clear_height = (int)(brush->last_draw_y + brush->r * CLEAR_MARGINE);
							}

							if(clear_start_y < 0)
							{
								clear_start_y = 0;
							}
							if(clear_height > window->height)
							{
								clear_height = window->height;
							}

							for(i=clear_start_y; i<clear_height; i++)
							{
								(void)memset(&mask[window->width*window->height+i*window->width+clear_start_x],
									0, clear_width);
							}
							for(i=0; i<height; i++)
							{
								(void)memcpy(&mask[window->width*window->height+(i+start_y)*window->width+start_x],
									&window->brush_buffer[(i+start_y)*window->width+start_x], width);
							}
#undef CLEAR_MARGINE

#define COLOR_MARGINE 1
							for(i=0; i<height; i++)
							{
								ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
								comp_pix = &window->brush_buffer[(i+start_y)*window->width+start_x];
								alpha_pix = &mask[(i+start_y)*window->width+start_x];
								memory_pix = &mask[window->width*window->height+(i+start_y)*window->width+start_x];

								for(j=0; j<width; j++, ref_pix+=4, comp_pix++, alpha_pix++, memory_pix++)
								{
									mask_value = *alpha_pix;

									if(mask_value > *comp_pix)
									{
										*memory_pix = mask_value;

										blend_alpha /*= dst_value*/ = (mask_value - *comp_pix);
										*comp_pix = mask_value;
										ref_pix[3] = ((blend_alpha+1)*color[3]+(0xff-blend_alpha+1)*ref_pix[3])>>8;
											blend_alpha = min(ref_pix[3]*2, 0xff);

										ref_pix[0] = (uint8)min(
											(((mask_value+1)*color[0]+(0xff-mask_value+1)*ref_pix[0])>>8), blend_alpha);
										ref_pix[1] = (uint8)min(
											(((mask_value+1)*color[1]+(0xff-mask_value+1)*ref_pix[1])>>8), blend_alpha);
										ref_pix[2] = (uint8)min(
											(((mask_value+1)*color[2]+(0xff-mask_value+1)*ref_pix[2])>>8), blend_alpha);
									}
								}
							}

							for(i=clear_start_y; i<clear_height; i++)
							{
								(void)memcpy(&window->brush_buffer[i*window->width+clear_start_x],
									&mask[window->width*window->height+i*window->width+clear_start_x], clear_width);
							}
							brush->last_draw_x = draw_x, brush->last_draw_y = draw_y;

skip_draw:
							dx -= step;
							if(dx < 1)
							{
								break;
							}
							if(dx >= step)
							{
								draw_x += diff_x, draw_y += diff_y;
							}
							else
							{
								draw_x = brush->points[ref_point][1];
								draw_y = brush->points[ref_point][2];
							}
						} while(1);
					}

					brush->finish_length += d;
					brush->travel += d;
					brush->draw_finished++;
					ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				}	// while(brush->sum_distance >= brush->draw_start)
			}	// if(brush->sum_distance >= brush->draw_start
					// && brush->sum_distance >= brush->enter_length)
		}	// if(distance >= 1.0)
	}

	window->flags |= DRAW_WINDOW_UPDATE_PART;
}

static void WaterColorBrushEditSelectionMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		WATER_COLOR_BRUSH *brush = (WATER_COLOR_BRUSH*)core->brush_data;
		FLOAT_T distance;
		int index = brush->ref_point % BRUSH_POINT_BUFFER_SIZE;

		distance = sqrt((x-brush->before_x)*(x-brush->before_x)+(y-brush->before_y)*(y-brush->before_y));

		if(distance >= 1.0)
		{
			brush->before_x = x, brush->before_y = y;
			brush->sum_distance += distance;
			brush->travel += distance;
			brush->points[index][0] = distance;
			brush->points[index][1] = x;
			brush->points[index][2] = y;
			brush->points[index][3] = pressure;
			brush->ref_point++;

			if(brush->sum_distance >= brush->draw_start
				&& brush->sum_distance >= brush->enter_length)
			{
				// ����̐F�␳�p
				gdouble enter_alpha;
				// �u���V�̔��a�ƕs�����x
				gdouble r, alpha;
				// �u���V�̊g�嗦
				gdouble zoom;
				// �u���V�̍��W�w��p
				cairo_matrix_t matrix;
				// ���W�̍ő�E�ŏ��l
				gdouble min_x, min_y, max_x, max_y;
				// X�AY�����̈ړ���
				gdouble dx, dy;
				// �u���V�̌X���p
				gdouble diff_x, diff_y;
				// �u���V�̈ړ���
				gdouble d, step;
				// �`����s�����W
				gdouble draw_x = brush->before_x, draw_y = brush->before_y;
				// �s�N�Z���f�[�^�����Z�b�g������W
				int start_x, start_y;
				// �`�悷�镝�A����
				int width, height;
				// �u���V�ʒu�̃s�N�Z���l���v
				unsigned int sum_color[2];
				// �Q�ƃs�N�Z��
				uint8 *ref_pix, *mask_pix, *comp_pix, *alpha_pix, *mask;
				int rev_alpha;
				uint8 blend_alpha;
				gdouble inv_alpha;
				// �`�悷��F
				uint8 color[2];
				// �}�X�N�̃o�C�g�l
				uint8 mask_value;
				// ���F�
				uint8 mix = (uint8)(brush->mix * 2.55);
				// �F���ы
				uint8 extend = (uint8)(brush->extend * 2.55);
				// �z��̃C���f�b�N�X�p
				int ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				int before_point;
				int i, j;	// for���p�̃J�E���^

				while(brush->sum_distance > brush->draw_start
					&& brush->draw_finished < brush->ref_point)
				{
					if(brush->finish_length < brush->enter_length)
					{
						enter_alpha = brush->finish_length / brush->enter_length;
						r = brush->enter_size + brush->finish_length * 0.25;
					}
					else
					{
						enter_alpha = 1;
						r = brush->r;
					}
					if((brush->flags & BRUSH_FLAG_SIZE) != 0)
					{
						r *= brush->points[ref_point][3];
					}

					d = brush->points[ref_point][0];
					if(r > 0.5)
					{
						zoom = brush->r / r;
						step = r * BRUSH_STEP;
						if(step < 1)
						{
							step = 1;
						}
						alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
							? 1 : brush->points[ref_point][3];
						alpha *= enter_alpha;
						before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
						brush->sum_distance -= d;
						if(brush->draw_finished == 0)
						{
							draw_x = brush->points[0][1], draw_y = brush->points[0][2];
						}
						else
						{
							draw_x = brush->points[before_point][1], draw_y = brush->points[before_point][2];
						}
						dx = brush->points[ref_point][1] - draw_x;
						dy = brush->points[ref_point][2] - draw_y;
						diff_x = step * dx / d, diff_y = step * dy / d;

						min_x = brush->points[ref_point][1] - r - 1;
						max_x = brush->points[ref_point][1] + r + 1;
						min_y = brush->points[ref_point][2] - r - 1;
						max_y = brush->points[ref_point][2] + r + 1;
						if(min_x < 0.0)
						{
							min_x = 0.0;
						}
						if(core->min_x > min_x)
						{
							core->min_x = min_x;
						}
						if(min_y < 0.0)
						{
							min_y = 0.0;
						}
						if(core->min_y > min_y)
						{
							core->min_y = min_y;
						}
						if(max_x > window->work_layer->width)
						{
							max_x = window->work_layer->width;
						}
						if(core->max_x < max_x)
						{
							core->max_x = max_x;
						}
						if(max_y > window->work_layer->height)
						{
							min_y = window->work_layer->height;
						}
						if(core->max_y < max_y)
						{
							core->max_y = max_y;
						}

						dx = d;
						do
						{
							start_x = (int)(draw_x - r);
							start_y = (int)(draw_y - r);
							width = (int)(draw_x + r);
							height = (int)(draw_y + r);

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(start_x > window->work_layer->width)
							{
								goto skip_draw;
							}
							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(start_y > window->work_layer->height)
							{
								goto skip_draw;
							}
							if(width > window->work_layer->width)
							{
								width = window->work_layer->width - start_x;
							}
							else
							{
								width = width - start_x;
							}
							if(height > window->work_layer->height)
							{
								height = window->work_layer->height - start_y;
							}
							else
							{
								height = height - start_y;
							}

							(void)memset(window->mask->pixels, 0, window->width*window->height);

							mask = window->mask->pixels;
							cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
							if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
							{
								if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
								{
									cairo_matrix_init_scale(&matrix, zoom, zoom);
									cairo_matrix_translate(&matrix, - draw_x + r, - draw_y + r);
									cairo_pattern_set_matrix(core->brush_pattern, &matrix);
									cairo_set_source(window->alpha_cairo, core->brush_pattern);
									cairo_paint_with_alpha(window->alpha_cairo, alpha);
								}
								else
								{
									cairo_matrix_init_scale(&matrix, zoom, zoom);
									cairo_pattern_set_matrix(core->brush_pattern, &matrix);
									cairo_set_source(core->temp_cairo, core->brush_pattern);
									cairo_paint_with_alpha(core->temp_cairo, alpha);
									cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
									cairo_pattern_set_matrix(core->temp_pattern, &matrix);
									cairo_set_source(window->alpha_cairo, core->temp_pattern);
									cairo_mask_surface(window->alpha_cairo,
										window->active_layer->surface_p, 0, 0);
								}
							}
							else
							{
								if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
								{
									cairo_matrix_init_scale(&matrix, zoom, zoom);
									cairo_pattern_set_matrix(core->brush_pattern, &matrix);
									cairo_set_source(core->temp_cairo, core->brush_pattern);
									cairo_paint_with_alpha(core->temp_cairo, alpha);
									cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
									cairo_pattern_set_matrix(core->temp_pattern, &matrix);
									cairo_set_source(window->alpha_cairo, core->temp_pattern);
									cairo_mask_surface(window->alpha_cairo,
										window->selection->surface_p, 0, 0);
								}
								else
								{
									cairo_matrix_init_scale(&matrix, zoom, zoom);
									cairo_pattern_set_matrix(core->brush_pattern, &matrix);
									cairo_set_source(core->temp_cairo, core->brush_pattern);
									cairo_paint_with_alpha(core->temp_cairo, alpha);
									cairo_matrix_init_translate(&matrix, - x + r, - y + r);
									cairo_pattern_set_matrix(core->temp_pattern, &matrix);
									cairo_set_source(window->alpha_cairo, core->temp_pattern);

									(void)memset(window->temp_layer->pixels, 0, window->width*window->height);

									cairo_mask_surface(window->alpha_cairo,
										window->selection->surface_p, 0, 0);
									cairo_set_source_surface(window->alpha_temp_cairo,
										window->alpha_surface, 0, 0);
									cairo_mask_surface(window->alpha_temp_cairo,
										window->active_layer->surface_p, 0, 0);

									mask = window->temp_layer->pixels;
								}
							}

							sum_color[0] = sum_color[1] = 1;
							for(i=0; i<height; i++)
							{
								ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
								mask_pix = &mask[(i+start_y)*window->width+start_x];
								for(j=0; j<width; j++, ref_pix++, mask_pix++)
								{
									sum_color[0] += ((ref_pix[0]+1) * *mask_pix) >> 8;
									sum_color[1] += *mask_pix;
								}
							}
							color[0] = (uint8)((sum_color[0] + (sum_color[1] / 255) / 2) / (sum_color[1] / 255.0));

							// �`�悷��F������
							rev_alpha = 0xff-mix+1;
							blend_alpha = (uint8)(brush->alpha * 2.55);
							color[0] = (rev_alpha*blend_alpha+mix*color[0])>>8;

							if(extend != 0 && ref_point != 0)
							{
								rev_alpha = 0xff-extend+1;
								color[0] = (rev_alpha*color[0]+extend*brush->before_color[0])>>8;
							}

							// ���݂̐F���L��
							brush->before_color[0] = color[0];
							inv_alpha = color[0] * DIV_PIXEL;

							for(i=0; i<height; i++)
							{
								ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
								comp_pix = &window->brush_buffer[(i+start_y)*window->width+start_x];
								alpha_pix = &mask[(i+start_y)*window->width+start_x];

								for(j=0; j<width; j++, ref_pix++, mask_pix++, comp_pix++, alpha_pix++)
								{
									mask_value = *alpha_pix;
									*alpha_pix = (uint8)(*alpha_pix * inv_alpha);

									if(mask_value > 0)
									{
										if(mask_value == 0xff)
										{
											ref_pix[0] = *alpha_pix;
										}
										else
										{
											ref_pix[0] = (mask_value > ref_pix[0]) ? *alpha_pix : ref_pix[0];
										}
									}
								}
							}
							(void)memcpy(window->brush_buffer, mask, window->width*window->height);
skip_draw:
							dx -= step;
							if(dx < 1)
							{
								break;
							}
							else if(dx >= step)
							{
								draw_x += diff_x, draw_y += diff_y;
							}
							else
							{
								draw_x = brush->points[before_point][1];
								draw_y = brush->points[before_point][2];
							}
						} while(1);
					}

					brush->finish_length += d;
					brush->travel += d;
					brush->draw_finished++;
					ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				}	// while(brush->sum_distance >= brush->draw_start)
			}	// if(brush->sum_distance >= brush->draw_start
					// && brush->sum_distance >= brush->enter_length)
		}	// if(distance >= 1.0)
	}
}

static void WaterColorBrushReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ��
	if(((GdkEventButton*)state)->button == 1)
	{
		WATER_COLOR_BRUSH *brush = (WATER_COLOR_BRUSH*)core->brush_data;
		cairo_t *update;
		cairo_surface_t *update_surface;
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̊g�嗦
		gdouble zoom;
		// �u���V�̍��W�w��p
		cairo_matrix_t matrix;
		// �u���V�̈ړ���
		gdouble d, step;
		// �`����s�����W
		gdouble draw_x, draw_y;
		// ����A�������̐F�␳
		gdouble enter_alpha, out_alpha;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A����
		int width, height;
		// �u���V�ʒu�̃s�N�Z���l���v
		unsigned int sum_color[6];
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *comp_pix, *alpha_pix, *mask, *memory_pix;
		// ���u�����h�p
		int rev_alpha;
		uint8 blend_alpha;
		// �o�b�t�@�X�V�p
		int clear_start_x, clear_start_y;
		int clear_width, clear_height;
		// �`�悷��F
		int color[4];
		// �}�X�N�̃o�C�g�l
		uint8 mask_value;
		// ���F�
		uint8 mix = (uint8)(brush->mix * 2.55);
		// �F���ы
		uint8 extend = (uint8)(brush->extend * 2.55);
		// �z��̃C���f�b�N�X�p
		int ref_point;
		int before_point;
		int i, j;	// for���p�̃J�E���^

		while(brush->ref_point > brush->draw_finished)
		{
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;

			if(brush->finish_length < brush->enter_length)
			{
				enter_alpha = brush->finish_length / brush->enter_length;
				r = brush->enter_size + brush->finish_length * 0.25;
			}
			else
			{
				enter_alpha = 1;
				r = brush->r;
			}
			if(brush->sum_distance < brush->draw_start)
			{
				out_alpha = brush->sum_distance / brush->draw_start;
				r -= (brush->draw_start - brush->sum_distance) * 0.25;
			}
			else
			{
				out_alpha = 1;
			}
			
			if((brush->flags & BRUSH_FLAG_SIZE) != 0)
			{
				r *= brush->points[ref_point][3];
			}

			if(r < 0.5)
			{
				brush->draw_finished++;
				continue;
			}

			zoom = brush->r / r;
			step = r * BRUSH_STEP;
			if(step < 1)
			{
				step = 1;
			}
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
				? 1 : brush->points[ref_point][3];
			alpha *= enter_alpha;
			before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
			d = brush->points[ref_point][0];
			brush->sum_distance -= d;
			if(brush->draw_finished == 0)
			{
				draw_x = brush->points[0][1], draw_y = brush->points[0][2];
			}
			else
			{
				draw_x = brush->points[before_point][1], draw_y = brush->points[before_point][2];
			}
			dx = brush->points[ref_point][1] - draw_x;
			dy = brush->points[ref_point][2] - draw_y;
			diff_x = step * dx / d, diff_y = step * dy / d;

			min_x = brush->points[ref_point][1] - r - 1;
			max_x = brush->points[ref_point][1] + r + 1;
			min_y = brush->points[ref_point][2] - r - 1;
			max_y = brush->points[ref_point][2] + r + 1;
			if(min_x < 0.0)
			{
				min_x = 0.0;
			}
			if(core->min_x > min_x)
			{
				core->min_x = min_x;
			}
			
			if(min_y < 0.0)
			{
				min_y = 0.0;
			}
			if(core->min_y > min_y)
			{
				core->min_y = min_y;
			}
			if(max_x > window->work_layer->width)
			{
				max_x = window->work_layer->width;
			}
			if(core->max_x < max_x)
			{
				core->max_x = max_x;
			}
			if(max_y > window->work_layer->height)
			{
				min_y = window->work_layer->height;
			}
			if(core->max_y < max_y)
			{
				core->max_y = max_y;
			}

			if(window->update.x > min_x)
			{
				window->update.width += window->update.x - (int)min_x;
				window->update.x = (int)min_x;
			}
			if(window->update.width + window->update.x < max_x)
			{
				window->update.width += (int)max_x - window->update.width + window->update.x;
			}
			if(window->update.y > min_y)
			{
				window->update.height += window->update.y - (int)min_y;
				window->update.y = (int)min_y;
			}
			if(window->update.height + window->update.y < max_y)
			{
				window->update.height += (int)max_y - window->update.height + window->update.y;
			}

			dx = d;
			do
			{
				start_x = (int)(draw_x - r);
				start_y = (int)(draw_y - r);
				width = (int)(draw_x + r);
				height = (int)(draw_y + r);

				if(width < 0 || height < 0)
				{
					goto skip_draw;
				}

				if(start_x < 0)
				{
					start_x = 0;
				}
				else if(start_x > window->work_layer->width)
				{
					goto skip_draw;
				}
				if(start_y < 0)
				{
					start_y = 0;
				}
				else if(start_y > window->work_layer->height)
				{
					goto skip_draw;
				}
				if(width > window->work_layer->width)
				{
					width = window->work_layer->width - start_x;
				}
				else
				{
					width = width - start_x;
				}
				if(height > window->work_layer->height)
				{
					height = window->work_layer->height - start_y;
				}
				else
				{
					height = height - start_y;
				}

				(void)memset(window->mask->pixels, 0, window->width*window->height);

				update_surface = cairo_surface_create_for_rectangle(
					window->alpha_surface, draw_x - r, draw_y - r, r*2+1, r*2+1);
				update = cairo_create(update_surface);

				mask = window->mask->pixels;
				cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
				if(window->app->textures.active_texture == 0)
				{
					if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(update, core->brush_pattern);
							cairo_paint_with_alpha(update, alpha);
						}
						else
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, 0, 0);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(update, core->temp_pattern);
							cairo_mask_surface(update,
								window->active_layer->surface_p, - draw_x + r, - draw_y + r);
						}
					}
					else
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, 0, 0);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(update, core->temp_pattern);
							cairo_mask_surface(update,
								window->selection->surface_p, - draw_x + r, - draw_y + r);
						}
						else
						{
							cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
								window->alpha_temp, draw_x - r, draw_y - r, r*2+1, r*2+1);
							cairo_t *update_temp = cairo_create(temp_surface);

							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, 0, 0);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(update, core->temp_pattern);

							(void)memset(window->temp_layer->pixels, 0,
								window->width*window->height);

							cairo_mask_surface(update,
								window->selection->surface_p, 0, 0);
							cairo_set_source_surface(update_temp,
								update_surface, 0, 0);
							cairo_mask_surface(update_temp,
								window->active_layer->surface_p, - draw_x + r, - draw_y + r);

							mask = window->temp_layer->pixels;

							cairo_surface_destroy(temp_surface);
							cairo_destroy(update_temp);
						}
					}
				}
				else
				{
					cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
						window->alpha_temp, draw_x - r, draw_y - r, r*2+1, r*2+1);
					cairo_t *update_temp = cairo_create(temp_surface);

					if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(update, core->brush_pattern);
							cairo_paint_with_alpha(update, alpha);
						}
						else
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, 0, 0);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(update, core->temp_pattern);
							cairo_mask_surface(update,
								window->active_layer->surface_p, - draw_x + r, - draw_y + r);
						}

						cairo_set_source_surface(update_temp, update_surface, 0, 0);
						cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

						mask = window->temp_layer->pixels;
					}
					else
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, 0, 0);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(update, core->temp_pattern);
							cairo_mask_surface(update,
								window->selection->surface_p, - draw_x + r, - draw_y + r);

							cairo_set_source_surface(update_temp, update_surface, 0, 0);
							cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

							mask = window->temp_layer->pixels;
						}
						else
						{
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_matrix_init_translate(&matrix, 0, 0);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_set_source(update, core->temp_pattern);

							(void)memset(window->temp_layer->pixels, 0,
								window->width*window->height);

							cairo_mask_surface(update,
								window->selection->surface_p, 0, 0);
							cairo_set_source_surface(update_temp,
								update_surface, 0, 0);
							cairo_mask_surface(update_temp,
								window->active_layer->surface_p, - draw_x + r, - draw_y + r);

							cairo_set_operator(update, CAIRO_OPERATOR_SOURCE);
							cairo_set_source_surface(update, temp_surface, 0, 0);
							cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
						}
					}

					cairo_surface_destroy(temp_surface);
					cairo_destroy(update_temp);
				}

				cairo_surface_destroy(update_surface);
				cairo_destroy(update);

				sum_color[0] = sum_color[1] = sum_color[2] = sum_color[4] = 0;
				sum_color[3] = sum_color[5] = 1;
				for(i=0; i<height; i++)
				{
					ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
					mask_pix = &mask[(i+start_y)*window->width+start_x];
					for(j=0; j<width; j++, ref_pix+=4, mask_pix++)
					{
						sum_color[0] += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
						sum_color[1] += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
						sum_color[2] += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
						sum_color[3] += ((ref_pix[3]+1) * *mask_pix) >> 8;
						sum_color[4] += ref_pix[3] * *mask_pix;
						sum_color[5] += *mask_pix;
					}
				}
				color[0] = (sum_color[0] + sum_color[3] / 2) / sum_color[3];
				if(color[0] > 0xff)
				{
					color[0] = 0xff;
				}
				color[1] = (sum_color[1] + sum_color[3] / 2) / sum_color[3];
				if(color[1] > 0xff)
				{
					color[1] = 0xff;
				}
				color[2] = (sum_color[2] + sum_color[3] / 2) / sum_color[3];
				if(color[2] > 0xff)
				{
					color[2] = 0xff;
				}
				color[3] = (uint8)(sum_color[4] / sum_color[5]);

				// �`�悷��F������
				rev_alpha = 0xff-mix+1;
				blend_alpha = (uint8)(brush->alpha * 2.55);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				color[0] = (rev_alpha*(*core->color)[2]+mix*color[0])>>8;
				color[1] = (rev_alpha*(*core->color)[1]+mix*color[1])>>8;
				color[2] = (rev_alpha*(*core->color)[0]+mix*color[2])>>8;
				color[3] = (rev_alpha*blend_alpha+mix*color[3])>>8;
#else
				color[0] = (rev_alpha*(*core->color)[0]+mix*color[0])>>8;
				color[1] = (rev_alpha*(*core->color)[1]+mix*color[1])>>8;
				color[2] = (rev_alpha*(*core->color)[2]+mix*color[2])>>8;
				color[3] = (rev_alpha*blend_alpha+mix*color[3])>>8;
#endif

				if(extend != 0 && ref_point != 0)
				{
					rev_alpha = 0xff-extend+1;
					color[0] = (rev_alpha*color[0]+extend*brush->before_color[0])>>8;
					color[1] = (rev_alpha*color[1]+extend*brush->before_color[1])>>8;
					color[2] = (rev_alpha*color[2]+extend*brush->before_color[2])>>8;
					color[3] = (rev_alpha*color[3]+extend*brush->before_color[3])>>8;
				}

				// ���݂̐F���L��
				brush->before_color[0] = color[0];
				brush->before_color[1] = color[1];
				brush->before_color[2] = color[2];
				brush->before_color[3] = color[3];
				
#define CLEAR_MARGINE 2.5
				//	���u�����h�p�̃o�b�t�@���X�V
				if(draw_x > brush->last_draw_x)
				{
					clear_start_x = (int)(brush->last_draw_x - brush->r * CLEAR_MARGINE);
					clear_width = (int)(draw_x + brush->r * CLEAR_MARGINE);
				}
				else
				{
					clear_start_x = (int)(draw_x - brush->r * CLEAR_MARGINE);
					clear_width = (int)(brush->last_draw_x + brush->r * CLEAR_MARGINE);
				}

				if(clear_start_x < 0)
				{
					clear_start_x = 0;
				}
				if(clear_width > window->width)
				{
					clear_width = window->width;
				}
				clear_width = clear_width - clear_start_x;

				if(draw_y > brush->last_draw_y)
				{
					clear_start_y = (int)(brush->last_draw_y - brush->r * CLEAR_MARGINE);
					clear_height = (int)(draw_y - brush->r * CLEAR_MARGINE);
				}
				else
				{
					clear_start_y = (int)(draw_y - brush->r * CLEAR_MARGINE);
					clear_height = (int)(brush->last_draw_y + brush->r * CLEAR_MARGINE);
				}

				if(clear_start_y < 0)
				{
					clear_start_y = 0;
				}
				if(clear_height > window->height)
				{
					clear_height = window->height;
				}

				for(i=clear_start_y; i<clear_height; i++)
				{
					(void)memset(&mask[window->width*window->height+i*window->width+clear_start_x],
						0, clear_width);
				}
				for(i=0; i<height; i++)
				{
					(void)memcpy(&mask[window->width*window->height+(i+start_y)*window->width+start_x],
						&window->brush_buffer[(i+start_y)*window->width+start_x], width);
				}
#undef CLEAR_MARGINE

				for(i=0; i<height; i++)
				{
					ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
					comp_pix = &window->brush_buffer[(i+start_y)*window->width+start_x];
					alpha_pix = &mask[(i+start_y)*window->width+start_x];
					memory_pix = &mask[window->width*window->height+(i+start_y)*window->width+start_x];

					for(j=0; j<width; j++, ref_pix+=4, comp_pix++, alpha_pix++, memory_pix++)
					{
						mask_value = *alpha_pix;

						if(mask_value > *comp_pix)
						{
							*memory_pix = mask_value;

							blend_alpha /*= dst_value*/ = (mask_value - *comp_pix);
							*comp_pix = mask_value;
							ref_pix[3] = ((blend_alpha+1)*color[3]+(0xff-blend_alpha+1)*ref_pix[3])>>8;
								blend_alpha = min(ref_pix[3]*2, 0xff);

							ref_pix[0] = (uint8)min(
								(((mask_value+1)*color[0]+(0xff-mask_value+1)*ref_pix[0])>>8), blend_alpha);
							ref_pix[1] = (uint8)min(
								(((mask_value+1)*color[1]+(0xff-mask_value+1)*ref_pix[1])>>8), blend_alpha);
							ref_pix[2] = (uint8)min(
								(((mask_value+1)*color[2]+(0xff-mask_value+1)*ref_pix[2])>>8), blend_alpha);
						}
					}
				}

				for(i=clear_start_y; i<clear_height; i++)
				{
					(void)memcpy(&window->brush_buffer[i*window->width+clear_start_x],
						&mask[window->width*window->height+i*window->width+clear_start_x], clear_width);
				}
				brush->last_draw_x = draw_x, brush->last_draw_y = draw_y;
skip_draw:
				dx -= step;
				if(dx < 1)
				{
					window->flags |= DRAW_WINDOW_UPDATE_PART;

					break;
				}
				else if(dx >= step)
				{
					draw_x += diff_x, draw_y += diff_y;
				}
				else
				{
					draw_x = brush->points[ref_point][1];
					draw_y = brush->points[ref_point][2];
				}
			} while(1);

			brush->finish_length += d;
			brush->travel += d;
			brush->draw_finished++;
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
		}	// while(brush->ref_point < brush->draw_finished)

		AddBrushHistory(core, window->active_layer);

		window->update.surface_p = cairo_surface_create_for_rectangle(
			window->active_layer->surface_p, window->update.x, window->update.y,
				window->update.width, window->update.height);
		window->update.cairo_p = cairo_create(window->update.surface_p);
		g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->update);
		cairo_surface_destroy(window->update.surface_p);
		cairo_destroy(window->update.cairo_p);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}	// ���N���b�N�Ȃ��
		// if(((GdkEventButton*)state)->button == 1)
}

static void WaterColorBrushEditSelectionReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ��
	if(((GdkEventButton*)state)->button == 1)
	{
		WATER_COLOR_BRUSH *brush = (WATER_COLOR_BRUSH*)core->brush_data;
		gdouble r, alpha;
		// �u���V�̊g�嗦
		gdouble zoom;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̍��W�w��p
		cairo_matrix_t matrix;
		// �u���V�̈ړ���
		gdouble d, step;
		// �`����s�����W
		gdouble draw_x, draw_y;
		// ����A�������̐F�␳
		gdouble enter_alpha, out_alpha;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A����
		int width, height;
		// �u���V�ʒu�̃s�N�Z���l���v
		unsigned int sum_color[2];
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *comp_pix, *alpha_pix, *mask;
		int rev_alpha;
		uint8 blend_alpha;
		gdouble inv_alpha;
		// �`�悷��F
		uint8 color[2];
		// �}�X�N�̃o�C�g�l
		uint8 mask_value;
		// ���F�
		uint8 mix = (uint8)(brush->mix * 2.55);
		// �F���ы
		uint8 extend = (uint8)(brush->extend * 2.55);
		// �z��̃C���f�b�N�X�p
		int ref_point;
		int before_point;
		int i, j;	// for���p�̃J�E���^

		while(brush->ref_point > brush->draw_finished)
		{
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;

			if(brush->finish_length < brush->enter_length)
			{
				enter_alpha = brush->finish_length / brush->enter_length;
				r = brush->enter_size + brush->finish_length * 0.25;
			}
			else
			{
				enter_alpha = 1;
				r = brush->r;
			}
			if(brush->sum_distance < brush->draw_start)
			{
				out_alpha = brush->sum_distance / brush->draw_start;
				r -= (brush->draw_start - brush->sum_distance) * 0.25;
			}
			else
			{
				out_alpha = 1;
			}

			if(r < 0.5)
			{
				brush->draw_finished++;
				continue;
			}

			d = brush->points[ref_point][0];
			zoom = brush->r / r;
			step = r * BRUSH_STEP;
			if(step < 1)
			{
				step = 1;
			}
			alpha = ((brush->flags & BRUSH_FLAG_FLOW) == 0)
				? 1 : brush->points[ref_point][3];
			alpha *= enter_alpha;
			before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
			brush->sum_distance -= d;
			if(brush->draw_finished == 0)
			{
				draw_x = brush->points[0][1], draw_y = brush->points[0][2];
			}
			else
			{
				draw_x = brush->points[before_point][1], draw_y = brush->points[before_point][2];
			}
			dx = brush->points[ref_point][1] - draw_x;
			dy = brush->points[ref_point][2] - draw_y;
			diff_x = step * dx / d, diff_y = step * dy / d;

			min_x = brush->points[ref_point][1] - r - 1;
			max_x = brush->points[ref_point][1] + r + 1;
			min_y = brush->points[ref_point][2] - r - 1;
			max_y = brush->points[ref_point][2] + r + 1;
			if(min_x < 0.0)
			{
				min_x = 0.0;
			}
			if(core->min_x > min_x)
			{
				core->min_x = min_x;
			}
			if(min_y < 0.0)
			{
				min_y = 0.0;
			}
			if(core->min_y > min_y)
			{
				core->min_y = min_y;
			}
			if(max_x > window->work_layer->width)
			{
				max_x = window->work_layer->width;
			}
			if(core->max_x < max_x)
			{
				core->max_x = max_x;
			}
			if(max_y > window->work_layer->height)
			{
				min_y = window->work_layer->height;
			}
			if(core->max_y < max_y)
			{
				core->max_y = max_y;
			}

			dx = d;
			do
			{
				start_x = (int)(draw_x - r);
				start_y = (int)(draw_y - r);
				width = (int)(draw_x + r);
				height = (int)(draw_y + r);

				if(start_x < 0)
				{
					start_x = 0;
				}
				else if(start_x > window->work_layer->width)
				{
					goto skip_draw;
				}
				if(start_y < 0)
				{
					start_y = 0;
				}
				else if(start_y > window->work_layer->height)
				{
					goto skip_draw;
				}
				if(width > window->work_layer->width)
				{
					width = window->work_layer->width - start_x;
				}
				else
				{
					width = width - start_x;
				}
				if(height > window->work_layer->height)
				{
					height = window->work_layer->height - start_y;
				}
				else
				{
					height = height - start_y;
				}

				(void)memset(window->mask->pixels, 0, window->width*window->height);

				mask = window->mask->pixels;
				cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_matrix_translate(&matrix, - draw_x + r, - draw_y + r);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(window->alpha_cairo, core->brush_pattern);
						cairo_paint_with_alpha(window->alpha_cairo, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(window->alpha_cairo, core->temp_pattern);
						cairo_mask_surface(window->alpha_cairo,
							window->active_layer->surface_p, 0, 0);
					}
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, - draw_x + r, - draw_y + r);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(window->alpha_cairo, core->temp_pattern);
						cairo_mask_surface(window->alpha_cairo,
							window->selection->surface_p, 0, 0);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, - x + r, - y + r);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(window->alpha_cairo, core->temp_pattern);

						(void)memset(window->temp_layer->pixels, 0, window->width*window->height);

						cairo_mask_surface(window->alpha_cairo,
							window->selection->surface_p, 0, 0);
						cairo_set_source_surface(window->alpha_temp_cairo,
							window->alpha_surface, 0, 0);
						cairo_mask_surface(window->alpha_temp_cairo,
							window->active_layer->surface_p, 0, 0);

						mask = window->temp_layer->pixels;
					}
				}

				sum_color[0] = sum_color[1] = 1;
				for(i=0; i<height; i++)
				{
					ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
					mask_pix = &mask[(i+start_y)*window->width+start_x];
					for(j=0; j<width; j++, ref_pix++, mask_pix++)
					{
						sum_color[0] += ((ref_pix[0]+1) * *mask_pix) >> 8;
						sum_color[1] += *mask_pix;
					}
				}
				color[0] = (uint8)((sum_color[0] + (sum_color[1] / 255) / 2) / (sum_color[1] / 255.0));

				// �`�悷��F������
				rev_alpha = 0xff-mix+1;
				blend_alpha = (uint8)(brush->alpha * 2.55);
				color[0] = (rev_alpha*blend_alpha+mix*color[0])>>8;

				if(extend != 0 && ref_point != 0)
				{
					rev_alpha = 0xff-extend+1;
					color[0] = (rev_alpha*color[0]+extend*brush->before_color[0])>>8;
				}

				// ���݂̐F���L��
				brush->before_color[0] = color[0];
				inv_alpha = color[0] * DIV_PIXEL;

				for(i=0; i<height; i++)
				{
					ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
					comp_pix = &window->brush_buffer[(i+start_y)*window->width+start_x];
					alpha_pix = &mask[(i+start_y)*window->width+start_x];

					for(j=0; j<width; j++, ref_pix++, mask_pix++, comp_pix++, alpha_pix++)
					{
						mask_value = *alpha_pix;
						*alpha_pix = (uint8)(*alpha_pix * inv_alpha);

						if(mask_value > 0)
						{
							if(mask_value == 0xff)
							{
								ref_pix[0] = *alpha_pix;
							}
							else
							{
								ref_pix[0] = (mask_value > ref_pix[0]) ? *alpha_pix : ref_pix[0];
							}
						}
					}
				}
				(void)memcpy(window->brush_buffer, mask, window->width*window->height);
skip_draw:
				dx -= step;
				if(dx < 1)
				{
					break;
				}
				else if(dx >= step)
				{
					draw_x += diff_x, draw_y += diff_y;
				}
				else
				{
					draw_x = brush->points[before_point][1];
					draw_y = brush->points[before_point][2];
				}
			} while(1);

			brush->finish_length += d;
			brush->travel += d;
			brush->draw_finished++;
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
		}	// while(brush->ref_point < brush->draw_finished)

		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[window->selection->layer_mode](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->selection->layer_mode = SELECTION_BLEND_NORMAL;
	}	// ���N���b�N�Ȃ��
		// if(((GdkEventButton*)state)->button == 1)
}

static void WaterColorBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	WATER_COLOR_BRUSH* brush = (WATER_COLOR_BRUSH*)data;
	gdouble r = brush->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void WaterColorBrushButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, WATER_COLOR_BRUSH* brush)
{
	gdouble r = brush->r * window->zoom_rate + 1;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2) + BRUSH_UPDATE_MARGIN, (gint)(r * 2) + BRUSH_UPDATE_MARGIN);
}

static void WaterColorBrushMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, WATER_COLOR_BRUSH* brush)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = brush->r * window->zoom_rate * 1.275 + BRUSH_UPDATE_MARGIN;
	gdouble enter = (brush->enter_length + brush->draw_start) * window->zoom_rate;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	start_x -= enter * 4;
	width += enter * 4 * 2 + 2;
	start_y -= enter * 4;
	height += enter * 4 * 2 + 2;

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width, (gint)height);
}

static void WaterColorBrushSetScale(GtkAdjustment* slider, WATER_COLOR_BRUSH* brush)
{
	brush->r = gtk_adjustment_get_value(slider) * 0.5;
	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);
}

static void WaterColorBrushSetFlow(GtkAdjustment* slider, WATER_COLOR_BRUSH* brush)
{
	brush->alpha = gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);
}

static void WaterColorBrushSetBlur(GtkAdjustment* slider, WATER_COLOR_BRUSH* brush)
{
	brush->blur = gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);
}

static void WaterColorBrushSetOutlineHardness(GtkAdjustment* slider, WATER_COLOR_BRUSH* brush)
{
	brush->outline_hardness = gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);
}

static void WaterColorBrushSetMix(GtkAdjustment* slider, WATER_COLOR_BRUSH* brush)
{
	brush->mix = gtk_adjustment_get_value(slider);
}

static void WaterColorBrushSetExtend(GtkAdjustment* slider, WATER_COLOR_BRUSH* brush)
{
	brush->extend = gtk_adjustment_get_value(slider);
}

static void WaterColorBrushSetEnterSize(GtkAdjustment* slider, WATER_COLOR_BRUSH* brush)
{
	brush->enter = gtk_adjustment_get_value(slider);
}

static void WaterColorBrushSetOutSize(GtkAdjustment* slider, WATER_COLOR_BRUSH* brush)
{
	brush->out = gtk_adjustment_get_value(slider);
}

static void WaterColorBrushSetPressureSize(GtkWidget* button, WATER_COLOR_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_SIZE);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_SIZE;
	}
}

static void WaterColorBrushSetPressureFlow(GtkWidget* button, WATER_COLOR_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(BRUSH_FLAG_FLOW);
	}
	else
	{
		brush->flags |= BRUSH_FLAG_FLOW;
	}
}

static GtkWidget* CreateWaterColorBrushDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	WATER_COLOR_BRUSH *brush = (WATER_COLOR_BRUSH*)core->brush_data;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox;
	GtkWidget *brush_scale;
	GtkWidget *check_button;
	GtkWidget *label;
	GtkWidget *base_scale;
	GtkAdjustment *scale_adjustment;
	char mark_up_buff[256];

	brush->core = core;

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		base_scale = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[2]);
#else
		base_scale = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), brush->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(brush->base_scale)
	{
	case 0:
		scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(brush->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(brush->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(WaterColorBrushSetScale), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.brush_scale, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(base_scale), "scale", brush_scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &brush->base_scale);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->alpha, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(WaterColorBrushSetFlow), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.flow, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->outline_hardness, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(WaterColorBrushSetOutlineHardness), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.outline_hardness, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->blur, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(WaterColorBrushSetBlur), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.blur, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->mix, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(WaterColorBrushSetMix), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.mix, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->extend, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(WaterColorBrushSetExtend), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.color_extend, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->enter, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(WaterColorBrushSetEnterSize), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.enter, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->out, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(WaterColorBrushSetOutSize), core->brush_data);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.out, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(WaterColorBrushSetPressureSize), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(WaterColorBrushSetPressureFlow), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);

	return vbox;
#undef UI_FONT_SIZE
}

/*****************************************************
* InitializeStampCore�֐�                            *
* �X�^���v�̊�{����������                         *
* ����                                               *
* core	: �X�^���v�n�c�[���̊�{���                 *
* mode	: �`�����l������2�̎��̍������@              *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
static void InitializeStampCore(STAMP_CORE* core, int mode, APPLICATION* app)
{
	// �J�[�\���T�[�t�F�[�X�쐬�p�Ƀp�^�[���T�[�t�F�[�X���쐬
	cairo_surface_t* surface_p;
	// �p�^�[���T�[�t�F�[�X�̃s�N�Z���f�[�^
	uint8* pixels;
	// �p�^�[���s�N�Z���f�[�^�̈�s���̃o�C�g��
	int stride;
	// �J�[�\���̕�
	int cursor_width;
	// for���p�̃J�E���^
	int i, j;

	// ���A�����̓��A�傫�����Ŕ��a�Z�b�g
	if(app->stamps.active_pattern->width > app->stamps.active_pattern->height)
	{
		core->r = app->stamps.active_pattern->width * core->inv_scale;
	}
	else
	{
		core->r = app->stamps.active_pattern->height * core->inv_scale;
	}

	// �X�^���v�̃T�C�Y�Z�b�g
	core->half_width = app->stamps.active_pattern->width * core->inv_scale * 0.5;
	core->half_height = app->stamps.active_pattern->height * core->inv_scale * 0.5;

	// �X�^���v�Ԃ̋�������
	core->d = core->stamp_distance *
		sqrt(core->half_width*core->half_width + core->half_height*core->half_height);

	if(core->d < 1)
	{
		core->d = 1;
	}

	// �J�n��]�p������
	core->rotate_start = fabs(core->rotate_start);
	if(core->rotate_direction < 0)
	{
		core->rotate_start = - core->rotate_start;
	}
	core->rotate = core->rotate_start;

	// ��]���x������
	core->rotate_speed = core->rotate_direction * fabs(core->rotate_speed);

	// �X�^���v�̃J�[�\�����X�V
	if(core->cursor_surface != NULL)
	{
		cairo_surface_destroy(core->cursor_surface);
	}
	if(app->stamp_buff_size < app->stamps.pattern_max_byte)
	{
		app->stamp_buff_size = app->stamps.pattern_max_byte;
		app->stamp_shape = (uint8*)MEM_REALLOC_FUNC(app->stamp_shape, app->stamp_buff_size);
	}

	// �p�^�[���T�[�t�F�[�X���쐬���ăJ�[�\���쐬
	surface_p = CreatePatternSurface(&app->stamps, app->tool_window.color_chooser->rgb,
		app->tool_window.color_chooser->back_rgb, core->pattern_flags, 0, 1);
	if(surface_p != NULL)
	{
		pixels = cairo_image_surface_get_data(surface_p);
		stride = cairo_image_surface_get_stride(surface_p);
		// �J�[�\���̕�����
		cursor_width = app->stamps.active_pattern->width;
		cursor_width += (4 - (cursor_width % 4)) % 4;
		(void)memset(app->stamps.pattern_pixels_temp, 0, cursor_width * app->stamps.active_pattern->height);
		// ���`�����l�����R�s�[
		for(i=0; i<app->stamps.active_pattern->height; i++)
		{
			for(j=0; j<app->stamps.active_pattern->width; j++)
			{
				if(pixels[i*stride+j*4+3] > 0)
				{
					app->stamps.pattern_pixels_temp[i*cursor_width+j] = 0xff;
				}
				else
				{
					app->stamps.pattern_pixels_temp[i*cursor_width+j] = 0;
				}
			}
		}
		// �G�b�W���o
		LaplacianFilter(app->stamps.pattern_pixels_temp, cursor_width, app->stamps.active_pattern->height,
			cursor_width, app->stamp_shape);
		// �G�b�W���o�����f�[�^�ŃT�[�t�F�[�X�쐬
		core->cursor_surface = cairo_image_surface_create_for_data(app->stamp_shape,
			CAIRO_FORMAT_A8, cursor_width, app->stamps.active_pattern->height, cursor_width);
		cairo_surface_destroy(surface_p);
	}

	if((app->flags & APPLICATION_INITIALIZED) != 0 && core->app != NULL)
	{
		// �u���V�p�^�[���T�[�t�F�[�X���X�V
		if(core->brush_core->brush_surface != NULL)
		{
			cairo_surface_destroy(core->brush_core->brush_surface);
		}
		if(core->brush_core->brush_pattern != NULL)
		{
			cairo_pattern_destroy(core->brush_core->brush_pattern);
		}
		core->brush_core->brush_surface = CreatePatternSurface(
			&app->stamps, *core->brush_core->color, app->tool_window.color_chooser->back_rgb, core->pattern_flags, mode, core->flow);
		core->brush_core->brush_pattern = cairo_pattern_create_for_surface(
			core->brush_core->brush_surface);
		cairo_pattern_set_extend(core->brush_core->brush_pattern, CAIRO_EXTEND_NONE);
		core->brush_core->temp_surface = cairo_image_surface_create_for_data(
			core->app->stamps.pattern_pixels_temp, CAIRO_FORMAT_ARGB32,
				core->app->stamps.active_pattern->width, core->app->stamps.active_pattern->height, core->app->stamps.active_pattern->width*4
		);
		core->brush_core->temp_cairo = cairo_create(core->brush_core->temp_surface);
		cairo_set_operator(core->brush_core->temp_cairo, CAIRO_OPERATOR_SOURCE);
		core->brush_core->temp_pattern = cairo_pattern_create_for_surface(core->brush_core->temp_surface);
		cairo_pattern_set_extend(core->brush_core->temp_pattern, CAIRO_EXTEND_NONE);
	}
}

/*****************************************
* StampCoreDrawCursor�֐�                *
* �X�^���v�n�c�[���̃J�[�\����`��       *
* ����                                   *
* window	: �`��̈�̏��             *
* x			: �J�[�\����X���W            *
* y			: �J�[�\����Y���W            *
* data		: �X�^���v�n�c�[���̊�{��� *
*****************************************/
static void StampCoreDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	STAMP_CORE* core
)
{
	// �J�[�\���u���V
	cairo_pattern_t* brush;
	// �J�[�\���̊g��k���A��]�E���s�ړ��p�̍s��
	cairo_matrix_t matrix;
	// �J�[�\���̕��s�E��]�ړ��p
	gdouble trans_x, trans_y;
	gdouble cos_x, sin_y;
	gdouble half_width, half_height;
	gdouble rev_zoom = window->rev_zoom;

	// ���s�ړ�������W���v�Z
	half_width = core->half_width * window->zoom_rate;
	half_height = core->half_height * window->zoom_rate;
	cos_x = cos(core->rotate), sin_y = sin(core->rotate);
	trans_x = x - (half_width * cos_x + half_height * sin_y);
	trans_y = y + (half_width * sin_y - half_height * cos_x);
	// �g��k�����A��]�p���Z�b�g
	brush = cairo_pattern_create_for_surface(core->cursor_surface);
	cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
	cairo_matrix_init_scale(&matrix, core->scale * rev_zoom, core->scale * rev_zoom);
	cairo_matrix_rotate(&matrix, core->rotate);
	cairo_matrix_translate(&matrix, - trans_x, - trans_y);
	cairo_pattern_set_matrix(brush, &matrix);

	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_mask(window->disp_temp->cairo_p, brush);

	cairo_rectangle(window->disp_layer->cairo_p, (int)(x - half_width - 1),
		(int)(y - half_height - 1), (int)half_width*2+2, (int)half_height*2+2);
	cairo_clip(window->disp_layer->cairo_p);

	cairo_pattern_destroy(brush);
}

/*************************************************************
* LoadStampCoreData�֐�                                      *
* �X�^���v�̊�{����ǂݍ���                               *
* ����                                                       *
* file			: �u���V�f�[�^�̃t�@�C���Ǘ��\���̂̃A�h���X *
* section_name	: �Z�N�V������                               *
* core			: �X�^���v�n�c�[���̊�{���                 *
* mode			: ���F���[�h���Ǘ�����ϐ��̃A�h���X         *
* app			: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*************************************************************/
static void LoadStampCoreData(
	INI_FILE_PTR file,
	const char* section_name,
	STAMP_CORE* core,
	uint8* mode,
	APPLICATION* app
)
{
	core->inv_scale = IniFileGetInt(file, section_name, "SIZE") * 0.01;
	core->scale = 1 / core->inv_scale;
	core->flow = IniFileGetInt(file, section_name, "FLOW") * 0.01;
	core->stamp_distance = IniFileGetDouble(file, section_name, "DISTANCE");
	core->rotate_start = IniFileGetInt(file, section_name, "ROTATE_START") * G_PI / 180;
	core->rotate_speed = IniFileGetInt(file, section_name, "ROTATE_SPEED") * G_PI / 180;
	core->rotate_direction = IniFileGetInt(file, section_name, "ROTATE_DIRECTION");
	if(core->rotate_direction < 0)
	{
		core->rotate_direction = -1;
	}
	else
	{
		core->rotate_direction = 1;
	}
	core->stamp_id = IniFileGetInt(file, section_name, "STAMP_ID");
	if(mode != NULL)
	{
		*mode = (uint8)IniFileGetInt(file, section_name, "MODE");
	}
	if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
	{
		core->flags |= STAMP_PRESSURE_SIZE;
	}
	if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
	{
		core->flags |= STAMP_PRESSURE_FLOW;
	}
	if(IniFileGetInt(file, section_name, "RANDOM_ROTATE") != 0)
	{
		core->flags |= STAMP_RANDOM_ROTATE;
	}
	if(IniFileGetInt(file, section_name, "RANDOM_SIZE") != 0)
	{
		core->flags |= STAMP_RANDOM_SIZE;
	}

	InitializeStampCore(core, (mode == NULL) ? STAMP_MODE_SATURATION : (int)(*mode), app);
}

/*************************************************************
* LoadStampCoreDefaultData�֐�                               *
* �X�^���v�̃f�t�H���g��{����ǂݍ���                     *
* ����                                                       *
* core			: �X�^���v�n�c�[���̊�{���                 *
* mode			: ���F���[�h���Ǘ�����ϐ��̃A�h���X         *
* app			: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*************************************************************/
static void LoadStampCoreDefaultData(
	STAMP_CORE* core,
	uint8* mode,
	APPLICATION* app
)
{
	core->inv_scale = 1;
	core->scale = 1 / core->inv_scale;
	core->flow = 1;
	core->stamp_distance = 1;
	// core->rotate_start = 0;
	// core->rotate_speed = 0;
	core->rotate_direction = 1;
	/*
	if(core->rotate_direction < 0)
	{
		core->rotate_direction = -1;
	}
	else
	{
		core->rotate_direction = 1;
	}
	*/
	// core->stamp_id = 0;
	/*
	if(mode != NULL)
	{
		*mode = (uint8)IniFileGetInt(file, section_name, "MODE");
	}
	*/
	/*
	if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
	{
		core->flags |= STAMP_PRESSURE_SIZE;
	}
	if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
	{
		core->flags |= STAMP_PRESSURE_FLOW;
	}
	if(IniFileGetInt(file, section_name, "RANDOM_ROTATE") != 0)
	{
		core->flags |= STAMP_RANDOM_ROTATE;
	}
	if(IniFileGetInt(file, section_name, "RANDOM_SIZE") != 0)
	{
		core->flags |= STAMP_RANDOM_SIZE;
	}
	*/

	InitializeStampCore(core, (mode == NULL) ? STAMP_MODE_SATURATION : (int)(*mode), app);
}

static void StampCoreChangeScale(GtkAdjustment* slider, STAMP_CORE* core)
{
	int mode = (core->mode != NULL) ? (int)(*core->mode) : 0;
	core->inv_scale = gtk_adjustment_get_value(slider) * 0.01;
	core->scale = 1 / core->inv_scale;
	InitializeStampCore(core, mode, (APPLICATION*)g_object_get_data(G_OBJECT(slider), "application"));
}

static void StampCoreChangeFlow(GtkAdjustment* slider, STAMP_CORE* core)
{
	int mode = (core->mode != NULL) ? (int)(*core->mode) : 0;
	core->flow = gtk_adjustment_get_value(slider) * 0.01;
	InitializeStampCore(core, mode, (APPLICATION*)g_object_get_data(G_OBJECT(slider), "application"));
}

static void StampCoreChangeDistance(GtkAdjustment* slider, STAMP_CORE* core)
{
	int mode = (core->mode != NULL) ? (int)(*core->mode) : 0;
	core->stamp_distance = gtk_adjustment_get_value(slider);
	InitializeStampCore(core, mode, (APPLICATION*)g_object_get_data(G_OBJECT(slider), "application"));
}

static void StampCoreChangeRotateStart(GtkAdjustment* slider, STAMP_CORE* core)
{
	int mode = (core->mode != NULL) ? (int)(*core->mode) : 0;
	core->rotate_start = gtk_adjustment_get_value(slider) * G_PI / 180;
	InitializeStampCore(core, mode, (APPLICATION*)g_object_get_data(G_OBJECT(slider), "application"));
}

static void StampCoreChangeRotateSpeed(GtkAdjustment* slider, STAMP_CORE* core)
{
	int mode = (core->mode != NULL) ? (int)(*core->mode) : 0;
	core->rotate_speed = gtk_adjustment_get_value(slider) * G_PI / 180;
	InitializeStampCore(core, mode, (APPLICATION*)g_object_get_data(G_OBJECT(slider), "application"));
}

static void StampCoreChangeRotateDirection(GtkWidget* widget, STAMP_CORE* core)
{
	int mode = (core->mode != NULL) ? (int)(*core->mode) : 0;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) != FALSE)
	{
		core->rotate_direction = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "direction"));
	}
	InitializeStampCore(core, mode, (APPLICATION*)g_object_get_data(G_OBJECT(widget), "application"));
}

static void StampCoreSetFlags(GtkWidget* widget, STAMP_CORE* core)
{
	unsigned int flag = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), "flag-id"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		core->flags &= ~(1 << flag);
	}
	else
	{
		core->flags |= (1 << flag);
	}
}

static void StampCoreSetPatternFlags(GtkWidget* widget, STAMP_CORE* core)
{
	int mode = (core->mode != NULL) ? (int)(*core->mode) : 0;
	unsigned int flag = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), "flag-id"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		core->pattern_flags &= ~(1 << flag);
	}
	else
	{
		core->pattern_flags |= (1 << flag);
	}

	InitializeStampCore(core, mode, (APPLICATION*)g_object_get_data(G_OBJECT(widget), "application"));
}

static void StampCoreSetFlag(GtkWidget* widget, STAMP_CORE* core)
{
	int mode = (core->mode != NULL) ? (int)(*core->mode) : 0;
	unsigned int flag = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), "flag-value"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		core->flags &= ~(flag);
	}
	else
	{
		core->flags |= flag;
	}

	InitializeStampCore(core, mode, (APPLICATION*)g_object_get_data(G_OBJECT(widget), "application"));
}

static void StampCoreSetMode(GtkWidget* widget, uint8* mode)
{
	STAMP_CORE *core = g_object_get_data(G_OBJECT(widget), "stamp_core");
	*mode = (uint8)GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), "mode"));
	InitializeStampCore(core, (int)(*mode), (APPLICATION*)g_object_get_data(G_OBJECT(widget), "application"));
}

/***************************************************************
* StampSelectButtonClicked�֐�                                 *
* �X�^���v�I��p�̃{�^�����N���b�N���ꂽ�Ƃ��̃R�[���o�b�N�֐� *
* ����                                                         *
* button	: �{�^���E�B�W�F�b�g                               *
* stamp		: �X�^���v�n�c�[���̊�{���                       *
***************************************************************/
static void StampSelectButtonClicked(GtkWidget* button, STAMP_CORE* core)
{
	// �p�^�[��ID���E�B�W�F�b�g�ɓo�^���ꂽ�f�[�^����擾
	int stamp_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "stamp-id"));
	// �{�^��ID�����p
	int button_add = (core->app->stamps.has_clip_board_pattern == FALSE) ? 0 : -1;
	// ���F���[�h
	int mode = (core->mode != NULL) ? (int)(*core->mode) : 0;
	// for���p�̃J�E���^
	int i;

	// �g�p�p�^�[���Ɖ����ꂽ�{�^������v���Ă�����{�^�����A�N�e�B�u�ɂ��ďI��
	if(stamp_id == core->stamp_id)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		}
		return;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{	// �g�p�p�^�[���ƕs��v�Ń{�^������A�N�e�B�u�Ȃ�I��
		return;
	}

	// �g�p�p�^�[����ݒ�
	core->stamp_id = stamp_id;
	if(core->app->stamps.has_clip_board_pattern != FALSE && core->stamp_id == 0)
	{
		core->app->stamps.active_pattern =
			&core->app->stamps.clip_board;
	}
	else
	{
		core->app->stamps.active_pattern =
			&core->app->stamps.patterns[stamp_id + button_add];
	}

	InitializeStampCore(core, mode, core->app);

	// �A�N�e�B�u�ȃp�^�[�����`�����l����2�ȊO�Ȃ�΍������[�h�͖���
	if(core->mode_select[0] != NULL)
	{
		gtk_widget_set_sensitive(core->mode_select[0], core->app->stamps.active_pattern->channel == 2);
		gtk_widget_set_sensitive(core->mode_select[1], core->app->stamps.active_pattern->channel == 2);
		gtk_widget_set_sensitive(core->mode_select[2], core->app->stamps.active_pattern->channel == 2);
	}

	// ���̃{�^�����A�N�e�B�u�ɂ���
	for(i=0; i<core->num_button; i++)
	{
		if(i == core->stamp_id)
		{
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(core->buttons[i])) == FALSE)
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(core->buttons[i]), TRUE);
			}
		}
		else
		{
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(core->buttons[i])) != FALSE)
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(core->buttons[i]), FALSE);
			}
		}
	}
}

// �p�^�[���I��p�e�[�u���̕�
#define STAMP_SELECT_TABLE_WIDTH 4

/*******************************************
* CreateStampSelectTable�֐�               *
* �X�^���v�I��p�̃{�^���e�[�u�����쐬���� *
* ����                                     *
* core	: �X�^���v�n�c�[���̊�{���       *
* �Ԃ�l                                   *
*	�쐬�����{�^���e�[�u���̃E�B�W�F�b�g   *
*******************************************/
static GtkWidget* CreateStampSelectTable(STAMP_CORE* core)
{
// �{�^���ɕ\������A�C�R���̃T�C�Y
#define ICON_SIZE 32
	// �Ԃ�l
	GtkWidget* table;
	// �{�^���ɓo�^����C���[�W�E�B�W�F�b�g
	GtkWidget* image;
	// �e�[�u���̍���
	int height;
	// �{�^���̃C���[�W�쐬�p�s�N�Z���o�b�t�@
	GdkPixbuf* pixbuf, *scaled_buf;
	// �`�����l������1�̂Ƃ��Ɏg�p����s�N�Z���f�[�^
	uint8* pixels;
	// BGR��RGB�ϊ��p
	uint8 *swap_pixels;
	uint8 r;
	// �C���[�W�̊g�嗦
	gdouble zoom;
	// �{�^��ID�����p
	int button_add = 0;
	// �e�[�u���̍��W
	int x = 0, y = 0;
	// for���p�̃J�E���^
	int i = 0, j;

	// �{�^���z��쐬
	core->buttons = (GtkWidget**)MEM_ALLOC_FUNC(
		sizeof(*core->buttons)*(core->app->stamps.num_pattern+1));
	core->num_button = core->app->stamps.num_pattern;

	// �e�[�u���̍��������肵�č쐬
	height = core->num_button / STAMP_SELECT_TABLE_WIDTH;
	if((core->num_button+1) % STAMP_SELECT_TABLE_WIDTH != 0)
	{
		height++;
	}
	table = gtk_table_new(height, STAMP_SELECT_TABLE_WIDTH, TRUE);

	// �N���b�v�{�[�h�̃p�^�[��������Ȃ��
	if(core->app->stamps.has_clip_board_pattern != FALSE)
	{
		PATTERN* pattern = &core->app->stamps.clip_board;

		pixbuf = gdk_pixbuf_new_from_data(pattern->pixels,
			GDK_COLORSPACE_RGB, TRUE, 8, pattern->width, pattern->height,
			pattern->stride, NULL, NULL
		);

		// �g�嗦���v�Z
		if(pattern->width > pattern->height)
		{
			zoom = ICON_SIZE / (gdouble)pattern->width;
		}
		else
		{
			zoom = ICON_SIZE / (gdouble)pattern->height;
		}

		// �s�N�Z���o�b�t�@���g��k�����ăC���[�W�E�B�W�F�b�g�쐬
		image = gtk_image_new_from_pixbuf(
			gdk_pixbuf_scale_simple(pixbuf, (int)(pattern->width*zoom),
			(int)(pattern->height*zoom), GDK_INTERP_BILINEAR)
		);

		// �{�^���쐬
		core->buttons[i] = gtk_toggle_button_new();
		// ���݂̎g�p�p�^�[���ƈ�v���Ă�����A�N�e�B�u��
		if(i == core->stamp_id + button_add)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(core->buttons[i]), TRUE);
		}

		// �R�[���o�b�N�֐��ƃf�[�^���Z�b�g
		g_signal_connect(G_OBJECT(core->buttons[i]), "toggled", G_CALLBACK(StampSelectButtonClicked), core);
		g_object_set_data(G_OBJECT(core->buttons[i]), "stamp-id", GINT_TO_POINTER(i));

		// �e�[�u���Ƀ{�^���ǉ�
		gtk_table_attach(GTK_TABLE(table), core->buttons[i], x, x+1, y, y+1, GTK_SHRINK, GTK_SHRINK, 0, 0);

		// �C���[�W�E�B�W�F�b�g���{�^���ɓo�^
		gtk_container_add(GTK_CONTAINER(core->buttons[i]), image);

		g_object_unref(pixbuf);
		core->num_button++;
		button_add = -1;
		i++, x++;
	}

	// �{�^�����쐬���ăe�[�u���ɓ����
	for( ; i<core->num_button; i++, x++)
	{
		// �{�^���ɓ����p�^�[��
		PATTERN* pattern = &core->app->stamps.patterns[i+button_add];

		// ��s�����܂����玟�̍s��
		if(x == STAMP_SELECT_TABLE_WIDTH)
		{
			x = 0;
			y++;
		}

		pixels = NULL;
		// �O���[�X�P�[���Ȃ�RGB�ɂ���
		if(pattern->channel == 1)
		{
			uint8 pixel_value;
			pixels = (uint8*)MEM_ALLOC_FUNC(pattern->stride*pattern->height*3);
			for(j=0; j<pattern->stride*pattern->height; j++)
			{
				pixel_value = 0xff - pattern->pixels[j];
				pixels[j*3] = pixel_value;
				pixels[j*3+1] = pixel_value;
				pixels[j*3+2] = pixel_value;
			}
			pixbuf = gdk_pixbuf_new_from_data(
				pixels, GDK_COLORSPACE_RGB, FALSE, 8,
				pattern->width, pattern->height, pattern->stride*3, NULL, NULL
			);
		}
		else if(pattern->channel == 2)
		{
			uint8 pixel_value;
			int k;

			pixels = (uint8*)MEM_ALLOC_FUNC(pattern->width*pattern->height*4);
			for(j=0; j<pattern->height; j++)
			{
				for(k=0; k<pattern->width; k++)
				{
					pixel_value = pattern->pixels[j*pattern->stride+k*2];
					pixels[j*pattern->width*4+k*4] = pixel_value;
					pixels[j*pattern->width*4+k*4+1] = pixel_value;
					pixels[j*pattern->width*4+k*4+2] = pixel_value;
					pixels[j*pattern->width*4+k*4+3] = pattern->pixels[j*pattern->stride+k*2+1];
				}
			}
			pixbuf = gdk_pixbuf_new_from_data(
				pixels, GDK_COLORSPACE_RGB, TRUE, 8,
				pattern->width, pattern->height, pattern->width*4, NULL, NULL
			);
		}
		else
		{
			// �p�^�[���̃s�N�Z���f�[�^����s�N�Z���o�b�t�@�쐬
			pixbuf = gdk_pixbuf_new_from_data(pattern->pixels, GDK_COLORSPACE_RGB,
				pattern->channel == 4, 8, pattern->width, pattern->height, pattern->width*pattern->channel,
				NULL, NULL
			);
		}

		// �g�嗦���v�Z
		if(pattern->width > pattern->height)
		{
			zoom = ICON_SIZE / (gdouble)pattern->width;
		}
		else
		{
			zoom = ICON_SIZE / (gdouble)pattern->height;
		}

		scaled_buf = gdk_pixbuf_scale_simple(pixbuf, (int)(pattern->width*zoom),
			(int)(pattern->height*zoom), GDK_INTERP_BILINEAR);
		swap_pixels = gdk_pixbuf_get_pixels(scaled_buf);
		if(swap_pixels != NULL)
		{
			int image_width = gdk_pixbuf_get_width(scaled_buf);
			int image_height = gdk_pixbuf_get_height(scaled_buf);
			int channel;

			if(gdk_pixbuf_get_has_alpha(scaled_buf) == FALSE)
			{
				channel = 3;
			}
			else
			{
				channel = 4;
			}

			for(j=0; j<image_width*image_height; j++)
			{
				r = swap_pixels[j*channel];
				swap_pixels[j*channel] = swap_pixels[j*channel+2];
				swap_pixels[j*channel+2] = r;
			}
		}
		// �s�N�Z���o�b�t�@���g��k�����ăC���[�W�E�B�W�F�b�g�쐬
		image = gtk_image_new_from_pixbuf(scaled_buf);

		// �{�^���쐬
		core->buttons[i] = gtk_toggle_button_new();
		// ���݂̎g�p�p�^�[���ƈ�v���Ă�����A�N�e�B�u��
		if(i == core->stamp_id + button_add)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(core->buttons[i]), TRUE);
		}

		// �R�[���o�b�N�֐��ƃf�[�^���Z�b�g
		g_signal_connect(G_OBJECT(core->buttons[i]), "toggled", G_CALLBACK(StampSelectButtonClicked), core);
		g_object_set_data(G_OBJECT(core->buttons[i]), "stamp-id", GINT_TO_POINTER(i));

		// �e�[�u���Ƀ{�^���ǉ�
		gtk_table_attach(GTK_TABLE(table), core->buttons[i], x, x+1, y, y+1, GTK_SHRINK, GTK_SHRINK, 0, 0);

		// �C���[�W�E�B�W�F�b�g���{�^���ɓo�^
		gtk_container_add(GTK_CONTAINER(core->buttons[i]), image);

		g_object_unref(pixbuf);
		g_object_unref(scaled_buf);
		MEM_FREE_FUNC(pixels);
	}

	return table;
}

static void OnStampDetailUIDestroy(GtkWidget* widget, STAMP_CORE* core)
{
	MEM_FREE_FUNC(core->buttons);
}

/*****************************************************
* CreateStampCoreDetailUI�֐�                        *
* �X�^���v�̊�{����ݒ肷��UI���쐬               *
* ����                                               *
* core	: �X�^���v�n�c�[���̊�{���                 *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* box	: UI�E�B�W�F�b�g������p�b�L���O�{�b�N�X   *
*****************************************************/
static void CreateStampCoreDetailUI(
	STAMP_CORE* core,
	uint8* mode,
	APPLICATION* app,
	GtkWidget* box
)
{
#define UI_FONT_SIZE 8.0
	// ���x���A�X���C�_
	GtkWidget* label, *scale;
	// �X�N���[���h�E�B���h�E
	GtkWidget* scrolled_window;
	// �E�B�W�F�b�g����p�̃e�[�u��
	GtkWidget* table;
	// ��]�����A���[�h�ݒ�p�̃��W�I�{�^��
	GtkWidget* radio_buttons[PATTERN_MODE_NUM];
	// �M���g�p�ݒ�̃`�F�b�N�{�b�N�X
	GtkWidget* check_button;
	// �X���C�_�Ɏg�p����A�W���X�^
	GtkAdjustment* scale_adjustment;
	// ���x���̃t�H���g�ύX�p�̃}�[�N�A�b�v�o�b�t�@
	char mark_up_buff[256];
	// �������F���[�h
	int first_mode = (mode != NULL) ? (int)(*mode) : 0;

	// �A�v���P�[�V�����Ǘ��̍\���̂ւ̃|�C���^���Z�b�g
	core->app = app;

	// �N���b�v�{�[�h�̃p�^�[�����X�V
	UpdateClipBoardPattern(&app->stamps);

	// �g��k�����ݒ�p�̃E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		(1 / core->scale) * 100, 10, 300, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeScale), core);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);

	// �Z�x�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		core->flow * 100, 0, 100, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.flow, 1);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeFlow), core);
	table = gtk_table_new(1, 3, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(box), table, FALSE, TRUE, 0);

	// �Ԋu�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		core->stamp_distance, 0.1, 3, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.distance, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeDistance), core);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);

	// �J�n��]�p�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		fabs(core->rotate_start) * 180 / G_PI, 0, 360, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.rotate_start, 1);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeRotateStart), core);
	gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);

	// ��]���x�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		fabs(core->rotate_speed) * 180 / G_PI, 0, 360, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.rotate_speed, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeRotateSpeed), core);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);

	// ��]�����ύX�p�E�B�W�F�b�g
	radio_buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.clockwise);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.counter_clockwise
	);

	if(core->rotate_direction < 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
	}

	g_object_set_data(G_OBJECT(radio_buttons[0]), "direction", GINT_TO_POINTER(-1));
	g_object_set_data(G_OBJECT(radio_buttons[0]), "application", app);
	g_signal_connect(G_OBJECT(radio_buttons[0]), "toggled", G_CALLBACK(StampCoreChangeRotateDirection), core);
	g_object_set_data(G_OBJECT(radio_buttons[1]), "direction", GINT_TO_POINTER(1));
	g_object_set_data(G_OBJECT(radio_buttons[1]), "application", app);
	g_signal_connect(G_OBJECT(radio_buttons[1]), "toggled", G_CALLBACK(StampCoreChangeRotateDirection), core);
	table = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[1], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), table, FALSE, TRUE, 0);

	// ���F���[�h�ύX�p�E�B�W�F�b�g
	if(mode != NULL)
	{
		core->mode_select[0] = radio_buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.saturation);
		core->mode_select[1] = radio_buttons[1] = gtk_radio_button_new_with_label(
			gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
			app->labels->tool_box.brightness
		);
		core->mode_select[2] = radio_buttons[2] = gtk_radio_button_new_with_label(
			gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
			app->labels->tool_box.gradation_reverse
		);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[*mode]), TRUE);
		g_object_set_data(G_OBJECT(radio_buttons[0]), "mode", GINT_TO_POINTER(0));
		g_object_set_data(G_OBJECT(radio_buttons[0]), "application", app);
		g_object_set_data(G_OBJECT(radio_buttons[0]), "stamp_core", core);
		g_signal_connect(G_OBJECT(radio_buttons[0]), "toggled", G_CALLBACK(StampCoreSetMode), mode);
		g_object_set_data(G_OBJECT(radio_buttons[1]), "mode", GINT_TO_POINTER(1));
		g_object_set_data(G_OBJECT(radio_buttons[1]), "application", app);
		g_object_set_data(G_OBJECT(radio_buttons[1]), "stamp_core", core);
		g_signal_connect(G_OBJECT(radio_buttons[1]), "toggled", G_CALLBACK(StampCoreSetMode), mode);
		g_object_set_data(G_OBJECT(radio_buttons[2]), "mode", GINT_TO_POINTER(2));
		g_object_set_data(G_OBJECT(radio_buttons[2]), "application", app);
		g_object_set_data(G_OBJECT(radio_buttons[2]), "stamp_core", core);
		g_signal_connect(G_OBJECT(radio_buttons[2]), "toggled", G_CALLBACK(StampCoreSetMode), mode);
		table = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(table), radio_buttons[0], FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(table), radio_buttons[1], FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(box), table, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(box), radio_buttons[2], FALSE, TRUE, 0);
	}

	// ���E���]�A�㉺���]�I��p�`�F�b�N�{�b�N�X
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.reverse);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.reverse_horizontally);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		core->pattern_flags & PATTERN_FLIP_HORIZONTALLY);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetPatternFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.reverse_vertically);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(1));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		core->pattern_flags & PATTERN_FLIP_VERTICALLY);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetPatternFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), table, FALSE, TRUE, 0);

	// �M���g�p�ݒ�ύX�p�E�B�W�F�b�g
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), core->flags & BRUSH_FLAG_SIZE);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), core->flags & BRUSH_FLAG_FLOW);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), table, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.randoam_size);
	g_object_set_data(G_OBJECT(check_button), "flag-value", GUINT_TO_POINTER(STAMP_RANDOM_SIZE));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), core->flags & STAMP_RANDOM_SIZE);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlag), core);
	gtk_box_pack_start(GTK_BOX(box), check_button, FALSE, TRUE, 0);
	// �����_���ɉ�]
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.random_rotate);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), (core->flags & STAMP_RANDOM_ROTATE) != 0);
	g_object_set_data(G_OBJECT(check_button), "flag-value", GUINT_TO_POINTER(STAMP_RANDOM_ROTATE));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlag), core);
	gtk_box_pack_start(GTK_BOX(box), check_button, FALSE, TRUE, 0);

	// �ڍאݒ��UI��������Ƃ��Ƀ{�^���z����J������
	g_signal_connect(G_OBJECT(box), "destroy", G_CALLBACK(OnStampDetailUIDestroy), core);

	// �X�^���v�I���e�[�u�����쐬���ăX�N���[���h�E�B���h�E�ɓ����
	table = gtk_hbox_new(FALSE, 0);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(table,
		ICON_SIZE*STAMP_SELECT_TABLE_WIDTH+4, ICON_SIZE*4+4);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
		CreateStampSelectTable(core));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(table), scrolled_window, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), table, FALSE, TRUE, 0);

	// �N���b�v�{�[�h�̃X�^���v���A�N�e�B�u�Ȃ�΃Z�b�g
	if(app->stamps.has_clip_board_pattern != FALSE)
	{
		if(core->stamp_id == 0)
		{
			app->stamps.active_pattern =
				&app->stamps.clip_board;
		}
		else
		{
			app->stamps.active_pattern =
				&app->stamps.patterns[core->stamp_id-1];
		}
	}

	// �A�N�e�B�u�ȃp�^�[�����`�����l����2�ȊO�Ȃ�Β��F���[�h�͖���
	if(mode != NULL)
	{
		gtk_widget_set_sensitive(core->mode_select[0], app->stamps.active_pattern->channel == 2);
		gtk_widget_set_sensitive(core->mode_select[1], app->stamps.active_pattern->channel == 2);
		gtk_widget_set_sensitive(core->mode_select[2], app->stamps.active_pattern->channel == 2);
	}

	core->mode = mode;

	InitializeStampCore(core, first_mode, app);
#undef UI_FONT_SIZE
#undef ICON_SIZE
}

/*************************************************************
* StampToolButtonPressCallBack�֐�                           *
* �X�^���v�c�[���Ń}�E�X�N���b�N���ꂽ�Ƃ��̃R�[���o�b�N�֐� *
* ����                                                       *
* window	: �`��̈�̏��                                 *
* x			: �}�E�X��X���W                                  *
* y			: �}�E�X��Y���W                                  *
* pressure	: �M��                                           *
* core		: �c�[���̊�{���                               *
* state		: �}�E�X�̏��                                   *
*************************************************************/
static void StampToolButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �X�^���v�c�[���̏ڍ׃f�[�^�փL���X�g
		STAMP* stamp = (STAMP*)core->brush_data;
		// �X�^���v�p�̃u���V
		cairo_pattern_t* brush;
		// �g��k���A��]�p�̍s��
		cairo_matrix_t matrix;
		// �X�^���v�̔��a�A�Z�x
		gdouble r, alpha;
		// �X�^���v������W�̍ő�ŏ�
		gdouble min_x, min_y, max_x, max_y;
		// �X�^���v�̊g��k����
		gdouble zoom;
		// �X�^���v�̕`����W�v�Z�p
		gdouble cos_x, sin_y, trans_x, trans_y;
		gdouble half_width, half_height;

		// ��]�p��������
			// �����_����]��ON/OFF�ŏ�����؂�ւ�
		if((stamp->core.flags & STAMP_RANDOM_ROTATE) == 0)
		{	// �����_����]OFF
			stamp->core.rotate = stamp->core.rotate_start;
		}
		else
		{	// �����_����]ON
			stamp->core.rotate = (rand() / (FLOAT_T)RAND_MAX) * (2*G_PI);
		}

		// �s�����x�ی�̗L���ō������[�h�ύX
		if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
		{
			window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		}
		else
		{
			window->work_layer->layer_mode = LAYER_BLEND_ATOP;
		}

		// �X�^���v�̔��a�v�Z
		if((stamp->core.flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = stamp->core.r, zoom = stamp->core.scale;
			half_width = stamp->core.half_width;
			half_height = stamp->core.half_height;
		}
		else
		{
			r =  stamp->core.r * pressure, zoom = stamp->core.scale * (1 / pressure);
			half_width = stamp->core.half_width * pressure;
			half_height = stamp->core.half_height * pressure;
		}

		// �����_���T�C�Y��ON�Ȃ痐���ŃT�C�Y�ύX
		if((stamp->core.flags & STAMP_RANDOM_SIZE) != 0)
		{
			FLOAT_T rate = rand() / (FLOAT_T)RAND_MAX;
			r *= rate;
			half_width *= rate;
			half_height *= rate;
		}

		// �X�^���v�̔Z�x�v�Z
		alpha = ((stamp->core.flags & BRUSH_FLAG_FLOW) == 0) ?
			stamp->core.flow : stamp->core.flow * pressure;

		// �O��̃X�^���v�T�[�t�F�[�X���폜���ĐV���ɍ쐬
		if(stamp->core.brush_surface != NULL)
		{
			cairo_surface_destroy(stamp->core.brush_surface);
		}
		stamp->core.brush_surface = CreatePatternSurface(
			&window->app->stamps, *core->color, window->app->tool_window.color_chooser->back_rgb,
				stamp->core.pattern_flags, stamp->mode, alpha);

		// �ő�ŏ��̍��W�v�Z
		min_x = x - r, min_y = y - r;
		max_x = x + r, max_y = y + r;

		// ���݂̍��W���L��
		stamp->core.before_x = x, stamp->core.before_y = y;

		// �`��̈悩��͂ݏo�Ă�����␳
		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		// ���s�ړ�������W���v�Z
		cos_x = cos(stamp->core.rotate), sin_y = sin(stamp->core.rotate);
		trans_x = x - (half_width * cos_x + half_height * sin_y);
		trans_y = y + (half_width * sin_y - half_height * cos_x);
		// �g��k�����A��]�p���Z�b�g
		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		brush = cairo_pattern_create_for_surface(stamp->core.brush_surface);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
		cairo_matrix_init_scale(&matrix, zoom, zoom);
		cairo_matrix_rotate(&matrix, stamp->core.rotate);
		cairo_matrix_translate(&matrix,  - trans_x, - trans_y);

		// �ړ��p�̍s����u���V�ɃZ�b�g
		cairo_pattern_set_matrix(brush, &matrix);
		// �쐬�����u���V����ƃ��C���[�ɃZ�b�g
		cairo_set_source(window->work_layer->cairo_p, brush);

		// �I��͈̗͂L���œh��ׂ����@�ύX
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{	// �I��͈͖���
			cairo_paint(window->work_layer->cairo_p);
		}
		else
		{	// �I��͈͗L
			cairo_mask_surface(window->work_layer->cairo_p, window->selection->surface_p,
				0, 0);
		}

		// �u���V�폜
		cairo_pattern_destroy(brush);

		// ��]�p���X�V
		stamp->core.rotate += stamp->core.rotate_speed;
	}
}

#define StampToolEditSelectionPressCallBack StampToolButtonPressCallBack

/*************************************************************
* StampToolMotionCallBack�֐�                                *
* �X�^���v�c�[���Ń}�E�X�h���b�O���ꂽ�Ƃ��̃R�[���o�b�N�֐� *
* ����                                                       *
* window	: �`��̈�̏��                                 *
* x			: �}�E�X��X���W                                  *
* y			: �}�E�X��Y���W                                  *
* pressure	: �M��                                           *
* core		: �c�[���̊�{���                               *
* state		: �}�E�X�̏��                                   *
*************************************************************/
static void StampToolMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^����������Ă�����
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{	// �X�^���v�c�[���̏ڍ׃f�[�^�ɃL���X�g
		STAMP* stamp = (STAMP*)core->brush_data;
		// �O�񏈗�����̋���
		gdouble d;
		// �`����W�v�Z�p
		gdouble dx, dy, cos_x, sin_y;
		gdouble half_width, half_height, zoom;

		// �O�񂩂�̋����v�Z
		dx = x - stamp->core.before_x, dy = y - stamp->core.before_y;
		d = sqrt(dx*dx + dy*dy);

		if(d >= stamp->core.d)
		{	// ���������ȏ�Ȃ�
				// �X�^���v�̔��a�A�Z�x�A�O�񂩂�̊p�x
			gdouble r, alpha, arg;
			gdouble min_x, min_y, max_x, max_y;
			int32 clear_x, clear_width, clear_y, clear_height;
			gdouble draw_x = x, draw_y = y;
			// �X�^���v�̊g��k���A��]�A���s�ړ��p�̍s��
			cairo_matrix_t matrix;
			// �X�^���v�̈ړ��ʒu�v�Z�p
			gdouble trans_x, trans_y;
			gdouble stamp_cos_x, stamp_sin_y;
			// �X�^���v�u���V
			cairo_pattern_t* brush;
			// for���p�̃J�E���^
			int i, j, k;

			// �����_����]��ON�Ȃ�p�x�ύX
			if((stamp->core.flags & STAMP_RANDOM_ROTATE) != 0)
			{
				stamp->core.rotate = (rand() / (FLOAT_T)RAND_MAX) * (2*G_PI);
			}

			arg = atan2(dy, dx);
			cos_x = cos(arg), sin_y = sin(arg);

			// �X�^���v�̔��a�v�Z
			if((stamp->core.flags & BRUSH_FLAG_SIZE) == 0)
			{
				r = stamp->core.r, zoom = stamp->core.scale;
				half_width = stamp->core.half_width;
				half_height = stamp->core.half_height;
			}
			else
			{
				r =  stamp->core.r * pressure, zoom = stamp->core.scale * (1 / pressure);
				half_width = stamp->core.half_width * pressure;
				half_height = stamp->core.half_height * pressure;
			}

			// �����_���T�C�Y��ON�Ȃ�T�C�Y�ύX
			if((stamp->core.flags & STAMP_RANDOM_SIZE) != 0)
			{
				FLOAT_T rate = rand() / (FLOAT_T)RAND_MAX;
				r *= rate;
				half_width *= rate;
				half_height *= rate;
			}

			// �X�^���v�̔Z�x�v�Z
			alpha = ((stamp->core.flags & BRUSH_FLAG_FLOW) == 0) ?
				stamp->core.flow : stamp->core.flow * pressure;

			// ���݂̍��W���L��
			stamp->core.before_x = x, stamp->core.before_y = y;

			// �ő�ŏ��̍��W���X�V
			min_x = x - r*2, min_y = y - r*2;
			max_x = x + r*2, max_y = y + r*2;

			if(min_x < 0.0)
			{
				min_x = 0.0;
			}
			if(min_y < 0.0)
			{
				min_y = 0.0;
			}
			if(max_x > window->work_layer->width)
			{
				max_x = window->work_layer->width;
			}
			if(max_y > window->work_layer->height)
			{
				max_y = window->work_layer->height;
			}

			if(core->min_x > min_x)
			{
				core->min_x = min_x;
			}
			if(core->min_y > min_y)
			{
				core->min_y = min_y;
			}
			if(core->max_x < max_x)
			{
				core->max_x = max_x;
			}
			if(core->max_y < max_y)
			{
				core->max_y = max_y;
			}

			cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
			dx = d;
			do
			{
				clear_x = (int32)(draw_x - r - 1);
				clear_width = (int32)(draw_x + r + 1);
				clear_y = (int32)(draw_y - r - 1);
				clear_height = (int32)(draw_y + r + 1);

				if(clear_x < 0)
				{
					clear_x = 0;
				}
				else if(clear_x >= window->width)
				{
					goto skip_draw;
				}
				if(clear_y < 0)
				{
					clear_y = 0;
				}
				if(clear_height >= window->height)
				{
					clear_height = window->height;
				}
				clear_width = clear_width - clear_x;
				clear_height = clear_height - clear_y;

				if(clear_width <= 0 || clear_height <= 0)
				{
					goto skip_draw;
				}

				for(i=0; i<clear_height; i++)
				{
					(void)memset(&window->temp_layer->pixels[(i+clear_y)*window->work_layer->stride+clear_x*window->work_layer->channel],
						0x0, clear_width*window->work_layer->channel);
				}

				// ���s�ړ�������W���v�Z
				stamp_cos_x = cos(stamp->core.rotate), stamp_sin_y = sin(stamp->core.rotate);
				trans_x = x - (half_width * stamp_cos_x + half_height * stamp_sin_y);
				trans_y = y + (half_width * stamp_sin_y - half_height * stamp_cos_x);
				// �g��k�����A��]�p���Z�b�g
				cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
				brush = cairo_pattern_create_for_surface(stamp->core.brush_surface);
				cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_matrix_rotate(&matrix, stamp->core.rotate);
				cairo_matrix_translate(&matrix,  - trans_x, - trans_y);

				// �ړ��p�̍s����u���V�ɃZ�b�g
				cairo_pattern_set_matrix(brush, &matrix);
				cairo_set_source(window->temp_layer->cairo_p, brush);

				// �I��͈̗͂L���ŏ����؂�ւ�
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{	// �I��͈͖���
					cairo_paint(window->temp_layer->cairo_p);
				}
				else
				{	// �I��͈͗L��
					cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);
				}

				// ���݂̍�ƃ��C���[�ɑ΂��āA���Z�x���傫�������̗p����
				for(i=0; i<clear_height; i++)
				{
					for(j=0; j<clear_width; j++)
					{
						if(window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3]
							> window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+3])
						{
							for(k=0; k<window->temp_layer->channel; k++)
							{
								window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k] =
									(uint32)(((int)window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+k]
									- (int)window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k])
										* window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3] >> 8)
										+ window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k];
							}
						}
					}
				}

				cairo_pattern_destroy(brush);

skip_draw:
				dx -= stamp->core.d;
				draw_x -= cos_x*stamp->core.d, draw_y -= sin_y*stamp->core.d;
				stamp->core.rotate += stamp->core.rotate_speed;
			} while(dx >= stamp->core.d);
		}
	}
}

#define StampToolEditSelectionMotionCallBack StampToolMotionCallBack

static void StampToolReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		STAMP* stamp = (STAMP*)core->brush_data;
		stamp->core.rotate = stamp->core.rotate_start;

		AddBrushHistory(core, window->active_layer);

		g_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, window->active_layer);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

static void StampToolEditSelectionReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		STAMP* stamp = (STAMP*)core->brush_data;
		stamp->core.rotate = stamp->core.rotate_start;

		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[window->selection->layer_mode](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		window->selection->layer_mode = SELECTION_BLEND_NORMAL;
	}
}

/*****************************************
* StampDrawCursor�֐�                    *
* �X�^���v�c�[���̃J�[�\����`�悷��     *
* ����                                   *
* window	: �`��̈�̏��             *
* x			: �}�E�X��X���W              *
* y			: �}�E�X��Y���W              *
* data		: �X�^���v�c�[���̏ڍ׃f�[�^ *
*****************************************/
static void StampToolDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	StampCoreDrawCursor(window, x, y, &((STAMP*)data)->core);
}

static void StampToolButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, STAMP* stamp)
{
	gdouble r;

	if(stamp->core.half_width > stamp->core.half_height)
	{
		r = stamp->core.half_width * 2 * window->zoom_rate + 1;
	}
	else
	{
		r = stamp->core.half_height * 2 * window->zoom_rate + 1;
	}

	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2 + 3), (gint)(r * 2 + 3));
}

static void StampToolMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, STAMP* stamp)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r;

	if(stamp->core.half_width > stamp->core.half_height)
	{
		r = stamp->core.half_width * 2 * window->zoom_rate + 1;
	}
	else
	{
		r = stamp->core.half_height * 2 * window->zoom_rate + 1;
	}

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width + 2, (gint)height + 2);
}

/*****************************************************
* CreateStampToolDetailUI�֐�                        *
* �X�^���v�c�[���̏ڍאݒ�UI���쐬����               *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* data	: �X�^���v�c�[���̏ڍ׃f�[�^                 *
* �Ԃ�l                                             *
*	�ڍאݒ�UI�E�B�W�F�b�g                           *
*****************************************************/
static GtkWidget* CreateStampToolDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
	STAMP* stamp = (STAMP*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);

	CreateStampCoreDetailUI(&stamp->core, &stamp->mode, app, vbox);

	return vbox;
}

static void ImageBrushPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		IMAGE_BRUSH *brush = (IMAGE_BRUSH*)core->brush_data;
		// �u���V�̔��a
		FLOAT_T r;
		// ���W�̍ő�l�E�ŏ��l
		FLOAT_T min_x, min_y, max_x, max_y;
		// �`��F
		FLOAT_T color[3] = {(*core->color)[0]*DIV_PIXEL,
			(*core->color)[1]*DIV_PIXEL, (*core->color)[2]*DIV_PIXEL};

		window->work_layer->layer_mode = brush->blend_mode;

		r = ((brush->core.flags & STAMP_PRESSURE_SIZE) == 0) ?
			brush->core.r : brush->core.r * pressure;

		if((brush->core.flags & STAMP_RANDOM_SIZE) != 0)
		{
			r *= rand() / (FLOAT_T)RAND_MAX;
		}

		brush->stamp_distance = 0;
		brush->core.before_x = x, brush->core.before_y = y;
		brush->points[0][1] = x, brush->points[0][2] = y;
		brush->points[0][3] = pressure;
		brush->sum_distance = brush->travel = brush->finish_length = 0;
		brush->remain_distance = brush->core.d * brush->core.d;
		brush->draw_start = r * brush->out * 0.01 * 4;
		brush->enter_length = r * brush->enter * 0.01 * 4;
		brush->enter_size = (1 - brush->enter * 0.01);
		brush->num_point = 0;
		brush->draw_finished = 0;
		brush->ref_point = 1;

		min_x = x - r, max_x = x + r;
		min_y = y - r, max_y = y + r;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		window->update.x = (int)min_x, window->update.y = (int)min_y;
		window->update.width = (int)max_x - window->update.x;
		window->update.height = (int)max_y - window->update.y;

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}	// ���N���b�N�Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

#define ImageBrushEditSelectionPress ImageBrushPressCallBack

static void ImageBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^����������Ă�����
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		IMAGE_BRUSH *brush = (IMAGE_BRUSH*)core->brush_data;
		FLOAT_T distance;
		int index = brush->ref_point % BRUSH_POINT_BUFFER_SIZE;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		dx = x-brush->core.before_x;
		dy = y-brush->core.before_y;
		distance = dx*dx + dy*dy;
		brush->stamp_distance += distance;
		brush->remain_distance -= distance;

		if(brush->remain_distance <= 0)
		{
			FLOAT_T update_x, update_y, update_r;

			distance = sqrt(distance);
			brush->core.before_x = x, brush->core.before_y = y;
			brush->sum_distance += distance;
			brush->travel += distance;
			brush->points[index][0] = distance;
			brush->points[index][1] = x, brush->points[index][2] = y;
			brush->points[index][3] = pressure;
			brush->ref_point++;

			if(brush->sum_distance >= brush->draw_start
				&& brush->sum_distance >= brush->enter_length)
			{
				// �X�V�͈͂̃C���[�W���
				cairo_t *update;
				cairo_surface_t *update_surface;
				// ����̐F�␳�p
				gdouble enter_alpha;
				// �u���V�̔��a�ƕs�����x
				gdouble r, alpha;
				// ���W�̍ő�E�ŏ��l
				gdouble min_x, min_y, max_x, max_y;
				// �u���V�̌X���p
				gdouble diff_x, diff_y;
				// �u���V�̈ړ���
				gdouble d;
				// �`����s�����W
				gdouble draw_x = brush->core.before_x, draw_y = brush->core.before_y;
				// �摜�ɑ΂��Ă̊g��E��]�����p�̍s��f�[�^
				cairo_matrix_t matrix, reset_matrix;
				// �摜�̊g�嗦
				gdouble zoom;
				// �ړ��ʌv�Z�p
				gdouble cos_x, sin_y, trans_x, trans_y;
				// �摜�̕��E�����̔���
				gdouble half_width, half_height;
				// �s�N�Z���f�[�^�����Z�b�g������W
				int start_x, start_y;
				// �`�悷�镝�A�����A��s���̃o�C�g��
				int width, height, stride;
				// �Q�ƃs�N�Z��
				uint8 *ref_pix, *mask_pix, *mask;
				// �z��̃C���f�b�N�X�p
				int ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				int before_point;
				int i, j;	// for���p�̃J�E���^

				cairo_matrix_init_scale(&reset_matrix, 1, 1);

				while(brush->sum_distance > brush->draw_start
					&& brush->draw_finished < brush->ref_point-1
					&& brush->remain_distance <= 0)
				{
					if(brush->finish_length < brush->enter_length)
					{
						enter_alpha = brush->finish_length / brush->enter_length;
						r = brush->enter_size + brush->finish_length * 0.25;
					}
					else
					{
						enter_alpha = 1, r = 1;
					}
					if((brush->core.flags & STAMP_PRESSURE_SIZE) != 0)
					{
						r *= brush->points[ref_point][3];
					}
					if((brush->core.flags & STAMP_RANDOM_SIZE) != 0)
					{
						r *= 1 - (brush->size_range * (rand() / (FLOAT_T)RAND_MAX));
					}

					if(r < MIN_BRUSH_STEP)
					{
						r = MIN_BRUSH_STEP;
					}

					half_width = brush->core.half_width * r;
					half_height = brush->core.half_height * r;
					zoom = (1/r) * brush->core.scale;
					r *= brush->core.r;

					alpha = ((brush->core.flags & BRUSH_FLAG_FLOW) == 0)
						? 1 : brush->points[ref_point][3];
					alpha *= enter_alpha;

					before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
					d = brush->points[ref_point][0];
					brush->sum_distance -= d;
					if(brush->draw_finished == 0)
					{
						draw_x = brush->points[0][1], draw_y = brush->points[0][2];
					}
					else
					{
						draw_x = brush->points[before_point][1];
						draw_y = brush->points[before_point][2];
					}
					dx = brush->points[ref_point][1] - draw_x;
					dy = brush->points[ref_point][2] - draw_y;
					if((brush->core.flags & STAMP_RANDOM_ROTATE) != 0)
					{
						brush->core.rotate = brush->core.rotate_start +
							(rand() / (FLOAT_T)RAND_MAX) * brush->core.rotate_direction * brush->rotate_range;
						if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
						{
							brush->core.rotate += brush->core.rotate_direction * atan2(dy, dx);
						}
					}
					else if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
					{
						brush->core.rotate = brush->core.rotate_start + brush->core.rotate_direction * atan2(dy, dx);
					}
					else
					{
						brush->core.rotate = brush->core.rotate_start;
					}
					cos_x = cos(brush->core.rotate), sin_y = sin(brush->core.rotate);
					diff_x = brush->core.d * dx/d, diff_y = brush->core.d * dy/d;

					min_x = brush->points[ref_point][1] - r - 1;
					max_x = brush->points[ref_point][1] + r + 1;
					min_y = brush->points[ref_point][2] - r - 1;
					max_y = brush->points[ref_point][2] + r + 1;

					if(min_x < 0.0)
					{
						min_x = 0.0;
					}
					if(core->min_x > min_x)
					{
						core->min_x = min_x;
					}
					if(min_y < 0.0)
					{
						min_y = 0.0;
					}
					if(core->min_y > min_y)
					{
						core->min_y = min_y;
					}
					if(max_x > window->work_layer->width)
					{
						max_x = window->work_layer->width;
					}
					if(core->max_x < max_x)
					{
						core->max_x = max_x;
					}
					if(max_y > window->work_layer->height)
					{
						min_y = window->work_layer->height;
					}
					if(core->max_y < max_y)
					{
						core->max_y = max_y;
					}

					if(window->update.x > min_x)
					{
						window->update.width += window->update.x - (int)min_x;
						window->update.x = (int)min_x;
					}
					if(window->update.width + window->update.x < max_x)
					{
						window->update.width += (int)max_x - window->update.width + window->update.x;
					}
					if(window->update.y > min_y)
					{
						window->update.height += window->update.y - (int)min_y;
						window->update.y = (int)min_y;
					}
					if(window->update.height + window->update.y < max_y)
					{
						window->update.height += (int)max_y - window->update.height + window->update.y;
					}

					if(brush->stamp_distance >= brush->core.d)
					{
						dx = d;
						do
						{
							start_x = (int)(draw_x - r);
							start_y = (int)(draw_y - r);
							width = (int)(draw_x + r);
							height = (int)(draw_y + r);

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(start_x > window->work_layer->width)
							{
								goto skip_draw;
							}
							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(start_y > window->work_layer->height)
							{
								goto skip_draw;
							}
							if(width > window->work_layer->width)
							{
								width = window->work_layer->width - start_x;
							}
							else
							{
								width = width - start_x;
							}
							if(height > window->work_layer->height)
							{
								height = window->work_layer->height - start_y;
							}
							else
							{
								height = height - start_y;
							}

							if(width < 0 || height < 0)
							{
								goto skip_draw;
							}

							stride = width*4;

							update_surface = cairo_surface_create_for_rectangle(
								window->mask_temp->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
							update = cairo_create(update_surface);

							for(i=0; i<height; i++)
							{
								(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
									0, stride);
							}

							trans_x = r - (half_width * cos_x + half_height * sin_y);
							trans_y = r + (half_width * sin_y - half_height * cos_x);

							// �s��f�[�^�Ɋg��E�k���A��]�p���Z�b�g
							cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_matrix_rotate(&matrix, brush->core.rotate);
							cairo_matrix_translate(&matrix, - trans_x, - trans_y);

							mask = window->mask_temp->pixels;
							if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
							{
								if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
								{
									cairo_set_source(update, core->brush_pattern);
									cairo_pattern_set_matrix(core->brush_pattern, &matrix);
									cairo_paint_with_alpha(update, alpha);
								}
								else
								{
									cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
									cairo_set_source(core->temp_cairo, core->brush_pattern);
									cairo_paint_with_alpha(core->temp_cairo, alpha);
									cairo_set_source(update, core->temp_pattern);
									cairo_pattern_set_matrix(core->temp_pattern, &matrix);
									cairo_mask_surface(update,
										window->active_layer->surface_p, - draw_x + r, - draw_y + r);
								}
							}
							else
							{
								if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
								{
									cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
									cairo_set_source(core->temp_cairo, core->brush_pattern);
									cairo_paint_with_alpha(core->temp_cairo, alpha);
									cairo_set_source(update, core->temp_pattern);
									cairo_pattern_set_matrix(core->temp_pattern, &matrix);
									cairo_mask_surface(update,
										window->selection->surface_p, - draw_x + r, - draw_y + r);
								}
								else
								{
									cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
										window->temp_layer->surface_p, draw_x-r, draw_y-r, r*2+1, r*2+1
									);
									cairo_t *update_temp = cairo_create(temp_surface);

									for(i=0; i<height; i++)
									{
										(void)memset(&window->temp_layer->pixels[
											(i+start_y)*window->temp_layer->stride+start_x*4],
											0, stride
										);
									}

									cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
									cairo_set_source(core->temp_cairo, core->brush_pattern);
									cairo_paint_with_alpha(core->temp_cairo, alpha);
									cairo_set_source(update, core->temp_pattern);
									cairo_mask_surface(update,
										window->selection->surface_p, - draw_x + r, - draw_y + r);
									cairo_set_source_surface(update_temp,
										update_surface, 0, 0);
									cairo_mask_surface(update_temp,
										window->active_layer->surface_p, - draw_x + r, - draw_y + r);

									mask = window->temp_layer->pixels;

									cairo_surface_destroy(temp_surface);
									cairo_destroy(update_temp);
								}
							}

							// �e�N�X�`����K�p
							if(window->app->textures.active_texture != 0)
							{
								cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
									window->temp_layer->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
								cairo_t *update_temp = cairo_create(temp_surface);

								if(mask == window->mask_temp->pixels)
								{
									for(i=0; i<height; i++)
									{
										(void)memset(&window->temp_layer->pixels[
											(i+start_y)*window->temp_layer->stride+start_x*4],
											0, stride
										);
									}
									cairo_set_source_surface(update_temp,
										update_surface, 0, 0);
									cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);
									mask = window->temp_layer->pixels;
								}
								else
								{
									for(i=0; i<height; i++)
									{
										(void)memset(&window->mask_temp->pixels[
											(i+start_y)*window->mask_temp->stride+start_x*4],
											0, stride
										);
									}
									cairo_set_source_surface(update,
										temp_surface, 0, 0);
									cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
									mask = window->mask_temp->pixels;
								}

								cairo_surface_destroy(temp_surface);
								cairo_destroy(update_temp);
							}

							cairo_surface_destroy(update_surface);
							cairo_destroy(update);

							for(i=0; i<height; i++)
							{
								ref_pix = &window->work_layer->pixels[
									(start_y+i)*window->work_layer->stride+start_x*4];
								mask_pix = &mask[(start_y+i)*window->work_layer->stride
									+start_x*4];

								for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
								{
									if(ref_pix[3] < mask_pix[3])
									{
										ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
											* mask_pix[3] >> 8) + ref_pix[0]);
										ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
											* mask_pix[3] >> 8) + ref_pix[1]);
										ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
											* mask_pix[3] >> 8) + ref_pix[2]);
										ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
											* mask_pix[3] >> 8) + ref_pix[3]);
									}
								}
							}

							update_r = r * window->zoom_rate;
							update_x = ((draw_x-window->width/2)*window->cos_value + (draw_y-window->height/2)*window->sin_value) * window->zoom_rate
								+ window->rev_add_cursor_x;
							update_y = (- (draw_x-window->width/2)*window->sin_value + (draw_y-window->height/2)*window->cos_value) * window->zoom_rate
								+ window->rev_add_cursor_y;
							gtk_widget_queue_draw_area(window->window, (gint)(update_x-update_r), (gint)(update_y-update_r),
								(gint)(update_r * 2 + BRUSH_UPDATE_MARGIN), (gint)(update_r * 2 + BRUSH_UPDATE_MARGIN));
skip_draw:
							dx -= brush->core.d;
							brush->remain_distance += brush->core.d * brush->core.d;
							brush->stamp_distance -= brush->core.d;
							if(brush->remain_distance > 0)
							{
								break;
							}
							else
							{
								draw_x += diff_x, draw_y += diff_y;
							}
						} while(1);
					}

					brush->finish_length += d;
					brush->travel += d;
					brush->draw_finished++;
					ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				}	// while(brush->sum_distance > brush->draw_start
						// && brush->draw_finished < brush->ref_point)
			}	// if(brush->sum_distance >= brush->draw_start
					// && brush->sum_distance >= brush->enter_length)
		}	// if(distance >= 1.0)

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}	// �}�E�X�̍��{�^����������Ă�����
		// if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
}

#define ImageBrushEditSelectionMotion ImageBrushMotionCallBack

static void ImageBrushReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^���Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		IMAGE_BRUSH *brush = (IMAGE_BRUSH*)core->brush_data;
		// �X�V�͈͂̃C���[�W���
		cairo_t *update;
		cairo_surface_t *update_surface;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̈ړ���
		gdouble d;
		// �`����s�����W
		gdouble draw_x, draw_y;
		// ��ʍX�V���s�����W�A�͈�
		gdouble update_x, update_y, update_r;
		// ����A�������̐F�␳
		gdouble enter_alpha, out_alpha;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �摜�ɑ΂��Ă̊g��E��]�����p�̍s��f�[�^
		cairo_matrix_t matrix, reset_matrix;
		// �摜�̊g�嗦
		gdouble zoom;
		// �ړ��ʌv�Z�p
		gdouble cos_x, sin_y, trans_x, trans_y;
		// �摜�̕��E�����̔���
		gdouble half_width, half_height;
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask;
		// �z��̃C���f�b�N�X�p
		int ref_point;
		int before_point;
		int i, j;	// for���p�̃J�E���^

		ref_point = brush->ref_point % BRUSH_POINT_BUFFER_SIZE;
		brush->points[ref_point][1] = x, brush->points[ref_point][2] = y;
		brush->ref_point++;

		while(brush->ref_point > brush->draw_finished)
		{
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;

			if(brush->finish_length < brush->enter_length)
			{
				enter_alpha = brush->finish_length / brush->enter_length;
				r = brush->enter_size + brush->finish_length * 0.25;
			}
			else
			{
				enter_alpha = 1;
				r = 1;
			}

			if((brush->core.flags & STAMP_PRESSURE_SIZE) != 0)
			{
				r *= brush->points[ref_point][3];
			}
			if((brush->core.flags & STAMP_RANDOM_SIZE) != 0)
			{
				r *= 1 - (brush->size_range * (rand() / (FLOAT_T)RAND_MAX));
			}
			if(r < MIN_BRUSH_STEP)
			{
				r = MIN_BRUSH_STEP;
			}

			if((brush->core.flags & STAMP_RANDOM_ROTATE) != 0)
			{
				brush->core.rotate = (rand() / (FLOAT_T)RAND_MAX) * (2*G_PI);
			}

			half_width = brush->core.half_width * r;
			half_height = brush->core.half_height * r;
			zoom = (1/r) * brush->core.scale;
			r *= brush->core.r;

			if(brush->sum_distance < brush->draw_start)
			{
				out_alpha = brush->sum_distance / brush->draw_start;
				r -= (brush->draw_start - brush->sum_distance) * 0.25;
			}
			else
			{
				out_alpha = 1;
			}

			if(r < 0.0)
			{
				brush->draw_finished++;
				continue;
			}

			alpha = ((brush->core.flags & BRUSH_FLAG_FLOW) == 0)
				? 1 : brush->points[ref_point][3];
			alpha *= enter_alpha;

			before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
			d = brush->points[ref_point][0];
			brush->sum_distance -= d;
			if(brush->draw_finished == 0)
			{
				draw_x = brush->points[0][1], draw_y = brush->points[0][2];
			}
			else
			{
				draw_x = brush->points[before_point][1];
				draw_y = brush->points[before_point][2];
			}
			dx = brush->points[ref_point][1] - draw_x;
			dy = brush->points[ref_point][2] - draw_y;

			if((brush->core.flags & STAMP_RANDOM_ROTATE) != 0)
			{
				brush->core.rotate = brush->core.rotate_start +
					(rand() / (FLOAT_T)RAND_MAX) * brush->core.rotate_direction * brush->rotate_range;
				if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
				{
					brush->core.rotate += brush->core.rotate_direction * atan2(dy, dx);
				}
			}
			else if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
			{
				brush->core.rotate = brush->core.rotate_start + brush->core.rotate_direction * atan2(dy, dx);
			}
			else
			{
				brush->core.rotate = brush->core.rotate_start;
			}
			cos_x = cos(brush->core.rotate), sin_y = sin(brush->core.rotate);
			diff_x = dx/d, diff_y = dy/d;

			min_x = brush->points[ref_point][1] - r - 1;
			max_x = brush->points[ref_point][1] + r + 1;
			min_y = brush->points[ref_point][2] - r - 1;
			max_y = brush->points[ref_point][2] + r + 1;

			if(min_x < 0.0)
			{
				min_x = 0.0;
			}
			if(core->min_x > min_x)
			{
				core->min_x = min_x;
			}
			if(min_y < 0.0)
			{
				min_y = 0.0;
			}
			if(core->min_y > min_y)
			{
				core->min_y = min_y;
			}
			if(max_x > window->work_layer->width)
			{
				max_x = window->work_layer->width;
			}
			if(core->max_x < max_x)
			{
				core->max_x = max_x;
			}
			if(max_y > window->work_layer->height)
			{
				min_y = window->work_layer->height;
			}
			if(core->max_y < max_y)
			{
				core->max_y = max_y;
			}

			if(window->update.x > min_x)
			{
				window->update.width += window->update.x - (int)min_x;
				window->update.x = (int)min_x;
			}
			if(window->update.width + window->update.x < max_x)
			{
				window->update.width += (int)max_x - window->update.width + window->update.x;
			}
			if(window->update.y > min_y)
			{
				window->update.height += window->update.y - (int)min_y;
				window->update.y = (int)min_y;
			}
			if(window->update.height + window->update.y < max_y)
			{
				window->update.height += (int)max_y - window->update.height + window->update.y;
			}

			if(brush->stamp_distance > brush->core.d || brush->draw_finished == 0)
			{
				dx = d;
				do
				{
					start_x = (int)(draw_x - r);
					start_y = (int)(draw_y - r);
					width = (int)(draw_x + r);
					height = (int)(draw_y + r);

					if(start_x < 0)
					{
						start_x = 0;
					}
					else if(start_x > window->work_layer->width)
					{
						goto skip_draw;
					}
					if(start_y < 0)
					{
						start_y = 0;
					}
					else if(start_y > window->work_layer->height)
					{
						goto skip_draw;
					}
					if(width > window->work_layer->width)
					{
						width = window->work_layer->width - start_x;
					}
					else
					{
						width = width - start_x;
					}
					if(height > window->work_layer->height)
					{
						height = window->work_layer->height - start_y;
					}
					else
					{
						height = height - start_y;
					}

					if(width < 0 || height < 0)
					{
						goto skip_draw;
					}

					stride = width*4;

					update_surface = cairo_surface_create_for_rectangle(
						window->mask_temp->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
					update = cairo_create(update_surface);

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride);
					}

					trans_x = r - (half_width * cos_x + half_height * sin_y);
					trans_y = r + (half_width * sin_y - half_height * cos_x);

					// �s��f�[�^�Ɋg��E�k���A��]�p���Z�b�g
					cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_matrix_rotate(&matrix, brush->core.rotate);
					cairo_matrix_translate(&matrix, - trans_x, - trans_y);

					mask = window->mask_temp->pixels;
					if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_set_source(update, core->brush_pattern);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_paint_with_alpha(update, alpha);
						}
						else
						{
							cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_set_source(update, core->temp_pattern);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_mask_surface(update,
								window->active_layer->surface_p, - draw_x + r, - draw_y + r);
						}
					}
					else
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_set_source(update, core->temp_pattern);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_mask_surface(update,
								window->selection->surface_p, - draw_x + r, - draw_y + r);
						}
						else
						{
							cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
								window->temp_layer->surface_p, draw_x-r, draw_y-r, r*2+1, r*2+1
							);
							cairo_t *update_temp = cairo_create(temp_surface);

							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}

							cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_set_source(update, core->temp_pattern);
							cairo_mask_surface(update,
								window->selection->surface_p, - draw_x + r, - draw_y + r);
							cairo_set_source_surface(update_temp,
								update_surface, 0, 0);
							cairo_mask_surface(update_temp,
								window->active_layer->surface_p, - draw_x + r, - draw_y + r);

							mask = window->temp_layer->pixels;

							cairo_surface_destroy(temp_surface);
							cairo_destroy(update_temp);
						}
					}

					// �e�N�X�`����K�p
					if(window->app->textures.active_texture != 0)
					{
						cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
						cairo_t *update_temp = cairo_create(temp_surface);

						if(mask == window->mask_temp->pixels)
						{
							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}
							cairo_set_source_surface(update_temp,
								update_surface, 0, 0);
							cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);
							mask = window->temp_layer->pixels;
						}
						else
						{
							for(i=0; i<height; i++)
							{
								(void)memset(&window->mask_temp->pixels[
									(i+start_y)*window->mask_temp->stride+start_x*4],
									0, stride
								);
							}
							cairo_set_source_surface(update,
								temp_surface, 0, 0);
							cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
							mask = window->mask_temp->pixels;
						}

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}

					cairo_surface_destroy(update_surface);
					cairo_destroy(update);

					for(i=0; i<height; i++)
					{
						ref_pix = &window->work_layer->pixels[
							(start_y+i)*window->work_layer->stride+start_x*4];
						mask_pix = &mask[(start_y+i)*window->work_layer->stride
							+start_x*4];

						for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
						{
							if(ref_pix[3] < mask_pix[3])
							{
								ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
									* mask_pix[3] >> 8) + ref_pix[0]);
								ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
									* mask_pix[3] >> 8) + ref_pix[1]);
								ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
									* mask_pix[3] >> 8) + ref_pix[2]);
								ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
									* mask_pix[3] >> 8) + ref_pix[3]);
							}
						}
					}

					update_r = r * window->zoom_rate;
					update_x = ((draw_x-window->width/2)*window->cos_value + (draw_y-window->height/2)*window->sin_value) * window->zoom_rate
						+ window->rev_add_cursor_x;
					update_y = (- (draw_x-window->width/2)*window->sin_value + (draw_y-window->height/2)*window->cos_value) * window->zoom_rate
						+ window->rev_add_cursor_y;
					gtk_widget_queue_draw_area(window->window, (gint)(update_x-update_r), (gint)(update_y-update_r),
						(gint)(update_r * 2 + BRUSH_UPDATE_MARGIN), (gint)(update_r * 2 + BRUSH_UPDATE_MARGIN));
skip_draw:
					dx -= brush->core.d;
					brush->stamp_distance -= brush->core.d;
					if(dx < brush->core.d)
					{
						break;
					}
					else
					{
						draw_x += diff_x, draw_y += diff_y;
					}
				} while(1);
			}

			brush->finish_length += d;
			brush->travel += d;
			brush->draw_finished++;
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
		}	// while(brush->ref_point > brush->draw_finished)

		AddBrushHistory(core, window->active_layer);

		window->update.surface_p = cairo_surface_create_for_rectangle(
			window->active_layer->surface_p, window->update.x, window->update.y,
				window->update.width, window->update.height);
		window->update.cairo_p = cairo_create(window->update.surface_p);
		g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->update);
		cairo_destroy(window->update.cairo_p);
		cairo_surface_destroy(window->update.surface_p);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;

		brush->core.rotate = brush->core.rotate_start;

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}	// �}�E�X�̍��{�^���Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

static void ImageBrushEditSelectionRelease(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^���Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		IMAGE_BRUSH *brush = (IMAGE_BRUSH*)core->brush_data;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̈ړ���
		gdouble d;
		// �`����s�����W
		gdouble draw_x, draw_y;
		// ����A�������̐F�␳
		gdouble enter_alpha, out_alpha;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �摜�ɑ΂��Ă̊g��E��]�����p�̍s��f�[�^
		cairo_matrix_t matrix, reset_matrix;
		// �摜�̊g�嗦
		gdouble zoom;
		// �ړ��ʌv�Z�p
		gdouble cos_x, sin_y, trans_x, trans_y;
		// �摜�̕��E�����̔���
		gdouble half_width, half_height;
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask;
		// �z��̃C���f�b�N�X�p
		int ref_point;
		int before_point;
		int i, j;	// for���p�̃J�E���^

		ref_point = brush->ref_point % BRUSH_POINT_BUFFER_SIZE;
		brush->points[ref_point][1] = x, brush->points[ref_point][2] = y;
		brush->ref_point++;

		while(brush->ref_point > brush->draw_finished)
		{
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;

			if(brush->finish_length < brush->enter_length)
			{
				enter_alpha = brush->finish_length / brush->enter_length;
				r = brush->enter_size + brush->finish_length * 0.25;
			}
			else
			{
				enter_alpha = 1;
				r = 1;
			}

			if((brush->core.flags & STAMP_PRESSURE_SIZE) != 0)
			{
				r *= brush->points[ref_point][3];
			}
			if((brush->core.flags & STAMP_RANDOM_SIZE) != 0)
			{
				r *= 1 - (brush->size_range * (rand() / (FLOAT_T)RAND_MAX));
			}
			if(r < MIN_BRUSH_STEP)
			{
				r = MIN_BRUSH_STEP;
			}

			if((brush->core.flags & STAMP_RANDOM_ROTATE) != 0)
			{
				brush->core.rotate = (rand() / (FLOAT_T)RAND_MAX) * (2*G_PI);
			}

			half_width = brush->core.half_width * r;
			half_height = brush->core.half_height * r;
			zoom = (1/r) * brush->core.scale;
			r *= brush->core.r;

			if(brush->sum_distance < brush->draw_start)
			{
				out_alpha = brush->sum_distance / brush->draw_start;
				r -= (brush->draw_start - brush->sum_distance) * 0.25;
			}
			else
			{
				out_alpha = 1;
			}

			if(r < 0.0)
			{
				brush->draw_finished++;
				continue;
			}

			alpha = ((brush->core.flags & BRUSH_FLAG_FLOW) == 0)
				? brush->core.flow : brush->core.flow * brush->points[ref_point][3];
			alpha *= enter_alpha;

			if(brush->core.brush_surface != NULL)
			{
				cairo_surface_destroy(brush->core.brush_surface);
			}
			brush->core.brush_surface = CreatePatternSurface(
				&window->app->stamps, *core->color, *core->back_color, brush->core.pattern_flags, brush->mode, alpha);

			before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
			d = brush->points[ref_point][0];
			brush->sum_distance -= d;
			if(brush->draw_finished == 0)
			{
				draw_x = brush->points[0][1], draw_y = brush->points[0][2];
			}
			else
			{
				draw_x = brush->points[before_point][1];
				draw_y = brush->points[before_point][2];
			}
			dx = brush->points[ref_point][1] - draw_x;
			dy = brush->points[ref_point][2] - draw_y;

			if((brush->core.flags & STAMP_RANDOM_ROTATE) != 0)
			{
				brush->core.rotate = brush->core.rotate_start +
					(rand() / (FLOAT_T)RAND_MAX) * brush->core.rotate_direction * brush->rotate_range;
				if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
				{
					brush->core.rotate += brush->core.rotate_direction * atan2(dy, dx);
				}
			}
			else if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
			{
				brush->core.rotate = brush->core.rotate_start + brush->core.rotate_direction * atan2(dy, dx);
			}
			else
			{
				brush->core.rotate = brush->core.rotate_start;
			}

			cos_x = cos(brush->core.rotate), sin_y = sin(brush->core.rotate);
			diff_x = dx/d, diff_y = dy/d;

			min_x = brush->points[ref_point][1] - r - 1;
			max_x = brush->points[ref_point][1] + r + 1;
			min_y = brush->points[ref_point][2] - r - 1;
			max_y = brush->points[ref_point][2] + r + 1;

			if(min_x < 0.0)
			{
				min_x = 0.0;
			}
			if(core->min_x > min_x)
			{
				core->min_x = min_x;
			}
			if(min_y < 0.0)
			{
				min_y = 0.0;
			}
			if(core->min_y > min_y)
			{
				core->min_y = min_y;
			}
			if(max_x > window->work_layer->width)
			{
				max_x = window->work_layer->width;
			}
			if(core->max_x < max_x)
			{
				core->max_x = max_x;
			}
			if(max_y > window->work_layer->height)
			{
				min_y = window->work_layer->height;
			}
			if(core->max_y < max_y)
			{
				core->max_y = max_y;
			}

			if(brush->stamp_distance > brush->core.d)
			{
				dx = d;
				do
				{
					start_x = (int)(draw_x - r);
					start_y = (int)(draw_y - r);
					width = (int)(draw_x + r);
					height = (int)(draw_y + r);

					if(start_x < 0)
					{
						start_x = 0;
					}
					else if(start_x > window->work_layer->width)
					{
						goto skip_draw;
					}
					if(start_y < 0)
					{
						start_y = 0;
					}
					else if(start_y > window->work_layer->height)
					{
						goto skip_draw;
					}
					if(width > window->work_layer->width)
					{
						width = window->work_layer->width - start_x;
					}
					else
					{
						width = width - start_x;
					}
					if(height > window->work_layer->height)
					{
						height = window->work_layer->height - start_y;
					}
					else
					{
						height = height - start_y;
					}

					if(width < 0 || height < 0)
					{
						goto skip_draw;
					}
					stride = width*4;

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride);
					}

					trans_x = draw_x - (half_width * cos_x + half_height * sin_y);
					trans_y = draw_y + (half_width * sin_y - half_height * cos_x);
					// �s��f�[�^�Ɋg��E�k���A��]�p���Z�b�g
					cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_matrix_rotate(&matrix, brush->core.rotate);
					cairo_matrix_translate(&matrix, - trans_x, - trans_y);

					mask = window->mask_temp->pixels;
					if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_set_source(window->mask_temp->cairo_p, core->brush_pattern);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_paint_with_alpha(window->mask_temp->cairo_p, alpha);
						}
						else
						{
							cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_set_source(window->mask_temp->cairo_p, core->temp_pattern);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_mask_surface(window->mask_temp->cairo_p,
								window->active_layer->surface_p, 0, 0);
						}
					}
					else
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_set_source(window->mask_temp->cairo_p, core->temp_pattern);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_mask_surface(window->mask_temp->cairo_p,
								window->selection->surface_p, 0, 0);
						}
						else
						{
							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}

							cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_set_source(window->mask_temp->cairo_p, core->temp_pattern);
							cairo_mask_surface(window->mask_temp->cairo_p,
								window->selection->surface_p, 0, 0);
							cairo_set_source_surface(window->temp_layer->cairo_p,
								window->mask_temp->surface_p, 0, 0);
							cairo_mask_surface(window->temp_layer->cairo_p,
								window->active_layer->surface_p, 0, 0);

							mask = window->temp_layer->pixels;
						}
					}

					// �e�N�X�`����K�p
					if(window->app->textures.active_texture != 0)
					{
						if(mask == window->mask_temp->pixels)
						{
							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}
							cairo_set_source_surface(window->temp_layer->cairo_p,
								window->mask_temp->surface_p, 0, 0);
							cairo_mask_surface(window->temp_layer->cairo_p, window->texture->surface_p, 0, 0);
							mask = window->temp_layer->pixels;
						}
						else
						{
							for(i=0; i<height; i++)
							{
								(void)memset(&window->mask_temp->pixels[
									(i+start_y)*window->mask_temp->stride+start_x*4],
									0, stride
								);
							}
							cairo_set_source_surface(window->mask_temp->cairo_p,
								window->temp_layer->surface_p, 0, 0);
							cairo_mask_surface(window->mask_temp->cairo_p, window->texture->surface_p, 0, 0);
							mask = window->mask_temp->pixels;
						}
					}

					for(i=0; i<height; i++)
					{
						ref_pix = &window->work_layer->pixels[
							(start_y+i)*window->work_layer->stride+start_x*4];
						mask_pix = &mask[(start_y+i)*window->work_layer->stride
							+start_x*4];

						for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
						{
							if(ref_pix[3] < mask_pix[3])
							{
								ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
									* mask_pix[3] >> 8) + ref_pix[0]);
								ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
									* mask_pix[3] >> 8) + ref_pix[1]);
								ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
									* mask_pix[3] >> 8) + ref_pix[2]);
								ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
									* mask_pix[3] >> 8) + ref_pix[3]);
							}
						}
					}
skip_draw:
					dx -= brush->core.d;
					brush->stamp_distance -= brush->core.d;
					if(dx < brush->core.d)
					{
						break;
					}
					else
					{
						draw_x += diff_x, draw_y += diff_y;
					}
				} while(1);
			}

			brush->finish_length += d;
			brush->travel += d;
			brush->draw_finished++;
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
		}	// while(brush->ref_point > brush->draw_finished)

		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[window->selection->layer_mode](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;

		brush->core.rotate = brush->core.rotate_start;
	}	// �}�E�X�̍��{�^���Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

static void ImageBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	StampCoreDrawCursor(window, x, y, &((IMAGE_BRUSH*)data)->core);
}

static void ImageBrushButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, IMAGE_BRUSH* brush)
{
	gdouble r;

	if(brush->core.half_width > brush->core.half_height)
	{
		r = brush->core.half_width * 2 * window->zoom_rate + 1;
	}
	else
	{
		r = brush->core.half_height * 2 * window->zoom_rate + 1;
	}

	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2) + 2, (gint)(r * 2) + 2);
}

static void ImageBrushMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, IMAGE_BRUSH* brush)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r;
	gdouble enter = (brush->enter_length + brush->draw_start) * window->zoom_rate;

	if(brush->core.half_width > brush->core.half_height)
	{
		r = brush->core.half_width * 2 * window->zoom_rate + 1;
	}
	else
	{
		r = brush->core.half_height * 2 * window->zoom_rate + 1;
	}

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	start_x -= enter * 4;
	width += enter * 4 * 2 + 2;
	start_y -= enter * 4;
	height += enter * 4 * 2 + 2;

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width, (gint)height);
}

static void ImageBrushChangeColor(const uint8 color[3], void* data)
{
	IMAGE_BRUSH *brush = (IMAGE_BRUSH*)data;
	InitializeStampCore(&brush->core, brush->mode, brush->core.app);
}

static void ImageBrushSetEnter(GtkAdjustment* slider, IMAGE_BRUSH* brush)
{
	brush->enter = gtk_adjustment_get_value(slider);
}

static void ImageBrushSetOut(GtkAdjustment* slider, IMAGE_BRUSH* brush)
{
	brush->out = gtk_adjustment_get_value(slider);
}

static void ImageBrushSetBlendMode(GtkComboBox* combo, IMAGE_BRUSH* brush)
{
	brush->blend_mode = (uint16)gtk_combo_box_get_active(combo);
}

static void ImageBrushSetRotate2Direction(GtkWidget* button, IMAGE_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->core.flags &= ~(BRUSH_FLAG_ROTATE);
	}
	else
	{
		brush->core.flags |= BRUSH_FLAG_ROTATE;
	}
}

static void ImageBrushSetRotateDirection(GtkWidget* button, IMAGE_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		brush->core.rotate_direction = GPOINTER_TO_INT(g_object_get_data(
			G_OBJECT(button), "rotate_direction"));
	}
}

static void ImageBrushSetSizeRange(GtkAdjustment* slider, IMAGE_BRUSH* brush)
{
	brush->size_range = gtk_adjustment_get_value(slider) * 0.01;
}

static void ImageBrushSetRotateRange(GtkAdjustment* slider, IMAGE_BRUSH* brush)
{
	brush->random_rotate_range = (int16)gtk_adjustment_get_value(slider);
	brush->rotate_range = brush->random_rotate_range * G_PI / 180;
}

static GtkWidget* CreateImageBrushDetailUI(APPLICATION* app, BRUSH_CORE* brush_core)
{
#define UI_FONT_SIZE 8.0
#define ICON_SIZE 32
	IMAGE_BRUSH *brush = (IMAGE_BRUSH*)brush_core->brush_data;
	STAMP_CORE *core = &brush->core;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	// ���x���A�X���C�_
	GtkWidget* label, *scale;
	// �X�N���[���h�E�B���h�E
	GtkWidget* scrolled_window;
	// �E�B�W�F�b�g����p�̃e�[�u��
	GtkWidget* table;
	// ��]�����A���[�h�ݒ�p�̃��W�I�{�^��
	GtkWidget* radio_buttons[PATTERN_MODE_NUM];
	// �M���g�p�ݒ�̃`�F�b�N�{�b�N�X
	GtkWidget* check_button;
	// �������[�h�I��p�̃R���{�{�b�N�X
	GtkWidget* combo;
	// �X���C�_�Ɏg�p����A�W���X�^
	GtkAdjustment* scale_adjustment;
	// ���x���̃t�H���g�ύX�p�̃}�[�N�A�b�v�o�b�t�@
	char mark_up_buff[256];
	int i;	// for���p�̃J�E���^

	// �A�v���P�[�V�����Ǘ��̍\���̂ւ̃|�C���^���Z�b�g
	core->app = app;

	// �N���b�v�{�[�h�̃p�^�[�����X�V
	UpdateClipBoardPattern(&app->stamps);

	// �g��k�����ݒ�p�̃E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		(1 / core->scale) * 100, 10, 300, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeScale), core);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �Z�x�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		core->flow * 100, 0, 100, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.flow, 1);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeFlow), core);
	table = gtk_table_new(1, 3, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �J�n��]�p�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		fabs(core->rotate_start) * 180 / G_PI, 0, 360, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.rotate_start, 1);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeRotateStart), core);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �Ԋu�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		core->stamp_distance, 0.1, 3, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.distance, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeDistance), core);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	core->mode = &brush->mode;
	core->mode_select[0] = radio_buttons[0] =
		gtk_radio_button_new_with_label(NULL, app->labels->tool_box.saturation);
	core->mode_select[1] = radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.brightness
	);
	core->mode_select[2] = radio_buttons[2] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.gradation_reverse
	);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[brush->mode]), TRUE);
	g_object_set_data(G_OBJECT(radio_buttons[0]), "mode", GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(radio_buttons[0]), "application", app);
	g_object_set_data(G_OBJECT(radio_buttons[0]), "stamp_core", core);
	g_signal_connect(G_OBJECT(radio_buttons[0]), "toggled", G_CALLBACK(StampCoreSetMode), &brush->mode);
	g_object_set_data(G_OBJECT(radio_buttons[1]), "mode", GINT_TO_POINTER(1));
	g_object_set_data(G_OBJECT(radio_buttons[1]), "application", app);
	g_object_set_data(G_OBJECT(radio_buttons[1]), "stamp_core", core);
	g_signal_connect(G_OBJECT(radio_buttons[1]), "toggled", G_CALLBACK(StampCoreSetMode), &brush->mode);
	g_object_set_data(G_OBJECT(radio_buttons[2]), "mode", GINT_TO_POINTER(2));
	g_object_set_data(G_OBJECT(radio_buttons[2]), "application", app);
	g_object_set_data(G_OBJECT(radio_buttons[2]), "stamp_core", core);
	g_signal_connect(G_OBJECT(radio_buttons[2]), "toggled", G_CALLBACK(StampCoreSetMode), &brush->mode);
	table = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[1], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[2], FALSE, TRUE, 0);

	// ���E���]�A�㉺���]�I��p�`�F�b�N�{�b�N�X
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.reverse);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.reverse_horizontally);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		core->pattern_flags & PATTERN_FLIP_HORIZONTALLY);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetPatternFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.reverse_vertically);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(1));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		core->pattern_flags & PATTERN_FLIP_VERTICALLY);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetPatternFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// ��]�����ύX�p�E�B�W�F�b�g
	radio_buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.clockwise);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.counter_clockwise
	);

	if(core->rotate_direction < 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
	}

	table = gtk_hbox_new(FALSE, 0);
	g_object_set_data(G_OBJECT(radio_buttons[0]), "rotate_direction", GINT_TO_POINTER(1));
	g_object_set_data(G_OBJECT(radio_buttons[1]), "rotate_direction", GINT_TO_POINTER(-1));
	g_signal_connect(G_OBJECT(radio_buttons[0]), "toggled",
		G_CALLBACK(ImageBrushSetRotateDirection), brush);
	g_signal_connect(G_OBJECT(radio_buttons[1]), "toggled",
		G_CALLBACK(ImageBrushSetRotateDirection), brush);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[1], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);
	// �ڍאݒ��UI��������Ƃ��Ƀ{�^���z����J������
	g_signal_connect(G_OBJECT(vbox), "destroy", G_CALLBACK(OnStampDetailUIDestroy), core);

	// �X�^���v�I���e�[�u�����쐬���ăX�N���[���h�E�B���h�E�ɓ����
	table = gtk_hbox_new(FALSE, 0);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(table,
		ICON_SIZE*STAMP_SELECT_TABLE_WIDTH+4, ICON_SIZE*4+4);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
		CreateStampSelectTable(core));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(table), scrolled_window, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �N���b�v�{�[�h�̃X�^���v���A�N�e�B�u�Ȃ�΃Z�b�g
	if(app->stamps.has_clip_board_pattern != FALSE)
	{
		if(core->stamp_id == 0)
		{
			app->stamps.active_pattern =
				&app->stamps.clip_board;
		}
		else
		{
			app->stamps.active_pattern =
				&app->stamps.patterns[core->stamp_id-1];
		}
	}

	// �A�N�e�B�u�ȃp�^�[�����`�����l����2�ȊO�Ȃ�Β��F���[�h�͖���
	gtk_widget_set_sensitive(core->mode_select[0], app->stamps.active_pattern->channel == 2);
	gtk_widget_set_sensitive(core->mode_select[1], app->stamps.active_pattern->channel == 2);
	gtk_widget_set_sensitive(core->mode_select[2], app->stamps.active_pattern->channel == 2);

	InitializeStampCore(core, (int)brush->mode, app);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
#if MAJOR_VERSION == 1
	combo = gtk_combo_box_new_text();
#else
	combo = gtk_combo_box_text_new();
#endif
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->layer_window.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
#if MAJOR_VERSION == 1
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), app->labels->layer_window.blend_modes[i]);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), app->labels->layer_window.blend_modes[i]);
#endif
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), brush->blend_mode);
	(void)g_signal_connect(G_OBJECT(combo), "changed",
		G_CALLBACK(ImageBrushSetBlendMode), brush);
	gtk_box_pack_start(GTK_BOX(table), combo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->enter, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.enter, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ImageBrushSetEnter), core);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->out, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.out, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ImageBrushSetOut), core);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �M���g�p�ݒ�ύX�p�E�B�W�F�b�g
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), core->flags & BRUSH_FLAG_SIZE);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), core->flags & BRUSH_FLAG_FLOW);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.randoam_size);
	g_object_set_data(G_OBJECT(check_button), "flag-value", GUINT_TO_POINTER(STAMP_RANDOM_SIZE));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), core->flags & STAMP_RANDOM_SIZE);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlag), core);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	// �u���V�̈ړ������։�]
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.rotate_to_brush_direction);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		brush->core.flags & BRUSH_FLAG_ROTATE);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(ImageBrushSetRotate2Direction), brush);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

	// �����_���ɉ�]
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.random_rotate);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), (core->flags & STAMP_RANDOM_ROTATE) != 0);
	g_object_set_data(G_OBJECT(check_button), "flag-value", GUINT_TO_POINTER(STAMP_RANDOM_ROTATE));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlag), core);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	// �����_���T�C�Y�̕ω���
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->size_range * 100, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.size_range, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ImageBrushSetSizeRange), brush);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �����_����]�̕ω���
	brush->rotate_range = brush->random_rotate_range * G_PI / 180;
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->random_rotate_range, 0, 360, 1, 1, 0));
	scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.rotate_range, 0);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ImageBrushSetRotateRange), brush);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

#undef UI_FONT_SIZE
#undef ICON_SIZE

	return vbox;
}

static void PickerBrushButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		PICKER_BRUSH *brush = (PICKER_BRUSH*)core->brush_data;
		// �u���V�̔w�i�ƕs�����x
		gdouble r, alpha;
		// �X�V�͈͂̃C���[�W���
		cairo_surface_t *update_surface;
		cairo_t *update;
		// �u���V�̊g��k����
		gdouble zoom;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, max_x, min_y, max_y;
		// �s�N�Z���f�[�^���X�V������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height;
		// �`����W�w��E�g��k���p
		cairo_matrix_t matrix;
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask;
		// �`��F
		uint8 color[4];
		// �u���V�̈ʒu�̃s�N�Z���l���v
		unsigned int sum_color[6] = {0, 0, 0, 1, 0, 1};
		// �F�𒊏o���郌�C���[
		LAYER *target;
		// �`�掞�Ƀ}�X�N����T�[�t�F�[�X
		cairo_surface_t *mask_surface;
		int i, j;	// for���p�̃J�E���^

		// �������[�h��ݒ�
		window->work_layer->layer_mode = brush->blend_mode;
		// �ꕔ�X�V���[�h��
		window->flags |= DRAW_WINDOW_UPDATE_PART;

		// ���W���L��
		brush->before_x = x, brush->before_y = y;

		// ��ʊO�N���b�N�Ȃ�I��
		if(x < 0 || x >= window->width || y < 0 || y >= window->height)
		{
			return;
		}

		// �M���ł̃T�C�Y�ύX�̗L��
		if((brush->flags & PICKER_FLAG_PRESSURE_SIZE) == 0)
		{
			r = brush->r;
			zoom = 1;
		}
		else
		{
			r = brush->r * pressure;
			zoom = 1 / pressure;
		}

		// �M���ł̔Z�x�ύX�̗L��
		if((brush->flags & PICKER_FLAG_PRESSURE_FLOW) == 0)
		{
			alpha = brush->alpha;
		}
		else
		{
			alpha = brush->alpha * pressure;
		}

		// �}�E�X�̍��W�ƃu���V�̔��a����
			// �`�悷����W�̍ő�E�ŏ��l������
		min_x = x - r, min_y = y - r;
		max_x = x + r + 1, max_y = y + r + 1;

		update_surface = cairo_surface_create_for_rectangle(
			window->alpha_surface, min_x, min_y, r*2+1, r*2+1);
		update = cairo_create(update_surface);

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		window->update.x = (int)min_x, window->update.y = (int)min_y;
		window->update.width = (int)max_x - window->update.x;
		window->update.height = (int)max_y - window->update.y;

		window->flags |= DRAW_WINDOW_UPDATE_PART;

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		min_x = x - r, max_x = x + r;
		min_y = y - r, max_y = y + r;

		mask = window->mask->pixels;
		(void)memset(window->mask->pixels, 0, window->width * window->height);
		mask_surface = window->alpha_surface;

		if(window->app->textures.active_texture == 0)
		{
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(update, core->brush_pattern);
					cairo_paint_with_alpha(update, alpha);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);
					cairo_mask_surface(update,
						window->active_layer->surface_p, - x + r, - y + r);
				}
			}
			else
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);
					cairo_mask_surface(update,
						window->selection->surface_p, - x + r, - y + r);
				}
				else
				{
					cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
						window->alpha_temp, x - r, y - r, r*2+1, r*2+1);
					cairo_t *update_temp = cairo_create(temp_surface);

					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);

					(void)memset(window->temp_layer->pixels, 0,
						window->width*window->height);

					cairo_mask_surface(update,
						window->selection->surface_p, 0, 0);
					cairo_set_source_surface(update_temp,
						update_surface, 0, 0);
					cairo_mask_surface(update_temp,
						window->active_layer->surface_p, - x + r, - y + r);

					mask = window->temp_layer->pixels;
					mask_surface = window->temp_layer->surface_p;

					cairo_surface_destroy(temp_surface);
					cairo_destroy(update_temp);
				}
			}
		}
		else
		{
			cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
				window->alpha_temp, x - r, y - r, r*2+1, r*2+1);
			cairo_t *update_temp = cairo_create(temp_surface);

			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(update, core->brush_pattern);
					cairo_paint_with_alpha(update, alpha);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);
					cairo_mask_surface(update,
						window->active_layer->surface_p, - x + r, - y + r);
				}

				cairo_set_source_surface(update_temp, update_surface, 0, 0);
				cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

				mask = window->temp_layer->pixels;
				mask_surface = window->temp_layer->surface_p;
			}
			else
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);
					cairo_mask_surface(update,
						window->selection->surface_p, - x + r, - y + r);

					cairo_set_source_surface(update_temp, update_surface, 0, 0);
					cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

					mask = window->temp_layer->pixels;
					mask_surface = window->temp_layer->surface_p;
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);

					(void)memset(window->temp_layer->pixels, 0,
						window->width*window->height);

					cairo_mask_surface(update,
						window->selection->surface_p, 0, 0);
					cairo_set_source_surface(update_temp,
						update_surface, 0, 0);
					cairo_mask_surface(update_temp,
						window->active_layer->surface_p, - x + r, - y + r);

					cairo_set_operator(update, CAIRO_OPERATOR_SOURCE);
					cairo_set_source_surface(update, temp_surface, 0, 0);
					cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
				}
			}

			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}

		start_x = (int)min_x, start_y = (int)min_y;
		width = (int)(max_x - min_x);
		height = (int)(max_y - min_y);

		if(brush->picker_source == COLOR_PICKER_SOURCE_ACTIVE_LAYER)
		{
			(void)memcpy(window->mask_temp->pixels, window->active_layer->pixels,
				window->pixel_buf_size);
			target = window->mask_temp;
			g_layer_blend_funcs[brush->blend_mode](window->work_layer, target);
		}
		else
		{
			target = window->mixed_layer;
		}

		if(brush->picker_mode == PICKER_MODE_SINGLE_PIXEL)
		{
			int int_x = (int)x, int_y = (int)y;
			color[0] = target->pixels[target->stride*int_y+int_x*4];
			color[1] = target->pixels[target->stride*int_y+int_x*4+1];
			color[2] = target->pixels[target->stride*int_y+int_x*4+2];
			color[3] = target->pixels[target->stride*int_y+int_x*4+3];
		}
		else
		{
			for(i=0; i<height; i++)
			{
				ref_pix = &target->pixels[(i+start_y)*target->stride+start_x*4];
				mask_pix = &mask[(i+start_y)*window->width+start_x];
				for(j=0; j<width; j++, ref_pix+=4, mask_pix++)
				{
					sum_color[0] += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
					sum_color[1] += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
					sum_color[2] += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
					sum_color[3] += ((ref_pix[3]+1) * *mask_pix) >> 8;
					sum_color[4] += ref_pix[3] * *mask_pix;
					sum_color[5] += *mask_pix;
				}
			}

			color[0] = (uint8)((sum_color[0] + sum_color[3] / 2) / sum_color[3]);
			color[1] = (uint8)((sum_color[1] + sum_color[3] / 2) / sum_color[3]);
			color[2] = (uint8)((sum_color[2] + sum_color[3] / 2) / sum_color[3]);
			color[3] = (uint8)(sum_color[4] / sum_color[5]);
		}

		// �F����
		if((brush->flags & PICKER_FLAG_CHANGE_HSV) != 0)
		{
			HSV hsv;
			int s, v;
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			uint8 r;
			r = color[2];
			color[2] = color[0];
			color[0] = r;
#endif
			RGB2HSV_Pixel(color, &hsv);
			hsv.h += brush->add_h;
			s = hsv.s, v = hsv.v;
			if(hsv.h < 0)
			{
				hsv.h += 360;
			}
			else if(hsv.h >= 360)
			{
				hsv.h -= 360;
			}
			s += brush->add_s;
			if(s < 0)
			{
				s = 0;
			}
			else if(s > 255)
			{
				s = 255;
			}
			hsv.s = (uint8)s;

			v += brush->add_v;
			if(v < 0)
			{
				v = 0;
			}
			else if(v > 255)
			{
				v = 255;
			}
			hsv.v = (uint8)v;
			HSV2RGB_Pixel(&hsv, color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			r = color[2];
			color[2] = color[0];
			color[0] = r;
#endif
		}

		cairo_destroy(update);
		cairo_surface_destroy(update_surface);

		update_surface = cairo_surface_create_for_rectangle(
			window->work_layer->surface_p, min_x, min_y, r*2+1, r*2+1);
		update = cairo_create(update_surface);

		// ���o�����F�ŕ`��
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
		cairo_set_source_rgba(update,
			color[2]*DIV_PIXEL, color[1]*DIV_PIXEL, color[1]*DIV_PIXEL, color[3]*DIV_PIXEL);
#else
		cairo_set_source_rgba(update,
			color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, color[3]*DIV_PIXEL);
#endif
		cairo_mask_surface(update, mask_surface, - x + r, - y + r);

		cairo_destroy(update);
		cairo_surface_destroy(update_surface);
	}
}

static void PickerBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		PICKER_BRUSH* brush = (PICKER_BRUSH*)core->brush_data;
		// �u���V�̔w�i�ƕs�����x
		gdouble r, alpha;
		// �X�V�͈͂̃C���[�W���
		cairo_surface_t *update_surface;
		cairo_t *update, *update_temp;
		// �`�悷����W
		FLOAT_T draw_x = brush->before_x, draw_y = brush->before_y;
		// �O��̕`�悩��̈ړ�����
		FLOAT_T d;
		// �`�掞�̈ړ���
		FLOAT_T dx, dy;
		// �O��`�掞����̈ړ���
		FLOAT_T diff_x, diff_y, step;
		// �u���V�̊g��k����
		gdouble zoom;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, max_x, min_y, max_y;
		// �s�N�Z���f�[�^���X�V������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height;
		// �`����W�w��E�g��k���p
		cairo_matrix_t matrix;
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask;
		// �`��F
		uint8 color[4];
		// �u���V�̈ʒu�̃s�N�Z���l���v
		unsigned int sum_color[6];
		// �F�𒊏o���郌�C���[
		LAYER *target;
		// �`�掞�Ƀ}�X�N����T�[�t�F�[�X
		cairo_surface_t *mask_surface, *temp_surface;
		// ���u�����h�p
		uint8 src_value, dst_value;
		FLOAT_T src_alpha, dst_alpha, div_alpha;
		int i, j;	// for���p�̃J�E���^

		if(x < 0 || y < 0 || x >= window->width || y >= window->height)
		{
			return;
		}

		if((brush->flags & PICKER_FLAG_PRESSURE_SIZE) == 0)
		{
			r = brush->r;
			zoom = 1;
		}
		else
		{
			r = brush->r * pressure;
			zoom = 1 / pressure;
		}

		step = r * BRUSH_STEP;
		if(step < MIN_BRUSH_STEP)
		{
			dx = 0;
			goto skip_draw;
		}
		alpha = ((brush->flags & PICKER_FLAG_PRESSURE_FLOW) == 0) ?
			1 : pressure;
		dx = x-brush->before_x, dy = y-brush->before_y;
		d = sqrt(dx*dx+dy*dy);
		diff_x = step * dx / d, diff_y = step * dy / d;

		min_x = x - r*2, min_y = y - r*2;
		max_x = x + r*2, max_y = y + r*2;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		if(window->update.x > min_x)
		{
			window->update.width += window->update.x - (int)min_x;
			window->update.x = (int)min_x;
		}
		if(window->update.width + window->update.x < max_x)
		{
			window->update.width += (int)max_x - window->update.width + window->update.x;
		}
		if(window->update.y > min_y)
		{
			window->update.height += window->update.y - (int)min_y;
			window->update.y = (int)min_y;
		}
		if(window->update.height + window->update.y < max_y)
		{
			window->update.height += (int)max_y - window->update.height + window->update.y;
		}

		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		dx = d;
		while(1)
		{
			if(draw_x < 0 || draw_x >= window->width
				|| draw_y < 0 || draw_y >= window->height)
			{
				dx = 0;
				goto skip_draw;
			}

			update_surface = cairo_surface_create_for_rectangle(
				window->alpha_surface, draw_x - r, draw_y - r, r*2+1, r*2+1);
			update = cairo_create(update_surface);
			start_x = (int32)(draw_x - r - 1);
			width = (int32)(draw_x + r + 1);
			start_y = (int32)(draw_y - r - 1);
			height = (int32)(draw_y + r + 1);

			if(start_x < 0)
			{
				start_x = 0;
			}
			else if(width > window->width)
			{
				width = window->width;
			}
			if(start_y < 0)
			{
				start_y = 0;
			}
			if(height > window->height)
			{
				height = window->height;
			}
			width = width - start_x;
			height = height - start_y;

			if(width <= 0 || height <= 0)
			{
				dx = 0;
				goto skip_draw;
			}

			for(i=0; i<height; i++)
			{
				(void)memset(&window->temp_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*window->work_layer->channel],
					0x0, width*window->work_layer->channel);
			}

			mask = window->mask->pixels;
			(void)memset(window->mask->pixels, 0, window->width * window->height);
			mask_surface = window->alpha_surface;

			if(window->app->textures.active_texture == 0)
			{
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - draw_x + r, - draw_y + r);
					}
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - draw_x + r, - draw_y + r);
					}
					else
					{
						temp_surface = cairo_surface_create_for_rectangle(
							window->alpha_temp, draw_x - r, draw_y - r, r*2+1, r*2+1);
						update_temp = cairo_create(temp_surface);

						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						(void)memset(window->temp_layer->pixels, 0,
							window->width*window->height);

						cairo_mask_surface(update,
							window->selection->surface_p, 0, 0);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - draw_x + r, - draw_y + r);

						mask = window->temp_layer->pixels;
						mask_surface = window->temp_layer->surface_p;

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}
				}
			}
			else
			{
				temp_surface = cairo_surface_create_for_rectangle(
					window->alpha_temp, draw_x - r, draw_y - r, r*2+1, r*2+1);
				update_temp = cairo_create(temp_surface);

				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(update, core->brush_pattern);
						cairo_paint_with_alpha(update, alpha);
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->active_layer->surface_p, - draw_x + r, - draw_y + r);
					}

					cairo_set_source_surface(update_temp, update_surface, 0, 0);
					cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

					mask = window->temp_layer->pixels;
					mask_surface = window->temp_layer->surface_p;
				}
				else
				{
					if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);
						cairo_mask_surface(update,
							window->selection->surface_p, - x + r, - y + r);

						cairo_set_source_surface(update_temp, update_surface, 0, 0);
						cairo_mask_surface(update_temp, window->texture->surface_p, - x + r, - y + r);

						mask = window->temp_layer->pixels;
						mask_surface = window->temp_layer->surface_p;
					}
					else
					{
						cairo_matrix_init_scale(&matrix, zoom, zoom);
						cairo_pattern_set_matrix(core->brush_pattern, &matrix);
						cairo_set_source(core->temp_cairo, core->brush_pattern);
						cairo_paint_with_alpha(core->temp_cairo, alpha);
						cairo_matrix_init_translate(&matrix, 0, 0);
						cairo_pattern_set_matrix(core->temp_pattern, &matrix);
						cairo_set_source(update, core->temp_pattern);

						(void)memset(window->temp_layer->pixels, 0,
							window->width*window->height);

						cairo_mask_surface(update,
							window->selection->surface_p, 0, 0);
						cairo_set_source_surface(update_temp,
							update_surface, 0, 0);
						cairo_mask_surface(update_temp,
							window->active_layer->surface_p, - x + r, - y + r);

						cairo_set_operator(update, CAIRO_OPERATOR_SOURCE);
						cairo_set_source_surface(update, temp_surface, 0, 0);
						cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
					}
				}

				cairo_surface_destroy(temp_surface);
				cairo_destroy(update_temp);
			}

			start_x = (int)min_x, start_y = (int)min_y;
			width = (int)(max_x - min_x);
			height = (int)(max_y - min_y);

			if(brush->picker_source == COLOR_PICKER_SOURCE_ACTIVE_LAYER)
			{
				(void)memcpy(window->mask_temp->pixels, window->active_layer->pixels,
					window->pixel_buf_size);
				target = window->mask_temp;
				g_layer_blend_funcs[brush->blend_mode](window->work_layer, target);
			}
			else
			{
				target = window->mixed_layer;
			}

			if(brush->picker_mode == PICKER_MODE_SINGLE_PIXEL)
			{
				int int_x = (int)x, int_y = (int)y;
				color[0] = target->pixels[target->stride*int_y+int_x*4];
				color[1] = target->pixels[target->stride*int_y+int_x*4+1];
				color[2] = target->pixels[target->stride*int_y+int_x*4+2];
				color[3] = target->pixels[target->stride*int_y+int_x*4+3];
			}
			else
			{
				sum_color[0] = sum_color[1] = sum_color[2] = sum_color[4] = 0;
				sum_color[3] = sum_color[5] = 1;
				for(i=0; i<height; i++)
				{
					ref_pix = &target->pixels[(i+start_y)*target->stride+start_x*4];
					mask_pix = &mask[(i+start_y)*window->width+start_x];
					for(j=0; j<width; j++, ref_pix+=4, mask_pix++)
					{
						sum_color[0] += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
						sum_color[1] += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
						sum_color[2] += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
						sum_color[3] += ((ref_pix[3]+1) * *mask_pix) >> 8;
						sum_color[4] += ref_pix[3] * *mask_pix;
						sum_color[5] += *mask_pix;
					}
				}

				color[0] = (uint8)((sum_color[0] + sum_color[3] / 2) / sum_color[3]);
				color[1] = (uint8)((sum_color[1] + sum_color[3] / 2) / sum_color[3]);
				color[2] = (uint8)((sum_color[2] + sum_color[3] / 2) / sum_color[3]);
				color[3] = (uint8)(sum_color[4] / sum_color[5]);
			}

			// �F����
			if((brush->flags & PICKER_FLAG_CHANGE_HSV) != 0)
			{
				HSV hsv;
				int s, v;
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				uint8 r;
				r = color[2];
				color[2] = color[0];
				color[0] = r;
#endif
				RGB2HSV_Pixel(color, &hsv);
				hsv.h += brush->add_h;
				s = hsv.s, v = hsv.v;
				if(hsv.h < 0)
				{
					hsv.h += 360;
				}
				else if(hsv.h >= 360)
				{
					hsv.h -= 360;
				}
				s += brush->add_s;
				if(s < 0)
				{
					s = 0;
				}
				else if(s > 255)
				{
					s = 255;
				}
				hsv.s = (uint8)s;

				v += brush->add_v;
				if(v < 0)
				{
					v = 0;
				}
				else if(v > 255)
				{
					v = 255;
				}
				hsv.v = (uint8)v;
				HSV2RGB_Pixel(&hsv, color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				r = color[2];
				color[2] = color[0];
				color[0] = r;
#endif
			}

			mask = window->brush_buffer;
			temp_surface = cairo_image_surface_create_for_data(
				mask, CAIRO_FORMAT_ARGB32, width, height, width*4);
			(void)memset(mask, 0, width*4*height);
			update_temp = cairo_create(temp_surface);

			// ���o�����F�ŕ`��
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			cairo_set_source_rgba(update_temp,
				color[2]*DIV_PIXEL, color[1]*DIV_PIXEL, color[0]*DIV_PIXEL, color[3]*DIV_PIXEL);
#else
			cairo_set_source_rgba(update_temp,
				color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, color[3]*DIV_PIXEL);
#endif
			cairo_mask_surface(update_temp, update_surface, r, r);

			cairo_destroy(update);
			cairo_surface_destroy(update_surface);
			cairo_destroy(update_temp);
			cairo_surface_destroy(temp_surface);

			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[
					(start_y+i)*window->work_layer->stride+start_x*4];
				mask_pix = &mask[i*width*4];

				for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
				{
					if(ref_pix[3] < mask_pix[3])
					{
						ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
							* mask_pix[3] >> 8) + ref_pix[0]);
						ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
							* mask_pix[3] >> 8) + ref_pix[1]);
						ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
							* mask_pix[3] >> 8) + ref_pix[2]);
						ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
							* mask_pix[3] >> 8) + ref_pix[3]);
					}
					else if(mask_pix[3] > 0)
					{
						src_value = mask_pix[3];
						dst_value = ref_pix[3];
						src_alpha = src_value * DIV_PIXEL;
						dst_alpha = dst_value * DIV_PIXEL;
						div_alpha = src_alpha + dst_alpha*(1-src_alpha);
						if(div_alpha > 0)
						{
							div_alpha = 1 / div_alpha;
							ref_pix[0] = (uint8)(color[0]*src_alpha+(ref_pix[0]*dst_alpha*(1-src_alpha))*div_alpha);
							ref_pix[1] = (uint8)(color[1]*src_alpha+(ref_pix[1]*dst_alpha*(1-src_alpha))*div_alpha);
							ref_pix[2] = (uint8)(color[2]*src_alpha+(ref_pix[2]*dst_alpha*(1-src_alpha))*div_alpha);
						}
					}
				}
			}

			if((brush->flags & PICKER_FLAG_ANTI_ALIAS) != 0)
			{
				ANTI_ALIAS_RECTANGLE range = {start_x-1, start_y-1, width+1, height+1};
				AntiAliasLayer(window->work_layer, window->temp_layer, &range);
			}


skip_draw:
			dx -= step;
			if(dx < 1)
			{
				break;
			}
			else if(dx >= step)
			{
				draw_x += diff_x, draw_y += diff_y;
			}
			else
			{
				draw_x = x, draw_y = y;
			}
		}

		brush->before_x = x, brush->before_y = y;
		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}
}

#define PickerBrushButtonReleaseCallBack DefaultReleaseCallBack

static void PickerBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	PICKER_BRUSH *brush = (PICKER_BRUSH*)data;
	gdouble r = brush->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void PickerBrushButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, PICKER_BRUSH* brush)
{
	gdouble r = brush->r * window->zoom_rate + 1;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2) + 2, (gint)(r * 2) + 2);
}

static void PickerBrushMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, PICKER_BRUSH* brush)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = brush->r * window->zoom_rate * 1.275 + BRUSH_UPDATE_MARGIN;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width + 2, (gint)height + 2);
}

static void PickerBrushSetScale(GtkAdjustment* slider, PICKER_BRUSH* brush)
{
	brush->r = gtk_adjustment_get_value(slider) * 0.5;
	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);
}

static void PickerBrushSetFlow(GtkAdjustment* slider, PICKER_BRUSH* brush)
{
	brush->alpha = gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);
}

static void PickerBrushSetBlur(GtkAdjustment* slider, PICKER_BRUSH* brush)
{
	brush->blur = gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);
}

static void PickerBrushSetOutlineHardness(GtkAdjustment* slider, PICKER_BRUSH* brush)
{
	brush->outline_hardness = gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);
}

static void PickerBrushSetAddHue(GtkAdjustment* slider, PICKER_BRUSH* brush)
{
	brush->add_h = (int)gtk_adjustment_get_value(slider);

	if(brush->add_h == 0 && brush->add_s == 0 && brush->add_v == 0)
	{
		brush->flags &= ~(PICKER_FLAG_CHANGE_HSV);
	}
	else
	{
		brush->flags |= PICKER_FLAG_CHANGE_HSV;
	}
}

static void PickerBrushSetAddSaturation(GtkAdjustment* slider, PICKER_BRUSH* brush)
{
	brush->add_s = (int)(gtk_adjustment_get_value(slider) * 2.552);

	if(brush->add_h == 0 && brush->add_s == 0 && brush->add_v == 0)
	{
		brush->flags &= ~(PICKER_FLAG_CHANGE_HSV);
	}
	else
	{
		brush->flags |= PICKER_FLAG_CHANGE_HSV;
	}
}

static void PickerBrushSetAddVivid(GtkAdjustment* slider, PICKER_BRUSH* brush)
{
	brush->add_v = (int)(gtk_adjustment_get_value(slider) * 2.552);

	if(brush->add_h == 0 && brush->add_s == 0 && brush->add_v == 0)
	{
		brush->flags &= ~(PICKER_FLAG_CHANGE_HSV);
	}
	else
	{
		brush->flags |= PICKER_FLAG_CHANGE_HSV;
	}
}

static void PickerBrushSetPickTarget(GtkWidget* button, PICKER_BRUSH* brush)
{
	int mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "pick_target"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		brush->picker_source = (uint8)mode;
	}
}

static void PickerBrushSetPickMode(GtkWidget* button, PICKER_BRUSH* brush)
{
	int mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "pick_mode"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		brush->picker_mode = (uint8)mode;
	}
}

static void PickerBrushSetPressureSize(GtkWidget* button, PICKER_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(PICKER_FLAG_PRESSURE_SIZE);
	}
	else
	{
		brush->flags |= PICKER_FLAG_PRESSURE_SIZE;
	}
}

static void PickerBrushSetPressureFlow(GtkWidget* button, PICKER_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(PICKER_FLAG_PRESSURE_FLOW);
	}
	else
	{
		brush->flags |= PICKER_FLAG_PRESSURE_FLOW;
	}
}

static void PickerBrushSetBlendMode(GtkComboBox* combo, PICKER_BRUSH* brush)
{
	brush->blend_mode = (uint16)gtk_combo_box_get_active(combo);
}

static void PickerBrushSetAntiAlias(GtkWidget* button, PICKER_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->flags &= ~(PICKER_FLAG_ANTI_ALIAS);
	}
	else
	{
		brush->flags |= PICKER_FLAG_ANTI_ALIAS;
	}
}

static GtkWidget* CreatePickerBrushDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	PICKER_BRUSH *brush = (PICKER_BRUSH*)core->brush_data;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox;
	GtkWidget *brush_scale;
	GtkWidget *check_button;
	GtkWidget *buttons[2];
	GtkWidget *combo;
	GtkWidget *label;
	GtkAdjustment *scale_adjustment;
	char mark_up_buff[256];
	int i;

	brush->core = core;

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		combo = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[2]);
#else
		combo = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), brush->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(brush->base_scale)
	{
	case 0:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			brush->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerBrushSetScale), brush);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.brush_scale, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(combo), "scale", brush_scale);
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(SetBrushBaseScale), &brush->base_scale);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->alpha, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerBrushSetFlow), brush);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.flow, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->outline_hardness, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerBrushSetOutlineHardness), brush);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.outline_hardness, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->blur, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerBrushSetBlur), brush);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.blur, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
#if MAJOR_VERSION == 1
	combo = gtk_combo_box_new_text();
#else
	combo = gtk_combo_box_text_new();
#endif
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->layer_window.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
#if MAJOR_VERSION == 1
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), app->labels->layer_window.blend_modes[i]);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), app->labels->layer_window.blend_modes[i]);
#endif
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), brush->blend_mode);
	(void)g_signal_connect(G_OBJECT(combo), "changed",
		G_CALLBACK(PickerBrushSetBlendMode), brush);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(PickerBrushSetPressureSize), brush);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(PickerBrushSetPressureFlow), brush);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(PickerBrushSetAntiAlias), brush);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), brush->flags & BRUSH_FLAG_ANTI_ALIAS);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->add_h, -180.0, 180.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerBrushSetAddHue), brush);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.hue, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		(brush->add_s/255.0)*100, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerBrushSetAddSaturation), brush);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.saturation, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		(brush->add_h/255.0)*100, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerBrushSetAddVivid), brush);
	brush_scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.brightness, 1);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, FALSE, 0);

	if(brush->add_h == 0 && brush->add_s == 0 && brush->add_v == 0)
	{
		brush->flags &= ~(PICKER_FLAG_CHANGE_HSV);
	}
	else
	{
		brush->flags |= PICKER_FLAG_CHANGE_HSV;
	}

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->unit.target);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.active_layer);
	g_object_set_data(G_OBJECT(buttons[0]), "pick_target", GINT_TO_POINTER(COLOR_PICKER_SOURCE_ACTIVE_LAYER));
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(PickerBrushSetPickTarget), brush);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
		GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.select.canvas);
	g_object_set_data(G_OBJECT(buttons[1]), "pick_target", GINT_TO_POINTER(COLOR_PICKER_SOURCE_CANVAS));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(PickerBrushSetPickTarget), brush);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[brush->picker_source]), TRUE);

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pick_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.single_pixels);
	g_object_set_data(G_OBJECT(buttons[0]), "pick_mode", GINT_TO_POINTER(PICKER_MODE_SINGLE_PIXEL));
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(PickerBrushSetPickMode), brush);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
		GTK_RADIO_BUTTON(buttons[0])), app->labels->tool_box.average_color);
	g_object_set_data(G_OBJECT(buttons[1]), "pick_mode", GINT_TO_POINTER(PICKER_MODE_AVERAGE));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(PickerBrushSetPickMode), brush);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[brush->picker_mode]), TRUE);

	BrushCoreSetGrayCirclePattern(brush->core, brush->r, brush->outline_hardness * 0.01,
		brush->blur * 0.01, brush->alpha * 0.01);

	return vbox;
}

static void PickerImageBrushPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		PICKER_IMAGE_BRUSH *brush = (PICKER_IMAGE_BRUSH*)core->brush_data;
		// �u���V�̔��a
		FLOAT_T r;
		// ���W�̍ő�l�E�ŏ��l
		FLOAT_T min_x, min_y, max_x, max_y;
		// �`��F
		FLOAT_T color[3] = {(*core->color)[0]*DIV_PIXEL,
			(*core->color)[1]*DIV_PIXEL, (*core->color)[2]*DIV_PIXEL};

		window->work_layer->layer_mode = brush->blend_mode;

		r = ((brush->core.flags & STAMP_PRESSURE_SIZE) == 0) ?
			brush->core.r : brush->core.r * pressure;

		if((brush->core.flags & STAMP_RANDOM_SIZE) != 0)
		{
			r *= rand() / (FLOAT_T)RAND_MAX;
		}

		brush->stamp_distance = 0;
		brush->core.before_x = x, brush->core.before_y = y;
		brush->points[0][1] = x, brush->points[0][2] = y;
		brush->points[0][3] = pressure;
		brush->sum_distance = brush->travel = brush->finish_length = 0;
		brush->remain_distance = brush->core.d * brush->core.d;
		brush->draw_start = r * brush->out * 0.01 * 4;
		brush->enter_length = r * brush->enter * 0.01 * 4;
		brush->enter_size = (1 - brush->enter * 0.01);
		brush->num_point = 0;
		brush->draw_finished = 0;
		brush->ref_point = 1;

		min_x = x - r, max_x = x + r;
		min_y = y - r, max_y = y + r;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		window->update.x = (int)min_x, window->update.y = (int)min_y;
		window->update.width = (int)max_x - window->update.x;
		window->update.height = (int)max_y - window->update.y;

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}	// ���N���b�N�Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

#define PickerImageBrushEditSelectionPress PickerImageBrushPressCallBack

static void PickerImageBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^����������Ă�����
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		PICKER_IMAGE_BRUSH *brush = (PICKER_IMAGE_BRUSH*)core->brush_data;
		FLOAT_T distance;
		int index = brush->ref_point % BRUSH_POINT_BUFFER_SIZE;

		distance = sqrt((x-brush->core.before_x)*(x-brush->core.before_x)
			+(y-brush->core.before_y)*(y-brush->core.before_y));
		brush->stamp_distance += distance;

		if(distance >= 1.0)
		{
			brush->core.before_x = x, brush->core.before_y = y;
			brush->sum_distance += distance;
			brush->remain_distance -= distance;
			brush->travel += distance;
			brush->points[index][0] = distance;
			brush->points[index][1] = x, brush->points[index][2] = y;
			brush->points[index][3] = pressure;
			brush->ref_point++;

			if(brush->sum_distance >= brush->draw_start
				&& brush->sum_distance >= brush->enter_length
				&& brush->remain_distance <= 0)
			{
				// �X�V�͈͂̃C���[�W���
				cairo_t *update, *update_temp;
				cairo_surface_t *update_surface, *temp_surface;
				// ����̐F�␳�p
				gdouble enter_alpha;
				// �u���V�̔��a�ƕs�����x
				gdouble r, alpha;
				// ���W�̍ő�E�ŏ��l
				gdouble min_x, min_y, max_x, max_y;
				// X�AY�����̈ړ���
				gdouble dx, dy;
				// �u���V�̌X���p
				gdouble diff_x, diff_y;
				// �u���V�̈ړ���
				gdouble d;
				// �`����s�����W
				gdouble draw_x = brush->core.before_x, draw_y = brush->core.before_y;
				// ��ʍX�V���s�����W�A�͈�
				gdouble update_x, update_y, update_r;
				// �摜�ɑ΂��Ă̊g��E��]�����p�̍s��f�[�^
				cairo_matrix_t matrix, reset_matrix;
				// �摜�̊g�嗦
				gdouble zoom;
				// �ړ��ʌv�Z�p
				gdouble cos_x, sin_y, trans_x, trans_y;
				// �摜�̕��E�����̔���
				gdouble half_width, half_height;
				// �s�N�Z���f�[�^�����Z�b�g������W
				int start_x, start_y;
				// �`�悷�镝�A�����A��s���̃o�C�g��
				int width, height, stride;
				// �Q�ƃs�N�Z��
				uint8 *ref_pix, *mask_pix, *mask;
				// �z��̃C���f�b�N�X�p
				int ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				int before_point;
				// �`��F
				uint8 color[4];
				// ���ϐF�v�Z�p
				uint64 sum_color[6];
				// �F�𒊏o���郌�C���[
				LAYER *target;
				// ���u�����h�p
				uint8 src_value, dst_value;
				FLOAT_T src_alpha, dst_alpha, div_alpha;
				int i, j;	// for���p�̃J�E���^

				cairo_matrix_init_scale(&reset_matrix, 1, 1);

				while(brush->sum_distance > brush->draw_start
					&& brush->draw_finished < brush->ref_point-1)
				{
					if(brush->finish_length < brush->enter_length)
					{
						enter_alpha = brush->finish_length / brush->enter_length;
						r = brush->enter_size + brush->finish_length * 0.25;
					}
					else
					{
						enter_alpha = 1, r = 1;
					}
					if((brush->core.flags & STAMP_PRESSURE_SIZE) != 0)
					{
						r *= brush->points[ref_point][3];
					}
					if((brush->core.flags & STAMP_RANDOM_SIZE) != 0)
					{
						r *= 1 - (brush->size_range * (rand() / (FLOAT_T)RAND_MAX));
					}

					if(r < MIN_BRUSH_STEP)
					{
						r = MIN_BRUSH_STEP;
					}

					half_width = brush->core.half_width * r;
					half_height = brush->core.half_height * r;
					zoom = (1/r) * brush->core.scale;
					r *= brush->core.r;

					alpha = ((brush->core.flags & BRUSH_FLAG_FLOW) == 0)
						? 1 : brush->points[ref_point][3];
					alpha *= enter_alpha;

					before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
					d = brush->points[ref_point][0];
					brush->sum_distance -= d;
					if(brush->draw_finished == 0)
					{
						draw_x = brush->points[0][1], draw_y = brush->points[0][2];
					}
					else
					{
						draw_x = brush->points[before_point][1];
						draw_y = brush->points[before_point][2];
					}
					dx = brush->points[ref_point][1] - draw_x;
					dy = brush->points[ref_point][2] - draw_y;

					if((brush->core.flags & STAMP_RANDOM_ROTATE) != 0)
					{
						brush->core.rotate = brush->core.rotate_start +
							(rand() / (FLOAT_T)RAND_MAX) * brush->core.rotate_direction * brush->rotate_range;
						if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
						{
							brush->core.rotate += brush->core.rotate_direction * atan2(dy, dx);
						}
					}
					else if((brush->core.flags & STAMP_RANDOM_ROTATE) != 0)
					{
						brush->core.rotate = (rand() / (FLOAT_T)RAND_MAX) * (2*G_PI);
					}
					else if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
					{
						brush->core.rotate = brush->core.rotate_start + brush->core.rotate_direction * atan2(dy, dx);
					}
					else
					{
						brush->core.rotate = brush->core.rotate_start;
					}
					cos_x = cos(brush->core.rotate), sin_y = sin(brush->core.rotate);
					diff_x = brush->core.d * dx/d, diff_y = brush->core.d * dy/d;

					min_x = brush->points[ref_point][1] - r - 1;
					max_x = brush->points[ref_point][1] + r + 1;
					min_y = brush->points[ref_point][2] - r - 1;
					max_y = brush->points[ref_point][2] + r + 1;

					if(min_x < 0.0)
					{
						min_x = 0.0;
					}
					if(core->min_x > min_x)
					{
						core->min_x = min_x;
					}
					if(min_y < 0.0)
					{
						min_y = 0.0;
					}
					if(core->min_y > min_y)
					{
						core->min_y = min_y;
					}
					if(max_x > window->work_layer->width)
					{
						max_x = window->work_layer->width;
					}
					if(core->max_x < max_x)
					{
						core->max_x = max_x;
					}
					if(max_y > window->work_layer->height)
					{
						min_y = window->work_layer->height;
					}
					if(core->max_y < max_y)
					{
						core->max_y = max_y;
					}

					if(window->update.x > min_x)
					{
						window->update.width += window->update.x - (int)min_x;
						window->update.x = (int)min_x;
					}
					if(window->update.width + window->update.x < max_x)
					{
						window->update.width += (int)max_x - window->update.width + window->update.x;
					}
					if(window->update.y > min_y)
					{
						window->update.height += window->update.y - (int)min_y;
						window->update.y = (int)min_y;
					}
					if(window->update.height + window->update.y < max_y)
					{
						window->update.height += (int)max_y - window->update.height + window->update.y;
					}

					if(brush->stamp_distance >= brush->core.d)
					{
						dx = d;
						do
						{
							if(draw_x < 0 || draw_x >= window->width
								|| draw_y < 0 || draw_y >= window->height)
							{
								dx = 0;
								goto skip_draw;
							}

							start_x = (int)(draw_x - r);
							start_y = (int)(draw_y - r);
							width = (int)(draw_x + r);
							height = (int)(draw_y + r);

							if(start_x < 0)
							{
								start_x = 0;
							}
							else if(start_x > window->work_layer->width)
							{
								goto skip_draw;
							}
							if(start_y < 0)
							{
								start_y = 0;
							}
							else if(start_y > window->work_layer->height)
							{
								goto skip_draw;
							}
							if(width > window->work_layer->width)
							{
								width = window->work_layer->width - start_x;
							}
							else
							{
								width = width - start_x;
							}
							if(height > window->work_layer->height)
							{
								height = window->work_layer->height - start_y;
							}
							else
							{
								height = height - start_y;
							}

							if(width < 0 || height < 0)
							{
								goto skip_draw;
							}

							stride = width*4;

							update_surface = cairo_surface_create_for_rectangle(
								window->mask_temp->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
							update = cairo_create(update_surface);

							for(i=0; i<height; i++)
							{
								(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
									0, stride);
							}

							trans_x = r - (half_width * cos_x + half_height * sin_y);
							trans_y = r + (half_width * sin_y - half_height * cos_x);

							// �s��f�[�^�Ɋg��E�k���A��]�p���Z�b�g
							cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
							cairo_matrix_init_scale(&matrix, zoom, zoom);
							cairo_matrix_rotate(&matrix, brush->core.rotate);
							cairo_matrix_translate(&matrix, - trans_x, - trans_y);

							mask = window->mask_temp->pixels;
							if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
							{
								if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
								{
									cairo_set_source(update, core->brush_pattern);
									cairo_pattern_set_matrix(core->brush_pattern, &matrix);
									cairo_paint_with_alpha(update, alpha);
								}
								else
								{
									cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
									cairo_set_source(core->temp_cairo, core->brush_pattern);
									cairo_paint_with_alpha(core->temp_cairo, alpha);
									cairo_set_source(update, core->temp_pattern);
									cairo_pattern_set_matrix(core->temp_pattern, &matrix);
									cairo_mask_surface(update,
										window->active_layer->surface_p, - draw_x + r, - draw_y + r);
								}
							}
							else
							{
								if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
								{
									cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
									cairo_set_source(core->temp_cairo, core->brush_pattern);
									cairo_paint_with_alpha(core->temp_cairo, alpha);
									cairo_set_source(update, core->temp_pattern);
									cairo_pattern_set_matrix(core->temp_pattern, &matrix);
									cairo_mask_surface(update,
										window->selection->surface_p, - draw_x + r, - draw_y + r);
								}
								else
								{
									temp_surface = cairo_surface_create_for_rectangle(
										window->temp_layer->surface_p, draw_x-r, draw_y-r, r*2+1, r*2+1
									);
									update_temp = cairo_create(temp_surface);

									for(i=0; i<height; i++)
									{
										(void)memset(&window->temp_layer->pixels[
											(i+start_y)*window->temp_layer->stride+start_x*4],
											0, stride
										);
									}

									cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
									cairo_set_source(core->temp_cairo, core->brush_pattern);
									cairo_paint_with_alpha(core->temp_cairo, alpha);
									cairo_set_source(update, core->temp_pattern);
									cairo_mask_surface(update,
										window->selection->surface_p, - draw_x + r, - draw_y + r);
									cairo_set_source_surface(update_temp,
										update_surface, 0, 0);
									cairo_mask_surface(update_temp,
										window->active_layer->surface_p, - draw_x + r, - draw_y + r);

									mask = window->temp_layer->pixels;

									cairo_surface_destroy(temp_surface);
									cairo_destroy(update_temp);
								}
							}

							// �e�N�X�`����K�p
							if(window->app->textures.active_texture != 0)
							{
								temp_surface = cairo_surface_create_for_rectangle(
									window->temp_layer->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
								update_temp = cairo_create(temp_surface);

								if(mask == window->mask_temp->pixels)
								{
									for(i=0; i<height; i++)
									{
										(void)memset(&window->temp_layer->pixels[
											(i+start_y)*window->temp_layer->stride+start_x*4],
											0, stride
										);
									}
									cairo_set_source_surface(update_temp,
										update_surface, 0, 0);
									cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);
									mask = window->temp_layer->pixels;
								}
								else
								{
									for(i=0; i<height; i++)
									{
										(void)memset(&window->mask_temp->pixels[
											(i+start_y)*window->mask_temp->stride+start_x*4],
											0, stride
										);
									}
									cairo_set_source_surface(update,
										temp_surface, 0, 0);
									cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
									mask = window->mask_temp->pixels;
								}

								cairo_surface_destroy(temp_surface);
								cairo_destroy(update_temp);
							}

							if(brush->picker_source == COLOR_PICKER_SOURCE_ACTIVE_LAYER)
							{
								(void)memcpy(window->mask->pixels, window->active_layer->pixels,
									window->pixel_buf_size);
								target = window->mask;
								g_layer_blend_funcs[brush->blend_mode](window->work_layer, target);
							}
							else
							{
								target = window->mixed_layer;
							}

							if(brush->picker_mode == PICKER_MODE_SINGLE_PIXEL)
							{
								int int_x = (int)x, int_y = (int)y;
								color[0] = target->pixels[target->stride*int_y+int_x*4];
								color[1] = target->pixels[target->stride*int_y+int_x*4+1];
								color[2] = target->pixels[target->stride*int_y+int_x*4+2];
								color[3] = target->pixels[target->stride*int_y+int_x*4+3];
							}
							else
							{
								sum_color[0] = sum_color[1] = sum_color[2] = sum_color[4] = 0;
								sum_color[3] = sum_color[5] = 1;
								for(i=0; i<height; i++)
								{
									ref_pix = &target->pixels[(i+start_y)*target->stride+start_x*4];
									mask_pix = &mask[(i+start_y)*window->stride+start_x*4+3];
									for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
									{
										sum_color[0] += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
										sum_color[1] += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
										sum_color[2] += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
										sum_color[3] += ((ref_pix[3]+1) * *mask_pix) >> 8;
										sum_color[4] += ref_pix[3] * *mask_pix;
										sum_color[5] += *mask_pix;
									}
								}

								color[0] = (uint8)((sum_color[0] + sum_color[3] / 2) / sum_color[3]);
								color[1] = (uint8)((sum_color[1] + sum_color[3] / 2) / sum_color[3]);
								color[2] = (uint8)((sum_color[2] + sum_color[3] / 2) / sum_color[3]);
								color[3] = (uint8)(sum_color[4] / sum_color[5]);
							}

							// �F����
							if((brush->flags & PICKER_FLAG_CHANGE_HSV) != 0)
							{
								HSV hsv;
								int s, v;
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
								uint8 r;
								r = color[2];
								color[2] = color[0];
								color[0] = r;
#endif
								RGB2HSV_Pixel(color, &hsv);
								hsv.h += brush->add_h;
								s = hsv.s, v = hsv.v;
								if(hsv.h < 0)
								{
									hsv.h += 360;
								}
								else if(hsv.h >= 360)
								{
									hsv.h -= 360;
								}
								s += brush->add_s;
								if(s < 0)
								{
									s = 0;
								}
								else if(s > 255)
								{
									s = 255;
								}
								hsv.s = (uint8)s;

								v += brush->add_v;
								if(v < 0)
								{
									v = 0;
								}
								else if(v > 255)
								{
									v = 255;
								}
								hsv.v = (uint8)v;
								HSV2RGB_Pixel(&hsv, color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
								r = color[2];
								color[2] = color[0];
								color[0] = r;
#endif
							}

							cairo_surface_destroy(update_surface);
							cairo_destroy(update);

							if(mask == window->mask_temp->pixels)
							{
								temp_surface = cairo_surface_create_for_rectangle(
									window->mask_temp->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
								update_surface = cairo_surface_create_for_rectangle(
									window->temp_layer->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
								mask = window->temp_layer->pixels;
							}
							else
							{
								temp_surface = cairo_surface_create_for_rectangle(
									window->temp_layer->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
								update_surface = cairo_surface_create_for_rectangle(
									window->mask_temp->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
								mask = window->mask_temp->pixels;
							}

							for(i=0; i<height; i++)
							{
								(void)memset(&mask[(start_y+i)*window->stride+start_x*4], 0, stride);
							}
							update = cairo_create(update_surface);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
							cairo_set_source_rgba(update, color[2]*DIV_PIXEL,
								color[1]*DIV_PIXEL, color[0]*DIV_PIXEL, color[3]*DIV_PIXEL);
#else
							cairo_set_source_rgba(update, color[0]*DIV_PIXEL,
								color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, color[3]*DIV_PIXEL);
#endif
							cairo_mask_surface(update, temp_surface, 0, 0);

							cairo_destroy(update);
							cairo_surface_destroy(temp_surface);
							cairo_surface_destroy(update_surface);

							for(i=0; i<height; i++)
							{
								ref_pix = &window->work_layer->pixels[
									(start_y+i)*window->work_layer->stride+start_x*4];
								mask_pix = &mask[(start_y+i)*window->work_layer->stride
									+start_x*4];

								for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
								{
									if(ref_pix[3] < mask_pix[3])
									{
										ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
											* mask_pix[3] >> 8) + ref_pix[0]);
										ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
											* mask_pix[3] >> 8) + ref_pix[1]);
										ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
											* mask_pix[3] >> 8) + ref_pix[2]);
										ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
											* mask_pix[3] >> 8) + ref_pix[3]);
									}
									else if(mask_pix[3] > 0)
									{
										src_value = mask_pix[3];
										dst_value = ref_pix[3];
										src_alpha = src_value * DIV_PIXEL;
										dst_alpha = dst_value * DIV_PIXEL;
										div_alpha = src_alpha + dst_alpha*(1-src_alpha);
										if(div_alpha > 0)
										{
											div_alpha = 1 / div_alpha;
											ref_pix[0] = (uint8)(color[0]*src_alpha+(ref_pix[0]*dst_alpha*(1-src_alpha))*div_alpha);
											ref_pix[1] = (uint8)(color[1]*src_alpha+(ref_pix[1]*dst_alpha*(1-src_alpha))*div_alpha);
											ref_pix[2] = (uint8)(color[2]*src_alpha+(ref_pix[2]*dst_alpha*(1-src_alpha))*div_alpha);
										}
									}
								}
							}

							update_r = r * window->zoom_rate;
							update_x = ((draw_x-window->width/2)*window->cos_value + (draw_y-window->height/2)*window->sin_value) * window->zoom_rate
								+ window->rev_add_cursor_x;
							update_y = (- (draw_x-window->width/2)*window->sin_value + (draw_y-window->height/2)*window->cos_value) * window->zoom_rate
								+ window->rev_add_cursor_y;
							gtk_widget_queue_draw_area(window->window, (gint)(update_x-update_r), (gint)(update_y-update_r),
								(gint)(update_r * 2 + BRUSH_UPDATE_MARGIN), (gint)(update_r * 2 + BRUSH_UPDATE_MARGIN));
skip_draw:
							dx -= brush->core.d;
							brush->stamp_distance -= brush->core.d;
							brush->remain_distance += brush->core.d * brush->core.d;
							if(dx < brush->core.d)
							{
								break;
							}
							else
							{
								draw_x += diff_x, draw_y += diff_y;
							}
						} while(1);
					}

					brush->finish_length += d;
					brush->travel += d;
					brush->draw_finished++;
					ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
				}	// while(brush->sum_distance > brush->draw_start
						// && brush->draw_finished < brush->ref_point)
			}	// if(brush->sum_distance >= brush->draw_start
					// && brush->sum_distance >= brush->enter_length)
		}	// if(distance >= 1.0)

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}	// �}�E�X�̍��{�^����������Ă�����
		// if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
}

#define PickerImageBrushEditSelectionMotion PickerImageBrushMotionCallBack

static void PickerImageBrushReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �}�E�X�̍��{�^���Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		PICKER_IMAGE_BRUSH *brush = (PICKER_IMAGE_BRUSH*)core->brush_data;
		// �X�V�͈͂̃C���[�W���
		cairo_t *update, *update_temp;
		cairo_surface_t *update_surface, *temp_surface;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̈ړ���
		gdouble d;
		// �`����s�����W
		gdouble draw_x, draw_y;
		// ��ʍX�V���s�����W�A�͈�
		gdouble update_x, update_y, update_r;
		// ����A�������̐F�␳
		gdouble enter_alpha, out_alpha;
		// �`��F
		uint8 color[4];
		// ���ϐF�v�Z�p
		uint64 sum_color[6];
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �摜�ɑ΂��Ă̊g��E��]�����p�̍s��f�[�^
		cairo_matrix_t matrix, reset_matrix;
		// �摜�̊g�嗦
		gdouble zoom;
		// �ړ��ʌv�Z�p
		gdouble cos_x, sin_y, trans_x, trans_y;
		// �摜�̕��E�����̔���
		gdouble half_width, half_height;
		// �F�𒊏o���郌�C���[
		LAYER *target;
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *mask;
		// ���u�����h�p
		uint8 src_value, dst_value;
		FLOAT_T src_alpha, dst_alpha, div_alpha;
		// �z��̃C���f�b�N�X�p
		int ref_point;
		int before_point;
		int i, j;	// for���p�̃J�E���^

		ref_point = brush->ref_point % BRUSH_POINT_BUFFER_SIZE;
		brush->points[ref_point][1] = x, brush->points[ref_point][2] = y;
		brush->ref_point++;

		while(brush->ref_point > brush->draw_finished)
		{
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;

			if(brush->finish_length < brush->enter_length)
			{
				enter_alpha = brush->finish_length / brush->enter_length;
				r = brush->enter_size + brush->finish_length * 0.25;
			}
			else
			{
				enter_alpha = 1;
				r = 1;
			}

			if((brush->core.flags & STAMP_PRESSURE_SIZE) != 0)
			{
				r *= brush->points[ref_point][3];
			}
			if((brush->core.flags & STAMP_RANDOM_SIZE) != 0)
			{
				r *= 1 - (brush->size_range * (rand() / (FLOAT_T)RAND_MAX));
			}
			if(r < MIN_BRUSH_STEP)
			{
				r = MIN_BRUSH_STEP;
			}

			if((brush->core.flags & STAMP_RANDOM_ROTATE) != 0)
			{
				brush->core.rotate = (rand() / (FLOAT_T)RAND_MAX) * (2*G_PI);
			}

			half_width = brush->core.half_width * r;
			half_height = brush->core.half_height * r;
			zoom = (1/r) * brush->core.scale;
			r *= brush->core.r;

			if(brush->sum_distance < brush->draw_start)
			{
				out_alpha = brush->sum_distance / brush->draw_start;
				r -= (brush->draw_start - brush->sum_distance) * 0.25;
			}
			else
			{
				out_alpha = 1;
			}

			if(r < 0.0)
			{
				brush->draw_finished++;
				continue;
			}

			alpha = ((brush->core.flags & BRUSH_FLAG_FLOW) == 0)
				? 1 : brush->points[ref_point][3];
			alpha *= enter_alpha;

			before_point = (ref_point == 0) ? BRUSH_POINT_BUFFER_SIZE - 1 : ref_point - 1;
			d = brush->points[ref_point][0];
			brush->sum_distance -= d;
			if(brush->draw_finished == 0)
			{
				draw_x = brush->points[0][1], draw_y = brush->points[0][2];
			}
			else
			{
				draw_x = brush->points[before_point][1];
				draw_y = brush->points[before_point][2];
			}
			dx = brush->points[ref_point][1] - draw_x;
			dy = brush->points[ref_point][2] - draw_y;

			if((brush->core.flags & STAMP_RANDOM_ROTATE) != 0)
			{
				brush->core.rotate = brush->core.rotate_start +
					(rand() / (FLOAT_T)RAND_MAX) * brush->core.rotate_direction * brush->rotate_range;
				if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
				{
					brush->core.rotate += brush->core.rotate_direction * atan2(dy, dx);
				}
			}
			else if((brush->core.flags & BRUSH_FLAG_ROTATE) != 0)
			{
				brush->core.rotate = brush->core.rotate_start + brush->core.rotate_direction * atan2(dy, dx);
			}
			else
			{
				brush->core.rotate = brush->core.rotate_start;
			}
			cos_x = cos(brush->core.rotate), sin_y = sin(brush->core.rotate);
			diff_x = dx/d, diff_y = dy/d;

			min_x = brush->points[ref_point][1] - r - 1;
			max_x = brush->points[ref_point][1] + r + 1;
			min_y = brush->points[ref_point][2] - r - 1;
			max_y = brush->points[ref_point][2] + r + 1;

			if(min_x < 0.0)
			{
				min_x = 0.0;
			}
			if(core->min_x > min_x)
			{
				core->min_x = min_x;
			}
			if(min_y < 0.0)
			{
				min_y = 0.0;
			}
			if(core->min_y > min_y)
			{
				core->min_y = min_y;
			}
			if(max_x > window->work_layer->width)
			{
				max_x = window->work_layer->width;
			}
			if(core->max_x < max_x)
			{
				core->max_x = max_x;
			}
			if(max_y > window->work_layer->height)
			{
				min_y = window->work_layer->height;
			}
			if(core->max_y < max_y)
			{
				core->max_y = max_y;
			}

			if(window->update.x > min_x)
			{
				window->update.width += window->update.x - (int)min_x;
				window->update.x = (int)min_x;
			}
			if(window->update.width + window->update.x < max_x)
			{
				window->update.width += (int)max_x - window->update.width + window->update.x;
			}
			if(window->update.y > min_y)
			{
				window->update.height += window->update.y - (int)min_y;
				window->update.y = (int)min_y;
			}
			if(window->update.height + window->update.y < max_y)
			{
				window->update.height += (int)max_y - window->update.height + window->update.y;
			}

			if(brush->stamp_distance > brush->core.d || brush->draw_finished == 0)
			{
				dx = d;
				do
				{
					if(draw_x < 0 || draw_x >= window->width
						|| draw_y < 0 || draw_y >= window->height)
					{
						dx = 0;
						goto skip_draw;
					}

					start_x = (int)(draw_x - r);
					start_y = (int)(draw_y - r);
					width = (int)(draw_x + r);
					height = (int)(draw_y + r);

					if(start_x < 0)
					{
						start_x = 0;
					}
					else if(start_x > window->work_layer->width)
					{
						goto skip_draw;
					}
					if(start_y < 0)
					{
						start_y = 0;
					}
					else if(start_y > window->work_layer->height)
					{
						goto skip_draw;
					}
					if(width > window->work_layer->width)
					{
						width = window->work_layer->width - start_x;
					}
					else
					{
						width = width - start_x;
					}
					if(height > window->work_layer->height)
					{
						height = window->work_layer->height - start_y;
					}
					else
					{
						height = height - start_y;
					}

					if(width < 0 || height < 0)
					{
						goto skip_draw;
					}

					stride = width*4;

					update_surface = cairo_surface_create_for_rectangle(
						window->mask_temp->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
					update = cairo_create(update_surface);

					for(i=0; i<height; i++)
					{
						(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
							0, stride);
					}

					trans_x = r - (half_width * cos_x + half_height * sin_y);
					trans_y = r + (half_width * sin_y - half_height * cos_x);

					// �s��f�[�^�Ɋg��E�k���A��]�p���Z�b�g
					cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_matrix_rotate(&matrix, brush->core.rotate);
					cairo_matrix_translate(&matrix, - trans_x, - trans_y);

					mask = window->mask_temp->pixels;
					if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_set_source(update, core->brush_pattern);
							cairo_pattern_set_matrix(core->brush_pattern, &matrix);
							cairo_paint_with_alpha(update, alpha);
						}
						else
						{
							cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_set_source(update, core->temp_pattern);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_mask_surface(update,
								window->active_layer->surface_p, - draw_x + r, - draw_y + r);
						}
					}
					else
					{
						if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
						{
							cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_set_source(update, core->temp_pattern);
							cairo_pattern_set_matrix(core->temp_pattern, &matrix);
							cairo_mask_surface(update,
								window->selection->surface_p, - draw_x + r, - draw_y + r);
						}
						else
						{
							temp_surface = cairo_surface_create_for_rectangle(
								window->temp_layer->surface_p, draw_x-r, draw_y-r, r*2+1, r*2+1
							);
							update_temp = cairo_create(temp_surface);

							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}

							cairo_pattern_set_matrix(core->brush_pattern, &reset_matrix);
							cairo_set_source(core->temp_cairo, core->brush_pattern);
							cairo_paint_with_alpha(core->temp_cairo, alpha);
							cairo_set_source(update, core->temp_pattern);
							cairo_mask_surface(update,
								window->selection->surface_p, - draw_x + r, - draw_y + r);
							cairo_set_source_surface(update_temp,
								update_surface, 0, 0);
							cairo_mask_surface(update_temp,
								window->active_layer->surface_p, - draw_x + r, - draw_y + r);

							mask = window->temp_layer->pixels;

							cairo_surface_destroy(temp_surface);
							cairo_destroy(update_temp);
						}
					}

					// �e�N�X�`����K�p
					if(window->app->textures.active_texture != 0)
					{
						temp_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
						update_temp = cairo_create(temp_surface);

						if(mask == window->mask_temp->pixels)
						{
							for(i=0; i<height; i++)
							{
								(void)memset(&window->temp_layer->pixels[
									(i+start_y)*window->temp_layer->stride+start_x*4],
									0, stride
								);
							}
							cairo_set_source_surface(update_temp,
								update_surface, 0, 0);
							cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);
							mask = window->temp_layer->pixels;
						}
						else
						{
							for(i=0; i<height; i++)
							{
								(void)memset(&window->mask_temp->pixels[
									(i+start_y)*window->mask_temp->stride+start_x*4],
									0, stride
								);
							}
							cairo_set_source_surface(update,
								temp_surface, 0, 0);
							cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
							mask = window->mask_temp->pixels;
						}

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}

					if(brush->picker_source == COLOR_PICKER_SOURCE_ACTIVE_LAYER)
					{
						(void)memcpy(window->mask_temp->pixels, window->active_layer->pixels,
							window->pixel_buf_size);
						target = window->mask_temp;
						g_layer_blend_funcs[brush->blend_mode](window->work_layer, target);
					}
					else
					{
						target = window->mixed_layer;
					}

					if(brush->picker_mode == PICKER_MODE_SINGLE_PIXEL)
					{
						int int_x = (int)x, int_y = (int)y;
						color[0] = target->pixels[target->stride*int_y+int_x*4];
						color[1] = target->pixels[target->stride*int_y+int_x*4+1];
						color[2] = target->pixels[target->stride*int_y+int_x*4+2];
						color[3] = target->pixels[target->stride*int_y+int_x*4+3];
					}
					else
					{
						sum_color[0] = sum_color[1] = sum_color[2] = sum_color[4] = 0;
						sum_color[3] = sum_color[5] = 1;
						for(i=0; i<height; i++)
						{
							ref_pix = &target->pixels[(i+start_y)*target->stride+start_x*4];
							mask_pix = &mask[(i+start_y)*window->stride+start_x*4+3];
							for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
							{
								sum_color[0] += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
								sum_color[1] += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
								sum_color[2] += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
								sum_color[3] += ((ref_pix[3]+1) * *mask_pix) >> 8;
								sum_color[4] += ref_pix[3] * *mask_pix;
								sum_color[5] += *mask_pix;
							}
						}

						color[0] = (uint8)((sum_color[0] + sum_color[3] / 2) / sum_color[3]);
						color[1] = (uint8)((sum_color[1] + sum_color[3] / 2) / sum_color[3]);
						color[2] = (uint8)((sum_color[2] + sum_color[3] / 2) / sum_color[3]);
						color[3] = (uint8)(sum_color[4] / sum_color[5]);
					}

					// �F����
					if((brush->flags & PICKER_FLAG_CHANGE_HSV) != 0)
					{
						HSV hsv;
						int s, v;
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
						uint8 r;
						r = color[2];
						color[2] = color[0];
						color[0] = r;
#endif
						RGB2HSV_Pixel(color, &hsv);
						hsv.h += brush->add_h;
						s = hsv.s, v = hsv.v;
						if(hsv.h < 0)
						{
							hsv.h += 360;
						}
						else if(hsv.h >= 360)
						{
							hsv.h -= 360;
						}
						s += brush->add_s;
						if(s < 0)
						{
							s = 0;
						}
						else if(s > 255)
						{
							s = 255;
						}
						hsv.s = (uint8)s;

						v += brush->add_v;
						if(v < 0)
						{
							v = 0;
						}
						else if(v > 255)
						{
							v = 255;
						}
						hsv.v = (uint8)v;
						HSV2RGB_Pixel(&hsv, color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
						r = color[2];
						color[2] = color[0];
						color[0] = r;
#endif
					}

					cairo_surface_destroy(update_surface);
					cairo_destroy(update);

					if(mask == window->mask_temp->pixels)
					{
						temp_surface = cairo_surface_create_for_rectangle(
							window->mask_temp->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
						update_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
						mask = window->temp_layer->pixels;
					}
					else
					{
						temp_surface = cairo_surface_create_for_rectangle(
							window->temp_layer->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
						update_surface = cairo_surface_create_for_rectangle(
							window->mask_temp->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
						mask = window->mask_temp->pixels;
					}
					update = cairo_create(update_surface);
					for(i=0; i<height; i++)
					{
						(void)memset(&mask[(start_y+i)*window->stride+start_x*4], 0, stride);
					}
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
					cairo_set_source_rgba(update, color[2]*DIV_PIXEL,
						color[1]*DIV_PIXEL, color[0]*DIV_PIXEL, color[3]*DIV_PIXEL);
#else
					cairo_set_source_rgba(update, color[0]*DIV_PIXEL,
						color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, color[3]*DIV_PIXEL);
#endif
					cairo_mask_surface(update, temp_surface, 0, 0);

					cairo_destroy(update);
					cairo_surface_destroy(temp_surface);
					cairo_surface_destroy(update_surface);

					for(i=0; i<height; i++)
					{
						ref_pix = &window->work_layer->pixels[
							(start_y+i)*window->work_layer->stride+start_x*4];
						mask_pix = &mask[(start_y+i)*window->work_layer->stride
							+start_x*4];

						for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
						{
							if(ref_pix[3] < mask_pix[3])
							{
								ref_pix[0] = (uint8)((uint32)(((int)mask_pix[0]-(int)ref_pix[0])
									* mask_pix[3] >> 8) + ref_pix[0]);
								ref_pix[1] = (uint8)((uint32)(((int)mask_pix[1]-(int)ref_pix[1])
									* mask_pix[3] >> 8) + ref_pix[1]);
								ref_pix[2] = (uint8)((uint32)(((int)mask_pix[2]-(int)ref_pix[2])
									* mask_pix[3] >> 8) + ref_pix[2]);
								ref_pix[3] = (uint8)((uint32)(((int)mask_pix[3]-(int)ref_pix[3])
									* mask_pix[3] >> 8) + ref_pix[3]);
							}
							else if(mask_pix[3] > 0)
							{
								src_value = mask_pix[3];
								dst_value = ref_pix[3];
								src_alpha = src_value * DIV_PIXEL;
								dst_alpha = dst_value * DIV_PIXEL;
								div_alpha = src_alpha + dst_alpha*(1-src_alpha);
								if(div_alpha > 0)
								{
									div_alpha = 1 / div_alpha;
									ref_pix[0] = (uint8)(color[0]*src_alpha+(ref_pix[0]*dst_alpha*(1-src_alpha))*div_alpha);
									ref_pix[1] = (uint8)(color[1]*src_alpha+(ref_pix[1]*dst_alpha*(1-src_alpha))*div_alpha);
									ref_pix[2] = (uint8)(color[2]*src_alpha+(ref_pix[2]*dst_alpha*(1-src_alpha))*div_alpha);
								}
							}
						}
					}

					update_r = r * window->zoom_rate;
					update_x = ((draw_x-window->width/2)*window->cos_value + (draw_y-window->height/2)*window->sin_value) * window->zoom_rate
						+ window->rev_add_cursor_x;
					update_y = (- (draw_x-window->width/2)*window->sin_value + (draw_y-window->height/2)*window->cos_value) * window->zoom_rate
						+ window->rev_add_cursor_y;
					gtk_widget_queue_draw_area(window->window, (gint)(update_x-update_r), (gint)(update_y-update_r),
						(gint)(update_r * 2 + BRUSH_UPDATE_MARGIN), (gint)(update_r * 2 + BRUSH_UPDATE_MARGIN));
skip_draw:
					dx -= brush->core.d;
					brush->stamp_distance -= brush->core.d;
					if(dx < brush->core.d)
					{
						break;
					}
					else
					{
						draw_x += diff_x, draw_y += diff_y;
					}
				} while(1);
			}

			brush->finish_length += d;
			brush->travel += d;
			brush->draw_finished++;
			ref_point = brush->draw_finished % BRUSH_POINT_BUFFER_SIZE;
		}	// while(brush->ref_point > brush->draw_finished)

		AddBrushHistory(core, window->active_layer);

		window->update.surface_p = cairo_surface_create_for_rectangle(
			window->active_layer->surface_p, window->update.x, window->update.y,
				window->update.width, window->update.height);
		window->update.cairo_p = cairo_create(window->update.surface_p);
		g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->update);
		cairo_destroy(window->update.cairo_p);
		cairo_surface_destroy(window->update.surface_p);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;

		brush->core.rotate = brush->core.rotate_start;

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}	// �}�E�X�̍��{�^���Ȃ�
		// if(((GdkEventButton*)state)->button == 1)
}

static void PickerImageBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	StampCoreDrawCursor(window, x, y, &((PICKER_IMAGE_BRUSH*)data)->core);
}

static void PickerImageBrushButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, PICKER_IMAGE_BRUSH* brush)
{
	gdouble r;

	if(brush->core.half_width > brush->core.half_height)
	{
		r = brush->core.half_width * 2 * window->zoom_rate + 1;
	}
	else
	{
		r = brush->core.half_height * 2 * window->zoom_rate + 1;
	}

	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2) + 2, (gint)(r * 2) + 2);
}

static void PickerImageBrushMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, PICKER_IMAGE_BRUSH* brush)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r;
	gdouble enter = (brush->enter_length + brush->draw_start) * window->zoom_rate;

	if(brush->core.half_width > brush->core.half_height)
	{
		r = brush->core.half_width * 2 * window->zoom_rate + 1;
	}
	else
	{
		r = brush->core.half_height * 2 * window->zoom_rate + 1;
	}

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	start_x -= enter * 4;
	width += enter * 4 * 2 + 2;
	start_y -= enter * 4;
	height += enter * 4 * 2 + 2;

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width, (gint)height);
}

static void PickerImageBrushSetEnter(GtkAdjustment* slider, PICKER_IMAGE_BRUSH* brush)
{
	brush->enter = gtk_adjustment_get_value(slider);
}

static void PickerImageBrushSetOut(GtkAdjustment* slider, PICKER_IMAGE_BRUSH* brush)
{
	brush->out = gtk_adjustment_get_value(slider);
}

static void PickerImageBrushSetBlendMode(GtkComboBox* combo, PICKER_IMAGE_BRUSH* brush)
{
	brush->blend_mode = (uint16)gtk_combo_box_get_active(combo);
}

static void PickerImageBrushSetRotate2Direction(GtkWidget* button, PICKER_IMAGE_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		brush->core.flags &= ~(BRUSH_FLAG_ROTATE);
	}
	else
	{
		brush->core.flags |= BRUSH_FLAG_ROTATE;
	}
}

static void PickerImageBrushSetRotateDirection(GtkWidget* button, PICKER_IMAGE_BRUSH* brush)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		brush->core.rotate_direction = GPOINTER_TO_INT(g_object_get_data(
			G_OBJECT(button), "rotate_direction"));
	}
}

static void PickerImageBrushSetAddHue(GtkAdjustment* slider, PICKER_IMAGE_BRUSH* brush)
{
	brush->add_h = (int)gtk_adjustment_get_value(slider);

	if(brush->add_h == 0 && brush->add_s == 0 && brush->add_v == 0)
	{
		brush->flags &= ~(PICKER_FLAG_CHANGE_HSV);
	}
	else
	{
		brush->flags |= PICKER_FLAG_CHANGE_HSV;
	}
}

static void PickerImageBrushSetAddSaturation(GtkAdjustment* slider, PICKER_IMAGE_BRUSH* brush)
{
	brush->add_s = (int)(gtk_adjustment_get_value(slider) * 2.552);

	if(brush->add_h == 0 && brush->add_s == 0 && brush->add_v == 0)
	{
		brush->flags &= ~(PICKER_FLAG_CHANGE_HSV);
	}
	else
	{
		brush->flags |= PICKER_FLAG_CHANGE_HSV;
	}
}

static void PickerImageBrushSetAddVivid(GtkAdjustment* slider, PICKER_IMAGE_BRUSH* brush)
{
	brush->add_v = (int)(gtk_adjustment_get_value(slider) * 2.552);

	if(brush->add_h == 0 && brush->add_s == 0 && brush->add_v == 0)
	{
		brush->flags &= ~(PICKER_FLAG_CHANGE_HSV);
	}
	else
	{
		brush->flags |= PICKER_FLAG_CHANGE_HSV;
	}
}

static void PickerImageBrushSetPickTarget(GtkWidget* button, PICKER_IMAGE_BRUSH* brush)
{
	int mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "pick_target"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		brush->picker_source = (uint8)mode;
	}
}

static void PickerImageBrushSetPickMode(GtkWidget* button, PICKER_IMAGE_BRUSH* brush)
{
	int mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "pick_mode"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		brush->picker_mode = (uint8)mode;
	}
}

static void PickerImageBrushSetSizeRange(GtkAdjustment* slider, PICKER_IMAGE_BRUSH* brush)
{
	brush->size_range = gtk_adjustment_get_value(slider) * 0.01;
}


static void PickerImageBrushSetRotateRange(GtkAdjustment* slider, PICKER_IMAGE_BRUSH* brush)
{
	brush->random_rotate_range = (int16)gtk_adjustment_get_value(slider);
	brush->rotate_range = brush->random_rotate_range * G_PI / 180;
}

static GtkWidget* CreatePickerImageBrushDetailUI(APPLICATION* app, BRUSH_CORE* brush_core)
{
#define UI_FONT_SIZE 8.0
#define ICON_SIZE 32
	PICKER_IMAGE_BRUSH *brush = (PICKER_IMAGE_BRUSH*)brush_core->brush_data;
	STAMP_CORE *core = &brush->core;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox;
	// ���x���A�X���C�_
	GtkWidget* label, *scale;
	// �X�N���[���h�E�B���h�E
	GtkWidget* scrolled_window;
	// �E�B�W�F�b�g����p�̃e�[�u��
	GtkWidget* table;
	// ��]�����A���[�h�ݒ�p�̃��W�I�{�^��
	GtkWidget* radio_buttons[PATTERN_MODE_NUM];
	// �M���g�p�ݒ�̃`�F�b�N�{�b�N�X
	GtkWidget* check_button;
	// �������[�h�I��p�̃R���{�{�b�N�X
	GtkWidget* combo;
	// �X���C�_�Ɏg�p����A�W���X�^
	GtkAdjustment* scale_adjustment;
	// ���x���̃t�H���g�ύX�p�̃}�[�N�A�b�v�o�b�t�@
	char mark_up_buff[256];
	int i;	// for���p�̃J�E���^

	// �A�v���P�[�V�����Ǘ��̍\���̂ւ̃|�C���^���Z�b�g
	core->app = app;

	// �N���b�v�{�[�h�̃p�^�[�����X�V
	UpdateClipBoardPattern(&app->stamps);

	// �g��k�����ݒ�p�̃E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		(1 / core->scale) * 100, 10, 300, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeScale), core);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �Z�x�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		core->flow * 100, 0, 100, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.flow, 1);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeFlow), core);
	table = gtk_table_new(1, 3, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �J�n��]�p�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		fabs(core->rotate_start) * 180 / G_PI, 0, 360, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.rotate_start, 1);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeRotateStart), core);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �Ԋu�ύX�p�E�B�W�F�b�g
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		core->stamp_distance, 0.1, 3, 0.1, 0.1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.distance, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(StampCoreChangeDistance), core);
	g_object_set_data(G_OBJECT(scale_adjustment), "application", app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// ���E���]�A�㉺���]�I��p�`�F�b�N�{�b�N�X
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.reverse);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.reverse_horizontally);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		core->pattern_flags & PATTERN_FLIP_HORIZONTALLY);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetPatternFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.reverse_vertically);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(1));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		core->pattern_flags & PATTERN_FLIP_VERTICALLY);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetPatternFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// ��]�����ύX�p�E�B�W�F�b�g
	radio_buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.clockwise);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.counter_clockwise
	);

	if(core->rotate_direction < 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
	}

	table = gtk_hbox_new(FALSE, 0);
	g_object_set_data(G_OBJECT(radio_buttons[0]), "rotate_direction", GINT_TO_POINTER(1));
	g_object_set_data(G_OBJECT(radio_buttons[1]), "rotate_direction", GINT_TO_POINTER(-1));
	g_signal_connect(G_OBJECT(radio_buttons[0]), "toggled",
		G_CALLBACK(ImageBrushSetRotateDirection), brush);
	g_signal_connect(G_OBJECT(radio_buttons[1]), "toggled",
		G_CALLBACK(ImageBrushSetRotateDirection), brush);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(table), radio_buttons[1], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �ڍאݒ��UI��������Ƃ��Ƀ{�^���z����J������
	g_signal_connect(G_OBJECT(vbox), "destroy", G_CALLBACK(OnStampDetailUIDestroy), core);

	// �X�^���v�I���e�[�u�����쐬���ăX�N���[���h�E�B���h�E�ɓ����
	table = gtk_hbox_new(FALSE, 0);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(table,
		ICON_SIZE*STAMP_SELECT_TABLE_WIDTH+4, ICON_SIZE*4+4);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
		CreateStampSelectTable(core));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(table), scrolled_window, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �N���b�v�{�[�h�̃X�^���v���A�N�e�B�u�Ȃ�΃Z�b�g
	if(app->stamps.has_clip_board_pattern != FALSE)
	{
		if(core->stamp_id == 0)
		{
			app->stamps.active_pattern =
				&app->stamps.clip_board;
		}
		else
		{
			app->stamps.active_pattern =
				&app->stamps.patterns[core->stamp_id-1];
		}
	}

	// �A�N�e�B�u�ȃp�^�[�����`�����l����2�ȊO�Ȃ�Β��F���[�h�͖���
	gtk_widget_set_sensitive(core->mode_select[0], app->stamps.active_pattern->channel == 2);
	gtk_widget_set_sensitive(core->mode_select[1], app->stamps.active_pattern->channel == 2);
	gtk_widget_set_sensitive(core->mode_select[2], app->stamps.active_pattern->channel == 2);

	InitializeStampCore(core, PATTERN_MODE_SATURATION, app);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
#if MAJOR_VERSION == 1
	combo = gtk_combo_box_new_text();
#else
	combo = gtk_combo_box_text_new();
#endif
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->layer_window.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
#if MAJOR_VERSION == 1
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), app->labels->layer_window.blend_modes[i]);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), app->labels->layer_window.blend_modes[i]);
#endif
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), brush->blend_mode);
	(void)g_signal_connect(G_OBJECT(combo), "changed",
		G_CALLBACK(ImageBrushSetBlendMode), brush);
	gtk_box_pack_start(GTK_BOX(table), combo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->enter, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.enter, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ImageBrushSetEnter), core);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->out, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(scale_adjustment,
		app->labels->tool_box.out, 1);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(ImageBrushSetOut), core);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �M���g�p�ݒ�ύX�p�E�B�W�F�b�g
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), core->flags & BRUSH_FLAG_SIZE);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), core->flags & BRUSH_FLAG_FLOW);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlags), core);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.randoam_size);
	g_object_set_data(G_OBJECT(check_button), "flag-value", GUINT_TO_POINTER(STAMP_RANDOM_SIZE));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), core->flags & STAMP_RANDOM_SIZE);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlag), core);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	// �u���V�̈ړ������։�]
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.rotate_to_brush_direction);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		brush->core.flags & BRUSH_FLAG_ROTATE);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(ImageBrushSetRotate2Direction), brush);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

	// �����_���ɉ�]
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.random_rotate);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), (core->flags & STAMP_RANDOM_ROTATE) != 0);
	g_object_set_data(G_OBJECT(check_button), "flag-value", GUINT_TO_POINTER(STAMP_RANDOM_ROTATE));
	g_object_set_data(G_OBJECT(check_button), "application", app);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(StampCoreSetFlag), core);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	// �F����
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		brush->add_h, -180.0, 180.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerImageBrushSetAddHue), brush);
	scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.hue, 1);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		(brush->add_s/255.0)*100, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerImageBrushSetAddSaturation), brush);
	scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.saturation, 1);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		(brush->add_h/255.0)*100, 0.0, 100.0, 1.0, 1.0, 0.0));
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerImageBrushSetAddVivid), brush);
	scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.brightness, 1);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	if(brush->add_h == 0 && brush->add_s == 0 && brush->add_v == 0)
	{
		brush->flags &= ~(PICKER_FLAG_CHANGE_HSV);
	}
	else
	{
		brush->flags |= PICKER_FLAG_CHANGE_HSV;
	}

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->unit.target);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	radio_buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.active_layer);
	g_object_set_data(G_OBJECT(radio_buttons[0]), "pick_target", GINT_TO_POINTER(COLOR_PICKER_SOURCE_ACTIVE_LAYER));
	g_signal_connect(G_OBJECT(radio_buttons[0]), "toggled", G_CALLBACK(PickerImageBrushSetPickTarget), brush);
	radio_buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
		GTK_RADIO_BUTTON(radio_buttons[0])), app->labels->tool_box.select.canvas);
	g_object_set_data(G_OBJECT(radio_buttons[1]), "pick_target", GINT_TO_POINTER(COLOR_PICKER_SOURCE_CANVAS));
	g_signal_connect(G_OBJECT(radio_buttons[1]), "toggled", G_CALLBACK(PickerImageBrushSetPickTarget), brush);
	gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[1], FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[brush->picker_source]), TRUE);

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pick_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	radio_buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.single_pixels);
	g_object_set_data(G_OBJECT(radio_buttons[0]), "pick_mode", GINT_TO_POINTER(PICKER_MODE_SINGLE_PIXEL));
	g_signal_connect(G_OBJECT(radio_buttons[0]), "toggled", G_CALLBACK(PickerImageBrushSetPickMode), brush);
	radio_buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
		GTK_RADIO_BUTTON(radio_buttons[0])), app->labels->tool_box.average_color);
	g_object_set_data(G_OBJECT(radio_buttons[1]), "pick_mode", GINT_TO_POINTER(PICKER_MODE_AVERAGE));
	g_signal_connect(G_OBJECT(radio_buttons[1]), "toggled", G_CALLBACK(PickerImageBrushSetPickMode), brush);
	gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[1], FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[brush->picker_mode]), TRUE);

	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->size_range * 100, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.size_range, 0);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerImageBrushSetSizeRange), brush);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	brush->rotate_range = brush->random_rotate_range * G_PI / 180;
	scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(brush->random_rotate_range, 0, 360, 1, 1, 0));
	scale = SpinScaleNew(scale_adjustment, app->labels->tool_box.rotate_range, 0);
	g_signal_connect(G_OBJECT(scale_adjustment), "value_changed",
		G_CALLBACK(PickerImageBrushSetRotateRange), brush);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

#undef UI_FONT_SIZE
#undef ICON_SIZE

	return vbox;
}

static void EraserButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		ERASER* eraser = (ERASER*)core->brush_data;
		cairo_t *update;
		cairo_surface_t *update_surface;
		cairo_matrix_t matrix;
		gdouble r, alpha, zoom;
		gdouble min_x, min_y, max_x, max_y;

		window->work_layer->layer_mode = LAYER_BLEND_ALPHA_MINUS;

		if((eraser->flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = eraser->r;
			zoom = 1;
		}
		else
		{
			r = eraser->r * pressure;
			zoom = 1/pressure;
		}
		alpha = ((eraser->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;

		min_x = x - r, min_y = y - r;
		max_x = x + r, max_y = y + r;

		eraser->before_x = x, eraser->before_y = y;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		window->update.x = (int)min_x, window->update.y = (int)min_y;
		window->update.width = (int)max_x - window->update.x;
		window->update.height = (int)max_y - window->update.y;

		update_surface = cairo_surface_create_for_rectangle(
			window->work_layer->surface_p, min_x, min_y, r*2+1, r*2+1);
		update = cairo_create(update_surface);

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);

		// �e�N�X�`���̗L���ŏ�����؂�ւ�
		if(window->app->textures.active_texture == 0)
		{
			// �I��͈̗͂L��������
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{	// �I��͈͖�
					// �u���V�͈̔͂�h��ׂ�
				// �u���V�̈ʒu�Ɗg�嗦���Z�b�g
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				// �u���V���X�^���o�C
				cairo_set_source(update, core->brush_pattern);
				// �M����K�p���ĕ`��
				cairo_paint_with_alpha(update, alpha);
			}
			else
			{	// �I��͈͗L
					// �M����K�p�����u���V�p�^�[�������
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �I��͈͂Ń}�X�N
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(update, core->temp_pattern);
				cairo_mask_surface(update, window->selection->surface_p, - x + r, - y + r);
			}
		}
		else
		{
			cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
				window->temp_layer->surface_p, min_x, min_y, r*2+1, r*2+1);
			cairo_t *update_temp = cairo_create(temp_surface);

			// �I��͈̗͂L��������
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{	// �I��͈͖�
					// �u���V�͈̔͂�h��ׂ�
				// �u���V�̈ʒu�Ɗg�嗦���Z�b�g
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				// �ꎞ�ۑ����C���[�Ɉ�x�`��
				(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
				// �u���V���X�^���o�C
				cairo_set_source(update_temp, core->brush_pattern);
				// �M����K�p���ĕ`��
				cairo_paint_with_alpha(update_temp, alpha);

				// �e�N�X�`����K�p
				cairo_set_source_surface(update, temp_surface, 0, 0);
				cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
			}
			else
			{	// �I��͈͗L
					// �M����K�p�����u���V�p�^�[�������
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �I��͈͂Ń}�X�N
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				// �ꎞ�ۑ��̃��C���[�Ɉ�x�`��
				(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
				cairo_set_source(update_temp, core->temp_pattern);
				cairo_mask_surface(update_temp, window->selection->surface_p, - x + r, - y + r);

				// �e�N�X�`����K�p
				cairo_set_source_surface(update, temp_surface, 0, 0);
				cairo_mask_surface(update, window->texture->surface_p, - x + r, - y + r);
			}

			cairo_destroy(update_temp);
			cairo_surface_destroy(temp_surface);
		}

		cairo_destroy(update);
		cairo_surface_destroy(update_surface);

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}
}

static void EraserEditSelectionButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		ERASER* eraser = (ERASER*)core->brush_data;
		gdouble r, alpha, zoom;
		gdouble min_x, min_y, max_x, max_y;
		cairo_matrix_t matrix;

		window->selection->layer_mode = SELECTION_BLEND_MINUS;

		if((eraser->flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = eraser->r;
			zoom = 1;
		}
		else
		{
			r = eraser->r * pressure;
			zoom = 1/pressure;
		}
		alpha = ((eraser->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;

		min_x = x - r, min_y = y - r;
		max_x = x + r, max_y = y + r;

		eraser->before_x = x, eraser->before_y = y;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		// �e�N�X�`���̗L���ŏ�����؂�ւ�
		if(window->app->textures.active_texture == 0)
		{
			// �I��͈̗͂L��������
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{	// �I��͈͖�
					// �u���V�͈̔͂�h��ׂ�
				// �u���V�̈ʒu�Ɗg�嗦���Z�b�g
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_matrix_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				// �u���V���X�^���o�C
				cairo_set_source(window->work_layer->cairo_p, core->brush_pattern);
				// �M����K�p���ĕ`��
				cairo_paint_with_alpha(window->work_layer->cairo_p, alpha);
			}
			else
			{	// �I��͈͗L
					// �M����K�p�����u���V�p�^�[�������
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �I��͈͂Ń}�X�N
				cairo_matrix_init_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(window->work_layer->cairo_p, core->temp_pattern);
				cairo_mask_surface(window->work_layer->cairo_p, window->selection->surface_p, 0, 0);
			}
		}
		else
		{
			// �I��͈̗͂L��������
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{	// �I��͈͖�
					// �u���V�͈̔͂�h��ׂ�
				// �u���V�̈ʒu�Ɗg�嗦���Z�b�g
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_matrix_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				// �ꎞ�ۑ����C���[�Ɉ�x�`��
				(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
				// �u���V���X�^���o�C
				cairo_set_source(window->temp_layer->cairo_p, core->brush_pattern);
				// �M����K�p���ĕ`��
				cairo_paint_with_alpha(window->temp_layer->cairo_p, alpha);

				// �e�N�X�`����K�p
				cairo_set_source_surface(window->work_layer->cairo_p, window->temp_layer->surface_p, 0, 0);
				cairo_mask_surface(window->work_layer->cairo_p, window->texture->surface_p, 0, 0);
			}
			else
			{	// �I��͈͗L
					// �M����K�p�����u���V�p�^�[�������
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �I��͈͂Ń}�X�N
				cairo_matrix_init_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				// �ꎞ�ۑ��̃��C���[�Ɉ�x�`��
				(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
				cairo_set_source(window->temp_layer->cairo_p, core->temp_pattern);
				cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);

				// �e�N�X�`����K�p
				cairo_set_source_surface(window->work_layer->cairo_p, window->temp_layer->surface_p, 0, 0);
				cairo_mask_surface(window->work_layer->cairo_p, window->texture->surface_p, 0, 0);
			}
		}
	}
}

static void EraserMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		ERASER* eraser = (ERASER*)core->brush_data;
		cairo_t *update;
		cairo_surface_t *update_surface;
		cairo_matrix_t matrix;
		gdouble r, step, alpha, d;
		gdouble min_x, min_y, max_x, max_y;
		gdouble draw_x = eraser->before_x, draw_y = eraser->before_y;
		gdouble dx, dy, diff_x, diff_y;
		gdouble zoom;
		int32 clear_x, clear_width, clear_y, clear_height;
		uint8* color = *core->color;
		int i, j, k;

		if((eraser->flags & BRUSH_FLAG_SIZE) == 0)
		{
			r = eraser->r;
			zoom = 1;
		}
		else
		{
			r = eraser->r * pressure;
			zoom = 1/pressure;
		}
		step = r * BRUSH_STEP;
		alpha = ((eraser->flags & BRUSH_FLAG_FLOW) == 0) ?
			1 : pressure;
		dx = x-eraser->before_x, dy = y-eraser->before_y;
		d = sqrt(dx*dx+dy*dy);
		diff_x = step * dx/d, diff_y = step * dy/d;
		if(step < 0.01)
		{
			dx = 0;
			goto skip_draw;
		}

		min_x = x - r*2, min_y = y - r*2;
		max_x = x + r*2, max_y = y + r*2;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		if(window->update.x > min_x)
		{
			window->update.width += window->update.x - (int)min_x;
			window->update.x = (int)min_x;
		}
		if(window->update.width + window->update.x < max_x)
		{
			window->update.width += (int)max_x - window->update.width + window->update.x;
		}
		if(window->update.y > min_y)
		{
			window->update.height += window->update.y - (int)min_y;
			window->update.y = (int)min_y;
		}
		if(window->update.height + window->update.y < max_y)
		{
			window->update.height += (int)max_y - window->update.height + window->update.y;
		}

		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		dx = d;
		while(1)
		{
			clear_x = (int32)(draw_x - r - 1);
			clear_width = (int32)(draw_x + r + 1);
			clear_y = (int32)(draw_y - r - 1);
			clear_height = (int32)(draw_y + r + 1);

			if(clear_x < 0)
			{
				clear_x = 0;
			}
			else if(clear_width > window->width)
			{
				clear_width = window->width;
			}
			if(clear_y < 0)
			{
				clear_y = 0;
			}
			if(clear_height > window->height)
			{
				clear_height = window->height;
			}
			clear_width = clear_width - clear_x;
			clear_height = clear_height - clear_y;

			if(clear_width <= 0 || clear_height <= 0)
			{
				goto skip_draw;
			}

			update_surface = cairo_surface_create_for_rectangle(
				window->temp_layer->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
			update = cairo_create(update_surface);

			for(i=0; i<clear_height; i++)
			{
				(void)memset(&window->temp_layer->pixels[(i+clear_y)*window->work_layer->stride+clear_x*window->work_layer->channel],
					0x0, clear_width*window->work_layer->channel);
			}

			// �e�N�X�`���̗L���ŏ�����؂�ւ�
			if(window->app->textures.active_texture == 0)
			{
				// �I��͈̗͂L��������
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{	// �I��͈͖�
						// �u���V�͈̔͂�h��ׂ�
					// �u���V�̈ʒu�Ɗg�嗦���Z�b�g
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					// �u���V���X�^���o�C
					cairo_set_source(update, core->brush_pattern);
					// �M����K�p���ĕ`��
					cairo_paint_with_alpha(update, alpha);
				}
				else
				{	// �I��͈͗L
						// �M����K�p�����u���V�p�^�[�������
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					// �I��͈͂Ń}�X�N
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update, core->temp_pattern);
					cairo_mask_surface(update, window->selection->surface_p, - draw_x + r, - draw_y + r);
				}
			}
			else
			{
				cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
					window->mask_temp->surface_p, draw_x - r, draw_y - r, r*2+1, r*2+1);
				cairo_t *update_temp = cairo_create(temp_surface);

				for(i=0; i<clear_height; i++)
				{
					(void)memset(&window->mask_temp->pixels[(i+clear_y)*window->work_layer->stride+clear_x*window->work_layer->channel],
						0x0, clear_width*window->work_layer->channel);
				}

				// �I��͈̗͂L��������
				if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
				{	// �I��͈͖�
						// �u���V�͈̔͂�h��ׂ�
					// �u���V�̈ʒu�Ɗg�嗦���Z�b�g
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					// �}�X�N�p�̈ꎞ�ۑ��Ɉ�x�`��
						// �u���V���X�^���o�C
					cairo_set_source(update_temp, core->brush_pattern);
						// �M����K�p���ĕ`��
					cairo_paint_with_alpha(update_temp, alpha);

					// �e�N�X�`����K�p
					cairo_set_source_surface(update, temp_surface, 0, 0);
					cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
				}
				else
				{	// �I��͈͗L
						// �M����K�p�����u���V�p�^�[�������
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					// �}�X�N�p�̈ꎞ�ۑ��Ɉ�x�`��
						// �I��͈͂Ń}�X�N
					cairo_matrix_init_translate(&matrix, 0, 0);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(update_temp, core->temp_pattern);
					cairo_mask_surface(update_temp, window->selection->surface_p, - draw_x + r, - draw_y + r);

					// �e�N�X�`����K�p
					cairo_set_source_surface(update, temp_surface, 0, 0);
					cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
				}

				cairo_destroy(update_temp);
				cairo_surface_destroy(temp_surface);
			}

			cairo_destroy(update);
			cairo_surface_destroy(update_surface);
		
			for(i=0; i<clear_height; i++)
			{
				for(j=0; j<clear_width; j++)
				{
					if(window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3]
						> window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+3])
					{
						for(k=0; k<window->temp_layer->channel; k++)
						{
							window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k] =
								(uint32)(((int)window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+k]
								- (int)window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k])
									* window->temp_layer->pixels[(clear_y+i)*window->temp_layer->stride+(clear_x+j)*window->temp_layer->channel+3] >> 8)
									+ window->work_layer->pixels[(clear_y+i)*window->work_layer->stride+(clear_x+j)*window->work_layer->channel+k];
						}
					}
				}
			}


skip_draw:
			dx -= step;
			if(dx < 1)
			{
				break;
			}
			else if(dx >= step)
			{
				draw_x += diff_x, draw_y += diff_y;
			}
			else
			{
				draw_x = x, draw_y = y;
			}
		}

		eraser->before_x = x, eraser->before_y = y;

		window->flags |= DRAW_WINDOW_UPDATE_PART;
	}
}

#define EraserEditSelectionMotionCallBack EraserMotionCallBack

#define EraserReleaseCallBack PencilReleaseCallBack
#define EraserEditSelectionReleaseCallBack DefaultEditSelectionReleaseCallBack

static void EraserDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	ERASER* eraser = (ERASER*)data;
	gdouble r = eraser->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void EraserButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, ERASER* eraser)
{
	gdouble r = eraser->r * window->zoom_rate + 3;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2 + 3), (gint)(r * 2 + 3));
}

static void EraserMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, ERASER* eraser)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = eraser->r * window->zoom_rate * 1.275 + BRUSH_UPDATE_MARGIN;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width + 2, (gint)height + 2);
}

static void EraserScaleChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	ERASER* eraser = (ERASER*)data;
	const uint8 color[3] = {0, 0, 0};
	eraser->r = gtk_adjustment_get_value(slider) * 0.5;

	BrushCoreSetCirclePattern(eraser->core, eraser->r, eraser->outline_hardness * 0.01,
		eraser->blur * 0.01, eraser->alpha * 0.01, color);
}

static void EraserFlowChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	ERASER* eraser = (ERASER*)data;
	const uint8 color[3] = {0, 0, 0};
	eraser->alpha = gtk_adjustment_get_value(slider);

	BrushCoreSetCirclePattern(eraser->core, eraser->r, eraser->outline_hardness * 0.01,
		eraser->blur * 0.01, eraser->alpha * 0.01, color);
}

static void EraserOutlineHardnessChange(GtkAdjustment* slider, ERASER* eraser)
{
	const uint8 color[3] = {0, 0, 0};
	eraser->outline_hardness = gtk_adjustment_get_value(slider);

	BrushCoreSetCirclePattern(eraser->core, eraser->r, eraser->outline_hardness * 0.01,
		eraser->blur * 0.01, eraser->alpha * 0.01, color);
}

static void EraserBlurChange(GtkAdjustment* slider, ERASER* eraser)
{
	const uint8 color[3] = {0, 0, 0};
	eraser->blur = gtk_adjustment_get_value(slider);

	BrushCoreSetCirclePattern(eraser->core, eraser->r, eraser->outline_hardness * 0.01,
		eraser->blur * 0.01, eraser->alpha * 0.01, color);
}

static void EraserPressureSizeChange(
	GtkWidget* widget,
	gpointer data
)
{
	ERASER* eraser = (ERASER*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		eraser->flags &= ~(BRUSH_FLAG_SIZE);
	}
	else
	{
		eraser->flags |= BRUSH_FLAG_SIZE;
	}
}

static void EraserPressureFlowChange(
	GtkWidget* widget,
	gpointer data
)
{
	ERASER* eraser = (ERASER*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		eraser->flags &= ~(BRUSH_FLAG_FLOW);
	}
	else
	{
		eraser->flags |= BRUSH_FLAG_FLOW;
	}
}

static GtkWidget* CreateEraserDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	ERASER* eraser = (ERASER*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* brush_scale;
	GtkWidget* table = gtk_table_new(1, 3, TRUE);
	GtkWidget* check_button;
	GtkWidget* base_scale;
	GtkWidget* hbox;
	GtkWidget* label;
	GtkAdjustment* brush_scale_adjustment;
	char mark_up_buff[256];
	const uint8 color[3] = {0, 0, 0};

	eraser->core = core;

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		base_scale = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[2]);
#else
		base_scale = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), eraser->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(eraser->base_scale)
	{
	case 0:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(eraser->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(eraser->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(eraser->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(PencilScaleChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	g_object_set_data(G_OBJECT(base_scale), "scale", brush_scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &eraser->base_scale);

	brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(eraser->alpha, 0.0, 100.0, 1.0, 1.0, 0.0));
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.flow, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(EraserFlowChange), core->brush_data);
	table = gtk_table_new(1, 3, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(eraser->outline_hardness, 0.0, 100.0, 1.0, 1.0, 0.0));
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.outline_hardness, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(EraserOutlineHardnessChange), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, TRUE, 0);

	brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(eraser->blur, 0.0, 100.0, 1.0, 1.0, 0.0));
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.blur, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(EraserBlurChange), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), brush_scale, FALSE, TRUE, 0);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(EraserPressureSizeChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), eraser->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(EraserPressureFlowChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), eraser->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	BrushCoreSetCirclePattern(eraser->core, eraser->r, eraser->outline_hardness * 0.01,
		eraser->blur * 0.01, eraser->alpha * 0.01, color);

	return vbox;
#undef UI_FONT_SIZE
}

static void BucketPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �`��̈�O�̃N���b�N�Ȃ�I��
	if(x < 0 || x > window->width || y < 0 || y > window->height)
	{
		return;
	}

	if(((GdkEventButton*)state)->button == 1)
	{
		BUCKET* bucket = (BUCKET*)core->brush_data;
		LAYER* target;
		uint8* buff = &window->temp_layer->pixels[window->width*window->height];
		int32 min_x, min_y, max_x, max_y;
		uint8 channel = (bucket->mode == BUCKET_RGB) ? 3 :
			(bucket->mode == BUCKET_ALPHA) ? 1 : 4;
		int i;

		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			if(window->selection->pixels[(int)y*window->selection->width+(int)x] < 0x80)
			{
				return;
			}
		}

		if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
		{
			window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		}
		else
		{
			window->work_layer->layer_mode = LAYER_BLEND_ATOP;
		}

		switch(bucket->target)
		{
		case BUCKET_TARGET_ACTIVE_LAYER:
			target = window->active_layer;
			break;
		case BUCKET_TARGET_CANVAS:
			target = MixLayerForSave(window);
			break;
		default:
			target = NULL;
		}

		(void)memset(window->mask_temp->pixels, 0, window->pixel_buf_size);
		(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);

		if(bucket->mode != BUCKET_ALPHA)
		{
			DetectSameColorArea(
				target,
				buff,
				&window->temp_layer->pixels[window->width*window->height*2],
				(int32)x, (int32)y,
				&target->pixels[(int32)y*target->stride + (int32)x*target->channel],
				channel, bucket->threshold, &min_x, &min_y, &max_x, &max_y,
				(eSELECT_FUZZY_DIRECTION)bucket->select_direction
			);
		}
		else
		{
			LAYER* local_target = CreateLayer(0, 0, target->width, target->height,
				1, TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);

			for(i=0; i<target->width*target->height; i++)
			{
				local_target->pixels[i] = target->pixels[i*4+3];
			}

			DetectSameColorArea(
				local_target,
				buff,
				&window->temp_layer->pixels[window->width*window->height*2],
				(int32)x, (int32)y,
				&target->pixels[(int32)y*target->stride + (int32)x*target->channel + 3],
				channel, bucket->threshold, &min_x, &min_y, &max_x, &max_y,
				(eSELECT_FUZZY_DIRECTION)bucket->select_direction
			);

			DeleteLayer(&local_target);
		}

		if((bucket->flags & BUCKET_FLAG_ANTI_ALIAS) != 0)
		{
			(void)memcpy(&window->temp_layer->pixels[window->width*window->height*2],
				buff, window->active_layer->width*window->active_layer->height);
			AntiAlias(&window->temp_layer->pixels[window->width*window->height*2],
				buff, window->active_layer->width, window->active_layer->height, window->active_layer->width, 1);
		}

		core->min_x = min_x - 1, core->min_y = min_y - 1;
		core->max_x = max_x + 1, core->max_y = max_y + 1;

		for(i=0; i<window->mask_temp->width*window->mask->height; i++)
		{
			window->mask_temp->pixels[i*4] = buff[i];
			window->mask_temp->pixels[i*4+1] = buff[i];
			window->mask_temp->pixels[i*4+2] = buff[i];
			window->mask_temp->pixels[i*4+3] = buff[i];
		}

		if(bucket->extend > 0)
		{
			(void)memcpy(window->mask_temp->pixels, buff,
				window->mask_temp->height * window->mask_temp->width);
			for(i=0; i<bucket->extend; i++)
			{
				ExtendSelectionAreaOneStep(window->mask_temp, window->temp_layer);
				(void)memcpy(window->mask_temp->pixels, window->temp_layer->pixels,
					window->mask_temp->width*window->mask_temp->height);
				core->min_x -= 1, core->min_y -= 1;
				core->max_x += 1, core->max_y += 1;
			}

			for(i=0; i<window->mask_temp->width*window->mask_temp->height; i++)
			{
				window->mask_temp->pixels[i*4] = window->temp_layer->pixels[i];
				window->mask_temp->pixels[i*4+1] = window->temp_layer->pixels[i];
				window->mask_temp->pixels[i*4+2] = window->temp_layer->pixels[i];
				window->mask_temp->pixels[i*4+3] = window->temp_layer->pixels[i];
			}
		}
		else
		{
			int end = abs(bucket->extend);

			(void)memcpy(window->mask_temp->pixels, buff,
				window->mask_temp->height * window->mask_temp->width);
			for(i=0; i<end; i++)
			{
				ReductSelectionAreaOneStep(window->mask_temp, window->temp_layer);
				(void)memcpy(window->mask_temp->pixels, window->temp_layer->pixels,
					window->mask_temp->width*window->mask_temp->height);
			}

			for(i=0; i<window->mask_temp->width*window->mask_temp->height; i++)
			{
				window->mask_temp->pixels[i*4] = buff[i];
				window->mask_temp->pixels[i*4+1] = buff[i];
				window->mask_temp->pixels[i*4+2] = buff[i];
				window->mask_temp->pixels[i*4+3] = buff[i];
			}
		}

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		if(window->app->textures.active_texture == 0)
		{
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				cairo_set_source_rgb(window->work_layer->cairo_p, (*core->color)[0]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL, (*core->color)[2]*DIV_PIXEL);
				cairo_rectangle(window->work_layer->cairo_p, 0, 0,
					window->work_layer->width, window->work_layer->height);
				cairo_mask_surface(window->work_layer->cairo_p, window->mask_temp->surface_p, 0, 0);
			}
			else
			{
				cairo_set_source_rgb(window->temp_layer->cairo_p, (*core->color)[0]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL, (*core->color)[2]*DIV_PIXEL);
				(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
				cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
				cairo_rectangle(window->temp_layer->cairo_p, 0, 0, window->width, window->height);
				cairo_mask_surface(window->temp_layer->cairo_p, window->mask_temp->surface_p, 0, 0);
				cairo_set_source_surface(window->work_layer->cairo_p, window->temp_layer->surface_p, 0, 0);
				cairo_mask_surface(window->work_layer->cairo_p, window->selection->surface_p, 0, 0);
			}
		}
		else
		{
			(void)memset(window->mask->pixels, 0, window->pixel_buf_size);
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				cairo_set_source_rgb(window->mask->cairo_p, (*core->color)[0]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL, (*core->color)[2]*DIV_PIXEL);
				cairo_rectangle(window->mask->cairo_p, 0, 0,
					window->mask->width, window->work_layer->height);
				cairo_mask_surface(window->mask->cairo_p, window->mask_temp->surface_p, 0, 0);
				cairo_set_source_surface(window->work_layer->cairo_p, window->mask->surface_p, 0, 0);
				cairo_mask_surface(window->work_layer->cairo_p, window->texture->surface_p, 0, 0);
			}
			else
			{
				cairo_set_source_rgb(window->temp_layer->cairo_p, (*core->color)[0]*DIV_PIXEL, (*core->color)[1]*DIV_PIXEL, (*core->color)[2]*DIV_PIXEL);
				(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
				cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
				cairo_rectangle(window->temp_layer->cairo_p, 0, 0, window->width, window->height);
				cairo_mask_surface(window->temp_layer->cairo_p, window->mask_temp->surface_p, 0, 0);
				cairo_set_source_surface(window->mask->cairo_p, window->temp_layer->surface_p, 0, 0);
				cairo_mask_surface(window->mask->cairo_p, window->selection->surface_p, 0, 0);
				cairo_set_source_surface(window->work_layer->cairo_p, window->mask->surface_p, 0, 0);
				cairo_mask_surface(window->work_layer->cairo_p, window->texture->surface_p, 0, 0);
			}
		}

		AddBrushHistory(core, window->active_layer);

		g_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, window->active_layer);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		if(bucket->target == BUCKET_TARGET_CANVAS)
		{
			DeleteLayer(&target);
		}

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

static void BucketEditSelectionPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		BUCKET* bucket = (BUCKET*)core->brush_data;
		LAYER* target = window->mask;
		uint8* buff = &window->temp_layer->pixels[window->width*window->height];
		int32 min_x, min_y, max_x, max_y;
		int32 before_stride = target->stride;
		uint8 before_channel = target->channel;
		uint8 channel = 1;
		int i;

		window->selection->layer_mode = SELECTION_BLEND_NORMAL;

		(void)memcpy(window->mask->pixels, window->selection->pixels, window->width * window->height);
		(void)memset(window->mask_temp->pixels, 0, window->pixel_buf_size);
		(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);

		target->channel = 1;
		target->stride = target->width;
		DetectSameColorArea(
			target,
			buff,
			&window->temp_layer->pixels[window->width*window->height*2],
			(int32)x, (int32)y,
			&target->pixels[(int32)y*target->stride + (int32)x*target->channel],
			channel, bucket->threshold, &min_x, &min_y, &max_x, &max_y,
			(eSELECT_FUZZY_DIRECTION)bucket->select_direction
		);
		target->stride = before_stride;
		target->channel = before_channel;

		if((bucket->flags & BUCKET_FLAG_ANTI_ALIAS) != 0)
		{
			(void)memcpy(&window->temp_layer->pixels[window->width*window->height*2],
				buff, window->active_layer->width*window->active_layer->height);
			AntiAlias(&window->temp_layer->pixels[window->width*window->height*2],
				buff, window->active_layer->width, window->active_layer->height, window->active_layer->width, 1);
		}

		core->min_x = min_x - 1, core->min_y = min_y - 1;
		core->max_x = max_x + 1, core->max_y = max_y + 1;

		for(i=0; i<window->mask_temp->width*window->mask->height; i++)
		{
			window->mask_temp->pixels[i*4] = buff[i];
			window->mask_temp->pixels[i*4+1] = buff[i];
			window->mask_temp->pixels[i*4+2] = buff[i];
			window->mask_temp->pixels[i*4+3] = buff[i];
		}

		if(bucket->extend > 0)
		{
			(void)memcpy(window->mask_temp->pixels, buff,
				window->mask_temp->height * window->mask_temp->width);
			for(i=0; i<bucket->extend; i++)
			{
				ExtendSelectionAreaOneStep(window->mask_temp, window->temp_layer);
				(void)memcpy(window->mask_temp->pixels, window->temp_layer->pixels,
					window->mask_temp->width*window->mask_temp->height);
				core->min_x -= 1, core->min_y -= 1;
				core->max_x += 1, core->max_y += 1;
			}

			for(i=0; i<window->mask_temp->width*window->mask->height; i++)
			{
				window->mask_temp->pixels[i*4] = buff[i];
				window->mask_temp->pixels[i*4+1] = buff[i];
				window->mask_temp->pixels[i*4+2] = buff[i];
				window->mask_temp->pixels[i*4+3] = buff[i];
			}
		}
		else
		{
			int end = abs(bucket->extend);

			(void)memcpy(window->mask_temp->pixels, buff,
				window->mask_temp->height * window->mask_temp->width);
			for(i=0; i<end; i++)
			{
				ReductSelectionAreaOneStep(window->mask_temp, window->temp_layer);
				(void)memcpy(window->mask_temp->pixels, window->temp_layer->pixels,
					window->mask_temp->width*window->mask_temp->height);
			}

			for(i=0; i<window->mask_temp->width*window->mask->height; i++)
			{
				window->mask_temp->pixels[i*4] = buff[i];
				window->mask_temp->pixels[i*4+1] = buff[i];
				window->mask_temp->pixels[i*4+2] = buff[i];
				window->mask_temp->pixels[i*4+3] = buff[i];
			}
		}

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgb(window->work_layer->cairo_p, 0, 0, 0);
		cairo_rectangle(window->work_layer->cairo_p, 0, 0,
			window->work_layer->width, window->work_layer->height);
		cairo_mask_surface(window->work_layer->cairo_p, window->mask_temp->surface_p, 0, 0);

		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[SELECTION_BLEND_NORMAL](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

#define BucketMotionCallBack DummyBrushCallBack
#define BucketEditSelectionMotionCallBack DummyBrushCallBack
#define BucketReleaseCallBack DummyBrushCallBack
#define BucketEditSelectionReleaseCallBack DummyBrushCallBack
#define BucketDrawCursor DummyBrushDrawCursor

static void BucketSetModeRGB(GtkWidget* widget, gpointer data)
{
	((BUCKET*)data)->mode = BUCKET_RGB;
}

static void BucketSetModeRGBA(GtkWidget* widget, gpointer data)
{
	((BUCKET*)data)->mode = BUCKET_RGBA;
}

static void BucketSetModeAlpha(GtkWidget* widget, gpointer data)
{
	((BUCKET*)data)->mode = BUCKET_ALPHA;
}

static void BucketChangeThreshold(GtkWidget* widget, gpointer data)
{
	BUCKET* bucket = (BUCKET*)data;
	bucket->threshold = (uint16)gtk_adjustment_get_value(GTK_ADJUSTMENT(widget));
	gtk_adjustment_set_value(GTK_ADJUSTMENT(widget), bucket->threshold);
}

static void BucketSetTargetActiveLayer(GtkWidget* widget, gpointer data)
{
	((BUCKET*)data)->target = BUCKET_TARGET_ACTIVE_LAYER;
}

static void BucketSetTargetCanvas(GtkWidget* widget, gpointer data)
{
	((BUCKET*)data)->target = BUCKET_TARGET_CANVAS;
}

static void BucketSetSelectDirection(GtkWidget* widget, gpointer data)
{
	((BUCKET*)data)->select_direction =
		(uint8)g_object_get_data(G_OBJECT(widget), "direction");
}

static void BucketChangeAntiAlias(GtkToggleButton* button, BUCKET* bucket)
{
	if(gtk_toggle_button_get_active(button) == FALSE)
	{
		bucket->flags &= ~(BUCKET_FLAG_ANTI_ALIAS);
	}
	else
	{
		bucket->flags |= BUCKET_FLAG_ANTI_ALIAS;
	}
}

static void BucketChangeExtend(GtkAdjustment* spin, BUCKET* bucket)
{
	bucket->extend = (int16)gtk_adjustment_get_value(spin);
}

static GtkWidget* CreateBucketDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	BUCKET* bucket = (BUCKET*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget* label;
	GtkWidget* table;
	GtkWidget* buttons[3];
	GtkWidget* threshold_scale;
	GtkAdjustment* threshold_adjustment;
	gchar mark_up_buff[256];

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.select.target);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.rgb);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.rgba);
	buttons[2] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.alpha);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[bucket->mode]), TRUE);
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(BucketSetModeRGB), core->brush_data);
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(BucketSetModeRGBA), core->brush_data);
	g_signal_connect(G_OBJECT(buttons[2]), "toggled", G_CALLBACK(BucketSetModeAlpha), core->brush_data);

	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[2], FALSE, TRUE, 0);

	threshold_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		bucket->threshold, 0, 255, 1, 1, 0));
	g_signal_connect(G_OBJECT(threshold_adjustment), "value_changed",
		G_CALLBACK(BucketChangeThreshold), core->brush_data);
	threshold_scale = SpinScaleNew(threshold_adjustment,
		app->labels->tool_box.select.threshold, 0);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), threshold_scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.select.area);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.active_layer);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.canvas);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[bucket->target]), TRUE);
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(BucketSetTargetActiveLayer), core->brush_data);
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(BucketSetTargetCanvas), core->brush_data);

	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);

	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.select.detect_area);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.area_normal);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.area_large);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[bucket->select_direction]), TRUE);
	g_object_set_data(G_OBJECT(buttons[0]), "direction", GINT_TO_POINTER(FUZZY_SELECT_DIRECTION_QUAD));
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(BucketSetSelectDirection), core->brush_data);
	g_object_set_data(G_OBJECT(buttons[1]), "direction", GINT_TO_POINTER(FUZZY_SELECT_DIRECTION_OCT));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(BucketSetSelectDirection), core->brush_data);

	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);

	buttons[0] = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[0]), bucket->flags & BUCKET_FLAG_ANTI_ALIAS);
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(BucketChangeAntiAlias), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 3);

	threshold_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(bucket->extend, -50, 50, 1, 1, 0));
	threshold_scale = SpinScaleNew(threshold_adjustment, app->labels->tool_box.extend, 0);
	g_signal_connect(G_OBJECT(threshold_adjustment), "value_changed",
		G_CALLBACK(BucketChangeExtend), bucket);
	gtk_box_pack_start(GTK_BOX(vbox), threshold_scale, FALSE, FALSE, 2);

	return vbox;
#undef UI_FONT_SIZE
}

/*********************************************************************
* PaternFillPressCallBack�֐�                                        *
* �p�^�[���h��ׂ��c�[���Ń}�E�X�N���b�N���ꂽ�Ƃ��̃R�[���o�b�N�֐� *
* ����                                                               *
* window	: �`��̈�̏��                                         *
* x			: �}�E�X��X���W                                          *
* y			: �}�E�X��Y���W                                          *
* pressure	: �M��                                                   *
* core		: �c�[���̊�{���                                       *
* state		: �}�E�X�̏��                                           *
*********************************************************************/
static void PatternFillPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// �`��̈�O�̃N���b�N�Ȃ�I��
	if(x < 0 || x > window->width || y < 0 || y > window->height)
	{
		return;
	}

	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �p�^�[���c�[���̏ڍ׃f�[�^�ɃL���X�g
		PATTERN_FILL* fill = (PATTERN_FILL*)core->brush_data;
		// �h��ׂ����C���[
		LAYER* target;
		// �h��ׂ��͈͂̃o�b�t�@
		uint8* buff = &window->temp_layer->pixels[window->width*window->height];
		// �h��ׂ������W�̍ŏ��ő�l
		int32 min_x, min_y, max_x, max_y;
		// �h��ׂ��͈͂����肷�邽�߂̃`�����l����
		uint8 channel = (fill->mode == PATTERN_FILL_RGB) ? 3 : 4;
		// �h��ׂ��Ɏg�p����p�^�[���̃T�[�t�F�[�X
		cairo_surface_t* pattern_surface = CreatePatternSurface(
			&window->app->patterns, *core->color, *core->back_color, fill->pattern_flags,
			fill->pattern_mode, fill->flow);
		// �h��ׂ��p�^�[��
		cairo_pattern_t* pattern;
		// �g��k����
		gdouble zoom = 1 / (fill->scale * 0.01);
		// �p�^�[���Ɋg��k�����Z�b�g���邽�߂̍s��
		cairo_matrix_t matrix;
		// for���p�̃J�E���^
		int i;

		// �p�^�[���T�[�t�F�[�X�쐬�Ɏ��s������I��
		if(pattern_surface == NULL)
		{
			return;
		}

		// �I��͈͂�����Ƃ��̓N���b�N���ꂽ�ʒu���I��͈͓��ł��邱�Ƃ��m�F
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			if(window->selection->pixels[(int)y*window->selection->width+(int)x] < 0x80)
			{
				return;
			}
		}

		if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
		{
			window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		}
		else
		{
			window->work_layer->layer_mode = LAYER_BLEND_ATOP;
		}

		// �h��ׂ��^�[�Q�b�g������
		switch(fill->target)
		{
		case PATTERN_FILL_TARGET_ACTIVE_LAYER:
			target = window->active_layer;
			break;
		case PATTERN_FILL_TARGET_CANVAS:
			target = MixLayerForSave(window);
			break;
		default:
			target = NULL;
		}

		// �h��ׂ��͈͂�����
		(void)memset(window->mask_temp->pixels, 0, window->pixel_buf_size);
		(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);

		if(fill->mode != PATTERN_FILL_ALPHA)
		{	// �s�����x�ȊO�ł̓h��Ԃ��͈͌���Ȃ��
			DetectSameColorArea(
				target,
				buff,
				&window->temp_layer->pixels[window->width*window->height*2],
				(int32)x, (int32)y,
				&target->pixels[(int32)y*target->stride + (int32)x*target->channel],
				channel, fill->threshold, &min_x, &min_y, &max_x, &max_y,
				(eSELECT_FUZZY_DIRECTION)fill->area_detect_direction
			);
		}
		else
		{	// ����ȊO�͈�x�s�����x���݂̂̃��C���[���쐬����
			LAYER *local_target = CreateLayer(0, 0, target->width, target->height, 1,
				TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);

			for(i=0; i<target->width*target->height; i++)
			{
				local_target->pixels[i] = target->pixels[i*4+3];
			}

			DetectSameColorArea(
				local_target,
				buff,
				&window->temp_layer->pixels[window->width*window->height*2],
				(int32)x, (int32)y,
				&target->pixels[(int32)y*target->stride + (int32)x*target->channel],
				channel, fill->threshold, &min_x, &min_y, &max_x, &max_y,
				(eSELECT_FUZZY_DIRECTION)fill->area_detect_direction
			);

			DeleteLayer(&local_target);
		}

		if((fill->flags & PATTERN_FILL_FLAG_ANTI_ALIAS) != 0)
		{
			(void)memcpy(&window->temp_layer->pixels[window->width*window->height*2],
				buff, window->width*window->height);
			AntiAlias(&window->temp_layer->pixels[window->width*window->height*2],
				buff, window->width, window->height, window->width, 1);
		}

		core->min_x = min_x - 1, core->min_y = min_y - 1;
		core->max_x = max_x + 1, core->max_y = max_y + 1;

		for(i=0; i<window->mask_temp->width*window->mask->height; i++)
		{
			window->mask_temp->pixels[i*4] = buff[i];
			window->mask_temp->pixels[i*4+1] = buff[i];
			window->mask_temp->pixels[i*4+2] = buff[i];
			window->mask_temp->pixels[i*4+3] = buff[i];
		}

		if(fill->extend > 0)
		{
			(void)memcpy(window->mask_temp->pixels, buff,
				window->mask_temp->height * window->mask_temp->width);

			for(i=0; i<fill->extend; i++)
			{
				ExtendSelectionAreaOneStep(window->mask_temp, window->temp_layer);
				(void)memcpy(window->mask_temp->pixels, window->temp_layer->pixels,
					window->mask_temp->width*window->mask_temp->height);
				core->min_x -= 1, core->min_y -= 1;
				core->max_x += 1, core->max_y += 1;
			}

			for(i=0; i<window->mask_temp->width*window->mask->height; i++)
			{
				window->mask_temp->pixels[i*4] = buff[i];
				window->mask_temp->pixels[i*4+1] = buff[i];
				window->mask_temp->pixels[i*4+2] = buff[i];
				window->mask_temp->pixels[i*4+3] = buff[i];
			}
		}
		else
		{
			int end = abs(fill->extend);

			(void)memcpy(window->mask_temp->pixels, buff,
				window->mask_temp->height * window->mask_temp->width);

			for(i=0; i<end; i++)
			{
				ReductSelectionAreaOneStep(window->mask_temp, window->temp_layer);
				(void)memcpy(window->mask_temp->pixels, window->temp_layer->pixels,
					window->mask_temp->width*window->mask_temp->height);
			}

			for(i=0; i<window->mask_temp->width*window->mask->height; i++)
			{
				window->mask_temp->pixels[i*4] = buff[i];
				window->mask_temp->pixels[i*4+1] = buff[i];
				window->mask_temp->pixels[i*4+2] = buff[i];
				window->mask_temp->pixels[i*4+3] = buff[i];
			}
		}

		// �h��ׂ����s
		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		pattern = cairo_pattern_create_for_surface(pattern_surface);
		cairo_matrix_init_scale(&matrix, zoom, zoom);
		cairo_pattern_set_matrix(pattern, &matrix);
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{	// �I��͈͂Ȃ�
			cairo_set_source(window->work_layer->cairo_p, pattern);
			cairo_rectangle(window->work_layer->cairo_p, 0, 0,
				window->work_layer->width, window->work_layer->height);
			cairo_mask_surface(window->work_layer->cairo_p, window->mask_temp->surface_p, 0, 0);
		}
		else
		{	// �I��͈͂���
			cairo_set_source(window->temp_layer->cairo_p, pattern);
			(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
			cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
			cairo_rectangle(window->temp_layer->cairo_p, 0, 0, window->width, window->height);
			cairo_mask_surface(window->temp_layer->cairo_p, window->mask_temp->surface_p, 0, 0);
			cairo_set_source_surface(window->work_layer->cairo_p, window->temp_layer->surface_p, 0, 0);
			cairo_mask_surface(window->work_layer->cairo_p, window->selection->surface_p, 0, 0);
		}

		cairo_pattern_destroy(pattern);
		cairo_surface_destroy(pattern_surface);

		AddBrushHistory(core, window->active_layer);

		g_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, window->active_layer);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		if(fill->target == PATTERN_FILL_TARGET_CANVAS)
		{
			DeleteLayer(&target);
		}

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

/*************************************************************************************
* PaternFillEditSelectionPressCallBack�֐�                                           *
* �I��͈͕ҏW���Ƀp�^�[���h��ׂ��c�[���Ń}�E�X�N���b�N���ꂽ�Ƃ��̃R�[���o�b�N�֐� *
* ����                                                                               *
* window	: �`��̈�̏��                                                         *
* x			: �}�E�X��X���W                                                          *
* y			: �}�E�X��Y���W                                                          *
* pressure	: �M��                                                                   *
* core		: �c�[���̊�{���                                                       *
* state		: �}�E�X�̏��                                                           *
*************************************************************************************/
static void PatternFillEditSelectionPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		// �p�^�[���c�[���̏ڍ׃f�[�^�ɃL���X�g
		PATTERN_FILL* fill = (PATTERN_FILL*)core->brush_data;
		// �h��ׂ����C���[
		LAYER* target = window->mask;
		// �h��ׂ��͈͂̃o�b�t�@
		uint8* buff = &window->temp_layer->pixels[window->width*window->height];
		// �h��ׂ������W�̍ŏ��ő�l
		int32 min_x, min_y, max_x, max_y;
		// �h��ׂ��͈͂����肷�邽�߂̃`�����l����
		uint8 channel = 1;
		// �h��ׂ��Ɏg�p����p�^�[���̃T�[�t�F�[�X
		cairo_surface_t* pattern_surface = CreatePatternSurface(
			&window->app->patterns, *core->color, *core->back_color, fill->pattern_flags,
			fill->pattern_mode, fill->flow);
		// �h��ׂ��p�^�[��
		cairo_pattern_t* pattern;
		// �g��k����
		gdouble zoom = 1 / (fill->scale * 0.01);
		// �p�^�[���Ɋg��k�����Z�b�g���邽�߂̍s��
		cairo_matrix_t matrix;
		// ���݂̃}�X�N���C���[�̃`�����l����1�s���̃o�C�g�����L��
		uint8 before_channel = target->channel;
		int32 before_stride = target->stride;
		// for���p�̃J�E���^
		int i;

		// �p�^�[���T�[�t�F�[�X�쐬�Ɏ��s������I��
		if(pattern_surface == NULL)
		{
			return;
		}

		// �h��ׂ��͈͂�����
		(void)memcpy(window->mask->pixels, window->selection->pixels, window->width*window->height);
		(void)memset(window->mask_temp->pixels, 0, window->pixel_buf_size);
		(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);

		target->channel = 1;
		target->stride = target->width;
		DetectSameColorArea(
			target,
			buff,
			&window->temp_layer->pixels[window->width*window->height*2],
			(int32)x, (int32)y,
			&target->pixels[(int32)y*target->stride + (int32)x*target->channel],
			channel, fill->threshold, &min_x, &min_y, &max_x, &max_y,
			(eSELECT_FUZZY_DIRECTION)fill->area_detect_direction
		);
		target->channel = before_channel;
		target->stride = before_stride;

		if((fill->flags & PATTERN_FILL_FLAG_ANTI_ALIAS) != 0)
		{
			(void)memcpy(&window->temp_layer->pixels[window->width*window->height*2],
				buff, window->width*window->height);
			AntiAlias(&window->temp_layer->pixels[window->width*window->height*2],
				buff, window->width, window->height, window->width, 1);
		}

		core->min_x = min_x - 1, core->min_y = min_y - 1;
		core->max_x = max_x + 1, core->max_y = max_y + 1;

		for(i=0; i<window->mask_temp->width*window->mask->height; i++)
		{
			window->mask_temp->pixels[i*4] = buff[i];
			window->mask_temp->pixels[i*4+1] = buff[i];
			window->mask_temp->pixels[i*4+2] = buff[i];
			window->mask_temp->pixels[i*4+3] = buff[i];
		}

		if(fill->extend > 0)
		{
			for(i=0; i<fill->extend; i++)
			{
				ExtendSelectionAreaOneStep(window->mask_temp, window->temp_layer);
				(void)memcpy(window->mask_temp->pixels, window->temp_layer->pixels,
					window->mask_temp->width*window->mask_temp->height);
				core->min_x -= 1, core->min_y -= 1;
				core->max_x += 1, core->max_y += 1;
			}
		}
		else
		{
			int end = abs(fill->extend);

			for(i=0; i<end; i++)
			{
				ReductSelectionAreaOneStep(window->mask_temp, window->temp_layer);
				(void)memcpy(window->mask_temp->pixels, window->temp_layer->pixels,
					window->mask_temp->width*window->mask_temp->height);
			}
		}

		// �h��ׂ����s
		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		pattern = cairo_pattern_create_for_surface(pattern_surface);
		cairo_matrix_init_scale(&matrix, zoom, zoom);
		cairo_pattern_set_matrix(pattern, &matrix);
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

		cairo_set_source(window->work_layer->cairo_p, pattern);
		cairo_rectangle(window->work_layer->cairo_p, 0, 0,
			window->work_layer->width, window->work_layer->height);
		cairo_mask_surface(window->work_layer->cairo_p, window->mask_temp->surface_p, 0, 0);

		cairo_pattern_destroy(pattern);
		cairo_surface_destroy(pattern_surface);

		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[SELECTION_BLEND_NORMAL](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

#define PatternFillMotionCallBack DummyBrushCallBack
#define PatternFillEditSelectionMotionCallBack DummyBrushCallBack
#define PatternFillReleaseCallBack DummyBrushCallBack
#define PatternFillEditSelectionReleaseCallBack DummyBrushCallBack
#define PatternFillDrawCursor DummyBrushDrawCursor

static void PatternFillSetMode(GtkWidget* widget, PATTERN_FILL* fill)
{
	fill->mode = (uint8)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "fill-mode"));
}

static void PatternFillChangeThreshold(GtkWidget* widget, PATTERN_FILL* fill)
{
	fill->threshold = (uint16)gtk_adjustment_get_value(GTK_ADJUSTMENT(widget));
	gtk_adjustment_set_value(GTK_ADJUSTMENT(widget), fill->threshold);
}

static void PatternFillSetTarget(GtkWidget* widget, PATTERN_FILL* fill)
{
	fill->target = (uint8)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "fill-target"));
}

static void PatternFillChangeScale(GtkAdjustment* slider, PATTERN_FILL* fill)
{
	fill->scale = gtk_adjustment_get_value(slider);
	fill->app->patterns.scale = fill->scale;
}

static void PatternFillChangeFlow(GtkAdjustment* slider, PATTERN_FILL* fill)
{
	fill->flow = gtk_adjustment_get_value(slider) * 0.01f;
}

static void PatternFillSetPatternFlag(GtkWidget* widget, PATTERN_FILL* fill)
{
	int flag_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "flag-id"));

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		fill->pattern_flags &= ~(1 << flag_id);
	}
	else
	{
		fill->pattern_flags |= (1 << flag_id);
	}
}

static void PatternFillSetBlendMode(GtkWidget* widget, PATTERN_FILL* fill)
{
	fill->pattern_mode = (uint8)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "blend-mode"));
}

static void PatternFillChangeAntiAlias(GtkToggleButton* button, PATTERN_FILL* fill)
{
	if(gtk_toggle_button_get_active(button) == FALSE)
	{
		fill->flags &= ~(PATTERN_FILL_FLAG_ANTI_ALIAS);
	}
	else
	{
		fill->flags |= PATTERN_FILL_FLAG_ANTI_ALIAS;
	}
}

/***************************************************************
* PatternSelectButtonClicked�֐�                               *
* �p�^�[���I��p�̃{�^�����N���b�N���ꂽ�Ƃ��̃R�[���o�b�N�֐� *
* ����                                                         *
* button	: �{�^���E�B�W�F�b�g                               *
* fill		: �p�^�[���h��ׂ��̏ڍ׃f�[�^                     *
***************************************************************/
static void PatternSelectButtonClicked(GtkWidget* button, PATTERN_FILL* fill)
{
	// �p�^�[��ID���E�B�W�F�b�g�ɓo�^���ꂽ�f�[�^����擾
	int pattern_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "pattern-id"));
	// �{�^��ID�����p
	int button_add = (fill->app->patterns.has_clip_board_pattern == FALSE) ? 0 : -1;
	// for���p�̃J�E���^
	int i;

	// �g�p�p�^�[���Ɖ����ꂽ�{�^������v���Ă�����{�^�����A�N�e�B�u�ɂ��ďI��
	if(pattern_id == fill->pattern_id)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		}
		return;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{	// �g�p�p�^�[���ƕs��v�Ń{�^������A�N�e�B�u�Ȃ�I��
		return;
	}

	// �g�p�p�^�[����ݒ�
	fill->pattern_id = pattern_id;
	if(fill->app->patterns.has_clip_board_pattern != FALSE && fill->pattern_id == 0)
	{
		fill->app->patterns.active_pattern =
			&fill->app->patterns.clip_board;
	}
	else
	{
		fill->app->patterns.active_pattern =
			&fill->app->patterns.patterns[pattern_id + button_add];
	}

	// �A�N�e�B�u�ȃp�^�[�����`�����l����2�ȊO�Ȃ�΍������[�h�͖���
	gtk_widget_set_sensitive(fill->mode_select[0], fill->app->patterns.active_pattern->channel == 2);
	gtk_widget_set_sensitive(fill->mode_select[1], fill->app->patterns.active_pattern->channel == 2);

	// ���̃{�^�����A�N�e�B�u�ɂ���
	for(i=0; i<fill->num_button; i++)
	{
		if(i == fill->pattern_id)
		{
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fill->buttons[i])) == FALSE)
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill->buttons[i]), TRUE);
			}
		}
		else
		{
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fill->buttons[i])) != FALSE)
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill->buttons[i]), FALSE);
			}
		}
	}
}

// �p�^�[���I��p�e�[�u���̕�
#define PATTERN_SELECT_TABLE_WIDTH 4

/*********************************************
* CreatePatternSelectTable�֐�               *
* �p�^�[���I��p�̃{�^���e�[�u�����쐬����   *
* ����                                       *
* fill	: �p�^�[���h��ׂ��c�[���̏ڍ׃f�[�^ *
* �Ԃ�l                                     *
*	�쐬�����{�^���e�[�u���̃E�B�W�F�b�g     *
*********************************************/
static GtkWidget* CreatePatternSelectTable(PATTERN_FILL* fill)
{
// �{�^���ɕ\������A�C�R���̃T�C�Y
#define ICON_SIZE 32
	// �Ԃ�l
	GtkWidget* table;
	// �{�^���ɓo�^����C���[�W�E�B�W�F�b�g
	GtkWidget* image;
	// �e�[�u���̍���
	int height;
	// �{�^���̃C���[�W�쐬�p�s�N�Z���o�b�t�@
	GdkPixbuf* pixbuf;
	// �`�����l������1�̂Ƃ��Ɏg�p����s�N�Z���f�[�^
	uint8* pixels;
	// �C���[�W�̊g�嗦
	gdouble zoom;
	// �{�^��ID�����p
	int button_add = 0;
	// �e�[�u���̍��W
	int x = 0, y = 0;
	// for���p�̃J�E���^
	int i = 0, j;

	// �{�^���z��쐬
	fill->buttons = (GtkWidget**)MEM_ALLOC_FUNC(
		sizeof(*fill->buttons)*(fill->app->patterns.num_pattern+1));
	fill->num_button = fill->app->patterns.num_pattern;

	// �e�[�u���̍��������肵�č쐬
	height = fill->num_button / PATTERN_SELECT_TABLE_WIDTH;
	if(fill->num_button % PATTERN_SELECT_TABLE_WIDTH != 0)
	{
		height++;
	}
	table = gtk_table_new(height, PATTERN_SELECT_TABLE_WIDTH, TRUE);

	// �N���b�v�{�[�h�̃p�^�[��������Ȃ��
	if(fill->app->patterns.has_clip_board_pattern != FALSE)
	{
		PATTERN* pattern = &fill->app->patterns.clip_board;

		pixbuf = gdk_pixbuf_new_from_data(pattern->pixels,
			GDK_COLORSPACE_RGB, TRUE, 8, pattern->width, pattern->height,
			pattern->stride, NULL, NULL
		);

		// �g�嗦���v�Z
		if(pattern->width > pattern->height)
		{
			zoom = ICON_SIZE / (gdouble)pattern->width;
		}
		else
		{
			zoom = ICON_SIZE / (gdouble)pattern->height;
		}

		// �s�N�Z���o�b�t�@���g��k�����ăC���[�W�E�B�W�F�b�g�쐬
		image = gtk_image_new_from_pixbuf(
			gdk_pixbuf_scale_simple(pixbuf, (int)(pattern->width*zoom),
			(int)(pattern->height*zoom), GDK_INTERP_BILINEAR)
		);

		// �{�^���쐬
		fill->buttons[i] = gtk_toggle_button_new();
		// ���݂̎g�p�p�^�[���ƈ�v���Ă�����A�N�e�B�u��
		if(i == fill->pattern_id + button_add)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill->buttons[i]), TRUE);
		}

		// �R�[���o�b�N�֐��ƃf�[�^���Z�b�g
		g_signal_connect(G_OBJECT(fill->buttons[i]), "toggled", G_CALLBACK(PatternSelectButtonClicked), fill);
		g_object_set_data(G_OBJECT(fill->buttons[i]), "pattern-id", GINT_TO_POINTER(i));

		// �e�[�u���Ƀ{�^���ǉ�
		gtk_table_attach(GTK_TABLE(table), fill->buttons[i], x, x+1, y, y+1, GTK_SHRINK, GTK_SHRINK, 0, 0);

		// �C���[�W�E�B�W�F�b�g���{�^���ɓo�^
		gtk_container_add(GTK_CONTAINER(fill->buttons[i]), image);

		g_object_unref(pixbuf);
		fill->num_button++;
		button_add = -1;
		i++, x++;
	}

	// �{�^�����쐬���ăe�[�u���ɓ����
	for( ; i<fill->num_button; i++, x++)
	{
		// �{�^���ɓ����p�^�[��
		PATTERN* pattern = &fill->app->patterns.patterns[i+button_add];

		// ��s�����܂����玟�̍s��
		if(x == PATTERN_SELECT_TABLE_WIDTH)
		{
			x = 0;
			y++;
		}

		pixels = NULL;
		// �O���[�X�P�[���Ȃ�RGB�ɂ���
		if(pattern->channel == 1)
		{
			uint8 pixel_value;
			pixels = (uint8*)MEM_ALLOC_FUNC(pattern->width*pattern->height*3);
			for(j=0; j<pattern->width*pattern->height; j++)
			{
				pixel_value = 0xff - pattern->pixels[j];
				pixels[j*3] = pixel_value;
				pixels[j*3+1] = pixel_value;
				pixels[j*3+2] = pixel_value;
			}
			pixbuf = gdk_pixbuf_new_from_data(
				pixels, GDK_COLORSPACE_RGB, FALSE, 8,
				pattern->width, pattern->height, pattern->width*3, NULL, NULL
			);
		}
		else if(pattern->channel == 2)
		{
			uint8 pixel_value;
			pixels = (uint8*)MEM_ALLOC_FUNC(pattern->width*pattern->height*4);
			for(j=0; j<pattern->width*pattern->height; j++)
			{
				pixel_value = pattern->pixels[j*2];
				pixels[j*4] = pixel_value;
				pixels[j*4+1] = pixel_value;
				pixels[j*4+2] = pixel_value;
				pixels[j*4+3] = pattern->pixels[j*2+1];
			}
			pixbuf = gdk_pixbuf_new_from_data(
				pixels, GDK_COLORSPACE_RGB, TRUE, 8,
				pattern->width, pattern->height, pattern->width*4, NULL, NULL
			);
		}
		else
		{
			// �p�^�[���̃s�N�Z���f�[�^����s�N�Z���o�b�t�@�쐬
			pixbuf = gdk_pixbuf_new_from_data(pattern->pixels, GDK_COLORSPACE_RGB,
				pattern->channel == 4, 8, pattern->width, pattern->height, pattern->width*pattern->channel,
				NULL, NULL
			);
		}

		// �g�嗦���v�Z
		if(pattern->width > pattern->height)
		{
			zoom = ICON_SIZE / (gdouble)pattern->width;
		}
		else
		{
			zoom = ICON_SIZE / (gdouble)pattern->height;
		}

		// �s�N�Z���o�b�t�@���g��k�����ăC���[�W�E�B�W�F�b�g�쐬
		image = gtk_image_new_from_pixbuf(
			gdk_pixbuf_scale_simple(pixbuf, (int)(pattern->width*zoom),
			(int)(pattern->height*zoom), GDK_INTERP_BILINEAR)
		);

		// �{�^���쐬
		fill->buttons[i] = gtk_toggle_button_new();
		// ���݂̎g�p�p�^�[���ƈ�v���Ă�����A�N�e�B�u��
		if(i == fill->pattern_id + button_add)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill->buttons[i]), TRUE);
		}

		// �R�[���o�b�N�֐��ƃf�[�^���Z�b�g
		g_signal_connect(G_OBJECT(fill->buttons[i]), "toggled", G_CALLBACK(PatternSelectButtonClicked), fill);
		g_object_set_data(G_OBJECT(fill->buttons[i]), "pattern-id", GINT_TO_POINTER(i));

		// �e�[�u���Ƀ{�^���ǉ�
		gtk_table_attach(GTK_TABLE(table), fill->buttons[i], x, x+1, y, y+1, GTK_SHRINK, GTK_SHRINK, 0, 0);

		// �C���[�W�E�B�W�F�b�g���{�^���ɓo�^
		gtk_container_add(GTK_CONTAINER(fill->buttons[i]), image);

		g_object_unref(pixbuf);
		MEM_FREE_FUNC(pixels);
	}

	return table;
}

static void OnPatternFillDetailUIDestroy(GtkWidget* widget, PATTERN_FILL* fill)
{
	MEM_FREE_FUNC(fill->buttons);
}

static void PatternFillSetDetectDirection(GtkWidget* widget, PATTERN_FILL* fill)
{
	fill->area_detect_direction = (uint8)g_object_get_data(G_OBJECT(widget), "direction");
}

static void PatternFillSetExtend(GtkAdjustment* slider, PATTERN_FILL* fill)
{
	fill->extend = (int16)gtk_adjustment_get_value(slider);
}

/*****************************************************
* CreatePatternFillDetailUI�֐�                      *
* �p�^�[���h��ׂ��̏ڍאݒ�E�B�W�F�b�g���쐬       *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* data	: �c�[���̏ڍ׃f�[�^                         *
* �Ԃ�l                                             *
*	�쐬�����E�B�W�F�b�g                             *
*****************************************************/
static GtkWidget* CreatePatternFillDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	// �p�^�[���h��ׂ��̏ڍ׃f�[�^�ɃL���X�g
	PATTERN_FILL* fill = (PATTERN_FILL*)core->brush_data;
	// �E�B�W�F�b�g�𐮗񂷂邽�߂̃p�b�L���O�{�b�N�X�A�e�[�u��
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* table;
	// �p�^�[���I��p�̃e�[�u���X�N���[���p
	GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	// �h��ׂ����[�h�A�^�[�Q�b�g�����肷�邽�߂̃��W�I�{�^��
	GtkWidget* buttons[3], *label;
	// �h��ׂ��͈͌���p��臒l�A�g�嗦�ݒ�̂��߂̃X���C�_
	GtkWidget* scale;
	// �X���C�_�p�̃A�W���X�^
	GtkAdjustment* adjustment;
	// ���E���]�A�㉺���]�p�̃`�F�b�N�{�b�N�X
	GtkWidget* check_button;
	// ���x�������T�C�Y����p�̃}�[�N�A�b�v�o�b�t�@
	char mark_up_buff[256];

	// �A�v���P�[�V�����̏��Ǘ��A�h���X��o�^
	fill->app = app;

	// �N���b�v�{�[�h����p�^�[�����擾
	UpdateClipBoardPattern(&app->patterns);

	// �h��ׂ����[�h����p�̃��W�I�{�^���쐬�ƃf�[�^�A�R�[���o�b�N�֐��̃Z�b�g
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.select.target);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.rgb);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.rgba);
	buttons[2] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.alpha);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[fill->mode]), TRUE);
	g_object_set_data(G_OBJECT(buttons[0]), "fill-mode", GINT_TO_POINTER(0));
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(PatternFillSetMode), core->brush_data);
	g_object_set_data(G_OBJECT(buttons[1]), "fill-mode", GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(PatternFillSetMode), core->brush_data);
	g_object_set_data(G_OBJECT(buttons[2]), "fill-mode", GINT_TO_POINTER(2));
	g_signal_connect(G_OBJECT(buttons[2]), "toggled", G_CALLBACK(PatternFillSetMode), core->brush_data);

	// �쐬�������x���ƃ��W�I�{�^�����{�b�N�X�ɒǉ�
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[2], FALSE, TRUE, 0);

	// 臒l����p�̃X���C�_���쐬
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		fill->threshold, 0, 255, 1, 1, 0));
	g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(PatternFillChangeThreshold), core->brush_data);
	scale = SpinScaleNew(adjustment,
		app->labels->tool_box.select.threshold, 0);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �h��ׂ��^�[�Q�b�g����p�̃��W�I�{�^���쐬�ƃf�[�^�A�R�[���o�b�N�֐����Z�b�g
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.select.area);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.active_layer);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.canvas);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[fill->target]), TRUE);
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(PatternFillSetTarget), core->brush_data);
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(PatternFillSetTarget), core->brush_data);

	// �쐬�������x���ƃ��W�I�{�^�����{�b�N�X�ɒǉ�
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[1], FALSE, TRUE, 0);

	// �p�^�[���̊g�嗦����p�X���C�_
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		fill->scale, 10, 400, 1, 10, 0));
	g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(PatternFillChangeScale), core->brush_data);
	scale = SpinScaleNew(adjustment,
		app->labels->tool_box.scale, 1);
	// �{�b�N�X�ɒǉ�
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, TRUE, 0);

	// �h��ׂ��Z�x�ݒ�p�X���C�_
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		fill->flow*100, 0, 100, 1, 10, 0));
	g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(PatternFillChangeFlow), core->brush_data);
	scale = SpinScaleNew(adjustment,
		app->labels->tool_box.flow, 1);
	table = gtk_table_new(1, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), scale, 0, 3, 0, 1);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// ���E���]�A�㉺���]�I��p�`�F�b�N�{�b�N�X
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.reverse);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.reverse_horizontally);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		fill->pattern_flags & PATTERN_FLIP_HORIZONTALLY);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(PatternFillSetPatternFlag), core->brush_data);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.reverse_vertically);
	g_object_set_data(G_OBJECT(check_button), "flag-id", GINT_TO_POINTER(1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		fill->pattern_flags & PATTERN_FLIP_VERTICALLY);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(PatternFillSetPatternFlag), core->brush_data);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	// �������[�h�I�����W�I�{�^��
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.blend_mode);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	fill->mode_select[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.saturation);
	g_object_set_data(G_OBJECT(fill->mode_select[0]), "blend-mode", GINT_TO_POINTER(0));
	g_signal_connect(G_OBJECT(fill->mode_select[0]), "toggled", G_CALLBACK(PatternFillSetBlendMode), core->brush_data);
	fill->mode_select[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(fill->mode_select[0])),
		app->labels->tool_box.brightness);
	g_object_set_data(G_OBJECT(fill->mode_select[1]), "blend-mode", GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(fill->mode_select[1]), "toggled", G_CALLBACK(PatternFillSetBlendMode), core->brush_data);
	gtk_box_pack_start(GTK_BOX(table), fill->mode_select[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(table), fill->mode_select[1], FALSE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill->mode_select[fill->mode]), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	// �p�^�[���I���e�[�u�����쐬���ăX�N���[���h�E�B���h�E�ɓ����
	gtk_widget_set_size_request(scrolled_window,
		ICON_SIZE*PATTERN_SELECT_TABLE_WIDTH+4, ICON_SIZE*4+4);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
		CreatePatternSelectTable(fill));
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, FALSE, FALSE, 0);

	// �ڍאݒ��UI��������Ƃ��Ƀ{�^���z����J������
	g_signal_connect(G_OBJECT(vbox), "destroy", G_CALLBACK(OnPatternFillDetailUIDestroy), fill);

	// �N���b�v�{�[�h�̃p�^�[�����A�N�e�B�u�Ȃ�΃Z�b�g
	if(app->patterns.has_clip_board_pattern != FALSE)
	{
		if(fill->pattern_id == 0)
		{
			app->patterns.active_pattern =
				&app->patterns.clip_board;
		}
		else
		{
			app->patterns.active_pattern =
				&app->patterns.patterns[fill->pattern_id-1];
		}
	}

	// �A�N�e�B�u�ȃp�^�[�����`�����l����2�ȊO�Ȃ�΍������[�h�͖���
	gtk_widget_set_sensitive(fill->mode_select[0], app->patterns.active_pattern->channel == 2);
	gtk_widget_set_sensitive(fill->mode_select[1], app->patterns.active_pattern->channel == 2);

	// ���o�����I��p���W�I�{�^��
	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.select.detect_area);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
	buttons[0] = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.select.area_normal);
	g_object_set_data(G_OBJECT(buttons[0]), "direction", GINT_TO_POINTER(FUZZY_SELECT_DIRECTION_QUAD));
	g_signal_connect(G_OBJECT(buttons[0]), "toggled", G_CALLBACK(PatternFillSetDetectDirection), core->brush_data);
	buttons[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[0])),
		app->labels->tool_box.select.area_large);
	g_object_set_data(G_OBJECT(buttons[1]), "direction", GINT_TO_POINTER(FUZZY_SELECT_DIRECTION_OCT));
	g_signal_connect(G_OBJECT(buttons[1]), "toggled", G_CALLBACK(PatternFillSetDetectDirection), core->brush_data);
	gtk_box_pack_start(GTK_BOX(table), buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(table), buttons[1], FALSE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[fill->area_detect_direction]), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	buttons[0] = gtk_check_button_new_with_label(app->labels->tool_box.anti_alias);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[0]), fill->flags & PATTERN_FILL_FLAG_ANTI_ALIAS);
	g_signal_connect(GTK_BOX(vbox), "toggled", G_CALLBACK(PatternFillChangeAntiAlias), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), buttons[0], FALSE, FALSE, 3);

	// �h��Ԃ��͈͂̊g��k���X���C�_
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(fill->extend, -50, 50, 1, 1, 0));
	scale = SpinScaleNew(adjustment, app->labels->tool_box.extend, 0);
	g_signal_connect(G_OBJECT(scale), "value_changed", G_CALLBACK(PatternFillSetExtend), fill);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 2);

	return vbox;
#undef UI_FONT_SIZE
#undef ICON_SIZE
}

static void GradationPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		GRADATION* gradation = (GRADATION*)core->brush_data;
		gradation->start_x = x;
		gradation->start_y = y;
		gradation->end_x = x;
		gradation->end_y = y;
		gradation->flags |= GRADATION_STARTED;
	}
}

#define GradationEditSelectionPressCallBack GradationPressCallBack

static void GradationMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		GRADATION* gradation = (GRADATION*)core->brush_data;
		if((gradation->flags & GRADATION_STARTED) != 0)
		{
			gradation->end_x = x;
			gradation->end_y = y;
		}
	}
}

#define GradationEditSelectionMotionCallBack GradationMotionCallBack

static void GradationReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		GRADATION* gradation = (GRADATION*)core->brush_data;
		cairo_pattern_t *pattern;
		uint8 (*start_color)[3];
		uint8 (*end_color)[3];
		gdouble start_alpha = 1, end_alpha;
		int i;

		if((gradation->flags & GRADATION_STARTED) == 0)
		{
			return;
		}

		gradation->flags &= ~(GRADATION_STARTED);

		if((gradation->flags & GRADATION_COLOR_REVERSE) == 0)
		{
			start_color = &window->app->tool_window.color_chooser->rgb;

			switch(gradation->mode)
			{
			case GRADATION_DRAW_RGB_TO_BACK_RGB:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_BILINEAR:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_BILINEAR:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT:
				end_color = &window->app->tool_window.color_chooser->back_rgb;
				end_alpha = 1;
				break;
			case GRADATION_DRAW_RGB_TO_TRANSPARENT:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_BILINEAR:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_BILINEAR:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT:
				end_color = &window->app->tool_window.color_chooser->rgb;
				end_alpha = 0;
				break;
			}
		}
		else
		{
			start_color = &window->app->tool_window.color_chooser->back_rgb;

			switch(gradation->mode)
			{
			case GRADATION_DRAW_RGB_TO_BACK_RGB:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_BILINEAR:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_BILINEAR:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT:
				end_color = &window->app->tool_window.color_chooser->rgb;
				end_alpha = 1;
				break;
			case GRADATION_DRAW_RGB_TO_TRANSPARENT:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_BILINEAR:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_BILINEAR:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT:
				end_color = &window->app->tool_window.color_chooser->rgb;
				start_alpha = 0;
				end_alpha = 1;
			}
		}

		switch(gradation->mode)
		{
		case GRADATION_DRAW_RGB_TO_BACK_RGB:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT:
			pattern = cairo_pattern_create_linear(
				gradation->start_x, gradation->start_y, gradation->end_x, gradation->end_y
			);
			cairo_pattern_add_color_stop_rgba(pattern, 0,
				(*start_color)[0]*DIV_PIXEL, (*start_color)[1]*DIV_PIXEL, (*start_color)[2]*DIV_PIXEL, start_alpha
			);
			cairo_pattern_add_color_stop_rgba(pattern, 1,
				(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
			);
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_BILINEAR:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_BILINEAR:
		case GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT:
			pattern = cairo_pattern_create_linear(
				gradation->start_x, gradation->start_y, gradation->end_x, gradation->end_y
			);
			cairo_pattern_add_color_stop_rgba(pattern, 0,
				(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
			);
			cairo_pattern_add_color_stop_rgba(pattern, 0.5,
				(*start_color)[0]*DIV_PIXEL, (*start_color)[1]*DIV_PIXEL, (*start_color)[2]*DIV_PIXEL, start_alpha
			);
			cairo_pattern_add_color_stop_rgba(pattern, 1,
				(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
			);
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY:
			{
				gdouble d = sqrt((gradation->end_x-gradation->start_x)*(gradation->end_x-gradation->start_x)
					+ (gradation->end_y-gradation->start_y)*(gradation->end_y-gradation->start_y));
				pattern = cairo_pattern_create_radial(gradation->start_x, gradation->start_y,
					0, gradation->start_x, gradation->start_y, d);
				cairo_pattern_add_color_stop_rgba(pattern, 0,
				(*start_color)[0]*DIV_PIXEL, (*start_color)[1]*DIV_PIXEL, (*start_color)[2]*DIV_PIXEL, start_alpha
				);
				cairo_pattern_add_color_stop_rgba(pattern, 1,
					(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
				);
			}
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_BILINEAR:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_BILINEAR:
		case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT:
			{
				gdouble d = sqrt((gradation->end_x-gradation->start_x)*(gradation->end_x-gradation->start_x)
					+ (gradation->end_y-gradation->start_y)*(gradation->end_y-gradation->start_y));
				pattern = cairo_pattern_create_radial(gradation->start_x, gradation->start_y,
					0, gradation->start_x, gradation->start_y, d);
				cairo_pattern_add_color_stop_rgba(pattern, 0,
					(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
				);
				cairo_pattern_add_color_stop_rgba(pattern, 0.5,
				(*start_color)[0]*DIV_PIXEL, (*start_color)[1]*DIV_PIXEL, (*start_color)[2]*DIV_PIXEL, start_alpha
				);
				cairo_pattern_add_color_stop_rgba(pattern, 1,
					(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
				);
			}
			break;
		case GRADATION_DRAW_IRIDESCENE_LINEAR:
		case GRADATION_DRAW_IRIDESCENE_RADIALLY:
			{
				const color[][3] = {
					{0xFF, 0x00, 0x00},
					{0xFF, 0x2F, 0x00},
					{0xFF, 0x62, 0x00},
					{0xFF, 0x90, 0x00},
					{0xFF, 0xBF, 0x00},
					{0xFF, 0xEE, 0x00},
					{0xDD, 0xFF, 0x00},
					{0xAE, 0xFF, 0x00},
					{0x80, 0xFF, 0x00},
					{0x51, 0xFF, 0x00},
					{0x1E, 0xFF, 0x00},
					{0x00, 0xFF, 0x11},
					{0x00, 0xFF, 0x40},
					{0x00, 0xFF, 0x6F},
					{0x00, 0xFF, 0xA1},
					{0x00, 0xFF, 0xD0},
					{0x00, 0xFF, 0xFF},
					{0x00, 0xD0, 0xFF},
					{0x00, 0x9D, 0xFF},
					{0x00, 0x6F, 0xFF},
					{0x00, 0x40, 0xFF},
					{0x00, 0x11, 0xFF},
					{0x22, 0x00, 0xFF},
					{0x51, 0x00, 0xFF},
					{0x80, 0x00, 0xFF},
					{0xAE, 0x00, 0xFF},
					{0xE1, 0x00, 0xFF},
					{0xFF, 0x00, 0xEE},
					{0xFF, 0x00, 0xBF},
					{0xFF, 0x00, 0x90},
					{0xFF, 0x00, 0x5E},
					{0xFF, 0x00, 0x2F}
				};
				const int num_color = sizeof(color)/sizeof(color[0]);

				gdouble d = sqrt((gradation->end_x-gradation->start_x)*(gradation->end_x-gradation->start_x)
					+ (gradation->end_y-gradation->start_y)*(gradation->end_y-gradation->start_y));
				if(gradation->mode == GRADATION_DRAW_IRIDESCENE_LINEAR)
				{
					pattern = cairo_pattern_create_linear(
						gradation->start_x, gradation->start_y, gradation->end_x, gradation->end_y
					);
				}
				else
				{
					pattern = cairo_pattern_create_radial(gradation->start_x, gradation->start_y,
						0, gradation->start_x, gradation->start_y, d);
				}

				if((gradation->flags & GRADATION_COLOR_REVERSE) == 0)
				{
					for(i=0; i<num_color; i++)
					{
						cairo_pattern_add_color_stop_rgb(pattern, (FLOAT_T)i/num_color,
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
							color[i][2] * DIV_PIXEL, color[i][1] * DIV_PIXEL, color[i][0] * DIV_PIXEL
#else
							color[i][0] * DIV_PIXEL, color[i][1] * DIV_PIXEL, color[i][2] * DIV_PIXEL
#endif
						);
					}
				}
				else
				{
					for(i=0; i<num_color; i++)
					{
						cairo_pattern_add_color_stop_rgb(pattern, (FLOAT_T)i/num_color,
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
							color[num_color-i-1][2] * DIV_PIXEL, color[num_color-i-1][1] * DIV_PIXEL, color[num_color-i-1][0] * DIV_PIXEL
#else
							color[num_color-i-1][0] * DIV_PIXEL, color[num_color-i-1][1] * DIV_PIXEL, color[num_color-i-1][2] * DIV_PIXEL
#endif
						);
					}
				}
			}
			break;
		}

		if(gradation->mode == GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT || gradation->mode == GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT
			|| gradation->mode == GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT || gradation->mode == GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT
			|| gradation->mode == GRADATION_DRAW_IRIDESCENE_LINEAR || gradation->mode == GRADATION_DRAW_IRIDESCENE_RADIALLY)
		{
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
		}
		else
		{
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
		}

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{
			cairo_set_source(window->work_layer->cairo_p, pattern);
			cairo_rectangle(window->work_layer->cairo_p, 0, 0,
				window->work_layer->width, window->work_layer->height);
			cairo_fill(window->work_layer->cairo_p);
		}
		else
		{
			cairo_set_source(window->work_layer->cairo_p, pattern);
			cairo_rectangle(window->work_layer->cairo_p, 0, 0,
				window->work_layer->width, window->work_layer->height);
			cairo_mask_surface(window->work_layer->cairo_p, window->selection->surface_p, 0, 0);
		}
		
		cairo_pattern_destroy(pattern);

		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			core->min_x = window->selection_area.min_x;
			core->min_y = window->selection_area.min_y;
			core->max_x = window->selection_area.max_x;
			core->max_y = window->selection_area.max_y;
		}
		else
		{
			core->min_x = core->min_y = 0;
			core->max_x = window->active_layer->width;
			core->max_y = window->active_layer->height;
		}

		if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
		{
			window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		}
		else
		{
			window->work_layer->layer_mode = LAYER_BLEND_ATOP;
		}

		AddBrushHistory(core, window->active_layer);

		g_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, window->active_layer);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

static void GradationEditSelectionReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		GRADATION* gradation = (GRADATION*)core->brush_data;
		cairo_pattern_t *pattern;
		uint8 (*start_color)[3];
		uint8 (*end_color)[3];
		gdouble start_alpha = 1, end_alpha;
		int i;

		if((gradation->flags & GRADATION_STARTED) == 0)
		{
			return;
		}

		gradation->flags &= ~(GRADATION_STARTED);

		if((gradation->flags & GRADATION_COLOR_REVERSE) == 0)
		{
			start_color = &window->app->tool_window.color_chooser->back_rgb;

			switch(gradation->mode)
			{
			case GRADATION_DRAW_RGB_TO_BACK_RGB:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_BILINEAR:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_BILINEAR:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT:
				end_color = &window->app->tool_window.color_chooser->rgb;
				end_alpha = 1;
				break;
			case GRADATION_DRAW_RGB_TO_TRANSPARENT:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_BILINEAR:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_BILINEAR:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT:
				end_color = &window->app->tool_window.color_chooser->back_rgb;
				start_alpha = 0;
				end_alpha = 1;
				break;
			}
		}
		else
		{
			start_color = &window->app->tool_window.color_chooser->rgb;

			switch(gradation->mode)
			{
			case GRADATION_DRAW_RGB_TO_BACK_RGB:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_BILINEAR:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_BILINEAR:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT:
			case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT:
				end_color = &window->app->tool_window.color_chooser->back_rgb;
				end_alpha = 1;
				break;
			case GRADATION_DRAW_RGB_TO_TRANSPARENT:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_BILINEAR:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_BILINEAR:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT:
			case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT:
				end_color = &window->app->tool_window.color_chooser->rgb;
				end_alpha = 0;
			}
		}

		switch(gradation->mode)
		{
		case GRADATION_DRAW_RGB_TO_BACK_RGB:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT:
			pattern = cairo_pattern_create_linear(
				gradation->start_x, gradation->start_y, gradation->end_x, gradation->end_y
			);
			cairo_pattern_add_color_stop_rgba(pattern, 0,
				(*start_color)[0]*DIV_PIXEL, (*start_color)[1]*DIV_PIXEL, (*start_color)[2]*DIV_PIXEL, start_alpha
			);
			cairo_pattern_add_color_stop_rgba(pattern, 1,
				(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
			);
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_BILINEAR:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_BILINEAR:
		case GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT:
			pattern = cairo_pattern_create_linear(
				gradation->start_x, gradation->start_y, gradation->end_x, gradation->end_y
			);
			cairo_pattern_add_color_stop_rgba(pattern, 0,
				(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
			);
			cairo_pattern_add_color_stop_rgba(pattern, 0.5,
				(*start_color)[0]*DIV_PIXEL, (*start_color)[1]*DIV_PIXEL, (*start_color)[2]*DIV_PIXEL, start_alpha
			);
			cairo_pattern_add_color_stop_rgba(pattern, 1,
				(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
			);
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY:
			{
				gdouble d = sqrt((gradation->end_x-gradation->start_x)*(gradation->end_x-gradation->start_x)
					+ (gradation->end_y-gradation->start_y)*(gradation->end_y-gradation->start_y));
				pattern = cairo_pattern_create_radial(gradation->start_x, gradation->start_y,
					0, gradation->start_x, gradation->start_y, d);
				cairo_pattern_add_color_stop_rgba(pattern, 0,
				(*start_color)[0]*DIV_PIXEL, (*start_color)[1]*DIV_PIXEL, (*start_color)[2]*DIV_PIXEL, start_alpha
				);
				cairo_pattern_add_color_stop_rgba(pattern, 1,
					(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
				);
			}
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_BILINEAR:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_BILINEAR:
		case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT:
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT:
			{
				gdouble d = sqrt((gradation->end_x-gradation->start_x)*(gradation->end_x-gradation->start_x)
					+ (gradation->end_y-gradation->start_y)*(gradation->end_y-gradation->start_y));
				pattern = cairo_pattern_create_radial(gradation->start_x, gradation->start_y,
					0, gradation->start_x, gradation->start_y, d);
				cairo_pattern_add_color_stop_rgba(pattern, 0,
					(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
				);
				cairo_pattern_add_color_stop_rgba(pattern, 0.5,
				(*start_color)[0]*DIV_PIXEL, (*start_color)[1]*DIV_PIXEL, (*start_color)[2]*DIV_PIXEL, start_alpha
				);
				cairo_pattern_add_color_stop_rgba(pattern, 1,
					(*end_color)[0]*DIV_PIXEL, (*end_color)[1]*DIV_PIXEL, (*end_color)[2]*DIV_PIXEL, end_alpha
				);
			}
			break;
		case GRADATION_DRAW_IRIDESCENE_LINEAR:
		case GRADATION_DRAW_IRIDESCENE_RADIALLY:
			{
				const color[][3] = {
					{0xFF, 0x00, 0x00},
					{0xFF, 0x2F, 0x00},
					{0xFF, 0x62, 0x00},
					{0xFF, 0x90, 0x00},
					{0xFF, 0xBF, 0x00},
					{0xFF, 0xEE, 0x00},
					{0xDD, 0xFF, 0x00},
					{0xAE, 0xFF, 0x00},
					{0x80, 0xFF, 0x00},
					{0x51, 0xFF, 0x00},
					{0x1E, 0xFF, 0x00},
					{0x00, 0xFF, 0x11},
					{0x00, 0xFF, 0x40},
					{0x00, 0xFF, 0x6F},
					{0x00, 0xFF, 0xA1},
					{0x00, 0xFF, 0xD0},
					{0x00, 0xFF, 0xFF},
					{0x00, 0xD0, 0xFF},
					{0x00, 0x9D, 0xFF},
					{0x00, 0x6F, 0xFF},
					{0x00, 0x40, 0xFF},
					{0x00, 0x11, 0xFF},
					{0x22, 0x00, 0xFF},
					{0x51, 0x00, 0xFF},
					{0x80, 0x00, 0xFF},
					{0xAE, 0x00, 0xFF},
					{0xE1, 0x00, 0xFF},
					{0xFF, 0x00, 0xEE},
					{0xFF, 0x00, 0xBF},
					{0xFF, 0x00, 0x90},
					{0xFF, 0x00, 0x5E},
					{0xFF, 0x00, 0x2F}
				};
				const int num_color = sizeof(color)/sizeof(color[0]);

				gdouble d = sqrt((gradation->end_x-gradation->start_x)*(gradation->end_x-gradation->start_x)
					+ (gradation->end_y-gradation->start_y)*(gradation->end_y-gradation->start_y));
				if(gradation->mode == GRADATION_DRAW_IRIDESCENE_LINEAR)
				{
					pattern = cairo_pattern_create_linear(
						gradation->start_x, gradation->start_y, gradation->end_x, gradation->end_y
					);
				}
				else
				{
					pattern = cairo_pattern_create_radial(gradation->start_x, gradation->start_y,
						0, gradation->start_x, gradation->start_y, d);
				}

				if((gradation->flags & GRADATION_COLOR_REVERSE) == 0)
				{
					for(i=0; i<num_color; i++)
					{
						cairo_pattern_add_color_stop_rgb(pattern, (FLOAT_T)i/num_color,
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
							color[i][2] * DIV_PIXEL, color[i][1] * DIV_PIXEL, color[i][0] * DIV_PIXEL
#else
							color[i][0] * DIV_PIXEL, color[i][1] * DIV_PIXEL, color[i][2] * DIV_PIXEL
#endif
						);
					}
				}
				else
				{
					for(i=0; i<num_color; i++)
					{
						cairo_pattern_add_color_stop_rgb(pattern, (FLOAT_T)i/num_color,
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
							color[num_color-i-1][2] * DIV_PIXEL, color[num_color-i-1][1] * DIV_PIXEL, color[num_color-i-1][0] * DIV_PIXEL
#else
							color[num_color-i-1][0] * DIV_PIXEL, color[num_color-i-1][1] * DIV_PIXEL, color[num_color-i-1][2] * DIV_PIXEL
#endif
						);
					}
				}
			}
			break;
		}

		if(gradation->mode == GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT || gradation->mode == GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT
			|| gradation->mode == GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT || gradation->mode == GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT
			|| gradation->mode == GRADATION_DRAW_IRIDESCENE_LINEAR || gradation->mode == GRADATION_DRAW_IRIDESCENE_RADIALLY)
		{
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
		}
		else
		{
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
		}

		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source(window->work_layer->cairo_p, pattern);
		cairo_rectangle(window->work_layer->cairo_p, 0, 0,
			window->work_layer->width, window->work_layer->height);
		cairo_fill(window->work_layer->cairo_p);
		
		cairo_pattern_destroy(pattern);

		core->min_x = core->min_y = 0;
		core->max_x = window->active_layer->width;
		core->max_y = window->active_layer->height;

		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[SELECTION_BLEND_NORMAL](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

static void DrawGradationCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	GRADATION* gradation = (GRADATION*)data;
	if((gradation->flags & GRADATION_STARTED) != 0)
	{
		FLOAT_T zoom = window->zoom * 0.01;
		cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
		cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
		cairo_move_to(window->disp_temp->cairo_p, gradation->start_x * zoom, gradation->start_y * zoom);
		cairo_line_to(window->disp_temp->cairo_p, gradation->end_x * zoom, gradation->end_y * zoom);
		cairo_stroke(window->disp_temp->cairo_p);
	}
}

static int IsGradationHasAlpha(int index)
{
	if(index == GRADATION_DRAW_RGB_TO_BACK_RGB
		|| index == GRADATION_DRAW_RGB_TO_BACK_RGB_BILINEAR
		|| index == GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT
		|| index == GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY
		|| index == GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_BILINEAR
		|| index == GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT
		|| index == GRADATION_DRAW_IRIDESCENE_LINEAR
		|| index == GRADATION_DRAW_IRIDESCENE_RADIALLY
	)
	{
		return FALSE;
	}

	return TRUE;
}

static void GradationOnChangeEditSelection(void* data, int is_editting)
{
	GRADATION *gradation = (GRADATION*)data;
	int index = 0;
	int i, j;

	if(IsGradationHasAlpha(gradation->mode) == FALSE)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
			gradation->button_table[0][GRADATION_DRAW_RGB_TO_TRANSPARENT]), TRUE);
	}

	for(i=0; i<GRADATION_PATTERN_TABLE_HEIGHT; i++)
	{
		for(j=0; j<GRADATION_PATTERN_TABLE_WIDTH; j++)
		{
			if(IsGradationHasAlpha(index) == FALSE)
			{
				gtk_widget_set_sensitive(gradation->button_table[i][j], !is_editting);
			}
			index++;
		}
	}
}

static void GradationPatternButtonClicked(GtkWidget* widget, GRADATION* gradation)
{
	int button_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "gradation-mode"));
	int i, j;

	if(button_mode == gradation->mode)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		}
		return;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		return;
	}

	gradation->mode = (uint16)button_mode;

	for(i=0; i<GRADATION_PATTERN_TABLE_HEIGHT; i++)
	{
		for(j=0; j<GRADATION_PATTERN_TABLE_WIDTH; j++)
		{
			if(gradation->button_table[i][j] != NULL)
			{
				if(i*GRADATION_PATTERN_TABLE_HEIGHT+j == gradation->mode)
				{
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
						gradation->button_table[i][j])) == FALSE)
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gradation->button_table[i][j]), TRUE);
					}
				}
				else
				{
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
						gradation->button_table[i][j])) != FALSE)
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gradation->button_table[i][j]), FALSE);
					}
				}
			}
		}
	}	
}

static GtkWidget* CreateGradationPatternTable(GRADATION* gradation)
{
#define ICON_SIZE 32
	GtkWidget* table = gtk_table_new(
		GRADATION_PATTERN_TABLE_WIDTH, GRADATION_PATTERN_TABLE_HEIGHT, TRUE);
	GtkWidget* button, *image;
	GdkPixbuf* pixbuf;
	cairo_t *cairo_p;
	cairo_surface_t* surface_p;
	cairo_pattern_t *pattern;
	uint8 *pixels;
	gboolean is_editting_selection = FALSE;
	int stride;
	int i, j, k;
	int x, y;

	if(gradation->app->window_num > 0)
	{
		if((gradation->app->draw_window[gradation->app->active_window]->flags
			& DRAW_WINDOW_EDIT_SELECTION) != 0)
		{
			is_editting_selection = TRUE;
		}
	}

	for(i=0, x=0, y=0; i<GRADATION_MODE_NUM; i++, x++)
	{
		if(x == GRADATION_PATTERN_TABLE_WIDTH)
		{
			x = 0;
			y++;
		}

		pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, ICON_SIZE, ICON_SIZE);
		gdk_pixbuf_composite_color(pixbuf, pixbuf, 0, 0, ICON_SIZE, ICON_SIZE,
			0, 0, 1, 1, GDK_INTERP_BILINEAR, 255, 0, 0, 4, 0xFFFFFF, 0x888888
		);
		pixels = gdk_pixbuf_get_pixels(pixbuf);
		stride = gdk_pixbuf_get_rowstride(pixbuf);
		for(j=0; j<ICON_SIZE; j++)
		{
			for(k=0; k<ICON_SIZE; k++)
			{
				uint8 pixel_value = pixels[j*stride+k*4] * 3;
				pixels[j*stride+k*4] = pixel_value;
				pixels[j*stride+k*4+1] = pixel_value;
				pixels[j*stride+k*4+2] = pixel_value;
				pixels[j*stride+k*4+3] = 0xff;
			}
		}

		surface_p = cairo_image_surface_create_for_data(
			pixels, CAIRO_FORMAT_ARGB32, ICON_SIZE, ICON_SIZE, stride
		);
		cairo_p = cairo_create(surface_p);
		switch(i)
		{
		case GRADATION_DRAW_RGB_TO_BACK_RGB:
			pattern = cairo_pattern_create_linear(0, 0, ICON_SIZE, ICON_SIZE);
			cairo_pattern_add_color_stop_rgb(pattern, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgb(pattern, 1, 1, 1, 1);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_TRANSPARENT:
			pattern = cairo_pattern_create_linear(0, 0, ICON_SIZE, ICON_SIZE);
			cairo_pattern_add_color_stop_rgba(pattern, 0, 0, 0, 0, 1);
			cairo_pattern_add_color_stop_rgba(pattern, 1, 0, 0, 0, 0);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_BILINEAR:
			pattern = cairo_pattern_create_linear(0, 0, ICON_SIZE, ICON_SIZE);
			cairo_pattern_add_color_stop_rgb(pattern, 0, 1, 1, 1);
			cairo_pattern_add_color_stop_rgb(pattern, 0.5, 0, 0, 0);
			cairo_pattern_add_color_stop_rgb(pattern, 1, 1, 1, 1);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_BILINEAR:
			pattern = cairo_pattern_create_linear(0, 0, ICON_SIZE, ICON_SIZE);
			cairo_pattern_add_color_stop_rgba(pattern, 0, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgba(pattern, 0.5, 0, 0, 0, 1);
			cairo_pattern_add_color_stop_rgba(pattern, 1, 0, 0, 0, 0);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT:
			pattern = cairo_pattern_create_linear(0, 0, ICON_SIZE, ICON_SIZE);
			cairo_pattern_add_color_stop_rgb(pattern, 0, 1, 1, 1);
			cairo_pattern_add_color_stop_rgb(pattern, 0.2f, 0, 0, 0);
			cairo_pattern_add_color_stop_rgb(pattern, 0.4f, 1, 1, 1);
			cairo_pattern_add_color_stop_rgb(pattern, 0.6f, 0, 0, 0);
			cairo_pattern_add_color_stop_rgb(pattern, 0.8f, 1, 1, 1);
			cairo_pattern_add_color_stop_rgb(pattern, 1, 0, 0, 0);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT:
			pattern = cairo_pattern_create_linear(0, 0, ICON_SIZE, ICON_SIZE);
			cairo_pattern_add_color_stop_rgba(pattern, 0, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgba(pattern, 0.2f, 0, 0, 0, 1);
			cairo_pattern_add_color_stop_rgba(pattern, 0.4f, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgba(pattern, 0.6f, 0, 0, 0, 1);
			cairo_pattern_add_color_stop_rgba(pattern, 0.8f, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgba(pattern, 1, 0, 0, 0, 1);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY:
			pattern = cairo_pattern_create_radial(
				ICON_SIZE/2, ICON_SIZE/2, 0, ICON_SIZE/2, ICON_SIZE/2, ICON_SIZE/2
			);
			cairo_pattern_add_color_stop_rgb(pattern, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgb(pattern, 1, 1, 1, 1);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY:
			pattern = cairo_pattern_create_radial(
				ICON_SIZE/2, ICON_SIZE/2, 0, ICON_SIZE/2, ICON_SIZE/2, ICON_SIZE/2
			);
			cairo_pattern_add_color_stop_rgba(pattern, 0, 0, 0, 0, 1);
			cairo_pattern_add_color_stop_rgba(pattern, 1, 0, 0, 0, 0);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_BILINEAR:
			pattern = cairo_pattern_create_radial(
				ICON_SIZE/2, ICON_SIZE/2, 0, ICON_SIZE/2, ICON_SIZE/2, ICON_SIZE/2
			);
			cairo_pattern_add_color_stop_rgb(pattern, 0, 1, 1, 1);
			cairo_pattern_add_color_stop_rgb(pattern, 0.5, 0, 0, 0);
			cairo_pattern_add_color_stop_rgb(pattern, 1, 1, 1, 1);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_BILINEAR:
			pattern = cairo_pattern_create_radial(
				ICON_SIZE/2, ICON_SIZE/2, 0, ICON_SIZE/2, ICON_SIZE/2, ICON_SIZE/2
			);
			cairo_pattern_add_color_stop_rgba(pattern, 0, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgba(pattern, 0.5, 0, 0, 0, 1);
			cairo_pattern_add_color_stop_rgba(pattern, 1, 0, 0, 0, 0);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT:
			pattern = cairo_pattern_create_radial(
				ICON_SIZE/2, ICON_SIZE/2, 0, ICON_SIZE/2, ICON_SIZE/2, ICON_SIZE/2
			);
			cairo_pattern_add_color_stop_rgb(pattern, 0, 1, 1, 1);
			cairo_pattern_add_color_stop_rgb(pattern, 0.2f, 0, 0, 0);
			cairo_pattern_add_color_stop_rgb(pattern, 0.4f, 1, 1, 1);
			cairo_pattern_add_color_stop_rgb(pattern, 0.6f, 0, 0, 0);
			cairo_pattern_add_color_stop_rgb(pattern, 0.8f, 1, 1, 1);
			cairo_pattern_add_color_stop_rgb(pattern, 1, 0, 0, 0);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT:
			pattern = cairo_pattern_create_radial(
				ICON_SIZE/2, ICON_SIZE/2, 0, ICON_SIZE/2, ICON_SIZE/2, ICON_SIZE/2
			);
			cairo_pattern_add_color_stop_rgba(pattern, 0, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgba(pattern, 0.2f, 0, 0, 0, 1);
			cairo_pattern_add_color_stop_rgba(pattern, 0.4f, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgba(pattern, 0.6f, 0, 0, 0, 1);
			cairo_pattern_add_color_stop_rgba(pattern, 0.8f, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgba(pattern, 1, 0, 0, 0, 1);
			cairo_set_source(cairo_p, pattern);
			cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
			cairo_fill(cairo_p);
			break;
		case GRADATION_DRAW_IRIDESCENE_LINEAR:
			{
				const color[][3] = {
					{0xFF, 0x00, 0x00},
					{0xFF, 0x2F, 0x00},
					{0xFF, 0x62, 0x00},
					{0xFF, 0x90, 0x00},
					{0xFF, 0xBF, 0x00},
					{0xFF, 0xEE, 0x00},
					{0xDD, 0xFF, 0x00},
					{0xAE, 0xFF, 0x00},
					{0x80, 0xFF, 0x00},
					{0x51, 0xFF, 0x00},
					{0x1E, 0xFF, 0x00},
					{0x00, 0xFF, 0x11},
					{0x00, 0xFF, 0x40},
					{0x00, 0xFF, 0x6F},
					{0x00, 0xFF, 0xA1},
					{0x00, 0xFF, 0xD0},
					{0x00, 0xFF, 0xFF},
					{0x00, 0xD0, 0xFF},
					{0x00, 0x9D, 0xFF},
					{0x00, 0x6F, 0xFF},
					{0x00, 0x40, 0xFF},
					{0x00, 0x11, 0xFF},
					{0x22, 0x00, 0xFF},
					{0x51, 0x00, 0xFF},
					{0x80, 0x00, 0xFF},
					{0xAE, 0x00, 0xFF},
					{0xE1, 0x00, 0xFF},
					{0xFF, 0x00, 0xEE},
					{0xFF, 0x00, 0xBF},
					{0xFF, 0x00, 0x90},
					{0xFF, 0x00, 0x5E},
					{0xFF, 0x00, 0x2F}
				};
				const int num_color = sizeof(color)/sizeof(color[0]);

				pattern = cairo_pattern_create_linear(0, 0, ICON_SIZE, ICON_SIZE);
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
				for(j=0; j<num_color; j++)
				{
					cairo_pattern_add_color_stop_rgb(pattern, (FLOAT_T)j/num_color,
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
						color[j][2] * DIV_PIXEL, color[j][1] * DIV_PIXEL, color[j][0] * DIV_PIXEL
#else
						color[j][0] * DIV_PIXEL, color[j][1] * DIV_PIXEL, color[j][2] * DIV_PIXEL
#endif
					);
				}
				cairo_set_source(cairo_p, pattern);
				cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
				cairo_fill(cairo_p);
			}
			break;
		case GRADATION_DRAW_IRIDESCENE_RADIALLY:
			{
				const color[][3] = {
					{0xFF, 0x00, 0x00},
					{0xFF, 0x2F, 0x00},
					{0xFF, 0x62, 0x00},
					{0xFF, 0x90, 0x00},
					{0xFF, 0xBF, 0x00},
					{0xFF, 0xEE, 0x00},
					{0xDD, 0xFF, 0x00},
					{0xAE, 0xFF, 0x00},
					{0x80, 0xFF, 0x00},
					{0x51, 0xFF, 0x00},
					{0x1E, 0xFF, 0x00},
					{0x00, 0xFF, 0x11},
					{0x00, 0xFF, 0x40},
					{0x00, 0xFF, 0x6F},
					{0x00, 0xFF, 0xA1},
					{0x00, 0xFF, 0xD0},
					{0x00, 0xFF, 0xFF},
					{0x00, 0xD0, 0xFF},
					{0x00, 0x9D, 0xFF},
					{0x00, 0x6F, 0xFF},
					{0x00, 0x40, 0xFF},
					{0x00, 0x11, 0xFF},
					{0x22, 0x00, 0xFF},
					{0x51, 0x00, 0xFF},
					{0x80, 0x00, 0xFF},
					{0xAE, 0x00, 0xFF},
					{0xE1, 0x00, 0xFF},
					{0xFF, 0x00, 0xEE},
					{0xFF, 0x00, 0xBF},
					{0xFF, 0x00, 0x90},
					{0xFF, 0x00, 0x5E},
					{0xFF, 0x00, 0x2F}
				};
				const int num_color = sizeof(color)/sizeof(color[0]);

				pattern = cairo_pattern_create_radial(
					ICON_SIZE/2, ICON_SIZE/2, 0, ICON_SIZE/2, ICON_SIZE/2, ICON_SIZE/2
				);
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
				for(j=0; j<num_color; j++)
				{
					cairo_pattern_add_color_stop_rgb(pattern, (FLOAT_T)j/num_color,
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
						color[j][2] * DIV_PIXEL, color[j][1] * DIV_PIXEL, color[j][0] * DIV_PIXEL
#else
						color[j][0] * DIV_PIXEL, color[j][1] * DIV_PIXEL, color[j][2] * DIV_PIXEL
#endif
					);
				}
				cairo_set_source(cairo_p, pattern);
				cairo_rectangle(cairo_p, 0, 0, ICON_SIZE, ICON_SIZE);
				cairo_fill(cairo_p);
			}
			break;
		}
		cairo_pattern_destroy(pattern);

		image = gtk_image_new_from_pixbuf(pixbuf);

		button = gtk_toggle_button_new();
		if(gradation->mode == i)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		}
		if(is_editting_selection != FALSE)
		{
			if(IsGradationHasAlpha(i) == FALSE)
			{
				gtk_widget_set_sensitive(button, FALSE);
			}
		}
		g_signal_connect(G_OBJECT(button), "toggled",
			G_CALLBACK(GradationPatternButtonClicked), gradation);
		gtk_table_attach(GTK_TABLE(table), button, x, x+1, y, y+1, GTK_SHRINK, GTK_SHRINK, 0, 0);
		g_object_set_data(G_OBJECT(button), "gradation-mode", GINT_TO_POINTER(i));
		gradation->button_table[y][x] = button;
		gtk_container_add(GTK_CONTAINER(button), image);

		cairo_surface_destroy(surface_p);
		cairo_destroy(cairo_p);
	}

	return table;
#undef ICON_SIZE
}

static void ChangeGradationReverse(GtkWidget* widget, GRADATION* gradation)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		gradation->flags &= ~(GRADATION_COLOR_REVERSE);
	}
	else
	{
		gradation->flags |= GRADATION_COLOR_REVERSE;
	}
}

static GtkWidget* CreateGrationDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define ICON_SIZE 32
	GRADATION* gradation = (GRADATION*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget* check_button;
	GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);

	check_button = gtk_check_button_new_with_label(app->labels->tool_box.gradation_reverse);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(ChangeGradationReverse), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, TRUE, 0);

	gtk_widget_set_size_request(scrolled_window, ICON_SIZE*4+4, ICON_SIZE*6+4);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
		CreateGradationPatternTable(gradation));
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, FALSE, FALSE, 0);

	return vbox;
#undef ICON_SIZE
}

static void BlurToolPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		BLUR_TOOL* blur = (BLUR_TOOL*)core->brush_data;
		gdouble r, alpha;
		gdouble zoom;
		cairo_matrix_t matrix;
		int extends;
		int num_samples;
		gdouble min_x, min_y, max_x, max_y;
		int sum_color[4], before_y_color[4];
		int stride = window->work_layer->stride;
		uint8* work_pixels = window->work_layer->pixels;
		uint8 *add_pixel, *temp_pixel, *mask;
		uint8 t;
		int counter_x, counter_y, start_x, start_y, width, height;
		int start_j, end_i, end_j;
		int temp_stride;
		int i, j;

		window->work_layer->layer_mode = LAYER_BLEND_SOURCE;

		(void)memcpy(window->work_layer->pixels, window->active_layer->pixels,
			window->pixel_buf_size);

		if((blur->flags & BRUSH_FLAG_SIZE) != 0)
		{
			r = blur->r * pressure;
			zoom = 1/pressure;
		}
		else
		{
			r = blur->r;
			zoom = 1;
		}

		r = ((blur->flags & BRUSH_FLAG_SIZE) == 0) ?
			blur->r : blur->r * pressure;
		alpha = ((blur->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;
		extends = (int)(r * (blur->color_extend * 0.01))/2;
		if(extends == 0)
		{
			return;
		}

		min_x = x - r, min_y = y - r;
		max_x = x + r, max_y = y + r;

		blur->before_x = x, blur->before_y = y;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		window->update.x = (int)min_x, window->update.y = (int)min_y;
		window->update.width = (int)max_x - window->update.x;
		window->update.height = (int)max_y - window->update.y;

		window->flags |= DRAW_WINDOW_UPDATE_PART;

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;
		start_x = (int32)core->min_x, start_y = (int32)core->min_y;
		width = (int32)(core->max_x-core->min_x);
		height = (int32)(core->max_y-core->min_y);
		temp_stride = width*5;

		for(counter_y=0; counter_y<height; counter_y++)
		{
			(void)memset(&window->mask->pixels[(start_y+counter_y)*window->width
				+ start_x], 0, width);
		}

		cairo_set_operator(window->mask->cairo_p, CAIRO_OPERATOR_OVER);
		mask = window->mask->pixels;
		cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_matrix_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(window->alpha_cairo, core->brush_pattern);
				cairo_paint_with_alpha(window->alpha_cairo, alpha);
			}
			else
			{
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(window->alpha_cairo, core->temp_pattern);
				cairo_mask_surface(window->alpha_cairo,
					window->active_layer->surface_p, 0, 0);
			}
		}
		else
		{
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(window->alpha_cairo, core->temp_pattern);
				cairo_mask_surface(window->alpha_cairo,
					window->selection->surface_p, 0, 0);
			}
			else
			{
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(window->alpha_cairo, core->temp_pattern);

				for(counter_y=0; counter_y<height; counter_y++)
				{
					(void)memset(&window->mask_temp->pixels[(start_y+counter_y)*window->width
						+ start_x], 0, width);
				}

				cairo_mask_surface(window->alpha_cairo,
					window->selection->surface_p, 0, 0);
				cairo_set_source_surface(window->gray_mask_cairo,
					window->alpha_surface, 0, 0);
				cairo_mask_surface(window->gray_mask_cairo,
					window->active_layer->surface_p, 0, 0);

				mask = window->mask_temp->pixels;
			}
		}

		for(counter_y=0; counter_y<height; counter_y++)
		{
			for(counter_x=0; counter_x<width; counter_x++)
			{
				window->temp_layer->pixels[temp_stride*counter_y+counter_x*5+4] =
					mask[window->width*(counter_y+start_y)+(counter_x+start_x)];
			}
		}

		i = start_y-extends, start_j=start_x-extends;
		end_i = start_y+extends, end_j=start_x+extends;
		if(i < 0)
		{
			i = 0;
		}
		if(start_j < 0)
		{
			start_j = 0;
		}
		if(end_i > window->work_layer->height)
		{
			if(i >= window->work_layer->height)
			{
				return;
			}
			end_i = window->work_layer->height;
		}
		if(end_j > window->work_layer->width)
		{
			if(start_j >= window->work_layer->width)
			{
				return;
			}
			end_j = window->work_layer->width;
		}

		sum_color[0] = sum_color[1] = sum_color[2] = sum_color[3] = 0;
		for( ; i<end_i; i++)
		{
			for(j=start_j; j<end_j; j++)
			{
				add_pixel = &work_pixels[i*stride+j*4];
				sum_color[0] += add_pixel[0];
				sum_color[1] += add_pixel[1];
				sum_color[2] += add_pixel[2];
				sum_color[3] += add_pixel[3];
			}
		}
		before_y_color[0] = sum_color[0];
		before_y_color[1] = sum_color[1];
		before_y_color[2] = sum_color[2];
		before_y_color[3] = sum_color[3];

		for(counter_y=0; counter_y<height; counter_y++)
		{
			start_j = start_x-extends, end_j = start_x+extends;
			i = counter_y+start_y-extends, end_i = counter_y+start_y+extends;
			if(i < 0)
			{
				i = 0;
			}
			if(end_i >= window->work_layer->height)
			{
				if(i >= window->work_layer->height-1)
				{
					return;
				}
				end_i = window->work_layer->height-1;
			}
			if(start_j < 0)
			{
				start_j = 0;
			}
			if(end_j > window->work_layer->width)
			{
				end_j = window->work_layer->width;
			}
			num_samples = (end_i-i)*(end_j-start_j);

			for(counter_x=0; counter_x<width; counter_x++, start_j++, end_j++)
			{
				temp_pixel = &window->temp_layer->pixels[counter_y*temp_stride+counter_x*5];
				temp_pixel[0] = sum_color[0] / num_samples;
				temp_pixel[1] = sum_color[1] / num_samples;
				temp_pixel[2] = sum_color[2] / num_samples;
				temp_pixel[3] = sum_color[3] / num_samples;
				
				for(j=i; j<end_i; j++)
				{
					add_pixel = &work_pixels[j*stride+start_j*4];
					sum_color[0] -= add_pixel[0];
					sum_color[1] -= add_pixel[1];
					sum_color[2] -= add_pixel[2];
					sum_color[3] -= add_pixel[3];
					
				}
				for(j=i; j<end_i; j++)
				{
					add_pixel = &work_pixels[j*stride+(end_j)*4];
					sum_color[0] += add_pixel[0];
					sum_color[1] += add_pixel[1];
					sum_color[2] += add_pixel[2];
					sum_color[3] += add_pixel[3];
				}
			}

			start_j = start_x-extends, end_j = start_x+extends;
			sum_color[0] = before_y_color[0];
			sum_color[1] = before_y_color[1];
			sum_color[2] = before_y_color[2];
			sum_color[3] = before_y_color[3];
			for(j=start_j; j<end_j; j++)
			{
				add_pixel = &work_pixels[i*stride+j*4];
				sum_color[0] -= add_pixel[0];
				sum_color[1] -= add_pixel[1];
				sum_color[2] -= add_pixel[2];
				sum_color[3] -= add_pixel[3];
			}
				
			for(j=start_j; j<end_j; j++)
			{
				add_pixel = &work_pixels[(end_i)*stride+j*4];
				sum_color[0] += add_pixel[0];
				sum_color[1] += add_pixel[1];
				sum_color[2] += add_pixel[2];
				sum_color[3] += add_pixel[3];
			}
			before_y_color[0] = sum_color[0];
			before_y_color[1] = sum_color[1];
			before_y_color[2] = sum_color[2];
			before_y_color[3] = sum_color[3];
		}

		for(counter_y=0; counter_y<height; counter_y++)
		{
			for(counter_x=0; counter_x<width; counter_x++)
			{
				temp_pixel = &window->temp_layer->pixels[counter_y*temp_stride+counter_x*5];
				t = temp_pixel[4];
				add_pixel = &work_pixels[stride*(counter_y+start_y)+(counter_x+start_x)*4];

				add_pixel[0] = ((0xff - t) * add_pixel[0] + t * temp_pixel[0]) / 255;
				add_pixel[1] = ((0xff - t) * add_pixel[1] + t * temp_pixel[1]) / 255;
				add_pixel[2] = ((0xff - t) * add_pixel[2] + t * temp_pixel[2]) / 255;
				add_pixel[3] = ((0xff - t) * add_pixel[3] + t * temp_pixel[3]) / 255;
			}
		}
	}
}

static void BlurToolEditSelectionPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		BLUR_TOOL* blur = (BLUR_TOOL*)core->brush_data;
		gdouble r, alpha;
		gdouble zoom;
		cairo_matrix_t matrix;
		int extends;
		int num_samples;
		gdouble min_x, min_y, max_x, max_y;
		int sum_color[2], before_y_color[2];
		int stride = window->work_layer->width;
		uint8* work_pixels = window->work_layer->pixels;
		uint8 *add_pixel, *temp_pixel, *mask;
		uint8 t;
		int counter_x, counter_y, start_x, start_y, width, height;
		int start_j, end_i, end_j;
		int temp_stride;
		int i, j;

		window->selection->layer_mode = SELECTION_BLEND_COPY;

		(void)memcpy(window->work_layer->pixels, window->selection->pixels,
			window->width * window->height);

		if((blur->flags & BRUSH_FLAG_SIZE) != 0)
		{
			r = blur->r * pressure;
			zoom = 1/pressure;
		}
		else
		{
			r = blur->r;
			zoom = 1;
		}
		alpha = ((blur->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;
		extends = (int)(r * (blur->color_extend * 0.01))/2;
		if(extends == 0)
		{
			return;
		}

		min_x = x - r, min_y = y - r;
		max_x = x + r, max_y = y + r;

		blur->before_x = x, blur->before_y = y;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;
		start_x = (int32)core->min_x, start_y = (int32)core->min_y;
		width = (int32)(core->max_x-core->min_x);
		height = (int32)(core->max_y-core->min_y);
		temp_stride = width*2;

		for(counter_y=0; counter_y<height; counter_y++)
		{
			(void)memset(&window->mask->pixels[(start_y+counter_y)*window->width
				+ start_x], 0, width);
		}

		cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
		mask = window->mask->pixels;
		cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_matrix_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(window->alpha_cairo, core->brush_pattern);
				cairo_paint_with_alpha(window->alpha_cairo, alpha);
			}
			else
			{
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(window->alpha_cairo, core->temp_pattern);
				cairo_mask_surface(window->alpha_cairo,
					window->active_layer->surface_p, 0, 0);
			}
		}
		else
		{
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(window->alpha_cairo, core->temp_pattern);
				cairo_mask_surface(window->alpha_cairo,
					window->selection->surface_p, 0, 0);
			}
			else
			{
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, - x + r, - y + r);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(window->alpha_cairo, core->temp_pattern);

				for(counter_y=0; counter_y<height; counter_y++)
				{
					(void)memset(&window->mask_temp->pixels[(start_y+counter_y)*window->width
						+ start_x], 0, width);
				}

				cairo_mask_surface(window->alpha_cairo,
					window->selection->surface_p, 0, 0);
				cairo_set_source_surface(window->gray_mask_cairo,
					window->alpha_surface, 0, 0);
				cairo_mask_surface(window->gray_mask_cairo,
					window->active_layer->surface_p, 0, 0);

				mask = window->mask_temp->pixels;
			}
		}

		for(counter_y=0; counter_y<height; counter_y++)
		{
			for(counter_x=0; counter_x<width; counter_x++)
			{
				window->temp_layer->pixels[temp_stride*counter_y+counter_x*2+1] =
					mask[window->width*(counter_y+start_y)+(counter_x+start_x)];
			}
		}

		i = start_y-extends, start_j=start_x-extends;
		end_i = start_y+extends, end_j=start_x+extends;
		if(i < 0)
		{
			i = 0;
		}
		if(start_j < 0)
		{
			start_j = 0;
		}
		if(end_i > window->work_layer->height)
		{
			if(i >= window->work_layer->height-1)
			{
				return;
			}
			end_i = window->work_layer->height-1;
		}
		if(end_j > window->work_layer->width)
		{
			if(start_j >= window->work_layer->width)
			{
				return;
			}
			end_j = window->work_layer->width;
		}

		sum_color[0] = 0;
		for( ; i<end_i; i++)
		{
			for(j=start_j; j<end_j; j++)
			{
				add_pixel = &work_pixels[i*stride+j];
				sum_color[0] += add_pixel[0];
			}
		}
		before_y_color[0] = sum_color[0];

		for(counter_y=0; counter_y<height; counter_y++)
		{
			start_j = start_x-extends, end_j = start_x+extends;
			i = counter_y+start_y-extends, end_i = counter_y+start_y+extends;
			if(i < 0)
			{
				i = 0;
			}
			if(end_i > window->work_layer->height)
			{
				end_i = window->work_layer->height;
			}
			if(start_j < 0)
			{
				start_j = 0;
			}
			if(end_j > window->work_layer->width)
			{
				end_j = window->work_layer->width;
			}
			num_samples = (end_i-i)*(end_j-start_j);

			for(counter_x=0; counter_x<width; counter_x++, start_j++, end_j++)
			{
				temp_pixel = &window->temp_layer->pixels[counter_y*temp_stride+counter_x*2];
				temp_pixel[0] = sum_color[0] / num_samples;
				
				for(j=i; j<end_i; j++)
				{
					add_pixel = &work_pixels[j*stride+start_j];
					sum_color[0] -= add_pixel[0];
					
				}
				for(j=i; j<end_i; j++)
				{
					add_pixel = &work_pixels[j*stride+(end_j)];
					sum_color[0] += add_pixel[0];
				}
			}

			start_j = start_x-extends, end_j = start_x+extends;
			sum_color[0] = before_y_color[0];
			for(j=start_j; j<end_j; j++)
			{
				add_pixel = &work_pixels[i*stride+j];
				sum_color[0] -= add_pixel[0];
			}
				
			for(j=start_j; j<end_j; j++)
			{
				add_pixel = &work_pixels[(end_i)*stride+j];
				sum_color[0] += add_pixel[0];
			}
			before_y_color[0] = sum_color[0];
		}

		for(counter_y=0; counter_y<height; counter_y++)
		{
			for(counter_x=0; counter_x<width; counter_x++)
			{
				temp_pixel = &window->temp_layer->pixels[counter_y*temp_stride+counter_x*2];
				t = temp_pixel[1];
				add_pixel = &work_pixels[stride*(counter_y+start_y)+(counter_x+start_x)];

				add_pixel[0] = ((0xff - t) * add_pixel[0] + t * temp_pixel[0]) / 255;
			}
		}
	}
}

static void BlurToolMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		BLUR_TOOL* blur = (BLUR_TOOL*)core->brush_data;
		gdouble r, alpha, d, step;
		gdouble zoom;
		cairo_matrix_t matrix;
		gdouble min_x, min_y, max_x, max_y;
		gdouble draw_x = blur->before_x, draw_y = blur->before_y;
		gdouble dx, dy, cos_x, sin_y;
		gdouble hardness = blur->outline_hardness * 0.01f;
		int32 clear_x, clear_width, clear_y, clear_height;
		int sum_color[4];
		int before_y_color[4];
		int extends, num_samples;
		uint8* work_pixels = window->work_layer->pixels;
		uint8* mask;
		uint8* add_pixel;
		uint8* temp_pixel;
		uint8 alpha_c, t;
		int stride = window->work_layer->stride;
		int start_j, end_i, end_j;
		int i, j, counter_x, counter_y;
		int temp_stride;

		if((blur->flags & BRUSH_FLAG_SIZE) != 0)
		{
			r = blur->r * pressure;
			zoom = 1/pressure;
		}
		else
		{
			r = blur->r;
			zoom = 1;
		}
		extends = (int)(r * (blur->color_extend*0.01f))/2;
		if(extends == 0)
		{
			return;
		}
		
		dx = x-blur->before_x, dy = y-blur->before_y;
		d = sqrt(dx*dx+dy*dy);
		step = r * BRUSH_STEP;
		if(step < 1)
		{
			step = 1;
		}

		alpha = ((blur->flags & BRUSH_FLAG_FLOW) == 0) ? 1 : pressure;
		alpha_c = (uint8)(alpha * 255);
		cos_x = step * dx / d, sin_y = step * dy / d;

		min_x = x - r*2, min_y = y - r*2;
		max_x = x + r*2, max_y = y + r*2;

		if(min_x < 0.0)
		{
			return;
			//min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			return;
			//min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			return;
			//max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			return;
			//max_y = window->work_layer->height;
		}

		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		if(window->update.x > min_x)
		{
			window->update.width += window->update.x - (int)min_x;
			window->update.x = (int)min_x;
		}
		if(window->update.width + window->update.x < max_x)
		{
			window->update.width += (int)max_x - window->update.width + window->update.x;
		}
		if(window->update.y > min_y)
		{
			window->update.height += window->update.y - (int)min_y;
			window->update.y = (int)min_y;
		}
		if(window->update.height + window->update.y < max_y)
		{
			window->update.height += (int)max_y - window->update.height + window->update.y;
		}
		window->flags |= DRAW_WINDOW_UPDATE_PART;

		cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
		(void)memcpy(window->mask_temp->pixels, work_pixels, window->pixel_buf_size);
		cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
		dx = d;

		do
		{
			clear_x = (int32)(draw_x - r - 1);
			clear_width = (int32)(draw_x + r + 1);
			clear_y = (int32)(draw_y - r - 1);
			clear_height = (int32)(draw_y + r + 1);

			if(clear_x < 0)
			{
				clear_x = 0;
			}
			else if(clear_x >= window->width)
			{
				goto skip_draw;
			}
			if(clear_y < 0)
			{
				clear_y = 0;
			}
			if(clear_height >= window->height)
			{
				clear_height = window->height;
			}
			clear_width = clear_width - clear_x;
			clear_height = clear_height - clear_y;
			temp_stride = clear_width*5;

			if(clear_width <= 0 || clear_height <= 0)
			{
				goto skip_draw;
			}

			for(counter_y=0; counter_y<clear_height; counter_y++)
			{
				(void)memset(&window->mask->pixels[(clear_y+counter_y)*window->width
					+ clear_x], 0, clear_width);
			}

			cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			mask = window->mask->pixels;
			cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_matrix_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->brush_pattern);
					cairo_paint_with_alpha(window->alpha_cairo, alpha);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->temp_pattern);
					cairo_mask_surface(window->alpha_cairo,
						window->active_layer->surface_p, 0, 0);
				}
			}
			else
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->temp_pattern);
					cairo_mask_surface(window->alpha_cairo,
						window->selection->surface_p, 0, 0);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->temp_pattern);

					for(counter_y=0; counter_y<clear_height; counter_y++)
					{
						(void)memset(&window->mask_temp->pixels[(clear_y+counter_y)*window->width
							+ clear_x], 0, clear_width);
					}

					cairo_mask_surface(window->alpha_cairo,
						window->selection->surface_p, 0, 0);
					cairo_set_source_surface(window->gray_mask_cairo,
						window->alpha_surface, 0, 0);
					cairo_mask_surface(window->gray_mask_cairo,
						window->active_layer->surface_p, 0, 0);

					mask = window->mask_temp->pixels;
				}
			}

			for(counter_y=0; counter_y<clear_height; counter_y++)
			{
				for(counter_x=0; counter_x<clear_width; counter_x++)
				{
					window->temp_layer->pixels[temp_stride*counter_y+counter_x*5+4] =
						mask[window->width*(counter_y+clear_y)+(counter_x+clear_x)];
				}
			}

			i = clear_y-extends, start_j=clear_x-extends;
			end_i = clear_y+extends, end_j=clear_x+extends;
			if(i < 0)
			{
				i = 0;
			}
			if(start_j < 0)
			{
				start_j = 0;
			}
			if(end_i > window->work_layer->height)
			{
				end_i = window->work_layer->height;
			}
			if(end_j > window->work_layer->width)
			{
				end_j = window->work_layer->width;
			}

			sum_color[0] = sum_color[1] = sum_color[2] = sum_color[3] = 0;
			for( ; i<end_i; i++)
			{
				for(j=start_j; j<end_j; j++)
				{
					add_pixel = &work_pixels[i*stride+j*4];
					sum_color[0] += add_pixel[0];
					sum_color[1] += add_pixel[1];
					sum_color[2] += add_pixel[2];
					sum_color[3] += add_pixel[3];
				}
			}
			before_y_color[0] = sum_color[0];
			before_y_color[1] = sum_color[1];
			before_y_color[2] = sum_color[2];
			before_y_color[3] = sum_color[3];

			for(counter_y=0; counter_y<clear_height; counter_y++)
			{
				start_j = clear_x-extends, end_j = clear_x+extends;
				i = counter_y+clear_y-extends, end_i = counter_y+clear_y+extends;
				if(i < 0)
				{
					i = 0;
				}
				if(end_i >= window->work_layer->height)
				{
					if(i >= window->work_layer->height-1)
					{
						return;
					}
					end_i = window->work_layer->height-1;
				}
				if(start_j < 0)
				{
					start_j = 0;
				}
				if(end_j > window->work_layer->width)
				{
					end_j = window->work_layer->width;
				}
				num_samples = (end_i-i)*(end_j-start_j);

				for(counter_x=0; counter_x<clear_width; counter_x++, start_j++, end_j++)
				{
					add_pixel = &window->temp_layer->pixels[counter_y*temp_stride+counter_x*5];
					add_pixel[0] = sum_color[0] / num_samples;
					add_pixel[1] = sum_color[1] / num_samples;
					add_pixel[2] = sum_color[2] / num_samples;
					add_pixel[3] = sum_color[3] / num_samples;

					for(j=i; j<end_i; j++)
					{
						add_pixel = &work_pixels[j*stride+start_j*4];
						sum_color[0] -= add_pixel[0];
						sum_color[1] -= add_pixel[1];
						sum_color[2] -= add_pixel[2];
						sum_color[3] -= add_pixel[3];
					}
					for(j=i; j<end_i; j++)
					{
						add_pixel = &work_pixels[j*stride+(end_j)*4];
						sum_color[0] += add_pixel[0];
						sum_color[1] += add_pixel[1];
						sum_color[2] += add_pixel[2];
						sum_color[3] += add_pixel[3];
					}
				}

				start_j = clear_x-extends, end_j = clear_x+extends;
				sum_color[0] = before_y_color[0];
				sum_color[1] = before_y_color[1];
				sum_color[2] = before_y_color[2];
				sum_color[3] = before_y_color[3];
				for(j=start_j; j<end_j; j++)
				{
					add_pixel = &work_pixels[i*stride+j*4];
					sum_color[0] -= add_pixel[0];
					sum_color[1] -= add_pixel[1];
					sum_color[2] -= add_pixel[2];
					sum_color[3] -= add_pixel[3];
				}
				
				for(j=start_j; j<end_j; j++)
				{
					add_pixel = &work_pixels[(end_i)*stride+j*4];
					sum_color[0] += add_pixel[0];
					sum_color[1] += add_pixel[1];
					sum_color[2] += add_pixel[2];
					sum_color[3] += add_pixel[3];
				}
				before_y_color[0] = sum_color[0];
				before_y_color[1] = sum_color[1];
				before_y_color[2] = sum_color[2];
				before_y_color[3] = sum_color[3];
			}

			for(counter_y=0; counter_y<clear_height; counter_y++)
			{
				for(counter_x=0; counter_x<clear_width; counter_x++)
				{
					temp_pixel = &window->temp_layer->pixels[counter_y*temp_stride+counter_x*5];
					t = temp_pixel[4];
					add_pixel = &work_pixels[stride*(counter_y+clear_y)+(counter_x+clear_x)*4];

					add_pixel[0] = ((0xff- t + 1) * add_pixel[0] + t * temp_pixel[0]) >> 8;
					add_pixel[1] = ((0xff- t + 1) * add_pixel[1] + t * temp_pixel[1]) >> 8;
					add_pixel[2] = ((0xff- t + 1) * add_pixel[2] + t * temp_pixel[2]) >> 8;
					add_pixel[3] = ((0xff- t + 1) * add_pixel[3] + t * temp_pixel[3]) >> 8;
				}
			}

skip_draw:
			dx -= step;
			if(dx < 1)
			{
				break;
			}
			else if(dx >= step)
			{
				draw_x += cos_x, draw_y += sin_y;
			}
			else
			{
				draw_x = x, draw_y = y;
			}
		} while(1);

		blur->before_x = x, blur->before_y = y;
	}
}

static void BlurToolEditSelectionMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		BLUR_TOOL* blur = (BLUR_TOOL*)core->brush_data;
		gdouble r, alpha, d, step;
		gdouble zoom;
		cairo_matrix_t matrix;
		gdouble min_x, min_y, max_x, max_y;
		gdouble draw_x = blur->before_x, draw_y = blur->before_y;
		gdouble dx, dy, cos_x, sin_y;
		gdouble hardness = blur->outline_hardness * 0.01f;
		int32 clear_x, clear_width, clear_y, clear_height;
		int sum_color[2];
		int before_y_color[2];
		int extends, num_samples;
		uint8* work_pixels = window->work_layer->pixels;
		uint8* add_pixel;
		uint8* temp_pixel;
		uint8* mask;
		uint8 alpha_c, t;
		int stride = window->work_layer->width;
		int start_j, end_i, end_j;
		int i, j, counter_x, counter_y;
		int temp_stride;

		if((blur->flags & BRUSH_FLAG_SIZE) != 0)
		{
			r = blur->r * pressure;
			zoom = 1/pressure;
		}
		else
		{
			r = blur->r;
			zoom = 1;
		}
		extends = (int)(r * (blur->color_extend*0.01f))/2;
		if(extends == 0)
		{
			return;
		}
		
		dx = x-blur->before_x, dy = y-blur->before_y;
		d = sqrt(dx*dx+dy*dy);
		step = r * BRUSH_STEP;
		if(step < 1)
		{
			step = 1;
		}

		alpha = ((blur->flags & BRUSH_FLAG_FLOW) == 0) ?
			blur->alpha * 0.01 : blur->alpha * 0.01 * pressure;
		alpha_c = (uint8)(alpha * 255);
		cos_x = step * dx / d, sin_y = step * dy / d;

		min_x = x - r*2, min_y = y - r*2;
		max_x = x + r*2, max_y = y + r*2;

		if(min_x < 0.0)
		{
			return;
			//min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			return;
			//min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			return;
			//max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			return;
			//max_y = window->work_layer->height;
		}

		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
		(void)memcpy(window->mask_temp->pixels, work_pixels, window->pixel_buf_size);
		cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
		dx = d;

		do
		{
			clear_x = (int32)(draw_x - r - 1);
			clear_width = (int32)(draw_x + r + 1);
			clear_y = (int32)(draw_y - r - 1);
			clear_height = (int32)(draw_y + r + 1);

			if(clear_x < 0)
			{
				clear_x = 0;
			}
			else if(clear_x >= window->width)
			{
				goto skip_draw;
			}
			if(clear_y < 0)
			{
				clear_y = 0;
			}
			if(clear_height >= window->height)
			{
				clear_height = window->height;
			}
			clear_width = clear_width - clear_x;
			clear_height = clear_height - clear_y;
			temp_stride = clear_width*2;

			if(clear_width <= 0 || clear_height <= 0)
			{
				goto skip_draw;
			}

			for(counter_y=0; counter_y<clear_height; counter_y++)
			{
				(void)memset(&window->mask->pixels[(clear_y+counter_y)*window->width
					+ clear_x], 0, clear_width);
			}

			cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			mask = window->mask->pixels;
			cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_matrix_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->brush_pattern);
					cairo_paint_with_alpha(window->alpha_cairo, alpha);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->temp_pattern);
					cairo_mask_surface(window->alpha_cairo,
						window->active_layer->surface_p, 0, 0);
				}
			}
			else
			{
				if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->temp_pattern);
					cairo_mask_surface(window->alpha_cairo,
						window->selection->surface_p, 0, 0);
				}
				else
				{
					cairo_matrix_init_scale(&matrix, zoom, zoom);
					cairo_pattern_set_matrix(core->brush_pattern, &matrix);
					cairo_set_source(core->temp_cairo, core->brush_pattern);
					cairo_paint_with_alpha(core->temp_cairo, alpha);
					cairo_matrix_init_translate(&matrix, - x + r, - y + r);
					cairo_pattern_set_matrix(core->temp_pattern, &matrix);
					cairo_set_source(window->alpha_cairo, core->temp_pattern);

					for(counter_y=0; counter_y<clear_height; counter_y++)
					{
						(void)memset(&window->mask_temp->pixels[(clear_y+counter_y)*window->width
							+ clear_x], 0, clear_width);
					}

					cairo_mask_surface(window->alpha_cairo,
						window->selection->surface_p, 0, 0);
					cairo_set_source_surface(window->gray_mask_cairo,
						window->alpha_surface, 0, 0);
					cairo_mask_surface(window->gray_mask_cairo,
						window->active_layer->surface_p, 0, 0);

					mask = window->mask_temp->pixels;
				}
			}

			for(counter_y=0; counter_y<clear_height; counter_y++)
			{
				for(counter_x=0; counter_x<clear_width; counter_x++)
				{
					window->temp_layer->pixels[temp_stride*counter_y+counter_x*2+1] =
						mask[window->width*(counter_y+clear_y)+(counter_x+clear_x)];
				}
			}

			i = clear_y-extends, start_j=clear_x-extends;
			end_i = clear_y+extends, end_j=clear_x+extends;
			if(i < 0)
			{
				i = 0;
			}
			if(start_j < 0)
			{
				start_j = 0;
			}
			if(end_i > window->work_layer->height)
			{
				end_i = window->work_layer->height;
			}
			if(end_j > window->work_layer->width)
			{
				end_j = window->work_layer->width;
			}

			sum_color[0] = 0;
			for( ; i<end_i; i++)
			{
				for(j=start_j; j<end_j; j++)
				{
					add_pixel = &work_pixels[i*stride+j];
					sum_color[0] += add_pixel[0];
				}
			}
			before_y_color[0] = sum_color[0];

			for(counter_y=0; counter_y<clear_height; counter_y++)
			{
				start_j = clear_x-extends, end_j = clear_x+extends;
				i = counter_y+clear_y-extends, end_i = counter_y+clear_y+extends;
				if(i < 0)
				{
					i = 0;
				}
				if(end_i > window->work_layer->height)
				{
					end_i = window->work_layer->height;
				}
				if(start_j < 0)
				{
					start_j = 0;
				}
				if(end_j > window->work_layer->width)
				{
					end_j = window->work_layer->width;
				}
				num_samples = (end_i-i)*(end_j-start_j);

				for(counter_x=0; counter_x<clear_width; counter_x++, start_j++, end_j++)
				{
					add_pixel = &window->temp_layer->pixels[counter_y*temp_stride+counter_x*2];
					add_pixel[0] = sum_color[0] / num_samples;

					for(j=i; j<end_i; j++)
					{
						add_pixel = &work_pixels[j*stride+start_j];
						sum_color[0] -= add_pixel[0];
					}
					for(j=i; j<end_i; j++)
					{
						add_pixel = &work_pixels[j*stride+(end_j)];
						sum_color[0] += add_pixel[0];
					}
				}

				start_j = clear_x-extends, end_j = clear_x+extends;
				sum_color[0] = before_y_color[0];
				for(j=start_j; j<end_j; j++)
				{
					add_pixel = &work_pixels[i*stride+j];
					sum_color[0] -= add_pixel[0];
				}
				
				for(j=start_j; j<end_j; j++)
				{
					add_pixel = &work_pixels[(end_i)*stride+j];
					sum_color[0] += add_pixel[0];
				}
				before_y_color[0] = sum_color[0];
			}

			for(counter_y=0; counter_y<clear_height; counter_y++)
			{
				for(counter_x=0; counter_x<clear_width; counter_x++)
				{
					temp_pixel = &window->temp_layer->pixels[counter_y*temp_stride+counter_x*2];
					t = temp_pixel[1];
					add_pixel = &work_pixels[stride*(counter_y+clear_y)+(counter_x+clear_x)];

					add_pixel[0] = ((0xff- t + 1) * add_pixel[0] + t * temp_pixel[0]) >> 8;
				}
			}

skip_draw:
			dx -= step;
			if(dx < 1)
			{
				break;
			}
			else if(dx >= step)
			{
				draw_x += cos_x, draw_y += sin_y;
			}
			else
			{
				draw_x = x, draw_y = y;
			}
		} while(1);

		blur->before_x = x, blur->before_y = y;
	}
}

#define BlurToolReleaseCallBack DefaultReleaseCallBack
#define BlurToolEditSelectionReleaseCallBack DefaultEditSelectionReleaseCallBack

static void BlurToolDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	BLUR_TOOL* blur = (BLUR_TOOL*)data;
	gdouble r = blur->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void BlurToolButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, BLUR_TOOL* blur)
{
	gdouble r = blur->r * window->zoom_rate + 3;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2 + 3), (gint)(r * 2 + 3));
}

static void BlurToolMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, BLUR_TOOL* blur)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = blur->r * window->zoom_rate * 1.275 + BRUSH_UPDATE_MARGIN;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width + 2, (gint)height + 2);
}

static void BlurToolScaleChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	BLUR_TOOL* blur = (BLUR_TOOL*)data;
	blur->r = gtk_adjustment_get_value(slider) * 0.5;
	BrushCoreSetGrayCirclePattern(blur->core, blur->r, blur->outline_hardness*0.01,
		blur->blur*0.01, blur->alpha*0.01);
}

static void BlurToolFlowChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	BLUR_TOOL* blur = (BLUR_TOOL*)data;
	blur->alpha = gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(blur->core, blur->r, blur->outline_hardness*0.01,
		blur->blur*0.01, blur->alpha*0.01);
}

static void BlurToolPressureSizeChange(
	GtkWidget* widget,
	gpointer data
)
{
	BLUR_TOOL* blur = (BLUR_TOOL*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		blur->flags &= ~(BRUSH_FLAG_SIZE);
	}
	else
	{
		blur->flags |= BRUSH_FLAG_SIZE;
	}
}

static void BlurToolOutlineHardnessChange(
	GtkAdjustment* slider,
	BLUR_TOOL* blur
)
{
	blur->outline_hardness =
		gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(blur->core, blur->r, blur->outline_hardness*0.01,
		blur->blur*0.01, blur->alpha*0.01);
}

static void BlurToolBlurChange(
	GtkAdjustment* slider,
	BLUR_TOOL* blur
)
{
	blur->blur =
		gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(blur->core, blur->r, blur->outline_hardness*0.01,
		blur->blur*0.01, blur->alpha*0.01);
}

static void BlurToolColorExtendsChange(
	GtkAdjustment* slider,
	BLUR_TOOL* blur
)
{
	blur->color_extend =
		gtk_adjustment_get_value(slider);
}

static void BlurToolPressureFlowChange(
	GtkWidget* widget,
	gpointer data
)
{
	BLUR_TOOL* blur = (BLUR_TOOL*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		blur->flags &= ~(BRUSH_FLAG_FLOW);
	}
	else
	{
		blur->flags |= BRUSH_FLAG_FLOW;
	}
}

static GtkWidget* CreateBlurToolDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	BLUR_TOOL* blur = (BLUR_TOOL*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* hbox;
	GtkWidget* base_scale;
	GtkWidget* brush_scale;
	GtkWidget* table = gtk_table_new(5, 3, TRUE);
	GtkWidget* check_button;
	GtkWidget* label;
	GtkAdjustment* brush_scale_adjustment;
	char mark_up_buff[256];

	blur->core = core;

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		base_scale = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[2]);
#else
		base_scale = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), blur->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(blur->base_scale)
	{
	case 0:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(blur->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			blur->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(blur->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(BlurToolScaleChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);

	g_object_set_data(G_OBJECT(base_scale), "scale", brush_scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &blur->base_scale);

	brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(blur->alpha, 0.0, 100.0, 1.0, 1.0, 0.0));
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.flow, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(BlurToolFlowChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 1, 2);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(blur->outline_hardness,
		0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(BlurToolOutlineHardnessChange), core->brush_data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.outline_hardness, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 2, 3);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(blur->blur,
		0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(BlurToolBlurChange), core->brush_data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.blur, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 3, 4);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(blur->color_extend,
		0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(BlurToolColorExtendsChange), core->brush_data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.color_extend, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 4, 5);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(BlurToolPressureSizeChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), blur->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(BlurToolPressureFlowChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), blur->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	return vbox;
}

static void SmudgePressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		SMUDGE* smudge = (SMUDGE*)core->brush_data;
		int start_x, start_y, width, height;
		int counter_y;

		smudge->before_r = ((smudge->flags & SMUDGE_PRESSURE_SIZE) != 0)
			? smudge->r * pressure : smudge->r;

		smudge->before_x = x;
		smudge->before_y = y;

		core->max_x = x + smudge->before_r + 1;
		core->max_y = y + smudge->before_r + 1;
		core->min_x = x - smudge->before_r - 1;
		core->min_y = y - smudge->before_r - 1;

		if(core->max_x >= window->width)
		{
			core->max_x = window->width;
		}
		if(core->max_y >= window->height)
		{
			core->max_y = window->height;
		}
		if(core->min_x < 0)
		{
			core->min_x = 0;
		}
		if(core->min_y < 0)
		{
			core->min_y = 0;
		}

		window->update.x = (int)core->min_x, window->update.y = (int)core->min_y;
		window->update.width = (int)core->max_x - window->update.x;
		window->update.height = (int)core->max_y - window->update.y;

		window->flags |= DRAW_WINDOW_UPDATE_PART;

		start_x = (int)core->min_x;
		start_y = (int)core->min_y;
		width = (int)(core->max_x - core->min_x);
		height = (int)(core->max_y -core->min_y);

		smudge->before_r = height;

		if(width <= 0 || height <= 0)
		{
			return;
		}

		(void)memcpy(window->work_layer->pixels, window->active_layer->pixels, window->pixel_buf_size);
		window->work_layer->layer_mode = LAYER_BLEND_SOURCE;

		for(counter_y=0; counter_y<height; counter_y++)
		{
			(void)memcpy(&window->brush_buffer[counter_y*width*4],
				&window->work_layer->pixels[(start_y+counter_y)*window->work_layer->stride+start_x*4], width*4
			);
		}

		smudge->flags |= SMUDGE_INITIALIZED;
		smudge->flags &= ~(SMUDGE_DRAW_STARTED);
	}
}

static void SmudgeEditSelectionPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		SMUDGE* smudge = (SMUDGE*)core->brush_data;
		int start_x, start_y, width, height;
		int counter_y;

		smudge->before_r = ((smudge->flags & SMUDGE_PRESSURE_SIZE) != 0)
			? smudge->r * pressure : smudge->r;

		smudge->before_x = x;
		smudge->before_y = y;

		core->max_x = x + smudge->before_r + 1;
		core->max_y = y + smudge->before_r + 1;
		core->min_x = x - smudge->before_r - 1;
		core->min_y = y - smudge->before_r - 1;

		if(core->max_x >= window->width)
		{
			core->max_x = window->width;
		}
		if(core->max_y >= window->height)
		{
			core->max_y = window->height;
		}
		if(core->min_x < 0)
		{
			core->min_x = 0;
		}
		if(core->min_y < 0)
		{
			core->min_y = 0;
		}

		start_x = (int)core->min_x;
		start_y = (int)core->min_y;
		width = (int)(core->max_x - core->min_x);
		height = (int)(core->max_y -core->min_y);

		smudge->before_r = height;

		if(width <= 0 || height <= 0)
		{
			return;
		}

		(void)memcpy(window->work_layer->pixels, window->selection->pixels, window->width * window->height);
		window->selection->layer_mode = SELECTION_BLEND_COPY;

		for(counter_y=0; counter_y<height; counter_y++)
		{
			(void)memcpy(&window->brush_buffer[counter_y*width*4],
				&window->work_layer->pixels[(start_y+counter_y)*window->work_layer->width*4+start_x*4], width*4
			);
		}

		smudge->flags |= SMUDGE_INITIALIZED;
		smudge->flags &= ~(SMUDGE_DRAW_STARTED);
	}
}

static void SmudgeMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	SMUDGE* smudge = (SMUDGE*)core->brush_data;

	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		cairo_t *update;
		cairo_surface_t *update_surface;
		cairo_matrix_t matrix;
		gdouble r, alpha, d;
		gdouble min_x, min_y, max_x, max_y;
		gdouble draw_x, draw_y;
		gdouble dx, dy, cos_x, sin_y;
		gdouble zoom;
		gdouble hardness = smudge->outline_hardness * 0.01f;
		int before_size = (int)smudge->before_r;
		int32 clear_x, clear_width, clear_y, clear_height;
		int i, j, k, t, /*c,*/ index;
		FLOAT_T c;
		uint8 alpha_c, extention = (uint8)(smudge->extention*0.01*255);
		uint8 *mask;

		r = smudge->r;

		if((smudge->flags & SMUDGE_PRESSURE_SIZE) != 0)
		{
			r *= pressure;
			zoom = 1 / pressure;
		}
		else
		{
			zoom = 1;
		}

		alpha = ((smudge->flags & SMUDGE_PRESSURE_EXTENTION) == 0) ?
			1 : 0.01f * pressure;
		alpha_c = (uint8)(alpha * 255);
		dx = x-smudge->before_x, dy = y-smudge->before_y;
		d = dx*dx+dy*dy;
		d = sqrt(d);

		if((smudge->flags & SMUDGE_DRAW_STARTED) == 0)
		{
			smudge->before_r = ((smudge->flags & SMUDGE_PRESSURE_SIZE) != 0)
				? smudge->r * pressure : smudge->r;

			if(d >= r * 0.25)
			{
				smudge->before_x = draw_x = x;
				smudge->before_y = draw_y = y;
				smudge->flags |= SMUDGE_DRAW_STARTED;

				clear_x = (int32)(draw_x - r - 1);
				clear_width = (int32)(draw_x + r + 1);
				clear_y = (int32)(draw_y - r - 1);
				clear_height = (int32)(draw_y + r + 1);

				if(clear_x < 0)
				{
					clear_x = 0;
				}
				else if(clear_width > window->width)
				{
					clear_width = window->width;
				}
				else if(clear_width < 0)
				{
					clear_width = 0;
				}
				if(clear_y < 0)
				{
					clear_y = 0;
				}
				if(clear_height >= window->height)
				{
					clear_height = window->height;
				}
				clear_width = clear_width - clear_x;
				clear_height = clear_height - clear_y;

				for(i=0; i<clear_height; i++)
				{
					(void)memcpy(&window->brush_buffer[i*before_size*4],
						&window->work_layer->pixels[(i+clear_y)*window->work_layer->stride+clear_x*4], before_size*4);
				}
			}
			
			smudge->before_r *= 2;

			return;
		}

		if(fabs(dx) < 2 && fabs(dy) < 2)
		{
			return;
		}
		cos_x = dx / d, sin_y = dy / d;
		draw_x = smudge->before_x + cos_x, draw_y = smudge->before_y + sin_y;

		min_x = x - r*2, min_y = y - r*2;
		max_x = x + r*2, max_y = y + r*2;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		if(window->update.x > min_x)
		{
			window->update.width += window->update.x - (int)min_x;
			window->update.x = (int)min_x;
		}
		if(window->update.width + window->update.x < max_x)
		{
			window->update.width += (int)max_x - window->update.width + window->update.x;
		}
		if(window->update.y > min_y)
		{
			window->update.height += window->update.y - (int)min_y;
			window->update.y = (int)min_y;
		}
		if(window->update.height + window->update.y < max_y)
		{
			window->update.height += (int)max_y - window->update.height + window->update.y;
		}

		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		if((smudge->flags & SMUDGE_INITIALIZED) != 0)
		{
			dx = d;
			do
			{
				if(fabs(draw_x - smudge->before_x) >= 1 || fabs(draw_y - smudge->before_y) >= 1)
				{
					smudge->before_x = draw_x, smudge->before_y = draw_y;
					clear_x = (int32)(draw_x - r - 1);
					clear_width = (int32)(draw_x + r + 1);
					clear_y = (int32)(draw_y - r - 1);
					clear_height = (int32)(draw_y + r + 1);

					if(clear_x < 0)
					{
						clear_x = 0;
					}
					else if(clear_x >= window->width)
					{
						goto skip_draw;
					}
					if(clear_y < 0)
					{
						clear_y = 0;
					}
					if(clear_height >= window->height)
					{
						goto skip_draw;
						//clear_height = window->height;
					}
					clear_width = clear_width - clear_x;
					clear_height = clear_height - clear_y;

					if(clear_width <= 0 || clear_height <= 0)
					{
						goto skip_draw;
					}

					for(i=0; i<clear_height; i++)
					{
						(void)memset(&window->temp_layer->pixels[(i+clear_y)*window->work_layer->stride+clear_x*window->work_layer->channel],
							0x0, clear_width*window->work_layer->channel);
					}

					update_surface = cairo_surface_create_for_rectangle(
						window->alpha_surface, draw_x - r, draw_y - r, r*2+1, r*2+1);
					update = cairo_create(update_surface);

					mask = window->mask->pixels;
					cairo_set_operator(window->alpha_cairo, CAIRO_OPERATOR_OVER);
					if(window->app->textures.active_texture == 0)
					{
						if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(update, core->brush_pattern);
								cairo_paint_with_alpha(update, alpha);
							}
							else
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
							}
						}
						else
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);
							}
							else
							{
								cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
									window->alpha_temp, draw_x - r, draw_y - r, r*2+1, r*2+1);
								cairo_t *update_temp = cairo_create(temp_surface);

								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);

								(void)memset(window->temp_layer->pixels, 0,
									window->width*window->height);

								cairo_mask_surface(update,
									window->selection->surface_p, 0, 0);
								cairo_set_source_surface(update_temp,
									update_surface, 0, 0);
								cairo_mask_surface(update_temp,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);

								mask = window->temp_layer->pixels;

								cairo_surface_destroy(temp_surface);
								cairo_destroy(update_temp);
							}
						}
					}
					else
					{
						cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
							window->alpha_temp, draw_x - r, draw_y - r, r*2+1, r*2+1);
						cairo_t *update_temp = cairo_create(temp_surface);

						if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(update, core->brush_pattern);
								cairo_paint_with_alpha(update, alpha);
							}
							else
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);
							}

							cairo_set_source_surface(update_temp, update_surface, 0, 0);
							cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

							mask = window->temp_layer->pixels;
						}
						else
						{
							if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);
								cairo_mask_surface(update,
									window->selection->surface_p, - draw_x + r, - draw_y + r);

								cairo_set_source_surface(update_temp, update_surface, 0, 0);
								cairo_mask_surface(update_temp, window->texture->surface_p, - draw_x + r, - draw_y + r);

								mask = window->temp_layer->pixels;
							}
							else
							{
								cairo_matrix_init_scale(&matrix, zoom, zoom);
								cairo_pattern_set_matrix(core->brush_pattern, &matrix);
								cairo_set_source(core->temp_cairo, core->brush_pattern);
								cairo_paint_with_alpha(core->temp_cairo, alpha);
								cairo_matrix_init_translate(&matrix, 0, 0);
								cairo_pattern_set_matrix(core->temp_pattern, &matrix);
								cairo_set_source(update, core->temp_pattern);

								(void)memset(window->temp_layer->pixels, 0,
									window->width*window->height);

								cairo_mask_surface(update,
									window->selection->surface_p, 0, 0);
								cairo_set_source_surface(update_temp,
									update_surface, 0, 0);
								cairo_mask_surface(update_temp,
									window->active_layer->surface_p, - draw_x + r, - draw_y + r);

								cairo_set_operator(update, CAIRO_OPERATOR_SOURCE);
								cairo_set_source_surface(update, temp_surface, 0, 0);
								cairo_mask_surface(update, window->texture->surface_p, - draw_x + r, - draw_y + r);
							}
						}

						cairo_surface_destroy(temp_surface);
						cairo_destroy(update_temp);
					}

					cairo_surface_destroy(update_surface);
					cairo_destroy(update);

					for(i=0; i<before_size; i++)
					{
						for(j=0; j<before_size; j++)
						{
							uint8 before_alpha;
							index = (clear_y+i)*window->temp_layer->stride+(clear_x+j)*4;
							t = mask[window->width*(clear_y+i)+clear_x+j];
							before_alpha = window->work_layer->pixels[index+3];
							for(k=0; k<4; k++)
							{
								c = ((0xff-extention)*window->work_layer->pixels[index+k]
								+ extention*window->brush_buffer[i*before_size*4+j*4+k]) / 255.0 + 0.49;
								window->work_layer->pixels[index+k] =
									(uint8)(((0xff-t)*window->work_layer->pixels[index+k]
										+t*c) / 255);
							}
						}
					}

					for(i=0; i<clear_height; i++)
					{
						(void)memcpy(&window->brush_buffer[i*before_size*4],
							&window->work_layer->pixels[(i+clear_y)*window->work_layer->stride+clear_x*4], before_size*4);
					}
				}
skip_draw:
				dx -= 1;
				if(dx < 1)
				{
					break;
				}
				else
				{
					draw_x += cos_x, draw_y += sin_y;
				}
			} while(1);
		}
		else
		{
			draw_x = x, draw_y = y;
		}

		window->flags |= DRAW_WINDOW_UPDATE_PART;

		smudge->before_r = ((smudge->flags & SMUDGE_PRESSURE_SIZE) != 0)
			? smudge->r * pressure : smudge->r;

		clear_x = (int32)(draw_x - smudge->before_r);
		clear_width = (int32)(draw_x + smudge->before_r);
		clear_y = (int32)(draw_y - smudge->before_r);
		clear_height = (int32)(draw_y + smudge->before_r);

		smudge->before_r *= 2;

		if(clear_x < 0)
		{
			clear_x = 0;
		}
		else if(clear_width > window->width)
		{
			clear_width = window->width;
		}
		else if(clear_width < 0)
		{
			clear_width = 0;
		}
		if(clear_y < 0)
		{
			clear_y = 0;
		}
		if(clear_height >= window->height)
		{
			clear_height = window->height;
		}
		clear_width = clear_width - clear_x;
		clear_height = clear_height - clear_y;

		if((smudge->flags & SMUDGE_INITIALIZED) == 0
			&& clear_width > 0 && clear_height > 0)
		{
			for(i=0; i<clear_height; i++)
			{
				(void)memcpy(&window->brush_buffer[i*before_size*4],
					&window->work_layer->pixels[(i+clear_y)*window->work_layer->width+clear_x*4], before_size*4);
			}

			smudge->flags |= SMUDGE_INITIALIZED;
		}
	}
}

static void SmudgeEditSelectionMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	SMUDGE* smudge = (SMUDGE*)core->brush_data;

	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		cairo_pattern_t* brush;
		gdouble r, alpha, d;
		gdouble min_x, min_y, max_x, max_y;
		gdouble draw_x, draw_y;
		gdouble dx, dy, cos_x, sin_y;
		gdouble hardness = smudge->outline_hardness * 0.01f;
		int before_size = (int)smudge->before_r;
		int32 clear_x, clear_width, clear_y, clear_height;
		int i, j, t, c, index;
		uint8 alpha_c, extention = (uint8)(smudge->extention*0.01*255);

		r = smudge->r;

		alpha = ((smudge->flags & SMUDGE_PRESSURE_EXTENTION) == 0) ?
			1 : 0.01f * pressure;
		alpha_c = (uint8)(alpha * 255);
		dx = x-smudge->before_x, dy = y-smudge->before_y;
		d = dx*dx+dy*dy;
		if(fabs(dx) < 2 && fabs(dy) < 2)
		{
			return;
		}
		d = sqrt(d);
		// arg = atan2(dy, dx);
		cos_x = dx / d, sin_y = dy / d;
		draw_x = smudge->before_x + cos_x, draw_y = smudge->before_y + sin_y;
		//cos_x = cos(arg), sin_y = sin(arg);

		min_x = x - r*2, min_y = y - r*2;
		max_x = x + r*2, max_y = y + r*2;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		if((smudge->flags & SMUDGE_INITIALIZED) != 0)
		{
			dx = d;
			do
			{
				if(fabs(draw_x - smudge->before_x) >= 1 || fabs(draw_y - smudge->before_y) >= 1)
				{
					smudge->before_x = draw_x, smudge->before_y = draw_y;
					clear_x = (int32)(draw_x - r - 1);
					clear_width = (int32)(draw_x + r + 1);
					clear_y = (int32)(draw_y - r - 1);
					clear_height = (int32)(draw_y + r + 1);

					if(clear_x < 0)
					{
						clear_x = 0;
					}
					else if(clear_x >= window->width)
					{
						goto skip_draw;
					}
					if(clear_y < 0)
					{
						clear_y = 0;
					}
					if(clear_height >= window->height)
					{
						goto skip_draw;
						//clear_height = window->height;
					}
					clear_width = clear_width - clear_x;
					clear_height = clear_height - clear_y;

					if(clear_width <= 0 || clear_height <= 0)
					{
						goto skip_draw;
					}

					for(i=0; i<clear_height; i++)
					{
						(void)memset(&window->temp_layer->pixels[(i+clear_y)*window->work_layer->stride+clear_x*window->work_layer->channel],
							0x0, clear_width*window->work_layer->channel);
					}
					brush = cairo_pattern_create_radial(draw_x, draw_y, r * 0.0, draw_x, draw_y, r);
					cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
					cairo_pattern_add_color_stop_rgba(brush, 0.0, 0, 0, 0, alpha);
					cairo_pattern_add_color_stop_rgba(brush, 1-smudge->opacity*0.01f, 0, 0, 0, alpha);
					cairo_pattern_add_color_stop_rgba(brush, 1.0, 0, 0, 0, alpha*hardness);
					cairo_set_source(window->temp_layer->cairo_p, brush);

					cairo_arc(window->temp_layer->cairo_p, draw_x, draw_y, r, 0, 2*G_PI);
					cairo_fill(window->temp_layer->cairo_p);

					for(i=0; i<before_size; i++)
					{
						for(j=0; j<before_size; j++)
						{
							uint8 before_alpha;
							index = (clear_y+i)*window->temp_layer->stride+(clear_x+j)*4;
							t = window->temp_layer->pixels[index+3];
							before_alpha = window->work_layer->pixels[(clear_y+i)*window->width+clear_x+j];

							c = ((0xff-extention+1)*window->work_layer->pixels[(clear_y+i)*window->width+clear_x+j]
								+ extention*window->brush_buffer[i*before_size+j]) >> 8;
							window->work_layer->pixels[(clear_y+i)*window->width+clear_x+j] =
								((0xff-t+1)*window->work_layer->pixels[(clear_y+i)*window->width+clear_x+j]
									+t*c) >> 8;
						}
					}

					for(i=0; i<clear_height; i++)
					{
						(void)memcpy(&window->brush_buffer[i*before_size],
							&window->work_layer->pixels[(i+clear_y)*window->work_layer->width+clear_x], before_size);
					}
				}
skip_draw:
				dx -= 1;
				if(dx < 1)
				{
					break;
				}
				else
				{
					draw_x += cos_x, draw_y += sin_y;
				}
			} while(1);
		}
		else
		{
			draw_x = x, draw_y = y;
		}

		//smudge->before_x = draw_x, smudge->before_y = draw_y;
		smudge->before_r = ((smudge->flags & SMUDGE_PRESSURE_SIZE) != 0)
			? smudge->r * pressure : smudge->r;

		clear_x = (int32)(draw_x - smudge->before_r);
		clear_width = (int32)(draw_x + smudge->before_r);
		clear_y = (int32)(draw_y - smudge->before_r);
		clear_height = (int32)(draw_y + smudge->before_r);

		smudge->before_r *= 2;

		if(clear_x < 0)
		{
			clear_x = 0;
		}
		else if(clear_width > window->width)
		{
			clear_width = window->width;
		}
		else if(clear_width < 0)
		{
			clear_width = 0;
		}
		if(clear_y < 0)
		{
			clear_y = 0;
		}
		if(clear_height >= window->height)
		{
			clear_height = window->height;
		}
		clear_width = clear_width - clear_x;
		clear_height = clear_height - clear_y;

		if((smudge->flags & SMUDGE_INITIALIZED) == 0
			&& clear_width > 0 && clear_height > 0)
		{
			for(i=0; i<clear_height; i++)
			{
				(void)memcpy(&window->brush_buffer[i*before_size],
					&window->work_layer->pixels[(i+clear_y)*window->work_layer->width+clear_x], before_size);
			}

			smudge->flags |= SMUDGE_INITIALIZED;
		}
	}
}

static void SmudgeReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		SMUDGE* smudge = (SMUDGE*)core->brush_data;

		AddBrushHistory(core, window->active_layer);

		window->update.surface_p = cairo_surface_create_for_rectangle(
			window->active_layer->surface_p, window->update.x, window->update.y,
				window->update.width, window->update.height);
		window->update.cairo_p = cairo_create(window->update.surface_p);
		g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->update);
		cairo_destroy(window->update.cairo_p);
		cairo_surface_destroy(window->update.surface_p);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
		window->flags |= DRAW_WINDOW_UPDATE_PART;

		smudge->flags &= ~(SMUDGE_INITIALIZED);
	}
}

static void SmudgeEditSelectionReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		SMUDGE* smudge = (SMUDGE*)core->brush_data;

		AddSelectionEditHistory(core, window->selection);

		g_blend_selection_funcs[window->selection->layer_mode](window->work_layer, window->selection);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		window->selection->layer_mode = SELECTION_BLEND_NORMAL;

		smudge->flags &= ~(SMUDGE_INITIALIZED);
	}
}

static void SmudgeDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	SMUDGE* smudge = (SMUDGE*)data;
	gdouble r = smudge->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void SmudgeButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, SMUDGE* smudge)
{
	gdouble r = smudge->r * window->zoom_rate + 3;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2 + 3), (gint)(r * 2 + 3));
}

static void SmudgeMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, SMUDGE* smudge)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = smudge->r * window->zoom_rate * 1.275 + BRUSH_UPDATE_MARGIN;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = (r + BRUSH_UPDATE_MARGIN) * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = (r + BRUSH_UPDATE_MARGIN) * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = (r + BRUSH_UPDATE_MARGIN) * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = (r + BRUSH_UPDATE_MARGIN) * 2 + window->before_cursor_y - y;
	}

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width, (gint)height);
}

static void SmudgeScaleChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	SMUDGE* smudge = (SMUDGE*)data;
	smudge->r = gtk_adjustment_get_value(slider) * 0.5;
	BrushCoreSetGrayCirclePattern(smudge->core, smudge->r, smudge->outline_hardness * 0.01,
		smudge->blur * 0.01, smudge->opacity * 0.01);
}

static void SmudgeOpacityChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	SMUDGE* smudge = (SMUDGE*)data;
	smudge->opacity = gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(smudge->core, smudge->r, smudge->outline_hardness * 0.01,
		smudge->blur * 0.01, smudge->opacity * 0.01);
}

static void SmudgeBlurChange(
	GtkAdjustment* slider,
	gpointer data
)
{
	SMUDGE* smudge = (SMUDGE*)data;
	smudge->blur = gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(smudge->core, smudge->r, smudge->outline_hardness * 0.01,
		smudge->blur * 0.01, smudge->opacity * 0.01);
}

static void SmudgePressureSizeChange(
	GtkWidget* widget,
	gpointer data
)
{
	SMUDGE* smudge = (SMUDGE*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		smudge->flags &= ~(SMUDGE_PRESSURE_SIZE);
	}
	else
	{
		smudge->flags |= SMUDGE_PRESSURE_SIZE;
	}
}

static void SmudgeOutlineHardnessChange(
	GtkAdjustment* slider,
	SMUDGE* smudge
)
{
	smudge->outline_hardness =
		gtk_adjustment_get_value(slider);
	BrushCoreSetGrayCirclePattern(smudge->core, smudge->r, smudge->outline_hardness * 0.01,
		smudge->blur * 0.01, smudge->opacity * 0.01);
}

static void SmudgeColorExtendsChange(
	GtkAdjustment* slider,
	SMUDGE* smudge
)
{
	smudge->extention =
		gtk_adjustment_get_value(slider);
}

static void SmudgeExtentionChange(
	GtkWidget* widget,
	gpointer data
)
{
	SMUDGE* smudge = (SMUDGE*)data;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		smudge->flags &= ~(SMUDGE_PRESSURE_EXTENTION);
	}
	else
	{
		smudge->flags |= SMUDGE_PRESSURE_EXTENTION;
	}
}

static GtkWidget* CreateSmudgeDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	SMUDGE* smudge = (SMUDGE*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget* hbox;
	GtkWidget* base_scale;
	GtkWidget* brush_scale;
	GtkWidget* table = gtk_table_new(5, 3, TRUE);
	GtkWidget* check_button;
	GtkWidget* label;
	GtkAdjustment* brush_scale_adjustment;
	char mark_up_buff[256];

	smudge->core = core;

	BrushCoreSetGrayCirclePattern(smudge->core, smudge->r, smudge->outline_hardness * 0.01,
		smudge->blur * 0.01, smudge->opacity * 0.01);

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		base_scale = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[2]);
#else
		base_scale = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), smudge->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(smudge->base_scale)
	{
	case 0:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(smudge->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
			smudge->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		brush_scale_adjustment =
			GTK_ADJUSTMENT(gtk_adjustment_new(smudge->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(SmudgeScaleChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 0, 1);

	g_object_set_data(G_OBJECT(base_scale), "scale", brush_scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &smudge->base_scale);

	brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(smudge->opacity, 0.0, 100.0, 1.0, 1.0, 0.0));
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.flow, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(SmudgeOpacityChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 1, 2);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(smudge->outline_hardness,
		0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(SmudgeOutlineHardnessChange), core->brush_data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.outline_hardness, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 2, 3);

	brush_scale_adjustment =
		GTK_ADJUSTMENT(gtk_adjustment_new(smudge->blur, 0.0, 100.0, 1.0, 1.0, 0.0));
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.blur, 1);
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(SmudgeBlurChange), core->brush_data);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 3, 4);

	brush_scale_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(smudge->extention,
		0, 100, 1, 1, 0));
	g_signal_connect(G_OBJECT(brush_scale_adjustment), "value_changed",
		G_CALLBACK(SmudgeColorExtendsChange), core->brush_data);
	brush_scale = SpinScaleNew(brush_scale_adjustment,
		app->labels->tool_box.color_extend, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), brush_scale, 0, 3, 4, 5);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	table = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(table), label, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(SmudgePressureSizeChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), smudge->flags & SMUDGE_PRESSURE_SIZE);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(SmudgeExtentionChange), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), smudge->flags & SMUDGE_PRESSURE_EXTENTION);
	gtk_box_pack_start(GTK_BOX(table), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

	return vbox;
}

static void MixBrushPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ��
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		MIX_BRUSH* mix = (MIX_BRUSH*)core->brush_data;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �u���V�ʒu�̃s�N�Z���l���v
		unsigned int sum_color[5] = {0, 0, 0, 1, 1};
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix;
		// �`�悷��F
		uint8 color[4];
		// ���u�����h�p
		uint8 rev_alpha;
		uint8 blend_alpha;
		// �u���V�p�^�[��
		cairo_pattern_t *brush;
		int i, j;	// for���p�̃J�E���^

		// �A�N�e�B�u���C���[�̓��e����ƃ��C���[�ɃR�s�[
		(void)memcpy(window->work_layer->pixels, window->active_layer->pixels, window->pixel_buf_size);

		// ��ƃ��C���[�̍������[�h���㏑�����[�h��
		window->work_layer->layer_mode = LAYER_BLEND_SOURCE;

		// ���[�N���C���[�ł�CAIRO�̍������[�h��ʏ탂�[�h��
		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);

		// ���a�ƔZ�x���v�Z
		r = ((mix->flags & BRUSH_FLAG_SIZE) == 0) ? mix->r : mix->r * pressure;
		alpha = ((mix->flags & BRUSH_FLAG_FLOW) == 0) ? mix->alpha : mix->alpha * pressure;
		alpha *= 0.01;

		// ���`�����l���L���p�Ƀo�b�t�@���N���A
		(void)memset(window->brush_buffer, 0, window->width*window->height);

		// ���݂̍��W���L��
		mix->before_x = x, mix->before_y = y;

		min_x = x - r, max_x = x + r;
		min_y = y - r, max_y = y + r;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;
		start_x = (int)min_x, start_y = (int)min_y;
		width = (int)(max_x - min_x);
		height = (int)(max_y - min_y);
		stride = width * 4;

		for(i=0; i<height; i++)
		{
			(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
				0, stride);
		}

		cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
		brush = cairo_pattern_create_radial(x, y, 0, x, y, r);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
		cairo_pattern_add_color_stop_rgba(brush, 0, 0, 0, 0, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0-mix->blur*0.01, 0, 0, 0, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1, 0, 0, 0, mix->outline_hardness*0.01*alpha);
		cairo_set_source(window->mask_temp->cairo_p, brush);

		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{
			cairo_arc(window->mask_temp->cairo_p, x, y, r, 0, 2*G_PI);
			cairo_fill(window->mask_temp->cairo_p);
		}
		else
		{
			cairo_mask_surface(window->mask_temp->cairo_p, window->selection->surface_p, 0, 0);
		}
		cairo_pattern_destroy(brush);

		for(i=0; i<height; i++)
		{
			ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
			mask_pix = &window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4+3];
			for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
			{
				sum_color[0] += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
				sum_color[1] += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
				sum_color[2] += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
				sum_color[3] += ((ref_pix[3]+1) * *mask_pix) >> 8;
				sum_color[4] += *mask_pix;
			}
		}

		color[0] = (uint8)((sum_color[0] + sum_color[3] / 2) / sum_color[3]);
		color[1] = (uint8)((sum_color[1] + sum_color[3] / 2) / sum_color[3]);
		color[2] = (uint8)((sum_color[2] + sum_color[3] / 2) / sum_color[3]);
		color[3] = (uint8)((sum_color[3] + (sum_color[4] / 255) / 2) / (sum_color[4] / 255.0));

		for(i=0; i<height; i++)
		{
			(void)memset(&window->temp_layer->pixels[(i+start_y)*window->temp_layer->stride+start_x*4],
				0, stride);
		}

		brush = cairo_pattern_create_radial(x, y, 0, x, y, r);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		cairo_pattern_add_color_stop_rgba(brush, 0,
			color[2]*DIV_PIXEL, color[1]*DIV_PIXEL, color[0]*DIV_PIXEL, alpha*(color[3]*DIV_PIXEL));
		cairo_pattern_add_color_stop_rgba(brush, 1.0-mix->blur*0.01,
			color[2]*DIV_PIXEL, color[1]*DIV_PIXEL, color[0]*DIV_PIXEL, alpha*(color[3]*DIV_PIXEL));
		cairo_pattern_add_color_stop_rgba(brush, 1,
			color[2]*DIV_PIXEL, color[1]*DIV_PIXEL, color[0]*DIV_PIXEL,
			alpha*mix->outline_hardness*0.01*(color[3]*DIV_PIXEL));
		cairo_set_source(window->temp_layer->cairo_p, brush);
#else
		cairo_pattern_add_color_stop_rgba(brush, 0,
			color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0-mix->blur*0.01,
			color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1,
			color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL, alpha*mix->outline_hardness);
		cairo_set_source(window->temp_layer->cairo_p, brush);
#endif
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{
			cairo_arc(window->temp_layer->cairo_p, x, y, r, 0, 2*G_PI);
			cairo_fill(window->temp_layer->cairo_p);
		}
		else
		{
			cairo_mask_surface(window->temp_layer->cairo_p, window->selection->surface_p, 0, 0);
		}
		cairo_pattern_destroy(brush);

		for(i=0; i<height; i++)
		{
			ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
			mask_pix = &window->temp_layer->pixels[(i+start_y)*window->temp_layer->stride+start_x*4];

			for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
			{
				rev_alpha = ~mask_pix[3];
				blend_alpha = ((rev_alpha+1)*ref_pix[3]+mask_pix[3]*color[3])>>8;

				ref_pix[0] = ((rev_alpha+1)*ref_pix[0]+color[0]*mask_pix[3])>>8;
				ref_pix[1] = ((rev_alpha+1)*ref_pix[1]+color[1]*mask_pix[3])>>8;
				ref_pix[2] = ((rev_alpha+1)*ref_pix[2]+color[2]*mask_pix[3])>>8;
				ref_pix[3] = blend_alpha;
			}
		}
	}
}

static void MixBrushEditSelectionPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ��
	if(((GdkEventButton*)state)->button == 1)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		MIX_BRUSH* mix = (MIX_BRUSH*)core->brush_data;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �u���V�ʒu�̃s�N�Z���l���v
		unsigned int sum_color[2] = {1, 1};
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix;
		// �`�悷��F
		uint8 color[2];
		// ���u�����h�p
		uint8 rev_alpha;
		uint8 blend_alpha;
		// �u���V�p�^�[��
		cairo_pattern_t *brush;
		int i, j;	// for���p�̃J�E���^

		// �I��͈͂̓��e����ƃ��C���[�ɃR�s�[
		(void)memcpy(window->work_layer->pixels, window->selection->pixels, window->width * window->height);

		// ��ƃ��C���[�̍������[�h���㏑�����[�h��
		window->selection->layer_mode = SELECTION_BLEND_COPY;

		// ���[�N���C���[�ł�CAIRO�̍������[�h��ʏ탂�[�h��
		cairo_set_operator(window->work_layer->cairo_p, CAIRO_OPERATOR_OVER);

		// ���a�ƔZ�x���v�Z
		r = ((mix->flags & BRUSH_FLAG_SIZE) == 0) ? mix->r : mix->r * pressure;
		alpha = ((mix->flags & BRUSH_FLAG_FLOW) == 0) ? mix->alpha : mix->alpha * pressure;
		alpha *= 0.01;

		// ���`�����l���L���p�Ƀo�b�t�@���N���A
		(void)memset(window->brush_buffer, 0, window->width*window->height);

		// ���݂̍��W���L��
		mix->before_x = x, mix->before_y = y;

		min_x = x - r, max_x = x + r;
		min_y = y - r, max_y = y + r;

		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}

		core->min_x = min_x, core->min_y = min_y;
		core->max_x = max_x, core->max_y = max_y;
		start_x = (int)min_x, start_y = (int)min_y;
		width = (int)(max_x - min_x);
		height = (int)(max_y - min_y);
		stride = width * 4;

		for(i=0; i<height; i++)
		{
			(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
				0, stride);
		}

		cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
		brush = cairo_pattern_create_radial(x, y, 0, x, y, r);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
		cairo_pattern_add_color_stop_rgba(brush, 0, 0, 0, 0, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1.0-mix->blur*0.01, 0, 0, 0, alpha);
		cairo_pattern_add_color_stop_rgba(brush, 1, 0, 0, 0, mix->outline_hardness*0.01*alpha);
		cairo_set_source(window->mask_temp->cairo_p, brush);

		cairo_arc(window->mask_temp->cairo_p, x, y, r, 0, 2*G_PI);
		cairo_fill(window->mask_temp->cairo_p);
		
		cairo_pattern_destroy(brush);

		for(i=0; i<height; i++)
		{
			ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
			mask_pix = &window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4+3];
			for(j=0; j<width; j++, ref_pix++, mask_pix+=4)
			{
				sum_color[0] += ((ref_pix[3]+1) * *mask_pix) >> 8;
				sum_color[1] += *mask_pix;
			}
		}

		color[0] = (uint8)((sum_color[0] + (sum_color[1] / 255) / 2) / (sum_color[1] / 255.0));

		for(i=0; i<height; i++)
		{
			(void)memset(&window->temp_layer->pixels[(i+start_y)*window->temp_layer->stride+start_x*4],
				0, stride);
		}

		brush = cairo_pattern_create_radial(x, y, 0, x, y, r);
		cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);

		cairo_pattern_add_color_stop_rgba(brush, 0,
			0, 0, 0, alpha*(color[0]*DIV_PIXEL));
		cairo_pattern_add_color_stop_rgba(brush, 1.0-mix->blur*0.01,
			0, 0, 0, alpha*(color[0]*DIV_PIXEL));
		cairo_pattern_add_color_stop_rgba(brush, 1,
			0, 0, 0,
			alpha*mix->outline_hardness*0.01*(color[0]*DIV_PIXEL));
		cairo_set_source(window->temp_layer->cairo_p, brush);

		cairo_arc(window->temp_layer->cairo_p, x, y, r, 0, 2*G_PI);
		cairo_fill(window->temp_layer->cairo_p);
		cairo_pattern_destroy(brush);

		for(i=0; i<height; i++)
		{
			ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
			mask_pix = &window->temp_layer->pixels[(i+start_y)*window->temp_layer->stride+start_x*4];

			for(j=0; j<width; j++, ref_pix++, mask_pix+=4)
			{
				rev_alpha = ~mask_pix[3];
				blend_alpha = ((rev_alpha+1)*ref_pix[0]+mask_pix[0]*color[0])>>8;

				ref_pix[0] = blend_alpha;
			}
		}
	}
}

static void MixBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		MIX_BRUSH* mix = (MIX_BRUSH*)core->brush_data;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̈ړ���
		gdouble d;
		// �`����s�����W
		gdouble draw_x = mix->before_x, draw_y = mix->before_y;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �u���V�ʒu�̃s�N�Z���l���v
		unsigned int sum_color[5];
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *comp_pix, *alpha_pix;
		// ���u�����h�p
		uint8 rev_alpha;
		uint8 blend_alpha;
		gdouble inv_alpha;
		// �`�悷��F
		uint8 color[4];
		// �u���V�p�^�[��
		cairo_pattern_t *brush;
		// �֊s�̍d���̃o�C�g�l
		uint8 hardness = (uint8)(mix->outline_hardness*2.55);
		// �}�X�N�̃o�C�g�l
		uint8 mask_value;
		int i, j;	// for���p�̃J�E���^

		// ���a�ƔZ�x���v�Z
		r = ((mix->flags & BRUSH_FLAG_SIZE) == 0) ? mix->r : mix->r * pressure;
		alpha = ((mix->flags & BRUSH_FLAG_FLOW) == 0) ? mix->alpha : mix->alpha * pressure;
		alpha *= 0.01;

		// �u���V�̈ړ��͈͂��X�V
		min_x = x - r - 1, min_y = y - r - 1;
		max_x = x + r + 1, max_y = y + r + 1;
		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		// �O�̃X�e�b�v����̋������v�Z
		dx = x - mix->before_x, dy = y - mix->before_y;
		d = sqrt(dx*dx+dy*dy);
		diff_x = dx / d, diff_y = dy / d;

		dx = d;
		do
		{
			sum_color[0] = sum_color[1] = sum_color[2] = 0;
			sum_color[3] = sum_color[4] = 1;
			start_x = (int)(draw_x - r);
			start_y = (int)(draw_y - r);
			width = (int)(draw_x + r);
			height = (int)(draw_y + r);

			if(start_x < 0)
			{
				start_x = 0;
			}
			else if(start_x > window->work_layer->width)
			{
				goto skip_draw;
			}
			if(start_y < 0)
			{
				start_y = 0;
			}
			else if(start_y > window->work_layer->height)
			{
				goto skip_draw;
			}
			if(width > window->work_layer->width)
			{
				width = window->work_layer->width - start_x;
			}
			else
			{
				width = width - start_x;
			}
			if(height > window->work_layer->height)
			{
				height = window->work_layer->height - start_y;
			}
			else
			{
				height = height - start_y;
			}
			stride = width*4;

			for(i=0; i<height; i++)
			{
				(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
					0, stride);
			}

			cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			brush = cairo_pattern_create_radial(draw_x, draw_y, 0, draw_x, draw_y, r);
			cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
			cairo_pattern_add_color_stop_rgba(brush, 0, 0, 0, 0, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1.0-mix->blur*0.01, 0, 0, 0, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1, 0, 0, 0, mix->outline_hardness*0.01*alpha);
			cairo_set_source(window->mask_temp->cairo_p, brush);

			if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
			{
				cairo_arc(window->mask_temp->cairo_p, draw_x, draw_y, r, 0, 2*G_PI);
				cairo_fill(window->mask_temp->cairo_p);
			}
			else
			{
				cairo_mask_surface(window->mask_temp->cairo_p, window->selection->surface_p, 0, 0);
			}
			cairo_pattern_destroy(brush);

			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
				mask_pix = &window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4+3];
				for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
				{
					sum_color[0] += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
					sum_color[1] += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
					sum_color[2] += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
					sum_color[3] += ((ref_pix[3]+1) * *mask_pix) >> 8;
					sum_color[4] += *mask_pix;
				}
			}
			color[0] = (uint8)((sum_color[0] + sum_color[3] / 2) / sum_color[3]);
			color[1] = (uint8)((sum_color[1] + sum_color[3] / 2) / sum_color[3]);
			color[2] = (uint8)((sum_color[2] + sum_color[3] / 2) / sum_color[3]);
			color[3] = (uint8)((sum_color[3] + (sum_color[4] / 255) / 2) / (sum_color[4] / 255.0));
			inv_alpha = color[3] * DIV_PIXEL;

			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->stride+start_x*4];
				mask_pix = &window->temp_layer->pixels[(i+start_y)*window->temp_layer->stride+start_x*4];
				comp_pix = &window->brush_buffer[(i+start_y)*window->width+start_x];
				alpha_pix = &window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4+3];

				for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4, comp_pix++, alpha_pix+=4)
				{
					mask_value = *alpha_pix;
					*alpha_pix = (uint8)(*alpha_pix * inv_alpha);
					blend_alpha = *alpha_pix;
					rev_alpha = ~blend_alpha;
					blend_alpha = ((rev_alpha+1)*ref_pix[3]+blend_alpha*color[3])>>8;

					if(*alpha_pix > *comp_pix)
					{
						*comp_pix = mask_value;
						ref_pix[0] = ((rev_alpha+1)*ref_pix[0]+color[0]*(*alpha_pix))>>8;
						ref_pix[1] = ((rev_alpha+1)*ref_pix[1]+color[1]*(*alpha_pix))>>8;
						ref_pix[2] = ((rev_alpha+1)*ref_pix[2]+color[2]*(*alpha_pix))>>8;
						ref_pix[3] = blend_alpha;
					}
					else if(*alpha_pix > ref_pix[3])//if(blend_alpha > ref_pix[3])
					{
						ref_pix[0] = ((rev_alpha+1)*ref_pix[0]+color[0]*(*alpha_pix))>>8;
						ref_pix[1] = ((rev_alpha+1)*ref_pix[1]+color[1]*(*alpha_pix))>>8;
						ref_pix[2] = ((rev_alpha+1)*ref_pix[2]+color[2]*(*alpha_pix))>>8;
						ref_pix[3] = ((0xff-hardness)*(*alpha_pix)+(hardness+1)*blend_alpha)>>8;
					}
				}
			}
skip_draw:
			dx -= 1;
			if(dx < 1)
			{
				break;
			}
			else
			{
				draw_x += diff_x, draw_y += diff_y;
			}
		} while(1);

		// ���݂̍��W���L��
		mix->before_x = x, mix->before_y = y;
	}
}

static void MixBrushEditSelectionMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		// �u���V�̏ڍ׏��ɃL���X�g
		MIX_BRUSH* mix = (MIX_BRUSH*)core->brush_data;
		// �u���V�̔��a�ƕs�����x
		gdouble r, alpha;
		// ���W�̍ő�E�ŏ��l
		gdouble min_x, min_y, max_x, max_y;
		// X�AY�����̈ړ���
		gdouble dx, dy;
		// �u���V�̌X���p
		gdouble diff_x, diff_y;
		// �u���V�̈ړ���
		gdouble d;
		// �`����s�����W
		gdouble draw_x = mix->before_x, draw_y = mix->before_y;
		// �s�N�Z���f�[�^�����Z�b�g������W
		int start_x, start_y;
		// �`�悷�镝�A�����A��s���̃o�C�g��
		int width, height, stride;
		// �u���V�ʒu�̃s�N�Z���l���v
		unsigned int sum_color[2];
		// �Q�ƃs�N�Z��
		uint8 *ref_pix, *mask_pix, *comp_pix, *alpha_pix;
		// ���u�����h�p
		uint8 rev_alpha;
		uint8 blend_alpha;
		gdouble inv_alpha;
		// �`�悷��F
		uint8 color[2];
		// �u���V�p�^�[��
		cairo_pattern_t *brush;
		// �֊s�̍d���̃o�C�g�l
		uint8 hardness = (uint8)(mix->outline_hardness*2.55);
		// �}�X�N�̃o�C�g�l
		uint8 mask_value;
		int i, j;	// for���p�̃J�E���^

		// ���a�ƔZ�x���v�Z
		r = ((mix->flags & BRUSH_FLAG_SIZE) == 0) ? mix->r : mix->r * pressure;
		alpha = ((mix->flags & BRUSH_FLAG_FLOW) == 0) ? mix->alpha : mix->alpha * pressure;
		alpha *= 0.01;

		// �u���V�̈ړ��͈͂��X�V
		min_x = x - r - 1, min_y = y - r - 1;
		max_x = x + r + 1, max_y = y + r + 1;
		if(min_x < 0.0)
		{
			min_x = 0.0;
		}
		if(core->min_x > min_x)
		{
			core->min_x = min_x;
		}
		if(min_y < 0.0)
		{
			min_y = 0.0;
		}
		if(core->min_y > min_y)
		{
			core->min_y = min_y;
		}
		if(max_x > window->work_layer->width)
		{
			max_x = window->work_layer->width;
		}
		if(core->max_x < max_x)
		{
			core->max_x = max_x;
		}
		if(max_y > window->work_layer->height)
		{
			max_y = window->work_layer->height;
		}
		if(core->max_y < max_y)
		{
			core->max_y = max_y;
		}

		// �O�̃X�e�b�v����̋������v�Z
		dx = x - mix->before_x, dy = y - mix->before_y;
		d = sqrt(dx*dx+dy*dy);
		diff_x = dx / d, diff_y = dy / d;

		dx = d;
		do
		{
			sum_color[0] = sum_color[1] = 1;
			start_x = (int)(draw_x - r);
			start_y = (int)(draw_y - r);
			width = (int)(draw_x + r);
			height = (int)(draw_y + r);

			if(start_x < 0)
			{
				start_x = 0;
			}
			else if(start_x > window->work_layer->width)
			{
				goto skip_draw;
			}
			if(start_y < 0)
			{
				start_y = 0;
			}
			else if(start_y > window->work_layer->height)
			{
				goto skip_draw;
			}
			if(width > window->work_layer->width)
			{
				width = window->work_layer->width - start_x;
			}
			else
			{
				width = width - start_x;
			}
			if(height > window->work_layer->height)
			{
				height = window->work_layer->height - start_y;
			}
			else
			{
				height = height - start_y;
			}
			stride = width*4;

			for(i=0; i<height; i++)
			{
				(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
					0, stride);
			}

			cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			brush = cairo_pattern_create_radial(draw_x, draw_y, 0, draw_x, draw_y, r);
			cairo_pattern_set_extend(brush, CAIRO_EXTEND_NONE);
			cairo_pattern_add_color_stop_rgba(brush, 0, 0, 0, 0, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1.0-mix->blur*0.01, 0, 0, 0, alpha);
			cairo_pattern_add_color_stop_rgba(brush, 1, 0, 0, 0, mix->outline_hardness*0.01*alpha);
			cairo_set_source(window->mask_temp->cairo_p, brush);

			cairo_arc(window->mask_temp->cairo_p, draw_x, draw_y, r, 0, 2*G_PI);
			cairo_fill(window->mask_temp->cairo_p);
			cairo_pattern_destroy(brush);

			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
				mask_pix = &window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4+3];
				for(j=0; j<width; j++, ref_pix++, mask_pix+=4)
				{
					sum_color[0] += ((ref_pix[0]+1) * *mask_pix) >> 8;
					sum_color[1] += *mask_pix;
				}
			}
			color[0] = (uint8)((sum_color[0] + (sum_color[1] / 255) / 2) / (sum_color[1] / 255.0));
			inv_alpha = color[0] * DIV_PIXEL;

			for(i=0; i<height; i++)
			{
				ref_pix = &window->work_layer->pixels[(i+start_y)*window->work_layer->width+start_x];
				mask_pix = &window->temp_layer->pixels[(i+start_y)*window->temp_layer->stride+start_x*4];
				comp_pix = &window->brush_buffer[(i+start_y)*window->width+start_x];
				alpha_pix = &window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4+3];

				for(j=0; j<width; j++, ref_pix++, mask_pix+=4, comp_pix++, alpha_pix+=4)
				{
					mask_value = *alpha_pix;
					*alpha_pix = (uint8)(*alpha_pix * inv_alpha);
					blend_alpha = *alpha_pix;
					rev_alpha = ~blend_alpha;
					blend_alpha = ((rev_alpha+1)*ref_pix[0]+blend_alpha*color[0])>>8;

					if(*alpha_pix > *comp_pix)
					{
						*comp_pix = mask_value;
						ref_pix[0] = blend_alpha;
					}
					else if(*alpha_pix > ref_pix[0])//if(blend_alpha > ref_pix[3])
					{
						ref_pix[0] = ((0xff-hardness)*(*alpha_pix)+(hardness+1)*blend_alpha)>>8;
					}
				}
			}
skip_draw:
			dx -= 1;
			if(dx < 1)
			{
				break;
			}
			else
			{
				draw_x += diff_x, draw_y += diff_y;
			}
		} while(1);

		// ���݂̍��W���L��
		mix->before_x = x, mix->before_y = y;
	}
}

#define MixBrushReleaseCallBack DefaultReleaseCallBack
#define MixBrushEditSelectionReleaseCallBack DefaultEditSelectionReleaseCallBack

static void MixBrushDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	MIX_BRUSH* mix = (MIX_BRUSH*)data;
	gdouble r = mix->r * window->zoom_rate;
	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
	cairo_stroke(window->disp_temp->cairo_p);

	cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
		(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
	cairo_clip(window->disp_layer->cairo_p);
}

static void MixBrushButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, MIX_BRUSH* mix)
{
	gdouble r = mix->r * window->zoom_rate + 3;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2 + 3), (gint)(r * 2 + 3));
}

static void MixBrushMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, MIX_BRUSH* mix)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = mix->r * window->zoom_rate + 3;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width + 2, (gint)height + 2);
}

static void MixBrushSetSize(GtkAdjustment* slider, MIX_BRUSH* mix)
{
	mix->r = gtk_adjustment_get_value(slider) * 0.5;
}

static void MixBrushSetAlpha(GtkAdjustment* slider, MIX_BRUSH* mix)
{
	mix->alpha = gtk_adjustment_get_value(slider);
}

static void MixBrushSetBlur(GtkAdjustment* slider, MIX_BRUSH* mix)
{
	mix->blur = gtk_adjustment_get_value(slider);
}

static void MixBrushSetOutlineHardness(GtkAdjustment* slider, MIX_BRUSH* mix)
{
	mix->outline_hardness = gtk_adjustment_get_value(slider);
}

static void MixBrushSetPressureSize(GtkWidget* button, MIX_BRUSH* mix)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		mix->flags &= ~(BRUSH_FLAG_SIZE);
	}
	else
	{
		mix->flags |= BRUSH_FLAG_SIZE;
	}
}

static void MixBrushSetPressureFlow(GtkWidget* button, MIX_BRUSH* mix)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		mix->flags &= ~(BRUSH_FLAG_FLOW);
	}
	else
	{
		mix->flags |= BRUSH_FLAG_FLOW;
	}
}

static GtkWidget* CreateMixBrushDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define UI_FONT_SIZE 8.0
	MIX_BRUSH *mix = (MIX_BRUSH*)core->brush_data;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox;
	GtkWidget *combo;
	GtkWidget *scale;
	GtkWidget *packing;
	GtkWidget *check_button;
	GtkWidget *label;
	GtkAdjustment *scale_adjust;
	char mark_up_buff[256];

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new("");
		(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		combo = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mag_str[2]);
#else
		combo = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), mix->base_scale);
		gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	switch(mix->base_scale)
	{
	case 0:
		scale_adjust =
			GTK_ADJUSTMENT(gtk_adjustment_new(mix->r * 2, 0.1, 10.0, 0.1, 0.1, 0.0));
		break;
	case 1:
		scale_adjust = GTK_ADJUSTMENT(gtk_adjustment_new(
			mix->r * 2, 1.0, 100.0, 1.0, 1.0, 0.0));
		break;
	default:
		scale_adjust =
			GTK_ADJUSTMENT(gtk_adjustment_new(mix->r * 2, 5.0, 500.0, 1.0, 1.0, 0.0));
	}

	scale = SpinScaleNew(scale_adjust, app->labels->tool_box.brush_scale, 1);
	g_signal_connect(G_OBJECT(scale_adjust), "value_changed",
		G_CALLBACK(MixBrushSetSize), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(combo), "scale", scale);
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(SetBrushBaseScale), &mix->base_scale);

	scale_adjust = GTK_ADJUSTMENT(gtk_adjustment_new(
		mix->alpha, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(scale_adjust, app->labels->tool_box.flow, 1);
	g_signal_connect(G_OBJECT(scale_adjust), "value_changed",
		G_CALLBACK(MixBrushSetAlpha), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	scale_adjust = GTK_ADJUSTMENT(gtk_adjustment_new(
		mix->outline_hardness, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(scale_adjust, app->labels->tool_box.outline_hardness, 1);
	g_signal_connect(G_OBJECT(scale_adjust), "value_changed",
		G_CALLBACK(MixBrushSetOutlineHardness), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	scale_adjust = GTK_ADJUSTMENT(gtk_adjustment_new(
		mix->blur, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(scale_adjust, app->labels->tool_box.blur, 1);
	g_signal_connect(G_OBJECT(scale_adjust), "value_changed",
		G_CALLBACK(MixBrushSetBlur), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	packing = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("");
	(void)sprintf(mark_up_buff, "<span font_desc=\"%.2f\">%s : </span>",
		UI_FONT_SIZE, app->labels->tool_box.pressure);
	gtk_label_set_markup(GTK_LABEL(label), mark_up_buff);
	gtk_box_pack_start(GTK_BOX(packing), label, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.scale);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(MixBrushSetPressureSize), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), mix->flags & BRUSH_FLAG_SIZE);
	gtk_box_pack_start(GTK_BOX(packing), check_button, FALSE, TRUE, 0);
	check_button = gtk_check_button_new_with_label(app->labels->tool_box.flow);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(MixBrushSetPressureFlow), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), mix->flags & BRUSH_FLAG_FLOW);
	gtk_box_pack_start(GTK_BOX(packing), check_button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), packing, FALSE, FALSE, 0);

	return vbox;
#undef UI_FONT_SIZE
}

static void TextToolPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		TEXT_TOOL* text = (TEXT_TOOL*)core->brush_data;
		text->start_x = x;
		text->start_y = y;
		text->flags |= TEXT_TOOL_STARTED;
	}
}

static void TextToolMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		TEXT_TOOL* text = (TEXT_TOOL*)core->brush_data;
		text->end_x = x;
		text->end_y = y;
	}
}

static void TextToolReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((GdkEventButton*)state)->button == 1)
	{
		char name[MAX_LAYER_NAME_LENGTH];
		TEXT_TOOL* text = (TEXT_TOOL*)core->brush_data;
		LAYER* layer;
		gdouble start_x, start_y, width, height;
		uint32 flags = 0;
		int i = 1;

		text->flags &= ~(TEXT_TOOL_STARTED);

		if((text->flags & TEXT_TOOL_VERTICAL) != 0)
		{
			flags |= TEXT_LAYER_VERTICAL;
		}
		if((text->flags & TEXT_TOOL_BOLD) != 0)
		{
			flags |= TEXT_LAYER_BOLD;
		}
		if((text->flags & TEXT_TOOL_ITALIC) != 0)
		{
			flags |= TEXT_LAYER_ITALIC;
		}
		else if((text->flags & TEXT_TOOL_OBLIQUE) != 0)
		{
			flags |= TEXT_LAYER_OBLIQUE;
		}

		if(text->start_x > x)
		{
			start_x = x;
			width = text->start_x - x;
		}
		else
		{
			start_x = text->start_x;
			width = x - text->start_x;
		}

		if(text->start_y > y)
		{
			start_y = y;
			height = text->start_y - y;
		}
		else
		{
			start_y = text->start_y;
			height = y - text->start_y;
		}

		do
		{
			(void)sprintf(name, "%s%d", window->app->labels->layer_window.new_text, i);
			i++;
		} while(CorrectLayerName(window->layer, name) == 0);

		layer = CreateLayer(0, 0,
			window->width, window->height, 4, TYPE_TEXT_LAYER,
			window->active_layer, window->active_layer->next, name, window
		);
		window->num_layer++;
		layer->layer_data.text_layer_p = CreateTextLayer(
			start_x, start_y, width, height, text->base_size, text->font_size,
				text->font_id, *core->color, flags
		);

		LayerViewAddLayer(layer, window->layer, window->app->layer_window.view, window->num_layer);
		ChangeActiveLayer(window, layer);
		LayerViewSetActiveLayer(layer, window->app->layer_window.view);
	}
}

static void TextToolDrawCursor(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	void* data
)
{
	TEXT_TOOL* text = (TEXT_TOOL*)data;
	gdouble start_x, start_y, width, height;

	if((text->flags & TEXT_TOOL_STARTED) == 0)
	{
		return;
	}

	if(text->start_x > x)
	{
		start_x = x;
		width = text->start_x - x;
	}
	else
	{
		start_x = text->start_x;
		width = x - text->start_x;
	}

	if(text->start_y > y)
	{
		start_y = y;
		height = text->start_y - y;
	}
	else
	{
		start_y = text->start_y;
		height = y - text->start_y;
	}

	cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_rectangle(window->disp_temp->cairo_p, start_x, start_y, width, height);
	cairo_stroke(window->disp_temp->cairo_p);
}

static void ChangeTextToolFontFamily(GtkComboBox *combo_box, TEXT_TOOL* text)
{
	text->font_id = gtk_combo_box_get_active(combo_box);
}

static void ChangeTextToolFontSize(GtkAdjustment* slider, TEXT_TOOL* text)
{
	text->font_size = gtk_adjustment_get_value(slider);
}

static void ChangeTextToolWriteDirection(GtkWidget* widget, TEXT_TOOL* text)
{
	if(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "radio_id")) == 0)
	{
		text->flags &= ~(TEXT_TOOL_VERTICAL);
	}
	else
	{
		text->flags |= TEXT_TOOL_VERTICAL;
	}
}

static void ChangeTextToolBold(GtkWidget* widget, TEXT_TOOL* text)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		text->flags &= ~(TEXT_TOOL_BOLD);
	}
	else
	{
		text->flags |= TEXT_TOOL_BOLD;
	}
}

static void ChangeTextToolStyle(GtkWidget* widget, TEXT_TOOL* text)
{
	int mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "text-style"));
	switch(mode)
	{
	case TEXT_STYLE_NORMAL:
		text->flags &= ~(TEXT_TOOL_ITALIC | TEXT_TOOL_OBLIQUE);
		break;
	case TEXT_STYLE_ITALIC:
		text->flags &= ~(TEXT_TOOL_OBLIQUE);
		text->flags |= TEXT_TOOL_ITALIC;
		break;
	case TEXT_STYLE_OBLIQUE:
		text->flags &= ~(TEXT_TOOL_ITALIC);
		text->flags |= TEXT_TOOL_OBLIQUE;
		break;
	}
}

static GtkWidget* CreateTextToolDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
#define MAX_STR_LENGTH 256
#define UI_FONT_SIZE 8.0
	TEXT_TOOL* text = (TEXT_TOOL*)core->brush_data;
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0), *hbox = gtk_hbox_new(FALSE, 0);
#if MAJOR_VERSION == 1
	GtkWidget* font_selection = gtk_combo_box_new_text();
#else
	GtkWidget* font_selection = gtk_combo_box_text_new();
#endif
	GtkWidget* font_scale, *label;
	GtkWidget* base_scale;
	GtkWidget* radio_buttons[3];
	GtkWidget* button;
	GtkAdjustment* adjust;
	GList* list;
	const gchar* font_name;
	char item_str[MAX_STR_LENGTH];
	int i;

	for(i=0; i<app->num_font; i++)
	{
		font_name = pango_font_family_get_name(app->font_list[i]);
		(void)sprintf(item_str, "<span font_family=\"%s\">%s</span>",
			font_name, font_name);
#if MAJOR_VERSION == 1
		gtk_combo_box_append_text(GTK_COMBO_BOX(font_selection), item_str);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX(font_selection), item_str);
#endif
	}
	list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(font_selection));
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(font_selection), list->data, "markup", 0, NULL);
	gtk_widget_set_size_request(font_selection, 128, 32);
	gtk_combo_box_set_active(GTK_COMBO_BOX(font_selection), text->font_id);
	g_signal_connect(G_OBJECT(font_selection), "changed", G_CALLBACK(ChangeTextToolFontFamily), core->brush_data);
	gtk_box_pack_start(GTK_BOX(vbox), font_selection, FALSE, TRUE, 0);

	switch(text->base_size)
	{
	case 0:
		adjust = GTK_ADJUSTMENT(gtk_adjustment_new(text->font_size, 0.1, 10, 0.1, 1, 0));
		break;
	case 1:
		adjust = GTK_ADJUSTMENT(gtk_adjustment_new(text->font_size, 1, 100, 1, 10, 0));
		break;
	default:
		adjust = GTK_ADJUSTMENT(gtk_adjustment_new(text->font_size, 5, 500, 1, 10, 0));
	}

	font_scale = SpinScaleNew(adjust,
		app->labels->tool_box.scale, 1);
	g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextToolFontSize), core->brush_data);

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		label = gtk_label_new("");
		(void)sprintf(item_str, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), item_str);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		base_scale = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[2]);
#else
		base_scale = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), text->base_size);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), font_scale, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	g_object_set_data(G_OBJECT(base_scale), "scale", font_scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &text->base_size);

	hbox = gtk_hbox_new(TRUE, 0);
	radio_buttons[0] = gtk_radio_button_new_with_label(
		NULL, app->labels->tool_box.text_horizonal);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.text_vertical
	);

	for(i=0; i<2; i++)
	{
		g_object_set_data(G_OBJECT(radio_buttons[i]), "radio_id", GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(radio_buttons[i]), "toggled",
			G_CALLBACK(ChangeTextToolWriteDirection), core->brush_data);
		gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[i], FALSE, TRUE, 0);
	}

	if((text->flags & TEXT_TOOL_VERTICAL) == 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
	}

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 2);
	label = gtk_label_new("");
	(void)sprintf(item_str, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.text_style);
	gtk_label_set_markup(GTK_LABEL(label), item_str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	button = gtk_check_button_new_with_label(app->labels->tool_box.text_bold);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(ChangeTextToolBold), core->brush_data);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), text->flags & TEXT_TOOL_BOLD);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(TRUE, 0);
	radio_buttons[0] = gtk_radio_button_new_with_label(
		NULL, app->labels->tool_box.text_normal);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.text_italic
	);
	radio_buttons[2] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.text_oblique
	);

	for(i=0; i<3; i++)
	{
		g_object_set_data(G_OBJECT(radio_buttons[i]), "text-style",
			GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(radio_buttons[i]), "toggled",
			G_CALLBACK(ChangeTextToolStyle), core->brush_data);
	}
	gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[0], FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[1], FALSE, TRUE, 0);
	if((text->flags & TEXT_TOOL_ITALIC) != 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
	}
	else if((text->flags & TEXT_TOOL_OBLIQUE) != 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[2]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
	}
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[2], FALSE, TRUE, 0);

	g_list_free(list);

	return vbox;
#undef UI_FONT_SIZE
}

static void ScriptBrushButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		SCRIPT *script = (SCRIPT*)core->brush_data;
		script->brush_data->flags &= ~(SCRIPT_BRUSH_STARTED);
		window->work_layer->layer_mode = script->brush_data->blend_mode;
		script->brush_data->distance = 0;
		script->brush_data->before_x = x;
		script->brush_data->before_y = y;

		lua_getglobal(script->state, "button_press");

		lua_pushnumber(script->state, x);
		lua_pushnumber(script->state, y);
		lua_pushnumber(script->state, pressure);

		lua_createtable(script->state, 0, 3);
		lua_pushinteger(script->state, ((GdkEventButton*)state)->button);
		lua_setfield(script->state, -2, "button");
		lua_pushunsigned(script->state, ((GdkEventButton*)state)->time);
		lua_setfield(script->state, -2, "event_time");
		lua_pushunsigned(script->state, ((GdkEventButton*)state)->state);
		lua_setfield(script->state, -2, "state");

		lua_call(script->state, 4, 0);
	}
}

static void ScriptBrushMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	if(((*(GdkModifierType*)state) & GDK_BUTTON1_MASK) != 0)
	{
		SCRIPT *script = (SCRIPT*)core->brush_data;
		script->brush_data->distance = 0;

		script->brush_data->distance += sqrt(
			(script->brush_data->before_x-x)*(script->brush_data->before_x-x)
				+(script->brush_data->before_y-y)*(script->brush_data->before_y-y)
		);
		script->brush_data->before_x = x;
		script->brush_data->before_y = y;

		lua_getglobal(script->state, "motion_notify");

		lua_pushnumber(script->state, x);
		lua_pushnumber(script->state, y);
		lua_pushnumber(script->state, pressure);

		lua_createtable(script->state, 0, 2);
		lua_pushunsigned(script->state, gtk_get_current_event_time());
		lua_setfield(script->state, -2, "event_time");
		lua_pushunsigned(script->state, *(GdkModifierType*)state);
		lua_setfield(script->state, -2, "state");

		lua_call(script->state, 4, 0);
	}
}

static void ScriptBrushButtonReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	gdouble pressure,
	BRUSH_CORE *core,
	void* state
)
{
	// ���N���b�N�Ȃ�
	if(((GdkEventButton*)state)->button == 1)
	{
		SCRIPT *script = (SCRIPT*)core->brush_data;

		lua_getglobal(script->state, "button_release");

		lua_pushnumber(script->state, x);
		lua_pushnumber(script->state, y);
		lua_pushnumber(script->state, pressure);

		lua_createtable(script->state, 0, 3);
		lua_pushinteger(script->state, ((GdkEventButton*)state)->button);
		lua_setfield(script->state, -2, "button");
		lua_pushunsigned(script->state, ((GdkEventButton*)state)->time);
		lua_setfield(script->state, -2, "event_time");
		lua_pushunsigned(script->state, ((GdkEventButton*)state)->state);
		lua_setfield(script->state, -2, "state");

		lua_call(script->state, 4, 0);

		AddBrushHistory(core, window->active_layer);

		window->update.surface_p = cairo_surface_create_for_rectangle(
			window->active_layer->surface_p, window->update.x, window->update.y,
				window->update.width, window->update.height);
		window->update.cairo_p = cairo_create(window->update.surface_p);

		g_part_layer_blend_funcs[window->work_layer->layer_mode](window->work_layer, &window->update);

		(void)memset(window->work_layer->pixels, 0, window->work_layer->stride*window->work_layer->height);

		cairo_surface_destroy(window->update.surface_p);
		cairo_destroy(window->update.cairo_p);

		window->work_layer->layer_mode = LAYER_BLEND_NORMAL;
	}
}

static void ScriptBrushDrawCursor(DRAW_WINDOW* window, gdouble x, gdouble y, void *data)
{
	SCRIPT *script = (SCRIPT*)data;
	SCRIPT_BRUSH *brush = script->brush_data;
	if(script->brush_data->cursor_pixels == NULL)
	{
		gdouble r = brush->r * window->zoom_rate;
		cairo_set_line_width(window->disp_temp->cairo_p, 1.0);
		cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
		cairo_arc(window->disp_temp->cairo_p, x, y, r, 0, 2*G_PI);
		cairo_stroke(window->disp_temp->cairo_p);

		cairo_rectangle(window->disp_layer->cairo_p, x - r, y - r,
			(r + BRUSH_UPDATE_MARGIN) * 2, (r + BRUSH_UPDATE_MARGIN) * 2);
		cairo_clip(window->disp_layer->cairo_p);
	}
	else
	{
		// �J�[�\���u���V
		cairo_pattern_t *cursor_pattern;
		// �J�[�\���̊g��k���A��]�E���s�ړ��p�̍s��
		cairo_matrix_t matrix;
		// �J�[�\���̕��s�E��]�ړ��p
		gdouble trans_x, trans_y;
		gdouble cos_x, sin_y;
		gdouble half_width, half_height;
		gdouble rev_zoom = window->rev_zoom * (1 / brush->zoom);

		// ���s�ړ�������W���v�Z
		half_width = brush->width * 0.5 * brush->zoom * window->zoom_rate;
		half_height = brush->height * 0.5 * brush->zoom * window->zoom_rate;
		cos_x = cos(brush->arg), sin_y = sin(brush->arg);
		trans_x = x - (half_width * cos_x + half_height * sin_y);
		trans_y = y + (half_width * sin_y - half_height * cos_x);
		// �g��k�����A��]�p���Z�b�g
		cursor_pattern = cairo_pattern_create_for_surface(brush->cursor_surface);
		cairo_pattern_set_extend(cursor_pattern, CAIRO_EXTEND_NONE);
		cairo_matrix_init_scale(&matrix, rev_zoom, rev_zoom);
		cairo_matrix_rotate(&matrix, brush->arg);
		cairo_matrix_translate(&matrix, - trans_x, - trans_y);
		cairo_pattern_set_matrix(cursor_pattern, &matrix);

		cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
		cairo_mask(window->disp_temp->cairo_p, cursor_pattern);

		cairo_rectangle(window->disp_layer->cairo_p, (int)(x - half_width - 1),
			(int)(y - half_height - 1), (int)half_width*2+2, (int)half_height*2+2);
		cairo_clip(window->disp_layer->cairo_p);

		cairo_pattern_destroy(cursor_pattern);
	}
}

static void ScriptBrushButtonUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, SCRIPT* script)
{
	gdouble r = script->brush_data->r * window->zoom_rate + 3;
	gtk_widget_queue_draw_area(window->window, (gint)(x - r - 1),
		(gint)(y - r - 1), (gint)(r * 2 + 3), (gint)(r * 2 + 3));
}

static void ScriptBrushMotionUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, SCRIPT* script)
{
	gdouble start_x, start_y;
	gdouble width, height;
	gdouble r = script->brush_data->r * script->brush_data->zoom
		* window->zoom_rate  * 1.275 + BRUSH_UPDATE_MARGIN;

	if(window->before_cursor_x < x)
	{
		start_x = window->before_cursor_x - r;
		width = r * 2 + x - window->before_cursor_x;
	}
	else
	{
		start_x = x - r;
		width = r * 2 + window->before_cursor_x - x;
	}

	if(window->before_cursor_y < y)
	{
		start_y = window->before_cursor_y - r;
		height = r * 2 + y - window->before_cursor_y;
	}
	else
	{
		start_y = y - r;
		height = r * 2 + window->before_cursor_y - y;
	}

	gtk_widget_queue_draw_area(window->window,
		(gint)start_x, (gint)start_y, (gint)width + 2, (gint)height + 2);
}

static void ScriptBrushColorChange(const uint8 color[3], void* data)
{
	SCRIPT *script = (SCRIPT*)data;
	ScriptBrushPatternNew(script);
}

static GtkWidget* CreateScriptBrushDetailUI(APPLICATION* app, BRUSH_CORE* core)
{
	SCRIPT *script = (SCRIPT*)core->brush_data;
	GtkWidget *ui;

	script->brush_data->core = core;

	lua_getglobal(script->state, "ui_new");
	lua_call(script->state, 0, 1);
	ui = GTK_WIDGET(lua_topointer(script->state, 1));
	lua_pop(script->state, 1);

	ScriptBrushPatternNew(script);

	return ui;
}

void LoadBrushDetailData(
	BRUSH_CORE* core,
	INI_FILE_PTR file,
	const char* section_name,
	const char* brush_type,
	APPLICATION* app
)
{
	if(StringCompareIgnoreCase(brush_type, "PENCIL") == 0)
	{
		PENCIL* pen;
		core->brush_type = (uint8)BRUSH_TYPE_PENCIL;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*pen));
		(void)memset(core->brush_data, 0, sizeof(*pen));
		core->detail_data_size = sizeof(*pen);
		pen = (PENCIL*)core->brush_data;

		pen->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		pen->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		pen->alpha = IniFileGetInt(file, section_name, "FLOW");
		pen->outline_hardness = IniFileGetInt(file, section_name, "OUTLINE_HARDNESS");
		core->press_func = PencilPressCallBack;
		core->motion_func = PencilMotionCallBack;
		core->release_func = PencilReleaseCallBack;
		core->draw_cursor = PencilDrawCursor;
		core->button_update = (brush_update_func)PencilButtonUpdate;
		core->motion_update = (brush_update_func)PencilMotionUpdate;
		core->color_change = PencilColorChange;
		core->create_detail_ui = CreatePencilDetailUI;
		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			pen->flags |= BRUSH_FLAG_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			pen->flags |= BRUSH_FLAG_FLOW;
		}
		if(IniFileGetInt(file, section_name, "ANTI_ALIAS") != 0)
		{
			pen->flags |= BRUSH_FLAG_ANTI_ALIAS;
		}
	}
	else if(StringCompareIgnoreCase(brush_type, "HARD_PEN") == 0)
	{
		HARD_PEN *pen;
		core->brush_type = (uint8)BRUSH_TYPE_HARD_PEN;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*pen));
		(void)memset(core->brush_data, 0, sizeof(*pen));
		core->detail_data_size = sizeof(*pen);
		pen = (HARD_PEN*)core->brush_data;

		pen->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		pen->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		pen->alpha = IniFileGetInt(file, section_name, "FLOW");
		core->press_func = HardPenButtonPressCallBack;
		core->motion_func = HardPenMotionCallBack;
		core->release_func = HardPenButtonReleaseCallBack;
		core->draw_cursor = HardPenDrawCursor;
		core->button_update = (brush_update_func)HardPenButtonUpdate;
		core->motion_update = (brush_update_func)HardPenMotionUpdate;
		core->create_detail_ui = CreateHardPenDetailUI;
		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			pen->flags |= BRUSH_FLAG_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			pen->flags |= BRUSH_FLAG_FLOW;
		}
		if(IniFileGetInt(file, section_name, "ANTI_ALIAS") != 0)
		{
			pen->flags |= BRUSH_FLAG_ANTI_ALIAS;
		}
	}
	else if(StringCompareIgnoreCase(brush_type, "AIR_BRUSH") == 0)
	{
		AIR_BRUSH* brush;
		core->brush_type = (uint8)BRUSH_TYPE_AIR_BRUSH;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
		(void)memset(core->brush_data, 0, sizeof(*brush));
		core->detail_data_size = sizeof(*brush);
		brush = (AIR_BRUSH*)core->brush_data;

		brush->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		brush->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		brush->opacity = IniFileGetInt(file, section_name, "FLOW");
		brush->outline_hardness = IniFileGetInt(file, section_name, "OUTLINE_HARDNESS");
		brush->blur = IniFileGetInt(file, section_name, "BLUR");
		brush->enter = IniFileGetInt(file, section_name, "ENTER_SIZE");
		brush->out = IniFileGetInt(file, section_name, "OUT_SIZE");
		brush->blend_mode = IniFileGetInt(file, section_name, "BLEND_MODE");
		brush->core = core;

		core->press_func = AirBrushPressCallBack;
		core->motion_func = AirBrushMotionCallBack;
		core->release_func = AirBrushReleaseCallBack;
		core->draw_cursor = AirBrushDrawCursor;
		core->button_update = (brush_update_func)AirBrushButtonUpdate;
		core->motion_update = (brush_update_func)AirBrushMotionUpdate;
		core->create_detail_ui = CreateAirBrushDetailUI;
		core->color_change = AirBrushColorChange;

		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			brush->flags |= BRUSH_FLAG_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			brush->flags |= BRUSH_FLAG_FLOW;
		}
		if(IniFileGetInt(file, section_name, "ANTI_ALIAS") != 0)
		{
			brush->flags |= BRUSH_FLAG_ANTI_ALIAS;
		}
	}
	else if(StringCompareIgnoreCase(brush_type, "OLD_AIR_BRUSH") == 0)
	{
		OLD_AIR_BRUSH* brush;
		core->brush_type = (uint8)BRUSH_TYPE_OLD_AIR_BRUSH;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
		(void)memset(core->brush_data, 0, sizeof(*brush));
		core->detail_data_size = sizeof(*brush);
		brush = (OLD_AIR_BRUSH*)core->brush_data;

		brush->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		brush->opacity = IniFileGetInt(file, section_name, "FLOW");
		brush->outline_hardness = IniFileGetInt(file, section_name, "OUTLINE_HARDNESS");
		brush->blur = IniFileGetInt(file, section_name, "BLUR");

		core->press_func = OldAirBrushPressCallBack;
		core->motion_func = OldAirBrushMotionCallBack;
		core->release_func = OldAirBrushReleaseCallBack;
		core->draw_cursor = OldAirBrushDrawCursor;
		core->create_detail_ui = CreateOldAirBrushDetailUI;
		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			brush->flags |= BRUSH_FLAG_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			brush->flags |= BRUSH_FLAG_FLOW;
		}
	}
	else if(StringCompareIgnoreCase(brush_type, "BLEND_BRUSH") == 0)
	{
		BLEND_BRUSH* brush;
		core->brush_type = (uint8)BRUSH_TYPE_BLEND_BRUSH;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
		(void)memset(core->brush_data, 0, sizeof(*brush));
		core->detail_data_size = sizeof(*brush);
		brush = (BLEND_BRUSH*)core->brush_data;

		brush->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		brush->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		brush->opacity = IniFileGetInt(file, section_name, "FLOW");
		brush->outline_hardness = IniFileGetInt(file, section_name, "OUTLINE_HARDNESS");
		brush->blur = IniFileGetInt(file, section_name, "BLUR");
		brush->enter = IniFileGetInt(file, section_name, "ENTER_SIZE");
		brush->out = IniFileGetInt(file, section_name, "OUT_SIZE");
		brush->target = IniFileGetInt(file, section_name, "TARGET");
		brush->blend_mode = IniFileGetInt(file, section_name, "BLEND_MODE");
		brush->core = core;

		core->press_func = BlendBrushButtonPressCallBack;
		core->motion_func = BlendBrushMotionCallBack;
		core->release_func = BlendBrushButtonReleaseCallBack;
		core->draw_cursor = BlendBrushDrawCursor;
		core->button_update = BlendBrushButtonUpdate;
		core->motion_update = BlendBrushMotionUpdate;
		core->create_detail_ui = CreateBlendBrushDetailUI;
		core->color_change = BlendBrushColorChange;

		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			brush->flags |= BRUSH_FLAG_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			brush->flags |= BRUSH_FLAG_FLOW;
		}
		if(IniFileGetInt(file, section_name, "ANTI_ALIAS") != 0)
		{
			brush->flags |= BRUSH_FLAG_ANTI_ALIAS;
		}
	}
	else if(StringCompareIgnoreCase(brush_type, "WATER_COLOR_BRUSH") == 0)
	{
		WATER_COLOR_BRUSH *brush;
		core->brush_type = (uint8)BRUSH_TYPE_WATER_COLOR_BRUSH;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
		(void)memset(core->brush_data, 0, sizeof(*brush));
		core->detail_data_size = sizeof(*brush);
		brush = (WATER_COLOR_BRUSH*)core->brush_data;

		brush->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		brush->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		brush->alpha = IniFileGetInt(file, section_name, "FLOW");
		brush->outline_hardness = IniFileGetInt(file, section_name,
			"OUTLINE_HARDNESS");
		brush->blur = IniFileGetInt(file, section_name, "BLUR");
		brush->mix = IniFileGetInt(file, section_name, "COLOR_MIX");
		brush->extend = IniFileGetInt(file, section_name, "COLOR_EXTEND");
		brush->enter = IniFileGetInt(file, section_name, "ENTER_SIZE");
		brush->out = IniFileGetInt(file, section_name, "OUT_SIZE");
		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			brush->flags |= BRUSH_FLAG_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			brush->flags |= BRUSH_FLAG_FLOW;
		}

		core->press_func = WaterColorBrushPressCallBack;
		core->motion_func = WaterColorBrushMotionCallBack;
		core->release_func = WaterColorBrushReleaseCallBack;
		core->draw_cursor = WaterColorBrushDrawCursor;
		core->button_update = WaterColorBrushButtonUpdate;
		core->motion_update = WaterColorBrushMotionUpdate;
		core->create_detail_ui = CreateWaterColorBrushDetailUI;
	}
	else if(StringCompareIgnoreCase(brush_type, "STAMP_TOOL") == 0)
	{
		STAMP* stamp;
		core->brush_type = (uint8)BRUSH_TYPE_STAMP_TOOL;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*stamp));
		(void)memset(core->brush_data, 0, sizeof(*stamp));
		core->detail_data_size = sizeof(*stamp);
		stamp = (STAMP*)core->brush_data;

		stamp->core.brush_core = core;
		LoadStampCoreData(file, section_name, &stamp->core, &stamp->mode, app);

		core->press_func = StampToolButtonPressCallBack;
		core->motion_func = StampToolMotionCallBack;
		core->release_func = StampToolReleaseCallBack;
		core->draw_cursor = StampToolDrawCursor;
		core->button_update = StampToolButtonUpdate;
		core->motion_update = StampToolMotionUpdate;
		core->create_detail_ui = CreateStampToolDetailUI;
	}
	else if(StringCompareIgnoreCase(brush_type, "IMAGE_BRUSH") == 0)
	{
		IMAGE_BRUSH* brush;
		core->brush_type = (uint8)BRUSH_TYPE_IMAGE_BRUSH;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
		(void)memset(core->brush_data, 0, sizeof(*brush));
		core->detail_data_size = sizeof(*brush);
		brush = (IMAGE_BRUSH*)core->brush_data;

		brush->core.brush_core = core;
		LoadStampCoreData(file, section_name, &brush->core, &brush->mode, app);
		brush->size_range = IniFileGetInt(file, section_name, "SIZE_RANGE") * 0.01;
		brush->random_rotate_range = IniFileGetInt(file, section_name, "ROTATE_RANGE");

		if(IniFileGetInt(file, section_name, "ROTATE_TO_BRUSH_DIRECTION") != 0)
		{
			brush->core.flags |= BRUSH_FLAG_ROTATE;
		}

		core->press_func = ImageBrushPressCallBack;
		core->motion_func = ImageBrushMotionCallBack;
		core->release_func = ImageBrushReleaseCallBack;
		core->draw_cursor = ImageBrushDrawCursor;
		core->button_update = ImageBrushButtonUpdate;
		core->motion_update = ImageBrushMotionUpdate;
		core->create_detail_ui = CreateImageBrushDetailUI;
		core->color_change = ImageBrushChangeColor;
	}
	else if(StringCompareIgnoreCase(brush_type, "PICKER_BRUSH") == 0)
	{
		PICKER_BRUSH *brush;
		core->brush_type = (uint8)BRUSH_TYPE_PICKER_BRUSH;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
		(void)memset(core->brush_data, 0, sizeof(*brush));
		core->detail_data_size = sizeof(*brush);
		brush = (PICKER_BRUSH*)core->brush_data;

		brush->core = core;
		brush->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		brush->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		brush->alpha = IniFileGetInt(file, section_name, "FLOW");
		brush->outline_hardness = IniFileGetInt(file, section_name, "OUTLINE_HARDNESS");
		brush->blur = IniFileGetInt(file, section_name, "BLUR");
		brush->picker_mode = IniFileGetInt(file, section_name, "PICK_MODE");
		brush->picker_source = IniFileGetInt(file, section_name, "PICK_TARGET");
		brush->blend_mode = IniFileGetInt(file, section_name, "BLEND_MODE");
		brush->add_h = IniFileGetInt(file, section_name, "HUE");
		brush->add_s = IniFileGetInt(file, section_name, "SATURATION");
		brush->add_v = IniFileGetInt(file, section_name, "BRIGHTNESS");
		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			brush->flags |= PICKER_FLAG_PRESSURE_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			brush->flags |= PICKER_FLAG_PRESSURE_FLOW;
		}
		if(IniFileGetInt(file, section_name, "ANTI_ALIAS") != 0)
		{
			brush->flags |= PICKER_FLAG_ANTI_ALIAS;
		}

		core->press_func = PickerBrushButtonPressCallBack;
		core->motion_func = PickerBrushMotionCallBack;
		core->release_func = PickerBrushButtonReleaseCallBack;
		core->draw_cursor = PickerBrushDrawCursor;
		core->button_update = PickerBrushButtonUpdate;
		core->motion_update = PickerBrushMotionUpdate;
		core->create_detail_ui = CreatePickerBrushDetailUI;
	}
	else if(StringCompareIgnoreCase(brush_type, "PICKER_IMAGE_BRUSH") == 0)
	{
		PICKER_IMAGE_BRUSH* brush;
		core->brush_type = (uint8)BRUSH_TYPE_PICKER_IMAGE_BRUSH;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
		(void)memset(core->brush_data, 0, sizeof(*brush));
		core->detail_data_size = sizeof(*brush);
		brush = (PICKER_IMAGE_BRUSH*)core->brush_data;

		brush->core.brush_core = core;
		LoadStampCoreData(file, section_name, &brush->core, NULL, app);
		brush->picker_mode = IniFileGetInt(file, section_name, "PICK_MODE");
		brush->picker_source = IniFileGetInt(file, section_name, "PICK_TARGET");
		brush->add_h = IniFileGetInt(file, section_name, "HUE");
		brush->add_s = IniFileGetInt(file, section_name, "SATURATION");
		brush->add_v = IniFileGetInt(file, section_name, "BRIGHTNESS");
		brush->size_range = IniFileGetInt(file, section_name, "SIZE_RANGE") * 0.01;
		brush->random_rotate_range = IniFileGetInt(file, section_name, "ROTATE_RANGE");

		if(IniFileGetInt(file, section_name, "ROTATE_TO_BRUSH_DIRECTION") != 0)
		{
			brush->core.flags |= BRUSH_FLAG_ROTATE;
		}

		core->press_func = PickerImageBrushPressCallBack;
		core->motion_func = PickerImageBrushMotionCallBack;
		core->release_func = PickerImageBrushReleaseCallBack;
		core->draw_cursor = PickerImageBrushDrawCursor;
		core->button_update = PickerImageBrushButtonUpdate;
		core->motion_update = PickerImageBrushMotionUpdate;
		core->create_detail_ui = CreatePickerImageBrushDetailUI;
	}
	else if(StringCompareIgnoreCase(brush_type, "ERASER") == 0)
	{
		ERASER* eraser;
		core->brush_type = (uint8)BRUSH_TYPE_ERASER;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*eraser));
		(void)memset(core->brush_data, 0, sizeof(*eraser));
		core->detail_data_size = sizeof(*eraser);
		eraser = (ERASER*)core->brush_data;

		eraser->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		eraser->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		eraser->alpha = IniFileGetInt(file, section_name, "FLOW");
		eraser->outline_hardness = IniFileGetInt(file, section_name, "OUTLINE_HARDNESS");
		eraser->blur = IniFileGetInt(file, section_name, "BLUR");
		core->press_func = EraserButtonPressCallBack;
		core->motion_func = EraserMotionCallBack;
		core->release_func = EraserReleaseCallBack;
		core->draw_cursor = EraserDrawCursor;
		core->button_update = EraserButtonUpdate;
		core->motion_update = EraserMotionUpdate;
		core->create_detail_ui = CreateEraserDetailUI;
		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			eraser->flags |= BRUSH_FLAG_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			eraser->flags |= BRUSH_FLAG_FLOW;
		}
	}
	else if(StringCompareIgnoreCase(brush_type, "BUCKET") == 0)
	{
		BUCKET *bucket;
		core->brush_type = (uint8)BRUSH_TYPE_BUCKET;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*bucket));
		(void)memset(core->brush_data, 0, sizeof(*bucket));
		core->detail_data_size = sizeof(*bucket);
		bucket = (BUCKET*)core->brush_data;

		bucket->threshold = (uint16)IniFileGetInt(file, section_name, "THRESHOLD");
		bucket->mode = (uint8)IniFileGetInt(file, section_name, "MODE");
		bucket->target = (uint8)IniFileGetInt(file, section_name, "TARGET");
		bucket->extend = (int16)IniFileGetInt(file, section_name, "EXTEND");

		core->press_func = BucketPressCallBack;
		core->motion_func = BucketMotionCallBack;
		core->release_func = BucketReleaseCallBack;
		core->draw_cursor = BucketDrawCursor;
		core->create_detail_ui = CreateBucketDetailUI;
	}
	else if(StringCompareIgnoreCase(brush_type, "PATTERN_FILL") == 0)
	{
		PATTERN_FILL *fill;
		core->brush_type = (uint8)BRUSH_TYPE_PATTERN_FILL;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*fill));
		(void)memset(core->brush_data, 0, sizeof(*fill));
		core->detail_data_size = sizeof(*fill);
		fill = (PATTERN_FILL*)core->brush_data;

		fill->threshold = (uint16)IniFileGetInt(file, section_name, "THRESHOLD");
		fill->mode = (uint8)IniFileGetInt(file, section_name, "MODE");
		fill->target = (uint8)IniFileGetInt(file, section_name, "TARGET");
		fill->pattern_id = IniFileGetInt(file, section_name, "PATTERN_ID");
		fill->scale = IniFileGetInt(file, section_name, "SIZE");
		fill->flow = IniFileGetInt(file, section_name, "FLOW") * 0.01;
		fill->extend = (int16)IniFileGetInt(file, section_name, "EXTEND");

		core->press_func = PatternFillPressCallBack;
		core->motion_func = PatternFillMotionCallBack;
		core->release_func = PatternFillReleaseCallBack;
		core->draw_cursor = PatternFillDrawCursor;
		core->create_detail_ui = CreatePatternFillDetailUI;
	}
	else if(StringCompareIgnoreCase(brush_type, "GRADATION") == 0)
	{
		GRADATION* gradation;
		core->brush_type = (uint8)BRUSH_TYPE_GRADATION;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*gradation));
		(void)memset(core->brush_data, 0, sizeof(*gradation));
		core->detail_data_size = sizeof(*gradation);
		core->brush_type = BRUSH_TYPE_GRADATION;
		gradation = (GRADATION*)core->brush_data;

		if(IniFileGetInt(file, section_name, "COLOR_REVERSE") != 0)
		{
			gradation->flags |= GRADATION_COLOR_REVERSE;
		}
		gradation->app = app;

		core->press_func = GradationPressCallBack;
		core->motion_func = GradationMotionCallBack;
		core->release_func = GradationReleaseCallBack;
		core->draw_cursor = DrawGradationCursor;
		core->create_detail_ui = CreateGrationDetailUI;
		core->change_editting_selection = GradationOnChangeEditSelection;
	}
	else if(StringCompareIgnoreCase(brush_type, "BLUR_TOOL") == 0)
	{
		BLUR_TOOL* blur;
		core->brush_type = (uint8)BRUSH_TYPE_BLUR;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*blur));
		(void)memset(core->brush_data, 0, sizeof(*blur));
		core->detail_data_size = sizeof(*blur);
		blur = (BLUR_TOOL*)core->brush_data;

		blur->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		blur->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		blur->alpha = IniFileGetInt(file, section_name, "FLOW");
		blur->outline_hardness = IniFileGetInt(file, section_name, "OUTLINE_HARDNESS");
		blur->blur = IniFileGetInt(file, section_name, "BLUR");
		blur->color_extend = IniFileGetInt(file, section_name, "COLOR_EXTEND");
		core->press_func = BlurToolPressCallBack;
		core->motion_func = BlurToolMotionCallBack;
		core->release_func = BlurToolReleaseCallBack;
		core->draw_cursor = BlurToolDrawCursor;
		core->button_update = BlurToolButtonUpdate;
		core->motion_update = BlurToolMotionUpdate;
		core->create_detail_ui = CreateBlurToolDetailUI;
		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			blur->flags |= BRUSH_FLAG_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			blur->flags |= BRUSH_FLAG_FLOW;
		}
	}
	else if(StringCompareIgnoreCase(brush_type, "SMUDGE") == 0)
	{
		SMUDGE* smudge;
		core->brush_type = (uint8)BRUSH_TYPE_SMUDGE;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*smudge));
		(void)memset(core->brush_data, 0, sizeof(*smudge));
		core->detail_data_size = sizeof(*smudge);
		smudge = (SMUDGE*)core->brush_data;

		smudge->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		smudge->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		smudge->opacity = IniFileGetInt(file, section_name, "OPACITY");
		smudge->blur = IniFileGetInt(file, section_name, "BLUR");
		smudge->outline_hardness = IniFileGetInt(file, section_name, "OUTLINE_HARDNESS");
		smudge->extention = IniFileGetInt(file, section_name, "COLOR_EXTEND");

		core->press_func = SmudgePressCallBack;
		core->motion_func = SmudgeMotionCallBack;
		core->release_func = SmudgeReleaseCallBack;
		core->draw_cursor = SmudgeDrawCursor;
		core->button_update = SmudgeButtonUpdate;
		core->motion_update = SmudgeMotionUpdate;
		core->create_detail_ui = CreateSmudgeDetailUI;
		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			smudge->flags |= SMUDGE_PRESSURE_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_EXTENTION") != 0)
		{
			smudge->flags |= SMUDGE_PRESSURE_EXTENTION;
		}
	}
	else if(StringCompareIgnoreCase(brush_type, "MIX_BRUSH") == 0)
	{
		MIX_BRUSH *mix;
		core->brush_type = (uint8)BRUSH_TYPE_MIX_BRUSH;
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*mix));
		(void)memset(core->brush_data, 0, sizeof(*mix));
		core->detail_data_size = sizeof(*mix);
		mix = (MIX_BRUSH*)core->brush_data;

		mix->base_scale = IniFileGetInt(file, section_name, "MAGNIFICATION");
		mix->r = IniFileGetInt(file, section_name, "SIZE") * 0.5;
		mix->alpha = IniFileGetInt(file, section_name, "FLOW");
		mix->outline_hardness = IniFileGetInt(file, section_name, "OUTLINE_HARDNESS");
		mix->blur = IniFileGetInt(file, section_name, "BLUR");

		if(IniFileGetInt(file, section_name, "PRESSURE_SIZE") != 0)
		{
			mix->flags |= BRUSH_FLAG_SIZE;
		}
		if(IniFileGetInt(file, section_name, "PRESSURE_FLOW") != 0)
		{
			mix->flags |= BRUSH_FLAG_FLOW;
		}

		core->press_func = MixBrushPressCallBack;
		core->motion_func = MixBrushMotionCallBack;
		core->release_func = MixBrushReleaseCallBack;
		core->draw_cursor = MixBrushDrawCursor;
		core->button_update = MixBrushButtonUpdate;
		core->motion_update = MixBrushMotionUpdate;
		core->create_detail_ui = CreateMixBrushDetailUI;
	}
	else if(StringCompareIgnoreCase(brush_type, "TEXT_TOOL") == 0)
	{
		TEXT_TOOL* text;
		PangoFontFamily** search_font;
		char font_name[256];
		core->brush_data = MEM_ALLOC_FUNC(sizeof(*text));
		(void)memset(core->brush_data, 0, sizeof(*text));
		core->detail_data_size = sizeof(*text);
		text = (TEXT_TOOL*)core->brush_data;
		core->brush_type = BRUSH_TYPE_TEXT;

		core->press_func = TextToolPressCallBack;
		core->motion_func = TextToolMotionCallBack;
		core->release_func = TextToolReleaseCallBack;
		core->draw_cursor = TextToolDrawCursor;
		core->create_detail_ui = CreateTextToolDetailUI;

		text->base_size = IniFileGetInt(file, section_name, "MAGNIFICATION");
		text->font_size = IniFileGetInt(file, section_name, "SIZE");
		if(IniFileGetInt(file, section_name, "VERTICAL") != 0)
		{
			text->flags |= TEXT_TOOL_VERTICAL;
		}

		(void)IniFileGetString(file, section_name, "FONT_NAME", font_name, 256);
		search_font = (PangoFontFamily**)bsearch(
			font_name, app->font_list, app->num_font, sizeof(*app->font_list),
				(int (*)(const void*, const void*))ForFontFamilySearchCompare);
		if(search_font == NULL)
		{
			text->font_id = 0;
		}
		else
		{
			text->font_id = (int32)(search_font - app->font_list);
		}
	}
	else if(StringCompareIgnoreCase(brush_type, "SCRIPT_BRUSH") == 0)
	{
		SCRIPT *script;
		char *system_name;
		char *script_file_name;
		char *file_path;
		system_name = IniFileStrdup(file, section_name, "FILE_NAME");
		script_file_name = g_locale_to_utf8(system_name, -1, NULL, NULL, NULL);
		file_path = g_build_filename(app->current_path, "brushes", script_file_name, NULL);
		script = CreateScript(app, file_path);
		MEM_FREE_FUNC(system_name);
		g_free(script_file_name);
		g_free(file_path);

		if(script != NULL)
		{
			script->brush_data = (SCRIPT_BRUSH*)MEM_ALLOC_FUNC(sizeof(*script->brush_data));
			(void)memset(script->brush_data, 0, sizeof(*script->brush_data));
			script->brush_data->r = 1;
			script->brush_data->zoom = 1;
			core->brush_type = (uint8)BRUSH_TYPE_SCRIPT_BRUSH;
			core->brush_data = (void*)script;
			core->detail_data_size = sizeof(*script);

			core->press_func = ScriptBrushButtonPressCallBack;
			core->motion_func = ScriptBrushMotionCallBack;
			core->release_func = ScriptBrushButtonReleaseCallBack;
			core->draw_cursor = ScriptBrushDrawCursor;
			core->button_update = (brush_update_func)ScriptBrushButtonUpdate;
			core->motion_update = (brush_update_func)ScriptBrushMotionUpdate;
			core->color_change = ScriptBrushColorChange;
			core->create_detail_ui = CreateScriptBrushDetailUI;
		}
	}

	if(core->button_update == NULL)
	{
		core->button_update = DefaultToolUpdate;
	}

	if(core->motion_update == NULL)
	{
		core->motion_update = DefaultToolUpdate;
	}
}

void LoadBrushDefaultData(
	BRUSH_CORE* core,
	eBRUSH_TYPE brush_type,
	APPLICATION* app
)
{
	core->app = app;
	core->color = &app->tool_window.color_chooser->rgb;
	core->brush_pattern_buff = &app->tool_window.brush_pattern_buff;
	core->temp_pattern_buff = &app->tool_window.temp_pattern_buff;

	switch(brush_type)
	{
	case BRUSH_TYPE_PENCIL:
		{
			PENCIL* pen;
			core->brush_type = (uint8)BRUSH_TYPE_PENCIL;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*pen));
			(void)memset(core->brush_data, 0, sizeof(*pen));
			core->detail_data_size = sizeof(*pen);
			pen = (PENCIL*)core->brush_data;

			pen->core = core;
			pen->base_scale = 1;
			pen->r = 10;
			pen->alpha = 100;
			pen->outline_hardness = 100;
			core->press_func = PencilPressCallBack;
			core->motion_func = PencilMotionCallBack;
			core->release_func = PencilReleaseCallBack;
			core->draw_cursor = PencilDrawCursor;
			core->button_update = (brush_update_func)PencilButtonUpdate;
			core->motion_update = (brush_update_func)PencilMotionUpdate;
			core->color_change = PencilColorChange;
			core->create_detail_ui = CreatePencilDetailUI;
		}
		break;
	case BRUSH_TYPE_HARD_PEN:
		{
			HARD_PEN *pen;
			core->brush_type = (uint8)BRUSH_TYPE_HARD_PEN;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*pen));
			(void)memset(core->brush_data, 0, sizeof(*pen));
			core->detail_data_size = sizeof(*pen);
			pen = (HARD_PEN*)core->brush_data;

			pen->core = core;
			pen->base_scale = 1;
			pen->r = 10;
			pen->alpha = 100;
			core->press_func = HardPenButtonPressCallBack;
			core->motion_func = HardPenMotionCallBack;
			core->release_func = HardPenButtonReleaseCallBack;
			core->draw_cursor = HardPenDrawCursor;
			core->button_update = (brush_update_func)HardPenButtonUpdate;
			core->motion_update = (brush_update_func)HardPenMotionUpdate;
			core->create_detail_ui = CreateHardPenDetailUI;
		}
		break;
	case BRUSH_TYPE_AIR_BRUSH:
		{
			AIR_BRUSH* brush;
			core->brush_type = (uint8)BRUSH_TYPE_AIR_BRUSH;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
			(void)memset(core->brush_data, 0, sizeof(*brush));
			core->detail_data_size = sizeof(*brush);
			brush = (AIR_BRUSH*)core->brush_data;

			brush->core = core;
			brush->base_scale = 1;
			brush->r = 10;
			brush->opacity = 100;
			// brush->outline_hardness = 0;
			brush->blur = 100;
			// brush->enter = 0;
			// brush->out = 0;
			// brush->blend_mode = 0;
			brush->core = core;

			core->press_func = AirBrushPressCallBack;
			core->motion_func = AirBrushMotionCallBack;
			core->release_func = AirBrushReleaseCallBack;
			core->draw_cursor = AirBrushDrawCursor;
			core->button_update = (brush_update_func)AirBrushButtonUpdate;
			core->motion_update = (brush_update_func)AirBrushMotionUpdate;
			core->create_detail_ui = CreateAirBrushDetailUI;
			core->color_change = AirBrushColorChange;
		}
		break;
	case BRUSH_TYPE_OLD_AIR_BRUSH:
		{
			OLD_AIR_BRUSH* brush;
			core->brush_type = (uint8)BRUSH_TYPE_OLD_AIR_BRUSH;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
			(void)memset(core->brush_data, 0, sizeof(*brush));
			core->detail_data_size = sizeof(*brush);
			brush = (OLD_AIR_BRUSH*)core->brush_data;

			brush->r = 10;
			brush->opacity = 100;
			// brush->outline_hardness = 0;
			brush->blur = 100;

			core->press_func = OldAirBrushPressCallBack;
			core->motion_func = OldAirBrushMotionCallBack;
			core->release_func = OldAirBrushReleaseCallBack;
			core->draw_cursor = OldAirBrushDrawCursor;
			core->create_detail_ui = CreateOldAirBrushDetailUI;
		}
		break;
	case BRUSH_TYPE_BLEND_BRUSH:
		{
			BLEND_BRUSH* brush;
			core->brush_type = (uint8)BRUSH_TYPE_BLEND_BRUSH;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
			(void)memset(core->brush_data, 0, sizeof(*brush));
			core->detail_data_size = sizeof(*brush);
			brush = (BLEND_BRUSH*)core->brush_data;

			brush->core = core;
			brush->base_scale = 1;
			brush->r = 10;
			brush->opacity = 100;
			// brush->outline_hardness = 0;
			brush->blur = 100;
			// brush->enter = 0;
			// brush->out = 0;
			// brush->target = 0;
			// brush->blend_mode = 0;
			brush->core = core;

			core->press_func = BlendBrushButtonPressCallBack;
			core->motion_func = BlendBrushMotionCallBack;
			core->release_func = BlendBrushButtonReleaseCallBack;
			core->draw_cursor = BlendBrushDrawCursor;
			core->button_update = BlendBrushButtonUpdate;
			core->motion_update = BlendBrushMotionUpdate;
			core->create_detail_ui = CreateBlendBrushDetailUI;
			core->color_change = BlendBrushColorChange;
		}
		break;
	case BRUSH_TYPE_WATER_COLOR_BRUSH:
		{
			WATER_COLOR_BRUSH *brush;
			core->brush_type = (uint8)BRUSH_TYPE_WATER_COLOR_BRUSH;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
			(void)memset(core->brush_data, 0, sizeof(*brush));
			core->detail_data_size = sizeof(*brush);
			brush = (WATER_COLOR_BRUSH*)core->brush_data;

			brush->core = core;
			brush->base_scale = 1;
			brush->r = 10;
			brush->alpha = 100;
			// brush->outline_hardness = 0;
			brush->blur = 100;
			// brush->mix = 0;
			// brush->extend = 0;
			// brush->enter = 0;
			// brush->out = 0;

			core->press_func = WaterColorBrushPressCallBack;
			core->motion_func = WaterColorBrushMotionCallBack;
			core->release_func = WaterColorBrushReleaseCallBack;
			core->draw_cursor = WaterColorBrushDrawCursor;
			core->button_update = WaterColorBrushButtonUpdate;
			core->motion_update = WaterColorBrushMotionUpdate;
			core->create_detail_ui = CreateWaterColorBrushDetailUI;
		}
		break;
	case BRUSH_TYPE_STAMP_TOOL:
		{
			STAMP* stamp;
			core->brush_type = (uint8)BRUSH_TYPE_STAMP_TOOL;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*stamp));
			(void)memset(core->brush_data, 0, sizeof(*stamp));
			core->detail_data_size = sizeof(*stamp);
			stamp = (STAMP*)core->brush_data;

			stamp->core.brush_core = core;
			LoadStampCoreDefaultData(&stamp->core, &stamp->mode, app);

			core->press_func = StampToolButtonPressCallBack;
			core->motion_func = StampToolMotionCallBack;
			core->release_func = StampToolReleaseCallBack;
			core->draw_cursor = StampToolDrawCursor;
			core->button_update = StampToolButtonUpdate;
			core->motion_update = StampToolMotionUpdate;
			core->create_detail_ui = CreateStampToolDetailUI;
		}
		break;
	case BRUSH_TYPE_IMAGE_BRUSH:
		{
			IMAGE_BRUSH* brush;
			core->brush_type = (uint8)BRUSH_TYPE_IMAGE_BRUSH;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
			(void)memset(core->brush_data, 0, sizeof(*brush));
			core->detail_data_size = sizeof(*brush);
			brush = (IMAGE_BRUSH*)core->brush_data;

			brush->core.brush_core = core;
			LoadStampCoreDefaultData(&brush->core, &brush->mode, app);
			brush->size_range = 100;
			brush->random_rotate_range = 100;

			core->press_func = ImageBrushPressCallBack;
			core->motion_func = ImageBrushMotionCallBack;
			core->release_func = ImageBrushReleaseCallBack;
			core->draw_cursor = ImageBrushDrawCursor;
			core->button_update = ImageBrushButtonUpdate;
			core->motion_update = ImageBrushMotionUpdate;
			core->create_detail_ui = CreateImageBrushDetailUI;
			core->color_change = ImageBrushChangeColor;
		}
		break;
	case BRUSH_TYPE_PICKER_BRUSH:
		{
			PICKER_BRUSH *brush;
			core->brush_type = (uint8)BRUSH_TYPE_PICKER_BRUSH;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
			(void)memset(core->brush_data, 0, sizeof(*brush));
			core->detail_data_size = sizeof(*brush);
			brush = (PICKER_BRUSH*)core->brush_data;

			brush->core = core;
			brush->base_scale = 1;
			brush->r = 20;
			brush->alpha = 100;
			// brush->outline_hardness = 0;
			brush->blur = 100;
			// brush->picker_mode = PICKER_MODE_SINGLE_PIXEL;
			// brush->picker_source = 0;
			// brush->blend_mode = LAYER_BLEND_NORMAL;
			// brush->add_h = 0;
			// brush->add_s = 0;
			// brush->add_v = 0;

			core->press_func = PickerBrushButtonPressCallBack;
			core->motion_func = PickerBrushMotionCallBack;
			core->release_func = PickerBrushButtonReleaseCallBack;
			core->draw_cursor = PickerBrushDrawCursor;
			core->button_update = PickerBrushButtonUpdate;
			core->motion_update = PickerBrushMotionUpdate;
			core->create_detail_ui = CreatePickerBrushDetailUI;
		}
		break;
	case BRUSH_TYPE_PICKER_IMAGE_BRUSH:
		{
			PICKER_IMAGE_BRUSH* brush;
			core->brush_type = (uint8)BRUSH_TYPE_PICKER_IMAGE_BRUSH;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*brush));
			(void)memset(core->brush_data, 0, sizeof(*brush));
			core->detail_data_size = sizeof(*brush);
			brush = (PICKER_IMAGE_BRUSH*)core->brush_data;

			brush->core.brush_core = core;
			LoadStampCoreDefaultData(&brush->core, NULL, app);
			// brush->picker_mode = PICKER_MODE_SINGLE_PIXEL;
			// brush->picker_source = 0;
			// brush->add_h = 0;
			// brush->add_s = 0;
			// brush->add_v = 0;
			brush->size_range = 100;
			brush->random_rotate_range = 100;

			core->press_func = PickerImageBrushPressCallBack;
			core->motion_func = PickerImageBrushMotionCallBack;
			core->release_func = PickerImageBrushReleaseCallBack;
			core->draw_cursor = PickerImageBrushDrawCursor;
			core->button_update = PickerImageBrushButtonUpdate;
			core->motion_update = PickerImageBrushMotionUpdate;
			core->create_detail_ui = CreatePickerImageBrushDetailUI;
		}
		break;
	case BRUSH_TYPE_ERASER:
		{
			ERASER* eraser;
			core->brush_type = (uint8)BRUSH_TYPE_ERASER;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*eraser));
			(void)memset(core->brush_data, 0, sizeof(*eraser));
			core->detail_data_size = sizeof(*eraser);
			eraser = (ERASER*)core->brush_data;

			eraser->core = core;
			eraser->base_scale = 1;
			eraser->r = 10;
			eraser->alpha = 100;
			eraser->outline_hardness = 100;
			// eraser->blur = 0;

			core->press_func = EraserButtonPressCallBack;
			core->motion_func = EraserMotionCallBack;
			core->release_func = EraserReleaseCallBack;
			core->draw_cursor = EraserDrawCursor;
			core->button_update = EraserButtonUpdate;
			core->motion_update = EraserMotionUpdate;
			core->create_detail_ui = CreateEraserDetailUI;
		}
		break;
	case BRUSH_TYPE_BUCKET:
		{
			BUCKET *bucket;
			core->brush_type = (uint8)BRUSH_TYPE_BUCKET;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*bucket));
			(void)memset(core->brush_data, 0, sizeof(*bucket));
			core->detail_data_size = sizeof(*bucket);
			bucket = (BUCKET*)core->brush_data;

			bucket->threshold = 20;
			bucket->mode = BUCKET_RGBA;
			// bucket->target = BUCKET_TARGET_ACTIVE_LAYER;
			// bucket->extend = 0;

			core->press_func = BucketPressCallBack;
			core->motion_func = BucketMotionCallBack;
			core->release_func = BucketReleaseCallBack;
			core->draw_cursor = BucketDrawCursor;
			core->create_detail_ui = CreateBucketDetailUI;
		}
		break;
	case BRUSH_TYPE_PATTERN_FILL:
		{
			PATTERN_FILL *fill;
			core->brush_type = (uint8)BRUSH_TYPE_PATTERN_FILL;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*fill));
			(void)memset(core->brush_data, 0, sizeof(*fill));
			core->detail_data_size = sizeof(*fill);
			fill = (PATTERN_FILL*)core->brush_data;

			fill->threshold = 20;
			fill->mode = PATTERN_FILL_RGBA;
			// fill->target = PATTERN_FILL_TARGET_ACTIVE_LAYER;
			// fill->pattern_id = 0;
			fill->scale = 100;
			fill->flow = 1;
			// fill->extend = 0;

			core->press_func = PatternFillPressCallBack;
			core->motion_func = PatternFillMotionCallBack;
			core->release_func = PatternFillReleaseCallBack;
			core->draw_cursor = PatternFillDrawCursor;
			core->create_detail_ui = CreatePatternFillDetailUI;
		}
		break;
	case BRUSH_TYPE_GRADATION:
		{
			GRADATION* gradation;
			core->brush_type = (uint8)BRUSH_TYPE_GRADATION;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*gradation));
			(void)memset(core->brush_data, 0, sizeof(*gradation));
			core->detail_data_size = sizeof(*gradation);
			core->brush_type = BRUSH_TYPE_GRADATION;
			gradation = (GRADATION*)core->brush_data;

			gradation->app = app;
			core->press_func = GradationPressCallBack;
			core->motion_func = GradationMotionCallBack;
			core->release_func = GradationReleaseCallBack;
			core->draw_cursor = DrawGradationCursor;
			core->create_detail_ui = CreateGrationDetailUI;
			core->change_editting_selection = GradationOnChangeEditSelection;
		}
		break;
	case BRUSH_TYPE_BLUR:
		{
			BLUR_TOOL* blur;
			core->brush_type = (uint8)BRUSH_TYPE_BLUR;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*blur));
			(void)memset(core->brush_data, 0, sizeof(*blur));
			core->detail_data_size = sizeof(*blur);
			blur = (BLUR_TOOL*)core->brush_data;

			blur->core = core;
			blur->base_scale = 1;
			blur->r = 10;
			blur->alpha = 100;
			blur->outline_hardness = 100;
			// blur->blur = 0;
			blur->color_extend = 50;

			core->press_func = BlurToolPressCallBack;
			core->motion_func = BlurToolMotionCallBack;
			core->release_func = BlurToolReleaseCallBack;
			core->draw_cursor = BlurToolDrawCursor;
			core->button_update = BlurToolButtonUpdate;
			core->motion_update = BlurToolMotionUpdate;
			core->create_detail_ui = CreateBlurToolDetailUI;
		}
		break;
	case BRUSH_TYPE_SMUDGE:
		{
			SMUDGE* smudge;
			core->brush_type = (uint8)BRUSH_TYPE_SMUDGE;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*smudge));
			(void)memset(core->brush_data, 0, sizeof(*smudge));
			core->detail_data_size = sizeof(*smudge);
			smudge = (SMUDGE*)core->brush_data;

			smudge->core = core;
			smudge->base_scale = 1;
			smudge->r = 10;
			smudge->opacity = 100;
			// smudge->outline_hardness = 0;
			smudge->extention = 50;

			core->press_func = SmudgePressCallBack;
			core->motion_func = SmudgeMotionCallBack;
			core->release_func = SmudgeReleaseCallBack;
			core->draw_cursor = SmudgeDrawCursor;
			core->button_update = SmudgeButtonUpdate;
			core->motion_update = SmudgeMotionUpdate;
			core->create_detail_ui = CreateSmudgeDetailUI;
		}
		break;
	case BRUSH_TYPE_MIX_BRUSH:
		{
			MIX_BRUSH *mix;
			core->brush_type = (uint8)BRUSH_TYPE_MIX_BRUSH;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*mix));
			(void)memset(core->brush_data, 0, sizeof(*mix));
			core->detail_data_size = sizeof(*mix);
			mix = (MIX_BRUSH*)core->brush_data;

			mix->base_scale = 1;
			mix->r = 10;
			mix->alpha = 100;
			// mix->outline_hardness = 0;
			mix->blur = 100;

			core->press_func = MixBrushPressCallBack;
			core->motion_func = MixBrushMotionCallBack;
			core->release_func = MixBrushReleaseCallBack;
			core->draw_cursor = MixBrushDrawCursor;
			core->button_update = MixBrushButtonUpdate;
			core->motion_update = MixBrushMotionUpdate;
			core->create_detail_ui = CreateMixBrushDetailUI;
		}
		break;
	case BRUSH_TYPE_TEXT:
		{
			TEXT_TOOL* text;
			core->brush_data = MEM_ALLOC_FUNC(sizeof(*text));
			(void)memset(core->brush_data, 0, sizeof(*text));
			core->detail_data_size = sizeof(*text);
			text = (TEXT_TOOL*)core->brush_data;
			core->brush_type = BRUSH_TYPE_TEXT;

			core->press_func = TextToolPressCallBack;
			core->motion_func = TextToolMotionCallBack;
			core->release_func = TextToolReleaseCallBack;
			core->draw_cursor = TextToolDrawCursor;
			core->create_detail_ui = CreateTextToolDetailUI;

			text->base_size = 1;
			text->font_size = 10;
			// text->font_id = 0;
		}
		break;
	case BRUSH_TYPE_SCRIPT_BRUSH:
		{
			core->brush_type = (uint8)BRUSH_TYPE_SCRIPT_BRUSH;
			core->press_func = ScriptBrushButtonPressCallBack;
			core->motion_func = ScriptBrushMotionCallBack;
			core->release_func = ScriptBrushButtonReleaseCallBack;
			core->draw_cursor = ScriptBrushDrawCursor;
			core->button_update = (brush_update_func)ScriptBrushButtonUpdate;
			core->motion_update = (brush_update_func)ScriptBrushMotionUpdate;
			core->color_change = ScriptBrushColorChange;
			core->create_detail_ui = CreateScriptBrushDetailUI;
		}
		break;
	}

	if(core->button_update == NULL)
	{
		core->button_update = DefaultToolUpdate;
	}

	if(core->motion_update == NULL)
	{
		core->motion_update = DefaultToolUpdate;
	}
}

/*********************************************************
* WriteBrushDetailData�֐�                               *
* �u���V�̏ڍאݒ�������o��                             *
* ����                                                   *
* window	: �c�[���{�b�N�X�E�B���h�E                   *
* file_path	: �����o���t�@�C���̃p�X                     *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
void WriteBrushDetailData(TOOL_WINDOW* window, const char* file_path, APPLICATION *app)
{
	GFile* fp = g_file_new_for_path(file_path);
	GFileOutputStream* stream =
		g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL);
	INI_FILE_PTR file;
	char brush_section_name[256];
	char *write_str, brush_name[1024], hot_key[2] = {0};
	int brush_id = 1;
	int x, y;

	// �t�@�C���I�[�v���Ɏ��s������㏑���Ŏ���
	if(stream == NULL)
	{
		stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);

		if(stream == NULL)
		{
			g_object_unref(fp);
			return;
		}
	}

	file = CreateIniFile(stream, NULL,0, INI_WRITE);

	// �����R�[�h����������
	IniFileAddString(file, "CODE", "CODE_TYPE", window->brush_code);

	// �t�H���g�t�@�C��������������
	IniFileAddString(file, "FONT", "FONT_FILE", window->font_file);

	// �u���V�e�[�u���̓��e�������o��
	for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT; y++)
	{
		for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH; x++)
		{
			if(window->brushes[y][x].name != NULL)
			{
				(void)sprintf(brush_section_name, "BRUSH%d", brush_id);

				(void)strcpy(brush_name, window->brushes[y][x].name);
				StringRepalce(brush_name, "\n", "\\n");
				write_str = g_convert(brush_name, -1,
					window->brush_code, "UTF-8", NULL, NULL, NULL);

				(void)IniFileAddString(file, brush_section_name,
					"NAME", write_str);
				g_free(write_str);
				(void)IniFileAddString(file, brush_section_name,
					"IMAGE", window->brushes[y][x].image_file_path);
				(void)IniFileAddInteger(file, brush_section_name,
					"X", x, 10);
				(void)IniFileAddInteger(file, brush_section_name,
					"Y", y, 10);
				hot_key[0] = window->brushes[y][x].hot_key;
				(void)IniFileAddString(file, brush_section_name,
					"HOT_KEY", hot_key);

				switch(window->brushes[y][x].brush_type)
				{
				case BRUSH_TYPE_PENCIL:
					{
						PENCIL* pen = (PENCIL*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name,
							"TYPE", "PENCIL");
						(void)IniFileAddInteger(file, brush_section_name,
							"MAGNIFICATION", pen->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"SIZE", (int)(pen->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"FLOW", (int)(pen->alpha), 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"PRESSURE_SIZE", ((pen->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0 , 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"PRESSURE_FLOW", ((pen->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0 , 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"OUTLINE_HARDNESS", (int)(pen->outline_hardness), 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"ANTI_ALIAS", ((pen->flags & BRUSH_FLAG_ANTI_ALIAS) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_HARD_PEN:
					{
						HARD_PEN *pen = (HARD_PEN*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name,
							"TYPE", "HARD_PEN");
						(void)IniFileAddInteger(file, brush_section_name,
							"MAGNIFICATION", pen->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"SIZE", (int)(pen->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"FLOW", (int)(pen->alpha), 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"PRESSURE_SIZE", ((pen->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0 , 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"PRESSURE_FLOW", ((pen->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0 , 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"ANTI_ALIAS", ((pen->flags & BRUSH_FLAG_ANTI_ALIAS) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_AIR_BRUSH:
					{
						AIR_BRUSH *brush = (AIR_BRUSH*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "AIR_BRUSH");
						(void)IniFileAddInteger(file, brush_section_name, "MAGNIFICATION", brush->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)(brush->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW", (int)(brush->opacity), 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)brush->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR", (int)brush->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "ENTER_SIZE", (int)brush->enter, 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUT_SIZE", (int)brush->out, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLEND_MODE", brush->blend_mode, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((brush->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((brush->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"ANTI_ALIAS", ((brush->flags & BRUSH_FLAG_ANTI_ALIAS) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_OLD_AIR_BRUSH:
					{
						OLD_AIR_BRUSH *brush = (OLD_AIR_BRUSH*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "OLD_AIR_BRUSH");
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)(brush->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW", (int)(brush->opacity), 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)brush->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR", (int)brush->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((brush->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((brush->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_BLEND_BRUSH:
					{
						BLEND_BRUSH *brush = (BLEND_BRUSH*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "BLEND_BRUSH");
						(void)IniFileAddInteger(file, brush_section_name, "MAGNIFICATION", brush->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)(brush->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW", (int)(brush->opacity), 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)brush->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR", (int)brush->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "ENTER_SIZE", (int)brush->enter, 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUT_SIZE", (int)brush->out, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLEND_MODE", brush->blend_mode, 10);
						(void)IniFileAddInteger(file, brush_section_name, "TARGET", brush->target, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((brush->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((brush->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"ANTI_ALIAS", ((brush->flags & BRUSH_FLAG_ANTI_ALIAS) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_WATER_COLOR_BRUSH:
					{
						WATER_COLOR_BRUSH *brush = (WATER_COLOR_BRUSH*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "WATER_COLOR_BRUSH");
						(void)IniFileAddInteger(file, brush_section_name, "MAGNIFICATION", brush->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)(brush->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW", (int)brush->alpha, 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)brush->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR", (int)brush->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "COLOR_MIX", (int)brush->mix, 10);
						(void)IniFileAddInteger(file, brush_section_name, "COLOR_EXTEND",
							(int)brush->extend, 10);
						(void)IniFileAddInteger(file, brush_section_name, "ENTER_SIZE", (int)brush->enter, 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUT_SIZE", (int)brush->out, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((brush->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((brush->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_STAMP_TOOL:
					{
						STAMP *stamp = (STAMP*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "STAMP_TOOL");
						(void)IniFileAddInteger(file, brush_section_name, "SIZE",
							(int)(stamp->core.inv_scale*100), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW",
							(int)(stamp->core.flow*100), 10);
						(void)IniFileAddDouble(file, brush_section_name, "DISTANCE",
							stamp->core.stamp_distance);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_START",
							(int)(stamp->core.rotate_start * 180 / G_PI), 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_SPEED",
							(int)(stamp->core.rotate_speed * 180 / G_PI), 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_DIRECTION",
							stamp->core.rotate_direction, 10);
						(void)IniFileAddInteger(file, brush_section_name, "STAMP_ID",
							stamp->core.stamp_id, 10);
						(void)IniFileAddInteger(file, brush_section_name, "MODE",
							stamp->mode, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((stamp->core.flags & STAMP_PRESSURE_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((stamp->core.flags & STAMP_PRESSURE_FLOW) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "RANDOM_ROTATE",
							((stamp->core.flags & STAMP_RANDOM_ROTATE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "RANDOM_SIZE",
							((stamp->core.flags & STAMP_RANDOM_SIZE) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_IMAGE_BRUSH:
					{
						IMAGE_BRUSH *brush = (IMAGE_BRUSH*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "IMAGE_BRUSH");
						(void)IniFileAddInteger(file, brush_section_name, "SIZE",
							(int)(brush->core.inv_scale*100), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW",
							(int)(brush->core.flow*100), 10);
						(void)IniFileAddDouble(file, brush_section_name, "DISTANCE",
							brush->core.stamp_distance);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_START",
							(int)(brush->core.rotate_start * 180 / G_PI), 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_SPEED",
							(int)(brush->core.rotate_speed * 180 / G_PI), 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_DIRECTION",
							brush->core.rotate_direction, 10);
						(void)IniFileAddInteger(file, brush_section_name, "STAMP_ID",
							brush->core.stamp_id, 10);
						(void)IniFileAddInteger(file, brush_section_name, "MODE",
							brush->mode, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((brush->core.flags & STAMP_PRESSURE_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((brush->core.flags & STAMP_PRESSURE_FLOW) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_TO_BRUSH_DIRECTION",
							((brush->core.flags & BRUSH_FLAG_ROTATE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "RANDOM_ROTATE",
							((brush->core.flags & STAMP_RANDOM_ROTATE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "RANDOM_SIZE",
							((brush->core.flags & STAMP_RANDOM_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE_RANGE",
							(int)(brush->size_range * 100), 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_RANGE",
							brush->random_rotate_range, 10);
					}
					break;
				case BRUSH_TYPE_PICKER_BRUSH:
					{
						PICKER_BRUSH *brush = (PICKER_BRUSH*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "PICKER_BRUSH");
						(void)IniFileAddInteger(file, brush_section_name, "MAGNIFICATION", brush->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)(brush->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW", (int)(brush->alpha), 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)brush->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR", (int)brush->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLEND_MODE", brush->blend_mode, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PICK_MODE", brush->picker_mode, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PICK_TARGET", brush->picker_source, 10);
						(void)IniFileAddInteger(file, brush_section_name, "HUE", brush->add_h, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SATURATION", brush->add_s, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BRIGHTNESS", brush->add_v, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((brush->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((brush->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"ANTI_ALIAS", ((brush->flags & BRUSH_FLAG_ANTI_ALIAS) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_PICKER_IMAGE_BRUSH:
					{
						PICKER_IMAGE_BRUSH *brush = (PICKER_IMAGE_BRUSH*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "PICKER_IMAGE_BRUSH");
						(void)IniFileAddInteger(file, brush_section_name, "SIZE",
							(int)(brush->core.inv_scale*100), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW",
							(int)(brush->core.flow*100), 10);
						(void)IniFileAddDouble(file, brush_section_name, "DISTANCE",
							brush->core.stamp_distance);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_START",
							(int)(brush->core.rotate_start * 180 / G_PI), 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_SPEED",
							(int)(brush->core.rotate_speed * 180 / G_PI), 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_DIRECTION",
							brush->core.rotate_direction, 10);
						(void)IniFileAddInteger(file, brush_section_name, "STAMP_ID",
							brush->core.stamp_id, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((brush->core.flags & STAMP_PRESSURE_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((brush->core.flags & STAMP_PRESSURE_FLOW) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_TO_BRUSH_DIRECTION",
							((brush->core.flags & BRUSH_FLAG_ROTATE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "RANDOM_ROTATE",
							((brush->core.flags & STAMP_RANDOM_ROTATE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "RANDOM_SIZE",
							((brush->core.flags & STAMP_RANDOM_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE_RANGE",
							(int)(brush->size_range * 100), 10);
						(void)IniFileAddInteger(file, brush_section_name, "ROTATE_RANGE",
							brush->random_rotate_range, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PICK_MODE", brush->picker_mode, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PICK_TARGET", brush->picker_source, 10);
						(void)IniFileAddInteger(file, brush_section_name, "HUE", brush->add_h, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SATURATION", brush->add_s, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BRIGHTNESS", brush->add_v, 10);
					}
					break;
				case BRUSH_TYPE_ERASER:
					{
						ERASER *eraser = (ERASER*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "ERASER");
						(void)IniFileAddInteger(file, brush_section_name,
							"MAGNIFICATION", eraser->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"SIZE", (int)(eraser->r * 2), 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"FLOW", (int)eraser->alpha, 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)eraser->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR", (int)eraser->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((eraser->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((eraser->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_BUCKET:
					{
						BUCKET *bucket = (BUCKET*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "BUCKET");
						(void)IniFileAddInteger(file, brush_section_name,
							"THRESHOLD", bucket->threshold, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"MODE", bucket->mode, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"TARGET", bucket->target, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"EXTEND", bucket->extend, 10);
					}
					break;
				case BRUSH_TYPE_PATTERN_FILL:
					{
						PATTERN_FILL *fill = (PATTERN_FILL*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "PATTERN_FILL");
						(void)IniFileAddInteger(file, brush_section_name,
							"THRESHOLD", fill->threshold, 10);
						(void)IniFileAddInteger(file, brush_section_name, "MODE", fill->mode, 10);
						(void)IniFileAddInteger(file, brush_section_name, "TARGET", fill->target, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"PATTERN_ID", fill->pattern_id, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)fill->scale, 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW", (int)(fill->flow*100), 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"EXTEND", fill->extend, 10);
					}
					break;
				case BRUSH_TYPE_GRADATION:
					{
						GRADATION *gradation = (GRADATION*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "GRADATION");
						(void)IniFileAddInteger(file, brush_section_name,
							"COLOR_REVERSE", ((gradation->flags & GRADATION_COLOR_REVERSE)  != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_BLUR:
					{
						BLUR_TOOL *blur = (BLUR_TOOL*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "BLUR_TOOL");
						(void)IniFileAddInteger(file, brush_section_name, "MAGNIFICATION", blur->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)(blur->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW", (int)blur->alpha, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"OUTLINE_HARDNESS", (int)blur->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"BLUR", (int)blur->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "COLOR_EXTEND",
							(int)blur->color_extend, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((blur->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((blur->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_SMUDGE:
					{
						SMUDGE *smudge = (SMUDGE*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "SMUDGE");
						(void)IniFileAddInteger(file, brush_section_name, "MAGNIFICATION", smudge->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)(smudge->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "OPACITY", (int)smudge->opacity, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"OUTLINE_HARDNESS", (int)smudge->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR",
							(int)smudge->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name,
							"COLOR_EXTEND", (int)smudge->extention, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((smudge->flags & SMUDGE_PRESSURE_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_EXTENTION",
							((smudge->flags & SMUDGE_PRESSURE_EXTENTION) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_MIX_BRUSH:
					{
						MIX_BRUSH *mix = (MIX_BRUSH*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "MIX_BRUSH");
						(void)IniFileAddInteger(file, brush_section_name, "MAGNIFICATION", mix->base_scale, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)(mix->r*2), 10);
						(void)IniFileAddInteger(file, brush_section_name, "FLOW", (int)(mix->alpha), 10);
						(void)IniFileAddInteger(file, brush_section_name, "OUTLINE_HARDNESS",
							(int)mix->outline_hardness, 10);
						(void)IniFileAddInteger(file, brush_section_name, "BLUR", (int)mix->blur, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_SIZE",
							((mix->flags & BRUSH_FLAG_SIZE) != 0) ? 1 : 0, 10);
						(void)IniFileAddInteger(file, brush_section_name, "PRESSURE_FLOW",
							((mix->flags & BRUSH_FLAG_FLOW) != 0) ? 1 : 0, 10);
					}
					break;
				case BRUSH_TYPE_TEXT:
					{
						TEXT_TOOL *text = (TEXT_TOOL*)window->brushes[y][x].brush_data;
						(void)IniFileAddString(file, brush_section_name, "TYPE", "TEXT_TOOL");
						(void)IniFileAddInteger(file, brush_section_name, "MAGNIFICATION", text->base_size, 10);
						(void)IniFileAddInteger(file, brush_section_name, "SIZE", (int)text->font_size, 10);
						(void)IniFileAddInteger(file, brush_section_name, "VERTICAL",
							((text->flags & TEXT_TOOL_VERTICAL) != 0) ? 1 : 0, 10);
						(void)IniFileAddString(file, brush_section_name, "FONT_NAME",
							pango_font_family_get_name(app->font_list[text->font_id]));
					}
					break;
				case BRUSH_TYPE_SCRIPT_BRUSH:
					{
						SCRIPT *script = (SCRIPT*)window->brushes[y][x].brush_data;
						gchar *system_name = g_locale_from_utf8(script->file_name, -1, NULL, NULL, NULL);
						(void)IniFileAddString(file, brush_section_name, "TYPE", "SCRIPT_BRUSH");
						(void)IniFileAddString(file, brush_section_name, "FILE_NAME", system_name);
						g_free(system_name);
					}
					break;
				}
				brush_id++;
			}	// if(window->brushes[y][x].name != NULL)
		}	// for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH; x++)
	}	// for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT; y++)

	WriteIniFile(file, (size_t (*)(void*, size_t, size_t, void*))FileWrite);
	file->delete_func(file);

	g_object_unref(fp);
	g_object_unref(stream);
}

/*****************************************************
* SetBrushCallBack�֐�                               *
* �u���V�̃}�E�X�֘A�̃R�[���o�b�N�֐���ݒ肷��     *
* ����                                               *
* core	: �R�[���o�b�N�֐���ݒ肷��u���V�̊�{��� *
*****************************************************/
void SetBrushCallBack(BRUSH_CORE* core)
{
	switch(core->brush_type)
	{
	case BRUSH_TYPE_PENCIL:
		core->press_func = PencilPressCallBack;
		core->motion_func = PencilMotionCallBack;
		core->release_func = PencilReleaseCallBack;
		break;
	case BRUSH_TYPE_HARD_PEN:
		core->press_func = HardPenButtonPressCallBack;
		core->motion_func = HardPenMotionCallBack;
		core->release_func = HardPenButtonReleaseCallBack;
		break;
	case BRUSH_TYPE_AIR_BRUSH:
		core->press_func = AirBrushPressCallBack;
		core->motion_func = AirBrushMotionCallBack;
		core->release_func = AirBrushReleaseCallBack;
		break;
	case BRUSH_TYPE_OLD_AIR_BRUSH:
		core->press_func = OldAirBrushPressCallBack;
		core->motion_func = OldAirBrushMotionCallBack;
		core->release_func = OldAirBrushReleaseCallBack;
		break;
	case BRUSH_TYPE_WATER_COLOR_BRUSH:
		core->press_func = WaterColorBrushPressCallBack;
		core->motion_func = WaterColorBrushMotionCallBack;
		core->release_func = WaterColorBrushReleaseCallBack;
		break;
	case BRUSH_TYPE_STAMP_TOOL:
		core->press_func = StampToolButtonPressCallBack;
		core->motion_func = StampToolMotionCallBack;
		core->release_func = StampToolReleaseCallBack;
		break;
	case BRUSH_TYPE_IMAGE_BRUSH:
		core->press_func = ImageBrushPressCallBack;
		core->motion_func = ImageBrushMotionCallBack;
		core->release_func = ImageBrushReleaseCallBack;
		break;
	case BRUSH_TYPE_ERASER:
		core->press_func = EraserButtonPressCallBack;
		core->motion_func = EraserMotionCallBack;
		core->release_func = EraserReleaseCallBack;
		break;
	case BRUSH_TYPE_BUCKET:
		core->press_func = BucketPressCallBack;
		core->motion_func = BucketMotionCallBack;
		core->release_func = BucketReleaseCallBack;
		break;
	case BRUSH_TYPE_PATTERN_FILL:
		core->press_func = PatternFillPressCallBack;
		core->motion_func = PatternFillMotionCallBack;
		core->release_func = PatternFillReleaseCallBack;
		break;
	case BRUSH_TYPE_GRADATION:
		core->press_func = GradationPressCallBack;
		core->motion_func = GradationMotionCallBack;
		core->release_func = GradationReleaseCallBack;
		break;
	case BRUSH_TYPE_BLUR:
		core->press_func = BlurToolPressCallBack;
		core->motion_func = BlurToolMotionCallBack;
		core->release_func = BlurToolReleaseCallBack;
		break;
	case BRUSH_TYPE_SMUDGE:
		core->press_func = SmudgePressCallBack;
		core->motion_func = SmudgeMotionCallBack;
		core->release_func = SmudgeReleaseCallBack;
		break;
	case BRUSH_TYPE_MIX_BRUSH:
		core->press_func = MixBrushPressCallBack;
		core->motion_func = MixBrushMotionCallBack;
		core->release_func = MixBrushReleaseCallBack;
		break;
	}
}

/*************************************************************
* SetEditSelectionCallBack�֐�                               *
* �u���V�̃}�E�X�֘A�̃R�[���o�b�N�֐���I��͈͕ҏW�p�ɂ��� *
* ����                                                       *
* core	: �R�[���o�b�N�֐���ݒ肷��u���V�̊�{���         *
*************************************************************/
void SetEditSelectionCallBack(BRUSH_CORE* core)
{
	switch(core->brush_type)
	{
	case BRUSH_TYPE_PENCIL:
		core->press_func = PencilEditSelectionPressCallBack;
		core->motion_func = PencilEditSelectionMotionCallBack;
		core->release_func = PencilEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_HARD_PEN:
		core->press_func = HardPenEditSelectionPressCallBack;
		core->motion_func = HardPenEditSelectionMotionCallBack;
		core->release_func = HardPenEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_AIR_BRUSH:
		core->press_func = AirBrushEditSelectionPressCallBack;
		core->motion_func = AirBrushEditSelectionMotionCallBack;
		core->release_func = AirBrushEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_OLD_AIR_BRUSH:
		core->press_func = OldAirBrushEditSelectionPressCallBack;
		core->motion_func = OldAirBrushEditSelectionMotionCallBack;
		core->release_func = OldAirBrushEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_WATER_COLOR_BRUSH:
		core->press_func = WaterColorBrushEditSelectionPressCallBack;
		core->motion_func = WaterColorBrushEditSelectionMotionCallBack;
		core->release_func = WaterColorBrushEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_STAMP_TOOL:
		core->press_func = StampToolEditSelectionPressCallBack;
		core->motion_func = StampToolEditSelectionMotionCallBack;
		core->release_func = StampToolEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_ERASER:
		core->press_func = EraserEditSelectionButtonPressCallBack;
		core->motion_func = EraserEditSelectionMotionCallBack;
		core->release_func = EraserEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_IMAGE_BRUSH:
		core->press_func = ImageBrushEditSelectionPress;
		core->motion_func = ImageBrushEditSelectionMotion;
		core->release_func = ImageBrushEditSelectionRelease;
		break;
	case BRUSH_TYPE_BUCKET:
		core->press_func = BucketEditSelectionPressCallBack;
		core->motion_func = BucketEditSelectionMotionCallBack;
		core->release_func = BucketEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_PATTERN_FILL:
		core->press_func = PatternFillEditSelectionPressCallBack;
		core->motion_func = PatternFillEditSelectionMotionCallBack;
		core->release_func = PatternFillEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_GRADATION:
		core->press_func = GradationEditSelectionPressCallBack;
		core->motion_func = GradationEditSelectionMotionCallBack;
		core->release_func = GradationEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_BLUR:
		core->press_func = BlurToolEditSelectionPressCallBack;
		core->motion_func = BlurToolEditSelectionMotionCallBack;
		core->release_func = BlurToolEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_SMUDGE:
		core->press_func = SmudgeEditSelectionPressCallBack;
		core->motion_func = SmudgeEditSelectionMotionCallBack;
		core->release_func = SmudgeEditSelectionReleaseCallBack;
		break;
	case BRUSH_TYPE_MIX_BRUSH:
		core->press_func = MixBrushEditSelectionPressCallBack;
		core->motion_func = MixBrushEditSelectionMotionCallBack;
		core->release_func = MixBrushEditSelectionReleaseCallBack;
		break;
	}
}

#ifdef __cplusplus
}
#endif