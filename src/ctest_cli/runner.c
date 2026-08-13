#include "runner.h"
#include "config.h"
#include "discovery.h"
#include "executor.h"
#include "filter.h"
#include "reporter.h"
#include "session.h"
#include <clib/iter.h>
#include <clib/vector.h>
#include <stdlib.h>

static int session_exit(SessionMetrics *session) {
  if (session->total_failures == 0 && session->total_crashes == 0 &&
      session->total_timeouts == 0)
    return 0;
  return 1;
}

static int ledger_init(Vector *ledger) {
  return vector_init(ledger, CTEST_MAX_FAIL_LINE_LEN);
}

static void ledger_free(Vector *ledger) { vector_free(ledger); }

static void test_binaries_free(Vector *test_bin) {
  vector_free(test_bin);
  free(test_bin);
}

int ctest_run_session(const CTestConfig *config) {
  SessionMetrics session = {0};
  Vector ledger;
  if (ledger_init(&ledger) == -1)
    return -1;

  ctest_report_start_banner(config->target_dir, config->verbosity);

  Vector *test_bins = ctest_discover_tests(config->target_dir);
  if (test_bins == NULL) {
    ledger_free(&ledger);
    return -1;
  }

  Iter it = vector_iter(test_bins);
  while (it.next(&it) == 0) {
    const char *bin_path = it.current.value;
    if (ctest_filter_matches(bin_path, config->filter_pattern) == 0)
      continue;

    ctest_report_suite_start(bin_path, config->verbosity);

    SuiteMetrics metrics = ctest_execute_suite(bin_path, config->timeout_sec,
                                               config->verbosity, &ledger);

    ctest_report_suite_metrics(bin_path, &metrics, config->verbosity);
    ctest_update_session(&session, &metrics);
  }
  test_binaries_free(test_bins);

  ctest_report_ledger(&ledger, config->verbosity);
  ledger_free(&ledger);

  ctest_report_summary(&session, config->verbosity);

  return session_exit(&session);
}
