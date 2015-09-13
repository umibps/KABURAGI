// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <math.h>
#include "shape_model.h"
#include "utils.h"
#include "scene.h"
#include "project.h"
#include "application.h"
#include "render_engine.h"
#include "memory.h"

#ifndef M_PI
# define M_PI 3.1415926535897932384626433832795
#endif

static void ShapeModelSetWorldPosition(SHAPE_MODEL* model, float* position)
{
	COPY_VECTOR3(model->position, position);
}

static void ShapeModelSetWorldRotation(SHAPE_MODEL* model, float* orientation)
{
	COPY_VECTOR4(model->rotation, orientation);
}

static void ShapeModelGetWorldTranslation(SHAPE_MODEL* model, float* translation)
{
	COPY_VECTOR3(translation, model->position);
}

static void ShapeModelSetWorldTranslation(SHAPE_MODEL* model, float* translation)
{
	COPY_VECTOR3(model->position, translation);
}

static void ShapeModelGetWorldRotation(SHAPE_MODEL* model, float* orientation)
{
	COPY_VECTOR4(orientation, model->rotation);
}

static void ShapeModelSetOpacity(SHAPE_MODEL* model, float opacity)
{
	model->interface_data.opacity = opacity;
}

static int ShapeModelIsVisible(SHAPE_MODEL* model)
{
	return model->flags & SHAPE_MODEL_FLAG_VISIBLE;
}

static void ShapeModelSetVisible(SHAPE_MODEL* model, int is_visible)
{
	if(is_visible != FALSE)
	{
		model->flags |= SHAPE_MODEL_FLAG_VISIBLE;
	}
	else
	{
		model->flags &= ~(SHAPE_MODEL_FLAG_VISIBLE);
	}
}

size_t ShapeDefaultDynamicVertexBufferGetSize(SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer)
{
	return buffer->interface_data.base.get_stride_size(buffer) * buffer->model->vertices->num_data;
}

size_t ShapeDefaultDymamicVertexBufferGetStrideSize(SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer)
{
	return sizeof(SHAPE_MODEL_DEFAULT_DYNAMIC_BUFFER_UNIT);
}

size_t ShapeDefaultDynamicVertexBufferGetStrideOffset(SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer, eMODEL_STRIDE_TYPE type)
{
	switch(type)
	{
	case MODEL_VERTEX_STRIDE:
		return offsetof(SHAPE_MODEL_DEFAULT_DYNAMIC_BUFFER_UNIT, position);
	case MODEL_NORMAL_STRIDE:
		return offsetof(SHAPE_MODEL_DEFAULT_DYNAMIC_BUFFER_UNIT, normal);
	}

	return 0;
}

void DeleteShapeDefaultDynamicVertexBuffer(SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer)
{
	MEM_FREE_FUNC(buffer);
}

void ShapeDefaultDynamicVertexBufferPerformTransform(
	SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER* buffer,
	void* address,
	float* camera_position
)
{
	SHAPE_MODEL_DEFAULT_DYNAMIC_BUFFER_UNIT *unit = (SHAPE_MODEL_DEFAULT_DYNAMIC_BUFFER_UNIT*)address;
	SHAPE_VERTEX *vertices = (SHAPE_VERTEX*)buffer->model->vertices->buffer;
	const int num_vertices = (int)buffer->model->vertices->num_data;
	int i;

	for(i=0; i<num_vertices; i++)
	{
		COPY_VECTOR3(unit[i].position, vertices[i].position);
		COPY_VECTOR3(unit[i].normal, vertices[i].normal);
	}
}

SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER* ShapeDefaultDynamicVertexBufferNew(
	SHAPE_MODEL* model,
	MODEL_INDEX_BUFFER_INTERFACE* index_buffer
)
{
	SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER *ret;

	ret = (SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER*)MEM_ALLOC_FUNC(sizeof(*ret));

	ret->model = model;

	ret->interface_data.base.get_size = (size_t (*)(void*))ShapeDefaultDynamicVertexBufferGetSize;
	ret->interface_data.base.get_stride_size = (size_t (*)(void*))ShapeDefaultDymamicVertexBufferGetStrideSize;
	ret->interface_data.base.get_stride_offset = (size_t (*)(void*, eMODEL_STRIDE_TYPE))ShapeDefaultDynamicVertexBufferGetStrideOffset;
	ret->interface_data.base.delete_func = (void (*)(void*))DeleteShapeDefaultDynamicVertexBuffer;

	return ret;
}

void ShapeModelGetDynamicVertexBuffer(
	SHAPE_MODEL* model,
	SHAPE_DEFAULT_DYNAMIC_VERTEX_BUFFER** buffer,
	MODEL_INDEX_BUFFER_INTERFACE* index_buffer
)
{
	if(buffer != NULL)
	{
		if(*buffer != NULL)
		{
			DeleteShapeDefaultDynamicVertexBuffer(*buffer);
		}

		*buffer = ShapeDefaultDynamicVertexBufferNew(model, index_buffer);
	}
}

size_t ShapeDefaultIndexBufferGetSize(SHAPE_DEFAULT_INDEX_BUFFER* buffer)
{
	return buffer->interface_data.base.get_stride_size(buffer) * buffer->model->indices->num_data;
}

size_t ShapeDefaultIndexBufferGetStrideSize(SHAPE_DEFAULT_INDEX_BUFFER* buffer)
{
	return sizeof(*buffer->model->indices->buffer);
}

void* ShapeDefaultIndexBufferGetBytes(SHAPE_DEFAULT_INDEX_BUFFER* buffer)
{
	return (void*)buffer->model->indices->buffer;
}

void DeleteShapeDefaultIndexBuffer(SHAPE_DEFAULT_INDEX_BUFFER* buffer)
{
	MEM_FREE_FUNC(buffer);
}

SHAPE_DEFAULT_INDEX_BUFFER* ShapeDefaultIndexBufferNew(
	WORD_ARRAY* indices,
	const int num_vertices,
	SHAPE_MODEL* model
)
{
	SHAPE_DEFAULT_INDEX_BUFFER *ret;

	ret = (SHAPE_DEFAULT_INDEX_BUFFER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0 , sizeof(*ret));

	ret->model = model;

	ret->interface_data.bytes = (void* (*)(void*))ShapeDefaultIndexBufferGetBytes;
	ret->interface_data.base.get_size = (size_t (*)(void*))ShapeDefaultIndexBufferGetSize;
	ret->interface_data.base.get_stride_size = (size_t (*)(void*))ShapeDefaultIndexBufferGetStrideSize;
	ret->interface_data.base.delete_func = (void (*)(void*))DeleteShapeDefaultIndexBuffer;

	return ret;
}

void ShapeModelGetIndexBuffer(
	SHAPE_MODEL* model,
	SHAPE_DEFAULT_INDEX_BUFFER** buffer
)
{
	if(buffer != NULL)
	{
		if(*buffer != NULL)
		{
			DeleteShapeDefaultIndexBuffer(*buffer);
		}

		*buffer = ShapeDefaultIndexBufferNew(model->indices, (int)model->indices->num_data, model);
	}
}

void** ShapeModelGetLabels(SHAPE_MODEL* model, int* num_labels)
{
	*num_labels = 0;
	return NULL;
}

void** ShapeModelGetBones(SHAPE_MODEL* model, int* num_bones)
{
	*num_bones = 0;
	return NULL;
}

void ReleaseShapeModel(SHAPE_MODEL* model)
{
	StructArrayDestroy(&model->vertices, NULL);
	WordArrayDestroy(&model->indices);
}

void ShapeModelPerformUpdate(SHAPE_MODEL* model, int force_sync)
{
	if(force_sync != FALSE || (model->flags & SHAPE_MODEL_FLAG_UPDATE) != 0)
	{
		model->flags |= SHAPE_MODEL_FLAG_RENDER_UPDATE;
		model->flags &= ~(SHAPE_MODEL_FLAG_UPDATE);
	}
}

void InitializeShapeModel(SHAPE_MODEL* model, SCENE* scene, eSHAPE_TYPE shape_type)
{
	char *shape_name = "";
	char name[256];

	(void)memset(model, 0, sizeof(*model));

	model->shape_type = shape_type;

	model->rotation[3] = 1;

	model->surface_color[0] = 0xFF;
	model->surface_color[1] = 0xFF;
	model->surface_color[2] = 0xFF;

	model->line_color[0] = 0x00;
	model->line_color[1] = 0x00;
	model->line_color[2] = 0x00;

	model->edge_color[0] = 0x00;
	model->edge_color[1] = 0x00;
	model->edge_color[2] = 0x00;

	model->interface_data.edge_width = 1.0f;
	model->line_width = 1.0f;
	model->num_lines = DEFAULT_SHAPE_MODEL_NUM_LINES;

	model->flags = SHAPE_MODEL_FLAG_VISIBLE | SHAPE_MODEL_FLAG_DRAW_LINE
		| SHAPE_MODEL_FLAG_UPDATE | SHAPE_MODEL_FLAG_INDEX_BUFFER_UPDATE;
	model->vertices = StructArrayNew(sizeof(SHAPE_VERTEX), DEFAULT_BUFFER_SIZE);
	model->indices = WordArrayNew(DEFAULT_BUFFER_SIZE);

	model->interface_data.scene = scene;
	switch(shape_type)
	{
	case SHAPE_TYPE_CUBOID:
		shape_name = scene->project->application_context->label.control.cuboid;
		break;
	case SHAPE_TYPE_CONE:
		shape_name = scene->project->application_context->label.control.cone;
		break;
	}
	(void)sprintf(name, "%s %d", shape_name, (int)scene->models->num_data + 1);
	model->interface_data.name = MEM_STRDUP_FUNC(name);
	model->interface_data.opacity = 1.0f;
	model->interface_data.scale_factor = 1.0f;

	model->interface_data.type = MODEL_TYPE_SHAPE;
	model->interface_data.find_bone =
		(void* (*)(void*, const char*))DummyFuncNullReturn2;
	model->interface_data.find_morph =
		(void* (*)(void*, const char*))DummyFuncNullReturn2;
	model->interface_data.get_world_position =
		(void (*)(void*, float*))ShapeModelGetWorldTranslation;
	model->interface_data.get_world_translation =
		(void (*)(void*, float*))ShapeModelGetWorldTranslation;
	model->interface_data.set_world_position =
		(void (*)(void*, float*))ShapeModelSetWorldTranslation;
	model->interface_data.get_world_orientation =
		(void (*)(void*, float*))ShapeModelGetWorldRotation;
	model->interface_data.set_world_orientation =
		(void (*)(void*, float*))ShapeModelSetWorldRotation;
	model->interface_data.is_visible =
		(int (*)(void*))ShapeModelIsVisible;
	model->interface_data.set_visible =
		(void (*)(void*, int))ShapeModelSetVisible;
	model->interface_data.get_labels =
		(void** (*)(void*, int*))ShapeModelGetLabels;
	model->interface_data.get_bones =
		(void** (*)(void*, int*))ShapeModelGetBones;
	model->interface_data.perform_update =
		(void (*)(void*, int))ShapeModelPerformUpdate;
	model->interface_data.get_dynamic_vertex_buffer =
		(void (*)(void*, void**, void*))ShapeModelGetDynamicVertexBuffer;
	model->interface_data.get_index_buffer =
		(void (*)(void*, void**))ShapeModelGetIndexBuffer;
}

size_t WriteShapeModelDataAndState(
	SHAPE_MODEL* model,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
)
{
	// 書き出したバイト数
	size_t write_size = 0;
	// 浮動小数点数書き出し用
	float float_values[4];
	// 1バイトデータ書き出し用
	uint8 byte_data;
	// 4バイトデータ書き出し用
	uint32 data32;

	// モデルの位置・向きを書き出す
	write_size += write_func(model->position, sizeof(*model->position), 3, dst) * sizeof(*model->position);
	write_size += write_func(model->rotation, sizeof(*model->rotation), 4, dst) * sizeof(*model->rotation);
	// モデルのサイズを書き出す
	float_values[0] = (float)model->interface_data.scale_factor;
	write_size += write_func(float_values, sizeof(*float_values), 1, dst) * sizeof(*float_values);

	// 形状のタイプを書き出す
	byte_data = (uint8)model->shape_type;
	write_size += write_func(&byte_data, sizeof(byte_data), 1, dst) * sizeof(byte_data);

	// 面、線、エッジ、塗り潰しの色を書き出す
	write_size += write_func(model->surface_color, sizeof(*model->surface_color), 3, dst) * sizeof(*model->surface_color);
	write_size += write_func(model->line_color, sizeof(*model->line_color), 3, dst) * sizeof(*model->line_color);
	write_size += write_func(model->edge_color, sizeof(*model->edge_color), 3, dst) * sizeof(*model->edge_color);

	// 線の太さを書き出す
	write_size += write_func(&model->line_width, sizeof(model->line_width), 1, dst) * sizeof(model->line_width);

	// 線の本数を書き出す
	data32 = model->num_lines;
	write_size += write_func(&data32, sizeof(data32), 1, dst) * sizeof(data32);

	// フラグ情報を書き出す
	data32 = model->flags;
	write_size += write_func(&data32, sizeof(data32), 1, dst) * sizeof(data32);

	return write_size;
}

void CuboidModelPerformUpdate(
	CUBOID_MODEL* model,
	int force_sync
)
{
	SHAPE_VERTEX vertices[3];
	int i;

	if((model->base_data.flags & SHAPE_MODEL_FLAG_UPDATE) == 0)
	{
		return;
	}

	ShapeModelPerformUpdate(&model->base_data, force_sync);

	model->base_data.vertices->num_data = 0;
	model->base_data.indices->num_data = 0;

	model->base_data.num_model_indices = 0;
	model->base_data.num_line_indices = 0;
	model->base_data.num_edge_indices = 0;

	vertices[0].position[0] = - model->width * 0.5f;
	vertices[0].position[1] = 0;
	vertices[0].position[2] = - model->depth * 0.5f;
	vertices[1].position[0] = model->width * 0.5f;
	vertices[1].position[1] = 0;
	vertices[1].position[2] = - model->depth * 0.5f;
	vertices[2].position[0] = - model->width * 0.5f;
	vertices[2].position[1] = model->height;
	vertices[2].position[2] = - model->depth * 0.5f;
	CalculateNormalVector(vertices[0].position, vertices[1].position, vertices[2].position, vertices[0].normal);
	REVERSE_VECTOR3(vertices[0].normal);
	StructArrayAppend(model->base_data.vertices, &vertices[0]);

	CalculateNormalVector(vertices[1].position, vertices[2].position, vertices[0].position, vertices[1].normal);
	REVERSE_VECTOR3(vertices[1].normal);
	StructArrayAppend(model->base_data.vertices, &vertices[1]);

	CalculateNormalVector(vertices[2].position, vertices[0].position, vertices[1].position, vertices[2].normal);
	REVERSE_VECTOR3(vertices[2].normal);
	StructArrayAppend(model->base_data.vertices, &vertices[2]);

	WordArrayAppend(model->base_data.indices, 2);
	WordArrayAppend(model->base_data.indices, 1);
	WordArrayAppend(model->base_data.indices, 0);

	vertices[0].position[0] = model->width * 0.5f;
	vertices[0].position[1] = model->height;
	CalculateNormalVector(vertices[0].position, vertices[1].position, vertices[2].position, vertices[0].normal);
	StructArrayAppend(model->base_data.vertices, &vertices[0]);

	WordArrayAppend(model->base_data.indices, 3);
	WordArrayAppend(model->base_data.indices, 1);
	WordArrayAppend(model->base_data.indices, 2);

	vertices[1].position[0] = - model->width * 0.5f;
	vertices[1].position[1] = model->height;
	vertices[1].position[2] = model->depth * 0.5f;
	CalculateNormalVector(vertices[1].position, vertices[2].position, vertices[0].position, vertices[1].normal);
	REVERSE_VECTOR3(vertices[1].normal);
	StructArrayAppend(model->base_data.vertices, &vertices[1]);

	WordArrayAppend(model->base_data.indices, 2);
	WordArrayAppend(model->base_data.indices, 4);
	WordArrayAppend(model->base_data.indices, 3);

	vertices[2].position[0] = model->width * 0.5f;
	vertices[2].position[1] = model->height;
	vertices[2].position[2] = model->depth * 0.5f;
	CalculateNormalVector(vertices[2].position, vertices[0].position, vertices[1].position, vertices[2].normal);
	StructArrayAppend(model->base_data.vertices, &vertices[2]);

	WordArrayAppend(model->base_data.indices, 3);
	WordArrayAppend(model->base_data.indices, 4);
	WordArrayAppend(model->base_data.indices, 5);

	vertices[0].position[0] = - model->width * 0.5f;
	vertices[0].position[1] = 0;
	vertices[0].position[2] = model->depth * 0.5f;
	CalculateNormalVector(vertices[0].position, vertices[1].position, vertices[2].position, vertices[0].normal);
	REVERSE_VECTOR3(vertices[0].normal);
	StructArrayAppend(model->base_data.vertices, &vertices[0]);

	WordArrayAppend(model->base_data.indices, 5);
	WordArrayAppend(model->base_data.indices, 4);
	WordArrayAppend(model->base_data.indices, 6);

	vertices[1].position[0] = model->width * 0.5f;
	vertices[1].position[1] = 0;
	vertices[1].position[2] = model->depth * 0.5f;
	CalculateNormalVector(vertices[1].position, vertices[2].position, vertices[0].position, vertices[1].normal);
	StructArrayAppend(model->base_data.vertices, &vertices[1]);

	WordArrayAppend(model->base_data.indices, 6);
	WordArrayAppend(model->base_data.indices, 7);
	WordArrayAppend(model->base_data.indices, 5);

	WordArrayAppend(model->base_data.indices, 6);
	WordArrayAppend(model->base_data.indices, 2);
	WordArrayAppend(model->base_data.indices, 0);

	WordArrayAppend(model->base_data.indices, 6);
	WordArrayAppend(model->base_data.indices, 4);
	WordArrayAppend(model->base_data.indices, 2);

	WordArrayAppend(model->base_data.indices, 1);
	WordArrayAppend(model->base_data.indices, 3);
	WordArrayAppend(model->base_data.indices, 5);

	WordArrayAppend(model->base_data.indices, 1);
	WordArrayAppend(model->base_data.indices, 5);
	WordArrayAppend(model->base_data.indices, 7);

	WordArrayAppend(model->base_data.indices, 0);
	WordArrayAppend(model->base_data.indices, 1);
	WordArrayAppend(model->base_data.indices, 6);

	WordArrayAppend(model->base_data.indices, 7);
	WordArrayAppend(model->base_data.indices, 6);
	WordArrayAppend(model->base_data.indices, 1);

	model->base_data.num_model_indices = (uint16)model->base_data.indices->num_data;

	if(model->base_data.num_lines > 0)
	{
		WordArrayAppend(model->base_data.indices, 0);
		WordArrayAppend(model->base_data.indices, 1);

		WordArrayAppend(model->base_data.indices, 0);
		WordArrayAppend(model->base_data.indices, 2);

		WordArrayAppend(model->base_data.indices, 1);
		WordArrayAppend(model->base_data.indices, 3);

		WordArrayAppend(model->base_data.indices, 2);
		WordArrayAppend(model->base_data.indices, 3);

		WordArrayAppend(model->base_data.indices, 2);
		WordArrayAppend(model->base_data.indices, 4);

		WordArrayAppend(model->base_data.indices, 3);
		WordArrayAppend(model->base_data.indices, 5);

		WordArrayAppend(model->base_data.indices, 4);
		WordArrayAppend(model->base_data.indices, 5);

		WordArrayAppend(model->base_data.indices, 4);
		WordArrayAppend(model->base_data.indices, 6);

		WordArrayAppend(model->base_data.indices, 5);
		WordArrayAppend(model->base_data.indices, 7);

		WordArrayAppend(model->base_data.indices, 6);
		WordArrayAppend(model->base_data.indices, 7);

		WordArrayAppend(model->base_data.indices, 0);
		WordArrayAppend(model->base_data.indices, 6);

		WordArrayAppend(model->base_data.indices, 1);
		WordArrayAppend(model->base_data.indices, 7);
	}

	model->base_data.num_edge_indices = (uint16)model->base_data.indices->num_data - model->base_data.num_model_indices;

	if(model->base_data.num_lines > 1)
	{
		VECTOR3 divide;
		float x, y, z;
		uint16 indices[4];

		divide[0] = model->width / (model->base_data.num_lines);
		divide[1] = model->height / (model->base_data.num_lines);
		divide[2] = model->depth / (model->base_data.num_lines);

		for(i=0, x = - model->width * 0.5f + divide[0]; i<model->base_data.num_lines-1; i++, x += divide[0])
		{
			vertices[0].position[0] = x;
			vertices[0].position[1] = 0;
			vertices[0].position[2] = - model->depth * 0.5f;

			vertices[1].position[0] = x;
			vertices[1].position[1] = model->height;
			vertices[1].position[2] = - model->depth * 0.5f;

			vertices[2].position[0] = x;
			vertices[2].position[1] = model->height;
			vertices[2].position[2] = model->depth * 0.5f;

			indices[0] = (uint16)model->base_data.vertices->num_data;
			CalculateNormalVector(vertices[0].position, vertices[1].position, vertices[2].position, vertices[0].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[0]);

			indices[1] = (uint16)model->base_data.vertices->num_data;
			CalculateNormalVector(vertices[1].position, vertices[2].position, vertices[0].position, vertices[1].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[1]);

			indices[2] = (uint16)model->base_data.vertices->num_data;
			CalculateNormalVector(vertices[2].position, vertices[0].position, vertices[1].position, vertices[2].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[2]);

			indices[3] = (uint16)model->base_data.vertices->num_data;
			vertices[0].position[0] = x;
			vertices[0].position[1] = 0;
			vertices[0].position[2] = model->depth * 0.5f;
			CalculateNormalVector(vertices[0].position, vertices[1].position, vertices[2].position, vertices[0].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[0]);

			WordArrayAppend(model->base_data.indices, indices[0]);
			WordArrayAppend(model->base_data.indices, indices[1]);

			WordArrayAppend(model->base_data.indices, indices[1]);
			WordArrayAppend(model->base_data.indices, indices[2]);

			WordArrayAppend(model->base_data.indices, indices[2]);
			WordArrayAppend(model->base_data.indices, indices[3]);

			WordArrayAppend(model->base_data.indices, indices[3]);
			WordArrayAppend(model->base_data.indices, indices[0]);
		}

		for(i=0, y = divide[1]; i<model->base_data.num_lines-1; i++, y += divide[1])
		{
			vertices[0].position[0] = - model->width * 0.5f;
			vertices[0].position[1] = y;
			vertices[0].position[2] = - model->depth * 0.5f;

			vertices[1].position[0] = model->width * 0.5f;
			vertices[1].position[1] = y;
			vertices[1].position[2] = - model->depth * 0.5f;

			vertices[2].position[0] = model->width * 0.5f;
			vertices[2].position[1] = y;
			vertices[2].position[2] = model->depth * 0.5f;

			indices[0] = (uint16)model->base_data.vertices->num_data;
			CalculateNormalVector(vertices[0].position, vertices[1].position, vertices[2].position, vertices[0].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[0]);

			indices[1] = (uint16)model->base_data.vertices->num_data;
			CalculateNormalVector(vertices[1].position, vertices[2].position, vertices[0].position, vertices[1].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[1]);

			indices[2] = (uint16)model->base_data.vertices->num_data;
			CalculateNormalVector(vertices[2].position, vertices[0].position, vertices[1].position, vertices[2].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[2]);

			indices[3] = (uint16)model->base_data.vertices->num_data;
			vertices[0].position[0] = - model->width * 0.5f;
			vertices[0].position[1] = y;
			vertices[0].position[2] = model->depth * 0.5f;
			CalculateNormalVector(vertices[0].position, vertices[1].position, vertices[2].position, vertices[0].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[0]);

			WordArrayAppend(model->base_data.indices, indices[0]);
			WordArrayAppend(model->base_data.indices, indices[1]);

			WordArrayAppend(model->base_data.indices, indices[1]);
			WordArrayAppend(model->base_data.indices, indices[2]);

			WordArrayAppend(model->base_data.indices, indices[2]);
			WordArrayAppend(model->base_data.indices, indices[3]);

			WordArrayAppend(model->base_data.indices, indices[3]);
			WordArrayAppend(model->base_data.indices, indices[0]);
		}

		for(i=0, z = - model->depth * 0.5f + divide[2]; i<model->base_data.num_lines-1; i++, z += divide[2])
		{
			vertices[0].position[0] = - model->width * 0.5f;
			vertices[0].position[1] = 0;
			vertices[0].position[2] = z;

			vertices[1].position[0] = model->width * 0.5f;
			vertices[1].position[1] = 0;
			vertices[1].position[2] = z;

			vertices[2].position[0] = model->width * 0.5f;
			vertices[2].position[1] = model->height;
			vertices[2].position[2] = z;

			indices[0] = (uint16)model->base_data.vertices->num_data;
			CalculateNormalVector(vertices[0].position, vertices[1].position, vertices[2].position, vertices[0].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[0]);

			indices[1] = (uint16)model->base_data.vertices->num_data;
			CalculateNormalVector(vertices[1].position, vertices[2].position, vertices[0].position, vertices[1].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[1]);

			indices[2] = (uint16)model->base_data.vertices->num_data;
			CalculateNormalVector(vertices[2].position, vertices[0].position, vertices[1].position, vertices[2].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[2]);

			indices[3] = (uint16)model->base_data.vertices->num_data;
			vertices[0].position[0] = - model->width * 0.5f;
			vertices[0].position[1] = model->height;
			vertices[0].position[2] = z;
			CalculateNormalVector(vertices[0].position, vertices[1].position, vertices[2].position, vertices[0].normal);
			StructArrayAppend(model->base_data.vertices, &vertices[0]);

			WordArrayAppend(model->base_data.indices, indices[0]);
			WordArrayAppend(model->base_data.indices, indices[1]);

			WordArrayAppend(model->base_data.indices, indices[1]);
			WordArrayAppend(model->base_data.indices, indices[2]);

			WordArrayAppend(model->base_data.indices, indices[2]);
			WordArrayAppend(model->base_data.indices, indices[3]);

			WordArrayAppend(model->base_data.indices, indices[3]);
			WordArrayAppend(model->base_data.indices, indices[0]);
		}
	}

	model->base_data.num_line_indices = (uint16)model->base_data.indices->num_data - model->base_data.num_model_indices
		- model->base_data.num_edge_indices;
}

size_t WriteCuboidModelDataAndState(
	CUBOID_MODEL* model,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
)
{
	size_t write_size;

	write_size = WriteShapeModelDataAndState(
		&model->base_data, dst, write_func, seek_func, tell_func);

	write_size += write_func(&model->width, sizeof(model->width), 1, dst) * sizeof(model->width);
	write_size += write_func(&model->height, sizeof(model->height), 1, dst) * sizeof(model->height);
	write_size += write_func(&model->depth, sizeof(model->depth), 1, dst) * sizeof(model->depth);

	return write_size;
}

void InitializeCuboidModel(
	CUBOID_MODEL* model,
	SCENE* scene
)
{
	(void)memset(model, 0, sizeof(*model));

	InitializeShapeModel(&model->base_data, scene, SHAPE_TYPE_CUBOID);

	model->width = DEFAULT_SHAPE_SIZE;
	model->height = DEFAULT_SHAPE_SIZE;
	model->depth = DEFAULT_SHAPE_SIZE;

	model->base_data.interface_data.perform_update =
		(void (*)(void*, int))CuboidModelPerformUpdate;

	CuboidModelPerformUpdate(model, TRUE);
}

void ConeModelPerformUpdate(
	CONE_MODEL* model,
	int force_sync
)
{
#define NUM_PARTITION 30
	SHAPE_VERTEX vertices[3];
	float step = (float)(M_PI / NUM_PARTITION);
	float radian;
	float next_radian;
	int upper_edge_count = 0;
	int lower_edge_count = 0;
	int i;

	if((model->base_data.flags & SHAPE_MODEL_FLAG_UPDATE) == 0)
	{
		return;
	}

	ShapeModelPerformUpdate(&model->base_data, force_sync);

	model->base_data.vertices->num_data = 0;
	model->base_data.indices->num_data = 0;

	model->base_data.num_model_indices = 0;
	model->base_data.num_edge_indices = 0;
	model->base_data.num_line_indices = 0;

	vertices[0].position[0] = 0;
	vertices[0].position[1] = model->height;
	vertices[0].position[2] = 0;
	vertices[0].normal[0] = 0;
	vertices[0].normal[1] = 1;
	vertices[0].normal[2] = 0;
	StructArrayAppend(model->base_data.vertices, &vertices[0]);
	upper_edge_count++;

	vertices[1].position[0] = model->upper_radius;
	vertices[1].position[1] = model->height;
	vertices[1].position[2] = 0;
	vertices[1].normal[0] = 0;
	vertices[1].normal[1] = 1;
	vertices[1].normal[2] = 0;
	StructArrayAppend(model->base_data.vertices, &vertices[1]);
	upper_edge_count++;

	vertices[2].position[1] = model->height;
	vertices[2].normal[0] = 0;
	vertices[2].normal[1] = 1;
	vertices[2].normal[2] = 0;
	for(i=1, radian=step, next_radian=step+step; radian<(float)(M_PI*2);
		next_radian += step, i++, upper_edge_count++)
	{
		vertices[2].position[0] = model->upper_radius * cosf(radian);
		vertices[2].position[2] = model->upper_radius * sinf(radian);
		StructArrayAppend(model->base_data.vertices, &vertices[2]);
		WordArrayAppend(model->base_data.indices, i+1);
		WordArrayAppend(model->base_data.indices, i);
		WordArrayAppend(model->base_data.indices, 0);
		radian = next_radian;
	}
	WordArrayAppend(model->base_data.indices, 1);
	WordArrayAppend(model->base_data.indices, i);
	WordArrayAppend(model->base_data.indices, 0);

	vertices[0].position[0] = 0;
	vertices[0].position[1] = 0;
	vertices[0].position[2] = 0;
	vertices[0].normal[0] = 0;
	vertices[0].normal[1] = -1;
	vertices[0].normal[2] = 0;
	StructArrayAppend(model->base_data.vertices, &vertices[0]);
	lower_edge_count++;

	vertices[1].position[0] = model->lower_radius;
	vertices[1].position[1] = 0;
	vertices[1].position[2] = 0;
	vertices[1].normal[0] = 0;
	vertices[1].normal[1] = -1;
	vertices[1].normal[2] = 0;
	StructArrayAppend(model->base_data.vertices, &vertices[1]);
	lower_edge_count++;

	vertices[2].position[1] = 0;
	vertices[2].normal[0] = 0;
	vertices[2].normal[1] = -1;
	vertices[2].normal[2] = 0;
	for(i=i+2, radian=step, next_radian=step+step; radian<(float)(M_PI*2);
		next_radian += step, i++, lower_edge_count++)
	{
		vertices[2].position[0] = model->lower_radius * cosf(radian);
		vertices[2].position[2] = model->lower_radius * sinf(radian);
		StructArrayAppend(model->base_data.vertices, &vertices[2]);
		WordArrayAppend(model->base_data.indices, upper_edge_count);
		WordArrayAppend(model->base_data.indices, i);
		WordArrayAppend(model->base_data.indices, i+1);
		radian = next_radian;
	}
	WordArrayAppend(model->base_data.indices, upper_edge_count);
	WordArrayAppend(model->base_data.indices, i);
	WordArrayAppend(model->base_data.indices, upper_edge_count+1);

	for(i=0; i<upper_edge_count-3; i++)
	{
		WordArrayAppend(model->base_data.indices, i+2);
		WordArrayAppend(model->base_data.indices, upper_edge_count+1+i);
		WordArrayAppend(model->base_data.indices, i+1);

		WordArrayAppend(model->base_data.indices, i+2);
		WordArrayAppend(model->base_data.indices, upper_edge_count+1+i+1);
		WordArrayAppend(model->base_data.indices, upper_edge_count+1+i);
	}
	WordArrayAppend(model->base_data.indices, 1);
	WordArrayAppend(model->base_data.indices, upper_edge_count+1+i);
	WordArrayAppend(model->base_data.indices, i+1);

	WordArrayAppend(model->base_data.indices, 1);
	WordArrayAppend(model->base_data.indices, upper_edge_count+1+i+1);
	WordArrayAppend(model->base_data.indices, upper_edge_count+1+i);

	model->base_data.num_model_indices = (int)model->base_data.indices->num_data;

	if(model->base_data.num_lines > 0)
	{
		for(i=1; i<upper_edge_count-2; i++)
		{
			WordArrayAppend(model->base_data.indices, i);
			WordArrayAppend(model->base_data.indices, i+1);
		}
		WordArrayAppend(model->base_data.indices, 1);
		WordArrayAppend(model->base_data.indices, i+1);

		for(i=upper_edge_count+1; i<upper_edge_count+lower_edge_count-2; i++)
		{
			WordArrayAppend(model->base_data.indices, i);
			WordArrayAppend(model->base_data.indices, i+1);
		}
		WordArrayAppend(model->base_data.indices, upper_edge_count+1);
		WordArrayAppend(model->base_data.indices, i+1);

		model->base_data.num_edge_indices = (uint16)model->base_data.indices->num_data - model->base_data.num_model_indices;
	}

	if(model->base_data.num_lines > 1)
	{
		uint16 indices[2];
		uint16 start_index;
		float sin_value, cos_value;
		float height_step = model->height / model->base_data.num_lines;
		float radius;
		float y;

		step = (float)((M_PI*2)/(model->base_data.num_lines-1));

		WordArrayAppend(model->base_data.indices, 1);
		WordArrayAppend(model->base_data.indices, upper_edge_count+1);

		vertices[0].position[1] = model->height;
		vertices[0].normal[1] = 1;
		vertices[1].position[1] = 0;
		vertices[1].normal[1] = -1;
		for(i=0, radian=step; i<model->base_data.num_lines-2; radian += step, i++)
		{
			sin_value = sinf(radian),	cos_value = cosf(radian);
			indices[0] = (uint16)model->base_data.vertices->num_data;
			vertices[0].position[0] = model->upper_radius * cos_value;
			vertices[0].position[2] = model->upper_radius * sin_value;
			StructArrayAppend(model->base_data.vertices, &vertices[0]);
			indices[1] = (uint16)model->base_data.vertices->num_data;
			vertices[1].position[0] = model->lower_radius * cos_value;
			vertices[1].position[2] = model->lower_radius * sin_value;
			StructArrayAppend(model->base_data.vertices, &vertices[1]);

			WordArrayAppend(model->base_data.indices, indices[0]);
			WordArrayAppend(model->base_data.indices, indices[1]);
		}

		step = (float)((M_PI*2) / NUM_PARTITION);
		for(i=0, y=height_step; i<model->base_data.num_lines-1; i++, y += height_step)
		{
			start_index = (uint16)model->base_data.vertices->num_data;
			radius = (1 - (y / model->height)) * model->lower_radius + (y / model->height) * model->upper_radius;
			vertices[0].position[1] = y;
			vertices[0].normal[1] = 0;

			vertices[0].position[0] = radius;
			vertices[0].position[2] = 0;
			vertices[0].normal[0] = 1;
			vertices[0].normal[2] = 0;
			indices[0] = (uint16)model->base_data.vertices->num_data;
			StructArrayAppend(model->base_data.vertices, &vertices[0]);
			for(radian=step; radian<(float)(M_PI*2); radian += step)
			{
				sin_value = sinf(radian),	cos_value = cosf(radian);
				indices[1] = (uint16)model->base_data.vertices->num_data;
				vertices[0].normal[0] = cos_value;
				vertices[0].normal[2] = sin_value;
				vertices[0].position[0] = radius * cos_value;
				vertices[0].position[2] = radius * sin_value;
				StructArrayAppend(model->base_data.vertices, &vertices[0]);
				WordArrayAppend(model->base_data.indices, indices[0]);
				WordArrayAppend(model->base_data.indices, indices[1]);
				indices[0] = indices[1];
			}
			WordArrayAppend(model->base_data.indices, indices[0]);
			WordArrayAppend(model->base_data.indices, start_index);
		}

		model->base_data.num_line_indices = (uint16)model->base_data.indices->num_data - model->base_data.num_model_indices
			- model->base_data.num_edge_indices;
	}
#undef NUM_PARTITION
}

void InitializeConeModel(
	CONE_MODEL* model,
	SCENE* scene
)
{
	(void)memset(model, 0, sizeof(*model));

	InitializeShapeModel(&model->base_data, scene, SHAPE_TYPE_CONE);

	model->base_data.interface_data.perform_update =
		(void (*)(void*, int))ConeModelPerformUpdate;

	model->upper_radius = DEFAULT_SHAPE_SIZE * 0.5f;
	model->lower_radius = DEFAULT_SHAPE_SIZE;
	model->height = DEFAULT_SHAPE_SIZE;

	ConeModelPerformUpdate(model, TRUE);
}

size_t WriteConeModelDataAndState(
	CONE_MODEL* model,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
)
{
	size_t write_size;

	write_size = WriteShapeModelDataAndState(
		&model->base_data, dst, write_func, seek_func, tell_func);

	write_size += write_func(&model->upper_radius, sizeof(model->upper_radius), 1, dst) * sizeof(model->upper_radius);
	write_size += write_func(&model->lower_radius, sizeof(model->lower_radius), 1, dst) * sizeof(model->lower_radius);
	write_size += write_func(&model->height, sizeof(model->height), 1, dst) * sizeof(model->height);

	return write_size;
}


int AddShapeModel(
	APPLICATION* application,
	SHAPE_MODEL* model
)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene = project->scene;
	SHAPE_RENDER_ENGINE *engine;

	engine = (SHAPE_RENDER_ENGINE*)SceneCreateRenderEngine(scene, RENDER_ENGINE_SHAPE,
		&model->interface_data, 0, project);
	if(engine != NULL)
	{
		PointerArrayAppend(scene->models, model);
		PointerArrayAppend(scene->engines, engine);
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

int ReadCuboidModelData(
	CUBOID_MODEL* model,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*)
)
{
	(void)read_func(&model->width, sizeof(model->width), 1, src);
	(void)read_func(&model->height, sizeof(model->height), 1, src);
	(void)read_func(&model->depth, sizeof(model->depth), 1, src);

	return TRUE;
}

int ReadConeModelData(
	CONE_MODEL* model,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*)
)
{
	(void)read_func(&model->upper_radius, sizeof(model->upper_radius), 1, src);
	(void)read_func(&model->lower_radius, sizeof(model->lower_radius), 1, src);
	(void)read_func(&model->height, sizeof(model->height), 1, src);

	return TRUE;
}

int ReadShapeModelDataAndState(
	SHAPE_MODEL* model,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	void* scene
)
{
	// モデルの位置、向き、サイズ
	VECTOR3 position;
	QUATERNION rotation;
	float model_scale;
	// 1バイトデータ読み込み用
	uint8 byte_data;
	// 4バイトデータ読み込み用
	uint32 data32;

	// モデルの位置・向きを読み込む
	(void)read_func(position, sizeof(*model->position), 3, src);
	(void)read_func(rotation, sizeof(*model->rotation), 4, src);
	// モデルのサイズを読み込む
	(void)read_func(&model_scale, sizeof(model_scale), 1, src);

	// 形状のタイプを読み込む
	(void)read_func(&byte_data, sizeof(byte_data), 1, src);

	switch(byte_data)
	{
	case SHAPE_TYPE_CUBOID:
		InitializeCuboidModel((CUBOID_MODEL*)model, (SCENE*)scene);
		break;
	case SHAPE_TYPE_CONE:
		InitializeConeModel((CONE_MODEL*)model, (SCENE*)scene);
		break;
	default:
		return FALSE;
	}

	COPY_VECTOR3(model->position, position);
	COPY_VECTOR4(model->rotation, rotation);
	model->interface_data.scale_factor = model_scale;

	// 面、線、エッジ、塗り潰しの色を読み込む
	(void)read_func(model->surface_color, sizeof(*model->surface_color), 3, src);
	(void)read_func(model->line_color, sizeof(*model->line_color), 3, src);
	(void)read_func(model->edge_color, sizeof(*model->edge_color), 3, src);

	// 線の太さを読み込む
	(void)read_func(&model->line_width, sizeof(model->line_width), 1, src);

	// 線の本数を読み込む
	(void)read_func(&data32, sizeof(data32), 1, src);
	model->num_lines = (int)data32;

	// フラグ情報を読み込む
	(void)read_func(&data32, sizeof(data32), 1, src);
	model->flags = (unsigned int)data32;

	switch(model->shape_type)
	{
	case SHAPE_TYPE_CUBOID:
		return ReadCuboidModelData((CUBOID_MODEL*)model, src, read_func);
	case SHAPE_TYPE_CONE:
		return ReadConeModelData((CONE_MODEL*)model, src, read_func);
	}

	return FALSE;
}

size_t WriteShapeModel(
	SHAPE_MODEL* model,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
)
{
	size_t write_size = 0;
	switch(model->shape_type)
	{
	case SHAPE_TYPE_CUBOID:
		write_size = WriteCuboidModelDataAndState((CUBOID_MODEL*)model,
			dst, write_func, seek_func, tell_func);
		break;
	case SHAPE_TYPE_CONE:
		write_size = WriteConeModelDataAndState((CONE_MODEL*)model,
			dst, write_func, seek_func, tell_func);
		break;
	}

	return write_size;
}
