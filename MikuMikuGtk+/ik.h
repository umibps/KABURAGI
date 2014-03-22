#ifndef _INCLUDED_IK_H_
#define _INCLUDED_IK_H_

#include "bone.h"
#include "utils.h"

typedef struct _IK
{
	BONE_INTERFACE *destination;
	BONE_INTERFACE *target;
	POINTER_ARRAY *bones;
	uint16 iteration;
	float angle_constraint;
	float raw_angle_constraint;
} IK;

#endif	// #ifndef _INCLUDED_IK_H_
