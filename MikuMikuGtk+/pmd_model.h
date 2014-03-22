#ifndef _INCLUDED_PMD_MODEL_H_
#define _INCLUDED_PMD_MODEL_H_

#include <stdio.h>
#include "types.h"
#include "utils.h"
#include "memory_stream.h"
#include "vertex.h"
#include "bone.h"
#include "model.h"

typedef struct _PMD_MODEL
{
	STRUCT_ARRAY *vertices;
	MODEL_DATA_INFO info;
} PMD_MODEL;

typedef struct _PMD2_MODEL
{
	STRUCT_ARRAY *vertices;
} PMD2_MODEL;

extern int Pmd2VertexPreparse(
	PMD2_VERTEX* vertex,
	MEMORY_STREAM_PTR stream,
	size_t* data_size,
	MODEL_DATA_INFO* info
);

extern void ReadPmd2Vertex(
	PMD2_VERTEX* vertex,
	MEMORY_STREAM_PTR stream,
	MODEL_DATA_INFO* info,
	size_t* data_size
);

extern int Pmd2BonePreparse(
	PMD2_BONE* bone,
	MEMORY_STREAM_PTR stream,
	MODEL_DATA_INFO* info
);

#endif	// #ifndef _INCLUDED_PMD_MODEL_H_
