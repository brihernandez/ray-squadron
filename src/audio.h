#pragma once

#include <raylib.h>

#define AUDIO_CHANNELS 64
#define AUDIO_CHANNEL_INVALID -1

//typedef struct SoundEffect
//{
//	const char name[64];
//	Sound sound;
//	float minDist;
//	float maxDist;
//} SoundEffect;

void AudioUpdate(void);
void AudioSetListener(Vector3 position);

int AudioPlaySound(Sound sound, float volume);
int AudioPlaySoundAt(Sound sound, Vector3 position, float minDistance, float maxDistance, float volume);
void AudioStopSound(int channelId);

int AudioGetNumOfChannelsInUse(void);
