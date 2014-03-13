#ifndef _INCLUDED_VMD_MOTION_H_
#define _INCLUDED_VMD_MOTION_H_

#include <stdio.h>
#include "types.h"
#include "bone.h"
#include "camera.h"
#include "face.h"
#include "light.h"
#include "motion.h"
#include "morph.h"
#include "vmd_keyframe.h"

#define VMD_MOTION_SIGNATURE_SIZE 30
#define VMD_MOTION_NAME_SIZE 20

typedef struct _VMD_MOTION_DATA_INFO
{
	uint8 *base_ptr;
	uint8 *name_ptr;
	uint8 *bone_keyframe_ptr;
	size_t bone_keyframe_count;
	uint8 *face_keyframe_ptr;
	size_t face_keyframe_count;
	uint8 *camera_keyframe_ptr;
	size_t camera_keyframe_count;
	uint8 *light_keyframe_ptr;
	size_t light_keyframe_count;
	uint8 *self_shadow_keyframe_ptr;
	size_t self_shadow_keyframe_count;
} VMD_MOTION_DATA_INFO;

typedef enum _eVMD_MOTION_FLAGS
{
	VMD_MOTION_FLAG_ACTIVE = 0x01
} eVMD_MOTION_FLAGS;

typedef struct _VMD_BASE_ANIMATION
{
	void (*read)(void*, uint8*, int);
	void (*seek)(void*, FLOAT_T);
	POINTER_ARRAY *keyframes;
	int last_time_index;
	FLOAT_T duration_time_index;
	FLOAT_T current_time_index;
	FLOAT_T previous_time_index;
} VMD_BASE_ANIMATION;

typedef struct _VMD_BONE_ANIMATION_CONTEXT
{
	struct _BONE_INTERFACE *bone;
	POINTER_ARRAY *keyframes;
	VECTOR3 position;
	QUATERNION rotation;
	int last_index;
} VMD_BONE_ANIMATION_CONTEXT;

typedef struct _VMD_BONE_ANIMATION
{
	VMD_BASE_ANIMATION animation;
	struct _MODEL_INTERFACE *model;
	ght_hash_table_t *name2contexts;
	struct _APPLICATION *application;
	unsigned int flags;
} VMD_BONE_ANIMATION;

typedef struct _VMD_MODEL_ANIMATION
{
	VMD_BASE_ANIMATION animation;
	MODEL_INTERFACE *model;
} VMD_MODEL_ANIMATION;

typedef struct _VMD_MORPH_ANIMATION_CONTEXT
{
	MORPH_INTERFACE *morph;
	POINTER_ARRAY *keyframes;
	FLOAT_T weight;
	int last_index;
} VMD_MORPH_ANIMATION_CONTEXT;

typedef struct _VMD_MORPH_ANIMATION
{
	VMD_BASE_ANIMATION animation;
	ght_hash_table_t *name2contexts;
	MODEL_INTERFACE *model;
	struct _APPLICATION *application;
} VMD_MORPH_ANIMATION;

typedef struct _VMD_MOTION
{
	MOTION_INTERFACE interface_data;
	VMD_MOTION_DATA_INFO data_info;
	VMD_BONE_ANIMATION bone_motion;
	VMD_MODEL_ANIMATION model_motion;
	VMD_MORPH_ANIMATION morph_motion;
	CAMERA_ANIMATION camera_motion;
	FACE_ANIMATION face_motion;
	LIGHT_ANIMATION light_motion;
	ght_hash_table_t *type2animation;
	unsigned int flags;
} VMD_MOTION;

extern void VmdMotionSeek(VMD_MOTION* motion, FLOAT_T time_index);

extern KEYFRAME_INTERFACE* VmdMotionFindKeyframe(
	VMD_MOTION* motion,
	eKEYFRAME_TYPE keyframe_type,
	FLOAT_T time_index,
	void* detail_data
);

extern void VmdMotionUpdate(VMD_MOTION* motion, eKEYFRAME_TYPE type);

extern void VmdMotionAddKeyframe(VMD_MOTION* motion, KEYFRAME_INTERFACE* frame);

extern void VmdMotionSetModel(VMD_MOTION* motion, MODEL_INTERFACE* model);

#endif	// #ifndef _INCLUDED_VMD_MOTION_H_
