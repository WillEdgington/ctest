#include "libctest/registry.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>

#define SUITE_NAME test_ctest_macro

static int test_one_ran = 0;
static int test_two_ran = 0;

CTEST(sample_suite, test_one) { test_one_ran = 1; }

CTEST(sample_suite, test_two) { test_two_ran = 1; }

static void test_ctest_macro(void) {
  CTestRegistry *reg = ctest_get_global_registry();

  ASSERT_INT_EQ((int)reg->suites.count, 1,
                "Constructor should auto-register 1 suite");

  const TestSuite *suite = (const TestSuite *)vector_get(&reg->suites, 0);
  ASSERT_PTR_NOT_NULL(suite, "Global suite pointer should not be NULL");

  if (suite != NULL) {
    ASSERT_STR_EQ(suite->suite_name, "sample_suite",
                  "Suite name should match target suite name");
    ASSERT_INT_EQ((int)suite->cases.count, 2,
                  "Constructor should auto-register 2 tests in suite");

    const TestCase *tc0 = (const TestCase *)vector_get(&suite->cases, 0);
    const TestCase *tc1 = (const TestCase *)vector_get(&suite->cases, 1);

    ASSERT(tc0 != NULL && tc1 != NULL, "Valid case indexes in global suite "
                                       "should not return NULL on vector_get");
    if (tc0 != NULL && tc1 != NULL) {
      ASSERT_STR_EQ(tc0->test_name, "test_one",
                    "TestCase at index 0 should be test_one");
      ASSERT_STR_EQ(tc1->test_name, "test_two",
                    "TestCase at index 1 should be test_two");

      // call both functions manually
      tc0->fn();
      tc1->fn();
      ASSERT_INT_EQ(test_one_ran, 1,
                    "Executing tc0->fn should run test_one body");
      ASSERT_INT_EQ(test_two_ran, 1,
                    "Executing tc1->fn should run test_two body");
    }
  }
  ctest_registry_clear(reg);
}

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_ctest_macro();

  ctest_summary();
  return ctest_fail_count > 0 ? 1 : 0;
}
