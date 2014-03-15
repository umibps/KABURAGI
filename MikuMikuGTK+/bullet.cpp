#include <btBulletDynamicsCommon.h>
#include <gtk/gtk.h>
#include "bullet.h"
#include "types.h"
#include "utils.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

class BT_DEBUG_DRAW : public btIDebugDraw
{
public:
	BT_DEBUG_DRAW(
		void *draw_data,
		void (*draw_line)(void* data, const float*, const float*, float* color),
		void (*draw_sphere)(void* data, float* point, float radius, float* color)
	)
	{
		m_draw_data = draw_data;
		m_draw_line = draw_line;
		m_draw_sphere = draw_sphere;
	}

	void reportErrorWarning(const char *warning_string)
	{
		(void)printf("[ERROR]: %s\n", warning_string);
	}

	void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
	{
		float from_vec[3], to_vec[3];
		float color_vec[3];

		color_vec[0] = color.getX();
		color_vec[1] = color.getY();
		color_vec[2] = color.getZ();

		from_vec[0] = from.getX();
		from_vec[1] = from.getY();
		from_vec[2] = from.getZ();

		to_vec[0] = from.getX();
		to_vec[1] = from.getY();
		to_vec[2] = from.getZ();

		m_draw_line(m_draw_data, from_vec, to_vec, color_vec);
	}

	void drawSphere(const btVector3 &p, btScalar radius, btVector3 &color)
	{
		if(m_draw_sphere != NULL)
		{
			float position[3] = {p.getX(), p.getY(), p.getZ()};
			float float_color[3] = {color.getX(), color.getY(), color.getZ()};

			m_draw_sphere(m_draw_data, position, radius, float_color);
		}
		else
		{
			btIDebugDraw::drawSphere(p, radius, color);
		}
	}

	void drawContactPoint(
		const btVector3 &PointOnB,
		const btVector3 &normalOnB,
		btScalar distance,
		int lifeTime,
		const btVector3 &color
	)
	{
		drawLine(PointOnB, PointOnB + normalOnB * distance, color);
	}

	void draw3dText(const btVector3 &location, const char* text)
	{
		(void)printf("[INFO]: %s\n", text);
	}

	void setDebugMode(int debug_mode)
	{
		m_flags = debug_mode;
	}

	int getDebugMode() const
	{
		return m_flags;
	}

private:
	void (*m_draw_line)(void* data, const float*, const float*, float* color);
	void (*m_draw_sphere)(void* data, float* point, float radius, float* color);
	void *m_draw_data;
	int m_flags;
};

typedef struct _BT_DYNAMICS_WORLD
{
	_BT_DYNAMICS_WORLD(const float* aa_bb_size)
	{
		dispatcher = new btCollisionDispatcher(&config);
		solver = new btSequentialImpulseConstraintSolver();
		if(aa_bb_size != NULL)
		{
			sweep_broadphase = new btAxisSweep3(btVector3(-aa_bb_size[0], -aa_bb_size[1], -aa_bb_size[2]),
				btVector3(aa_bb_size[0], aa_bb_size[1], aa_bb_size[2]));
			world = new btDiscreteDynamicsWorld(dispatcher, sweep_broadphase, solver, &config);
		}
		else
		{
			dbvt_broadphase = new btDbvtBroadphase();
			world = new btDiscreteDynamicsWorld(dispatcher, dbvt_broadphase, solver, &config);
			world->getSolverInfo().m_solverMode &= ~(SOLVER_RANDMIZE_ORDER);
		}
	}

	btDiscreteDynamicsWorld *world;
	btDefaultCollisionConfiguration config;
	btCollisionDispatcher *dispatcher;
	btDbvtBroadphase *dbvt_broadphase;
	btAxisSweep3 *sweep_broadphase;
	btSequentialImpulseConstraintSolver *solver;
} BT_DYNAMICS_WORLD;

void* BtDynamicsWorldNew(const float* aa_bb_size)
{
	BT_DYNAMICS_WORLD *world = new BT_DYNAMICS_WORLD(aa_bb_size);

	return static_cast<void*>(world);
}

void DeleteBtDynamicsWorld(void* world)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);

	delete w->dispatcher;
	delete w->sweep_broadphase;
	delete w->dbvt_broadphase;
	delete w->solver;
	delete w->world;

	delete w;
}

void BtDynamicsWorldStepSimulationSimple(void* world, float time_step)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);

	w->world->stepSimulation(time_step);
}

void BtDynamicsWorldStepSimulation(void* world, float time_step, int max_sub_step, float fixed_time_step)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);

	w->world->stepSimulation(time_step, max_sub_step, fixed_time_step);
}

void BtDynamicsWorldSetGravity(void* world, const float* gravity)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	const btVector3 g(gravity[0], gravity[1], gravity[2]);

	w->world->setGravity(g);
}

void BtDynamicsWorldRayTest(void* world, float* from, float* to, void* callback)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	btCollisionWorld::ClosestRayResultCallback *c =
		static_cast<btCollisionWorld::ClosestRayResultCallback*>(callback);

	w->world->rayTest(btVector3(from[0], from[1], from[2]), btVector3(to[0], to[1], to[2]), *c);
}

void BtDynamicsWorldAddRigidBody(void* world, void* body)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	btRigidBody *b = static_cast<btRigidBody*>(body);

	w->world->addRigidBody(b);
}

void BtDynamicsWorldAddRigidBodyWithDetailData(void* world, void* body, short group, short mask)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	btRigidBody *b = static_cast<btRigidBody*>(body);

	w->world->addRigidBody(b, group, mask);
}

void BtDynamicsWorldRemoveRigidBody(void* world, void* body)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	btRigidBody *b = static_cast<btRigidBody*>(body);

	w->world->removeRigidBody(b);
}

void BtDynamicsWorldAddConstraint(void* world, void* constraint)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	btTypedConstraint *c = static_cast<btTypedConstraint*>(constraint);

	w->world->addConstraint(c);
}

void BtDynamicsWorldRemoveConstraint(void* world, void* constraint)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	btTypedConstraint *c = static_cast<btTypedConstraint*>(constraint);

	w->world->removeConstraint(c);
}

void** BtDynamicsWorldGetCollisionObjects(void* world, int* num_objects)
{
	const void **ret;
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	const btCollisionObjectArray &objects = w->world->getCollisionObjectArray();

	*num_objects = w->world->getNumCollisionObjects();
	ret = (const void**)MEM_ALLOC_FUNC(sizeof(void*)*(*num_objects));
	for(int i= 0; i < *num_objects; i++)
	{
		ret[i] = static_cast<const void*>(objects[i]);
	}

	return (void**)ret;
}

void BtDynamicsWorldRemoveCollisionObject(void* world, void* object)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);

	w->world->removeCollisionObject(static_cast<btCollisionObject*>(object));
}

void* BtDynamicsWorldGetPairCache(void* world)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	return static_cast<void*>(w->world->getPairCache());
}

void* BtDynamicsWorldGetDispatcher(void* world)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	return static_cast<void*>(w->world->getDispatcher());
}

void BtOverlappingPairCacheCleanProxyFromPairs(void* cache, void* broadphase_handle, void* dispatcher)
{
	btOverlappingPairCache *c = static_cast<btOverlappingPairCache*>(cache);
	btBroadphaseProxy *proxy = static_cast<btBroadphaseProxy*>(broadphase_handle);
	btDispatcher *patcher = static_cast<btDispatcher*>(dispatcher);

	c->cleanProxyFromPairs(proxy, patcher);
}

void* BtClosestRayResultCallbackNew(float* from, float* to)
{
	btCollisionWorld::ClosestRayResultCallback *callback =
		new btCollisionWorld::ClosestRayResultCallback(
			btVector3(from[0], from[1], from[2]),
			btVector3(to[0], to[1], to[2])
	);

	return static_cast<void*>(callback);
}

void DeleteBtClosestRayResultCallback(void* callback)
{
	btCollisionWorld::ClosestRayResultCallback *c =
		static_cast<btCollisionWorld::ClosestRayResultCallback*>(callback);

	delete c;
}

int BtClosestRayResultCallbackHasHit(void* callback)
{
	btCollisionWorld::ClosestRayResultCallback *c =
		static_cast<btCollisionWorld::ClosestRayResultCallback*>(callback);

	return (c->hasHit()) ? 1 : 0;
}

void BtClosestRayResultCallbackGetHitPointWorld(void* callback, float* point)
{
	btCollisionWorld::ClosestRayResultCallback *c =
		static_cast<btCollisionWorld::ClosestRayResultCallback*>(callback);
	btVector3 hit = c->m_hitPointWorld;

	point[0] = hit.getX();
	point[1] = hit.getY();
	point[2] = hit.getZ();
}

void BtClosestRayCallbackSetFlags(void* callback, unsigned int flags)
{
	btCollisionWorld::ClosestRayResultCallback *c =
		static_cast<btCollisionWorld::ClosestRayResultCallback*>(callback);

	c->m_flags = flags;
}

unsigned int BtClosestRayCallbackGetFlags(void* callback)
{
	btCollisionWorld::ClosestRayResultCallback *c =
		static_cast<btCollisionWorld::ClosestRayResultCallback*>(callback);

	return c->m_flags;
}

void* BtClosestRayResultCallbackGetCollisionObject(void* callback)
{
	btCollisionWorld::ClosestRayResultCallback *c =
		static_cast<btCollisionWorld::ClosestRayResultCallback*>(callback);

	return (void*)static_cast<const void*>(c->m_collisionObject);
}

void* BtDebugDrawNew(
	void *draw_data,
	void (*draw_line)(void* data, const float* from, const float* to, float* color),
	void (*draw_sphere)(void* data, float* point, float radius, float* color)
)
{
	BT_DEBUG_DRAW *draw = new BT_DEBUG_DRAW(
		draw_data,
		draw_line,
		draw_sphere
	);

	return static_cast<void*>(draw);
}

void DeleteBtDebugDraw(void* draw)
{
	BT_DEBUG_DRAW *d = static_cast<BT_DEBUG_DRAW*>(draw);

	delete d;
}

void SetBtWorldDebugDrawer(void* world, void* draw)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);
	BT_DEBUG_DRAW *d = static_cast<BT_DEBUG_DRAW*>(draw);

	w->world->setDebugDrawer(d);
}

void BtDebugDrawSphere(void* draw, float* position, float radius, const float* color)
{
	BT_DEBUG_DRAW *d = static_cast<BT_DEBUG_DRAW*>(draw);

	d->drawSphere(btVector3(position[0], position[1], position[2]), radius,
		btVector3(color[0], color[1], color[2]));
}

void BtDebugDrawWorld(void* world)
{
	BT_DYNAMICS_WORLD *w = static_cast<BT_DYNAMICS_WORLD*>(world);

	w->world->debugDrawWorld();
}

void SetBtDebugMode(void* draw, int mode)
{
	BT_DEBUG_DRAW *d = static_cast<BT_DEBUG_DRAW*>(draw);

	d->setDebugMode(mode);
}

void* BtTransformNew(const float* matrix)
{
	btTransform *transform = new btTransform(
		btMatrix3x3(
			matrix[0], matrix[1], matrix[2],
			matrix[3], matrix[4], matrix[5],
			matrix[6], matrix[7], matrix[8])
	);

	return static_cast<void*>(transform);
}

void* BtTransformNewWithVector(const float* matrix, const float* vector)
{
	btTransform *transform = new btTransform(
		btMatrix3x3(
			matrix[0], matrix[1], matrix[2],
			matrix[3], matrix[4], matrix[5],
			matrix[6], matrix[7], matrix[8]),
		btVector3(vector[0], vector[1], vector[2])
	);

	return static_cast<void*>(transform);
}

void* BtTransformNewWithQuaternion(const float* quaternion, const float* vector)
{
	btTransform *transform = new btTransform(
		btQuaternion(quaternion[0], quaternion[1], quaternion[2], quaternion[3]),
		btVector3(vector[0], vector[1], vector[2]));

	return static_cast<void*>(transform);
}

void DeleteBtTransform(void* transform)
{
	btTransform *trans = static_cast<btTransform*>(transform);

	delete trans;
}

void* BtTransformCopy(void* transform)
{
	btTransform *trans = static_cast<btTransform*>(transform);
	btTransform *copy = new btTransform(*trans);

	return static_cast<void*>(copy);
}

void BtTransformSet(void* dst, void* src)
{
	btTransform *dst_trans = static_cast<btTransform*>(dst);
	btTransform *src_trans = static_cast<btTransform*>(src);

	*dst_trans = *src_trans;
}

void BtTransformInverse(void* transform)
{
	btTransform *trans = static_cast<btTransform*>(transform);
	*trans = trans->inverse();
}

void BtTransformGetOpenGLMatrix(void* transform, float* matrix)
{
	btTransform *trans = static_cast<btTransform*>(transform);

	trans->getOpenGLMatrix(matrix);
}

void BtTransformSetIdentity(void* transform)
{
	btTransform *trans = static_cast<btTransform*>(transform);

	trans->setIdentity();
}

void BtTransformSetRotation(void* transform, const float* rotation)
{
	btTransform *trans = static_cast<btTransform*>(transform);

	trans->setRotation(btQuaternion(rotation[0], rotation[1], rotation[2], rotation[3]));
}

void BtTransformSetOrigin(void* transform, const float* origin)
{
	btTransform *trans = static_cast<btTransform*>(transform);

	trans->setOrigin(btVector3(origin[0], origin[1], origin[2]));
}

void BtTransformGetOrigin(void* transform, float* origin)
{
	btTransform *trans = static_cast<btTransform*>(transform);
	btVector3 vector = trans->getOrigin();

	origin[0] = vector.getX();
	origin[1] = vector.getY();
	origin[2] = vector.getZ();
}

void BtTransformGetRotation(void* transform, float* rotation)
{
	btTransform *trans = static_cast<btTransform*>(transform);
	btQuaternion quaternion = trans->getRotation();

	rotation[0] = quaternion.getX();
	rotation[1] = quaternion.getY();
	rotation[2] = quaternion.getZ();
	rotation[3] = quaternion.getW();
}

void BtTransformGetBasis(void* transform, float* basis)
{
	btTransform *trans = static_cast<btTransform*>(transform);
	btMatrix3x3 matrix = trans->getBasis();

	btVector3 row = matrix.getRow(0);
	basis[0] = row.getX();
	basis[1] = row.getY();
	basis[2] = row.getZ();
	row = matrix.getRow(1);
	basis[3] = row.getX();
	basis[4] = row.getY();
	basis[5] = row.getZ();
	row = matrix.getRow(2);
	basis[6] = row.getX();
	basis[7] = row.getY();
	basis[8] = row.getZ();
}

void BtTransformMulti(void* transform1, void* transform2, void* result)
{
	btTransform *t1 = static_cast<btTransform*>(transform1);
	btTransform *t2 = static_cast<btTransform*>(transform2);
	btTransform *ret = static_cast<btTransform*>(result);

	*ret = *t1 * *t2;
}

void BtTransformMultiVector3(void* transform, const float* vector, float* ret)
{
	btTransform *trans = static_cast<btTransform*>(transform);

	btVector3 vec = *trans * btVector3(vector[0], vector[1], vector[2]);

	ret[0] = vec.getX();
	ret[1] = vec.getY();
	ret[2] = vec.getZ();
}

void BtTransformSetBasis(void* transform, const float* basis)
{
	btTransform *trans = static_cast<btTransform*>(transform);

	trans->setBasis(btMatrix3x3(
		basis[0], basis[1], basis[2],
		basis[3], basis[4], basis[5],
		basis[6], basis[7], basis[8])
	);
}

void* BtCollisionShapeNew(eCOLLISION_SHAPE_TYPE type, float* size)
{
	switch(type)
	{
	case COLLISION_SHAPE_TYPE_SPHERE:
		return static_cast<void*>(new btSphereShape(size[0]));
	case COLLISION_SHAPE_TYPE_BOX:
		return static_cast<void*>(new btBoxShape(btVector3(size[0], size[1], size[2])));
	case COLLISION_SHAPE_TYPE_CAPSULE:
		return static_cast<void*>(new btCapsuleShape(size[0], size[1]));
	}

	return NULL;
}

void DeleteBtCollisionShape(void* shape)
{
	btCollisionShape *s = static_cast<btCollisionShape*>(shape);
	delete s;
}

void BtCollisionShapeCalculateLocalInertia(void* shape, const float mass, float* inertia)
{
	btCollisionShape *s = static_cast<btCollisionShape*>(shape);
	btVector3 local_inertia;
	s->calculateLocalInertia(mass, local_inertia);
	inertia[0] = local_inertia.getX();
	inertia[1] = local_inertia.getY();
	inertia[2] = local_inertia.getZ();
}

class BT_MOTION_STATE : public btMotionState
{
public:
	BT_MOTION_STATE(
		void* user_data,
		void (*get_world_transform)(void* setup_transform, void* user_data),
		void (*set_world_transform)(const void* world_transform, void* user_data)
	)
	{
		m_user_data = user_data;
		m_get_world_transform = get_world_transform;
		m_set_world_transform = set_world_transform;
	}

	void getWorldTransform(btTransform& worldTrans) const
	{
		m_get_world_transform(static_cast<void*>(&worldTrans), m_user_data);
	}

	void setWorldTransform(const btTransform& worldTrans)
	{
		const btTransform *trans = &worldTrans;
		m_set_world_transform(static_cast<const void*>(trans), m_user_data);
	}

private:
	void *m_user_data;
	void (*m_get_world_transform)(void* setup_transform, void* user_data);
	void (*m_set_world_transform)(const void* world_transform, void* user_data);
};

void* BtMotionStateNew(
	void* user_data,
	void (*get_world_transform)(void* setup_transform, void* user_data),
	void (*set_world_transform)(const void* world_transform, void* user_data)
)
{
	BT_MOTION_STATE *state = new BT_MOTION_STATE(
		user_data, get_world_transform, set_world_transform);
	return static_cast<void*>(state);
}

void DeleteBtMotionState(void* state)
{
	BT_MOTION_STATE *s = static_cast<BT_MOTION_STATE*>(state);
	delete s;
}

void BtMotionStateSetWorldTransform(void* state, void* transform)
{
	btMotionState *s = static_cast<btMotionState*>(state);
	btTransform *t = static_cast<btTransform*>(transform);
	s->setWorldTransform(*t);
}

void* BtRigidBodyNew(float mass, void* motion_state, void* shape, const float* local_interia)
{
	btVector3 interia;
	if(local_interia != NULL)
	{
		interia = btVector3(local_interia[0], local_interia[1], local_interia[2]);
	}
	else
	{
		interia = btVector3(0, 0, 0);
	}

	btRigidBody::btRigidBodyConstructionInfo info(
		mass,
		static_cast<btMotionState*>(motion_state),
		static_cast<btCollisionShape*>(shape),
		interia
	);
	btRigidBody *body = new btRigidBody(info);
	return static_cast<void*>(body);
}

void* BtRigidBodyNewWithDetailData(
	float mass,
	void* motion_state,
	void* shape,
	float* local_interia,
	float linear_damping,
	float angular_damping,
	float restitution,
	float friction,
	int additional_damping
)
{
	btVector3 interia;
	if(local_interia != NULL)
	{
		interia = btVector3(local_interia[0], local_interia[1], local_interia[2]);
	}
	else
	{
		interia = btVector3(0, 0, 0);
	}

	btRigidBody::btRigidBodyConstructionInfo info(
		mass,
		static_cast<btMotionState*>(motion_state),
		static_cast<btCollisionShape*>(shape),
		interia
	);
	info.m_linearDamping = linear_damping;
	info.m_angularDamping = angular_damping;
	info.m_restitution = restitution;
	info.m_friction = friction;
	info.m_additionalDamping = (additional_damping != FALSE) ? true : false;
	btRigidBody *body = new btRigidBody(info);
	return static_cast<void*>(body);
}

void DeleteBtRigidBody(void* body)
{
	btRigidBody *rigid_body = static_cast<btRigidBody*>(body);
	delete rigid_body;
}

void BtRigidBodySetActivationState(void* body, int state)
{
	btRigidBody *rigid_body = static_cast<btRigidBody*>(body);
	rigid_body->setActivationState(state);
}

void BtRigidBodySetUserData(void* body, void* user_data)
{
	btRigidBody *rigid_body = static_cast<btRigidBody*>(body);
	rigid_body->setUserPointer(user_data);
}

void* BtRigidBodyGetUserData(void* body)
{
	btRigidBody *rigid_body = static_cast<btRigidBody*>(body);
	return rigid_body->getUserPointer();
}

int BtRigidBodyGetCollisionFlags(void* body)
{
	btRigidBody *rigid_body = static_cast<btRigidBody*>(body);
	return rigid_body->getCollisionFlags();
}

void BtRigidBodySetCollisionFlags(void* body, int flags)
{
	btRigidBody *rigid_body = static_cast<btRigidBody*>(body);
	rigid_body->setCollisionFlags(flags);
}

void BtRigidBodyGetCenterOfMassTransform(void* rigid_body, void* transform)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	btTransform *t = static_cast<btTransform*>(transform);

	*t = body->getCenterOfMassTransform();
}

void BtRigidBodySetCenterOfMassTransform(void* rigid_body, void* transform)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	btTransform *trans = static_cast<btTransform*>(transform);

	body->setCenterOfMassTransform(*trans);
}

void BtRigidBodyAddConstraint(void* rigid_body, void* constraint)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	btTypedConstraint *c = static_cast<btTypedConstraint*>(constraint);

	body->addConstraintRef(c);
}

void BtRigidBodyRemoveConstraint(void* rigid_body, void* constraint)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	btTypedConstraint *c = static_cast<btTypedConstraint*>(constraint);

	body->removeConstraintRef(c);
}

void* BtRigidBodyUpcast(void* object)
{
	return static_cast<void*>(btRigidBody::upcast(static_cast<btCollisionObject*>(object)));
}

void BtRigidBodySetWorldTransform(void* rigid_body, void* transform)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	btTransform *t = static_cast<btTransform*>(transform);

	body->setWorldTransform(*t);
}

void BtRigidBodySetInterpolationWorldTransform(void* rigid_body, void* transform)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	btTransform *t = static_cast<btTransform*>(transform);

	body->setInterpolationWorldTransform(*t);
}

void BtRigidBodySetMotionState(void* rigid_body, void* motion_state)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	btMotionState *state = static_cast<btMotionState*>(motion_state);

	body->setMotionState(state);
}

void* BtRigidBodyGetBroadphaseHandle(void* rigid_body)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	return (void*)static_cast<const void*>(body->getBroadphaseHandle());
}

void BtRigidBodyApplyForce(void* rigid_body, const float* force, const float* relative_position)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	if(relative_position != NULL)
	{
		body->applyForce(btVector3(force[0], force[1], force[2]),
			btVector3(relative_position[0], relative_position[1], relative_position[2]));
	}
	else
	{
		body->applyCentralForce(btVector3(force[0], force[1], force[2]));
	}
}

void BtRigidBodyClearForces(void* rigid_body)
{
	btRigidBody *body = static_cast<btRigidBody*>(rigid_body);
	body->clearForces();
}

void* BtGeneric6DofSpringConstraintNew(
	void* rigid_bodyA,
	void* rigid_bodyB,
	void* transformA,
	void* transformB,
	int use_linear_reference_frameA
)
{
	btRigidBody *bodyA = static_cast<btRigidBody*>(rigid_bodyA);
	btRigidBody *bodyB = static_cast<btRigidBody*>(rigid_bodyB);
	btTransform *transA = static_cast<btTransform*>(transformA);
	btTransform *transB = static_cast<btTransform*>(transformB);
	const bool use_linear = (use_linear_reference_frameA != 0) ? true : false;
	btGeneric6DofConstraint *constraint = new btGeneric6DofSpringConstraint(
		*bodyA, *bodyB, *transA, *transB, use_linear
	);

	return static_cast<void*>(constraint);
}

void DeleteBtGeneric6DofSpringConstraint(void* constraint)
{
	btGeneric6DofConstraint *c = static_cast<btGeneric6DofConstraint*>(constraint);

	delete c;
}

void BtGeneric6DofConstraintSetLinearLimitUpper(
	void* constraint,
	float x,
	float y,
	float z
)
{
	btGeneric6DofConstraint *c = static_cast<btGeneric6DofConstraint*>(constraint);

	c->setLinearUpperLimit(btVector3(x, y, z));
}

void BtGeneric6DofConstraintSetLinearLimitLower(
	void* constraint,
	float x,
	float y,
	float z
)
{
	btGeneric6DofConstraint *c = static_cast<btGeneric6DofConstraint*>(constraint);

	c->setLinearLowerLimit(btVector3(x, y, z));
}

void BtGeneric6DofConstraintSetAngularLimitUpper(
	void* constraint,
	float x,
	float y,
	float z
)
{
	btGeneric6DofConstraint *c = static_cast<btGeneric6DofConstraint*>(constraint);

	c->setAngularUpperLimit(btVector3(x, y, z));
}

void BtGeneric6DofConstraintSetAngularLimitLower(
	void* constraint,
	float x,
	float y,
	float z
)
{
	btGeneric6DofConstraint *c = static_cast<btGeneric6DofConstraint*>(constraint);

	c->setAngularLowerLimit(btVector3(x, y, z));
}

void BtGeneric6DofSpringConstraintEnableSpring(void* constraint, int index, int is_on)
{
	btGeneric6DofSpringConstraint *c = static_cast<btGeneric6DofSpringConstraint*>(constraint);

	c->enableSpring(index, is_on != FALSE);
}

void BtGeneric6DofSpringConstraintSetStiffness(void* constraint, int index, float stiffness)
{
	btGeneric6DofSpringConstraint *c = static_cast<btGeneric6DofSpringConstraint*>(constraint);

	c->setStiffness(index, stiffness);
}

void BtGeneric6DofSpringConstraintSetDamping(void* constraint, int index, float damping)
{
	btGeneric6DofSpringConstraint *c = static_cast<btGeneric6DofSpringConstraint*>(constraint);

	c->setDamping(index, damping);
}

void BtGeneric6DofSpringConstraintSetEquilibriumPoint(void* constraint)
{
	btGeneric6DofSpringConstraint *c = static_cast<btGeneric6DofSpringConstraint*>(constraint);

	c->setEquilibriumPoint();
}

void BtGeneric6DofSpringConstraintCalculateTransforms(void* constraint)
{
	btGeneric6DofSpringConstraint *c = static_cast<btGeneric6DofSpringConstraint*>(constraint);

	c->calculateTransforms();
}

void* BtPoint2PointConstraintNew(
	void* rigid_body_a,
	void* rigid_body_b,
	const float* pivot_a,
	const float* pivot_b
)
{
	btRigidBody *body_a = static_cast<btRigidBody*>(rigid_body_a);
	btRigidBody *body_b = static_cast<btRigidBody*>(rigid_body_b);

	return static_cast<void*>(new btPoint2PointConstraint(*body_a, *body_b,
		btVector3(pivot_a[0], pivot_a[1], pivot_a[2]), btVector3(pivot_b[0], pivot_b[1], pivot_b[2])));
}

void* BtConeTwistConstraintNew(
	void* rigid_body_a,
	void* rigid_body_b,
	void* transform_a,
	void* transform_b
)
{
	btRigidBody *body_a = static_cast<btRigidBody*>(rigid_body_a);
	btRigidBody *body_b = static_cast<btRigidBody*>(rigid_body_b);
	btTransform *trans_a = static_cast<btTransform*>(transform_a);
	btTransform *trans_b = static_cast<btTransform*>(transform_b);

	return static_cast<void*>(new btConeTwistConstraint(*body_a, *body_b, *trans_a, *trans_b));
}

void BtConeTwistConstraintSetLimit(
	void* constraint,
	float lower_x,
	float lower_y,
	float lower_z,
	float stiffness_x,
	float stiffness_y,
	float stiffness_z
)
{
	btConeTwistConstraint *c = static_cast<btConeTwistConstraint*>(constraint);

	c->setLimit(lower_x, lower_y, lower_z, stiffness_x, stiffness_y, stiffness_z);
}

void BtConeTwistConstraintSetDamping(void* constraint, float damping)
{
	btConeTwistConstraint *c = static_cast<btConeTwistConstraint*>(constraint);

	c->setDamping(damping);
}

void BtConeTwistConstraintSetFixThreshold(void* constraint, float threshold)
{
	btConeTwistConstraint *c = static_cast<btConeTwistConstraint*>(constraint);

	c->setFixThresh(threshold);
}

void BtConeTwistConstraintEnableMotor(void* constraint, int is_enable)
{
	btConeTwistConstraint *c = static_cast<btConeTwistConstraint*>(constraint);

	c->enableMotor(is_enable != FALSE);
}

void BtConeTwistConstraintSetMaxMotorImpulse(void* constraint, float impulse)
{
	btConeTwistConstraint *c = static_cast<btConeTwistConstraint*>(constraint);

	c->setMaxMotorImpulse(impulse);
}

void BtConeTwistConstraintSetMotorTargetInConstraintSpace(void* constraint, const float* quaternion)
{
	btConeTwistConstraint *c = static_cast<btConeTwistConstraint*>(constraint);

	c->setMotorTargetInConstraintSpace(btQuaternion(
		quaternion[0], quaternion[1], quaternion[2], quaternion[3]));
}

void* BtSliderConstraintNew(
	void* rigid_body_a,
	void* rigid_body_b,
	void* transform_a,
	void* transform_b,
	int use_linear_reference_frame
)
{
	btRigidBody *body_a = static_cast<btRigidBody*>(rigid_body_a);
	btRigidBody *body_b = static_cast<btRigidBody*>(rigid_body_b);
	btTransform *trans_a = static_cast<btTransform*>(transform_a);
	btTransform *trans_b = static_cast<btTransform*>(transform_b);

	return static_cast<void*>(new btSliderConstraint(*body_a, *body_b, *trans_a, *trans_b,
		use_linear_reference_frame != FALSE));
}

void BtSliderConstraintSetUpperLinearLimit(void* constraint, float limit)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setUpperLinLimit(limit);
}

void BtSliderConstraintSetLowerLinearLimit(void* constraint, float limit)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setLowerLinLimit(limit);
}

void BtSliderConstraintSetUpperAngularLimit(void* constraint, float limit)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setUpperAngLimit(limit);
}

void BtSliderConstraintSetLowerAngularLimit(void* constraint, float limit)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setLowerAngLimit(limit);
}

void BtSliderConstraintSetPoweredLinearMotor(void* constraint, int is_on)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setPoweredLinMotor(is_on != FALSE);
}

void BtSliderConstraintSetTargetLinearMotorVelocity(void* constraint, float velocity)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setTargetLinMotorVelocity(velocity);
}

void BtSliderConstraintSetMaxLinearMotorForce(void* constraint, float force)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setMaxLinMotorForce(force);
}

void BtSliderConstraintSetPoweredAngluarMotor(void* constraint, int is_on)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setPoweredAngMotor(is_on != FALSE);
}

void BtSliderConstraintSetTargetAngluarMotorVelocity(void* constraint, float velocity)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setTargetAngMotorVelocity(velocity);
}

void BtSliderConstraintSetMaxAngluarMotorForce(void* constraint, float force)
{
	btSliderConstraint *c = static_cast<btSliderConstraint*>(constraint);

	c->setMaxAngMotorForce(force);
}

void* BtHingeConstraintNew(
	void* rigid_body_a,
	void* rigid_body_b,
	void* transform_a,
	void* transform_b
)
{
	btRigidBody *body_a = static_cast<btRigidBody*>(rigid_body_a);
	btRigidBody *body_b = static_cast<btRigidBody*>(rigid_body_b);
	btTransform *trans_a = static_cast<btTransform*>(transform_a);
	btTransform *trans_b = static_cast<btTransform*>(transform_b);

	return static_cast<void*>(new btHingeConstraint(*body_a, *body_b, *trans_a, *trans_b));
}

void BtHingeConstraintSetLimit(
	void* constraint,
	float rotation_lower,
	float rotation_upper,
	float softness,
	float bias,
	float relaxation
)
{
	btHingeConstraint *c = static_cast<btHingeConstraint*>(constraint);

	c->setLimit(rotation_lower, rotation_upper, softness, bias, relaxation);
}

void BtHingeConstraintEnableMotor(void* constraint, int is_on)
{
	btHingeConstraint *c = static_cast<btHingeConstraint*>(constraint);

	c->enableMotor(is_on != FALSE);
}

void BtHingeConstraintEnableAngularMotor(void* constraint, int is_on, float target_velocity, float max_motor_impulse)
{
	btHingeConstraint *c = static_cast<btHingeConstraint*>(constraint);

	c->enableAngularMotor(is_on != FALSE, target_velocity, max_motor_impulse);
}

void* BtDefaultMotionStateNew(void)
{
	return static_cast<void*>(new btDefaultMotionState());
}

void DeleteBtDefaultMotionState(void* state)
{
	btDefaultMotionState *motion_state = static_cast<btDefaultMotionState*>(state);

	delete motion_state;
}

void DeleteBtTypedConstraint(void* constraint)
{
	btTypedConstraint *c = static_cast<btTypedConstraint*>(constraint);

	delete c;
}

int BtTypedConstraintGetType(void* constraint)
{
	btTypedConstraint *c = static_cast<btTypedConstraint*>(constraint);

	return c->getConstraintType();
}

void BtTypedConstraintSetUserConstraintPointer(void* constraint, void* user_constraint)
{
	btTypedConstraint *c = static_cast<btTypedConstraint*>(constraint);

	c->setUserConstraintPtr(user_constraint);
}

void* BtBoxShapeNew(float* vector)
{
	return static_cast<void*>(new btBoxShape(btVector3(vector[0], vector[1], vector[2])));
}

void DeleteBtBoxShape(void* shape)
{
	btBoxShape *s = static_cast<btBoxShape*>(shape);

	delete s;
}

void* BtTriangleMeshNew(void)
{
	btTriangleMesh *mesh = new btTriangleMesh();

	return static_cast<void*>(mesh);
}

void DeleteBtTriangleMesh(void* mesh)
{
	btTriangleMesh *m = static_cast<btTriangleMesh*>(mesh);

	delete m;
}

void BtTriangleMeshAddTriangle(
	void* mesh,
	float position1[3],
	float position2[3],
	float position3[3]
)
{
	btTriangleMesh *m = static_cast<btTriangleMesh*>(mesh);

	m->addTriangle(btVector3(position1[0], position1[1], position1[2]),
		btVector3(position2[0], position2[1], position2[2]),
		btVector3(position3[0], position3[1], position3[2])
	);
}

void* BtBvhTriangleMeshShapeNew(void* triangle_mesh, int use_aa_bb_compression)
{
	btTriangleMesh *mesh = static_cast<btTriangleMesh*>(triangle_mesh);
	btBvhTriangleMeshShape *shape = new btBvhTriangleMeshShape(mesh,
		use_aa_bb_compression != 0);

	return static_cast<void*>(shape);
}

void DeleteBtBvhTriangleMeshShape(void* shape)
{
	btBvhTriangleMeshShape *s = static_cast<btBvhTriangleMeshShape*>(shape);

	delete s;
}

#ifdef __cplusplus
};
#endif
