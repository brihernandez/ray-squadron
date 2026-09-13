#include "ship.h"

#include <raymath.h>

#include "world.h"
#include "smoothdamp.h"

static ShipInput ShipInputNormalize(ShipInput input)
{
	input.pitch = Clamp(input.pitch, -1, 1);
	input.yaw = Clamp(input.yaw, -1, 1);
	input.roll = Clamp(input.roll, -1, 1);
	return input;
}

static Matrix BuildTransformMatrix(Vector3 position, Quaternion rotation)
{
	Matrix rotationMat = QuaternionToMatrix(rotation);
	Matrix positionMat = MatrixTranslate(position.x, position.y, position.z);
	return MatrixMultiply(rotationMat, positionMat);
}

Ship ShipInit(ShipHandling handling, ShipWeapons weapons, bool isEnemy)
{
	Ship ship = {0};
	ship.position = (Vector3){0, 0, 0};
	ship.rotation = (Quaternion){ 0.0f, 0.0f, 0.0f, 1.0f };
	ship.transform = BuildTransformMatrix(ship.position, ship.rotation);
	ship.forward = (Vector3){0, 0, 1};
	ship.right = (Vector3){1, 0, 0};
	ship.up = (Vector3){0, 1, 0};
	ship.localAngularVelocity = (Vector3){0, 0, 0};
	ship.handling = handling;
	ship.weapons = weapons;
	ship.speed = handling.maxSpeed;
	ship.isActive = true;
	ship.isEnemy = isEnemy;
	return ship;
}

void ShipUpdate(WorldState* world, Ship* ship, ShipInput input, float deltaTime)
{
	// Add autolevel to the input.
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
	if (!IsKeyDown(KEY_SPACE))
	ship->position = Vector3Add(ship->position, Vector3Scale(ship->forward, ship->speed * deltaTime));
	if (ship->position.y < 2)
		ship->position.y = 2;

	ship->bounds = ShipCalculateBounds(ship->position);

	// Update the transform matrix and other shortcuts.
	ship->transform = BuildTransformMatrix(ship->position, ship->rotation);
	ship->forward = Vector3RotateByQuaternion((Vector3) { 0, 0, 1 }, ship->rotation);
	ship->up = Vector3RotateByQuaternion((Vector3) { 0, 1, 0 }, ship->rotation);
	ship->right = Vector3RotateByQuaternion((Vector3) { 1, 0, 0 }, ship->rotation);

	// Ship weapons.
	ship->weapons.timeSinceLastShot += deltaTime;
	if (input.isFiring && ship->weapons.timeSinceLastShot > ship->weapons.fireDelay)
	{
		Vector3 localFirePos = ship->weapons.barrels[ship->weapons.barrelIndex];
		Vector3 worldFirePos = Vector3Transform(localFirePos, ship->transform);
		Vector3 bulletVelocity = Vector3Scale(ship->forward, ship->speed + ship->weapons.muzzleVelocity);
		BulletsFire(world, worldFirePos, bulletVelocity, 1, ship->isEnemy);
		ship->weapons.barrelIndex = (ship->weapons.barrelIndex + 1) % ship->weapons.barrelCount;
		ship->weapons.timeSinceLastShot = 0;

		// TODO: Lowering the volume of enemy shots to make them distinct but this means
		// 1. Enemy shots can overstep player shots and make the player shots sound quiet.
		// 2. Enemy shots should probably have a unique fire sound.
		// 3. There needs to be some kind of audio manager that adjusts pooled sounds to
		// fade their volume based on distance.
		if (ship->isEnemy)
			SetSoundVolume(ship->weapons.fireSound, 0.2f);
		else
			SetSoundVolume(ship->weapons.fireSound, 1.0f);
		PlaySound(ship->weapons.fireSound);
	}
}

void ShipDraw(Ship* ship, Model* model, Color color)
{
	float bankAngle = 30.f * DEG2RAD;
	float visualBank = Remap(ship->localAngularVelocity.y, -ship->handling.yawRate, ship->handling.yawRate, bankAngle, -bankAngle);
	Quaternion visualRotation = QuaternionMultiply(QuaternionFromAxisAngle(ship->forward, visualBank), ship->rotation);
	Matrix transform = MatrixTranslate(ship->position.x, ship->position.y, ship->position.z);
	model->transform = MatrixMultiply(QuaternionToMatrix(visualRotation), transform);
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
		enemy->input.isFiring = !enemy->input.isFiring;
	}
}
