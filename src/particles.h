#pragma once

#include <raylib.h>

#define MAX_PARTICLES 1000

enum ParticleType
{
	PARTICLE_LINE,
	PARTICLE_POINT,
	PARTICLE_CUBE,
};

typedef struct Particle
{
	Vector3 position;
	Vector3 velocity;
	Color color;
	float gravity;
	float drag;
	float size;
	float timeToLive;
	enum ParticleType type;
} Particle;

void EmitParticle(Particle* template);
void UpdateParticles(float deltaTime);
void DrawParticles();
