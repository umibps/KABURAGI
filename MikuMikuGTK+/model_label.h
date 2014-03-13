#ifndef _INCLUDED_MODEL_LABEL_H_
#define _INCLUDED_MODEL_LABEL_H_

#include "types.h"
#include "utils.h"

#define DEFAULT_MODEL_LABEL_BUFFER_SIZE 16

typedef struct _LABEL_INTERFACE
{
	char *name;
	char *english_name;
	int index;
	int special;
	int (*get_count)(void*);
	void* (*get_bone)(void*, int);
} LABEL_INTERFACE;

typedef struct _PMX_MODEL_LABEL
{
	LABEL_INTERFACE interface_data;
	struct _PMX_MODEL *model;
	STRUCT_ARRAY *pairs;
} PMX_MODEL_LABEL;

typedef struct _ASSET_MODEL_LABEL
{
	LABEL_INTERFACE interface_data;
	struct _ASSET_MODEL *model;
	POINTER_ARRAY *bones;
} ASSET_MODEL_LABEL;

extern int LoadPmxModelLabels(
	STRUCT_ARRAY* labels,
	STRUCT_ARRAY* bones,
	STRUCT_ARRAY* morphs
);

extern void ReleasePmxModelLabel(PMX_MODEL_LABEL* label);

#endif	// #ifndef _INCLUDED_MODEL_LABEL_H_
