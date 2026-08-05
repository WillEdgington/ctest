#include "ctest_cli/filter.h"
#include <ctest/ctest.h>

static void test_filter_null_patterns(void) {
  ASSERT_INT_EQ(ctest_filter_matches("fake/path", NULL), 1,
                "When pattern is NULL, filter should return 1 (match)");
  ASSERT_INT_EQ(
      ctest_filter_matches("fake/path", ""), 1,
      "When pattern is an empty string, filter should return 1 (match)");
}

static void test_filter_non_glob_patterns(void) {
  ASSERT_INT_EQ(ctest_filter_matches("tests/ctest_cli/test_filter", "tests/"),
                1,
                "Filter should recognise an explicit prefix match (return 1) "
                "for non-glob pattern");
  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter", "test_filter"), 1,
      "Filter should recognise an explicit suffix match (return 1) for "
      "non-glob pattern");
  ASSERT_INT_EQ(ctest_filter_matches("tests/ctest_cli/test_filter", "/ctest_"),
                1,
                "Filter should recognise any explicit sub-string match (return "
                "1) for non-glob pattern");
  ASSERT_INT_EQ(ctest_filter_matches("tests/ctest_cli/test_filter", "te"), 1,
                "Filter should return 1 (match) when there are multiple "
                "explicit instances of pattern in target path");
  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter", "invalid_pattern"), 0,
      "Filter should return 0 (no match) for non-glob pattern that is not a "
      "sub-string of the target path");
}

static void test_filter_glob_patterns(void) {
  // Asterisk (*) wildcard - matches zero or more characters
  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter", "*test_filter"), 1,
      "Filter should match (return 1) leading wildcard asterisk pattern");
  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter", "tests/*"), 1,
      "Filter should match (return 1) trailing wildcard asterisk pattern");
  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter",
                           "tests/*/test_filter"),
      1, "Filter should match (return 1) infix wildcard asterisk pattern");
  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter", "*cli*filter"), 1,
      "Filter should match (return 1) multiple wildcard asterisks");
  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter", "*test_filter*"), 1,
      "Filter should match (return 1) when trailing asterisk matches zero "
      "remaining characters");
  ASSERT_INT_EQ(ctest_filter_matches("tests/ctest_cli/test_filter",
                                     "*tests/ctest_cli/test_filter"),
                1,
                "Filter should match (return 1) when leading asterisk matches "
                "zero preceding characters");
  ASSERT_INT_EQ(ctest_filter_matches("tests/ctest_cli/test_filter", "src/*"), 0,
                "Filter should return 0 (no match) for non-matching wildcard "
                "asterisk pattern");

  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter",
                           "tests/ctest_cli/test_filte?"),
      1, "Filter should match (return 1) single character wildcard ? pattern");
  ASSERT_INT_EQ(ctest_filter_matches("tests/ctest_cli/test_filter",
                                     "tests/ctest_cli/test_filte??"),
                0,
                "Filter should return 0 (no match) when ? character count "
                "exceeds string length");

  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter", "*test_[a-z]ilter"),
      1,
      "Filter should match (return 1) target path when character falls within "
      "bracket range [a-z]");
  ASSERT_INT_EQ(
      ctest_filter_matches("tests/ctest_cli/test_filter", "*test_[0-9]ilter"),
      0,
      "Filter should return 0 (no match) when character does not fall within "
      "bracket range [0-9]");
}

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_filter_null_patterns();
  test_filter_non_glob_patterns();
  test_filter_glob_patterns();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
