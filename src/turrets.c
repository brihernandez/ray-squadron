#include "turrets.h"
#include "world.h"

#include <raymath.h>
#include <stdio.h>

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
		if (turret->numFirepoints == 0)
		{
			printf("Turret has no firepoints!");
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
			BulletsFire(world, firePosition, muzzleVelocity, 3);
		}
		turret->fireCooldown = turret->fireDelay;
	}
}

void TurretDraw(Model* model, Turret* turret)
{
	Material defaultMaterial = LoadMaterialDefault();

	Matrix worldMat = MatrixBuildTransform(
		turret->position,
		turret->rotation);

	Matrix azimuthMat = MatrixBuildTransform(
		(Vector3) { 0, 32, 0 },
		QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, turret->azimuth));
	Matrix worldAzimuthMat = MatrixMultiply(azimuthMat, worldMat);

	Matrix elevationMat = MatrixBuildTransform(
		(Vector3) { 0, 10, 0 },
		QuaternionFromAxisAngle((Vector3) { 1, 0, 0 }, turret->elevation));
	Matrix worldElevationMat = MatrixMultiply(elevationMat, worldAzimuthMat);

	DrawMesh(model->meshes[0], defaultMaterial, worldMat);
	DrawMesh(model->meshes[1], defaultMaterial, worldAzimuthMat);
	DrawMesh(model->meshes[2], defaultMaterial, worldElevationMat);
}
