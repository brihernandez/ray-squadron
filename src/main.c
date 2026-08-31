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

#include "world.h"
#include "ship.h"
#include "bullets.h"

#include "smoothdamp.h"

#define BUILDING_SIZE 20

typedef enum GameScreen {
	TITLE,
	GAMEPLAY,
	ENDING,
} GameScreen;

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

	Vector3 playerStartPosition = {0, 100, 0};

	// =======================================
	// World declare
	// =======================================

	WorldState world = {0};
	// =======================================
	// Weapons init
	// =======================================
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
					world.playerShip = ShipInit(playerShipHandling, playerShipWeapons);
					world.playerShip.position = playerStartPosition;

					for (int i = 0; i < MAX_BUILDINGS; i++)
					{
						Building* b = &world.buildings[i];
						b->position = (Vector3){
							(float)GetRandomValue(-500, 500),
							BUILDING_SIZE,
							(float)GetRandomValue(-500, 500)
						};
						b->active = true;
						b->bounds = (BoundingBox){
							.min = (Vector3) {b->position.x - BUILDING_SIZE, b->position.y - BUILDING_SIZE, b->position.z - BUILDING_SIZE},
							.max = (Vector3) {b->position.x + BUILDING_SIZE, b->position.y + BUILDING_SIZE, b->position.z + BUILDING_SIZE},
						};
					}

					for (int i = 0; i < MAX_ENEMIES; i++)
					{
						world.enemyShips[i] = ShipInit(playerShipHandling, playerShipWeapons);

						world.enemyShips[i].position = (Vector3){
							(float)GetRandomValue(-500, 500),
							(float)GetRandomValue(50, 200),
							(float)GetRandomValue(-500, 500),
						};

						Quaternion randomRotation = QuaternionFromEuler(0, GetRandomValue(0, 360) * DEG2RAD, 0);
						world.enemyShips[i].rotation = randomRotation;
						world.enemyControllers[i].targetAltitude = (float)GetRandomValue(100, 200);
					}

					BulletsInit(&world);
					// Use this to tell if coming from the main menu or not.
					if (world.targetsDestroyed > 0)
						PlayMusicStream(bgm);

					currentScreen = GAMEPLAY;
					world.targetsDestroyed = 0;

					PlaySound(sfx_confirm);
				}
				break;
			}
			case GAMEPLAY:
			{
				float deltaTime = GetFrameTime();

				// Rotate
				ShipInput input = {0};
				if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
					input.pitch += 1;
				}
				if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
					input.pitch -= 1;
				}
				if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
					input.roll -= 0.2f;
					input.yaw += 1;
				}
				if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
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

				ShipUpdate(&world.playerShip, input, deltaTime);

				// Position chase camera.
				Vector3 camPos = world.playerShip.position;
				camPos = Vector3Add(camPos, Vector3Scale(world.playerShip.forward, -40));
				camPos = Vector3Add(camPos, Vector3Scale(world.playerShip.up, 10));

				// Apply to the raylib camera.
				camera.position = Vector3SmoothDamp(camera.position, camPos, 10, deltaTime);
				camera.target = Vector3Add(world.playerShip.position, Vector3Scale(world.playerShip.forward, 225));
				camera.up = world.playerShip.up;

				// Update weapons (firing)
				timeSinceLastShot += deltaTime;
				if ((IsKeyDown(KEY_LEFT_CONTROL) || IsMouseButtonDown(0)) && timeSinceLastShot >= fireDelay)
				{
					Vector3 bulletPosition = {0};
					if (barrelIndex == 0)
					{
						Vector3 firePoint = Vector3Scale(world.playerShip.forward, 5);
						firePoint = Vector3Add(Vector3Scale(world.playerShip.right, -2), firePoint);
						bulletPosition = Vector3Add(world.playerShip.position, firePoint);
						barrelIndex = 1;
					}
					else
					{
						Vector3 firePoint = Vector3Scale(world.playerShip.forward, 5);
						firePoint = Vector3Add(Vector3Scale(world.playerShip.right, 2), firePoint);
						bulletPosition = Vector3Add(world.playerShip.position, firePoint);
						barrelIndex = 0;
					}
					Vector3 bulletVelocity = Vector3Scale(world.playerShip.forward, world.playerShip.speed + muzzleVelocity);
					timeSinceLastShot = 0;
					PlaySound(sfx_shoot);
					float playerBulletLifetime = 1;
					BulletsFire(&world, bulletPosition, bulletVelocity, playerBulletLifetime);
					break;
				}

				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (world.enemyShips[i].isActive == false)
						continue;

					EnemyControllerUpdate(&world.enemyControllers[i], &world.enemyShips[i], deltaTime);
					ShipUpdate(&world.enemyShips[i], world.enemyControllers[i].input, deltaTime);

					// Enemy ships don't have a way to steer smartly so just constrain them to the world.
					Ship* e = &world.enemyShips[i];
					if (e->position.x > 500) e->position.x = 500;
					if (e->position.x < -500) e->position.x = -500;
					if (e->position.y > 200) e->position.y = 200;
					if (e->position.y < 10) e->position.y = 10;
					if (e->position.z > 500) e->position.z = 500;
					if (e->position.z < -500) e->position.z = -500;
				}

				bool wasSomethingExplodedThisFrame = BulletsUpdate(&world, deltaTime);
				if (wasSomethingExplodedThisFrame)
					PlaySound(sfx_explode);

				// Check win condition.
				if (world.targetsDestroyed >= MAX_BUILDINGS + MAX_ENEMIES)
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
						if (!world.buildings[i].active)
							continue;

						DrawCube(world.buildings[i].position, BUILDING_SIZE * 2, BUILDING_SIZE * 2, BUILDING_SIZE * 2, (Color) { 80, 80, 80, 255 });
						DrawCubeWires(world.buildings[i].position, BUILDING_SIZE * 2, BUILDING_SIZE * 2, BUILDING_SIZE * 2, BLACK);
					}

					// Draw enemies.
					for (int i = 0; i < MAX_ENEMIES; i++)
					{
						if (!world.enemyShips[i].isActive)
							continue;

						ShipDraw(&world.enemyShips[i], &mdl_ship, RED);
					}

					BulletsDraw(&world);
					ShipDraw(&world.playerShip, &mdl_ship, WHITE);

				} EndMode3D();

				// HUD
				Vector3 cameraForward = Vector3Subtract(camera.target, camera.position);
				for (int i = 0; i < MAX_BUILDINGS; i++)
				{
					Vector3 cameraToBuilding = Vector3Subtract(world.buildings[i].position, camera.position);
					if (Vector3DotProduct(cameraForward, cameraToBuilding) < 0)
						continue;

					Vector2 buildingScreenPos = GetWorldToScreen(world.buildings[i].position, camera);
					DrawText(TextFormat("%d", i), (int)buildingScreenPos.x, (int)buildingScreenPos.y, 10, MAGENTA);
				}

				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (world.enemyShips[i].isActive == false)
						continue;

					Vector3 cameraToEnemy = Vector3Subtract(world.enemyShips[i].position, camera.position);
					if (Vector3DotProduct(cameraForward, cameraToEnemy) < 0)
						continue;

					Vector2 screenPos = GetWorldToScreen(world.enemyShips[i].position, camera);
					DrawText(TextFormat("%d", (int)world.enemyShips[i].position.y), (int)screenPos.x, (int)screenPos.y, 10, MAGENTA);
				}

				// Crosshairs
				Vector3 forward = Vector3RotateByQuaternion((Vector3) { 0, 0, 1 }, world.playerShip.rotation);
				Vector3 xhairPos = Vector3Add(world.playerShip.position, Vector3Scale(forward, 75));
				Vector2 xhairScreenPos = GetWorldToScreen(xhairPos, camera);
				DrawCrosshair(xhairScreenPos, 40);
				xhairPos = Vector3Add(world.playerShip.position, Vector3Scale(forward, 225));
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
					DrawTextCentered(TextFormat("TARGETS DESTROYED: %d", world.targetsDestroyed), ScreenWidth / 2, 100, 40, ORANGE);
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
