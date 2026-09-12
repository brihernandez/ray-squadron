#pragma once

#include <raylib.h>

struct WorldState;

#define TURRET_MAXHP 10
typedef struct Turret
{
	Vector3 position;
	Quaternion rotation;
	Vector3 targetPos;
	Vector3 azimuthLocalPosition;
	Vector3 elevationLocalPosition;
	Vector3 firepoints[2];
	int numFirepoints;
	float azimuth;
	float elevation;
	float fireDelay;
	float fireCooldown;
	float turnRate;
	int hp;
} Turret;

void TurretUpdate(struct WorldState* world, Turret* turret, float deltaTime);
void TurretDraw(Model* model, Turret* turret);
