#include <string.h>
#include <limits.h>
#include "morph.h"
#include "pmx_model.h"
#include "asset_model.h"
#include "memory_stream.h"
#include "application.h"
#include "memory.h"

void ReleaseMorphInterface(MORPH_INTERFACE* morph)
{
	MEM_FREE_FUNC(morph->name);
	MEM_FREE_FUNC(morph->english_name);
}

#define PMX_MORPH_UNIT_SIZE 6
typedef struct _PMX_MORPH_UNIT
{
	uint8 category;
	uint8 type;
	int32 size;
} PMX_MORPH_UNIT;

#define PMX_MORPH_VERTEX_SIZE 12
#define PMX_MORPH_UV_SIZE 16

#define PMX_MORPH_BONE_SIZE 28
typedef struct _PMX_MORPH_BONE
{
	float position[3];
	float rotation[4];
} PMX_MORPH_BONE;

#define PMX_MORPH_MATERIAL_SIZE 113
typedef struct _PMX_MORPH_MATERIAL
{
	uint8 operation;
	float diffuse[4];
	float specular[3];
	float shininess;
	float ambient[3];
	float edge_color[4];
	float edge_size;
	float texture_weight[4];
	float sphere_texture_weight[4];
	float toon_texture_weight[4];
} PMX_MORPH_MATERIAL;

#define PMX_MORPH_GROUP_SIZE 4
#define PMX_MORPH_FLIP_SIZE 4

#define PMX_MORPH_IMPULSE_SIZE 25
typedef struct _PMX_MORPH_IMPULSE
{
	uint8 is_local;
	float velocity[3];
	float torque[3];
} PMX_MORPH_IMPULSE;

void PmxMorphSetWegiht(PMX_MORPH* morph, FLOAT_T weight)
{
	if(morph->weight != weight)
	{
		// ADD_QUEUE_EVENT
		morph->weight = weight;
	}
	morph->flags |= PMX_MORPH_FLAG_DIRTY;
}

void InitializePmxMorph(PMX_MORPH* morph, PMX_MODEL* model)
{
	(void)memset(morph, 0, sizeof(*morph));
	morph->vertices = StructArrayNew(sizeof(MORPH_VERTEX), MORPH_BUFFER_SIZE);
	morph->uvs = StructArrayNew(sizeof(MORPH_UV), MORPH_BUFFER_SIZE);
	morph->bones = StructArrayNew(sizeof(MORPH_BONE), MORPH_BUFFER_SIZE);
	morph->materials = StructArrayNew(sizeof(MORPH_MATERIAL), MORPH_BUFFER_SIZE);
	morph->groups = StructArrayNew(sizeof(MORPH_GROUP), MORPH_BUFFER_SIZE);
	morph->flips = StructArrayNew(sizeof(MORPH_FLIP), MORPH_BUFFER_SIZE);
	morph->impulses = StructArrayNew(sizeof(MORPH_IMPULSE), MORPH_BUFFER_SIZE);
	morph->parent_model = model;
	morph->type = MORPH_TYPE_UNKNOWN;
	morph->index = -1;
	morph->interface_data.set_weight =
		(void (*)(void*, FLOAT_T))PmxMorphSetWegiht;
}

int PmxMorphPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	MODEL_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, rest, 1};
	PMX_MORPH_UNIT unit;
	int32 num_morphs;
	char *name_ptr;
	int length;
	int num_morphs_in_morph;
	size_t extra_size;
	int i, j;

	if(MemRead(&num_morphs, sizeof(num_morphs), 1, &stream) == 0)
	{
		return FALSE;
	}
	info->morphs = &data[stream.data_point];

	for(i=0; i<num_morphs; i++)
	{
		// “ú–{Œê–¼
		if((length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;

		// ‰pŒê–¼
		if((length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;

		unit.category = data[stream.data_point];
		stream.data_point++;
		unit.type = data[stream.data_point];
		stream.data_point++;
		(void)MemRead(&unit.size, sizeof(unit.size), 1, &stream);

		num_morphs_in_morph = unit.size;
		switch(unit.type)
		{
		case MORPH_TYPE_GROUP:
			extra_size = info->morph_index_size + PMX_MORPH_GROUP_SIZE;
			break;
		case MORPH_TYPE_VERTEX:
			extra_size = info->vertex_index_size + PMX_MORPH_VERTEX_SIZE;
			break;
		case MORPH_TYPE_BONE:
			extra_size = info->bone_index_size + PMX_MORPH_BONE_SIZE;
			break;
		case MORPH_TYPE_TEXTURE_COORD:
		case MORPH_TYPE_UVA1:
		case MORPH_TYPE_UVA2:
		case MORPH_TYPE_UVA3:
		case MORPH_TYPE_UVA4:
			extra_size = info->vertex_index_size + PMX_MORPH_UV_SIZE;
			break;
		case MORPH_TYPE_MATERIAL:
			extra_size = info->material_index_size + PMX_MORPH_MATERIAL_SIZE;
			break;
		case MORPH_TYPE_FLIP:
			if(info->version < 2.1f)
			{
				return FALSE;
			}
			extra_size = info->morph_index_size + PMX_MORPH_FLIP_SIZE;
			break;
		case MORPH_TYPE_IMPULSE:
			if(info->version < 2.1f)
			{
				return FALSE;
			}
			extra_size = info->rigid_body_index_size + PMX_MORPH_IMPULSE_SIZE;
			break;
		default:
			return FALSE;
		}

		for(j=0; j<num_morphs_in_morph; j++)
		{
			if(MemSeek(&stream, (long)extra_size, SEEK_CUR) != 0)
			{
				return FALSE;
			}
		}
	}

	info->morphs_count = num_morphs;
	*data_size = stream.data_point;

	return TRUE;
}

static int PmxMorphLoadBones(PMX_MORPH* morph, STRUCT_ARRAY* bones)
{
	const int num_morph_bones = (int)morph->bones->num_data;
	const int num_bones = (int)bones->num_data;
	PMX_BONE *b = (PMX_BONE*)bones->buffer;
	int i;

	for(i=0; i<num_morph_bones; i++)
	{
		MORPH_BONE *bone = (MORPH_BONE*)&morph->bones->buffer[morph->bones->data_size * i];
		int bone_index = bone->index;
		if(bone_index >= 0)
		{
			if(bone_index < num_bones)
			{
				bone->bone = (void*)&b[bone_index];
			}
			else
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

static int PmxMorphLoadGroups(PMX_MORPH* morph, STRUCT_ARRAY* morphs)
{
	const int num_morph_groups = (int)morph->groups->num_data;
	const int num_morphs = (int)morphs->num_data;
	int i;

	for(i=0; i<num_morph_groups; i++)
	{
		MORPH_GROUP *group = (MORPH_GROUP*)&morph->groups->buffer[morph->groups->data_size * i];
		int group_index = group->index;
		if(group_index >= 0)
		{
			if(group_index < num_morphs)
			{
				PMX_MORPH *morph = (PMX_MORPH*)&morphs->buffer[morphs->data_size * group_index];
				group->morph = (void*)morph;
				morph->flags |= PMX_MORPH_FLAG_HAS_PARENT;
			}
			else
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

static int PmxMorphLoadMaterials(PMX_MORPH* morph, STRUCT_ARRAY* materials)
{
	const int num_morph_materials = (int)morph->materials->num_data;
	const int num_materials = (int)materials->num_data;
	PMX_MATERIAL *m = (PMX_MATERIAL*)materials->buffer;
	int i;

	for(i=0; i<num_morph_materials; i++)
	{
		MORPH_MATERIAL* material = (MORPH_MATERIAL*)&morph->materials->buffer[morph->materials->data_size * i];
		int material_index = material->index;
		if(material_index >= 0)
		{
			if(material_index < num_materials)
			{
				PointerArrayAppend(material->materials, &m[material_index]);
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			PMX_MATERIAL *target;
			int j;
			for(j=0; j<num_materials; j++)
			{
				target = &m[j];
				PointerArrayAppend(material->materials, target);
			}
		}
	}

	return TRUE;
}

static int PmxMorphLoadUVs(PMX_MORPH* morph, STRUCT_ARRAY* vertices, int offset)
{
	const int num_morph_uvs = (int)morph->uvs->num_data;
	const int num_vertices = (int)vertices->num_data;
	PMX_VERTEX *v = (PMX_VERTEX*)vertices->buffer;
	int i;

	for(i=0; i<num_morph_uvs; i++)
	{
		MORPH_UV *uv = (MORPH_UV*)&morph->uvs->buffer[morph->uvs->data_size * i];
		int vertex_index = uv->index;
		if(vertex_index >= 0)
		{
			if(vertex_index < num_vertices)
			{
				uv->vertex = &v[vertex_index];
				uv->offset = offset;
			}
			else
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

static int PmxMorphLoadVertices(PMX_MORPH* morph, STRUCT_ARRAY* vertices)
{
	const int num_morph_vertices = (int)morph->vertices->num_data;
	const int num_vertices = (int)vertices->num_data;
	PMX_VERTEX *v = (PMX_VERTEX*)vertices->buffer;
	int i;

	for(i=0; i<num_morph_vertices; i++)
	{
		MORPH_VERTEX *vertex = (MORPH_VERTEX*)&morph->vertices->buffer[morph->vertices->data_size * i];
		int vertex_index = vertex->index;
		if(vertex_index >= 0)
		{
			if(vertex_index < num_vertices)
			{
				vertex->vertex = (void*)&v[vertex_index];
			}
			else
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

static int PmxMorphLoadFlips(PMX_MORPH* morph, STRUCT_ARRAY* morphs)
{
	const int num_morph_flips = (int)morph->impulses->num_data;
	const int num_flips = (int)morphs->num_data;
	PMX_MORPH *m = (PMX_MORPH*)morphs->buffer;
	int i;

	for(i=0; i<num_morph_flips; i++)
	{
		MORPH_FLIP *flip = (MORPH_FLIP*)&morph->flips->buffer[morph->flips->data_size * i];
		int flip_index = flip->index;
		if(flip_index >= 0)
		{
			if(flip_index < num_flips)
			{
				flip->morph = (void*)&m[flip_index];
			}
			else
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

static int PmxMorphLoadImpulses(PMX_MORPH* morph, STRUCT_ARRAY* bodies)
{
	const int num_morph_impulses = (int)morph->impulses->num_data;
	const int num_bodies = (int)bodies->num_data;
	PMX_RIGID_BODY *rb = (PMX_RIGID_BODY*)bodies->buffer;
	MORPH_IMPULSE *impulses = (MORPH_IMPULSE*)morph->impulses->buffer;
	int i;

	for(i=0; i<num_morph_impulses; i++)
	{
		MORPH_IMPULSE *impulse = &impulses[i];
		int body_index = impulse->index;

		if(body_index >= 0)
		{
			if(body_index < num_bodies)
			{
				impulse->rigid_body = (void*)&rb[body_index];
			}
			else
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

int PmxMorphLoad(
	STRUCT_ARRAY* morphs,
	STRUCT_ARRAY* bones,
	STRUCT_ARRAY* materials,
	STRUCT_ARRAY* rigid_bodies,
	STRUCT_ARRAY* vertices
)
{
	const int num_morphs = (int)morphs->num_data;
	PMX_MORPH *morph;
	PMX_MORPH *m = (PMX_MORPH*)morphs->buffer;
	int i;

	for(i=0; i<num_morphs; i++)
	{
		morph = &m[i];
		switch(morph->type)
		{
		case MORPH_TYPE_GROUP:
			if(PmxMorphLoadGroups(morph, morphs) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_VERTEX:
			if(PmxMorphLoadVertices(morph, vertices) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_BONE:
			if(PmxMorphLoadBones(morph, bones) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_TEXTURE_COORD:
			if(PmxMorphLoadUVs(morph, vertices, 0) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_UVA1:
			if(PmxMorphLoadUVs(morph, vertices, 1) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_UVA2:
			if(PmxMorphLoadUVs(morph, vertices, 2) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_UVA3:
			if(PmxMorphLoadUVs(morph, vertices, 3) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_UVA4:
			if(PmxMorphLoadUVs(morph, vertices, 4) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_MATERIAL:
			if(PmxMorphLoadMaterials(morph, materials) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_FLIP:
			if(PmxMorphLoadFlips(morph, morphs) == FALSE)
			{
				return FALSE;
			}
			break;
		case MORPH_TYPE_IMPULSE:
			if(PmxMorphLoadImpulses(morph, rigid_bodies) == FALSE)
			{
				return FALSE;
			}
			break;
		default:
			return FALSE;
		}
		morph->index = i;
	}

	return TRUE;
}

void PmxMorphReadBones(
	PMX_MORPH* morph,
	MEMORY_STREAM* stream,
	int count,
	MODEL_DATA_INFO* info
)
{
	PMX_MORPH_BONE m;
	int i;

	for(i=0; i<count; i++)
	{
		MORPH_BONE *bone = (MORPH_BONE*)StructArrayReserve(morph->bones);
		int index = GetSignedValue(&stream->buff_ptr[stream->data_point], (int)info->bone_index_size);
		stream->data_point += info->bone_index_size;
		(void)MemRead(m.position, sizeof(m.position), 1, stream);
		(void)MemRead(m.rotation, sizeof(m.rotation), 1, stream);
		SET_POSITION(bone->position, m.position);
		SET_ROTATION(bone->rotation, m.rotation);
		bone->index = index;
	}
}

void PmxMorphReadGroups(
	PMX_MORPH* morph,
	MEMORY_STREAM* stream,
	int count,
	MODEL_DATA_INFO* info
)
{
	float weight;
	int i;

	for(i=0; i<count; i++)
	{
		MORPH_GROUP *group = (MORPH_GROUP*)StructArrayReserve(morph->groups);
		int morph_index = GetSignedValue(&stream->buff_ptr[stream->data_point], (int)info->morph_index_size);
		stream->data_point += info->morph_index_size;
		(void)MemRead(&weight, sizeof(weight), 1, stream);
		group->fixed_weight = weight;
		group->index = morph_index;
	}
}

void PmxMorphReadMaterials(
	PMX_MORPH* morph,
	MEMORY_STREAM* stream,
	int count,
	MODEL_DATA_INFO* info
)
{
	PMX_MORPH_MATERIAL m;
	int i;

	for(i=0; i<count; i++)
	{
		MORPH_MATERIAL *material = (MORPH_MATERIAL*)StructArrayReserve(morph->materials);
		int material_index = GetSignedValue(&stream->buff_ptr[stream->data_point], (int)info->material_index_size);
		stream->data_point += info->material_index_size;
		m.operation = stream->buff_ptr[stream->data_point];
		stream->data_point++;
		(void)MemRead(m.diffuse, sizeof(m.diffuse), 1, stream);
		(void)MemRead(m.specular, sizeof(m.specular), 1, stream);
		(void)MemRead(&m.shininess, sizeof(m.shininess), 1, stream);
		(void)MemRead(m.ambient, sizeof(m.ambient), 1, stream);
		(void)MemRead(m.edge_color, sizeof(m.edge_color), 1, stream);
		(void)MemRead(&m.edge_size, sizeof(m.edge_size), 1, stream);
		(void)MemRead(m.texture_weight, sizeof(m.texture_weight), 1, stream);
		(void)MemRead(m.sphere_texture_weight, sizeof(m.sphere_texture_weight), 1, stream);
		(void)MemRead(m.toon_texture_weight, sizeof(m.toon_texture_weight), 1, stream);

		material->materials = PointerArrayNew(MORPH_BUFFER_SIZE);
		COPY_VECTOR3(material->ambient, m.ambient);
		COPY_VECTOR4(material->diffuse, m.diffuse);
		COPY_VECTOR4(material->edge_color, m.edge_color);
		material->edge_size = m.edge_size;
		material->index = material_index;
		material->operation = m.operation;
		material->shininess = m.shininess;
		COPY_VECTOR3(material->specular, m.specular);
		COPY_VECTOR4(material->sphere_texture_weight, m.sphere_texture_weight);
		COPY_VECTOR4(material->toon_texture_weight, m.toon_texture_weight);
	}
}

void PmxMorphReadUVs(
	PMX_MORPH* morph,
	MEMORY_STREAM* stream,
	int count,
	int offset,
	MODEL_DATA_INFO* info
)
{
	int i;

	for(i=0; i<count; i++)
	{
		MORPH_UV *uv = (MORPH_UV*)StructArrayReserve(morph->uvs);
		int vertex_index = GetSignedValue(&stream->buff_ptr[stream->data_point], (int)info->vertex_index_size);
		stream->data_point += info->vertex_index_size;
		(void)MemRead(uv->position, sizeof(uv->position), 1, stream);
		uv->index = vertex_index;
		uv->offset = offset;
	}
}

void PmxMorphReadVertices(
	PMX_MORPH* morph,
	MEMORY_STREAM* stream,
	int count,
	MODEL_DATA_INFO* info
)
{
	float position[3];
	int i;

	for(i=0; i<count; i++)
	{
		MORPH_VERTEX *vertex = (MORPH_VERTEX*)StructArrayReserve(morph->vertices);
		int vertex_index = GetSignedValue(&stream->buff_ptr[stream->data_point], (int)info->vertex_index_size);
		stream->data_point += info->vertex_index_size;
		(void)MemRead(position, sizeof(position), 1, stream);
		SET_POSITION(vertex->position, position);
		vertex->index = vertex_index;
	}
}

void PmxMorphReadFlips(
	PMX_MORPH* morph,
	MEMORY_STREAM* stream,
	int count,
	MODEL_DATA_INFO* info
)
{
	int i;

	for(i=0; i<count; i++)
	{
		MORPH_FLIP *flip = (MORPH_FLIP*)StructArrayReserve(morph->flips);
		int morph_index = GetSignedValue(&stream->buff_ptr[stream->data_point], (int)info->morph_index_size);
		stream->data_point += info->morph_index_size;
		(void)MemRead(&flip->fixed_weight, sizeof(flip->fixed_weight), 1, stream);
		flip->index = morph_index;
	}
}

void PmxMorphReadImpulses(
	PMX_MORPH* morph,
	MEMORY_STREAM* stream,
	int count,
	MODEL_DATA_INFO* info
)
{
	PMX_MORPH_IMPULSE imp;
	int i;

	for(i=0; i<count; i++)
	{
		MORPH_IMPULSE *impulse = (MORPH_IMPULSE*)StructArrayReserve(morph->impulses);
		int rigid_body_index = GetSignedValue(&stream->buff_ptr[stream->data_point], (int)info->rigid_body_index_size);
		stream->data_point += info->rigid_body_index_size;
		imp.is_local = stream->buff_ptr[stream->data_point];
		stream->data_point++;
		(void)MemRead(imp.velocity, sizeof(imp.velocity), 1, stream);
		(void)MemRead(imp.torque, sizeof(imp.torque), 1, stream);

		impulse->is_local = imp.is_local;
		impulse->index = rigid_body_index;
		COPY_VECTOR3(impulse->velocity, imp.velocity);
		COPY_VECTOR3(impulse->torque, imp.torque);
	}
}

void PmxMorphRead(
	PMX_MORPH* morph,
	uint8* data,
	size_t* data_size,
	MODEL_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, INT_MAX, 1};
	PMX_MORPH_UNIT unit;
	char* name_ptr;
	int name_size;
	TEXT_ENCODE *encode = info->encoding;

	// “ú–{Œê–¼
	name_size = GetTextFromStream((char*)data, &name_ptr);
	morph->interface_data.name = EncodeText(encode, name_ptr, name_size);
	stream.data_point += sizeof(int32) + name_size;

	// ‰pŒê–¼
	name_size = GetTextFromStream((char*)&data[stream.data_point], &name_ptr);
	morph->interface_data.english_name = EncodeText(encode, name_ptr, name_size);
	stream.data_point += sizeof(int32) + name_size;

	unit.category = data[stream.data_point];
	stream.data_point++;
	unit.type = data[stream.data_point];
	stream.data_point++;
	(void)MemRead(&unit.size, sizeof(unit.size), 1, &stream);

	morph->category = (eMORPH_CATEGORY)unit.category;
	morph->type = (eMORPH_TYPE)unit.type;
	switch(morph->type)
	{
	case MORPH_TYPE_GROUP:
		PmxMorphReadGroups(morph, &stream, unit.size, info);
		break;
	case MORPH_TYPE_VERTEX:
		PmxMorphReadVertices(morph, &stream, unit.size, info);
		break;
	case MORPH_TYPE_BONE:
		PmxMorphReadBones(morph, &stream, unit.size, info);
		break;
	case MORPH_TYPE_TEXTURE_COORD:
	case MORPH_TYPE_UVA1:
	case MORPH_TYPE_UVA2:
	case MORPH_TYPE_UVA3:
	case MORPH_TYPE_UVA4:
		PmxMorphReadUVs(morph, &stream, unit.size, morph->type - MORPH_TYPE_TEXTURE_COORD, info);
		break;
	case MORPH_TYPE_MATERIAL:
		PmxMorphReadMaterials(morph, &stream, unit.size, info);
		break;
	case MORPH_TYPE_FLIP:
		PmxMorphReadFlips(morph, &stream, unit.size, info);
		break;
	case MORPH_TYPE_IMPULSE:
		PmxMorphReadImpulses(morph, &stream, unit.size, info);
		break;
	default:
		break;
	}

	*data_size = stream.data_point;
}

static void ReleaseMorphMaterial(MORPH_MATERIAL* material)
{
	PointerArrayDestroy(&material->materials, NULL);
}

void ReleasePmxMorph(PMX_MORPH* morph)
{
	ReleaseMorphInterface(&morph->interface_data);
	StructArrayDestroy(&morph->vertices, NULL);
	StructArrayDestroy(&morph->uvs, NULL);
	StructArrayDestroy(&morph->bones, NULL);
	StructArrayDestroy(&morph->materials, (void (*)(void*))ReleaseMorphMaterial);
	StructArrayDestroy(&morph->groups, NULL);
	StructArrayDestroy(&morph->flips, NULL);
	StructArrayDestroy(&morph->impulses, NULL);
}

void PmxMorphUpdateVertexMorphs(PMX_MORPH* morph, const FLOAT_T value)
{
	const int num_morphs = (int)morph->vertices->num_data;
	MORPH_VERTEX *vertices = (MORPH_VERTEX*)morph->vertices->buffer;
	int i;

	for(i=0; i<num_morphs; i++)
	{
		MORPH_VERTEX *v = &vertices[i];
		PMX_VERTEX *vertex = (PMX_VERTEX*)v->vertex;
		if(vertex != NULL)
		{
			PmxVertexMergeMorphByVertex(vertex, v, value);
		}
	}
}

void PmxMorphSetInternalWeight(PMX_MORPH* morph, const FLOAT_T weight)
{
	morph->internal_weight = weight;
	morph->flags |= PMX_MORPH_FLAG_DIRTY;
}

void PmxMorphUpdateBoneMorphs(PMX_MORPH* morph, const FLOAT_T value)
{
	const int num_morphs = (int)morph->bones->num_data;
	MORPH_BONE *bones = (MORPH_BONE*)morph->bones->buffer;
	int i;

	for(i=0; i<num_morphs; i++)
	{
		MORPH_BONE *bone = &bones[i];
		PMX_BONE *b = (PMX_BONE*)bone->bone;
		if(b != NULL)
		{
			PmxBoneMergeMorph(b, bone, value);
		}
	}
}

void PmxMorphUpdateUVMorphs(PMX_MORPH* morph, const FLOAT_T value)
{
	const int num_morphs = (int)morph->uvs->num_data;
	MORPH_UV *uvs = (MORPH_UV*)morph->uvs->buffer;
	int i;

	for(i=0; i<num_morphs; i++)
	{
		MORPH_UV *uv = &uvs[i];
		PMX_VERTEX *vertex = (PMX_VERTEX*)uv->vertex;
		if(vertex != NULL)
		{
			PmxVertexMergeMorphByUV(vertex, uv, value);
		}
	}
}

void PmxMorphUpdateMaterialMorphs(PMX_MORPH* morph, const FLOAT_T value)
{
	const int num_morphs = (int)morph->materials->num_data;
	MORPH_MATERIAL *materials = (MORPH_MATERIAL*)morph->materials->buffer;
	PMX_MATERIAL *m;
	int i, j;

	for(i=0; i<num_morphs; i++)
	{
		MORPH_MATERIAL *material = &materials[i];
		const int num_materials = (int)material->materials->num_data;
		for(j=0; j<num_materials; j++)
		{
			m = (PMX_MATERIAL*)material->materials->buffer[j];
			PmxMaterialMergeMorph(m, material, value);
		}
	}
}

void PmxMorphUpdateGroupMorphs(PMX_MORPH* morph, const FLOAT_T value, int flip_only)
{
	const int num_morphs = (int)morph->groups->num_data;
	MORPH_GROUP *groups = (MORPH_GROUP*)morph->groups->buffer;
	int i;

	for(i=0; i<num_morphs; i++)
	{
		MORPH_GROUP *group = &groups[i];
		PMX_MORPH *m = (PMX_MORPH*)group->morph;
		if(m != NULL)
		{
			int is_flip_morph = m->type == MORPH_TYPE_FLIP;
			if(BOOL_COMPARE(is_flip_morph, flip_only))
			{
				if(m != morph)
				{
					PmxMorphSetInternalWeight(m, group->fixed_weight * value);
					PmxMorphUpdate(m);
				}
			}
		}
	}
}

void PmxMorphUpdateFlipMorphs(PMX_MORPH* morph, FLOAT_T value)
{
	const int num_morphs = (int)morph->flips->num_data;
	MORPH_FLIP *flips = (MORPH_FLIP*)morph->flips->buffer;

	if(num_morphs > 0)
	{
		PMX_MORPH *m;
		MORPH_FLIP *flip;
		int index;

		CLAMPED(value, 0, 1);
		index = (int)((num_morphs + 1) * value) -1;
		flip = &flips[index];
		m = (PMX_MORPH*)flip->morph;
		if(m != NULL)
		{
			if(m != morph)
			{
				PmxMorphSetInternalWeight(m, flip->fixed_weight);
				PmxMorphUpdate(m);
			}
		}
	}
}

void PmxMorphUpdateImpulseMorphs(PMX_MORPH* morph, const FLOAT_T value)
{
	const int num_morphs = (int)morph->impulses->num_data;
	MORPH_IMPULSE *impulses = (MORPH_IMPULSE*)morph->impulses->buffer;
	int i;

	for(i=0; i<num_morphs; i++)
	{
		MORPH_IMPULSE *impulse = &impulses[i];
		PMX_RIGID_BODY *body = (PMX_RIGID_BODY*)impulse->rigid_body;
		if(body != NULL)
		{
			PmxRigidBodyMergeMorph(body, impulse, value);
		}
	}
}

void PmxMorphUpdate(PMX_MORPH* morph)
{
	if(morph->type == MORPH_TYPE_VERTEX)
	{
		PmxMorphUpdateVertexMorphs(morph, morph->weight);
	}
	else if(morph->type == MORPH_TYPE_GROUP)
	{
		PmxMorphUpdateGroupMorphs(morph, morph->internal_weight, FALSE);
	}
	else if((morph->flags & PMX_MORPH_FLAG_DIRTY) != 0
		|| (morph->parent_model->parent_scene->flags & SCENE_FLAG_MODEL_CONTROLLED) != 0)
	{
		switch(morph->type)
		{
		case MORPH_TYPE_BONE:
			PmxMorphUpdateBoneMorphs(morph, morph->internal_weight);
			break;
		case MORPH_TYPE_TEXTURE_COORD:
		case MORPH_TYPE_UVA1:
		case MORPH_TYPE_UVA2:
		case MORPH_TYPE_UVA3:
		case MORPH_TYPE_UVA4:
			PmxMorphUpdateUVMorphs(morph, morph->internal_weight);
			break;
		case MORPH_TYPE_MATERIAL:
			PmxMorphUpdateMaterialMorphs(morph, morph->internal_weight);
			break;
		case MORPH_TYPE_IMPULSE:
			PmxMorphUpdateImpulseMorphs(morph, morph->internal_weight);
			break;
		case MORPH_TYPE_FLIP:
			break;
		}

		morph->flags &= ~(PMX_MORPH_FLAG_DIRTY);
	}
}

void PmxMorphSyncWeight(PMX_MORPH* morph)
{
	if((morph->flags & PMX_MORPH_FLAG_DIRTY) != 0
		|| (morph->parent_model->parent_scene->flags & SCENE_FLAG_MODEL_CONTROLLED) != 0)
	{
		switch(morph->type)
		{
		case MORPH_TYPE_GROUP:
			PmxMorphUpdateGroupMorphs(morph, morph->weight, TRUE);
			break;
		case MORPH_TYPE_FLIP:
			PmxMorphUpdateFlipMorphs(morph, morph->weight);
			break;
		case MORPH_TYPE_VERTEX:
		case MORPH_TYPE_BONE:
		case MORPH_TYPE_TEXTURE_COORD:
		case MORPH_TYPE_UVA1:
		case MORPH_TYPE_UVA2:
		case MORPH_TYPE_UVA3:
		case MORPH_TYPE_UVA4:
		case MORPH_TYPE_MATERIAL:
		case MORPH_TYPE_IMPULSE:
		default:
			break;
		}
		PmxMorphSetInternalWeight(morph, morph->weight);
	}
}

void InitializeAssetOpacityMorph(
	ASSET_OPACITY_MORPH* morph,
	ASSET_MODEL* model,
	void* application_context
)
{
	(void)memset(morph, 0, sizeof(morph));
	morph->model = model;
	morph->opacity = model->interface_data.opacity;
	morph->application = (APPLICATION*)application_context;
	morph->interface_data.name = "OpacityMorphAsset";
}
