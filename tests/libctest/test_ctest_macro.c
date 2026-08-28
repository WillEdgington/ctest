#include "libctest/registry.h"
#include <ctest/ctest.h>
#include <stdio.h>

#define SUITE_NAME test_ctest_macro

static int test_one_ran = 0;
static int test_two_ran = 0;

CTEST(sample_suite, test_one) { test_one_ran = 1; }

CTEST(sample_suite, test_two) { test_two_ran = 1; }

static void test_ctest_macro(void) {
  CTestRegistry *reg = ctest_get_global_registry();

  ASSERT_INT_EQ((int)reg->cases.count, 2,
                "Constructor should auto-register 2 tests");

  const TestCase *tc0 = ctest_registry_get(reg, 0);
  const TestCase *tc1 = ctest_registry_get(reg, 1);

  ASSERT(tc0 != NULL && tc1 != NULL,
         "Valid indexes in global registry should not return NULL on get call");
  if (tc0 != NULL && tc1 != NULL) {
    ASSERT_STR_EQ(
        tc0->suite_name, "sample_suite",
        "TestCase at index 0 (of global reg) should match the one assigned");
    ASSERT_STR_EQ(
        tc0->test_name, "test_one",
        "TestCase at index 0 (of global reg) should match the one assigned");

    // call both functions manually
    tc0->fn();
    tc1->fn();
    ASSERT_INT_EQ(test_one_ran, 1,
                  "Executing tc0->fn should run test_one body");
    ASSERT_INT_EQ(test_two_ran, 1,
                  "Executing tc1->fn should run test_two body");
  }
  ctest_registry_clear(reg);
}

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_ctest_macro();

  ctest_summary();
  return ctest_fail_count > 0 ? 1 : 0;
}
