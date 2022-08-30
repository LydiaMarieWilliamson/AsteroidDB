#include "Timer.h"

#if 1 // SDL-generic.
static inline TimeT PerformanceCounter(void) { return SDL_GetTicks(); }
static inline TimeT PerformanceFrequency(void) { return (TimeT)1000; }
#else // Windows-tuned.
static inline TimeT PerformanceCounter(void) {
   LARGE_INTEGER T; QueryPerformanceCounter(&T);
   return (TimeT)T.QuadPart;
}
static inline TimeT PerformanceFrequency(void) {
   LARGE_INTEGER F; QueryPerformanceFrequency(&F);
   return (TimeT)F.QuadPart;
}
#endif

TimeT BegTimer(void) { return PerformanceCounter(); }

double EndTimer(TimeT T) { return (double)(PerformanceCounter() - T)/(double)PerformanceFrequency(); }
