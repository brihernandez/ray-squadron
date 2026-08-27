/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "resource_dir.h"	// utility header for SearchAndSetResourceDir

#include "ship.h"

#include "smoothdamp.h"

#define BUILDING_SIZE 20
#define MAX_BUILDINGS 15
#define MAX_ENEMIES 10
#define MAX_BULLETS 100
#define BULLET_LIFETIME 1

typedef enum GameScreen {
	TITLE,
	GAMEPLAY,
	ENDING,
} GameScreen;

typedef struct Projectile {
	Vector3 position;
	Vector3 velocity;
	float lifeTime;
	bool active;
} Projectile;

typedef struct Building {
	Vector3 position;
	BoundingBox bounds;
	bool active;
} Building;

typedef struct Enemy {
	Vector3 position;
	Vector3 velocity;
	BoundingBox bounds;
	bool active;
} Enemy;

BoundingBox EnemyCalculateBounds(Enemy e)
{
	float size = 8;
	BoundingBox b = (BoundingBox){
		//.min = Vector3Subtract(e.position, (Vector3) { size, size, size }),
		//.max = Vector3Add(e.position, (Vector3) { size, size, size }),
		.min = (Vector3){e.position.x - size, e.position.y - size, e.position.z - size},
		.max = (Vector3){e.position.x + size, e.position.y + size, e.position.z + size},
	};
	return b;
}

void DrawCrosshair(Vector2 screenPos, float radius);
void DrawTextCentered(const char* message, int x, int y, int size, Color color);
Matrix MatrixBuildTransform(Vector3 position, Quaternion rotation);
void DrawGridColored(int slices, float spacing, Color color);

int main()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);

	// Create the window and OpenGL context
	const int ScreenWidth = 800;
	const int ScreenHeight = 600;
	InitWindow(ScreenWidth, ScreenHeight, "Ray Squadron");
	InitAudioDevice();

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	Music bgm = LoadMusicStream("realtrees.ogg");
	bgm.looping = true;
	PlayMusicStream(bgm);

	Sound sfx_confirm = LoadSound("confirm.ogg");
	Sound sfx_shoot = LoadSound("shoot.ogg");
	Sound sfx_explode = LoadSound("explode.ogg");
	Sound sfx_win = LoadSound("win.ogg");

	Model mdl_ship = LoadModel("ship.glb");

	// =======================================
	// Player init
	// =======================================

	ShipHandling playerShipHandling = {
		.pitchRate = 1.5f,
		.yawRate = 1.5f,
		.rollRate = 3.0f,
		.maxSpeed = 80,
		.minSpeed = 30,
	};
	ShipWeapons playerShipWeapons = {
		.fireDelay = 0.10f,
		.muzzleVelocity = 800,
	};

	Vector3 startPosition = {0, 100, 0};

	Ship playerShip = ShipInit(playerShipHandling, playerShipWeapons);
	playerShip.position = startPosition;

	// =======================================
	// Enemies init
	// =======================================
	Building buildings[MAX_BUILDINGS] = {0};

	EnemyController enemyControllers[MAX_ENEMIES] = {0};
	Ship enemyShips[MAX_ENEMIES] = {0};

	// =======================================
	// Weapons init
	// =======================================
	Projectile bullets[MAX_BULLETS] = {0};
	float fireDelay = 0.10f;
	float timeSinceLastShot = 0;
	float muzzleVelocity = 800;
	int barrelIndex = 0;

	// =======================================
	// Camera Init
	// =======================================
	Camera3D camera = {0};
	camera.fovy = 50;
	camera.projection = CAMERA_PERSPECTIVE;

	// =======================================
	// Main Loop
	// =======================================

	GameScreen currentScreen = TITLE;
	int targetsDestroyed = 0;

	while (!WindowShouldClose())
	{
		// =======================================
		// Update
		// =======================================

		UpdateMusicStream(bgm);

		if (IsKeyPressed(KEY_F11))
			ToggleFullscreen();

		switch (currentScreen) {
			case TITLE:
			case ENDING:
			{
				if (IsKeyPressed(KEY_ENTER))
				{
					playerShip = ShipInit(playerShipHandling, playerShipWeapons);
					playerShip.position = startPosition;

					for (int i = 0; i < MAX_BUILDINGS; i++)
					{
						buildings[i].position = (Vector3){
							(float)GetRandomValue(-500, 500),
							BUILDING_SIZE,
							(float)GetRandomValue(-500, 500)
						};
						buildings[i].active = true;
						buildings[i].bounds = (BoundingBox){
							.min = (Vector3) {buildings[i].position.x - BUILDING_SIZE, buildings[i].position.y - BUILDING_SIZE, buildings[i].position.z - BUILDING_SIZE},
							.max = (Vector3) {buildings[i].position.x + BUILDING_SIZE, buildings[i].position.y + BUILDING_SIZE, buildings[i].position.z + BUILDING_SIZE},
						};
					}

					for (int i = 0; i < MAX_ENEMIES; i++)
					{
						enemyShips[i] = ShipInit(playerShipHandling, playerShipWeapons);

						enemyShips[i].position = (Vector3){
							(float)GetRandomValue(-500, 500),
							(float)GetRandomValue(50, 200),
							(float)GetRandomValue(-500, 500),
						};

						Quaternion randomRotation = QuaternionFromEuler(0, GetRandomValue(0, 360) * DEG2RAD, 0);
						enemyShips[i].rotation = randomRotation;
						enemyControllers[i].targetAltitude = (float)GetRandomValue(100, 200);
					}

					for (int i = 0; i < MAX_BULLETS; i++)
					{
						bullets[i].active = false;
					}

					// Use this to tell if coming from the main menu or not.
					if (targetsDestroyed > 0)
						PlayMusicStream(bgm);

					currentScreen = GAMEPLAY;
					targetsDestroyed = 0;

					PlaySound(sfx_confirm);
				}
				break;
			}
			case GAMEPLAY:
			{
				float deltaTime = GetFrameTime();

				// Rotate
				ShipInput input = {0};
				if (IsKeyDown(KEY_W)) {
					input.pitch += 1;
				}
				if (IsKeyDown(KEY_S)) {
					input.pitch -= 1;
				}
				if (IsKeyDown(KEY_A)) {
					input.roll -= 0.2f;
					input.yaw += 1;
				}
				if (IsKeyDown(KEY_D)) {
					input.roll += 0.2f;
					input.yaw -= 1;
				}
				if (IsKeyDown(KEY_Q)) {
					input.roll -= 1;
				}
				if (IsKeyDown(KEY_E)) {
					input.roll += 1;
				}
				input.isFiring = IsKeyDown(KEY_LEFT_CONTROL) || IsMouseButtonDown(0);

				ShipUpdate(&playerShip, input, deltaTime);

				// Position chase camera.
				Vector3 camPos = playerShip.position;
				camPos = Vector3Add(camPos, Vector3Scale(playerShip.forward, -40));
				camPos = Vector3Add(camPos, Vector3Scale(playerShip.up, 10));

				// Apply to the raylib camera.
				camera.position = Vector3SmoothDamp(camera.position, camPos, 10, deltaTime);
				camera.target = Vector3Add(playerShip.position, Vector3Scale(playerShip.forward, 225));
				camera.up = playerShip.up;

				// Update weapons (firing)
				timeSinceLastShot += deltaTime;
				if ((IsKeyDown(KEY_LEFT_CONTROL) || IsMouseButtonDown(0)) && timeSinceLastShot >= fireDelay)
				{
					// Find first inactive bullet and use it to spawn.
					// This whole thing can be done much better.
					for (int i = 0; i < MAX_BULLETS; i++)
					{
						if (!bullets[i].active)
						{
							bullets[i].active = true;
							bullets[i].lifeTime = BULLET_LIFETIME;
							if (barrelIndex == 0)
							{
								Vector3 firePoint = Vector3Scale(playerShip.forward, 5);
								firePoint = Vector3Add(Vector3Scale(playerShip.right, -2), firePoint);
								bullets[i].position = Vector3Add(playerShip.position, firePoint);
								barrelIndex = 1;
							}
							else
							{
								Vector3 firePoint = Vector3Scale(playerShip.forward, 5);
								firePoint = Vector3Add(Vector3Scale(playerShip.right, 2), firePoint);
								bullets[i].position = Vector3Add(playerShip.position, firePoint);
								barrelIndex = 0;
							}
							bullets[i].velocity = Vector3Scale(playerShip.forward, playerShip.speed + muzzleVelocity);
							timeSinceLastShot = 0;
							PlaySound(sfx_shoot);
							break;
						}
					}
				}

				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (enemyShips[i].isActive == false)
						continue;

					EnemyControllerUpdate(&enemyControllers[i], &enemyShips[i], deltaTime);
					ShipUpdate(&enemyShips[i], enemyControllers[i].input, deltaTime);

					// Enemy ships don't have a way to steer smartly so just constrain them to the world.
					Ship* e = &enemyShips[i];
					if (e->position.x > 500) e->position.x = 500;
					if (e->position.x < -500) e->position.x = -500;
					if (e->position.y > 200) e->position.y = 200;
					if (e->position.y < 10) e->position.y = 10;
					if (e->position.z > 500) e->position.z = 500;
					if (e->position.z < -500) e->position.z = -500;
				}

				// Update weapons (bullet movement)
				for (int i = 0; i < MAX_BULLETS; i++)
				{
					if (bullets[i].active)
					{
						Vector3 bulletDelta = Vector3Scale(bullets[i].velocity, deltaTime);
						bullets[i].position = Vector3Add(bullets[i].position, bulletDelta);
						bullets[i].lifeTime -= deltaTime;
						bullets[i].active = bullets[i].lifeTime > 0 && bullets[i].position.y > 0;

						for (int b = 0; b < MAX_BUILDINGS; b++)
						{
							if (!buildings[b].active)
								continue;

							if (CheckCollisionBoxSphere(buildings[b].bounds, bullets[i].position, 1))
							{
								buildings[b].active = false;
								bullets[i].active = false;
								targetsDestroyed += 1;
								PlaySound(sfx_explode);
								break;
							}
						}

						for (int e = 0; e < MAX_ENEMIES; e++)
						{
							if (!enemyShips[e].isActive)
								continue;

							if (CheckCollisionBoxSphere(enemyShips[e].bounds, bullets[i].position, 1))
							{
								enemyShips[e].isActive = false;
								bullets[i].active = false;
								targetsDestroyed += 1;
								PlaySound(sfx_explode);
								break;
							}
						}
					}
				}

				// Check win condition.
				if (targetsDestroyed >= MAX_BUILDINGS + MAX_ENEMIES)
				{
					currentScreen = ENDING;
					StopMusicStream(bgm);
					PlaySound(sfx_win);
				}

				break;
			}
		}

		// =======================================
		// Render
		// =======================================

		BeginDrawing();
		{
			if (currentScreen == TITLE)
			{
				ClearBackground((Color) { 16, 32, 64, 255 });
				DrawTextCentered("Destroy all targets!", ScreenWidth / 2, ScreenHeight / 2 - 30, 60, ORANGE);
				DrawTextCentered("Press [ENTER] to start.", ScreenWidth / 2, ScreenHeight / 2 + 30, 20, ORANGE);
			}
			else
			{
				Color sky = {135, 180, 215, 255};
				Color ground = {0, 117, 44, 255};
				ClearBackground(sky);

				BeginMode3D(camera);
				{
					// Draw the background on a separate pass so that depth can be disabled.
					rlDisableDepthMask();
					DrawPlane(Vector3Zero(), (Vector2) { 10000, 10000 }, ground);
					DrawGridColored(100, 100, ColorLerp(ground, WHITE, 0.3f));

				} EndMode3D();

				BeginMode3D(camera);
				{
					// Depth needs to be reenabled.
					rlEnableDepthMask();

					// Draw buildings.
					for (int i = 0; i < MAX_BUILDINGS; i++)
					{
						if (!buildings[i].active)
							continue;

						DrawCube(buildings[i].position, BUILDING_SIZE * 2, BUILDING_SIZE * 2, BUILDING_SIZE * 2, (Color) { 80, 80, 80, 255 });
						DrawCubeWires(buildings[i].position, BUILDING_SIZE * 2, BUILDING_SIZE * 2, BUILDING_SIZE * 2, BLACK);
					}

					// Draw enemies.
					for (int i = 0; i < MAX_ENEMIES; i++)
					{
						if (!enemyShips[i].isActive)
							continue;

						ShipDraw(&enemyShips[i], &mdl_ship, RED);
					}

					// Draw bullets.
					for (int i = 0; i < MAX_BULLETS; i++)
					{
						if (!bullets[i].active)
							continue;
						DrawCube(
							bullets[i].position,
							1.5, 1.5, 1.5,
							YELLOW);
					}

					// Draw the plane.
					ShipDraw(&playerShip, &mdl_ship, WHITE);

				} EndMode3D();

				// HUD
				Vector3 cameraForward = Vector3Subtract(camera.target, camera.position);
				for (int i = 0; i < MAX_BUILDINGS; i++)
				{
					Vector3 cameraToBuilding = Vector3Subtract(buildings[i].position, camera.position);
					if (Vector3DotProduct(cameraForward, cameraToBuilding) < 0)
						continue;

					Vector2 buildingScreenPos = GetWorldToScreen(buildings[i].position, camera);
					DrawText(TextFormat("%d", i), (int)buildingScreenPos.x, (int)buildingScreenPos.y, 10, MAGENTA);
				}

				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (enemyShips[i].isActive == false)
						continue;

					Vector3 cameraToEnemy = Vector3Subtract(enemyShips[i].position, camera.position);
					if (Vector3DotProduct(cameraForward, cameraToEnemy) < 0)
						continue;

					Vector2 screenPos = GetWorldToScreen(enemyShips[i].position, camera);
					DrawText(TextFormat("%d", (int)enemyShips[i].position.y), (int)screenPos.x, (int)screenPos.y, 10, MAGENTA);
				}

				// Crosshairs
				Vector3 forward = Vector3RotateByQuaternion((Vector3) { 0, 0, 1 }, playerShip.rotation);
				Vector3 xhairPos = Vector3Add(playerShip.position, Vector3Scale(forward, 75));
				Vector2 xhairScreenPos = GetWorldToScreen(xhairPos, camera);
				DrawCrosshair(xhairScreenPos, 40);
				xhairPos = Vector3Add(playerShip.position, Vector3Scale(forward, 225));
				xhairScreenPos = GetWorldToScreen(xhairPos, camera);
				DrawCrosshair(xhairScreenPos, 13);

				if (currentScreen == ENDING)
				{
					// Win message on completion
					DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Fade(BLACK, 0.8f));
					DrawTextCentered("YOU BEAT THE GAME!", ScreenWidth / 2, ScreenHeight / 2 - 30, 60, ORANGE);
					DrawTextCentered("Press [ENTER] to play again.", ScreenWidth / 2, ScreenHeight / 2 + 30, 20, ORANGE);
				}
				else
				{
					DrawTextCentered(TextFormat("TARGETS DESTROYED: %d", targetsDestroyed), ScreenWidth / 2, 100, 40, ORANGE);
				}

				BeginBlendMode(BLEND_ADDITIVE);
				{
					DrawText(TextFormat("%d", GetFPS()), 10, 10, 10, GREEN);
				} EndBlendMode();
			}
		} EndDrawing();
	}

	// cleanup
	// unload our texture so it can be cleaned up
	//UnloadTexture(wabbit);

	CloseAudioDevice();
	CloseWindow();
	return 0;
}

void DrawCrosshair(Vector2 screenPos, float radius)
{
	Color color = RED;
	DrawCircleLinesV(screenPos, radius, color);
	DrawLine(
		screenPos.x - radius, screenPos.y,
		screenPos.x - radius / 1.5f, screenPos.y,
		color);
	DrawLine(
		screenPos.x + radius, screenPos.y,
		screenPos.x + radius / 1.5f, screenPos.y,
		color);
}

void DrawTextCentered(const char* message, int x, int y, int size, Color color)
{
	x -= MeasureText(message, size) / 2;
	y -= size / 2;
	DrawText(message, x, y, size, color);
}

Matrix MatrixBuildTransform(Vector3 position, Quaternion rotation)
{
	Matrix transform = MatrixTranslate(position.x, position.y, position.z);
	transform = MatrixMultiply(QuaternionToMatrix(rotation), transform);
	return transform;
}

void DrawGridColored(int slices, float spacing, Color color)
{
	int halfSlices = slices / 2;
	Vector4 c = ColorNormalize(color);

	rlBegin(RL_LINES);
	for (int i = -halfSlices; i <= halfSlices; i++)
	{
		rlColor3f(c.x, c.y, c.z);

		rlVertex3f((float)i * spacing, 0.0f, (float)-halfSlices * spacing);
		rlVertex3f((float)i * spacing, 0.0f, (float)halfSlices * spacing);

		rlVertex3f((float)-halfSlices * spacing, 0.0f, (float)i * spacing);
		rlVertex3f((float)halfSlices * spacing, 0.0f, (float)i * spacing);
	}
	rlEnd();
}
