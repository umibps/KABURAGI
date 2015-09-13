#ifndef _INCLUDED_PROJECT_H_
#define _INCLUDED_PROJECT_H_

#include "types.h"
#include "utils.h"
#include "scene.h"
#include "world.h"
#include "motion.h"
#include "pose.h"
#include "annotation.h"
#include "shadow_map.h"
#include "debug_drawer.h"
#include "pmx_model.h"
#include "pmd_model.h"
#include "asset_model.h"
#include "grid.h"
#include "history.h"
#include "control.h"
#include "ui.h"

typedef enum _ePROJECT_FLAGS
{
	PROJECT_FLAG_DRAW_GRID = 0x01,
	PROJECT_FLAG_GL_INTIALIZED = 0x02,
	PROJECT_FLAG_UPDATE_ALWAYS = 0x04,
	PROJECT_FLAG_ENABLE_BONE_MOVE = 0x08,
	PROJECT_FLAG_ENABLE_BONE_ROTATE = 0x10,
	PROJECT_FLAG_SHOW_MODEL_DIALOG = 0x20,
	PROJECT_FLAG_LOCK_TOUCH_EVENT = 0x40,
	PROJECT_FLAG_ENABLE_GESTURES = 0x80,
	PROJECT_FLAG_ENABLE_MOVE_GESTURE = 0x100,
	PROJECT_FLAG_ENABLE_ROTATE_GESTURE = 0x200,
	PROJECT_FLAG_ENABLE_SCALE_GESTURE = 0x400,
	PROJECT_FLAG_ENABLE_UNDO_GESTURE = 0x800,
	PROJECT_FLAG_ENABLE_UPDATE_GL = 0x1000,
	PROJECT_FLAG_ENABLE_DEBUG_DRAWING = 0x2000,
	PROJECT_FLAG_IS_IMAGE_HANDLE_RECT_INTERSECT = 0x4000,
	PROJECT_FLAG_ALWAYS_PHYSICS = 0x8000,
	PROJECT_FLAG_RENDER_SHADOW = 0x10000,
	PROJECT_FLAG_RENDER_EDGE_ONLY = 0x20000,
	PROJECT_FLAG_MARK_DIRTY = 0x40000
} ePROJECT_FLAGS;

typedef enum _eSHADER_TYPE
{
	SHADER_TYPE_EDGE_VERTEX,
	SHADER_TYPE_EDGE_FRAGMENT,
	SHADER_TYPE_MODEL_VERTEX,
	SHADER_TYPE_MODEL_FRAGMENT,
	SHADER_TYPE_SHADOW_VERTEX,
	SHADER_TYPE_SHADOW_FRAGMENT,
	SHADER_TYPE_Z_PLOT_VERTEX,
	SHADER_TYPE_Z_PLOT_FRAGMENT,
	SHADER_TYPE_EDGE_WITH_SKINNING_VERTEX,
	SHADER_TYPE_MODEL_WITH_SKINNING_VERTEX,
	SHADER_TYPE_SHADOW_WITH_SKINNING_VERTEX,
	SHADER_TYPE_Z_PLOT_WITH_SKINNING_VERTEX,
	SHADER_TYPE_MODEL_EFFECT_TECHNIQUES,
	SHADER_TYPE_TRANSFORM_FEEDBACK_VERTEX,
	SHADER_TYPE_SHAPE_MODEL_VERTEX,
	SHADER_TYPE_SHAPE_MODEL_FRAGMENT,
	SHADER_TYPE_SHAPE_EDGE_VERTEX,
	SHADER_TYPE_SHAPE_EDGE_FRAGMENT,
	MAX_SHADER_TYPE
} eSHADER_TYPE;

typedef enum _eMATRIX_TYPE_FLAGS
{
	MATRIX_FLAG_WORLD_MATRIX = 0x001,
	MATRIX_FLAG_VIEW_MATRIX = 0x002,
	MATRIX_FLAG_PROJECTION_MATRIX = 0x004,
	MATRIX_FLAG_INVERSE_MATRIX = 0x008,
	MATRIX_FLAG_TRANSPOSE_MATRIX = 0x010,
	MATRIX_FLAG_CAMERA_MATRIX = 0x020,
	MATRIX_FLAG_LIGHT_MATRIX = 0x040,
	MATRIX_FLAG_SHADOW_MATRIX = 0x080
} eMATRIX_TYPE_FLAGS;

typedef struct _MAP_BUFFER
{
	uint8 *address;
	size_t size;
	int *opaque;
} MAP_BUFFER;

typedef struct _PROJECT
{
	WORLD world;
	SCENE *scene;
	POINTER_ARRAY *motions;
	SHADOW_MAP *shadow_map;
	DEBUG_DRAWER debug_drawer;
	GRID grid;
	CONTROL control;
	PROJECT_WIDGETS widgets;
	HISTORY history;

	ght_hash_table_t *texture_chache_map;

	eSHADER_TYPE shader_type;
	MAP_BUFFER map_buffer;
	void *timer;

	float camera_projection_matrix[16];
	float camera_view_matrix[16];
	float camera_world_matrix[16];
	float light_projection_matrix[16];
	float light_view_matrix[16];
	float light_world_matrix[16];

	float view_projection_matrix[16];

	float view_port[2];

	int debug_flags;

	float aspect_ratio;

	unsigned int flags;

	PMX_DEFAULT_BUFFER_IDENT pmx_default_buffer_ident;
	PMD2_DEFAULT_BUFFER_IDENT pmd2_default_buffer_ident;

	char *file_path;

	int original_width,	original_height;

	struct _APPLICATION *application_context;
} PROJECT;

#ifdef __cplusplus
extern "C" {
#endif

extern PROJECT* ProjectNew(
	void* application_context,
	int widget_width,
	int widget_height,
	unsigned int flags
);

extern void LoadProject(
	PROJECT* project,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int)
);

extern void SaveProject(
	PROJECT* project,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
);

extern void InitializeScene(
	SCENE* scene,
	int width,
	int height,
	void* world,
	PROJECT* project
);

extern void GetContextMatrix(float *value, MODEL_INTERFACE* model, int flags, PROJECT* project);

extern void RenderEngines(PROJECT* project, int width, int height);

extern void DisplayProject(PROJECT* project, int width, int height);

extern RENDER_ENGINE_INTERFACE* SceneCreateRenderEngine(
	SCENE* scene,
	eRENDER_ENGINE_TYPE type,
	MODEL_INTERFACE* model,
	unsigned int flags,
	PROJECT* project
);

extern PMX_RENDER_ENGINE* PmxRenderEngineNew(
	PROJECT* project_context,
	SCENE* scene,
	MODEL_INTERFACE* model,
	int is_vertex_skinning
);

extern ASSET_RENDER_ENGINE* AssetRenderEngineNew(
	PROJECT* project_context,
	SCENE* scene,
	MODEL_INTERFACE* model
);

extern SHAPE_RENDER_ENGINE* ShapeRenderEngineNew(
	PROJECT* project_context,
	SCENE* scene,
	MODEL_INTERFACE* model
);

extern void ResizeProject(PROJECT* project, int available_width, int available_height);

extern void MakeRay(PROJECT* project, FLOAT_T mouse_x, FLOAT_T mouse_y, float *ray_from, float *ray_to);
extern int RayAabb(
	const float ray_from[3],
	const float ray_to[3],
	const float aa_bb_min[3],
	const float aa_bb_max[3],
	float* param,
	float normal[3]
);

extern void LoadControlHandle(CONTROL_HANDLE* handle, PROJECT* project);
extern void InitializeControl(
	CONTROL* control,
	int width,
	int height,
	PROJECT* project
);

extern void ChangeSelectedBones(PROJECT* project, BONE_INTERFACE** bones, int num_selected);

extern int TestHitModelHandle(PROJECT* project, FLOAT_T x, FLOAT_T y);

extern void ProjectSetEnableAlwaysPhysics(PROJECT* project, int enabled);

extern void AddBoneMoveHistory(HISTORY* history, PROJECT* project);

extern void AddBoneRotateHistory(HISTORY* history, PROJECT* project);

extern int FindTextureCache(const char* path, TEXTURE_DATA_BRIDGE* bridge, PROJECT* project);

extern int UploadTexture(const char* path, TEXTURE_DATA_BRIDGE* bridge, PROJECT* project);

extern int UploadTextureFromMemory(
	uint8* pixels,
	int width,
	int height,
	int channel,
	const char* path,
	TEXTURE_DATA_BRIDGE* bridge,
	PROJECT* project
);

extern int UploadWhiteTexture(int width, int height, PROJECT* project);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_PROJECT_H_
