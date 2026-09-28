#define _GNU_SOURCE
#include "ctest_cli/config.h"
#include "ctest_cli/discovery.h"
#include "ctest_cli/executor.h"
#include "ctest_cli/pool.h"
#include "ctest_cli/runner.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SUITE_NAME test_pool
#define MOCK_POOL_DIR "sandbox_pool_dir_11222422/"

// 3 passes, return 0
static const char *mock_pass_3_bin = MOCK_POOL_DIR "test_pass_3_bin_85392333";
static const char *mock_pass_3_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "3" CTEST_TEST_DELIM "0\\n\");\n"
    "  return 0;\n"
    "}\n";

// 1 pass, return 0
static const char *mock_pass_1_bin = MOCK_POOL_DIR "test_pass_1_bin_99395859";
static const char *mock_pass_1_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "1" CTEST_TEST_DELIM "0\\n\");\n"
    "  return 0;\n"
    "}\n";

// 1 pass, 2 fails, return 1
static const char *mock_pass_1_fail_2_bin =
    MOCK_POOL_DIR "test_pass_1_fail_2_bin_00829649";
static const char *mock_pass_1_fail_2_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"FAIL" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"FAIL" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "3" CTEST_TEST_DELIM "1\\n\");\n"
    "  return 1;\n"
    "}\n";

// crashes (triggers SIGSEGV)
static const char *mock_crashing_bin =
    MOCK_POOL_DIR "test_crashing_bin_70896493";
static const char *mock_crashing_c_code = "#include <stdlib.h>\n"
                                          "int main(void) {\n"
                                          "  int *p = NULL;\n"
                                          "  *p = 2;\n"
                                          "  return 0;\n"
                                          "}\n";

CTEST_SETUP_SUITE(SUITE_NAME) {
  ctest_setup_mock_dir((const char *)MOCK_POOL_DIR);
}

CTEST(SUITE_NAME, test_pool_all_passing_parallel) {
  const char *mock_pass_1_bin_2 = MOCK_POOL_DIR "test_pass_1_bin_00096458";
  ctest_setup_mock_binary(mock_pass_1_bin, mock_pass_1_c_code);
  ctest_setup_mock_binary(mock_pass_3_bin, mock_pass_3_c_code);
  ctest_setup_mock_binary(mock_pass_1_bin_2, mock_pass_1_c_code);

  Vector *suites = ctest_discover_tests((const char *)MOCK_POOL_DIR);

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_FAIL_LINE_LEN);

  SessionMetrics metrics = {0};

  CTestConfig config;
  ctest_config_init(&config);
  config.jobs = 2;

  int saved_stdout = ctest_mute_output(STDOUT_FILENO);
  int status = ctest_pool_run(suites, &config, &metrics, &ledger);
  ctest_unmute_output(saved_stdout, STDOUT_FILENO);

  ASSERT_INT_EQ(status, 0, "ctest_pool_run returns success status code");
  ASSERT_INT_EQ(metrics.total_suites, 3,
                "Correct number of suites ran stored in session metrics");
  ASSERT_INT_EQ(metrics.total_runs, 5,
                "Aggregated total runs across parallel workers");
  ASSERT_INT_EQ(metrics.total_failures, 0, "Recorded zero assertion failures");
  ASSERT_INT_EQ(ledger.count, 0, "Ledger remains empty for clean pass");

  vector_free(&ledger);
  vector_free(suites);
  free(suites);
  ctest_teardown_mock_binary(mock_pass_1_bin);
  ctest_teardown_mock_binary(mock_pass_3_bin);
  ctest_teardown_mock_binary(mock_pass_1_bin_2);
}

CTEST(SUITE_NAME, test_pool_mixed_outcomes_parallel) {
  ctest_setup_mock_binary(mock_pass_3_bin, mock_pass_3_c_code);
  ctest_setup_mock_binary(mock_pass_1_fail_2_bin, mock_pass_1_fail_2_c_code);
  ctest_setup_mock_binary(mock_crashing_bin, mock_crashing_c_code);

  Vector *suites = ctest_discover_tests((const char *)MOCK_POOL_DIR);

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_FAIL_LINE_LEN);

  SessionMetrics metrics = {0};

  CTestConfig config;
  ctest_config_init(&config);
  config.jobs = 3;

  int saved_stdout = ctest_mute_output(STDOUT_FILENO);
  int status = ctest_pool_run(suites, &config, &metrics, &ledger);
  ctest_unmute_output(saved_stdout, STDOUT_FILENO);

  ASSERT_INT_EQ(status, 0,
                "ctest_pool_run handles mixed execution outcomes cleanly");
  ASSERT_INT_EQ(metrics.total_crashes, 1, "Recorded crashed suite");
  ASSERT_INT_EQ(metrics.total_failures, 2,
                "Parsed assertion failures into SessionMetrics");
  ASSERT_INT_EQ(ledger.count, 3,
                "Failure and crash packets populated in ledger");

  vector_free(&ledger);
  vector_free(suites);
  free(suites);
  ctest_teardown_mock_binary(mock_pass_3_bin);
  ctest_teardown_mock_binary(mock_pass_1_fail_2_bin);
  ctest_teardown_mock_binary(mock_crashing_bin);
}

CTEST(SUITE_NAME, test_pool_empty_suites_vector) {
  Vector suites;
  vector_init(&suites, DISCOVERY_MAX_PATH_LEN);

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_FAIL_LINE_LEN);

  SessionMetrics metrics;
  memset(&metrics, 0, sizeof(SessionMetrics));

  CTestConfig config;
  ctest_config_init(&config);
  config.jobs = 4;

  int saved_stdout = ctest_mute_output(STDOUT_FILENO);
  int status = ctest_pool_run(&suites, &config, &metrics, &ledger);
  ctest_unmute_output(saved_stdout, STDOUT_FILENO);

  ASSERT_INT_EQ(status, 0, "Handled empty suite queue cleanly");
  ASSERT_INT_EQ(metrics.total_runs, 0,
                "Zero runs recorded for empty suite queue");
  ASSERT_INT_EQ(metrics.total_failures, 0,
                "Zero suites failed for empty suite queue");
  ASSERT_INT_EQ(ledger.count, 0, "Ledger remains empty for empty suite queue");

  vector_free(&ledger);
  vector_free(&suites);
}

CTEST(SUITE_NAME, test_pool_single_worker) {
  ctest_setup_mock_binary(mock_pass_3_bin, mock_pass_3_c_code);
  ctest_setup_mock_binary(mock_pass_1_bin, mock_pass_1_c_code);

  Vector *suites = ctest_discover_tests((const char *)MOCK_POOL_DIR);

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_FAIL_LINE_LEN);

  SessionMetrics metrics = {0};

  CTestConfig config;
  ctest_config_init(&config);
  config.jobs = 1;

  int saved_stdout = ctest_mute_output(STDOUT_FILENO);
  int status = ctest_pool_run(suites, &config, &metrics, &ledger);
  ctest_unmute_output(saved_stdout, STDOUT_FILENO);

  ASSERT_INT_EQ(status, 0, "Pool executes successfully when jobs == 1");
  ASSERT_INT_EQ(metrics.total_suites, 2,
                "Both suites ran with single worker slot");
  ASSERT_INT_EQ(metrics.total_runs, 4,
                "Test runs aggregated correctly for single worker");
  ASSERT_INT_EQ(metrics.total_failures, 0,
                "Total failures aggregated correctly for single worker");

  vector_free(&ledger);
  vector_free(suites);
  free(suites);
  ctest_teardown_mock_binary(mock_pass_3_bin);
  ctest_teardown_mock_binary(mock_pass_1_bin);
}
