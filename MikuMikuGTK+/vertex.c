#include <string.h>
#include "vertex.h"
#include "pmx_model.h"
#include "asset_model.h"
#include "model_helper.h"
#include "memory.h"
#include "memory_stream.h"
#include "utils.h"
#include "application.h"

void ResetVertex(void* vertex, int index, void* dummy)
{
	VERTEX_INTERFACE *inter = (VERTEX_INTERFACE*)vertex;

	inter->reset(vertex);
}

#define PMX_VERTEX_MAX_BONES 4
#define PMX_VERTEX_MAX_MORPHS 5

#define PMX_VERTEX_UNIT_SIZE 32

typedef struct _PMX_VERTEX_UNIT
{
	float position[3];
	float normal[3];
	float texture_coord[2];
} PMX_VERTEX_UNIT;

#define ADDITIONAL_UV_UNIT_SIZE 16
#define B_DEF_2_UNIT_SIZE 4
#define B_DEF_4_UNIT_SIZE 16

#define PMX_S_DEF_UNIT_SIZE 40
typedef struct _PMX_S_DEF_UNIT
{
	float weight;
	float c[3];
	float r0[3];
	float r1[3];
} PMX_S_DEF_UNIT;

void PmxVertexSetMaterial(PMX_VERTEX* vertex, PMX_MATERIAL* material)
{
	if(vertex->material != material)
	{
		// ADD_QUEUE_EVENT
		vertex->material = (material != NULL) ? material :
			(PMX_MATERIAL*)&vertex->model->interface_data.scene->project->application_context->default_data.material;
	}
}

static void PmxVertexReset(PMX_VERTEX* vertex)
{
	int i;

	vertex->morph_delta[0] = 0;
	vertex->morph_delta[1] = 0;
	vertex->morph_delta[2] = 0;

	for(i=0; i<PMX_VERTEX_MAX_MORPHS; i++)
	{
		vertex->morph_uvs[i][0] = 0;
		vertex->morph_uvs[i][1] = 0;
		vertex->morph_uvs[i][2] = 0;
		vertex->morph_uvs[i][3] = 0;
	}
}

static void* PmxVertexGetBone(PMX_VERTEX* vertex, int index)
{
	if(index < 0 || index >= PMX_VERTEX_MAX_BONES)
	{
		return NULL;
	}

	return (void*)vertex->bone[index];
}

static void PmxVertexPerformSkinning(PMX_VERTEX* vertex, float* position, float* normal)
{
	float vertex_position[3];

	vertex_position[0] = vertex->origin[0] + vertex->morph_delta[0];
	vertex_position[1] = vertex->origin[1] + vertex->morph_delta[1];
	vertex_position[2] = vertex->origin[2] + vertex->morph_delta[2];

	switch(vertex->interface_data.type)
	{
	case BASE_VERTEX_TYPE_B_DEF1:
		TransformVertex(vertex->bone[0]->interface_data.local_transform,
			vertex_position, vertex->normal, position, normal);
		break;
	case BASE_VERTEX_TYPE_B_DEF2:
	case BASE_VERTEX_TYPE_S_DEF:
		{
			FLOAT_T weight = vertex->weight[0];
			if(FuzzyZero((float)(1 - weight)))
			{
				TransformVertex(vertex->bone[0]->interface_data.local_transform,
					vertex_position, vertex->normal, position, normal);
			}
			else if(FuzzyZero((float)weight))
			{
				TransformVertex(vertex->bone[1]->interface_data.local_transform,
					vertex_position, vertex->normal, position, normal);
			}
			else
			{
				TransformVertexInterpolate(vertex->bone[0]->interface_data.local_transform,
					vertex->bone[1]->interface_data.local_transform, vertex_position, vertex->normal, position, normal, weight);
			}
		}
		break;
	case BASE_VERTEX_TYPE_B_DEF4:
		{
			float basis[9];
			float v1[3];
			float n1[3];
			float v2[3];
			float n2[3];
			float v3[3];
			float n3[3];
			float v4[3];
			float n4[3];
			FLOAT_T s;
			FLOAT_T w1s;
			FLOAT_T w2s;
			FLOAT_T w3s;
			FLOAT_T w4s;

			BtTransformMultiVector3(vertex->bone[0]->interface_data.local_transform, vertex_position, v1);
			BtTransformGetBasis(vertex->bone[0]->interface_data.local_transform, basis);
			COPY_VECTOR3(n1, vertex->normal);
			MulMatrixVector3(n1, basis);
			BtTransformMultiVector3(vertex->bone[1]->interface_data.local_transform, vertex_position, v2);
			BtTransformGetBasis(vertex->bone[1]->interface_data.local_transform, basis);
			COPY_VECTOR3(n2, vertex->normal);
			MulMatrixVector3(n2, basis);
			BtTransformMultiVector3(vertex->bone[2]->interface_data.local_transform, vertex_position, v3);
			BtTransformGetBasis(vertex->bone[2]->interface_data.local_transform, basis);
			COPY_VECTOR3(n3, vertex->normal);
			MulMatrixVector3(n3, basis);
			BtTransformMultiVector3(vertex->bone[3]->interface_data.local_transform, vertex_position, v4);
			BtTransformGetBasis(vertex->bone[3]->interface_data.local_transform, basis);
			COPY_VECTOR3(n4, vertex->normal);
			MulMatrixVector3(n4, basis);

			s = vertex->weight[0] + vertex->weight[1] + vertex->weight[2] + vertex->weight[3];
			w1s = vertex->weight[0] / s;
			w2s = vertex->weight[1] / s;
			w3s = vertex->weight[2] / s;
			w4s = vertex->weight[3] / s;

			ScaleVector3(v1, (float)w1s);
			ScaleVector3(v2, (float)w2s);
			ScaleVector3(v3, (float)w3s);
			ScaleVector3(v4, (float)w4s);
			COPY_VECTOR3(position, v1);
			PlusVector3(position, v2);
			PlusVector3(position, v3);
			PlusVector3(position, v4);

			ScaleVector3(n1, (float)w1s);
			ScaleVector3(n2, (float)w2s);
			ScaleVector3(n3, (float)w3s);
			ScaleVector3(n4, (float)w4s);
			COPY_VECTOR3(normal, n1);
			PlusVector3(normal, n2);
			PlusVector3(normal, n3);
			PlusVector3(normal, n4);
		}
		break;
	}
}

void PmxVertexGetDelta(PMX_VERTEX* vertex, float* delta)
{
	COPY_VECTOR3(delta, vertex->morph_delta);
}

void PmxVertexGetUVs(PMX_VERTEX* vertex, int index, float* uv)
{
	COPY_VECTOR4(uv, vertex->morph_uvs[index]);
}

PMX_MATERIAL* PmxVertexGetMaterial(PMX_VERTEX* vertex)
{
	return vertex->material;
}

void InitializePmxVertex(PMX_VERTEX* vertex, PMX_MODEL* model)
{
	int i;

	(void)memset(vertex, 0, sizeof(*vertex));
	vertex->model = model;
	vertex->interface_data.type = BASE_VERTEX_TYPE_B_DEF1;
	vertex->interface_data.index = -1;

	for(i=0; i<PMX_VERTEX_MAX_BONES; i++)
	{
		vertex->bone[i] = (PMX_BONE*)&model->interface_data.scene->project->application_context->default_data.bone;
		vertex->bone_indices[i] = -1;
	}

	vertex->material = (PMX_MATERIAL*)&model->interface_data.scene->project->application_context->default_data.material;

	vertex->interface_data.set_material =
		(void (*)(void*, void*))PmxVertexSetMaterial;
	vertex->interface_data.reset =
		(void (*)(void*))PmxVertexReset;
	vertex->interface_data.get_bone =
		(void* (*)(void*, int))PmxVertexGetBone;
	vertex->interface_data.get_delta =
		(void (*)(void*, float*))PmxVertexGetDelta;
	vertex->interface_data.get_uv =
		(void (*)(void*, int, float*))PmxVertexGetUVs;
	vertex->interface_data.perform_skinning =
		(void (*)(void*, float*, float*))PmxVertexPerformSkinning;
	vertex->interface_data.get_material =
		(void* (*)(void*))PmxVertexGetMaterial;
}

int PmxVertexPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, rest, 1};
	size_t base_size;
	size_t bone_size;
	int32 num_vertices;
	uint8 type;
	int i;

	(void)MemRead(&num_vertices,
		sizeof(num_vertices), 1, &stream);
	if(stream.data_point > rest)
	{
		return FALSE;
	}
	if(CHECK_BOUND(info->additional_uv_size, 0, PMX_VERTEX_MAX_MORPHS) == FALSE)
	{
		return FALSE;
	}
	info->vertices = &data[stream.data_point];
	base_size = PMX_VERTEX_UNIT_SIZE + info->additional_uv_size;

	for(i=0; i<num_vertices; i++)
	{
		stream.data_point += base_size;
		// ボーンのタイプ
		MemRead(&type, sizeof(type), 1, &stream);
		if(stream.data_point > rest)
		{
			return FALSE;
		}
		if(type == BASE_VERTEX_TYPE_Q_DEF && info->version < 2.1f)
		{
			return FALSE;
		}

		bone_size = 0;
		switch(type)
		{
		case BASE_VERTEX_TYPE_B_DEF1:
			bone_size = info->bone_index_size;
			break;
		case BASE_VERTEX_TYPE_B_DEF2:
			bone_size = info->bone_index_size * 2 + B_DEF_2_UNIT_SIZE;
			break;
		case BASE_VERTEX_TYPE_B_DEF4:
		case BASE_VERTEX_TYPE_Q_DEF:
			bone_size = info->bone_index_size * 4 + B_DEF_4_UNIT_SIZE;
			break;
		case BASE_VERTEX_TYPE_S_DEF:
			bone_size = info->bone_index_size * 2 + PMX_S_DEF_UNIT_SIZE;
			break;
		default:
			return FALSE;
		}
		// エッジ
		bone_size += sizeof(float);
		stream.data_point += bone_size;
		if(stream.data_point > rest)
		{
			return FALSE;
		}
	}

	info->vertices_count = num_vertices;
	*data_size = stream.data_point;
	return TRUE;
}

int PmxVerticesLoad(STRUCT_ARRAY* vertices, STRUCT_ARRAY* bones)
{
	PMX_VERTEX *v = (PMX_VERTEX*)vertices->buffer;
	PMX_BONE *b = (PMX_BONE*)bones->buffer;
	PMX_VERTEX *vertex;
	const int num_vertices = (int)vertices->num_data;
	const int num_bones = (int)bones->num_data;
	int i, j;

	for(i=0; i<num_vertices; i++)
	{
		vertex = &v[i];
		vertex->interface_data.index = i;
		switch(vertex->interface_data.type)
		{
		case BASE_VERTEX_TYPE_B_DEF1:
			{
				int bone_index = vertex->bone_indices[0];
				if(bone_index >= 0)
				{
					if(bone_index >= num_bones)
					{
						return FALSE;
					}
					else
					{
						vertex->bone[0] = &b[bone_index];
					}
				}
				else
				{
					vertex->bone[0] = NULL;
				}
			}
			break;
		case BASE_VERTEX_TYPE_B_DEF2:
		case BASE_VERTEX_TYPE_S_DEF:
			for(j=0; j<2; j++)
			{
				int bone_index = vertex->bone_indices[j];
				if(bone_index >= 0)
				{
					if(bone_index >= num_bones)
					{
						return FALSE;
					}
					else
					{
						vertex->bone[j] = &b[bone_index];
					}
				}
				else
				{
					vertex->bone[j] = NULL;
				}
			}
			break;
		case BASE_VERTEX_TYPE_B_DEF4:
		case BASE_VERTEX_TYPE_Q_DEF:
			for(j=0; j<4; j++)
			{
				int bone_index = vertex->bone_indices[j];
				if(bone_index >= 0)
				{
					if(bone_index >= num_bones)
					{
						return FALSE;
					}
					else
					{
						vertex->bone[j] = &b[bone_index];
					}
				}
				else
				{
					vertex->bone[j] = NULL;
				}
			}
			break;
		default:
			break;
		}
	}

	return TRUE;
}

void PmxVertexRead(
	PMX_VERTEX* vertex,
	uint8* data,
	PMX_DATA_INFO* info,
	size_t* data_size
)
{
	MEMORY_STREAM stream = {data, 0, INT_MAX, 1};
	float uv[4];
	float edge_size;
	int additional_uv_size;
	int i;

	(void)MemRead(vertex->origin, sizeof(vertex->origin), 1, &stream);
	vertex->origin[2] = - vertex->origin[2];
	(void)MemRead(vertex->normal, sizeof(vertex->normal), 1, &stream);
	vertex->normal[2] = - vertex->normal[2];
	(void)MemRead(vertex->tex_coord, sizeof(float), 2, &stream);
	vertex->tex_coord[2] = 0;

	additional_uv_size = (int)info->additional_uv_size;
	vertex->origin_uvs[0][0] = vertex->tex_coord[0];
	vertex->origin_uvs[0][1] = vertex->tex_coord[1];
	vertex->origin_uvs[0][2] = 0;
	vertex->origin_uvs[0][3] = 0;
	for(i=0; i<additional_uv_size; i++)
	{
		(void)MemRead(uv, sizeof(uv), 1, &stream);
		vertex->origin_uvs[i+1][0] = uv[0];
		vertex->origin_uvs[i+1][1] = uv[1];
		vertex->origin_uvs[i+1][2] = uv[2];
		vertex->origin_uvs[i+1][3] = uv[3];
	}
	vertex->interface_data.type = (eBASE_VERTEX_TYPE)data[stream.data_point];
	stream.data_point++;
	switch(vertex->interface_data.type)
	{
	case BASE_VERTEX_TYPE_B_DEF1:
		vertex->bone_indices[0] = GetSignedValue(
			&data[stream.data_point], (int)info->bone_index_size);
		stream.data_point += info->bone_index_size;
		break;
	case BASE_VERTEX_TYPE_B_DEF2:
		{
			float unit;
			for(i=0; i<2; i++)
			{
				vertex->bone_indices[i] = GetSignedValue(
					&data[stream.data_point], (int)info->bone_index_size);
				stream.data_point += info->bone_index_size;
			}
			(void)MemRead(&unit, sizeof(unit), 1, &stream);
			vertex->weight[0] = CLAMPED(unit, 0.0f, 1.0f);
		}
		break;
	case BASE_VERTEX_TYPE_B_DEF4:
	case BASE_VERTEX_TYPE_Q_DEF:
		{
			float unit[4];
			for(i=0; i<4; i++)
			{
				vertex->bone_indices[i] = GetSignedValue(
					&data[stream.data_point], (int)info->bone_index_size);
				stream.data_point += info->bone_index_size;
			}
			(void)MemRead(unit, sizeof(unit), 1, &stream);
			for(i=0; i<4; i++)
			{
				vertex->weight[i] = CLAMPED(unit[i], 0.0f, 1.0f);
			}
		}
		break;
	case BASE_VERTEX_TYPE_S_DEF:
		{
			PMX_S_DEF_UNIT unit;
			for(i=0; i<2; i++)
			{
				vertex->bone_indices[i] = GetSignedValue(
					&data[stream.data_point], (int)info->bone_index_size);
				stream.data_point += info->bone_index_size;
			}

			(void)MemRead(&unit.weight, sizeof(unit.weight), 1, &stream);
			(void)MemRead(unit.c, sizeof(unit.c), 1, &stream);
			(void)MemRead(unit.r0, sizeof(unit.r0), 1, &stream);
			(void)MemRead(unit.r1, sizeof(unit.r1), 1, &stream);
			
			vertex->c[0] = unit.c[0];
			vertex->c[1] = unit.c[1];
			vertex->c[2] = unit.c[2];

			vertex->r0[0] = unit.r0[0];
			vertex->r0[1] = unit.r0[1];
			vertex->r0[2] = unit.r0[2];
			
			vertex->r1[0] = unit.r1[0];
			vertex->r1[1] = unit.r1[1];
			vertex->r1[2] = unit.r1[2];

			vertex->weight[0] = CLAMPED(unit.weight, 0.0f, 1.0f);
		}
		break;
	default:
		return;
	}

	(void)MemRead(&edge_size, sizeof(edge_size), 1, &stream);
	vertex->interface_data.edge_size = edge_size;
	*data_size = stream.data_point;
}

void PmxVertexMergeMorphByUV(PMX_VERTEX* vertex, MORPH_UV* morph, FLOAT_T weight)
{
	int offset = morph->offset;
	if(CHECK_BOUND(offset, 0, PMX_VERTEX_MAX_MORPHS))
	{
		float m[4];
		float o[4];
		float v[4];

		COPY_VECTOR4(m, morph->position);
		COPY_VECTOR4(o, vertex->morph_uvs[offset]);

		v[0] = (float)(o[0] + m[0] * weight);
		v[1] = (float)(o[1] + m[1] * weight);
		v[2] = (float)(o[2] + m[2] * weight);
		v[3] = (float)(o[3] + m[3] * weight);

		COPY_VECTOR4(vertex->morph_uvs[offset], v);
	}
}

void PmxVertexMergeMorphByVertex(PMX_VERTEX* vertex, MORPH_VERTEX* morph, FLOAT_T weight)
{
	vertex->morph_delta[0] += (float)(morph->position[0] * weight);
	vertex->morph_delta[1] += (float)(morph->position[1] * weight);
	vertex->morph_delta[2] += (float)(morph->position[2] * weight);
}

void InitializeAssetVertex(
	ASSET_VERTEX* vertex,
	ASSET_MODEL* model,
	const float origin[3],
	const float normal[3],
	const float tex_coord[3],
	int index
)
{
	(void)memset(vertex, 0, sizeof(*vertex));
	vertex->model = model;
	COPY_VECTOR3(vertex->origin, origin);
	COPY_VECTOR3(vertex->normal, normal);
	COPY_VECTOR3(vertex->tex_coord, tex_coord);
	vertex->interface_data.index = index;
}

static GLuint Type2Target(eVERTEX_BUNDLE_BUFFER_TYPE type)
{
	switch(type)
	{
	case VERTEX_BUNDLE_VERTEX_BUFFER:
		return GL_ARRAY_BUFFER;
	case VERTEX_BUNDLE_INDEX_BUFFER:
		return GL_ELEMENT_ARRAY_BUFFER;
	}

	return 0;
}

static GLuint InternalGenerate(GLenum target, GLenum usage, const void *pointer, size_t size)
{
	GLuint name;
	glGenBuffers(1, &name);
	glBindBuffer(target, name);
	glBufferData(target, size, pointer, usage);
	glBindBuffer(target, 0);
	return name;
}

VERTEX_BUNDLE* VertexBundleNew(void)
{
	VERTEX_BUNDLE *ret;

	ret = (VERTEX_BUNDLE*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->vertex_buffers = ght_create(DEFAULT_BUFFER_SIZE);
	if(CheckHasExtension("glMapBufferRange"))
	{
		ret->has_map_buffer_range = TRUE;
	}

	return ret;
}

void DeleteVertexBundle(VERTEX_BUNDLE** bundle)
{
	if(bundle == NULL || *bundle == NULL)
	{
		return;
	}

	ght_finalize((*bundle)->vertex_buffers);

	MEM_FREE_FUNC(*bundle);
	*bundle = NULL;
}

void MakeVertexBundle(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	GLuint key,
	GLenum usage,
	const void *pointer,
	size_t size
)
{
	VertexBundleRelease(bundle, type, key);
	switch(type)
	{
	case VERTEX_BUNDLE_VERTEX_BUFFER:
		{
			GLenum target = Type2Target(type);
			(void)ght_insert(
				bundle->vertex_buffers,
				(void*)InternalGenerate(target, usage, pointer, size),
				sizeof(key),
				(void*)&key
			);
		}
		break;
	case VERTEX_BUNDLE_INDEX_BUFFER:
		bundle->index_buffer = InternalGenerate(Type2Target(type), usage, pointer, size);
		break;
	}
}

void VertexBundleRelease(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	GLuint key
)
{
	switch(type)
	{
	case VERTEX_BUNDLE_VERTEX_BUFFER:
		{
			GLuint *buffer = (GLuint*)ght_get(bundle->vertex_buffers, sizeof(key), (const void*)&key);
			if(buffer != NULL)
			{
				glDeleteBuffers(1, buffer);
				ght_remove(bundle->vertex_buffers, sizeof(key), (const void*)key);
			}
		}
		break;
	case VERTEX_BUNDLE_INDEX_BUFFER:
		if(bundle->index_buffer != 0)
		{
			glDeleteBuffers(1, &bundle->index_buffer);
			bundle->index_buffer = 0;
		}
		break;
	}
}

void VertexBundleBind(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	GLuint key
)
{
	switch(type)
	{
	case VERTEX_BUNDLE_VERTEX_BUFFER:
		{
			GLuint buffer;
			buffer = (GLuint)ght_get(bundle->vertex_buffers, sizeof(GLuint), (const void*)&key);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
		}
		break;
	case VERTEX_BUNDLE_INDEX_BUFFER:
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bundle->index_buffer);
		break;
	}
}

void VertexBundleUnbind(
	eVERTEX_BUNDLE_BUFFER_TYPE type
)
{
	GLuint target = Type2Target(type);
	glBindBuffer(target, 0);
}

void* VertexBundleMap(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	size_t offset,
	size_t size
)
{
	GLuint target = Type2Target(type);

	if(bundle->has_map_buffer_range)
	{
		return glMapBufferRange(target, offset, size, GL_MAP_WRITE_BIT);
	}
	return glMapBuffer(target, GL_WRITE_ONLY);
}

void VertexBundleUnmap(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	void* address
)
{
	GLuint target = Type2Target(type);
	glUnmapBuffer(target);
}

void VertexBundleAllocate(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	GLuint usage,
	size_t size,
	const void *data
)
{
	GLuint target = Type2Target(type);
	glBufferData(target, size, data, usage);
}

void VertexBundleWrite(
	VERTEX_BUNDLE* bundle,
	eVERTEX_BUNDLE_BUFFER_TYPE type,
	size_t offset,
	size_t size,
	void* data
)
{
	GLuint target = Type2Target(type);
	glBufferSubData(target, offset, size, data);
}

VERTEX_BUNDLE_LAYOUT* VertexBundleLayoutNew(void)
{
	static const char *vertex_array_object_extension_candidates[] = {
		"ARB_vertex_array_object",
		"OES_vertex_array_object",
		"APPLE_vertex_array_object",
		0
	};
	VERTEX_BUNDLE_LAYOUT *ret;

	ret = (VERTEX_BUNDLE_LAYOUT*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	if(CheckHasAnyExtension(vertex_array_object_extension_candidates) != FALSE)
	{
		ret->has_extension = TRUE;
	}

	return ret;
}

void DeleteVertexBundleLayout(VERTEX_BUNDLE_LAYOUT** layout)
{
	if(layout == NULL || *layout == NULL)
	{
		return;
	}

	MEM_FREE_FUNC(*layout);

	*layout = NULL;
}

int MakeVertexBundleLayout(VERTEX_BUNDLE_LAYOUT* layout)
{
	if(layout->has_extension != FALSE)
	{
		glGenVertexArrays(1, &layout->name);
		return layout->name != 0;
	}

	return FALSE;
}

int VertexBundleLayoutBind(
	VERTEX_BUNDLE_LAYOUT* layout
)
{
	if(layout->has_extension != FALSE && layout->name != 0)
	{
		glBindVertexArray(layout->name);
		return TRUE;
	}

	return FALSE;
}

int VertexBundleLayoutUnbind(
	VERTEX_BUNDLE_LAYOUT* layout
)
{
	if(layout->has_extension != 0)
	{
		glBindVertexArray(0);
		return 1;
	}
	return 0;
}
