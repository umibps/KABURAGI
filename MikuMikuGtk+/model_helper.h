#ifndef _INCLUDED_MODEL_HELPER_H_
#define _INCLUDED_MODEL_HELPER_H_

#include "utils.h"
#include "types.h"
#include "model.h"
#include "vertex.h"

extern INLINE uint8 AdjustSharedToonTextureIndex(uint8 index);

extern void TransformVertex(
	void * transform,
	float* in_position,
	float* in_normal,
	float* out_position,
	float* out_normal
);

extern void TransformVertexInterpolate(
	void* transform_a,
	void* transform_b,
	float* in_position,
	float* in_normal,
	float* out_position,
	float* out_normal,
	FLOAT_T weight
);

extern void InitializeSkinningVertex(
	PARALLEL_SKINNING_VERTEX* skinning,
	MODEL_INTERFACE* model,
	STRUCT_ARRAY* vertices,
	float* camera_postion,
	void* address,
	void (*update)(void*, struct _VERTEX_INTERFACE*, FLOAT_T, int, float*),
	size_t unit_size
);
extern void SkinningVertexStart(PARALLEL_SKINNING_VERTEX* skinning);
extern void SkinningVertexProcess(VERTEX_INTERFACE* vertex, int index, PARALLEL_SKINNING_VERTEX* skinning);
extern void SkinningVertexEnd(PARALLEL_SKINNING_VERTEX* skinning);
extern void SkinningVertexMakeNext(PARALLEL_SKINNING_VERTEX* src, PARALLEL_SKINNING_VERTEX* dst);
extern void SkinningVertexJoin(PARALLEL_SKINNING_VERTEX* obj, PARALLEL_SKINNING_VERTEX* self);

#endif	// #ifndef _INCLUDED_MODEL_HELPER_H_
