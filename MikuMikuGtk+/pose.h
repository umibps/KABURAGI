#ifndef _INCLUDED_POSE_H_
#define _INCLUDED_POSE_H_

#include "types.h"
#include "utils.h"
#include "model.h"
#include "text_encode.h"

typedef struct _POSE_BONE
{
	char *name;
	VECTOR3 position;
	QUATERNION rotation;
} POSE_BONE;

typedef struct _POSE_MORPH
{
	char *name;
	FLOAT_T weight;
} POSE_MORPH;

typedef struct _POSE
{
	STRUCT_ARRAY *bones;
	STRUCT_ARRAY *morphs;
	TEXT_ENCODE text_encode;
} POSE;

extern POSE* LoadPoseData(const char* data);

extern void DeletePose(POSE** pose);

extern void ApplyPose(POSE* pose, MODEL_INTERFACE* model, int apply_center);

#endif	// #ifndef _INCLUDED_POSE_H_
