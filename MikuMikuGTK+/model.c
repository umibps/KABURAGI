#include "model.h"
#include "memory.h"

void ReleaseModelInterface(MODEL_INTERFACE* model)
{
	MEM_FREE_FUNC(model->name);
	MEM_FREE_FUNC(model->english_name);
	MEM_FREE_FUNC(model->comment);
	MEM_FREE_FUNC(model->english_comment);
	MEM_FREE_FUNC(model->model_path);
}
