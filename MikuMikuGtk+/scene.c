// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include "scene.h"
#include "project.h"
#include "bullet.h"
#include "application.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void SetRequiredOpengGLState(void)
{
	int num_attribs;
	int i;

	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_attribs);
	for(i=0; i<num_attribs; i++)
	{
		glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glDisableVertexAttribArray(i);
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glStencilMask(0xff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_ALWAYS, 0, 0xff);

	glUseProgram(0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_STENCIL_TEST);
/*
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_INDEX_ARRAY);
*/
	// Á‹ŽF‚ðÝ’è
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void ResizeSceneRenderEngines(SCENE* scene, size_t new_size)
{
	scene->render_engines.buffer_size = new_size;
	scene->render_engines.pre_process =
		(RENDER_ENGINE_INTERFACE**)MEM_REALLOC_FUNC(
			scene->render_engines.pre_process, sizeof(*scene->render_engines.pre_process) * new_size);
	scene->render_engines.standard =
		(RENDER_ENGINE_INTERFACE**)MEM_REALLOC_FUNC(
			scene->render_engines.standard, sizeof(*scene->render_engines.standard) * new_size);
	scene->render_engines.post_process =
		(RENDER_ENGINE_INTERFACE**)MEM_REALLOC_FUNC(
			scene->render_engines.post_process, sizeof(*scene->render_engines.post_process) * new_size);
}

void InitializePlane(PLANE* plane, SCENE* scene)
{
#define PLANE_AA_BB_SIZE 100000.0f
	float size[] = {PLANE_AA_BB_SIZE, PLANE_AA_BB_SIZE, 0.01f};
	const float local_inertia[3] = {0, 0, 0};
	plane->state = BtDefaultMotionStateNew();
	plane->shape = BtBoxShapeNew(size);
	size[2] = PLANE_AA_BB_SIZE;
	plane->world = BtDynamicsWorldNew(size);
	plane->body = BtRigidBodyNew(0.0f, plane->state, plane->shape, local_inertia);
	BtDynamicsWorldAddRigidBody(plane->world, plane->body);
	plane->scene = scene;
}

void UpdatePlaneTransform(
	PLANE* plane,
	void* transform
)
{
	BtRigidBodySetWorldTransform(plane->body, transform);
	BtDynamicsWorldStepSimulationSimple(plane->world, 1);
}

void UpdatePlane(
	PLANE* plane,
	CAMERA* camera
)
{
	const float origin[] = {0, 0, 0};
	void *transform = plane->scene->project->application_context->transform;
	BtTransformSet(transform, camera->transform);
	BtTransformInverse(transform);
	BtTransformSetOrigin(transform, origin);
	UpdatePlaneTransform(plane, transform);
}

void InitializeScene(
	SCENE* scene,
	int width,
	int height,
	void* world,
	PROJECT* project
)
{
	(void)memset(scene, 0, sizeof(*scene));
	InitializeLight(&scene->light, (void*)scene);
	InitializeCamera(&scene->camera, (void*)scene);

	SetRequiredOpengGLState();

	if(scene->models != NULL)
	{
		PointerArrayDestroy(&scene->models, NULL);
	}
	scene->models = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	if(scene->motions != NULL)
	{
		PointerArrayDestroy(&scene->motions, NULL);
	}
	scene->motions = PointerArrayNew(DEFAULT_BUFFER_SIZE);

	if(scene->engines != NULL)
	{
		PointerArrayDestroy(&scene->engines, NULL);
	}
	scene->engines = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	ResizeSceneRenderEngines(scene, DEFAULT_BUFFER_SIZE);

	InitializePlane(&scene->plane, scene);
	scene->world = world;

	scene->selected_bones = PointerArrayNew(DEFAULT_BUFFER_SIZE);

	scene->rotation[3] = 1;

	scene->width = width;
	scene->height = height;

	scene->project = project;

	scene->flags |= SCENE_FLAG_INITIALIZED;
}

RENDER_ENGINE_INTERFACE* SceneCreateRenderEngine(
	SCENE* scene,
	eRENDER_ENGINE_TYPE type,
	MODEL_INTERFACE* model,
	unsigned int flags,
	PROJECT* project
)
{
	RENDER_ENGINE_INTERFACE* ret = NULL;

	if(model != NULL)
	{
		switch(type)
		{
		case RENDER_ENGINE_PMX:
			ret = (RENDER_ENGINE_INTERFACE*)PmxRenderEngineNew(project, scene, model,
				scene->acceleration_type == SCENE_ACCELERATION_TYPE_VERTEX_SHADER_ACCELERATION1);
			if(ret != NULL)
			{
				if(PmxRenderEngineUpload((PMX_RENDER_ENGINE*)ret, scene->project->application_context) == FALSE)
				{
					return NULL;
				}
			}
			break;
		case RENDER_ENGINE_ASSET:
			ret = (RENDER_ENGINE_INTERFACE*)AssetRenderEngineNew(project, scene, model);
			if(ret != NULL)
			{
				if(AssetRenderEngineUpload((ASSET_RENDER_ENGINE*)ret,
					((ASSET_MODEL*)model)->model_path) == FALSE)
				{
					return NULL;
				}
			}
		}
	}

	return ret;
}

void UpdateScene(SCENE* scene)
{
	if((scene->flags & SCENE_FLAG_UPDATE_MODELS) != 0)
	{
		MODEL_INTERFACE **models = (MODEL_INTERFACE**)scene->models->buffer;
		const int num_models = (int)scene->models->num_data;
		int i;
		for(i=0; i<num_models; i++)
		{
			models[i]->perform_update(models[i],
				scene->flags & SCENE_FLAG_MODEL_CONTROLLED | (scene->project->flags & PROJECT_FLAG_ALWAYS_PHYSICS));
		}
	}

	if((scene->flags & SCENE_FLAG_MODEL_CONTROLLED) != 0)
	{
		MODEL_INTERFACE **models = (MODEL_INTERFACE**)scene->models->buffer;
		const int num_models = (int)scene->models->num_data;
		int i;
		for(i=0; i<num_models; i++)
		{
			models[i]->reset_motion_state(models[i], scene->world);
		}
	}

	if((scene->flags & SCENE_FLAG_UPDATE_RENDER_ENGINES) != 0)
	{
		RENDER_ENGINE_INTERFACE **engines = (RENDER_ENGINE_INTERFACE**)scene->engines->buffer;
		const int num_engines = (int)scene->engines->num_data;
		int i;
		for(i=0; i<num_engines; i++)
		{
			engines[i]->update(engines[i]);
		}
	}

	if((scene->flags & SCENE_FLAG_UPDATE_CAMERA) != 0)
	{
		CameraUpdateTransform(&scene->camera);
	}

	UpdatePlane(&scene->plane, &scene->camera);

	scene->flags &= ~(SCENE_FLAG_UPDATE_ALL);
}

void SceneGetRenderEnginesByRenderOrder(SCENE* scene)
{
	RENDER_ENGINE_INTERFACE **engines = (RENDER_ENGINE_INTERFACE**)scene->engines->buffer;
	const int num_engines = (int)scene->engines->num_data;
	int i;

	scene->render_engines.num_pre_process = 0;
	scene->render_engines.num_standard = 0;
	scene->render_engines.num_post_process = 0;

	for(i=0; i<num_engines; i++)
	{
		RENDER_ENGINE_INTERFACE *engine = engines[i];
		if(engine->get_effect(engine, EFFECT_SCRIPT_ORDER_TYPE_PRE_PROCESS) != NULL)
		{
			scene->render_engines.pre_process[scene->render_engines.num_pre_process] = engine;
			scene->render_engines.num_pre_process++;
		}
		else if(engine->get_effect(engine, EFFECT_SCRIPT_ORDER_TYPE_POST_PROCESS) != NULL)
		{
			scene->render_engines.post_process[scene->render_engines.num_post_process] = engine;
			scene->render_engines.num_post_process++;
		}
		else
		{
			scene->render_engines.standard[scene->render_engines.num_standard] = engine;
			scene->render_engines.num_standard++;
		}
	}
}

void SceneAddModel(SCENE* scene, void* model)
{
}

void SceneUpdateModel(SCENE* scene, MODEL_INTERFACE* model, int force_physics)
{
	RENDER_ENGINE_INTERFACE **engines = (RENDER_ENGINE_INTERFACE**)scene->engines->buffer;
	RENDER_ENGINE_INTERFACE *engine;
	const int num_engines = (int)scene->engines->num_data;
	int i;

	if(model != NULL)
	{
		model->perform_update(model, force_physics);
		for(i=0; i<num_engines; i++)
		{
			engine = engines[i];
			if(engine->model == model)
			{
				engine->update(engine);
			}
		}
	}
}

BONE_INTERFACE* SceneFindNearestBone(
	SCENE* secne,
	MODEL_INTERFACE* model,
	const float z_near[3],
	const float z_far[3],
	const float threshold
)
{
	static const float basis[9] = IDENTITY_MATRIX3x3;
	const float size[] = {threshold, threshold, threshold};
	void *world_transform;
	BONE_INTERFACE **bones;
	BONE_INTERFACE *bone;
	int num_bones;
	BONE_INTERFACE *nearest_bone = NULL;
	float hit_lambda = 1.0f;
	float normal[3];
	float position[3];
	float origin[3];
	float maximum[3];
	float minimum[3];
	int i;

	world_transform = BtTransformNew(basis);
	model->get_world_translation(model, position);

	bones = (BONE_INTERFACE**)model->get_bones(model, &num_bones);
	for(i=0; i<num_bones; i++)
	{
		bone = bones[i];
		bone->get_world_transform(bone, world_transform);
		BtTransformGetOrigin(world_transform, origin);
		origin[0] += position[0];
		origin[1] += position[1];
		origin[2] += position[2];

		minimum[0] = origin[0] - size[0];
		minimum[0] = origin[1] - size[1];
		minimum[0] = origin[2] - size[2];
		maximum[0] = origin[0] + size[0];
		maximum[0] = origin[1] + size[1];
		maximum[0] = origin[2] + size[2];

		if(RayAabb(z_near, z_far, minimum, maximum, &hit_lambda, normal) != FALSE
			&& bone->is_interactive(bone) != FALSE)
		{
			nearest_bone = bone;
			break;
		}
	}

	DeleteBtTransform(world_transform);
	MEM_FREE_FUNC(bones);

	return nearest_bone;
}

int SceneIntersectsBone(
	SCENE* scene,
	BONE_INTERFACE* bone,
	const float z_near[3],
	const float z_far[3],
	const float threshold
)
{
	static const float basis[] = IDENTITY_MATRIX3x3;
	const float size[] = {threshold, threshold, threshold};
	void *world_transform;
	float origin[3];
	float minimum[3];
	float maximum[3];
	float hit_lambda = 1;
	float normal[3];

	world_transform = BtTransformNew(basis);

	bone->get_world_transform(bone, world_transform);
	BtTransformGetOrigin(world_transform, origin);
	minimum[0] = origin[0] - size[0];
	minimum[1] = origin[1] - size[1];
	minimum[2] = origin[2] - size[2];
	maximum[0] = origin[0] + size[0];
	maximum[1] = origin[1] + size[1];
	maximum[2] = origin[2] + size[2];
	bone->get_world_transform(bone, world_transform);

	DeleteBtTransform(world_transform);

	return RayAabb(z_near, z_far, minimum, maximum, &hit_lambda, normal);
}

void SceneTranslate(SCENE* scene, float* delta)
{
	float basis[9];
	float look_at[3];

	BtTransformGetBasis(scene->camera.transform, basis);
	InvertMatrix3x3(basis, basis);
	COPY_VECTOR3(look_at, delta);
	MulMatrixVector3(look_at, basis);
	scene->camera.look_at[0] += look_at[0];
	scene->camera.look_at[1] += look_at[1];
	scene->camera.look_at[2] += look_at[2];

	scene->flags |= SCENE_FLAG_UPDATE_CAMERA;
}

void SceneRotate(SCENE* scene, float* delta)
{
	scene->camera.angle[0] += delta[0];
	scene->camera.angle[1] += delta[1];
	scene->camera.angle[2] += delta[2];

	scene->flags |= SCENE_FLAG_UPDATE_CAMERA;
}

void SceneRemoveModel(SCENE* scene, MODEL_INTERFACE* model)
{
	int num_items;
	int i;

	if(model == NULL)
	{
		return;
	}

	ModelLeaveWorld(model, &scene->project->world);
	num_items = (int)scene->engines->num_data;
	for(i=0; i<num_items; i++)
	{
		RENDER_ENGINE_INTERFACE *engine = (RENDER_ENGINE_INTERFACE*)scene->engines->buffer[i];
		if(engine->model == model)
		{
			PointerArrayRemoveByData(scene->engines, engine, engine->delete_func);
			break;
		}
	}
	PointerArrayRemoveByData(scene->models, model, (void (*)(void*))DeleteModel);
	if(scene->selected_model == model)
	{
		scene->selected_model = NULL;
	}
}

MOTION_INTERFACE* SceneModelMotionNew(SCENE* scene, MODEL_INTERFACE* model)
{
	MOTION_INTERFACE *ret = NULL;
	if(model != NULL)
	{
		BONE_INTERFACE **bones;
		BONE_INTERFACE *bone;
		BONE_KEYFRAME_INTERFACE *bone_keyframe;
		float float_value[4];
		int num_items;
		int i;

		ret = NewMotion(MOTION_TYPE_VMD_MOTION, scene, model);
		bones = (BONE_INTERFACE**)model->get_bones(model, &num_items);

		for(i=0; i<num_items; i++)
		{
			bone = bones[i];
			if(bone->is_movable(bone) || bone->is_rotatable(bone))
			{
				bone_keyframe = BoneKeyframeNew(ret, scene);
				bone_keyframe->set_default_interpolation_parameter(bone_keyframe);
				bone_keyframe->base_data.name = MEM_STRDUP_FUNC(bone->name);
				bone->get_local_translation(bone, float_value);
				bone_keyframe->set_local_translation(bone_keyframe, float_value);
				bone->get_local_rotation(bone, float_value);
				bone_keyframe->set_local_rotation(bone_keyframe, float_value);
				ret->add_keyframe(ret, bone_keyframe);
			}
		}
		ret->update(ret, KEYFRAME_TYPE_BONE);
		MEM_FREE_FUNC(bones);
	}

	return ret;
}

void SceneSetModelMotion(SCENE* scene, MOTION_INTERFACE* motion, MODEL_INTERFACE* model)
{
	if(model != NULL)
	{
		MOTION_INTERFACE **motions = (MOTION_INTERFACE**)scene->motions->buffer;
		MOTION_INTERFACE *m;
		const int num_motions = (int)scene->motions->num_data;
		int i;
		for(i=0; i<num_motions; i++)
		{
			m = motions[i];
			if(m->model == model)
			{
				PointerArrayRemoveByIndex(scene->motions, i, NULL);
			}
		}
		motion->set_model(motion, model);
		PointerArrayAppend(scene->motions, motion);
	}
}

void SceneSetEmptyMotion(SCENE* scene, MODEL_INTERFACE* model)
{
	if(model != NULL)
	{
		MOTION_INTERFACE *motion = SceneModelMotionNew(scene, model);
		SceneSetModelMotion(scene, motion, model);
	}
}

KEYFRAME_INTERFACE* SceneGetCurrentKeyframe(
	SCENE* scene,
	eKEYFRAME_TYPE keyframe_type,
	void* detail_data
)
{
	KEYFRAME_INTERFACE *ret;
	MOTION_INTERFACE **motions = (MOTION_INTERFACE**)scene->motions->buffer;
	MOTION_INTERFACE *motion;
	const int num_motions = (int)scene->motions->num_data;
	int i;

	if(scene->selected_model == NULL)
	{
		return NULL;
	}

	for(i=0; i<num_motions; i++)
	{
		motion = motions[i];
		if(motion->model == scene->selected_model)
		{
			ret = motion->find_keyframe(motion, keyframe_type, scene->current_time_index, detail_data);
			if(ret != NULL)
			{
				return ret;
			}
		}
	}

	return NULL;
}

int PlaneTest(PLANE* plane, float* from, float* to, float* hit)
{
	int result = FALSE;
	void *callback = BtClosestRayResultCallbackNew(from, to);
	BtClosestRayCallbackSetFlags(callback,
		BtClosestRayCallbackGetFlags(callback) | RAY_TEST_FLAG_UUSE_SUB_SIMPLEX_CONVEX_CAST);
	BtDynamicsWorldRayTest(plane->world, from, to, callback);
	hit[0] = 0;
	hit[1] = 0;
	hit[2] = 0;
	if(BtClosestRayResultCallbackHasHit(callback) != FALSE)
	{
		BtClosestRayResultCallbackGetHitPointWorld(callback, hit);
		result = TRUE;
	}
	DeleteBtClosestRayResultCallback(callback);

	return result;
}

#ifdef __cplusplus
}
#endif
