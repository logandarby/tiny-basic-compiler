#include "timer.h"

#include <stdio.h>
#include <time.h>

// Initialize a timer
void timer_init(Timer *timer) {
  timer->is_running = 0;

#ifdef _WIN32
  QueryPerformanceFrequency(&timer->frequency);
#endif
}

// Start the timer
void timer_start(Timer *timer) {
  timer->is_running = 1;

#ifdef _WIN32
  QueryPerformanceCounter(&timer->start);
#else
  clock_gettime(CLOCK_MONOTONIC, &timer->start);
#endif
}

// Stop the timer
void timer_stop(Timer *timer) {
  if (!timer->is_running) {
    fprintf(stderr, "Warning: timer_stop() called on stopped timer\n");
    return;
  }

  timer->is_running = 0;

#ifdef _WIN32
  QueryPerformanceCounter(&timer->end);
#else
  clock_gettime(CLOCK_MONOTONIC, &timer->end);
#endif
}

// Get elapsed time in milliseconds
double timer_elapsed_ms(Timer *timer) {
  if (timer->is_running) {
    fprintf(stderr, "Warning: timer_elapsed_ms() called on running timer\n");
    return 0.0;
  }

#ifdef _WIN32
  return ((double)(timer->end.QuadPart - timer->start.QuadPart) * 1000.0) /
         timer->frequency.QuadPart;
#else
  double start_ms =
      timer->start.tv_sec * 1000.0 + timer->start.tv_nsec / 1000000.0;
  double end_ms = timer->end.tv_sec * 1000.0 + timer->end.tv_nsec / 1000000.0;
  return end_ms - start_ms;
#endif
}

// Get elapsed time in seconds
double timer_elapsed_seconds(Timer *timer) {
  return timer_elapsed_ms(timer) / 1000.0;
}
