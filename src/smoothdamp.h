#pragma once

#include <raymath.h>

static float SmoothDamp(float from, float to, float speed, float dt)
{
	return Lerp(from, to, 1 - expf(-speed * dt));
}

static Vector3 Vector3SmoothDamp(Vector3 from, Vector3 to, float speed, float dt)
{
	return Vector3Lerp(from, to, 1 - expf(-speed * dt));
}

static Quaternion QuaternionSmoothDamp(Quaternion from, Quaternion to, float speed, float dt)
{
	return QuaternionSlerp(from, to, 1 - expf(-speed * dt));
}
