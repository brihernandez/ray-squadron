#include "audio.h"

#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>

// The alias is of a fixed size to keep things simple. This means that there will be "wasted"
// zero-initialized aliases in memory, but it's fine. They don't take up that much memory.

// Why is there an instance count at all? Some sounds you actually don't want to overlap with each
// other, and in that case it's better to just pretend there's only a single instance.

typedef struct AudioSource
{
	Sound sound;
	Sound aliases[AUDIO_MAX_ALIASES];
	// Negative value indicates a 2D sound.
	float minDistance;
	// Negative value indicates a 2D sound.
	float maxDistance;
	int instanceCount;
	int nextAliasIndex;
} AudioSource;

static Sound AudioSourceGetNextAlias(AudioSource* source)
{
	Sound alias = source->aliases[source->nextAliasIndex];
	source->nextAliasIndex = (source->nextAliasIndex + 1) % source->instanceCount;
	return alias;
}

static AudioSource sources[AUDIO_MAX_SOURCES] = {0};
static int soundCount = 0;
static Vector3 listener = {0};

static bool IsSource2D(AudioSource* source)
{
	return source->maxDistance < 0 || source->minDistance < 0;
}

static bool IsSource3D(AudioSource* source)
{
	return source->maxDistance > 0 && source->maxDistance > 0;
}

int AudioRegisterSound(Sound sound, int aliases)
{
	return AudioRegisterSound3D(sound, -1, -1, aliases);
}

int AudioRegisterSound3D(Sound sound, float minDistance, float maxDistance, int aliases)
{
	if (soundCount >= AUDIO_MAX_SOURCES)
	{
		TraceLog(LOG_ERROR, "No available sources left to register sound %s! Consider raising the AUDIO_MAX_SOURCES!");
		return false;
	}

	int newIndex = soundCount;
	if (aliases > AUDIO_MAX_ALIASES)
		aliases = AUDIO_MAX_ALIASES;
	if (aliases < 1)
		aliases = 1;

	sources[newIndex] = (AudioSource)
	{
		.sound = sound,
		.aliases = {0},
		.minDistance = minDistance,
		.maxDistance = maxDistance,
		.instanceCount = aliases,
		.nextAliasIndex = 0,
	};

	for (int i = 0; i < AUDIO_MAX_ALIASES; i++)
		sources[newIndex].aliases[i] = LoadSoundAlias(sources[newIndex].sound);

	soundCount += 1;
	return newIndex;
}

static bool IsValidIndex(int index)
{
	return index >= 0 && index < AUDIO_MAX_SOURCES && index < soundCount;
}

void AudioPlaySound(int soundIndex, float volume)
{
	if (!IsValidIndex(soundIndex))
	{
		TraceLog(LOG_ERROR, "Tried to play sound with invalid index %d!", soundIndex);
		return;
	}

	Sound alias = AudioSourceGetNextAlias(&sources[soundIndex]);
	PlaySound(alias);
}

void AudioPlaySound3D(int soundIndex, Vector3 position, float volume)
{
	if (!IsValidIndex(soundIndex))
	{
		TraceLog(LOG_ERROR, "Tried to play sound with invalid index %d!", soundIndex);
		return;
	}

	AudioSource* source = &sources[soundIndex];

	// Negative min/max distances indicate a 2D sound.
	if (source->minDistance < 0)
		AudioPlaySound(soundIndex, volume);

	float distance = Vector3Distance(position, listener);
	if (distance > source->maxDistance)
		return;

	Sound alias = AudioSourceGetNextAlias(source);
	if (distance < source->minDistance)
	{
		SetSoundVolume(alias, volume);
	}
	else
	{
		float attenuatedVolume = Normalize(distance, source->maxDistance, source->minDistance);
		attenuatedVolume = Clamp(attenuatedVolume, 0, 1);
		SetSoundVolume(alias, attenuatedVolume * volume);
	}
	PlaySound(alias);
}

void AudioSetListenerPosition(Vector3 position)
{
	listener = position;
}

void AudioUnloadAllSounds()
{
	for (int i = 0; i < soundCount; i++)
	{
		for (int k = 0; k < AUDIO_MAX_ALIASES; k++)
			UnloadSoundAlias(sources[i].aliases[k]);
		UnloadSound(sources[i].sound);
	}
}
