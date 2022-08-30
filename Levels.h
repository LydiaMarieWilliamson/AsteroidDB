#ifndef OnceOnlyLevels_h
#define OnceOnlyLevels_h

#if 0
#   pragma once // Still needed for the Windows version?
#endif

#include <stdio.h> // For size_t.

struct level {
   float factor; // from 1.0 to 1.5 - multiply astereoid speed by this
   int aliens; // how many aliens
   int nums[4]; // how many of each size of asteroid
};

extern struct level levels[];
extern const size_t MaxLevels;

#endif // OnceOnly
