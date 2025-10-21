// Source : https://bitbucket.org/dandago/gigilabs/src/3a16971c18c1b4db0dd3f75de7fe880e80d8dbf7/Sdl2PlayWav/?at=master

#include <SDL2/SDL.h>
#include <cstdio>

int main(int argc, char ** argv)
{
	if(SDL_Init(SDL_INIT_AUDIO) < 0) {
		puts("Unable to init SDL:");
		puts(SDL_GetError());
		return 1;
	}

	// load WAV file

	SDL_AudioSpec wavSpec;
	Uint32 wavLength;
	Uint8 *wavBuffer;

	auto* spec = SDL_LoadWAV("sound.wav", &wavSpec, &wavBuffer, &wavLength);
	if(!spec) {
		puts("Unable to load sound");
		return 1;
	} {
		puts("Did open file");
	}
	
	// open audio device

	SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
	if(deviceId == 0) {
		puts("Unable to open audio device:");
		puts(SDL_GetError());
		return 1;
	}

	// play audio

	int success = SDL_QueueAudio(deviceId, wavBuffer, wavLength);
	SDL_PauseAudioDevice(deviceId, 0);

	// keep window open enough to hear the sound

	SDL_Delay(1000);

	// clean up

	SDL_CloseAudioDevice(deviceId);
	SDL_FreeWAV(wavBuffer);
	SDL_Quit();

	return 0;
}