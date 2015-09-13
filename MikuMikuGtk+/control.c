// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/cimport.h>
#include "application.h"
#include "control.h"
#include "bullet.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
# define M_PI 3.1415926535897932384626433832795
#endif

void MouseButtonPressCallback(
	void* project_context,
	double x,
	double y,
	int button,
	int state
)
{
	PROJECT *project = (PROJECT*)project_context;
	SCENE *scene = project->scene;
	CONTROL *control = &project->control;
	void *transform = project->application_context->transform;
	float z_near[3], z_far[3], hit[3];
	float rect[4];
	float delta;
	int movable = 0;
	int rotatable = 0;
	unsigned int flags = HANDLE_FLAG_NONE;

	project->flags |= PROJECT_FLAG_LOCK_TOUCH_EVENT;
	control->click_origin[0] = x;
	control->click_origin[1] = y;

	control->handle.prev_position_2d[0] = x;
	control->handle.prev_position_2d[1] = y;

	MakeRay(project, x, y, z_near, z_far);
	(void)PlaneTest(&scene->plane, z_near, z_far, hit);
	delta = 0.0005f * scene->camera.distance[2];
	control->delta[0] = delta;
	control->delta[1] = delta;

	switch(button)
	{
	case MOUSE_BUTTON_LEFT:
		control->flags |= CONTROL_FLAG_LEFT_BUTTON_MASK;
		if(scene->selected_bones->num_data > 0)
		{
			BONE_INTERFACE *bone = (BONE_INTERFACE*)scene->selected_bones->buffer[
				scene->selected_bones->num_data-1];
			movable = bone->is_movable(bone);
			rotatable = bone->is_rotatable(bone);

			if(ControlHandleTestHitImage(&control->handle, (float)x, (float)y,
				movable, rotatable, &flags, rect) != FALSE)
			{
				switch(flags)
				{
				case HANDLE_FLAG_VIEW:
					control->handle.constraint = HANDLE_FLAG_LOCAL;
					break;
				case HANDLE_FLAG_LOCAL:
					control->handle.constraint = HANDLE_FLAG_GLOBAL;
					break;
				case HANDLE_FLAG_GLOBAL:
					control->handle.constraint = HANDLE_FLAG_VIEW;
					break;
				default:
					control->handle_flags = flags;
					break;
				}

				return;
			}
		}
		break;
	case MOUSE_BUTTON_CENTER:
		control->flags |= CONTROL_FLAG_CENTER_BUTTON_MASK;
		break;
	case MOUSE_BUTTON_RITHGT:
		control->flags |= CONTROL_FLAG_RIGHT_BUTTON_MASK;
		break;
	}

	if(scene->selected_bones->num_data != 0)
	{
		BONE_INTERFACE *bone = (BONE_INTERFACE*)scene->selected_bones->buffer[
			scene->selected_bones->num_data-1];
		movable = bone->is_movable(bone);
		rotatable = bone->is_rotatable(bone);
	}
	
	if(scene->selected_model != NULL)
	{
		MODEL_INTERFACE *model = scene->selected_model;

		if(control->edit_mode == EDIT_MODE_SELECT)
		{
			BONE_INTERFACE *nearest_bone = SceneFindNearestBone(scene, model, z_near, z_far, 0.1f);
			if(nearest_bone != NULL)
			{
				if((state & MODIFIER_CONTROL_MASK) == 0)
				{
					scene->selected_bones->num_data = 0;
				}

				if(PointerArrayDoesCointainData(scene->selected_bones, nearest_bone) != FALSE)
				{
					PointerArrayRemoveByData(scene->selected_bones, nearest_bone, NULL);
				}
				else
				{
					PointerArrayAppend(scene->selected_bones, nearest_bone);
				}
			}
		}
		else if(control->edit_mode == EDIT_MODE_ROTATE || control->edit_mode == EDIT_MODE_MOVE)
		{
			float ray_from[3];
			float ray_to[3];
			float pick[3];
			BONE_INTERFACE *bone = control->handle.bone;

			MakeRay(project, x, y, ray_from, ray_to);
			if(bone != NULL)
			{
				if(control->edit_mode == EDIT_MODE_MOVE)
				{
					bone->get_local_translation(bone, control->before_bone_value);
					control->flags |= CONTROL_FLAG_BONE_MOVE;
				}
				else if(control->edit_mode == EDIT_MODE_ROTATE)
				{
					bone->get_local_rotation(bone, control->before_bone_value);
					control->flags |= CONTROL_FLAG_BONE_ROTATE;
				}

				if(bone->is_movable(bone) != FALSE)
				{
					if(SceneIntersectsBone(scene, bone, z_near, z_far, 0.5f) != FALSE
						|| ControlHandleTestHitModel(&control->handle, ray_from, ray_to, FALSE, &flags, pick) != FALSE)
					{
						control->current_bone = bone;
						bone->get_world_transform(bone, transform);
						BtTransformGetOrigin(transform, control->last_bone_position);
					}
				}
				else if(bone->is_rotatable(bone) != FALSE && control->edit_mode == EDIT_MODE_ROTATE)
				{
					control->current_bone = bone;
					bone->get_world_transform(bone, transform);
					BtTransformGetOrigin(transform, control->last_bone_position);
				}
			}

			if(ControlHandleTestHitModel(&control->handle, ray_from, ray_to, FALSE, &flags, pick) != FALSE)
			{
				control->handle.flags &= ~(HANDLE_FLAG_VISIBLE_ALL);
				control->handle.flags |= flags;
				if(control->edit_mode == EDIT_MODE_ROTATE)
				{
					ControlHandleSetVisibilityFlags(&control->handle, flags);
				}
			}
		}
	}

	project->flags |= PROJECT_FLAG_MARK_DIRTY;
}

void MouseMotionCallback(
	void* project_context,
	double x,
	double y,
	int state
)
{
	PROJECT *project = (PROJECT*)project_context;
	SCENE *scene = project->scene;
	CONTROL *control = &project->control;
	void *transform = project->application_context->transform;
	float rect[4];
	double diff[2];
	unsigned int flags = HANDLE_FLAG_NONE;

	diff[0] = x - control->handle.prev_position_2d[0];
	diff[1] = y - control->handle.prev_position_2d[1];

	if(diff[0] * diff[0] + diff[1] * diff[1] < 0.75)
	{
		return;
	}

	if((control->flags & CONTROL_FLAG_LEFT_BUTTON_MASK) != 0)
	{
		if(control->current_bone != NULL)
		{
			if(control->edit_mode == EDIT_MODE_ROTATE)
			{
				ControlRotateBone(control, control->current_bone, diff);
				control->handle.prev_position_2d[0] = x;
				control->handle.prev_position_2d[1] = y;
			}
			else
			{
				float z_near[3], z_far[3], hit[3];
				float delta[3];
				MakeRay(project, x, y, z_near, z_far);
				PlaneTest(&project->scene->plane, z_near, z_far, hit);
				delta[0] = hit[0] - control->last_bone_position[0];
				delta[1] = hit[1] - control->last_bone_position[1];
				delta[2] = hit[2] - control->last_bone_position[2];

				BoneMotionModelTranslateTo(&control->handle.motion_state,
					delta, control->current_bone, 'G');
				scene->flags |= SCENE_FLAG_UPDATE_RENDER_ENGINES;
			}
		}
	}
	else if((control->flags & CONTROL_FLAG_CENTER_BUTTON_MASK) != 0)
	{
		float delta[3] = {(float)(diff[0] * (- control->delta[0]) * 0.5), (float)(diff[1] * control->delta[1] * 0.5), 0};
		SceneTranslate(scene, delta);
	}
	else if((control->flags & CONTROL_FLAG_RIGHT_BUTTON_MASK) != 0)
	{
		//float delta[3] = {(float)(diff[1] * 0.5), (float)(diff[0] * 0.5), 0.0f};
		float delta[3] = {(float)(diff[1] * 0.5 * M_PI / 180), (float)(diff[0] * 0.5 * M_PI / 180), 0.0f};
		SceneRotate(scene, delta);

		control->handle.prev_position_2d[0] = x;
		control->handle.prev_position_2d[1] = y;

		project->flags |= PROJECT_FLAG_MARK_DIRTY;
	}
	else
	{
		BONE_INTERFACE *bone = control->handle.bone;
		if(bone != NULL)
		{
			unsigned int flags = 0;
			int movable = bone->is_movable(bone);
			int rotateable = bone->is_rotatable(bone);
			float z_near[3], z_far[3];
			MakeRay(project, x, y, z_near, z_far);
			if(movable != FALSE && control->edit_mode != EDIT_MODE_SELECT
				&& SceneIntersectsBone(scene, bone, z_near, z_far, 0.5f) != FALSE)
			{
				return;
			}

			if(ControlHandleTestHitImage(&control->handle, (float)x, (float)y,
				movable, rotateable, &flags, rect) != FALSE)
			{
				control->flags |= CONTROL_FLAG_IMAGE_HANDLE_RECT_INTERSECT;
			}
			else
			{
				control->flags &= ~(CONTROL_FLAG_IMAGE_HANDLE_RECT_INTERSECT);
			}

			if(control->handle_flags == HANDLE_FLAG_NONE)
			{
				if((control->flags & CONTROL_FLAG_IMAGE_HANDLE_RECT_INTERSECT) != 0)
				{
				}
				else if(TestHitModelHandle(project, x, y) != FALSE)
				{
				}
			}
		}
	}
}

void MouseButtonReleaseCallback(
	void* project_context,
	double x,
	double y,
	int button,
	int state
)
{
	PROJECT *project = (PROJECT*)project_context;
	CONTROL *control = &project->control;

	switch(button)
	{
	case MOUSE_BUTTON_LEFT:
		if((control->flags & CONTROL_FLAG_BONE_MOVE) != 0)
		{
			AddBoneMoveHistory(&project->history, project);
		}
		else if((control->flags & CONTROL_FLAG_BONE_ROTATE) != 0)
		{
			AddBoneRotateHistory(&project->history, project);
		}
		control->flags &= ~(CONTROL_FLAG_LEFT_BUTTON_MASK
			| CONTROL_FLAG_BONE_MOVE | CONTROL_FLAG_BONE_ROTATE);
		ControlSetEditMode(control, control->edit_mode);
		break;
	case MOUSE_BUTTON_CENTER:
		control->flags &= ~(CONTROL_FLAG_CENTER_BUTTON_MASK);
		break;
	case MOUSE_BUTTON_RITHGT:
		control->flags &= ~(CONTROL_FLAG_RIGHT_BUTTON_MASK);
		break;
	}

	control->handle_flags = HANDLE_FLAG_NONE;
	control->handle.prev_angle = 0.0f;
	control->handle.prev_position_3d[0] = 0;
	control->handle.prev_position_3d[1] = 0;
	control->handle.prev_position_3d[2] = 0;

	control->current_bone = NULL;

	project->flags |= PROJECT_FLAG_MARK_DIRTY;
}

void MouseScrollCallback(
	void* project_context,
	int direction,
	int state
)
{
#define FOVY_STEP 1.0f
#define DEFAULT_DISTANCE_STEP 4.0f

	PROJECT *project = (PROJECT*)project_context;
	CAMERA *camera = &project->scene->camera;

	if((state & MODIFIER_CONTROL_MASK) != 0
		&& (state & MODIFIER_SHIFT_MASK) != 0)
	{
		switch(direction)
		{
		case MOUSE_WHEEL_UP:
			camera->fov -= FOVY_STEP;
			break;
		case MOUSE_WHEEL_DOWN:
			camera->fov += FOVY_STEP;
			break;
		}
	}
	else
	{
		float distance_step = DEFAULT_DISTANCE_STEP;
		float distance = camera->distance[2];

		if((state & MODIFIER_CONTROL_MASK) != 0)
		{
			distance_step *= 5.0f;
		}
		else if((state & MODIFIER_SHIFT_MASK) != 0)
		{
			distance_step *= 0.2f;
		}

		if(distance_step != 0.0f)
		{
			switch(direction)
			{
			case MOUSE_WHEEL_UP:
				distance -= distance_step;
				break;
			case MOUSE_WHEEL_DOWN:
				distance += distance_step;
				break;
			default:
				return;
			}

			SetCameraDistance(camera, distance);
		}
	}

	project->scene->flags |= SCENE_FLAG_UPDATE_CAMERA;
	project->flags |= PROJECT_FLAG_MARK_DIRTY;
}

int HandleModeFromConstraint(CONTROL_HANDLE* handle)
{
	switch(handle->flags & ~(HANDLE_FLAG_VISIBLE | HANDLE_FLAG_LOADED))
	{
	case HANDLE_FLAG_VIEW:
		return 'V';
	case HANDLE_FLAG_LOCAL:
		return 'L';
	case HANDLE_FLAG_GLOBAL:
		return 'G';
	}

	return 'L';
}

void* ControlHandleGetModelHandleTransform(CONTROL_HANDLE* handle)
{
	SCENE *scene = handle->application_context->projects[handle->application_context->active_project]->scene;
	const float basis[] = IDENTITY_MATRIX3x3;
	void *transform = BtTransformNew(basis);
	void *local_trans = handle->application_context->transform;

	if(handle->bone != NULL)
	{
		int mode = HandleModeFromConstraint(handle);
		if(mode == 'G')
		{
			float origin[3];
			handle->bone->get_world_transform(handle->bone, local_trans);
			BtTransformGetOrigin(local_trans, origin);
			BtTransformSetOrigin(transform, origin);
		}
		else if(mode == 'L')
		{
			handle->bone->get_world_transform(handle->bone, transform);
		}
		else if(mode == 'V')
		{
			float scene_basis[9];
			float new_basis[9];

			BtTransformGetBasis(scene->camera.transform, scene_basis);
			new_basis[0] = scene_basis[0];
			new_basis[1] = 0;
			new_basis[2] = 0;
			new_basis[3] = 0;
			new_basis[4] = scene_basis[4];
			new_basis[5] = 0;
			new_basis[6] = 0;
			new_basis[7] = 0;
			new_basis[8] = scene_basis[8];
			BtTransformSetBasis(transform, new_basis);
		}
	}
	if(scene->selected_model != NULL)
	{
		float origin[3];
		float position[3];
		BtTransformGetOrigin(transform, origin);

		scene->selected_model->get_world_position(scene->selected_model, position);
		origin[0] += position[0];
		origin[1] += position[1];
		origin[2] += position[2];
		BtTransformSetOrigin(transform, origin);
	}

	return transform;
}

void BoneHandleStateGetWorldTransform(void *setup_transform, BONE_HANDLE_MOTION_STATE* state)
{
	void *transform = ControlHandleGetModelHandleTransform(state->handle);
	BtTransformSet(setup_transform, transform);
	DeleteBtTransform(transform);
}

void InitializeHandleStaticWorld(HANDLE_STATIC_WORLD* world)
{
#define HANDLE_WORLD_AA_BB_SIZE 100000.0f
	static const float aa_bb_size[] = {HANDLE_WORLD_AA_BB_SIZE, HANDLE_WORLD_AA_BB_SIZE, HANDLE_WORLD_AA_BB_SIZE};
	world->world = BtDynamicsWorldNew(aa_bb_size);
	world->bodies = PointerArrayNew(DEFAULT_BUFFER_SIZE);
}

void HandleStaticWorldAddRigidBody(HANDLE_STATIC_WORLD* world, void* body)
{
	BtDynamicsWorldAddRigidBody(world->world, body);
	PointerArrayAppend(world->bodies, body);
}

void HandleStaticWorldRestoreObjects(HANDLE_STATIC_WORLD* world)
{
	void **objects;
	int num_objects;
	int i;

	objects = BtDynamicsWorldGetCollisionObjects(world->world, &num_objects);

	for(i = num_objects - 1; i >= 0; i--)
	{
		BtDynamicsWorldRemoveCollisionObject(world->world, (void*)objects[i]);
	}
	for(i=0; i<(int)world->bodies->num_data; i++)
	{
		BtDynamicsWorldAddRigidBody(world->world, world->bodies->buffer[i]);
	}

	BtDynamicsWorldStepSimulationSimple(world->world, 1);

	MEM_FREE_FUNC(objects);
}

void HandleStaticWorldFilterObjects(HANDLE_STATIC_WORLD* world, POINTER_ARRAY* visibles)
{
	void **objects;
	void *body;
	int num_objects;
	int i;

	objects = BtDynamicsWorldGetCollisionObjects(world->world, &num_objects);

	for(i = num_objects - 1; i >= 0; i--)
	{
		body = BtRigidBodyUpcast(objects[i]);
		if(PointerArrayDoesCointainData(visibles, body) == FALSE)
		{
			BtDynamicsWorldRemoveCollisionObject(world->world, body);
		}
	}

	MEM_FREE_FUNC(objects);
}

void InitializeBoneHandleMotionState(BONE_HANDLE_MOTION_STATE* state, CONTROL_HANDLE* handle)
{
	const float basis[] = IDENTITY_MATRIX3x3;
	state->bone_transform_states = ght_create(DEFAULT_BUFFER_SIZE);
	state->bone_transform_states_data = StructArrayNew(sizeof(BONE_TRANSFORM_STATE), DEFAULT_BUFFER_SIZE);
	state->selected_bones = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	state->view_transform = BtTransformNew(basis);
	state->handle = handle;
}

void InitializeControlHandle(
	CONTROL_HANDLE* handle,
	int width,
	int height,
	PROJECT* project,
	APPLICATION* application
)
{
	(void)memset(handle, 0, sizeof(*handle));
	InitializeHandleStaticWorld(&handle->world);
	InitializeHandleProgram(&handle->program);
	InitializeTextureDrawHelper(&handle->helper, width, height);
	InitializeBoneHandleMotionState(&handle->motion_state, handle);
	handle->set_visibility_bodies = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	handle->project_context = project;
	handle->application_context = application;
	handle->flags |= HANDLE_FLAG_VISIBLE | HANDLE_FLAG_VISIBLE_ALL;
}

void HandleModelBindVertexBundle(HANDLE_MODEL* model, int bundle)
{
	if(bundle == FALSE || VertexBundleLayoutBind(model->layout) == FALSE)
	{
		VertexBundleBind(model->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0);
		glVertexAttribPointer(HANDLE_PROGRAM_VERTEX_TYPE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(HANDLE_VERTEX), NULL);
		VertexBundleBind(model->bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0);
	}
}

void HandleModelReleaseVertexBundle(HANDLE_MODEL* model, int bundle)
{
	if(bundle == FALSE || VertexBundleLayoutUnbind(model->layout) == FALSE)
	{
		VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
		VertexBundleUnbind(VERTEX_BUNDLE_INDEX_BUFFER);
	}
}

void HandleModelSetVertexBuffer(HANDLE_MODEL* model, STRUCT_ARRAY* vertices, UINT32_ARRAY* indices)
{
	MakeVertexBundle(model->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0, GL_STATIC_DRAW, vertices->buffer,
		sizeof(HANDLE_VERTEX) * vertices->num_data);
	MakeVertexBundle(model->bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0, GL_STATIC_DRAW, indices->buffer,
		sizeof(unsigned int) * indices->num_data);
	(void)MakeVertexBundleLayout(model->layout);
	VertexBundleLayoutBind(model->layout);
	HandleModelBindVertexBundle(model, FALSE);
	HandleProgramEnableAttributes();
	(void)VertexBundleLayoutUnbind(model->layout);
	ShaderProgramUnbind(&model->handle->program.base_data);
	HandleModelReleaseVertexBundle(model, FALSE);
	model->num_indices = (int)indices->num_data;
}

void InitializeHandleModel(HANDLE_MODEL* model, CONTROL_HANDLE* handle)
{
	model->handle = handle;
	model->bundle = VertexBundleNew();
	model->layout = VertexBundleLayoutNew();
	model->vertices = StructArrayNew(sizeof(HANDLE_VERTEX), DEFAULT_BUFFER_SIZE);
	model->indices = Uint32ArrayNew(DEFAULT_BUFFER_SIZE);
}

void LoadHandleModel(HANDLE_MODEL* model, const struct aiMesh* mesh, STRUCT_ARRAY* vertices, UINT32_ARRAY* indices)
{
	HANDLE_VERTEX vertex = {{0, 0, 0, 1}, {0, 0, 0}};
	const struct aiVector3D *mesh_vertices = mesh->mVertices;
	const struct aiVector3D *mesh_normals = mesh->mNormals;
	const struct aiVector3D *v;
	const struct aiVector3D *n;
	const unsigned int num_faces = mesh->mNumFaces;
	const unsigned int num_vertices = mesh->mNumVertices;
	unsigned int i, j;

	indices->num_data = 0;
	for(i=0; i<num_faces; i++)
	{
		const struct aiFace *face = &mesh->mFaces[i];
		const unsigned int num_indices = face->mNumIndices;
		unsigned int vertex_index;
		for(j=0; j<num_indices; j++)
		{
			vertex_index = face->mIndices[j];
			Uint32ArrayAppend(indices, (uint32)vertex_index);
		}
	}

	vertices->num_data = 0;
	for(i=0; i<num_vertices; i++)
	{
		v = &mesh_vertices[i];
		n = &mesh_normals[i];
		vertex.position[0] = v->x;
		vertex.position[1] = v->y;
		vertex.position[2] = v->z;
		vertex.normal[0] = n->x;
		vertex.normal[1] = n->y;
		vertex.normal[2] = n->z;
		StructArrayAppend(vertices, (void*)&vertex);
	}

	HandleModelSetVertexBuffer(model, vertices, indices);
}

void LoadHandleModelWithState(HANDLE_MODEL* model, const struct aiMesh* mesh, HANDLE_STATIC_WORLD* world, void* state)
{
#define SCALE_FOR_HIT 1.3f
	HANDLE_VERTEX *vertices;
	void *triangle_mesh = BtTriangleMeshNew();
	static const float local_inertia[] = {0, 0, 0};
	void *shape;
	void *body;
	size_t num_faces;
	size_t index;
	uint32 *indices;
	float v[3][3];
	size_t i;

	LoadHandleModel(model, mesh, model->vertices, model->indices);
	vertices = (HANDLE_VERTEX*)model->vertices->buffer;
	indices = (uint32*)model->indices->buffer;
	num_faces = model->indices->num_data / 3;
	for(i=0; i<num_faces; i++)
	{
		index = i * 3;
		COPY_VECTOR3(v[0], vertices[indices[index + 0]].position);
		ScaleVector3(v[0], SCALE_FOR_HIT);
		COPY_VECTOR3(v[1], vertices[indices[index + 1]].position);
		ScaleVector3(v[1], SCALE_FOR_HIT);
		COPY_VECTOR3(v[2], vertices[indices[index + 2]].position);
		ScaleVector3(v[2], SCALE_FOR_HIT);
		BtTriangleMeshAddTriangle(triangle_mesh,
			v[0], v[1], v[2]
		);
		//BtTriangleMeshAddTriangle(triangle_mesh,
		//	vertices[indices[index + 0]].position,
		//	vertices[indices[index + 1]].position,
		//	vertices[indices[index + 2]].position
		//);
	}

	shape = BtBvhTriangleMeshShapeNew(triangle_mesh, TRUE);
	body = BtRigidBodyNew(0.0f, state, shape, local_inertia);

	BtRigidBodySetActivationState(body, COLLISION_ACTIVATE_STATE_DISABLE_DEACTIVATION);
	BtRigidBodySetCollisionFlags(body, BtRigidBodyGetCollisionFlags(body) | COLLISION_FLAG_KINEMATIC_OBJECT);
	BtRigidBodySetUserData(body, model);
	HandleStaticWorldAddRigidBody(world, body);
	model->body = body;
}

void LoadImageHandles(CONTROL_HANDLE* handle, PROJECT* project)
{
	TEXTURE_DATA_BRIDGE bridge;
	char utf8_path[8192];
	char *vertex_shader_path;
	char *fragment_shader_path;
	float texture_coord[8] = {0, 0, 1, 0, 1, -1, 0, -1};

	(void)sprintf(utf8_path, "%sgui/texture.vsh", project->application_context->paths.shader_directory_path);
	vertex_shader_path = MEM_STRDUP_FUNC(utf8_path);
	(void)sprintf(utf8_path, "%sgui/texture.fsh", project->application_context->paths.shader_directory_path);
	fragment_shader_path = MEM_STRDUP_FUNC(utf8_path);
	bridge.flags = TEXTURE_FLAG_TEXTURE_2D | TEXTURE_FLAG_ASYNC_LOADING_TEXTURE;
	LoadTextureDrawHelper(&handle->helper, texture_coord,
		vertex_shader_path, fragment_shader_path, project->application_context);
	MEM_FREE_FUNC(vertex_shader_path);
	MEM_FREE_FUNC(fragment_shader_path);

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/x-enable-move.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->x.enable_move.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/x-enable-move.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->x.enable_move.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/x-enable-rotate.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->x.enable_rotate.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/x-disable-move.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->x.disable_move.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/x-disable-rotate.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->x.disable_rotate.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/y-enable-move.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->y.enable_move.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/y-enable-rotate.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->y.enable_rotate.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/y-disable-move.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->y.disable_move.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/y-disable-rotate.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->y.disable_rotate.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/z-enable-move.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->z.enable_move.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/z-enable-rotate.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->z.enable_rotate.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/z-disable-move.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->z.disable_move.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/z-disable-rotate.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->z.disable_rotate.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/global.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->global.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/local.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->local.texture = (TEXTURE_2D*)bridge.texture;
	}

	(void)strcpy(utf8_path, project->application_context->paths.image_directory_path);
	(void)strcat(utf8_path, "icons/view.png");
	if(UploadTexture(utf8_path, &bridge, project) != FALSE)
	{
		handle->view.texture = (TEXTURE_2D*)bridge.texture;
	}
}

void LoadControlHandle(CONTROL_HANDLE* handle, PROJECT* project)
{
	if((handle->flags & HANDLE_FLAG_LOADED) == 0)
	{
		APPLICATION *application = project->application_context;
		char utf8_path[8192];
		char *system_path;

		MakeShaderProgram(&handle->program.base_data);
		(void)strcpy(utf8_path, application->paths.shader_directory_path);
		(void)strcat(utf8_path, "gui/handle.vsh");
		system_path = g_locale_from_utf8(utf8_path, -1, NULL, NULL, NULL);
		HandleProgramAddShader(&handle->program, system_path, GL_VERTEX_SHADER);
		g_free(system_path);
		(void)strcpy(utf8_path, application->paths.shader_directory_path);
		(void)strcat(utf8_path, "gui/handle.fsh");
		system_path = g_locale_from_utf8(utf8_path, -1, NULL, NULL, NULL);
		HandleProgramAddShader(&handle->program, system_path, GL_FRAGMENT_SHADER);
		g_free(system_path);

		if(HandleProgramLink(&handle->program) != FALSE)
		{
			void *state;
			const struct aiScene *scene;
			STRUCT_ARRAY *vertices;
			UINT32_ARRAY *indices;

			(void)strcpy(utf8_path, application->paths.model_directory_path);
			(void)strcat(utf8_path, "rotation.3ds");
			system_path = g_locale_from_utf8(utf8_path, -1, NULL, NULL, NULL);
			scene = aiImportFile(system_path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
			if(scene == NULL)
			{
				size_t data_size;
				uint8 *source = LoadSubstituteModel(system_path, &data_size);
				scene = aiImportFileFromMemory(source, (unsigned int)data_size,
					aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType, "3ds");
			}
			g_free(system_path);

			if(scene != NULL)
			{
				struct aiMesh **meshes = scene->mMeshes;
				state = BtMotionStateNew(&handle->motion_state, (void (*)(void*, void*))BoneHandleStateGetWorldTransform,
					(void (*)(const void*, void*))DummyFuncNoReturn2);
				InitializeHandleModel(&handle->rotation.x, handle);
				LoadHandleModelWithState(&handle->rotation.x, meshes[1], &handle->world, state);
				state = BtMotionStateNew(&handle->motion_state, (void (*)(void*, void*))BoneHandleStateGetWorldTransform,
					(void (*)(const void*, void*))DummyFuncNoReturn2);
				InitializeHandleModel(&handle->rotation.y, handle);
				LoadHandleModelWithState(&handle->rotation.y, meshes[0], &handle->world, state);
				state = BtMotionStateNew(&handle->motion_state, (void (*)(void*, void*))BoneHandleStateGetWorldTransform,
					(void (*)(const void*, void*))DummyFuncNoReturn2);
				InitializeHandleModel(&handle->rotation.z, handle);
				LoadHandleModelWithState(&handle->rotation.z, meshes[2], &handle->world, state);
			}

			(void)strcpy(utf8_path, application->paths.model_directory_path);
			(void)strcat(utf8_path, "translation.3ds");
			system_path = g_locale_from_utf8(utf8_path, -1, NULL, NULL, NULL);
			scene = aiImportFile(system_path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
			if(scene == NULL)
			{
				size_t data_size;
				uint8 *source = LoadSubstituteModel(system_path, &data_size);
				scene = aiImportFileFromMemory(source, (unsigned int)data_size,
					aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType, "3ds");
			}
			g_free(system_path);

			if(scene != NULL)
			{
				struct aiMesh **meshes = scene->mMeshes;
				state = BtMotionStateNew(&handle->motion_state, (void (*)(void*, void*))BoneHandleStateGetWorldTransform,
					(void (*)(const void*, void*))DummyFuncNoReturn2);
				InitializeHandleModel(&handle->translation.x, handle);
				LoadHandleModelWithState(&handle->translation.x, meshes[0], &handle->world, state);
				state = BtMotionStateNew(&handle->motion_state, (void (*)(void*, void*))BoneHandleStateGetWorldTransform,
					(void (*)(const void*, void*))DummyFuncNoReturn2);
				InitializeHandleModel(&handle->translation.y, handle);
				LoadHandleModelWithState(&handle->translation.y, meshes[2], &handle->world, state);
				state = BtMotionStateNew(&handle->motion_state, (void (*)(void*, void*))BoneHandleStateGetWorldTransform,
					(void (*)(const void*, void*))DummyFuncNoReturn2);
				InitializeHandleModel(&handle->translation.z, handle);
				LoadHandleModelWithState(&handle->translation.z, meshes[1], &handle->world, state);

				vertices = StructArrayNew(sizeof(HANDLE_VERTEX), DEFAULT_BUFFER_SIZE);
				indices = Uint32ArrayNew(DEFAULT_BUFFER_SIZE);
				InitializeHandleModel(&handle->translation.axis_x, handle);
				LoadHandleModel(&handle->translation.axis_x, meshes[3], vertices, indices);
				InitializeHandleModel(&handle->translation.axis_y, handle);
				LoadHandleModel(&handle->translation.axis_y, meshes[5], vertices, indices);
				InitializeHandleModel(&handle->translation.axis_z, handle);
				LoadHandleModel(&handle->translation.axis_z, meshes[4], vertices, indices);
				StructArrayDestroy(&vertices, NULL);
				Uint32ArrayDestroy(&indices);
			}
		}

		LoadImageHandles(handle, project);

		handle->flags |= HANDLE_FLAG_LOADED;
	}
}

void ControlHandleBeginDraw(CONTROL_HANDLE* handle)
{
	float matrix[16];
	float world[16], view[16], projection[16];
	float scale[3] = {handle->project_context->aspect_ratio, 1, 1};
	void *transform = ControlHandleGetModelHandleTransform(handle);
	PROJECT *project = handle->project_context;

	glDisable(GL_DEPTH_TEST);
	COPY_MATRIX4x4(projection, project->camera_projection_matrix);
	COPY_MATRIX4x4(view, project->camera_view_matrix);
	COPY_MATRIX4x4(world, project->camera_world_matrix);
	BtTransformGetOpenGLMatrix(transform, matrix);

	MulMatrix4x4(matrix, world, matrix);
	MulMatrix4x4(matrix, view, matrix);
	MulMatrix4x4(matrix, projection, matrix);

	ScaleMatrix4x4(matrix, scale);

	ShaderProgramBind(&handle->program.base_data);
	HandleProgramSetMatrix(&handle->program, matrix);
}

void ControlHandleFlushDraw(CONTROL_HANDLE* handle)
{
	ShaderProgramUnbind(&handle->program.base_data);
	glEnable(GL_DEPTH_TEST);
}

void ControlHandleDrawModel(CONTROL_HANDLE* handle, HANDLE_MODEL* model, uint8 color[4], int visiblity_flags)
{
	if(handle->flags & visiblity_flags)
	{
		float draw_color[4];
		if(model == handle->tracked_handle)
		{
			draw_color[0] = 1.0f,	draw_color[1] = 1.0f,	draw_color[2] = 0.0f,	draw_color[3] = 0.5f;
		}
		else
		{
			draw_color[0] = color[0] * DIV_PIXEL, draw_color[1] = color[1] * DIV_PIXEL,
				draw_color[2] = color[2] * DIV_PIXEL, draw_color[3] = color[3] * DIV_PIXEL;
		}
		HandleProgramSetColor(&handle->program, draw_color[0], draw_color[1], draw_color[2], draw_color[3]);
		HandleModelBindVertexBundle(model, TRUE);
		glDrawElements(GL_TRIANGLES, (GLsizei)model->indices->num_data, GL_UNSIGNED_INT, 0);
		HandleModelReleaseVertexBundle(model, TRUE);
	}
}

void DrawImageHandles(CONTROL_HANDLE* handle, BONE_INTERFACE* bone)
{
	const float default_position[] = {0, 0, 0};

	if((handle->flags & HANDLE_FLAG_VISIBLE) == 0)
	{
		return;
	}

	if(bone != NULL && bone->is_movable(bone) != FALSE)
	{
		TextureDrawHelperDraw(&handle->helper, handle->x.enable_move.rect, default_position,
			handle->x.enable_move.texture->base_data.interface_data.name);
		TextureDrawHelperDraw(&handle->helper, handle->y.enable_move.rect, default_position,
			handle->y.enable_move.texture->base_data.interface_data.name);
		TextureDrawHelperDraw(&handle->helper, handle->z.enable_move.rect, default_position,
			handle->z.enable_move.texture->base_data.interface_data.name);
	}
	else
	{
		TextureDrawHelperDraw(&handle->helper, handle->x.disable_move.rect, default_position,
			handle->x.disable_move.texture->base_data.interface_data.name);
		TextureDrawHelperDraw(&handle->helper, handle->y.disable_move.rect, default_position,
			handle->y.disable_move.texture->base_data.interface_data.name);
		TextureDrawHelperDraw(&handle->helper, handle->z.disable_move.rect, default_position,
			handle->z.disable_move.texture->base_data.interface_data.name);
	}

	if(bone != NULL && bone->is_rotatable(bone) != FALSE)
	{
		TextureDrawHelperDraw(&handle->helper, handle->x.enable_rotate.rect, default_position,
			handle->x.enable_rotate.texture->base_data.interface_data.name);
		TextureDrawHelperDraw(&handle->helper, handle->y.enable_rotate.rect, default_position,
			handle->y.enable_rotate.texture->base_data.interface_data.name);
		TextureDrawHelperDraw(&handle->helper, handle->z.enable_rotate.rect, default_position,
			handle->z.enable_rotate.texture->base_data.interface_data.name);
	}
	else
	{
		TextureDrawHelperDraw(&handle->helper, handle->x.disable_rotate.rect, default_position,
			handle->x.disable_rotate.texture->base_data.interface_data.name);
		TextureDrawHelperDraw(&handle->helper, handle->y.disable_rotate.rect, default_position,
			handle->y.disable_rotate.texture->base_data.interface_data.name);
		TextureDrawHelperDraw(&handle->helper, handle->z.disable_rotate.rect, default_position,
			handle->z.disable_rotate.texture->base_data.interface_data.name);
	}

	switch(handle->flags & ~(HANDLE_FLAG_VISIBLE | HANDLE_FLAG_LOADED))
	{
	case HANDLE_FLAG_VIEW:
		TextureDrawHelperDraw(&handle->helper, handle->view.rect, default_position, handle->view.texture->base_data.interface_data.name);
		break;
	case HANDLE_FLAG_LOCAL:
		TextureDrawHelperDraw(&handle->helper, handle->local.rect, default_position, handle->local.texture->base_data.interface_data.name);
		break;
	case HANDLE_FLAG_GLOBAL:
		TextureDrawHelperDraw(&handle->helper, handle->global.rect, default_position, handle->global.texture->base_data.interface_data.name);
		break;
	}
}

void DrawMoveHandle(CONTROL_HANDLE* handle)
{
	if((handle->flags & HANDLE_FLAG_VISIBLE) == 0 || (handle->program.base_data.flags & SHADER_PROGRAM_FLAG_LINKED) == 0
		|| handle->bone == NULL)
	{
		return;
	}
	if(handle->bone->is_movable(handle->bone) != FALSE && (handle->flags & HANDLE_FLAG_MOVE) != 0)
	{
		uint8 color[4] = {255, 0, 0, 127};
		ControlHandleBeginDraw(handle);
		ControlHandleDrawModel(handle, &handle->translation.x, color, HANDLE_FLAG_X);
		color[0] = 0,	color[1] = 255;
		ControlHandleDrawModel(handle, &handle->translation.y, color, HANDLE_FLAG_Y);
		color[1] = 0,	color[2] = 255;
		ControlHandleDrawModel(handle, &handle->translation.z, color, HANDLE_FLAG_Z);
		color[0] = 255,	color[2] = 0;
		ControlHandleDrawModel(handle, &handle->translation.axis_x, color, HANDLE_FLAG_X);
		color[0] = 0,	color[1] = 255;
		ControlHandleDrawModel(handle, &handle->translation.axis_y, color, HANDLE_FLAG_Y);
		color[1] = 0,	color[2] = 255;
		ControlHandleDrawModel(handle, &handle->translation.axis_y, color, HANDLE_FLAG_Z);
		ControlHandleFlushDraw(handle);
	}
}

void DrawRotationHandle(CONTROL_HANDLE* handle)
{
	if((handle->flags & HANDLE_FLAG_VISIBLE) == 0 || (handle->program.base_data.flags & SHADER_PROGRAM_FLAG_LINKED) == 0
		|| handle->bone == NULL)
	{
		return;
	}
	if(handle->bone->is_rotatable(handle->bone) != FALSE &&
		(handle->flags & HANDLE_FLAG_ROTATE) != 0)
	{
		uint8 color[4] = {255, 0, 0, 127};
		ControlHandleBeginDraw(handle);
		ControlHandleDrawModel(handle, &handle->rotation.x, color, HANDLE_FLAG_X);
		color[0] = 0,	color[1] = 255;
		ControlHandleDrawModel(handle, &handle->rotation.y, color, HANDLE_FLAG_Y);
		color[1] = 0,	color[2] = 255;
		ControlHandleDrawModel(handle, &handle->rotation.z, color, HANDLE_FLAG_Z);
		ControlHandleFlushDraw(handle);
	}
}

void ResizeHandle(CONTROL_HANDLE* handle, int width, int height)
{
#define BASE_Y 4
#define X_OFFSET 32
#define Y_OFFSET 40
	float base_x = (float)(width - 104);
	RectangleSetTopLeft(handle->x.enable_move.rect, base_x, BASE_Y);
	RectangleSetSize(handle->x.enable_move.rect, (float)(handle->x.enable_move.texture->base_data.size[0]),
		(float)(handle->x.enable_move.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->y.enable_move.rect, base_x + X_OFFSET, BASE_Y);
	RectangleSetSize(handle->y.enable_move.rect, (float)(handle->y.enable_move.texture->base_data.size[0]),
		(float)(handle->y.enable_move.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->z.enable_move.rect, base_x + X_OFFSET * 2, BASE_Y);
	RectangleSetSize(handle->z.enable_move.rect, (float)(handle->z.enable_move.texture->base_data.size[0]),
		(float)(handle->z.enable_move.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->x.disable_move.rect, base_x, BASE_Y);
	RectangleSetSize(handle->x.disable_move.rect, (float)(handle->x.disable_move.texture->base_data.size[0]),
		(float)(handle->x.disable_move.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->y.disable_move.rect, base_x + X_OFFSET, BASE_Y);
	RectangleSetSize(handle->y.disable_move.rect, (float)(handle->y.disable_move.texture->base_data.size[0]),
		(float)(handle->y.disable_move.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->z.disable_move.rect, base_x + X_OFFSET * 2, BASE_Y);
	RectangleSetSize(handle->z.disable_move.rect, (float)(handle->z.disable_move.texture->base_data.size[0]),
		(float)(handle->z.disable_move.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->x.enable_rotate.rect, base_x, BASE_Y);
	RectangleSetSize(handle->x.enable_rotate.rect, (float)(handle->x.enable_rotate.texture->base_data.size[0]),
		(float)(handle->x.enable_rotate.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->y.enable_rotate.rect, base_x + X_OFFSET, BASE_Y);
	RectangleSetSize(handle->y.enable_rotate.rect, (float)(handle->y.enable_rotate.texture->base_data.size[0]),
		(float)(handle->y.enable_rotate.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->z.enable_rotate.rect, base_x + X_OFFSET * 2, BASE_Y);
	RectangleSetSize(handle->z.enable_rotate.rect, (float)(handle->z.enable_rotate.texture->base_data.size[0]),
		(float)(handle->z.enable_rotate.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->x.disable_move.rect, base_x, BASE_Y);
	RectangleSetSize(handle->x.disable_rotate.rect, (float)(handle->x.disable_rotate.texture->base_data.size[0]),
		(float)(handle->x.disable_rotate.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->y.disable_rotate.rect, base_x + X_OFFSET, BASE_Y);
	RectangleSetSize(handle->y.disable_rotate.rect, (float)(handle->y.disable_rotate.texture->base_data.size[0]),
		(float)(handle->y.disable_rotate.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->z.disable_rotate.rect, base_x + X_OFFSET * 2, BASE_Y);
	RectangleSetSize(handle->z.disable_rotate.rect, (float)(handle->z.disable_rotate.texture->base_data.size[0]),
		(float)(handle->z.disable_rotate.texture->base_data.size[1]));
	RectangleSetTopLeft(handle->global.rect, base_x, BASE_Y + Y_OFFSET * 2);
	RectangleSetSize(handle->global.rect, (float)handle->global.texture->base_data.size[0], (float)handle->global.texture->base_data.size[1]);
	RectangleSetTopLeft(handle->local.rect, (float)(base_x + handle->global.texture->base_data.size[0] - handle->local.texture->base_data.size[0] / 2),
		BASE_Y + Y_OFFSET * 2);
	RectangleSetSize(handle->local.rect, (float)handle->local.texture->base_data.size[0], (float)handle->local.texture->base_data.size[1]);
	RectangleSetTopLeft(handle->local.rect, (float)(base_x + handle->global.texture->base_data.size[0] - handle->view.texture->base_data.size[0] / 2),
		BASE_Y + Y_OFFSET * 2);
	RectangleSetSize(handle->view.rect, (float)handle->view.texture->base_data.size[0], (float)handle->view.texture->base_data.size[1]);

	handle->helper.width = width,	handle->helper.height = height;
}

int ControlHandleTestHitModel(
	CONTROL_HANDLE* handle,
	float* ray_from,
	float* ray_to,
	int set_tracked,
	unsigned int* flags,
	float* pick
)
{
	int ret = FALSE;
	BONE_INTERFACE *bone = handle->bone;
	*flags &= ~(HANDLE_FLAG_VISIBLE_ALL);
	if(bone != NULL)
	{
		void *callback = BtClosestRayResultCallbackNew(ray_from, ray_to);
		BtDynamicsWorldRayTest(handle->world.world, ray_from, ray_to, callback);
		handle->tracked_handle = NULL;
		if(BtClosestRayResultCallbackHasHit(callback) != FALSE)
		{
			void *body = BtRigidBodyUpcast(BtClosestRayResultCallbackGetCollisionObject(callback));
			HANDLE_MODEL *model = (HANDLE_MODEL*)BtRigidBodyGetUserData(body);
			if(bone->is_movable(bone) != FALSE && (handle->flags & HANDLE_FLAG_MOVE) != 0)
			{
				if(model == &handle->translation.x && (handle->flags & HANDLE_FLAG_X) != 0)
				{
					*flags = HANDLE_FLAG_MODEL | HANDLE_FLAG_MOVE | HANDLE_FLAG_X;
				}
				else if(model == &handle->translation.y && (handle->flags & HANDLE_FLAG_Y) != 0)
				{
					*flags = HANDLE_FLAG_MODEL | HANDLE_FLAG_MOVE | HANDLE_FLAG_Y;
				}
				else if(model == &handle->translation.z && (handle->flags & HANDLE_FLAG_Z) != 0)
				{
					*flags = HANDLE_FLAG_MODEL | HANDLE_FLAG_MOVE | HANDLE_FLAG_Z;
				}
			}
			if(bone->is_rotatable(bone) != FALSE && (handle->flags & HANDLE_FLAG_ROTATE) != 0)
			{
				if(model == &handle->rotation.x && (handle->flags & HANDLE_FLAG_X) != 0)
				{
					*flags = HANDLE_FLAG_MODEL | HANDLE_FLAG_ROTATE | HANDLE_FLAG_X;
				}
				else if(model == &handle->rotation.y && (handle->flags & HANDLE_FLAG_Y) != 0)
				{
					*flags = HANDLE_FLAG_MODEL | HANDLE_FLAG_ROTATE | HANDLE_FLAG_Y;
				}
				else if(model == &handle->rotation.z && (handle->flags & HANDLE_FLAG_Z) != 0)
				{
					*flags = HANDLE_FLAG_MODEL | HANDLE_FLAG_ROTATE | HANDLE_FLAG_Z;
				}
			}
			if(set_tracked != FALSE)
			{
				handle->tracked_handle = model;
			}
			BtClosestRayResultCallbackGetHitPointWorld(callback, pick);
		}

		ret = (*flags & HANDLE_FLAG_VISIBLE_ALL) != 0;
		DeleteBtClosestRayResultCallback(callback);
	}

	return ret;
}

int ControlHandleTestHitImage(
	CONTROL_HANDLE* handle,
	float x,
	float y,
	int movable,
	int rotateable,
	int* flags,
	float rect[4]
)
{
	float test_x = x,	test_y = handle->helper.height - y;
	unsigned int local_flags = 0;
	*flags &= ~(HANDLE_FLAG_VISIBLE_ALL);
	if(movable != FALSE)
	{
		if(RectangleContainsPosition(handle->x.enable_move.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->x.enable_move.rect);
			local_flags |= HANDLE_FLAG_ENABLE | HANDLE_FLAG_MOVE | HANDLE_FLAG_X;
		}
		else if(RectangleContainsPosition(handle->y.enable_move.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->y.enable_move.rect);
			local_flags |= HANDLE_FLAG_ENABLE | HANDLE_FLAG_MOVE | HANDLE_FLAG_Y;
		}
		else if(RectangleContainsPosition(handle->z.enable_move.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->z.enable_move.rect);
			local_flags |= HANDLE_FLAG_ENABLE | HANDLE_FLAG_MOVE | HANDLE_FLAG_Z;
		}
	}
	else
	{
		if(RectangleContainsPosition(handle->x.disable_move.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->x.disable_move.rect);
			local_flags |= HANDLE_FLAG_DISABLE | HANDLE_FLAG_MOVE | HANDLE_FLAG_X;
		}
		else if(RectangleContainsPosition(handle->y.disable_move.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->y.disable_move.rect);
			local_flags |= HANDLE_FLAG_DISABLE | HANDLE_FLAG_MOVE | HANDLE_FLAG_Y;
		}
		else if(RectangleContainsPosition(handle->z.disable_move.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->z.disable_move.rect);
			local_flags |= HANDLE_FLAG_DISABLE | HANDLE_FLAG_MOVE | HANDLE_FLAG_Z;
		}
	}

	if(rotateable != FALSE)
	{
		if(RectangleContainsPosition(handle->x.enable_rotate.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->x.enable_rotate.rect);
			local_flags |= HANDLE_FLAG_ENABLE | HANDLE_FLAG_ROTATE | HANDLE_FLAG_X;
		}
		else if(RectangleContainsPosition(handle->y.enable_rotate.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->y.enable_rotate.rect);
			local_flags |= HANDLE_FLAG_ENABLE | HANDLE_FLAG_ROTATE | HANDLE_FLAG_Y;
		}
		else if(RectangleContainsPosition(handle->z.enable_rotate.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->z.enable_rotate.rect);
			local_flags |= HANDLE_FLAG_ENABLE | HANDLE_FLAG_ROTATE | HANDLE_FLAG_Z;
		}
	}
	else
	{
		if(RectangleContainsPosition(handle->x.disable_rotate.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->x.disable_rotate.rect);
			local_flags |= HANDLE_FLAG_DISABLE | HANDLE_FLAG_ROTATE | HANDLE_FLAG_X;
		}
		else if(RectangleContainsPosition(handle->y.disable_rotate.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->y.disable_rotate.rect);
			local_flags |= HANDLE_FLAG_DISABLE | HANDLE_FLAG_ROTATE | HANDLE_FLAG_Y;
		}
		else if(RectangleContainsPosition(handle->z.disable_rotate.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->z.disable_rotate.rect);
			local_flags |= HANDLE_FLAG_DISABLE | HANDLE_FLAG_ROTATE | HANDLE_FLAG_Z;
		}
	}

	switch(handle->constraint)
	{
	case HANDLE_FLAG_VIEW:
		if(RectangleContainsPosition(handle->view.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->view.rect);
			local_flags = HANDLE_FLAG_VIEW;
		}
		break;
	case HANDLE_FLAG_LOCAL:
		if(RectangleContainsPosition(handle->local.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->local.rect);
			local_flags = HANDLE_FLAG_LOCAL;
		}
		break;
	case HANDLE_FLAG_GLOBAL:
		if(RectangleContainsPosition(handle->global.rect, test_x, test_y) != FALSE)
		{
			COPY_VECTOR4(rect, handle->global.rect);
			local_flags = HANDLE_FLAG_GLOBAL;
		}
		break;
	}

	*flags |= local_flags;
	return local_flags != HANDLE_FLAG_NONE;
}

void ControlHandleSetVisibilityFlags(CONTROL_HANDLE* handle, unsigned int flags)
{
	HandleStaticWorldRestoreObjects(&handle->world);
	if((flags & ~(HANDLE_FLAG_VISIBLE_ALL)) != HANDLE_FLAG_VISIBLE_ALL)
	{
		handle->set_visibility_bodies->num_data = 0;
		if((flags & HANDLE_FLAG_MOVE) != 0)
		{
			if((flags & HANDLE_FLAG_X) != 0)
			{
				PointerArrayAppend(handle->set_visibility_bodies, handle->translation.x.body);
			}
			if((flags & HANDLE_FLAG_Y) != 0)
			{
				PointerArrayAppend(handle->set_visibility_bodies, handle->translation.y.body);
			}
			if((flags & HANDLE_FLAG_Z) != 0)
			{
				PointerArrayAppend(handle->set_visibility_bodies, handle->translation.z.body);
			}
		}
		if((flags & HANDLE_FLAG_ROTATE) != 0)
		{
			if((flags & HANDLE_FLAG_X) != 0)
			{
				PointerArrayAppend(handle->set_visibility_bodies, handle->rotation.x.body);
			}
			if((flags & HANDLE_FLAG_Y) != 0)
			{
				PointerArrayAppend(handle->set_visibility_bodies, handle->rotation.y.body);
			}
			if((flags & HANDLE_FLAG_Z) != 0)
			{
				PointerArrayAppend(handle->set_visibility_bodies, handle->rotation.z.body);
			}
		}
		HandleStaticWorldFilterObjects(&handle->world, handle->set_visibility_bodies);
	}
	handle->flags &= ~(HANDLE_FLAG_VISIBLE_ALL);
	handle->flags |= flags;
}

void ControlHandleSetBone(CONTROL_HANDLE* handle, BONE_INTERFACE* bone)
{
	handle->bone = bone;
	BtDynamicsWorldStepSimulationSimple(handle->world.world, 1);
}

static void TranslateFromView(void* transform, float delta[3], float vector[3])
{
	float matrix[9];
	BtTransformGetBasis(transform, matrix);
	vector[0] = matrix[0] * delta[0];
	vector[0] += matrix[3] * delta[1];
	vector[0] += matrix[6] * delta[2];
	vector[1] = matrix[1] * delta[0];
	vector[1] += matrix[4] * delta[1];
	vector[1] += matrix[7] * delta[2];
	vector[2] = matrix[2] * delta[0];
	vector[2] += matrix[5] * delta[1];
	vector[2] += matrix[8] * delta[2];
}

void BoneMotionModelTranslateInternal(
	BONE_HANDLE_MOTION_STATE* state,
	float translation[3],
	float delta[3],
	BONE_INTERFACE* bone,
	unsigned int flags
)
{
	SCENE *scene = state->handle->project_context->scene;
	float local_delta[3];
	float vector[3];
	float rotation[4];
	void *transform;

	COPY_VECTOR3(local_delta, delta);

	if((state->handle->flags & HANDLE_FLAG_X) != 0)
	{
		local_delta[1] = local_delta[2] = 0;
	}
	else if((state->handle->flags & HANDLE_FLAG_Y) != 0)
	{
		local_delta[0] = local_delta[2] = 0;
	}
	else if((state->handle->flags & HANDLE_FLAG_Z) != 0)
	{
		local_delta[0] = local_delta[1] = 0;
	}

	switch(flags & 0xff)
	{
	case 'V':	// ビュー変形(カメラ視点)
		bone->get_local_rotation(bone, rotation);
		transform = BtTransformNewWithQuaternion(rotation, translation);
		TranslateFromView(state->view_transform, local_delta, vector);
		BtTransformMultiVector3(transform, vector, vector);
		DeleteBtTransform(transform);
		break;
	case 'L':	// ローカル変形
		bone->get_local_rotation(bone, rotation);
		transform = BtTransformNewWithQuaternion(rotation, translation);
		BtTransformMultiVector3(transform, local_delta, vector);
		DeleteBtTransform(transform);
		break;
	case 'G':	// グローバル変形
		vector[0] = translation[0] + local_delta[0];
		vector[1] = translation[1] + local_delta[1];
		vector[2] = translation[2] + local_delta[2];
		break;
	}

	bone->set_local_translation(bone, vector);
	//COPY_VECTOR3(keyframe->position, vector);
	WorldStepSimulation(&scene->project->world, scene->project->world.fixed_time_step);
	SceneUpdateModel(scene, scene->selected_model, TRUE);
}

void BoneMotionModelTranslateTo(
	BONE_HANDLE_MOTION_STATE* state,
	float translation[3],
	BONE_INTERFACE* bone,
	unsigned int flags
)
{
	float last_translation[3] = {0, 0, 0};
	BONE_TRANSFORM_STATE *motion;

	if(bone == NULL)
	{
		if(state->selected_bones->num_data > 0)
		{
			bone = (BONE_INTERFACE*)state->selected_bones->buffer[
				state->selected_bones->num_data-1];
		}
		else
		{
			return;
		}
	}

	if((motion = (BONE_TRANSFORM_STATE*)ght_get(state->bone_transform_states,
		sizeof(bone), bone)) != NULL)
	{
		COPY_VECTOR3(last_translation, motion->position);
	}

	BoneMotionModelTranslateInternal(state, last_translation, translation, bone, flags);
}

static void RotateViewAxisAngle(
	BONE_INTERFACE* bone,
	const float angle,
	unsigned int flags,
	void* transform,
	float* quaternion
)
{
	float radian = angle * (float)M_PI / 180.0f;
	LOAD_IDENTITY_QUATERNION(quaternion);

	if(bone->has_fixed_axis(bone) != FALSE)
	{
		float axis[3];
		bone->get_fixed_axis(bone, axis);
		QuaternionSetRotation(quaternion, axis, radian);
	}
	else
	{
		float matrix[9];
		BtTransformGetBasis(transform, matrix);

		switch((flags & 0xff00) >> 8)
		{
		case 'X':
			QuaternionSetRotation(quaternion, matrix, - radian);
			break;
		case 'Y':
			QuaternionSetRotation(quaternion, &matrix[3], - radian);
			break;
		case 'Z':
			QuaternionSetRotation(quaternion, &matrix[6], - radian);
			break;
		}
	}
}

static void RotateLocalAxisAngle(
	BONE_INTERFACE* bone,
	const float angle,
	unsigned int flags,
	float* rotation,
	float* quaternion
)
{
	float radian = angle * (float)M_PI / 180.0f;
	LOAD_IDENTITY_QUATERNION(quaternion);

	if(bone->has_fixed_axis(bone) != FALSE)
	{
		float axis[3];
		bone->get_fixed_axis(bone, axis);
		QuaternionSetRotation(quaternion, axis, radian);
	}
	else
	{
		float axis_x[] = {1, 0, 0};
		float axis_y[] = {0, 1, 0};
		float axis_z[] = {0, 0, 1};
		if(bone->has_local_axis(bone) != FALSE)
		{
			float matrix[] = IDENTITY_MATRIX3x3;
			bone->get_local_axes(bone, matrix);
			COPY_VECTOR3(axis_x, matrix);
			COPY_VECTOR3(axis_y, &matrix[3]);
			COPY_VECTOR3(axis_z, &matrix[6]);
		}
		else
		{
			float matrix[9];
			RotateMatrix3x3(matrix, rotation);
			MulVector3(axis_x, matrix);
			MulVector3(axis_y, &matrix[3]);
			MulVector3(axis_z, &matrix[6]);
		}

		switch((flags & 0xff00) >> 8)
		{
		case 'X':
			QuaternionSetRotation(quaternion, axis_x, - radian);
			break;
		case 'Y':
			QuaternionSetRotation(quaternion, axis_y, - radian);
			break;
		case 'Z':
			QuaternionSetRotation(quaternion, axis_z, - radian);
			break;
		}
	}
}

static void RotateGlobalAxisAngle(
	BONE_INTERFACE* bone,
	const float angle,
	unsigned int flags,
	float* quaternion
)
{
	float radian = angle * (float)M_PI / 180.0f;
	LOAD_IDENTITY_QUATERNION(quaternion);

	if(bone->has_fixed_axis(bone) != FALSE)
	{
		float axis[3];
		bone->get_fixed_axis(bone, axis);
		QuaternionSetRotation(quaternion, axis, radian);
	}
	else
	{
		float axis[3] = {0, 0, 0};
		switch((flags & 0xff00) >> 8)
		{
		case 'X':
			axis[0] = 1;
			QuaternionSetRotation(quaternion, axis, - radian);
			break;
		case 'Y':
			axis[1] = 1;
			QuaternionSetRotation(quaternion, axis, - radian);
			break;
		case 'Z':
			axis[2] = 1;
			QuaternionSetRotation(quaternion, axis, - radian);
			break;
		}
	}
}

void BoneMotionModelRotateAngle(
	BONE_HANDLE_MOTION_STATE* state,
	float angle,
	BONE_INTERFACE* bone,
	unsigned int flags
)
{
	SCENE *scene = state->handle->project_context->scene;
	float *bone_rotation;
	//QUATERNION bone_rotation;
	QUATERNION last_rotation = IDENTITY_QUATERNION;
	QUATERNION set_rotation;
	BONE_TRANSFORM_STATE *motion;
	BONE_KEYFRAME_INTERFACE *keyframe;

	if(bone == NULL)
	{
		if(state->selected_bones->num_data > 0)
		{
			bone = (BONE_INTERFACE*)state->selected_bones->buffer[
				state->selected_bones->num_data-1];
		}
		else
		{
			return;
		}
	}
	keyframe = (BONE_KEYFRAME_INTERFACE*)SceneGetCurrentKeyframe(
		scene, KEYFRAME_TYPE_BONE, bone->name);
	bone_rotation = keyframe->rotation;
	//bone->get_local_rotation(bone, bone_rotation);

	if((motion = (BONE_TRANSFORM_STATE*)ght_get(state->bone_transform_states,
		sizeof(bone), bone)) != NULL)
	{
		COPY_VECTOR4(last_rotation, motion->rotation);
	}
	switch(flags & 0xff)
	{
	case 'V':	// ビュー変形
		RotateViewAxisAngle(bone, angle, flags, state->view_transform, set_rotation);
		MultiQuaternion(last_rotation, set_rotation);
		MultiQuaternion(last_rotation, bone_rotation);
		COPY_VECTOR4(bone_rotation, last_rotation);
		//bone->set_local_rotation(bone, last_rotation);
		break;
	case 'L':	// ローカル変形
		RotateLocalAxisAngle(bone, angle, flags, last_rotation, set_rotation);
		MultiQuaternion(last_rotation, set_rotation);
		MultiQuaternion(last_rotation, bone_rotation);
		COPY_VECTOR4(bone_rotation, last_rotation);
		//bone->set_local_rotation(bone, last_rotation);
		break;
	case 'G':	// グローバル変型
		RotateGlobalAxisAngle(bone, angle, flags, set_rotation);
		MultiQuaternion(last_rotation, set_rotation);
		MultiQuaternion(last_rotation, bone_rotation);
		COPY_VECTOR4(bone_rotation, last_rotation);
		//bone->set_local_rotation(bone, last_rotation);
		break;
	}

	bone->set_local_rotation(bone, last_rotation);
	WorldStepSimulation(&scene->project->world, scene->project->world.fixed_time_step);
	SceneUpdateModel(scene, scene->selected_model, TRUE);
}

void InitializeControl(CONTROL* control, int width, int height, PROJECT* project)
{
	(void)memset(control, 0, sizeof(*control));
	InitializeControlHandle(&control->handle, width, height, project,
		project->application_context);
}

void ControlSetEditMode(CONTROL* control, eEDIT_MODE mode)
{
	switch(mode)
	{
	case EDIT_MODE_MOVE:
		ControlHandleSetVisibilityFlags(&control->handle, HANDLE_FLAG_VISIBLE_MOVE);
		break;
	case EDIT_MODE_ROTATE:
		ControlHandleSetVisibilityFlags(&control->handle, HANDLE_FLAG_VISIBLE_ROTATE);
		break;
	default: ControlHandleSetVisibilityFlags(&control->handle, HANDLE_FLAG_NONE);
			break;
	}
	control->edit_mode = mode;
}

void ControlRotateBone(CONTROL* control, BONE_INTERFACE* bone, FLOAT_T diff[2])
{
	float distance = (float)sqrt(diff[0]*diff[0] + diff[1]*diff[1]);
	float angle = - ((float)diff[0] + (float)diff[1]) * 0.25f;
	unsigned int mode = HandleModeFromConstraint(&control->handle);
	unsigned int axis = 0;

	if((control->handle.flags & HANDLE_FLAG_X) != 0)
	{
		axis = 'X' << 8;
	}
	else if((control->handle.flags & HANDLE_FLAG_Y) != 0)
	{
		axis = 'Y' << 8;
	}
	else
	{
		axis = 'Z' << 8;
	}

	BoneMotionModelRotateAngle(&control->handle.motion_state,
		angle, bone, mode | axis);
}

#ifdef __cplusplus
}
#endif
