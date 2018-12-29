#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#ifdef _OPENMP
# include <omp.h>
#endif
#include "application.h"
#include "brush_core.h"
#include "brushes.h"
#include "anti_alias.h"
#include "memory_stream.h"
#include "layer.h"
#include "memory.h"
#include "bezier.h"

#ifdef __cplusplus
extern "C" {
#endif

void ChangeBrush(
	BRUSH_CORE* core,
	void* brush_data,
	brush_core_func press_func,
	brush_core_func motion_func,
	brush_core_func release_func
)
{
	core->brush_data = brush_data;
	core->press_func = press_func;
	core->motion_func = motion_func;
	core->release_func = release_func;

	BrushCorePointReset(core);
}

void BrushCorePointReset(BRUSH_CORE* core)
{
	core->min_x = -1.0;
	core->min_y = -1.0;
	core->max_x = -1.0;
	core->max_y = -1.0;
}

/***************************************
* BrushCoreSetCirclePattern�֐�        *
* �u���V�̉~�`�摜�p�^�[�����쐬       *
* ����                                 *
* core				: �u���V�̊�{��� *
* r					: ���a             *
* outline_hardness	: �֊s�̍d��       *
* blur				: �{�P��           *
* alpha				: �s�����x         *
* color				: �F               *
***************************************/
void BrushCoreSetCirclePattern(
	BRUSH_CORE* core,
	FLOAT_T r,
	FLOAT_T outline_hardness,
	FLOAT_T blur,
	FLOAT_T alpha,
	const uint8 color[3]
)
{
	cairo_t *cairo_p;
	cairo_pattern_t *radial_pattern;
	FLOAT_T float_color[3] = {color[0] * DIV_PIXEL, color[1] * DIV_PIXEL, color[2] * DIV_PIXEL};

	if(core->brush_pattern != NULL)
	{
		cairo_pattern_destroy(core->brush_pattern);
	}

	if(core->brush_surface != NULL)
	{
		cairo_surface_destroy(core->brush_surface);
	}

	if(core->temp_pattern != NULL)
	{
		cairo_pattern_destroy(core->temp_pattern);
	}

	if(core->temp_surface != NULL)
	{
		cairo_surface_destroy(core->temp_surface);
	}

	if(core->temp_cairo != NULL)
	{
		cairo_destroy(core->temp_cairo);
	}

	core->brush_surface = cairo_image_surface_create_for_data(*core->brush_pattern_buff,
		CAIRO_FORMAT_ARGB32, (int)(r*2+1), (int)(r*2+1), (int)(r*2+1)*4);
	cairo_p = cairo_create(core->brush_surface);
	cairo_set_operator(cairo_p, CAIRO_OPERATOR_SOURCE);
	radial_pattern = cairo_pattern_create_radial(r, r, 0, r, r, r);
	cairo_pattern_set_extend(radial_pattern, CAIRO_EXTEND_NONE);
	cairo_pattern_add_color_stop_rgba(radial_pattern, 0, float_color[0],
		float_color[1], float_color[2], alpha);
	cairo_pattern_add_color_stop_rgba(radial_pattern, 1.0-blur, float_color[0],
		float_color[1], float_color[2], alpha);
	cairo_pattern_add_color_stop_rgba(radial_pattern, 1, float_color[0],
		float_color[1], float_color[2], alpha * outline_hardness);
	cairo_set_source(cairo_p, radial_pattern);
	cairo_paint(cairo_p);

	core->temp_surface = cairo_image_surface_create_for_data(*core->temp_pattern_buff,
		CAIRO_FORMAT_ARGB32, (int)(r*2+1), (int)(r*2+1), (int)(r*2+1)*4);
	core->temp_cairo = cairo_create(core->temp_surface);
	cairo_set_operator(core->temp_cairo, CAIRO_OPERATOR_SOURCE);
	core->temp_pattern = cairo_pattern_create_for_surface(core->temp_surface);

	core->brush_pattern = cairo_pattern_create_for_surface(core->brush_surface);

	cairo_pattern_destroy(radial_pattern);
	cairo_destroy(cairo_p);
}

/***********************************************
* BrushCoreSetGrayCirclePattern�֐�            *
* �u���V�̃O���[�X�P�[���~�`�摜�p�^�[�����쐬 *
* ����                                         *
* core				: �u���V�̊�{���         *
* r					: ���a                     *
* outline_hardness	: �֊s�̍d��               *
* blur				: �{�P��                   *
* alpha				: �s�����x                 *
***********************************************/
void BrushCoreSetGrayCirclePattern(
	BRUSH_CORE* core,
	FLOAT_T r,
	FLOAT_T outline_hardness,
	FLOAT_T blur,
	FLOAT_T alpha
)
{
	cairo_t *cairo_p;
	cairo_pattern_t *radial_pattern;
	int width = (int)(r * 2 + 1);

	core->stride = width + (4 - (width % 4)) % 4;

	if(core->brush_pattern != NULL)
	{
		cairo_pattern_destroy(core->brush_pattern);
	}

	if(core->brush_surface != NULL)
	{
		cairo_surface_destroy(core->brush_surface);
	}

	if(core->temp_pattern != NULL)
	{
		cairo_pattern_destroy(core->temp_pattern);
	}

	if(core->temp_surface != NULL)
	{
		cairo_surface_destroy(core->temp_surface);
	}

	if(core->temp_cairo != NULL)
	{
		cairo_destroy(core->temp_cairo);
	}

	core->brush_surface = cairo_image_surface_create_for_data(*core->brush_pattern_buff,
		CAIRO_FORMAT_A8, width, width, core->stride);
	cairo_p = cairo_create(core->brush_surface);
	cairo_set_operator(cairo_p, CAIRO_OPERATOR_SOURCE);
	radial_pattern = cairo_pattern_create_radial(r, r, 0, r, r, r);
	cairo_pattern_add_color_stop_rgba(radial_pattern, 0, 0, 0, 0, alpha);
	cairo_pattern_add_color_stop_rgba(radial_pattern, 1-blur, 0, 0, 0, alpha);
	cairo_pattern_add_color_stop_rgba(radial_pattern, 1, 0, 0, 0, alpha*outline_hardness);
	cairo_pattern_set_extend(radial_pattern, CAIRO_EXTEND_NONE);
	cairo_set_source(cairo_p, radial_pattern);
	cairo_paint(cairo_p);

	core->temp_surface = cairo_image_surface_create_for_data(*core->temp_pattern_buff,
		CAIRO_FORMAT_A8, width, width, core->stride);
	core->temp_cairo = cairo_create(core->temp_surface);
	cairo_set_operator(core->temp_cairo, CAIRO_OPERATOR_SOURCE);
	core->temp_pattern = cairo_pattern_create_for_surface(core->temp_surface);
	cairo_pattern_set_extend(core->temp_pattern, CAIRO_EXTEND_NONE);

	core->brush_pattern = cairo_pattern_create_for_surface(core->brush_surface);

	cairo_pattern_destroy(radial_pattern);
	cairo_destroy(cairo_p);
}


typedef struct _BRUSH_HISTORY_DATA
{
	int32 x, y;
	int32 width, height;
	int32 name_len;
	gchar *layer_name;
	uint8 *pixels;
} BRUSH_HISTORY_DATA;

void AddBrushHistory(
	BRUSH_CORE* core,
	LAYER* active
)
{
	BRUSH_HISTORY_DATA data;
	MEMORY_STREAM_PTR stream;
	int i;

	data.x = (int32)core->min_x - 1;
	data.y = (int32)core->min_y - 1;
	data.width = (int32)(core->max_x + 1.5 - core->min_x);
	data.height = (int32)(core->max_y + 1.5 - core->min_y);
	if(data.x < 0)
	{
		data.x = 0;
	}
	else if(data.x > active->width)
	{
		return;
	}
	if(data.y < 0)
	{
		data.y = 0;
	}
	else if(data.y > active->height)
	{
		return;
	}
	if (data.width < 0)
	{
		return;
	}
	else if(data.x + data.width > active->width)
	{
		data.width = active->width - data.x;
	}
	if(data.height < 0)
	{
		return;
	}
	else if(data.y + data.height > active->height)
	{
		data.height = active->height - data.y;
	}
	data.name_len = (int32)strlen(active->name) + 1;

	stream = CreateMemoryStream(
		offsetof(BRUSH_HISTORY_DATA, layer_name)
		+ data.name_len+data.height*data.width*active->channel);
	(void)MemWrite(&data, offsetof(BRUSH_HISTORY_DATA, layer_name), 1, stream);
	(void)MemWrite(active->name, 1, data.name_len, stream);
	for(i=0; i<data.height; i++)
	{
		(void)MemWrite(&active->pixels[(data.y+i)*active->stride+data.x*active->channel],
			1, data.width * active->channel, stream);
	}
	AddHistory(
		&active->window->history,
		core->name,
		stream->buff_ptr,
		(uint32)stream->data_size,
		BrushCoreUndoRedo,
		BrushCoreUndoRedo
	);
	(void)DeleteMemoryStream(stream);
}

void BrushCoreUndoRedo(DRAW_WINDOW* window, void* p)
{
	BRUSH_HISTORY_DATA data;
	LAYER* layer = window->layer;
	uint8* buff = (uint8*)p;
	uint8* before_data;
	int i;

	(void)memcpy(&data, buff, offsetof(BRUSH_HISTORY_DATA, layer_name));
	buff += offsetof(BRUSH_HISTORY_DATA, layer_name);
	data.layer_name = (gchar*)buff;
	buff += data.name_len;
	data.pixels = buff;

	while(strcmp(layer->name, data.layer_name) != 0)
	{
		layer = layer->next;
	}

	before_data = (uint8*)MEM_ALLOC_FUNC(data.height*data.width*layer->channel);
	for(i=0; i<data.height; i++)
	{
		(void)memcpy(&before_data[i*data.width*layer->channel],
			&layer->pixels[(data.y+i)*layer->stride+data.x*layer->channel],
			data.width*layer->channel);
	}

	for(i=0; i<data.height; i++)
	{
		(void)memcpy(&layer->pixels[(data.y+i)*layer->stride+data.x*layer->channel],
			&data.pixels[i*data.width*layer->channel], data.width*layer->channel);
	}

	(void)memcpy(data.pixels, before_data, data.height*data.width*layer->channel);

	MEM_FREE_FUNC(before_data);
}

typedef struct _EDIT_SELECTION_DATA
{
	int32 x, y;
	int32 width, height;
	uint8 *pixels;
} EDIT_SELECTION_DATA;

void EditSelectionUndoRedo(DRAW_WINDOW* window, void* p)
{
	EDIT_SELECTION_DATA *data = (EDIT_SELECTION_DATA*)p;
	uint8 *buff = (uint8*)p;
	uint8 *pixels, *src, *before_data;
	int i;

	before_data = (uint8*)MEM_ALLOC_FUNC(data->width*data->height);
	src = &window->selection->pixels[data->y*window->selection->stride+data->x];
	pixels = before_data;
	for(i=0; i<data->height; i++)
	{
		(void)memcpy(pixels, src, data->width);
		pixels += data->width;
		src += window->selection->stride;
	}

	pixels = &window->selection->pixels[data->y*window->selection->stride+data->x];
	src = &buff[offsetof(EDIT_SELECTION_DATA, pixels)];
	for(i=0; i<data->height; i++)
	{
		(void)memcpy(pixels, src, data->width);
		pixels += window->selection->stride;
		src += data->width;
	}

	(void)memcpy(&buff[offsetof(EDIT_SELECTION_DATA, pixels)], before_data,
		data->width * data->height);

	MEM_FREE_FUNC(before_data);

	if((window->flags & DRAW_WINDOW_EDIT_SELECTION) == 0)
	{
		if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) == FALSE)
		{
			window->flags &= ~(DRAW_WINDOW_HAS_SELECTION_AREA);
		}
		else
		{
			window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
		}
	}
}

void AddSelectionEditHistory(BRUSH_CORE* core, LAYER* selection)
{
	EDIT_SELECTION_DATA data;
	uint8 *buff, *pixels, *src;
	int i;

	data.x = (int32)core->min_x - 1;
	data.y = (int32)core->min_y - 1;
	data.width = (int32)(core->max_x + 1 - core->min_x);
	data.height = (int32)(core->max_y + 1 - core->min_y);
	if(data.x < 0) data.x = 0;
	if(data.y < 0) data.y = 0;
	if(data.x + data.width > selection->width) data.width = selection->width - data.x;
	if(data.y + data.height > selection->height) data.height = selection->height - data.y;

	buff = (uint8*)MEM_ALLOC_FUNC(offsetof(EDIT_SELECTION_DATA, pixels) + data.width*data.height);

	(void)memcpy(buff, &data, offsetof(EDIT_SELECTION_DATA, pixels));

	pixels = &buff[offsetof(EDIT_SELECTION_DATA, pixels)];
	src = &selection->pixels[data.y*selection->stride+data.x];
	for(i=0; i<data.height; i++)
	{
		(void)memcpy(pixels, src, data.width);
		pixels += data.width;
		src += selection->stride;
	}

	AddHistory(
		&selection->window->history,
		core->name,
		buff,
		offsetof(EDIT_SELECTION_DATA, pixels) + data.width * data.height,
		EditSelectionUndoRedo,
		EditSelectionUndoRedo
	);

	MEM_FREE_FUNC(buff);
}

/*****************************************************
* DrawCircleBrush�֐�                                *
* �u���V���}�X�N���C���[�ɕ`�悷��                   *
* ����                                               *
* window		: �L�����o�X                         *
* core			: �u���V�̊�{���                   *
* x				: �`��͈͂̍����X���W              *
* y				: �`��͈͂̍����Y���W              *
* width			: �`��͈͂̕�                       *
* height		: �`��͈͂̍���                     *
* mask			: ��ƃ��C���[�ɃR�s�[����ۂ̃}�X�N *
* zoom			: �g��E�k����                       *
* alpha			: �s�����x                           *
* blend_mode	: �������[�h                         *
*****************************************************/
void DrawCircleBrush(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	gdouble x,
	gdouble y,
	gdouble width,
	gdouble height,
	uint8** mask,
	gdouble zoom,
	gdouble alpha,
	uint16 blend_mode
)
{
	// �`����s�����߂�Cairo���
	cairo_t *update;
	cairo_surface_t *update_surface;
	cairo_surface_t *draw_surface;
	// �`�掞�̊g��E�k���A�ʒu�ݒ�p
	cairo_matrix_t matrix;
	// �摜��s���̃o�C�g��
	int stride = (int)width * 4;
	// �s�N�Z���f�[�^�����Z�b�g������W
	int start_x = (int)x, start_y = (int)y;
	// for���p�̃J�E���^
	int i;

	// �`��p��Cairo�쐬
	update_surface = cairo_surface_create_for_rectangle(
		window->mask_temp->surface_p, x, y,
			width, height);
	update = cairo_create(update_surface);

	*mask = window->mask_temp->pixels;
	if(window->app->textures.active_texture == 0)
	{	// �e�N�X�`����	
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{	// �I��͈͖�
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{	// �s�����ی얳
					// �u���V�̃T�C�Y��ݒ肵�ĕ`��
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(update, core->brush_pattern);
				cairo_paint_with_alpha(update, alpha);
			}
			else
			{	// �s�����ی�L
					// �u���V�̃T�C�Y��ݒ肵��
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				// ��x�A�ꎞ�ۑ��̈�ɕ`��
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �`�悷��ʒu�����Z�b�g
				cairo_matrix_init_translate(&matrix, 0,0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				// �`�挋�ʂ��g����
				cairo_set_source(update, core->temp_pattern);
				// �A�N�e�B�u�ȃ��C���[�Ń}�X�N
				cairo_mask_surface(update,
					window->active_layer->surface_p, - x, - y);
			}
		}
		else
		{	// �I��͈͗L
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{	// �s�����ی얳
					// �u���V�̃T�C�Y��ݒ肵��
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				// ��x�ꎞ�ۑ��̈�ɕ`��
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �`�悷��ʒu�����Z�b�g
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				// �`�挋�ʂ��g����
				cairo_set_source(update, core->temp_pattern);
				// �I��͈͂Ń}�X�N
				cairo_mask_surface(window->mask_temp->cairo_p,
					window->selection->surface_p, - x, - y);
			}
			else
			{
				// �I��͈͗L
					// �A�N�e�B�u�ȃ��C���[�ƑI��͈͂Ń}�X�N����
				// �`��p�Ɉꎞ�I��Cairo���쐬
				cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
					window->temp_layer->surface_p, x, y, width, height);
				cairo_t *update_temp = cairo_create(temp_surface);

				// �܂��͈ꎞ�ۑ����C���[�ɑI��͈͂Ń}�X�N���ĕ`��
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(update, core->temp_pattern);

				// �`��O�Ƀs�N�Z���f�[�^�����������Ă���
				for(i=0; i<height; i++)
				{
					(void)memset(&window->temp_layer->pixels[
						(i+start_y)*window->temp_layer->stride+start_x*4],
						0, stride
					);
				}

				cairo_mask_surface(update,
					window->selection->surface_p, - x, - y);
				cairo_set_source_surface(update_temp,
					update_surface, 0, 0);
				cairo_mask_surface(update_temp,
					window->active_layer->surface_p, - x, - y);

				// �g�p����}�X�N��ύX
				*mask = window->temp_layer->pixels;

				// �ꎞ�I�ɍ쐬����Cairo��j��
				cairo_surface_destroy(temp_surface);
				cairo_destroy(update_temp);
			}
		}
	}
	else
	{	// �e�N�X�`���L
			// �ꎞ�ۑ��p��Cairo�����쐬
		cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
			window->temp_layer->surface_p, x, y, width, height);
		cairo_t *update_temp = cairo_create(temp_surface);

		// �`��O�Ɉꎞ�ۑ����C���[�̃s�N�Z���f�[�^��������
		for(i=0; i<height; i++)
		{
			(void)memset(&window->temp_layer->pixels[
				(i+start_y)*window->temp_layer->stride+start_x*4],
				0, stride
			);
		}

		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{	// �I��͈͖�
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{	// �s�����ی얳
					// �g��E�k�����A�s�����x��ݒ肵�ĕ`��
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(update_temp, core->brush_pattern);
				cairo_paint_with_alpha(update_temp, alpha);
			}
			else
			{	// �s�����ی�L
					// �A�N�e�B�u�ȃ��C���[�Ń}�X�N���ĕ`��
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(update_temp, core->temp_pattern);
				cairo_mask_surface(update_temp,
					window->active_layer->surface_p, - x, - y);
			}

			cairo_set_source_surface(update, temp_surface, 0, 0);
			cairo_mask_surface(update, window->texture->surface_p, - x, - y);

			*mask = window->temp_layer->pixels;
		}
		else
		{	// �I��͈͗L
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{	// �s�����ی얳
					// �I��͈͂ƃe�N�X�`���Ń}�X�N
				// �܂��͈ꎞ�ۑ��̈�ɕs�����x���w�肵�ĕ`�悵�Ă���
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �`�挋�ʂ��g����
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(update, core->temp_pattern);
				// �I��͈͂Ń}�X�N
				cairo_mask_surface(update,
					window->selection->surface_p, - x, - y);

				// �ꎞ�ۑ��̈�̃s�N�Z���f�[�^����x���Z�b�g����
				for(i=0; i<height; i++)
				{
					(void)memset(&window->temp_layer->pixels[
						(i+start_y)*window->temp_layer->stride+start_x*4],
						0, stride
					);
				}

				// �e�N�X�`���Ń}�X�N���ĕ`��
				cairo_set_source_surface(update_temp, update_surface, 0, 0);
				cairo_mask_surface(update_temp, window->texture->surface_p, - x, - y);
			}
			else
			{	// �s�����ی�L
					// �I��͈́A�e�N�X�`���A�A�N�e�B�u�ȃ��C���[�Ń}�X�N
				// �܂��͕s�����x��ݒ肵�ĕ`�悵�Ă���
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �`�挋�ʂ��g����
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(update, core->temp_pattern);

				// �`��O�Ɉꎞ�ۑ��̈�̃s�N�Z���f�[�^��������
				for(i=0; i<height; i++)
				{
					(void)memset(&window->temp_layer->pixels[
						(i+start_y)*window->temp_layer->stride+start_x*4],
						0, stride
					);
				}

				// �I��͈͂Ń}�X�N
				cairo_mask_surface(update,
					window->selection->surface_p, - x, - y);
				cairo_set_source_surface(update_temp,
					update_surface, 0, 0);
				// �A�N�e�B�u�ȃ��C���[�Ń}�X�N
				cairo_mask_surface(update_temp,
					window->active_layer->surface_p, - x, - y);

				// 2�߂̈ꎞ�ۑ��̈�̃s�N�Z���f�[�^��������
				for(i=0; i<height; i++)
				{
					(void)memset(&window->mask_temp->pixels[
						(i+start_y)*window->mask_temp->stride+start_x*4],
						0, stride
					);
				}

				// �e�N�X�`���Ń}�X�N���ĕ`��
				cairo_set_source_surface(update, temp_surface, 0, 0);
				cairo_mask_surface(update, window->texture->surface_p, - x, - y);
			}
		}

		// �ꎞ�I�ɍ쐬����Cairo����j��
		cairo_surface_destroy(temp_surface);
		cairo_destroy(update_temp);
	}

	// �X�V�p�ɍ쐬����Cairo����j��
	cairo_surface_destroy(update_surface);
	cairo_destroy(update);

	if(*mask == window->mask_temp->pixels)
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
		CAIRO_FORMAT_ARGB32, (int)width, (int)height, stride);
	if(start_x < 0)
	{
		start_x = 0;
	}
	if(start_y < 0)
	{
		start_y = 0;
	}
	if(start_x + width > window->width)
	{
		stride = (window->width - start_x) * 4;
	}
	if(start_y + height > window->height)
	{
		stride = (window->height - start_y) * 4;
	}
	for(i=0; i<height; i++)
	{
		(void)memcpy(&window->mask->pixels[i*stride],
			&window->brush_buffer[(start_y+i)*window->stride+start_x*4], stride);
	}

	update = cairo_create(draw_surface);
	cairo_set_operator(update, window->app->layer_blend_operators[blend_mode]);
	cairo_set_source_surface(update, update_surface, 0, 0);
	cairo_paint(update);

	cairo_destroy(update);
	cairo_surface_destroy(draw_surface);
	cairo_surface_destroy(update_surface);
}

/*************************************************
* DrawCircleBrushWorkLayer�֐�                   *
* �u���V����ƃ��C���[�ɕ`�悷��                 *
* ����                                           *
* window	: �L�����o�X                         *
* core		: �u���V�̊�{���                   *
* x			: �`��͈͂̍����X���W              *
* y			: �`��͈͂̍����Y���W              *
* width		: �`��͈͂̕�                       *
* height	: �`��͈͂̍���                     *
* mask		: ��ƃ��C���[�ɃR�s�[����ۂ̃}�X�N *
* zoom		: �g��E�k����                       *
* alpha		: �s�����x                           *
*************************************************/
void DrawCircleBrushWorkLayer(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	gdouble x,
	gdouble y,
	gdouble width,
	gdouble height,
	uint8** mask,
	gdouble zoom,
	gdouble alpha
)
{
	// �`����s�����߂�Cairo���
	cairo_t *update;
	cairo_surface_t *update_surface;
	// �`�掞�̊g��E�k���A�ʒu�ݒ�p
	cairo_matrix_t matrix;
	// CAIRO�̃X�P�[�����O�Ɏg���l
	gdouble rev_zoom = 1.0 / zoom;
	// �摜��s���̃o�C�g��
	int stride = (int)width * 4;
	// �s�N�Z���f�[�^�����Z�b�g������W
	int start_x = (int)x, start_y = (int)y;
	// for���p�̃J�E���^
	int i;

	if(start_x < 0)
	{
		start_x = 0;
	}
	if(start_y < 0)
	{
		start_y = 0;
	}

	// �`��p��Cairo�쐬
	update_surface = cairo_surface_create_for_rectangle(
		window->mask_temp->surface_p, x, y,
			width, height);
	update = cairo_create(update_surface);

	*mask = window->mask_temp->pixels;
	for(i=0; i<(int)height; i++)
	{
		(void)memset(&((*mask)[(i+start_y)*stride+start_x*4]),
			0, stride
		);
	}

	if(window->app->textures.active_texture == 0)
	{	// �e�N�X�`����	
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{	// �I��͈͖�
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{	// �s�����ی얳
					// �u���V�̃T�C�Y��ݒ肵�ĕ`��
				cairo_matrix_init_scale(&matrix, rev_zoom, rev_zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(update, core->brush_pattern);
				cairo_paint_with_alpha(update, alpha);
			}
			else
			{	// �s�����ی�L
					// �u���V�̃T�C�Y��ݒ肵��
				cairo_matrix_init_scale(&matrix, rev_zoom, rev_zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				// ��x�A�ꎞ�ۑ��̈�ɕ`��
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �`�悷��ʒu�����Z�b�g
				cairo_matrix_init_translate(&matrix, 0,0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				// �`�挋�ʂ��g����
				cairo_set_source(update, core->temp_pattern);
				// �A�N�e�B�u�ȃ��C���[�Ń}�X�N
				cairo_mask_surface(update,
					window->active_layer->surface_p, - x, - y);
			}
		}
		else
		{	// �I��͈͗L
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{	// �s�����ی얳
					// �u���V�̃T�C�Y��ݒ肵��
				cairo_matrix_init_scale(&matrix, rev_zoom, rev_zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				// ��x�ꎞ�ۑ��̈�ɕ`��
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �`�悷��ʒu�����Z�b�g
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				// �`�挋�ʂ��g����
				cairo_set_source(update, core->temp_pattern);
				// �I��͈͂Ń}�X�N
				cairo_mask_surface(window->mask_temp->cairo_p,
					window->selection->surface_p, - x, - y);
			}
			else
			{
				// �I��͈͗L
					// �A�N�e�B�u�ȃ��C���[�ƑI��͈͂Ń}�X�N����
				// �`��p�Ɉꎞ�I��Cairo���쐬
				cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
					window->temp_layer->surface_p, x, y, width, height);
				cairo_t *update_temp = cairo_create(temp_surface);

				// �܂��͈ꎞ�ۑ����C���[�ɑI��͈͂Ń}�X�N���ĕ`��
				cairo_matrix_init_scale(&matrix, rev_zoom, rev_zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(update, core->temp_pattern);

				// �`��O�Ƀs�N�Z���f�[�^�����������Ă���
				for(i=0; i<height; i++)
				{
					(void)memset(&window->temp_layer->pixels[
						(i+start_y)*window->temp_layer->stride+start_x*4],
						0, stride
					);
				}

				cairo_mask_surface(update,
					window->selection->surface_p, - x, - y);
				cairo_set_source_surface(update_temp,
					update_surface, 0, 0);
				cairo_mask_surface(update_temp,
					window->active_layer->surface_p, - x, - y);

				// �g�p����}�X�N��ύX
				*mask = window->temp_layer->pixels;

				// �ꎞ�I�ɍ쐬����Cairo��j��
				cairo_surface_destroy(temp_surface);
				cairo_destroy(update_temp);
			}
		}
	}
	else
	{	// �e�N�X�`���L
			// �ꎞ�ۑ��p��Cairo�����쐬
		cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
			window->temp_layer->surface_p, x, y, width, height);
		cairo_t *update_temp = cairo_create(temp_surface);

		// �`��O�Ɉꎞ�ۑ����C���[�̃s�N�Z���f�[�^��������
		for(i=0; i<height; i++)
		{
			(void)memset(&window->temp_layer->pixels[
				(i+start_y)*window->temp_layer->stride+start_x*4],
				0, stride
			);
		}

		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
		{	// �I��͈͖�
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{	// �s�����ی얳
					// �g��E�k�����A�s�����x��ݒ肵�ĕ`��
				cairo_matrix_init_scale(&matrix, zoom, zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(update_temp, core->brush_pattern);
				cairo_paint_with_alpha(update_temp, alpha);
			}
			else
			{	// �s�����ی�L
					// �A�N�e�B�u�ȃ��C���[�Ń}�X�N���ĕ`��
				cairo_matrix_init_scale(&matrix, rev_zoom, rev_zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(update_temp, core->temp_pattern);
				cairo_mask_surface(update_temp,
					window->active_layer->surface_p, - x, - y);
			}

			cairo_set_source_surface(update, temp_surface, 0, 0);
			cairo_mask_surface(update, window->texture->surface_p, - x, - y);

			*mask = window->temp_layer->pixels;
		}
		else
		{	// �I��͈͗L
			if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
			{	// �s�����ی얳
					// �I��͈͂ƃe�N�X�`���Ń}�X�N
				// �܂��͈ꎞ�ۑ��̈�ɕs�����x���w�肵�ĕ`�悵�Ă���
				cairo_matrix_init_scale(&matrix, rev_zoom, rev_zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �`�挋�ʂ��g����
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(update, core->temp_pattern);
				// �I��͈͂Ń}�X�N
				cairo_mask_surface(update,
					window->selection->surface_p, - x, - y);

				// �ꎞ�ۑ��̈�̃s�N�Z���f�[�^����x���Z�b�g����
				for(i=0; i<height; i++)
				{
					(void)memset(&window->temp_layer->pixels[
						(i+start_y)*window->temp_layer->stride+start_x*4],
						0, stride
					);
				}

				// �e�N�X�`���Ń}�X�N���ĕ`��
				cairo_set_source_surface(update_temp, update_surface, 0, 0);
				cairo_mask_surface(update_temp, window->texture->surface_p, - x, - y);
			}
			else
			{	// �s�����ی�L
					// �I��͈́A�e�N�X�`���A�A�N�e�B�u�ȃ��C���[�Ń}�X�N
				// �܂��͕s�����x��ݒ肵�ĕ`�悵�Ă���
				cairo_matrix_init_scale(&matrix, rev_zoom, rev_zoom);
				cairo_pattern_set_matrix(core->brush_pattern, &matrix);
				cairo_set_source(core->temp_cairo, core->brush_pattern);
				cairo_paint_with_alpha(core->temp_cairo, alpha);
				// �`�挋�ʂ��g����
				cairo_matrix_init_translate(&matrix, 0, 0);
				cairo_pattern_set_matrix(core->temp_pattern, &matrix);
				cairo_set_source(update, core->temp_pattern);

				// �`��O�Ɉꎞ�ۑ��̈�̃s�N�Z���f�[�^��������
				for(i=0; i<height; i++)
				{
					(void)memset(&window->temp_layer->pixels[
						(i+start_y)*window->temp_layer->stride+start_x*4],
						0, stride
					);
				}

				// �I��͈͂Ń}�X�N
				cairo_mask_surface(update,
					window->selection->surface_p, - x, - y);
				cairo_set_source_surface(update_temp,
					update_surface, 0, 0);
				// �A�N�e�B�u�ȃ��C���[�Ń}�X�N
				cairo_mask_surface(update_temp,
					window->active_layer->surface_p, - x, - y);

				// 2�߂̈ꎞ�ۑ��̈�̃s�N�Z���f�[�^��������
				for(i=0; i<height; i++)
				{
					(void)memset(&window->mask_temp->pixels[
						(i+start_y)*window->mask_temp->stride+start_x*4],
						0, stride
					);
				}

				// �e�N�X�`���Ń}�X�N���ĕ`��
				cairo_set_source_surface(update, temp_surface, 0, 0);
				cairo_mask_surface(update, window->texture->surface_p, - x, - y);
			}
		}

		// �ꎞ�I�ɍ쐬����Cairo����j��
		cairo_surface_destroy(temp_surface);
		cairo_destroy(update_temp);
	}

	// �X�V�p�ɍ쐬����Cairo����j��
	cairo_surface_destroy(update_surface);
	cairo_destroy(update);
}

/*************************************************
* DrawImageBrush�֐�                             *
* �摜�u���V���}�X�N���C���[�ɕ`�悷��           *
* ����                                           *
* window		: �L�����o�X�̏��               *
* core			: �u���V�̊�{���               *
* x				: �`��͈͂̒��SX���W            *
* y				: �`��͈͂̒��SY���W            *
* width			: �`��͈͂̕�                   *
* height		: �`��͈͂̍���                 *
* scale			: �`�悷��g�嗦                 *
* size			: �摜�̒��ӂ̒���               *
* angle			: �摜�̊p�x                     *
* image_width	: �摜�̕�                       *
* image_height	: �摜�̍���                     *
* mask			: �������̃}�X�N���󂯂�|�C���^ *
* alpha			: �Z�x                           *
* blend_mode	: �������[�h                     *
*************************************************/
void DrawImageBrush(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	gdouble x,
	gdouble y,
	gdouble width,
	gdouble height,
	gdouble scale,
	gdouble size,
	gdouble angle,
	gdouble image_width,
	gdouble image_height,
	uint8** mask,
	gdouble alpha,
	uint16 blend_mode
)
{
	// �`�悷��摜�̃T�[�t�F�[�X
	cairo_surface_t *draw_surface;
	// �X�V���s���͈͂̃T�[�t�F�[�X
	cairo_surface_t *update_surface;
	// �X�V���s���͈͂�Cairo�R���e�L�X�g
	cairo_t *update;
	// �g��E��]�����p�̍s��f�[�^
	cairo_matrix_t matrix, reset_matrix;
	// �`��T�C�Y����p
	gdouble half_width,	half_height;
	// �g�嗦
	gdouble zoom;
	// �s�N�Z���f�[�^�̃��Z�b�g���J�n������W
	int start_x = (int)x,	start_y = (int)y;
	// �`��p�s��Ɏw�肷��ړ���
	gdouble trans_x,	trans_y;
	// ��]��̈ړ��ʌv�Z�p
	gdouble cos_value = cos(angle),	sin_value = sin(angle);
	// �`��͈͂�1�s���̃o�C�g��
	int stride;
	// for���p�̃J�E���^
	int i;

	if(start_x < 0)
	{
		width += start_x;
		start_x = 0;
		if(width <= 0)
		{
			return;
		}
	}
	else if(start_x >= window->width)
	{
		return;
	}
	if(start_x + width > window->width)
	{
		width = window->width - start_x;
	}
	if(start_y < 0)
	{
		height += start_y;
		start_y = 0;
		if(height <= 0)
		{
			return;
		}
	}
	else if(start_y >= window->height)
	{
		return;
	}
	if(start_y + height > window->height)
	{
		height = window->height - start_y;
	}

	stride = (int)width*4;
	half_width = image_width * scale * 0.5;
	half_height = image_height * scale * 0.5;
	//zoom = (1/scale) * size;
	zoom = 1 / scale;

	update_surface = cairo_surface_create_for_rectangle(
		window->mask_temp->surface_p, x - size, y - size, size*2+1, size*2+1);
	update = cairo_create(update_surface);

	for(i=0; i<height; i++)
	{
		(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
			0, stride);
	}

	trans_x = size - (half_width * cos_value + half_height * sin_value);
	trans_y = size + (half_width * sin_value - half_height * cos_value);

	// �s��f�[�^�Ɋg��E�k���A��]�p���Z�b�g
	cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_matrix_init_scale(&matrix, zoom, zoom);
	cairo_matrix_rotate(&matrix, angle);
	cairo_matrix_translate(&matrix, - trans_x, - trans_y);

	*mask = window->mask_temp->pixels;
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
				window->active_layer->surface_p, - x + size, - y + size);
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
				window->selection->surface_p, - x + size, - y + size);
		}
		else
		{
			cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
				window->temp_layer->surface_p, x-size, y-size, size*2+1, size*2+1
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
				window->selection->surface_p, - x + size, - y + size);
			cairo_set_source_surface(update_temp,
				update_surface, 0, 0);
			cairo_mask_surface(update_temp,
				window->active_layer->surface_p, - x + size, - y + size);

			*mask = window->temp_layer->pixels;

			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}
	}

	// �e�N�X�`����K�p
	if(window->app->textures.active_texture != 0)
	{
		cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
			window->temp_layer->surface_p, x - size, y - size, size*2+1, size*2+1);
		cairo_t *update_temp = cairo_create(temp_surface);

		if(*mask == window->mask_temp->pixels)
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
			cairo_mask_surface(update_temp, window->texture->surface_p, - x + size, - y + size);
			*mask = window->temp_layer->pixels;
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
			cairo_mask_surface(update, window->texture->surface_p, - x + size, - y + size);
			*mask = window->mask_temp->pixels;
		}

		cairo_surface_destroy(temp_surface);
		cairo_destroy(update_temp);
	}

	cairo_surface_destroy(update_surface);
	cairo_destroy(update);

	if(*mask == window->mask_temp->pixels)
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
		CAIRO_FORMAT_ARGB32, (int)width, (int)height, stride);
	for(i=0; i<height; i++)
	{
		(void)memcpy(&window->mask->pixels[i*stride],
			&window->brush_buffer[(start_y+i)*window->stride+start_x*4], stride);
	}

	update = cairo_create(draw_surface);
	cairo_set_operator(update, window->app->layer_blend_operators[blend_mode]);
	cairo_set_source_surface(update, update_surface, 0, 0);
	cairo_paint(update);

	cairo_destroy(update);
	cairo_surface_destroy(draw_surface);
	cairo_surface_destroy(update_surface);
}

/*************************************************
* DrawImageBrushWorkLayer�֐�                    *
* �摜�u���V����ƃ��C���[�ɕ`�悷��             *
* ����                                           *
* window		: �L�����o�X�̏��               *
* core			: �u���V�̊�{���               *
* x				: �`��͈͂̒��SX���W            *
* y				: �`��͈͂̒��SY���W            *
* width			: �`��͈͂̕�                   *
* height		: �`��͈͂̍���                 *
* scale			: �`�悷��g�嗦                 *
* size			: �摜�̒��ӂ̒���               *
* angle			: �摜�̊p�x                     *
* image_width	: �摜�̕�                       *
* image_height	: �摜�̍���                     *
* mask			: �������̃}�X�N���󂯂�|�C���^ *
* alpha			: �Z�x                           *
*************************************************/
void DrawImageBrushWorkLayer(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	gdouble x,
	gdouble y,
	gdouble width,
	gdouble height,
	gdouble scale,
	gdouble size,
	gdouble angle,
	gdouble image_width,
	gdouble image_height,
	uint8** mask,
	gdouble alpha
)
{
	// �X�V���s���͈͂̃T�[�t�F�[�X
	cairo_surface_t *update_surface;
	// �X�V���s���͈͂�Cairo�R���e�L�X�g
	cairo_t *update;
	// �g��E��]�����p�̍s��f�[�^
	cairo_matrix_t matrix, reset_matrix;
	// �`��T�C�Y����p
	gdouble half_width,	half_height;
	// �g�嗦
	gdouble zoom;
	// �s�N�Z���f�[�^�̃��Z�b�g���J�n������W
	int start_x = (int)x,	start_y = (int)y;
	// �`��p�s��Ɏw�肷��ړ���
	gdouble trans_x,	trans_y;
	// ��]��̈ړ��ʌv�Z�p
	gdouble cos_value = cos(angle),	sin_value = sin(angle);
	// �`��͈͂�1�s���̃o�C�g��
	int stride;
	// for���p�̃J�E���^
	int i;

	if(start_x < 0)
	{
		width += start_x;
		start_x = 0;
		if(width <= 0)
		{
			return;
		}
	}
	else if(start_x >= window->width)
	{
		return;
	}
	if(start_x + width > window->width)
	{
		width = window->width - start_x;
	}
	if(start_y < 0)
	{
		height += start_y;
		start_y = 0;
		if(height <= 0)
		{
			return;
		}
	}
	else if(start_y >= window->height)
	{
		return;
	}
	if(start_y + height > window->height)
	{
		height = window->height - start_y;
	}

	stride = (int)width*4;
	half_width = image_width * scale * 0.5;
	half_height = image_height * scale * 0.5;
	//zoom = (1/scale) * size;
	zoom = 1 / scale;

	update_surface = cairo_surface_create_for_rectangle(
		window->mask_temp->surface_p, x - size, y - size, size*2+1, size*2+1);
	update = cairo_create(update_surface);

	for(i=0; i<height; i++)
	{
		(void)memset(&window->mask_temp->pixels[(i+start_y)*window->mask_temp->stride+start_x*4],
			0, stride);
	}

	trans_x = size - (half_width * cos_value + half_height * sin_value);
	trans_y = size + (half_width * sin_value - half_height * cos_value);

	// �s��f�[�^�Ɋg��E�k���A��]�p���Z�b�g
	cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_matrix_init_scale(&matrix, zoom, zoom);
	cairo_matrix_rotate(&matrix, angle);
	cairo_matrix_translate(&matrix, - trans_x, - trans_y);

	*mask = window->mask_temp->pixels;
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
				window->active_layer->surface_p, - x + size, - y + size);
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
				window->selection->surface_p, - x + size, - y + size);
		}
		else
		{
			cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
				window->temp_layer->surface_p, x-size, y-size, size*2+1, size*2+1
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
				window->selection->surface_p, - x + size, - y + size);
			cairo_set_source_surface(update_temp,
				update_surface, 0, 0);
			cairo_mask_surface(update_temp,
				window->active_layer->surface_p, - x + size, - y + size);

			*mask = window->temp_layer->pixels;

			cairo_surface_destroy(temp_surface);
			cairo_destroy(update_temp);
		}
	}

	// �e�N�X�`����K�p
	if(window->app->textures.active_texture != 0)
	{
		cairo_surface_t *temp_surface = cairo_surface_create_for_rectangle(
			window->temp_layer->surface_p, x - size, y - size, size*2+1, size*2+1);
		cairo_t *update_temp = cairo_create(temp_surface);

		if(*mask == window->mask_temp->pixels)
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
			cairo_mask_surface(update_temp, window->texture->surface_p, - x + size, - y + size);
			*mask = window->temp_layer->pixels;
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
			cairo_mask_surface(update, window->texture->surface_p, - x + size, - y + size);
			*mask = window->mask_temp->pixels;
		}

		cairo_surface_destroy(temp_surface);
		cairo_destroy(update_temp);
	}

	cairo_surface_destroy(update_surface);
	cairo_destroy(update);
}

/*************************************************
* AdaptNormalBrush�֐�                           *
* �ʏ�̃u���V�̕`�挋�ʂ���ƃ��C���[�ɔ��f���� *
* ����                                           *
* window		: �L�����o�X�̏��               *
* draw_pixel	: �`�挋�ʂ̓������s�N�Z���f�[�^ *
* width			: �`��͈͂̕�                   *
* height		: �`��͈͂̍���                 *
* start_x		: �`��͈͂̍����X���W          *
* start_y		: �`��͈͂̍����Y���W          *
* anti_alias	: �A���`�G�C���A�X���s�����ۂ�   *
*************************************************/
void AdaptNormalBrush(
	DRAW_WINDOW* window,
	uint8* draw_pixel,
	int width,
	int height,
	int start_x,
	int start_y,
	int anti_alias
)
{
	uint8 *work_pixel = window->work_layer->pixels;
	int layer_stride = window->work_layer->stride;
	int i;

	if(start_x < 0)
	{
		width += start_x;
		if(width < 0)
		{
			return;
		}
		start_x = 0;
	}

	if(start_x + width > window->width)
	{
		width = window->width - start_x;
	}

	if(start_y < 0)
	{
		height += start_y;
		if(height < 0)
		{
			return;
		}
		start_y = 0;
	}

	if(start_y + height > window->height)
	{
		height = window->height - start_y;
	}

#ifdef _OPENMP
	if(height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
#pragma omp parallel for firstprivate(width, work_pixel, layer_stride, start_x, start_y, draw_pixel)
#endif
	for(i=0; i<height; i++)
	{
		uint8 *ref_pix = &work_pixel[
			(start_y+i)*layer_stride+start_x*4];
		uint8 *draw_pix = &draw_pixel[(start_y+i)*layer_stride
			+start_x*4];
		int j;

		for(j=0; j<width; j++, ref_pix+=4, draw_pix+=4)
		{
			if(ref_pix[3] < draw_pix[3])
			{
				ref_pix[0] = (uint8)((uint32)(((int)draw_pix[0]-(int)ref_pix[0])
					* draw_pix[3] >> 8) + ref_pix[0]);
				ref_pix[1] = (uint8)((uint32)(((int)draw_pix[1]-(int)ref_pix[1])
					* draw_pix[3] >> 8) + ref_pix[1]);
				ref_pix[2] = (uint8)((uint32)(((int)draw_pix[2]-(int)ref_pix[2])
					* draw_pix[3] >> 8) + ref_pix[2]);
				ref_pix[3] = (uint8)((uint32)(((int)draw_pix[3]-(int)ref_pix[3])
					* draw_pix[3] >> 8) + ref_pix[3]);
			}
		}
	}
#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(window->app->max_threads);
#endif

	// �A���`�G�C���A�X��K�p
	if(anti_alias != FALSE)
	{
		ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
			width + 3, height + 3};
		AntiAliasLayer(window->work_layer, window->temp_layer, &range);
	}
}

/*************************************************
* AdaptBlendBrush�֐�                            *
* �����u���V�̕`�挋�ʂ���ƃ��C���[�ɔ��f����   *
* ����                                           *
* window		: �L�����o�X�̏��               *
* draw_pixel	: �`�挋�ʂ̓������s�N�Z���f�[�^ *
* width			: �`��͈͂̕�                   *
* height		: �`��͈͂̍���                 *
* start_x		: �`��͈͂̍����X���W          *
* start_y		: �`��͈͂̍����Y���W          *
* anti_alias	: �A���`�G�C���A�X���s�����ۂ�   *
* blend_mode	: �������[�h                     *
*************************************************/
void AdaptBlendBrush(
	DRAW_WINDOW* window,
	uint8* draw_pixel,
	int width,
	int height,
	int start_x,
	int start_y,
	int anti_alias,
	int blend_mode
)
{
	cairo_t *update;
	cairo_surface_t *target_surface;
	cairo_surface_t *source_surface;
	uint8 *work_pixel = window->work_layer->pixels;
	uint8 *brush_buffer = window->brush_buffer;
	uint8 *mask_pixel = window->mask->pixels;
	const int stride = width * 4;
	const int layer_stride = window->work_layer->stride;
	int i;

	if(anti_alias != FALSE)
	{
		ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
			width + 3, height + 3};
		AntiAliasLayer((draw_pixel == window->mask_temp->pixels) ? window->mask_temp : window->temp_layer,
			window->mask, &range);
	}

	for(i=0; i<height; i++)
	{
		(void)memcpy(&mask_pixel[i*stride],
			&brush_buffer[(start_y+i)*layer_stride+start_x*4], stride);
	}
	target_surface = cairo_image_surface_create_for_data(mask_pixel,
		CAIRO_FORMAT_ARGB32, width, height, stride);
	source_surface = cairo_image_surface_create_for_data(&draw_pixel[start_y*layer_stride+start_x*4],
		CAIRO_FORMAT_ARGB32, width, height, layer_stride);
	update = cairo_create(target_surface);
	cairo_set_operator(update, window->app->layer_blend_operators[blend_mode]);
	cairo_set_source_surface(update, source_surface, 0, 0);
	cairo_paint(update);

	cairo_destroy(update);
	cairo_surface_destroy(source_surface);
	cairo_surface_destroy(target_surface);

#ifdef _OPENMP
	if(height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
#pragma omp parallel for firstprivate(width, work_pixel, layer_stride, start_x, start_y, draw_pixel)
#endif
	for(i=0; i<height; i++)
	{
		const uint8 *mask_pix = &mask_pixel[i*stride];
		uint8 *draw_pix = &draw_pixel[(start_y+i)*layer_stride+start_x*4];
		uint8 *work_pix = &work_pixel[(start_y+i)*layer_stride+start_x*4];
		uint8 draw_value;
		int j;

		for(j=0; j<width; j++, draw_pix+=4, work_pix+=4, mask_pix+=4)
		{
			if(draw_pix[3] > work_pix[3])
			{
				draw_value = (uint8)((uint32)(((int)mask_pix[0]) * draw_pix[3] >> 8));
				if(draw_value > work_pix[0])
				{
					work_pix[0] = (uint8)((uint32)(((int)draw_value-(int)work_pix[0])
						* draw_pix[3] >> 8) + work_pix[0]);
				}
				draw_value = (uint8)((uint32)(((int)mask_pix[1]) * draw_pix[3] >> 8));
				if(draw_value > work_pix[1])
				{
					work_pix[1] = (uint8)((uint32)(((int)draw_value-(int)work_pix[1])
						* draw_pix[3] >> 8) + work_pix[1]);
				}
				draw_value = (uint8)((uint32)(((int)mask_pix[2]) * draw_pix[3] >> 8));
				if(draw_value > work_pix[1])
				{
					work_pix[2] = (uint8)((uint32)(((int)draw_value-(int)work_pix[2])
						* draw_pix[3] >> 8) + work_pix[2]);
				}
				work_pix[3] = draw_pix[3];
			}
		}
	}
#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(window->app->max_threads);
#endif
}

/******************************************************
* UpdateBrushButtonPressDrawArea�֐�                  *
* �u���V�̃N���b�N�ɑ΂���X�V����͈͂�ݒ肷��      *
* ����                                                *
* window		: �L�����o�X�̏��                    *
* area			: �X�V�͈͂��L������A�h���X          *
* core			: �u���V�̊�{���                    *
* x				: �`��͈͂̒��S��X���W               *
* y				: �`��͈͂̒��S��Y���W               *
* size			: �u���V�̒��ӂ̒���                  *
* brush_area	: �u���V�X�g���[�N�I�����̍X�V�͈�    *
******************************************************/
void UpdateBrushButtonPressDrawArea(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T size,
	BRUSH_UPDATE_AREA* brush_area
)
{
	area->min_x = x - size;
	area->min_y = y - size;
	area->max_x = x + size;
	area->max_y = y + size;

	if(area->min_x < 0.0)
	{
		area->min_x = 0.0;
	}
	if(area->min_y < 0.0)
	{
		area->min_y = 0.0;
	}
	if(area->max_x > window->work_layer->width)
	{
		area->max_x = window->work_layer->width;
	}
	if(area->max_y > window->work_layer->height)
	{
		area->max_y = window->work_layer->height;
	}

	area->start_x = (int)area->min_x;
	area->start_y = (int)area->min_y;
	area->width = area->max_x - area->min_x;
	area->height = area->max_y - area->min_y;

	core->min_x = area->min_x;
	core->max_x = area->max_x;
	core->min_y = area->min_y;
	core->max_y = area->max_y;

	if(area->width <= 0 || area->height <= 0)
	{
		area->width = 0;
		area->height = 0;
		area->update = FALSE;
	}
	else
	{
		area->update = TRUE;
	}

	if((window->flags & DRAW_WINDOW_UPDATE_AREA_INITIALIZED) == 0)
	{
		window->update.x = area->start_x;
		window->update.y = area->start_y;
		window->update.width = (int)(area->width);
		window->update.height = (int)(area->height);

		window->flags |= DRAW_WINDOW_UPDATE_AREA_INITIALIZED;
	}
	else
	{
		int min_x = (int)area->min_x;
		int min_y = (int)area->min_y;
		if(window->update.x > min_x)
		{
			window->update.x = min_x;
		}
		if(window->update.x + window->update.width < area->max_x)
		{
			window->update.width = (int)area->max_x - window->update.x;
		}
		if(window->update.y > min_y)
		{
			window->update.y = min_y;
		}
		if(window->update.y + window->update.height < area->max_y)
		{
			window->update.height = (int)area->max_y - window->update.y;
		}
	}

	if(brush_area != NULL)
	{
		if(brush_area->initialized == FALSE)
		{
			brush_area->initialized = TRUE;
			brush_area->min_x = area->min_x;
			brush_area->min_y = area->min_y;
			brush_area->max_x = area->max_x;
			brush_area->max_y = area->max_y;
		}
		else
		{
			if(brush_area->min_x > area->min_x)
			{
				brush_area->min_x = area->min_x;
			}
			if(brush_area->min_y > area->min_y)
			{
				brush_area->min_y = area->min_y;
			}
			if(brush_area->max_x < area->max_x)
			{
				brush_area->max_x = area->max_x;
			}
			if(brush_area->max_y < area->max_y)
			{
				brush_area->max_y = area->max_y;
			}
		}
	}
}

/*****************************************************
* AdaptPickerBrush�֐�                               *
*�X�|�C�g�u���V�̕`�挋�ʂ���ƃ��C���[�ɔ��f����    *
* ����                                               *
* window			: �L�����o�X�̏��               *
* draw_pixel		: �`�挋�ʂ̓������s�N�Z���f�[�^ *
* width				: �`��͈͂̕�                   *
* height			: �`��͈͂̍���                 *
* start_x			: �`��͈͂̍����X���W          *
* start_y			: �`��͈͂̍����Y���W          *
* anti_alias		: �A���`�G�C���A�X���s�����ۂ�   *
* pick_target		: �F���E���Ώ�                   *
* picker_mode		: �F�̎��W���[�h                 *
* blend_mode		: �u���V�̍������[�h             *
* change_hue		: �F���̕ω���                   *
* change_saturation	: �ʓx�̕ω���                   *
* change_brightness	: ���x�̕ω���                   *
*****************************************************/
void AdaptPickerBrush(
	DRAW_WINDOW* window,
	uint8* draw_pixel,
	int width,
	int height,
	int start_x,
	int start_y,
	int anti_alias,
	int pick_target,
	int picker_mode,
	int blend_mode,
	int16 change_hue,
	int16 change_saturation,
	int16 change_brightness
)
{
	LAYER *target;
	HSV hsv;
	uint8 *work_pixel = window->work_layer->pixels;
	uint8 *brush_buffer = window->brush_buffer;
	uint8 *mask_pixel = window->mask->pixels;
	uint64 sum_color0, sum_color1, sum_color2, sum_color3;
	uint64 sum_color4, sum_color5;
	uint8 color[3];
	const int stride = width * 4;
	const int layer_stride = window->work_layer->stride;
	int saturation;
	int brightness;
	int i;

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	uint8 r;
#endif

	if(pick_target == COLOR_PICKER_SOURCE_ACTIVE_LAYER)
	{
		UPDATE_RECTANGLE rectangle = {start_x, start_y, width, height,
			window->mask->cairo_p, window->mask->surface_p};

		for(i=0; i<height; i++)
		{
			(void)memcpy(&window->mask->pixels[(start_y+i)*layer_stride+start_x+4],
				&window->active_layer->pixels[(start_y+i)*layer_stride+start_x+4], stride);
		}
		target = window->mask;
		window->part_layer_blend_functions[blend_mode](window->work_layer, &rectangle);
	}
	else
	{
		target = window->mixed_layer;
	}

	if(picker_mode == PICKER_MODE_SINGLE_PIXEL)
	{
		int x,	y;

		x = start_x + width / 2;
		y = start_y + height / 2;

		if(x >= window->width)
		{
			x = window->width - 1;
		}
		if(y >= window->height)
		{
			y = window->height - 1;
		}

		color[0] = target->pixels[y*layer_stride+x*4];
		color[1] = target->pixels[y*layer_stride+x*4+1];
		color[2] = target->pixels[y*layer_stride+x*4+2];
	}
	else
	{
		sum_color0 = sum_color1 = sum_color2 = sum_color4 = 0;
		sum_color3 = sum_color5 = 0;

#ifdef _OPENMP
		if(height <= MINIMUM_PARALLEL_SIZE)
		{
			omp_set_dynamic(FALSE);
			omp_set_num_threads(1);
		}
#pragma omp parallel for reduction( +: sum_color0, sum_color1, sum_color2, sum_color3, sum_color4, sum_color5) firstprivate(draw_pixel, start_x, start_y, width, layer_stride)
#endif
		for(i=0; i<height; i++)
		{
			uint8 *ref_pix = &target->pixels[(i+start_y)*layer_stride+start_x*4];
			uint8 *mask_pix = &draw_pixel[(i+start_y)*layer_stride+start_x*4];
			int j;

			for(j=0; j<width; j++, ref_pix+=4, mask_pix+=4)
			{
				sum_color0 += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
				sum_color1 += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
				sum_color2 += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
				sum_color3 += ((ref_pix[3]+1) * *mask_pix) >> 8;
				sum_color4 += ref_pix[3] * *mask_pix;
				sum_color5 += *mask_pix;
			}
		}
#ifdef _OPENMP
		omp_set_dynamic(TRUE);
		omp_set_num_threads(window->app->max_threads);
#endif
		color[0] = (uint8)((sum_color0 + sum_color3 / 2) / sum_color3);
		color[1] = (uint8)((sum_color1 + sum_color3 / 2) / sum_color3);
		color[2] = (uint8)((sum_color2 + sum_color3 / 2) / sum_color3);
	}

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	r = color[2];
	color[2] = color[0];
	color[0] = r;
#endif

	RGB2HSV_Pixel(color, &hsv);
	hsv.h += change_hue;
	while(hsv.h < 0)
	{
		hsv.h += 360;
	}
	while(hsv.h >= 360)
	{
		hsv.h -= 360;
	}
	saturation = hsv.s + change_saturation;
	if(saturation < 0)
	{
		saturation = 0;
	}
	else if(saturation > 255)
	{
		saturation = 255;
	}
	hsv.s = (uint8)saturation;
	brightness = hsv.v + change_brightness;
	if(brightness < 0)
	{
		brightness = 0;
	}
	else if(brightness > 255)
	{
		brightness = 255;
	}
	hsv.v = (uint8)brightness;

	HSV2RGB_Pixel(&hsv, color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	r = color[2];
	color[2] = color[0];
	color[0] = r;
#endif

#ifdef _OPENMP
	if(height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
#pragma omp parallel for firstprivate(work_pixel, width, layer_stride, start_x, start_y, draw_pixel)
#endif
	for(i=0; i<height; i++)
	{
		uint8 *ref_pix = &work_pixel[
			(start_y+i)*layer_stride+start_x*4];
		uint8 *mask_pix = &draw_pixel[(start_y+i)*layer_stride
			+start_x*4];
		int j;

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
				uint8 src_value = mask_pix[3];
				uint8 dst_value = ref_pix[3];
				FLOAT_T src_alpha = src_value * DIV_PIXEL;
				FLOAT_T dst_alpha = dst_value * DIV_PIXEL;
				FLOAT_T div_alpha = src_alpha + dst_alpha*(1-src_alpha);
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
#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(window->app->max_threads);
#endif

	if(anti_alias != FALSE)
	{
		ANTI_ALIAS_RECTANGLE range = {start_x - 1, start_y - 1,
			width + 3, height + 3};
		AntiAliasLayer((draw_pixel == window->mask_temp->pixels) ? window->mask_temp : window->temp_layer,
			window->mask, &range);
	}
}

/******************************************************
* UpdateBrushMotionDrawArea�֐�                       *
* �u���V�̃h���b�O�ɑ΂���X�V����͈͂�ݒ肷��      *
* ����                                                *
* window		: �L�����o�X�̏��                    *
* area			: �X�V�͈͂��L������A�h���X          *
* core			: �u���V�̊�{���                    *
* x				: �`��͈͂̒��S��X���W               *
* y				: �`��͈͂̒��S��Y���W               *
* before_x		: �O��`�掞�̃}�E�X��X���W           *
* before_y		: �O��`�掞�̃}�E�X��Y���W           *
* size			: �u���V�̒��ӂ̒���                  *
* brush_area	: �u���V�X�g���[�N�I�����̍X�V�͈�    *
******************************************************/
void UpdateBrushMotionDrawArea(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T before_x,
	FLOAT_T before_y,
	FLOAT_T size,
	BRUSH_UPDATE_AREA* brush_area
)
{
	if((window->flags & DRAW_WINDOW_UPDATE_AREA_INITIALIZED) == 0)
	{
		if(x > before_x)
		{
			area->min_x = before_x - size;
			area->max_x = x + size;
		}
		else
		{
			area->min_x = x - size;
			area->max_x = before_x + size;
		}

		if(y > before_y)
		{
			area->min_y = before_y - size;
			area->max_y = y + size;
		}
		else
		{
			area->min_y = y - size;
			area->max_y = before_y + size;
		}

		if(area->min_x < 0.0)
		{
			area->min_x = 0.0;
		}
		if(area->min_y < 0.0)
		{
			area->min_y = 0.0;
		}
		if(area->max_x > window->work_layer->width)
		{
			area->max_x = window->work_layer->width;
		}
		if(area->max_y > window->work_layer->height)
		{
			area->max_y = window->work_layer->height;
		}

		area->start_x = (int)area->min_x;
		area->start_y = (int)area->min_y;
		area->width = area->max_x - area->min_x;
		area->height = area->max_y - area->min_y;

		if(area->width <= 0 || area->height <= 0)
		{
			area->width = 0;
			area->height = 0;
			area->update = FALSE;
		}
		else
		{
			area->update = TRUE;
		}

		window->update.x = area->start_x;
		window->update.y = area->start_y;
		window->update.width = (int)(area->width);
		window->update.height = (int)(area->height);

		window->flags |= DRAW_WINDOW_UPDATE_AREA_INITIALIZED;
	}
	else
	{
		int min_x, min_y;

		area->min_x = x - size;
		area->min_y = y - size;
		area->max_x = x + size;
		area->max_y = y + size;

		if(area->min_x < 0.0)
		{
			area->min_x = 0.0;
		}
		if(area->min_y < 0.0)
		{
			area->min_y = 0.0;
		}
		if(area->max_x > window->work_layer->width)
		{
			area->max_x = window->work_layer->width;
		}
		if(area->max_y > window->work_layer->height)
		{
			area->max_y = window->work_layer->height;
		}

		area->start_x = (int)area->min_x;
		area->start_y = (int)area->min_y;
		area->width = area->max_x - area->min_x;
		area->height = area->max_y - area->min_y;

		if(area->width <= 0 || area->height <= 0)
		{
			area->update = FALSE;
		}
		else
		{
			area->update = TRUE;
		}

		min_x = (int)area->min_x;
		min_y = (int)area->min_y;

		if(window->update.x > min_x)
		{
			window->update.x = min_x;
		}
		if(window->update.x + window->update.width < area->max_x)
		{
			window->update.width = (int)area->max_x - window->update.x;
		}
		if(window->update.y > min_y)
		{
			window->update.y = min_y;
		}
		if(window->update.y + window->update.height < area->max_y)
		{
			window->update.height = (int)area->max_y - window->update.y;
		}
	}

	if(core->min_x > area->min_x)
	{
		core->min_x = area->min_x;
	}
	if(core->max_x < area->max_x)
	{
		core->max_x = area->max_x;
	}
	if(core->min_y > area->min_y)
	{
		core->min_y = area->min_y;
	}
	if(core->max_y < area->max_y)
	{
		core->max_y = area->max_y;
	}

	if(brush_area != NULL)
	{
		if(brush_area->initialized == FALSE)
		{
			brush_area->initialized = TRUE;
			brush_area->min_x = area->min_x;
			brush_area->min_y = area->min_y;
			brush_area->max_x = area->max_x;
			brush_area->max_y = area->max_y;
		}
		else
		{
			if(brush_area->min_x > area->min_x)
			{
				brush_area->min_x = area->min_x;
			}
			if(brush_area->min_y > area->min_y)
			{
				brush_area->min_y = area->min_y;
			}
			if(brush_area->max_x < area->max_x)
			{
				brush_area->max_x = area->max_x;
			}
			if(brush_area->max_y < area->max_y)
			{
				brush_area->max_y = area->max_y;
			}
		}
	}
}

/*************************************
* UpdateBrushMotionDrawAreaSize�֐�  *
* �u���V�X�V�͈͂̃T�C�Y���X�V����   *
* ����                               *
* window	: �L�����o�X�̏��       *
* area	: �X�V�͈͂��L������A�h���X *
*************************************/
void UpdateBrushMotionDrawAreaSize(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area
)
{
	if(area->max_x > window->update.x + window->update.width)
	{
		window->update.width = (int)(area->max_x - window->update.x);
	}
	if(area->max_y > window->update.y + window->update.height)
	{
		window->update.height = (int)(area->max_y - window->update.y);
	}
	/*
	if(area->min_x < window->update.x)
	{
		if(area->max_x > area->min_x + window->width)
		{
			window->update.x = (int)area->min_x;
			window->update.width = (int)(area->max_x - window->update.x);
		}
		else
		{
			window->update.width = (int)(window->update.x + window->width - area->min_x);
			window->update.x = (int)area->min_x;
		}
	}
	else
	{
		if(area->max_x > window->update.x + window->update.width)
		{
			window->width = (int)(area->max_x - window->update.x);
		}
	}

	if(area->min_y < window->update.y)
	{
		if(area->max_y > area->min_y + window->height)
		{
			window->update.y = (int)area->min_y;
			window->update.height = (int)(area->max_y - window->update.y);
		}
		else
		{
			window->update.height = (int)(window->update.y + window->height - area->min_y);
			window->update.y = (int)area->min_y;
		}
	}
	else
	{
		if(area->max_y > window->update.y + window->update.height)
		{
			window->height = (int)(area->max_y - window->update.y);
		}
	}
	*/
}

/***************************************************
* UpdateBrushScatterDrawArea�֐�                �@ *
* �u���V�̎U�z�ɑ΂���X�V����͈͂�ݒ肷��    �@ *
* ����                                             *
* window		: �L�����o�X�̏��                 *
* area			: �X�V�͈͂��L������A�h���X       *
* core			: �u���V�̊�{���                 *
* x				: �`��͈͂̒��S��X���W            *
* y				: �`��͈͂̒��S��Y���W            *
* size			: �u���V�̒��ӂ̒���               *
* brush_area	: �u���V�X�g���[�N�I�����̍X�V�͈� *
***************************************************/
void UpdateBrushScatterDrawArea(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T size,
	BRUSH_UPDATE_AREA* brush_area
)
{
	area->min_x = x - size;
	area->min_y = y - size;
	area->max_x = x + size;
	area->max_y = y + size;

	if(area->min_x < 0.0)
	{
		area->min_x = 0.0;
	}
	if(area->min_y < 0.0)
	{
		area->min_y = 0.0;
	}
	if(area->max_x > window->work_layer->width)
	{
		area->max_x = window->work_layer->width;
	}
	if(area->max_y > window->work_layer->height)
	{
		area->max_y = window->work_layer->height;
	}

	area->start_x = (int)area->min_x;
	area->start_y = (int)area->min_y;
	area->width = area->max_x - area->min_x;
	area->height = area->max_y - area->min_y;

	if(core->min_x > area->min_x)
	{
		core->min_x = area->min_x;
	}
	if(core->max_x < area->max_x)
	{
		core->max_x = area->max_x;
	}
	if(core->min_y > area->min_y)
	{
		core->min_y = area->min_y;
	}
	if(core->max_y < area->max_y)
	{
		core->max_y = area->max_y;
	}

	if(area->width <= 0 || area->height <= 0)
	{
		area->width = 0;
		area->height = 0;
		area->update = FALSE;
	}
	else
	{
		area->update = TRUE;
	}

	if((window->flags & DRAW_WINDOW_UPDATE_AREA_INITIALIZED) == 0)
	{
		window->update.x = area->start_x;
		window->update.y = area->start_y;
		window->update.width = (int)(area->width);
		window->update.height = (int)(area->height);

		window->flags |= DRAW_WINDOW_UPDATE_AREA_INITIALIZED;
	}
	else
	{
		int min_x = (int)area->min_x;
		int min_y = (int)area->min_y;
		if(window->update.x > min_x)
		{
			window->update.x = min_x;
		}
		if(window->update.x + window->update.width < area->max_x)
		{
			window->update.width = (int)area->max_x - window->update.x;
		}
		if(window->update.y > min_y)
		{
			window->update.y = min_y;
		}
		if(window->update.y + window->update.height < area->max_y)
		{
			window->update.height = (int)area->max_y - window->update.y;
		}
	}

	if(brush_area != NULL)
	{
		if(brush_area->initialized == FALSE)
		{
			brush_area->initialized = TRUE;
			brush_area->min_x = area->min_x;
			brush_area->min_y = area->min_y;
			brush_area->max_x = area->max_x;
			brush_area->max_y = area->max_y;
		}
		else
		{
			if(brush_area->min_x > area->min_x)
			{
				brush_area->min_x = area->min_x;
			}
			if(brush_area->min_y > area->min_y)
			{
				brush_area->min_y = area->min_y;
			}
			if(brush_area->max_x < area->max_x)
			{
				brush_area->max_x = area->max_x;
			}
			if(brush_area->max_y < area->max_y)
			{
				brush_area->max_y = area->max_y;
			}
		}
	}
}

/*****************************************************
* SetBrushBaseScale�֐�                              *
* �u���V�T�C�Y�̔{����ݒ肷��                       *
* ����                                               *
* widget	: �{���ݒ�p�̃R���{�{�b�N�X�E�B�W�F�b�g *
* index		: �{���̃C���f�b�N�X��ێ�����A�h���X   *
*****************************************************/
void SetBrushBaseScale(GtkWidget* widget, int* index)
{
	GtkWidget *scale = GTK_WIDGET(g_object_get_data(
		G_OBJECT(widget), "scale"));

	*index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

	switch(*index)
	{
	case 0:
		SpinScaleSetScaleLimits((SPIN_SCALE*)scale, 0.1, 10);
		SpinScaleSetStep((SPIN_SCALE*)scale, 0.1);
		SpinScaleSetPage((SPIN_SCALE*)scale, 0.1);
		break;
	case 1:
		SpinScaleSetScaleLimits((SPIN_SCALE*)scale, 1, 100);
		SpinScaleSetStep((SPIN_SCALE*)scale, 1);
		SpinScaleSetPage((SPIN_SCALE*)scale, 1);
		break;
	case 2:
		SpinScaleSetScaleLimits((SPIN_SCALE*)scale, 5, 500);
		SpinScaleSetStep((SPIN_SCALE*)scale, 1);
		SpinScaleSetPage((SPIN_SCALE*)scale, 1);
		break;
	}
}

/***************************************
* DefaultToolUpdate�֐�                *
* �f�t�H���g�̃c�[���A�b�v�f�[�g�̊֐� *
* ����                                 *
* window	: �A�N�e�B�u�ȕ`��̈�     *
* x			: �}�E�X�J�[�\����X���W    *
* y			: �}�E�X�J�[�\����Y���W    *
* dummy		: �_�~�[�|�C���^           *
***************************************/
void DefaultToolUpdate(DRAW_WINDOW* window, gdouble x, gdouble y, void* dummy)
{
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	gtk_widget_queue_draw(window->window);
}

/***************************************************
* UpdateBrushPreviewWindow�֐�                     *
* �u���V�̃v���r���[�L�����o�X���X�V����           *
* ����                                             *
* window		: �u���V�̃v���r���[�L�����o�X     *
* core			: �v���r���[����u���V�̊�{���   *
* press_func	: �N���b�N���̃R�[���o�b�N�֐�     *
* motion_func	: �h���b�O���̃R�[���o�b�N�֐�     *
* release_func	: �h���b�O�I�����̃R�[���o�b�N�֐� *
***************************************************/
void UpdateBrushPreviewWindow(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	brush_core_func press_func,
	brush_core_func motion_func,
	brush_core_func release_func
)
{
#define BRUSH_PREVIEW_MARGINE 30
	const BEZIER_POINT points[5] =
	{
		{BRUSH_PREVIEW_MARGINE, BRUSH_PREVIEW_CANVAS_HEIGHT / 2},
		{BRUSH_PREVIEW_MARGINE + (- BRUSH_PREVIEW_MARGINE + BRUSH_PREVIEW_CANVAS_WIDTH / 2) / 2, BRUSH_PREVIEW_MARGINE},
		{BRUSH_PREVIEW_CANVAS_WIDTH / 2, BRUSH_PREVIEW_CANVAS_HEIGHT / 2},
		{BRUSH_PREVIEW_CANVAS_WIDTH / 2 + ((BRUSH_PREVIEW_CANVAS_WIDTH / 2 - BRUSH_PREVIEW_MARGINE) / 2), BRUSH_PREVIEW_CANVAS_HEIGHT - BRUSH_PREVIEW_MARGINE},
		{BRUSH_PREVIEW_CANVAS_WIDTH - BRUSH_PREVIEW_MARGINE, BRUSH_PREVIEW_CANVAS_HEIGHT / 2}
	};
	BEZIER_POINT calc[4], inter[2];

	const FLOAT_T step = 1.0 / (
		(BRUSH_PREVIEW_CANVAS_WIDTH / 2) * (BRUSH_PREVIEW_CANVAS_WIDTH / 2) + (BRUSH_PREVIEW_CANVAS_HEIGHT / 2) * (BRUSH_PREVIEW_CANVAS_HEIGHT / 2));
	GdkEventButton event_info = {0};
	FLOAT_T t;
	int i;

	(void)memset(window->layer->pixels, 0, window->pixel_buf_size);

	event_info.button = 1;
	event_info.state = GDK_BUTTON1_MASK;

	press_func(window, points[0].x, points[0].y, 1, core, &event_info);

	calc[0] = points[0];
	calc[1] = points[1];
	calc[2] = points[2];
	calc[3] = points[3];
	MakeBezier3EdgeControlPoint(calc, inter);
	calc[1] = points[0];
	calc[2] = inter[0];
	calc[3] = points[1];

	t = step;
	while(t <= 1)
	{
		CalcBezier3(calc, t, inter);
		motion_func(window, inter[0].x, inter[0].y, 1,
			core, &event_info.state);
		t += step;
	}

	for(i=0; i<sizeof(points)/sizeof(*points) - 3; i++)
	{
		calc[0] = points[i];
		calc[1] = points[i+1];
		calc[2] = points[i+2];
		calc[3] = points[i+3];
		MakeBezier3ControlPoints(calc, inter);
		calc[0] = points[i+1];
		calc[1] = inter[0],	calc[2] = inter[1];
		calc[3] = points[i+2];

		t = step;
		while(t <= 1)
		{
			CalcBezier3(calc, t, inter);
			motion_func(window, inter[0].x, inter[0].y, 1,
				core, &event_info.state);
			t += step;
		}
	}

	calc[0] = points[i];
	calc[1] = points[i+1];
	calc[2] = points[i+2];
	calc[3] = points[i+3];
	MakeBezier3EdgeControlPoint(calc, inter);

	calc[0] = points[i+1];
	calc[1] = inter[1];
	calc[2] = calc[3] = points[i+2];

	t = step;
	while(t <= 1)
	{
		CalcBezier3(calc, t, inter);
		motion_func(window, inter[0].x, inter[0].y, 1,
			core, &event_info.state);
		t += step;
	}

	release_func(window, points[sizeof(points)/sizeof(*points)-1].x,
		points[sizeof(points)/sizeof(*points)-1].y, 1, core, &event_info);

	MEM_FREE_FUNC(window->history.history[0].data);
	window->history.history[0].data = NULL;
	window->history.num_step = 0;
	window->history.rest_redo = 0;
	window->history.rest_undo = 0;

	gtk_widget_queue_draw(window->window);
}

/*********************************************************
* AdaptSmudge�֐�                                        *
* �w��c�[���̍�ƃ��C���[�Ƃ̍�������                   *
* ����                                                   *
* canvas		: �L�����o�X�̏��                       *
* start_x		: �`��͈͂̍����X���W                  *
* start_y		: �`��͈͂̍����Y���W                  *
* width			: �`��͈͂̕�                           *
* height		: �`��͈͂̍���                         *
* before_width	: �O��`�掞�̕`��͈͂̕�               *
* before_height	: �O��`�掞�̕`��͈͂̍���             *
* mask			: �u���V�̕`�挋�ʂ̓������s�N�Z���f�[�^ *
* extend		: �F���т̊���                           *
* initialized	: �������ς݂��ۂ�(�������ς�:0�ȊO)     *
*********************************************************/
void AdaptSmudge(
	DRAW_WINDOW* canvas,
	int start_x,
	int start_y,
	int width,
	int height,
	int before_width,
	int before_height,
	uint8* mask,
	uint8 extend,
	int initialized
)
{
	int layer_width = canvas->width;
	int layer_height = canvas->height;
	int layer_stride = canvas->stride;
	uint8 *brush_buffer = canvas->brush_buffer;
	uint8 *work_pixel = canvas->work_layer->pixels;
	const int stride = width * 4;
	int i;

	if(initialized == FALSE)
	{
		for(i=0; i<height; i++)
		{
			uint8 *src = &work_pixel[(i+start_y)*layer_stride+start_x*4];
			uint8 *dst = &brush_buffer[width*i];
			(void)memcpy(dst, src, stride);
		}
	}
	else
	{
		int execute_width;
		int execute_height;

		execute_width = MINIMUM(width, before_width);
		execute_height = MINIMUM(height, before_height);

#ifdef _OPENMP
		if(height <= MINIMUM_PARALLEL_SIZE)
		{
			omp_set_dynamic(FALSE);
			omp_set_num_threads(1);
		}
#pragma omp parallel for firstprivate(start_x, start_y, execute_width, execute_height, brush_buffer, work_pixel, mask, layer_stride)
#endif
		for(i=0; i<execute_height; i++)
		{
			FLOAT_T c;
			uint8 *work_pix = &work_pixel[(i+start_y)*layer_stride+start_x*4];
			uint8 *mask_pix = &mask[(i+start_y)*layer_stride+start_x*4];
			uint8 *ref_pix = &brush_buffer[i*before_width];
			int j;
			uint8 t;
			uint8 before_alpha;

			for(j=0; j<execute_width; j++, mask_pix+=4, ref_pix+=4, work_pix+=4)
			{
				t = mask[3];
				before_alpha = ref_pix[3];

				c = ((0xff - extend) * work_pix[0] + extend * ref_pix[0]) / 255.0 + 0.49;
				work_pix[0] = (uint8)(((0xff-t)*work_pix[0]+t*c) / 255);
				c = ((0xff - extend) * work_pix[1] + extend * ref_pix[1]) / 255.0 + 0.49;
				work_pix[1] = (uint8)(((0xff-t)*work_pix[1]+t*c) / 255);
				c = ((0xff - extend) * work_pix[2] + extend * ref_pix[2]) / 255.0 + 0.49;
				work_pix[2] = (uint8)(((0xff-t)*work_pix[2]+t*c) / 255);
				c = ((0xff - extend) * work_pix[3] + extend * ref_pix[3]) / 255.0 + 0.49;
				work_pix[3] = (uint8)(((0xff-t)*work_pix[3]+t*c) / 255);
			}
		}
#ifdef _OPENMP
		omp_set_dynamic(TRUE);
		omp_set_num_threads(canvas->app->max_threads);
#endif
		for(i=0; i<height; i++)
		{
			(void)memcpy(&brush_buffer[i*width],
				&work_pixel[(i+start_y)*layer_stride+start_x*4], stride
			);
		}
	}
}

/*********************************************************
* AdaptSmudgeScatter�֐�                                 *
* �w��c�[���̍�ƃ��C���[�Ƃ̍�������(�U�z�p)           *
* ����                                                   *
* canvas		: �L�����o�X�̏��                       *
* start_x		: �`��͈͂̍����X���W                  *
* start_y		: �`��͈͂̍����Y���W                  *
* width			: �`��͈͂̕�                           *
* height		: �`��͈͂̍���                         *
* before_width	: �O��`�掞�̕`��͈͂̕�               *
* before_height	: �O��`�掞�̕`��͈͂̍���             *
* mask			: �u���V�̕`�挋�ʂ̓������s�N�Z���f�[�^ *
* extend		: �F���т̊���                           *
* initialized	: �������ς݂��ۂ�(�������ς�:0�ȊO)     *
*********************************************************/
void AdaptSmudgeScatter(
	DRAW_WINDOW* canvas,
	int start_x,
	int start_y,
	int width,
	int height,
	int before_width,
	int before_height,
	uint8* mask,
	uint8 extend,
	int initialized
)
{
	int layer_width = canvas->width;
	int layer_height = canvas->height;
	int layer_stride = canvas->stride;
	uint8 *brush_buffer = canvas->brush_buffer;
	uint8 *work_pixel = canvas->work_layer->pixels;
	const int stride = width * 4;
	int i;

	if(initialized != FALSE)
	{
		int execute_width;
		int execute_height;

		execute_width = MINIMUM(width, before_width);
		execute_height = MINIMUM(height, before_height);

#ifdef _OPENMP
		if(height <= MINIMUM_PARALLEL_SIZE)
		{
			omp_set_dynamic(FALSE);
			omp_set_num_threads(1);
		}
#pragma omp parallel for firstprivate(start_x, start_y, execute_width, execute_height, brush_buffer, work_pixel, mask, layer_stride)
#endif
		for(i=0; i<execute_height; i++)
		{
			FLOAT_T c;
			uint8 *work_pix = &work_pixel[(i+start_y)*layer_stride+start_x*4];
			uint8 *mask_pix = &mask[(i+start_y)*layer_stride+start_x*4];
			uint8 *ref_pix = &brush_buffer[i*before_width];
			int j;
			uint8 t;
			uint8 before_alpha;

			for(j=0; j<execute_width; j++, mask_pix+=4, ref_pix+=4, work_pix+=4)
			{
				t = mask[3];
				before_alpha = ref_pix[3];

				c = ((0xff - extend) * work_pix[0] + extend * ref_pix[0]) / 255.0 + 0.49;
				work_pix[0] = (uint8)(((0xff-t)*work_pix[0]+t*c) / 255);
				c = ((0xff - extend) * work_pix[1] + extend * ref_pix[1]) / 255.0 + 0.49;
				work_pix[1] = (uint8)(((0xff-t)*work_pix[1]+t*c) / 255);
				c = ((0xff - extend) * work_pix[2] + extend * ref_pix[2]) / 255.0 + 0.49;
				work_pix[2] = (uint8)(((0xff-t)*work_pix[2]+t*c) / 255);
				c = ((0xff - extend) * work_pix[3] + extend * ref_pix[3]) / 255.0 + 0.49;
				work_pix[3] = (uint8)(((0xff-t)*work_pix[3]+t*c) / 255);
			}
		}
#ifdef _OPENMP
		omp_set_dynamic(TRUE);
		omp_set_num_threads(canvas->app->max_threads);
#endif
	}
}

/*********************************************************
* BlendWaterBrush�֐�                                    *
* ���ʃu���V�̍�ƃ��C���[�Ƃ̍�������                   *
* ����                                                   *
* canvas				: �L�����o�X�̏��               *
* core					: �u���V�̊�{���               *
* x						: �`�悷��X���W                  *
* y						: �`�悷��Y���W                  *
* before_x				: �O��`�掞��X���W              *
* before_y				: �O��`�掞��Y���W              *
* brush_size			: �u���V�̔��a                   *
* start_x				: �`��͈͂̍����X���W          *
* start_y				: �`��͈͂̍����Y���W          *
* width					: �`��͈͂̕�                   *
* height				: �`��͈͂̍���                 *
* work_pixel			: ��ƃ��C���[�̃s�N�Z���f�[�^   *
* mask					: �`�挋�ʂ̓������s�N�Z���f�[�^ *
* brush_alpha			: �u���V�̔Z�x                   *
* brush_before_color	: �O��`�掞�ɋL�������F         *
* mix					: ���F���銄��                   *
* extend				: �F�����΂�����                 *
*********************************************************/
void BlendWaterBrush(
	DRAW_WINDOW* canvas,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T before_x,
	FLOAT_T before_y,
	FLOAT_T brush_size,
	int start_x,
	int start_y,
	int width,
	int height,
	uint8* work_pixel,
	uint8* mask,
	FLOAT_T brush_alpha,
	uint8 brush_before_color[4],
	uint8 mix,
	uint8 extend
)
{
	uint8 color[4];
	uint8 before_color[4];
	uint8 rev_alpha;
	uint8 blend_alpha;
	uint8 *brush_buffer = canvas->brush_buffer;
	int clear_start_x,	clear_start_y;
	int clear_width,	clear_height;
	int layer_width = canvas->active_layer->width;
	int layer_height = canvas->active_layer->height;
	int layer_stride = canvas->active_layer->stride;
	uint8 *alpha_pixel = &canvas->mask->pixels[layer_width*layer_height];
	unsigned int sum_color0, sum_color1, sum_color2, sum_color3, sum_color4, sum_color5;
	int i;

#ifdef _OPENMP
	if(height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
#pragma omp parallel for firstprivate(width, mask, alpha_pixel)
#endif
	for(i=0; i<height; i++)
	{
		uint8 *mask_pix = &alpha_pixel[(start_y+i)*layer_width + start_x];
		uint8 *alpha_pix = &mask[(start_y+i)*layer_stride+start_x*4+3];
		int j;
		for(j=0; j<width; j++, alpha_pix+=4, mask_pix++)
		{
			*mask_pix = *alpha_pix;
		}
	}

	sum_color0 = sum_color1 = sum_color2 = sum_color4 = 0;
								sum_color3 = sum_color5 = 1;

#ifdef _OPENMP
#pragma omp parallel for reduction( +: sum_color0, sum_color1, sum_color2, sum_color3, sum_color4, sum_color5) firstprivate(width, work_pixel, layer_width, layer_stride, start_x, start_y)
#endif
	for(i=0; i<height; i++)
	{
		uint8 *ref_pix = &work_pixel[(i+start_y)*layer_stride+start_x*4];
		uint8 *mask_pix = &alpha_pixel[(i+start_y)*layer_width+start_x];
		int j;

		for(j=0; j<width; j++, ref_pix+=4, mask_pix++)
		{
			sum_color0 += ((ref_pix[0]+1) * *mask_pix * ref_pix[3]) >> 8;
			sum_color1 += ((ref_pix[1]+1) * *mask_pix * ref_pix[3]) >> 8;
			sum_color2 += ((ref_pix[2]+1) * *mask_pix * ref_pix[3]) >> 8;
			sum_color3 += ((ref_pix[3]+1) * *mask_pix) >> 8;
			sum_color4 += *mask_pix * ref_pix[3];
			sum_color5 += *mask_pix;
		}
	}

	color[0] = (sum_color0 + sum_color3 / 2) / sum_color3;
	if(color[0] > 0xff)
	{
		color[0] = 0xff;
	}
	color[1] = (sum_color1 + sum_color3 / 2) / sum_color3;
	if(color[1] > 0xff)
	{
		color[1] = 0xff;
	}
	color[2] = (sum_color2 + sum_color3 / 2) / sum_color3;
	if(color[2] > 0xff)
	{
		color[2] = 0xff;
	}
	color[3] = (uint8)((sum_color4) / sum_color5);

	// �`�悷��F������
	rev_alpha = 0xff-mix;
	blend_alpha = (uint8)(brush_alpha * 255);
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

	if(extend != 0)
	{
		rev_alpha = 0xff-extend+1;
		color[0] = (rev_alpha*color[0]+(extend+1)*brush_before_color[0])>>8;
		color[1] = (rev_alpha*color[1]+(extend+1)*brush_before_color[1])>>8;
		color[2] = (rev_alpha*color[2]+(extend+1)*brush_before_color[2])>>8;
		color[3] = (rev_alpha*color[3]+(extend+1)*brush_before_color[3])>>8;
	}

	// ���݂̐F���L��
	brush_before_color[0] = before_color[0];
	brush_before_color[1] = before_color[1];
	brush_before_color[2] = before_color[2];
	brush_before_color[3] = before_color[3];

#undef CLEAR_MARGINE

#define COLOR_MARGINE 1

	if(x == before_x && y == before_y)
	{
#ifdef _OPENMP
#pragma omp parallel for firstprivate(width, work_pixel, layer_width, layer_stride, start_x, start_y)
#endif
		// ��ƃ��C���[�ɕ`��
		for(i=0; i<height; i++)
		{
			uint8 *ref_pix = &work_pixel[(i+start_y)*layer_stride+start_x*4];
			uint8 *mask_pix = &alpha_pixel[(i+start_y)*layer_width+start_x];
			uint8 mask_value;
			int j;

			for(j=0; j<width; j++, ref_pix+=4, mask_pix++)
			{
				mask_value = *mask_pix;
				ref_pix[3] = ((mask_value+1)*color[3]+(0xff-mask_value+1)*ref_pix[3])>>8;

				ref_pix[0] = (uint8)(((mask_value+1)*color[0]+(0xff-mask_value+1)*ref_pix[0])>>8);
				ref_pix[1] = (uint8)(((mask_value+1)*color[1]+(0xff-mask_value+1)*ref_pix[1])>>8);
				ref_pix[2] = (uint8)(((mask_value+1)*color[2]+(0xff-mask_value+1)*ref_pix[2])>>8);
			}
		}

		for(i=0; i<height; i++)
		{
			(void)memcpy(&brush_buffer[(i+start_y)*layer_width+start_x],
				&alpha_pixel[(i+start_y)*layer_width+start_x], width
			);
		}
	}
	else
	{
#ifdef _OPENMP
#pragma omp parallel for firstprivate(width, work_pixel, layer_width, layer_height, layer_stride, start_x, start_y, brush_buffer)
#endif
		for(i=0; i<height; i++)
		{
			uint8 *ref_pix = &work_pixel[(i+start_y)*layer_stride+start_x*4];
			uint8 *comp_pix = &brush_buffer[(i+start_y)*layer_width+start_x];
			uint8 *alpha_pix = &alpha_pixel[(i+start_y)*layer_width+start_x];
			uint8 *memory_pix = &alpha_pixel[layer_width*layer_height+(i+start_y)*layer_width+start_x];
			uint8 blend_alpha;
			uint8 mask_value;
			int j;

			for(j=0; j<width; j++, ref_pix+=4, comp_pix++, alpha_pix++, memory_pix++)
			{
				mask_value = *alpha_pix;

				if(mask_value > *comp_pix)
				{
					*memory_pix = mask_value;

					blend_alpha /*= dst_value*/ = (mask_value - *comp_pix);
					*comp_pix = mask_value;
					ref_pix[3] = ((blend_alpha+1)*color[3]+(0xff-blend_alpha+1)*ref_pix[3])>>8;
					blend_alpha = MINIMUM(ref_pix[3]*2, 0xff);

					ref_pix[0] = (uint8)MINIMUM(
						(((mask_value+1)*color[0]+(0xff-mask_value+1)*ref_pix[0])>>8), blend_alpha);
					ref_pix[1] = (uint8)MINIMUM(
						(((mask_value+1)*color[1]+(0xff-mask_value+1)*ref_pix[1])>>8), blend_alpha);
					ref_pix[2] = (uint8)MINIMUM(
						(((mask_value+1)*color[2]+(0xff-mask_value+1)*ref_pix[2])>>8), blend_alpha);
				}
			}
		}

#define CLEAR_MARGINE 3
		//	���u�����h�p�̃o�b�t�@���X�V
		if(x > before_x)
		{
			clear_start_x = (int)(before_x - brush_size * CLEAR_MARGINE);
			clear_width = (int)(x + brush_size * CLEAR_MARGINE);
		}
		else
		{
			clear_start_x = (int)(x - brush_size * CLEAR_MARGINE);
			clear_width = (int)(before_x + brush_size * CLEAR_MARGINE);
		}

		if(clear_start_x < 0)
		{
			clear_start_x = 0;
		}
		if(clear_width > layer_width)
		{
			clear_width = layer_width;
		}
		clear_width = clear_width - clear_start_x;

		if(y > before_y)
		{
			clear_start_y = (int)(before_y - brush_size * CLEAR_MARGINE);
			clear_height = (int)(y - brush_size * CLEAR_MARGINE);
		}
		else
		{
			clear_start_y = (int)(y - brush_size * CLEAR_MARGINE);
			clear_height = (int)(before_y + brush_size * CLEAR_MARGINE);
		}

		if(clear_start_y < 0)
		{
			clear_start_y = 0;
		}
		if(clear_height > layer_height)
		{
			clear_height = layer_height;
		}

		for(i=clear_start_y; i<clear_height; i++)
		{
			(void)memset(&alpha_pixel[layer_width*layer_height+i*layer_width+clear_start_x],
				0, clear_width);
		}
		for(i=0; i<height; i++)
		{
			(void)memcpy(&alpha_pixel[layer_width*layer_height+(i+start_y)*layer_width+start_x],
				&brush_buffer[(i+start_y)*layer_width+start_x], width);
		}
#undef CLEAR_MARGINE
		for(i=clear_start_y; i<clear_height; i++)
		{
			(void)memcpy(&brush_buffer[i*layer_width+clear_start_x],
				&alpha_pixel[layer_width*layer_height+i*layer_width+clear_start_x], clear_width);
		}
	}
#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(canvas->app->max_threads);
#endif
}

/********************************************
* InitializeBrushChainItem�֐�              *
* �ȈՃu���V�؂�ւ���1�Z�b�g�������������� *
* ����                                      *
* item	: �ȈՃu���V�؂�ւ���1�Z�b�g       *
********************************************/
void InitializeBrushChainItem(BRUSH_CHAIN_ITEM* item)
{
	(void)memset(item, 0, sizeof(*item));
	item->names = PointerArrayNew(4);
}

/*****************************************
* InitializeBrushChain�֐�               *
* �ȈՃu���V�؂�ւ��̃f�[�^������������ *
* ����                                   *
* chain	: �ȈՃu���V�؂�ւ��̃f�[�^     *
*****************************************/
void InitializeBrushChain(BRUSH_CHAIN* chain)
{
	(void)memset(chain, 0, sizeof(*chain));
	chain->chains = PointerArrayNew(4);
}

#ifdef __cplusplus
}
#endif
