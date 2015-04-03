/* Inspired by the timer* macros in <sys/time.h> in glibc-2.20 */

#ifndef _TIMESPEC_OPS_H
#define _TIMESPEC_OPS_H 1

/* Convenience macros for operations on timespec.
 *    NOTE: `timespeccmp' does not work for >= or <=.  */
# define timespecisset(tvp)        ((tvp)->tv_sec || (tvp)->tv_nsec)
# define timespecclear(tvp)        ((tvp)->tv_sec = (tvp)->tv_nsec = 0)

# define timespeccmp(a, b, CMP)                                               \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_nsec CMP (b)->tv_nsec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))

# define timespecadd(a, b, result)                                            \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;                          \
    if ((result)->tv_nsec >= 1000000000)                                      \
      {                                                                       \
        ++(result)->tv_sec;                                                   \
        (result)->tv_nsec -= 1000000000;                                      \
      }                                                                       \
  } while (0)

# define timespecsub(a, b, result)                                            \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;                          \
    if ((result)->tv_nsec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_nsec += 1000000;                                           \
    }                                                                         \
  } while (0)

#endif
