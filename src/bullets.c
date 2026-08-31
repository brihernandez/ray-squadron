#include "bullets.h"
#include "world.h"

#include <raymath.h>

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
			break;
		}
	}

}

bool BulletsUpdate(WorldState* world, float deltaTime)
{
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

			for (int b = 0; b < MAX_BUILDINGS; b++)
			{
				if (!world->buildings[b].active)
					continue;

				if (CheckCollisionBoxSphere(world->buildings[b].bounds, bul->position, 1))
				{
					world->buildings[b].active = false;
					bul->isActive = false;
					world->targetsDestroyed += 1;
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
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (!world->bullets[i].isActive)
			continue;
		DrawCube(
			world->bullets[i].position,
			1.5, 1.5, 1.5,
			YELLOW);
	}
}
