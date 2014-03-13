#include "camera.h"
#include "application.h"
#include "bullet.h"

void InitializeCamera(CAMERA* camera, void* scene)
{
	const float identity[9] = IDENTITY_MATRIX3x3;

	camera->scene = scene;
	camera->znear = 0.5f;
	camera->zfar = 1000000;
	camera->transform = BtTransformNew(identity);

	ResetCameraDefault(camera);
}

void SetCameraLookAt(CAMERA* camera, const float* look_at)
{
	camera->look_at[0] = look_at[0];
	camera->look_at[1] = look_at[1];
	camera->look_at[2] = look_at[2];
}

void SetCameraAngle(CAMERA* camera, const float* angle)
{
	camera->angle[0] = angle[0];
	camera->angle[1] = angle[1];
	camera->angle[2] = angle[2];
}

void SetCameraDistance(CAMERA* camera, const float distance)
{
	camera->distance[2] = distance;
	camera->position[0] = camera->look_at[0] + camera->distance[0];
	camera->position[1] = camera->look_at[1] + camera->distance[1];
	camera->position[2] = camera->look_at[2] + camera->distance[2];
}

void CameraUpdateTransform(CAMERA* camera)
{
	float rotation_x[4], rotation_y[4], rotation_z[4];

	const float axis_x[] = {1, 0, 0};
	const float axis_y[] = {0, 1, 0};
	const float axis_z[] = {0, 0, 1};

	float position[3];
	float look_at[3];

	QuaternionSetRotation(rotation_x, axis_x, camera->angle[0]);
	QuaternionSetRotation(rotation_y, axis_y, camera->angle[1]);
	QuaternionSetRotation(rotation_z, axis_z, camera->angle[2]);

	BtTransformSetIdentity(camera->transform);
	MultiQuaternion(rotation_z, rotation_y);
	MultiQuaternion(rotation_z, rotation_x);
	BtTransformSetRotation(camera->transform, rotation_z);

	look_at[0] = - camera->look_at[0];
	look_at[1] = - camera->look_at[1];
	look_at[2] = - camera->look_at[2];
	BtTransformMultiVector3(camera->transform, look_at, position);
	position[0] -= camera->distance[0];
	position[1] -= camera->distance[1];
	position[2] -= camera->distance[2];
	BtTransformSetOrigin(camera->transform, position);
}

void ResetCameraDefault(CAMERA* camera)
{
	const float look_at[] = {0.0f, 10.0f, 0.0f};
	const float angle[] = {0.0f, 0.0f, 0.0f};

	SetCameraLookAt(camera, look_at);
	SetCameraAngle(camera, angle);
	camera->fov = 27;
	SetCameraDistance(camera, 50);

	CameraUpdateTransform(camera);
}
