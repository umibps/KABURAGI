#include "world.h"
#include "bullet.h"
#include "application.h"

#define DEFAULT_MAX_SUB_STEPS 3

#ifdef __cplusplus
extern "C" {
#endif

void InitializeWorld(WORLD* world)
{
	float gravity[3] = {0.0f, -9.8f, 0.0f};

	(void)memset(world, 0, sizeof(*world));
	world->world = BtDynamicsWorldNew(NULL);
	SetWorldGravity(world, gravity);
	SetWorldPreferredFPS(world, DEFAULT_FPS);
	world->max_sub_steps = DEFAULT_MAX_SUB_STEPS;
}

void SetWorldGravity(WORLD* world, const float* gravity)
{
	COPY_VECTOR3(world->gravity, gravity);
	BtDynamicsWorldSetGravity(world->world, gravity);
}

void SetWorldPreferredFPS(WORLD* world, float fps)
{
	world->motion_fps = fps;
	world->fixed_time_step = 1.0f / fps;
}

void WorldStepSimulation(WORLD* world, float time_step)
{
	BtDynamicsWorldStepSimulation(world->world, time_step, world->max_sub_steps, world->fixed_time_step);
}

void WorldStepsSimulation(WORLD* world, float time_step, int max_sub_steps)
{
	BtDynamicsWorldStepSimulation(world->world, time_step, max_sub_steps, world->fixed_time_step);
}

#ifdef __cplusplus
}
#endif
