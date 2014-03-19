#include "light.h"
#include "application.h"

void SetLightColor(LIGHT* light, const uint8* color)
{
	light->vertex.color[0] = color[0];
	light->vertex.color[1] = color[1];
	light->vertex.color[2] = color[2];
	light->vertex.color[3] = color[3];
	light->flags |= LIGHT_FLAG_COLOR_CHANGE;
}

void SetLightDirection(LIGHT* light, const float* direction)
{
	light->vertex.position[0] = direction[0];
	light->vertex.position[1] = direction[1];
	light->vertex.position[2] = direction[2];
	light->flags |= LIGHT_FLAG_DIRECTION_CHANGE;
}

void ResetLightDefault(LIGHT* light)
{
	const uint8 color[] = {153, 153, 153, 255};
	const float direction[] = {-0.5f, -1.0f, -0.5f};

	SetLightColor(light, color);
	SetLightDirection(light, direction);
	light->flags &= ~(LIGHT_FLAG_TOON_ENABLE);
}

void InitializeLight(LIGHT* light, void* scene)
{
	light->scene = (SCENE*)scene;
	ResetLightDefault(light);
}
