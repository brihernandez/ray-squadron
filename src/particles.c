#include "particles.h"

#include <raylib.h>
#include <raymath.h>

#include <string.h>

static Particle particles[MAX_PARTICLES] = {0};
static int nextParticleIndex = 0;

static const Vector3 gravity = {0, -10, 0};
static const float lineLength = 1.f / 30.f;

void ParticlesEmit(Particle* template)
{
	// It's not an error per se, but if the game runs out of particles, it should be logged because
	// it means a particle somewhere is being overwritten and that can look bad.
	if (particles[nextParticleIndex].timeToLive > 0)
		TraceLog(LOG_WARNING, "Newly emitted particle will overwrite currently active particle!");

	Particle* p = &particles[nextParticleIndex];
	*p = *template;
	nextParticleIndex = (nextParticleIndex + 1) % MAX_PARTICLES;
}

void ParticlesClear()
{
	for (int i = 0; i < MAX_PARTICLES; i++)
		memset(&particles[i], 0, sizeof(Particle));
}

void ParticlesUpdate(float deltaTime)
{
	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		Particle* p = &particles[i];
		if (p->timeToLive <= 0)
			continue;

		p->velocity = Vector3Add(p->velocity, Vector3Scale(gravity, p->gravity * deltaTime));
		p->velocity = Vector3Add(p->velocity, Vector3Scale(p->velocity, -p->drag * deltaTime));
		p->position = Vector3Add(p->position, Vector3Scale(p->velocity, deltaTime));
		p->timeToLive -= deltaTime;
	}
}

void ParticlesDraw()
{
	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		Particle* p = &particles[i];
		if (p->timeToLive <= 0)
			continue;

		switch (p->type)
		{
			case PARTICLE_LINE:
				// Trailing line
				DrawLine3D(
					p->position,
					Vector3Subtract(p->position, Vector3Scale(p->velocity, lineLength)),
					p->color);
				// Leading line
				//DrawLine3D(
				//	Vector3Add(p->position, Vector3Scale(p->velocity, lineLength)),
				//	p->position,
				//	p->color);
				break;
			case PARTICLE_POINT:
				DrawPoint3D(p->position, p->color);
				break;
			case PARTICLE_CUBE:
				DrawCube(p->position, p->size, p->size, p->size, p->color);
				break;
		}
	}
}
