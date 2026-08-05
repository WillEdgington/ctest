#include "ctest_cli/config.h"
#include "ctest_cli/runner.h"
#include <ctest/ctest.h>
#include <stdio.h>

#define CAPTURE_BUF_SIZE 2048

static void test_runner_all_passed(void) {
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

static void test_runner_with_failures(void) {
  const char *test_dir = "sandbox_runner_fail_dir_86946555";
  const char *bin_path = "sandbox_runner_fail_dir_86946555/test_fail_95035553";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_binary(
      bin_path, "#include <stdio.h>\n"
                "int main(void) {\n"
                "    printf(\"FAIL|test.c|12|x == y|Expected equality\\n\");\n"
                "    printf(\"SUMMARY|2|1|0\\n\");\n"
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

static void test_runner_with_crash(void) {
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

static void test_runner_invalid_directory(void) {
  CTestConfig config;
  ctest_config_init(&config);
  config.target_dir = "non_existent_directory_92817344";

  char out_buf[CAPTURE_BUF_SIZE];
  ctest_capture_stdout_start();
  int res = ctest_run_session(&config);
  ctest_capture_stdout_end(out_buf, sizeof(out_buf));

  ASSERT_INT_EQ(res, -1,
                "Runner should return -1 on discovery/directory error");
}

static void test_runner_with_filter(void) {
  const char *test_dir = "sandbox_runner_filter_dir_55443322";
  const char *bin_pass =
      "sandbox_runner_filter_dir_55443322/test_apple_11223344";
  const char *bin_fail =
      "sandbox_runner_filter_dir_55443322/test_banana_55667788";

  ctest_setup_mock_dir(test_dir);

  ctest_setup_mock_binary(bin_pass, "#include <stdio.h>\n"
                                    "int main(void) {\n"
                                    "    printf(\"SUMMARY|1|0|0\\n\");\n"
                                    "    return 0;\n"
                                    "}\n");

  ctest_setup_mock_binary(bin_fail,
                          "#include <stdio.h>\n"
                          "int main(void) {\n"
                          "    printf(\"FAIL|test.c|5|0|Failed\\n\");\n"
                          "    printf(\"SUMMARY|1|1|0\\n\");\n"
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

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_runner_all_passed();
  test_runner_with_failures();
  test_runner_with_crash();
  test_runner_invalid_directory();
  test_runner_with_filter();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
