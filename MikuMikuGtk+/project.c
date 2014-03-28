#include <string.h>
#include <GL/glew.h>
#include "project.h"
#include "application.h"
#include "bullet.h"
#include "memory.h"

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

	// レンダリングの初期化
		// 配列の初期化
	LoadIdentityMatrix4x4(ret->light_world_matrix);
	LoadIdentityMatrix4x4(ret->light_view_matrix);
	LoadIdentityMatrix4x4(ret->light_projection_matrix);

	LoadIdentityMatrix4x4(ret->camera_world_matrix);
	LoadIdentityMatrix4x4(ret->camera_view_matrix);
	LoadIdentityMatrix4x4(ret->camera_projection_matrix);

	ret->flags = flags;
	ret->debug_flags = DEBUG_DRAW_FLAG_VISIBLE;
	ret->application_context = (APPLICATION*)application_context;

	return ret;
}


void GetContextMatrix(float *value, MODEL_INTERFACE* model, int flags, PROJECT* project)
{
	float matrix[16] = {
		1,0,0,0,
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
			float multi_mat[16] = IDENTITY_MATRIX;
			float scale[3];
			int index;
			int offset;
			int i, j;
			for(i=0; i<3; i++)
			{
				offset = i * 4;
				for(j=0; j<3; j++)
				{
					index = offset+j;
					multi_mat[index] = plane[i] * project->scene->light.vertex.position[j];
					if(i ==  j)
					{
						multi_mat[index] += scalar;
					}
				}
			}

			MulMatrix4x4(multi_mat, matrix, matrix);
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
			static const float basis[] = IDENTITY_MATRIX3x3;
			BONE_INTERFACE *bone = model->parent_bone;
			void *transform = BtTransformNew(basis);
			float mul_matrix[16];
			float vector[4];

			model->get_world_translation(model, vector);
			BtTransformSetOrigin(transform, vector);
			model->get_world_orientation(model, vector);
			BtTransformSetRotation(transform, vector);
			BtTransformGetOpenGLMatrix(transform, mul_matrix);
			MulMatrix4x4(mul_matrix, matrix, matrix);
			if(bone != NULL)
			{
				// アクセサリーの拡大率を変更すると位置が合わなくなるので調整
				float scale_value = 1.0f / model->scale_factor;
				bone->get_world_transform(bone, transform);
				BtTransformGetOpenGLMatrix(transform, mul_matrix);
				mul_matrix[12] *= scale_value,	mul_matrix[13] *= scale_value, mul_matrix[14] *= scale_value;
				MulMatrix4x4(matrix, mul_matrix, matrix);
			}
			MulMatrix4x4(project->camera_world_matrix, matrix, matrix);
			vector[0] = vector[1] = vector[2] = model->scale_factor;
			ScaleMatrix4x4(matrix, vector);

			DeleteBtTransform(transform);
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

static void RenderAssetZPlot(RENDER_ENGINE* engine)
{
	if(engine->model == NULL
		|| (engine->model->flags & MODEL_FLAG_VISIBLE) == 0
		|| engine->current_engine == NULL
	)
	{
		return;
	}


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

	glViewport(0, 0, width, height);
}

void RenderEngines(PROJECT* project, int width, int height)
{
	SCENE *scene = project->scene;
	BONE_INTERFACE *bone = NULL;
	size_t loop;
	size_t i;

	SetRequiredOpengGLState();
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
			/*
			if(シャドウマップ)
			{
				project->scene->render_engines.standard[i]->render_shadow(project->scene->render_engines.standard[i]);
			}
			*/
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
