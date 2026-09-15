#define _GNU_SOURCE
#include "ctest_cli/config.h"
#include "ctest_cli/executor.h"
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SUITE_NAME test_executor

static size_t CAPTURE_BUF_SIZE = 2048;

static const char *mock_passing_bin = "./mock_passing_bin_35893333";
static const char *mock_passing_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "5" CTEST_TEST_DELIM "0\\n\");\n"
    "  return 0;\n"
    "}\n";

static const char *mock_failing_bin = "./mock_failing_bin_78677545";
static const char *mock_failing_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"FAIL" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "12" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM "<message>\\n\");\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "2" CTEST_TEST_DELIM "1\\n\");\n"
    "  return 0;\n"
    "}\n";

static const char *mock_crashing_bin = "./mock_crashing_bin_48691321";
static const char *mock_crashing_c_code = "#include <stdlib.h>\n"
                                          "int main(void) {\n"
                                          "  int *p = NULL;\n"
                                          "  *p = 42; /* Triggers SIGSEGV */\n"
                                          "  return 0;\n"
                                          "}\n";

// sleeps for 1.1s, set timeout to 1s
static const char *mock_timeout_bin = "./mock_timeout_bin_99182736";
static const char *mock_timeout_c_code = "#include <unistd.h>\n"
                                         "int main(void) {\n"
                                         "  usleep(1100000);\n"
                                         "  return 0;\n"
                                         "}\n";

CTEST(SUITE_NAME, test_executor_async_primitives_passing) {
  ctest_setup_mock_binary(mock_passing_bin, mock_passing_c_code);

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
  ASSERT_INT_EQ(metrics.total_runs, 5, "Parsed 4 total runs via async harvest");
  ASSERT_INT_EQ(metrics.total_failures, 0,
                "Parsed 0 failures via async harvest");

  vector_free(&ledger);
  ctest_teardown_mock_binary(mock_passing_bin);
}

CTEST(SUITE_NAME, test_executor_async_primitives_timeout) {
  ctest_setup_mock_binary(mock_timeout_bin, mock_timeout_c_code);

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
