#include "ctest_cli/executor.h"
#include "ctest_cli/session.h"
#include <ctest/ctest.h>
#include <time.h>

#define SUITE_NAME test_session

CTEST(SUITE_NAME, test_update_session) {
  size_t n_suites = 2;
  size_t n_runs = 7;
  size_t n_crashes = 1;
  size_t n_fails = 2;
  size_t n_timeouts = 0;

  SessionMetrics session = {.total_suites = n_suites,
                            .total_runs = n_runs,
                            .total_crashes = n_crashes,
                            .total_failures = n_fails,
                            .total_timeouts = n_timeouts};

  SuiteMetrics suite = {
      .total_runs = 5, .total_failures = 3, .state = SUITE_DEFAULT};

  ctest_update_session(&session, &suite);

  ASSERT_INT_EQ(session.total_suites - n_suites, 1,
                "Updating session should increment suite count by 1");
  ASSERT_INT_EQ(session.total_runs - n_runs, suite.total_runs,
                "Total runs should increment by the number of runs in suite");
  ASSERT_INT_EQ(
      session.total_failures - n_fails, suite.total_failures,
      "Total failures should increment by number of failures in suite");
  ASSERT_INT_EQ(session.total_crashes - n_crashes, 0,
                "Total crashes should not increment if suite did not crash");

  SuiteMetrics crashed_suite = {0};
  crashed_suite.state = SUITE_CRASH;
  ctest_update_session(&session, &crashed_suite);

  ASSERT_INT_EQ(session.total_crashes - n_crashes, 1,
                "Total crashes should increment by 1 if suite crashed");

  SuiteMetrics timedout_suite = {0};
  timedout_suite.state = SUITE_TIMEOUT;
  ctest_update_session(&session, &timedout_suite);

  ASSERT_INT_EQ(session.total_timeouts - n_timeouts, 1,
                "Total timeouts should increment by 1 if suite timed out");
}

CTEST(SUITE_NAME, test_session_start) {
  SessionMetrics session = {.total_runs = 59,
                            .total_failures = 43,
                            .total_suites = 18,
                            .total_timeouts = 2,
                            .end_time = {.tv_nsec = 8593L, .tv_sec = 384}};
  struct timespec start = session.start_time;

  ctest_session_start(&session);

  ASSERT_INT_EQ(
      session.total_runs, 0,
      "ctest_session_start() call should zero-out SessionMetrics.total_runs");
  ASSERT_INT_EQ(session.total_failures, 0,
                "ctest_session_start() call should zero-out "
                "SessionMetrics.total_failures");
  ASSERT_INT_EQ(
      session.total_suites, 0,
      "ctest_session_start() call should zero-out SessionMetrics.total_suites");
  ASSERT_INT_EQ(session.total_timeouts, 0,
                "ctest_session_start() call should zero-out "
                "SessionMetrics.total_timeouts");
  ASSERT(session.end_time.tv_nsec == 0 && session.end_time.tv_sec == 0,
         "ctest_session_start() call should zero-out SessionMetrics.end_time");

  ASSERT(start.tv_nsec != session.start_time.tv_nsec &&
             start.tv_sec != session.start_time.tv_sec,
         "ctest_session_start() call should update SessionMetrics.start_time");
}

CTEST(SUITE_NAME, test_session_end) {
  SessionMetrics session = {0};
  struct timespec end = session.end_time;
  ctest_session_end(&session);

  ASSERT(end.tv_nsec != session.end_time.tv_nsec &&
             end.tv_sec != session.end_time.tv_sec,
         "ctest_session_end() call should update SessionMetrics.end_time");
}

CTEST(SUITE_NAME, test_session_get_duration_ms) {
  SessionMetrics session = {0};

  // diff == 1000ms
  session.start_time.tv_sec = 1;
  session.end_time.tv_sec = 2;

  // diff == 101.24ms
  session.start_time.tv_nsec = 111100000;
  session.end_time.tv_nsec = 212340000;

  ASSERT_DOUBLE_EQ(ctest_session_get_duration_ms(&session), 1101.24, 0.001,
                   "ctest_session_get_duration_ms() call should return an "
                   "accurately calculated duration value");
}
