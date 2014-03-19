#ifndef _INCLUDED_VMD_KEYFRAME_H_
#define _INCLUDED_VMD_KEYFRAME_H_

#include "keyframe.h"
#include "model.h"

#define VMD_BONE_KEYFRAME_TABLE_SIZE 64

typedef struct _VMD_BONE_KEYFRAME
{
	BONE_KEYFRAME_INTERFACE interface_data;
	uint8 linear[4];
	uint8 enable;
	FLOAT_T *interpolation_table[4];
	int8 raw_interpolation_table[VMD_BONE_KEYFRAME_TABLE_SIZE];
	BONE_KEYFARAME_INTERPOLIATION_PARAMETER parameter;
} VMD_BONE_KEYFRAME;

typedef struct _VMD_MODEL_KEYFRAME
{
	MODEL_KEYFRAME_INTERFACE interface_data;
	ght_hash_table_t *states;
	int visible;
} VMD_MODEL_KEYFRAME;

typedef struct _VMD_MORPH_KEYFRAME
{
	MORPH_KEYFRAME_INTERFACE interface_data;
	FLOAT_T weight;
} VMD_MORPH_KEYFRAME;

extern void VmdBoneKeyframeSetDefaultInterpolationParameter(VMD_BONE_KEYFRAME* frame);

extern void VmdBoneKeyframeSetTranslation(VMD_BONE_KEYFRAME* frame, float* translation);

extern void VmdBoneKeyframeSetRotation(VMD_BONE_KEYFRAME* frame, float* rotation);

extern MODEL_KEYFRAME_INTERFACE* VmdModelKeyframeNew(void* application);

extern void VmdModelKeyframeUpdateInverseKinematics(VMD_MODEL_KEYFRAME* keyframe, MODEL_INTERFACE* model);

#endif	// #ifndef _INCLUDED_VMD_KEYFRAME_H_
