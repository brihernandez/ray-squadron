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

#define MAX_BUILDINGS 100
#define MAX_BULLETS 100
#define BULLET_LIFETIME 2.5

typedef struct Projectile {
	Vector3 position;
	Vector3 velocity;
	float lifeTime;
	bool active;
} Projectile;

typedef struct Building {
	Vector3 position;
	bool active;
} Building;

int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);

	// Create the window and OpenGL context
	const int ScreenWidth = 800;
	const int ScreenHeight = 600;
	InitWindow(ScreenWidth, ScreenHeight, "Ray Squadron");

	// =======================================
	// Buildings init
	// =======================================
	Building buildings[MAX_BUILDINGS] = {0};
	for (int i = 0; i < MAX_BUILDINGS; i++)
	{
		buildings[i].position = (Vector3){
			(float)GetRandomValue(-500, 500),
			10,
			(float)GetRandomValue(-500, 500)
		};
		buildings[i].active = true;
	}

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
	Vector3 position = {0, 100, 0};
	Quaternion rotation = QuaternionIdentity();
	float speed = 80;

	// =======================================
	// Camera Init
	// =======================================
	Camera3D camera = {0};
	camera.fovy = 50;
	camera.projection = CAMERA_PERSPECTIVE;

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");
	//Texture wabbit = LoadTexture("wabbit_alpha.png");

	// =======================================
	// Main Loop
	// =======================================

	while (!WindowShouldClose())
	{
		// =======================================
		// Update
		// =======================================

		float deltaTime = GetFrameTime();
		float pitchSpeed = 1.5 * deltaTime;
		float rollSpeed = 3 * deltaTime;
		float yawSpeed = 0.5 * deltaTime;

		// Rotate
		if (IsKeyDown(KEY_W)) rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle((Vector3) { 1, 0, 0 }, pitchSpeed));
		if (IsKeyDown(KEY_S)) rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle((Vector3) { 1, 0, 0 }, -pitchSpeed));
		if (IsKeyDown(KEY_A)) rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle((Vector3) { 0, 0, 1 }, -rollSpeed));
		if (IsKeyDown(KEY_D)) rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle((Vector3) { 0, 0, 1 }, rollSpeed));
		if (IsKeyDown(KEY_Q)) rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, yawSpeed));
		if (IsKeyDown(KEY_E)) rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle((Vector3) { 0, 1, 0 }, -yawSpeed));
		rotation = QuaternionNormalize(rotation);

		// Translate
		Vector3 forward = Vector3RotateByQuaternion((Vector3) { 0, 0, 1 }, rotation);
		Vector3 up = Vector3RotateByQuaternion((Vector3) { 0, 1, 0 }, rotation);
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
			}
		}

		// =======================================
		// Render
		// =======================================

		BeginDrawing();
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
					rlRotatef(angle* RAD2DEG, axis.x, axis.y, axis.z);

					// Now that the matrix has been setup, draw the plane at the "origin".
					// Fuselage, Wings, Tail
					DrawCylinderEx(
						(Vector3) { 0.0f, 0.0f, -4.0f },
						(Vector3) { 0.0f, 0.0f, 6.0f },
						1.2f, 0.2f, 6,
						(Color) { 55, 75, 65, 255 });
					DrawCube(
						(Vector3) { 0.0f, 0.0f, -1.0f },
						16.0f, 0.2f, 4.0f,
						(Color) { 50, 68, 58, 255 });
					DrawCube(
						(Vector3) { 0.0f, 1.5f, -3.0f },
						0.2f, 3.0f, 2.0f,
						(Color) { 45, 60, 50, 255 });

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
				DrawText(TextFormat("%i", i), buildingScreenPos.x, buildingScreenPos.y, 10, MAGENTA);
			}

			DrawText(TextFormat("SPEED: %i KTS", (int)speed), 40, 40, 20, GREEN);
			DrawText(TextFormat("ALTITUDE: %i FT", (int)position.y * 10), 40, 70, 20, GREEN);

			// Crosshair reticle?
			DrawCircleLines(ScreenWidth / 2, ScreenHeight / 2, 50, GREEN);
			DrawLine(
				ScreenWidth / 2 - 100, ScreenHeight / 2,
				ScreenWidth / 2 + 100, ScreenHeight / 2,
				GREEN);

			DrawFPS(10, 10);

		} EndDrawing();
	}

	// cleanup
	// unload our texture so it can be cleaned up
	//UnloadTexture(wabbit);

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
