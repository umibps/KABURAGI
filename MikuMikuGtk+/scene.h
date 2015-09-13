#ifndef _INCLUDED_SCENE_H_
#define _INCLUDED_SCENE_H_

#include "ght_hash_table.h"
#include "model.h"
#include "motion.h"
#include "light.h"
#include "camera.h"
#include "utils.h"
#include "render_engine.h"
#include "shadow_map.h"
#include "pmx_model.h"
#include "pmd_model.h"
#include "asset_model.h"
#include "shape_model.h"
#include "keyframe.h"

typedef enum _eSCENE_ACCELERATION_TYPE
{
	SCENE_ACCELERATION_TYPE_SOFTWARE_FALLBACK,
	SCENE_ACCELERATION_TYPE_OPENCL_ACCELERATION1,
	SCENE_ACCELERATION_TYPE_VERTEX_SHADER_ACCELERATION1,
	SCENE_ACCELERATION_TYPE_OPENCL_ACCELERATION2,
	MAX_SCENE_ACCELERATION_TYPE
} eSCENE_ACCELERATION_TYPE;

typedef enum _eSCENE_FLAGS
{
	SCENE_FLAG_INITIALIZED = 0x01,
	SCENE_FLAG_RENDER_ENGINE_EFFECT_CAPABLE = 0x02,
	SCENE_FLAG_UPDATE_MODELS = 0x04,
	SCENE_FLAG_UPDATE_IK = 0x08,
	SCENE_FLAG_UPDATE_RENDER_ENGINES = 0x10,
	SCENE_FLAG_UPDATE_CAMERA = 0x20,
	SCENE_FLAG_UPDATE_LIGHT = 0x40,
	SCENE_FLAG_MODEL_CONTROLLED = 0x80,
	SCENE_FLAG_UPDATE_ALL = SCENE_FLAG_UPDATE_MODELS | SCENE_FLAG_UPDATE_RENDER_ENGINES
		| SCENE_FLAG_UPDATE_IK | SCENE_FLAG_UPDATE_CAMERA | SCENE_FLAG_UPDATE_LIGHT | SCENE_FLAG_MODEL_CONTROLLED,
	SCENE_FLAG_RESET_MOTION_STATE = 0x100,
	SCENE_FLAG_FORCE_UPDATE_ALL_MORPHS = 0x200
} eSCENE_FLAGS;

typedef struct _SCENE_MODEL
{
	void *model;
	int priority;
} SCENE_MODEL;

typedef struct _SCENE_ENGINE
{
	void *engine;
	int priority;
} SCENE_ENGINE;

typedef struct _SCENE_MOTION
{
	MOTION_INTERFACE *model;
	int priority;
} SCENE_MOTION;

typedef struct _RENDER_ENGINE
{
	MODEL *model;
	MODEL *current_engine;
	eRENDER_ENGINE_TYPE engine_type;
	void *engine_data;
	void (*render_zplot)(void* engine_data);
	int priority;
} RENDER_ENGINE;

typedef struct _RENDER_ENGINES
{
	RENDER_ENGINE_INTERFACE **pre_process;
	int num_pre_process;
	RENDER_ENGINE_INTERFACE **standard;
	int num_standard;
	RENDER_ENGINE_INTERFACE **post_process;
	int num_post_process;
	size_t buffer_size;
} RENDER_ENGINES;

typedef struct _PLANE
{
	void *world;
	void *body;
	void *shape;
	void *state;
	struct _SCENE *scene;
} PLANE;

typedef struct _SCENE
{
	POINTER_ARRAY *models;
	POINTER_ARRAY *motions;
	LIGHT light;
	CAMERA camera;
	PLANE plane;
	void *world;
	SHADOW_MAP_INTERFACE *shadow_map;
	QUATERNION rotation;
	POINTER_ARRAY *engines;
	ght_hash_table_t *model2engine;
	ght_hash_table_t *name2model;
	eSCENE_ACCELERATION_TYPE acceleration_type;
	RENDER_ENGINES render_engines;
	POINTER_ARRAY *selected_bones;
	MODEL_INTERFACE *selected_model;

	FLOAT_T current_time_index;

	float projection[16];

	int width, height;
	uint8 clear_color[3];

	struct _PROJECT *project;

	unsigned int flags;
} SCENE;

#ifdef __cplusplus
extern "C" {
#endif

extern void SetRequiredOpengGLState(SCENE* scene);

extern void UpdateScene(SCENE* scene);

extern void SceneUpdateProjection(SCENE* scene);

extern void SceneUpdateModel(SCENE* scene, MODEL_INTERFACE* model, int force_physics);

extern void InitializePmxModel(
	PMX_MODEL* model,
	SCENE* scene,
	TEXT_ENCODE* encode,
	const char* model_path
);

extern void InitializeCuboidModel(
	CUBOID_MODEL* model,
	SCENE* scene
);

extern void InitializeConeModel(
	CONE_MODEL* model,
	SCENE* scene
);

extern void SceneGetRenderEnginesByRenderOrder(SCENE* scene);

extern BONE_INTERFACE* SceneFindNearestBone(
	SCENE* secne,
	MODEL_INTERFACE* model,
	const float z_near[3],
	const float z_far[3],
	const float threshold
);

extern int SceneIntersectsBone(
	SCENE* scene,
	BONE_INTERFACE* bone,
	const float z_near[3],
	const float z_far[3],
	const float threshold
);

extern void SceneTranslate(SCENE* scene, float* delta);

extern void SceneRotate(SCENE* scene, float* delta);

extern void SceneRemoveModel(SCENE* scene, MODEL_INTERFACE* model);

extern void SceneSetEmptyMotion(SCENE* scene, MODEL_INTERFACE* model);

extern KEYFRAME_INTERFACE* SceneGetCurrentKeyframe(
	SCENE* scene,
	eKEYFRAME_TYPE keyframe_type,
	void* detail_data
);

extern void InitializePmd2Model(PMD2_MODEL* model, SCENE* scene, const char* model_path);

extern void InitializeAssetModel(
	ASSET_MODEL* model,
	SCENE* scene,
	void* application_context
);

extern int PlaneTest(PLANE* plane, float* from, float* to, float* hit);

extern BONE_KEYFRAME_INTERFACE* BoneKeyframeNew(
	MOTION_INTERFACE* motion,
	SCENE* scene
);

extern BONE_KEYFRAME_INTERFACE* VmdBoneKeyframeNew(SCENE* scene);

extern MOTION_INTERFACE* NewMotion(eMOTION_TYPE type, SCENE* scene, MODEL_INTERFACE* model);

extern MOTION_INTERFACE* VmdMotionNew(
	SCENE* scene,
	MODEL_INTERFACE* model
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_SCENE_H_
