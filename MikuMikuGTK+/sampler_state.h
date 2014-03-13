#ifndef _INCLUDED_SAMPLER_STATE_H_
#define _INCLUDED_SAMPLER_STATE_H_

typedef enum _eSAMPLER_TYPE
{
	SAMPLER_TYPE_NV,
	SAMPLER_TYPE_CF,
	NUM_SAMPLER_TYPE
} eSAMPLER_TYPE;

typedef struct _NV_SAMPLER_STATE
{
	struct _EFFECT *effect;
	struct _PARAMETER *value;
} NV_SAMPLER_STATE;

typedef struct _CF_STATE
{
	char *name;
	char *value;
} CF_STATE;

typedef struct _SAMPLER_STATE
{
	eSAMPLER_TYPE type;
	union
	{
		NV_SAMPLER_STATE nv;
		CF_STATE cf;
	} state;
} SAMPLER_STATE;

#endif	// #ifndef _INCLUDED_SAMPLER_STATE_H_
