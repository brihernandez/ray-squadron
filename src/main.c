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

#define MAX_BUILDINGS 15
#define MAX_BULLETS 100
#define BULLET_LIFETIME 2.5

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

void DrawCrosshair(Vector2 screenPos, float radius);
void DrawTextCentered(const char* message, int x, int y, int size, Color color);

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

	// =======================================
	// Buildings init
	// =======================================
	Building buildings[MAX_BUILDINGS] = {0};

	// =======================================
	// Weapons init
	// =======================================
	Projectile bullets[MAX_BULLETS] = {0};
	float fireDelay = 0.08f;
	float timeSinceLastShot = 0;
	float muzzleVelocity = 400;

	// =======================================
	// Physics state
	// =======================================
	const Vector3 startPosition = {0, 100, 0};
	const float startSpeed = 80;

	Vector3 position = startPosition;
	Quaternion rotation = QuaternionIdentity();
	float speed = startSpeed;

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

		switch (currentScreen) {
			case TITLE:
			case ENDING:
			{
				if (IsKeyPressed(KEY_ENTER))
				{
					position = startPosition;
					rotation = QuaternionIdentity();
					speed = startSpeed;

					for (int i = 0; i < MAX_BUILDINGS; i++)
					{
						const int size = 10;
						buildings[i].position = (Vector3){
							(float)GetRandomValue(-500, 500),
							size,
							(float)GetRandomValue(-500, 500)
						};
						buildings[i].active = true;
						buildings[i].bounds = (BoundingBox){
							.min = (Vector3) {buildings[i].position.x - size, buildings[i].position.y - size, buildings[i].position.z - size},
							.max = (Vector3) {buildings[i].position.x + size, buildings[i].position.y + size, buildings[i].position.z + size},
						};
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
				float pitchSpeed = 1.5 * deltaTime;
				float rollSpeed = 3 * deltaTime;
				float yawSpeed = 3 * deltaTime;

				// Rotate
				float pitch = 0;
				float yaw = 0;
				float roll = 0;
				if (IsKeyDown(KEY_W)) {
					pitch += 1;
				}
				if (IsKeyDown(KEY_S)) {
					pitch -= 1;
				}
				if (IsKeyDown(KEY_A)) {
					roll -= 0.2f;
					yaw += 0.5f;
				}
				if (IsKeyDown(KEY_D)) {
					roll += 0.2f;
					yaw -= 0.5f;
				}
				if (IsKeyDown(KEY_Q)) {
					yaw -= 1;
				}
				if (IsKeyDown(KEY_E)) {
					yaw += 1;
				}

				// Autolevel
				Vector3 forward = Vector3RotateByQuaternion((Vector3) { 0, 0, 1 }, rotation);
				Vector3 up = Vector3RotateByQuaternion((Vector3) { 0, 1, 0 }, rotation);
				Vector3 right = Vector3RotateByQuaternion((Vector3){1, 0, 0}, rotation);
				roll -= right.y / 2;

				pitch = Clamp(pitch, -1, 1);
				yaw = Clamp(yaw, -1, 1);
				roll = Clamp(roll, -1, 1);

				rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle((Vector3) { 1, 0, 0 }, pitch * pitchSpeed));
				rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, yaw * yawSpeed));
				rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle((Vector3) { 0, 0, 1 }, roll * rollSpeed));
				rotation = QuaternionNormalize(rotation);

				// Translate
				position = Vector3Add(position, Vector3Scale(forward, speed * deltaTime));
				if (position.y < 2)
					position.y = 2;

				// Position chase camera.
				Vector3 camPos = position;
				camPos = Vector3Add(camPos, Vector3Scale(forward, -25));
				camPos = Vector3Add(camPos, Vector3Scale(up, 8));

				// Apply to the raylib camera.
				camera.position = camPos;
				camera.target = Vector3Add(position, Vector3Scale(forward, 20));
				camera.up = up;

				// Update weapons (firing)
				timeSinceLastShot += deltaTime;
				if (IsKeyDown(KEY_LEFT_CONTROL) && timeSinceLastShot >= fireDelay)
				{
					// Find first inactive bullet and use it to spawn.
					// This whole thing can be done much better.
					for (int i = 0; i < MAX_BULLETS; i++)
					{
						if (!bullets[i].active)
						{
							bullets[i].active = true;
							bullets[i].lifeTime = BULLET_LIFETIME;
							bullets[i].position = Vector3Add(position, Vector3Scale(forward, 5));
							bullets[i].velocity = Vector3Scale(forward, speed + muzzleVelocity);
							timeSinceLastShot = 0;
							PlaySound(sfx_shoot);
							break;
						}
					}
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
					}
				}

				// Check win condition.
				if (targetsDestroyed >= MAX_BUILDINGS)
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
				DrawTextCentered("Destroy all buildings!", ScreenWidth / 2, ScreenHeight / 2, 60, ORANGE);
				DrawTextCentered("Press [ENTER] to start.", ScreenWidth / 2, ScreenHeight / 2 + 80, 20, ORANGE);
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
					DrawGrid(1000, 100);

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

						DrawCube(buildings[i].position, 20, 20, 20, (Color) { 80, 80, 80, 255 });
						DrawCubeWires(buildings[i].position, 20, 20, 20, BLACK);
					}

					// Draw bullets.
					for (int i = 0; i < MAX_BULLETS; i++)
					{
						if (!bullets[i].active)
							continue;
						DrawCube(
							bullets[i].position,
							1, 1, 1,
							YELLOW);
					}

					// Draw the plane.
					Vector3 axis;
					float angle;
					QuaternionToAxisAngle(rotation, &axis, &angle);

					rlPushMatrix();
					{
						rlTranslatef(position.x, position.y, position.z);
						rlRotatef(angle * RAD2DEG, axis.x, axis.y, axis.z);

						// Now that the matrix has been setup, draw the plane at the "origin".
						// Fuselage, Wings, Tail
						DrawCylinderEx(
							(Vector3) {
							0.0f, 0.0f, -4.0f
						},
							(Vector3) {
							0.0f, 0.0f, 6.0f
						},
							1.2f, 0.2f, 6,
							(Color) {
							55, 75, 65, 255
						});
						DrawCube(
							(Vector3) {
							0.0f, 0.0f, -1.0f
						},
							16.0f, 0.2f, 4.0f,
							(Color) {
							50, 68, 58, 255
						});
						DrawCube(
							(Vector3) {
							0.0f, 1.5f, -3.0f
						},
							0.2f, 3.0f, 2.0f,
							(Color) {
							45, 60, 50, 255
						});

					} rlPopMatrix();

				} EndMode3D();

				// HUD
				Vector3 cameraForward = Vector3Subtract(camera.target, camera.position);
				for (int i = 0; i < MAX_BUILDINGS; i++)
				{
					Vector3 cameraToBuilding = Vector3Subtract(buildings[i].position, camera.position);
					if (Vector3DotProduct(cameraForward, cameraToBuilding) < 0)
						continue;

					Vector2 buildingScreenPos = GetWorldToScreen(buildings[i].position, camera);
					DrawText(TextFormat("%d", i), buildingScreenPos.x, buildingScreenPos.y, 10, MAGENTA);
				}

				// Crosshairs
				Vector3 forward = Vector3RotateByQuaternion((Vector3) { 0, 0, 1 }, rotation);
				Vector3 xhairPos = Vector3Add(position, Vector3Scale(forward, 75));
				Vector2 xhairScreenPos = GetWorldToScreen(xhairPos, camera);
				DrawCrosshair(xhairScreenPos, 40);
				xhairPos = Vector3Add(position, Vector3Scale(forward, 225));
				xhairScreenPos = GetWorldToScreen(xhairPos, camera);
				DrawCrosshair(xhairScreenPos, 13);

				if (currentScreen == ENDING)
				{
					// Win message on completion
					DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Fade(BLACK, 0.5));
					DrawTextCentered("YOU BEAT THE GAME!", ScreenWidth / 2, ScreenHeight / 2, 60, ORANGE);
					DrawTextCentered("Press [ENTER] to play again.", ScreenWidth / 2, ScreenHeight / 2 + 80, 20, ORANGE);
				}
				else
				{
					DrawTextCentered(TextFormat("TARGETS DESTROYED: %d", targetsDestroyed), ScreenWidth / 2, 60, 40, ORANGE);
				}

			}

			BeginBlendMode(BLEND_ADDITIVE);
			{
				DrawFPS(10, 10);
			} EndBlendMode();

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
