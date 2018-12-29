/*
* layer.c
* ���C���[�̒ǉ��A�폜�A�����ύX�����`
*/

// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <zlib.h>
#include "layer.h"
#include "memory.h"
#include "application.h"
#include "history.h"
#include "tool_box.h"
#include "memory_stream.h"
#include "image_read_write.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************
* CreateLayer�֐�                                    *
* ����                                               *
* x				: ���C���[�����X���W                *
* y				: ���C���[�����Y���W                *
* width			: ���C���[�̕�                       *
* height		: ���C���[�̍���                     *
* channel		: ���C���[�̃`�����l����             *
* layer_type	: ���C���[�̃^�C�v                   *
* prev_layer	: �ǉ����郌�C���[�̑O�̃��C���[     *
* next_layer	: �ǉ����郌�C���[�̎��̃��C���[     *
* name			: �ǉ����郌�C���[�̖��O             *
* window		: �ǉ����郌�C���[���Ǘ�����`��̈� *
* �Ԃ�l                                             *
*	���������ꂽ�\���̂̃A�h���X                     *
*****************************************************/
LAYER* CreateLayer(
	int32 x,
	int32 y,
	int32 width,
	int32 height,
	int8 channel,
	eLAYER_TYPE layer_type,
	LAYER* prev_layer,
	LAYER* next_layer,
	const char *name,
	struct _DRAW_WINDOW* window
)
{
	// �Ԃ�l
	LAYER* ret = (LAYER*)MEM_ALLOC_FUNC(sizeof(*ret));
	// ���C���[�̃t�H�[�}�b�g
	cairo_format_t format =
		(channel > 3) ? CAIRO_FORMAT_ARGB32
			: (channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;

	// 0������
	(void)memset(ret, 0, sizeof(*ret));

	// �l�̃Z�b�g
	ret->name = (name == NULL) ? NULL : MEM_STRDUP_FUNC(name);
	ret->channel = (channel <= 4) ? channel : 4;
	ret->layer_type = (uint16)layer_type;
	ret->x = x, ret->y = y;
	ret->width = width, ret->height = height, ret->stride = width * ret->channel;
	ret->pixels = (uint8*)MEM_ALLOC_FUNC(sizeof(uint8)*width*height*channel);
	ret->alpha = 100;
	ret->window = window;

	// �s�N�Z����������
	(void)memset(ret->pixels, 0x0, sizeof(uint8)*(ret->stride*height));

	// cairo�̐���
	ret->surface_p = cairo_image_surface_create_for_data(
		ret->pixels, format, width, height, ret->stride
	);
	ret->cairo_p = cairo_create(ret->surface_p);

	if(layer_type == TYPE_VECTOR_LAYER)
	{	// �x�N�g�����C���[�Ȃ�
		VECTOR_LAYER_RECTANGLE rect = {0, 0, width, height};
		// �������C���[�ɂ��邽�߂̏���
		ret->layer_data.vector_layer_p =
			(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*ret->layer_data.vector_layer_p));
		(void)memset(ret->layer_data.vector_layer_p, 0, sizeof(*ret->layer_data.vector_layer_p));
		// ���C���������������C���[
		ret->layer_data.vector_layer_p->mix = CreateVectorLineLayer(ret, NULL, &rect);

		// ��ԉ��ɋ�̃��C���[���쐬����
		ret->layer_data.vector_layer_p->base =
			(VECTOR_DATA*)CreateVectorLine(NULL, NULL);
		(void)memset(ret->layer_data.vector_layer_p->base, 0, sizeof(VECTOR_LINE));
		ret->layer_data.vector_layer_p->base->line.base_data.layer =
			CreateVectorLineLayer(ret, NULL, &rect);
		ret->layer_data.vector_layer_p->top_data = ret->layer_data.vector_layer_p->base;
	}
	else if(layer_type == TYPE_LAYER_SET)
	{
		ret->layer_data.layer_set_p =
			(LAYER_SET*)MEM_ALLOC_FUNC(sizeof(*ret->layer_data.layer_set_p));
		(void)memset(ret->layer_data.layer_set_p, 0, sizeof(*ret->layer_data.layer_set_p));
		ret->layer_data.layer_set_p->active_under =
			CreateLayer(0, 0, width, height, channel, TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
	}
	else if(layer_type == TYPE_ADJUSTMENT_LAYER)
	{
		ret->layer_data.adjustment_layer_p =
			CreateAdjustmentLayer(ADJUSTMENT_LAYER_TYPE_BRIGHT_CONTRAST, ADJUSTMENT_LAYER_TARGET_UNDER_LAYER, prev_layer, ret);
	}
	else if(layer_type == TYPE_3D_LAYER)
	{
		ret->layer_data.project = window->first_project;
	}

	// �O��̃��C���[�����݂���Ȃ�֌W���Z�b�g
	if(prev_layer != NULL)
	{
		ret->prev = prev_layer;
		ret->layer_set = prev_layer->layer_set;
		prev_layer->next = ret;
	}
	if(next_layer != NULL)
	{
		ret->next = next_layer;
		next_layer->prev = ret;
	}

	return ret;
}

/*************************************************************
* AddLayer�֐�                                               *
* �`��̈�Ƀ��C���[��ǉ�����                               *
* ����                                                       *
* canvas		: ���C���[��ǉ�����`��̈�                 *
* layer_type	: �ǉ����郌�C���[�̃^�C�v(�ʏ�A�x�N�g����) *
* layer_name	: �ǉ����郌�C���[�̖��O                     *
* prev_layer	: �ǉ��������C���[�̉��ɂ��郌�C���[         *
* �Ԃ�l                                                     *
*	����:���C���[�\���̂̃A�h���X	���s:NULL                *
*************************************************************/
LAYER* AddLayer(
	DRAW_WINDOW* window,
	eLAYER_TYPE layer_type,
	const char* layer_name,
	LAYER* prev_layer
)
{
	LAYER *new_layer;
	LAYER *prev = prev_layer;
	LAYER *next;

	if(layer_name == NULL)
	{
		return NULL;
	}

	if(*layer_name == '\0')
	{
		return NULL;
	}

	if(CorrectLayerName(window->layer, layer_name) == FALSE)
	{
		return NULL;
	}

	if(prev == NULL)
	{
		next = window->layer;
	}
	else
	{
		next = prev->next;
	}

	new_layer = CreateLayer(0, 0, window->width, window->height, window->channel,
		layer_type, prev, next, layer_name, window);
	window->num_layer++;

	LayerViewAddLayer(new_layer, window->layer, window->app->layer_window.view, window->num_layer);

	return new_layer;
}

/*********************************************************
* RemoveLayer�֐�                                        *
* ���C���[���폜����(�L�����o�X�ɓo�^����Ă�����̂̂�) *
* ����                                                   *
* target	: �폜���郌�C���[                           *
*********************************************************/
void RemoveLayer(LAYER* target)
{
	DRAW_WINDOW *window;
	LAYER *layer = target;

	if(target == NULL || target->name == NULL || target->window == NULL)
	{
		return;
	}

	window = target->window;
	if(target->window->num_layer <= 1)
	{
		return;
	}

	DeleteLayer(&layer);
	window->num_layer--;

	UpdateDrawWindow(window);
}

/*****************************************************
* CreateDispTempLayer�֐�                            *
* �\���p�ꎞ�ۑ����C���[���쐬                       *
* ����                                               *
* x				: ���C���[�����X���W                *
* y				: ���C���[�����Y���W                *
* width			: ���C���[�̕�                       *
* height		: ���C���[�̍���                     *
* channel		: ���C���[�̃`�����l����             *
* layer_type	: ���C���[�̃^�C�v                   *
* window		: �ǉ����郌�C���[���Ǘ�����`��̈� *
* �Ԃ�l                                             *
*	���������ꂽ�\���̂̃A�h���X                     *
*****************************************************/
LAYER* CreateDispTempLayer(
	int32 x,
	int32 y,
	int32 width,
	int32 height,
	int8 channel,
	eLAYER_TYPE layer_type,
	struct _DRAW_WINDOW* window
)
{
	// �Ԃ�l
	LAYER* ret = (LAYER*)MEM_ALLOC_FUNC(sizeof(*ret));
	// ���C���[�̃t�H�[�}�b�g
	cairo_format_t format =
		(channel > 3) ? CAIRO_FORMAT_ARGB32
			: (channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;

	// �\���p�̕`��̈�T�C�Y���v�Z
	window->disp_size = (int)(2 * sqrt((width/2)*(width/2)+(height/2)*(height/2)) + 1);
	window->disp_stride = window->disp_size * 4;
	window->half_size = window->disp_size * 0.5;

	// 0������
	(void)memset(ret, 0, sizeof(*ret));

	// �l�̃Z�b�g
	ret->channel = (channel <= 4) ? channel : 4;
	ret->layer_type = (uint16)layer_type;
	ret->x = x, ret->y = y;
	ret->width = width, ret->height = height, ret->stride = width * ret->channel;
	ret->pixels = (uint8*)MEM_ALLOC_FUNC(sizeof(uint8)*window->disp_stride*window->disp_size);
	ret->alpha = 100;
	ret->window = window;

	// �s�N�Z����������
	(void)memset(ret->pixels, 0x0, sizeof(uint8)*window->disp_stride*window->disp_size);

	// cairo�̐���
	ret->surface_p = cairo_image_surface_create_for_data(
		ret->pixels, format, width, height, ret->stride
	);
	ret->cairo_p = cairo_create(ret->surface_p);

	return ret;
}

/*********************************************
* DeleteLayer�֐�                            *
* ���C���[���폜����                         *
* ����                                       *
* layer	: �폜���郌�C���[�|�C���^�̃A�h���X *
*********************************************/
void DeleteLayer(LAYER** layer)
{
	if((*layer)->layer_type == TYPE_VECTOR_LAYER)
	{
		DeleteVectorLayer(&(*layer)->layer_data.vector_layer_p);
	}
	else if((*layer)->layer_type == TYPE_TEXT_LAYER)
	{
		DeleteTextLayer(&(*layer)->layer_data.text_layer_p);
	}
	else if((*layer)->layer_type == TYPE_LAYER_SET)
	{
		DeleteLayerSet(*layer, (*layer)->window);
	}
	else if((*layer)->layer_type == TYPE_ADJUSTMENT_LAYER)
	{
		DeleteAdjustmentLayer(*layer);
	}

	if((*layer)->next != NULL)
	{
		(*layer)->next->prev = (*layer)->prev;
	}
	if((*layer)->prev != NULL)
	{
		(*layer)->prev->next = (*layer)->next;
	}

	if((*layer)->widget != NULL)
	{
		MEM_FREE_FUNC((*layer)->widget->eye);
		MEM_FREE_FUNC((*layer)->widget->pin);
		gtk_widget_destroy((*layer)->widget->box);
		MEM_FREE_FUNC((*layer)->widget);
	}

	cairo_destroy((*layer)->cairo_p);
	cairo_surface_destroy((*layer)->surface_p);
	MEM_FREE_FUNC((*layer)->name);

	if((*layer)->widget != NULL)
	{
		(*layer)->window->num_layer--;
		(*layer)->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}

	(void)memset((*layer)->window->work_layer->pixels, 0,
		(*layer)->window->pixel_buf_size);

	MEM_FREE_FUNC((*layer)->pixels);

	MEM_FREE_FUNC(*layer);

	*layer = NULL;
}

/*********************************************
* DeleteTempLayer�֐�                        *
* �ꎞ�I�ɍ쐬�������C���[���폜����         *
* ����                                       *
* layer	: �폜���郌�C���[�|�C���^�̃A�h���X *
*********************************************/
void DeleteTempLayer(LAYER** layer)
{
	if((*layer)->layer_type == TYPE_VECTOR_LAYER)
	{
		DeleteVectorLayer(&(*layer)->layer_data.vector_layer_p);
	}
	else if((*layer)->layer_type == TYPE_TEXT_LAYER)
	{
		DeleteTextLayer(&(*layer)->layer_data.text_layer_p);
	}
	else if((*layer)->layer_type == TYPE_LAYER_SET)
	{
		DeleteLayerSet(*layer, (*layer)->window);
	}

	if((*layer)->next != NULL)
	{
		(*layer)->next->prev = (*layer)->prev;
	}
	if((*layer)->prev != NULL)
	{
		(*layer)->prev->next = (*layer)->next;
	}

	cairo_destroy((*layer)->cairo_p);
	cairo_surface_destroy((*layer)->surface_p);
	MEM_FREE_FUNC((*layer)->name);

	if((*layer)->widget != NULL)
	{
		(*layer)->window->num_layer--;
		(*layer)->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}

	MEM_FREE_FUNC((*layer)->pixels);

	MEM_FREE_FUNC(*layer);

	*layer = NULL;
}

/*********************************
* DELETE_LAYER_HISTORY�\����     *
* ���C���[�폜�̗����f�[�^���i�[ *
*********************************/
typedef struct _DELETE_LAYER_HISTORY
{
	uint16 layer_type;			// ���C���[�^�C�v
	uint16 layer_mode;			// ���C���[�̍������[�h
	int32 x, y;					// ���C���[����̍��W
	// ���C���[�̕��A�����A��s���̃o�C�g��
	int32 width, height, stride;
	uint32 flags;				// ���C���[�̕\���E��\�����̃t���O
	int8 channel;				// ���C���[�̃`�����l����
	int8 alpha;					// ���C���[�̕s�����x
	uint16 name_length;			// ���C���[�̖��O�̒���
	gchar* name;				// ���C���[�̖��O
	uint16 prev_name_length;	// �O�̃��C���[�̕�����
	gchar* prev_name;			// �O�̃��C���[�̖��O
	size_t iamge_data_size;		// �摜�f�[�^�̃o�C�g��
	uint8* image_data;			// �摜�f�[�^
} DELETE_LAYER_HISTORY;

/*****************************
* DeleteLayerUndo�֐�        *
* ���C���[�폜����̃A���h�D *
* ����                       *
* window	: �`��̈�̏�� *
* p			: �����f�[�^     *
*****************************/
static void DeleteLayerUndo(DRAW_WINDOW* window, void* p)
{
	// �����f�[�^
	DELETE_LAYER_HISTORY history;
	// �����f�[�^�ǂݍ��ݗp�X�g���[��
	MEMORY_STREAM stream;
	// ���C���[�̃s�N�Z���f�[�^
	uint8* pixels;
	// ���C���[�̕��A�����A��s���̃o�C�g��
	gint32 width, height, stride;
	// �����f�[�^�o�C�g��
	size_t data_size;
	// ���������郌�C���[
	LAYER* layer;
	// �O�̃��C���[
	LAYER* prev;

	// �����f�[�^��0������
	(void)memset(&history, 0, sizeof(history));

	// �����f�[�^�̃o�C�g�����擾
	(void)memcpy(&data_size, p, sizeof(data_size));

	// �X�g���[����������
	stream.block_size = 1;
	stream.data_size = data_size;
	stream.buff_ptr = (uint8*)p;
	stream.data_point = sizeof(data_size);

	// ���C���[����ǂݍ���
	(void)MemRead(&history, 1,
		offsetof(DELETE_LAYER_HISTORY, name_length), &stream);
	// ���C���[�̖��O��ǂݍ���
	(void)MemRead(&history.name_length,
		sizeof(history.name_length), 1, &stream);
	history.name = (gchar*)MEM_ALLOC_FUNC(history.name_length);
	(void)MemRead(history.name, 1, history.name_length, &stream);
	// �O�̃��C���[�̖��O��ǂݍ���
	(void)MemRead(&history.prev_name_length,
		sizeof(history.prev_name_length), 1, &stream);
	// �O�̃��C���[�̗L�����m�F
		// ���łɑO�̃��C���[�̃|�C���^���Z�b�g����
	if(history.prev_name_length != 0)
	{
		history.prev_name = (gchar*)MEM_ALLOC_FUNC(history.prev_name_length);
		(void)MemRead(history.prev_name, 1,
			history.prev_name_length, &stream);
		prev = SearchLayer(window->layer, history.prev_name);
	}
	else
	{
		prev = NULL;
	}

	// ���������郌�C���[�쐬
	layer = CreateLayer(
		history.x, history.y, history.width, history.height, history.channel,
		history.layer_type, prev, (prev == NULL) ? window->layer : prev->next,
		history.name, window
	);

	// ���C���[�̃s�N�Z���f�[�^��ǂݍ���
	(void)MemSeek(&stream, sizeof(size_t), SEEK_CUR);
	pixels = ReadPNGStream(
		&stream,
		(size_t (*)(void*, size_t, size_t, void*))MemRead,
		&width, &height, &stride
	);

	// ���C���[�̃s�N�Z���������ɃR�s�[
	(void)memcpy(layer->pixels, pixels, stride*height);

	MEM_FREE_FUNC(history.name);
	MEM_FREE_FUNC(history.prev_name);
	MEM_FREE_FUNC(pixels);

	// �`��̈���X�V
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	// ���C���[�̐����X�V
	window->num_layer++;

	// ���C���[�r���[�Ƀ��C���[��ǉ�
	LayerViewAddLayer(layer, window->layer,
		window->app->layer_window.view, window->num_layer);
}

/*****************************
* DeleteLayerRedo�֐�        *
* ���C���[�폜����̃��h�D   *
* ����                       *
* window	: �`��̈�̏�� *
* p			: �����f�[�^     *
*****************************/
static void DeleteLayerRedo(DRAW_WINDOW* window, void* p)
{
	// �폜���C���[
	LAYER* delete_layer;
	// �폜���C���[���ǂݍ��ݗp
	MEMORY_STREAM stream;
	// �폜���C���[�̖��O
	gchar* name;
	// �폜���C���[���̒���
	uint16 name_length;

	// �X�g���[����������
	stream.block_size = 1;
	(void)memcpy(&stream.data_size, p, sizeof(stream.data_size));
	stream.data_point =
		offsetof(DELETE_LAYER_HISTORY, name_length) + sizeof(size_t);
	stream.buff_ptr = (uint8*)p;

	// �폜���C���[���̒������擾
	(void)MemRead(&name_length, sizeof(name_length), 1, &stream);
	name = (gchar*)MEM_ALLOC_FUNC(name_length);
	(void)MemRead(name, 1, name_length, &stream);

	delete_layer = SearchLayer(window->layer, name);
	DeleteLayer(&delete_layer);

	MEM_FREE_FUNC(name);
}

/*********************************
* AddDeleteLayerHistory�֐�      *
* ���C���[�̍폜�����f�[�^���쐬 *
* ����                           *
* window	: �`��̈�̃f�[�^   *
* target	: �폜���郌�C���[   *
*********************************/
void AddDeleteLayerHistory(
	DRAW_WINDOW* window,
	LAYER* target
)
{
// �f�[�^�̈��k���x��
#define DATA_COMPRESSION_LEVEL 8
	// �����f�[�^
	DELETE_LAYER_HISTORY history;
	// PNG�f�[�^���쐬���邽�߂̃X�g���[��
	MEMORY_STREAM_PTR stream = NULL;
	// �����f�[�^���쐬���邽�߂̃X�g���[��
	MEMORY_STREAM_PTR history_data =
		CreateMemoryStream(target->stride*target->height*4);
	// �O�̃��C���[�̖��O�̒���
	uint16 name_length;
	// �����f�[�^�̃o�C�g��
	size_t data_size;

	// �f�[�^�̃o�C�g�����L�����镔�����󂯂�
	(void)MemSeek(history_data, sizeof(size_t), SEEK_SET);

	// ���C���[�̃f�[�^�𗚗��ɃZ�b�g
	history.layer_type = target->layer_type;
	history.layer_mode = target->layer_mode;
	history.x = target->x;
	history.y = target->y;
	history.width = target->width;
	history.height = target->height;
	history.stride = target->stride;
	history.flags = target->flags;
	history.channel = target->channel;
	history.alpha = target->alpha;
	// �X�g���[���֏�������
	(void)MemWrite(&history, 1,
		offsetof(DELETE_LAYER_HISTORY, name_length), history_data);

	// ���C���[�̖��O����������
	name_length = (uint16)strlen(target->name) + 1;
	(void)MemWrite(&name_length, sizeof(name_length), 1, history_data);
	(void)MemWrite(target->name, 1, name_length, history_data);

	// �O�̃��C���[�̖��O����������
	if(target->prev == NULL)
	{
		name_length = 0;
		(void)MemWrite(&name_length, sizeof(name_length), 1, history_data);
	}
	else
	{
		name_length = (uint16)strlen(target->name) + 1;
		(void)MemWrite(&name_length, sizeof(name_length), 1, history_data);
		(void)MemWrite(target->prev->name, 1, name_length, history_data);
	}

	if(target->layer_type == TYPE_NORMAL_LAYER)
	{
		stream =
			CreateMemoryStream(target->stride*target->height*2);

		// �폜���郌�C���[�̃s�N�Z���f�[�^����PNG�f�[�^�쐬
		WritePNGStream(
			(void*)stream,
			(size_t (*)(void*, size_t, size_t, void*))MemWrite,
			NULL,
			target->pixels,
			target->width,
			target->height,
			target->stride,
			target->channel,
			0,
			DATA_COMPRESSION_LEVEL
		);

		// PNG�f�[�^����������
		(void)MemWrite(&stream->data_point, sizeof(stream->data_point), 1, history_data);
		(void)MemWrite(stream->buff_ptr, 1, stream->data_point, history_data);
	}
	else if(target->layer_type == TYPE_VECTOR_LAYER)
	{	// �x�N�g��������������
		VECTOR_LINE* line = target->layer_data.vector_layer_p->base->line.base_data.next;
		size_t stream_size = 0;
		VECTOR_LINE_BASE_DATA base;

		while(line != NULL)
		{
			if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
			{
				stream_size += sizeof(VECTOR_LINE_BASE_DATA) + sizeof(VECTOR_POINT)*line->num_points;
			}
			else if(line->base_data.vector_type == VECTOR_TYPE_SQUARE)
			{
				stream_size += sizeof(VECTOR_SQUARE);
			}
			else if(line->base_data.vector_type == VECTOR_TYPE_ECLIPSE)
			{
				stream_size += sizeof(VECTOR_ECLIPSE);
			}
			line = line->base_data.next;
		}

		if(stream_size != 0)
		{
			stream = CreateMemoryStream(stream_size);
			line = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next;
			while(line != NULL)
			{
				if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					base.vector_type = line->base_data.vector_type;
					base.num_points = line->num_points;
					base.blur = line->blur;
					base.outline_hardness = line->outline_hardness;

					(void)MemWrite(&base, sizeof(base), 1, stream);
					(void)MemWrite(line->points, sizeof(*line->points), line->num_points, stream);
				}
				else
				{
					WriteVectorShapeData((VECTOR_DATA*)line, stream);
				}

				line = line->base_data.next;
			}
		
			// �����f�[�^�ɒǉ�����
			(void)MemWrite(stream->buff_ptr, 1, stream->data_point, history_data);
		}
	}
	else if(target->layer_type == TYPE_ADJUSTMENT_LAYER)
	{
		stream =
			CreateMemoryStream(sizeof(*target->layer_data.adjustment_layer_p));

		WriteAdjustmentLayerData((void*)stream,
			(stream_func_t)MemWrite, target->layer_data.adjustment_layer_p);
	}

	// �f�[�^�o�C�g������������
	data_size = history_data->data_point;
	(void)MemSeek(history_data, 0, SEEK_SET);
	(void)MemWrite(&data_size, sizeof(data_size), 1,
		history_data);

	// �����ɒǉ�
	AddHistory(&window->history, window->app->labels->menu.delete_layer,
		history_data->buff_ptr, (uint32)data_size, DeleteLayerUndo, DeleteLayerRedo
	);

	(void)DeleteMemoryStream(stream);
	(void)DeleteMemoryStream(history_data);
}

/***********************************************************
* ResizeLayerBuffer�֐�                                    *
* ���C���[�̃s�N�Z���f�[�^�̃T�C�Y��ύX����               *
* ����                                                     *
* target		: �s�N�Z���f�[�^�̃T�C�Y��ύX���郌�C���[ *
* new_width		: �V�������C���[�̕�                       *
* new_height	: �V�������C���[�̍���                     *
***********************************************************/
void ResizeLayerBuffer(
	LAYER* target,
	int32 new_width,
	int32 new_height
)
{
	// CAIRO�ɐݒ肷��t�H�[�}�b�g���
	cairo_format_t format = (target->channel == 4) ?
		CAIRO_FORMAT_ARGB32 : (target->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;
	// ���C���[�ɐV�������A�����A��s���̃o�C�g����ݒ�
	target->width = new_width;
	target->height = new_height;
	target->stride = new_width * target->channel;

	// CAIRO����x�j������
	cairo_surface_destroy(target->surface_p);
	cairo_destroy(target->cairo_p);

	// �s�N�Z���f�[�^��CAIRO����т��̃T�[�t�F�[�X���ēx�쐬
	target->pixels = (uint8*)MEM_REALLOC_FUNC(target->pixels, target->stride*target->height);
	target->surface_p = cairo_image_surface_create_for_data(
		target->pixels, format, new_width, new_height, target->stride);
	target->cairo_p = cairo_create(target->surface_p);
}

/*******************************************
* ResizeLayer�֐�                          *
* ���C���[�̃T�C�Y��ύX����(�g��k���L)   *
* ����                                     *
* target		: �T�C�Y��ύX���郌�C���[ *
* new_width		: �V�������C���[�̕�       *
* new_height	: �V�������C���[�̍���     *
*******************************************/
void ResizeLayer(
	LAYER* target,
	int32 new_width,
	int32 new_height
)
{
	// CAIRO���쐬�������̂Ń��[�J���ň�x�A�h���X���󂯂�
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	uint8 *pixels;
	// CAIRO�ɐݒ肷��t�H�[�}�b�g���
	cairo_format_t format = (target->channel == 4) ?
		CAIRO_FORMAT_ARGB32 : (target->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;
	// �T�C�Y�ύX���̕��E�����ɑ΂���g��k����
	gdouble zoom_x = (gdouble)new_width / (gdouble)target->width,
		zoom_y = (gdouble)new_height / (gdouble)target->height;

	// �s�N�Z���f�[�^�̃������̈���m��
	pixels = (uint8*)MEM_ALLOC_FUNC(
		sizeof(*pixels)*new_width*new_height*target->channel);
	// 0������
	(void)memset(pixels, 0, sizeof(*pixels)*new_width*new_height*target->channel);

	// CAIRO���쐬
	surface_p = cairo_image_surface_create_for_data(pixels,
		format, new_width, new_height, new_width * target->channel);
	cairo_p = cairo_create(surface_p);

	// ���C���[�̎�ނŏ����؂�ւ�
	switch(target->layer_type)
	{
	case TYPE_NORMAL_LAYER:	// �ʏ탌�C���[
		// �s�N�Z���f�[�^���g��or�k�����Ďʂ�
		cairo_scale(cairo_p, zoom_x, zoom_y);
		cairo_set_source_surface(cairo_p, target->surface_p, 0, 0);
		cairo_paint(cairo_p);
		// �g��k���������ɖ߂�
		cairo_scale(cairo_p, 1/zoom_x, 1/zoom_y);

		// �ύX���I������̂Ńs�N�Z���f�[�^��CAIRO�̏����X�V
		(void)MEM_FREE_FUNC(target->pixels);
		target->pixels = pixels;
		cairo_surface_destroy(target->surface_p);
		target->surface_p = surface_p;
		cairo_destroy(target->cairo_p);
		target->cairo_p = cairo_p;

		break;
	case TYPE_VECTOR_LAYER:	// �x�N�g�����C���[
		{
			// ���W���C������x�N�g���f�[�^
			VECTOR_LINE *line = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next;
			// ��ԉ��̃��C���̓������Ċm��
			VECTOR_LINE_LAYER *line_layer = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.layer;
			int i;	// for���p�̃J�E���^

			// ��Ƀ��C���[�̃s�N�Z���f�[�^��CAIRO�����X�V
			(void)MEM_FREE_FUNC(target->pixels);
			target->pixels = pixels;
			cairo_surface_destroy(target->surface_p);
			target->surface_p = surface_p;
			cairo_destroy(target->cairo_p);
			target->cairo_p = cairo_p;

			// �S�Ẵ��C���̍��W���C��
			while(line != NULL)
			{
				if(line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
				{
					for(i=0; i<line->num_points; i++)
					{
						line->points[i].x *= zoom_x;
						line->points[i].y *= zoom_y;
						line->points[i].size *= (zoom_x + zoom_y) * 0.5;
					}
				}
				else if(line->base_data.vector_type == VECTOR_TYPE_SQUARE)
				{
					VECTOR_SQUARE *square = (VECTOR_SQUARE*)line;
					square->left *= zoom_x;
					square->top *= zoom_y;;
					square->width *= zoom_x;
					square->height *= zoom_y;
					square->line_width *= (zoom_x + zoom_y) * 0.5;
				}
				else if(line->base_data.vector_type == VECTOR_TYPE_ECLIPSE)
				{
					VECTOR_ECLIPSE *eclipse = (VECTOR_ECLIPSE*)line;
					eclipse->x *= zoom_x;
					eclipse->y *= zoom_y;
					eclipse->radius *= (zoom_x + zoom_y) * 0.5;
					eclipse->line_width *= (zoom_x + zoom_y) * 0.5;
				}

				// ���̃��C����
				line = line->base_data.next;
			}

			// ��ԉ��̃��C���[�̃��C�����C���[���쐬������
			line_layer->width = new_width, line_layer->height = new_height;
			line_layer->stride = new_width * target->channel;
			line_layer->pixels = (uint8*)MEM_REALLOC_FUNC(
				line_layer->pixels, new_width*new_height*target->channel);
			(void)memset(line_layer->pixels, 0,
				new_width*new_height*target->channel);
			cairo_surface_destroy(line_layer->surface_p);
			cairo_destroy(line_layer->cairo_p);
			line_layer->surface_p = cairo_image_surface_create_for_data(
				line_layer->pixels, format, new_width, new_height, new_width*target->channel);
			line_layer->cairo_p = cairo_create(line_layer->surface_p);

			// ���C���������������C���[����蒼��
			line_layer = target->layer_data.vector_layer_p->mix;
			line_layer->width = new_width, line_layer->height = new_height;
			line_layer->stride = new_width * target->channel;
			line_layer->pixels = (uint8*)MEM_REALLOC_FUNC(
				line_layer->pixels, new_width*new_height*target->channel);
			(void)memset(line_layer->pixels, 0,
				new_width*new_height*target->channel);
			cairo_surface_destroy(line_layer->surface_p);
			cairo_destroy(line_layer->cairo_p);
			line_layer->surface_p = cairo_image_surface_create_for_data(
				line_layer->pixels, format, new_width, new_height, new_width*target->channel);
			line_layer->cairo_p = cairo_create(line_layer->surface_p);

			target->width = new_width;
			target->height = new_height;
			target->stride = new_width * target->channel;

			// ���C����S�ă��X�^���C�Y
			ChangeActiveLayer(target->window, target);
			target->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ALL | VECTOR_LAYER_FIX_LINE;
			RasterizeVectorLayer(target->window, target, target->layer_data.vector_layer_p);
		}
		break;
	case TYPE_TEXT_LAYER:	// �e�L�X�g���C���[
		{
			// �����̊g��k�����ɓK�p����l
			gdouble zoom = (zoom_x > zoom_y) ? zoom_x : zoom_y;

			// ��Ƀ��C���[�̃s�N�Z���f�[�^��CAIRO�����X�V
			(void)MEM_FREE_FUNC(target->pixels);
			target->pixels = pixels;
			cairo_surface_destroy(target->surface_p);
			target->surface_p = surface_p;
			cairo_destroy(target->cairo_p);
			target->cairo_p = cairo_p;

			// ������\��������W�E�͈͂�ύX
			target->layer_data.text_layer_p->x *= zoom_x;
			target->layer_data.text_layer_p->y *= zoom_y;
			target->layer_data.text_layer_p->width *= zoom_x;
			target->layer_data.text_layer_p->height *= zoom_y;

			// �t�H���g�T�C�Y��ύX
			target->layer_data.text_layer_p->font_size *= zoom;

			// �������������_�����O
			RenderTextLayer(target->window, target, target->layer_data.text_layer_p);
		}
		break;
	case TYPE_LAYER_SET:
		// �s�N�Z���f�[�^���g��or�k�����Ďʂ�
		cairo_scale(cairo_p, zoom_x, zoom_y);
		cairo_set_source_surface(cairo_p, target->surface_p, 0, 0);
		cairo_paint(cairo_p);
		// �g��k���������ɖ߂�
		cairo_scale(cairo_p, 1/zoom_x, 1/zoom_y);

		// �ύX���I������̂Ńs�N�Z���f�[�^��CAIRO�̏����X�V
		(void)MEM_FREE_FUNC(target->pixels);
		target->pixels = pixels;
		cairo_surface_destroy(target->surface_p);
		target->surface_p = surface_p;
		cairo_destroy(target->cairo_p);
		target->cairo_p = cairo_p;

		break;
	}	// ���C���[�̎�ނŏ����؂�ւ�
		// switch(target->layer_type)

	// ���C���[�̕��A�����A1�s���̃o�C�g����ݒ�
	target->width = new_width;
	target->height = new_height;
	target->stride = new_width * target->channel;
}

/*******************************************
* ChangeLayerSize�֐�                      *
* ���C���[�̃T�C�Y��ύX����(�g��k����)   *
* ����                                     *
* target		: �T�C�Y��ύX���郌�C���[ *
* new_width		: �V�������C���[�̕�       *
* new_height	: �V�������C���[�̍���     *
*******************************************/
void ChangeLayerSize(
	LAYER* target,
	int32 new_width,
	int32 new_height
)
{
	// CAIRO���쐬�������̂Ń��[�J���ň�x�A�h���X���󂯂�
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	uint8 *pixels;
	// CAIRO�ɐݒ肷��t�H�[�}�b�g���
	cairo_format_t format = (target->channel == 4) ?
		CAIRO_FORMAT_ARGB32 : (target->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;

	// �s�N�Z���f�[�^�̃������̈���m��
	pixels = (uint8*)MEM_ALLOC_FUNC(
		sizeof(*pixels)*new_width*new_height*target->channel);
	// 0������
	(void)memset(pixels, 0, sizeof(*pixels)*new_width*new_height*target->channel);

	// CAIRO���쐬
	surface_p = cairo_image_surface_create_for_data(pixels,
		format, new_width, new_height, new_width * target->channel);
	cairo_p = cairo_create(surface_p);

	// ���C���[�̎�ނŏ����؂�ւ�
	switch(target->layer_type)
	{
	case TYPE_NORMAL_LAYER:	// �ʏ탌�C���[
		// �s�N�Z���f�[�^���ʂ�
		cairo_set_source_surface(cairo_p, target->surface_p, 0, 0);
		cairo_paint(cairo_p);

		// �ύX���I������̂Ńs�N�Z���f�[�^��CAIRO�̏����X�V
		(void)MEM_FREE_FUNC(target->pixels);
		target->pixels = pixels;
		cairo_surface_destroy(target->surface_p);
		target->surface_p = surface_p;
		cairo_destroy(target->cairo_p);
		target->cairo_p = cairo_p;

		break;
	case TYPE_VECTOR_LAYER:	// �x�N�g�����C���[
		{
			// ���W���C������x�N�g���f�[�^
			VECTOR_LINE *line = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next;
			// ��ԉ��̃��C���̓������Ċm��
			VECTOR_LINE_LAYER *line_layer = ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.layer;

			// ��Ƀ��C���[�̃s�N�Z���f�[�^��CAIRO�����X�V
			(void)MEM_FREE_FUNC(target->pixels);
			target->pixels = pixels;
			cairo_surface_destroy(target->surface_p);
			target->surface_p = surface_p;
			cairo_destroy(target->cairo_p);
			target->cairo_p = cairo_p;

			// ��ԉ��̃��C���[�̃��C�����C���[���쐬������
			line_layer->width = new_width, line_layer->height = new_height;
			line_layer->stride = new_width * target->channel;
			line_layer->pixels = (uint8*)MEM_REALLOC_FUNC(
				line_layer->pixels, new_width*new_height*target->channel);
			(void)memset(line_layer->pixels, 0,
				new_width*new_height*target->channel);
			cairo_surface_destroy(line_layer->surface_p);
			cairo_destroy(line_layer->cairo_p);
			line_layer->surface_p = cairo_image_surface_create_for_data(
				line_layer->pixels, format, new_width, new_height, new_width*target->channel);
			line_layer->cairo_p = cairo_create(line_layer->surface_p);

			// ���C���������������C���[����蒼��
			line_layer = target->layer_data.vector_layer_p->mix;
			line_layer->width = new_width, line_layer->height = new_height;
			line_layer->stride = new_width * target->channel;
			line_layer->pixels = (uint8*)MEM_REALLOC_FUNC(
				line_layer->pixels, new_width*new_height*target->channel);
			(void)memset(line_layer->pixels, 0,
				new_width*new_height*target->channel);
			cairo_surface_destroy(line_layer->surface_p);
			cairo_destroy(line_layer->cairo_p);
			line_layer->surface_p = cairo_image_surface_create_for_data(
				line_layer->pixels, format, new_width, new_height, new_width*target->channel);
			line_layer->cairo_p = cairo_create(line_layer->surface_p);

			// ���C����S�ă��X�^���C�Y
			ChangeActiveLayer(target->window, target);
			target->layer_data.vector_layer_p->flags = VECTOR_LAYER_RASTERIZE_ALL;
			RasterizeVectorLayer(target->window, target, target->layer_data.vector_layer_p);
		}
		break;
	case TYPE_TEXT_LAYER:	// �e�L�X�g���C���[
		{
			// ��Ƀ��C���[�̃s�N�Z���f�[�^��CAIRO�����X�V
			(void)MEM_FREE_FUNC(target->pixels);
			target->pixels = pixels;
			cairo_surface_destroy(target->surface_p);
			target->surface_p = surface_p;
			cairo_destroy(target->cairo_p);
			target->cairo_p = cairo_p;

			// �������������_�����O
			RenderTextLayer(target->window, target, target->layer_data.text_layer_p);
		}
		break;
	case TYPE_LAYER_SET:
		{
		}
		break;
	}	// ���C���[�̎�ނŏ����؂�ւ�
		// switch(target->layer_type)

	// ���C���[�̕��A�����A1�s���̃o�C�g����ݒ�
	target->width = new_width;
	target->height = new_height;
	target->stride = new_width * target->channel;
}

/***********************************
* ADD_NEW_LAYER_HISTORY_DATA�\���� *
* �V�������C���[�ǉ����̗����f�[�^ *
***********************************/
typedef struct _ADD_NEW_LAYER_HISTORY_DATA
{
	uint16 layer_type;
	uint16 name_length;
	int32 x, y;
	int32 width, height;
	uint8 channel;
	uint16 prev_name_length;
	char* layer_name, *prev_name;
} ADD_NEW_LAYER_HISTORY_DATA;

static void AddNewLayerUndo(DRAW_WINDOW* window, void* p)
{
	ADD_NEW_LAYER_HISTORY_DATA data;
	uint8* buff = (uint8*)p;
	LAYER* delete_layer;

	(void)memcpy(&data, buff, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	buff += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	data.layer_name = (char*)buff;

	delete_layer = SearchLayer(window->layer, data.layer_name);
	if(delete_layer == window->active_layer)
	{
		LAYER *new_active = (delete_layer->prev == NULL)
			? delete_layer->next : delete_layer->prev;	
		ChangeActiveLayer(window, new_active);
			LayerViewSetActiveLayer(new_active, window->app->layer_window.view);
	}

	DeleteLayer(&delete_layer);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

static void AddNewLayerRedo(DRAW_WINDOW* window, void* p)
{
	ADD_NEW_LAYER_HISTORY_DATA data;
	uint8* buff = (uint8*)p;
	LAYER *prev, *next, *new_layer;

	(void)memcpy(&data, buff, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	buff += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	data.layer_name = (char*)buff;
	buff += data.name_length;
	data.prev_name = (char*)buff;

	if(*data.prev_name == '\0')
	{
		prev = NULL;
		next = window->layer;
	}
	else
	{
		prev = SearchLayer(window->layer, data.prev_name);
		next = prev->next;
	}

	new_layer = CreateLayer(
		data.x, data.y, data.width, data.height, data.channel,
		data.layer_type, prev, next, data.layer_name, window
	);

	window->num_layer++;
	LayerViewAddLayer(new_layer, window->layer,
		window->app->layer_window.view, window->num_layer);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

void AddNewLayerHistory(
	const LAYER* new_layer,
	uint16 layer_type
)
{
	ADD_NEW_LAYER_HISTORY_DATA data;
	gchar *history_name;
	uint8 *buff, *write;
	size_t data_size;

	data.layer_type = layer_type;
	data.x = new_layer->x, data.y = new_layer->y;
	data.width = new_layer->width, data.height = new_layer->height;
	data.channel = new_layer->channel;
	data.name_length = (uint16)strlen(new_layer->name) + 1;
	if(new_layer->prev == NULL)
	{
		data.prev_name_length = 1;
	}
	else
	{
		data.prev_name_length = (uint16)strlen(new_layer->prev->name) + 1;
	}
	data_size = offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name)
		+ data.name_length + data.prev_name_length;

	buff = write = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memcpy(write, &data, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	write += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	(void)memcpy(write, new_layer->name, data.name_length);
	write += data.name_length;
	if(new_layer->prev == NULL)
	{
		*write = 0;
	}
	else
	{
		(void)memcpy(write, new_layer->prev->name, data.prev_name_length);
	}

	switch(layer_type)
	{
	case TYPE_NORMAL_LAYER:
		history_name = new_layer->window->app->labels->layer_window.add_normal;
		break;
	case TYPE_VECTOR_LAYER:
		history_name = new_layer->window->app->labels->layer_window.add_vector;
		break;
	case TYPE_3D_LAYER:
		history_name = new_layer->window->app->labels->layer_window.add_3d_modeling;
		break;
	case TYPE_ADJUSTMENT_LAYER:
		history_name = new_layer->window->app->labels->layer_window.add_adjustment_layer;
		break;
	default:
		history_name = new_layer->window->app->labels->layer_window.add_layer_set;
	}

	AddHistory(
		&new_layer->window->app->draw_window[new_layer->window->app->active_window]->history,
		history_name, buff, (uint32)data_size, AddNewLayerUndo, AddNewLayerRedo
	);

	MEM_FREE_FUNC(buff);
}

/**********************************************
* ADD_NEW_LAYER_WITH_IMAGE_HISTORY_DATA�\���� *
* �摜�f�[�^�t���ł̃��C���[�ǉ������f�[�^    *
**********************************************/
typedef struct _ADD_NEW_LAYER_WITH_IMAGE_HISTORY_DATA
{
	ADD_NEW_LAYER_HISTORY_DATA layer_history;
	size_t image_data_size;
	uint8* image_data;
} ADD_NEW_LAYER_WITH_IMAGE_HISTORY_DATA;

/*******************************************
* AddNewLayerWithImageRedo�֐�             *
* �摜�f�[�^�t���ł̃��C���[�ǉ��̂�蒼�� *
* ����                                     *
* window	: �`��̈�̏��               *
* p			: �����f�[�^�̃A�h���X         *
*******************************************/
static void AddNewLayerWithImageRedo(DRAW_WINDOW* window, void* p)
{
	// �����f�[�^
	ADD_NEW_LAYER_HISTORY_DATA data;
	// �����f�[�^���o�C�g�P�ʂň������߂ɃL���X�g
	uint8* buff = (uint8*)p;
	// �O��̃��C���[�ƒǉ����郌�C���[�ւ̃|�C���^
	LAYER *prev, *next, *new_layer;
	// �摜�f�[�^�W�J�p�̃X�g���[��
	MEMORY_STREAM stream;
	// �摜�f�[�^�̃s�N�Z���f�[�^
	uint8* pixels;
	// �摜�f�[�^�̏��
	int32 width, height, stride;
	// for���p�̃J�E���^
	int i;

	// ���C���[�̏���ǂݍ���
	(void)memcpy(&data, buff, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	buff += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	data.layer_name = (char*)buff;
	buff += data.name_length;
	data.prev_name = (char*)buff;

	// �O�̃��C���[��T��
	if(*data.prev_name == '\0')
	{
		prev = NULL;
		next = window->layer;
	}
	else
	{
		prev = SearchLayer(window->layer, data.prev_name);
		next = prev->next;
	}

	// ���C���[�ǉ�
	new_layer = CreateLayer(
		data.x, data.y, data.width, data.height, data.channel,
		data.layer_type, prev, next, data.layer_name, window
	);

	// �摜�f�[�^��W�J
	buff += data.prev_name_length;
	(void)memcpy(&stream.data_size, buff, sizeof(stream.data_size));
	buff += sizeof(stream.data_point);
	stream.buff_ptr = buff;
	stream.data_point = 0;
	stream.block_size = 1;
	pixels = ReadPNGStream(
		(void*)&stream, (stream_func_t)MemRead, &width, &height, &stride
	);

	// �ǉ��������C���[�ɉ摜�f�[�^���R�s�[
	for(i=0; i<height; i++)
	{
		(void)memcpy(&new_layer->pixels[i*new_layer->stride],
			&pixels[i*stride], stride);
	}

	window->num_layer++;
	LayerViewAddLayer(new_layer, window->layer,
		window->app->layer_window.view, window->num_layer);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	MEM_FREE_FUNC(pixels);
}

/*********************************************
* AddNewLayerWithImageHistory�֐�            *
* �摜�f�[�^�t���ł̃��C���[�ǉ��̗���ǉ�   *
* ����                                       *
* new_layer		: �ǉ��������C���[           *
* pixels		: �摜�f�[�^�̃s�N�Z���f�[�^ *
* width			: �摜�f�[�^�̕�             *
* height		: �摜�f�[�^�̍���           *
* stride		: �摜�f�[�^��s���̃o�C�g�� *
* channel		: �摜�f�[�^�̃`�����l����   *
* history_name	: �����̖��O                 *
*********************************************/
void AddNewLayerWithImageHistory(
	const LAYER* new_layer,
	uint8* pixels,
	int32 width,
	int32 height,
	int32 stride,
	uint8 channel,
	const char* history_name
)
{
// �f�[�^�̈��k���x��
#define DATA_COMPRESSION_LEVEL 8
	// �����f�[�^
	ADD_NEW_LAYER_WITH_IMAGE_HISTORY_DATA data;
	// �f�[�^���k�p�X�g���[��
	MEMORY_STREAM_PTR image;
	// �����f�[�^�̃o�b�t�@�Ə������ݐ�̃A�h���X
	uint8 *buff, *write;
	// �����f�[�^�̃o�C�g��
	size_t data_size;

	// ���C���[�̃f�[�^���Z�b�g
	data.layer_history.layer_type = TYPE_NORMAL_LAYER;
	data.layer_history.x = new_layer->x, data.layer_history.y = new_layer->y;
	data.layer_history.width = new_layer->width, data.layer_history.height = new_layer->height;
	data.layer_history.channel = new_layer->channel;
	data.layer_history.name_length = (uint16)strlen(new_layer->name) + 1;

	// �s�N�Z���f�[�^�����k
	image = CreateMemoryStream(height*stride*2);
	WritePNGStream(image, (stream_func_t)MemWrite, NULL,
		pixels, width, height, stride, channel, 0, DATA_COMPRESSION_LEVEL);

	// �O�̃��C���[�̖��O�̒������Z�b�g
	if(new_layer->prev == NULL)
	{
		data.layer_history.prev_name_length = 1;
	}
	else
	{
		data.layer_history.prev_name_length = (uint16)strlen(new_layer->prev->name) + 1;
	}

	// �����f�[�^�̃o�C�g�����v�Z
	data_size = offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name)
		+ data.layer_history.name_length + data.layer_history.prev_name_length
			+ sizeof(image->data_point)	+ image->data_point;

	// �����f�[�^�쐬
	buff = write = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memcpy(write, &data, offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name));
	write += offsetof(ADD_NEW_LAYER_HISTORY_DATA, layer_name);
	(void)memcpy(write, new_layer->name, data.layer_history.name_length);
	write += data.layer_history.name_length;
	if(new_layer->prev == NULL)
	{
		*write = 0;
	}
	else
	{
		(void)memcpy(write, new_layer->prev->name, data.layer_history.prev_name_length);
	}
	write += data.layer_history.prev_name_length;
	(void)memcpy(write, &image->data_point, sizeof(image->data_point));
	write += sizeof(image->data_point);
	(void)memcpy(write, image->buff_ptr, image->data_point);

	// �A���h�D�A���h�D�p�̊֐��|�C���^���Z�b�g���ė����f�[�^�ǉ�
	AddHistory(
		&new_layer->window->history,
		history_name,
		buff, (uint32)data_size, AddNewLayerUndo, AddNewLayerWithImageRedo
	);

	// �������J��
	DeleteMemoryStream(image);
	MEM_FREE_FUNC(buff);
}

int CorrectLayerName(const LAYER* bottom_layer, const char* name)
{
	const LAYER *layer = bottom_layer;

	do
	{
		if(strcmp(layer->name, name) == 0)
		{
			return FALSE;
		}
		layer = layer->next;
	} while(layer != NULL);

	return TRUE;
}

LAYER* SearchLayer(LAYER* bottom_layer, const gchar* name)
{
	LAYER* layer = bottom_layer;

	while(layer != NULL)
	{
		if(strcmp(layer->name, name) == 0)
		{
			return layer;
		}
		layer = layer->next;
	}

	return NULL;
}

int32 GetLayerID(const LAYER* bottom, const LAYER* prev, uint16 num_layer)
{
	const LAYER *layer = bottom;
	int32 id = 1;

	if(prev == NULL)
	{
		return num_layer - 1;
	}

	while(layer != prev)
	{
		layer = layer->next;
		id++;
	}

	return num_layer - id - 1;
}

typedef struct _LAYER_NAME_CHANGE_HISTORY
{
	uint16 before_length, after_length;
	gchar *before_name, *after_name;
} LAYER_NAME_CHANGE_HISTORY;

static void LayerNameChangeUndo(DRAW_WINDOW* window, void* data)
{
	LAYER_NAME_CHANGE_HISTORY history;
	uint8 *p = (uint8*)data;
	LAYER* layer;
	GList* label;

	(void)memcpy(&history.before_length, p, sizeof(history.before_length));
	p += sizeof(history.before_length);
	(void)memcpy(&history.after_length, p, sizeof(history.before_length));
	p += sizeof(history.before_length);
	history.before_name = (gchar*)p;
	p += history.before_length;
	history.after_name = (gchar*)p;

	layer = SearchLayer(window->layer, history.after_name);

	MEM_FREE_FUNC(layer->name);
	layer->name = MEM_STRDUP_FUNC(history.before_name);

	label = gtk_container_get_children(GTK_CONTAINER(layer->widget->name));

	gtk_label_set_text(GTK_LABEL(label->data), layer->name);

	g_list_free(label);
}

static void LayerNameChangeRedo(DRAW_WINDOW* window, void* data)
{
	LAYER_NAME_CHANGE_HISTORY history;
	uint8 *p = (uint8*)data;
	LAYER* layer;
	GList* label;

	(void)memcpy(&history.before_length, p, sizeof(history.before_length));
	p += sizeof(history.before_length);
	(void)memcpy(&history.after_length, p, sizeof(history.before_length));
	p += sizeof(history.before_length);
	history.before_name = (gchar*)p;
	p += history.before_length;
	history.after_name = (gchar*)p;

	layer = SearchLayer(window->layer, history.before_name);

	MEM_FREE_FUNC(layer->name);
	layer->name = MEM_STRDUP_FUNC(history.after_name);

	label = gtk_container_get_children(GTK_CONTAINER(layer->widget->name));

	gtk_label_set_text(GTK_LABEL(label->data), layer->name);
}

void AddLayerNameChangeHistory(
	struct _APPLICATION* app,
	const gchar* before_name,
	const gchar* after_name
)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	int32 data_size;
	uint16 before_len = (uint16)strlen(before_name)+1,
		after_len = (uint16)strlen(after_name)+1;
	uint8* data, *p;
	
	data_size = sizeof(before_len)*2 + before_len + after_len;

	p = data = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memcpy(p, &before_len, sizeof(before_len));
	p += sizeof(before_len);
	(void)memcpy(p, &after_len, sizeof(after_len));
	p += sizeof(after_len);
	(void)memcpy(p, before_name, before_len);
	p += before_len;
	(void)memcpy(p, after_name, after_len);

	AddHistory(
		&window->history,
		app->labels->layer_window.rename,
		data,
		data_size,
		LayerNameChangeUndo,
		LayerNameChangeRedo
	);

	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		AddHistory(
			&app->draw_window[app->active_window]->history,
			app->labels->layer_window.rename,
			data,
			data_size,
			LayerNameChangeUndo,
			LayerNameChangeRedo
		);
	}

	MEM_FREE_FUNC(data);
}

void ChangeActiveLayer(DRAW_WINDOW* window, LAYER* layer)
{
	if((window->app->current_tool != layer->layer_type || layer->layer_type == TYPE_TEXT_LAYER)
		&& window->transform == NULL)
	{
		// ���j���[��L���ɂ���t���O
		gboolean enable_menu = TRUE;
		// for���p�̃J�E���^
		int i;

		if(layer->layer_set != NULL)
		{
			window->active_layer_set = layer->layer_set;
		}

		// ���݂̃c�[����ύX
		window->app->current_tool = layer->layer_type;

		gtk_widget_destroy(layer->window->app->tool_window.brush_table);
		switch(layer->layer_type)
		{
		case TYPE_NORMAL_LAYER:
			CreateBrushTable(layer->window->app, &layer->window->app->tool_window, window->app->tool_window.brushes);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layer->window->app->tool_window.active_brush[window->app->input]->button), TRUE);

			if((window->app->tool_window.flags & TOOL_USING_BRUSH) != 0)
			{
				if(layer->window->app->tool_window.detail_ui != NULL)
				{
					gtk_widget_destroy(layer->window->app->tool_window.detail_ui);
				}
				layer->window->app->tool_window.detail_ui = layer->window->app->tool_window.active_brush[window->app->input]->create_detail_ui(
					layer->window->app, layer->window->app->tool_window.active_brush[window->app->input]);
				gtk_scrolled_window_add_with_viewport(
					GTK_SCROLLED_WINDOW(layer->window->app->tool_window.detail_ui_scroll), layer->window->app->tool_window.detail_ui);
			}

			enable_menu = FALSE;
			break;
		case TYPE_VECTOR_LAYER:
			CreateVectorBrushTable(layer->window->app, &layer->window->app->tool_window, window->app->tool_window.vector_brushes);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layer->window->app->tool_window.active_vector_brush[window->app->input]->button), TRUE);

			if((window->app->tool_window.flags & TOOL_USING_BRUSH) != 0)
			{
				if(layer->window->app->tool_window.detail_ui != NULL)
				{
					gtk_widget_destroy(layer->window->app->tool_window.detail_ui);
				}
				layer->window->app->tool_window.detail_ui = layer->window->app->tool_window.active_vector_brush[window->app->input]->create_detail_ui(
					layer->window->app, layer->window->app->tool_window.active_vector_brush[window->app->input]->brush_data);
				gtk_scrolled_window_add_with_viewport(
					GTK_SCROLLED_WINDOW(layer->window->app->tool_window.detail_ui_scroll), layer->window->app->tool_window.detail_ui);
			}
			break;
		case TYPE_TEXT_LAYER:
			{
				layer->layer_data.text_layer_p->text_field = layer->window->app->tool_window.brush_table
					= gtk_text_view_new();
				(void)g_signal_connect(G_OBJECT(layer->layer_data.text_layer_p->text_field), "focus-in-event",
					G_CALLBACK(TextFieldFocusIn), layer->window->app);
				(void)g_signal_connect(G_OBJECT(layer->layer_data.text_layer_p->text_field), "focus-out-event",
					G_CALLBACK(TextFieldFocusOut), layer->window->app);
				(void)g_signal_connect(G_OBJECT(layer->layer_data.text_layer_p->text_field), "destroy",
					G_CALLBACK(OnDestroyTextField), layer->window->app);
				layer->layer_data.text_layer_p->buffer =
					gtk_text_view_get_buffer(GTK_TEXT_VIEW(layer->layer_data.text_layer_p->text_field));
				if(layer->layer_data.text_layer_p->text != NULL)
				{
					gtk_text_buffer_set_text(layer->layer_data.text_layer_p->buffer, layer->layer_data.text_layer_p->text, -1);
				}
				(void)g_signal_connect(G_OBJECT(layer->layer_data.text_layer_p->buffer), "changed",
					G_CALLBACK(OnChangeTextCallBack), layer);
				if(layer->window->app->tool_window.detail_ui != NULL)
				{
					gtk_widget_destroy(window->app->tool_window.detail_ui);
				}
				layer->window->app->tool_window.detail_ui = CreateTextLayerDetailUI(
					window->app, layer, layer->layer_data.text_layer_p);
				gtk_scrolled_window_add_with_viewport(
					GTK_SCROLLED_WINDOW(layer->window->app->tool_window.detail_ui_scroll),
					layer->window->app->tool_window.detail_ui
				);
			}
			break;
		case TYPE_ADJUSTMENT_LAYER:
			if(layer->window->app->tool_window.detail_ui != NULL)
			{
				gtk_widget_destroy(layer->window->app->tool_window.detail_ui);
			}
			layer->window->app->tool_window.detail_ui = NULL;
			CreateAdjustmentLayerWidget(layer);
			break;
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
		case TYPE_3D_LAYER:
			CreateChange3DLayerUI(layer->window->app, &layer->window->app->tool_window);
			break;
#endif
		}

		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(layer->window->app->tool_window.brush_scroll),
			layer->window->app->tool_window.brush_table);
		gtk_widget_show_all(layer->window->app->tool_window.brush_table);
		if(layer->window->app->tool_window.detail_ui != NULL)
		{
			gtk_widget_show_all(layer->window->app->tool_window.detail_ui);
		}

		// �ʏ�̃��C���[�Ȃ烉�X�^���C�Y���̃��j���[�͖�����
		for(i=0; i<window->app->menus.num_disable_if_normal_layer; i++)
		{
			gtk_widget_set_sensitive(window->app->menus.disable_if_normal_layer[i], enable_menu);
		}

		// �ʏ탌�C���[�Ȃ�Εs�����x�ی�̋@�\ON
		gtk_widget_set_sensitive(window->app->layer_window.layer_control.lock_opacity, !enable_menu);
	}

	// ��ԉ��̃��C���[�Ȃ�u���̃��C���[�ƌ������j���[�v�A�u���ֈړ��v���@�\OFF
	gtk_widget_set_sensitive(window->app->menus.merge_down_menu,(window->layer != layer));
	gtk_widget_set_sensitive(window->app->layer_window.layer_control.down, (window->layer != layer));

	// ��ԏ�̃��C���[�Ȃ�Ȃ�u��ֈړ��v���@�\OFF
	gtk_widget_set_sensitive(window->app->layer_window.layer_control.up, layer->next != NULL);

	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);

	window->active_layer = layer;
	window->active_layer_set = layer->layer_set;
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
}

typedef struct _LAYER_MERGE_DOWN_HISTORY
{
	uint16 layer_type;				// ���C���[�^�C�v
	uint16 layer_mode;				// ���C���[�̍������[�h
	int32 x, y;						// ���C���[����̍��W
	// ���C���[�̕��A�����A��s���̃o�C�g��
	int32 width, height, stride;
	uint32 flags;					// ���C���[�̕\���E��\�����̃t���O
	int8 channel;					// ���C���[�̃`�����l����
	int8 alpha;						// ���C���[�̕s�����x
	uint16 name_length;				// ���C���[�̖��O�̒���
	uint16 prev_name_length;		// �O�̃��C���[�̕�����
	size_t image_data_size;			// �摜�f�[�^�̃o�C�g��
	size_t prev_image_data_size;	// ������̃��C���[�̉摜�f�[�^�o�C�g��
	gchar* name;					// ���C���[�̖��O
	gchar* prev_name;				// �O�̃��C���[�̖��O
	uint8* image_data;				// �摜�f�[�^
	uint8* prev_image_data;			// ������̉摜�f�[�^
} LAYER_MERGE_DOWN_HISTORY;

/*****************************
* LayerMeargeDownUndo�֐�    *
* ���C���[��������̃A���h�D *
* ����                       *
* window	: �`��̈�̏�� *
* p			: �����f�[�^     *
*****************************/
static void LayerMergeDownUndo(DRAW_WINDOW* window, void* p)
{
	// �����f�[�^
	LAYER_MERGE_DOWN_HISTORY history;
	// �����f�[�^�ǂݍ��ݗp�X�g���[��
	MEMORY_STREAM stream;
	// ���C���[�̃s�N�Z���f�[�^
	uint8* pixels;
	// ���C���[�̕��A�����A��s���̃o�C�g��
	gint32 width, height, stride;
	// �����f�[�^�o�C�g��
	size_t data_size;
	// ���������郌�C���[
	LAYER* layer;
	// �O�̃��C���[
	LAYER* prev;
	// ���݂̃f�[�^�Q�ƈʒu
	size_t data_point;

	// �����f�[�^��0������
	(void)memset(&history, 0, sizeof(history));

	// �����f�[�^�̃o�C�g����ǂݍ���
	(void)memcpy(&data_size, p, sizeof(data_size));

	// �X�g���[����������
	stream.block_size = 1;
	stream.data_size = data_size;
	stream.buff_ptr = (uint8*)p;
	stream.data_point = sizeof(data_size);

	// ���C���[����ǂݍ���
	(void)MemRead(&history, 1,
		offsetof(LAYER_MERGE_DOWN_HISTORY, name), &stream);
	// ���C���[�̖��O�A�O�̃��C���[�̖��O���Z�b�g
	history.name = (char*)&stream.buff_ptr[stream.data_point];
	(void)MemSeek(&stream, history.name_length, SEEK_CUR);
	history.prev_name = (char*)&stream.buff_ptr[stream.data_point];
	(void)MemSeek(&stream, history.prev_name_length, SEEK_CUR);

	// �O�̃��C���[�̃|�C���^���Z�b�g����
	prev = SearchLayer(window->layer, history.prev_name);

	// ���������郌�C���[�쐬
	layer = CreateLayer(
		history.x, history.y, history.width, history.height, history.channel,
		history.layer_type, prev, prev->next,
		history.name, window
	);
	layer->alpha = history.alpha;
	layer->layer_mode = history.layer_mode;

	data_point = stream.data_point;
	// ���C���[�̃s�N�Z���f�[�^��ǂݍ���
	pixels = ReadPNGStream(
		&stream,
		(size_t (*)(void*, size_t, size_t, void*))MemRead,
		&width, &height, &stride
	);

	// ���C���[�̃s�N�Z���������ɃR�s�[
	(void)memcpy(layer->pixels, pixels, stride*height);

	MEM_FREE_FUNC(pixels);

	// PNG�ǂݍ��݂ōŌ�܂ŃV�[�N����Ă���̂�߂�
	stream.data_point = data_point + history.image_data_size;
	// �����O�̃��C���[�f�[�^��ǂݍ����
	pixels = ReadPNGStream(&stream, (stream_func_t)MemRead,
		&width, &height, &stride);

	// �s�N�Z���f�[�^���R�s�[
	(void)memcpy(prev->pixels, pixels, height*stride);

	MEM_FREE_FUNC(pixels);

	// �`��̈���X�V
	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	// ���C���[�̐����X�V
	window->num_layer++;

	// ���C���[�r���[�Ƀ��C���[��ǉ�
	LayerViewAddLayer(layer, window->layer,
		window->app->layer_window.view, window->num_layer);
}

/*****************************
* LayerMeargeDownRedo�֐�    *
* ���C���[��������̃��h�D   *
* ����                       *
* window	: �`��̈�̏�� *
* p			: �����f�[�^     *
*****************************/
static void LayerMergeDownRedo(DRAW_WINDOW* window, void* p)
{
	// �����f�[�^
	LAYER_MERGE_DOWN_HISTORY history;
	// �����f�[�^�ǂݍ��ݗp�X�g���[��
	MEMORY_STREAM stream;
	// ��������^�[�Q�b�g
	LAYER* target;
	// �����f�[�^�̃o�C�g��
	size_t data_size;

	// �����f�[�^�̃o�C�g����ǂݍ���
	(void)memcpy(&data_size, p, sizeof(data_size));

	// �X�g���[����������
	stream.block_size = 1;
	stream.data_size = data_size;
	stream.buff_ptr = (uint8*)p;
	stream.data_point = sizeof(data_size);

	// ���C���[����ǂݍ���
	(void)MemRead(&history, 1,
		offsetof(LAYER_MERGE_DOWN_HISTORY, name), &stream);
	// ���C���[�̖��O�A�O�̃��C���[�̖��O���Z�b�g
	history.name = (char*)&stream.buff_ptr[stream.data_point];

	// ��������^�[�Q�b�g���T�[�`
	target = SearchLayer(window->layer, history.name);

	// ���������s
	LayerMergeDown(target);
}

/*******************************************
* AddLayerMergeDownHistory�֐�             *
* ���̃��C���[�ƌ����̗����f�[�^��ǉ����� *
* ����                                     *
* window	: �`��̈�̏��               *
* target	: �������郌�C���[             *
*******************************************/
void AddLayerMergeDownHistory(
	DRAW_WINDOW* window,
	LAYER* target
)
{
// �f�[�^�̈��k���x��
#ifndef DATA_COMPRESSION_LEVEL
#define DATA_COMPRESSION_LEVEL Z_DEFAULT_COMPRESSION
#endif
	// �����f�[�^
	LAYER_MERGE_DOWN_HISTORY history;
	// �����f�[�^���쐬���邽�߂̃X�g���[��
	MEMORY_STREAM_PTR history_data =
		CreateMemoryStream(target->stride*target->height*4);
	// �����f�[�^�̃o�C�g��
	size_t data_size;

	(void)MemSeek(history_data, sizeof(data_size), SEEK_SET);

	// ���C���[�̃f�[�^�𗚗��ɃZ�b�g
	history.layer_type = target->layer_type;
	history.layer_mode = target->layer_mode;
	history.x = target->x;
	history.y = target->y;
	history.width = target->width;
	history.height = target->height;
	history.stride = target->stride;
	history.flags = target->flags;
	history.channel = target->channel;
	history.alpha = target->alpha;
	history.name_length = (uint16)strlen(target->name) + 1;
	history.prev_name_length = (uint16)strlen(target->prev->name) + 1;

	// ��Ƀ��C���[�̖��O�ƃs�N�Z���f�[�^��o�^����
	(void)MemSeek(
		history_data,
		offsetof(LAYER_MERGE_DOWN_HISTORY, name),
		SEEK_CUR
	);
	(void)MemWrite(target->name, 1, history.name_length, history_data);
	(void)MemWrite(target->prev->name, 1, history.prev_name_length, history_data);

	data_size = history_data->data_point;
	WritePNGStream(history_data, (stream_func_t)MemWrite, NULL, target->pixels,
		target->width, target->height, target->stride, target->channel, 0, DATA_COMPRESSION_LEVEL);
	history.image_data_size = history_data->data_point - data_size;

	data_size = history_data->data_point;
	WritePNGStream(history_data, (stream_func_t)MemWrite, NULL, target->prev->pixels,
		target->prev->width, target->prev->height, target->prev->stride, target->prev->channel,
		0, DATA_COMPRESSION_LEVEL);
	history.prev_image_data_size = history_data->data_point - data_size;
	
	data_size = offsetof(LAYER_MERGE_DOWN_HISTORY, name) + history.name_length + history.prev_name_length
		+ history.image_data_size + history.prev_image_data_size;

	(void)MemSeek(history_data, 0, SEEK_SET);
	(void)MemWrite(&data_size, sizeof(data_size), 1, history_data);
	(void)MemWrite(&history, 1, offsetof(LAYER_MERGE_DOWN_HISTORY, name), history_data);

	AddHistory(&window->history, window->app->labels->menu.merge_down_layer, history_data->buff_ptr,
		(uint32)data_size, LayerMergeDownUndo, LayerMergeDownRedo);

	DeleteMemoryStream(history_data);
}

/*******************************
* LayerMergeDown�֐�           *
* ���̃��C���[�ƌ�������       *
* ����                         *
* target	: �������郌�C���[ *
*******************************/
void LayerMergeDown(LAYER* target)
{
	if(target->layer_type == TYPE_NORMAL_LAYER)
	{
		switch(target->layer_mode)
		{
		case LAYER_BLEND_OVERLAY:
			{
				DRAW_WINDOW *window = target->window;
				// �������܂܂ɍ������邽�߂Ɉ�x����������
				LAYER *temp_layer = CreateLayer(0, 0, target->width, target->height, target->channel,
					TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
				LAYER *new_prev = CreateLayer(0, 0, target->width, target->height, target->channel,
					TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
				LAYER *blend = window->layer;
				FLOAT_T rc, ra, dc, da, sa;
				uint8 alpha;
				int i;

				// �������郌�C���[�܂ō���
				(void)memcpy(new_prev->pixels, window->back_ground, window->pixel_buf_size);
				(void)memcpy(temp_layer->pixels, window->back_ground, window->pixel_buf_size);
				while(blend != target)
				{
					window->layer_blend_functions[blend->layer_mode](blend, temp_layer);

					blend = blend->next;

					if(blend == target->prev)
					{
						(void)memcpy(new_prev->pixels, temp_layer->pixels, window->pixel_buf_size);
						for(i=0; i<target->width * target->height; i++)
						{
							new_prev->pixels[i*4+3] = blend->pixels[i*4+3];
						}
					}
				}
				window->layer_blend_functions[blend->layer_mode](blend, temp_layer);

				(void)memcpy(window->temp_layer->pixels, target->prev->pixels, window->pixel_buf_size);
				window->layer_blend_functions[LAYER_BLEND_NORMAL](target, window->temp_layer);

				for(i=0; i<target->width * target->height; i++)
				{
					if(window->temp_layer->pixels[i*4+3] > 0)
					{
						alpha = window->temp_layer->pixels[i*4+3];
						ra = 1;
						da = new_prev->pixels[i*4+3] * DIV_PIXEL;
						sa = alpha * DIV_PIXEL;
						dc = new_prev->pixels[i*4] * DIV_PIXEL;
						rc = temp_layer->pixels[i*4] * DIV_PIXEL;
						target->prev->pixels[i*4] = (uint8)MINIMUM(alpha, ((ra*rc - da*dc*(1-sa)) / sa) * alpha);
						dc = new_prev->pixels[i*4+1] * DIV_PIXEL;
						rc = temp_layer->pixels[i*4+1] * DIV_PIXEL;
						target->prev->pixels[i*4+1] = (uint8)MINIMUM(alpha, ((ra*rc - da*dc*(1-sa)) / sa) * alpha);
						dc = new_prev->pixels[i*4+2] * DIV_PIXEL;
						rc = temp_layer->pixels[i*4+2] * DIV_PIXEL;
						target->prev->pixels[i*4+2] = (uint8)MINIMUM(alpha, ((ra*rc - da*dc*(1-sa)) / sa) * alpha);
						target->prev->pixels[i*4+3] = window->temp_layer->pixels[i*4+3];
					}
				}

				DeleteLayer(&temp_layer);
			}

			break;
		default:
			target->window->layer_blend_functions[target->layer_mode](target, target->prev);
		}

		DeleteLayer(&target);
	}
	else if(target->layer_type == TYPE_VECTOR_LAYER)
	{
		LAYER *prev = target->prev;

		if(target->prev->layer_type == TYPE_VECTOR_LAYER)
		{
			prev->layer_data.vector_layer_p->num_lines += target->layer_data.vector_layer_p->num_lines;
			if(((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next != NULL)
			{
				prev->layer_data.vector_layer_p->top_data->line.base_data.next
					= ((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next;
				((VECTOR_LINE*)((VECTOR_LINE*)target->layer_data.vector_layer_p->base)->base_data.next)->base_data.prev
					= prev->layer_data.vector_layer_p->top_data;
				prev->layer_data.vector_layer_p->top_data = target->layer_data.vector_layer_p->top_data;
			}

			MEM_FREE_FUNC(target->layer_data.vector_layer_p);

			prev->layer_data.vector_layer_p->flags |= VECTOR_LAYER_RASTERIZE_ALL;

			RasterizeVectorLayer(target->window, prev, prev->layer_data.vector_layer_p);
		}

		target->layer_type = TYPE_NORMAL_LAYER;

		DeleteLayer(&target);
	}
}

void ChangeLayerOrder(LAYER* change_layer, LAYER* new_prev, LAYER** bottom)
{
	if(change_layer->prev != NULL)
	{
		change_layer->prev->next = change_layer->next;
	}
	if(change_layer->next != NULL)
	{
		change_layer->next->prev = change_layer->prev;
	}

	if(change_layer->layer_type == TYPE_ADJUSTMENT_LAYER)
	{
		change_layer->layer_data.adjustment_layer_p->target_layer = new_prev;
		change_layer->layer_data.adjustment_layer_p->release(change_layer->layer_data.adjustment_layer_p);
		change_layer->layer_data.adjustment_layer_p->update(change_layer->layer_data.adjustment_layer_p,
			new_prev, change_layer->window->mixed_layer, 0, 0, change_layer->width, change_layer->height);
	}

	if(new_prev != NULL)
	{
		if(new_prev->next != NULL)
		{
			new_prev->next->prev = change_layer;
		}
		change_layer->next = new_prev->next;
		new_prev->next = change_layer;
		
		if(change_layer == *bottom)
		{
			*bottom = new_prev;
		}
	}
	else
	{
		(*bottom)->prev = change_layer;
		change_layer->next = *bottom;
		*bottom = change_layer;
	}

	change_layer->prev = new_prev;

	// �Ǐ��L�����o�X���[�h�Ȃ�e�L�����o�X�̏������ύX
	if((change_layer->window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		LAYER *parent_new_prev = NULL;
		if(new_prev != NULL)
		{
			parent_new_prev = SearchLayer(change_layer->window->focal_window->layer, new_prev->name);
		}
		ChangeLayerOrder(SearchLayer(change_layer->window->focal_window->layer, change_layer->name),
			parent_new_prev, &change_layer->window->focal_window->layer);
	}
}

typedef struct _CHANGE_LAYER_ORDER_HISTORY
{
	uint16 layer_name_length;
	uint16 before_prev_name_length, after_prev_name_length;
	uint16 before_parent_name_length, after_parent_name_length;
	gchar *layer_name, *before_prev_name, *after_prev_name,
		*before_parent_name, *after_parent_name;
} CHANGE_LAYER_ORDER_HISTORY;

static void ChangeLayerOrderUndo(DRAW_WINDOW* window, void *data)
{
	CHANGE_LAYER_ORDER_HISTORY history;
	LAYER *new_prev = NULL, *change_layer;
	LAYER *parent;
	LAYER *target = NULL;
	LAYER *next_target;
	uint8 *buff = (uint8*)data;

	(void)memcpy(&history, buff, offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name));
	buff += offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name);
	history.layer_name = (gchar*)buff;
	buff += history.layer_name_length;
	history.before_prev_name = (gchar*)buff;
	buff += history.before_prev_name_length + history.after_prev_name_length
		+ history.before_parent_name_length;
	history.before_parent_name = (gchar*)buff;

	change_layer = SearchLayer(window->layer, history.layer_name);
	if(*history.before_prev_name != '\0')
	{
		new_prev = SearchLayer(window->layer, history.before_prev_name);
	}

	if(*history.before_parent_name != '\0')
	{
		int hierarchy = 1;;
		parent = SearchLayer(window->layer, history.before_parent_name);
		change_layer->layer_set = parent;
		while(parent->layer_set != NULL)
		{
			hierarchy++;
			parent = parent->layer_set;
		}
	}

	if(change_layer->layer_type == TYPE_LAYER_SET)
	{
		target =  change_layer->prev;
	}
	ChangeLayerOrder(change_layer, new_prev, &window->layer);
	while(target != NULL && target->layer_set == change_layer)
	{
		next_target = target->prev;
		ChangeLayerOrder(target, new_prev, &window->layer);
		target = next_target;
	}

	ClearLayerView(&window->app->layer_window);
	LayerViewSetDrawWindow(&window->app->layer_window, window);
}

static void ChangeLayerOrderRedo(DRAW_WINDOW* window, void *data)
{
	CHANGE_LAYER_ORDER_HISTORY history;
	LAYER *new_prev = NULL, *change_layer;
	LAYER *parent;
	LAYER *target = NULL;
	LAYER *next_target;
	uint8 *buff = (uint8*)data;

	(void)memcpy(&history, buff, offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name));
	buff += offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name);
	history.layer_name = (gchar*)buff;
	buff += history.layer_name_length;
	buff += history.before_prev_name_length;
	history.after_prev_name = (gchar*)buff;
	buff += history.after_prev_name_length + history.before_parent_name_length;
	history.after_parent_name = (gchar*)buff;

	change_layer = SearchLayer(window->layer, history.layer_name);
	if(*history.after_prev_name != '\0')
	{
		new_prev = SearchLayer(window->layer, history.after_prev_name);
	}

	if(*history.after_parent_name != '\0')
	{
		int hierarchy = 1;
		parent = SearchLayer(window->layer, history.after_parent_name);
		change_layer->layer_set = parent;
		while(parent->layer_set != NULL)
		{
			hierarchy++;
			parent = parent->layer_set;
		}
	}

	if(change_layer->layer_type == TYPE_LAYER_SET)
	{
		target = change_layer->prev;
	}
	ChangeLayerOrder(change_layer, new_prev, &window->layer);
	while(target != NULL && target->layer_set == change_layer)
	{
		next_target = target->prev;
		ChangeLayerOrder(target, new_prev, &window->layer);
		target = next_target;
	}

	ClearLayerView(&window->app->layer_window);
	LayerViewSetDrawWindow(&window->app->layer_window, window);
}

/*************************************************
* AddChangeLayerOrderHistory�֐�                 *
* ���C���[�̏����ύX�̗����f�[�^��ǉ�           *
* ����                                           *
* change_layer	: �����ύX�������C���[           *
* before_prev	: �����ύX�O�̑O�̃��C���[       *
* after_prev	: �����ύX��̑O�̃��C���[       *
* before_parent	: �����ύX�O�̏������C���[�Z�b�g *
*************************************************/
void AddChangeLayerOrderHistory(
	const LAYER* change_layer,
	const LAYER* before_prev,
	const LAYER* after_prev,
	const LAYER* before_parent
)
{
	CHANGE_LAYER_ORDER_HISTORY history;
	size_t data_size = 0;
	uint8 *buff, *data;
	data_size += history.layer_name_length = (uint16)strlen(change_layer->name) + 1;
	if(before_prev != NULL)
	{
		history.before_prev_name_length = (uint16)strlen(before_prev->name) + 1;
	}
	else
	{
		history.before_prev_name_length = 1;
	}
	data_size += history.before_prev_name_length;
	if(after_prev != NULL)
	{
		history.after_prev_name_length = (uint16)strlen(after_prev->name) + 1;
	}
	else
	{
		history.after_prev_name_length = 1;
	}
	data_size += history.after_prev_name_length;
	if(before_parent != NULL)
	{
		history.before_parent_name_length = (uint16)strlen(before_parent->name) + 1;
	}
	else
	{
		history.before_parent_name_length = 1;
	}
	data_size += history.before_parent_name_length;
	if(change_layer->layer_set != NULL)
	{
		history.after_parent_name_length = (uint16)strlen(change_layer->layer_set->name) + 1;
	}
	else
	{
		history.after_parent_name_length = 1;
	}
	data_size += history.after_parent_name_length;
	data_size += offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name);

	data = buff = (uint8*)MEM_ALLOC_FUNC(data_size);
	(void)memset(buff, 0, data_size);
	(void)memcpy(buff, &history, offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name));
	buff += offsetof(CHANGE_LAYER_ORDER_HISTORY, layer_name);
	(void)strcpy(buff, change_layer->name);
	buff += history.layer_name_length;
	if(before_prev != NULL)
	{
		(void)strcpy(buff, before_prev->name);
	}
	buff += history.before_prev_name_length;
	if(after_prev != NULL)
	{
		(void)strcpy(buff, after_prev->name);
	}
	buff += history.after_prev_name_length;
	if(before_parent != NULL)
	{
		(void)strcpy(buff, before_parent->name);
	}

	AddHistory(
		&change_layer->window->app->draw_window[
			change_layer->window->app->active_window]->history,
		change_layer->window->app->labels->layer_window.reorder,
		data,
		(uint32)data_size,
		ChangeLayerOrderUndo,
		ChangeLayerOrderRedo
	);

	MEM_FREE_FUNC(data);

	// �Ǐ��L�����o�X���[�h�Ȃ�e�L�����o�X�̗������쐬
	if((change_layer->window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		LAYER *parent_before_prev = NULL;
		LAYER *parent_after_prev = NULL;
		LAYER *parent_before_parent = NULL;
		if(before_prev != NULL)
		{
			parent_before_prev = SearchLayer(change_layer->window->focal_window->layer, before_prev->name);
		}
		if(after_prev != NULL)
		{
			parent_after_prev = SearchLayer(change_layer->window->focal_window->layer, after_prev->name);
		}
		if(before_parent != NULL)
		{
			parent_before_parent = SearchLayer(change_layer->window->focal_window->layer, before_parent->name);
		}
		AddChangeLayerOrderHistory(SearchLayer(change_layer->window->focal_window->layer, change_layer->name),
			parent_before_prev, parent_after_prev, parent_before_parent);
	}
}

void FillLayerColor(LAYER* target, uint8 color[3])
{
	if(target->layer_type != TYPE_NORMAL_LAYER)
	{	// �ʏ탌�C���[�ȊO�Ȃ�I��
		return;
	}

	cairo_set_source_rgb(target->cairo_p, color[0]*DIV_PIXEL, color[1]*DIV_PIXEL, color[2]*DIV_PIXEL);
	cairo_rectangle(target->cairo_p, 0, 0, target->width, target->height);
	if((target->window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{
		if((target->flags & LAYER_LOCK_OPACITY) == 0)
		{
			cairo_fill(target->cairo_p);
		}
		else
		{
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_ATOP);
			cairo_fill(target->cairo_p);
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_OVER);
		}
	}
	else
	{
		if((target->flags & LAYER_LOCK_OPACITY) == 0)
		{
			cairo_mask_surface(target->cairo_p, target->window->selection->surface_p, 0, 0);
		}
		else
		{
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_ATOP);
			cairo_mask_surface(target->cairo_p, target->window->selection->surface_p, 0, 0);
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_OVER);
		}
	}

	target->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

/*********************************************************
* FillLayerPattern�֐�                                   *
* ���C���[���p�^�[���œh��ׂ�                           *
* ����                                                   *
* target	: �h��ׂ����s�����C���[                     *
* patterns	: �p�^�[�����Ǘ�����\���̂̃A�h���X         *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* color		: �p�^�[�����O���[�X�P�[���̂Ƃ��Ɏg���F     *
*********************************************************/
void FillLayerPattern(
	LAYER* target,
	PATTERNS* patterns,
	APPLICATION* app,
	uint8 color[3]
)
{
	// �`��̈�̏��
	DRAW_WINDOW* window = app->draw_window[app->active_window];
	// �h��ׂ��Ɏg�p����p�^�[���̃T�[�t�F�[�X
	cairo_surface_t* pattern_surface = CreatePatternSurface(patterns,
		app->tool_window.color_chooser->rgb,app->tool_window.color_chooser->back_rgb,  0, PATTERN_MODE_SATURATION, 1);
	// �h��ׂ��p�^�[��
	cairo_pattern_t* pattern;
	// �g��k����
	gdouble zoom = 1 / (app->patterns.scale * 0.01);
	// �p�^�[���Ɋg��k�������Z�b�g���邽�߂̍s��
	cairo_matrix_t matrix;

	if(pattern_surface == NULL)
	{	// �p�^�[���T�[�t�F�[�X�쐬�Ɏ��s������I��
		return;
	}

	if(target->layer_type != TYPE_NORMAL_LAYER)
	{	// �ʏ탌�C���[�ȊO�Ȃ�Γh��Ԃ����~
		goto func_end;
	}

	// �T�[�t�F�[�X����p�^�[���쐬
	pattern = cairo_pattern_create_for_surface(pattern_surface);
	cairo_matrix_init_scale(&matrix, zoom, zoom);
	cairo_pattern_set_matrix(pattern, &matrix);
	cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

	if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{	// �I��͈͂Ȃ�
		cairo_set_source(target->cairo_p, pattern);
		cairo_rectangle(target->cairo_p, 0, 0,
			target->width, target->height);

		if((target->flags & LAYER_LOCK_OPACITY) == 0)
		{
			cairo_fill(target->cairo_p);
		}
		else
		{
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_ATOP);
			cairo_fill(target->cairo_p);
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_OVER);
		}
	}
	else
	{	// �I��͈͂���
		cairo_set_source(window->temp_layer->cairo_p, pattern);
		(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
		cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_rectangle(window->temp_layer->cairo_p, 0, 0, window->width, window->height);
		cairo_fill(window->temp_layer->cairo_p);
		cairo_set_source_surface(target->cairo_p, window->temp_layer->surface_p, 0, 0);

		if((target->flags & LAYER_LOCK_OPACITY) == 0)
		{
			cairo_mask_surface(target->cairo_p, window->selection->surface_p, 0, 0);
		}
		else
		{
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_ATOP);
			cairo_mask_surface(target->cairo_p, window->selection->surface_p, 0, 0);
			cairo_set_operator(target->cairo_p, CAIRO_OPERATOR_OVER);
		}
	}

func_end:
	cairo_pattern_destroy(pattern);
	cairo_surface_destroy(pattern_surface);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

void FlipLayerHorizontally(LAYER* target, LAYER* temp)
{
	int x, y;

	(void)memcpy(temp->pixels, target->pixels, target->height*target->stride);

	for(y=0; y<target->height; y++)
	{
		for(x=0; x<target->width; x++)
		{
			(void)memcpy(&target->pixels[y*target->stride+x*target->channel],
				&temp->pixels[y*target->stride+(target->width-x-1)*target->channel], target->channel);
		}
	}
}

void FlipLayerVertically(LAYER* target, LAYER* temp)
{
	int y;

	(void)memcpy(temp->pixels, target->pixels, target->height*target->stride);

	for(y=0; y<target->height; y++)
	{
		(void)memcpy(&target->pixels[y*target->stride],
			&temp->pixels[(target->height-y-1)*target->stride], target->stride);
	}
}

/***************************************
* RasterizeLayer�֐�                   *
* ���C���[�����X�^���C�Y               *
* ����                                 *
* target	: ���X�^���C�Y���郌�C���[ *
***************************************/
void RasterizeLayer(LAYER* target)
{
	// for���p�̃J�E���^
	int i;

	// �ʏ�̃��C���[�ł͖����ȃ��j���[��ݒ�
	for(i=0; i<target->window->app->menus.num_disable_if_normal_layer; i++)
	{
		gtk_widget_set_sensitive(target->window->app->menus.disable_if_normal_layer[i], FALSE);
	}

	// �x�N�g���f�[�^��j��
	if(target->layer_type == TYPE_VECTOR_LAYER)
	{
		DeleteVectorLayer(&target->layer_data.vector_layer_p);
	}
	else if(target->layer_type == TYPE_TEXT_LAYER)
	{
		DeleteTextLayer(&target->layer_data.text_layer_p);
	}

	// ���C���[�̃^�C�v�ύX
	target->layer_type = TYPE_NORMAL_LAYER;

	// ���X�^���C�Y���郌�C���[���A�N�e�B�u���C���[�Ȃ��
		// �ڍ�UI��؂�ւ�
	gtk_widget_destroy(target->window->app->tool_window.brush_table);
	CreateBrushTable(target->window->app, &target->window->app->tool_window,
		target->window->app->tool_window.brushes);
	// ���ʃc�[�����g���Ă��Ȃ��ꍇ�͏ڍ׃c�[���ݒ�UI���쐬
	if((target->window->app->tool_window.flags & TOOL_USING_BRUSH) != 0)
	{
		gtk_widget_destroy(target->window->app->tool_window.detail_ui);
		target->window->app->tool_window.detail_ui =
			target->window->app->tool_window.active_brush[target->window->app->input]->create_detail_ui(
				target->window->app, target->window->app->tool_window.active_brush[target->window->app->input]);
		// �X�N���[���h�E�B���h�E�ɓ����
		gtk_scrolled_window_add_with_viewport(
			GTK_SCROLLED_WINDOW(target->window->app->tool_window.detail_ui_scroll), target->window->app->tool_window.detail_ui);
		gtk_widget_show_all(target->window->app->tool_window.detail_ui);
	}

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(target->window->app->tool_window.brush_scroll),
		target->window->app->tool_window.brush_table);
	gtk_widget_show_all(target->window->app->tool_window.brush_table);

	(void)memset(target->window->work_layer->pixels, 0, target->window->pixel_buf_size);
}

/*****************************
* CreateLayerCopy�֐�        *
* ���C���[�̃R�s�[���쐬���� *
* ����                       *
* src	: �R�s�[���̃��C���[ *
* �Ԃ�l                     *
*	�쐬�������C���[         *
*****************************/
LAYER* CreateLayerCopy(LAYER* src)
{
	// �Ԃ�l
	LAYER* ret;
	// ���C���[�̖��O
	char layer_name[4096];
	// �J�E���^
	int i;

	// ���C���[�����쐬
	(void)sprintf(layer_name, "%s %s", src->window->app->labels->menu.copy, src->name);
	i = 1;
	while(CorrectLayerName(src->window->layer, layer_name) == 0)
	{
		(void)sprintf(layer_name, "%s %s (%d)", src->window->app->labels->menu.copy, src->name, i);
		i++;
	}

	// ���C���[�쐬
	ret = CreateLayer(
		src->x, src->y, src->width, src->height, src->channel, src->layer_type,
			src, src->next, layer_name, src->window);
	// ���C���[�̃t���O���R�s�[
	ret->flags = src->flags;

	// �s�N�Z���f�[�^���R�s�[
	(void)memcpy(ret->pixels, src->pixels, src->stride*src->height);

	if(src->layer_type == TYPE_VECTOR_LAYER)
	{	// �x�N�g�����C���[�Ȃ�x�N�g���f�[�^���R�s�[
		VECTOR_LINE *line = (VECTOR_LINE*)ret->layer_data.vector_layer_p->base,
			*src_line = (VECTOR_LINE*)((VECTOR_LINE*)src->layer_data.vector_layer_p->base)->base_data.next;
		while(src_line != NULL)
		{
			if(src_line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
			{
				line = CreateVectorLine(line, NULL);
				line->base_data.vector_type = src_line->base_data.vector_type;
				line->num_points = src_line->num_points;
				line->buff_size = src_line->buff_size;
				line->blur = src_line->blur;
				line->outline_hardness = src_line->outline_hardness;
			}
			else if(src_line->base_data.vector_type == VECTOR_TYPE_SQUARE)
			{
				VECTOR_SQUARE *square = (VECTOR_SQUARE*)CreateVectorShape(&line->base_data, NULL, VECTOR_TYPE_SQUARE);
				*square = *((VECTOR_SQUARE*)src_line);
				line = (VECTOR_LINE*)square;
			}
			else if(src_line->base_data.vector_type == VECTOR_TYPE_ECLIPSE)
			{
				VECTOR_ECLIPSE *square = (VECTOR_ECLIPSE*)CreateVectorShape(&line->base_data, NULL, VECTOR_TYPE_ECLIPSE);
				*square = *((VECTOR_ECLIPSE*)src_line);
				line = (VECTOR_LINE*)square;
			}

			line->base_data.layer = (VECTOR_LINE_LAYER*)MEM_ALLOC_FUNC(sizeof(*line->base_data.layer));
			line->base_data.layer->x = src_line->base_data.layer->x;
			line->base_data.layer->y = src_line->base_data.layer->y;
			line->base_data.layer->width = src_line->base_data.layer->width;
			line->base_data.layer->height = src_line->base_data.layer->height;
			line->base_data.layer->stride = src_line->base_data.layer->stride;
			line->base_data.layer->pixels = (uint8*)MEM_ALLOC_FUNC(line->base_data.layer->stride*line->base_data.layer->height);
			(void)memcpy(line->base_data.layer->pixels, src_line->base_data.layer->pixels,
				line->base_data.layer->stride*line->base_data.layer->height);
			line->base_data.layer->surface_p = cairo_image_surface_create_for_data(
				line->base_data.layer->pixels, CAIRO_FORMAT_ARGB32, line->base_data.layer->width,
				line->base_data.layer->height, line->base_data.layer->stride
			);
			line->base_data.layer->cairo_p = cairo_create(line->base_data.layer->surface_p);

			if(src_line->base_data.vector_type < NUM_VECTOR_LINE_TYPE)
			{
				line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*line->points)*src_line->buff_size);
				(void)memcpy(line->points, src_line->points, sizeof(*line->points)*src_line->num_points);
			}

			src_line = (VECTOR_LINE*)src_line->base_data.next;
		}

		ret->layer_data.vector_layer_p->num_lines = src->layer_data.vector_layer_p->num_lines;
		ret->layer_data.vector_layer_p->top_data = (void*)line;
		(void)memcpy(ret->layer_data.vector_layer_p->mix->pixels,
			src->layer_data.vector_layer_p->mix->pixels, src->stride*src->height);
	}
	else if(src->layer_type == TYPE_TEXT_LAYER)
	{	// �e�L�X�g���C���[�Ȃ�e�L�X�g�f�[�^���R�s�[
		ret->layer_data.text_layer_p = CreateTextLayer(
			src->window,
			src->layer_data.text_layer_p->x, src->layer_data.text_layer_p->y,
			src->layer_data.text_layer_p->width, src->layer_data.text_layer_p->height,
			src->layer_data.text_layer_p->base_size, src->layer_data.text_layer_p->font_size,
			src->layer_data.text_layer_p->font_id,
			src->layer_data.text_layer_p->color,
			src->layer_data.text_layer_p->balloon_type,
			src->layer_data.text_layer_p->back_color,
			src->layer_data.text_layer_p->line_color,
			src->layer_data.text_layer_p->line_width,
			&src->layer_data.text_layer_p->balloon_data,
			src->layer_data.text_layer_p->flags
		);
		ret->layer_data.text_layer_p->text = MEM_STRDUP_FUNC(
			src->layer_data.text_layer_p->text);
	}
	else
	{
		ret->layer_type = TYPE_NORMAL_LAYER;
	}

	return ret;
}

/*************************************************
* GetLayerChain�֐�                              *
* �s�����߂��ꂽ���C���[�z����擾����           *
* ����                                           *
* window	: �`��̈�̏��                     *
* num_layer	: ���C���[�����i�[����ϐ��̃A�h���X *
* �Ԃ�l                                         *
*	���C���[�z��                                 *
*************************************************/
LAYER** GetLayerChain(DRAW_WINDOW* window, uint16* num_layer)
{
	// �Ԃ�l
	LAYER** ret =
		(LAYER**)MEM_ALLOC_FUNC(sizeof(*ret)*LAYER_CHAIN_BUFFER_SIZE);
	// �o�b�t�@�T�C�Y
	unsigned int buffer_size = LAYER_CHAIN_BUFFER_SIZE;
	// �`�F�b�N���郌�C���[
	LAYER* layer = window->layer;
	// ���C���[�̐�
	unsigned int num = 0;

	// �s�����߂��ꂽ���C���[��T��
	while(layer != NULL)
	{	// �A�N�e�B�u�ȃ��C���[���s�����߂��ꂽ���C���[�Ȃ�
		if(layer == window->active_layer || (layer->flags & LAYER_CHAINED) != 0)
		{	// �z��ɒǉ�
			ret[num] = layer;
			num++;
		}

		// �o�b�t�@���s�����Ă�����Ċm��
		if(num >= buffer_size)
		{
			buffer_size += LAYER_CHAIN_BUFFER_SIZE;
			ret = MEM_REALLOC_FUNC(ret, buffer_size * sizeof(*ret));
		}

		layer = layer->next;
	}

	*num_layer = (uint16)num;

	return ret;
}

/***********************************************
* DeleteSelectAreaPixels�֐�                   *
* �I��͈͂̃s�N�Z���f�[�^���폜����           *
* ����                                         *
* window	: �`��̈�̏��                   *
* target	: �s�N�Z���f�[�^���폜���郌�C���[ *
* selection	: �I��͈͂��Ǘ����郌�C���[       *
***********************************************/
void DeleteSelectAreaPixels(
	DRAW_WINDOW* window,
	LAYER* target,
	LAYER* selection
)
{
	// for���p�̃J�E���^
	int i;

	// �ꎞ�ۑ��̃��C���[�ɑI��͈͂��ʂ�
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	for(i=0; i<selection->width*selection->height; i++)
	{
		window->temp_layer->pixels[i*4+3] = selection->pixels[i];
	}

	// ���C���[�����Ńs�N�Z���f�[�^�̍폜�����s
	window->layer_blend_functions[LAYER_BLEND_ALPHA_MINUS](window->temp_layer, target);
}

typedef struct _DELETE_SELECT_AREA_PIXELS_HISTORY
{
	// ���������s�N�Z���f�[�^�̍ő�ŏ��̍��W
	int32 min_x, min_y, max_x, max_y;
	// ���C���[���̒���
	uint16 layer_name_length;
	// ���C���[��
	char* layer_name;
	// �s�N�Z���f�[�^
	uint8* pixels;
} DELETE_SELECT_AREA_PIXELS_HISTORY;

/*******************************************
* DeletePixelsUndoRedo�֐�                 *
* �s�N�Z���f�[�^�폜����̃��h�D�A�A���h�D *
* ����                                     *
* window	: �`��̈�̏��               *
* p			: �����f�[�^                   *
*******************************************/
static void DeletePixelsUndoRedo(DRAW_WINDOW* window, void* p)
{
	// �����f�[�^�̌^�ɃL���X�g
	DELETE_SELECT_AREA_PIXELS_HISTORY data;
	// ���삪�s��ꂽ���C���[
	LAYER* target;
	// �o�C�g�P�ʂŃf�[�^���������߂̃L���X�g
	uint8* byte_data = (uint8*)p;
	// �s�N�Z���f�[�^��ύX���镝�A�����A��s���̃o�C�g��
	int32 width, height, stride;
	// �f�[�^�����ύX�p�̃o�b�t�@
	uint8* before_data;
	// for���p�̃J�E���^
	int i;

	// �����̊�{�����R�s�[
	(void)memcpy(&data, p, offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name));

	// ���C���[�����擾
	data.layer_name = (char*)&byte_data[offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name)];

	// �s�N�Z���f�[�^���擾
	data.pixels = &byte_data[offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name)+data.layer_name_length];

	// ���C���[������
	target = SearchLayer(window->layer, data.layer_name);

	// �s�N�Z���f�[�^�̕��A�����A��s���̃o�C�g�����v�Z
	width = data.max_x - data.min_x;
	height = data.max_y - data.min_y;
	stride = width * target->channel;

	// ���݂̃s�N�Z���f�[�^���L��
	before_data = (uint8*)MEM_ALLOC_FUNC(height*stride);
	for(i=0; i<height; i++)
	{
		(void)memcpy(&before_data[i*stride],
			&target->pixels[(i+data.min_y)*target->stride+data.min_x*target->channel], stride);
	}

	// �����f�[�^����s�N�Z���f�[�^���R�s�[
	for(i=0; i<height; i++)
	{
		(void)memcpy(&target->pixels[(i+data.min_y)*target->stride+data.min_x*target->channel],
			&data.pixels[i*stride], stride);
	}

	// �L�������f�[�^�ɗ����f�[�^��ύX����
	for(i=0; i<height; i++)
	{
		(void)memcpy(&data.pixels[i*stride], &before_data[i*stride], stride);
	}

	MEM_FREE_FUNC(before_data);
}

/*******************************************************
* AddDeletePixelsHistory�֐�                           *
* �s�N�Z���f�[�^��������̗����f�[�^��ǉ�����         *
* ����                                                 *
* tool_name	: �s�N�Z���f�[�^�����Ɏg�p�����c�[���̖��O *
* window	: �`��̈�̏��                           *
* min_x		: �s�N�Z���f�[�^��X���W�ŏ��l              *
* min_y		: �s�N�Z���f�[�^��Y���W�ŏ��l              *
* max_x		: �s�N�Z���f�[�^��X���W�ő�l              *
* max_y		: �s�N�Z���f�[�^��Y���W�ő�l              *
*******************************************************/
void AddDeletePixelsHistory(
	const char* tool_name,
	DRAW_WINDOW* window,
	LAYER* target,
	int32 min_x,
	int32 min_y,
	int32 max_x,
	int32 max_y
)
{
	// �����f�[�^�쐬�p
	DELETE_SELECT_AREA_PIXELS_HISTORY data;
	// ���ۂɗ���z��ɒǉ�����f�[�^�̃X�g���[��
	MEMORY_STREAM_PTR stream;
	// �����Ɏc���s�N�Z���f�[�^�̕��ƍ����ƈ�s���̃o�C�g��
	int32 width, height, stride;
	// �����f�[�^�̃T�C�Y
	size_t data_size;
	// for���p�̃J�E���^
	int i;

	// �����̊�{�����Z�b�g
	data.min_x = min_x - 1;
	data.min_y = min_y - 1;
	data.max_x = max_x + 1;
	data.max_y = max_y + 1;
	data.layer_name_length = (uint16)strlen(target->name) + 1;

	// �`��̈�O�֏o�Ă�����C��
	if(data.min_x < 0)
	{
		data.min_x = 0;
	}
	if(data.min_y < 0)
	{
		data.min_y = 0;
	}
	if(data.max_x >= window->width - 1)
	{
		data.max_x = window->width - 1;
	}
	if(data.max_y = window->height - 1)
	{
		data.max_y = window->height - 1;
	}

	// �I��͈͂̃f�[�^�𗚗��f�[�^�Ɏc��
	width = data.max_x - data.min_x;
	height = data.max_y - data.min_y;
	stride = width * target->channel;

	// �����f�[�^�p�̃X�g���[�����쐬���ď�������
	data_size = offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name)
		+ data.layer_name_length + stride*height;
	stream = CreateMemoryStream(data_size);

	(void)MemWrite(&data, 1, offsetof(DELETE_SELECT_AREA_PIXELS_HISTORY, layer_name), stream);
	(void)MemWrite(target->name, 1, data.layer_name_length, stream);

	for(i=0; i<height; i++)
	{
		(void)MemWrite(&target->pixels[(min_y+i)*target->stride+min_x*target->channel], 1,
			stride, stream);
	}

	// �����f�[�^�𗚗�z��ɒǉ�
	AddHistory(&window->history, tool_name, stream->buff_ptr, (uint32)data_size,
		DeletePixelsUndoRedo, DeletePixelsUndoRedo);

	(void)DeleteMemoryStream(stream);
}

/*****************************************************
* CalcLayerAverageRGBA�֐�                           *
* ���C���[�̕��σs�N�Z���f�[�^�l���v�Z               *
* ����                                               *
* target	: ���σs�N�Z���f�[�^�l���v�Z���郌�C���[ *
* �Ԃ�l                                             *
*	�Ώۃ��C���[�̕��σs�N�Z���f�[�^�l               *
*****************************************************/
RGBA_DATA CalcLayerAverageRGBA(LAYER* target)
{
	RGBA_DATA average;
	uint64 sum_r = 0, sum_g = 0,
		sum_b = 0, sum_a = 0;
	unsigned int index;
	int i;

	for(i=0; i<target->width*target->height; i++)
	{
		index = i * 4;
		sum_r += target->pixels[index];
		sum_g += target->pixels[index+1];
		sum_b += target->pixels[index+2];
		sum_a += target->pixels[index+3];
	}

	average.r = (uint8)(sum_r / index);
	average.g = (uint8)(sum_g / index);
	average.b = (uint8)(sum_b / index);
	average.a = (uint8)(sum_a / index);

	return average;
}

/*****************************************************
* CalcLayerAverageRGBAwithAlpha�֐�                  *
* ���l���l�����ă��C���[�̕��σs�N�Z���f�[�^�l���v�Z *
* ����                                               *
* target	: ���σs�N�Z���f�[�^�l���v�Z���郌�C���[ *
* �Ԃ�l                                             *
*	�Ώۃ��C���[�̕��σs�N�Z���f�[�^�l               *
*****************************************************/
RGBA_DATA CalcLayerAverageRGBAwithAlpha(LAYER* target)
{
	RGBA_DATA average;
	uint64 sum_r = 0, sum_g = 0,
		sum_b = 0, sum_a = 0;
	unsigned int index;
	int i;

	for(i=0; i<target->width*target->height; i++)
	{
		index = i * 4;
		sum_r += target->pixels[index] * target->pixels[index+3];
		sum_g += target->pixels[index+1] * target->pixels[index+3];
		sum_b += target->pixels[index+2] * target->pixels[index+3];
		sum_a += target->pixels[index+3];
	}

	average.r = (uint8)(sum_r / sum_a);
	average.g = (uint8)(sum_g / sum_a);
	average.b = (uint8)(sum_b / sum_a);
	average.a = (uint8)(sum_a / index);

	return average;
}

/*******************************************************
* FillTextureLayer�֐�                                 *
* �e�N�X�`�����C���[��I�����ꂽ�p�����[�^�[�œh��ׂ� *
* ����                                                 *
* layer		: �e�N�X�`�����C���[                       *
* textures	: �e�N�X�`���̃p�����[�^�[                 *
*******************************************************/
void FillTextureLayer(LAYER* layer, TEXTURES* textures)
{
	// ���C���[�����Z�b�g
	(void)memset(layer->pixels, 0, layer->width*layer->height);

	// �g�p����e�N�X�`�����w�肳��Ă�����
	if(textures->active_texture > 0)
	{
		cairo_surface_t *surface_p;
		cairo_pattern_t *pattern;
		cairo_matrix_t matrix;
		FLOAT_T zoom = 1 / textures->scale;
		FLOAT_T rate = (1 - textures->strength * 0.01);
		FLOAT_T angle = textures->angle * G_PI / 180;
		int i;
		// �s�N�Z���f�[�^�̕��������
		uint8 *pixel_copy = (uint8*)MEM_ALLOC_FUNC(
			textures->texture[textures->active_texture-1].stride*textures->texture[textures->active_texture-1].height);
		(void)memcpy(pixel_copy, textures->texture[textures->active_texture-1].pixels,
			textures->texture[textures->active_texture-1].stride*textures->texture[textures->active_texture-1].height);

		for(i=0; i<textures->texture[textures->active_texture-1].stride*textures->texture[textures->active_texture-1].height; i++)
		{
			pixel_copy[i] = (uint8)(pixel_copy[i] + (0xff - pixel_copy[i]) * rate + 0.8);
		}

		// �e�N�X�`���p�^�[�����쐬
		surface_p = cairo_image_surface_create_for_data(pixel_copy, CAIRO_FORMAT_A8,
			textures->texture[textures->active_texture-1].width, textures->texture[textures->active_texture-1].height,
				textures->texture[textures->active_texture-1].stride);
		pattern = cairo_pattern_create_for_surface(surface_p);
		cairo_matrix_init_scale(&matrix, zoom ,zoom);
		cairo_matrix_rotate(&matrix, angle);
		cairo_pattern_set_matrix(pattern, &matrix);
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

		// �h��ׂ������s
		cairo_set_operator(layer->cairo_p, CAIRO_OPERATOR_OVER);
		cairo_set_source(layer->cairo_p, pattern);
		cairo_rectangle(layer->cairo_p, 0, 0, layer->width, layer->height);
		cairo_fill(layer->cairo_p);

		cairo_pattern_destroy(pattern);
		cairo_surface_destroy(surface_p);
		MEM_FREE_FUNC(pixel_copy);
	}
}

/*********************************************
* GetLayerTypeString�֐�                     *
* ���C���[�̃^�C�v�萔�𕶎���ɂ���         *
* ����                                       *
* type	: ���C���[�̃^�C�v                   *
* �Ԃ�l                                     *
*	���C���[�̃^�C�v�̕�����(free���Ȃ�����) *
*********************************************/
const char* GetLayerTypeString(eLAYER_TYPE type)
{
	static const char *names[] =
	{"NORMAL", "VECTOR", "TEXT", "LAYER_SET", "3D_MODEL", "ADJUSTMENT"};

	return names[type];
}

/*********************************************
* GetLayerTypeFromString�֐�                 *
* �����񂩂烌�C���[�̃^�C�v�̒萔���擾���� *
* ����                                       *
* str	: ���C���[�̃^�C�v�̕�����           *
* �Ԃ�l                                     *
*	���C���[�̃^�C�v�̒萔                   *
*********************************************/
eLAYER_TYPE GetLayerTypeFromString(const char* str)
{
	if(strcmp(str, "NORMAL") == 0)
	{
		return TYPE_NORMAL_LAYER;
	}
	else if(strcmp(str, "VECTOR") == 0)
	{
		return TYPE_VECTOR_LAYER;
	}
	else if(strcmp(str, "TEXT") == 0)
	{
		return TYPE_TEXT_LAYER;
	}
	else if(strcmp(str, "3D_MODEL") == 0)
	{
		return TYPE_3D_LAYER;
	}
	else if(strcmp(str, "ADJUSTMENT") == 0)
	{
		return TYPE_ADJUSTMENT_LAYER;
	}

	return TYPE_NORMAL_LAYER;
}

/******************************************
* GetAverageColor�֐�                     *
* �w����W���ӂ̕��ϐF���擾              *
* ����                                    *
* target	: �F���擾���郌�C���[        *
* x			: X���W                       *
* y			: Y���W                       *
* size		: �F���擾����͈�            *
* color		: �擾�����F���i�[(4�o�C�g��) *
******************************************/
void GetAverageColor(LAYER* target, int x, int y, int size, uint8 color[4])
{
	unsigned int sum_color[4] = {0, 0, 0, 1};
	uint8 *ref;
	int start_x, start_y;
	int width, height;
	int count_x, count_y;
	int count = 0;

	start_x = x - size,	start_y = y - size;
	width = height = size * 2 + 1;

	if(start_x < 0)
	{
		start_x = 0;
	}
	if(start_x + width > target->width)
	{
		width = target->width - start_x;
	}

	if(start_y < 0)
	{
		start_y = 0;
	}
	if(start_y + height > target->height)
	{
		height = target->height - start_y;
	}

	for(count_y=0; count_y<height; count_y++)
	{
		ref = &target->pixels[(start_y+count_y)*target->stride+start_x*4];
		for(count_x=0; count_x<width; count_x++, ref+=4)
		{
			sum_color[0] += ref[0] * ref[3];
			sum_color[1] += ref[1] * ref[3];
			sum_color[2] += ref[2] * ref[3];
			sum_color[3] += ref[3];
			count++;
		}
	}

	color[0] = (uint8)(sum_color[0] / sum_color[3]);
	color[1] = (uint8)(sum_color[1] / sum_color[3]);
	color[2] = (uint8)(sum_color[2] / sum_color[3]);
	color[3] = (uint8)(sum_color[3] / count);
}

/***************************************************
* GetBlendedUnderLayer�֐�                         *
* �Ώۂ�艺�̃��C���[�������������C���[���擾���� *
* ����                                             *
* target			: �Ώۂ̃��C���[               *
* window			: �`��̈�̏��               *
* use_back_ground	: �w�i�F���g�p���邩�ǂ���     *
* �Ԃ�l                                           *
*	�����������C���[                               *
***************************************************/
LAYER* GetBlendedUnderLayer(LAYER* target, DRAW_WINDOW* window, int use_back_ground)
{
	LAYER *ret = CreateLayer(0, 0, window->width, window->height, window->channel,
		TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
	LAYER *src = window->layer;

	if(use_back_ground != 0)
	{
		(void)memcpy(ret->pixels, window->back_ground, window->pixel_buf_size);
	}

	while(src != NULL && src != target)
	{
		if((src->flags & LAYER_FLAG_INVISIBLE) == 0)
		{
			window->layer_blend_functions[src->layer_mode](src, ret);
		}
		src = src->next;
	}

	return ret;
}

/***************************************************
* AddLayerGroupTemplate�֐�                        *
* ���C���[���܂Ƃ߂č쐬����                       *
* ����                                             *
* group			: ���C���[���܂Ƃ߂č쐬����f�[�^ *
* add_flags		: ���C���[��ǉ����邩�̃t���O     *
* previous		: �O�̃��C���[                     *
* next			: ���̃��C���[                     *
* num_layers	: �ǉ��������C���[�̐�             *
* �Ԃ�l                                           *
*	����I��:�ǉ��������C���[	�ُ�I��:NULL      *
***************************************************/
LAYER** AddLayerGroupTemplate(
	LAYER_GROUP_TEMPLATE* group,
	uint8* add_flags,
	LAYER* previous,
	LAYER* next,
	int* num_layers
)
{
	DRAW_WINDOW *canvas = NULL;
	LAYER **result = NULL;
	LAYER_GROUP_TEMPLATE_NODE *node;
	LAYER *layer_set;
	LAYER *local_previous = previous;
	char group_name[MAX_LAYER_NAME_LENGTH];
	char layer_name[MAX_LAYER_NAME_LENGTH];
	int32 width, height;
	int layer_counter;
	int counter;
	int i;

	if(previous != NULL)
	{
		canvas = previous->window;
		width = previous->width;
		height = previous->height;
	}
	else if(next != NULL)
	{
		canvas = next->window;
		width = next->width;
		height = next->height;
	}
	else
	{
		return NULL;
	}

	*num_layers = ((group->flags & LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET) != 0) ? 1 : 0;
	node = group->names;
	layer_counter = 0;
	while(node != NULL)
	{
		if(FLAG_CHECK(add_flags, layer_counter) != FALSE)
		{
			(*num_layers)++;
		}
		layer_counter++;
		node = node->next;
	}

	if(*num_layers == 0)
	{
		return NULL;
	}

	result = (LAYER**)MEM_ALLOC_FUNC(sizeof(*result)*(*num_layers));
	if(result == NULL)
	{
		return NULL;
	}

	layer_counter = 0;
	if((group->flags & LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET) != 0)
	{
		if(CorrectLayerName(canvas->layer, group->group_name) == 0)
		{
			int name_counter = 1;
			do
			{
				(void)sprintf(group_name, "%s (%d)", group->group_name, name_counter);
				if(CorrectLayerName(canvas->layer, group_name) != FALSE)
				{
					break;
				}
				name_counter++;
			} while(1);
		}
		else
		{
			(void)strcpy(group_name, group->group_name);
		}
	}
	else
	{
		(void)strcpy(group_name, group->group_name);
	}

	node = group->names;
	layer_counter = 0;
	counter = 0;
	while(node != NULL)
	{
		if(FLAG_CHECK(add_flags, counter) != FALSE)
		{
			int name_counter = 1;

			(void)sprintf(layer_name, "%s - %s", group_name, node->name);
			if(CorrectLayerName(canvas->layer, layer_name) == FALSE)
			{
				do
				{
					(void)sprintf(group_name, "%s - %s (%d)", group_name, node->name, name_counter);
					if(CorrectLayerName(canvas->layer, group_name) != FALSE)
					{
						break;
					}
					name_counter++;
				} while(1);
			}

			result[layer_counter] = CreateLayer(0, 0, width, height, 4,
				node->layer_type, local_previous, next, layer_name, canvas);
			local_previous = result[layer_counter];
			layer_counter++;
		}
			
		counter++;
		node = node->next;
	}

	if((group->flags & LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET) != 0)
	{
		layer_set = CreateLayer(0, 0, width, height, 4,
			TYPE_LAYER_SET, local_previous, next, group_name, canvas);
		result[(*num_layers)-1] = layer_set;
		for(i=0; i<(*num_layers)-1; i++)
		{
			result[i]->layer_set = layer_set;
		}
	}

	if(previous == NULL)
	{
		canvas->layer = result[0];
	}

	if((canvas->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		canvas = canvas->focal_window;

		layer_counter = 0;
		if((group->flags & LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET) != 0)
		{
			if(CorrectLayerName(canvas->layer, group->group_name) == 0)
			{
				int name_counter = 1;
				do
				{
					(void)sprintf(group_name, "%s (%d)", group->group_name, name_counter);
					if(CorrectLayerName(canvas->layer, group_name) != FALSE)
					{
						break;
					}
					name_counter++;
				} while(1);
			}
			else
			{
				(void)strcpy(group_name, group->group_name);
			}
		}
		else
		{
			(void)strcpy(group_name, group->group_name);
		}

		node = group->names;
		counter = layer_counter = 0;
		while(node != NULL)
		{
			if(FLAG_CHECK(add_flags, counter) != FALSE)
			{
				int name_counter = 1;

				(void)sprintf(layer_name, "%s - %s", group_name, node->name);
				if(CorrectLayerName(canvas->layer, layer_name) == FALSE)
				{
					do
					{
						(void)sprintf(group_name, "%s - %s (%d)", group_name, node->name, name_counter);
						if(CorrectLayerName(canvas->layer, group_name) != FALSE)
						{
							break;
						}
						name_counter++;
					} while(1);
				}

				result[layer_counter] = CreateLayer(0, 0, width, height, 4,
					node->layer_type, local_previous, next, layer_name, canvas);
				local_previous = result[layer_counter];
				layer_counter++;
			}

			counter++;
			node = node->next;
		}

		if((group->flags & LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET) != 0)
		{
			layer_set = CreateLayer(0, 0, width, height, 4,
				TYPE_LAYER_SET, local_previous, next, group_name, canvas);
			result[(*num_layers)-1] = layer_set;;
			for(i=0; i<(*num_layers)-1; i++)
			{
				result[i]->layer_set = layer_set;
			}
		}

		if(previous == NULL)
		{
			canvas->layer = result[0];
		}
	}

	return result;
}

static void AddLayerGroupUndo(DRAW_WINDOW* canvas, void* p)
{
	MEMORY_STREAM stream = {0};
	LAYER *target;
	int32 *data32;
	int32 num;
	int32 num_layer;
	int i;

	stream.buff_ptr = (uint8*)p;
	data32 = (int32*)p;
	stream.data_size = *data32;
	(void)MemSeek(&stream, sizeof(num), SEEK_CUR);

	(void)MemRead(&num, sizeof(num), 1, &stream);
	(void)MemSeek(&stream, num, SEEK_CUR);

	(void)MemRead(&num_layer, sizeof(num_layer), 1, &stream);
	for(i=0; i<num_layer; i++)
	{
		(void)MemRead(&num, sizeof(num), 1, &stream);
		target = SearchLayer(canvas->layer, (const gchar*)&stream.buff_ptr[stream.data_point]);
		DeleteLayer(&target);
		canvas->num_layer--;
		if((canvas->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
		{
			target = SearchLayer(canvas->focal_window->layer, (const gchar*)&stream.buff_ptr[stream.data_point]);
			DeleteLayer(&target);
			canvas->focal_window->num_layer--;
		}
		(void)MemSeek(&stream, num + sizeof(num), SEEK_CUR);
	}
}

static void AddLayerGroupRedo(DRAW_WINDOW* canvas, void* p)
{
	APPLICATION *app;
	MEMORY_STREAM stream = {0};
	LAYER *target;
	LAYER *prev = NULL;
	LAYER *next = NULL;
	char name[MAX_LAYER_NAME_LENGTH];
	int32 *data32;
	int32 num;
	int32 num_layer;
	int i;

	app = canvas->app;

	stream.buff_ptr = (uint8*)p;
	data32 = (int32*)p;
	stream.data_size = *data32;
	(void)MemSeek(&stream, sizeof(num), SEEK_CUR);

	(void)MemRead(&num, sizeof(num), 1, &stream);
	if(num > 0)
	{
		prev = SearchLayer(canvas->layer, (const gchar*)&stream.buff_ptr[stream.data_point]);
	}
	(void)MemSeek(&stream, num, SEEK_CUR);

	(void)MemRead(&num_layer, sizeof(num_layer), 1, &stream);
	for(i=0; i<num_layer; i++)
	{
		(void)MemRead(&num, sizeof(num), 1, &stream);
		(void)MemRead(name, 1, num, &stream);
		(void)MemRead(&num, sizeof(num), 1, &stream);
		if(prev == NULL)
		{
			next = canvas->layer;
		}
		else
		{
			next = prev->next;
		}
		target = CreateLayer(0, 0, canvas->width, canvas->height, 4, (eLAYER_TYPE)num, prev, next,
			name, canvas);
		canvas->num_layer++;
		LayerViewAddLayer(target, canvas->layer,
			app->layer_window.view, canvas->num_layer);
		if((canvas->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
		{
			LAYER *new_layer;
			new_layer = CreateLayer(0, 0, canvas->focal_window->width, canvas->focal_window->height, 4, (eLAYER_TYPE)num,
				(prev == NULL) ? NULL : SearchLayer(canvas->focal_window->layer, prev->name),
				(next == NULL) ? NULL : SearchLayer(canvas->focal_window->layer, next->name),
				name, canvas->focal_window
			);
			canvas->focal_window->num_layer++;
		}

		prev = target;
		next = target->next;
	}
}

/*************************************************
* AddLayerGroupHistory�֐�                       *
* ���C���[���܂Ƃ߂č쐬���闚�����쐬           *
* ����                                           *
* layers		: �ǉ��������C���[               *
* num_layers	: �ǉ��������C���[�̐�           *
* prev			: �ǉ��������C���[�̑O�̃��C���[ *
*************************************************/
void AddLayerGroupHistory(LAYER**layers, int num_layers, LAYER* prev)
{
	DRAW_WINDOW *canvas;
	MEMORY_STREAM *stream;
	size_t size;
	int32 num;
	int i;

	if(layers == NULL || layers[0] == NULL)
	{
		return;
	}

	canvas = layers[0]->window;
	stream = CreateMemoryStream(4096);

	(void)MemSeek(stream, sizeof(num), SEEK_CUR);

	if(prev != NULL)
	{
		num = (int32)strlen(prev->name) + 1;
	}
	else
	{
		num = 0;
	}
	(void)MemWrite(&num, sizeof(num), 1, stream);
	(void)MemWrite(prev->name, 1, num, stream);

	num = num_layers;
	(void)MemWrite(&num, sizeof(num), 1, stream);
	for(i=0; i<num_layers; i++)
	{
		num = (int32)strlen(layers[i]->name) + 1;
		(void)MemWrite(&num, sizeof(num), 1, stream);
		(void)MemWrite(layers[i]->name, 1, num, stream);
		num = layers[i]->layer_type;
		(void)MemWrite(&num, sizeof(num), 1, stream);
	}
	size = stream->data_point;
	num = (int32)size;
	(void)MemSeek(stream, 0, SEEK_SET);
	(void)MemWrite(&num, sizeof(num), 1, stream);

	AddHistory(&canvas->history, canvas->app->labels->menu.new_layer_group,
		(const void*)stream->buff_ptr, size, (history_func)AddLayerGroupUndo, (history_func)AddLayerGroupRedo);

	DeleteMemoryStream(stream);
}

#ifdef __cplusplus
}
#endif
