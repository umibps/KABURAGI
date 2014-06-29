#include <string.h>
#include "memory.h"
#include "keyframe.h"
#include "application.h"
#include "vmd_keyframe.h"

#ifdef __cplusplus
extern "C" {
#endif

BONE_KEYFRAME_INTERFACE* BoneKeyframeNew(
	MOTION_INTERFACE* motion,
	SCENE* scene
)
{
	BONE_KEYFRAME_INTERFACE *ret = NULL;
	switch(motion->type)
	{
	case MOTION_TYPE_VMD_MOTION:
		ret = VmdBoneKeyframeNew(scene);
	}

	return ret;
}

int FindKeyframeIndex(
	FLOAT_T key,
	KEYFRAME_INTERFACE** keyframes,
	const int num_keyframes
)
{
	KEYFRAME_INTERFACE *keyframe;
	int minimum = 0;
	int maximum = num_keyframes - 1;

	if(maximum == 0)
	{
		keyframe = keyframes[0];
		if(FuzzyZero((float)(keyframe->time_index - key)) != FALSE)
		{
			return 0;
		}
	}

	while(minimum < maximum)
	{
		int mid = (minimum + maximum) / 2;
		keyframe = keyframes[mid];
		if(keyframe->time_index < key)
		{
			minimum = mid + 1;
		}
		else
		{
			maximum = mid;
		}
		if(minimum == maximum)
		{
			if(FuzzyZero((float)(keyframe->time_index - key)) != FALSE)
			{
				return minimum;
			}
		}
	}

	return -1;
}

#ifdef __cplusplus
}
#endif
