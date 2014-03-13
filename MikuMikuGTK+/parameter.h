#ifndef _INCLUDED_PARAMETER_H_
#define _INCLUDED_PARAMETER_H_

#include <stdio.h>
#include <GL/glew.h>
#include "model.h"
#include "types.h"
#include "ght_hash_table.h"
#include "technique.h"
#include "pass.h"
#include "annotation.h"
#include "sampler_state.h"

typedef enum _ePARAMETER_TYPE
{
	PARAMETER_TYPE_UNKNOWN,
	PARAMETER_TYPE_BOOLEAN,
	PARAMETER_TYPE_BOOL1,
	PARAMETER_TYPE_BOOL2,
	PARAMETER_TYPE_BOOL3,
	PARAMETER_TYPE_BOOL4,
	PARAMETER_TYPE_INTEGER,
	PARAMETER_TYPE_INT1,
	PARAMETER_TYPE_INT2,
	PARAMETER_TYPE_INT3,
	PARAMETER_TYPE_INT4,
	PARAMETER_TYPE_FLOAT,
	PARAMETER_TYPE_FLOAT1,
	PARAMETER_TYPE_FLOAT2,
	PARAMETER_TYPE_FLOAT3,
	PARAMETER_TYPE_FLOAT4,
	PARAMETER_TYPE_FLOAT2x2,
	PARAMETER_TYPE_FLOAT3x3,
	PARAMETER_TYPE_FLOAT4x4,
	PARAMETER_TYPE_STRING,
	PARAMETER_TYPE_TEXTURE,
	PARAMETER_TYPE_SAMPLER,
	PARAMETER_TYPE_SAMPLER1D,
	PARAMETER_TYPE_SAMPLER2D,
	PARAMETER_TYPE_SAMPLER3D,
	PARAMETER_TYPE_SAMPLER_CUBE,
	MAX_PARAMETER_TYPE
} ePARAMETER_TYPE;

typedef struct _NV_PARAMETER
{
	void **states;
	int num_states;
	int states_buffer_size;
	struct _EFFECT *effect;
} NV_PARAMETER;

typedef struct _CF_PARAMETER
{
	struct _EFFECT *parent_effect;
	struct _ANNOTATION *annotation;
	TYPE_UNION *values;
	int num_values;
	int size_values_buffer;
	char *symbol;
	char *semantic;
	char *value;
	ePARAMETER_TYPE base;
	ePARAMETER_TYPE full;
} CF_PARAMETER;

typedef struct _PARAMETER
{
	ePARAMETER_TYPE type;
	union
	{
		int scalari;
		int is_scalar;
		float scalarf;
		float vector[4];
		float matrix[16];
		int *texture;
		int *sampler;
	} value;

	union
	{
		NV_PARAMETER nv;
		CF_PARAMETER cf;
	} data;
} PARAMETER;

typedef struct _MATRIX_SEMANTIC
{
	PARAMETER parameter;
	PARAMETER camera;
	PARAMETER camera_inversed;
	PARAMETER camera_transposed;
	PARAMETER camera_inverse_transposed;
	PARAMETER light;
	PARAMETER light_inversed;
	PARAMETER light_transposed;
	PARAMETER light_inverse_transposed;
	struct _PROJECT *project;
	int flags;
} MATRIX_SEMANTIC;

typedef struct _MATERIAL_SEMANTIC
{
	PARAMETER *geometry;
	PARAMETER *light;
} MATERIAL_SEMANTIC;

typedef struct _DRAW_PRIMITIVE_COMMAND
{
	GLenum mode;
	GLsizei count;
	GLenum type;
	uint8 *pointer;
	size_t offset;
	size_t stride;
	int start;
	int end;
} DRAW_PRIMITIVE_COMMAND;

typedef enum _eSCRIPT_ORDER_TYPE
{
	SCRIPT_ORDER_TYPE_PRE_PROCESS,
	SCRIPT_ORDER_TYPE_STANDARD,
	SCRIPT_ORDER_TYPE_STNDRD_OFFSCREEN,
	SCRIPT_ORDER_TYPE_POST_PROCESS,
	SCRIPT_ORDER_TYPE_AUTO_DETECTION,
	SCRIPT_ORDER_TYPE_DEFAULT,
	MAX_SCRIPT_ORDER_TYPE
} eSCRIPT_ORDER_TYPE;

typedef struct _OFFSCREEN_RENDER_TARGET
{
	struct _TEXTURE *texture;
	PARAMETER *texture_parameter;
	PARAMETER *sampler_parameter;
} OFFSCREEN_RENDER_TARGET;

typedef enum _eEFFECT_TYPE
{
	EFFECT_TYPE_NV,
	EFFECT_TYPE_CF,
	NUM_EFFECT_TYPE
} eEFFECT_TYPE;

typedef struct _NV_EFFECT
{
	NV_PARAMETER parameter;
	NV_TECHNIQUE tech;
	NV_PASS pass;
	NV_SAMPLER_STATE state;
	NV_ANNOTATION annotation;

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

typedef enum _eVERTEX_ATTRIBUTE_TYPE
{
	VERTEX_ATTRIBUTE_UNKNOWN = -1,
	VERTEX_ATTRIBUTE_POSTION,
	VERTEX_ATTRIBUTE_NORMAL,
	VERTEX_ATTRIBUTE_TEXTURE_COORD,
	VERTEX_ATTRIBUTE_BONE_INDEX,
	VERTEX_ATTRIBUTE_BONE_WEIGHT,
	VERTEX_ATTRIBUTE_UVA1,
	VERTEX_ATTRIBUTE_UVA2,
	VERTEX_ATTRIBUTE_UVA3,
	VERTEX_ATTRIBUTE_UVA4,
	MAX_VERTEX_ATTRIBUTE
} eVERTEX_ATTRIBUTE_TYPE;

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

#endif	// #ifndef _INCLUDED_PARAMETER_H_
