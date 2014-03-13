#ifndef _INCLUDED_WORLD_H_
#define _INCLUDED_WORLD_H_

#include "model.h"

typedef struct _WORLD
{
	void *world;
	struct _DEBUG_DRAWER *debug_drawer;
	float gravity[3];
	float motion_fps;
	float fixed_time_step;
	int max_sub_steps;
	MODEL_INTERFACE *model;
} WORLD;

extern void InitializeWorld(WORLD* world);

extern void SetWorldGravity(WORLD* world, const float* gravity);

extern void SetWorldPreferredFPS(WORLD* world, float fps);

extern void WorldStepSimulation(WORLD* world, float time_step);

#endif	// #ifndef _INCLUDED_WORLD_H_
