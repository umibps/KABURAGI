#include <string.h>
#include "memory.h"
#include "vmd_motion.h"
#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

static void InitializeVmdBaseAnimation(VMD_BASE_ANIMATION* animation)
{
	animation->keyframes = PointerArrayNew(DEFAULT_BUFFER_SIZE);
}

static void InitializeVmdBoneAnimation(
	VMD_BONE_ANIMATION* animation,
	MODEL_INTERFACE* model,
	APPLICATION* application
)
{
	(void)memset(animation, 0, sizeof(*animation));
	InitializeVmdBaseAnimation(&animation->animation);
	animation->model = model;
	animation->name2contexts = ght_create(DEFAULT_BUFFER_SIZE);
	ght_set_hash(animation->name2contexts, (ght_fn_hash_t)GetStringHash);
	animation->application = application;
}

FLOAT_T VmdBoneAnimationWeightValue(
	VMD_BONE_ANIMATION* animation,
	VMD_BONE_KEYFRAME* keyframe,
	FLOAT_T w,
	int at
)
{
	int index = (int)(w * VMD_BONE_KEYFRAME_TABLE_SIZE);
	FLOAT_T *v = keyframe->interpolation_table[at];
	return v[index] + (v[index+1] - v[index]) * (w * VMD_BONE_KEYFRAME_TABLE_SIZE - index);
}

static void VmdBoneAnimationLearpVector3(
	VMD_BONE_ANIMATION* animation,
	VMD_BONE_KEYFRAME* keyframe,
	const float* from,
	const float* to,
	FLOAT_T w,
	int at,
	FLOAT_T* value
)
{
	FLOAT_T value_from = from[at];
	FLOAT_T value_to = to[at];
	if(keyframe->linear[at] != FALSE)
	{
		*value = KeyframeLerp(&value_from, &value_to, &w);
	}
	else
	{
		FLOAT_T w2 = VmdBoneAnimationWeightValue(animation, keyframe, w, at);
		*value = KeyframeLerp(&value_from, &value_to, &w2);
	}
}

void VmdBoneAnimationCalculateKeyframes(
	VMD_BONE_ANIMATION* animation,
	FLOAT_T time_index_at,
	VMD_BONE_ANIMATION_CONTEXT* context
)
{
	VMD_BONE_KEYFRAME **keyframes = (VMD_BONE_KEYFRAME**)context->keyframes->buffer;
	int from_index;
	int to_index;
	VMD_BONE_KEYFRAME *keyframe_from;
	VMD_BONE_KEYFRAME *keyframe_to;
	VMD_BONE_KEYFRAME *keyframe_interpolation;
	FLOAT_T time_index_from;
	FLOAT_T time_index_to;
	float *position_from;
	float *rotation_from;
	float *position_to;
	float *rotation_to;

	FindKeyframeIndices(time_index_at, &animation->animation.current_time_index,
		&context->last_index, &from_index, &to_index, (KEYFRAME_INTERFACE**)keyframes, (int)context->keyframes->num_data);
	keyframe_from = keyframes[from_index];
	keyframe_to = keyframes[to_index];
	keyframe_interpolation = keyframe_to;
	position_from = keyframe_from->interface_data.position;
	rotation_from = keyframe_from->interface_data.rotation;
	time_index_from = keyframe_from->interface_data.base_data.time_index;
	position_to = keyframe_to->interface_data.position;
	rotation_to = keyframe_to->interface_data.rotation;
	time_index_to = keyframe_to->interface_data.base_data.time_index;

	if(time_index_from != time_index_to)
	{
		if(animation->animation.current_time_index <= time_index_from)
		{
			COPY_VECTOR3(context->position, position_from);
			COPY_VECTOR4(context->rotation, rotation_from);
		}
		else if(animation->animation.current_time_index >= time_index_to)
		{
			COPY_VECTOR3(context->position, position_to);
			COPY_VECTOR4(context->rotation, rotation_to);
		}
		else
		{
			const FLOAT_T w = (animation->animation.current_time_index - time_index_from) / (time_index_to - time_index_from);
			FLOAT_T x = 0, y = 0, z = 0;
			VmdBoneAnimationLearpVector3(animation, keyframe_interpolation, position_from, position_to, w, 0, &x);
			VmdBoneAnimationLearpVector3(animation, keyframe_interpolation, position_from, position_to, w, 1, &y);
			VmdBoneAnimationLearpVector3(animation, keyframe_interpolation, position_from, position_to, w, 2, &z);
			context->position[0] = (float)x;
			context->position[1] = (float)y;
			context->position[2] = (float)z;
			if(keyframe_interpolation->linear[3] != FALSE)
			{
				QuaternionSlerp(context->rotation, rotation_from, rotation_to, (float)w);
			}
			else
			{
				FLOAT_T w2 = VmdBoneAnimationWeightValue(animation, keyframe_interpolation, w, 3);
				QuaternionSlerp(context->rotation, rotation_from, rotation_to, (float)w2);
			}
		}
	}
	else
	{
		COPY_VECTOR3(context->position, position_from);
		COPY_VECTOR4(context->rotation, rotation_from);
	}
}

static VMD_BONE_ANIMATION_CONTEXT* VmdBoneAnimationContextNew(void)
{
	VMD_BONE_ANIMATION_CONTEXT *ret = (VMD_BONE_ANIMATION_CONTEXT*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));
	ret->keyframes = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	ret->rotation[3] = 1;

	return ret;
}

static void VmdBoneAnimationContextDestroy(VMD_BONE_ANIMATION_CONTEXT* context)
{
	PointerArrayDestroy(&context->keyframes, NULL);
	MEM_FREE_FUNC(context);
}

void VmdBoneAnimationMakeContext(VMD_BONE_ANIMATION* animation, MODEL_INTERFACE* model)
{
	VMD_BONE_KEYFRAME **keyframes;
	VMD_BONE_KEYFRAME *keyframe;
	VMD_BONE_ANIMATION_CONTEXT *context;
	VMD_BONE_ANIMATION_CONTEXT *ptr;
	BONE_INTERFACE *bone;
	const int num_keyframes = (int)animation->animation.keyframes->num_data;
	ght_iterator_t iterator;
	void *key;
	char *name;
	int i;

	if(model == NULL)
	{
		return;
	}

	HashTableReleaseAll(animation->name2contexts,
		(void (*)(void*))VmdBoneAnimationContextDestroy);
	for(i=0; i<num_keyframes; i++)
	{
		keyframes = (VMD_BONE_KEYFRAME**)animation->animation.keyframes->buffer;
		keyframe = keyframes[i];
		name = keyframe->interface_data.base_data.name;
		ptr = (VMD_BONE_ANIMATION_CONTEXT*)ght_get(animation->name2contexts,
			(unsigned int)strlen(name), name);
		if(ptr != NULL)
		{
			context = ptr;
			PointerArrayAppend(context->keyframes, keyframe);
		}
		else if((bone = model->find_bone(model, name)) != NULL)
		{
			context = VmdBoneAnimationContextNew();
			PointerArrayAppend(context->keyframes, keyframe);
			(void)ght_insert(animation->name2contexts, context,
				(unsigned int)strlen(name), name);
			context->bone = bone;
		}
	}

	for(context = (VMD_BONE_ANIMATION_CONTEXT*)ght_first(animation->name2contexts, &iterator, &key);
		context != NULL; context = (VMD_BONE_ANIMATION_CONTEXT*)ght_next(animation->name2contexts, &iterator, &key))
	{
		qsort(context->keyframes->buffer, context->keyframes->num_data,
			sizeof(void*), (int (*)(const void*, const void*))KeyframeTimeIndexCompare);
		keyframe = (VMD_BONE_KEYFRAME*)context->keyframes->buffer[context->keyframes->num_data-1];
		if(animation->animation.duration_time_index < keyframe->interface_data.base_data.time_index)
		{
			animation->animation.duration_time_index = keyframe->interface_data.base_data.time_index;
		}
	}
}

void VmdBoneAnimationSeek(VMD_BONE_ANIMATION* animation, FLOAT_T time_index_at)
{
	VMD_BONE_ANIMATION_CONTEXT *context;
	ght_iterator_t iterator;
	BONE_INTERFACE *bone;
	void *key;

	if(animation->model == NULL)
	{
		return;
	}

	for(context = (VMD_BONE_ANIMATION_CONTEXT*)ght_first(animation->name2contexts, &iterator, &key);
		context != NULL; context = (VMD_BONE_ANIMATION_CONTEXT*)ght_next(animation->name2contexts, &iterator, &key))
	{
		VmdBoneAnimationCalculateKeyframes(animation, time_index_at, context);
		bone = context->bone;
		bone->set_local_translation(bone, context->position);
		bone->set_local_rotation(bone, context->rotation);
	}

	animation->animation.previous_time_index = animation->animation.current_time_index;
	animation->animation.current_time_index = time_index_at;
}

void VmdBoneAnimationSetModel(VMD_BONE_ANIMATION* animation, MODEL_INTERFACE* model)
{
	VmdBoneAnimationMakeContext(animation, model);
	animation->model = model;
}

KEYFRAME_INTERFACE* VmdBoneAnimationFindKeyframeAt(
	VMD_BONE_ANIMATION* animation,
	FLOAT_T time_index,
	const char* name
)
{
	if(name != NULL)
	{
		VMD_BONE_ANIMATION_CONTEXT *context = ght_get(
			animation->name2contexts, (unsigned int)strlen(name), name);
		if(context != NULL)
		{
			int index = FindKeyframeIndex(time_index, (KEYFRAME_INTERFACE**)context->keyframes->buffer,
				(int)context->keyframes->num_data);
			return (index >= 0) ? (KEYFRAME_INTERFACE*)context->keyframes->buffer[index] : NULL;
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

void InitializeVmdModelAnimation(VMD_MODEL_ANIMATION* animation)
{
	InitializeVmdBaseAnimation(&animation->animation);
}

VMD_MODEL_KEYFRAME* VmdModelAnimationFindKeyframeAt(VMD_MODEL_ANIMATION* animation, int at)
{
	if(CHECK_BOUND(at, 0, (int)animation->animation.keyframes->num_data))
	{
		return (VMD_MODEL_KEYFRAME*)animation->animation.keyframes->buffer[at];
	}
	return NULL;
}

KEYFRAME_INTERFACE* VmdModelAnimationFindKeyframe(VMD_MODEL_ANIMATION* animation, FLOAT_T time_index)
{
	int index = FindKeyframeIndex(time_index, (KEYFRAME_INTERFACE**)animation->animation.keyframes->buffer,
		(int)animation->animation.keyframes->num_data);
	return (index >= 0) ? (KEYFRAME_INTERFACE*)animation->animation.keyframes->buffer[index] : NULL;
}

void VmdModelAnimationSeek(VMD_MODEL_ANIMATION* animation, FLOAT_T time_index_at)
{
	if(animation->model != NULL && animation->animation.keyframes->num_data > 0)
	{
		VMD_MODEL_KEYFRAME *keyframe_from;
		int from_index;
		int to_index;
		FindKeyframeIndices(time_index_at, &animation->animation.current_time_index,
			&animation->animation.last_time_index, &from_index, &to_index,
				(KEYFRAME_INTERFACE**)animation->animation.keyframes->buffer, (int)animation->animation.keyframes->num_data);
		keyframe_from = VmdModelAnimationFindKeyframeAt(animation, from_index);
		VmdModelKeyframeUpdateInverseKinematics(keyframe_from, animation->model);
		animation->model->set_visible(animation->model, keyframe_from->visible);
		animation->animation.previous_time_index = animation->animation.current_time_index;
		animation->animation.current_time_index = time_index_at;
	}
}

void VmdModelAnimationSetModel(VMD_MODEL_ANIMATION* animation, MODEL_INTERFACE* model)
{
	animation->model = model;
}

////////////////////////////////////////////////////////////////////////////////////////////

void InitializeVmdMorphAnimation(
	VMD_MORPH_ANIMATION* animation,
	MODEL_INTERFACE* model,
	APPLICATION* application
)
{
	InitializeVmdBaseAnimation(&animation->animation);
	animation->name2contexts = ght_create(DEFAULT_BUFFER_SIZE);
	ght_set_hash(animation->name2contexts, (ght_fn_hash_t)GetStringHash);
	animation->model = model;
	animation->application = application;
}

void VmdMorphAnimationCalculateFrames(
	VMD_MORPH_ANIMATION* animation,
	FLOAT_T time_index_at,
	VMD_MORPH_ANIMATION_CONTEXT* context
)
{
	VMD_MORPH_KEYFRAME **keyframes = (VMD_MORPH_KEYFRAME**)context->keyframes->buffer;
	VMD_MORPH_KEYFRAME *keyframe_from;
	VMD_MORPH_KEYFRAME *keyframe_to;
	int from_index;
	int to_index;
	FLOAT_T weight_from;
	FLOAT_T weight_to;

	FindKeyframeIndices(time_index_at, &animation->animation.current_time_index,
		&animation->animation.last_time_index, &from_index, &to_index,
			(KEYFRAME_INTERFACE**)context->keyframes->buffer, (int)context->keyframes->num_data);
	keyframe_from = keyframes[from_index];
	keyframe_to = keyframes[to_index];
	weight_from = keyframe_from->weight;
	weight_to = keyframe_to->weight;
	if(keyframe_from->interface_data.base_data.time_index != keyframe_to->interface_data.base_data.time_index)
	{
		FLOAT_T w = (animation->animation.current_time_index - keyframe_from->interface_data.base_data.time_index)
			/ (keyframe_to->interface_data.base_data.time_index - keyframe_from->interface_data.base_data.time_index);
		context->weight = KeyframeLerp(&weight_from, &weight_to, &w);
	}
	else
	{
		context->weight = weight_from;
	}
	animation->animation.previous_time_index = animation->animation.current_time_index;
	animation->animation.current_time_index = time_index_at;
}

void VmdMorphAnimationSeek(VMD_MORPH_ANIMATION* animation, FLOAT_T time_index_at)
{
	VMD_MORPH_ANIMATION_CONTEXT *context;
	MORPH_INTERFACE *morph;
	ght_iterator_t iterator;
	void *key;

	if(animation->model == NULL)
	{
		return;
	}

	for(context = (VMD_MORPH_ANIMATION_CONTEXT*)ght_first(animation->name2contexts, &iterator, &key);
		context != NULL; context = (VMD_MORPH_ANIMATION_CONTEXT*)ght_next(animation->name2contexts, &iterator, &key))
	{
		VmdMorphAnimationCalculateFrames(animation, time_index_at, context);
		morph = context->morph;
		morph->set_weight(morph, context->weight);
	}
	animation->animation.previous_time_index = animation->animation.current_time_index;
	animation->animation.current_time_index = time_index_at;
}

VMD_MORPH_ANIMATION_CONTEXT* VmdMorphAnimationContextNew(void)
{
	VMD_MORPH_ANIMATION_CONTEXT *ret = (VMD_MORPH_ANIMATION_CONTEXT*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));
	ret->keyframes = PointerArrayNew(DEFAULT_BUFFER_SIZE);

	return ret;
}

void DestroyVmdMorphAnimationContext(VMD_MORPH_ANIMATION_CONTEXT* context)
{
	PointerArrayDestroy(&context->keyframes, NULL);
	MEM_FREE_FUNC(context);
}

void VmdMorphAnimationMakeContexts(VMD_MORPH_ANIMATION* animation, MODEL_INTERFACE* model)
{
	VMD_MORPH_ANIMATION_CONTEXT *context;
	VMD_MORPH_KEYFRAME **keyframes = (VMD_MORPH_KEYFRAME**)animation->animation.keyframes->buffer;
	VMD_MORPH_KEYFRAME *keyframe;
	MORPH_INTERFACE *morph;
	ght_iterator_t iterator;
	void *key;
	char *name;
	const int num_keyframes = (int)animation->animation.keyframes->num_data;
	int i;

	if(model == NULL)
	{
		return;
	}

	HashTableReleaseAll(animation->name2contexts, (void (*)(void*))DestroyVmdMorphAnimationContext);
	for(i=0; i<num_keyframes; i++)
	{
		keyframe = keyframes[i];
		name = keyframe->interface_data.base_data.name;
		context = (VMD_MORPH_ANIMATION_CONTEXT*)ght_get(animation->name2contexts, (unsigned int)strlen(name), name);
		if(context != NULL)
		{
			PointerArrayAppend(context->keyframes, keyframe);
		}
		else if((morph = model->find_morph(model, name)) != NULL)
		{
			context = VmdMorphAnimationContextNew();
			PointerArrayAppend(context->keyframes, keyframe);
			context->morph = morph;
		}
	}

	for(context = (VMD_MORPH_ANIMATION_CONTEXT*)ght_first(animation->name2contexts, &iterator, &key);
		context != NULL; context = ght_next(animation->name2contexts, &iterator, &key))
	{
		qsort(context->keyframes->buffer, context->keyframes->num_data,
			sizeof(void*), (int (*)(const void*, const void*))KeyframeTimeIndexCompare);
		keyframe = (VMD_MORPH_KEYFRAME*)context->keyframes->buffer[context->keyframes->num_data-1];
		if(animation->animation.duration_time_index < keyframe->interface_data.base_data.time_index)
		{
			animation->animation.duration_time_index = keyframe->interface_data.base_data.time_index;
		}
	}
}

void VmdMorphAnimationSetModel(VMD_MORPH_ANIMATION* animation, MODEL_INTERFACE* model)
{
	VmdMorphAnimationMakeContexts(animation, model);
	animation->model = model;
}

////////////////////////////////////////////////////////////////////////////////////////////

MOTION_INTERFACE* VmdMotionNew(
	SCENE* scene,
	MODEL_INTERFACE* model
)
{
	VMD_MOTION *ret = (VMD_MOTION*)MEM_ALLOC_FUNC(sizeof(*ret));
	eKEYFRAME_TYPE keyframe_type;
	APPLICATION *application = scene->project->application_context;
	(void)memset(ret, 0, sizeof(*ret));

	ret->interface_data.type = MOTION_TYPE_VMD_MOTION;
	ret->interface_data.scene = scene;
	ret->interface_data.application = application;
	ret->interface_data.model = model;
	ret->type2animation = ght_create(DEFAULT_BUFFER_SIZE);
	ret->interface_data.seek =
		(void (*)(void*, FLOAT_T))VmdMotionSeek;
	ret->interface_data.find_keyframe =
		(void* (*)(void*, eKEYFRAME_TYPE, FLOAT_T, void*))VmdMotionFindKeyframe;
	ret->interface_data.update =
		(void (*)(void*, eKEYFRAME_TYPE))VmdMotionUpdate;
	ret->interface_data.add_keyframe =
		(void (*)(void*, void*))VmdMotionAddKeyframe;
	ret->interface_data.set_model =
		(void (*)(void*, void*))VmdMotionSetModel;
	ret->flags |= VMD_MOTION_FLAG_ACTIVE;

	InitializeVmdBoneAnimation(&ret->bone_motion, model, application);
	InitializeVmdModelAnimation(&ret->model_motion);
	InitializeVmdMorphAnimation(&ret->morph_motion, model, application);
	keyframe_type = KEYFRAME_TYPE_BONE;
	ght_insert(ret->type2animation, &ret->bone_motion, sizeof(keyframe_type), &keyframe_type);
	keyframe_type = KEYFRAME_TYPE_MODEL;
	ght_insert(ret->type2animation, &ret->model_motion, sizeof(keyframe_type), &keyframe_type);

	return (MOTION_INTERFACE*)ret;
}

void VmdMotionUpdate(VMD_MOTION* motion, eKEYFRAME_TYPE type)
{
	switch(type)
	{
	case KEYFRAME_TYPE_BONE:
		VmdBoneAnimationSetModel(&motion->bone_motion, motion->interface_data.model);
		break;
	}
}

void VmdMotionAddKeyframe(VMD_MOTION* motion, KEYFRAME_INTERFACE* frame)
{
	VMD_BASE_ANIMATION *animation;

	if(frame == NULL || frame->layer_index != 0)
	{
		return;
	}

	if((animation = (VMD_BASE_ANIMATION*)ght_get(motion->type2animation,
		sizeof(frame->type), &frame->type)) != NULL)
	{
		PointerArrayAppend(animation->keyframes, frame);
	}
}

void VmdMotionSeek(VMD_MOTION* motion, FLOAT_T time_index)
{
	VmdBoneAnimationSeek(&motion->bone_motion, time_index);
	VmdModelAnimationSeek(&motion->model_motion, time_index);
	VmdMorphAnimationSeek(&motion->morph_motion, time_index);
}

KEYFRAME_INTERFACE* VmdMotionFindKeyframe(
	VMD_MOTION* motion,
	eKEYFRAME_TYPE keyframe_type,
	FLOAT_T time_index,
	void* detail_data
)
{
	switch(keyframe_type)
	{
	case KEYFRAME_TYPE_BONE:
		return VmdBoneAnimationFindKeyframeAt(&motion->bone_motion, time_index, (char*)detail_data);
	case KEYFRAME_TYPE_MODEL:
		return VmdModelAnimationFindKeyframe(&motion->model_motion, time_index);
	}
	return NULL;
}

void VmdMotionSetModel(VMD_MOTION* motion, MODEL_INTERFACE* model)
{
	VmdBoneAnimationSetModel(&motion->bone_motion, model);
	VmdModelAnimationSetModel(&motion->model_motion, model);
	VmdMorphAnimationSetModel(&motion->morph_motion, model);
	motion->interface_data.model = model;
}

#ifdef __cplusplus
}
#endif
