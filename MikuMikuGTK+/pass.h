#ifndef _INCLUDED_PASS_H_
#define _INCLUDED_PASS_H_

#include "types.h"

typedef enum _ePASS_TYPE
{
	PASS_TYPE_NV,
	PASS_TYPE_CF,
	NUM_PASS_TYPE
} ePASS_TYPE;

typedef struct _NV_PASS
{
	struct _EFFECT *effect;
	struct _TECHNIQUE *tech;
	struct _PASS *override_passes;
	int num_override;
	int size_override_buffer;
	struct _PASS *value;
} NV_PASS;

typedef struct _CF_PASS
{
	STR_STR_MAP *sources;
	STR_PTR_MAP *annotations;
	struct _CF_PARAMETER *parameters;
	int num_parameters;
	int size_parameters_buffer;
	char *name;
	struct _CF_TECHNIQUE *tech;
} CF_PASS;

typedef struct _PASS
{
	ePASS_TYPE type;
	union
	{
		NV_PASS nv;
		CF_PASS cf;
	} pass;
} PASS;

#endif	// #ifndef _INCLUDED_PASS_H_
