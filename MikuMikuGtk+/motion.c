#include <math.h>
#include "motion.h"
#include "application.h"
#include "vmd_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

static FLOAT_T Spline1(FLOAT_T* t, FLOAT_T* p1, FLOAT_T* p2)
{
	return ((1 + 3 * (*p1) - 3 * (*p2)) * (*t) * (*t) * (*t)
		+ (3 * (*p2) - 6 * (*p1)) * (*t) * (*t)
		+ 3 * (*p1) * (*t));
}

static FLOAT_T Spline2(FLOAT_T* t, FLOAT_T* p1, FLOAT_T* p2)
{
	return ((3 + 9 * (*p1) - 9 * (*p2)) * (*t) * (*t)
		+ (6 * (*p2) - 12 * (*p1)) * (*t) + 3 * (*p1));
}

void BuildInterpolationTable(
	FLOAT_T x1,
	FLOAT_T x2,
	FLOAT_T y1,
	FLOAT_T y2,
	int size,
	FLOAT_T* table
)
{
	FLOAT_T in;
	FLOAT_T tt;
	FLOAT_T t;
	FLOAT_T v;
	int i;

	for(i=0; i<size; i++)
	{
		in = (FLOAT_T)i / size;
		t = in;
		for( ; ; )
		{
			v = Spline1(&t, &x1, &x2) - in;
			if(fabsf((float)v) < 0.0001f)
			{
				break;
			}
			tt = Spline2(&t, &x1, &x2);
			if(FuzzyZero((float)tt) != 0)
			{
				break;
			}
			t -= v / tt;
		}
		table[i] = Spline1(&t, &y1, &y2);
	}
	table[size] = 1;
}

MOTION_INTERFACE* NewMotion(eMOTION_TYPE type, SCENE* scene, MODEL_INTERFACE* model)
{
	switch(type)
	{
	case MOTION_TYPE_VMD_MOTION:
		return VmdMotionNew(scene, model);
	}

	return NULL;
}

int KeyframeTimeIndexCompare(const KEYFRAME_INTERFACE** left, const KEYFRAME_INTERFACE** right)
{
	if((*left)->layer_index == (*right)->layer_index)
	{
		FLOAT_T diff = (*left)->time_index - (*right)->time_index;
		if(diff < 0)
		{
			return -1;
		}
		else if(diff > 0)
		{
			return 1;
		}
		return 0;
	}

	return (*left)->layer_index - (*right)->layer_index;
}

void FindKeyframeIndices(
	FLOAT_T seek_index,
	FLOAT_T* current_keyframe,
	int* last_index,
	int* from_index,
	int* to_index,
	KEYFRAME_INTERFACE** keyframes,
	const int num_keyframes
)
{
	KEYFRAME_INTERFACE *last_keyframe = keyframes[num_keyframes-1];
	int i;

	*current_keyframe = (seek_index < last_keyframe->time_index) ? seek_index : last_keyframe->time_index;
	*from_index = *to_index = 0;
	if(*current_keyframe >= keyframes[*last_index]->time_index)
	{
		for(i=*last_index; i<num_keyframes; i++)
		{
			if(*current_keyframe <= keyframes[i]->time_index)
			{
				*to_index = i;
				break;
			}
		}
	}
	else
	{
		for(i=0; i<=*last_index && i<num_keyframes; i++)
		{
			if(*current_keyframe <= keyframes[i]->time_index)
			{
				*to_index = i;
				break;
			}
		}
	}
	if(*to_index >= num_keyframes)
	{
		*to_index = num_keyframes - 1;
	}
	*from_index = (*to_index <= 1) ? 0 : *to_index - 1;
	*last_index = *from_index;
}

FLOAT_T KeyframeLerp(const FLOAT_T* x, const FLOAT_T* y, const FLOAT_T* t)
{
	return *x + (*y - *x) * (*t);
}

#ifdef __cplusplus
}
#endif
