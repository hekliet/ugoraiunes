#include <math.h>
#include <stdint.h>

#include "globals.h"

float apu_sample;

#define APU_CLOCK_RATE_X1000 894886500
#define SAMPLE_DIVIDER_SCALE 1000

const unsigned sample_divider = APU_CLOCK_RATE_X1000 / HOST_SAMPLE_RATE;

static unsigned cycle_acc;

int apu_step(void) {
    static float phase;
    cycle_acc += SAMPLE_DIVIDER_SCALE;
    if (cycle_acc >= sample_divider) {
        cycle_acc -= sample_divider;
        apu_sample = sinf(phase) * 0.1f;
        phase += 6.2831853f * 440.0f / HOST_SAMPLE_RATE;
        if (phase >= 6.2831853f) phase -= 6.2831853f;
        return 1;
    }
    return 0;
}
