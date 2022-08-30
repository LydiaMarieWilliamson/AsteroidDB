#ifndef hr_timer
#   include "hr_time.h"
#   define hr_timer
#endif

#if 1 // SDL-generic.
static inline stopWatch PerformanceCounter(void) {
   return SDL_GetTicks();
}
static inline stopWatch PerformanceFrequency(void) {
   return (stopWatch) 1000;
}
#else // Windows-tuned.
static inline stopWatch PerformanceCounter(void) {
   LARGE_INTEGER T;
   QueryPerformanceCounter(&T);
   return (stopWatch) T.QuadPart;
}
static inline stopWatch PerformanceFrequency(void) {
   LARGE_INTEGER F;
   QueryPerformanceCounter(&F);
   return (stopWatch) F.QuadPart;
}
#endif

stopWatch startTimer(void) {
   return PerformanceCounter();
}

double stopTimer(stopWatch timer) {
   stopWatch time = PerformanceCounter() - timer;
   stopWatch frequency = PerformanceFrequency();
   return ((double)time/(double)frequency);
}
