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

CTEST_SETUP_SUITE(SUITE_NAME) {
  ctest_setup_mock_dir((const char *)MOCK_POOL_DIR);
}

CTEST_TEARDOWN_SUITE(SUITE_NAME) {
  ctest_teardown_mock_dir((const char *)MOCK_POOL_DIR);
}

CTEST(SUITE_NAME, test_pool_all_passing_parallel) {
  const char *bin1 = MOCK_POOL_DIR "test_pass_1";
  const char *bin2 = MOCK_POOL_DIR "test_pass_2";
  const char *bin3 = MOCK_POOL_DIR "test_pass_3";

  ctest_setup_mock_binary(
      bin1,
      "#include <stdio.h>\nint main(void) { printf(\"SUMMARY" CTEST_TEST_DELIM
      "3" CTEST_TEST_DELIM "0\\n\"); return 0; }\n");
  ctest_setup_mock_binary(
      bin2,
      "#include <stdio.h>\nint main(void) { printf(\"SUMMARY" CTEST_TEST_DELIM
      "2" CTEST_TEST_DELIM "0\\n\"); return 0; }\n");
  ctest_setup_mock_binary(
      bin3,
      "#include <stdio.h>\nint main(void) { printf(\"SUMMARY" CTEST_TEST_DELIM
      "4" CTEST_TEST_DELIM "0\\n\"); return 0; }\n");

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
  ASSERT_INT_EQ(metrics.total_runs, 9,
                "Aggregated total runs across parallel workers");
  ASSERT_INT_EQ(metrics.total_failures, 0, "Recorded zero assertion failures");
  ASSERT_INT_EQ(ledger.count, 0, "Ledger remains empty for clean pass");

  vector_free(&ledger);
  vector_free(suites);
  free(suites);
  ctest_teardown_mock_binary(bin1);
  ctest_teardown_mock_binary(bin2);
  ctest_teardown_mock_binary(bin3);
}

CTEST(SUITE_NAME, test_pool_mixed_outcomes_parallel) {
  const char *bin_pass = MOCK_POOL_DIR "test_mix_pass";
  const char *bin_fail = MOCK_POOL_DIR "test_mix_fail";
  const char *bin_crash = MOCK_POOL_DIR "test_mix_crash";

  ctest_setup_mock_binary(bin_pass, "#include <stdio.h>\n"
                                    "int main(void) {\n"
                                    "  printf(\"SUMMARY" CTEST_TEST_DELIM
                                    "2" CTEST_TEST_DELIM "0\\n\");\n"
                                    "  return 0;\n"
                                    "}\n");

  ctest_setup_mock_binary(
      bin_fail,
      "#include <stdio.h>\n"
      "int main(void) {\n"
      "  printf(\"FAIL" CTEST_TEST_DELIM "m.c" CTEST_TEST_DELIM
      "10" CTEST_TEST_DELIM "a == b" CTEST_TEST_DELIM "Mismatch\\n\");\n"
      "  printf(\"SUMMARY" CTEST_TEST_DELIM "2" CTEST_TEST_DELIM "1\\n\");\n"
      "  return 0;\n"
      "}\n");

  ctest_setup_mock_binary(bin_crash, "#include <stdlib.h>\n"
                                     "int main(void) {\n"
                                     "  int *p = NULL; *p = 1;\n"
                                     "  return 0;\n"
                                     "}\n");

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
  ASSERT_INT_EQ(metrics.total_failures, 1,
                "Parsed assertion failures into SessionMetrics");
  ASSERT_INT_EQ((int)ledger.count, 2,
                "Failure and crash packets populated in ledger");

  vector_free(&ledger);
  vector_free(suites);
  free(suites);
  ctest_teardown_mock_binary(bin_pass);
  ctest_teardown_mock_binary(bin_fail);
  ctest_teardown_mock_binary(bin_crash);
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

CTEST(SUITE_NAME, test_pool_single_worker_jobs_one) {
  const char *bin1 = MOCK_POOL_DIR "test_seq_1";
  const char *bin2 = MOCK_POOL_DIR "test_seq_2";

  ctest_setup_mock_binary(
      bin1,
      "#include <stdio.h>\nint main(void) { printf(\"SUMMARY" CTEST_TEST_DELIM
      "1" CTEST_TEST_DELIM "0\\n\"); return 0; }\n");
  ctest_setup_mock_binary(
      bin2,
      "#include <stdio.h>\nint main(void) { printf(\"SUMMARY" CTEST_TEST_DELIM
      "1" CTEST_TEST_DELIM "0\\n\"); return 0; }\n");

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
  ASSERT_INT_EQ(metrics.total_runs, 2,
                "Test runs aggregated correctly for single worker");

  vector_free(&ledger);
  vector_free(suites);
  free(suites);
  ctest_teardown_mock_binary(bin1);
  ctest_teardown_mock_binary(bin2);
}
