// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include "pmd_model.h"
#include "material.h"
#include "vmd_motion.h"
#include "application.h"
#include "memory_stream.h"
#include "memory.h"

typedef struct _PMD_HEADER
{
	uint8 signature[3];
	float version;
	uint8 name[PMD_MODEL_NAME_SIZE];
	uint8 comment[PMD_MODEL_COMMEMT_SIZE];
} PMD_HEADER;

typedef struct _PMD_STATE
{
	PMD_MODEL *model;
	STRUCT_ARRAY *positions;
	STRUCT_ARRAY *rotations;
	UINT32_ARRAY *weights;
} PMD_STATE;

#define IK_UNIT_SIZE 10
typedef struct _IK_UNIT
{
	int16 root_bone_id;
	int16 target_bone_id;
	uint8 num_joints;
	uint8 num_iterations;
	float angle;
} IK_UNIT;

static int ComparePmd2Bone(const PMD2_BONE** left, const PMD2_BONE** right)
{
	PMD2_BONE *parent_left = (*left)->parent_bone;
	PMD2_BONE *parent_right = (*right)->parent_bone;
	if(parent_left != NULL && parent_right != NULL)
	{
		return parent_left->interface_data.index - parent_right->interface_data.index;
	}
	else if(parent_left == NULL && parent_right != NULL)
	{
		return 1;
	}
	else if(parent_left != NULL && parent_right == NULL)
	{
		return -1;
	}
	return (*left)->interface_data.index - (*right)->interface_data.index;
}

void InitializePmd2Model(PMD2_MODEL* model, SCENE* scene, const char* model_path)
{
	(void)memset(model, 0, sizeof(*model));
	model->rotation[3] = 1;
	model->vertices = StructArrayNew(sizeof(PMD2_VERTEX), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->indices = Uint32ArrayNew(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->textures = ght_create(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	ght_set_hash(model->textures, (ght_fn_hash_t)GetStringHash);
	model->materials = StructArrayNew(sizeof(PMD2_MATERIAL), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->bones = StructArrayNew(sizeof(PMD2_BONE), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->labels = StructArrayNew(sizeof(PMD2_MODEL_LABEL), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->rigid_bodies = StructArrayNew(sizeof(PMD2_RIGID_BODY), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->joints = StructArrayNew(sizeof(PMD2_JOINT), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->constraints = StructArrayNew(sizeof(IK_UNIT), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->custom_toon_textures = PointerArrayNew(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->sorted_bones = PointerArrayNew(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->name2bone = ght_create(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	ght_set_hash(model->name2bone, (ght_fn_hash_t)GetStringHash);
	model->name2morph = ght_create(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	ght_set_hash(model->name2morph, (ght_fn_hash_t)GetStringHash);
	model->interface_data.opacity = 1;
	model->interface_data.scale_factor = 1;
	model->interface_data.scene = scene;
	model->interface_data.model_path = MEM_STRDUP_FUNC(model_path);
	model->edge_color[3] = 1;
}

int Pmd2IkConstraintsPreparse(
	MEMORY_STREAM_PTR stream,
	MODEL_DATA_INFO* info
)
{
	IK_UNIT unit;
	uint16 size;
	size_t unit_size;
	unsigned int i;
	if(MemRead(&size, sizeof(size), 1, stream) == 0)
	{
		return FALSE;
	}

	for(i=0; i<size; i++)
	{
		if(IK_UNIT_SIZE > stream->data_size - stream->data_point)
		{
			return FALSE;
		}
		(void)MemRead(&unit.root_bone_id, sizeof(unit.root_bone_id), 1, stream);
		(void)MemRead(&unit.target_bone_id, sizeof(unit.target_bone_id), 1, stream);
		(void)MemRead(&unit.num_joints, sizeof(unit.num_joints), 1, stream);
		(void)MemRead(&unit.num_iterations, sizeof(unit.num_iterations), 1, stream);
		(void)MemRead(&unit.angle, sizeof(unit.angle), 1, stream);
		unit_size = IK_UNIT_SIZE + unit.num_joints * sizeof(uint16);
		if(unit_size > stream->data_size - stream->data_point)
		{
			return FALSE;
		}
		(void)MemSeek(stream, (long)unit_size, SEEK_CUR);
	}
	return TRUE;
}

int Pmd2ModelPreparse(
	PMD2_MODEL* model,
	MEMORY_STREAM_PTR stream,
	MODEL_DATA_INFO* info,
	TEXT_ENCODE* encode
)
{
	PMD_HEADER header;
	size_t data_size;
	size_t custom_toon_texture_name_size;
	int32 int32_data;
	uint8 has_english;

	if(MemRead(header.signature, sizeof(header.signature), 1, stream) == 0)
	{
		return FALSE;
	}
	if(MemRead(&header.version, sizeof(header.version), 1, stream) == 0)
	{
		return FALSE;
	}
	info->name = &stream->buff_ptr[stream->data_point];
	if(MemRead(header.name, sizeof(header.name), 1, stream) == 0)
	{
		return FALSE;
	}
	info->comment = &stream->buff_ptr[stream->data_point];
	if(MemRead(header.comment, sizeof(header.comment), 1, stream) == 0)
	{
		return FALSE;
	}
	info->base = stream->buff_ptr;
	info->encoding = encode;
	if(memcmp(header.signature, "Pmd", sizeof(header.signature)) != 0)
	{
		return FALSE;
	}
	if(FuzzyZero(header.version-1.0f) == 0)
	{
		return FALSE;
	}

	// 頂点データ
	if(Pmd2VertexPreparse(stream, &data_size, info) == FALSE)
	{
		return FALSE;
	}
	// インデックスデータ
	if(MemRead(&int32_data, sizeof(int32_data), 1, stream) == 0
		|| int32_data * sizeof(uint16) > stream->data_size - stream->data_point)
	{
		return FALSE;
	}
	info->indices = &stream->buff_ptr[stream->data_point];
	info->indices_count = int32_data;
	// 材質
	if(Pmd2MaterialPreparse(stream, &data_size, info) == FALSE)
	{
		return FALSE;
	}
	// ボーン
	if(Pmd2BonePreparse(stream, info) == FALSE)
	{
		return FALSE;
	}
	// IK
	if(Pmd2IkConstraintsPreparse(stream, info) == FALSE)
	{
		return FALSE;
	}
	// 表情
	if(Pmd2MorphPreparse(stream, &data_size, info) == FALSE)
	{
		return FALSE;
	}
	// ラベル
	if(Pmd2ModelLabelPreparse(stream, info) == FALSE)
	{
		return FALSE;
	}
	if(stream->data_point == stream->data_size)
	{
		return TRUE;
	}

	(void)MemRead(&has_english, sizeof(has_english), 1, stream);
	if(has_english != FALSE)
	{
		size_t bone_name_size = BONE_NAME_SIZE * info->bones_count;
		size_t morph_name_size = PMD_MORPH_NAME_SIZE * info->morphs_count;
		size_t bone_category_name_size = PMD_BONE_CATEGORY_NAME_SIZE * info->bone_category_names_count;
		size_t required = PMD_MODEL_NAME_SIZE + PMD_MODEL_COMMEMT_SIZE
			+ bone_name_size + morph_name_size + bone_category_name_size;
		model->flags |= PMD2_MODEL_FLAG_HAS_ENGLISH;
		if(required > stream->data_size - stream->data_point)
		{
			return FALSE;
		}
		info->english_name = &stream->buff_ptr[stream->data_point];
		(void)MemSeek(stream, PMD_MODEL_NAME_SIZE, SEEK_CUR);
		info->english_comment = &stream->buff_ptr[stream->data_point];
		(void)MemSeek(stream, PMD_MODEL_COMMEMT_SIZE, SEEK_CUR);
		info->english_bone_names = &stream->buff_ptr[stream->data_point];
		(void)MemSeek(stream, (long)bone_name_size, SEEK_CUR);
		info->english_face_names = &stream->buff_ptr[stream->data_point];
		(void)MemSeek(stream, (long)morph_name_size, SEEK_CUR);
		info->english_bone_frames = &stream->buff_ptr[stream->data_point];
		(void)MemSeek(stream, (long)bone_category_name_size, SEEK_CUR);
	}
	// カスタムトゥーンテクスチャ
	custom_toon_texture_name_size = PMD_MODEL_MAX_CUSTOM_TOON_TEXTURES * PMD_MODEL_CUSTOM_TOON_TEXTURE_NAME_SIZE;
	if(custom_toon_texture_name_size > stream->data_size - stream->data_point)
	{
		return FALSE;
	}
	info->custom_toon_texture_names = &stream->buff_ptr[stream->data_point];
	if(stream->data_point == stream->data_size)
	{
		return TRUE;
	}
	// 剛体
	if(Pmd2RigidBodyPreparse(stream, &data_size, info) == FALSE)
	{
		return FALSE;
	}
	// 拘束
	if(Pmd2JointPreparse(stream, info) == FALSE)
	{
		return FALSE;
	}
	info->end = &stream->buff_ptr[stream->data_point];

	return stream->data_point == stream->data_size;
}

void Pmd2ModelParseNamesAndComments(PMD2_MODEL* model, MODEL_DATA_INFO* info)
{
	model->interface_data.name = EncodeText(&model->interface_data.scene->project->application_context->encode,
		(char*)info->name, PMD_MODEL_NAME_SIZE);
	if(info->english_name != NULL)
	{
		model->interface_data.english_name = EncodeText(&model->interface_data.scene->project->application_context->encode,
			(char*)info->english_name, PMD_MODEL_NAME_SIZE);
	}
	model->interface_data.comment = EncodeText(&model->interface_data.scene->project->application_context->encode,
		(char*)info->comment, PMD_MODEL_COMMEMT_SIZE);
	if(info->english_comment != NULL)
	{
		model->interface_data.english_comment = EncodeText(&model->interface_data.scene->project->application_context->encode,
			(char*)info->english_comment, PMD_MODEL_COMMEMT_SIZE);
	}
}

void Pmd2ModelParseVertices(PMD2_MODEL* model, MODEL_DATA_INFO* info)
{
	const int num_vertices = (int)info->vertices_count;
	size_t rest = (size_t)(info->end - info->vertices);
	MEMORY_STREAM stream = {info->vertices, 0, rest, 1};
	size_t data_size;
	int i;
	for(i=0; i<num_vertices; i++)
	{
		PMD2_VERTEX *vertex = (PMD2_VERTEX*)StructArrayReserve(model->vertices);
		InitializePmd2Vertex(vertex, model,
			(MATERIAL_INTERFACE*)&model->interface_data.scene->project->application_context->default_data.material);
		ReadPmd2Vertex(vertex, &stream, info, &data_size);
	}
}

void Pmd2ModelParseIndices(PMD2_MODEL* model, MODEL_DATA_INFO* info)
{
	const int num_indices = (int)info->indices_count;
	size_t rest = (size_t)(info->end - info->indices);
	MEMORY_STREAM stream = {info->indices, 0, rest, 1};
	size_t data_size;
	int i;
	for(i=0; i<num_indices; i++)
	{
		uint16 index = (uint16)GetSignedValue(&stream.buff_ptr[stream.data_point], sizeof(uint16));
		Uint32ArrayAppend(model->indices, index);
		stream.data_point += sizeof(uint16);
	}
}

void Pmd2ModelParseMaterials(PMD2_MODEL* model, MODEL_DATA_INFO* info)
{
	const int num_materials = (int)info->materials_count;
	const int num_indices = (int)model->indices->num_data;
	PMD2_VERTEX *vertices = (PMD2_VERTEX*)model->vertices->buffer;
	PMD2_VERTEX *vertex;
	size_t data_size = (size_t)(info->end - info->materials);
	MEMORY_STREAM stream = {info->materials, 0, data_size, 1};
	MATERIAL_INDEX_RANGE range;
	int index_offset = 0;
	int index_offset_to;
	int i, j;

	StructArrayResize(model->materials, num_materials + 1);
	for(i=0; i<num_materials; i++)
	{
		PMD2_MATERIAL *material = (PMD2_MATERIAL*)StructArrayReserve(model->materials);
		InitializePmd2Material(material, model, model->interface_data.scene->project->application_context);
		ReadPmd2Material(material, &stream, &data_size);
		range = material->index_range;
		index_offset_to = index_offset + range.count;
		range.start = num_indices;
		range.end = 0;
		for(j=index_offset; j<index_offset_to; j++)
		{
			int index = (int)model->indices->buffer[i];
			vertex = &vertices[index];
			vertex->material = (MATERIAL_INTERFACE*)material;
			SET_MIN(range.start, index);
			SET_MAX(range.end, index);
		}
		Pmd2MaterialSetIndexRange(material, &range);
		index_offset = index_offset_to;
	}
}

void Pmd2ModelParseBones(PMD2_MODEL* model, MODEL_DATA_INFO* info)
{
	const int num_bones = (int)info->bones_count;
	int has_english = model->flags & PMD2_MODEL_FLAG_HAS_ENGLISH;
	size_t data_size = (size_t)(info->end - info->bones);
	MEMORY_STREAM stream = {info->bones, 0, data_size, 1};
	MEMORY_STREAM english_stream = {info->english_bone_names, 0,
		(size_t)(info->end - info->english_bone_names), 1};
	int i;

	StructArrayResize(model->bones, num_bones + 1);
	for(i=0; i<num_bones; i++)
	{
		PMD2_BONE *bone = (PMD2_BONE*)StructArrayReserve(model->bones);
		ReadPmd2Bone(bone, &stream, info, &data_size);
		PointerArrayAppend(model->sorted_bones, bone);
		(void)ght_insert(model->name2bone, bone,
			(unsigned int)strlen(bone->interface_data.name), bone->interface_data.name);
		if(has_english != FALSE)
		{
			Pmd2BoneReadEnglishName(bone, &english_stream, i);
			(void)ght_insert(model->name2bone, bone,
				(unsigned int)strlen(bone->interface_data.english_name), bone->interface_data.english_name);
		}
	}
	(void)LoadPmd2Bones(model->bones);
	qsort(model->sorted_bones->buffer, model->sorted_bones->num_data,
		sizeof(*model->sorted_bones->buffer), (int (*)(const void*, const void*))ComparePmd2Bone);
	//performujpdate
}

int LoadPmd2Model(PMD2_MODEL* model, uint8* data, size_t data_size)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	if(Pmd2ModelPreparse(model, &stream, &model->info,
		&model->interface_data.scene->project->application_context->encode) != FALSE)
	{
		TextEncodeSetSource(&model->interface_data.scene->project->application_context->encode, "SHIFT-JIS");
		//
	}

	return FALSE;
}

void Pmd2ModelAddTexture(PMD2_MODEL* model, const char* texture)
{
	if(texture != NULL)
	{
		if(ght_get(model->textures, (unsigned int)strlen(texture), texture) == NULL)
		{
			char *insert = MEM_STRDUP_FUNC(texture);
			ght_insert(model->textures, insert, (unsigned int)strlen(insert), insert);
		}
	}
}
