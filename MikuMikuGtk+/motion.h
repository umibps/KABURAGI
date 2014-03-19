#ifndef _INCLUDED_MOTION_H_
#define _INCLUDED_MOTION_H_

#include "keyframe.h"
#include "utils.h"
#include "types.h"

typedef enum _eMOTION_TYPE
{
	MOTION_TYPE_UNKNOWN,
	MOTION_TYPE_VMD_MOTION,
	MOTION_TYPE_MVD_MOTION,
	MAX_MOTION_TYPE
} eMOTION_TYPE;

typedef struct _MOTION_INTERFACE
{
	eMOTION_TYPE type;
	struct _SCENE *scene;
	struct _APPLICATION *application;
	struct _MODEL_INTERFACE *model;
	char *name;
	void (*seek)(void*, FLOAT_T);
	void* (*find_keyframe)(void*, eKEYFRAME_TYPE, FLOAT_T, void*);
	void (*update)(void*, eKEYFRAME_TYPE);
	void (*add_keyframe)(void*, void*);
	void (*set_model)(void*, void*);
} MOTION_INTERFACE;

extern void BuildInterpolationTable(
	FLOAT_T x1,
	FLOAT_T x2,
	FLOAT_T y1,
	FLOAT_T y2,
	int size,
	FLOAT_T* table
);

extern int KeyframeTimeIndexCompare(const KEYFRAME_INTERFACE** left, const KEYFRAME_INTERFACE** right);

extern void FindKeyframeIndices(
	FLOAT_T seek_index,
	FLOAT_T* current_keyframe,
	int* last_index,
	int* from_index,
	int* to_index,
	KEYFRAME_INTERFACE** keyframes,
	const int num_keyframes
);

extern FLOAT_T KeyframeLerp(const FLOAT_T* x, const FLOAT_T* y, const FLOAT_T* t);

#endif	// #ifndef _INCLUDED_MOTION_H_
