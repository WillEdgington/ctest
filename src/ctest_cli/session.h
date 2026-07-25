#ifndef CTEST_SESSION_H
#define CTEST_SESSION_H

#include "executor.h"
#include <stddef.h>

typedef struct {
  size_t total_suites;
  size_t total_runs;
  size_t total_failures;
  size_t total_crashes;
} SessionMetrics;

void ctest_update_session(SessionMetrics *session, SuiteMetrics *suite);

#endif
