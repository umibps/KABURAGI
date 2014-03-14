// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include "memory_stream.h"
#include "memory.h"
#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

static void ReleaseRedoData(HISTORY* history)
{
	int ref;
	int i;

	for(i=0; i<history->rest_redo; i++)
	{
		ref = (history->point+i) % MAX_HISTORY_NUM;
		MEM_FREE_FUNC(history->histories[ref].data);
		history->histories[ref].data = NULL;
	}
	history->rest_redo = 0;
}

static void AddHistoryData(
	HISTORY_DATA* history,
	const char* name,
	const void* data,
	size_t data_size,
	history_func undo,
	history_func redo
)
{
	(void)strcpy(history->name, name);
	history->data_size = data_size;
	history->undo = undo;
	history->redo = redo;
	history->data = MEM_ALLOC_FUNC(data_size);
	(void)memcpy(history->data, data, data_size);
}

void AddControlHistory(
	HISTORY* history,
	const char* name,
	const void* data,
	size_t data_size,
	history_func undo,
	history_func redo
)
{
	ReleaseRedoData(history);

	history->num_step++;

	if(history->rest_undo < MAX_HISTORY_NUM)
	{
		history->rest_undo++;
	}

	if(history->point >= MAX_HISTORY_NUM)
	{
		history->point = 0;
		MEM_FREE_FUNC(history->histories->data);
		AddHistoryData(history->histories, name, data, data_size, undo, redo);
	}
	else
	{
		MEM_FREE_FUNC(history->histories[history->point].data);
		AddHistoryData(&history->histories[history->point],
			name, data, data_size, undo, redo);
	}

	history->point++;

#ifdef _DEBUG
	g_print("History Add : %s\n", name);
#endif
}

void ExecuteControlUndo(APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];

	if(project->history.rest_undo > 0)
	{
		int execute = (project->history.point == 0) ? MAX_HISTORY_NUM - 1
			: project->history.point - 1;
		project->history.histories[execute].undo(
			&project->history.histories[execute], project
		);
		project->history.rest_undo--;
		project->history.rest_redo++;

		if(project->history.point > 0)
		{
			project->history.point--;
		}
		else
		{
			project->history.point = MAX_HISTORY_NUM - 1;
		}
	}

	WorldStepSimulation(&project->world, 2);
	SceneUpdateModel(project->scene, project->scene->selected_model, TRUE);
}

void ExecuteControlRedo(APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];

	if(project->history.rest_redo > 0)
	{
		int execute = project->history.point;
		if(execute == MAX_HISTORY_NUM)
		{
			execute = 0;
		}
		project->history.histories[execute].redo(
			&project->history.histories[execute], project
		);
		project->history.point++;
		project->history.rest_undo++;
		project->history.rest_redo--;

		if(project->history.point == MAX_HISTORY_NUM)
		{
			project->history.point = 0;
		}
	}

	WorldStepSimulation(&project->world, 2);
	SceneUpdateModel(project->scene, project->scene->selected_model, TRUE);
}

typedef struct _BONE_MOVE_HISTORY_DATA
{
	float position[3];
	uint16 bone_name_length;
	int model_id;
	char *bone_name;
} BONE_MOVE_HISTORY_DATA;

static void UndoRedoBoneMove(HISTORY_DATA* history, PROJECT* project)
{
	MODEL_INTERFACE *model = NULL;
	BONE_INTERFACE *bone;
	BONE_MOVE_HISTORY_DATA data;
	float temp[3];
	int num_models = (int)project->scene->models->num_data;

	(void)memcpy(&data, history->data, offsetof(BONE_MOVE_HISTORY_DATA, bone_name));
	data.bone_name = (char*)&history->data[offsetof(BONE_MOVE_HISTORY_DATA, bone_name)];
	model = (MODEL_INTERFACE*)project->scene->models->buffer[data.model_id];
	if(model == NULL)
	{
		return;
	}

	bone = model->find_bone(model, data.bone_name);
	if(bone == NULL)
	{
		return;
	}
	bone->get_local_translation(bone, temp);
	bone->set_local_translation(bone, data.position);
	COPY_VECTOR3(data.position, temp);
}

void AddBoneMoveHistory(HISTORY* history, PROJECT* project)
{
	MEMORY_STREAM *history_data = CreateMemoryStream(1024);
	BONE_MOVE_HISTORY_DATA data;
	BONE_INTERFACE *bone;
	int num_models = (int)project->scene->models->num_data;
	int i;
	COPY_VECTOR3(data.position, project->control.before_bone_value);
	data.model_id = -1;
	for(i=0; i<num_models; i++)
	{
		if(project->scene->models->buffer[i] == (void*)project->scene->selected_model)
		{
			data.model_id = i;
			break;
		}
	}
	if(data.model_id < 0)
	{
		(void)DeleteMemoryStream(history_data);
		return;
	}
	bone = (BONE_INTERFACE*)project->scene->selected_bones->buffer[project->scene->selected_bones->num_data-1];
	data.bone_name_length = (uint16)strlen(bone->name)+1;
	(void)MemWrite(&data, 1, offsetof(BONE_MOVE_HISTORY_DATA, bone_name), history_data);
	(void)MemWrite(bone->name, 1, data.bone_name_length, history_data);
	AddControlHistory(history, "bone rotation", history_data->buff_ptr, history_data->data_point,
		(history_func)UndoRedoBoneMove, (history_func)UndoRedoBoneMove);
	(void)DeleteMemoryStream(history_data);
}

typedef struct _BONE_ROTATE_HISTORY_DATA
{
	QUATERNION rotation;
	uint16 bone_name_length;
	int model_id;
	char *bone_name;
} BONE_ROTATE_HISTORY_DATA;

static void UndoRedoBoneRotate(HISTORY_DATA* history, PROJECT* project)
{
	MODEL_INTERFACE *model = NULL;
	BONE_INTERFACE *bone;
	BONE_ROTATE_HISTORY_DATA data;
	QUATERNION temp;
	int num_models = (int)project->scene->models->num_data;

	(void)memcpy(&data, history->data, offsetof(BONE_ROTATE_HISTORY_DATA, bone_name));
	data.bone_name = (char*)&history->data[offsetof(BONE_ROTATE_HISTORY_DATA, bone_name)];
	model = (MODEL_INTERFACE*)project->scene->models->buffer[data.model_id];
	if(model == NULL)
	{
		return;
	}

	bone = model->find_bone(model, data.bone_name);
	if(bone == NULL)
	{
		return;
	}
	bone->get_local_rotation(bone, temp);
	bone->set_local_rotation(bone, data.rotation);
	COPY_VECTOR4(data.rotation, temp);
}

void AddBoneRotateHistory(HISTORY* history, PROJECT* project)
{
	MEMORY_STREAM *history_data = CreateMemoryStream(1024);
	BONE_ROTATE_HISTORY_DATA data;
	BONE_INTERFACE *bone;
	int num_models = (int)project->scene->models->num_data;
	int i;
	COPY_VECTOR4(data.rotation, project->control.before_bone_value);
	data.model_id = -1;
	for(i=0; i<num_models; i++)
	{
		if(project->scene->models->buffer[i] == (void*)project->scene->selected_model)
		{
			data.model_id = i;
			break;
		}
	}
	if(data.model_id < 0)
	{
		(void)DeleteMemoryStream(history_data);
		return;
	}
	bone = (BONE_INTERFACE*)project->scene->selected_bones->buffer[project->scene->selected_bones->num_data-1];
	data.bone_name_length = (uint16)strlen(bone->name)+1;
	(void)MemWrite(&data, 1, offsetof(BONE_ROTATE_HISTORY_DATA, bone_name), history_data);
	(void)MemWrite(bone->name, 1, data.bone_name_length, history_data);
	AddControlHistory(history, "bone rotation", history_data->buff_ptr, history_data->data_point,
		(history_func)UndoRedoBoneRotate, (history_func)UndoRedoBoneRotate);
	(void)DeleteMemoryStream(history_data);
}

#ifdef __cplusplus
}
#endif
