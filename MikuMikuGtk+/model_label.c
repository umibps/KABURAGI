// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <limits.h>
#include "model_label.h"
#include "pmx_model.h"
#include "pmd_model.h"
#include "asset_model.h"
#include "application.h"
#include "memory_stream.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void ReleaseLabelInterface(LABEL_INTERFACE* label)
{
	MEM_FREE_FUNC(label->name);
	MEM_FREE_FUNC(label->english_name);
}

typedef struct _PMX_LABEL_PAIR
{
	int id;
	int type;
	struct _BONE_INTERFACE *bone;
	struct _MORPH_INTERFACE *morph;
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

int PmxModelLabelPreparse(uint8* data, size_t* data_size, size_t rest, PMX_DATA_INFO* info)
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
							pair->bone = (BONE_INTERFACE*)&b[bone_index];
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
							pair->morph = (MORPH_INTERFACE*)&m[morph_index];
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
	PMX_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, (size_t)(info->end - data), 1};
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

#define PMD_BONE_LABEL_SIZE 3
typedef struct _PMD2_BONE_LABEL
{
	uint16 bone_index;
	uint8 category_index;
} PMD2_BONE_LABEL;

static int Pmd2ModelLabelGetCount(PMD2_MODEL_LABEL* label)
{
	switch(label->type)
	{
	case PMD2_MODEL_LABEL_TYPE_SPECIAL_BONE_CATEGORY:
	case PMD2_MODEL_LABEL_TYPE_BONE_CATEGORY:
		return (int)label->bones->num_data;
	case PMD2_MODEL_LABEL_TYPE_MORPH_CATEGORY:
		return (int)label->morphs->num_data;
	}
	return 0;
}

static void* Pmd2ModelLabelGetBone(PMD2_MODEL_LABEL* label, int index)
{
	if((label->type == PMD2_MODEL_LABEL_TYPE_SPECIAL_BONE_CATEGORY
		|| label->type == PMD2_MODEL_LABEL_TYPE_BONE_CATEGORY)
		&& CHECK_BOUND(index, 0, (int)label->bones->num_data))
	{
		return label->bones->buffer[index];
	}
	return NULL;
}

void InitializePmd2ModelLabel(
	PMD2_MODEL_LABEL* label,
	PMD2_MODEL* model,
	const char* name,
	ePMD2_MODEL_LABEL_TYPE type,
	void* application_context
)
{
	(void)memset(label, 0, sizeof(*label));
	label->model = model;
	label->type = type;
	label->bones = PointerArrayNew(DEFAULT_MODEL_LABEL_BUFFER_SIZE);
	label->morphs = PointerArrayNew(DEFAULT_MODEL_LABEL_BUFFER_SIZE);
	label->bone_indices = Uint32ArrayNew(DEFAULT_MODEL_LABEL_BUFFER_SIZE);
	label->morph_indices = Uint32ArrayNew(DEFAULT_MODEL_LABEL_BUFFER_SIZE);
	if(name != NULL)
	{
		label->interface_data.name =
			EncodeText(&((APPLICATION*)application_context)->encode, name, PMD_BONE_CATEGORY_NAME_SIZE);
	}
	label->interface_data.index = -1;
	label->application = (APPLICATION*)application_context;

	label->interface_data.get_count = (int (*)(void*))Pmd2ModelLabelGetCount;
	label->interface_data.get_bone = (void* (*)(void*, int))Pmd2ModelLabelGetBone;
}

int Pmd2ModelLabelPreparse(
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info
)
{
	uint8 size;
	uint32 bone_labels_size;
	if(MemRead(&size, sizeof(size), 1, stream) == 0 ||
		size * sizeof(uint16) > stream->data_size - stream->data_point)
	{
		return FALSE;
	}
	info->morph_labels_count = size;
	info->morph_labels = &stream->buff_ptr[stream->data_point];
	(void)MemSeek(stream, size * sizeof(uint16), SEEK_CUR);
	if(MemRead(&size, sizeof(size), 1, stream) == 0
		|| size * sizeof(uint16) > stream->data_size - stream->data_point)
	{
		return FALSE;
	}
	info->bone_category_names_count = size;
	info->bone_category_names = &stream->buff_ptr[stream->data_point];
	(void)MemSeek(stream, size * PMD_BONE_CATEGORY_NAME_SIZE, SEEK_CUR);
	if(MemRead(&bone_labels_size, sizeof(bone_labels_size), 1, stream) == 0
		|| bone_labels_size * PMD_BONE_LABEL_SIZE > stream->data_size - stream->data_point)
	{
		return FALSE;
	}
	info->bone_labels_count = bone_labels_size;
	info->bone_labels = &stream->buff_ptr[stream->data_point];
	(void)MemSeek(stream, bone_labels_size * PMD_BONE_LABEL_SIZE, SEEK_CUR);
	return TRUE;
}

int LoadPmd2ModelLabels(
	STRUCT_ARRAY* model_labels,
	STRUCT_ARRAY* bones,
	STRUCT_ARRAY* morphs
)
{
	PMD2_MODEL_LABEL *labels = (PMD2_MODEL_LABEL*)model_labels->buffer;
	PMD2_MODEL_LABEL *label;
	const int num_labels = (int)model_labels->num_data;
	PMD2_BONE *b = (PMD2_BONE*)bones->buffer;
	PMD2_BONE *bone;
	const int num_bones = (int)bones->num_data;
	PMD2_MORPH *m = (PMD2_MORPH*)morphs->buffer;
	PMD2_MORPH *morph;
	const int num_morphs = (int)morphs->num_data;
	int i, j;
	for(i=0; i<num_labels; i++)
	{
		label = &labels[i];
		switch(label->type)
		{
		case PMD2_MODEL_LABEL_TYPE_SPECIAL_BONE_CATEGORY:
			if(num_bones > 0)
			{
				bone = &b[0];
				PointerArrayAppend(label->bones, bone);
			}
			break;
		case PMD2_MODEL_LABEL_TYPE_BONE_CATEGORY:
			{
				int index;
				const int num_indices = (int)label->bone_indices->num_data;
				for(j=0; j<num_indices; j++)
				{
					index = (int)label->bone_indices->buffer[j];
					if(CHECK_BOUND(index, 0, num_bones))
					{
						bone = &b[index];
						PointerArrayAppend(label->bones, bone);
					}
				}
			}
			break;
		case PMD2_MODEL_LABEL_TYPE_MORPH_CATEGORY:
			{
				int index;
				const int num_indices = (int)label->morph_indices->num_data;
				for(j=0; j<num_indices; j++)
				{
					index = (int)label->morph_indices->buffer[j];
					if(CHECK_BOUND(index, 0, num_morphs))
					{
						morph = &m[index];
						PointerArrayAppend(label->morphs, morph);
					}
				}
			}
			break;
		}
		label->interface_data.index = i;
	}
	return TRUE;
}

void ReadPmd2ModelLabel(
	PMD2_MODEL_LABEL* label,
	MEMORY_STREAM_PTR stream,
	size_t* data_size
)
{
	switch(label->type)
	{
	case PMD2_MODEL_LABEL_TYPE_SPECIAL_BONE_CATEGORY:
	case PMD2_MODEL_LABEL_TYPE_BONE_CATEGORY:
		{
			PMD2_BONE_LABEL unit;
			(void)MemRead(&unit.bone_index, sizeof(unit.bone_index), 1, stream);
			(void)MemRead(&unit.category_index, sizeof(unit.category_index), 1, stream);
			Uint32ArrayAppend(label->bone_indices, unit.bone_index);
			*data_size = PMD_BONE_LABEL_SIZE;
		}
		break;
	case PMD2_MODEL_LABEL_TYPE_MORPH_CATEGORY:
		{
			uint16 morph_index;
			(void)MemRead(&morph_index, sizeof(morph_index), 1, stream);
			Uint32ArrayAppend(label->morph_indices, morph_index);
			*data_size = sizeof(morph_index);
		}
		break;
	default:
		*data_size = 0;
		break;
	}
}

void Pmd2ModelLabelReadEnglishName(PMD2_MODEL_LABEL* label, uint8* data, int index)
{
	if(data != NULL && index >= 0)
	{
		label->interface_data.english_name = EncodeText(&label->application->encode,
			(char*)&data[PMD_BONE_CATEGORY_NAME_SIZE*index], PMD_BONE_CATEGORY_NAME_SIZE);
	}
}

PMD2_MODEL_LABEL* Pmd2ModelLabelSelectCategory(STRUCT_ARRAY* labels, uint8* data)
{
	PMD2_MODEL_LABEL *l = (PMD2_MODEL_LABEL*)labels->buffer;
	PMD2_BONE_LABEL label_data;
	MEMORY_STREAM stream = {data, 0, sizeof(PMD2_BONE_LABEL), 1};

	(void)MemRead(&label_data.bone_index, sizeof(label_data.bone_index), 1, &stream);
	(void)MemRead(&label_data.category_index, sizeof(label_data.category_index), 1, &stream);
	if(CHECK_BOUND(label_data.category_index, 0, (uint8)labels->num_data))
	{
		return &l[label_data.category_index];
	}
	return NULL;
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

#ifdef __cplusplus
}
#endif
