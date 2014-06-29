#include <string.h>
#include <limits.h>
#include "rigid_body.h"
#include "bullet.h"
#include "pmx_model.h"
#include "pmd_model.h"
#include "application.h"
#include "memory_stream.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void RigidBodySyncLocalTransform(void* rigid_body, int index, void* dummy)
{
	BASE_RIGID_BODY *body = (BASE_RIGID_BODY*)rigid_body;
	BaseRigidBodySyncLocalTransform(body);
}

#define PMX_RIGID_BODY_UNIT_SIZE 61
typedef struct _PMX_RIGID_BODY_UNIT
{
	uint8 collision_group_id;
	uint16 collision_mask;
	uint8 shape_type;
	float size[3];
	float position[3];
	float rotation[3];
	float mass;
	float linear_damping;
	float angular_damping;
	float restitution;
	float friction;
	uint8 type;
} PMX_RIGID_BODY_UNIT;

#define PMD2_RIGID_BODY_UNIT_SIZE 83
typedef struct _PMD2_RIGID_BODY_UNIT
{
	uint8 name[PMD2_RIGID_BODY_NAME_SIZE];
	uint16 bone_id;
	uint8 collision_group_id;
	uint16 collision_mask;
	uint8 shape_type;
	float size[3];
	float position[3];
	float rotation[3];
	float mass;
	float linear_damping;
	float angular_damping;
	float restitution;
	float friction;
	uint8 type;
} PMD2_RIGID_BODY_UNIT;

void InitializeBaseRigidBody(BASE_RIGID_BODY* body, void* parent, void* application_context)
{
	APPLICATION *application = (APPLICATION*)application_context;
	const float identity[] = IDENTITY_MATRIX3x3;
	(void)memset(body, 0, sizeof(*body));
	body->world_transform = BtTransformNew(identity);
	body->world2local_transform = BtTransformNew(identity);
	body->parent_model = (MODEL_INTERFACE*)parent;
	body->encode = &application->encode;
	body->bone = (void*)&application->default_data.bone;
	body->bone_index = -1;
	body->index = -1;
	body->shape_type = COLLISION_SHAPE_TYPE_UNKNOWN;
	body->type = RIGID_BODY_OBJECT_TYPE_STATIC;
	body->interface_data.application = application;
}

void ReleaseBaseRigidBody(BASE_RIGID_BODY* body)
{
	DeleteBtMotionState(body->active_motion_state);
	DeleteBtMotionState(body->kinematic_motion_state);
	DeleteBtRigidBody(body->body);
	DeleteBtCollisionShape(body->collision_shape);
	DeleteBtTransform(body->motion_state.start_transform);
	DeleteBtTransform(body->motion_state.world_transform);
	DeleteBtTransform(body->kinematic_state.start_transform);
	DeleteBtTransform(body->kinematic_state.world_transform);
	DeleteBtTransform(body->world_transform);
	DeleteBtTransform(body->world2local_transform);
	MEM_FREE_FUNC(body->name);
	MEM_FREE_FUNC(body->english_name);
}

void InitializeBaseRigidBodyMotionState(
	BASE_RIGID_BODY_MOTION_STATE* state,
	void* start_transform,
	BASE_RIGID_BODY* parent
)
{
	(void)memset(state, 0, sizeof(*state));
	state->start_transform = BtTransformCopy(start_transform);
	state->world_transform = BtTransformCopy(start_transform);
	state->parent = parent;
}

void BaseRigidBodyMotionStateGetDefaultTransform(void* setup_transform, BASE_RIGID_BODY_MOTION_STATE* state)
{
	BtTransformSet(setup_transform, state->world_transform);
}

void BaseRigidBodyMotionStateSetDefaultTransform(void* transform, BASE_RIGID_BODY_MOTION_STATE* state)
{
	BtTransformSet(state->world_transform, transform);
}

void BaseRigidBodyMotionStateGetKinematicTransform(void* setup_transform, BASE_RIGID_BODY_MOTION_STATE* state)
{
	if(state->parent->bone != NULL)
	{
		static const float basis[] = IDENTITY_MATRIX3x3;
		uint8 local_transform[TRANSFORM_SIZE];
		BONE_INTERFACE *inter = (BONE_INTERFACE*)state->parent->bone;
		inter->get_local_transform(state->parent->bone, local_transform);
		BtTransformMulti(local_transform, state->start_transform, setup_transform);
	}
	else
	{
		BtTransformSetIdentity(setup_transform);
	}
}

void BaseRigidBodySetBone(BASE_RIGID_BODY* body, void* bone)
{
	if(bone != body->bone)
	{
		// ADD_QUEUE_EVENT
		if(bone != NULL)
		{
			BONE_INTERFACE *inter = (BONE_INTERFACE*)bone;
			inter->body = body;
			body->bone_index = inter->index;
			body->bone = bone;
			inter->set_inverse_kinematics_enable(
				bone, body->type == RIGID_BODY_OBJECT_TYPE_STATIC);
		}
		else
		{
			body->bone = (void*)&body->interface_data.application->default_data.bone;
			body->bone_index = -1;
		}
	}
}

void* BaseRigidBodyTransformNew(BASE_RIGID_BODY* body)
{
	float mx[9], my[9], mz[9];
	Matrix3x3SetEulerZYX(mx, -body->rotation[0], 0, 0);
	Matrix3x3SetEulerZYX(my, 0, -body->rotation[1], 0);
	Matrix3x3SetEulerZYX(mz, 0, 0, body->rotation[2]);

	MulMatrix3x3(my, mz);
	MulMatrix3x3(my, mx);

	return BtTransformNewWithVector(my, body->position);
}

void* BaseRigidBodyCreate(BASE_RIGID_BODY* body, void* shape)
{
	float local_inertia[3] = {0, 0, 0};
	float mass_value = 0;
	if(body->type != RIGID_BODY_OBJECT_TYPE_STATIC)
	{
		mass_value = body->mass;
		if(shape != NULL && !FuzzyZero(mass_value))
		{
			BtCollisionShapeCalculateLocalInertia(shape, mass_value, local_inertia);
		}
	}
	if(body->world_transform != NULL)
	{
		DeleteBtTransform(body->world_transform);
	}
	if(body->world2local_transform != NULL)
	{
		DeleteBtTransform(body->world2local_transform);
	}
	body->world_transform = body->parent_model->rigid_body_transform_new(body);
	body->world2local_transform = BtTransformCopy(body->world_transform);
	BtTransformInverse(body->world2local_transform);

	InitializeBaseRigidBodyMotionState(
		&body->motion_state, body->world_transform, body);
	InitializeBaseRigidBodyMotionState(
		&body->kinematic_state, body->world_transform, body);
	switch(body->type)
	{
	default:
	case RIGID_BODY_OBJECT_TYPE_STATIC:
		body->active_motion_state = BtMotionStateNew(
			&body->motion_state,
			(void (*)(void*, void*))BaseRigidBodyMotionStateGetKinematicTransform,
			(void (*)(const void*, void*))BaseRigidBodyMotionStateSetDefaultTransform
		);
		body->kinematic_motion_state = NULL;
		break;
	case RIGID_BODY_OBJECT_TYPE_DYNAMIC:
	case RIGID_BODY_OBJECT_TYPE_ALIGNED:
		body->active_motion_state = BtMotionStateNew(
			&body->motion_state,
			(void (*)(void*, void*))BaseRigidBodyMotionStateGetDefaultTransform,
			(void (*)(const void*, void*))BaseRigidBodyMotionStateSetDefaultTransform
		);
		body->kinematic_motion_state = BtMotionStateNew(
			&body->kinematic_state,
			(void (*)(void*, void*))BaseRigidBodyMotionStateGetKinematicTransform,
			(void (*)(const void*, void*))BaseRigidBodyMotionStateSetDefaultTransform
		);
		break;
	}
	body->body_ptr = BtRigidBodyNewWithDetailData(
		mass_value, body->active_motion_state, shape, local_inertia,
		body->linear_damping, body->angular_damping, body->restitution,
		body->friction, TRUE
	);
	BtRigidBodySetActivationState(body->body_ptr, COLLISION_ACTIVATE_STATE_DISABLE_DEACTIVATION);
	BtRigidBodySetUserData(body->body_ptr, body);
	switch(body->type)
	{
	case RIGID_BODY_OBJECT_TYPE_STATIC:
		BtRigidBodySetCollisionFlags(body->body_ptr,
			BtRigidBodyGetCollisionFlags(body->body_ptr) | COLLISION_FLAG_KINEMATIC_OBJECT);
		break;
	default:
		break;
	}

	return body->body_ptr;
}

void BaseRigidBodyBuild(BASE_RIGID_BODY* body, void* bone, int index)
{
	BaseRigidBodySetBone(body, bone);
	body->collision_shape = BtCollisionShapeNew(
		body->shape_type, body->size);
	body->body = BaseRigidBodyCreate(body, body->collision_shape);
	body->body_ptr = NULL;
	body->index = index;
}

void BaseRigidBodySyncLocalTransform(BASE_RIGID_BODY* body)
{
	if(body->type != RIGID_BODY_OBJECT_TYPE_STATIC && body->bone != NULL)
	{
		static const float basis[] = IDENTITY_MATRIX3x3;
		BONE_INTERFACE *bone = (BONE_INTERFACE*)body->bone;
		uint8 world_bone_transform[TRANSFORM_SIZE];
		uint8 center_of_mass_transform[TRANSFORM_SIZE];
		BtRigidBodyGetCenterOfMassTransform(body->body, center_of_mass_transform);
		BtTransformSet(world_bone_transform, bone->local_transform);
		//bone->get_local_transform(bone, world_bone_transform);

		BtTransformMulti(world_bone_transform, body->world_transform, world_bone_transform);
		if(body->type == RIGID_BODY_OBJECT_TYPE_ALIGNED)
		{
			float origin[3];
			BtTransformGetOrigin(world_bone_transform, origin);
			BtTransformSetOrigin(center_of_mass_transform, origin);
			BtRigidBodySetCenterOfMassTransform(body->body, center_of_mass_transform);
		}

		BtTransformMulti(center_of_mass_transform, body->world2local_transform, center_of_mass_transform);
		bone->set_local_transform(bone, center_of_mass_transform);
	}
}

void BaseRigidBodyJoinWorld(BASE_RIGID_BODY* body, void* world)
{
	BtDynamicsWorldAddRigidBodyWithDetailData(world, body->body, body->group_id, body->collision_group_mask);
	BtRigidBodySetUserData(body->body, body);
}

void BaseRigidBodyLeaveWorld(BASE_RIGID_BODY* body, void* world)
{
	BtDynamicsWorldRemoveRigidBody(world, body->body);
	BtRigidBodySetUserData(body->body, NULL);
}

void BaseRigidBodyUpdateTransform(BASE_RIGID_BODY* body)
{
	MODEL_INTERFACE *model = body->parent_model;
	void *transform = model->scene->project->application_context->transform;
	BONE_INTERFACE *bone = (BONE_INTERFACE*)body->bone;
	bone->get_local_transform(bone, transform);
	BtMotionStateSetWorldTransform(body->active_motion_state, transform);
	BtRigidBodySetInterpolationWorldTransform(body->body, transform);
}

void BaseRigidBodySetActivation(BASE_RIGID_BODY* body, int active)
{
	if(body->type != RIGID_BODY_OBJECT_TYPE_STATIC)
	{
		if(active != FALSE)
		{
			BtRigidBodySetCollisionFlags(body->body,
				BtRigidBodyGetCollisionFlags(body->body) & ~(COLLISION_FLAG_KINEMATIC_OBJECT));
			BtRigidBodySetMotionState(body->body, body->active_motion_state);
		}
		else
		{
			BtRigidBodySetCollisionFlags(body->body,
				BtRigidBodyGetCollisionFlags(body->body) | COLLISION_FLAG_KINEMATIC_OBJECT);
			BtRigidBodySetMotionState(body->body, body->kinematic_motion_state);
		}
	}
	else
	{
		BtRigidBodySetMotionState(body->body, body->active_motion_state);
	}
}

void ResetBaseRigidBody(BASE_RIGID_BODY* body, void* world)
{
	void *cache = BtDynamicsWorldGetPairCache(world);
	if(cache != NULL)
	{
		void *dispatcher = BtDynamicsWorldGetDispatcher(world);
		BtOverlappingPairCacheCleanProxyFromPairs(cache,
			BtRigidBodyGetBroadphaseHandle(body->body), dispatcher);
	}
	BtRigidBodyClearForces(body->body);
}

void InitializePmxRigidBody(PMX_RIGID_BODY* body, PMX_MODEL* model, void* application_context)
{
	(void)memset(body, 0, sizeof(*body));
	InitializeBaseRigidBody(body, (void*)model, application_context);
}

int PmxRigidBodyPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, rest, 1};
	int32 num_bodies;
	int bone_index_size = (int)info->bone_index_size;
	char *name_ptr;
	int length;
	int i;

	if(MemRead(&num_bodies, sizeof(num_bodies), 1, &stream) == 0)
	{
		return FALSE;
	}

	info->rigid_bodies = &data[stream.data_point];
	for(i=0; i<num_bodies; i++)
	{
		// 日本語名
		if((length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;

		// 英語名
		if((length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;

		if(MemSeek(&stream, PMX_RIGID_BODY_UNIT_SIZE + bone_index_size, SEEK_CUR) != 0)
		{
			return FALSE;
		}
	}

	info->rigid_bodies_count = num_bodies;
	*data_size = stream.data_point;

	return TRUE;
}

int PmxRigidBodyLoad(STRUCT_ARRAY* bodies, STRUCT_ARRAY* bones)
{
	const int num_rigid_bodies = (int)bodies->num_data;
	const int num_bones = (int)bones->num_data;
	PMX_RIGID_BODY *rigid_bodies = (PMX_RIGID_BODY*)bodies->buffer;
	PMX_BONE *b = (PMX_BONE*)bones->buffer;
	int i;

	for(i=0; i<num_rigid_bodies; i++)
	{
		PMX_RIGID_BODY *rigid_body = &rigid_bodies[i];
		const int bone_index = rigid_body->bone_index;
		if(bone_index >= 0)
		{
			if(bone_index < num_bones)
			{
				PMX_BONE* bone = &b[bone_index];
				BaseRigidBodyBuild(rigid_body, (void*)bone, i);
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			void *parent_model = rigid_body->parent_model;
			TEXT_ENCODE *encode = rigid_body->encode;
			MODEL_INTERFACE *inter = (MODEL_INTERFACE*)parent_model;
			void *bone = inter->find_bone(parent_model,
				// センター
				"\xE3\x82\xBB\xE3\x83\xB3\xE3\x82\xBF\xE3\x83\xBC");
			BaseRigidBodyBuild(rigid_body, bone, i);
		}
	}

	return TRUE;
}

void PmxRigidBodyRead(
	PMX_RIGID_BODY* body,
	uint8* data,
	PMX_DATA_INFO* info,
	size_t* data_size
)
{
	MEMORY_STREAM stream = {data, 0, (size_t)(info->end - data), 1};
	PMX_RIGID_BODY_UNIT unit;
	char *name_ptr;
	int name_size;
	TEXT_ENCODE *encode = info->encoding;

	// 日本語名
	name_size = GetTextFromStream((char*)data, &name_ptr);
	body->name = EncodeText(encode, name_ptr, name_size);
	stream.data_point += sizeof(int32) + name_size;

	// 英語名
	name_size = GetTextFromStream((char*)&data[stream.data_point], &name_ptr);
	body->english_name = EncodeText(encode, name_ptr, name_size);
	stream.data_point += sizeof(int32) + name_size;

	body->bone_index = GetSignedValue(&data[stream.data_point], (int)info->bone_index_size);
	stream.data_point += info->bone_index_size;

	unit.collision_group_id = data[stream.data_point];
	stream.data_point++;
	(void)MemRead(&unit.collision_mask, sizeof(unit.collision_mask), 1, &stream);
	unit.shape_type = data[stream.data_point];
	stream.data_point++;
	(void)MemRead(unit.size, sizeof(unit.size), 1, &stream);
	(void)MemRead(unit.position, sizeof(unit.position), 1, &stream);
	(void)MemRead(unit.rotation, sizeof(unit.rotation), 1, &stream);
	(void)MemRead(&unit.mass, sizeof(unit.mass), 1, &stream);
	(void)MemRead(&unit.linear_damping, sizeof(unit.linear_damping), 1, &stream);
	(void)MemRead(&unit.angular_damping, sizeof(unit.angular_damping), 1, &stream);
	(void)MemRead(&unit.restitution, sizeof(unit.restitution), 1, &stream);
	(void)MemRead(&unit.friction, sizeof(unit.friction), 1, &stream);
	unit.type = data[stream.data_point];
	stream.data_point++;

	body->collision_group_id = CLAMPED(unit.collision_group_id, 0, 15);
	body->group_id = 0x0001 << body->collision_group_id;
	body->collision_group_mask = unit.collision_mask;
	body->shape_type = (eCOLLISION_SHAPE_TYPE)unit.shape_type;
	body->size[0] = unit.size[0];
	body->size[1] = unit.size[1];
	body->size[2] = unit.size[2];
	body->position[0] = unit.position[0];
	body->position[1] = unit.position[1];
	body->position[2] = - unit.position[2];
	body->rotation[0] = unit.rotation[0];
	body->rotation[1] = unit.rotation[1];
	body->rotation[2] = unit.rotation[2];
	body->mass = unit.mass;
	body->linear_damping = unit.linear_damping;
	body->angular_damping = unit.angular_damping;
	body->restitution = unit.restitution;
	body->friction = unit.friction;
	body->type = (eRIGID_BODY_OBJECT_TYPE)unit.type;

	*data_size = stream.data_point;
}

void PmxRigidBodyMergeMorph(PMX_RIGID_BODY* body, MORPH_IMPULSE* impulse, FLOAT_T weight)
{
}

int Pmd2RigidBodyPreparse(
	MEMORY_STREAM_PTR stream,
	size_t* data_size,
	PMD_DATA_INFO* info
)
{
	int32 size;
	if(MemRead(&size, sizeof(size), 1, stream) == 0
		|| (size_t)(PMD2_RIGID_BODY_UNIT_SIZE * size) > stream->data_point + stream->data_size)
	{
		return FALSE;
	}
	info->rigid_bodies_count = size;
	info->rigid_bodies = &stream->buff_ptr[stream->data_point];
	(void)MemSeek(stream, size * PMD2_RIGID_BODY_UNIT_SIZE, SEEK_CUR);
	return TRUE;
}

void InitializePmd2RigidBody(PMD2_RIGID_BODY* body, PMD2_MODEL* model, void* application_context)
{
	(void)memset(body, 0, sizeof(*body));
	InitializeBaseRigidBody(body, (void*)model, application_context);
}

int LoadPmd2RigidBodies(STRUCT_ARRAY* rigid_bodies, STRUCT_ARRAY* bones)
{
	PMD2_RIGID_BODY *bodies = (PMD2_RIGID_BODY*)rigid_bodies->buffer;
	PMD2_RIGID_BODY *body;
	const int num_bodies = (int)rigid_bodies->num_data;
	PMD2_BONE *b = (PMD2_BONE*)bones->buffer;
	const int num_bones = (int)bones->num_data;
	int bone_index;
	int i;

	for(i=0; i<num_bodies; i++)
	{
		body = &bodies[i];
		bone_index = body->bone_index;
		if(bone_index >= 0)
		{
			if(bone_index == 0xffff)
			{
				MODEL_INTERFACE *model = body->parent_model;
				BONE_INTERFACE *bone = (BONE_INTERFACE*)model->find_bone(model, "center");
				BaseRigidBodyBuild(body, bone, i);
			}
			else if(bone_index >= num_bones)
			{
				return FALSE;
			}
			else
			{
				PMD2_BONE *bone = &b[bone_index];
				BaseRigidBodyBuild(body, bone, i);
			}
		}
		else
		{
			MODEL_INTERFACE *model = body->parent_model;
			BaseRigidBodyBuild(body, &model->scene->project->application_context->default_data.bone, i);
		}
	}
	return TRUE;
}

void ReadPmd2RigidBody(
	PMD2_RIGID_BODY* body,
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info,
	size_t* data_size
)
{
	PMD2_RIGID_BODY_UNIT unit;
	(void)MemRead(unit.name, sizeof(unit.name), 1, stream);
	(void)MemRead(&unit.bone_id, sizeof(unit.bone_id), 1, stream);
	(void)MemRead(&unit.collision_group_id, sizeof(unit.collision_group_id), 1, stream);
	(void)MemRead(&unit.collision_mask, sizeof(unit.collision_mask), 1, stream);
	(void)MemRead(&unit.shape_type, sizeof(unit.shape_type), 1, stream);
	(void)MemRead(unit.size, sizeof(unit.size), 1, stream);
	(void)MemRead(unit.position, sizeof(unit.position), 1, stream);
	(void)MemRead(unit.rotation, sizeof(unit.rotation), 1, stream);
	(void)MemRead(&unit.mass, sizeof(unit.mass), 1, stream);
	(void)MemRead(&unit.linear_damping, sizeof(unit.linear_damping), 1, stream);
	(void)MemRead(&unit.angular_damping, sizeof(unit.angular_damping), 1, stream);
	(void)MemRead(&unit.restitution, sizeof(unit.restitution), 1, stream);
	(void)MemRead(&unit.friction, sizeof(unit.friction), 1, stream);
	(void)MemRead(&unit.type, sizeof(unit.type), 1, stream);
	body->name = EncodeText(body->encode, unit.name, sizeof(unit.name));
	body->bone_index = unit.bone_id;
	body->collision_group_id = unit.collision_group_id;
	body->collision_group_mask = unit.collision_mask;
	body->group_id = 0x0001 << body->collision_group_id;
	body->shape_type = (eCOLLISION_SHAPE_TYPE)unit.shape_type;
	COPY_VECTOR3(body->size, unit.size);
	SET_POSITION(body->position, unit.position);
	COPY_VECTOR3(body->rotation, unit.rotation);
	body->mass = unit.mass;
	body->linear_damping = unit.linear_damping;
	body->angular_damping = unit.angular_damping;
	body->restitution = unit.restitution;
	body->friction = unit.friction;
	body->type = (eRIGID_BODY_OBJECT_TYPE)unit.type;
	*data_size = PMD2_RIGID_BODY_UNIT_SIZE;
}

void* Pmd2RigidBodyTrasnsformNew(BASE_RIGID_BODY* body)
{
	const float basis[] = IDENTITY_MATRIX3x3;
	void *local_transform = BaseRigidBodyTransformNew(body);
	void *world_transform = BtTransformNew(basis);
	void *ret;
	float origin[3];

	body->bone->get_world_transform(body->bone, world_transform);
	BtTransformGetOrigin(world_transform, origin);
	BtTransformMultiVector3(local_transform, origin, origin);
	ret = BtTransformNewWithVector(basis, origin);

	DeleteBtTransform(local_transform);
	DeleteBtTransform(world_transform);

	return ret;
}

#ifdef __cplusplus
}
#endif
