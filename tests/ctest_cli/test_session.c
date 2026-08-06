#include "ctest_cli/executor.h"
#include "ctest_cli/session.h"
#include <ctest/ctest.h>

void test_update_session(void) {
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
}

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_update_session();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
