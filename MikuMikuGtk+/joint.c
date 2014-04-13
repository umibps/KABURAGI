#include <string.h>
#include <limits.h>
#include "joint.h"
#include "pmx_model.h"
#include "pmd_model.h"
#include "memory_stream.h"
#include "memory.h"

#define BASE_JOINT_DEFAULT_DAMPING 0.25f

void InitializeBaseJoint(BASE_JOINT* joint, void *model)
{
	(void)memset(joint, 0, sizeof(*joint));
	joint->parent_model = model;
	joint->rigid_body1_index = -1;
	joint->rigid_body2_index = -1;
}

void ReleaseBaseJoint(BASE_JOINT* joint)
{
	BtRigidBodyRemoveConstraint(joint->rigid_body1, joint->constraint);
	BtRigidBodyRemoveConstraint(joint->rigid_body2, joint->constraint);
	DeleteBtTypedConstraint(joint->constraint);
	MEM_FREE_FUNC(joint->name);
	MEM_FREE_FUNC(joint->english_name);
}

void BaseJointGetWorldTransform(BASE_JOINT* joint, void* transform)
{
	float mx[9], my[9], mz[9];
	float position[3];

	Matrix3x3SetEulerZYX(mx, - joint->rotation[0], 0, 0);
	Matrix3x3SetEulerZYX(my, 0, - joint->rotation[1], 0);
	Matrix3x3SetEulerZYX(mz, 0, 0, joint->rotation[2]);

	MulMatrix3x3(my, mz);
	MulMatrix3x3(my, mx);

	position[0] = joint->position[0];
	position[1] = joint->position[1];
	position[2] = - joint->position[2];
	BtTransformSetOrigin(transform, position);

	BtTransformSetBasis(transform, my);
}

void* BaseJointCreateGeneric6DofSpringConstraint(BASE_JOINT* joint)
{
	const float basis[9] = IDENTITY_MATRIX3x3;
	void *transform;
	uint8 transform_a[TRANSFORM_SIZE];
	uint8 transform_b[TRANSFORM_SIZE];

	transform = BtTransformNew(basis);
	BaseJointGetWorldTransform(joint, transform);

	BtRigidBodyGetCenterOfMassTransform(joint->rigid_body1, transform_a);
	BtTransformInverse(transform_a);
	BtTransformMulti(transform_a, transform, transform_a);

	BtRigidBodyGetCenterOfMassTransform(joint->rigid_body2, transform_b);
	BtTransformInverse(transform_b);
	BtTransformMulti(transform_b, transform, transform_b);

	if(joint->constraint_ptr != NULL)
	{
		DeleteBtGeneric6DofSpringConstraint(joint->constraint_ptr);
	}
	joint->constraint_ptr = BtGeneric6DofSpringConstraintNew(
		joint->rigid_body1, joint->rigid_body2, transform_a, transform_b, TRUE);

	BtGeneric6DofConstraintSetLinearLimitUpper(
		joint->constraint_ptr, joint->position_upper_limit[0], joint->position_upper_limit[1], - joint->position_lower_limit[2]);
	BtGeneric6DofConstraintSetLinearLimitLower(
		joint->constraint_ptr, joint->position_lower_limit[0], joint->position_lower_limit[1], - joint->position_upper_limit[2]);
	BtGeneric6DofConstraintSetAngularLimitUpper(
		joint->constraint_ptr, - joint->rotation_lower_limit[0], - joint->rotation_lower_limit[1], joint->rotation_upper_limit[2]);
	BtGeneric6DofConstraintSetAngularLimitLower(
		joint->constraint_ptr, - joint->rotation_upper_limit[0], - joint->rotation_upper_limit[1], joint->rotation_lower_limit[2]);

	DeleteBtTransform(transform);

	return joint->constraint_ptr;
}

void* BaseJointCreateConstraint(BASE_JOINT* joint)
{
	void *ret;
	int index;
	int i;

	switch(joint->type)
	{
	case JOINT_TYPE_GENERIC_6DOF_SPRING_CONSTRAINT:
		{
			float scalar_value;
			void *constraint = BaseJointCreateGeneric6DofSpringConstraint(joint);

			for(i=0; i<3; i++)
			{
				scalar_value = joint->position_stiffness[i];
				if(!FuzzyZero(scalar_value))
				{
					BtGeneric6DofSpringConstraintEnableSpring(constraint, i, TRUE);
					BtGeneric6DofSpringConstraintSetStiffness(constraint, i, scalar_value);
					BtGeneric6DofSpringConstraintSetDamping(constraint, i, BASE_JOINT_DEFAULT_DAMPING);
				}
			}

			for(i=0; i<3; i++)
			{
				scalar_value = joint->rotation_stiffness[i];
				if(!FuzzyZero(scalar_value))
				{
					index = i + 3;
					BtGeneric6DofSpringConstraintEnableSpring(constraint, index, TRUE);
					BtGeneric6DofSpringConstraintSetStiffness(constraint, index, scalar_value);
					BtGeneric6DofSpringConstraintSetDamping(constraint, index, BASE_JOINT_DEFAULT_DAMPING);
				}
			}

			BtGeneric6DofSpringConstraintSetEquilibriumPoint(constraint);
			ret = constraint;
		}
		break;
	case JOINT_TYPE_GENERIC_6DOF_CONSTRAINT:
		ret = BaseJointCreateGeneric6DofSpringConstraint(joint);
		break;
	case JOINT_TYPE_POINT2POINT_CONSTRAINT:
		{
			const float zero_vector[3] = {0, 0, 0};
			joint->constraint_ptr = BtPoint2PointConstraintNew(
				joint->rigid_body1, joint->rigid_body2, zero_vector, zero_vector);
			ret = joint->constraint_ptr;
		}
		break;
	case JOINT_TYPE_CONE_TWIST_CONSTRAINT:
		{
			void *world_transform;
			uint8 transform_a[TRANSFORM_SIZE];
			uint8 transform_b[TRANSFORM_SIZE];
			const float basis[9] = {0};
			int enable_motor;

			world_transform = BtTransformNew(basis);
			BaseJointGetWorldTransform(joint, world_transform);

			BtRigidBodyGetCenterOfMassTransform(joint->rigid_body1, transform_a);
			BtTransformInverse(transform_a);
			BtTransformMulti(transform_a, world_transform, transform_a);

			BtRigidBodyGetCenterOfMassTransform(joint->rigid_body2, transform_b);
			BtTransformInverse(transform_b);
			BtTransformMulti(transform_b, world_transform, transform_b);

			joint->constraint_ptr = BtConeTwistConstraintNew(
				joint->rigid_body1, joint->rigid_body2, transform_a, transform_b);
			BtConeTwistConstraintSetLimit(joint->constraint_ptr,
				joint->rotation_lower_limit[0], joint->rotation_lower_limit[1], joint->rotation_lower_limit[2],
					joint->position_stiffness[0], joint->position_stiffness[1], joint->position_stiffness[2]);
			BtConeTwistConstraintSetDamping(joint->constraint_ptr, joint->position_lower_limit[0]);
			BtConeTwistConstraintSetFixThreshold(joint->constraint_ptr, joint->position_upper_limit[0]);
			enable_motor = FuzzyZero(joint->position_lower_limit[2]);
			BtConeTwistConstraintEnableMotor(joint->constraint_ptr, enable_motor);
			if(enable_motor)
			{
				float rotation[4];

				BtConeTwistConstraintSetMaxMotorImpulse(joint->constraint_ptr, joint->position_upper_limit[2]);
				QuaternionSetEulerZYX(rotation, - joint->rotation_stiffness[0], - joint->rotation_stiffness[1], joint->rotation_stiffness[2]);
				BtConeTwistConstraintSetMotorTargetInConstraintSpace(
					joint->constraint_ptr, rotation);
			}

			DeleteBtTransform(world_transform);

			ret = joint->constraint_ptr;
		}
		break;
	case JOINT_TYPE_SLIDER_CONSTRAINT:
		{
			const float basis[9] = {0};
			void *world_transform;
			uint8 frame_in_a[TRANSFORM_SIZE];
			uint8 frame_in_b[TRANSFORM_SIZE];
			int enable_powered_motor;

			world_transform = BtTransformNew(basis);

			BaseJointGetWorldTransform(joint, world_transform);

			BtRigidBodyGetCenterOfMassTransform(joint->rigid_body1, frame_in_a);
			BtTransformInverse(frame_in_a);
			BtTransformMulti(frame_in_a, world_transform, frame_in_a);

			BtRigidBodyGetCenterOfMassTransform(joint->rigid_body2, frame_in_b);
			BtTransformInverse(frame_in_b);
			BtTransformMulti(frame_in_b, world_transform, frame_in_b);

			joint->constraint_ptr = BtSliderConstraintNew(
				joint->rigid_body1, joint->rigid_body2, frame_in_a, frame_in_b, TRUE);
			BtSliderConstraintSetLowerLinearLimit(joint->constraint_ptr, joint->position_lower_limit[0]);
			BtSliderConstraintSetUpperLinearLimit(joint->constraint_ptr, joint->position_upper_limit[0]);
			BtSliderConstraintSetLowerAngularLimit(joint->constraint_ptr, joint->rotation_lower_limit[0]);
			BtSliderConstraintSetUpperAngularLimit(joint->constraint_ptr, joint->rotation_upper_limit[0]);

			enable_powered_motor = FuzzyZero(joint->position_stiffness[0]);
			BtSliderConstraintSetPoweredLinearMotor(joint->constraint_ptr, enable_powered_motor);
			if(enable_powered_motor)
			{
				BtSliderConstraintSetTargetLinearMotorVelocity(joint->constraint_ptr, joint->position_stiffness[1]);
				BtSliderConstraintSetMaxLinearMotorForce(joint->constraint_ptr, joint->position_stiffness[2]);
			}

			enable_powered_motor = FuzzyZero(joint->rotation_stiffness[0]);
			BtSliderConstraintSetPoweredAngluarMotor(joint->constraint_ptr, enable_powered_motor);
			if(enable_powered_motor)
			{
				BtSliderConstraintSetTargetAngluarMotorVelocity(joint->constraint_ptr, joint->rotation_stiffness[1]);
				BtSliderConstraintSetMaxAngluarMotorForce(joint->constraint_ptr, joint->rotation_stiffness[2]);
			}

			DeleteBtTransform(world_transform);

			ret = joint->constraint_ptr;
		}
		break;
	case JOINT_TYPE_HINGE_CONSTRAINT:
		{
			void *world_transform;
			uint8 transform_a[TRANSFORM_SIZE];
			uint8 transform_b[TRANSFORM_SIZE];
			const float basis[9] = {0};
			int enable_motor;

			world_transform = BtTransformNew(basis);
			BaseJointGetWorldTransform(joint, world_transform);

			BtRigidBodyGetCenterOfMassTransform(joint->rigid_body1, transform_a);
			BtTransformInverse(transform_a);
			BtTransformMulti(transform_a, world_transform, transform_a);

			BtRigidBodyGetCenterOfMassTransform(joint->rigid_body2, transform_b);
			BtTransformInverse(transform_b);
			BtTransformMulti(transform_b, world_transform, transform_b);

			joint->constraint_ptr = BtHingeConstraintNew(joint->rigid_body1, joint->rigid_body2, transform_a, transform_b);
			BtHingeConstraintSetLimit(joint->constraint_ptr, joint->rotation_lower_limit[0], joint->rotation_upper_limit[1],
				joint->position_stiffness[0], joint->position_stiffness[1], joint->position_stiffness[2]);

			enable_motor = FuzzyZero(joint->rotation_stiffness[2]);
			BtHingeConstraintEnableMotor(joint->constraint_ptr, enable_motor);
			if(enable_motor)
			{
				BtHingeConstraintEnableAngularMotor(joint->constraint_ptr, enable_motor,
					joint->rotation_stiffness[1], joint->rotation_stiffness[2]);
			}

			DeleteBtTransform(world_transform);

			ret = joint->constraint;
		}
		break;
	default:
		ret = NULL;
	}

	if(ret != NULL)
	{
		BtTypedConstraintSetUserConstraintPointer(ret, (void*)joint);
	}

	return ret;
}

void BuildBaseJoint(BASE_JOINT* joint, int index)
{
	if(joint->rigid_body1 != NULL && joint->rigid_body2 != NULL)
	{
		if(joint->constraint != NULL)
		{
			DeleteBtTypedConstraint(joint->constraint);
		}
		joint->constraint = BaseJointCreateConstraint(joint);
		BtRigidBodyAddConstraint(joint->rigid_body1, joint->constraint);
		BtRigidBodyAddConstraint(joint->rigid_body2, joint->constraint);
		joint->constraint_ptr = NULL;
	}

	joint->index = index;
}

void BaseJointJoinWorld(BASE_JOINT* joint, void* world)
{
	BtDynamicsWorldAddConstraint(world, joint->constraint);
	BtTypedConstraintSetUserConstraintPointer(joint->constraint, joint);
}

void BaseJointLeaveWorld(BASE_JOINT* joint, void* world)
{
	BtDynamicsWorldRemoveConstraint(world, joint->constraint);
	BtTypedConstraintSetUserConstraintPointer(joint->constraint, NULL);
}

void BaseJointUpdateTransform(BASE_JOINT* joint)
{
	switch(BtTypedConstraintGetType(joint->constraint))
	{
	case CONSTRAINT_TYPE_D6_SPRING:
		BtGeneric6DofSpringConstraintCalculateTransforms(joint->constraint);
		BtGeneric6DofSpringConstraintSetEquilibriumPoint(joint->constraint);
		break;
	}
}

#define PMX_JOINT_UNIT_SIZE 96
typedef struct _PMX_JOINT_UNIT
{
	float position[3];
	float rotation[3];
	float position_lower_limit[3];
	float position_upper_limit[3];
	float rotation_lower_limit[3];
	float rotation_upper_limit[3];
	float position_stiffness[3];
	float rotation_stiffness[3];
} PMX_JOINT_UNIT;

void InitializePmxJoint(PMX_JOINT* joint, PMX_MODEL* model)
{
	InitializeBaseJoint(joint, (void*)model);
}

int PmxJointPreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, rest, 1};
	int rigid_body_index_size = (int)info->rigid_body_index_size * 2;
	char *name_ptr;
	int length;
	int32 num_joints;
	uint8 type;
	int i;

	if(MemRead(&num_joints, sizeof(num_joints), 1, &stream) == 0)
	{
		return FALSE;
	}

	info->joints = &data[stream.data_point];
	for(i=0; i<num_joints; i++)
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

		type = data[stream.data_point];
		stream.data_point++;
		if(type != JOINT_TYPE_GENERIC_6DOF_SPRING_CONSTRAINT && info->version < 2.1f)
		{
			return FALSE;
		}

		switch(type)
		{
		case JOINT_TYPE_GENERIC_6DOF_SPRING_CONSTRAINT:
		case JOINT_TYPE_GENERIC_6DOF_CONSTRAINT:
		case JOINT_TYPE_POINT2POINT_CONSTRAINT:
		case JOINT_TYPE_CONE_TWIST_CONSTRAINT:
		case JOINT_TYPE_SLIDER_CONSTRAINT:
		case JOINT_TYPE_HINGE_CONSTRAINT:
			if(MemSeek(&stream, rigid_body_index_size + PMX_JOINT_UNIT_SIZE, SEEK_CUR) != 0)
			{
				return FALSE;
			}
			break;
		default:
			return FALSE;
		}
	}

	info->joints_count = num_joints;
	return TRUE;
}

int LoadPmxJoints(STRUCT_ARRAY* joints, STRUCT_ARRAY* bodies)
{
	const int num_joints = (int)joints->num_data;
	const int num_rigid_bodies = (int)bodies->num_data;
	PMX_JOINT *jnt = (PMX_JOINT*)joints->buffer;
	PMX_RIGID_BODY *r = (PMX_RIGID_BODY*)bodies->buffer;
	int i;

	for(i=0; i<num_joints; i++)
	{
		PMX_JOINT *joint = &jnt[i];
		const int rigid_body_index1 = joint->rigid_body1_index;
		const int rigid_body_index2 = joint->rigid_body2_index;

		if(rigid_body_index1 >= 0)
		{
			if(rigid_body_index1 < num_rigid_bodies)
			{
				joint->rigid_body1 = (void*)(&r[rigid_body_index1])->body;
			}
			else
			{
				return FALSE;
			}
		}

		if(rigid_body_index2 >= 0)
		{
			if(rigid_body_index2 < num_rigid_bodies)
			{
				joint->rigid_body2 = (void*)(&r[rigid_body_index2])->body;
			}
			else
			{
				return FALSE;
			}
		}

		BuildBaseJoint(joint, i);
	}

	return TRUE;
}

void PmxJointRead(PMX_JOINT* joint, uint8* data, PMX_DATA_INFO* info, size_t* data_size)
{
	PMX_JOINT_UNIT unit;
	MEMORY_STREAM stream = {data, 0, INT_MAX, 1};
	char *name_ptr;
	int length;
	TEXT_ENCODE *encode = info->encoding;
	uint8 type;

	// “ú–{Œê–¼
	length = GetTextFromStream((char*)data, &name_ptr);
	joint->name = EncodeText(encode, name_ptr, length);
	stream.data_point = sizeof(int32) + length;

	// ‰pŒê–¼
	length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr);
	joint->english_name = EncodeText(encode, name_ptr, length);
	stream.data_point += sizeof(int32) + length;

	type = data[stream.data_point];
	stream.data_point++;
	joint->type = (eJOINT_TYPE)type;

	joint->rigid_body1_index = GetSignedValue(&data[stream.data_point], (int)info->rigid_body_index_size);
	stream.data_point += info->rigid_body_index_size;
	joint->rigid_body2_index = GetSignedValue(&data[stream.data_point], (int)info->rigid_body_index_size);
	stream.data_point += info->rigid_body_index_size;

	(void)MemRead(unit.position, sizeof(unit.position), 1, &stream);
	(void)MemRead(unit.rotation, sizeof(unit.rotation), 1, &stream);
	(void)MemRead(unit.position_lower_limit, sizeof(unit.position_lower_limit), 1, &stream);
	(void)MemRead(unit.position_upper_limit, sizeof(unit.position_upper_limit), 1, &stream);
	(void)MemRead(unit.rotation_lower_limit, sizeof(unit.rotation_lower_limit), 1, &stream);
	(void)MemRead(unit.rotation_upper_limit, sizeof(unit.rotation_upper_limit), 1, &stream);
	(void)MemRead(unit.position_stiffness, sizeof(unit.position_stiffness), 1, &stream);
	(void)MemRead(unit.rotation_stiffness, sizeof(unit.rotation_stiffness), 1, &stream);

	COPY_VECTOR3(joint->position, unit.position);
	COPY_VECTOR3(joint->rotation, unit.rotation);
	COPY_VECTOR3(joint->position_lower_limit, unit.position_lower_limit);
	COPY_VECTOR3(joint->position_upper_limit, unit.position_upper_limit);
	COPY_VECTOR3(joint->rotation_lower_limit, unit.rotation_lower_limit);
	COPY_VECTOR3(joint->rotation_upper_limit, unit.rotation_upper_limit);
	COPY_VECTOR3(joint->position_stiffness, unit.position_stiffness);
	COPY_VECTOR3(joint->rotation_stiffness, unit.rotation_stiffness);

	*data_size = stream.data_point;
}

#define PMD2_JOINT_UNIT_SIZE 124
typedef struct _PMD2_JONIT_UNIT
{
	uint8 name[PMD_JOINT_NAME_SIZE];
	int32 body_id_a;
	int32 body_id_b;
	float position[3];
	float rotation[3];
	float position_lower_limit[3];
	float position_upper_limit[3];
	float rotation_lower_limit[3];
	float rotation_upper_limit[3];
	float position_stiffness[3];
	float rotation_stiffness[3];
} PMD2_JOINT_UNIT;

void InitializePmd2Joint(PMD2_JOINT* joint, PMD2_MODEL* model)
{
	InitializeBaseJoint(joint, (void*)model);
}

int Pmd2JointPreparse(MEMORY_STREAM_PTR stream, PMD_DATA_INFO* info)
{
	int32 size;
	if(MemRead(&size, sizeof(size), 1, stream) == 0
		|| size * PMD2_JOINT_UNIT_SIZE > stream->data_size - stream->data_point)
	{
		return FALSE;
	}
	info->joints_count = size;
	info->joints = &stream->buff_ptr[stream->data_point];
	(void)MemSeek(stream, size * PMD2_JOINT_UNIT_SIZE, SEEK_CUR);
	return TRUE;
}

int LoadPmd2Joints(STRUCT_ARRAY* joints, STRUCT_ARRAY* rigid_bodies)
{
	PMD2_JOINT *pmd2_joints = (PMD2_JOINT*)joints->buffer;
	const int num_joints = (int)joints->num_data;
	PMD2_JOINT *joint;
	PMD2_RIGID_BODY *bodies = (PMD2_RIGID_BODY*)rigid_bodies->buffer;
	const int num_bodies = (int)rigid_bodies->num_data;
	int i;
	for(i=0; i<num_joints; i++)
	{
		joint = &pmd2_joints[i];
		if(joint->rigid_body1_index >= 0)
		{
			if(joint->rigid_body1_index >= num_bodies)
			{
				return FALSE;
			}
			else
			{
				joint->rigid_body1 = bodies[joint->rigid_body1_index].body;
			}
		}
		if(joint->rigid_body2_index >= 0)
		{
			if(joint->rigid_body2_index >= num_bodies)
			{
				return FALSE;
			}
			else
			{
				joint->rigid_body2 = bodies[joint->rigid_body2_index].body;
			}
		}
		BuildBaseJoint(joint, i);
	}
	return TRUE;
}

void ReadPmd2Joint(PMD2_JOINT* joint, MEMORY_STREAM_PTR stream, size_t* data_size, PMD_DATA_INFO* info)
{
	PMD2_JOINT_UNIT unit;
	(void)MemRead(unit.name, sizeof(unit.name), 1, stream);
	(void)MemRead(&unit.body_id_a, sizeof(unit.body_id_a), 1, stream);
	(void)MemRead(&unit.body_id_b, sizeof(unit.body_id_b), 1, stream);
	(void)MemRead(unit.position, sizeof(unit.position), 1, stream);
	(void)MemRead(unit.rotation, sizeof(unit.rotation), 1, stream);
	(void)MemRead(unit.position_lower_limit, sizeof(unit.position_lower_limit), 1, stream);
	(void)MemRead(unit.position_upper_limit, sizeof(unit.position_upper_limit), 1, stream);
	(void)MemRead(unit.rotation_lower_limit, sizeof(unit.rotation_lower_limit), 1, stream);
	(void)MemRead(unit.rotation_upper_limit, sizeof(unit.rotation_upper_limit), 1, stream);
	(void)MemRead(unit.position_stiffness, sizeof(unit.position_stiffness), 1, stream);
	(void)MemRead(unit.rotation_stiffness, sizeof(unit.rotation_stiffness), 1, stream);
	joint->rigid_body1_index = unit.body_id_a;
	joint->rigid_body2_index = unit.body_id_b;
	COPY_VECTOR3(joint->position, unit.position);
	COPY_VECTOR3(joint->rotation, unit.rotation);
	COPY_VECTOR3(joint->position_lower_limit, unit.position_lower_limit);
	COPY_VECTOR3(joint->position_upper_limit, unit.position_upper_limit);
	COPY_VECTOR3(joint->rotation_lower_limit, unit.rotation_lower_limit);
	COPY_VECTOR3(joint->rotation_upper_limit, unit.rotation_upper_limit);
	COPY_VECTOR3(joint->position_stiffness, unit.position_stiffness);
	COPY_VECTOR3(joint->rotation_stiffness, unit.rotation_stiffness);
	*data_size = PMD2_JOINT_UNIT_SIZE;
}
