#include "ctest_cli/config.h"
#include "ctest_cli/runner.h"
#include <ctest/ctest.h>
#include <stdio.h>

#define SUITE_NAME test_runner

static int CAPTURE_BUF_SIZE = 2048;

CTEST(SUITE_NAME, test_runner_all_passed) {
  const char *test_dir = "sandbox_runner_pass_dir_47383244";
  const char *bin_path = "sandbox_runner_pass_dir_47383244/test_pass_89454444";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path, "#include <stdio.h>\n"
                                    "int main(void) {\n"
                                    "    printf(\"SUMMARY|2|0|0\\n\");\n"
                                    "    return 0;\n"
                                    "}\n");

  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = test_dir;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, 0, "Runner should return 0 when all test suites pass");

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_with_failures) {
  const char *test_dir = "sandbox_runner_fail_dir_86946555";
  const char *bin_path = "sandbox_runner_fail_dir_86946555/test_fail_95035553";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path,
                          "#include <stdio.h>\n"
                          "int main(void) {\n"
                          "    printf(\"FAIL" CTEST_TEST_DELIM
                          "test.c" CTEST_TEST_DELIM "12" CTEST_TEST_DELIM
                          "x == y" CTEST_TEST_DELIM "Expected equality\\n\");\n"
                          "    printf(\"SUMMARY" CTEST_TEST_DELIM
                          "2" CTEST_TEST_DELIM "1" CTEST_TEST_DELIM "0\\n\");\n"
                          "    return 0;\n"
                          "}\n");

  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = test_dir;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, 1, "Runner should return 1 when test failures occur");

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_with_crash) {
  const char *test_dir = "sandbox_runner_crash_dir_09586433";
  const char *bin_path =
      "sandbox_runner_crash_dir_09586433/test_crash_11195300";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path, "#include <stdlib.h>\n"
                                    "int main(void) {\n"
                                    "    int *ptr = NULL;\n"
                                    "    *ptr = 42;\n"
                                    "    return 0;\n"
                                    "}\n");

  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = test_dir;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, 1, "Runner should return 1 when a test suite crashes");

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

// same as the test_executor test for the timeout feature, it would be good to
// find a different way to test this current approach sleeps for a second, would
// be great if it didn't
CTEST(SUITE_NAME, test_runner_with_timeout) {
  const char *test_dir = "sandbox_runner_timeout_dir_77112233";
  const char *bin_path =
      "sandbox_runner_timeout_dir_77112233/test_timeout_88223344";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path, "#include <unistd.h>\n"
                                    "int main(void) {\n"
                                    "    usleep(1100000);\n"
                                    "    return 0;\n"
                                    "}\n");

  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = test_dir;
  config.timeout_sec = 1;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, 1, "Runner should return 1 when a test suite times out");

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_invalid_directory) {
  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = "non_existent_directory_92817344";

  int saved_stderr = ctest_mute_output(STDERR_FILENO);
  int res = ctest_run_session(&config);
  ctest_unmute_output(saved_stderr, STDERR_FILENO);

  ASSERT_INT_EQ(res, -1,
                "Runner should return -1 on discovery/directory error");
}

CTEST(SUITE_NAME, test_runner_with_filter) {
  const char *test_dir = "sandbox_runner_filter_dir_55443322";
  const char *bin_pass =
      "sandbox_runner_filter_dir_55443322/test_apple_11223344";
  const char *bin_fail =
      "sandbox_runner_filter_dir_55443322/test_banana_55667788";

  ctest_setup_mock_dir(test_dir);

  ctest_setup_mock_binary(bin_pass,
                          "#include <stdio.h>\n"
                          "int main(void) {\n"
                          "    printf(\"SUMMARY" CTEST_TEST_DELIM
                          "1" CTEST_TEST_DELIM "0" CTEST_TEST_DELIM "0\\n\");\n"
                          "    return 0;\n"
                          "}\n");

  ctest_setup_mock_binary(
      bin_fail, "#include <stdio.h>\n"
                "int main(void) {\n"
                "    printf(\"FAIL" CTEST_TEST_DELIM "test.c" CTEST_TEST_DELIM
                "5" CTEST_TEST_DELIM "0" CTEST_TEST_DELIM "Failed\\n\");\n"
                "    printf(\"SUMMARY" CTEST_TEST_DELIM "1" CTEST_TEST_DELIM
                "1" CTEST_TEST_DELIM "0\\n\");\n"
                "    return 0;\n"
                "}\n");

  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = test_dir;
  config.filter_pattern = "apple";

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(
      res, 0,
      "Runner should return 0 when failing suite is excluded by filter");

  ASSERT_INT_EQ(strstr(out_buf, "test_apple") != NULL, 1,
                "Output should contain matched binary 'test_apple'");
  ASSERT_INT_EQ(strstr(out_buf, "test_banana") == NULL, 1,
                "Output should not contain filtered binary 'test_banana'");

  ctest_teardown_mock_binary(bin_pass);
  ctest_teardown_mock_binary(bin_fail);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_verbose_session) {
  const char *test_dir = "sandbox_runner_verbose_dir_11223344";
  const char *bin_path =
      "sandbox_runner_verbose_dir_11223344/test_verb_55667788";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path, "#include <stdio.h>\n"
                                    "int main(void) {\n"
                                    "    printf(\"SUMMARY" CTEST_TEST_DELIM
                                    "1" CTEST_TEST_DELIM "0\\n\");\n"
                                    "    return 0;\n"
                                    "}\n");

  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = test_dir;
  config.verbosity = CTEST_VERBOSITY_VERBOSE;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, 0, "Runner returns 0 in verbose mode");
  ASSERT(strstr(out_buf, "[RUN]") != NULL,
         "Output contains [RUN] suite banner");

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_json_export) {
  const char *test_dir = "sandbox_runner_json_dir_99182311";
  const char *bin_path = "sandbox_runner_json_dir_99182311/test_json_11223344";
  const char *json_path = "sandbox_runner_json_dir_99182311/report.json";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(bin_path,
                          "#include <stdio.h>\n"
                          "int main(void) {\n"
                          "    printf(\"FAIL" CTEST_TEST_DELIM
                          "test.c" CTEST_TEST_DELIM "10" CTEST_TEST_DELIM
                          "x == y" CTEST_TEST_DELIM "Expected equality\\n\");\n"
                          "    printf(\"SUMMARY" CTEST_TEST_DELIM
                          "1" CTEST_TEST_DELIM "1" CTEST_TEST_DELIM "0\\n\");\n"
                          "    return 0;\n"
                          "}\n");

  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = test_dir;
  config.json_output_path = json_path;

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int status = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(status, 1, "Runner should return 1 when test failures occur");

  FILE *f = fopen(json_path, "r");
  ASSERT_PTR_NOT_NULL(f, "JSON output file should be created");

  if (f != NULL) {
    char json_buf[CAPTURE_BUF_SIZE];
    size_t bytes_read = fread(json_buf, 1, sizeof(json_buf) - 1, f);
    json_buf[bytes_read] = '\0';
    fclose(f);
    unlink(json_path);

    ASSERT_PTR_NOT_NULL(strstr(json_buf, "\"summary\""),
                        "JSON output should contain 'summary'");
    ASSERT_PTR_NOT_NULL(strstr(json_buf, "\"failures\""),
                        "JSON output should contain 'failures'");
    ASSERT_PTR_NOT_NULL(strstr(json_buf, "Expected equality"),
                        "JSON output should contain failure details");
  }

  ctest_teardown_mock_binary(bin_path);
  ctest_teardown_mock_dir(test_dir);
}

CTEST(SUITE_NAME, test_runner_concurrent_jobs) {
  const char *test_dir = "sandbox_runner_jobs_dir_33445566";
  const char *bin_a = "sandbox_runner_jobs_dir_33445566/test_suite_a";
  const char *bin_b = "sandbox_runner_jobs_dir_33445566/test_suite_b";
  const char *bin_c = "sandbox_runner_jobs_dir_33445566/test_suite_c";
  const char *bin_d = "sandbox_runner_jobs_dir_33445566/test_suite_d";

  ctest_setup_mock_dir(test_dir);

  const char *mock_code = "#include <stdio.h>\n"
                          "int main(void) {\n"
                          "    printf(\"SUMMARY" CTEST_TEST_DELIM
                          "1" CTEST_TEST_DELIM "0" CTEST_TEST_DELIM "0\\n\");\n"
                          "    return 0;\n"
                          "}\n";

  ctest_setup_mock_binary(bin_a, mock_code);
  ctest_setup_mock_binary(bin_b, mock_code);
  ctest_setup_mock_binary(bin_c, mock_code);
  ctest_setup_mock_binary(bin_d, mock_code);

  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = test_dir;
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
