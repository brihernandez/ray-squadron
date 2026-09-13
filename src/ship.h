#pragma once

#include <raylib.h>

struct WorldState;

typedef struct ShipHandling
{
	float pitchRate, yawRate, rollRate;
	float maxSpeed;
	float minSpeed;
} ShipHandling;

typedef struct ShipWeapons
{
	Vector3 barrels[2];
	Sound fireSound;
	float fireDelay;
	float muzzleVelocity;
	float timeSinceLastShot;
	int barrelCount;
	int barrelIndex;
} ShipWeapons;

typedef struct Ship
{
	Vector3 position;
	Quaternion rotation;
	Matrix transform;
	Vector3 forward, right, up;
	Vector3 localAngularVelocity;
	BoundingBox bounds;
	ShipHandling handling;
	ShipWeapons weapons;
	float speed;
	bool isActive;
	bool isEnemy;
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

Ship ShipInit(ShipHandling handling, ShipWeapons weapons, bool isEnemy);
void ShipUpdate(struct WorldState* world, Ship* ship, ShipInput input, float deltaTime);
void ShipDraw(Ship* ship, Model* model, Color color);

BoundingBox ShipCalculateBounds(Vector3 position);

void EnemyControllerUpdate(EnemyController* enemy, Ship* ship, float deltaTime);
