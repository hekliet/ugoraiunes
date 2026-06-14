#include <SDL2/SDL.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "globals.h"

#define NES_CYCLES_PER_FRAME_SHL10 30495232
#define SILENCE_NUMBYTES (((102 * HOST_SAMPLE_RATE) >> 8) & 0xFFFFFFF8)

void cpu_tables_init(void);
void cart_load(const char *path);
int cpu_interrupt(int type);
int cpu_step(void);
void ppu_init(void);
void ppu_step(void);
int apu_step(void);

void *presentation_start(void *);

extern uint8_t fi;
extern volatile int presentation_sentinel;
extern float apu_sample;
extern SDL_AudioDeviceID audio_dev;

int nmi_pending, irq_pending;

static volatile sig_atomic_t running = 1;
static int cpu_cycles;
static float sample_buf[1024];
static int sample_count;

static void handle_sigint(int sig) { running = 0; }

int step(void) {
    static float apu_acc;
    for (int i = 0; i < 3 * cpu_cycles; i++) {
        ppu_step();  // may set nmi_pending
    }

    float apu_cycles_f = .5f * cpu_cycles;
    int apu_cycles = apu_cycles_f;
    apu_acc += apu_cycles_f - apu_cycles;
    if (apu_acc >= 1.0f) {
        int apu_acc_i = apu_acc;
        apu_cycles += apu_acc_i;
        apu_acc -= apu_acc_i;
    }
    for (int i = 0; i < apu_cycles; i++) {
        if (apu_step()) {  // may set irq_pending
            sample_buf[sample_count++] = apu_sample;
            if (sample_count == 1024) {
                SDL_QueueAudio(audio_dev, sample_buf, sample_count * 4);
                sample_count = 0;
            }
        }
    }

    // output logs here, accounting for
    //   nmi_pending, irq_pending && !fi

    int prev_cpu_cycles = cpu_cycles;
    if (nmi_pending) {
        nmi_pending = 0;  // edge-triggered
        cpu_cycles = cpu_interrupt(NMI);
    } else if (irq_pending && !fi) {
        // level-triggered, not resetting here
        cpu_cycles = cpu_interrupt(IRQ);
    } else {
        cpu_cycles = cpu_step();
    }
    return prev_cpu_cycles;
}

void main(int argc, char **argv) {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, presentation_start, NULL);

    {
        float silence[SILENCE_NUMBYTES >> 2] = {0};
        while (SDL_GetQueuedAudioSize(audio_dev) < SILENCE_NUMBYTES)
            SDL_QueueAudio(audio_dev, silence, SILENCE_NUMBYTES);
    }

    signal(SIGINT, handle_sigint);
    cpu_tables_init();
    cart_load(argv[1]);
    ppu_init();
    cpu_cycles = cpu_interrupt(RESET);

    uint64_t perf_freq = SDL_GetPerformanceFrequency();
    uint64_t next_frame_incr = (double)perf_freq / 60.0988;
    uint64_t next_frame = SDL_GetPerformanceCounter();
    int cycle_count = 0;
    struct timespec ts = {.tv_sec = 0};

    while (running && !presentation_sentinel) {
        while (cycle_count < NES_CYCLES_PER_FRAME_SHL10) cycle_count += step() << 10;
        cycle_count -= NES_CYCLES_PER_FRAME_SHL10;
        next_frame += next_frame_incr;

        if (sample_count > 0) {
            SDL_QueueAudio(audio_dev, sample_buf, sample_count * 4);
            sample_count = 0;
        }

        int64_t ahead = next_frame - SDL_GetPerformanceCounter();
        if (ahead > 0) {
            ts.tv_nsec = ahead * 1000000000LL / perf_freq;
            nanosleep(&ts, NULL);
            while (SDL_GetPerformanceCounter() < next_frame);
        }
    }

    presentation_sentinel = 1;
    pthread_join(thread_id, NULL);
}
