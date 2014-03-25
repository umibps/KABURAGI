#ifndef _INCLUDED_PMD_MODEL_H_
#define _INCLUDED_PMD_MODEL_H_

#include <stdio.h>
#include "ght_hash_table.h"
#include "types.h"
#include "utils.h"
#include "memory_stream.h"
#include "vertex.h"
#include "bone.h"
#include "material.h"
#include "model_label.h"
#include "morph.h"
#include "rigid_body.h"
#include "joint.h"
#include "ik.h"
#include "model.h"

#define PMD_MODEL_NAME_SIZE 20
#define PMD_MODEL_COMMEMT_SIZE 256
#define PMD_MODEL_CUSTOM_TOON_TEXTURE_NAME_SIZE 100
#define PMD_MODEL_MAX_CUSTOM_TOON_TEXTURES 10

#define PMD_MODEL_DEFAULT_BUFFER_SIZE 32

typedef struct _PMD_MODEL
{
	MODEL_INTERFACE interface_data;
	STRUCT_ARRAY *vertices;
	MODEL_DATA_INFO info;
} PMD_MODEL;

typedef enum _ePMD2_MODEL_FLAGS
{
	PMD2_MODEL_FLAG_HAS_ENGLISH = 0x01,
	PMD2_MODEL_FLAG_VISIBLE = 0x02
} ePMD2_MODEL_FLAGS;

typedef struct _PMD2_MODEL
{
	MODEL_INTERFACE interface_data;
	STRUCT_ARRAY *vertices;
	UINT32_ARRAY *indices;
	ght_hash_table_t *textures;
	STRUCT_ARRAY *materials;
	STRUCT_ARRAY *bones;
	STRUCT_ARRAY *labels;
	STRUCT_ARRAY *rigid_bodies;
	STRUCT_ARRAY *joints;
	STRUCT_ARRAY *constraints;
	POINTER_ARRAY *custom_toon_textures;
	POINTER_ARRAY *sorted_bones;
	ght_hash_table_t *name2bone;
	ght_hash_table_t *name2morph;
	VECTOR3 position;
	QUATERNION rotation;
	float edge_color[4];
	VECTOR3 aa_bb_max;
	VECTOR3 aa_bb_min;
	unsigned int flags;
	MODEL_DATA_INFO info;
} PMD2_MODEL;

extern void InitializePmd2Vertex(
	PMD2_VERTEX* vertex, 
	PMD2_MODEL* model,
	MATERIAL_INTERFACE* default_material
);

extern int Pmd2VertexPreparse(
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
	MEMORY_STREAM_PTR stream,
	MODEL_DATA_INFO* info
);

extern void InitializePmd2Material(
	PMD2_MATERIAL* material,
	PMD2_MODEL* model,
	void* application_context
);

extern int Pmd2MaterialPreparse(
	MEMORY_STREAM_PTR stream,
	size_t* data_size,
	MODEL_DATA_INFO* info
);

extern void InitializePmd2Morph(
	PMD2_MORPH *morph,
	PMD2_MODEL *model,
	void *application_context
);

extern int Pmd2MorphPreparse(
	MEMORY_STREAM_PTR stream,
	size_t* data_size,
	MODEL_DATA_INFO* info
);

extern void InitializePmd2ModelLabel(
	PMD2_MODEL_LABEL* label,
	PMD2_MODEL* model,
	const char* name,
	ePMD2_MODEL_LABEL_TYPE type,
	void* application_context
);

extern int Pmd2ModelLabelPreparse(
	MEMORY_STREAM_PTR stream,
	MODEL_DATA_INFO* info
);

extern int Pmd2RigidBodyPreparse(
	MEMORY_STREAM_PTR stream,
	size_t* data_size,
	MODEL_DATA_INFO* info
);

extern void ReadPmd2RigidBody(
	PMD2_RIGID_BODY* body,
	MEMORY_STREAM_PTR stream,
	MODEL_DATA_INFO* info,
	size_t* data_size
);

extern int Pmd2JointPreparse(MEMORY_STREAM_PTR stream, MODEL_DATA_INFO* info);

extern void ReadPmd2Joint(PMD2_JOINT* joint, MEMORY_STREAM_PTR stream, size_t* data_size, MODEL_DATA_INFO* info);

extern void Pmd2ModelAddTexture(PMD2_MODEL* model, const char* texture);

#endif	// #ifndef _INCLUDED_PMD_MODEL_H_
