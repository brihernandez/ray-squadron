/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include "world.h"
#include "ship.h"
#include "bullets.h"
#include "particles.h"

#include "resource_dir.h"
#include "smoothdamp.h"

#include <stdio.h>

#define TURRET_COUNT 30
#define BUILDING_SIZE 20

const int RenderWidth = 800;
const int RenderHeight = 600;
bool isPointFiltered = false;

void UpdateRenderFiltering(RenderTexture2D* rt, bool usePoint)
{
	SetTextureFilter(rt->texture, usePoint ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);
}

typedef enum GameScreen {
	TITLE,
	GAMEPLAY,
	ENDING,
} GameScreen;

static float GetRandomFloat(float min, float max)
{
	float val = (float)GetRandomValue(0, 1000);
	val /= 1000.f;
	return Lerp(min, max, val);
}

void DrawCrosshair(Vector2 screenPos, float radius);
void DrawTextCentered(const char* message, int x, int y, int size, Color color);
void DrawText3D(Camera camera, Vector3 position, const char* text, Color color);
Matrix MatrixBuildTransform(Vector3 position, Quaternion rotation);
void DrawGridColored(int slices, float spacing, Color color);

typedef struct Turret
{
	Vector3 position;
	Quaternion rotation;
	Vector3 targetPos;
	Vector3 azimuthLocalPosition;
	Vector3 elevationLocalPosition;
	Vector3 firepoints[2];
	int numFirepoints;
	float azimuth;
	float elevation;
	float fireDelay;
	float fireCooldown;
	float turnRate;
} Turret;

static void UpdateTurret(WorldState* world, Turret* turret, float deltaTime)
{
	turret->azimuth += turret->turnRate * DEG2RAD * GetFrameTime();
	turret->elevation = (float)sin(GetTime() * 10) * 0.2f - 0.4f;

	turret->fireCooldown -= deltaTime;
	if (turret->fireCooldown <= 0)
	{
		if (turret->numFirepoints == 0)
		{
			printf("Turret has no firepoints!");
			turret->fireCooldown = turret->fireDelay;
			return;
		}

		Matrix worldMat = MatrixBuildTransform(
			turret->position,
			turret->rotation);

		Matrix azimuthMat = MatrixBuildTransform(
			turret->azimuthLocalPosition,
			QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, turret->azimuth));
		Matrix worldAzimuthMat = MatrixMultiply(azimuthMat, worldMat);

		Matrix elevationMat = MatrixBuildTransform(
			turret->elevationLocalPosition,
			QuaternionFromAxisAngle((Vector3) { 1, 0, 0 }, turret->elevation));
		Matrix worldElevationMat = MatrixMultiply(elevationMat, worldAzimuthMat);

		for (int i = 0; i < turret->numFirepoints; i++)
		{
			Vector3 firePosition = Vector3Transform(turret->firepoints[i], worldElevationMat);
			Vector3 muzzleVelocity = Vector3RotateByQuaternion(
				(Vector3) { 0, 0, 400 },
				QuaternionFromMatrix(worldElevationMat));
			BulletsFire(world, firePosition, muzzleVelocity, 3);
		}
		turret->fireCooldown = turret->fireDelay;
	}
}

static void DrawTurret(Model* model, Turret* turret)
{
	Material defaultMaterial = LoadMaterialDefault();

	Matrix worldMat = MatrixBuildTransform(
		turret->position,
		turret->rotation);

	Matrix azimuthMat = MatrixBuildTransform(
		(Vector3){0, 32, 0},
		QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, turret->azimuth));
	Matrix worldAzimuthMat = MatrixMultiply(azimuthMat, worldMat);

	Matrix elevationMat = MatrixBuildTransform(
		(Vector3){0, 10, 0},
		QuaternionFromAxisAngle((Vector3) { 1, 0, 0 }, turret->elevation));
	Matrix worldElevationMat = MatrixMultiply(elevationMat, worldAzimuthMat);

	DrawMesh(model->meshes[0], defaultMaterial, worldMat);
	DrawMesh(model->meshes[1], defaultMaterial, worldAzimuthMat);
	DrawMesh(model->meshes[2], defaultMaterial, worldElevationMat);
}

void DrawAnimatedBillboard(
	Camera camera,
	Texture2D texture,
	int rows, int columns,
	Vector3 position, float size,
	double time, float fps,
	Color color)
{
	float frameWidth = (float)texture.width / columns;
	float frameHeight = (float)texture.height / rows;
	int frameCount = columns * rows;

	int currentFrame = (int)(time * fps) % frameCount;
	Rectangle rec = {
		.x = (currentFrame % columns) * frameWidth,
		.y = (currentFrame / rows) * frameHeight,
		.width = frameWidth,
		.height = frameHeight,
	};

	Vector2 quadSize = {size, size};
	DrawBillboardRec(camera, texture, rec, position, quadSize, color);
}

int main()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);

	// Create the window and OpenGL context
	InitWindow(RenderWidth, RenderHeight, "Ray Squadron");
	InitAudioDevice();

	// Fixed res screen
	RenderTexture2D target = LoadRenderTexture(RenderWidth, RenderHeight);
	float renderRatio = (float)RenderWidth / (float)RenderHeight;
	UpdateRenderFiltering(&target, isPointFiltered);

	// Utility function from resource_dir.h to find the resources folder and set it as the
	// current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	Music bgm = LoadMusicStream("realtrees.ogg");
	bgm.looping = true;
	//PlayMusicStream(bgm);

	Sound sfx_confirm = LoadSound("confirm.ogg");
	Sound sfx_shoot = LoadSound("shoot.ogg");
	Sound sfx_explode = LoadSound("explode.ogg");
	Sound sfx_win = LoadSound("win.ogg");

	Model mdl_ship = LoadModel("ship.glb");
	Model mdl_turret = LoadModel("turret.glb");
	Texture tex_expl = LoadTexture("explosion.png");

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

	Vector3 playerStartPosition = {0, 100, -1000};

	// =======================================
	// World declare
	// =======================================

	WorldState world = {0};
	WorldState worldSave = {0};

	Turret turretTemplate = {
		.position = {0, 0, 500},
		.rotation = QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, 180 * DEG2RAD),
		.targetPos = {0, 150, 0},
		.azimuthLocalPosition = {0, 32, 0},
		.elevationLocalPosition = {0, 10, 0},
		.firepoints = {{0, 0, 25}},
		.numFirepoints = 1,
		.azimuth = 0,
		.elevation = 0,
		.fireDelay = 0.5,
		.fireCooldown = 0,
		.turnRate = 90,
	};
	Turret turrets[TURRET_COUNT] = {0};
	for (int i = 0; i < TURRET_COUNT; i++)
	{
		turrets[i] = turretTemplate;
		turrets[i].fireCooldown = GetRandomFloat(0, turretTemplate.fireDelay);
		turrets[i].azimuth = GetRandomFloat(0, 360 * DEG2RAD);
	}

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
	float timeSinceQuickSave = 999.f;
	float timeSinceQuickLoad = 999.f;

	while (!WindowShouldClose())
	{
		// =======================================
		// Update
		// =======================================

		UpdateMusicStream(bgm);

		if (IsKeyPressed(KEY_F11))
			ToggleFullscreen();

		if (IsKeyPressed(KEY_F1))
		{
			isPointFiltered = !isPointFiltered;
			UpdateRenderFiltering(&target, isPointFiltered);
		}

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

					for (int i = 0; i < TURRET_COUNT; i++)
					{
						turrets[i].position = (Vector3){
							GetRandomFloat(-2000, 2000),
							0,
							GetRandomFloat(-2000, 2000),
						};
					}

					BulletsInit(&world);
					// Use this to tell if coming from the main menu or not.
					if (world.targetsDestroyed > 0)
						PlayMusicStream(bgm);

					currentScreen = GAMEPLAY;
					world.targetsDestroyed = 0;

					// Create a quicksave at the start of the level.
					worldSave = world;

					PlaySound(sfx_confirm);
				}
				break;
			}
			case GAMEPLAY:
			{
				float deltaTime = GetFrameTime();

				// Quicksaving and loading.
				timeSinceQuickLoad += deltaTime;
				timeSinceQuickSave += deltaTime;
				if (IsKeyPressed(KEY_F5))
				{
					worldSave = world;
					timeSinceQuickSave = 0.f;
				}
				if (IsKeyPressed(KEY_F9))
				{
					world = worldSave;
					timeSinceQuickLoad = 0.f;
					ParticlesClear();
				}

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

				int gamepadIndex = 2;
				input.pitch -= GetGamepadAxisMovement(gamepadIndex, GAMEPAD_AXIS_LEFT_Y);
				float gamepadYaw = GetGamepadAxisMovement(gamepadIndex, GAMEPAD_AXIS_LEFT_X);
				float gamepadRoll = gamepadYaw * -0.2f;
				input.yaw -= gamepadYaw;
				input.roll -= gamepadRoll;

				input.isFiring = IsKeyDown(KEY_LEFT_CONTROL) || IsMouseButtonDown(0);
				input.isFiring |= IsGamepadButtonDown(gamepadIndex, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

				ShipUpdate(&world.playerShip, input, deltaTime);

				// Position chase camera.
				Vector3 camPos = world.playerShip.position;
				camPos = Vector3Add(camPos, Vector3Scale(world.playerShip.forward, -40));
				camPos = Vector3Add(camPos, Vector3Scale(world.playerShip.up, 10));

				// Apply to the raylib camera.
				camera.position = Vector3SmoothDamp(camera.position, camPos, 10, deltaTime);
				camera.target = Vector3Add(world.playerShip.position, Vector3Scale(world.playerShip.forward, 225));
				camera.up = world.playerShip.up;

				// Update weapons (bullets)
				bool wasSomethingExplodedThisFrame = BulletsUpdate(&world, deltaTime);
				if (wasSomethingExplodedThisFrame)
					PlaySound(sfx_explode);

				// Update weapons (firing)
				timeSinceLastShot += deltaTime;
				if (input.isFiring && timeSinceLastShot >= fireDelay)
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

				ParticlesUpdate(deltaTime);

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

		BeginTextureMode(target);
		{
			if (currentScreen == TITLE)
			{
				ClearBackground((Color) { 16, 32, 64, 255 });
				DrawTextCentered("Destroy all targets!", RenderWidth / 2, RenderHeight / 2 - 30, 60, ORANGE);
				DrawTextCentered("Press [ENTER] to start.", RenderWidth / 2, RenderHeight / 2 + 30, 20, ORANGE);
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

					//DrawAnimatedBillboard(
					//	camera,
					//	tex_expl,
					//	4, 4,
					//	(Vector3){0, 100, 200},
					//	50,
					//	GetTime(),
					//	12,
					//	WHITE);

					ParticlesDraw();

					for (int i = 0; i < TURRET_COUNT; i++)
					{
						UpdateTurret(&world, &turrets[i], GetFrameTime());
						DrawTurret(&mdl_turret, &turrets[i]);
					}

				} EndMode3D();

				// HUD
				//for (int i = 0; i < MAX_BUILDINGS; i++)
				//	DrawText3D(camera, world.buildings[i].position, TextFormat("%d", i), MAGENTA);
				//for (int i = 0; i < MAX_ENEMIES; i++)
				//	DrawText3D(camera, world.enemyShips[i].position, TextFormat("%d", (int)world.enemyShips[i].position.y), MAGENTA);

				// Crosshairs
				Vector3 forward = Vector3RotateByQuaternion((Vector3) { 0, 0, 1 }, world.playerShip.rotation);
				Vector3 xhairPos = Vector3Add(world.playerShip.position, Vector3Scale(forward, 75));
				Vector2 xhairScreenPos = GetWorldToScreenEx(xhairPos, camera, RenderWidth, RenderHeight);
				DrawCrosshair(xhairScreenPos, 40);
				xhairPos = Vector3Add(world.playerShip.position, Vector3Scale(forward, 225));
				xhairScreenPos = GetWorldToScreenEx(xhairPos, camera, RenderWidth, RenderHeight);
				DrawCrosshair(xhairScreenPos, 13);

				if (currentScreen == ENDING)
				{
					// Win message on completion
					DrawRectangle(0, 0, RenderHeight, RenderWidth, Fade(BLACK, 0.8f));
					DrawTextCentered("YOU BEAT THE GAME!", RenderHeight / 2, RenderWidth / 2 - 30, 60, ORANGE);
					DrawTextCentered("Press [ENTER] to play again.", RenderHeight / 2, RenderWidth / 2 + 30, 20, ORANGE);
				}
				else
				{
					const int shadowOffset = 2;
					DrawTextCentered(TextFormat("TARGETS DESTROYED: %d", world.targetsDestroyed), RenderWidth / 2 + shadowOffset, 100 + shadowOffset, 40, BLACK);
					DrawTextCentered(TextFormat("TARGETS DESTROYED: %d", world.targetsDestroyed), RenderWidth / 2, 100, 40, ORANGE);

					BeginBlendMode(BLEND_ADDITIVE);

					float fadeTime = 1.5f;
					if (timeSinceQuickLoad < timeSinceQuickSave && timeSinceQuickLoad < fadeTime)
					{
						float lerp = Normalize(timeSinceQuickLoad, 0, fadeTime);
						lerp = Clamp(lerp, 0, fadeTime);
						Color col = ColorLerp(ORANGE, BLACK, lerp);
						DrawTextCentered("Quick Loaded", RenderHeight / 2, RenderWidth - 120, 20, col);
					}
					else if (timeSinceQuickSave < fadeTime)
					{
						float lerp = Normalize(timeSinceQuickSave, 0, fadeTime);
						lerp = Clamp(lerp, 0, fadeTime);
						Color col = ColorLerp(ORANGE, BLACK, lerp);
						DrawTextCentered("Quick Saved", RenderHeight / 2, RenderWidth - 120, 20, col);
					}

					EndBlendMode();
				}

				BeginBlendMode(BLEND_ADDITIVE);
				{
					DrawText(TextFormat("%d", GetFPS()), 10, 10, 10, GREEN);
				} EndBlendMode();
			}
		} EndTextureMode();

		BeginDrawing();
		{
			ClearBackground(BLACK);
			Rectangle sourceRec = {0, 0, target.texture.width, -target.texture.height};
			float screenWidth = GetScreenWidth();
			float screenHeight = GetScreenHeight();
			float renderAspect = (float)RenderWidth / (float)RenderHeight;
			float screenRecWidth = screenHeight * renderAspect;
			Rectangle destRec = {0, 0, screenRecWidth, screenHeight};
			DrawTexturePro(
				target.texture,
				sourceRec, destRec,
				(Vector2) { (screenRecWidth - screenWidth) / 2, 0 },
				0,
				WHITE);
		}
		EndDrawing();
	}

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

void DrawText3D(Camera camera, Vector3 position, const char* text, Color color)
{
	Vector3 cameraForward = Vector3Subtract(camera.target, camera.position);
	Vector3 cameraToPosition = Vector3Subtract(position, camera.position);
	if (Vector3DotProduct(cameraForward, cameraToPosition) < 0)
		return;

	Vector2 screenPos = GetWorldToScreenEx(position, camera, RenderWidth, RenderHeight);
	DrawText(text, (int)screenPos.x, (int)screenPos.y, 10, color);
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
