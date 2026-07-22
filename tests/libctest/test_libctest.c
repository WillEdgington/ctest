#define _GNU_SOURCE
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>

void test_basic_assertions(void) {
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

void test_environment_fixtures(void) {
  const char *test_dir = "sandbox_dir_92358835";
  const char *test_file = "sandbox_dir_92358835/sandbox_f_89984111.txt";
  const char *file_content = "ctest compilation verification content";

  ctest_setup_mock_dir(test_dir);
  ctest_setup_mock_file(test_file, file_content);

  FILE *f = fopen(test_file, "r");
  ASSERT_PTR_NOT_NULL(f, "Mock file should exist and open successfully");
  if (f != NULL) {
    char buffer[128] = {0};
    fgets(buffer, sizeof(buffer), f);
    fclose(f);
    ASSERT_STR_EQ(buffer, file_content,
                  "Mock file content should match expected setup string");
  }

  ctest_teardown_mock_file(test_file);
  ctest_teardown_mock_dir(test_dir);

  FILE *f_cleanup = fopen(test_file, "r");
  ASSERT_PTR_NULL(f_cleanup, "Mock file should no longer exist after teardown");
  if (f_cleanup != NULL)
    fclose(f_cleanup);
}

void test_mock_binary_fixtures(void) {
  const char *mock_bin = "./sandbox_mock_bin_90123847";

  int setup_res = ctest_setup_mock_binary(mock_bin, "#include <stdio.h>\n"
                                                    "int main(void) {\n"
                                                    "    return 42;\n"
                                                    "}\n");

  ASSERT_INT_EQ(setup_res, 0,
                "Mock binary setup returns 0 on successful compile");

  struct stat st;
  int stat_res = stat(mock_bin, &st);
  ASSERT_INT_EQ(stat_res, 0, "Mock binary executable exists on disk");
  ASSERT(S_ISREG(st.st_mode), "Mock binary is a regular file");
  ASSERT((st.st_mode & S_IXUSR) != 0,
         "Mock binary has user execute permissions");

  int exit_status = system(mock_bin);
  ASSERT_INT_EQ(WEXITSTATUS(exit_status), 42,
                "Mock binary executes and returns expected status");

  ctest_teardown_mock_binary(mock_bin);

  stat_res = stat(mock_bin, &st);
  ASSERT_INT_EQ(stat_res, -1, "Mock binary removed after teardown");

  char src_path[256];
  snprintf(src_path, sizeof(src_path), "%s.c", mock_bin);
  stat_res = stat(src_path, &st);
  ASSERT_INT_EQ(stat_res, -1, "Mock source (.c) file removed after teardown");

  int stderr = ctest_mute_output(STDERR_FILENO);
  int failed_setup =
      ctest_setup_mock_binary(mock_bin, "invalid c syntax code {");
  ASSERT_INT_EQ(failed_setup, -1,
                "Mock binary setup returns -1 on compilation failure");
  ctest_unmute_output(stderr, STDERR_FILENO);

  stat_res = stat(src_path, &st);
  ASSERT_INT_EQ(stat_res, -1, "Failed setup cleans up temp .c source file");
}

void test_output_muting(void) {
  int saved_stdout = ctest_mute_output(STDOUT_FILENO);
  ASSERT(saved_stdout >= 0,
         "Muting stdout should return a valid backup file descriptor");

  printf("THIS SHOULD BE HIDDEN FROM THE TERMINAL STANDARD OUTPUT STREAM\n");

  ctest_unmute_output(saved_stdout, STDOUT_FILENO);
  printf("  [INFO] Terminal output unmuted successfully.");
}

void test_stdout_capturing(void) {
  char buf[128];

  int start_res = ctest_capture_stdout_start();
  ASSERT_INT_EQ(start_res, 0, "Stdout capture setup succeeds");

  printf("CAPTURE_TEST_PAYLOAD");

  ssize_t bytes = ctest_capture_stdout_end(buf, sizeof(buf));
  ASSERT(bytes > 0, "Captured bytes should be greater than zero");
  ASSERT_STR_EQ(buf, "CAPTURE_TEST_PAYLOAD",
                "Captured string matches stdout output");
}

int main(void) {
  printf("\nRunning: %s...", __FILE__);

  test_basic_assertions();
  test_environment_fixtures();
  test_output_muting();
  test_mock_binary_fixtures();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
