#ifndef _INCLUDED_BULLET_H_
#define _INCLUDED_BULLET_H_

#include "types.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eCOLLISION_SHAPE_TYPE
{
	COLLISION_SHAPE_TYPE_UNKNOWN = -1,
	COLLISION_SHAPE_TYPE_SPHERE,
	COLLISION_SHAPE_TYPE_BOX,
	COLLISION_SHAPE_TYPE_CAPSULE
} eCOLLISION_SHAPE_TYPE;

typedef enum _eCOLLISION_ACTIVATE_STATE
{
	COLLISION_ACTIVATE_STATE_ACTIVE_TAG = 1,
	COLLISION_ACTIVATE_STATE_ISLAND_SLEEPING = 2,
	COLLISION_ACTIVATE_STATE_WANTS_DEACTIVATION = 3,
	COLLISION_ACTIVATE_STATE_DISABLE_DEACTIVATION = 4,
	COLLISION_ACTIVATE_STATE_DISABLE_SIMULATION = 5,
} eCOLLISION_ACTIVATE_STATE;

typedef enum _eCOLLISION_FLAGS
{
	COLLISION_FLAG_STATIC_OBJECT = 0x01,
	COLLISION_FLAG_KINEMATIC_OBJECT = 0x02,
	COLLISION_FLAG_NO_CONTACT_RESPONSE = 0x04,
	COLLISION_FLAG_CUSTUM_MATERIAL_CALLBACK = 0x08,
	COLLISION_FLAG_CHARACTER_OBJECT = 0x10,
	COLLISION_FLAG_DISABLE_VISUALIZE_OBJECT = 0x20,
	COLLISION_FLAG_DISABLE_SPU_COLLISION_PROCESSING = 0x40
} eCOLLISION_FLAGS;

typedef enum _eCONSTRAINT_TYPE
{
	CONSTRAINT_TYPE_POINT2POINT = 3,
	CONSTRAINT_TYPE_HINGE,
	CONSTRAINT_TYPE_CONETWIST,
	CONSTRAINT_TYPE_D6,
	CONSTRAINT_TYPE_SLIDER,
	CONSTRAINT_TYPE_CONTACT,
	CONSTRAINT_TYPE_D6_SPRING,
	CONSTRAINT_TYPE_GEAR_CONSTRAINT,
	CONSTRAINT_TYPE_FIXED_CONSTRAINT
} eCONSTRAINT_TYPE;

typedef enum _eRAY_TEST_CALLBACK_FLAGS
{
	RAY_TEST_FLAG_UUSE_SUB_SIMPLEX_CONVEX_CAST = 1 << 2
} eRAY_TEST_CALLBACK_FLAGS;

extern void* BtDynamicsWorldNew(const float* aa_bb_size);

extern void DeleteBtDynamicsWorld(void* world);

extern void BtDynamicsWorldStepSimulation(void* world, float time_step, int max_sub_step, float fixed_time_step);

extern void BtDynamicsWorldStepSimulationSimple(void* world, float time_step);

extern void BtDynamicsWorldSetGravity(void* world, const float* gravity);

extern void BtDynamicsWorldRayTest(void* world, float* from, float* to, void* callback);

extern void BtDynamicsWorldAddRigidBody(void* world, void* body);

extern void BtDynamicsWorldAddRigidBodyWithDetailData(void* world, void* body, short group, short mask);

extern void BtDynamicsWorldRemoveRigidBody(void* world, void* body);

extern void BtDynamicsWorldAddConstraint(void* world, void* constraint);

extern void BtDynamicsWorldRemoveConstraint(void* world, void* constraint);

extern void** BtDynamicsWorldGetCollisionObjects(void* world, int* num_objects);

extern void BtDynamicsWorldRemoveCollisionObject(void* world, void* object);

extern void* BtDynamicsWorldGetPairCache(void* world);

extern void* BtDynamicsWorldGetDispatcher(void* world);

extern void BtOverlappingPairCacheCleanProxyFromPairs(void* cache, void* broadphase_handle, void* dispatcher);

extern void* BtClosestRayResultCallbackNew(float* from, float* to);

extern void DeleteBtClosestRayResultCallback(void* callback);

extern int BtClosestRayResultCallbackHasHit(void* callback);

extern void BtClosestRayResultCallbackGetHitPointWorld(void* callback, float* point);

extern void BtClosestRayCallbackSetFlags(void* callback, unsigned int flags);

extern unsigned int BtClosestRayCallbackGetFlags(void* callback);

extern void* BtClosestRayResultCallbackGetCollisionObject(void* callback);

extern void* BtDebugDrawNew(
	void *draw_data,
	void (*draw_line)(void* data, const float* from, const float* to, float* color),
	void (*draw_sphere)(void* data, float* point, float radius, float* color)
);

extern void DeleteBtDebugDraw(void* draw);

extern void SetBtWorldDebugDrawer(void* world, void* draw);

extern void BtDebugDrawSphere(void* draw, float* position, float radius, const float* color);

extern void BtDebugDrawWorld(void* world);

extern void SetBtDebugMode(void* draw, int mode);

extern void* BtTransformNew(const float* matrix);

extern void* BtTransformNewWithVector(const float* matrix, const float* vector);

extern void* BtTransformNewWithQuaternion(const float* quaternion, const float* vector);

extern void DeleteBtTransform(void* transform);

extern void* BtTransformCopy(void* transform);

extern void BtTransformSet(void* dst, void* src);

extern void BtTransformInverse(void* transform);

extern void BtTransformGetOpenGLMatrix(void* transform, float* matrix);

extern void BtTransformSetIdentity(void* transform);

extern void BtTransformSetRotation(void* transform, const float* rotation);

extern void BtTransformSetOrigin(void* transform, const float* origin);

extern void BtTransformGetOrigin(void* transform, float* origin);

extern void BtTransformGetRotation(void* transform, float* rotation);

extern void BtTransformGetBasis(void* transform, float* basis);

extern void BtTransformMulti(void* transform1, void* transform2, void* result);

extern void BtTransformMultiVector3(void* transform, const float* vector, float* ret);

extern void BtTransformMultiQuaternion(void* transform, const float* quaternion, float* ret);

extern void BtTransformSetBasis(void* transform, const float* basis);

extern void* BtCollisionShapeNew(eCOLLISION_SHAPE_TYPE type, float* size);

extern void DeleteBtCollisionShape(void* shape);

extern void BtCollisionShapeCalculateLocalInertia(void* shape, const float mass, float* inertia);

extern void* BtMotionStateNew(
	void* user_data,
	void (*get_world_transform)(void* setup_transform, void* user_data),
	void (*set_world_transform)(const void* world_transform, void* user_data)
);
extern void DeleteBtMotionState(void* state);
extern void BtMotionStateSetWorldTransform(void* state, void* transform);

extern void* BtRigidBodyNew(float mass, void* motion_state, void* shape, const float* local_interia);

extern void* BtRigidBodyNewWithDetailData(
	float mass,
	void* motion_state,
	void* shape,
	float* local_interia,
	float linear_damping,
	float angular_damping,
	float restitution,
	float friction,
	int additional_damping
);

extern void DeleteBtRigidBody(void* body);

extern void BtRigidBodySetActivationState(void* body, int state);

extern void BtRigidBodySetUserData(void* body, void* user_data);

extern void* BtRigidBodyGetUserData(void* body);

extern int BtRigidBodyGetCollisionFlags(void* body);

extern void BtRigidBodySetCollisionFlags(void* body, int flags);

extern void BtRigidBodyGetCenterOfMassTransform(void* rigid_body, void* transform);

extern void BtRigidBodySetCenterOfMassTransform(void* rigid_body, void* transform);

extern void BtRigidBodyAddConstraint(void* rigid_body, void* constraint);
extern void BtRigidBodyRemoveConstraint(void* rigid_body, void* constraint);

extern void* BtRigidBodyUpcast(void* object);

extern void BtRigidBodySetWorldTransform(void* rigid_body, void* transform);

extern void BtRigidBodySetInterpolationWorldTransform(void* rigid_body, void* transform);

extern void BtRigidBodySetMotionState(void* rigid_body, void* motion_state);

extern void* BtRigidBodyGetBroadphaseHandle(void* rigid_body);

extern void BtRigidBodyApplyForce(void* rigid_body, const float* force, const float* relative_position);

extern void BtRigidBodyClearForces(void* rigid_body);

extern void* BtGeneric6DofSpringConstraintNew(
	void* rigid_bodyA,
	void* rigid_bodyB,
	void* transformA,
	void* transformB,
	int use_linear_reference_frameA
);

extern void DeleteBtGeneric6DofSpringConstraint(void* constraint);

extern void BtGeneric6DofConstraintSetLinearLimitUpper(
	void* constraint,
	float x,
	float y,
	float z
);

extern void BtGeneric6DofConstraintSetLinearLimitLower(
	void* constraint,
	float x,
	float y,
	float z
);

extern void BtGeneric6DofConstraintSetAngularLimitUpper(
	void* constraint,
	float x,
	float y,
	float z
);

extern void BtGeneric6DofConstraintSetAngularLimitLower(
	void* constraint,
	float x,
	float y,
	float z
);

extern void BtGeneric6DofSpringConstraintEnableSpring(void* constraint, int index, int is_on);

extern void BtGeneric6DofSpringConstraintSetStiffness(void* constraint, int index, float stiffness);

extern void BtGeneric6DofSpringConstraintSetDamping(void* constraint, int index, float damping);

extern void BtGeneric6DofSpringConstraintSetEquilibriumPoint(void* constraint);

extern void BtGeneric6DofSpringConstraintCalculateTransforms(void* constraint);

extern void* BtPoint2PointConstraintNew(
	void* rigid_body_a,
	void* rigid_body_b,
	const float* pivot_a,
	const float* pivot_b
);

extern void* BtConeTwistConstraintNew(
	void* rigid_body_a,
	void* rigid_body_b,
	void* transform_a,
	void* transform_b
);

extern void BtConeTwistConstraintSetLimit(
	void* constraint,
	float lower_x,
	float lower_y,
	float lower_z,
	float stiffness_x,
	float stiffness_y,
	float stiffness_z
);

extern void BtConeTwistConstraintSetDamping(void* constraint, float damping);

extern void BtConeTwistConstraintSetFixThreshold(void* constraint, float threshold);

extern void BtConeTwistConstraintEnableMotor(void* constraint, int is_enable);

extern void BtConeTwistConstraintSetMaxMotorImpulse(void* constraint, float impulse);

extern void BtConeTwistConstraintSetMotorTargetInConstraintSpace(void* constraint, const float* quaternion);

extern void* BtSliderConstraintNew(
	void* rigid_body_a,
	void* rigid_body_b,
	void* transform_a,
	void* transform_b,
	int use_linear_reference_frame
);

extern void BtSliderConstraintSetUpperLinearLimit(void* constraint, float limit);
extern void BtSliderConstraintSetLowerLinearLimit(void* constraint, float limit);
extern void BtSliderConstraintSetUpperAngularLimit(void* constraint, float limit);
extern void BtSliderConstraintSetLowerAngularLimit(void* constraint, float limit);
extern void BtSliderConstraintSetPoweredLinearMotor(void* constraint, int is_on);
extern void BtSliderConstraintSetTargetLinearMotorVelocity(void* constraint, float velocity);
extern void BtSliderConstraintSetMaxLinearMotorForce(void* constraint, float force);
extern void BtSliderConstraintSetPoweredAngluarMotor(void* constraint, int is_on);
extern void BtSliderConstraintSetTargetAngluarMotorVelocity(void* constraint, float velocity);
extern void BtSliderConstraintSetMaxAngluarMotorForce(void* constraint, float force);

extern void* BtHingeConstraintNew(
	void* rigid_body_a,
	void* rigid_body_b,
	void* transform_a,
	void* transform_b
);
extern void BtHingeConstraintSetLimit(
	void* constraint,
	float rotation_lower,
	float rotation_upper,
	float softness,
	float bias,
	float relaxation
);
extern void BtHingeConstraintEnableMotor(void* constraint, int is_on);
extern void BtHingeConstraintEnableAngularMotor(void* constraint, int is_on, float target_velocity, float max_motor_impulse);

extern void* BtDefaultMotionStateNew(void);
extern void DeleteBtDefaultMotionState(void* state);

extern void DeleteBtTypedConstraint(void* constraint);
extern int BtTypedConstraintGetType(void* constraint);
extern void BtTypedConstraintSetUserConstraintPointer(void* constraint, void* user_constraint);

extern void* BtBoxShapeNew(float* vector);
extern void DeleteBtBoxShape(void* shape);

extern void* BtTriangleMeshNew(void);
extern void DeleteBtTriangleMesh(void* mesh);
extern void BtTriangleMeshAddTriangle(
	void* mesh,
	float position1[3],
	float position2[3],
	float position3[3]
);

extern void* BtBvhTriangleMeshShapeNew(void* triangle_mesh, int use_aa_bb_compression);
extern void DeleteBtBvhTriangleMeshShape(void* shape);

#ifdef __cplusplus
};
#endif

#endif	// #ifndef _INCLUDED_BULLET_H_
