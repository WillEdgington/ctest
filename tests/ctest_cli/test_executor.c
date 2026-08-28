#define _GNU_SOURCE
#include "ctest_cli/config.h"
#include "ctest_cli/executor.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SUITE_NAME test_executor

static size_t CAPTURE_BUF_SIZE = 2048;

static const char *mock_passing_bin = "./mock_passing_bin_35893333";
static const char *mock_failing_bin = "./mock_failing_bin_78677545";
static const char *mock_crashing_bin = "./mock_crashing_bin_48691321";
static const char *mock_timeout_bin = "./mock_timeout_bin_99182736";

CTEST(SUITE_NAME, test_executor_passing_suite) {
  ctest_setup_mock_binary(mock_passing_bin,
                          "#include <stdio.h>\n"
                          "int main(void) {\n"
                          "  printf(\"SUMMARY" CTEST_TEST_DELIM
                          "5" CTEST_TEST_DELIM "0\\n\");\n"
                          "  return 0;\n"
                          "}\n");

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_LINE_LEN);

  SuiteMetrics metrics =
      ctest_execute_suite(mock_passing_bin, 0, CTEST_VERBOSITY_NORMAL, &ledger);

  ASSERT_INT_EQ(metrics.total_runs, 5,
                "Total runs parsed correctly for passing suite");
  ASSERT_INT_EQ(metrics.total_failures, 0,
                "Total failures parsed correctly for passing suite");
  ASSERT_INT_EQ(metrics.state, SUITE_DEFAULT, "Passing suite did not crash");
  ASSERT_INT_EQ((int)ledger.count, 0, "Ledger remains empty for passing suite");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_passing_bin);
}

CTEST(SUITE_NAME, test_executor_failing_suite) {
  ctest_setup_mock_binary(
      mock_failing_bin,
      "#include <stdio.h>\n"
      "int main(void) {\n"
      "  printf(\"FAIL" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
      "12" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM "<message>\\n\");\n"
      "  printf(\"SUMMARY" CTEST_TEST_DELIM "2" CTEST_TEST_DELIM "1\\n\");\n"
      "  return 0;\n"
      "}\n");

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_LINE_LEN);

  SuiteMetrics metrics =
      ctest_execute_suite(mock_failing_bin, 0, CTEST_VERBOSITY_NORMAL, &ledger);

  ASSERT_INT_EQ(metrics.total_runs, 2,
                "Total runs parsed correctly for failing suite");
  ASSERT_INT_EQ(metrics.total_failures, 1,
                "Total failures parsed correctly for failing suite");
  ASSERT_INT_EQ(metrics.state, SUITE_DEFAULT, "Failing suite did not crash");
  ASSERT_INT_EQ(ledger.count, 1, "Ledger caught right amount of failures");

  char *captured_fail = (char *)vector_get(&ledger, 0);
  char *fail_pattern =
      "  " CTEST_COLOR_RED "[FAIL]" CTEST_COLOR_RESET " <message>\n"
      "         Expression: <expression>\n"
      "         Location  : Line 12 in <file>\n";

  ASSERT_STR_EQ(captured_fail, fail_pattern, "Captured exact failure string");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_failing_bin);
}

CTEST(SUITE_NAME, test_executor_crashing_suite) {
  ctest_setup_mock_binary(mock_crashing_bin,
                          "#include <stdlib.h>\n"
                          "int main(void) {\n"
                          "  int *p = NULL;\n"
                          "  *p = 42; /* Triggers SIGSEGV */\n"
                          "  return 0;\n"
                          "}\n");

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_LINE_LEN);

  SuiteMetrics metrics = ctest_execute_suite(mock_crashing_bin, 0,
                                             CTEST_VERBOSITY_NORMAL, &ledger);

  ASSERT_INT_EQ(metrics.state, SUITE_CRASH, "Suite detected crash");
  ASSERT_INT_EQ((int)ledger.count, 1, "Ledger caught crash packet");

  char *captured_crash = (char *)vector_get(&ledger, 0);
  ASSERT(strstr(captured_crash, "[CRASH]") != NULL,
         "Packet contains CRASH prefix");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_crashing_bin);
}

// This test seems like it could be done better, but validates well enough for
// now. Need to find a way that it can be done without sleeping for so long.
CTEST(SUITE_NAME, test_executor_timeout_suite) {
  ctest_setup_mock_binary(mock_timeout_bin, "#include <unistd.h>\n"
                                            "int main(void) {\n"
                                            "  usleep(1100000);\n"
                                            "  return 0;\n"
                                            "}\n");

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_LINE_LEN);

  SuiteMetrics metrics =
      ctest_execute_suite(mock_timeout_bin, 1, CTEST_VERBOSITY_NORMAL, &ledger);

  ASSERT_INT_EQ(metrics.state, SUITE_TIMEOUT, "Suite detected timeout");
  ASSERT_INT_EQ((int)ledger.count, 1, "Ledger caught timeout packet");

  char *captured_timeout = (char *)vector_get(&ledger, 0);
  ASSERT(captured_timeout != NULL && strstr(captured_timeout, "[TIME]") != NULL,
         "Packet contains TIMEOUT prefix");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_timeout_bin);
}

CTEST(SUITE_NAME, test_executor_verbose_passing_suite) {
  ctest_setup_mock_binary(
      mock_passing_bin,
      "#include <stdio.h>\n"
      "int main(void) {\n"
      "  printf(\"PASS" CTEST_TEST_DELIM "test.c" CTEST_TEST_DELIM
      "10" CTEST_TEST_DELIM "1 == 1" CTEST_TEST_DELIM "ok\\n\");\n"
      "  printf(\"SUMMARY" CTEST_TEST_DELIM "1" CTEST_TEST_DELIM "0\\n\");\n"
      "  return 0;\n"
      "}\n");

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_LINE_LEN);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  SuiteMetrics metrics = ctest_execute_suite(mock_passing_bin, 0,
                                             CTEST_VERBOSITY_VERBOSE, &ledger);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(metrics.total_runs, 1, "Runs parsed under verbose mode");
  ASSERT(strstr(out_buf, "[PASS]") != NULL,
         "Verbose stdout contains [PASS] tag");
  ASSERT(strstr(out_buf, "1 == 1") != NULL,
         "Verbose stdout contains expression");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_passing_bin);
}

CTEST(SUITE_NAME, test_executor_async_primitives_passing) {
  ctest_setup_mock_binary(mock_passing_bin,
                          "#include <stdio.h>\n"
                          "int main(void) {\n"
                          "  printf(\"SUMMARY" CTEST_TEST_DELIM
                          "4" CTEST_TEST_DELIM "0\\n\");\n"
                          "  return 0;\n"
                          "}\n");

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_LINE_LEN);

  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};

  int launch_res =
      ctest_launch_suite(mock_passing_bin, CTEST_VERBOSITY_NORMAL, &slot);
  ASSERT_INT_EQ(launch_res, 0, "ctest_launch_suite succeeded");
  ASSERT_INT_EQ(slot.is_active, 1, "WorkerSlot set to active post-launch");
  ASSERT(slot.read_fd >= 0, "WorkerSlot holds valid read_fd");

  // Allow process to output data and harvest
  usleep(10000);
  ctest_harvest_output(&slot, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  int fin_res =
      ctest_finalise_suite(&slot, 0, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);
  ASSERT_INT_EQ(fin_res, 0, "ctest_finalise_suite succeeded");
  ASSERT_INT_EQ(slot.is_active, 0, "WorkerSlot marked inactive post-finalise");
  ASSERT_INT_EQ(metrics.total_runs, 4, "Parsed 4 total runs via async harvest");
  ASSERT_INT_EQ(metrics.total_failures, 0,
                "Parsed 0 failures via async harvest");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_passing_bin);
}

CTEST(SUITE_NAME, test_executor_async_primitives_timeout) {
  ctest_setup_mock_binary(mock_timeout_bin, "#include <unistd.h>\n"
                                            "int main(void) {\n"
                                            "  usleep(500000);\n"
                                            "  return 0;\n"
                                            "}\n");

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_LINE_LEN);

  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};

  ctest_launch_suite(mock_timeout_bin, CTEST_VERBOSITY_NORMAL, &slot);

  // Force timeout state without sleeping for the full timeout duration
  slot.timed_out = 1;

  ctest_finalise_suite(&slot, 1, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT_INT_EQ(metrics.state, SUITE_TIMEOUT,
                "Async finalise handled forced timeout correctly");
  ASSERT_INT_EQ((int)ledger.count, 1,
                "Timeout packet captured in failure ledger");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_timeout_bin);
}
