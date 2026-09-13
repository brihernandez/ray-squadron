#pragma once

#include <raylib.h>
#include <stdbool.h>

struct WorldState;

typedef struct Projectile {
	Vector3 position;
	Vector3 velocity;
	float timeActive;
	float timeToLive;
	bool isEnemy;
	bool isActive;
} Projectile;

void BulletsInit(struct WorldState* world);
void BulletsFire(struct WorldState* world, Vector3 position, Vector3 velocity, float timeToLive, bool isEnemy);
bool BulletsUpdate(struct WorldState* world, float deltaTime);
void BulletsDraw(struct WorldState* world);
