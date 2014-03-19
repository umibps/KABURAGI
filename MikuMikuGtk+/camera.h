#ifndef _INCLUED_CAMERA_H_
#define _INCLUED_CAMERA_H_

#include "motion.h"
#include "animation.h"

typedef struct _CAMERA
{
	struct _SCENE *scene;
	MOTION_INTERFACE *motion;
	float look_at[3];
	float position[3];
	float angle[3];
	float distance[3];
	float fov;
	float znear;
	float zfar;
	void *transform;
} CAMERA;

#define CAMERA_KEYFRAME_TABLE_SIZE 24

typedef enum _eCAMERA_INTERPOLATION
{
	CAMERA_INTERPOLATION_X,
	CAMERA_INTERPOLATION_Y,
	CAMERA_INTERPOLATION_Z,
	CAMERA_INTERPOLATION_ROTATION,
	CAMERA_INTERPOLATION_DISTANCE,
	CAMERA_INTERPOLATION_FOVY,
	MAX_CAMERA_INTERPOLATION
} eCAMERA_INTERPOLATION;

typedef struct _CAMERA_INTERPOLATION_PARAMETER
{
	QUAD_WORD x;
	QUAD_WORD y;
	QUAD_WORD z;
	QUAD_WORD rotation;
	QUAD_WORD distance;
	QUAD_WORD fovy;
} CAMERA_INTERPOLATION_PARAMETER;

typedef enum _eCAMERA_KEYFRAME_FLAGS
{
	CAMERA_KEYFRAME_FLAG_LINEAR0 = 0x01,
	CAMERA_KEYFRAME_FLAG_LINEAR1 = 0x02,
	CAMERA_KEYFRAME_FLAG_LINEAR2 = 0x04,
	CAMERA_KEYFRAME_FLAG_LINEAR3 = 0x08,
	CAMERA_KEYFRAME_FLAG_LINEAR4 = 0x10,
	CAMERA_KEYFRAME_FLAG_LINEAR5 = 0x20,
	CAMERA_KEYFRAME_FLAG_NO_PERSPECTIVE = 0x40
} eCAMERA_KEYFRAME_FLAGS;

typedef struct _CAMERA_KEYFRAME
{
	KEY_FRAME base_data;
	uint8 name[2];
	float distance;
	float fovy;
	float position[3];
	float angle[3];
	float *interpolation_table[6];
	int8 raw_interpolation_table[CAMERA_KEYFRAME_TABLE_SIZE];
	CAMERA_INTERPOLATION_PARAMETER parameter;

	unsigned int flags;
} CAMERA_KEYFRAME;

typedef struct _CAMERA_ANIMATION
{
	ANIMATION base_data;
	float position[3];
	float angle[3];
	float distance;
	float fovy;
} CAMERA_ANIMATION;

extern void InitializeCamera(CAMERA* camera, void* scene);

extern void ResetCameraDefault(CAMERA* camera);

extern void SetCameraDistance(CAMERA* camera, const float distance);

extern void CameraUpdateTransform(CAMERA* camera);

#endif	// #ifndef _INCLUED_CAMERA_H_
