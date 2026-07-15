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

void test_output_muting(void) {
  int saved_stdout = ctest_mute_output(STDOUT_FILENO);
  ASSERT(saved_stdout >= 0,
         "Muting stdout should return a valid backup file descriptor");

  printf("THIS SHOULD BE HIDDEN FROM THE TERMINAL STANDARD OUTPUT STREAM\n");

  ctest_unmute_output(saved_stdout, STDOUT_FILENO);
  printf("  [INFO] Terminal output unmuted successfully.");
}

int main(void) {
  printf("\nRunning: %s...", __FILE__);

  test_basic_assertions();
  test_environment_fixtures();
  test_output_muting();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
