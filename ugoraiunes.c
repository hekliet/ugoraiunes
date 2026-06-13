#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "globals.h"

void cpu_tables_init(void);
void cart_load(const char *path);
int cpu_interrupt(int type);
int cpu_step(void);
void ppu_init(void);
void ppu_step(void);

void *presentation_start(void *);
void presentation_blit(uint32_t *pixels);

extern uint8_t fi;
extern volatile int presentation_sentinel;
extern int ppu_frame_ready;
extern uint32_t render_pixels[256 * 240];

int nmi_pending, irq_pending;

static volatile sig_atomic_t running = 1;
static uint32_t pixels[256 * 240];

static void handle_sigint(int sig) { running = 0; }

void main(int argc, char **argv) {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, presentation_start, NULL);

    signal(SIGINT, handle_sigint);
    cpu_tables_init();
    cart_load(argv[1]);
    ppu_init();
    int cpu_cycles = cpu_interrupt(RESET);
    float apu_acc = .0f;

    while (running && !presentation_sentinel) {
        for (int i = 0; i < 3 * cpu_cycles; i++) {
            ppu_step();  // may set nmi_pending
        }
        if (ppu_frame_ready) {
            ppu_frame_ready = 0;
            presentation_blit(render_pixels);
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
            // apu_step();  // may set irq_pending
        }

        // output logs here, accounting for
        //   nmi_pending, irq_pending && !fi

        if (nmi_pending) {
            nmi_pending = 0;  // edge-triggered
            cpu_cycles = cpu_interrupt(NMI);
        } else if (irq_pending && !fi) {
            // level-triggered, not resetting here
            cpu_cycles = cpu_interrupt(IRQ);
        } else {
            cpu_cycles = cpu_step();
        }
    }

    presentation_sentinel = 1;
    pthread_join(thread_id, NULL);
}
