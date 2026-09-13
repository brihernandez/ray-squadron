#pragma once

#include <raylib.h>
#include <stdbool.h>

#include "ship.h"
#include "bullets.h"
#include "turrets.h"

#define MAX_BULLETS 250
#define MAX_BUILDINGS 15
#define MAX_ENEMIES 10
#define MAX_TURRETS 30

#define BUILDING_SIZE 20

typedef struct Building {
	Vector3 position;
	BoundingBox bounds;
	bool active;
} Building;

typedef struct WorldState
{
	Building buildings[MAX_BUILDINGS];
	EnemyController enemyControllers[MAX_ENEMIES];
	Ship playerShip;
	Ship enemyShips[MAX_ENEMIES];
	Projectile bullets[MAX_BULLETS];
	Turret turrets[MAX_TURRETS];
	int targetsDestroyed;
} WorldState;
