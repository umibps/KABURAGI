#ifndef _INCLUDED_RENDER_ENGINE_H_
#define _INCLUDED_RENDER_ENGINE_H_

#include "model.h"
#include "shader_program.h"
#include "vertex.h"
#include "texture.h"
#include "effect.h"

typedef enum _eRENDER_ENGINE_TYPE
{
	RENDER_ENGINE_PMX,
	RENDER_ENGINE_ASSET
} eRENDER_ENGINE_TYPE;

typedef struct _RENDER_ENGINE_INTERFACE
{
	eRENDER_ENGINE_TYPE type;
	EFFECT_INTERFACE* (*get_effect)(void*, eEFFECT_SCRIPT_ORDER_TYPE);
	struct _MODEL_INTERFACE *model;
	void (*render_model)(void*);
	void (*render_shadow)(void*);
	void (*render_edge)(void*);
	void (*render_z_plot)(void*);
	void (*update)(void*);
	void (*prepare_post_process)(void*);
	void (*perform_pre_process)(void*);
	void (*delete_func)(void*);
} RENDER_ENGINE_INTERFACE;

typedef enum _ePMX_VERTEX_BUFFER_OBJECT_TYPE
{
	PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_EVEN,
	PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_ODD,
	PMX_VERTEX_BUFFER_OBJECT_TYPE_STATIC_VERTEX_BUFFER,
	PMX_VERTEX_BUFFER_OBJECT_TYPE_INDEX_BUFFER,
	MAX_PMX_VERTEX_BUFFER_OBJECT_TYPE
} ePMX_VERTEX_BUFFER_OBJECT_TYPE;

typedef enum _ePMX_VERTEX_ARRAY_OBJECT_TYPE
{
	PMX_VERTEX_ARRAY_OBJECT_TYPE_VERTEX_ARRAY_EVEN,
	PMX_VERTEX_ARRAY_OBJECT_TYPE_VERTEX_ARRAY_ODD,
	PMX_VERTEX_ARRAY_OBJECT_TYPE_EDGE_VERTEX_ARRAY_EVEN,
	PMX_VERTEX_ARRAY_OBJECT_TYPE_EDGE_VERTEX_ARRAY_ODD,
	MAX_PMX_VERTEX_ARRAY_OBJECT_TYPE
} ePMX_VERTEX_ARRAY_OBJECT_TYPE;

typedef enum _ePMX_RENDER_ENGINE_FLAGS
{
	PMX_RENDER_ENGINE_FLAG_CULL_FACE_STATE = 0x01,
	PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING = 0x02,
	PMX_RENDER_ENGINE_FLAG_UPDATE_EVEN = 0x04
} ePMX_RENDER_ENGINE_FLAGS;

typedef struct _PMX_RENDER_ENGINE_MATERIAL_TEXTURE
{
	TEXTURE_INTERFACE *main_texture;
	TEXTURE_INTERFACE *sphere_texture;
	TEXTURE_INTERFACE *toon_texture;
} PMX_RENDER_ENGINE_MATERIAL_TEXTURE;

typedef struct _PMX_RENDER_ENGINE
{
	RENDER_ENGINE_INTERFACE interface_data;
	struct _PROJECT *project_context;
	struct _SCENE *scene;
	MODEL_INDEX_BUFFER_INTERFACE *index_buffer;
	MODEL_STATIC_VERTEX_BUFFER_INTERFACE *static_buffer;
	MODEL_DYNAMIC_VERTEX_BUFFER_INTERFACE *dynamic_buffer;
	MODEL_MATRIX_BUFFER_INTERFACE *matrix_buffer;
	PMX_EDGE_PROGRAM edge_program;
	PMX_MODEL_PROGRAM model_program;
	PMX_SHADOW_PROGRAM shadow_program;
	PMX_EXTENDED_Z_PLOT_PROGRAM z_plot_program;
	VERTEX_BUNDLE *buffer;
	VERTEX_BUNDLE_LAYOUT *bundles[MAX_PMX_VERTEX_ARRAY_OBJECT_TYPE];
	GLenum index_type;
	ght_hash_table_t *allocated_textures;
	STRUCT_ARRAY *material_textures;
	float aa_bb_min[3];
	float aa_bb_max[3];
	unsigned int flags;
} PMX_RENDER_ENGINE;

typedef struct _ASSET_RENDER_ENGINE_VERTEX
{
	VECTOR4 position;
	VECTOR3 normal;
	VECTOR3 tex_coord;
} ASSET_RENDER_ENGINE_VERTEX;

typedef struct _ASSET_RENDER_ENGINE
{
	RENDER_ENGINE_INTERFACE interface_data;
	struct _SCENE *scene;
	struct _PROJECT *project;
	VERTEX_BUNDLE_LAYOUT *layout;
	ght_hash_table_t *textures;
	ght_hash_table_t *indices;
	ght_hash_table_t *vbo;
	ght_hash_table_t *vao;
	ght_hash_table_t *asset_programs;
	ght_hash_table_t *z_plot_programs;
	int cull_face_state;
} ASSET_RENDER_ENGINE;

extern void DeletePmxRenderEngine(PMX_RENDER_ENGINE* engine);
extern void PmxRenderEngineRenderModel(PMX_RENDER_ENGINE* engine);
extern void PmxRenderEngineRenderShadow(PMX_RENDER_ENGINE* engine);
extern void PmxRenderEngineRenderEdge(PMX_RENDER_ENGINE* engine);
extern void PmxRenderEngineRenderZPlot(PMX_RENDER_ENGINE* engine);
extern int PmxRenderEngineUpload(PMX_RENDER_ENGINE* engine, void* user_data);
extern void PmxRenderEngineUpdate(PMX_RENDER_ENGINE* engine);

extern EFFECT_INTERFACE* PmxRenderEngineGetEffect(PMX_RENDER_ENGINE* engine, eEFFECT_SCRIPT_ORDER_TYPE type);

extern int AssetRenderEngineUpload(ASSET_RENDER_ENGINE* engine, const char* directory);
extern void AssetRenderEngineRenderModel(ASSET_RENDER_ENGINE* engine);
extern EFFECT_INTERFACE* AssetRenderEngineGetEffect(ASSET_RENDER_ENGINE* engine, eEFFECT_SCRIPT_ORDER_TYPE type);

#endif	// #ifndef _INCLUDED_RENDER_ENGINE_H_
