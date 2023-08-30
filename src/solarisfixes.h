//
// Created by fly on 8/2/23.
//

#ifndef CACHE_1_0_0_SOLARISFIXES_H
#define CACHE_1_0_0_SOLARISFIXES_H

#if defined(__sun)
#if defined(__GUNC__)

#include <math.h>

#undef isnan
#define isnan(x)                                                               \
  __extension__({                                                              \
    __typeof(x) __x_a = (x);                                                   \
    __builtin_expect(__x_a != __x_a, 0);                                       \
  })

#undef isfinite
#define isfinite(x)                                                            \
  __extension__({                                                              \
    __typeof(x) __x_f = (x);                                                   \
    __builtin_expect(!isnan(__x_f - __x_f), 1);                                \
  })

#undef isinf
#define isinf(x)                                                               \
  __extension__({                                                              \
    __typeof(x) __x_i = (x);                                                   \
    __builtin_expect(!isnan(__x_i) && !isfinite(__x_i), 0);                    \
  })

#define u_int uint
#define u_int32_t uint32_t
#endif
#endif
#endif // CACHE_1_0_0_SOLARISFIXES_H
