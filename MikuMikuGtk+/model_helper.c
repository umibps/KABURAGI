#include <string.h>
#include <float.h>
#include "model_helper.h"
#include "model.h"
#include "material.h"
#include "bullet.h"

INLINE uint8 AdjustSharedToonTextureIndex(uint8 index)
{
	return (index == 0xff) ? 0 : index + 1;
}

void TransformVertex(
	void * transform,
	float* in_position,
	float* in_normal,
	float* out_position,
	float* out_normal
)
{
	float basis[9];
	BtTransformMultiVector3(transform, in_position, out_position);
	BtTransformGetBasis(transform, basis);
	COPY_VECTOR3(out_normal, in_normal);
	MulMatrixVector3(out_normal, basis);
}

void TransformVertexInterpolate(
	void* transform_a,
	void* transform_b,
	float* in_position,
	float* in_normal,
	float* out_position,
	float* out_normal,
	FLOAT_T weight
)
{
	float v1[3];
	float n1[3];
	float v2[3];
	float n2[3];
	float basis[9];

	BtTransformMultiVector3(transform_a, in_position, v1);
	COPY_VECTOR3(n1, in_normal);
	BtTransformGetBasis(transform_a, basis);
	MulMatrixVector3(n1, basis);
	BtTransformMultiVector3(transform_b, in_position, v2);
	COPY_VECTOR3(n2, in_normal);
	BtTransformGetBasis(transform_b, basis);
	MulMatrixVector3(n2, basis);
	SetInterpolateVector3(out_position, v2, v1, (float)weight);
	SetInterpolateVector3(out_normal, n2, n1, (float)weight);
}

void InitializeSkinningVertex(
	PARALLEL_SKINNING_VERTEX* skinning,
	MODEL_INTERFACE* model,
	STRUCT_ARRAY* vertices,
	float* camera_postion,
	void* address,
	void (*update)(void*, struct _VERTEX_INTERFACE*, FLOAT_T, int, float*),
	size_t unit_size
)
{
	(void)memset(skinning, 0, sizeof(*skinning));
	skinning->vertices = vertices;
	skinning->aa_bb_max[0] = - FLT_MAX;
	skinning->aa_bb_max[1] = - FLT_MAX;
	skinning->aa_bb_max[2] = - FLT_MAX;
	skinning->aa_bb_min[0] = FLT_MAX;
	skinning->aa_bb_min[1] = FLT_MAX;
	skinning->aa_bb_min[2] = FLT_MAX;
	skinning->edge_scale_factor =
		model->get_edge_scale_factor(model, camera_postion);
	skinning->unit = (struct _MODEL_BUFFER_UNIT_INTERFACE*)address;
	skinning->update = update;
	skinning->unit_size = unit_size;
}

void SkinningVertexStart(PARALLEL_SKINNING_VERTEX* skinning)
{
	COPY_VECTOR3(skinning->local_min, skinning->aa_bb_min);
	COPY_VECTOR3(skinning->local_max, skinning->aa_bb_max);
}

void SkinningVertexProcess(VERTEX_INTERFACE* vertex, int index, PARALLEL_SKINNING_VERTEX* skinning)
{
	MATERIAL_INTERFACE *material = vertex->material;
	if(material != NULL)
	{
		float position[3];
		float material_edge_size = (float)(material->get_edge_size(material) * skinning->edge_scale_factor);
		struct _MODEL_BUFFER_UNIT_INTERFACE *unit;
		uint8 *buff = (uint8*)skinning->unit;
		unit = (struct _MODEL_BUFFER_UNIT_INTERFACE*)&buff[skinning->unit_size * index];
		skinning->update((void*)unit, vertex, material_edge_size, index, position);
		SET_MIN(skinning->aa_bb_min[0], position[0]);
		SET_MIN(skinning->aa_bb_min[1], position[1]);
		SET_MIN(skinning->aa_bb_min[2], position[2]);
		SET_MAX(skinning->aa_bb_max[0], position[0]);
		SET_MAX(skinning->aa_bb_max[1], position[1]);
		SET_MAX(skinning->aa_bb_max[2], position[2]);
		//skinning->update((void*)unit, vertex, material_edge_size, index, skinning->local_position);
		//SET_MIN(skinning->local_min[0], skinning->local_position[0]);
		//SET_MIN(skinning->local_min[1], skinning->local_position[1]);
		//SET_MIN(skinning->local_min[2], skinning->local_position[2]);
		//SET_MAX(skinning->local_max[0], skinning->local_position[0]);
		//SET_MAX(skinning->local_max[1], skinning->local_position[1]);
		//SET_MAX(skinning->local_max[2], skinning->local_position[2]);
	}
}

void SkinningVertexEnd(PARALLEL_SKINNING_VERTEX* skinning)
{
	//COPY_VECTOR3(skinning->aa_bb_min, skinning->local_min);
	//COPY_VECTOR3(skinning->aa_bb_max, skinning->local_max);
}

void SkinningVertexMakeNext(PARALLEL_SKINNING_VERTEX* src, PARALLEL_SKINNING_VERTEX* dst)
{
	dst->vertices = src->vertices;
	dst->edge_scale_factor = src->edge_scale_factor;
	COPY_VECTOR3(dst->aa_bb_min, src->aa_bb_min);
	COPY_VECTOR3(dst->aa_bb_max, src->aa_bb_max);
	//dst->aa_bb_min[0] = FLT_MAX;
	//dst->aa_bb_min[1] = FLT_MAX;
	//dst->aa_bb_min[2] = FLT_MAX;
	//dst->aa_bb_max[0] = - FLT_MAX;
	//dst->aa_bb_max[1] = - FLT_MAX;
	//dst->aa_bb_max[2] = - FLT_MAX;
	dst->unit = src->unit;
	dst->update = src->update;
	dst->unit_size = src->unit_size;
}

void SkinningVertexJoin(PARALLEL_SKINNING_VERTEX* obj, PARALLEL_SKINNING_VERTEX* self)
{
	SET_MIN(obj->aa_bb_min[0], self->aa_bb_min[0]);
	SET_MIN(obj->aa_bb_min[1], self->aa_bb_min[1]);
	SET_MIN(obj->aa_bb_min[2], self->aa_bb_min[2]);
	SET_MAX(obj->aa_bb_max[0], self->aa_bb_max[0]);
	SET_MAX(obj->aa_bb_max[1], self->aa_bb_max[1]);
	SET_MAX(obj->aa_bb_max[2], self->aa_bb_max[2]);
}
