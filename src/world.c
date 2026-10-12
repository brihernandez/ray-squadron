#include "world.h"
#include "particles.h"

#include <raymath.h>

static float GetRandomFloat(float min, float max)
{
	float val = (float)GetRandomValue(0, 1000);
	val /= 1000.f;
	return Lerp(min, max, val);
}

void WorldSpawnExplosionParticles(Vector3 position)
{
	Particle p = {
		.position = position,
		.velocity = {0},
		.color = GRAY,
		.gravity = 10,
		.drag = 1,
		.size = 5,
		.timeToLive = 5,
		.type = PARTICLE_CUBE
	};
	for (int i = 0; i < 25; i++)
	{
		p.velocity.x = GetRandomFloat(-100, 100);
		p.velocity.y = GetRandomFloat(25, 150);
		p.velocity.z = GetRandomFloat(-100, 100);
		ParticlesEmit(&p);
	}
}

void WorldExplodeBuilding(WorldState* world, int index)
{
	if (!world->buildings[index].active)
		return;

	world->targetsDestroyed += 1;
	world->buildings[index].active = false;
	WorldSpawnExplosionParticles(world->buildings[index].position);
}
