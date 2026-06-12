#include <SDL2/SDL.h>

volatile int presentation_sentinel;

static SDL_Window *win;
static SDL_Surface *winsu;

void *presentation_start(void *) {
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    win = SDL_CreateWindow("ugoraiu", 427, 144, 512, 480, 0);
    winsu = SDL_GetWindowSurface(win);
    // SDL_AudioSpec spec = {44100, AUDIO_F32LSB, 1, 0, 1764, 0, 0, sdl_audio_callback, NULL};
    // SDL_AudioDeviceID audio = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    // SDL_PauseAudioDevice(audio, 0);

    SDL_Event ev;
    for (;;) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE))
                presentation_sentinel = 1;
        }

        if (presentation_sentinel) break;
    }
    // SDL_CloseAudioDevice(audio);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return NULL;
}

void presentation_blit(uint32_t *pixels) {
    SDL_Surface *nessu =
        SDL_CreateRGBSurfaceFrom(pixels, 256, 240, 32, 1024, 0xff0000, 0xff00, 0xff, 0);
    SDL_BlitScaled(nessu, NULL, winsu, NULL);
    SDL_UpdateWindowSurface(win);
    SDL_FreeSurface(nessu);
}
