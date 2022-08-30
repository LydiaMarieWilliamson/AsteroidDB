#if 1 // SDL-generic.
#   include <SDL2/SDL.h>
typedef Uint64 stopWatch;
#else // Windows-tuned.
#   include <windows.h>
typedef LARGE_INTEGER stopWatch;
#endif

stopWatch startTimer(void);
double stopTimer(stopWatch timer);
