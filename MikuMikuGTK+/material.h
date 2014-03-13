#ifndef _INCLUDED_MATERIAL_H_
#define _INCLUDED_MATERIAL_H_

#include <assimp/material.h>
#include "model.h"
#include "types.h"
#include "utils.h"

typedef enum _eMATERIAL_SPHERE_TEXTURE_RENDER_MODE
{
	MATERIAL_RENDER_NONE,
	MATERIAL_MULTI_TEXTURE,
	MATERIAL_ADD_TEXTURE,
	MATERIAL_SUB_TEXTURE,
	MAX_MATERIAL_SPHERE_TEXTURE_RENDER_MODE
} eMATERIAL_SPHERE_TEXTURE_RENDER_MODE;

typedef struct _MATERIAL_INTERFACE
{
	char *name;
	char *english_name;
	int index;
	char *main_texture;
	char *sphere_texture;
	char *toon_texture;
	eMATERIAL_SPHERE_TEXTURE_RENDER_MODE sphere_texture_render_mode;
	struct _MATERIAL_INDEX_RANGE (*get_index_range)(void*);
	void (*set_index_range)(void*, struct _MATERIAL_INDEX_RANGE*);
	int (*get_toon_texture_index)(void*);
	FLOAT_T (*get_edge_size)(void*);
	void (*get_edge_color)(void*, float*);
	void (*get_ambient)(void*, float*);
	void (*get_diffuse)(void*, float*);
	void (*get_specular)(void*, float*);
	float (*get_shininess)(void*);
	void (*get_main_texture_blend)(void*, float*);
	void (*get_sphere_texture_blend)(void*, float*);
	void (*get_toon_texture_blend)(void*, float*);
	int (*has_shadow)(void*);
	int (*has_shadow_map)(void*);
	int (*is_self_shadow_enabled)(void*);
	int (*is_edge_enabled)(void*);
	unsigned int flags;
} MATERIAL_INTERFACE;

typedef struct _DEFAULT_MATERIAL
{
	MATERIAL_INTERFACE interface_data;
	struct _APPLICATION *application_context;
} DEFAULT_MATERIAL;

typedef enum _eMATERIAL_FLAGS
{
	MATERIAL_FLAG_DISABLE_CULLING = 0x01,
	MATERIAL_FLAG_HAS_SHADOW = 0x02,
	MATERIAL_FLAG_HAS_SHADOW_MAP = 0x04,
	MATERIAL_FLAG_ENABLE_SELF_SHADOW = 0x08,
	MATERIAL_FLAG_ENABLE_EDGE = 0x10,
	MATERIAL_FLAG_HAS_VERTEX_COLOR = 0x20,
	MATERIAL_FLAG_ENABLE_POINAT_DRAW = 0x40,
	MATERIAL_FLAG_ENABLE_LINE_DRAW = 0x80,
	MATERIAL_FLAG_USE_SHARED_TOON_TEXTURE = 0x100,
	MAX_MATERIAL_FLAGS = 0x200
} eMATERIAL_FLAGS;

typedef struct _MATERIAL_INDEX_RANGE
{
	int start;
	int end;
	int count;
} MATERIAL_INDEX_RANGE;

typedef struct _MATERIAL_RGB3
{
	float result[4];
	float base[3];
	float multi[3];
	float add[3];
} MATERIAL_RGB3;

typedef struct _MATERIAL_RGBA3
{
	float result[4];
	float base[4];
	float multi[4];
	float add[4];
} MATERIAL_RGBA3;

typedef struct _MATERIAL
{
	MODEL *model;
	struct _MATERIAL *material;
	char *main_texture;
	char *sphere_texture;
	char *toon_texture;
	eMATERIAL_SPHERE_TEXTURE_RENDER_MODE sphere_texture_render_mode;
	uint8 diffuse[3];
	MATERIAL_INDEX_RANGE index_range;
	int toon_texture_index;
} MATERIAL;

typedef struct _PMX_MATERIAL
{
	MATERIAL_INTERFACE interface_data;
	struct _PMX_MODEL *model;
	char *user_data_area;
	MATERIAL_RGB3 ambient;
	MATERIAL_RGBA3 diffuse;
	MATERIAL_RGB3 specular;
	MATERIAL_RGBA3 edge_color;
	MATERIAL_RGBA3 main_texture_blend;
	MATERIAL_RGBA3 sphere_texture_blend;
	MATERIAL_RGBA3 toon_texture_blend;
	MATERIAL_INDEX_RANGE index_range;
	float shininess[3];
	float edge_size[3];
	int main_texture_index;
	int sphere_texture_index;
	int toon_texture_index;
} PMX_MATERIAL;

typedef struct _ASSET_MATERIAL
{
	MATERIAL_INTERFACE interface_data;
	struct aiMaterial *material;
	struct _ASSET_MODEL *model;
	struct _APPLICATION *application;
	char *main_texture;
	char *sphere_texture;
	eMATERIAL_SPHERE_TEXTURE_RENDER_MODE sphere_texture_render_mode;
	VECTOR4 ambient;
	VECTOR4 diffuse;
	VECTOR4 specular;
	float shininess;
	int num_indices;
} ASSET_MATERIAL;

extern void InitializePmxMaterial(PMX_MATERIAL* material);

extern int LoadPmxMaterials(
	STRUCT_ARRAY* materials,
	POINTER_ARRAY* textures,
	int expected_indices
);

#endif	// #ifndef _INCLUDED_MATERIAL_H_