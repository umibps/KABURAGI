// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <math.h>
#include <zlib.h>
#include "pmd_model.h"
#include "model_helper.h"
#include "rigid_body.h"
#include "material.h"
#include "vmd_motion.h"
#include "application.h"
#include "memory_stream.h"
#include "tbb.h"
#include "memory.h"

#define MIN_ROTATION 0.00001f
#define MIN_ROTATION_SUM 0.002f
#define MAX_CUSTOM_TOON_TEXTURES 10
#define MAX_CUSTOM_TOON_TEXTURE_NAME_SIZE 100

#ifndef M_PI
# define M_PI 3.1415926535897932384626433832795
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

#define IK_UNIT_SIZE 11
typedef struct _IK_UNIT
{
	int16 root_bone_id;
	int16 target_bone_id;
	uint8 num_joints;
	uint16 num_iterations;
	float angle;
} IK_UNIT;

typedef struct _IK_CONSTRAINT
{
	POINTER_ARRAY *joint_bones;
	PMD2_BONE *root_bone;
	PMD2_BONE *effector_bone;
	int num_iterations;
	float angle_limit;
} IK_CONSTRAINT;

typedef struct _RAW_IK_CONSTRAINT
{
	IK_UNIT unit;
	UINT32_ARRAY *joint_bone_indices;
} RAW_IK_CONSTRAINT;

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

static void InitializeRawIkConstraint(RAW_IK_CONSTRAINT* constraint)
{
	constraint->joint_bone_indices = Uint32ArrayNew(PMD_MODEL_DEFAULT_BUFFER_SIZE);
}

static void ReleaseRawIkConstraint(RAW_IK_CONSTRAINT* constraint)
{
	Uint32ArrayDestroy(&constraint->joint_bone_indices);
}

static PMD2_BONE* Pmd2ModelFindBone(PMD2_MODEL* model, const char* name)
{
	if(name != NULL)
	{
		return (PMD2_BONE*)ght_get(model->name2bone, (unsigned int)strlen(name), name);
	}
	return NULL;
}

static PMD2_MORPH* Pmd2ModelFindMorph(PMD2_MODEL* model, const char* name)
{
	if(name != NULL)
	{
		return (PMD2_MORPH*)ght_get(model->name2morph, (unsigned int)strlen(name), name);
	}
	return NULL;
}

static PMD2_BONE** Pmd2ModelGetBones(PMD2_MODEL* model, int* num_bones)
{
	PMD2_BONE *bones = (PMD2_BONE*)model->bones->buffer;
	PMD2_BONE **ret;
	int i;

	*num_bones = (int)model->bones->num_data;
	ret = (PMD2_BONE**)MEM_ALLOC_FUNC(sizeof(*ret)*(*num_bones));
	for(i=0; i<*num_bones; i++)
	{
		ret[i] = &bones[i];
	}
	return ret;
}

static PMD2_MORPH** Pmd2ModelGetMorphs(PMD2_MODEL* model, int* num_morphs)
{
	PMD2_MORPH *morphs = (PMD2_MORPH*)model->morphs->buffer;
	PMD2_MORPH **ret;
	int i;

	*num_morphs = (int)model->morphs->num_data;
	ret = (PMD2_MORPH**)MEM_ALLOC_FUNC(sizeof(*ret)*(*num_morphs));
	for(i=0; i<*num_morphs; i++)
	{
		ret[i] = &morphs[i];
	}
	return ret;
}

static PMD2_MATERIAL** Pmd2ModelGetMaterials(PMD2_MODEL* model, int* num_materials)
{
	PMD2_MATERIAL *materials = (PMD2_MATERIAL*)model->materials->buffer;
	PMD2_MATERIAL **ret;
	int i;

	*num_materials = (int)model->materials->num_data;
	ret = (PMD2_MATERIAL**)MEM_ALLOC_FUNC(sizeof(*ret)*(*num_materials));
	for(i=0; i<*num_materials; i++)
	{
		ret[i] = &materials[i];
	}
	return ret;
}

static PMD2_RIGID_BODY** Pmd2ModelGetRigdiBodies(PMD2_MODEL* model, int* num_rigid_bodies)
{
	PMD2_RIGID_BODY *rigid_bodies = (PMD2_RIGID_BODY*)model->rigid_bodies->buffer;
	PMD2_RIGID_BODY **ret;
	int i;

	*num_rigid_bodies = (int)model->rigid_bodies->num_data;
	ret = (PMD2_RIGID_BODY**)MEM_ALLOC_FUNC(sizeof(*ret)*(*num_rigid_bodies));
	for(i=0; i<*num_rigid_bodies; i++)
	{
		ret[i] = &rigid_bodies[i];
	}
	return ret;
}

static PMD2_MODEL_LABEL** Pmd2ModelGetLabels(PMD2_MODEL* model, int* num_labels)
{
	PMD2_MODEL_LABEL *labels = (PMD2_MODEL_LABEL*)model->labels->buffer;
	PMD2_MODEL_LABEL **ret;
	int i;

	*num_labels = (int)model->labels->num_data;
	ret = (PMD2_MODEL_LABEL**)MEM_ALLOC_FUNC(sizeof(*ret)*(*num_labels));
	for(i=0; i<*num_labels; i++)
	{
		ret[i] = &labels[i];
	}
	return ret;
}

static void Pmd2ModelGetWorldPosition(PMD2_MODEL* model, float* position)
{
	COPY_VECTOR3(position, model->position);
}

static void Pmd2ModelGetWorldOrientation(PMD2_MODEL* model, float* orientation)
{
	COPY_VECTOR4(orientation, model->rotation);
}

static FLOAT_T Pmd2ModelGetEdgeScaelFactor(PMD2_MODEL* model, float* camera_position)
{
	FLOAT_T length = 0;
	if(model->bones->num_data > 1)
	{
		PMD2_BONE *bones = (PMD2_BONE*)model->bones->buffer;
		PMD2_BONE *bone = &bones[1];
		VECTOR3 position;
		BtTransformGetOrigin(bone->world_transform, position);
		position[0] = camera_position[0] - position[0];
		position[1] = camera_position[1] - position[1];
		position[2] = camera_position[2] - position[2];
		length = Length3DVector(position);
	}
	return length;
}

static int Pmd2ModelIsVisible(PMD2_MODEL* model)
{
	return model->flags & PMD2_MODEL_FLAG_VISIBLE;
}

static void Pmd2ModelSetWorldPosition(PMD2_MODEL* model, float* position)
{
	COPY_VECTOR3(model->position, position);
}

static void Pmd2ModelSetWorldOrientation(PMD2_MODEL* model, float* orientation)
{
	COPY_VECTOR4(model->rotation, orientation);
}

static void Pmd2ModelSetVisible(PMD2_MODEL* model, int visible)
{
	if(visible == FALSE && (model->flags & PMD2_MODEL_FLAG_VISIBLE) != 0)
	{
		// ADD_QUEUE_EVENT
		model->flags &= ~(PMD2_MODEL_FLAG_VISIBLE);
	}
	else if(visible != FALSE && (model->flags & PMD2_MODEL_FLAG_VISIBLE) == 0)
	{
		// ADD_QUEUE_EVENT
		model->flags |= PMD2_MODEL_FLAG_VISIBLE;
	}
}

static void Pmd2ModelSetOpacity(PMD2_MODEL* model, float opacity)
{
	if(model->interface_data.opacity != opacity)
	{
		// ADD_QUEUE_EVENT
		model->interface_data.opacity = opacity;
	}
}

static void Pmd2ModelSetAaBb(PMD2_MODEL* model, float* minimum, float* maximum)
{
	if(CompareVector3(model->aa_bb_min, minimum) == 0 || CompareVector3(model->aa_bb_max, maximum) == 0)
	{
		COPY_VECTOR3(model->aa_bb_min, minimum);
		COPY_VECTOR3(model->aa_bb_max, maximum);
	}
}

static void Pmd2ModelSetEnablePhysics(PMD2_MODEL* model, int enable)
{
	if(enable != FALSE)
	{
		model->flags |= PMD2_MODEL_FLAG_ENABLE_PHYSICS;
	}
	else
	{
		model->flags &= ~(PMD2_MODEL_FLAG_ENABLE_PHYSICS);
	}
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
	model->morphs = StructArrayNew(sizeof(PMD2_MORPH), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->labels = StructArrayNew(sizeof(PMD2_MODEL_LABEL), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->rigid_bodies = StructArrayNew(sizeof(PMD2_RIGID_BODY), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->joints = StructArrayNew(sizeof(PMD2_JOINT), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->constraints = StructArrayNew(sizeof(IK_CONSTRAINT), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->raw_ik_constraints = StructArrayNew(sizeof(RAW_IK_CONSTRAINT), PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->custom_toon_textures = PointerArrayNew(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->sorted_bones = PointerArrayNew(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	model->name2bone = ght_create(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	ght_set_hash(model->name2bone, (ght_fn_hash_t)GetStringHash);
	model->name2morph = ght_create(PMD_MODEL_DEFAULT_BUFFER_SIZE);
	ght_set_hash(model->name2morph, (ght_fn_hash_t)GetStringHash);
	model->interface_data.type = MODEL_TYPE_PMD_MODEL;
	model->interface_data.opacity = 1;
	model->interface_data.scale_factor = 1;
	model->interface_data.scene = scene;
	model->interface_data.model_path = MEM_STRDUP_FUNC(model_path);
	model->edge_color[3] = 1;
	model->interface_data.find_bone =
		(void* (*)(void*, const char*))Pmd2ModelFindBone;
	model->interface_data.find_morph =
		(void* (*)(void*, const char*))Pmd2ModelFindMorph;
	model->interface_data.get_index_buffer =
		(void (*)(void*, void**))Pmd2ModelGetDefaultIndexBuffer;
	model->interface_data.get_static_vertex_buffer =
		(void (*)(void*, void**))Pmd2ModelGetDefaultStaticVertexBuffer;
	model->interface_data.get_dynanic_vertex_buffer =
		(void (*)(void*, void**, void*))Pmd2ModelGetDefaultDynamicVertexBuffer;
	model->interface_data.get_bones =
		(void** (*)(void*, int*))Pmd2ModelGetBones;
	model->interface_data.get_morphs =
		(void** (*)(void*, int*))Pmd2ModelGetMorphs;
	model->interface_data.get_materials =
		(void** (*)(void*, int*))Pmd2ModelGetMaterials;
	model->interface_data.get_rigid_bodies =
		(void** (*)(void*, int*))Pmd2ModelGetRigdiBodies;
	model->interface_data.get_labels =
		(void** (*)(void*, int*))Pmd2ModelGetLabels;
	model->interface_data.get_world_position = (void (*)(void*, float*))Pmd2ModelGetWorldPosition;
	model->interface_data.get_world_translation = (void (*)(void*, float*))Pmd2ModelGetWorldPosition;
	model->interface_data.get_world_orientation = (void (*)(void*, float*))Pmd2ModelGetWorldOrientation;
	model->interface_data.get_edge_scale_factor = (FLOAT_T (*)(void*, float*))Pmd2ModelGetEdgeScaelFactor;
	model->interface_data.is_visible = (int (*)(void*))Pmd2ModelIsVisible;
	model->interface_data.set_world_position = (void (*)(void*, float*))Pmd2ModelSetWorldPosition;
	model->interface_data.set_world_orientation = (void (*)(void*, float*))Pmd2ModelSetWorldOrientation;
	model->interface_data.set_visible = (void (*)(void*, int))Pmd2ModelSetVisible;
	model->interface_data.set_opacity = (void (*)(void*, float))Pmd2ModelSetOpacity;
	model->interface_data.set_aa_bb = (void (*)(void*, float*, float*))Pmd2ModelSetAaBb;
	model->interface_data.perform_update = (void (*)(void*, int))Pmd2ModelPerformUpdate;
	model->interface_data.reset_motion_state = (void (*)(void*, void*))Pmd2ModelResetMotionState;
	model->interface_data.set_enable_physics = (void (*)(void*, int))Pmd2ModelSetEnablePhysics;
	model->interface_data.rigid_body_transform_new = (void* (*)(struct _BASE_RIGID_BODY*))Pmd2RigidBodyTrasnsformNew;
}

void InitializePmd2IkConstraint(IK_CONSTRAINT* constraint)
{
	constraint->joint_bones = PointerArrayNew(PMD_MODEL_DEFAULT_BUFFER_SIZE);
}

int Pmd2IkConstraintsPreparse(
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info
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
	info->ik_constraints_count = size;
	info->ik_constraints = &stream->buff_ptr[stream->data_point];

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
		unit_size = unit.num_joints * sizeof(uint16);
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
	PMD_DATA_INFO* info,
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
	(void)MemSeek(stream, int32_data * sizeof(uint16), SEEK_CUR);
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
		size_t morph_name_size = (info->morphs_count > 0) ? PMD_MORPH_NAME_SIZE * (info->morphs_count-1) : 0;
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
	(void)MemSeek(stream, (long)custom_toon_texture_name_size, SEEK_CUR);
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

void Pmd2ModelParseNamesAndComments(PMD2_MODEL* model, PMD_DATA_INFO* info)
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

void Pmd2ModelParseVertices(PMD2_MODEL* model, PMD_DATA_INFO* info)
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

void Pmd2ModelParseIndices(PMD2_MODEL* model, PMD_DATA_INFO* info)
{
	const int num_indices = (int)info->indices_count;
	size_t rest = (size_t)(info->end - info->indices);
	MEMORY_STREAM stream = {info->indices, 0, rest, 1};
	int i;
	for(i=0; i<num_indices; i++)
	{
		uint16 index = (uint16)GetSignedValue(&stream.buff_ptr[stream.data_point], sizeof(uint16));
		Uint32ArrayAppend(model->indices, index);
		stream.data_point += sizeof(uint16);
	}
}

void Pmd2ModelParseMaterials(PMD2_MODEL* model, PMD_DATA_INFO* info)
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
		range = material->interface_data.index_range;
		index_offset_to = index_offset + range.count;
		range.start = num_indices;
		range.end = 0;
		for(j=index_offset; j<index_offset_to; j++)
		{
			int index = (int)model->indices->buffer[i];
			vertex = &vertices[index];
			vertex->interface_data.material = (MATERIAL_INTERFACE*)material;
			SET_MIN(range.start, index);
			SET_MAX(range.end, index);
		}
		Pmd2MaterialSetIndexRange(material, &range);
		index_offset = index_offset_to;
	}
}

void Pmd2ModelParseBones(PMD2_MODEL* model, PMD_DATA_INFO* info)
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
		InitializePmd2Bone(bone, (MODEL_INTERFACE*)model, model->interface_data.scene->project->application_context);
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
	Pmd2ModelPerformUpdate(model, FALSE);
}

void Pmd2ModelParseIKConstraints(PMD2_MODEL* model, PMD_DATA_INFO* info)
{
	const int num_constraints = (int)info->ik_constraints_count;
	size_t data_size = (size_t)(info->end - info->ik_constraints);
	MEMORY_STREAM stream = {info->ik_constraints, 0, data_size, 1};
	int i, j;
	StructArrayResize(model->raw_ik_constraints, num_constraints + 1);
	for(i=0; i<num_constraints; i++)
	{
		RAW_IK_CONSTRAINT *raw_constraint = (RAW_IK_CONSTRAINT*)StructArrayReserve(model->raw_ik_constraints);
		IK_UNIT *unit = &raw_constraint->unit;
		uint8 *ptr;
		(void)MemRead(&unit->root_bone_id, sizeof(unit->root_bone_id), 1, &stream);
		(void)MemRead(&unit->target_bone_id, sizeof(unit->target_bone_id), 1, &stream);
		(void)MemRead(&unit->num_joints, sizeof(unit->num_joints), 1, &stream);
		(void)MemRead(&unit->num_iterations, sizeof(unit->num_iterations), 1, &stream);
		(void)MemRead(&unit->angle, sizeof(unit->angle), 1, &stream);
		InitializeRawIkConstraint(raw_constraint);
		ptr = &stream.buff_ptr[stream.data_point];
		for(j=0; j<unit->num_joints; j++)
		{
			uint32 bone_index = (uint32)GetSignedValue(ptr, sizeof(uint16));
			ptr += sizeof(uint16);
			Uint32ArrayAppend(raw_constraint->joint_bone_indices, bone_index);
		}
		(void)MemSeek(&stream, (long)(sizeof(uint16) * unit->num_joints), SEEK_CUR);
	}
}

void Pmd2ModelParseMorphs(PMD2_MODEL* model, PMD_DATA_INFO* info)
{
	const int num_morphs = (int)info->morphs_count;
	size_t data_size = (size_t)(info->end - info->morphs);
	MEMORY_STREAM stream = {info->morphs, 0, data_size, 1};
	uint8 *english_ptr = info->english_face_names;
	int i;
	StructArrayResize(model->morphs, num_morphs + 1);
	for(i=0; i<num_morphs; i++)
	{
		PMD2_MORPH *morph = (PMD2_MORPH*)StructArrayReserve(model->morphs);
		InitializePmd2Morph(morph, model, model->interface_data.scene->project->application_context);
		ReadPmd2Morph(morph, &stream, &data_size);
		(void)ght_insert(model->name2morph, morph, (unsigned int)strlen(morph->interface_data.name),
			morph->interface_data.name);
		if((model->flags & PMD2_MODEL_FLAG_HAS_ENGLISH) != 0)
		{
			Pmd2MorphReadEnglishName(morph, english_ptr, i);
			(void)ght_insert(model->name2morph, morph, (unsigned int)strlen(morph->interface_data.english_name),
				morph->interface_data.english_name);
		}
	}
}

void Pmd2ModelParseLabels(PMD2_MODEL* model, PMD_DATA_INFO* info)
{
	PMD2_MODEL_LABEL *label;
	size_t data_size = (size_t)(info->end - info->bone_category_names);
	MEMORY_STREAM stream = {info->bone_category_names, 0, data_size, 1};
	int num_items;
	int i;

	label = (PMD2_MODEL_LABEL*)StructArrayReserve(model->labels);
	InitializePmd2ModelLabel(label, model, "Root", PMD2_MODEL_LABEL_TYPE_SPECIAL_BONE_CATEGORY,
		model->interface_data.scene->project->application_context);
	num_items = (int)info->bone_category_names_count;
	for(i=0; i<num_items; i++)
	{
		label = (PMD2_MODEL_LABEL*)StructArrayReserve(model->labels);
		InitializePmd2ModelLabel(label, model, (char*)&stream.buff_ptr[stream.data_point],
			PMD2_MODEL_LABEL_TYPE_BONE_CATEGORY, model->interface_data.scene->project->application_context);
		Pmd2ModelLabelReadEnglishName(label, info->english_bone_frames, i);
		(void)MemSeek(&stream, PMD_BONE_CATEGORY_NAME_SIZE, SEEK_CUR);
	}
	num_items = (int)info->bone_labels_count;
	stream.buff_ptr = info->bone_labels;
	stream.data_point = 0;
	stream.data_size = (size_t)(info->end - info->bone_labels);
	for(i=0; i<num_items; i++)
	{
		if((label = Pmd2ModelLabelSelectCategory(model->labels, &stream.buff_ptr[stream.data_point])) != NULL)
		{
			ReadPmd2ModelLabel(label, &stream, &data_size);
		}
	}
	num_items = (int)info->morph_labels_count;
	stream.buff_ptr = info->morph_labels;
	stream.data_point = 0;
	stream.data_size = (size_t)(info->end - info->morph_labels);
	label = (PMD2_MODEL_LABEL*)StructArrayReserve(model->labels);
	InitializePmd2ModelLabel(label, model, NULL, PMD2_MODEL_LABEL_TYPE_MORPH_CATEGORY,
		model->interface_data.scene->project->application_context);
	for(i=0; i<num_items; i++)
	{
		ReadPmd2ModelLabel(label, &stream, &data_size);
	}
}

void Pmd2ModelParseCustomToonTextures(PMD2_MODEL* model, PMD_DATA_INFO* info)
{
	const char fall_back_toon_texture_name[] = "toon0.bmp";
	size_t data_size = (size_t)(info->end - info->custom_toon_texture_names);
	MEMORY_STREAM stream = {info->custom_toon_texture_names, 0, data_size, 1};
	char *path = MEM_STRDUP_FUNC(fall_back_toon_texture_name);
	int i;

	(void)ght_insert(model->textures, path, (unsigned int)strlen(path), path);
	PointerArrayAppend(model->custom_toon_textures, path);
	for(i=0; i<MAX_CUSTOM_TOON_TEXTURES; i++)
	{
		path = EncodeText(&model->interface_data.scene->project->application_context->encode,
			(char*)&stream.buff_ptr[stream.data_point], MAX_CUSTOM_TOON_TEXTURE_NAME_SIZE);
		(void)ght_insert(model->textures, path, (unsigned int)strlen(path), path);
		PointerArrayAppend(model->custom_toon_textures, path);
		(void)MemSeek(&stream, MAX_CUSTOM_TOON_TEXTURE_NAME_SIZE, SEEK_CUR);
	}
}

void Pmd2ModelParseRigidBodies(PMD2_MODEL* model, PMD_DATA_INFO* info)
{
	const int num_rigid_bodies = (int)info->rigid_bodies_count;
	size_t data_size = (size_t)(info->end - info->rigid_bodies);
	MEMORY_STREAM stream = {info->rigid_bodies, 0, data_size, 1};
	int i;

	for(i=0; i<num_rigid_bodies; i++)
	{
		PMD2_RIGID_BODY *rigid_body = (PMD2_RIGID_BODY*)StructArrayReserve(model->rigid_bodies);
		InitializePmd2RigidBody(rigid_body, model, model->interface_data.scene->project->application_context);
		ReadPmd2RigidBody(rigid_body, &stream, info, &data_size);
	}
}

void Pmd2ModelParseJoints(PMD2_MODEL* model, PMD_DATA_INFO* info)
{
	const int num_joints = (int)info->joints_count; 
	size_t data_size = (size_t)(info->end - info->joints);
	MEMORY_STREAM stream = {info->joints, 0, data_size, 1};
	int i;

	for(i=0; i<num_joints; i++)
	{
		PMD2_JOINT *joint = (PMD2_JOINT*)StructArrayReserve(model->joints);
		InitializePmd2Joint(joint, model);
		ReadPmd2Joint(joint, &stream, &data_size, info);
	}
}

void Pmd2ModelLoadIkConstraints(PMD2_MODEL* model)
{
	RAW_IK_CONSTRAINT *constraints = (RAW_IK_CONSTRAINT*)model->raw_ik_constraints->buffer;
	const int num_constraints = (int)model->raw_ik_constraints->num_data;
	PMD2_BONE *bones = (PMD2_BONE*)model->bones->buffer;
	const int num_bones = (int)model->bones->num_data;
	int i, j;
	for(i=0; i<num_constraints; i++)
	{
		RAW_IK_CONSTRAINT *raw_constraint = &constraints[i];
		IK_UNIT *unit = &raw_constraint->unit;
		int target_index = unit->target_bone_id;
		int root_index = unit->root_bone_id;
		if(CHECK_BOUND(target_index, 0, num_bones)
			&& CHECK_BOUND(root_index, 0, num_bones))
		{
			PMD2_BONE *root_bone = &bones[root_index];
			PMD2_BONE *effector_bone = &bones[target_index];
			IK_CONSTRAINT *constraint = (IK_CONSTRAINT*)StructArrayReserve(model->constraints);
			const int num_joints = (int)raw_constraint->joint_bone_indices->num_data;

			InitializePmd2IkConstraint(constraint);
			for(j=0; j<num_joints; j++)
			{
				int32 bone_index = (int32)raw_constraint->joint_bone_indices->buffer[j];
				if(CHECK_BOUND(bone_index, 0, num_bones))
				{
					PMD2_BONE *joint_bone = &bones[bone_index];
					PointerArrayAppend(constraint->joint_bones, joint_bone);
				}
			}
			constraint->root_bone = root_bone;
			constraint->effector_bone = effector_bone;
			constraint->num_iterations = unit->num_iterations;
			constraint->angle_limit = unit->angle * (float)M_PI;
			root_bone->target_bone = effector_bone;
		}
	}
}

int LoadPmd2Model(PMD2_MODEL* model, uint8* data, size_t data_size)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	if(Pmd2ModelPreparse(model, &stream, &model->info,
		&model->interface_data.scene->project->application_context->encode) != FALSE)
	{
		TextEncodeSetSource(&model->interface_data.scene->project->application_context->encode, "SHIFT-JIS");
		Pmd2ModelParseNamesAndComments(model, &model->info);
		Pmd2ModelParseVertices(model, &model->info);
		Pmd2ModelParseIndices(model, &model->info);
		Pmd2ModelParseMaterials(model, &model->info);
		Pmd2ModelParseBones(model, &model->info);
		Pmd2ModelParseIKConstraints(model, &model->info);
		Pmd2ModelParseMorphs(model, &model->info);
		Pmd2ModelParseLabels(model, &model->info);
		Pmd2ModelParseCustomToonTextures(model, &model->info);
		Pmd2ModelParseRigidBodies(model, &model->info);
		Pmd2ModelParseJoints(model, &model->info);
		Pmd2ModelLoadIkConstraints(model);
		if(LoadPmd2Materials(model->materials, model->custom_toon_textures, (int)model->indices->num_data) == FALSE
			|| LoadPmd2Vertices(model->vertices, model->bones) == FALSE
			|| LoadPmd2Morphs(model->morphs, model->vertices) == FALSE
			|| LoadPmd2ModelLabels(model->labels, model->bones, model->morphs) == FALSE
			|| LoadPmd2RigidBodies(model->rigid_bodies, model->bones) == FALSE
			|| LoadPmd2Joints(model->joints, model->rigid_bodies) == FALSE
		)
		{
			return FALSE;
		}
		return TRUE;
	}

	return FALSE;
}

void ReadPmd2ModelDataAndState(
	void *scene,
	PMD2_MODEL* model,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int)
)
{
	SCENE *scene_ptr = (SCENE*)scene;
	float float_values[4];
	uint32 data32;
	uint32 data_size;
	uint32 section_size;

	(void)read_func(&data_size, sizeof(data_size), 1, src);

	// モデルのデータを読み込む
	{
		uint32 decode_size;
		uint8 *section_data;
		uint8 *decode_data;
		(void)read_func(&section_size, sizeof(section_size), 1, src);
		(void)read_func(&decode_size, sizeof(decode_size), 1, src);
		section_data = (uint8*)MEM_ALLOC_FUNC(section_size);
		decode_data = (uint8*)MEM_ALLOC_FUNC(decode_size);
		(void)read_func(section_data, 1, section_size, src);
		if(InflateData(section_data, decode_data, section_size, decode_size, NULL) != 0)
		{
			return;
		}
		LoadPmd2Model(model, decode_data, decode_size);
		MEM_FREE_FUNC(section_data);
	}

	// モデルの位置・向きを読み込む
	(void)read_func(model->position, sizeof(*model->position), 3, src);
	(void)read_func(model->rotation, sizeof(*model->rotation), 4, src);
	// モデルのサイズを読み込む
	(void)read_func(float_values, sizeof(*float_values), 1, src);
	model->interface_data.scale_factor = float_values[0];

	// テクスチャデータを読み込む
	{
		(void)read_func(&section_size, sizeof(section_size), 1, src);
		model->interface_data.texture_archive_size = section_size;
		model->interface_data.texture_archive = MEM_ALLOC_FUNC(section_size);
		(void)read_func(model->interface_data.texture_archive, 1, section_size, src);

		PointerArrayAppend(scene_ptr->engines, SceneCreateRenderEngine(
			scene_ptr, RENDER_ENGINE_PMX, &model->interface_data, 0, scene_ptr->project));

		Pmd2ModelJoinWorld(model, scene_ptr->project->world.world);
	}

	// ボーンデータの読み込み
	{
		size_t section_size;
		MEMORY_STREAM *stream;

		(void)read_func(&data32, sizeof(data32), 1, src);
		section_size = (size_t)data32;
		stream = CreateMemoryStream(section_size);

		(void)read_func(stream->buff_ptr, 1, section_size, src);
		ReadBoneData(stream, &model->interface_data, model->interface_data.scene->project);

		(void)DeleteMemoryStream(stream);
	}

	// 表情データの読み込み
	{
		size_t section_size;
		MEMORY_STREAM *stream;

		(void)read_func(&data32, sizeof(data32), 1, src);
		section_size = (size_t)data32;
		stream = CreateMemoryStream(section_size);

		(void)read_func(stream->buff_ptr, 1, section_size, src);
		ReadMorphData(stream, &model->interface_data);

		(void)DeleteMemoryStream(stream);
	}

	// 親ボーンの読み込み
	(void)read_func(&data32, sizeof(data32), 1, src);
	if(data32 > 0)
	{
		model->interface_data.parent_model =
			(MODEL_INTERFACE*)MEM_ALLOC_FUNC(data32);
		(void)read_func(model->interface_data.parent_model, 1,
			data32, src);
		(void)read_func(&data32, sizeof(data32), 1, src);
		model->interface_data.parent_bone =
			(BONE_INTERFACE*)MEM_ALLOC_FUNC(data32);
		(void)read_func(model->interface_data.parent_bone, 1, data32, src);
	}
}

size_t WritePmd2ModelDataAndState(
	PMD2_MODEL* model,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
)
{
	// 最後にサイズを書き出すので位置を記憶
	long size_position = tell_func(dst);
	uint32 write_size;
	// ZIP圧縮用
	uint8 *compressed_data = (uint8*)MEM_ALLOC_FUNC((size_t)(model->info.end - model->info.base)*2);
	// 圧縮後のサイズ
	size_t compressed_size;
	// 浮動小数点数書き出し用
	float float_values[4];
	// 4バイト書き出し用
	uint32 data32;

	// データサイズ(4バイト)分をスキップ
	(void)seek_func(dst, sizeof(uint32), SEEK_CUR);

	// モデルのデータを書き出す
		// ZIP圧縮して書き出し
	(void)DeflateData(model->info.base, compressed_data,
		(size_t)(model->info.end - model->info.base),
		(size_t)(model->info.end - model->info.base) * 2,
		&compressed_size,
		Z_DEFAULT_COMPRESSION
	);
	data32 = (uint32)compressed_size;
	(void)write_func(&data32, sizeof(data32), 1, dst);
	data32 = (uint32)(model->info.end - model->info.base);
	(void)write_func(&data32, sizeof(data32), 1, dst);
	(void)write_func(compressed_data, 1, compressed_size, dst);

	MEM_FREE_FUNC(compressed_data);

	// モデルの位置・向きを書き出す
	(void)write_func(model->position, sizeof(*model->position), 3, dst);
	(void)write_func(model->rotation, sizeof(*model->rotation), 4, dst);
	// モデルのサイズを書き出す
	float_values[0] = (float)model->interface_data.scale_factor;
	(void)write_func(float_values, sizeof(*float_values), 1, dst);

	// テクスチャデータの書き出し
	if(model->interface_data.texture_archive_size > 0)
	{
		data32 = (uint32)model->interface_data.texture_archive_size;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(model->interface_data.texture_archive, 1,
			model->interface_data.texture_archive_size, dst);
	}
	else
	{
		size_t write_data_size;
		uint8 *texture_data = WriteMmdModelMaterials(
			&model->interface_data, &write_data_size);
		data32 = (uint32)write_data_size;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(texture_data, 1, write_data_size, dst);
		MEM_FREE_FUNC(texture_data);
	}

	// ボーンデータの書き出し
	{
		size_t write_data_size;
		uint8 *bone_data = WriteBoneData(
			&model->interface_data, &write_data_size);
		data32 = (uint32)write_data_size;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(bone_data, 1, write_data_size, dst);
		MEM_FREE_FUNC(bone_data);
	}

	// 表情データの書き出し
	{
		size_t write_data_size;
		uint8 *morph_data = WriteMorphData(
			&model->interface_data, &write_data_size);
		data32 = (uint32)write_data_size;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(morph_data, 1, write_data_size, dst);
		MEM_FREE_FUNC(morph_data);
	}

	// 親モデル・ボーンの書き出し
	if(model->interface_data.parent_bone != NULL)
	{
		data32 = (uint32)strlen(model->interface_data.parent_model->name) + 1;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(model->interface_data.parent_model->name, 1, data32, dst);
		data32 = (uint32)strlen(model->interface_data.parent_bone->name) + 1;
		(void)write_func(&data32, sizeof(data32), 1, dst);
		(void)write_func(model->interface_data.parent_bone->name, 1, data32, dst);
	}
	else
	{
		data32 = 0;
		(void)write_func(&data32, sizeof(data32), 1, dst);
	}

	data32 = (uint32)tell_func(dst);
	(void)seek_func(dst, size_position, SEEK_SET);
	write_size = data32 - size_position;
	(void)write_func(&write_size, sizeof(write_size), 1, dst);
	(void)seek_func(dst, data32, SEEK_SET);

	return data32;
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

void Pmd2ModelPerformUpdate(PMD2_MODEL* model, int force_sync)
{
	PMD2_MORPH *morphs;
	PMD2_BONE **bones;
	void *processor;
	int num_items;
	int i;

	processor = SimpleParallelProcessorNew((void*)model->vertices->buffer, sizeof(PMD2_VERTEX),
		(int)model->vertices->num_data, (void (*)(void*, int, void*))ResetVertex, NULL);
	ExecuteSimpleParallelProcessor(processor);
	DeleteSimpleParallelProcessor(processor);

	num_items = (int)model->morphs->num_data;
	morphs = (PMD2_MORPH*)model->morphs->buffer;
	for(i=0; i<num_items; i++)
	{
		Pmd2MorphUpdate(&morphs[i]);
	}
	num_items = (int)model->sorted_bones->num_data;
	bones = (PMD2_BONE**)model->sorted_bones->buffer;
	for(i=0; i<num_items; i++)
	{
		Pmd2BonePerformTransform(bones[i]);
	}
	Pmd2ModelSolveInverseKinematics(model);
	if((model->flags & PMD2_MODEL_FLAG_ENABLE_PHYSICS) != 0 || force_sync != FALSE)
	{
		processor = SimpleParallelProcessorNew((void*)model->rigid_bodies->buffer,
			sizeof(PMD2_RIGID_BODY), (int)model->rigid_bodies->num_data, RigidBodySyncLocalTransform, NULL);
		ExecuteSimpleParallelProcessor(processor);
		DeleteSimpleParallelProcessor(processor);
	}
}

void Pmd2ModelSolveInverseKinematics(PMD2_MODEL* model)
{
	IK_CONSTRAINT *constraints = (IK_CONSTRAINT*)model->constraints->buffer;
	IK_CONSTRAINT *constraint;
	const int num_constraints = (int)model->constraints->num_data;
	QUATERNION new_rotation = IDENTITY_QUATERNION;
	float matrix[] = IDENTITY_MATRIX3x3;
	VECTOR3 local_root_bone_position = {0};
	VECTOR3 local_effector_position = {0};
	VECTOR3 rotation_euler = {0};
	VECTOR3 joint_euler = {0};
	VECTOR3 local_axis = {0};
	int i, j, k, l;
	for(i=0; i<num_constraints; i++)
	{
		PMD2_BONE **joint_bones;
		int num_joints;
		int num_iterations;
		PMD2_BONE *effector_bone;
		QUATERNION origin_target_bone_rotation;
		VECTOR3 root_bone_position;
		constraint = &constraints[i];
		effector_bone = constraint->effector_bone;
		COPY_VECTOR4(origin_target_bone_rotation, effector_bone->rotation);
		BtTransformGetOrigin(constraint->root_bone->world_transform, root_bone_position);
		joint_bones = (PMD2_BONE**)constraint->joint_bones->buffer;
		num_iterations = constraint->num_iterations;
		num_joints = (int)constraint->joint_bones->num_data;
		Pmd2BonePerformTransform(effector_bone);
		for(j=0; j<num_iterations; j++)
		{
			for(k=0; k<num_joints; k++)
			{
				PMD2_BONE *joint_bone = joint_bones[k];
				VECTOR3 current_effector_position;
				uint8 joint_bone_transform[TRANSFORM_SIZE];
				float dot;
				float new_angle_limit;
				float angle;
				BtTransformGetOrigin(effector_bone->world_transform, current_effector_position);
				BtTransformSet(joint_bone_transform, joint_bone->world_transform);
				BtTransformInverse(joint_bone_transform);
				BtTransformMultiVector3(joint_bone_transform, root_bone_position, local_root_bone_position);
				SafeNormalize3DVector(local_root_bone_position);
				BtTransformMultiVector3(joint_bone_transform, current_effector_position, local_effector_position);
				SafeNormalize3DVector(local_effector_position);
				dot = Dot3DVector(local_root_bone_position, local_effector_position);
				if(FuzzyZero(dot) != FALSE)
				{
					break;
				}
				new_angle_limit = constraint->angle_limit * (k + 1) * 2;
				if(dot > 1.0f)
				{
					dot = 1.0f;
				}
				else if(dot < -1.0f)
				{
					dot = -1.0f;
				}
				angle = acosf(dot);
				CLAMPED(angle, - new_angle_limit, new_angle_limit);
				Cross3DVector(local_axis, local_effector_position, local_root_bone_position);
				SafeNormalize3DVector(local_axis);
				if(Pmd2BoneIsAxisXAligned(joint_bone) != FALSE)
				{
					QUATERNION local_rotation;
					if(j == 0)
					{
						const float axis_x[] = {1, 0, 0};
						QuaternionSetRotation(new_rotation, axis_x, fabsf(angle));
					}
					else
					{
						float x, ex;
						QuaternionSetRotation(new_rotation, local_axis, angle);
						Matrix3x3SetRotation(matrix, new_rotation);
						Matrix3x3GetEulerZYX(matrix, &rotation_euler[2], &rotation_euler[1], &rotation_euler[0]);
						Matrix3x3SetRotation(matrix, joint_bone->rotation);
						Matrix3x3GetEulerZYX(matrix, &joint_euler[2], &joint_euler[1], &joint_euler[0]);
						x = rotation_euler[0];
						ex = joint_euler[0];
						if(x + ex > (float)M_PI)
						{
							x = (float)M_PI - ex;
						}
						if(MIN_ROTATION_SUM > x + ex)
						{
							x = MIN_ROTATION_SUM - ex;
						}
						CLAMPED(x, - constraint->angle_limit, constraint->angle_limit);
						if(fabsf(x) < MIN_ROTATION)
						{
							continue;
						}
						QuaternionSetEulerZYX(new_rotation, 0, 0, x);
					}
					COPY_VECTOR4(local_rotation, new_rotation);
					MultiQuaternion(local_rotation, joint_bone->rotation);
					COPY_VECTOR4(joint_bone->rotation, local_rotation);
				}
				else
				{
					QUATERNION local_rotation;
					QuaternionSetRotation(new_rotation, local_axis, angle);
					COPY_VECTOR4(local_rotation, joint_bone->rotation);
					MultiQuaternion(local_rotation, new_rotation);
					COPY_VECTOR4(joint_bone->rotation, local_rotation);
				}
				for(l=k; l >= 0; l--)
				{
					Pmd2BonePerformTransform(joint_bones[l]);
				}
				Pmd2BonePerformTransform(effector_bone);
			}
		}
		COPY_VECTOR4(effector_bone->rotation, origin_target_bone_rotation);
		Pmd2BonePerformTransform(effector_bone);
	}
}

static void DeletePmd2DefaultStaticBuffer(PMD2_DEFAULT_STATIC_BUFFER* buffer)
{
	MEM_FREE_FUNC(buffer);
}

static size_t Pmd2DefaultStaticBufferGetStrideOffset(PMD2_DEFAULT_STATIC_BUFFER* buffer, eMODEL_STRIDE_TYPE type)
{
	switch(type)
	{
	case MODEL_BONE_INDEX_STRIDE:
		return offsetof(PMD2_DEFAULT_STATIC_BUFFER_UNIT, bone_indices);
	case MODEL_BONE_WEIGHT_STRIDE:
		return offsetof(PMD2_DEFAULT_STATIC_BUFFER_UNIT, bone_weights);
	case MODEL_TEXTURE_COORD_STRIDE:
		return offsetof(PMD2_DEFAULT_STATIC_BUFFER_UNIT, tex_coord);
	}
	return 0;
}

static size_t Pmd2DefaultStaticBufferGetStrideSize(PMD2_DEFAULT_STATIC_BUFFER* buffer)
{
	return sizeof(PMD2_DEFAULT_STATIC_BUFFER_UNIT);
}

static size_t Pmd2DefaultStaticBufferGetSize(PMD2_DEFAULT_STATIC_BUFFER* buffer)
{
	return Pmd2DefaultStaticBufferGetStrideSize(buffer) * buffer->model->vertices->num_data;
}

static void Pmd2DefaultStaticBufferUnitUpdate(PMD2_DEFAULT_STATIC_BUFFER_UNIT* unit, VERTEX_INTERFACE* vertex)
{
	BONE_INTERFACE *bone1 = vertex->get_bone(vertex, 0);
	BONE_INTERFACE *bone2 = vertex->get_bone(vertex, 1);
	COPY_VECTOR3(unit->tex_coord, ((PMD2_VERTEX*)vertex)->tex_coord);
	unit->bone_indices[0] = (float)bone1->index;
	unit->bone_indices[1] = (float)bone2->index;
	unit->bone_indices[2] = unit->bone_indices[3] = 0;
	unit->bone_weights[0] = (float)((PMD2_VERTEX*)vertex)->weight;
	unit->bone_weights[1] = unit->bone_weights[2] = unit->bone_weights[3] = 0;
}

static void Pmd2DefaultStaticBufferUpdate(PMD2_DEFAULT_STATIC_BUFFER* buffer, void* address)
{
	PMD2_DEFAULT_STATIC_BUFFER_UNIT *unit = (PMD2_DEFAULT_STATIC_BUFFER_UNIT*)address;
	PMD2_VERTEX *vertices = (PMD2_VERTEX*)buffer->model->vertices->buffer;
	const int num_vertices = (int)buffer->model->vertices->num_data;
	int i;
	for(i=0; i<num_vertices; i++)
	{
		Pmd2DefaultStaticBufferUnitUpdate(&unit[i], (VERTEX_INTERFACE*)&vertices[i]);
	}
}

PMD2_DEFAULT_STATIC_BUFFER* Pmd2DefaultStaticVertexBufferNew(PMD2_MODEL* model)
{
	PMD2_DEFAULT_STATIC_BUFFER *ret;

	ret = (PMD2_DEFAULT_STATIC_BUFFER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->model = model;
	ret->interface_data.base.get_size =
		(size_t (*)(void*))Pmd2DefaultStaticBufferGetSize;
	ret->interface_data.base.get_stride_size =
		(size_t (*)(void*))Pmd2DefaultStaticBufferGetStrideSize;
	ret->interface_data.base.get_stride_offset =
		(size_t (*)(void*, eMODEL_STRIDE_TYPE))Pmd2DefaultStaticBufferGetStrideOffset;
	ret->interface_data.base.delete_func =
		(void (*)(void*))DeletePmd2DefaultStaticBuffer;
	ret->interface_data.update =
		(void (*)(void*, void*))Pmd2DefaultStaticBufferUpdate;

	return ret;
}

void Pmd2ModelGetDefaultStaticVertexBuffer(PMD2_MODEL* model, PMD2_DEFAULT_STATIC_BUFFER** buffer)
{
	if(buffer != NULL)
	{
		if(*buffer != NULL)
		{
			DeletePmd2DefaultStaticBuffer(*buffer);
		}
		*buffer = Pmd2DefaultStaticVertexBufferNew(model);
	}
}

static void DeletePmd2DefaultDynamicBuffer(PMD2_DEFAULT_DYNAMIC_BUFFER* buffer)
{
	MEM_FREE_FUNC(buffer);
}

void* Pmd2DefaultDynamicBufferGetIdent(PMD2_DEFAULT_DYNAMIC_BUFFER* buffer)
{
	return (void*)&buffer->model->interface_data.scene->project->pmd2_default_buffer_ident.dynamic_buffer_ident;
}

static size_t Pmd2DefaultDynamicBufferGetStrideOffset(PMD2_DEFAULT_DYNAMIC_BUFFER* buffer, eMODEL_STRIDE_TYPE type)
{
	switch(type)
	{
	case MODEL_VERTEX_STRIDE:
		return offsetof(PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT, position);
	case MODEL_NORMAL_STRIDE:
		return offsetof(PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT, normal);
	case MODEL_MORPH_DELTA_STRIDE:
		return offsetof(PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT, delta);
	case MODEL_EDGE_VERTEX_STRIDE:
		return offsetof(PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT, edge);
	case MODEL_EDGE_SIZE_STRIDE:
		return (offsetof(PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT, normal) + sizeof(float)*3);
	case MODEL_VERTEX_INDEX_STRIDE:
		return (offsetof(PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT, edge) + sizeof(float)*3);
	}
	return 0;
}

static size_t Pmd2DefaultDynamicBufferGetStrideSize(PMD2_DEFAULT_DYNAMIC_BUFFER* buffer)
{
	return sizeof(PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT);
}

static size_t Pmd2DefaultDynamicBufferGetSize(PMD2_DEFAULT_DYNAMIC_BUFFER* buffer)
{
	return Pmd2DefaultDynamicBufferGetStrideSize(buffer) * buffer->model->vertices->num_data;
}

static void Pmd2DefaultDynamicBufferUnitUpdate(
	PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT* unit,
	VERTEX_INTERFACE* vertex,
	FLOAT_T material_edge_size,
	int index,
	float* p
)
{
	VECTOR3 n;
	FLOAT_T edge_size = vertex->edge_size * material_edge_size;
	vertex->perform_skinning(vertex, p, n);
	COPY_VECTOR3(unit->position, p);
	COPY_VECTOR3(unit->normal, n);
	unit->normal[3] = (float)vertex->edge_size;
	unit->edge[0] = unit->position[0] + unit->normal[0] * (float)edge_size;
	unit->edge[1] = unit->position[1] + unit->normal[1] * (float)edge_size;
	unit->edge[2] = unit->position[2] + unit->normal[2] * (float)edge_size;
	unit->edge[3] = (float)index;
}

void Pmd2DefaultDynamicBufferPerformTransform(
	PMD2_DEFAULT_DYNAMIC_BUFFER* buffer,
	void* address,
	float* camera_position,
	float* aa_bb_min,
	float* aa_bb_max
)
{
	PMD2_VERTEX *vertices = (PMD2_VERTEX*)buffer->model->vertices->buffer;
	PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT *unit = (PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT*)address;
	PARALLEL_SKINNING_VERTEX skinning;
	void *processor;

	InitializeSkinningVertex(&skinning, (MODEL_INTERFACE*)buffer->model, buffer->model->vertices,
		camera_position, address, (void (*)(void*, VERTEX_INTERFACE*, FLOAT_T, int, float*))Pmd2DefaultDynamicBufferUnitUpdate, sizeof(PMD2_DEFAULT_DYNAMIC_BUFFER_UNIT));
	processor = ParallelProcessorNew((void*)buffer->model->vertices->buffer, (void (*)(void*, int, void*))SkinningVertexProcess,
		(void (*)(void*))SkinningVertexStart, (void (*)(void*))SkinningVertexEnd, (void (*)(void*, void*))SkinningVertexMakeNext,
		(void (*)(void*, void*))SkinningVertexJoin, buffer->model->vertices->data_size, (int)buffer->model->vertices->num_data,
		(void*)&skinning, sizeof(skinning), PARALLEL_PROCESSOR_TYPE_REDUCTION);
	ExecuteParallelProcessor(processor);
	DeleteParallelProcessor(processor);
	COPY_VECTOR3(aa_bb_min, skinning.aa_bb_min);
	COPY_VECTOR3(aa_bb_max, skinning.aa_bb_max);
}

PMD2_DEFAULT_DYNAMIC_BUFFER* Pmd2DefaultDynamicVertexBufferNew(
	PMD2_MODEL* model,
	MODEL_INDEX_BUFFER_INTERFACE* index_buffer
)
{
	PMD2_DEFAULT_DYNAMIC_BUFFER *ret;

	ret = (PMD2_DEFAULT_DYNAMIC_BUFFER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->model = model;
	ret->index_buffer = (PMD2_DEFAULT_INDEX_BUFFER*)index_buffer;

	ret->interface_data.base.ident = (void* (*)(void*))Pmd2DefaultDynamicBufferGetIdent;
	ret->interface_data.base.get_size = (size_t (*)(void*))Pmd2DefaultDynamicBufferGetSize;
	ret->interface_data.base.get_stride_size = (size_t (*)(void*))Pmd2DefaultDynamicBufferGetStrideSize;
	ret->interface_data.base.get_stride_offset = (size_t (*)(void*, eMODEL_STRIDE_TYPE))Pmd2DefaultDynamicBufferGetStrideOffset;
	ret->interface_data.base.delete_func = (void (*)(void*))DeletePmd2DefaultDynamicBuffer;
	ret->interface_data.perform_transform =
		(void (*)(void*, void*, const float*, float*, float*))Pmd2DefaultDynamicBufferPerformTransform;

	return ret;
}

void Pmd2ModelGetDefaultDynamicVertexBuffer(
	PMD2_MODEL* model,
	PMD2_DEFAULT_DYNAMIC_BUFFER** buffer,
	PMD2_DEFAULT_INDEX_BUFFER* index_buffer
)
{
	if(buffer != NULL)
	{
		if(*buffer != NULL)
		{
			DeletePmd2DefaultDynamicBuffer(*buffer);
		}

		if(index_buffer != NULL && index_buffer->interface_data.base.ident(index_buffer)
			== &model->interface_data.scene->project->pmd2_default_buffer_ident.index_buffer_ident)
		{
			*buffer = Pmd2DefaultDynamicVertexBufferNew(model, (MODEL_INDEX_BUFFER_INTERFACE*)index_buffer);
		}
		else
		{
			*buffer = NULL;
		}
	}
}

static void DeletePmd2DefaultIndexBuffer(PMD2_DEFAULT_INDEX_BUFFER* buffer)
{
	MEM_FREE_FUNC(buffer);
}

static void* Pmd2DefaultIndexBufferGetIdent(PMD2_DEFAULT_INDEX_BUFFER* buffer)
{
	return (void*)&buffer->model->interface_data.scene->project->pmd2_default_buffer_ident.index_buffer_ident;
}

static eMODEL_INDEX_BUFFER_TYPE Pmd2DefaultIndexBufferGetType(PMD2_DEFAULT_INDEX_BUFFER* buffer)
{
	return MODEL_INDEX_BUFFER_TYPE_INDEX16;
}

static void* Pmd2DefaultIndexBufferGetBytes(PMD2_DEFAULT_INDEX_BUFFER* buffer)
{
	return (void*)buffer->indices->buffer;
}

static size_t Pmd2DefaultIndexBufferGetStrideSize(PMD2_DEFAULT_INDEX_BUFFER* buffer)
{
	return sizeof(uint16);
}

static size_t Pmd2DefaultIndexBufferGetSize(PMD2_DEFAULT_INDEX_BUFFER* buffer)
{
	return Pmd2DefaultIndexBufferGetStrideSize(buffer) * buffer->indices->num_data;
}

static void Pmd2DefaultIndexBufferSetIndexAt(
	PMD2_DEFAULT_INDEX_BUFFER* buffer,
	int i,
	uint16 value
)
{
	buffer->indices->buffer[i] = value;
}

PMD2_DEFAULT_INDEX_BUFFER* Pmd2DefaultIndexBufferNew(
	PMD2_MODEL* model,
	UINT32_ARRAY* indices,
	const int num_vertices
)
{
	PMD2_DEFAULT_INDEX_BUFFER *ret;
	uint16 temp;
	size_t i;

	ret = (PMD2_DEFAULT_INDEX_BUFFER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->indices = WordArrayNew(indices->num_data + 1);
	for(i=0; i<indices->num_data; i++)
	{
		int32 index = (int32)indices->buffer[i];
		if(index >= 0 && index < num_vertices)
		{
			Pmd2DefaultIndexBufferSetIndexAt(ret, (int)i, (uint16)index);
		}
		else
		{
			Pmd2DefaultIndexBufferSetIndexAt(ret, (int)i, 0);
		}
	}
	for(i=0; i<indices->num_data; i += 3)
	{
		temp = ret->indices->buffer[i];
		ret->indices->buffer[i] = ret->indices->buffer[i+1];
		ret->indices->buffer[i+1] = temp;
	}

	ret->model = model;
	ret->indices->num_data = indices->num_data;
	ret->interface_data.base.ident = (void* (*)(void*))Pmd2DefaultIndexBufferGetIdent;
	ret->interface_data.get_type = (eMODEL_INDEX_BUFFER_TYPE (*)(void*))Pmd2DefaultIndexBufferGetType;
	ret->interface_data.bytes = (void* (*)(void*))Pmd2DefaultIndexBufferGetBytes;
	ret->interface_data.base.get_size = (size_t (*)(void*))Pmd2DefaultIndexBufferGetSize;
	ret->interface_data.base.get_stride_size = (size_t (*)(void*))Pmd2DefaultIndexBufferGetStrideSize;
	ret->interface_data.base.delete_func = (void (*)(void*))DeletePmd2DefaultIndexBuffer;

	return ret;
}

void Pmd2ModelGetDefaultIndexBuffer(PMD2_MODEL* model, PMD2_DEFAULT_INDEX_BUFFER** buffer)
{
	if(buffer != NULL)
	{
		if(*buffer != NULL)
		{
			DeletePmd2DefaultIndexBuffer(*buffer);
		}
		*buffer = Pmd2DefaultIndexBufferNew(model, model->indices, (int)model->vertices->num_data);
	}
}

void Pmd2ModelJoinWorld(PMD2_MODEL* model, void* world)
{
	if(world != NULL)
	{
		const int num_rigid_bodies = (int)model->rigid_bodies->num_data;
		const int num_joints = (int)model->joints->num_data;
		PMD2_RIGID_BODY *bodies = (PMD2_RIGID_BODY*)model->rigid_bodies->buffer;
		PMD2_RIGID_BODY *body;
		PMD2_JOINT *joints = (PMD2_JOINT*)model->joints->buffer;
		PMD2_JOINT *joint;
		int i;

		for(i=0; i<num_rigid_bodies; i++)
		{
			body = &bodies[i];
			BaseRigidBodyJoinWorld((BASE_RIGID_BODY*)body, world);
		}
		for(i=0; i<num_joints; i++)
		{
			joint = &joints[i];
			BaseJointJoinWorld((BASE_JOINT*)joint, world);
		}
	}
}

void Pmd2ModelResetMotionState(PMD2_MODEL* model, void* world)
{
	if((model->flags & PMD2_MODEL_FLAG_ENABLE_PHYSICS) != 0)
	{
		int num_items;
		PMD2_BONE **bones = (PMD2_BONE**)model->sorted_bones->buffer;
		PMD2_RIGID_BODY *rigid_bodies = (PMD2_RIGID_BODY*)model->rigid_bodies->buffer;
		PMD2_JOINT *joints = (PMD2_JOINT*)model->joints->buffer;
		int i;

		num_items = (int)model->sorted_bones->num_data;
		for(i=0; i<num_items; i++)
		{
			Pmd2BonePerformTransform(bones[i]);
		}
		num_items = (int)model->rigid_bodies->num_data;
		for(i=0; i<num_items; i++)
		{
			ResetBaseRigidBody((BASE_RIGID_BODY*)&rigid_bodies[i], world);
			BaseRigidBodyUpdateTransform((BASE_RIGID_BODY*)&rigid_bodies[i]);
			BaseRigidBodySetActivation((BASE_RIGID_BODY*)&rigid_bodies[i], TRUE);
		}
		num_items = (int)model->joints->num_data;
		for(i=0; i<num_items; i++)
		{
			BaseJointUpdateTransform((BASE_JOINT*)&joints[i]);
		}
	}
}

#ifdef __cplusplus
}
#endif
