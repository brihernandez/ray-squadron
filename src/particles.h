#pragma once

#include <raylib.h>

#define MAX_PARTICLES 1000

typedef enum ParticleType
{
	PARTICLE_LINE,
	PARTICLE_POINT,
	PARTICLE_CUBE,
} ParticleType;

typedef struct Particle
{
	Vector3 position;
	Vector3 velocity;
	Color color;
	float gravity;
	float drag;
	float size;
	float timeToLive;
	ParticleType type;
} Particle;

void ParticlesEmit(Particle* template);
void ParticlesClear();
void ParticlesUpdate(float deltaTime);
void ParticlesDraw();
