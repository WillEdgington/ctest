#define _GNU_SOURCE
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>

#define SUITE_NAME test_libctest

static size_t BUF_SIZE = 128;

CTEST(SUITE_NAME, test_basic_assertions) {
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

CTEST(SUITE_NAME, test_output_muting) {
  int saved_stdout = ctest_mute_output(STDOUT_FILENO);
  ASSERT(saved_stdout >= 0,
         "Muting stdout should return a valid backup file descriptor");

  printf("THIS SHOULD BE HIDDEN FROM THE TERMINAL STANDARD OUTPUT STREAM\n");

  ctest_unmute_output(saved_stdout, STDOUT_FILENO);
  printf("  [INFO] Terminal output unmuted successfully.\n");
}

CTEST(SUITE_NAME, test_stdout_capturing) {
  char buf[BUF_SIZE];

  int start_res = ctest_capture_stdout_start();
  printf("CAPTURE_TEST_PAYLOAD");
  ssize_t bytes = ctest_capture_stdout_end(buf, sizeof(buf));

  ASSERT_INT_EQ(start_res, 0, "Stdout capture setup succeeds");
  ASSERT(bytes > 0, "Captured bytes should be greater than zero");
  ASSERT_STR_EQ(buf, "CAPTURE_TEST_PAYLOAD",
                "Captured string matches stdout output");
}
