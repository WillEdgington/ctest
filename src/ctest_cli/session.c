#include "session.h"
#include "executor.h"

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
