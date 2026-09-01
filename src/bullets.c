#include "bullets.h"
#include "world.h"

#include <raymath.h>

#define MUZZLE_FLASH_SIZE 3
static Vector3 muzzleFlashes[MAX_BULLETS];
static int muzzleFlashCount = 0;

#define IMPACT_SIZE 15
static Vector3 impacts[MAX_BULLETS];
static int impactCount = 0;

void BulletsInit(WorldState* world)
{
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		world->bullets[i].isActive = false;
	}
}

void BulletsFire(WorldState* world, Vector3 position, Vector3 velocity, float timeToLive)
{
	// Find first inactive bullet and use it to spawn.
	// This whole thing can be done much better.
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (!world->bullets[i].isActive)
		{
			world->bullets[i].isActive = true;
			world->bullets[i].lifeTime = timeToLive;
			world->bullets[i].position = position;
			world->bullets[i].velocity = velocity;

			muzzleFlashes[muzzleFlashCount] = position;
			muzzleFlashCount += 1;

			break;
		}
	}
}

bool BulletsUpdate(WorldState* world, float deltaTime)
{
	muzzleFlashCount = 0;
	impactCount = 0;

	bool explodedSomething = false;
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (world->bullets[i].isActive)
		{
			Projectile* bul = &world->bullets[i];
			Vector3 bulletDelta = Vector3Scale(bul->velocity, deltaTime);
			bul->position = Vector3Add(bul->position, bulletDelta);
			bul->lifeTime -= deltaTime;
			bul->isActive = bul->lifeTime > 0 && bul->position.y > 0;

			if (bul->position.y <= 0)
			{
				bul->isActive = false;
				Vector3 groundClampedPos = bul->position;
				groundClampedPos.y = 0;
				impacts[impactCount] = groundClampedPos;
				impactCount += 1;
				continue;
			}

			for (int b = 0; b < MAX_BUILDINGS; b++)
			{
				if (!world->buildings[b].active)
					continue;

				if (CheckCollisionBoxSphere(world->buildings[b].bounds, bul->position, 1))
				{
					world->buildings[b].active = false;
					bul->isActive = false;
					world->targetsDestroyed += 1;

					impacts[impactCount] = bul->position;
					impactCount += 1;
					explodedSomething |= true;

					break;
				}
			}

			for (int e = 0; e < MAX_ENEMIES; e++)
			{
				if (!world->enemyShips[e].isActive)
					continue;

				if (CheckCollisionBoxSphere(world->enemyShips[e].bounds, bul->position, 1))
				{
					world->enemyShips[e].isActive = false;
					bul->isActive = false;
					world->targetsDestroyed += 1;

					impacts[impactCount] = bul->position;
					impactCount += 1;
					explodedSomething |= true;

					break;
				}
			}
		}
	}
	return explodedSomething;
}

void BulletsDraw(WorldState* world)
{
	// Draw bullets.
	static const float radius = 0.8f;
	static const float length = 1.0f / 60.0f;
	static const Color color = {255, 0, 0, 255};
	BeginBlendMode(BLEND_ADDITIVE);
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (!world->bullets[i].isActive)
			continue;

		Projectile* b = &world->bullets[i];
		DrawCylinderEx(
			b->position,
			Vector3Add(b->position, Vector3Scale(b->velocity, length)),
			radius * 0.3f, radius * 0.3f, 6,
			WHITE);

		DrawCylinderEx(
			b->position,
			Vector3Add(b->position, Vector3Scale(b->velocity, length)),
			radius, radius, 6,
			color);
	}

	// Draw muzzle flashes.
	for (int i = 0; i < muzzleFlashCount; i++)
	{
		DrawSphere(muzzleFlashes[i], MUZZLE_FLASH_SIZE * 0.3f, WHITE);
		DrawSphere(muzzleFlashes[i], MUZZLE_FLASH_SIZE, color);
	}

	// Draw impacts.
	for (int i = 0; i < impactCount; i++)
	{
		DrawSphere(impacts[i], IMPACT_SIZE * 0.3f, WHITE);
		DrawSphere(impacts[i], IMPACT_SIZE, color);
	}

	EndBlendMode();
}
