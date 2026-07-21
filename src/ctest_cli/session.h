#ifndef CTEST_SESSION_H
#define CTEST_SESSION_H

#include <stddef.h>

typedef struct {
  size_t total_suites;
  size_t total_runs;
  size_t total_failures;
  size_t total_crashes;
} SessionMetrics;

#endif
