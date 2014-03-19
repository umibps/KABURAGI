// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

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

void DeleteModel(MODEL_INTERFACE* model)
{
	switch(model->type)
	{
	case MODEL_TYPE_ASSET_MODEL:
		break;
	case MODEL_TYPE_PMX_MODEL:
		ReleasePmxModel((PMX_MODEL*)model);
		MEM_FREE_FUNC(model);
		break;
	}
}

typedef struct _SET_LABEL_DATA
{
	char *label;
	POINTER_ARRAY *child_names;
} SET_LABEL_DATA;

char** GetChildBoneNames(MODEL_INTERFACE* model, void* application_context)
{
	APPLICATION *application = (APPLICATION*)application_context;
	PROJECT *project = application->projects[application->active_project];
	STRUCT_ARRAY *set_label_data = StructArrayNew(sizeof(SET_LABEL_DATA), DEFAULT_BUFFER_SIZE);
	SET_LABEL_DATA *parent;
	SET_LABEL_DATA *data;
	LABEL_INTERFACE **labels;
	LABEL_INTERFACE *label;
	BONE_INTERFACE *bone;
	POINTER_ARRAY *added_bones;
	POINTER_ARRAY *names;
	char **ret;
	int num_labels;
	int num_children;
	int num_added_bones = 0;
	int i, j;

	if(model == NULL)
	{
		return NULL;
	}

	labels = (LABEL_INTERFACE**)model->get_labels(model, &num_labels);

	parent = (SET_LABEL_DATA*)StructArrayReserve(set_label_data);
	parent->child_names = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	added_bones = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	for(i=0; i<num_labels; i++)
	{
		label = labels[i];
		num_children = label->get_count(label);
		if(label->special != FALSE)
		{
			if(num_children > 0 && strcmp(label->name, "Root") == 0)
			{
				bone = label->get_bone(label, 0);
				if(bone != NULL)
				{
					parent->label = bone->name;
					PointerArrayAppend(added_bones, bone);
				}
			}
			if(parent->label == NULL)
			{
				continue;
			}
		}
		else
		{
			parent->label = label->name;
			PointerArrayAppend(added_bones, bone);
		}

		for(j=0; j<num_children; j++)
		{
			bone = label->get_bone(label, j);
			if(bone != NULL)
			{
				PointerArrayAppend(parent->child_names, bone->name);
				PointerArrayAppend(added_bones, bone);
			}
		}

		if(parent->child_names->num_data > 0)
		{
			parent = (SET_LABEL_DATA*)StructArrayReserve(set_label_data);
			parent->child_names = PointerArrayNew(DEFAULT_BUFFER_SIZE);
		}
	}

	data = (SET_LABEL_DATA*)set_label_data->buffer;
	names = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	for(i=0; i<(int)set_label_data->num_data; i++)
	{
		parent = &data[i];
		bone = (BONE_INTERFACE*)added_bones->buffer[num_added_bones];
		num_added_bones++;

		for(j=0; j<(int)parent->child_names->num_data; j++)
		{
			bone = (BONE_INTERFACE*)added_bones->buffer[num_added_bones];
			PointerArrayAppend(names, MEM_STRDUP_FUNC(bone->name));
			num_added_bones++;
		}
	}
	PointerArrayAppend(names, NULL);
	ret = (char**)names->buffer;

	for(i=0; i<(int)set_label_data->num_data; i++)
	{
		parent = &data[i];
		PointerArrayDestroy(&parent->child_names, NULL);
	}
	StructArrayDestroy(&set_label_data, NULL);

	MEM_FREE_FUNC(labels);
	MEM_FREE_FUNC(names);

	return ret;
}
