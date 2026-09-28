#define _GNU_SOURCE
#include <ctest/ctest.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SUITE_NAME test_libctest

static size_t BUF_SIZE = 128;

CTEST(SUITE_NAME, test_basic_assertions) {
  ASSERT(1 == 1, "One should equal one");
  ASSERT(5 > 3, "Five should be greater than three");

  ASSERT_INT_EQ(42, 42, "Integer equality match");
  ASSERT_INT_EQ(-10, -10, "Negative integer match");

  ASSERT_STR_EQ("hello", "hello", "Identical string matching");
  ASSERT_STR_EQ(NULL, NULL, "NULL string matching");

  int x = 100;
  int *ptr = &x;
  ASSERT_PTR_NOT_NULL(ptr, "Pointer should not be NULL");
  ASSERT_PTR_NULL(NULL, "Pointer should be NULL");

  ASSERT_DOUBLE_EQ(3.14159, 3.14000, 0.002,
                   "Double matching within epsilon tolerance");
}

// this test is weak. Apart from the first assertion the rest has not aged well
// for the current ctest runtime. needs a rewrite.
CTEST(SUITE_NAME, test_output_muting) {
  int saved_stdout = ctest_mute_output(STDOUT_FILENO);
  ASSERT(saved_stdout >= 0,
         "Muting stdout should return a valid backup file descriptor");

  printf("THIS SHOULD BE HIDDEN FROM THE TERMINAL STANDARD OUTPUT STREAM\n");

  ctest_unmute_output(saved_stdout, STDOUT_FILENO);
  printf("  [INFO] Terminal output unmuted successfully.\n");
}

CTEST(SUITE_NAME, test_stdout_capturing) {
  char buf[BUF_SIZE];

  int start_res = ctest_capture_stdout_start();
  printf("CAPTURE_TEST_PAYLOAD");
  ssize_t bytes = ctest_capture_stdout_end(buf, sizeof(buf));

  ASSERT_INT_EQ(start_res, 0, "Stdout capture setup succeeds");
  ASSERT(bytes > 0, "Captured bytes should be greater than zero");
  ASSERT_STR_EQ(buf, "CAPTURE_TEST_PAYLOAD",
                "Captured string matches stdout output");
}

static void func_abort_sig(void *arg) {
  (void)arg;

  // stop ASan intercepting SIGABRT in child
  signal(SIGABRT, SIG_DFL);

  abort();
}

CTEST(SUITE_NAME, test_assert_signal_success) {
  ASSERT_SIGNAL(func_abort_sig, NULL, SIGABRT,
                "ASSERT_SIGNAL() should pass if expected signal (SIGABRT) is "
                "raised in the input func");
}

static void func_exit_code_forty(void *arg) {
  (void)arg;
  exit(40);
}

CTEST(SUITE_NAME, test_assert_exit_code_success) {
  ASSERT_EXIT_CODE(func_exit_code_forty, NULL, 40,
                   "ASSERT_EXIT_CODE() should pass if expected exit code (40) "
                   "is the actual exit code in the input func");
}

CTEST(SUITE_NAME, test_report_result_standalone_mode_pass) {
  const char *runner_active = getenv("CTEST_RUNNER");
  bool is_runner = (runner_active && strcmp(runner_active, "1") == 0);
  if (is_runner)
    unsetenv("CTEST_RUNNER");

  ctest_capture_stdout_start();
  ctest_report_result(true, "test.c", 10, "1 == 1", "Val: %d", 53);
  char buf[512];
  ctest_capture_stdout_end(buf, sizeof(buf));

  const char *expected_pass =
      "  " CTEST_COLOR_GREEN "[PASS]" CTEST_COLOR_RESET " Val: 53\n";

  if (is_runner)
    setenv("CTEST_RUNNER", "1", 1);

  ASSERT_STR_EQ(buf, expected_pass,
                "Test pass output from ctest_report_result() call in "
                "standalone mode should match the expected formatted string");
}

CTEST(SUITE_NAME, test_report_result_runner_mode_pass) {
  const char *runner_active = getenv("CTEST_RUNNER");
  bool is_runner = (runner_active && strcmp(runner_active, "1") == 0);
  if (!is_runner)
    setenv("CTEST_RUNNER", "1", 1);

  ctest_capture_stdout_start();
  ctest_report_result(true, "test.c", 10, "1 == 1", "Val: %d", 29);
  char buf[512];
  ctest_capture_stdout_end(buf, sizeof(buf));

  const char *expected_pass =
      "PASS" CTEST_TEST_DELIM "test.c" CTEST_TEST_DELIM "10" CTEST_TEST_DELIM
      "1 == 1" CTEST_TEST_DELIM "Val: 29\n";

  if (!is_runner)
    unsetenv("CTEST_RUNNER");

  ASSERT_STR_EQ(buf, expected_pass,
                "Test pass output from ctest_report_result() call in runner "
                "mode should match the expecter delimiter-separated record");
}

CTEST(SUITE_NAME, test_report_result_runner_mode_fail) {
  const char *runner_active = getenv("CTEST_RUNNER");
  bool is_runner = (runner_active && strcmp(runner_active, "1") == 0);
  if (!is_runner)
    setenv("CTEST_RUNNER", "1", 1);

  ctest_capture_stdout_start();
  ctest_report_result(false, "fail.c", 13, "1 != 1", "This failed");
  char buf[512];
  ctest_capture_stdout_end(buf, sizeof(buf));

  const char *expected_fail =
      "FAIL" CTEST_TEST_DELIM "fail.c" CTEST_TEST_DELIM "13" CTEST_TEST_DELIM
      "1 != 1" CTEST_TEST_DELIM "This failed\n";

  if (!is_runner)
    unsetenv("CTEST_RUNNER");

  ASSERT_STR_EQ(buf, expected_fail,
                "Test fail output from ctest_report_result() call in runner "
                "mode should match the expecter delimiter-separated record");
}
