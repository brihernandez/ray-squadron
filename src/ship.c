#include "ship.h"

#include <raymath.h>
#include "smoothdamp.h"

#include <stdio.h>

ShipInput ShipInputNormalize(ShipInput input)
{
	input.pitch = Clamp(input.pitch, -1, 1);
	input.yaw = Clamp(input.yaw, -1, 1);
	input.roll = Clamp(input.roll, -1, 1);
	return input;
}

Ship ShipInit(ShipHandling handling, ShipWeapons weapons)
{
	Ship ship = {0};

	ship.position = (Vector3){0, 0, 0};
	ship.rotation = (Quaternion){ 0.0f, 0.0f, 0.0f, 1.0f };
	ship.forward = (Vector3){0, 0, 1};
	ship.right = (Vector3){1, 0, 0};
	ship.up = (Vector3){0, 1, 0};
	ship.localAngularVelocity = (Vector3){0, 0, 0};
	ship.handling = handling;
	ship.weapons = weapons;

	ship.speed = handling.maxSpeed;
	ship.timeSinceLastShot = 0;
	ship.barrelIndex = 0;

	ship.isActive = true;

	return ship;
}

void ShipUpdate(Ship* ship, ShipInput input, float deltaTime)
{
	// Add autolevel to the input.
	ship->forward = Vector3RotateByQuaternion((Vector3) { 0, 0, 1 }, ship->rotation);
	ship->up = Vector3RotateByQuaternion((Vector3) { 0, 1, 0 }, ship->rotation);
	ship->right = Vector3RotateByQuaternion((Vector3) { 1, 0, 0 }, ship->rotation);
	input.roll -= ship->right.y / 2.0f;
	input = ShipInputNormalize(input);

	// Ship rotation.
	float smoothSpeed = 5;
	ship->localAngularVelocity.x = SmoothDamp(ship->localAngularVelocity.x, input.pitch * ship->handling.pitchRate, smoothSpeed, deltaTime);
	ship->localAngularVelocity.y = SmoothDamp(ship->localAngularVelocity.y, input.yaw * ship->handling.yawRate, smoothSpeed, deltaTime);
	ship->localAngularVelocity.z = SmoothDamp(ship->localAngularVelocity.z, input.roll * ship->handling.rollRate, smoothSpeed, deltaTime);

	ship->rotation = QuaternionMultiply(ship->rotation, QuaternionFromAxisAngle((Vector3) { 1, 0, 0 }, ship->localAngularVelocity.x* deltaTime));
	ship->rotation = QuaternionMultiply(ship->rotation, QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, ship->localAngularVelocity.y* deltaTime));
	ship->rotation = QuaternionMultiply(ship->rotation, QuaternionFromAxisAngle((Vector3) { 0, 0, 1 }, ship->localAngularVelocity.z* deltaTime));
	ship->rotation = QuaternionNormalize(ship->rotation);

	// Ship translation.
	ship->position = Vector3Add(ship->position, Vector3Scale(ship->forward, ship->speed * deltaTime));
	if (ship->position.y < 2)
		ship->position.y = 2;

	ship->bounds = ShipCalculateBounds(ship->position);
}

void ShipDraw(Ship* ship, Model* model, Color color)
{
	Matrix transform = MatrixTranslate(ship->position.x, ship->position.y, ship->position.z);
	model->transform = MatrixMultiply(QuaternionToMatrix(ship->rotation), transform);
	DrawModel(*model, Vector3Zero(), 1, color);
}

BoundingBox ShipCalculateBounds(Vector3 position)
{
	float size = 8;
	BoundingBox b = (BoundingBox){
		.min = (Vector3){position.x - size, position.y - size, position.z - size},
		.max = (Vector3){position.x + size, position.y + size, position.z + size},
	};
	return b;
}

void EnemyControllerUpdate(EnemyController* enemy, Ship* ship, float deltaTime)
{
	enemy->thinkCooldown -= deltaTime;

	float deltaHeight = enemy->targetAltitude - ship->position.y;
	float targetForwardY = Remap(deltaHeight, 50.f, -50.f, 0.3f, -0.3f);
	//targetForwardY = Clamp(targetForwardY, -0.3f, 0.3f);

	float forwardY = ship->forward.y;
	float deltaForwardY = targetForwardY - forwardY;
	enemy->input.pitch = -deltaForwardY;

	if (enemy->thinkCooldown <= 0)
	{
		enemy->input.yaw = GetRandomValue(-100, 100) / 100.0f;
		enemy->input.roll = enemy->input.yaw * -0.2f;
		enemy->thinkCooldown = GetRandomValue(200, 400) / 100.0f;
	}
}
