#ifndef TIMER_H
#define TIMER_H

#include <time.h>

typedef struct {
    struct timespec start;
    struct timespec end;
} Timer;

static inline void timer_start(Timer* timer) {
    clock_gettime(CLOCK_MONOTONIC, &timer -> start);
}

static inline void timer_end(Timer* timer) {
    clock_gettime(CLOCK_MONOTONIC, &timer -> end);
}

static inline double timer_elapsed_ms(Timer* timer) {
    double start = timer -> start.tv_sec * 1000.0 + timer -> start.tv_nsec / 1000000.0;
    double end = timer -> end.tv_sec * 1000.0 + timer -> end.tv_nsec / 1000000.0;
    return end - start;
}

static inline double timer_elapsed_seconds(Timer* timer) {
    return timer_elapsed_ms(timer) / 1000.0;
}

#endif // !TIMER_H
