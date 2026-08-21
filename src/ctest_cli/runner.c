#include "runner.h"
#include "config.h"
#include "discovery.h"
#include "executor.h"
#include "filter.h"
#include "json.h"
#include "pool.h"
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
  if (ledger_init(&ledger) == -1) {
    fprintf(stderr, "ctest: failed to initialise failure ledger\n");
    return -1;
  }

  ctest_report_start_banner(config->target_dir, config->verbosity);

  Vector *test_bins = ctest_discover_tests(config->target_dir);
  if (test_bins == NULL) {
    fprintf(stderr, "ctest: failed to discover test binaries at %s\n",
            config->target_dir);
    ledger_free(&ledger);
    return -1;
  }

  if (ctest_pool_run(test_bins, config, &session, &ledger) != 0) {
    ledger_free(&ledger);
    test_binaries_free(test_bins);
    return -1;
  }

  test_binaries_free(test_bins);

  ctest_report_ledger(&ledger, config->verbosity);
  ctest_report_summary(&session, config->verbosity);

  if (config->json_output_path != NULL) {
    if (ctest_json_write(config->json_output_path, &session, &ledger) != 0) {
      fprintf(stderr, "ctest: could no write JSON report to %s\n",
              config->json_output_path);
      ledger_free(&ledger);
      return -1;
    }
  }

  ledger_free(&ledger);
  return session_exit(&session);
}
