#define _GNU_SOURCE
#include "session.h"
#include "executor.h"
#include <string.h>
#include <time.h>

void ctest_update_session(SessionMetrics *session, SuiteMetrics *suite) {
  session->total_suites++;
  session->total_runs += suite->total_runs;
  session->total_failures += suite->total_failures;

  switch (suite->state) {
  case SUITE_CRASH:
    session->total_crashes++;
    break;
  case SUITE_TIMEOUT:
    session->total_timeouts++;
    break;
  default:
    break;
  }
}

void ctest_session_start(SessionMetrics *session) {
  memset(session, 0, sizeof(SessionMetrics));
  clock_gettime(CLOCK_MONOTONIC, &session->start_time);
}

void ctest_session_end(SessionMetrics *session) {
  clock_gettime(CLOCK_MONOTONIC, &session->end_time);
}

double ctest_session_get_duration_ms(const SessionMetrics *session) {
  double sec_diff =
      (double)(session->end_time.tv_sec - session->start_time.tv_sec);
  double nsec_diff =
      (double)(session->end_time.tv_nsec - session->start_time.tv_nsec);
  return (sec_diff * 1000.0) + (nsec_diff / 1000000.0);
}
