#include "audio.h"

#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>

typedef struct AudioChannel
{
	Vector3 position;
	bool isLooping;
	Sound alias;
	float maxDist;
	float minDist;
} AudioChannel;

static AudioChannel channels[AUDIO_CHANNELS] = {0};
static Vector3 listenerPosition = {0, 0, 0};

static float GetNormalizedAttenuation(Vector3 position, float minDistance, float maxDistance)
{
	float distance = Vector3Distance(listenerPosition, position);
	if (distance > maxDistance)
		return 0;
	if (distance <= minDistance)
		return 1;

	float attenuation = Normalize(distance, maxDistance, minDistance);
	attenuation = Clamp(attenuation, 0, 1);
	return attenuation;
}

void AudioUpdate()
{

}

void AudioSetListener(Vector3 position)
{
	listenerPosition = position;
}

static int FindFreeChannel()
{
	for (int i = 0; i < AUDIO_CHANNELS; i++)
	{
		if (!IsSoundPlaying(channels[i].alias))
		{
			StopSound(channels[i].alias);
			UnloadSoundAlias(channels[i].alias);
			return i;
		}
	}
	return AUDIO_CHANNEL_INVALID;
}

int AudioPlaySound(Sound sound, float volume)
{
	int id = FindFreeChannel();
	if (id == AUDIO_CHANNEL_INVALID)
	{
		TraceLog(LOG_ERROR, "Ran out of audio channels! Sound will not play!");
		return id;
	}

	channels[id].alias = LoadSoundAlias(sound);
	SetSoundVolume(channels[id].alias, volume);
	PlaySound(channels[id].alias);

	return id;
}

int AudioPlaySoundAt(Sound sound, Vector3 position, float minDistance, float maxDistance, float volume)
{
	float attenuation = GetNormalizedAttenuation(position, minDistance, maxDistance);
	if (attenuation <= 0)
		return AUDIO_CHANNEL_INVALID;

	int id = FindFreeChannel();
	if (id == AUDIO_CHANNEL_INVALID)
	{
		TraceLog(LOG_ERROR, "Ran out of audio channels! Sound will not play!");
		return id;
	}

	channels[id].position = position;
	channels[id].minDist = minDistance;
	channels[id].maxDist = maxDistance;
	channels[id].alias = LoadSoundAlias(sound);
	SetSoundVolume(channels[id].alias, attenuation * volume);
	PlaySound(channels[id].alias);

	return id;
}

void AudioStopSound(int channelId)
{
	if (channelId >= 0 && channelId < AUDIO_CHANNELS)
	{
		StopSound(channels[channelId].alias);
		UnloadSoundAlias(channels[channelId].alias);
	}
}

int AudioGetNumOfChannelsInUse()
{
	int count = 0;
	for (int i = 0; i < AUDIO_CHANNELS; i++)
	{
		if (IsSoundPlaying(channels[i].alias))
			count += 1;
	}
	return count;
}
