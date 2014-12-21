#ifndef _INCLUDED_EFFECT_H_
#define _INCLUDED_EFFECT_H_

#include "ght_hash_table.h"

typedef enum _eEFFECT_SCRIPT_ORDER_TYPE
{
	EFFECT_SCRIPT_ORDER_TYPE_PRE_PROCESS,
	EFFECT_SCRIPT_ORDER_TYPE_STANDARD,
	EFFECT_SCRIPT_ORDER_TYPE_STANDARD_OFFSCRESSN,
	EFFECT_SCRIPT_ORDER_TYPE_POST_PROCESS,
	EFFECT_SCRIPT_ORDER_TYPE_AUTO_DETECTION,
	EFFECT_SCRIPT_ORDER_TYPE_DEFAULT,
	MAX_EFFECT_SCRIPT_ORDER_TYPE
} eEFFECT_SCRIPT_ORDER_TYPE;

typedef struct _EFFECT_INETERFACE
{
	eEFFECT_SCRIPT_ORDER_TYPE order_type;
} EFFECT_INTERFACE;

typedef union _FX_TYPE_UNION
{
	int integer_value;
	float float_value;
} FX_TYPE_UNION;

typedef struct _FX_EFFECT
{
	ght_hash_table_t *annotations;
	ght_hash_table_t *techniques;
	ght_hash_table_t *passes;
	ght_hash_table_t *sources;
	ght_hash_table_t *parameters;
} FX_EFFECT;

#endif	// #ifndef _INCLUDED_EFFECT_H_
