#include <SDL2/SDL.h>

volatile int presentation_sentinel;
uint8_t presentation_keys[8];

static SDL_Window *win;
static SDL_Surface *winsu;

static const unsigned scancodes[8] = {18, 13, 225, 44, 26, 22, 4, 7};

void *presentation_start(void *) {
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER);
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

int presentation_blit(uint32_t *pixels) {
    static uint64_t last_tick = 0;
    uint64_t now = SDL_GetTicks64();
    while (now - last_tick < 16) now = SDL_GetTicks64();
    last_tick = now;
    SDL_Surface *nessu =
        SDL_CreateRGBSurfaceFrom(pixels, 256, 240, 32, 1024, 0xff0000, 0xff00, 0xff, 0);
    SDL_BlitScaled(nessu, NULL, winsu, NULL);
    SDL_UpdateWindowSurface(win);
    SDL_FreeSurface(nessu);
    return 0;
}

void presentation_refresh_keys(void) {
    const uint8_t *kbstate = SDL_GetKeyboardState(NULL);
    for (int i = 0; i < 8; i++) presentation_keys[i] = kbstate[scancodes[i]];
}