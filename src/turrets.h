#pragma once

#include <raylib.h>

struct WorldState;

#define TURRET_MAX_HP 10
#define TURRET_MAX_FIREPOINTS 2
typedef struct Turret
{
	Vector3 position;
	Quaternion rotation;
	BoundingBox bounds;
	Vector3 size;
	Vector3 targetPos;
	Vector3 azimuthLocalPosition;
	Vector3 elevationLocalPosition;
	Vector3 firepoints[TURRET_MAX_FIREPOINTS];
	int numFirepoints;
	float azimuth;
	float elevation;
	float fireDelay;
	float fireCooldown;
	Sound fireSound;
	float turnRate;
	int hp;
	bool isActive;
} Turret;

BoundingBox TurretGetBounds(Turret* turret, Vector3 position);
void TurretUpdate(struct WorldState* world, Turret* turret, float deltaTime);
void TurretDraw(Model* model, Turret* turret);
