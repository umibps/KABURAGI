#ifndef _INCLUDED_MORPH_H_
#define _INCLUDED_MORPH_H_

#include "types.h"
#include "utils.h"
#include "memory_stream.h"

#define MORPH_BUFFER_SIZE 8

typedef enum _eMORPH_TYPE
{
	MORPH_TYPE_UNKNOWN = -1,
	MORPH_TYPE_GROUP,
	MORPH_TYPE_VERTEX,
	MORPH_TYPE_BONE,
	MORPH_TYPE_TEXTURE_COORD,
	MORPH_TYPE_UVA1,
	MORPH_TYPE_UVA2,
	MORPH_TYPE_UVA3,
	MORPH_TYPE_UVA4,
	MORPH_TYPE_MATERIAL,
	MORPH_TYPE_FLIP,
	MORPH_TYPE_IMPULSE,
	MAX_MORPH_TYPE
} eMORPH_TYPE;

typedef enum _eMORPH_CATEGORY
{
	MORPH_CATEGORY_BASE,
	MORPH_CATEGORY_EYEBLOW,
	MORPH_CATEGORY_EYE,
	MORPH_CATEGORY_LIP,
	MORPH_CATEGORY_OTHER,
	MAX_MORPH_CATEGORY_TYPE
} eMORPH_CATEGORY;

typedef struct _MORPH_BONE
{
	struct _BONE_INTERFACE *bone;
	float position[3];
	QUATERNION rotation;
	int index;
} MORPH_BONE;

typedef struct _MORPH_GROUP
{
	void *morph;
	FLOAT_T fixed_weight;
	int index;
} MORPH_GROUP;

typedef struct _MORPH_MATERIAL
{
	POINTER_ARRAY *materials;
	float ambient[3];
	float diffuse[4];
	float specular[3];
	float edge_color[4];
	float texture_weight[4];
	float sphere_texture_weight[4];
	float toon_texture_weight[4];
	float shininess;
	FLOAT_T edge_size;
	int index;
	uint8 operation;
} MORPH_MATERIAL;

typedef struct _MORPH_UV
{
	struct _VERTEX_INTERFACE *vertex;
	float position[4];
	uint32 index;
	int offset;
} MORPH_UV;

typedef struct _MORPH_VERTEX
{
	struct _VERTEX_INTERFACE *vertex;
	float position[3];
	uint32 index;
	uint32 base;
} MORPH_VERTEX;

typedef struct _MORPH_FLIP
{
	struct _MORPH_INTERFACE *morph;
	FLOAT_T fixed_weight;
	int index;
} MORPH_FLIP;

typedef struct _MORPH_IMPULSE
{
	struct _BASE_RIGID_BODY *rigid_body;
	float velocity[3];
	float torque[3];
	int index;
	int is_local;
} MORPH_IMPULSE;

typedef struct _MORPH_INTERFACE
{
	int index;
	char *name;
	char *english_name;
	eMORPH_CATEGORY category;
	eMORPH_TYPE type;
	FLOAT_T weight;
	void (*set_weight)(void*, FLOAT_T);
} MORPH_INTERFACE;

typedef enum _ePMX_MORPH_FLAGS
{
	PMX_MORPH_FLAG_HAS_PARENT = 0x01,
	PMX_MORPH_FLAG_DIRTY = 0x02
} ePMX_MORPH_FLAGS;

typedef struct _PMX_MORPH
{
	MORPH_INTERFACE interface_data;
	STRUCT_ARRAY *vertices;
	STRUCT_ARRAY *uvs;
	STRUCT_ARRAY *bones;
	STRUCT_ARRAY *materials;
	STRUCT_ARRAY *groups;
	STRUCT_ARRAY *flips;
	STRUCT_ARRAY *impulses;
	struct _PMX_MODEL *parent_model;
	FLOAT_T internal_weight;
	FLOAT_T last_update_weight;
	int flags;
} PMX_MORPH;

#define PMD_MORPH_NAME_SIZE 20

typedef struct _PMD2_MORPH
{
	MORPH_INTERFACE interface_data;
	struct _PMD2_MODEL *model;
	eMORPH_CATEGORY category;
	STRUCT_ARRAY *vertices;
	POINTER_ARRAY *vertex_refs;
	struct _APPLICATION *application;
} PMD2_MORPH;

typedef struct _ASSET_OPACITY_MORPH
{
	MORPH_INTERFACE interface_data;
	struct _ASSET_MODEL *model;
	FLOAT_T opacity;
	struct _APPLICATION *application;
} ASSET_OPACITY_MORPH;

#ifdef __cplusplus
extern "C" {
#endif

extern int PmxMorphLoad(
	STRUCT_ARRAY* morphs,
	STRUCT_ARRAY* bones,
	STRUCT_ARRAY* materials,
	STRUCT_ARRAY* rigid_bodies,
	STRUCT_ARRAY* vertices
);

extern void ReleasePmxMorph(PMX_MORPH* morph);

extern void PmxMorphUpdate(PMX_MORPH* morph);

extern void PmxMorphSyncWeight(PMX_MORPH* morph);

extern void PmxMorphSetInternalWeight(PMX_MORPH* morph, const FLOAT_T weight);

extern int LoadPmd2Morphs(STRUCT_ARRAY *morphs, STRUCT_ARRAY* vertices);

extern void ReadPmd2Morph(PMD2_MORPH* morph, MEMORY_STREAM_PTR stream, size_t* data_size);;

extern void Pmd2MorphReadEnglishName(PMD2_MORPH* morph, uint8* data, int index);

extern void Pmd2MorphUpdate(PMD2_MORPH* morph);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_MORPH_H_
