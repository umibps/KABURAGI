// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include "vmd_keyframe.h"
#include "application.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void InitializeVmdBoneKeyframe(VMD_BONE_KEYFRAME* frame, SCENE* scene)
{
	(void)memset(frame, 0, sizeof(*frame));
	LOAD_IDENTITY_QUATERNION(frame->interface_data.rotation);
	frame->interface_data.base_data.type = KEYFRAME_TYPE_BONE;
	frame->interface_data.base_data.application = scene->project->application_context;
	frame->interface_data.set_default_interpolation_parameter =
		(void (*)(void*))VmdBoneKeyframeSetDefaultInterpolationParameter;
	frame->interface_data.set_local_translation =
		(void (*)(void*, float*))VmdBoneKeyframeSetTranslation;
	frame->interface_data.set_local_rotation =
		(void (*)(void*, float*))VmdBoneKeyframeSetRotation;
}

BONE_KEYFRAME_INTERFACE* VmdBoneKeyframeNew(SCENE* scene)
{
	VMD_BONE_KEYFRAME *ret = (VMD_BONE_KEYFRAME*)MEM_ALLOC_FUNC(sizeof(*ret));
	InitializeVmdBoneKeyframe(ret, scene);
	return (BONE_KEYFRAME_INTERFACE*)ret;
}

static void GetValueFromTable(int8* table, int i, float v[4])
{
	v[0] = (float)((table[i] > 0) ? table[i] : 0);
	v[1] = (float)((table[i+4] > 0) ? table[i+4] : 0);
	v[2] = (float)((table[i+8] > 0) ? table[i+8] : 0);
	v[3] = (float)((table[i+12] > 0) ? table[i+12] : 0);
}

float* VmdBoneKeyframeGetInterpolationParameter(
	VMD_BONE_KEYFRAME* frame,
	eBONE_KEYFRAME_INTERPOLATION_TYPE type
)
{
	switch(type)
	{
	case BONE_KEYFRAME_INTERPOLATION_TYPE_X:
		return frame->parameter.x;
	case BONE_KEYFRAME_INTERPOLATION_TYPE_Y:
		return frame->parameter.y;
	case BONE_KEYFRAME_INTERPOLATION_TYPE_Z:
		return frame->parameter.z;
	case BONE_KEYFRAME_INTERPOLATION_TYPE_ROTATION:
		return frame->parameter.rotation;
	}

	return NULL;
}

void VmdBoneKeyframeSetInterpolationParameterInternal(
	VMD_BONE_KEYFRAME* frame,
	eBONE_KEYFRAME_INTERPOLATION_TYPE type,
	const float value[4]
)
{
	float *w = VmdBoneKeyframeGetInterpolationParameter(frame, type);
	COPY_VECTOR4(w, value);
}

void VmdBoneKeyframeSetInterpolationTable(
	VMD_BONE_KEYFRAME* frame,
	int8* table
)
{
	float v[4];
	int i;
	for(i=0; i<MAX_BONE_KEYFRAME_INTERPOLATION_TYPE; i++)
	{
		frame->linear[i] = (table[i] == table[i+4] && table[i*12]);
	}

	for(i=0; i<MAX_BONE_KEYFRAME_INTERPOLATION_TYPE; i++)
	{
		GetValueFromTable(table, i, v);
		MEM_FREE_FUNC(frame->interpolation_table[i]);
		if(frame->linear[i] != FALSE)
		{
			frame->interpolation_table[i] = NULL;
			VmdBoneKeyframeSetInterpolationParameterInternal(
				frame, (eBONE_KEYFRAME_INTERPOLATION)i, v);
			continue;
		}
		frame->interpolation_table[i] = (FLOAT_T*)MEM_ALLOC_FUNC(
			sizeof(FLOAT_T) * (VMD_BONE_KEYFRAME_TABLE_SIZE + 1));
		BuildInterpolationTable(v[0]/(FLOAT_T)127, v[2]/(FLOAT_T)127,
			v[1]/(FLOAT_T)127, v[3]/(FLOAT_T)127, VMD_BONE_KEYFRAME_TABLE_SIZE, frame->interpolation_table[i]);
	}
}

void VmdBoneKeyframeSetInterpolationParameter(
	VMD_BONE_KEYFRAME* frame,
	eBONE_KEYFRAME_INTERPOLATION_TYPE type,
	const float value[4]
)
{
	int8 table[VMD_BONE_KEYFRAME_TABLE_SIZE];
	int index;
	int i;

	VmdBoneKeyframeSetInterpolationParameterInternal(frame, type, value);
	for(i=0; i<4; i++)
	{
		index = i * MAX_BONE_KEYFRAME_INTERPOLATION_TYPE;
		table[index + BONE_KEYFRAME_INTERPOLATION_TYPE_X] = (int8)frame->parameter.x[i];
		table[index + BONE_KEYFRAME_INTERPOLATION_TYPE_Y] = (int8)frame->parameter.y[i];
		table[index + BONE_KEYFRAME_INTERPOLATION_TYPE_Z] = (int8)frame->parameter.z[i];
		table[index + BONE_KEYFRAME_INTERPOLATION_TYPE_ROTATION] = (int8)frame->parameter.rotation[i];
	}
	(void)memcpy(frame->raw_interpolation_table, table, sizeof(table));
	VmdBoneKeyframeSetInterpolationTable(frame, table);
}

void VmdBoneKeyframeSetDefaultInterpolationParameter(VMD_BONE_KEYFRAME* frame)
{
	const float default_parameter[4] = {20, 20, 107, 107};
	int i;
	for(i=0; i<MAX_BONE_KEYFRAME_INTERPOLATION_TYPE; i++)
	{
		VmdBoneKeyframeSetInterpolationParameter(frame,
			(eBONE_KEYFRAME_INTERPOLATION_TYPE)i, default_parameter);
	}
}

void VmdBoneKeyframeSetTranslation(VMD_BONE_KEYFRAME* frame, float* translation)
{
	COPY_VECTOR3(frame->interface_data.position, translation);
}

void VmdBoneKeyframeSetRotation(VMD_BONE_KEYFRAME* frame, float* rotation)
{
	COPY_VECTOR4(frame->interface_data.rotation, rotation);
}

typedef struct _VMD_MODEL_KEYFRAME_IK_STATE
{
	char *name;
	int enabled;
} VMD_MODEL_KEYFRAME_IK_STATE;

VMD_MODEL_KEYFRAME_IK_STATE* VmdModelKeyframeIkStateNew(char* name, int enabled)
{
	VMD_MODEL_KEYFRAME_IK_STATE *ret = (VMD_MODEL_KEYFRAME_IK_STATE*)MEM_ALLOC_FUNC(sizeof(*ret));
	ret->name = MEM_STRDUP_FUNC(name);
	ret->enabled = enabled;

	return ret;
}

MODEL_KEYFRAME_INTERFACE* VmdModelKeyframeNew(void* application_context)
{
	VMD_MODEL_KEYFRAME *ret = (VMD_MODEL_KEYFRAME*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));
	ret->states = ght_create(DEFAULT_BUFFER_SIZE);
	ght_set_hash(ret->states, (ght_fn_hash_t)GetStringHash);
	ret->interface_data.base_data.application = (APPLICATION*)application_context;

	return (MODEL_KEYFRAME_INTERFACE*)ret;
}

void VmdModelKeyframeUpdateInverseKinematics(VMD_MODEL_KEYFRAME* keyframe, MODEL_INTERFACE* model)
{
	VMD_MODEL_KEYFRAME_IK_STATE *state;
	BONE_INTERFACE *bone;
	ght_iterator_t iterator;
	void *key;
	for(state = (VMD_MODEL_KEYFRAME_IK_STATE*)ght_first(keyframe->states, &iterator, &key);
		state != NULL; state = (VMD_MODEL_KEYFRAME_IK_STATE*)ght_next(keyframe->states, &iterator, &key))
	{
		if((bone = model->find_bone(model, state->name)) != NULL)
		{
			bone->set_inverse_kinematics_enable(bone, state->enabled);
		}
	}
}

#ifdef __cplusplus
}
#endif
