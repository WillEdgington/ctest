#include "ctest_cli/config.h"
#include "ctest_cli/executor.h"
#include "ctest_cli/reporter.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <string.h>

#define SUITE_NAME test_reporter

#define CAPTURE_BUF_SIZE 2048

// -- ctest_report_start_banner() tests --

CTEST(SUITE_NAME, test_report_start_banner_normal) {
  const char *root_dir = "tests/fixtures";
  char out_buf[CAPTURE_BUF_SIZE];

  ctest_capture_stdout_start();
  ctest_report_start_banner(root_dir, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "RUNNING CTEST ON: tests/fixtures"),
      "Start banner should contain root dir in CTEST_VERBOSITY_NORMAL mode");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "\n======"),
                      "Start banner should include horizontal margins in "
                      "CTEST_VERBOSITY_NORMAL mode");
}

CTEST(SUITE_NAME, test_report_start_banner_quiet) {
  const char *root_dir = "tests/fixtures";
  char out_buf[CAPTURE_BUF_SIZE];

  ctest_capture_stdout_start();
  ctest_report_start_banner(root_dir, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT(out_buf[0] == '\0',
         "Start banner should produce no output in CTEST_VERBOSITY_QUIET mode");
}

// -- ctest_report_suite_metrics() tests --

CTEST(SUITE_NAME, test_report_suite_metrics_pass_normal) {
  const char *bin_path = "tests/fake_passing_test";
  SuiteMetrics metrics = {.file_path = "tests/fake_passing_test.c"};

  ctest_suite_metrics_init(&metrics);

  TestCaseResult pass_case = {.status = TEST_PASS};

  vector_push(&metrics.test_results, &pass_case);
  vector_push(&metrics.test_results, &pass_case);
  vector_push(&metrics.test_results, &pass_case);
  metrics.total_runs = 3;
  metrics.state = SUITE_DEFAULT;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, CTEST_COLOR_GREEN "[PASS]"),
                      "Passed suite output should contain green [PASS] tag");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, bin_path),
                      "Passed suite output should contain binary path");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, CTEST_COLOR_GREEN "..."),
      "Passed suite should output green dot indicators for passes");

  ctest_suite_metrics_cleanup(&metrics);
}

CTEST(SUITE_NAME, test_report_suite_metrics_fail) {
  const char *bin_path = "tests/fake_failing_test";
  SuiteMetrics metrics = {.file_path = "tests/fake_failing_test.c"};
  ctest_suite_metrics_init(&metrics);

  TestCaseResult pass_case = {.status = TEST_PASS};
  TestCaseResult fail_case = {.status = TEST_FAIL};

  vector_push(&metrics.test_results, &pass_case);
  vector_push(&metrics.test_results, &fail_case);
  vector_push(&metrics.test_results, &pass_case);
  metrics.total_runs = 3;
  metrics.total_failures = 1;
  metrics.state = SUITE_DEFAULT;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, CTEST_COLOR_RED "[FAIL]"),
                      "Failed suite output should contain red [FAIL] tag");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, bin_path),
                      "Failed suite output should contain binary path");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, CTEST_COLOR_GREEN
                             "." CTEST_COLOR_RED "F" CTEST_COLOR_GREEN "."),
                      "Failed suite output should print green dot and red F "
                      "indicators in order of case pushed to SuiteMetrics");

  ctest_suite_metrics_cleanup(&metrics);
}

CTEST(SUITE_NAME, test_report_suite_metrics_crash_normal) {
  const char *bin_path = "tests/fake_crashing_test";
  SuiteMetrics metrics = {.file_path = "tests/fake_crashing_test.c"};
  ctest_suite_metrics_init(&metrics);
  metrics.state = SUITE_CRASH;

  TestCaseResult pass_case = {.status = TEST_PASS};
  vector_push(&metrics.test_results, &pass_case);
  metrics.total_runs = 1;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, CTEST_COLOR_YELLOW "[CRASH]"),
                      "Crashed suite output should contain yellow [CRASH] tag");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, CTEST_COLOR_GREEN "." CTEST_COLOR_YELLOW "C"),
      "Crashed suite outcome string should contain yellow 'C' after ran test "
      "case signals");

  ctest_suite_metrics_cleanup(&metrics);
}

CTEST(SUITE_NAME, test_report_suite_metrics_timeout) {
  const char *bin_path = "tests/fake_timeout_test";
  SuiteMetrics metrics = {.file_path = "tests/fake_timeout_test.c"};
  ctest_suite_metrics_init(&metrics);
  metrics.state = SUITE_TIMEOUT;

  TestCaseResult pass_case = {.status = TEST_PASS};
  vector_push(&metrics.test_results, &pass_case);
  metrics.total_runs = 1;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, CTEST_COLOR_CYAN "[TIME]"),
                      "Timed out suite output should contain cyan [TIME] tag");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, CTEST_COLOR_GREEN "." CTEST_COLOR_CYAN "T"),
      "Timed out suite outcome string should contain cyan 'T' after ran test "
      "case signals");

  ctest_suite_metrics_cleanup(&metrics);
}

CTEST(SUITE_NAME, test_report_suite_metrics_verbose) {
  const char *bin_path = "tests/fake_test";
  SuiteMetrics metrics;
  ctest_suite_metrics_init(&metrics);
  strcpy(metrics.file_path, "tests/test_demo.c");

  TestCaseResult tc1 = {.line_num = 15,
                        .status = TEST_PASS,
                        .expr = "x == 10",
                        .msg = "x is ten"};
  TestCaseResult tc2 = {
      .line_num = 20, .status = TEST_FAIL, .expr = "y == 20", .msg = ""};

  vector_push(&metrics.test_results, &tc1);
  vector_push(&metrics.test_results, &tc2);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_VERBOSE);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "\n[RUN] tests/fake_test\n"),
                      "Verbose output should include [RUN] header");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "Line 15 in tests/test_demo.c"),
                      "Verbose output should contain line number and file");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "x is ten (x == 10)"),
                      "Verbose output should format custom message and expr");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "--- (y == 20)"),
                      "Verbose output should print --- when msg is empty");

  ctest_suite_metrics_cleanup(&metrics);
}

CTEST(SUITE_NAME, test_report_suite_metrics_quiet) {
  const char *bin_path = "tests/fake_passing_test";
  SuiteMetrics metrics = {.file_path = "tests/fake_passing_test.c"};
  ctest_suite_metrics_init(&metrics);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_suite_metrics(bin_path, &metrics, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT(
      out_buf[0] == '\0',
      "Suite metrics should produce no output in CTEST_VERBOSITY_QUIET mode");

  ctest_suite_metrics_cleanup(&metrics);
}

// -- ctest_report_ledger() tests --

CTEST(SUITE_NAME, test_report_ledger_normal) {
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  FailureEntry entry_case = {.type = FAILURE_TEST_CASE,
                             .file_path = "tests/test_a.c",
                             .test_case = {.line_num = 42,
                                           .expr = "x == y",
                                           .msg = "Expected 5, got 3",
                                           .status = TEST_FAIL}};

  FailureEntry entry_crash = {.type = FAILURE_SUITE_CRASH,
                              .file_path = "tests/test_b.c",
                              .signal_num = 11};

  FailureEntry entry_timeout = {.type = FAILURE_SUITE_TIMEOUT,
                                .file_path = "tests/test_c.c",
                                .timeout_sec = 5};

  vector_push(&ledger, &entry_case);
  vector_push(&ledger, &entry_crash);
  vector_push(&ledger, &entry_timeout);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_ledger(&ledger, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, CTEST_COLOR_RED " FAILURE REPORT "),
      "Ledger report in NORMAL mode should contain red report header");

  ASSERT_PTR_NOT_NULL(
      strstr(out_buf,
             CTEST_COLOR_RED "[FAIL]" CTEST_COLOR_RESET " Expected 5, got 3\n"),
      "Ledger report should contain test failure message for failed test case");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "Expression: x == y\n"),
      "Ledger report should contain expression of test case failure");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "Location  : Line 42 in tests/test_a.c\n"),
      "Ledger report should contain location of test failure");

  ASSERT_PTR_NOT_NULL(
      strstr(out_buf,
             CTEST_COLOR_YELLOW "[CRASH]" CTEST_COLOR_RESET
                                " Suite execution terminated unexpectedly\n"),
      "Ledger report should contain crash message for crashed test suite");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "Signal    : Terminated by signal 11\n"),
                      "Ledger report output should contain crash signal info");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "Location  : tests/test_b.c\n"),
      "Ledger report should contain location of crashed test suite file");

  ASSERT_PTR_NOT_NULL(strstr(out_buf,
                             CTEST_COLOR_CYAN "[TIME]" CTEST_COLOR_RESET
                                              " Suite execution timed out\n"),
                      "Ledger report should contain test timeout message for "
                      "timed out test suite");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "Limit     : Exceeded 5 second(s) threshold\n"),
      "Ledger report output should contain timeout threshold");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "Location  : tests/test_c.c\n"),
      "Ledger report should contain location of timed out test suite file");

  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_report_ledger_quiet) {
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  FailureEntry entry_case = {.type = FAILURE_TEST_CASE,
                             .file_path = "tests/test_a.c",
                             .test_case = {.line_num = 42,
                                           .expr = "x == y",
                                           .msg = "Expected 5, got 3",
                                           .status = TEST_FAIL}};

  vector_push(&ledger, &entry_case);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_ledger(&ledger, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NULL(strstr(out_buf, CTEST_COLOR_RED " FAILURE REPORT "),
                  "Ledger report header should be suppressed in QUIET mode");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "Location  : Line 42 in tests/test_a.c\n"),
      "Ledger entries should still print in QUIET mode");

  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_report_ledger_empty) {
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_ledger(&ledger, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT(out_buf[0] == '\0', "Empty ledger should produce no output");

  vector_free(&ledger);
}

// -- ctest_report_summary() tests --

CTEST(SUITE_NAME, test_report_summary_all_passed_normal) {
  SessionMetrics session = {.total_suites = 3,
                            .total_runs = 15,
                            .total_failures = 0,
                            .total_crashes = 0,
                            .total_timeouts = 0};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_summary(&session, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, CTEST_COLOR_GREEN "ALL " CTEST_COLOR_RESET
                                                        "3" CTEST_COLOR_GREEN
                                                        " SUITES PASSED"),
                      "Passing summary should indicate all suites passed");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "\n======"),
      "CTEST_VERBOSITY_NORMAL mode summary should include horizontal margins");
}

CTEST(SUITE_NAME, test_report_summary_with_failures_normal) {
  SessionMetrics session = {.total_suites = 5,
                            .total_runs = 20,
                            .total_failures = 3,
                            .total_crashes = 1,
                            .total_timeouts = 2};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_summary(&session, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "SUITES: 5"),
                      "Summary should display total suites count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "TESTS: 20"),
                      "Summary should display total run count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "PASSED: " CTEST_COLOR_GREEN "17"),
                      "Summary should display tests passed count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "FAILED: " CTEST_COLOR_RED "3"),
                      "Summary should display test failure count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "CRASHED: " CTEST_COLOR_YELLOW "1"),
                      "Summary should display crashed suites count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "TIMEOUTS: " CTEST_COLOR_CYAN "2"),
                      "Summary should display timed out suites count");
  ASSERT_PTR_NOT_NULL(
      strstr(out_buf, "\n====="),
      "CTEST_VERBOSITY_NORMAL mode summary should include horizontal margins");
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
      strstr(out_buf, CTEST_COLOR_GREEN "ALL " CTEST_COLOR_RESET
                                        "3" CTEST_COLOR_GREEN " SUITES PASSED"),
      "Quiet passing summary should indicate all suites passed");
  ASSERT_PTR_NULL(strstr(out_buf, "\n====="),
                  "Quiet summary should omit horizontal margin characters");
}

CTEST(SUITE_NAME, test_report_summary_quiet_with_failures) {
  SessionMetrics session = {.total_suites = 4,
                            .total_runs = 17,
                            .total_failures = 2,
                            .total_crashes = 2,
                            .total_timeouts = 1};

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  ctest_report_summary(&session, CTEST_VERBOSITY_QUIET);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_PTR_NOT_NULL(strstr(out_buf, "SUITES: 4"),
                      "Quiet summary should display total suites count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "TESTS: 17"),
                      "Quiet summary should display total run count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "PASSED: " CTEST_COLOR_GREEN "15"),
                      "Quiet summary should display tests passed count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "FAILED: " CTEST_COLOR_RED "2"),
                      "Quiet summary should display test failure count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "CRASHED: " CTEST_COLOR_YELLOW "2"),
                      "Quiet summary should display crashed suites count");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "TIMEOUTS: " CTEST_COLOR_CYAN "1"),
                      "Quiet summary should display timed out suites count");
  ASSERT_PTR_NULL(strstr(out_buf, "\n====="),
                  "Quiet summary should omit horizontal margin characters");
}
