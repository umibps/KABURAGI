// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "application.h"
#include "pose.h"
#include "libguess/libguess.h"
#include "memory.h"

#ifndef M_PI
# define M_PI 3.1415926535897932384626433832795
#endif

#ifdef __cplusplus
extern "C" {
#endif

static int GetLineInternal(const char* str, char* dst)
{
	int count = 0;
	*dst = *str;
	if(*str == '\r' || *str == '\n')
	{
		return 1;
	}
	while(*str != '\0' && *str != '\r' && *str != '\n')
	{
		*dst = *str;
		str++;
		dst++;
		count++;
	}
	*dst = '\0';
	return count;
}

static int Trim(const char* str, char* dst)
{
	const char *start = str;
	const char *end = &str[strlen(str)-1];
	int length;
	*dst = '\0';
	while(isspace(*start) && *start != '\0')
	{
		start++;
	}
	while(isspace(*end) && end < str)
	{
		end--;
	}
	length = (int)(end - start);
	if(length >= 0)
	{
		(void)memcpy(dst, start, length);
		dst[length] = '\0';
		return TRUE;
	}
	return FALSE;
}

static int GetLine(const char* data, char* next_line, char* buff)
{
	int length;
	int count = 0;
	do
	{
		count += length = GetLineInternal(&data[count], buff);
	} while(data[count] != '\0' && (*buff == '\r' || *buff == '\n'));

	length = (int)strlen(buff);
	if(length > 0)
	{
		if(buff[length-1] == '\r' || buff[length-1] == '\n')
		{
			buff[length-1] = '\0';
		}
		(void)strcpy(next_line, buff);
	}
	else
	{
		*next_line = '\0';
	}

	return count;
}

static int StringGetLine(const char* str, char* dst, char delim)
{
	int count = 0;
	while(*str != '\0' && (*str == '\r' || *str == '\n'))
	{
		count++;
		str++;
	}
	*dst = *str;
	while(*str != '\0' && *str != delim && *str != '\r' && *str != '\n')
	{
		*str++,	*dst++;
		*dst = *str;
		count++;
	}
	*dst = '\0';
	if(*str != '\0')
	{
		count++;
	}
	return count;
}

int ParsePoseBones(POSE* pose, const char* data, char* next_line, char* buff)
{
	POSE_BONE *bone;
	char index[2048];
	char name[2048];
	char position[2048];
	char rotation[2048];
	int count;

	// 総ボーン数を読み飛ばす
	count = GetLine(data, next_line, buff);
	for( ; ; )
	{
		count += StringGetLine(&data[count], index, '{');
		if(StringNumCompareIgnoreCase(index, "BONE", (int)strlen("BONE")) != 0)
		{
			break;
		}
		count += StringGetLine(&data[count], name, '\0');
		if(*name == '\0')
		{
			return 0;
		}
		bone = (POSE_BONE*)StructArrayReserve(pose->bones);
		bone->name = EncodeText(&pose->text_encode, name, strlen(name));
		count += GetLine(&data[count], position, buff);
		count += GetLine(&data[count], rotation, buff);
		(void)sscanf(position, "%f,%f,%f", &bone->position[0], &bone->position[1], &bone->position[2]);
		SET_POSITION(bone->position, bone->position);
		(void)sscanf(rotation, "%f,%f,%f,%f", &bone->rotation[0], &bone->rotation[1], &bone->rotation[2], &bone->rotation[3]);
		bone->rotation[0] = - bone->rotation[0];
		bone->rotation[1] = - bone->rotation[1];
		count += GetLine(&data[count], next_line, buff);
		//count += GetLine(&data[count], next_line, buff);
		if(data[count] == '\0')
		{
			break;
		}
	}
	return count;
}

int ParsePoseMorphs(POSE* pose, const char* data, char* next_line, char* buff)
{
	POSE_MORPH *morph;
	char index[2048];
	char name[2048];
	char weight[2048];
	int count = 0;

	for( ; ; )
	{
		count += StringGetLine(&data[count], index, '{');
		if(StringNumCompareIgnoreCase(index, "MORPH", (int)strlen("MORPH")) != 0)
		{
			break;
		}
		count += StringGetLine(&data[count], name, '\0');
		if(*name == '\0')
		{
			return 0;
		}
		morph = (POSE_MORPH*)StructArrayReserve(pose->morphs);
		morph->name = EncodeText(&pose->text_encode, name, strlen(name));
		count += GetLine(&data[count], weight, buff);
		if(sizeof(morph->weight) == 8)
		{
			(void)sscanf(weight, "%lf", &morph->weight);
		}
		else
		{
			(void)sscanf(weight, "%f", &morph->weight);
		}
		count += GetLine(&data[count], next_line, buff);
		count += GetLine(&data[count], next_line, buff);
		if(data[count] == '\0')
		{
			break;
		}
	}
	return count;
}

POSE* LoadPoseData(const char* data)
{
	char current_line[4096];
	char buffer[4096];
	int point;
	POSE *pose = (POSE*)MEM_ALLOC_FUNC(sizeof(*pose));
	(void)memset(pose, 0, sizeof(*pose));
	pose->bones = StructArrayNew(sizeof(POSE_BONE), DEFAULT_BUFFER_SIZE);
	pose->morphs = StructArrayNew(sizeof(POSE_MORPH), DEFAULT_BUFFER_SIZE);
	InitializeTextEncode(&pose->text_encode, guess_jp(data, (int)strlen(data)), "UTF-8");

	// モデル名を読み飛ばす
	point = GetLine(data, current_line, buffer);
	// ボーンの数を読み飛ばす
	point += GetLine(&data[point], current_line, buffer);
	if(ParsePoseBones(pose, &data[point], current_line, buffer) == 0)
	{
		DeletePose(&pose);
		return NULL;
	}
	(void)ParsePoseMorphs(pose, &data[point], current_line, buffer);
	return pose;
}

static void ReleasePoseBone(POSE_BONE* bone)
{
	MEM_FREE_FUNC(bone->name);
}

static void ReleasePoseMorph(POSE_MORPH* morph)
{
	MEM_FREE_FUNC(morph->name);
}

void DeletePose(POSE** pose)
{
	StructArrayDestroy(&((*pose)->bones), (void (*)(void*))ReleasePoseBone);
	StructArrayDestroy(&((*pose)->morphs), (void (*)(void*))ReleasePoseMorph);
	ReleaseTextEncode(&(*pose)->text_encode);
	MEM_FREE_FUNC(*pose);
	*pose = NULL;
}

void ApplyPose(POSE* pose, MODEL_INTERFACE* model, int apply_center)
{
	BONE_INTERFACE *target_bone;
	POSE_BONE *bones = (POSE_BONE*)pose->bones->buffer;
	POSE_BONE *bone;
	const int num_bones = (int)pose->bones->num_data;
	MORPH_INTERFACE *target_morph;
	POSE_MORPH *morphs = (POSE_MORPH*)pose->morphs->buffer;
	POSE_MORPH *morph;
	const int num_morphs = (int)pose->morphs->num_data;
	int i, j;

	if(!(model->type == MODEL_TYPE_PMD_MODEL || model->type == MODEL_TYPE_PMX_MODEL))
	{
		return;
	}

	if(apply_center != FALSE)
	{	// センターボーンの位置を変更
			// ポーズの適用は服等を貫通しないように徐々に適用する
		float maximum = 0;
		VECTOR3 *original_positions;
		VECTOR4 *original_rotations;
		VECTOR4 set_value;
		float scalar_value;
		float rate;
		int loop;

		original_positions = (VECTOR3*)MEM_ALLOC_FUNC(sizeof(*original_positions)*num_bones);
		original_rotations = (VECTOR4*)MEM_ALLOC_FUNC(sizeof(*original_rotations)*num_bones);

		for(i=0; i<num_bones; i++)
		{
			bone = &bones[i];
			if((target_bone = model->find_bone(model, bone->name)) != NULL)
			{
				target_bone->get_local_translation(target_bone, original_positions[i]);
				scalar_value = bone->position[0] = original_positions[i][0];
				if(maximum < scalar_value)
				{
					maximum = scalar_value;
				}
				scalar_value = bone->position[1] = original_positions[i][1];
				if(maximum < scalar_value)
				{
					maximum = scalar_value;
				}
				scalar_value = bone->position[2] = original_positions[i][2];
				if(maximum < scalar_value)
				{
					maximum = scalar_value;
				}
				target_bone->get_local_rotation(target_bone, original_rotations[i]);
				scalar_value = (bone->rotation[0] - original_rotations[i][0]) * 180 / (float)M_PI;
				if(maximum < scalar_value)
				{
					maximum = scalar_value;
				}
				scalar_value = (bone->rotation[1] - original_rotations[i][1]) * 180 / (float)M_PI;
				if(maximum < scalar_value)
				{
					maximum = scalar_value;
				}
				scalar_value = (bone->rotation[2] - original_rotations[i][2]) * 180 / (float)M_PI;
				if(maximum < scalar_value)
				{
					maximum = scalar_value;
				}
				scalar_value = (bone->rotation[3] - original_rotations[i][3]) * 180 / (float)M_PI;
				if(maximum < scalar_value)
				{
					maximum = scalar_value;
				}
			}
		}
		loop = (int)maximum;
		for(i=0; i<loop; i++)
		{
			rate = i / maximum;
			for(j=0; j<num_bones; j++)
			{
				bone = &bones[j];
				if((target_bone = model->find_bone(model, bone->name)) != NULL)
				{
					set_value[0] = bone->position[0] * rate + original_positions[j][0] * (1 - rate);
					set_value[1] = bone->position[1] * rate + original_positions[j][1] * (1 - rate);
					set_value[2] = bone->position[2] * rate + original_positions[j][2] * (1 - rate);
					target_bone->set_local_translation(target_bone, set_value);
					set_value[0] = bone->rotation[0] * rate + original_rotations[j][0] * (1 - rate);
					set_value[1] = bone->rotation[1] * rate + original_rotations[j][1] * (1 - rate);
					set_value[2] = bone->rotation[2] * rate + original_rotations[j][2] * (1 - rate);
					set_value[2] = bone->rotation[3] * rate + original_rotations[j][3] * (1 - rate);
					target_bone->set_local_rotation(target_bone, set_value);
				}
			}

			WorldStepSimulation(&model->scene->project->world, model->scene->project->world.fixed_time_step);
			SceneUpdateModel(model->scene, model, TRUE);
		}
		for(i=0; i<num_bones; i++)
		{
			bone = &bones[i];
			if((target_bone = model->find_bone(model, bone->name)) != NULL)
			{
				target_bone->set_local_translation(target_bone, bone->position);
				target_bone->set_local_rotation(target_bone, bone->rotation);
			}
		}

		MEM_FREE_FUNC(original_positions);
		MEM_FREE_FUNC(original_rotations);
	}
	else
	{
		VECTOR3 center_position = {0};
		VECTOR3 set_position;
		VECTOR3 model_center = {0};
		for(i=0; i<num_bones; i++)
		{
			bone = &bones[i];
			// センターボーン
			if(strcmp(bone->name, "\xE3\x82\xBB\xE3\x83\xB3\xE3\x82\xBF\xE3\x83\xBC") == 0)
			{
				COPY_VECTOR3(center_position, bone->position);
				break;
			}
		}
		if((target_bone = (BONE_INTERFACE*)model->find_bone(
			model, "\xE3\x82\xBB\xE3\x83\xB3\xE3\x82\xBF\xE3\x83\xBC")) != NULL)
		{
			target_bone->get_local_translation(bone, model_center);
		}
		for(i=0; i<num_bones; i++)
		{
			bone = &bones[i];
			set_position[0] = (fabsf(bone->position[0]) >= 1.5f)
				? bone->position[0] - center_position[0] + model_center[0] : bone->position[0];
			set_position[1] = (fabsf(bone->position[1]) >= 1.5f)
				? bone->position[1] - center_position[1] + model_center[1] : bone->position[1];
			set_position[2] = (fabsf(bone->position[2]) >= 1.5f)
				? bone->position[2] - center_position[2] + model_center[2] : bone->position[2];
			if((target_bone = model->find_bone(model, bone->name)) != NULL)
			{
				target_bone->set_local_translation(target_bone, set_position);
				target_bone->set_local_rotation(target_bone, bone->rotation);
			}
		}
	}

	for(i=0; i<num_morphs; i++)
	{
		morph = &morphs[i];
		if((target_morph = model->find_morph(model, morph->name)) != NULL)
		{
			target_morph->set_weight(target_morph, morph->weight);
		}
	}
}

#ifdef __cplusplus
}
#endif
