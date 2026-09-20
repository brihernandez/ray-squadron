#include "turrets.h"
#include "bullets.h"
#include "world.h"
#include "audio.h"

#include <raylib.h>
#include <raymath.h>

BoundingBox TurretGetBounds(Turret* turret, Vector3 position)
{
	float halfX = turret->size.x / 2;
	float halfZ = turret->size.z / 2;
	BoundingBox bounds = {
		.min = Vector3Subtract(position, (Vector3) {halfX, 0, halfZ}),
		.max = Vector3Add(position, (Vector3) {halfX, turret->size.y, halfZ}),
	};
	return bounds;
}

Vector3 TurretGetCenter(Turret* turret)
{
	Vector3 center = {
		turret->position.x,
		turret->position.y + turret->size.y / 2,
		turret->position.z
	};
	return center;
}

static Matrix MatrixBuildTransform(Vector3 position, Quaternion rotation)
{
	Matrix transform = MatrixTranslate(position.x, position.y, position.z);
	transform = MatrixMultiply(QuaternionToMatrix(rotation), transform);
	return transform;
}

void TurretUpdate(WorldState* world, Turret* turret, float deltaTime)
{
	turret->azimuth += turret->turnRate * DEG2RAD * GetFrameTime();
	turret->elevation = (float)sin(GetTime() * 10) * 0.2f - 0.4f;

	turret->fireCooldown -= deltaTime;
	if (turret->fireCooldown <= 0)
	{
		// TODO: These two warnings should go on some Initialize function instead.
		if (turret->numFirepoints == 0)
		{
			TraceLog(LOG_ERROR, "Turret has no firepoints!");
			turret->fireCooldown = turret->fireDelay;
			return;
		}
		if (turret->numFirepoints > TURRET_MAX_FIREPOINTS)
		{
			TraceLog(LOG_ERROR, "Turret has too many firepoints! Cannot be greater than %d", TURRET_MAX_FIREPOINTS);
			turret->fireCooldown = turret->fireDelay;
			return;
		}

		Matrix worldMat = MatrixBuildTransform(
			turret->position,
			turret->rotation);

		Matrix azimuthMat = MatrixBuildTransform(
			turret->azimuthLocalPosition,
			QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, turret->azimuth));
		Matrix worldAzimuthMat = MatrixMultiply(azimuthMat, worldMat);

		Matrix elevationMat = MatrixBuildTransform(
			turret->elevationLocalPosition,
			QuaternionFromAxisAngle((Vector3) { 1, 0, 0 }, turret->elevation));
		Matrix worldElevationMat = MatrixMultiply(elevationMat, worldAzimuthMat);

		for (int i = 0; i < turret->numFirepoints; i++)
		{
			Vector3 firePosition = Vector3Transform(turret->firepoints[i], worldElevationMat);
			Vector3 muzzleVelocity = Vector3RotateByQuaternion(
				(Vector3) { 0, 0, 400 },
				QuaternionFromMatrix(worldElevationMat));
			// Currently turrets fire "friendly" bullets!
			BulletsFire(world, firePosition, muzzleVelocity, 3, false);
			AudioPlaySoundAt(turret->fireSound, firePosition, 200, 500, 1);
		}
		turret->fireCooldown = turret->fireDelay;
	}
}

void TurretDraw(Model* model, Turret* turret)
{
	Matrix worldMat = MatrixBuildTransform(
		turret->position,
		turret->rotation);

	// Only draw the turret stuff if the turret is still alive.
	if (turret->isActive)
	{
		Matrix azimuthMat = MatrixBuildTransform(
			(Vector3) { 0, 32, 0 },
			QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, turret->azimuth));
		Matrix worldAzimuthMat = MatrixMultiply(azimuthMat, worldMat);

		Matrix elevationMat = MatrixBuildTransform(
			(Vector3) { 0, 10, 0 },
			QuaternionFromAxisAngle((Vector3) { 1, 0, 0 }, turret->elevation));
		Matrix worldElevationMat = MatrixMultiply(elevationMat, worldAzimuthMat);

		DrawMesh(model->meshes[1], model->materials[0], worldAzimuthMat);
		DrawMesh(model->meshes[2], model->materials[0], worldElevationMat);
	}

	// The base of the turret always gets drawn even if the turret is dead.
	DrawMesh(model->meshes[0], model->materials[0], worldMat);

#ifdef SHOW_HITBOXES
	DrawBoundingBox(turret->bounds, RED);
#endif
}
