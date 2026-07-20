#include "ctest_cli/executor.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *mock_passing_bin = "./mock_passing_bin_35893333";
static const char *mock_failing_bin = "./mock_failing_bin_78677545";
static const char *mock_crashing_bin = "./mock_crashing_bin_48691321";

static void test_executor_passing_suite(void) {
  ctest_setup_mock_binary(mock_passing_bin, "#include <stdio.h>\n"
                                            "int main(void) {\n"
                                            "  printf(\"SUMMARY|5|0\\n\");\n"
                                            "  return 0;\n"
                                            "}\n");

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_LINE_LEN);

  SuiteMetrics metrics = ctest_execute_suite(mock_passing_bin, &ledger);

  ASSERT_INT_EQ(metrics.total_runs, 5,
                "Total runs parsed correctly for passing suite");
  ASSERT_INT_EQ(metrics.total_failures, 0,
                "Total failures parsed correctly for passing suite");
  ASSERT_INT_EQ(metrics.crashed, 0, "Passing suite did not crash");
  ASSERT_INT_EQ((int)ledger.count, 0, "Ledger remains empty for passing suite");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_passing_bin);
}

static void test_executor_failing_suite(void) {
  ctest_setup_mock_binary(mock_failing_bin,
                          "#include <stdio.h>\n"
                          "int main(void) {\n"
                          "  printf(\"FAIL|Line 12 in test.c\\n\");\n"
                          "  printf(\"SUMMARY|2|1\\n\");\n"
                          "  return 0;\n"
                          "}\n");

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_LINE_LEN);

  SuiteMetrics metrics = ctest_execute_suite(mock_failing_bin, &ledger);

  ASSERT_INT_EQ(metrics.total_runs, 2,
                "Total runs parsed correctly for failing suite");
  ASSERT_INT_EQ(metrics.total_failures, 1,
                "Total failures parsed correctly for failing suite");
  ASSERT_INT_EQ(metrics.crashed, 0, "Failing suite did not crash");
  ASSERT_INT_EQ((int)ledger.count, 1, "Ledger caught right amount of failures");

  char *captured_fail = (char *)vector_get(&ledger, 0);
  ASSERT_STR_EQ(captured_fail, "FAIL|Line 12 in test.c\n",
                "Captured exact failure string");

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

  SuiteMetrics metrics = ctest_execute_suite(mock_crashing_bin, &ledger);

  ASSERT_INT_EQ(metrics.crashed, 1, "Suite detected crash");
  ASSERT_INT_EQ((int)ledger.count, 1, "Ledger caught crash packet");

  char *captured_crash = (char *)vector_get(&ledger, 0);
  ASSERT(strstr(captured_crash, "CRASH|") != NULL,
         "Packet contains CRASH prefix");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_crashing_bin);
}

int main(void) {
  printf("\nRunning: %s...", __FILE__);

  test_executor_passing_suite();
  test_executor_failing_suite();
  test_executor_crashing_suite();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
