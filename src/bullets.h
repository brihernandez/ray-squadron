#pragma once

#include <raylib.h>
#include <stdbool.h>

struct WorldState;

typedef struct Projectile {
	Vector3 position;
	Vector3 velocity;
	float lifeTime;
	bool isActive;
} Projectile;

void BulletsInit(struct WorldState* world);
void BulletsFire(struct WorldState* world, Vector3 position, Vector3 velocity, float timeToLive);
bool BulletsUpdate(struct WorldState* world, float deltaTime);
void BulletsDraw(struct WorldState* world);
