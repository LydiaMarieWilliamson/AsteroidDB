#ifndef OnceOnlyTimer_h
#define OnceOnlyTimer_h

#if 1 // SDL-generic.
#   include <SDL2/SDL.h>
typedef Uint64 stopWatch;
#else // Windows-tuned.
#   pragma once // Still needed for the Windows version?
#   include <windows.h>
typedef LARGE_INTEGER stopWatch;
#endif

stopWatch startTimer(void);
double stopTimer(stopWatch timer);

#endif // OnceOnly
