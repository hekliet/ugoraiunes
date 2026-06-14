#include <SDL2/SDL.h>
#include <string.h>

#include "globals.h"

SDL_AudioDeviceID audio_dev;
volatile int presentation_sentinel;
uint8_t presentation_keys[8];

static SDL_Window *win;
static SDL_Renderer *renderer;
static const unsigned scancodes[8] = {18, 13, 225, 44, 26, 22, 4, 7};
static Uint32 pixelbuf[256 * 240];

void *presentation_start(void *unused) {
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER);
    win = SDL_CreateWindow("ugoraiu", 427, 144, 512, 480, 0);
    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
    SDL_AudioSpec want = {44100, AUDIO_F32LSB, 1, 0, 4096, 0, 0, NULL, NULL};
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    SDL_PauseAudioDevice(audio_dev, 0);

    SDL_Event ev;
    for (;;) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE))
                presentation_sentinel = 1;
        }

        if (presentation_sentinel) break;

        void *dest;
        int pitch;
        if (SDL_LockTexture(texture, NULL, &dest, &pitch)) {
            fprintf(stderr, "%s\n", SDL_GetError());
            presentation_sentinel = 1;
        } else {
            memcpy(dest, pixelbuf, 256 * 240 * 4);
            SDL_UnlockTexture(texture);
        }
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    SDL_CloseAudioDevice(audio_dev);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return NULL;
}

void presentation_refresh_keys(void) {
    const uint8_t *kbstate = SDL_GetKeyboardState(NULL);
    for (int i = 0; i < 8; i++) presentation_keys[i] = kbstate[scancodes[i]];
}

void presentation_blit(Uint32 *pixels) { memcpy(pixelbuf, pixels, 256 * 240 * 4); }