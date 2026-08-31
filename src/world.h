#pragma once

#include <raylib.h>

#include "ship.h"
#include "bullets.h"

#define MAX_BULLETS 100
#define MAX_BUILDINGS 15
#define MAX_ENEMIES 10

typedef struct Building {
	Vector3 position;
	BoundingBox bounds;
	bool active;
} Building;

typedef struct WorldState
{
	Building buildings[MAX_BUILDINGS];
	EnemyController enemyControllers[MAX_ENEMIES];
	Ship enemyShips[MAX_ENEMIES];
	Projectile bullets[MAX_BULLETS];
	int targetsDestroyed;
} WorldState;
