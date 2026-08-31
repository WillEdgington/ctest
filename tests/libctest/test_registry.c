#include "libctest/registry.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <string.h>

#define SUITE_NAME test_registry

static void dummy_fn_a(void) {}
static void dummy_fn_b(void) {}

// test lifecycle: initialise registry, add 2 dummy TestCase records across
// suites, retrieve and inspect
CTEST(SUITE_NAME, test_registry_basic_lifecycle) {
  CTestRegistry reg;
  ctest_registry_init(&reg);

  ctest_registry_add(&reg, "suite_a", "test_1", dummy_fn_a);
  ctest_registry_add(&reg, "suite_b", "test_2", dummy_fn_b);

  ASSERT_INT_EQ((int)reg.suites.count, 2, "Registry should contain 2 suites");

  const TestSuite *s0 = (const TestSuite *)vector_get(&reg.suites, 0);
  const TestSuite *s1 = (const TestSuite *)vector_get(&reg.suites, 1);

  ASSERT(s0 != NULL && s1 != NULL,
         "Valid suite indexes should return non-null TestSuite");
  if (s0 != NULL && s1 != NULL) {
    ASSERT_STR_EQ(s0->suite_name, "suite_a",
                  "Suite 0 name should match assigned");
    ASSERT_STR_EQ(s1->suite_name, "suite_b",
                  "Suite 1 name should match assigned");

    ASSERT_INT_EQ((int)s0->cases.count, 1, "Suite 0 should have 1 test case");
    ASSERT_INT_EQ((int)s1->cases.count, 1, "Suite 1 should have 1 test case");

    const TestCase *tc0 = (const TestCase *)vector_get(&s0->cases, 0);
    const TestCase *tc1 = (const TestCase *)vector_get(&s1->cases, 0);

    ASSERT(tc0 != NULL && tc1 != NULL,
           "Valid case indexes should return non-null TestCase");
    if (tc0 != NULL && tc1 != NULL) {
      ASSERT(
          tc0->fn == dummy_fn_a && tc1->fn == dummy_fn_b,
          "Function pointers in TestCase items should match the ones assigned");
      ASSERT_STR_EQ(tc0->test_name, "test_1",
                    "TestCase at index 0 of suite_a should match test_1");
      ASSERT_STR_EQ(tc1->test_name, "test_2",
                    "TestCase at index 0 of suite_b should match test_2");
    }
  }
  ctest_registry_clear(&reg);
}

// test oob access: check out-of-bounds access on suites and cases vectors
CTEST(SUITE_NAME, test_registry_out_of_bounds) {
  CTestRegistry reg;
  ctest_registry_init(&reg);

  ctest_registry_add(&reg, "suite_a", "test_1", dummy_fn_a);

  ASSERT_PTR_NULL(vector_get(&reg.suites, 1),
                  "Suite index equal to suite count should return NULL");
  ASSERT_PTR_NULL(vector_get(&reg.suites, 999),
                  "Large out-of-bounds suite index should return NULL");

  const TestSuite *s0 = (const TestSuite *)vector_get(&reg.suites, 0);
  ASSERT_PTR_NOT_NULL(s0, "Suite index 0 should be valid");
  if (s0 != NULL) {
    ASSERT_PTR_NULL(vector_get(&s0->cases, 1),
                    "Case index equal to case count should return NULL");
    ASSERT_PTR_NULL(vector_get(&s0->cases, 999),
                    "Large out-of-bounds case index should return NULL");
  }

  ctest_registry_clear(&reg);
}

// test global init: Reset a CTestRegistry struct to {0} via memset. call add
// and verify element is added successfully (auto-initialise zero-initialised
// registry)
CTEST(SUITE_NAME, test_registry_lazy_init) {
  CTestRegistry reg;
  memset(&reg, 0, sizeof(reg));

  ctest_registry_add(&reg, "suite_lazy", "test_lazy", dummy_fn_a);

  ASSERT_INT_EQ((int)reg.suites.count, 1,
                "Lazy-initialised registry should contain 1 suite");
  const TestSuite *s = (const TestSuite *)vector_get(&reg.suites, 0);
  ASSERT_PTR_NOT_NULL(s, "Suite pointer should not be NULL");
  if (s != NULL) {
    ASSERT_STR_EQ(s->suite_name, "suite_lazy",
                  "Suite name should match after lazy init");
    ASSERT_INT_EQ((int)s->cases.count, 1, "Suite should contain 1 test case");
    const TestCase *tc = (const TestCase *)vector_get(&s->cases, 0);
    ASSERT_PTR_NOT_NULL(tc, "TestCase pointer should not be NULL");
    if (tc != NULL) {
      ASSERT_STR_EQ(tc->test_name, "test_lazy",
                    "Test name in TestCase should match after lazy init");
    }
  }

  ctest_registry_clear(&reg);
}

// test order preservation: push x amount of TestSuite and TestCase structs to
// CTestRegistry, iterate through and check they are in the expected order
// (FIFO)
CTEST(SUITE_NAME, test_registry_order_preservation) {
  CTestRegistry reg;
  ctest_registry_init(&reg);

  size_t ntests = 100;
  size_t char_buf_len = 32;
  char suite_names[ntests][char_buf_len];
  char test_names[ntests][char_buf_len];

  for (size_t i = 0; i < ntests; i++) {
    snprintf(suite_names[i], sizeof(suite_names[i]), "suite_%zu", i);
    snprintf(test_names[i], sizeof(test_names[i]), "test_%zu", i);
    ctest_registry_add(&reg, suite_names[i], test_names[i], dummy_fn_a);
  }

  ASSERT_INT_EQ((int)reg.suites.count, (int)ntests,
                "Registry should contain all added suites");

  size_t sname_match = 0;
  size_t tname_match = 0;
  for (size_t i = 0; i < ntests; i++) {
    const TestSuite *s = (const TestSuite *)vector_get(&reg.suites, i);
    if (s != NULL) {
      if (strcmp(s->suite_name, suite_names[i]) == 0)
        sname_match++;
      const TestCase *tc = (const TestCase *)vector_get(&s->cases, 0);
      if (tc != NULL && strcmp(tc->test_name, test_names[i]) == 0)
        tname_match++;
    }
  }

  ASSERT_INT_EQ(sname_match, ntests,
                "Suite name should match expected for suite at given index in "
                "FIFO order");
  ASSERT_INT_EQ(
      tname_match, ntests,
      "Test name should match expected for test at given index in FIFO order");

  ctest_registry_clear(&reg);
}

// test double clear: Create non-empty CTestRegistry. Call clear, assert count
// == 0, call clear again and verify that no memory fault occurs
CTEST(SUITE_NAME, test_registry_double_clear) {
  CTestRegistry reg;
  ctest_registry_init(&reg);
  ctest_registry_add(&reg, "suite_clear", "test_clear", dummy_fn_a);

  ctest_registry_clear(&reg);
  ASSERT_INT_EQ((int)reg.suites.count, 0,
                "Suite count should be 0 after first clear");

  ctest_registry_clear(&reg);
  ASSERT_INT_EQ((int)reg.suites.count, 0,
                "Suite count should remain 0 after second clear");
}

// test global singleton state: check get global registry returns a stable
// static reference
CTEST(SUITE_NAME, test_registry_global_singleton) {
  CTestRegistry *reg1 = ctest_get_global_registry();
  CTestRegistry *reg2 = ctest_get_global_registry();

  ASSERT_PTR_NOT_NULL(reg1, "Global registry pointer should not be NULL");
  ASSERT(reg1 == reg2,
         "ctest_get_global_registry should return a stable pointer");
}
