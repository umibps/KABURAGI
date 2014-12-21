#ifndef _INCLUDED_ANNOTATION_H_
#define _INCLUDED_ANNOTATION_H_

#include "types.h"
#include "ght_hash_table.h"
#include "parameter.h"
#include "effect.h"
#include "utils.h"

typedef enum _eANNOTATION_TYPE
{
	ANNOTATION_TYPE_FX,
	ANNOTATION_TYPE_NV,
	NUM_ANNOTATION_TYPE
} eANNOTATION_TYPE;

typedef struct _FX_ANNOTATION
{
	STRUCT_ARRAY *values;
	char *name;
	char *value;
	ePARAMETER_TYPE base;
	ePARAMETER_TYPE full;
} FX_ANNOTATION;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_ANNOTATION_H_
