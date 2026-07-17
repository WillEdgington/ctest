#include "clib/vector.h"
#include "ctest_cli/discovery.h"
#include <ctest/ctest.h>
#include <stdlib.h>

char mock_root[] = "mock_root_78323422";
char mock_c[] = "mock_root_78323422/test_37589331.c";
char mock_d[] = "mock_root_78323422/test_37589331.c";
char mock_o[] = "mock_root_78323422/test_37589331.c";
char mock_test_f[] = "mock_root_78323422/test_37589331";
char mock_misc_f[] = "mock_root_78323422/misc_48395722";

char nested_dir[] = "mock_root_78323422/nested_dir_84392311";
char nested_test_f[] = "mock_root_78323422/nested_dir_84392311/test_43893322";

void setup_mock_test_suite(void) {
  ctest_setup_mock_dir(mock_root);
  ctest_setup_mock_file(mock_c, NULL);
  ctest_setup_mock_file(mock_d, NULL);
  ctest_setup_mock_file(mock_o, NULL);
  ctest_setup_mock_file(mock_test_f, NULL);
  ctest_setup_mock_file(mock_misc_f, NULL);

  ctest_setup_mock_dir(nested_dir);
  ctest_setup_mock_file(nested_test_f, NULL);

  // Grant user access (allows S_IXUSR check to pass)
  chmod(mock_test_f, 0755);
  chmod(nested_test_f, 0755);
}

void teardown_mock_test_suite(void) {
  ctest_teardown_mock_dir(mock_root);
  ctest_teardown_mock_file(mock_c);
  ctest_teardown_mock_file(mock_d);
  ctest_teardown_mock_file(mock_o);
  ctest_teardown_mock_file(mock_test_f);
  ctest_teardown_mock_file(mock_misc_f);

  ctest_teardown_mock_dir(nested_dir);
  ctest_teardown_mock_file(nested_test_f);
}

void test_discovery_nested_traversal(void) {
  setup_mock_test_suite();

  Vector *paths = ctest_discover_tests(mock_root);
  ASSERT_PTR_NOT_NULL(
      paths, "Test discovery should return a valid pointer to a Vector struct");
  ASSERT_INT_EQ(paths->count, 2,
                "Test discovery should correctly identify and append the right "
                "number of test files");
  ASSERT_STR_EQ((char *)vector_get(paths, 0), mock_test_f,
                "First appended test binary should be the correct path");
  ASSERT_STR_EQ((char *)vector_get(paths, 1), nested_test_f,
                "Second appended test binary should be the correct path (this "
                "is a nested file)");

  teardown_mock_test_suite();
  vector_free(paths);
  free(paths);
}

int main(void) {
  printf("\nRunning: %s...", __FILE__);

  test_discovery_nested_traversal();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
