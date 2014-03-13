#ifndef _INCLUDED_LIGHT_H_
#define _INCLUDED_LIGHT_H_

#include "motion.h"
#include "types.h"
#include "animation.h"

typedef enum _eLIGHT_FLAGS
{
	LIGHT_FLAG_COLOR_CHANGE = 0x01,
	LIGHT_FLAG_DIRECTION_CHANGE = 0x02,
	LIGHT_FLAG_TOON_ENABLE = 0x04
} eLIGHT_FLAGS;

typedef struct _LIGHT
{
	MOTION_INTERFACE *motion;
	VERTEX_C4UB_V3F vertex;
	void *scene;
	unsigned int flags;
} LIGHT;

typedef struct _LIGHT_KEYFRAME
{
	KEY_FRAME base_data;
	uint8 name[2];
	uint8 color[3];
	float direction[3];
} LIGHT_KEYFRAME;

typedef struct _LIGHT_ANIMATION
{
	uint8 color[3];
	float direction[3];
} LIGHT_ANIMATION;

extern void InitializeLight(LIGHT* light, void* scene);

extern void ResetLightDefault(LIGHT* light);

#endif	// #ifndef _INCLUDED_LIGHT_H_
