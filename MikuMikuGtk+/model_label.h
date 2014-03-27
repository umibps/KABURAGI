#ifndef _INCLUDED_MODEL_LABEL_H_
#define _INCLUDED_MODEL_LABEL_H_

#include "types.h"
#include "utils.h"
#include "memory_stream.h"

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

#define PMD_BONE_CATEGORY_NAME_SIZE 50

typedef enum _ePMD2_MODEL_LABEL_TYPE
{
	PMD2_MODEL_LABEL_TYPE_SPECIAL_BONE_CATEGORY,
	PMD2_MODEL_LABEL_TYPE_BONE_CATEGORY,
	PMD2_MODEL_LABEL_TYPE_MORPH_CATEGORY,
	MAX_PMD2_MODEL_LABEL_TYPE
} ePMD2_MODEL_LABEL_TYPE;

typedef struct _PMD2_MODEL_LABEL
{
	LABEL_INTERFACE interface_data;
	ePMD2_MODEL_LABEL_TYPE type;
	POINTER_ARRAY *bones;
	POINTER_ARRAY *morphs;
	UINT32_ARRAY *bone_indices;
	UINT32_ARRAY *morph_indices;
	struct _PMD2_MODEL *model;
	struct _APPLICATION *application;
} PMD2_MODEL_LABEL;

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

extern int LoadPmd2ModelLabels(
	STRUCT_ARRAY* model_labels,
	STRUCT_ARRAY* bones,
	STRUCT_ARRAY* morphs
);

extern void ReadPmd2ModelLabel(
	PMD2_MODEL_LABEL* label,
	MEMORY_STREAM_PTR stream,
	size_t* data_size
);

#endif	// #ifndef _INCLUDED_MODEL_LABEL_H_
