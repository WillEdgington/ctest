#include "ctest_cli/reporter.h"
#include <clib/vector.h>
#include <ctest/ctest.h>

#define CAPTURE_BUF_SIZE 2048
#define LEDGER_ITEM_SIZE 256

static void test_report_suite_metrics_pass(void) {
  const char *bin_path = "tests/fake_passing_test";
  SuiteMetrics metrics = {
      .total_runs = 6, .total_failures = 0, .state = SUITE_DEFAULT};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics);
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
  ctest_report_suite_metrics(bin_path, &metrics);
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
  ctest_report_suite_metrics(bin_path, &metrics);
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
  ctest_report_suite_metrics(bin_path, &metrics);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "[TIME]"),
                      "Timed out suite output should contain [TIME] tag");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, bin_path),
                      "Timed out suite output should contain binary path");
}

static void test_report_suite_metrics(void) {
  test_report_suite_metrics_pass();
  test_report_suite_metrics_fail();
  test_report_suite_metrics_crash();
  test_report_suite_metrics_timeout();
}

static void test_report_ledger(void) {
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
  ctest_report_ledger(&ledger);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "tests/test_a.c"),
                      "Ledger report output should contain first failure path");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "Terminated by signal 11"),
      "Ledger report output should contain crash signal message");

  vector_free(&ledger);
}

static void test_report_summary_all_passed(void) {
  SessionMetrics session = {.total_suites = 3,
                            .total_runs = 15,
                            .total_failures = 0,
                            .total_crashes = 0,
                            .total_timeouts = 0};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_summary(&session);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "ALL"),
                      "Passing summary should indicate all tests passed");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "3"),
                      "Passing summary should report total suite count");
}

static void test_report_summary_with_failures(void) {
  SessionMetrics session = {.total_suites = 5,
                            .total_runs = 20,
                            .total_failures = 3,
                            .total_crashes = 1,
                            .total_timeouts = 2};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_summary(&session);
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
}

static void test_report_summary(void) {
  test_report_summary_all_passed();
  test_report_summary_with_failures();
}

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_report_suite_metrics();
  test_report_ledger();
  test_report_summary();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
