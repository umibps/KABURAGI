#ifndef _INCLUDED_RIGID_BODY_H_
#define _INCLUDED_RIGID_BODY_H_

#include "types.h"
#include "utils.h"
#include "bone.h"
#include "text_encode.h"
#include "bullet.h"

typedef struct _RIGID_BODY_INTERFACE
{
	struct _APPLICATION *application;
} RIGID_BODY_INTERFACE;

#define RIGID_BODY_NAME_SIZE 20

typedef enum _eRIGID_BODY_FLAGS
{
	RIGID_BODY_FLAG_NO_BONE = 0x01
} eRIGID_BODY_FLAGS;

typedef enum _eRIGID_BODY_OBJECT_TYPE
{
	RIGID_BODY_OBJECT_TYPE_STATIC,
	RIGID_BODY_OBJECT_TYPE_DYNAMIC,
	RIGID_BODY_OBJECT_TYPE_ALIGNED,
	MAX_RIGID_BODY_OBJECT_TYPE
} eRIGID_BODY_OBJECT_TYPE;

typedef struct _RIGID_BODY
{
	uint8 name[RIGID_BODY_NAME_SIZE+1];
	BONE *bone;
	void *shape;
	void *motion_state;
	void *transform;
	void *inverted_transform;
	void *kinematic_motion_state;
	float position[3];
	float rotation[3];
	float width, height;
	float depth;
	float mass;
	uint16 group_id;
	uint16 group_mask;
	uint8 collision_group_id;
	uint8 shape_type;
	uint8 type;
	uint8 flags;
} RIGID_BODY;

typedef struct _BASE_RIGID_BODY_MOTION_STATE
{
	struct _BASE_RIGID_BODY *parent;
	void *start_transform;
	void *world_transform;
} BASE_RIGID_BODY_MOTION_STATE;

typedef struct _BASE_RIGID_BODY
{
	RIGID_BODY_INTERFACE interface_data;
	void *body;
	void *body_ptr;
	void *collision_shape;
	BASE_RIGID_BODY_MOTION_STATE motion_state;
	void *active_motion_state;
	BASE_RIGID_BODY_MOTION_STATE kinematic_state;
	void *kinematic_motion_state;
	void *world_transform;
	void *world2local_transform;
	void *parent_model;
	TEXT_ENCODE *encode;
	void *bone;
	char *name;
	char *english_name;
	int bone_index;
	float size[3];
	float position[3];
	float rotation[3];
	float mass;
	float linear_damping;
	float angular_damping;
	float restitution;
	float friction;
	int index;
	uint16 group_id;
	uint16 collision_group_mask;
	uint8 collision_group_id;
	eCOLLISION_SHAPE_TYPE shape_type;
	eRIGID_BODY_OBJECT_TYPE type;
} BASE_RIGID_BODY;

typedef BASE_RIGID_BODY PMX_RIGID_BODY;

extern void RigidBodySyncLocalTransform(void* rigid_body, int index, void* dummy);

extern void ReleaseBaseRigidBody(BASE_RIGID_BODY* body);

extern void BaseRigidBodySyncLocalTransform(BASE_RIGID_BODY* body);

extern void BaseRigidBodyJoinWorld(BASE_RIGID_BODY* body, void* world);

extern void BaseRigidBodyLeaveWorld(BASE_RIGID_BODY* body, void* world);

extern void BaseRigidBodyUpdateTransform(BASE_RIGID_BODY* body);

extern void BaseRigidBodySetActivation(BASE_RIGID_BODY* body, int active);

extern void ResetBaseRigidBody(BASE_RIGID_BODY* body, void* world);

extern BASE_RIGID_BODY* GetBoneBody(BONE_INTERFACE* bone);

extern int PmxRigidBodyLoad(STRUCT_ARRAY* bodies, STRUCT_ARRAY* bones);

#endif	// #ifndef _INCLUDED_RIGID_BODY_H_
