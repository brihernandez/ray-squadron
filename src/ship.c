#include "ship.h"

#include <raylib.h>
#include <raymath.h>

#include "world.h"
#include "audio.h"
#include "particles.h"

#include "smoothdamp.h"

static float GetRandomFloat(float min, float max)
{
	float val = (float)GetRandomValue(0, 1000);
	val /= 1000.f;
	return Lerp(min, max, val);
}

static ShipInput ShipInputNormalize(ShipInput input)
{
	input.pitch = Clamp(input.pitch, -1, 1);
	input.yaw = Clamp(input.yaw, -1, 1);
	input.roll = Clamp(input.roll, -1, 1);
	input.throttle = Clamp(input.throttle, -1, 1);
	return input;
}

static Matrix BuildTransformMatrix(Vector3 position, Quaternion rotation)
{
	Matrix rotationMat = QuaternionToMatrix(rotation);
	Matrix positionMat = MatrixTranslate(position.x, position.y, position.z);
	return MatrixMultiply(rotationMat, positionMat);
}

static Quaternion RotateQuaternionWithLocalAngularVelocity(Quaternion rotation, Vector3 localAngularVelocity, float deltaTime)
{
	rotation = QuaternionMultiply(
		rotation,
		QuaternionFromAxisAngle(
			(Vector3) { 1, 0, 0 },
			localAngularVelocity.x * deltaTime) );

	rotation = QuaternionMultiply(
		rotation,
		QuaternionFromAxisAngle(
			(Vector3) { 0, 1, 0 },
			localAngularVelocity.y* deltaTime));

	rotation = QuaternionMultiply(
		rotation,
		QuaternionFromAxisAngle(
			(Vector3) { 0, 0, 1 },
			localAngularVelocity.z* deltaTime));

	// Prevents building up of floating point errors over time.
	return QuaternionNormalize(rotation);
}

static void ShipUpdateTransform(Ship* ship)
{
	ship->transform = BuildTransformMatrix(ship->position, ship->rotation);
	ship->forward = Vector3RotateByQuaternion((Vector3) { 0, 0, 1 }, ship->rotation);
	ship->up = Vector3RotateByQuaternion((Vector3) { 0, 1, 0 }, ship->rotation);
	ship->right = Vector3RotateByQuaternion((Vector3) { 1, 0, 0 }, ship->rotation);
}

Ship ShipInit(ShipHandling handling, ShipWeapons weapons, int hp, bool isEnemy)
{
	Ship ship = {0};
	ship.rotation = (Quaternion){ 0.0f, 0.0f, 0.0f, 1.0f };
	ship.handling = handling;
	ship.weapons = weapons;
	ship.speed = handling.maxSpeed;
	ship.state = SHIPSTATE_FLIGHT;
	ship.isEnemy = isEnemy;
	ship.hp = hp;

	ShipUpdateTransform(&ship);
	ship.bounds = ShipCalculateBounds(ship.position);

	// Protect against array index out of bounds.
	if (ship.weapons.barrelCount > SHIP_MAX_BARRELS)
		ship.weapons.barrelCount = SHIP_MAX_BARRELS;

	return ship;
}

static void SpawnSparkParticles(Vector3 position)
{
	Particle p = {
		.position = position,
		.velocity = {0},
		.color = YELLOW,
		.gravity = 10,
		.drag = 1,
		.size = 5,
		.timeToLive = 5,
		.type = PARTICLE_LINE
	};
	for (int i = 0; i < 25; i++)
	{
		p.velocity.x = GetRandomFloat(-100, 100);
		p.velocity.y = GetRandomFloat(50, 300);
		p.velocity.z = GetRandomFloat(-100, 100);
		ParticlesEmit(&p);
	}
}

static void ShipPlayExplodeFX(Ship* ship, bool onGround)
{
	Vector3 explodePos = ship->position;
	if (onGround)
	{
		float shipHeight = ship->bounds.max.y - ship->bounds.min.y;
		explodePos.y += shipHeight;
	}
	SpawnSparkParticles(explodePos);
}

void ShipExplode(Ship* ship)
{
	ship->state = SHIPSTATE_CRASHING;

	ship->localAngularVelocity.x = GetRandomFloat(3, 5);
	ship->localAngularVelocity.y = GetRandomFloat(3, 5);
	ship->localAngularVelocity.z = GetRandomFloat(3, 5);
	ship->crashVelocity = Vector3Scale(ship->forward, ship->speed);

	ShipPlayExplodeFX(ship, false);
}

static bool ShipUpdateFlight(WorldState* world, Ship* ship, ShipInput input, float deltaTime)
{
	// Add autolevel to the input.
	input.roll -= ship->right.y / 2.0f;
	input = ShipInputNormalize(input);

	// Speed control.
	const ShipHandling* handling = &ship->handling;
	float smoothSpeed = handling->responsiveness;
	float targetSpeed = input.throttle >= 0
		? Remap(input.throttle, 0, 1, handling->cruiseSpeed, handling->maxSpeed)
		: Remap(input.throttle, -1, 0, handling->minSpeed, handling->cruiseSpeed);
	ship->speed = SmoothDamp(ship->speed, targetSpeed, smoothSpeed, deltaTime);

	// Cruise speed is the optimal turn rate. Faster/slower will incur a penalty to turn rate.
	const float turnPenalty = 0.67f;
	float turnSpeedRamp = ship->speed > handling->cruiseSpeed
		? Remap(ship->speed, handling->cruiseSpeed, handling->maxSpeed, 1.0f, turnPenalty)
		: Remap(ship->speed, handling->minSpeed, handling->cruiseSpeed, turnPenalty, 1.0f);

	// Ship rotation.
	ship->localAngularVelocity.x = SmoothDamp(
		ship->localAngularVelocity.x,
		input.pitch * handling->pitchRate * turnSpeedRamp,
		smoothSpeed, deltaTime);
	ship->localAngularVelocity.y = SmoothDamp(
		ship->localAngularVelocity.y,
		input.yaw * handling->yawRate * turnSpeedRamp,
		smoothSpeed, deltaTime);
	ship->localAngularVelocity.z = SmoothDamp(
		ship->localAngularVelocity.z, input.roll * handling->rollRate * turnSpeedRamp,
		smoothSpeed, deltaTime);

	ship->rotation = RotateQuaternionWithLocalAngularVelocity(
		ship->rotation, ship->localAngularVelocity,
		deltaTime);

	// Ship translation.
#ifdef _DEBUG
	if (IsKeyDown(KEY_SPACE))
		ship->speed = 0;
#endif

	Vector3 position = Vector3Add(ship->position, Vector3Scale(ship->forward, ship->speed * deltaTime));
	if (position.y < 2)
		position.y = 2;
	ShipSetPosition(ship, position);

	// Ship weapons.
	ShipWeapons* weapons = &ship->weapons;
	weapons->timeSinceLastShot += deltaTime;
	bool isReadyToFire = weapons->timeSinceLastShot > weapons->fireDelay && weapons->barrelCount > 0;
	if (input.isFiring && isReadyToFire)
	{
		Vector3 localFirePos = weapons->barrels[weapons->barrelIndex];
		Vector3 worldFirePos = Vector3Transform(localFirePos, ship->transform);
		Vector3 bulletVelocity = Vector3Scale(ship->forward, ship->speed + weapons->muzzleVelocity);
		BulletsFire(world, worldFirePos, bulletVelocity, 1, ship->isEnemy);
		weapons->barrelIndex = (weapons->barrelIndex + 1) % weapons->barrelCount;
		weapons->timeSinceLastShot = 0;

		float volume = ship->isEnemy ? 0.8f : 1.0f;
		AudioPlaySoundAt(weapons->fireSound, worldFirePos, 100, 250, volume);
	}

	// Returns true if the ship exploded, but that doesn't happen here.
	return false;
}

static bool ShipUpdateCrashing(WorldState* world, Ship* ship, float deltaTime)
{
	if (ship->position.y <= 0)
	{
		ship->state = SHIPSTATE_INACTIVE;
		ShipPlayExplodeFX(ship, true);
		return true;
	}

	for (int i = 0; i < MAX_BUILDINGS; i++)
	{
		Building* building = &world->buildings[i];
		if (!building->active)
			continue;
		if (CheckCollisionBoxes(ship->bounds, building->bounds))
		{
			ship->state = SHIPSTATE_INACTIVE;
			WorldExplodeBuilding(world, i);
			ShipPlayExplodeFX(ship, true);
			return true;
		}
	}

	const float terminalVelocity = -350.0f;
	ship->crashVelocity.y -= 50.0f * deltaTime;
	if (ship->crashVelocity.y < terminalVelocity)
		ship->crashVelocity.y = terminalVelocity;
	Vector3 position = Vector3Add(ship->position, Vector3Scale(ship->crashVelocity, deltaTime));

	ship->rotation = RotateQuaternionWithLocalAngularVelocity(
		ship->rotation, ship->localAngularVelocity,
		deltaTime);

	ShipSetPosition(ship, position);

	return false;
}

bool ShipUpdate(WorldState* world, Ship* ship, ShipInput input, float deltaTime)
{
	bool wasExploded = false;
	switch (ship->state)
	{
		case SHIPSTATE_FLIGHT:
			wasExploded = ShipUpdateFlight(world, ship, input, deltaTime);
			break;
		case SHIPSTATE_CRASHING:
			wasExploded = ShipUpdateCrashing(world, ship, deltaTime);
			break;
	}

	return wasExploded;
}

void ShipDraw(Ship* ship, Model* model, Color color)
{
	float bankAngle = 30.f * DEG2RAD;
	float visualBank = Remap(ship->localAngularVelocity.y, -ship->handling.yawRate, ship->handling.yawRate, bankAngle, -bankAngle);
	Quaternion visualRotation = QuaternionMultiply(QuaternionFromAxisAngle(ship->forward, visualBank), ship->rotation);
	model->transform = BuildTransformMatrix(ship->position, visualRotation);
	DrawModel(*model, Vector3Zero(), 1, color);

#ifdef SHOW_HITBOXES
	DrawBoundingBox(ship->bounds, RED);
#endif
}

void ShipSetPosition(Ship* ship, Vector3 position)
{
	ship->position = position;
	ship->bounds = ShipCalculateBounds(position);
	ShipUpdateTransform(ship);
}

BoundingBox ShipCalculateBounds(Vector3 position)
{
	float size = 8;
	BoundingBox b = (BoundingBox){
		.min = (Vector3){position.x - size, position.y - size, position.z - size},
		.max = (Vector3){position.x + size, position.y + size, position.z + size},
	};
	return b;
}

void EnemyControllerUpdate(EnemyController* enemy, Ship* ship, float deltaTime)
{
	enemy->thinkCooldown -= deltaTime;

	float deltaHeight = enemy->targetAltitude - ship->position.y;
	float targetForwardY = Remap(deltaHeight, 50.f, -50.f, 0.3f, -0.3f);
	//targetForwardY = Clamp(targetForwardY, -0.3f, 0.3f);

	float forwardY = ship->forward.y;
	float deltaForwardY = targetForwardY - forwardY;
	enemy->input.pitch = -deltaForwardY;

	if (enemy->thinkCooldown <= 0)
	{
		enemy->input.yaw = GetRandomValue(-100, 100) / 100.0f;
		enemy->input.roll = enemy->input.yaw * -0.2f;
		enemy->thinkCooldown = GetRandomValue(200, 400) / 100.0f;
		enemy->input.isFiring = !enemy->input.isFiring;
	}
}
