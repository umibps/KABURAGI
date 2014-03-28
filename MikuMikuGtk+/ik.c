#include <string.h>
#include <math.h>
#include "ik.h"
#include "memory_stream.h"
#include "memory.h"

#define MIN_DISTANCE 0.0001f
#define MIN_ANGLE 0.00000001f
#define MIN_AXIS 0.0000001f
#define MIN_ROTATION_SUM 0.002f
#define MIN_ROTATION 0.00001f

#define IK_CHUNK_SIZE 11

#ifndef M_PI
# define M_PI 3.1415926535897932384626433832795
#endif

typedef struct _IK_CHUNK
{
	uint16 dest_bone_id;
	uint16 target_bone_id;
	uint8 num_links;
	uint16 num_iterations;
	float angle_constraint;
} IK_CHUNK;

static size_t Stride(uint8* data)
{
	return IK_CHUNK_SIZE * data[4] * sizeof(uint16);
}

size_t IkTotalSize(
	uint8* data,
	size_t rest,
	size_t count,
	int *ok
)
{
	size_t size = 0;
	uint8 *ptr = data;
	size_t i;

	for(i=0; i<count; i++)
	{
		size_t required;
		if(IK_CHUNK_SIZE > rest)
		{
			*ok = FALSE;
			return 0;
		}

		required = Stride(ptr);
		if(required > rest)
		{
			*ok = FALSE;
			return 0;
		}
		rest -= required;
		size += required;
		ptr += required;
	}

	*ok = TRUE;
	return size;
}

IK* IkNew(void)
{
	IK *ret;
	ret = (IK*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	return ret;
}

void IkSetAngleConstraint(IK* ik, float value)
{
	ik->raw_angle_constraint = value;
	ik->angle_constraint = value * (float)M_PI;
}

void ReadIk(IK* ik, uint8* data, POINTER_ARRAY* bones)
{
	IK_CHUNK chunk;
	MEMORY_STREAM stream = {0};
	WORD_ARRAY *bone_iks;
	POINTER_ARRAY *bone_iks2;
	uint8 *ptr;
	int num_bones;
	int i;

	stream.buff_ptr = data;
	stream.block_size = 1;
	stream.data_size = sizeof(IK_CHUNK);

	(void)MemRead(&chunk.dest_bone_id, sizeof(chunk.dest_bone_id), 1, &stream);
	(void)MemRead(&chunk.target_bone_id, sizeof(chunk.target_bone_id), 1, &stream);
	(void)MemRead(&chunk.num_links, sizeof(chunk.num_links), 1, &stream);
	(void)MemRead(&chunk.num_iterations, sizeof(chunk.num_iterations), 1, &stream);
	(void)MemRead(&chunk.angle_constraint, sizeof(chunk.angle_constraint), 1, &stream);

	bone_iks = WordArrayNew(DEFAULT_BUFFER_SIZE);
	num_bones = (int)bones->num_data;
	ptr = data + IK_CHUNK_SIZE;
	for(i=0; i<num_bones; i++)
	{
		int16 bone_id = *((int16*)ptr);
		if(bone_id >= 0 && bone_id < num_bones)
		{
			WordArrayAppend(bone_iks, bone_id);
		}
		ptr += sizeof(uint16);
	}

	if(chunk.dest_bone_id >= 0 && chunk.dest_bone_id < num_bones
		&& chunk.target_bone_id >= 0 && chunk.target_bone_id < num_bones)
	{
		bone_iks2 = PointerArrayNew(DEFAULT_BUFFER_SIZE);
		chunk.num_links = (uint8)bone_iks->num_data;
		for(i=0; i<chunk.num_links; i++)
		{
			BONE *bone = (BONE*)bones->buffer[bone_iks->buffer[i]];
			PointerArrayAppend(bone_iks2, bone);
		}
		IkSetAngleConstraint(ik, chunk.angle_constraint);
		ik->iteration = chunk.num_iterations;
		PointerArrayDestroy(&ik->bones, NULL);
		ik->bones = PointerArrayNew(bone_iks2->num_data);
		for(i=0; i<(int)bone_iks2->num_data; i++)
		{
			ik->bones->buffer[i] = bone_iks2->buffer[i];
		}
		PointerArrayDestroy(&bone_iks2, NULL);
	}

	WordArrayDestroy(&bone_iks);
}
