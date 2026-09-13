#pragma once

#include <raylib.h>

#define AUDIO_MAX_SOUNDS 32
#define AUDIO_MAX_INSTANCES 16

// Registers a sound and creates a bunch of instances of it.
// Returns index of created sound.
int AudioRegisterSound(Sound sound, int instances);
// Registers a sound with min/max distance so that it attenuates with distance.
// Returns index of created sound.
int AudioRegisterSound3D(Sound sound, float minDistance, float maxDistance, int instances);

// Plays the oldest instance of the sound at the specified volume.
void AudioPlaySound(int soundIndex, float volume);
// Plays the oldest instance of the sound in 3D.
void AudioPlaySound3D(int soundIndex, Vector3 position, float volume);
// Where the camera is located when the next sound plays.
void AudioSetListenerPosition(Vector3 position);

// Automatically handles cleanup of all the sounds.
void AudioUnloadAllSounds();
