#include "ctest_cli/config.h"
#include "ctest_cli/reporter.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <string.h>

#define SUITE_NAME test_reporter

#define CAPTURE_BUF_SIZE 2048
#define LEDGER_ITEM_SIZE 256

CTEST(SUITE_NAME, test_report_start_banner_normal) {
  const char *root_dir = "tests/fixtures";
  char out_buf[CAPTURE_BUF_SIZE];

  ctest_capture_stdout_start();
  ctest_report_start_banner(root_dir, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "RUNNING CTEST ON: tests/fixtures"),
                      "Start banner should contain root dir in NORMAL mode");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "="),
      "Start banner should include horizontal margins in NORMAL mode");
}

CTEST(SUITE_NAME, test_report_start_banner_quiet) {
  const char *root_dir = "tests/fixtures";
  char out_buf[CAPTURE_BUF_SIZE];

  ctest_capture_stdout_start();
  ctest_report_start_banner(root_dir, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT(out_buf[0] == '\0',
         "Start banner should produce no output in QUIET mode");
}

CTEST(SUITE_NAME, test_reporter_verbose_suite_start) {
  char out_buf[CAPTURE_BUF_SIZE];

  ctest_capture_stdout_start();
  ctest_report_suite_start("./tests/mock_bin", CTEST_VERBOSITY_VERBOSE);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "[RUN] ./tests/mock_bin"),
                      "Verbose suite start outputs '[RUN] <bin_path>'");

  memset(out_buf, 0, sizeof(out_buf));
  ctest_capture_stdout_start();
  ctest_report_suite_start("./tests/mock_bin", CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT(out_buf[0] == '\0', "Normal suite start produces no banner output");
}

CTEST(SUITE_NAME, test_report_suite_metrics_pass) {
  const char *bin_path = "tests/fake_passing_test";
  SuiteMetrics metrics = {
      .total_runs = 6, .total_failures = 0, .state = SUITE_DEFAULT};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "[PASS]"),
                      "Passed suite output should contain [PASS] tag");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, bin_path),
                      "Passed suite output should contain binary path");
}

CTEST(SUITE_NAME, test_report_suite_metrics_fail) {
  const char *bin_path = "tests/fake_failing_test";
  SuiteMetrics metrics = {
      .total_runs = 6, .total_failures = 2, .state = SUITE_DEFAULT};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "[FAIL]"),
                      "Failed suite output should contain [FAIL] tag");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, bin_path),
                      "Failed suite output should contain binary path");
}

CTEST(SUITE_NAME, test_report_suite_metrics_crash) {
  const char *bin_path = "tests/fake_crashing_test";
  SuiteMetrics metrics = {
      .total_runs = 6, .total_failures = 3, .state = SUITE_CRASH};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "[CRASH]"),
                      "Crashed suite output should contain [CRASH] tag");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, bin_path),
                      "Crashed suite output should contain binary path");
}

CTEST(SUITE_NAME, test_report_suite_metrics_timeout) {
  const char *bin_path = "tests/fake_timeout_test";
  SuiteMetrics metrics = {
      .total_runs = 2, .total_failures = 0, .state = SUITE_TIMEOUT};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "[TIME]"),
                      "Timed out suite output should contain [TIME] tag");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, bin_path),
                      "Timed out suite output should contain binary path");
}

CTEST(SUITE_NAME, test_report_suite_metrics_quiet) {
  const char *bin_path = "tests/fake_passing_test";
  SuiteMetrics metrics = {
      .total_runs = 6, .total_failures = 0, .state = SUITE_DEFAULT};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT(out_buf[0] == '\0',
         "Suite metrics should produce no output in QUIET mode");
}

CTEST(SUITE_NAME, test_report_ledger_normal) {
  Vector ledger;
  vector_init(&ledger, LEDGER_ITEM_SIZE);

  char entry_1[LEDGER_ITEM_SIZE] = {0};
  char entry_2[LEDGER_ITEM_SIZE] = {0};

  snprintf(entry_1, sizeof(entry_1),
           CTEST_COLOR_RED
           "FAIL|tests/test_a.c|42|x == y|Expected 5, got 3" CTEST_COLOR_RESET
           "\n");
  snprintf(entry_2, sizeof(entry_2),
           CTEST_COLOR_YELLOW
           "CRASH|tests/test_b.c|Terminated by signal 11" CTEST_COLOR_RESET
           "\n");

  vector_push(&ledger, entry_1);
  vector_push(&ledger, entry_2);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_ledger(&ledger, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "FAILURE REPORT"),
                      "Ledger report in NORMAL mode should contain header");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "tests/test_a.c"),
                      "Ledger report output should contain first failure path");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "Terminated by signal 11"),
      "Ledger report output should contain crash signal message");

  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_report_ledger_quiet) {
  Vector ledger;
  vector_init(&ledger, LEDGER_ITEM_SIZE);

  char entry_1[LEDGER_ITEM_SIZE] = {0};
  snprintf(entry_1, sizeof(entry_1),
           CTEST_COLOR_RED
           "FAIL|tests/test_a.c|42|x == y|Expected 5, got 3" CTEST_COLOR_RESET
           "\n");

  vector_push(&ledger, entry_1);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_ledger(&ledger, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NULL(strstr(out_buf, "FAILURE REPORT"),
                  "Ledger report header should be suppressed in QUIET mode");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "tests/test_a.c"),
                      "Ledger entries should still print in QUIET mode");

  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_report_ledger_empty) {
  Vector ledger;
  vector_init(&ledger, LEDGER_ITEM_SIZE);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_ledger(&ledger, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT(out_buf[0] == '\0', "Empty ledger should produce no output");

  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_report_summary_all_passed) {
  SessionMetrics session = {.total_suites = 3,
                            .total_runs = 15,
                            .total_failures = 0,
                            .total_crashes = 0,
                            .total_timeouts = 0};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_summary(&session, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "ALL"),
                      "Passing summary should indicate all tests passed");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "3"),
                      "Passing summary should report total suite count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "====\n"),
                      "NORMAL mode summary should include horizontal margins");
}

CTEST(SUITE_NAME, test_report_summary_with_failures) {
  SessionMetrics session = {.total_suites = 5,
                            .total_runs = 20,
                            .total_failures = 3,
                            .total_crashes = 1,
                            .total_timeouts = 2};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_summary(&session, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "SUITES"),
                      "Summary should display SUITES metrics category");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "5"),
                      "Summary should display total suites count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "20"),
                      "Summary should display total run count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "17"),
                      "Summary should display passed count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "3"),
                      "Summary should display failure count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "1"),
                      "Summary should display crash count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "2"),
                      "Summary should display timeouts count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "====\n"),
                      "NORMAL mode summary should include horizontal margins");
}

CTEST(SUITE_NAME, test_report_summary_quiet_all_passed) {
  SessionMetrics session = {.total_suites = 3,
                            .total_runs = 15,
                            .total_failures = 0,
                            .total_crashes = 0,
                            .total_timeouts = 0};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_summary(&session, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "ALL"),
      "Quiet passing summary should indicate all suites passed");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "3"),
                      "Quiet passing summary should contain total suite count");
  ASSERT_PTR_NULL(strstr(out_buf, "====\n"),
                  "Quiet summary should omit horizontal margin characters");
}

CTEST(SUITE_NAME, test_report_summary_quiet_with_failures) {
  SessionMetrics session = {.total_suites = 5,
                            .total_runs = 20,
                            .total_failures = 3,
                            .total_crashes = 1,
                            .total_timeouts = 2};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_summary(&session, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "SUITES: 5"),
                      "Quiet summary should print unbordered metrics");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "FAILED:"),
                      "Quiet summary should contain FAILED metric label");
  ASSERT_PTR_NULL(strstr(out_buf, "====\n"),
                  "Quiet summary should omit horizontal margin characters");
}
