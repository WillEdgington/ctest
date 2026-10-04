#include "ctest_cli/config.h"
#include "ctest_cli/runner.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SUITE_NAME test_runner

static const size_t CAPTURE_BUF_SIZE = 2048;

// 1 pass, return 0
static const char *mock_pass_2_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "2" CTEST_TEST_DELIM "0\\n\");\n"
    "  return 0;\n"
    "}\n";

// 1 pass, 2 fails, return 1
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
static const char *mock_crashing_c_code = "#include <stdlib.h>\n"
                                          "int main(void) {\n"
                                          "  int *p = NULL;\n"
                                          "  *p = 2;\n"
                                          "  return 0;\n"
                                          "}\n";

CTEST(SUITE_NAME, test_runner_all_passed) {
  const char *test_dir = "./sandbox_runner_pass_dir_47383244";
  const char *bin_path =
      "./sandbox_runner_pass_dir_47383244/test_pass_89454444";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path, mock_pass_2_c_code);

  CTestConfig config;
  ctest_config_init(&config);
  snprintf(config.target_dir, sizeof(config.target_dir), "%s", test_dir);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, 0, "Runner should return 0 when all test suites pass");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, bin_path),
                      "Output should report execution of passing suite");

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_with_failures) {
  const char *test_dir = "./sandbox_runner_fail_dir_86946555";
  const char *bin_path =
      "./sandbox_runner_fail_dir_86946555/test_fail_95035553";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path, mock_pass_1_fail_2_c_code);

  CTestConfig config;
  ctest_config_init(&config);
  snprintf(config.target_dir, sizeof(config.target_dir), "%s", test_dir);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, 1, "Runner should return 1 when test failures occur");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "FAIL"),
                      "Output should contain failure indicator");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "<expression>"),
                      "Output should contain failed expression");

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_with_crash) {
  const char *test_dir = "./sandbox_runner_crash_dir_09586433";
  const char *bin_path =
      "./sandbox_runner_crash_dir_09586433/test_crash_11195300";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path, mock_crashing_c_code);

  CTestConfig config;
  ctest_config_init(&config);
  snprintf(config.target_dir, sizeof(config.target_dir), "%s", test_dir);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, 1, "Runner should return 1 when a test suite crashes");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "CRASH"),
                      "Output should indicate suite crash");

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_invalid_directory) {
  CTestConfig config;
  ctest_config_init(&config);
  snprintf(config.target_dir, sizeof(config.target_dir),
           "non_existent_directory_92817344");

  int saved_stderr = ctest_mute_output(STDERR_FILENO);
  int res = ctest_run_session(&config);
  ctest_unmute_output(saved_stderr, STDERR_FILENO);

  ASSERT_INT_EQ(res, -1,
                "Runner should return -1 on discovery/directory error");
}

CTEST(SUITE_NAME, test_runner_with_filter) {
  const char *test_dir = "./sandbox_runner_filter_dir_55443322";
  const char *bin_pass =
      "./sandbox_runner_filter_dir_55443322/test_apple_11223344";
  const char *bin_fail =
      "./sandbox_runner_filter_dir_55443322/test_banana_55667788";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_pass, mock_pass_2_c_code);
  ctest_setup_mock_binary(bin_fail, mock_pass_1_fail_2_c_code);

  CTestConfig config;
  ctest_config_init(&config);
  snprintf(config.target_dir, sizeof(config.target_dir), "%s", test_dir);
  snprintf(config.filter_pattern, sizeof(config.filter_pattern), "apple");

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(
      res, 0,
      "Runner should return 0 when failing suite is excluded by filter");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, bin_pass),
                      "Output should contain matched binary 'test_apple'");
  ASSERT_PTR_NULL(strstr(out_buf, bin_fail),
                  "Output should not contain filtered binary 'test_banana'");

  ctest_teardown_mock_binary(bin_pass);
  ctest_teardown_mock_binary(bin_fail);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_verbose_session) {
  const char *test_dir = "./sandbox_runner_verbose_dir_11223344";
  const char *bin_path =
      "./sandbox_runner_verbose_dir_11223344/test_verb_55667788";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path, mock_pass_2_c_code);

  CTestConfig config;
  ctest_config_init(&config);
  snprintf(config.target_dir, sizeof(config.target_dir), "%s", test_dir);
  config.verbosity = CTEST_VERBOSITY_VERBOSE;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, 0, "Runner returns 0 in verbose mode");
  ASSERT_PTR_NOT_NULL(strstr(out_buf, "[RUN]"),
                      "Output contains [RUN] suite banner");

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_json_export) {
  const char *test_dir = "./sandbox_runner_json_dir_99182311";
  const char *bin_path =
      "./sandbox_runner_json_dir_99182311/test_json_11223344";
  const char *json_path = "./sandbox_runner_json_dir_99182311/report.json";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path, mock_pass_1_fail_2_c_code);

  CTestConfig config;
  ctest_config_init(&config);
  snprintf(config.target_dir, sizeof(config.target_dir), "%s", test_dir);
  snprintf(config.json_output_path, sizeof(config.json_output_path), "%s",
           json_path);

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int status = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(status, 1, "Runner should return 1 when test failures occur");

  FILE *f = fopen(json_path, "r");
  ASSERT_PTR_NOT_NULL(f, "JSON output file should be created");

  unlink(json_path);
  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_concurrent_jobs) {
  const char *test_dir = "./sandbox_runner_jobs_dir_33445566";
  const char *bin_a = "./sandbox_runner_jobs_dir_33445566/test_suite_a";
  const char *bin_b = "./sandbox_runner_jobs_dir_33445566/test_suite_b";
  const char *bin_c = "./sandbox_runner_jobs_dir_33445566/test_suite_c";
  const char *bin_d = "./sandbox_runner_jobs_dir_33445566/test_suite_d";

  ctest_setup_mock_dir(test_dir);

  ctest_setup_mock_binary(bin_a, mock_pass_2_c_code);
  ctest_setup_mock_binary(bin_b, mock_pass_2_c_code);
  ctest_setup_mock_binary(bin_c, mock_pass_2_c_code);
  ctest_setup_mock_binary(bin_d, mock_pass_2_c_code);

  CTestConfig config;
  ctest_config_init(&config);
  snprintf(config.target_dir, sizeof(config.target_dir), "%s", test_dir);
  config.jobs = 4;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int status = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(status, 0,
                "Runner should return 0 when all concurrent suites pass");
  ASSERT(strstr(out_buf, "test_suite_a") != NULL &&
             strstr(out_buf, "test_suite_b") != NULL &&
             strstr(out_buf, "test_suite_c") != NULL &&
             strstr(out_buf, "test_suite_d") != NULL,
         "Output should contain all test suite bin names");

  ctest_teardown_mock_binary(bin_a);
  ctest_teardown_mock_binary(bin_b);
  ctest_teardown_mock_binary(bin_c);
  ctest_teardown_mock_binary(bin_d);
  ctest_teardown_mock_dir(test_dir);
}
