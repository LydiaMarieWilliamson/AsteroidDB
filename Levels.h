#ifndef OnceOnlyLevels_h
#define OnceOnlyLevels_h

#if 0
#   pragma once // Still needed for the Windows version?
#endif

struct level {
   int nums[4]; // how many of each size of asteroid
   int aliens; // how many aliens
   float factor; // from 1.0 to 1.5 - multiply astereoid speed by this
};

extern struct level levels[50];

#endif // OnceOnly
