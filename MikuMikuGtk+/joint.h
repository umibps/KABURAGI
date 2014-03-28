#ifndef _INCLUDED_JOINT_H_
#define _INCLUDED_JOINT_H_

#include "types.h"
#include "utils.h"

typedef enum _eJOINT_TYPE
{
	JOINT_TYPE_GENERIC_6DOF_SPRING_CONSTRAINT,
	JOINT_TYPE_GENERIC_6DOF_CONSTRAINT,
	JOINT_TYPE_POINT2POINT_CONSTRAINT,
	JOINT_TYPE_CONE_TWIST_CONSTRAINT,
	JOINT_TYPE_SLIDER_CONSTRAINT,
	JOINT_TYPE_HINGE_CONSTRAINT,
	MAX_JOINT_TYPE
} eJOINT_TYPE;

typedef struct _BASE_JOINT
{
	void *constraint;
	void *constraint_ptr;
	void *parent_model;
	void *rigid_body1;
	void *rigid_body2;
	char *name;
	char *english_name;
	float position[3];
	float rotation[3];
	float position_lower_limit[3];
	float rotation_lower_limit[3];
	float position_upper_limit[3];
	float rotation_upper_limit[3];
	float position_stiffness[3];
	float rotation_stiffness[3];
	eJOINT_TYPE type;
	int rigid_body1_index;
	int rigid_body2_index;
	int index;
} BASE_JOINT;

typedef BASE_JOINT PMX_JOINT;

extern void ReleaseBaseJoint(BASE_JOINT* joint);

extern void BaseJointJoinWorld(BASE_JOINT* joint, void* world);

extern void BaseJointLeaveWorld(BASE_JOINT* joint, void* world);

extern void BaseJointUpdateTransform(BASE_JOINT* joint);

extern int LoadPmxJoints(STRUCT_ARRAY* joints, STRUCT_ARRAY* bodies);

#endif	// #ifndef _INCLUDED_JOINT_H_
