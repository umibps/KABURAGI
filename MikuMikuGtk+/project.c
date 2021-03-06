// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <GL/glew.h>
#include "project.h"
#include "application.h"
#include "load_image.h"
#include "bullet.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void* ProjectContextNew(void* application_context, int width, int height, void** widget)
{
	APPLICATION *application = (APPLICATION*)application_context;
	PROJECT *project = ProjectNew(application_context, width, height,
		PROJECT_FLAG_DRAW_GRID | PROJECT_FLAG_ALWAYS_PHYSICS);
	*widget = (void*)project->widgets.drawing_area;

	application->projects[application->num_projects] = project;
	application->active_project = application->num_projects;
	application->num_projects++;

	return (void*)project;
}

PROJECT* ProjectNew(
	void* application_context,
	int widget_width,
	int widget_height,
	unsigned int flags
)
{
	PROJECT *ret;

	ret = (PROJECT*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	InitializeProjectWidgets(&ret->widgets, widget_width, widget_height, (void*)ret);

	ret->aspect_ratio = 1.0f;

	// レンダリングの初期化
		// 配列の初期化
	LoadIdentityMatrix4x4(ret->light_world_matrix);
	LoadIdentityMatrix4x4(ret->light_view_matrix);
	LoadIdentityMatrix4x4(ret->light_projection_matrix);

	LoadIdentityMatrix4x4(ret->camera_world_matrix);
	LoadIdentityMatrix4x4(ret->camera_view_matrix);
	LoadIdentityMatrix4x4(ret->camera_projection_matrix);

	ret->texture_chache_map = ght_create(DEFAULT_BUFFER_SIZE*2);
	ght_set_hash(ret->texture_chache_map, (ght_fn_hash_t)GetStringHash);

	ret->flags = flags;
	ret->debug_flags = DEBUG_DRAW_FLAG_VISIBLE;
	ret->application_context = (APPLICATION*)application_context;

	return ret;
}

void LoadProject(
	PROJECT* project,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int)
)
{
	uint16 data16;
	int i, j;

	// カメラの情報を読み込む
	(void)read_func(project->scene->camera.distance,
		sizeof(project->scene->camera.distance), 1, src);
	(void)read_func(project->scene->camera.position,
		sizeof(project->scene->camera.position), 1, src);
	(void)read_func(project->scene->camera.angle,
		sizeof(project->scene->camera.angle), 1, src);
	(void)read_func(project->scene->camera.distance,
		sizeof(project->scene->camera.distance), 1, src);
	(void)read_func(&project->scene->camera.fov,
		sizeof(project->scene->camera.fov), 1, src);
	CameraUpdateTransform(&project->scene->camera);

	// 光源の情報を読み込む
	(void)read_func(project->scene->light.vertex.color,
		sizeof(project->scene->light.vertex.color), 1, src);
	(void)read_func(project->scene->light.vertex.position,
		sizeof(*project->scene->light.vertex.position), 3, src);
	project->scene->light.flags |= LIGHT_FLAG_COLOR_CHANGE | LIGHT_FLAG_DIRECTION_CHANGE;

	// 背景色の情報を読み込む
	(void)read_func(project->scene->clear_color,
		sizeof(project->scene->clear_color), 1, src);

	// グリッドの色の情報を読み込む
	(void)read_func(project->grid.line_color,
		sizeof(project->grid.line_color), 1, src);

	// モデルの情報を読み込む
	{
		uint32 num_models;
		(void)read_func(&num_models, sizeof(num_models), 1, src);
		for(i=0; i<(int)num_models; i++)
		{
			MODEL_INTERFACE *model;
			(void)read_func(&data16, sizeof(data16), 1, src);
			model = MakeModelContext((eMODEL_TYPE)data16,
				(void*)project->scene, project->application_context);
			if(ReadModelData(model, project->scene, src, read_func, seek_func) != FALSE)
			{
				PointerArrayAppend(project->scene->models, model);
			}
			else
			{
				DeleteModel(model);
			}
		}
	}

	// 親ボーンの設定
	{
		MODEL_INTERFACE **models = (MODEL_INTERFACE**)project->scene->models->buffer;
		const int num_models = (int)project->scene->models->num_data;
		char *parent_model_name;
		char *parent_bone_name;

		for(i=0; i<num_models; i++)
		{
			if(models[i]->parent_model != NULL)
			{
				parent_model_name = (char*)models[i]->parent_model;
				parent_bone_name = (char*)models[i]->parent_bone;

				models[i]->parent_model = NULL;
				models[i]->parent_bone = NULL;
				for(j=0; j<num_models; j++)
				{
					if(i != j)
					{
						if(models[j]->parent_model != NULL
							&& strcmp(parent_model_name, models[j]->parent_model->name) == 0)
						{
							models[i]->parent_model = models[j];
							break;
						}
					}
				}
				if(models[i]->parent_model != NULL)
				{
					models[i]->parent_bone = models[i]->parent_model->find_bone(
						models[i]->parent_model, parent_bone_name);
				}

				MEM_FREE_FUNC(parent_bone_name);
				MEM_FREE_FUNC(parent_model_name);
			}
		}
	}
}

void LoadProjectContextData(
	void* project_context,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int)
)
{
	LoadProject((PROJECT*)project_context, src, read_func, seek_func);
}

void SaveProject(
	PROJECT* project,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
)
{
	MODEL_INTERFACE **models = (MODEL_INTERFACE**)project->scene->models->buffer;
	const int num_models = (int)project->scene->models->num_data;
	size_t out_data_size;
	uint32 data32;
	uint16 data16;
	int i;

	// カメラの情報を書き出す
	(void)write_func(project->scene->camera.distance,
		sizeof(project->scene->camera.distance), 1, dst);
	(void)write_func(project->scene->camera.position,
		sizeof(project->scene->camera.position), 1, dst);
	(void)write_func(project->scene->camera.angle,
		sizeof(project->scene->camera.angle), 1, dst);
	(void)write_func(project->scene->camera.distance,
		sizeof(project->scene->camera.distance), 1, dst);
	(void)write_func(&project->scene->camera.fov,
		sizeof(project->scene->camera.fov), 1, dst);

	// 光源の情報を書き出す
	(void)write_func(project->scene->light.vertex.color,
		sizeof(project->scene->light.vertex.color), 1, dst);
	(void)write_func(project->scene->light.vertex.position,
		sizeof(*project->scene->light.vertex.position), 3, dst);

	// 背景色の情報を書き出す
	(void)write_func(project->scene->clear_color,
		sizeof(project->scene->clear_color), 1, dst);

	// グリッドの色情報を書き出す
	(void)write_func(project->grid.line_color,
		sizeof(project->grid.line_color), 1, dst);

	// モデルの情報を書き出す
	data32 = (uint32)num_models;
	(void)write_func(&data32, sizeof(data32), 1, dst);
	for(i=0; i<num_models; i++)
	{
		data16 = (uint16)models[i]->type;
		(void)write_func(&data16, sizeof(data16), 1, dst);
		(void)WriteModelData(models[i], dst, write_func,
			seek_func, tell_func, &out_data_size);
	}
}

void SaveProjectContextData(
	void* project_context,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
)
{
	SaveProject((PROJECT*)project_context, dst, write_func, seek_func, tell_func);
}

void GetContextMatrix(float *value, MODEL_INTERFACE* model, int flags, PROJECT* project)
{
	float matrix[16] = {
		project->aspect_ratio,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};

	if((flags & MATRIX_FLAG_SHADOW_MATRIX) != 0)
	{
		if((flags & MATRIX_FLAG_PROJECTION_MATRIX) != 0)
		{
			MulMatrix4x4(project->camera_projection_matrix, matrix, matrix);
		}

		if((flags & MATRIX_FLAG_VIEW_MATRIX) != 0)
		{
			MulMatrix4x4(project->camera_view_matrix, matrix, matrix);
		}

		if((flags & MATRIX_FLAG_WORLD_MATRIX) != 0)
		{
			const float plane[3] = {0, 1, 0};
			float scalar = - project->scene->light.vertex.position[1];
			float multi_matrix[16] = IDENTITY_MATRIX;
			float model_matrix[16];
			void *transform = project->application_context->transform;
			float scale[4];
			int index;
			int offset;
			int i, j;
			for(i=0; i<3; i++)
			{
				offset = i * 4;
				for(j=0; j<3; j++)
				{
					index = offset+j;
					multi_matrix[index] = plane[i] * project->scene->light.vertex.position[j];
					if(i ==  j)
					{
						multi_matrix[index] += scalar;
					}
				}
			}

			BtTransformSetIdentity(transform);
			model->get_world_translation(model, scale);
			BtTransformSetOrigin(transform, scale);
			model->get_world_orientation(model, scale);
			BtTransformSetRotation(transform, scale);
			BtTransformGetOpenGLMatrix(transform, model_matrix);
			MulMatrix4x4(model_matrix, matrix, matrix);

			MulMatrix4x4(multi_matrix, matrix, matrix);
			MulMatrix4x4(project->camera_world_matrix, matrix, matrix);
			scale[0] = scale[1] = scale[2] = model->scale_factor;
			ScaleMatrix4x4(matrix, scale);
		}
	}
	else if((flags & MATRIX_FLAG_CAMERA_MATRIX) != 0)
	{
		if((flags & MATRIX_FLAG_PROJECTION_MATRIX) != 0)
		{
			MulMatrix4x4(project->camera_projection_matrix, matrix, matrix);
		}

		if((flags & MATRIX_FLAG_VIEW_MATRIX) != 0)
		{
			MulMatrix4x4(project->camera_view_matrix, matrix, matrix);
		}

		if(model != NULL && (flags & MATRIX_FLAG_WORLD_MATRIX) != 0)
		{
			BONE_INTERFACE *bone = model->parent_bone;
			void *transform = project->application_context->transform;
			float mul_matrix[16];
			float vector[4];

			BtTransformSetIdentity(transform);
			model->get_world_translation(model, vector);
			BtTransformSetOrigin(transform, vector);
			model->get_world_orientation(model, vector);
			BtTransformSetRotation(transform, vector);
			BtTransformGetOpenGLMatrix(transform, mul_matrix);
			MulMatrix4x4(mul_matrix, matrix, matrix);
			if(bone != NULL)
			{
				// アクセサリーの拡大率を変更すると位置が合わなくなるので調整
				float mul_vector[4];
				float rotate_matrix[16];
				float scale_value = 1.0f / model->scale_factor;
				bone->get_world_transform(bone, transform);
				BtTransformGetOpenGLMatrix(transform, mul_matrix);
				bone->model->get_world_orientation(bone->model, mul_vector);
				Quaternion2Matrix(mul_vector, rotate_matrix);
				MulMatrix4x4(mul_matrix, rotate_matrix, mul_matrix);
				bone->model->get_world_position(bone->model, mul_vector);
				mul_matrix[12] += mul_vector[0],	mul_matrix[13] += mul_vector[1],	mul_matrix[14] += mul_vector[2];
				mul_matrix[12] *= scale_value,	mul_matrix[13] *= scale_value, mul_matrix[14] *= scale_value;
				MulMatrix4x4(matrix, mul_matrix, matrix);
			}
			MulMatrix4x4(project->camera_world_matrix, matrix, matrix);
			vector[0] = vector[1] = vector[2] = model->scale_factor;
			ScaleMatrix4x4(matrix, vector);
		}
	}
	else if((flags & MATRIX_FLAG_LIGHT_MATRIX) != 0)
	{
		if((flags & MATRIX_FLAG_PROJECTION_MATRIX) != 0)
		{
			MulMatrix4x4(project->light_projection_matrix, matrix, matrix);
		}

		if((flags & MATRIX_FLAG_VIEW_MATRIX) != 0)
		{
			MulMatrix4x4(project->light_view_matrix, matrix, matrix);
		}

		if((flags & MATRIX_FLAG_WORLD_MATRIX) != 0)
		{
			float scale[3];

			MulMatrix4x4(project->light_world_matrix, matrix, matrix);
			scale[0] = scale[1] = scale[2] = model->scale_factor;
			ScaleMatrix4x4(matrix, scale);
		}
	}
	if((flags & MATRIX_FLAG_INVERSE_MATRIX) != 0)
	{
		InvertMatrix4x4(matrix, matrix);
	}
	if((flags & MATRIX_FLAG_TRANSPOSE_MATRIX) != 0)
	{
		TransposeMatrix4x4(matrix, matrix);
	}

	COPY_MATRIX4x4(value, matrix);
}

static void RenderShadowMap(PROJECT* project)
{
	if(project->shadow_map != NULL)
	{
		RENDER_ENGINE *engine;
		int num_engines;
		int i;

		glBindFramebuffer(GL_FRAMEBUFFER, project->shadow_map->frame_buffer);

		glViewport(0, 0, (GLsizei)project->shadow_map->size[0],
			(GLsizei)project->shadow_map->size[1]);
		num_engines = (int)project->scene->engines->num_data;
		for(i=0; i<num_engines; i++)
		{
			engine = (RENDER_ENGINE*)project->scene->engines->buffer[i];
			engine->render_zplot(engine->engine_data);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

static void SetCameraMatrices(
	PROJECT* project,
	const float* world,
	const float* view,
	const float* projection
)
{
	COPY_MATRIX4x4(project->camera_world_matrix, world);
	COPY_MATRIX4x4(project->camera_view_matrix, view);
	COPY_MATRIX4x4(project->camera_projection_matrix, projection);
}

static void SetViewPort(PROJECT* project, float* view_port)
{
	project->view_port[0] = view_port[0];
	project->view_port[1] = view_port[1];
}

static void UpdateCameraMatrices(PROJECT* project, float* size)
{
	const float world[] = IDENTITY_MATRIX;
	float projection[16];
	CAMERA *camera = &project->scene->camera;
	float view[16];
	float aspect;

	BtTransformGetOpenGLMatrix(camera->transform, view);
	if(size[0] > size[1])
	{
		aspect = size[0] / size[1];
	}
	else
	{
		aspect = size[1] / size[0];
	}

	InfinitePerspective(projection, camera->fov, aspect, camera->znear);
	SetCameraMatrices(project, world, view, projection);

	SetViewPort(project, size);
}

static void UpdateViewPort(PROJECT* project, int width, int height, int update)
{
	int start_x = 0,	start_y = 0;

	if(update != FALSE
		|| (project->flags & PROJECT_FLAG_MARK_DIRTY) != 0)
	{
		float size[2] = {(float)width, (float)height};
		UpdateCameraMatrices(project, size);
		COPY_MATRIX4x4(project->view_projection_matrix, project->camera_world_matrix);
		MulMatrix4x4(project->view_projection_matrix, project->camera_view_matrix, project->view_projection_matrix);
		MulMatrix4x4(project->view_projection_matrix, project->camera_projection_matrix, project->view_projection_matrix);
		project->flags &= ~(PROJECT_FLAG_MARK_DIRTY);
	}

	if(width != project->scene->width)
	{
		start_x = (width - project->scene->width) / 2;
	}
	if(height != project->scene->height)
	{
		start_y = (height - project->scene->height) / 2;
	}

	glViewport(start_x, start_y, project->scene->width, project->scene->height);
}

void RenderEngines(PROJECT* project, int width, int height)
{
	SCENE *scene = project->scene;
	BONE_INTERFACE *bone = NULL;
	size_t loop;
	size_t i;

	SetRequiredOpengGLState(project->scene);
	SceneGetRenderEnginesByRenderOrder(scene);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//RenderShadowMap(project);
	UpdateViewPort(project, width, height, TRUE);

	// グリッドのレンダリング
	if((project->flags & PROJECT_FLAG_DRAW_GRID) != 0)
	{
		DrawGrid(&project->grid, project->view_projection_matrix);
	}

	loop = project->scene->render_engines.num_post_process;
	for(i=0; i<loop; i++)
	{
		scene->render_engines.post_process[i]->update(scene->render_engines.post_process[i]);
		scene->render_engines.post_process[i]->prepare_post_process(scene->render_engines.post_process[i]);
	}

	loop = scene->render_engines.num_pre_process;
	for(i=0; i<loop; i++)
	{
		scene->render_engines.pre_process[i]->update(scene->render_engines.pre_process[i]);
		scene->render_engines.pre_process[i]->perform_pre_process(scene->render_engines.pre_process[i]);
	}

	loop = scene->render_engines.num_standard;
	if((project->flags & PROJECT_FLAG_RENDER_EDGE_ONLY) == 0)
	{
		for(i=0; i<loop; i++)
		{
			if((project->flags & PROJECT_FLAG_RENDER_SHADOW) != 0)
			{
				project->scene->render_engines.standard[i]->render_shadow(project->scene->render_engines.standard[i]);
			}
			scene->render_engines.standard[i]->render_model(scene->render_engines.standard[i]);
			scene->render_engines.standard[i]->render_edge(scene->render_engines.standard[i]);

			if((project->flags & PROJECT_FLAG_UPDATE_ALWAYS) != 0)
			{
				scene->render_engines.standard[i]->update(scene->render_engines.standard[i]);
			}
		}
	}
	else
	{
		for(i=0; i<loop; i++)
		{
			scene->render_engines.standard[i]->render_model_reverse(scene->render_engines.standard[i]);
			scene->render_engines.standard[i]->render_edge(scene->render_engines.standard[i]);

			if((project->flags & PROJECT_FLAG_UPDATE_ALWAYS) != 0)
			{
				scene->render_engines.standard[i]->update(scene->render_engines.standard[i]);
			}
		}
	}
}

void DisplayProject(PROJECT* project, int width, int height)
{
	SCENE *scene = project->scene;
	BONE_INTERFACE *bone = NULL;
	MODEL_INTERFACE *model;

	project->aspect_ratio = DEFAULT_ASPECT_RATIO
		/ ((float)project->scene->width / (float)project->scene->height);

	if((scene->flags & SCENE_FLAG_MODEL_CONTROLLED) != 0)
	{
		const num_motions = (int)scene->motions->num_data;
		int i;
		for(i=0; i<num_motions; i++)
		{
			((MOTION_INTERFACE*)scene->motions->buffer[i])->seek(scene->motions->buffer[i], scene->current_time_index);
		}
	}
	else if((project->flags & PROJECT_FLAG_ALWAYS_PHYSICS) != 0)
	{
		WorldStepSimulation(&project->world, project->world.fixed_time_step);
		scene->flags |= SCENE_FLAG_UPDATE_MODELS | SCENE_FLAG_UPDATE_RENDER_ENGINES;
	}

	UpdateScene(project->scene);

	RenderEngines(project, width, height);

#ifdef _DEBUG
	DebugDrawerDrawWorld(&project->debug_drawer, &project->world, 1);
#else
	DebugDrawerDrawWorld(&project->debug_drawer, &project->world, project->debug_flags);
#endif
	model = scene->selected_model;
	if(scene->selected_bones->num_data != 0)
	{
		bone = (BONE_INTERFACE*)scene->selected_bones->buffer[0];
	}
	switch(project->control.edit_mode)
	{
	case EDIT_MODE_SELECT:	// ボーン選択モード
		if((project->control.handle.flags & HANDLE_FLAG_ENABLE) == 0)
		{
			DebugDrawerDrawBones(&project->debug_drawer, model, scene->selected_bones);
		}

		if((project->flags & PROJECT_FLAG_IS_IMAGE_HANDLE_RECT_INTERSECT) != 0)
		{
			DebugDrawerDrawBoneTransform(&project->debug_drawer, bone, model,
				HandleModeFromConstraint(&project->control.handle));
		}

		if(scene->selected_model != NULL)
		{
			DrawImageHandles(&project->control.handle, bone);
		}

		break;
	case EDIT_MODE_MOVE:	// ボーン移動モード
		DebugDrawerDrawMovableBone(&project->debug_drawer, bone, model);
		DrawImageHandles(&project->control.handle, bone);
		DrawMoveHandle(&project->control.handle);
		break;
	case EDIT_MODE_ROTATE:	// ボーン回転モード
		DebugDrawerDrawMovableBone(&project->debug_drawer, bone, model);
		DrawImageHandles(&project->control.handle, bone);
		DrawRotationHandle(&project->control.handle);
		break;
	}
}

void MakeRay(PROJECT* project, FLOAT_T mouse_x, FLOAT_T mouse_y, float *ray_from, float *ray_to)
{
	float world[16], view[16], projection[16];
	float window[3] = {(float)mouse_x, project->scene->height - (float)mouse_y, 0};
	float viewport[4] = {0, 0, (float)project->scene->width, (float)project->scene->height};

	COPY_MATRIX4x4(world, project->camera_world_matrix);
	COPY_MATRIX4x4(view, project->camera_view_matrix);

	Perspective(projection, project->scene->camera.fov,
		(float)project->scene->width / project->scene->height, 0.1f, 10000.0f);
	MulMatrix4x4(world, view, world);
	Unprojection(ray_from, window, world, projection, viewport);
	window[2] = 1;
	Unprojection(ray_to, window, world, projection, viewport);
}

int RayAabb(
	const float ray_from[3],
	const float ray_to[3],
	const float aa_bb_min[3],
	const float aa_bb_max[3],
	float* param,
	float normal[3]
)
{
	float aa_bb_half_extent[3];
	float aa_bb_center[3];
	float source[3];
	float target[3];
	int source_out_code;
	int target_out_code;

	aa_bb_half_extent[0] = (aa_bb_max[0] - aa_bb_min[0]) * 0.5f;
	aa_bb_half_extent[1] = (aa_bb_max[1] - aa_bb_min[1]) * 0.5f;
	aa_bb_half_extent[2] = (aa_bb_max[2] - aa_bb_min[2]) * 0.5f;
	aa_bb_center[0] = (aa_bb_max[0] + aa_bb_min[0]) * 0.5f;
	aa_bb_center[1] = (aa_bb_max[1] + aa_bb_min[1]) * 0.5f;
	aa_bb_center[2] = (aa_bb_max[2] + aa_bb_min[2]) * 0.5f;
	source[0] = ray_from[0] - aa_bb_center[0];
	source[1] = ray_from[1] - aa_bb_center[1];
	source[2] = ray_from[2] - aa_bb_center[2];
	target[0] = ray_to[0] - aa_bb_center[0];
	target[1] = ray_to[1] - aa_bb_center[1];
	target[2] = ray_to[2] - aa_bb_center[2];
	source_out_code = OutCode(source, aa_bb_half_extent);
	target_out_code = OutCode(target, aa_bb_half_extent);
	if((source_out_code & target_out_code) == 0x0)
	{
		float lambda_enter = 0;
		float lambda_exit = *param;
		float r[3] = {target[0] - source[0], target[1] - source[1], target[2] - source[2]};
		float norm_sign = 1;
		float hit_normal[3] = {0};
		int bit = 1;
		int i, j;

		for(j=0; j<2; j++)
		{
			for(i=0; i!=3; ++i)
			{
				if((source_out_code & bit) != 0)
				{
					float lambda = (- source[i] - aa_bb_half_extent[i]*norm_sign) / r[i];
					if(lambda_enter <= lambda)
					{
						lambda_enter = lambda;
						hit_normal[0] = 0;
						hit_normal[1] = 0;
						hit_normal[2] = 0;
						hit_normal[i] = norm_sign;
					}
				}
				else if((target_out_code & bit) != 0)
				{
					float lambda = (-source[i] - aa_bb_half_extent[i]*norm_sign) / r[i];
					SET_MIN(lambda_exit, lambda);
				}
				bit <<= 1;
			}
			norm_sign = -1;
		}
		if(lambda_enter <= lambda_exit)
		{
			*param = lambda_enter;
			COPY_VECTOR3(normal, hit_normal);
			return TRUE;
		}
	}

	return FALSE;
}

void ChangeSelectedBones(PROJECT* project, BONE_INTERFACE** bones, int num_selected)
{
	int i;
	project->scene->selected_bones->num_data = 0;

	if(num_selected == 0)
	{
		project->control.handle.bone = NULL;
	}
	else
	{
		for(i=0; i<num_selected; i++)
		{
			PointerArrayAppend(project->scene->selected_bones, bones[i]);
		}
		ControlHandleSetBone(&project->control.handle, bones[0]);
	}
}

int TestHitModelHandle(PROJECT* project, FLOAT_T x, FLOAT_T y)
{
	if(project->control.edit_mode == EDIT_MODE_ROTATE
		|| project->control.edit_mode == EDIT_MODE_MOVE)
	{
		float ray_from[3], ray_to[3], pick[3];
		unsigned int flags = HANDLE_FLAG_NONE;
		MakeRay(project, x, y, ray_from, ray_to);
		return ControlHandleTestHitModel(&project->control.handle, ray_from, ray_to,
			TRUE, &flags, pick);
	}

	return FALSE;
}

void ProjectSetEnableAlwaysPhysics(PROJECT* project, int enabled)
{
	MODEL_INTERFACE **models = (MODEL_INTERFACE**)project->scene->models->buffer;
	int num_models = (int)project->scene->models->num_data;
	int i;

	for(i=0; i<num_models; i++)
	{
		models[i]->set_enable_physics(models[i], enabled);
	}
}

int FindTextureCache(const char* path, TEXTURE_DATA_BRIDGE* bridge, PROJECT* project)
{
	void *value;

	value = ght_get(project->texture_chache_map, (unsigned int)strlen(path), path);
	if(value != NULL)
	{
		bridge->texture = (TEXTURE_INTERFACE*)value;

		return TRUE;
	}

	return FALSE;
}

#define LOAD_DEFAULT_FORMAT_ALPHA(FORMAT) \
	(FORMAT).external = GL_ALPHA; \
	(FORMAT).internal = GL_ALPHA; \
	(FORMAT).type = GL_UNSIGNED_BYTE; \
	(FORMAT).target = GL_TEXTURE_2D;

#define LOAD_DEFAULT_FORMAT_RGB(FORMAT) \
	(FORMAT).external = GL_RGB; \
	(FORMAT).internal = GL_RGB8; \
	(FORMAT).type = GL_UNSIGNED_BYTE; \
	(FORMAT).target = GL_TEXTURE_2D;

#define LOAD_DEFAULT_FORMAT_RGBA(FORMAT) \
	(FORMAT).external = GL_RGBA; \
	(FORMAT).internal = GL_RGBA8; \
	(FORMAT).type = GL_UNSIGNED_INT_8_8_8_8_REV; \
	(FORMAT).target = GL_TEXTURE_2D;

TEXTURE_INTERFACE* CreateTexture(
	uint8* pixels,
	TEXTURE_FORMAT* format,
	int* size,
	int mipmap
)
{
	TEXTURE_2D *ret = (TEXTURE_2D*)MEM_ALLOC_FUNC(sizeof(*ret));
	InitializeTexture2D(ret, format, size, 0);
	BaseTextureMake(&ret->base_data);
	BaseTextureBind(&ret->base_data);
	if(CheckHasExtension("ARB_texture_storage"))
	{
		glTexStorage2D(format->target, 1, format->internal, size[0], size[1]);
		glTexSubImage2D(format->target, 0, 0, 0, size[0], size[1], format->external, format->type, pixels);
	}
	else
	{
		glTexImage2D(format->target, 0, format->internal, size[0], size[1], 0, format->external, format->type, pixels);
	}
	BaseTextureUnbind(&ret->base_data);

	return (TEXTURE_INTERFACE*)ret;
}

int CacheTexture(
	const char* key,
	TEXTURE_INTERFACE* texture,
	TEXTURE_DATA_BRIDGE* bridge,
	PROJECT* project
)
{
	GLuint name;

	if(key == NULL)
	{
		return FALSE;
	}

	name = texture->name;
	glBindTexture(GL_TEXTURE_2D, name);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if((bridge->flags & TEXTURE_FLAG_SYSTEM_TOON_TEXTURE) != 0)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	bridge->texture = texture;
	if(texture != NULL)
	{
		(void)ght_insert(project->texture_chache_map, texture, (unsigned int)strlen(key), (void*)key);
	}

	return TRUE;
}

int UploadTexture(const char* path, TEXTURE_DATA_BRIDGE* bridge, PROJECT* project)
{
	TEXTURE_INTERFACE *texture;
	TEXTURE_FORMAT format;
	uint8 *pixels;
	int size[3];
	int channel;

	if(path[strlen(path)-1] == '/')
	{
		return TRUE;
	}
	else if(FindTextureCache(path, bridge, project) != FALSE)
	{
		return TRUE;
	}

	pixels = LoadImage(path, &size[0], &size[1], &channel);
	if(pixels == NULL)
	{
		return FALSE;
	}

	if(channel == 3)
	{
		LOAD_DEFAULT_FORMAT_RGB(format);
	}
	else
	{
		LOAD_DEFAULT_FORMAT_RGBA(format);
	}

	texture = CreateTexture(pixels, &format, size, bridge->flags & TEXTURE_FLAG_GENERATE_TEXTURE_MIPMAP);

	MEM_FREE_FUNC(pixels);

	return CacheTexture(path, texture, bridge, project);
}

int UploadTextureFromMemory(
	uint8* pixels,
	int width,
	int height,
	int channel,
	const char* path,
	TEXTURE_DATA_BRIDGE* bridge,
	PROJECT* project
)
{
	TEXTURE_INTERFACE *texture;
	TEXTURE_FORMAT format;
	int size[3] = {width, height, 0};

	if(FindTextureCache(path, bridge, project) != FALSE)
	{
		return TRUE;
	}

	if(pixels == NULL)
	{
		return FALSE;
	}

	switch(channel)
	{
	case 1:
		LOAD_DEFAULT_FORMAT_ALPHA(format);
		break;
	case 3:
		LOAD_DEFAULT_FORMAT_RGB(format);
		break;
	case 4:
		LOAD_DEFAULT_FORMAT_RGBA(format);
		break;
	default:
		return FALSE;
	}

	texture = CreateTexture(pixels, &format, size,
		bridge->flags & TEXTURE_FLAG_GENERATE_TEXTURE_MIPMAP);

	return CacheTexture(path, texture, bridge, project);
}

int UploadWhiteTexture(int width, int height, PROJECT* project)
{
	TEXTURE_DATA_BRIDGE bridge;
	TEXTURE_FORMAT format;
	TEXTURE_INTERFACE *texture;
	int size[3];
	uint8 *pixels;

	if(FindTextureCache(WHITE_TEXTURE_NAME, &bridge, project) != FALSE)
	{
		return TRUE;
	}

	pixels = (uint8*)MEM_ALLOC_FUNC(width * height * 4);
	(void)memset(pixels, 0xff, width * height * 4);
	LOAD_DEFAULT_FORMAT_RGBA(format);

	size[0] = width,	size[1] = height;
	texture = CreateTexture(pixels, &format, size, 0);

	MEM_FREE_FUNC(pixels);

	return CacheTexture(WHITE_TEXTURE_NAME, texture, &bridge, project);
}

void ProjectSetOriginalSize(void* project_context, int width, int height)
{
	((PROJECT*)project_context)->original_width = width;
	((PROJECT*)project_context)->original_height = height;
}

void ResizeProject(PROJECT* project, int available_width, int available_height)
{
	int new_width, new_height;

	if(project->original_width == 0
		|| project->original_height == 0)
	{
		new_width = available_width;
		new_height = available_height;
	}
	else
	{
		new_width = available_width;
		new_height = new_width * project->original_height / project->original_width;
		if(new_height > available_height)
		{
			new_height = available_height;
			new_width = new_height * project->original_width / project->original_height;
		}
	}

	project->scene->width = new_width;
	project->scene->height = new_height;
}

#ifdef __cplusplus
}
#endif
