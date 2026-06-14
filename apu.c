#include <stdint.h>

float apu_sample;

static unsigned cycle;
static unsigned cycles_to_sample = 20;

int apu_step(void) {
    if (++cycle == cycles_to_sample) {
        cycles_to_sample ^= 1;
        cycle = 0;
        return 1;
    }
    return 0;
}
