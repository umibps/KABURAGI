#ifndef _INCLUDED_ANNOTATION_H_
#define _INCLUDED_ANNOTATION_H_

#include "types.h"
#include "ght_hash_table.h"
#include "parameter.h"

typedef enum _eANNOTATION_TYPE
{
	ANNOTATION_TYPE_FX,
	ANNOTATION_TYPE_NV,
	NUM_ANNOTATION_TYPE
} eANNOTATION_TYPE;

typedef struct _FX_ANNOTATION
{
	TYPE_UNION *values;
	ght_hash_table_t *int_table;
	ght_hash_table_t *float_table;
	ght_hash_table_t *string_table;
	struct _ANNOTATION *annotation;
	int num_values;
	int values_buffer_size;
	char *name;
	char *value_str;
	enum _ePARAMETER_TYPE base;
	enum _ePARAMETER_TYPE full;
} FX_ANNOTATION;

typedef struct _NV_ANNOTATION
{
	struct _EFFECT *effect;
	char *name;
	struct _ANNOTATION *annotation;
	float fvalue;
	int ivalue;
} NV_ANNOTATION;

typedef struct _NV_EFFECT
{
	NV_PARAMETER parameter;
	NV_TECHNIQUE tech;
	NV_PASS pass;
	NV_SAMPLER_STATE state;
	struct _NV_ANNOTATION annotation;

	ght_hash_table_t *annotation_hash;
	NV_PARAMETER *parameters;
	int num_parameters;
	int size_parameter_buffer;
	NV_TECHNIQUE *techs;
	int num_techs;
	int size_techs_buffer;
	struct _APPLICATION *context;
	struct _EFFECT_CONTEXT *effect_context;
	uint32 render_color_target_indices;
	int num_indices;
	int size_indices_buffer;
	struct _OFFSCREEN_RENDER_TAREGET *offscreen_render_targets;
	int num_offscreen_render;
	int size_offscreen_buffer;
	PARAMETER *inter_active_parameters;
	int num_inter_active;
	int size_inter_active_buffer;
	struct _EFFECT *parent_effect;
	struct _FRAME_BUFFER_OBJECT *parent_frame_buffer;
	eSCRIPT_ORDER_TYPE script_order_type;
} NV_EFFECT;

typedef struct _EFFECT
{
	MATRIX_SEMANTIC world;
	MATRIX_SEMANTIC view;
	MATRIX_SEMANTIC projection;
	MATRIX_SEMANTIC world_view;
	MATRIX_SEMANTIC view_projection;
	MATRIX_SEMANTIC world_view_projection;

	eEFFECT_TYPE type;
	union
	{
		NV_EFFECT nv;
	} data;
} EFFECT;

typedef struct _ANNOTATION
{
	eANNOTATION_TYPE type;
	union
	{
		FX_ANNOTATION fx;
		NV_ANNOTATION nv;
	} anno;
	ght_hash_table_t *annotation_table;
} ANNOTATION;

#ifdef __cplusplus
extern "C" {
#endif

extern void EffectSetModelMatrixParameter(
	EFFECT* effect,
	MODEL_INTERFACE* model,
	int extra_camera_flags,
	int extra_light_flags
);

extern void EffectSetVertexAttributePointer(
	EFFECT* effect,
	eVERTEX_ATTRIBUTE_TYPE vertex_type,
	ePARAMETER_TYPE type,
	size_t stride,
	void* pointer
);

extern void EffectActivateVertexAttribute(
	EFFECT* effect,
	eVERTEX_ATTRIBUTE_TYPE vertex_type
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_ANNOTATION_H_
