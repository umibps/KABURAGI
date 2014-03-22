#include <string.h>
#include <limits.h>
#include "model_label.h"
#include "pmx_model.h"
#include "asset_model.h"
#include "memory_stream.h"
#include "memory.h"

void ReleaseLabelInterface(LABEL_INTERFACE* label)
{
	MEM_FREE_FUNC(label->name);
	MEM_FREE_FUNC(label->english_name);
}

typedef struct _PMX_LABEL_PAIR
{
	int id;
	int type;
	void *bone;
	void *morph;
} PMX_LABEL_PAIR;

int PmxModelLabelGetCount(PMX_MODEL_LABEL* label)
{
	return (int)label->pairs->num_data;
}

void* PmxModelLabelGetBone(PMX_MODEL_LABEL* label, int index)
{
	PMX_LABEL_PAIR *pair = (PMX_LABEL_PAIR*)label->pairs->buffer;
	if(CHECK_BOUND(index, 0, (int)label->pairs->num_data))
	{
		return pair[index].bone;
	}

	return NULL;
}

void InitializePmxModelLabel(PMX_MODEL_LABEL* label, PMX_MODEL* model)
{
	(void)memset(label, 0, sizeof(*label));

	label->model = model;
	label->interface_data.index = -1;

	label->pairs = StructArrayNew(sizeof(PMX_LABEL_PAIR), DEFAULT_MODEL_LABEL_BUFFER_SIZE);
	label->interface_data.get_count = (int (*)(void*))PmxModelLabelGetCount;
	label->interface_data.get_bone = (void* (*)(void*, int))PmxModelLabelGetBone;
}

int PmxModelLabelPreparse(uint8* data, size_t* data_size, size_t rest, MODEL_DATA_INFO* info)
{
	MEMORY_STREAM stream = {data, 0, rest, 1};
	char *name_ptr;
	int length;
	int32 size;
	int32 num_labels;
	uint8 type;
	int i, j;

	if(MemRead(&num_labels, sizeof(num_labels), 1, &stream) == 0)
	{
		return FALSE;
	}

	info->labels = &data[stream.data_point];
	for(i=0; i<num_labels; i++)
	{
		// “ú–{Œê–¼
		if((length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;

		// ‰pŒê–¼
		if((length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;

		if(MemSeek(&stream, 1, SEEK_CUR) != 0)
		{
			return FALSE;
		}

		if(MemRead(&size, sizeof(size), 1, &stream) == 0)
		{
			return FALSE;
		}

		for(j=0; j<size; j++)
		{
			type = data[stream.data_point];
			stream.data_point++;
			switch(type)
			{
			case 0:
				if(MemSeek(&stream, (long)info->bone_index_size, SEEK_CUR) != 0)
				{
					return FALSE;
				}
				break;
			case 1:
				if(MemSeek(&stream, (long)info->morph_index_size, SEEK_CUR) != 0)
				{
					return FALSE;
				}
				break;
			default:
				return FALSE;
			}
		}
	}

	info->labels_count = num_labels;
	*data_size = stream.data_point;

	return TRUE;
}

int LoadPmxModelLabels(
	STRUCT_ARRAY* labels,
	STRUCT_ARRAY* bones,
	STRUCT_ARRAY* morphs
)
{
	const int num_labels = (int)labels->num_data;
	const int num_bones = (int)bones->num_data;
	const int num_morphs = (int)morphs->num_data;
	PMX_MODEL_LABEL *l = (PMX_MODEL_LABEL*)labels->buffer;
	PMX_BONE *b = (PMX_BONE*)bones->buffer;
	PMX_MORPH *m = (PMX_MORPH*)morphs->buffer;
	int i, j;

	for(i=0; i<num_labels; i++)
	{
		PMX_MODEL_LABEL *label = &l[i];
		const int num_pairs = (int)label->pairs->num_data;
		PMX_LABEL_PAIR *p = (PMX_LABEL_PAIR*)label->pairs->buffer;
		for(j=0; j<num_pairs; j++)
		{
			PMX_LABEL_PAIR *pair = &p[j];
			switch(pair->type)
			{
			case 0:
				{
					const int bone_index = pair->id;
					if(bone_index >= 0)
					{
						if(bone_index < num_bones)
						{
							pair->bone = (void*)&b[bone_index];
						}
						else
						{
							return FALSE;
						}
					}
				}
				break;
			case 1:
				{
					const int morph_index = pair->id;
					if(morph_index >= 0)
					{
						if(morph_index < num_morphs)
						{
							pair->morph = (void*)&m[morph_index];
						}
						else
						{
							return FALSE;
						}
					}
				}
				break;
			default:
				return FALSE;
			}
		}

		label->interface_data.index = i;
	}

	return TRUE;
}

void PmxModelLabelRead(
	PMX_MODEL_LABEL* label,
	uint8* data,
	size_t* data_size,
	MODEL_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, INT_MAX, 1};
	char *name_ptr;
	int32 length;
	TEXT_ENCODE *encode = info->encoding;
	uint8 type;
	int i;

	// “ú–{Œê–¼
	length = GetTextFromStream((char*)data, &name_ptr);
	label->interface_data.name = EncodeText(encode, name_ptr, length);
	stream.data_point = sizeof(int32) + length;

	// ‰pŒê–¼
	length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr);
	label->interface_data.english_name = EncodeText(encode, name_ptr, length);
	stream.data_point += sizeof(int32) + length;

	type = data[stream.data_point];
	stream.data_point++;
	label->interface_data.special = type == 1;

	(void)MemRead(&length, sizeof(length), 1, &stream);
	for(i=0; i<length; i++)
	{
		PMX_LABEL_PAIR *pair = StructArrayReserve(label->pairs);
		type = data[stream.data_point];
		stream.data_point++;
		pair->type = type;
		switch(type)
		{
		case 0:
			pair->id = GetSignedValue(&data[stream.data_point], (int)info->bone_index_size);
			stream.data_point += info->bone_index_size;
			break;
		case 1:
			pair->id = GetSignedValue(&data[stream.data_point], (int)info->morph_index_size);
			stream.data_point += info->morph_index_size;
			break;
		default:
			return;
		}
	}

	*data_size = stream.data_point;
}

void ReleasePmxModelLabel(PMX_MODEL_LABEL* label)
{
	ReleaseLabelInterface(&label->interface_data);
	StructArrayDestroy(&label->pairs, NULL);
}

static int AssetModelLabelGetCount(ASSET_MODEL_LABEL* label)
{
	return (int)label->bones->num_data;
}

static void* AssetModelLabelGetBone(ASSET_MODEL_LABEL* label, int index)
{
	if(index >= 0 && index < (int)label->bones->num_data)
	{
		return label->bones->buffer[index];
	}
	return NULL;
}

void InitializeAssetModelLabel(
	ASSET_MODEL_LABEL* label,
	ASSET_MODEL* model,
	POINTER_ARRAY* bones
)
{
	static const char name[] = "Root";
	const int num_bones = (int)bones->num_data;
	int i;

	(void)memset(label, 0, sizeof(*label));
	label->model = model;
	label->bones = PointerArrayNew(ASSET_MODEL_BUFFER_SIZE);
	for(i=0; i<num_bones; i++)
	{
		PointerArrayAppend(label->bones, bones->buffer[i]);
	}
	label->interface_data.name = (char*)name;
	label->interface_data.get_count =
		(int (*)(void*))AssetModelLabelGetCount;
	label->interface_data.get_bone =
		(void* (*)(void*, int))AssetModelLabelGetBone;
	label->interface_data.special = TRUE;
}
