#pragma once

#include <raylib.h>

typedef struct ShipHandling
{
	float pitchRate, yawRate, rollRate;
	float maxSpeed;
	float minSpeed;
} ShipHandling;

typedef struct ShipWeapons
{
	float fireDelay;
	float muzzleVelocity;
} ShipWeapons;

typedef struct Ship
{
	Vector3 position;
	Quaternion rotation;
	Vector3 forward, right, up;
	Vector3 localAngularVelocity;
	BoundingBox bounds;
	ShipHandling handling;
	ShipWeapons weapons;
	float speed;
	float timeSinceLastShot;
	int barrelIndex;
	bool isActive;
} Ship;

typedef struct ShipInput
{
	float pitch, yaw, roll;
	bool isFiring;
} ShipInput;

typedef struct EnemyController
{
	ShipInput input;
	float targetAltitude;
	float thinkCooldown;
} EnemyController;

ShipInput ShipInputNormalize(ShipInput input);

Ship ShipInit(ShipHandling handling, ShipWeapons weapons);
void ShipUpdate(Ship* ship, ShipInput input, float deltaTime);
void ShipDraw(Ship* ship, Model* model, Color color);

BoundingBox ShipCalculateBounds(Vector3 position);

void EnemyControllerUpdate(EnemyController* enemy, Ship* ship, float deltaTime);
