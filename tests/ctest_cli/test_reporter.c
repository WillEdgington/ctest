#include "ctest_cli/config.h"
#include "ctest_cli/reporter.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <string.h>

#define CAPTURE_BUF_SIZE 2048
#define LEDGER_ITEM_SIZE 256

static void test_report_start_banner_normal(void) {
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

static void test_report_start_banner_quiet(void) {
  const char *root_dir = "tests/fixtures";
  char out_buf[CAPTURE_BUF_SIZE];

  ctest_capture_stdout_start();
  ctest_report_start_banner(root_dir, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT(out_buf[0] == '\0',
         "Start banner should produce no output in QUIET mode");
}

static void test_report_start_banner(void) {
  test_report_start_banner_normal();
  test_report_start_banner_quiet();
}

void test_reporter_verbose_suite_start(void) {
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

static void test_report_suite_metrics_pass(void) {
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

static void test_report_suite_metrics_fail(void) {
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

static void test_report_suite_metrics_crash(void) {
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

static void test_report_suite_metrics_timeout(void) {
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

static void test_report_suite_metrics_quiet(void) {
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

static void test_report_suite_metrics(void) {
  test_report_suite_metrics_pass();
  test_report_suite_metrics_fail();
  test_report_suite_metrics_crash();
  test_report_suite_metrics_timeout();
  test_report_suite_metrics_quiet();
}

static void test_report_ledger_normal(void) {
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

static void test_report_ledger_quiet(void) {
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

static void test_report_ledger_empty(void) {
  Vector ledger;
  vector_init(&ledger, LEDGER_ITEM_SIZE);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_ledger(&ledger, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT(out_buf[0] == '\0', "Empty ledger should produce no output");

  vector_free(&ledger);
}

static void test_report_ledger(void) {
  test_report_ledger_normal();
  test_report_ledger_quiet();
  test_report_ledger_empty();
}

static void test_report_summary_all_passed(void) {
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

static void test_report_summary_with_failures(void) {
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

static void test_report_summary_quiet_all_passed(void) {
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

static void test_report_summary_quiet_with_failures(void) {
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

static void test_report_summary(void) {
  test_report_summary_all_passed();
  test_report_summary_with_failures();
  test_report_summary_quiet_all_passed();
  test_report_summary_quiet_with_failures();
}

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_report_start_banner();
  test_report_suite_metrics();
  test_report_ledger();
  test_report_summary();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
