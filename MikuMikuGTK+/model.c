#include "model.h"
#include "application.h"
#include "memory.h"

void ReleaseModelInterface(MODEL_INTERFACE* model)
{
	MEM_FREE_FUNC(model->name);
	MEM_FREE_FUNC(model->english_name);
	MEM_FREE_FUNC(model->comment);
	MEM_FREE_FUNC(model->english_comment);
	MEM_FREE_FUNC(model->model_path);
}

void ModelJoinWorld(MODEL_INTERFACE* model, WORLD* world)
{
	switch(model->type)
	{
	case MODEL_TYPE_ASSET_MODEL:
		break;
	case MODEL_TYPE_PMX_MODEL:
		PmxModelJoinWorld((PMX_MODEL*)model, world->world);
		break;
	}
}

void ModelLeaveWorld(MODEL_INTERFACE* model, WORLD* world)
{
	switch(model->type)
	{
	case MODEL_TYPE_ASSET_MODEL:
		break;
	case MODEL_TYPE_PMX_MODEL:
		PmxModelLeaveWorld((PMX_MODEL*)model, world->world);
		break;
	}
}
