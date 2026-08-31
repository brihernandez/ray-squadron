#include "bullets.h"

#include <raymath.h>

#define MAX_BULLETS 100
Projectile bullets[MAX_BULLETS] = {0};

void BulletsInit()
{
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		bullets[i].isActive = false;
	}
}

void BulletsFire(Vector3 position, Vector3 velocity, float timeToLive)
{
	// Find first inactive bullet and use it to spawn.
	// This whole thing can be done much better.
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (!bullets[i].isActive)
		{
			bullets[i].isActive = true;
			bullets[i].lifeTime = timeToLive;
			bullets[i].position = position;
			bullets[i].velocity = velocity;
			break;
		}
	}

}

bool BulletsUpdate(WorldState* world, float deltaTime)
{
	bool explodedSomething = false;
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (bullets[i].isActive)
		{
			Vector3 bulletDelta = Vector3Scale(bullets[i].velocity, deltaTime);
			bullets[i].position = Vector3Add(bullets[i].position, bulletDelta);
			bullets[i].lifeTime -= deltaTime;
			bullets[i].isActive = bullets[i].lifeTime > 0 && bullets[i].position.y > 0;

			for (int b = 0; b < MAX_BUILDINGS; b++)
			{
				if (!world->buildings[b].active)
					continue;

				if (CheckCollisionBoxSphere(world->buildings[b].bounds, bullets[i].position, 1))
				{
					world->buildings[b].active = false;
					bullets[i].isActive = false;
					world->targetsDestroyed += 1;
					explodedSomething |= true;
					break;
				}
			}

			for (int e = 0; e < MAX_ENEMIES; e++)
			{
				if (!world->enemyShips[e].isActive)
					continue;

				if (CheckCollisionBoxSphere(world->enemyShips[e].bounds, bullets[i].position, 1))
				{
					world->enemyShips[e].isActive = false;
					bullets[i].isActive = false;
					world->targetsDestroyed += 1;
					explodedSomething |= true;
					break;
				}
			}
		}
	}
	return explodedSomething;
}

void BulletsDraw()
{
	// Draw bullets.
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (!bullets[i].isActive)
			continue;
		DrawCube(
			bullets[i].position,
			1.5, 1.5, 1.5,
			YELLOW);
	}
}
