#include <string.h>
#include <limits.h>
#include "soft_body.h"
#include "memory_stream.h"
#include "pmx_model.h"
#include "memory.h"

#define PMX_SOFT_BODY_UNIT_SIZE 24
typedef struct _PMX_SOFT_BODY_UNIT
{
	uint8 group;
	uint16 uncollidable_group;
	uint8 flags;
	int32 distance_b_link;
	int32 num_clusters;
	float mass;
	float margin;
	int32 aero_model;
} PMX_SOFT_BODY_UNIT;

#define PMX_SOFT_BODY_CONFIG_UNIT_SIZE 48
typedef struct _PMX_SOFT_BODY_CONFIG_UNIT
{
	float vcf;
	float vp;
	float dg;
	float lf;
	float pr;
	float vc;
	float df;
	float mt;
	float chr;
	float khr;
	float shr;
	float ahr;
} PMX_SOFT_BODY_CONFIG_UNIT;

#define PMX_SOFT_BODY_CLUSTER_UNIT_SIZE 24
typedef struct _PMX_SOFT_BODY_CLUSTER_UNIT
{
	float srhr_cl;
	float skhr_cl;
	float sshr_cl;
	float sr_splt_cl;
	float sk_splt_cl;
	float ss_splt_cl;
} PMX_SOFT_BODY_CLUSTER_UNIT;

#define PMX_SOFT_BODY_ITERATION_UNIT_SIZE 16
typedef struct _PMX_SOFT_BODY_ITERATION_UNIT
{
	int32 v_it;
	int32 p_it;
	int32 d_it;
	int32 c_it;
} PMX_SOFT_BODY_ITERATION_UNIT;

#define PMX_SOFT_BODY_MATERIAL_UNIT_SIZE 12
typedef struct _PMX_SOFT_BODY_MATERIAL_UNIT
{
	float lst;
	float ast;
	float vst;
} PMX_SOFT_BODY_MATERIAL_UNIT;

void InitializePmxSoftBody(PMX_SOFT_BODY* body, PMX_MODEL* model)
{
	(void)memset(body, 0, sizeof(*body));

	body->model = model;
	body->index = -1;
}

int PmxSoftBodyPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	MODEL_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, rest, 1};
	size_t base_size;
	int32 num_bodies;
	char *name_ptr;
	int length;
	int i;

	info->soft_bodies = data;
	if(info->version >= 2.1f)
	{
		if(MemRead(&num_bodies, sizeof(num_bodies), 1, &stream) == 0)
		{
			return FALSE;
		}

		for(i=0; i<num_bodies; i++)
		{
			// “ú–{Œê–¼
			if((length = GetTextFromStream(&data[stream.data_point], &name_ptr)) < 0)
			{
				return FALSE;
			}
			stream.data_point += sizeof(int32) + length;

			// ‰pŒê–¼
			if((length = GetTextFromStream(&data[stream.data_point], &name_ptr)) < 0)
			{
				return FALSE;
			}
			stream.data_point += sizeof(int32) + length;

			base_size = PMX_SOFT_BODY_UNIT_SIZE + PMX_SOFT_BODY_CONFIG_UNIT_SIZE
				+ PMX_SOFT_BODY_CLUSTER_UNIT_SIZE + PMX_SOFT_BODY_ITERATION_UNIT_SIZE
					+ PMX_SOFT_BODY_MATERIAL_UNIT_SIZE + info->material_index_size + 1;
			if(MemSeek(&stream, (long)base_size, SEEK_CUR) != 0)
			{
				return FALSE;
			}
		}

		info->soft_bodies_count = num_bodies;
	}

	return TRUE;
}

int LoadPmxSoftBodies(STRUCT_ARRAY* bodies)
{
	const int num_bodies = (int)bodies->num_data;
	PMX_SOFT_BODY *b = (PMX_SOFT_BODY*)bodies->buffer;
	int i;

	for(i=0; i<num_bodies; i++)
	{
		b[i].index = i;
	}

	return TRUE;
}

void PmxSoftBodyRead(
	PMX_SOFT_BODY* body,
	uint8* data,
	size_t* data_size,
	MODEL_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, INT_MAX, 1};
	char *name_ptr;
	int length;
	TEXT_ENCODE *encode = info->encoding;

	// “ú–{Œê–¼
	length = GetTextFromStream((char*)data, &name_ptr);
	body->name = EncodeText(encode, name_ptr, length);
	stream.data_point += sizeof(int32) + length;

	// ‰pŒê–¼
	length = GetTextFromStream((char*)data, &name_ptr);
	body->name = EncodeText(encode, name_ptr, length);
	stream.data_point += sizeof(int32) + length;

	*data_size = stream.data_point;
}

void ReleasePmxSoftBody(PMX_SOFT_BODY* body)
{
	MEM_FREE_FUNC(body->name);
	MEM_FREE_FUNC(body->english_name);
}
