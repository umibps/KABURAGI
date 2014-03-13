#ifndef _INCLUDED_ANNOTATION_H_
#define _INCLUDED_ANNOTATION_H_

#include "types.h"
#include "ght_hash_table.h"

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

#endif	// #ifndef _INCLUDED_ANNOTATION_H_
