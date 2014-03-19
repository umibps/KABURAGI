#ifndef _INCLUDED_TECHNIQUE_H_
#define _INCLUDED_TECHNIQUE_H_

#include "ght_hash_table.h"
#include "types.h"

typedef enum _eTECHNIQUE_TYPE
{
	TECHNIQUE_TYPE_CF,
	TECHNIQUE_TYPE_NV,
	NUM_TECHNIQUE_TYPE
} eTECHNIQUE_TYPE;

typedef struct _CF_TECHNIQUE
{
	struct _EFFECT *parent_effect;
	ght_hash_table_t *parameter_map;
	ght_hash_table_t *pass_map;
	ght_hash_table_t *annotation_map;
	char *name;
} CF_TECHNIQUE;

typedef struct _NV_TECHNIQUE
{
	struct _EFFECT *effect;
	ght_hash_table_t *pass_hash;
} NV_TECHNIQUE;

typedef struct _TECHNIQUE
{
	eTECHNIQUE_TYPE type;
	union
	{
		CF_TECHNIQUE cf;
		NV_TECHNIQUE nv;
	} tech;
} TECHNIQUE;

typedef struct _TECHNIQUES
{
	TECHNIQUE *techs;
	int num_tech;
	int size_techs_buffer;
} TECHNIQUES;

#endif	// #ifndef _INCLUDED_TECHNIQUE_H_
