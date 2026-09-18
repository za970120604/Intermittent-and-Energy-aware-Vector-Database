#include "debug.h"
#include <string.h>
#include <float.h>

#pragma PERSISTENT(breakpoint)
BREAKPOINT_TYPE breakpoint = BP_NONE;

#pragma PERSISTENT(stats)
Statistics stats = {0};

void init_stats() {
    for (uint8_t i = 0; i < PHASE_NUMBER; ++i) {
        stats.min_time[i] = DBL_MAX;
    }
}
