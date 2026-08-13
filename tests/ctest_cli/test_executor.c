#include "ctest_cli/config.h"
#include "ctest_cli/executor.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static size_t CAPTURE_BUF_SIZE = 2048;

static const char *mock_passing_bin = "./mock_passing_bin_35893333";
static const char *mock_failing_bin = "./mock_failing_bin_78677545";
static const char *mock_crashing_bin = "./mock_crashing_bin_48691321";
static const char *mock_timeout_bin = "./mock_timeout_bin_99182736";

static void test_executor_passing_suite(void) {
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

static void test_executor_failing_suite(void) {
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

static void test_executor_crashing_suite(void) {
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
static void test_executor_timeout_suite(void) {
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

static void test_executor_verbose_passing_suite(void) {
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

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_executor_passing_suite();
  test_executor_failing_suite();
  test_executor_crashing_suite();
  test_executor_timeout_suite();
  test_executor_verbose_passing_suite();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
