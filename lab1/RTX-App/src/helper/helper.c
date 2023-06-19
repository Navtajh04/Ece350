#include "helper.h"
#include <stdio.h>

static const char LogTable256[256] = 
{
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

unsigned int log_two_floor(unsigned int num){
  register unsigned int t, tt; // temporaries
  
  if (tt = num >> 16)
  {
    return (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
  }
  else 
  {
    return (t = num >> 8) ? 8 + LogTable256[t] : LogTable256[num];
  }
}

unsigned int log_two_ceil(unsigned int num){
  register unsigned int t, tt; // temporaries

  if (num <= 1) {
    return 0; // Handle special case for num = 0 or 1
  }

  if ((num & (num - 1)) == 0) {
    // The value is a power of 2, no rounding needed
    return LogTable256[num];
  }

  if (tt = num >> 16) {
    return (t = tt >> 8) ? 24 + LogTable256[t] + ((tt & 0xFF) != 0) : 16 + LogTable256[tt] + 1;
  } else {
    return (t = num >> 8) ? 8 + LogTable256[t] + ((num & 0xFF) != 0) : LogTable256[num] + 1;
  }
}