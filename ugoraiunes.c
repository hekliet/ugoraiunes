#include <stdint.h>
#include <stdio.h>

#include "globals.h"

void cpu_tables_init(void);
void cart_load(const char *path);
int cpu_interrupt(int type);
int cpu_step(void);

extern uint8_t fi;

int nmi_pending, irq_pending;

void main(int argc, char **argv) {
    cpu_tables_init();
    cart_load(argv[1]);
    int cpu_cycles = cpu_interrupt(RESET);
    float apu_acc = .0f;
    for (;;) {
        for (int i = 0; i < 3 * cpu_cycles; i++) {
            // ppu_step();  // may set nmi_pending
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
}
