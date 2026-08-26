#include "libctest/registry.h"
#include <ctest/ctest.h>
#include <string.h>

static void dummy_fn_a(void) {}
static void dummy_fn_b(void) {}

// test lifecycle: initialise registry, add 2 dummy TestCase records, retrieve
// from indexes 0 and 1
static void test_registry_basic_lifecycle(void) {
  CTestRegistry reg;
  ctest_registry_init(&reg);

  ctest_registry_add(&reg, "suite_a", "test_1", dummy_fn_a);
  ctest_registry_add(&reg, "suite_b", "test_2", dummy_fn_b);

  const TestCase *tc0 = ctest_registry_get(&reg, 0);
  const TestCase *tc1 = ctest_registry_get(&reg, 1);

  ASSERT(tc0 != NULL && tc1 != NULL, "In bound indexes of CTestRegistry should "
                                     "return valid TestCase on get call");
  if (tc0 != NULL && tc1 != NULL) {
    ASSERT(tc0->fn == dummy_fn_a && tc1->fn == dummy_fn_b,
           "Function pointers in CTestRegistry TestCase items should match the "
           "ones given when added");

    ASSERT_STR_EQ(tc0->suite_name, "suite_a",
                  "TestCase at index 0 should match the one assigned");
    ASSERT_STR_EQ(tc0->test_name, "test_1",
                  "TestCase at index 0 should match the one assigned");

    ASSERT_STR_EQ(tc1->suite_name, "suite_b",
                  "TestCase at index 1 should match the one assigned");
    ASSERT_STR_EQ(tc1->test_name, "test_2",
                  "TestCase at index 1 should match the one assigned");
  }
  ctest_registry_clear(&reg);
}

// test oob access: call out of index get
static void test_registry_out_of_bounds(void) {
  CTestRegistry reg;
  ctest_registry_init(&reg);

  ctest_registry_add(&reg, "suite_a", "test_1", dummy_fn_a);

  ASSERT_PTR_NULL(ctest_registry_get(&reg, 1),
                  "Index equal to item count should return NULL");
  ASSERT_PTR_NULL(ctest_registry_get(&reg, 999),
                  "Large out-of-bounds index should return NULL");

  ctest_registry_clear(&reg);
}

// test global init: Reset a CTestRegistry struct to {0} via memset. call add
// and verify element is add successfully (auto-initialise zero-initialised
// global registry)
static void test_registry_lazy_init(void) {
  CTestRegistry reg;
  memset(&reg, 0, sizeof(reg));

  ctest_registry_add(&reg, "suite_lazy", "test_lazy", dummy_fn_a);

  const TestCase *tc = ctest_registry_get(&reg, 0);
  ASSERT_PTR_NOT_NULL(tc,
                      "Lazy-initialised registry should contain inserted case");
  if (tc != NULL) {
    ASSERT_STR_EQ(tc->suite_name, "suite_lazy",
                  "Suite name in TestCase should match after lazy init");
    ASSERT_STR_EQ(tc->test_name, "test_lazy",
                  "Test name in TestCase should match after lazy init");
  }

  ctest_registry_clear(&reg);
}

// test order preservation: push x amount of TestCase structs to CTestRegistry,
// iterate through and check they are in the expected order (FIFO)
static void test_registry_order_preservation(void) {
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

  size_t non_null_cnt = 0;
  size_t sname_match = 0;
  size_t tname_match = 0;
  for (size_t i = 0; i < ntests; i++) {
    const TestCase *tc = ctest_registry_get(&reg, (size_t)i);
    if (tc != NULL) {
      non_null_cnt++;
      if (strcmp(tc->suite_name, suite_names[i]) == 0)
        sname_match++;
      if (strcmp(tc->test_name, test_names[i]) == 0)
        tname_match++;
    }
  }

  ASSERT_INT_EQ(non_null_cnt, ntests,
                "TestCase should be non-null for all valid indexes");
  ASSERT_INT_EQ(sname_match, ntests,
                "Suite name should match the expected for TestCase at given "
                "index given FIFO order");
  ASSERT_INT_EQ(tname_match, ntests,
                "Test name should match the expected for TestCase at given "
                "index given FIFO order");

  ctest_registry_clear(&reg);
}

// test double clear: Create non-empty CTestRegistry. Call clear, assert count
// == 0, call clear again and verify that there are no memory fault occurs
static void test_registry_double_clear(void) {
  CTestRegistry reg;
  ctest_registry_init(&reg);
  ctest_registry_add(&reg, "suite_clear", "test_clear", dummy_fn_a);

  ctest_registry_clear(&reg);
  ASSERT_INT_EQ(reg.cases.count, 0, "Count should be 0 after first clear");

  ctest_registry_clear(&reg);
  ASSERT_INT_EQ(reg.cases.count, 0, "Count should remain 0 after second clear");
}

// test global singleton state: need to check get global registry returns a
// stable static reference across calls. call it twice, verify both returns are
// the same.
static void test_registry_global_singleton(void) {
  CTestRegistry *reg1 = ctest_get_global_registry();
  CTestRegistry *reg2 = ctest_get_global_registry();

  ASSERT_PTR_NOT_NULL(reg1, "Global registry pointer should not be NULL");
  ASSERT(reg1 == reg2,
         "ctest_get_global_registry should return a stable pointer");

  ctest_registry_clear(reg1);
}

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_registry_basic_lifecycle();
  test_registry_out_of_bounds();
  test_registry_lazy_init();
  test_registry_order_preservation();
  test_registry_double_clear();
  test_registry_global_singleton();

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
