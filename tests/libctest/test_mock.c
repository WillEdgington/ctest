#include "libctest/mock.h"
#include <ctest/ctest.h>
#include <stdlib.h>

#define SUITE_NAME test_mock

CTEST_TEARDOWN(SUITE_NAME) { ctest_mock_clear(); }

// ctest_mock_path_exists() tests
// returns true if path exists on machine
// returns false if the path does not exist
// returns false if path == NULL

CTEST(SUITE_NAME, test_mock_path_exists_null_path_returns_false) {
  ASSERT(ctest_mock_path_exists(NULL) == false,
         "ctest_mock_path_exists() should return false if given path is NULL");
}

CTEST(SUITE_NAME, test_mock_path_exists_non_existent_path_returns_false) {
  const char *mock_path = "non_existent_12864344/horse_fish_guitar_jGi43FEe1";

  ASSERT(ctest_mock_path_exists(mock_path) == false,
         "ctest_mock_path_exists() should return false for non-existent path");
}

CTEST(SUITE_NAME, test_mock_path_exists_existent_path_returns_true) {
  const char *mock_path = "tmp_existent_file_58392222";
  ctest_setup_mock_file(mock_path, "");

  ASSERT(ctest_mock_path_exists(mock_path),
         "ctest_mock_path_exists() should return true for existent path");

  ctest_teardown_mock_file(mock_path);
}

// ctest_mock_push() tests
// validate path != NULL (should return -1)
// if the path already exists then return -1
// allocates a deep copy of path and appends the entry to the internal vector
// return 0 on success (mock successfully pushed)

CTEST(SUITE_NAME, test_mock_push_null_path_returns_error) {
  ASSERT_INT_EQ(ctest_mock_push(NULL, CTEST_MOCK_FILE), -1,
                "ctest_mock_push() should return -1 (error) if path is NULL");
}

CTEST(SUITE_NAME, test_mock_push_existent_path_returns_error) {
  const char *mock_path = "tmp_existent_file_35278594";
  ctest_setup_mock_file(mock_path, "");
  ASSERT_INT_EQ(ctest_mock_push(mock_path, CTEST_MOCK_FILE), -1,
                "ctest_mock_push() should return -1 (error) if path exists");
  ctest_teardown_mock_file(mock_path);
}

CTEST(SUITE_NAME, test_mock_valid_push_increases_mock_count) {
  const char *mock_path = "tmp_mock_file_58393443";
  ASSERT_INT_EQ(ctest_mock_push(mock_path, CTEST_MOCK_FILE), 0,
                "ctest_mock_push() should return 0 (ok) if registering "
                "currently non-existent path");

  ASSERT_INT_EQ(ctest_mock_count(), 1,
                "ctest_mock_count() should return 1 after a single push to the "
                "mock vector register");
}

// ctest_mock_get() tests
// get index OOB should return NULL
// in bound index should return the entry
// entry->path should be a copy of the intended path string (strings are equal,
// their pointers are not)

CTEST(SUITE_NAME, test_mock_get_oob_index_returns_null) {
  ASSERT_PTR_NULL(ctest_mock_get(5),
                  "Calling ctest_mock_get() with an out-of-bounds index should "
                  "return NULL pointer");
}

CTEST(SUITE_NAME, test_mock_get_in_bounds_returns_entry_with_path_copy) {
  const char *mock_path = "tmp_mock_file_58393443";
  ctest_mock_push(mock_path, CTEST_MOCK_FILE);

  const CTestMockEntry *entry = ctest_mock_get(0);

  ASSERT_PTR_NOT_NULL(entry, "Calling ctest_mock_get() for an index in bounds "
                             "should return a non-null pointer to the entry");
  ASSERT_STR_EQ(
      entry->path, mock_path,
      "Path in retrieved entry should be equivalent to the one pushed");
  ASSERT(entry->path != mock_path,
         "Pointer should be different for the path in the retrieved entry and "
         "the one pushed (copy)");
}

// ctest_mock_pop() tests
// return -1 if the stack is empty
// return -1 if out_entry == NULL
// pops the most recent mock from the underlying vector, write the contents to
// *out_entry, return 0

CTEST(SUITE_NAME, test_mock_pop_returns_error_on_empty) {
  CTestMockEntry entry;
  ASSERT_INT_EQ(ctest_mock_pop(&entry), -1,
                "ctest_mock_pop() call on empty mock entries vector should "
                "return -1 (error)");
}

CTEST(SUITE_NAME, test_mock_pop_with_input_entry_point_null_errors) {
  const char *mock_path = "tmp_mock_file_58393443";
  ctest_mock_push(mock_path, CTEST_MOCK_FILE);

  ASSERT_INT_EQ(
      ctest_mock_pop(NULL), -1,
      "ctest_mock_pop() call with NULL entry input should return -1 (error)");
}

CTEST(SUITE_NAME, test_mock_pop_removes_from_top_of_mock_entry_stack) {
  const char *mock_path0 = "tmp_mock_file_0_58393443";
  ctest_mock_push(mock_path0, CTEST_MOCK_FILE);

  const char *mock_path1 = "tmp_mock_file_1_38593342/";
  ctest_mock_push(mock_path1, CTEST_MOCK_DIR);

  CTestMockEntry entry;

  ASSERT_INT_EQ(ctest_mock_pop(&entry), 0,
                "Valid ctest_mock_pop() call should return 0 (ok)");

  ASSERT_INT_EQ(
      ctest_mock_count(), 1,
      "ctest_mock_count() should return the correct number after pop");

  ASSERT_PTR_NOT_NULL(entry.path,
                      "Popped entry should have non-NULL path string");
  ASSERT_STR_EQ(entry.path, mock_path1,
                "Popped entry should have the correct path string");
  ASSERT_INT_EQ(
      entry.type, CTEST_MOCK_DIR,
      "Popped entry should have the correct CTestMockType enum assigned");
  free(entry.path);
}

// ctest_mock_remove_by_path() tests
// if path found in vector, free the internal path string in the entry, shift
// remaining entries, return 0 return -1 if path not in the stack return -1 if
// path == NULL

CTEST(SUITE_NAME, test_mock_remove_by_path_null_path_returns_error) {
  ASSERT_INT_EQ(ctest_mock_remove_by_path(NULL), -1,
                "ctest_mock_remove_by_path() should return -1 if path is NULL");
}

CTEST(SUITE_NAME, test_mock_remove_by_path_non_existent_returns_error) {
  ASSERT_INT_EQ(ctest_mock_remove_by_path("non_existent_path_999"), -1,
                "ctest_mock_remove_by_path() should return -1 if path is not "
                "in the registry");
}

CTEST(SUITE_NAME, test_mock_remove_by_path_removes_entry_and_shifts_stack) {
  const char *path0 = "tmp_mock_file_0_58393443";
  const char *path1 = "tmp_mock_dir_1_38593342/";
  const char *path2 = "tmp_mock_file_2_88392111";

  ctest_mock_push(path0, CTEST_MOCK_FILE);
  ctest_mock_push(path1, CTEST_MOCK_DIR);
  ctest_mock_push(path2, CTEST_MOCK_FILE);

  ASSERT_INT_EQ(ctest_mock_remove_by_path(path1), 0,
                "ctest_mock_remove_by_path() should return 0 when removing "
                "existing path");

  ASSERT_INT_EQ(ctest_mock_count(), 2,
                "ctest_mock_count() should decrement to 2 after removal");

  const CTestMockEntry *entry0 = ctest_mock_get(0);
  const CTestMockEntry *entry1 = ctest_mock_get(1);

  ASSERT_PTR_NOT_NULL(entry0, "Entry at index 0 should exist");
  ASSERT_STR_EQ(entry0->path, path0, "Index 0 should remain path0");

  ASSERT_PTR_NOT_NULL(entry1, "Entry at index 1 should exist");
  ASSERT_STR_EQ(entry1->path, path2,
                "Index 1 should now be path2 after shifting");
}
