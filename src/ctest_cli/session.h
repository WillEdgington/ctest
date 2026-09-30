#ifndef CTEST_SESSION_H
#define CTEST_SESSION_H

#include "executor.h"
#include <stddef.h>

typedef struct {
  size_t total_suites;
  size_t total_runs;
  size_t total_failures;
  size_t total_crashes;
  size_t total_timeouts;
  struct timespec start_time;
  struct timespec end_time;
} SessionMetrics;

void ctest_update_session(SessionMetrics *session, SuiteMetrics *suite);
void ctest_session_start(SessionMetrics *session);
void ctest_session_end(SessionMetrics *session);
double ctest_session_get_duration_ms(const SessionMetrics *session);

#endif
