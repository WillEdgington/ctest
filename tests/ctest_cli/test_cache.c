#include "ctest_cli/cache.h"
#include "ctest_cli/config.h"
#include "ctest_cli/discovery.h"
#include "ctest_cli/executor.h"
#include "ctest_cli/session.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SUITE_NAME test_cache

// .ctest_cache line: "binary_path:mtime:tests_passed\n"

CTEST(SUITE_NAME, test_cache_load_and_save_valid_file) {
  const char *cache_load_file_path = ".ctest_cache_00935382";
  const char *cache_save_file_path = ".ctest_cache_99998593";
  const char *cache_contents = "mock_tests/test_a:835923:12\n"
                               "mock_tests/test_b:126742:6\n"
                               "mock_tests/test_c:472648:20\n";
  ctest_setup_mock_file(cache_load_file_path, cache_contents);

  ASSERT_INT_EQ(ctest_cache_load(cache_load_file_path), 0,
                "ctest_cache_load() should return successful status (0) after "
                "loading valid cache file");
  ASSERT_INT_EQ(ctest_cache_save_and_free(cache_save_file_path), 0,
                "ctest_cache_save_and_free() should return status successful "
                "(0) after saving straight after valid load");

  FILE *f = fopen(cache_save_file_path, "rb");
  ASSERT_PTR_NOT_NULL(f, "cache_file_path used in ctest_cache_save_and_free() "
                         "should be able to be used to load said file");

  char buf[2048];
  size_t read_bytes = fread(buf, 1, 2048, f);
  buf[read_bytes] = '\0';
  fclose(f);

  ASSERT_PTR_NOT_NULL(strstr(buf, "mock_tests/test_a:835923:12\n"),
                      "test_a cached results from the load cache should be "
                      "present in saved cache");
  ASSERT_PTR_NOT_NULL(strstr(buf, "mock_tests/test_b:126742:6\n"),
                      "test_b cached results from the load cache should be "
                      "present in saved cache");
  ASSERT_PTR_NOT_NULL(strstr(buf, "mock_tests/test_c:472648:20\n"),
                      "test_c cached results from the load cache should be "
                      "present in saved cache");

  unlink(cache_save_file_path);
}

CTEST(SUITE_NAME, test_cache_update_existing_cache_with_new_passing) {
  const char *cache_file_path = ".ctest_cache_22829522";
  const char *cache_contents = "mock_tests/test_a:425224:24\n";
  ctest_setup_mock_file(cache_file_path, cache_contents);

  ctest_cache_load(cache_file_path);

  SuiteMetrics suite = {
      .total_runs = 32, .total_failures = 0, .state = SUITE_DEFAULT};
  ctest_cache_update("mock_tests/test_b", 535324, &suite);

  ctest_cache_save_and_free(cache_file_path);
  FILE *f = fopen(cache_file_path, "rb");

  char buf[2048];
  size_t read_bytes = fread(buf, 1, 2048, f);
  buf[read_bytes] = '\0';
  fclose(f);

  ASSERT_PTR_NOT_NULL(strstr(buf, "mock_tests/test_a:425224:24\n"),
                      "Updating cache with a new passing suite should not "
                      "remove unrelated cached results");
  ASSERT_PTR_NOT_NULL(strstr(buf, "mock_tests/test_b:535324:32\n"),
                      "Updating cache with a new passing suite should add the "
                      "result to the cache");
}

CTEST(SUITE_NAME, test_cache_update_existing_cache_with_existing_passing) {
  const char *cache_file_path = ".ctest_cache_09538539";
  const char *cache_contents = "mock_tests/test_a:839523:10\n";
  ctest_setup_mock_file(cache_file_path, cache_contents);

  ctest_cache_load(cache_file_path);

  SuiteMetrics suite = {
      .total_runs = 21, .total_failures = 0, .state = SUITE_DEFAULT};
  ctest_cache_update("mock_tests/test_a", 783523, &suite);

  ctest_cache_save_and_free(cache_file_path);
  FILE *f = fopen(cache_file_path, "rb");

  char buf[2048];
  size_t read_bytes = fread(buf, 1, 2048, f);
  buf[read_bytes] = '\0';
  fclose(f);

  ASSERT_PTR_NULL(strstr(buf, "mock_tests/test_a:839523:10\n"),
                  "Updating cache with a passing suite that already exists in "
                  "the cache should remove previous instance");
  ASSERT_PTR_NOT_NULL(strstr(buf, "mock_tests/test_a:783523:21\n"),
                      "Updating cache with a passing suite that already exists "
                      "in the cache should overwrite with new result");
}

CTEST(SUITE_NAME, test_cache_update_existing_cache_with_new_failing) {
  const char *cache_file_path = ".ctest_cache_55738532";
  const char *cache_contents = "mock_tests/test_a:839523:10\n";
  ctest_setup_mock_file(cache_file_path, cache_contents);

  ctest_cache_load(cache_file_path);

  SuiteMetrics suite = {
      .total_runs = 21, .total_failures = 5, .state = SUITE_DEFAULT};
  ctest_cache_update("mock_tests/test_b", 783523, &suite);

  ctest_cache_save_and_free(cache_file_path);
  FILE *f = fopen(cache_file_path, "rb");

  char buf[2048];
  size_t read_bytes = fread(buf, 1, 2048, f);
  buf[read_bytes] = '\0';
  fclose(f);

  ASSERT_PTR_NOT_NULL(strstr(buf, "mock_tests/test_a:839523:10\n"),
                      "Updating the cache with a new failing suite should not "
                      "affect remove unrelated results");
  ASSERT_PTR_NULL(strstr(buf, "mock_tests/test_b:"),
                  "Updating the cache with a new failing suite should not "
                  "write suite to the cache");
}

CTEST(SUITE_NAME, test_cache_update_existing_cache_with_existing_failing) {
  const char *cache_file_path = ".ctest_cache_00020853";
  const char *cache_contents = "mock_tests/test_a:839523:10\n";
  ctest_setup_mock_file(cache_file_path, cache_contents);

  ctest_cache_load(cache_file_path);

  SuiteMetrics suite = {
      .total_runs = 21, .total_failures = 6, .state = SUITE_DEFAULT};
  ctest_cache_update("mock_tests/test_a", 783523, &suite);

  ctest_cache_save_and_free(cache_file_path);
  FILE *f = fopen(cache_file_path, "rb");

  char buf[2048];
  size_t read_bytes = fread(buf, 1, 2048, f);
  buf[read_bytes] = '\0';
  fclose(f);

  ASSERT_PTR_NULL(strstr(buf, "mock_tests/test_a:"),
                  "Updating cache with a failing suite that already exists in "
                  "the cache should remove that instance from the cache");
}

CTEST(SUITE_NAME, test_cache_update_existing_cache_with_new_crashing) {
  const char *cache_file_path = ".ctest_cache_82350300";
  const char *cache_contents = "mock_tests/test_a:839523:10\n";
  ctest_setup_mock_file(cache_file_path, cache_contents);

  ctest_cache_load(cache_file_path);

  SuiteMetrics suite = {
      .total_runs = 21, .total_failures = 0, .state = SUITE_CRASH};
  ctest_cache_update("mock_tests/test_b", 783523, &suite);

  ctest_cache_save_and_free(cache_file_path);
  FILE *f = fopen(cache_file_path, "rb");

  char buf[2048];
  size_t read_bytes = fread(buf, 1, 2048, f);
  buf[read_bytes] = '\0';
  fclose(f);

  ASSERT_PTR_NULL(strstr(buf, "mock_tests/test_b:"),
                  "Updating the cache with a new crashing suite should not "
                  "write suite to the cache");
}

CTEST(SUITE_NAME, test_cache_update_existing_cache_with_existing_crashing) {
  const char *cache_file_path = ".ctest_cache_00004340";
  const char *cache_contents = "mock_tests/test_a:839523:10\n";
  ctest_setup_mock_file(cache_file_path, cache_contents);

  ctest_cache_load(cache_file_path);

  SuiteMetrics suite = {
      .total_runs = 21, .total_failures = 0, .state = SUITE_CRASH};
  ctest_cache_update("mock_tests/test_a", 783523, &suite);

  ctest_cache_save_and_free(cache_file_path);
  FILE *f = fopen(cache_file_path, "rb");

  char buf[2048];
  size_t read_bytes = fread(buf, 1, 2048, f);
  buf[read_bytes] = '\0';
  fclose(f);

  ASSERT_PTR_NULL(strstr(buf, "mock_tests/test_a:"),
                  "Updating cache with a crashing suite that already exists in "
                  "the cache should remove that instance from the cache");
}

CTEST(SUITE_NAME, test_cache_update_new_cache_with_passing_suite) {
  const char *cache_file_path = ".ctest_cache_11117581";

  ctest_cache_load(cache_file_path);

  SuiteMetrics suite = {
      .total_runs = 21, .total_failures = 0, .state = SUITE_DEFAULT};
  ctest_cache_update("mock_tests/test_a", 783523, &suite);

  ctest_cache_save_and_free(cache_file_path);
  FILE *f = fopen(cache_file_path, "rb");

  char buf[2048];
  size_t read_bytes = fread(buf, 1, 2048, f);
  buf[read_bytes] = '\0';
  fclose(f);

  ASSERT_STR_EQ(buf, "mock_tests/test_a:783523:21\n",
                "Updating new cache with passing suite should create and "
                "populate cache with suite instance");

  unlink(cache_file_path);
}

CTEST(SUITE_NAME, test_cache_update_new_cache_with_failing_suite) {
  const char *cache_file_path = ".ctest_cache_28530953";

  ctest_cache_load(cache_file_path);

  SuiteMetrics suite = {
      .total_runs = 21, .total_failures = 5, .state = SUITE_DEFAULT};
  ctest_cache_update("mock_tests/test_a", 783523, &suite);

  ctest_cache_save_and_free(cache_file_path);
  FILE *f = fopen(cache_file_path, "rb");

  char buf[2048];
  size_t read_bytes = fread(buf, 1, 2048, f);
  buf[read_bytes] = '\0';
  fclose(f);

  ASSERT_STR_EQ(
      buf, "",
      "Updating new cache with failing suite should create empty cache");

  unlink(cache_file_path);
}

CTEST(SUITE_NAME, test_cache_check_and_bypass) {
  const char *mock_tests_dir = "mock_tests_83523999";
  const char *mock_test_a = "mock_tests_83523999/test_a";
  const char *mock_test_b = "mock_tests_83523999/test_b";
  ctest_setup_mock_dir(mock_tests_dir);
  ctest_setup_mock_binary(mock_test_a, "int main(void) {}");
  ctest_setup_mock_binary(mock_test_b, "int main(void) {}");

  ctest_cache_load(".ctest_cache_09025395");

  struct stat st_a;
  stat(mock_test_a, &st_a);

  ctest_cache_update(mock_test_a, (unsigned long)st_a.st_mtime,
                     &(SuiteMetrics){.total_runs = 10, .state = SUITE_DEFAULT});
  ctest_cache_update(mock_test_b, 0,
                     &(SuiteMetrics){.total_runs = 15, .state = SUITE_DEFAULT});
  Vector *test_bins = ctest_discover_tests(mock_tests_dir);

  SessionMetrics session = {0};

  char buf[2048];
  ctest_capture_stdout_start();
  ctest_cache_check_and_bypass(test_bins, &session, CTEST_VERBOSITY_NORMAL);
  ctest_capture_stdout_end(buf, sizeof(buf));

  ASSERT_INT_EQ(test_bins->count, 1,
                "ctest_cache_check_and_bypass() call should remove cached "
                "suite binaries from test_bins");
  ASSERT_STR_EQ(
      (char *)vector_get(test_bins, 0), mock_test_b,
      "Remaining suite binary path in test_bins should be the uncached suite");
  ASSERT_INT_EQ(session.total_suites, 1,
                "SessionMetrics.total_suites should increase by the number of "
                "cached suites");
  ASSERT_INT_EQ(session.total_runs, 10,
                "SessionMetrics.total_runs should increase by the number of "
                "tests runs in the cached test suites");
  ASSERT_PTR_NOT_NULL(strstr(buf, CTEST_COLOR_GREEN
                             "[PASS]  " CTEST_COLOR_RESET
                             "mock_tests_83523999/test_a: " CTEST_COLOR_GREEN
                             ".........."),
                      "Cached result should report correctly to stdout");

  vector_free(test_bins);
  free(test_bins);
  ctest_cache_save_and_free(NULL);
}
