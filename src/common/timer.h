#pragma once

// ----------------------------------
// TIMER
//
// Simple timer to measure performance
// ----------------------------------

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

typedef struct {
#ifdef _WIN32
  LARGE_INTEGER start;
  LARGE_INTEGER end;
  LARGE_INTEGER frequency;
#else
  struct timespec start;
  struct timespec end;
#endif
  int is_running;
} Timer;

// Initialize a timer
void timer_init(Timer *timer);

// Start the timer
void timer_start(Timer *timer);

// Stop the timer
void timer_stop(Timer *timer);

// Get elapsed time in milliseconds
double timer_elapsed_ms(Timer *timer);

// Get elapsed time in seconds
double timer_elapsed_seconds(Timer *timer);
