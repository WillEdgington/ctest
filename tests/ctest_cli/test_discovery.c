#include "ctest_cli/discovery.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdlib.h>

#define SUITE_NAME test_discovery

#define MOCK_DIR_PATH "mock_dir_78323422"

static char test_a[] = MOCK_DIR_PATH "/test_a_05345035";
static char test_b[] = MOCK_DIR_PATH "/test_b_09090358";
static char fake_test[] = MOCK_DIR_PATH "/test_fake_95359333";
static char mock_txt[] = MOCK_DIR_PATH "/text_file_00923583.txt";
static char mock_bin[] = MOCK_DIR_PATH "/cbin_00013258";

static char nested_dir[] = MOCK_DIR_PATH "/nested_dir_84392311";
static char nested_test[] =
    MOCK_DIR_PATH "/nested_dir_84392311/test_nested_43893322";
static char nested_md[] =
    MOCK_DIR_PATH "/nested_dir_84392311/test_md_00935539.md";

// test standard discovery
// nested
// files with no suffix, but not flags
CTEST_SETUP_SUITE(SUITE_NAME) {
  ctest_setup_mock_dir(MOCK_DIR_PATH);
  ctest_setup_mock_binary(test_a, "int main(void) { return 0; }");
  ctest_setup_mock_binary(test_b, "int main(void) { return 0; }");
  ctest_setup_mock_file(fake_test, "this is fake");
  ctest_setup_mock_file(mock_txt, "text file");
  ctest_setup_mock_binary(mock_bin, "int main(void) { return 0; }");

  ctest_setup_mock_dir(nested_dir);
  ctest_setup_mock_binary(nested_test, "int main(void) { return 0; }");
  ctest_setup_mock_file(nested_md, ".md file");
}

CTEST(SUITE_NAME, test_discover_tests_no_filter) {
  Vector *bin_paths = ctest_discover_tests(MOCK_DIR_PATH, NULL);
  ASSERT_PTR_NOT_NULL(bin_paths, "Discovered tests Vector should not be NULL");
  ASSERT_INT_EQ(bin_paths->count, 3,
                "Discovered tests Vector item count should be equal to the "
                "number of suites in the test root");
  ASSERT_STR_EQ((char *)vector_get(bin_paths, 1), test_a,
                "test_a should be discovered");
  ASSERT_STR_EQ((char *)vector_get(bin_paths, 2), test_b,
                "test_b should be discovered");
  ASSERT_STR_EQ((char *)vector_get(bin_paths, 0), nested_test,
                "Test in nested directory should be dicovered");

  vector_free(bin_paths);
  free(bin_paths);
}

CTEST(SUITE_NAME, test_discover_tests_with_filter) {
  const char *filter_pattern = "*nested_*";
  Vector *bin_paths = ctest_discover_tests(MOCK_DIR_PATH, filter_pattern);
  ASSERT_PTR_NOT_NULL(
      bin_paths, "Discovered tests (with filter) Vector should not be NULL");
  ASSERT_INT_EQ(bin_paths->count, 1,
                "Discovered tests Vector item count should equal the number of "
                "suites that match the given filter in test root");
  ASSERT_STR_EQ((char *)vector_get(bin_paths, 0), nested_test,
                "Discovered test that matches filter pattern should be the "
                "expected suite");

  vector_free(bin_paths);
  free(bin_paths);
}