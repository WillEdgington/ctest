#include "libctest/mock.h"
#include <ctest/ctest.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define SUITE_NAME test_mock

// mock dir tests

CTEST(SUITE_NAME, test_mock_dir_successful_lifecycle) {
  const char *mock_dir_path = "mock_dir_85934211/";
  ASSERT_INT_EQ(ctest_setup_mock_dir(mock_dir_path), 0,
                "ctest_setup_mock_dir() call should return 0 (success) if able "
                "to setup mock directory");
  ASSERT_INT_EQ(access(mock_dir_path, F_OK), 0,
                "Mock directory should exist after successful setup");
  ASSERT_INT_EQ(ctest_teardown_mock_dir(mock_dir_path), 0,
                "ctest_teardown_mock_dir() call should return 0 (success) if "
                "path is a non-blocked mock directory");
  ASSERT_INT_EQ(access(mock_dir_path, F_OK), -1,
                "Mock directory should not exist after successful teardown");
}

CTEST(SUITE_NAME, test_mock_dir_setup_null_path) {
  ASSERT_INT_EQ(
      ctest_setup_mock_dir(NULL), -1,
      "ctest_setup_mock_dir() call with path == NULL should return -1 (fail)");
}

CTEST(SUITE_NAME, test_mock_dir_teardown_null_path) {
  ASSERT_INT_EQ(ctest_teardown_mock_dir(NULL), -1,
                "ctest_teardown_mock_dir() call with path == NULL should "
                "return -1 (fail)");
}

CTEST(SUITE_NAME, test_mock_dir_already_exists) {
  const char *mock_dir_path = "mock_dir_54898923/";
  ctest_setup_mock_dir(mock_dir_path);
  ASSERT_INT_EQ(ctest_setup_mock_dir(mock_dir_path), -1,
                "ctest_setup_mock_dir() call should return -1 (fail) if the "
                "directory already exists");
  ctest_teardown_mock_dir(mock_dir_path);
}

CTEST(SUITE_NAME, test_mock_dir_teardown_non_existent_mock) {
  const char *non_existent_dir = "non_existent_mock_dir_58494343";
  ASSERT_INT_EQ(ctest_teardown_mock_dir(non_existent_dir), -1,
                "ctest_teardown_mock_dir() call for non existent mock dir "
                "should return -1 (fail)");
}

CTEST(SUITE_NAME, test_mock_dir_teardown_fails_with_non_mock_dir) {
  const char *mock_dir_path = "mock_dir_85934211/";

  mkdir(mock_dir_path, MKDIR_MODE_FLAGS); // bypass mock directory setup
  ASSERT_INT_EQ(ctest_teardown_mock_dir(mock_dir_path), -1,
                "ctest_teardown_mock_dir() call should return -1 (fail) if "
                "directory is not on the mock stack");
  ASSERT_INT_EQ(access(mock_dir_path, F_OK), 0,
                "Non-mock directory should still exist after mock teardown "
                "attempt on its path");
  rmdir(mock_dir_path); // teardown dir
}

// mock file tests

CTEST(SUITE_NAME, test_mock_file_successful_lifecycle) {
  const char *mock_file_path = "mock_file_22353980.txt";
  const char *content = "hello";

  ASSERT_INT_EQ(ctest_setup_mock_file(mock_file_path, content), 0,
                "ctest_setup_mock_file() call should return 0 (success) if "
                "able to setup mock file");
  ASSERT_INT_EQ(access(mock_file_path, F_OK), 0,
                "Mock file should exist after successful setup");

  FILE *f = fopen(mock_file_path, "r");
  ASSERT_PTR_NOT_NULL(f, "Mock file should be readable");
  char buf[32] = {0};
  fgets(buf, sizeof(buf), f);
  fclose(f);
  ASSERT_STR_EQ(buf, content, "Mock file content should match written input");

  ASSERT_INT_EQ(ctest_teardown_mock_file(mock_file_path), 0,
                "ctest_teardown_mock_file() call should return 0 (success) if "
                "path is a mock file");
  ASSERT_INT_EQ(access(mock_file_path, F_OK), -1,
                "Mock file should not exist after teardown");
}

CTEST(SUITE_NAME, test_mock_file_setup_null_path) {
  ASSERT_INT_EQ(
      ctest_setup_mock_file(NULL, "hello"), -1,
      "ctest_setup_mock_file() call with path == NULL should return -1 (fail)");
}

CTEST(SUITE_NAME, test_mock_file_setup_null_content) {
  const char *mock_file_path = "mock_file_65893554";
  ASSERT_INT_EQ(ctest_setup_mock_file(mock_file_path, NULL), 0,
                "ctest_setup_mock_file() call with content == NULL should "
                "return 0 (success)");
  ASSERT_INT_EQ(
      access(mock_file_path, F_OK), 0,
      "Mock file should exist after mock file setup that had no input content");
  ctest_teardown_mock_file(mock_file_path);
}

CTEST(SUITE_NAME, test_mock_file_teardown_null_path) {
  ASSERT_INT_EQ(ctest_teardown_mock_file(NULL), -1,
                "ctest_teardown_mock_file() call with path == NULL should "
                "return -1 (fail)");
}

CTEST(SUITE_NAME, test_mock_file_already_exists) {
  const char *mock_file_path = "mock_file_12312411";
  ctest_setup_mock_file(mock_file_path, "hello");
  ASSERT_INT_EQ(ctest_setup_mock_file(mock_file_path, "this fails"), -1,
                "ctest_setup_mock_file() call should return -1 (fail) if the "
                "mock file already exists");
  ctest_teardown_mock_file(mock_file_path);
}

CTEST(SUITE_NAME, test_mock_file_invalid_dir_path) {
  const char *file_path = "non_existent_dir_75367733/mock_file_11267322";
  ASSERT_INT_EQ(ctest_setup_mock_file(file_path, "hello"), -1,
                "ctest_setup_mock_file() call should return -1 (fail) if file "
                "path has invalid directory path");
}

CTEST(SUITE_NAME, test_mock_file_teardown_non_existent_mock) {
  const char *non_existent_file = "non_existent_mock_file_99835233";
  ASSERT_INT_EQ(ctest_teardown_mock_file(non_existent_file), -1,
                "ctest_teardown_mock_file() call for non existent mock file "
                "should return -1 (fail)");
}

CTEST(SUITE_NAME, test_mock_file_teardown_fails_with_non_mock_file) {
  const char *mock_file_path = "mock_file_77684763";

  FILE *f = fopen(mock_file_path, "w"); // bypass mock file setup
  fclose(f);

  ASSERT_INT_EQ(ctest_teardown_mock_file(mock_file_path), -1,
                "ctest_teardown_mock_file() call should return -1 (fail) if "
                "directory is not on the mock stack");
  ASSERT_INT_EQ(access(mock_file_path, F_OK), 0,
                "Non-mock file should still exist after mock teardown attempt "
                "on its path");
  unlink(mock_file_path); // teardown file
}

CTEST(SUITE_NAME, test_teardown_mock_file_external_removal_returns_success) {
  const char *mock_file = "mock_file_target_del_83720194";
  ctest_setup_mock_file(mock_file, "data");
  unlink(mock_file);

  ASSERT_INT_EQ(ctest_teardown_mock_file(mock_file), 0,
                "Targeted teardown on missing resource should return 0 and "
                "prune vector entry");
}

// mock parent dir teardown test

CTEST(SUITE_NAME, test_mock_parent_dir_teardown) {
  const char *mock_dir_path = "mock_dir_09342233/";
  const char *mock_file_child = "mock_dir_09342233/mock_file_90002432";

  ctest_setup_mock_dir(mock_dir_path);
  ctest_setup_mock_file(mock_file_child, NULL);

  ASSERT_INT_EQ(ctest_teardown_mock_dir(mock_dir_path), -1,
                "ctest_teardown_mock_dir() call for path with existing mock "
                "children should return -1 (fail)");
  ctest_teardown_mock_file(mock_file_child);
  ASSERT_INT_EQ(ctest_teardown_mock_dir(mock_dir_path), 0,
                "ctest_teardown_mock_dir() call returns 0 (success) after "
                "teardown of mock children");
}

// mock binary tests

CTEST(SUITE_NAME, test_mock_binary_successful_lifecycle) {
  const char *mock_bin_path = "mock_bin_95843033";
  const char *mock_c_path = "mock_bin_95843033.c";
  const char *c_code = "int main(void) { return 42; }";

  ASSERT_INT_EQ(ctest_setup_mock_binary(mock_bin_path, c_code), 0,
                "ctest_setup_mock_binary() call should return 0 (success) if "
                "able to setup mock binary");
  ASSERT_INT_EQ(access(mock_c_path, F_OK), 0,
                "Mock source .c file should exist after setup");
  ASSERT_INT_EQ(access(mock_bin_path, F_OK), 0,
                "Mock binary should exist after successful setup");
  ASSERT_INT_EQ(access(mock_bin_path, X_OK), 0,
                "Mock binary should have executable permissions");

  char run_cmd[128];
  snprintf(run_cmd, sizeof(run_cmd), "./%s", mock_bin_path);
  int status = system(run_cmd);
  ASSERT_INT_EQ(WEXITSTATUS(status), 42,
                "Executing mock binary should return exit code 42");

  ASSERT_INT_EQ(ctest_teardown_mock_binary(mock_bin_path), 0,
                "ctest_teardown_mock_binary() call should return 0 (success) "
                "if path is a mock binary");
  ASSERT_INT_EQ(access(mock_c_path, F_OK), -1,
                "Mock source .c file should not exist after teardown");
  ASSERT_INT_EQ(access(mock_bin_path, F_OK), -1,
                "Mock binary should not exist after teardown");
}

CTEST(SUITE_NAME, test_mock_binary_setup_null_path) {
  ASSERT_INT_EQ(ctest_setup_mock_binary(NULL, "int main() { return 0; }"), -1,
                "ctest_setup_mock_binary() call with path == NULL should "
                "return -1 (fail)");
}

CTEST(SUITE_NAME, test_mock_binary_setup_invalid_c_code) {
  const char *mock_bin = "mock_bin_57383322";
  ASSERT_INT_EQ(ctest_setup_mock_binary(mock_bin, NULL), -1,
                "ctest_setup_mock_binary() call with c_code == NULL should "
                "return -1 (fail)");
  ASSERT_INT_EQ(
      access(mock_bin, F_OK), -1,
      "mock binary should not exist with setup attempt with c_code == NULL");

  ASSERT_INT_EQ(ctest_setup_mock_binary(mock_bin, "this is invalid c code"), -1,
                "ctest_setup_mock_binary() call with invalid c_code should "
                "return -1 (fail)");
  ASSERT_INT_EQ(
      access(mock_bin, F_OK), -1,
      "mock binary should not exist with setup attempt with invalid c_code");
}

CTEST(SUITE_NAME, test_mock_binary_setup_path_with_suffix) {
  const char *json_path = "json_path_58393322.json";
  const char *c_code = "int main(void) { return 0; }";

  ASSERT_INT_EQ(ctest_setup_mock_binary(json_path, c_code), -1,
                "ctest_setup_mock_binary() call with path with file identifier "
                "suffix should return -1 (fail)");
  ASSERT_INT_EQ(access(json_path, F_OK), -1,
                "mock binary with file identifier suffix should not exist "
                "after failed setup attempt");
}

CTEST(SUITE_NAME, test_mock_binary_teardown_null_path) {
  ASSERT_INT_EQ(ctest_teardown_mock_binary(NULL), -1,
                "ctest_teardown_mock_binary() call with path == NULL should "
                "return -1 (fail)");
}

CTEST(SUITE_NAME, test_mock_binary_already_exists) {
  const char *mock_bin_path = "mock_bin_88392100";
  const char *c_code = "int main() { return 0; }";

  ctest_setup_mock_binary(mock_bin_path, c_code);
  ASSERT_INT_EQ(ctest_setup_mock_binary(mock_bin_path, c_code), -1,
                "ctest_setup_mock_binary() call should return -1 (fail) if the "
                "mock binary already exists");
  ctest_teardown_mock_binary(mock_bin_path);
}

CTEST(SUITE_NAME, test_mock_binary_invalid_dir_path) {
  const char *mock_bin_path = "non_existent_dir_99381223/mock_bin_44920112";
  ASSERT_INT_EQ(
      ctest_setup_mock_binary(mock_bin_path, "int main() { return 0; }"), -1,
      "ctest_setup_mock_binary() call should return -1 (fail) if binary path "
      "has invalid directory path");
}

CTEST(SUITE_NAME, test_mock_binary_teardown_non_existent_mock) {
  const char *non_existent_bin = "non_existent_mock_bin_88192301";
  ASSERT_INT_EQ(ctest_teardown_mock_binary(non_existent_bin), -1,
                "ctest_teardown_mock_binary() call for non existent mock "
                "binary should return -1 (fail)");
}

CTEST(SUITE_NAME, test_mock_binary_teardown_fails_with_non_mock_binary) {
  const char *mock_bin_path = "mock_bin_33910294";

  FILE *f = fopen(mock_bin_path, "w"); // bypass mock binary setup
  fclose(f);

  ASSERT_INT_EQ(ctest_teardown_mock_binary(mock_bin_path), -1,
                "ctest_teardown_mock_binary() call should return -1 (fail) if "
                "binary is not on the mock stack");
  ASSERT_INT_EQ(access(mock_bin_path, F_OK), 0,
                "Non-mock binary should still exist after mock teardown "
                "attempt on its path");
  unlink(mock_bin_path); // manual teardown
}

// teardown all mocks LIFO tests

CTEST(SUITE_NAME, test_teardown_all_mocks_empty_mock_stack) {
  ASSERT_INT_EQ(ctest_teardown_all_mocks(), 0,
                "When no mocks have been created, ctest_teardown_all_mocks() "
                "should return 0 (success)");
}

CTEST(SUITE_NAME, test_teardown_all_mocks_complex_mock_structure) {
  const char *mock_dir_path = "mock_dir_05849944/";
  const char *mock_file_path = "mock_dir_05849944/mock_file_58933333";
  const char *mock_bin_path = "mock_bin_65387242";

  ctest_setup_mock_dir(mock_dir_path);
  ctest_setup_mock_file(mock_file_path, NULL);
  ctest_setup_mock_binary(mock_bin_path, "int main(void) { return 0; }");

  ASSERT_INT_EQ(ctest_teardown_all_mocks(), 0,
                "ctest_teardown_all_mocks() should return 0 (success) when "
                "tearing down a complex LIFO mock hierarchy");
  ASSERT_INT_EQ(access(mock_dir_path, F_OK), -1,
                "Parent mock directory should be unlinked after bulk teardown");
  ASSERT_INT_EQ(access(mock_file_path, F_OK), -1,
                "Child mock file should be unlinked after bulk teardown");
  ASSERT_INT_EQ(access(mock_bin_path, F_OK), -1,
                "Mock binary should be unlinked after bulk teardown");
}

CTEST(SUITE_NAME, test_interleaved_targeted_and_bulk_mock_teardown) {
  const char *file1 = "mock_file_77182910";
  const char *file2 = "mock_file_82395230";
  const char *file3 = "mock_file_00823500";

  ctest_setup_mock_file(file1, "1");
  ctest_setup_mock_file(file2, "2");
  ctest_setup_mock_file(file3, "3");

  ASSERT_INT_EQ(
      ctest_teardown_mock_file(file2), 0,
      "Targeted teardown of intermediate mock file should return 0 (success)");
  ASSERT_INT_EQ(access(file2, F_OK), -1,
                "Targeted mock file should be unlinked immediately");

  ASSERT_INT_EQ(
      ctest_teardown_all_mocks(), 0,
      "Bulk teardown after targeted removal should return 0 (success)");
  ASSERT_INT_EQ(access(file1, F_OK), -1,
                "First mock file should be unlinked during bulk teardown");
  ASSERT_INT_EQ(access(file3, F_OK), -1,
                "Third mock file should be unlinked during bulk teardown");
}

CTEST(SUITE_NAME, test_teardown_all_mocks_idempotent_call) {
  const char *mock_file = "mock_file_91028344";
  ctest_setup_mock_file(mock_file, "data");

  ASSERT_INT_EQ(
      ctest_teardown_all_mocks(), 0,
      "First call to ctest_teardown_all_mocks() should return 0 (success)");
  ASSERT_INT_EQ(ctest_teardown_all_mocks(), 0,
                "Subsequent back-to-back call to ctest_teardown_all_mocks() on "
                "empty stack should return 0 (success)");
}

CTEST(SUITE_NAME, test_teardown_all_mocks_allows_path_recreation) {
  const char *mock_file = "mock_file_recreate_02935102";
  ctest_setup_mock_file(mock_file, "v1");
  ctest_teardown_all_mocks();

  ASSERT_INT_EQ(ctest_setup_mock_file(mock_file, "v2"), 0,
                "Setting up a mock path after bulk teardown should succeed");
  ctest_teardown_all_mocks();
}

CTEST(SUITE_NAME, test_teardown_all_mocks_handles_external_file_removal) {
  const char *mock_file = "mock_file_external_del_99845239";
  ctest_setup_mock_file(mock_file, "data");
  unlink(mock_file);

  ASSERT_INT_EQ(ctest_teardown_all_mocks(), 0,
                "ctest_teardown_all_mocks() should return 0 (success) when a "
                "tracked mock resource was already unlinked externally");
}
