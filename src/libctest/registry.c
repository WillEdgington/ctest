#include "registry.h"
#include <clib/vector.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void ctest_registry_init(CTestRegistry *reg) {
  if (reg != NULL && reg->initialised == false) {
    if (vector_init(&reg->suites, sizeof(TestSuite)) != 0) {
      fprintf(stderr, "ctest: registry: failed to initialise suites Vector for "
                      "CTestRegistry\n");
      return;
    }
    reg->initialised = true;
  }
}

TestSuite *ctest_registry_get_or_add_suite(CTestRegistry *reg,
                                           const char *suite_name) {
  if (reg == NULL)
    return NULL;
  ctest_registry_init(reg);

  for (size_t i = 0; i < reg->suites.count; i++) {
    TestSuite *ts = (TestSuite *)vector_get(&reg->suites, i);
    if (ts->suite_name != NULL && strcmp(ts->suite_name, suite_name) == 0)
      return ts;
  }

  TestSuite new_suite = {.suite_name = suite_name,
                         .setup_suite_fn = NULL,
                         .teardown_suite_fn = NULL,
                         .setup_fn = NULL,
                         .teardown_fn = NULL};

  if (vector_init(&new_suite.cases, sizeof(TestCase)) != 0) {
    fprintf(stderr,
            "ctest: registry: failed to initialise Vector for test suite %s\n",
            suite_name);
    return NULL;
  }

  if (vector_push(&reg->suites, &new_suite) != 0) {
    fprintf(stderr, "ctest: registry: failed to push %s to the suite Vector",
            suite_name);
    vector_free(&new_suite.cases);
    return NULL;
  }

  return (TestSuite *)vector_get(&reg->suites, reg->suites.count - 1);
}

void ctest_registry_add(CTestRegistry *reg, const char *suite_name,
                        const char *test_name, CTestFn fn) {
  TestSuite *suite = ctest_registry_get_or_add_suite(reg, suite_name);
  if (suite == NULL)
    return;

  TestCase tc = {.test_name = test_name, .fn = fn};

  if (vector_push(&suite->cases, &tc) != 0) {
    fprintf(stderr,
            "ctest: registry: failed to register test case %s to suite %s\n",
            test_name, suite_name);
  }
}

void ctest_registry_clear(CTestRegistry *reg) {
  if (reg == NULL || reg->initialised == false)
    return;

  for (size_t i = 0; i < reg->suites.count; i++) {
    TestSuite *ts = (TestSuite *)vector_get(&reg->suites, i);
    vector_free(&ts->cases);
    memset(&ts->cases, 0, sizeof(Vector));
  }

  vector_free(&reg->suites);
  memset(&reg->suites, 0, sizeof(Vector));
  reg->initialised = false;
}

void ctest_registry_set_setup_suite(CTestRegistry *reg, const char *suite,
                                    CTestFn fn) {
  TestSuite *ts = ctest_registry_get_or_add_suite(reg, suite);
  if (ts != NULL)
    ts->setup_suite_fn = fn;
}

void ctest_registry_set_teardown_suite(CTestRegistry *reg, const char *suite,
                                       CTestFn fn) {
  TestSuite *ts = ctest_registry_get_or_add_suite(reg, suite);
  if (ts != NULL)
    ts->teardown_suite_fn = fn;
}

void ctest_registry_set_setup(CTestRegistry *reg, const char *suite,
                              CTestFn fn) {
  TestSuite *ts = ctest_registry_get_or_add_suite(reg, suite);
  if (ts != NULL)
    ts->setup_fn = fn;
}

void ctest_registry_set_teardown(CTestRegistry *reg, const char *suite,
                                 CTestFn fn) {
  TestSuite *ts = ctest_registry_get_or_add_suite(reg, suite);
  if (ts != NULL)
    ts->teardown_fn = fn;
}

CTestRegistry *ctest_get_global_registry(void) {
  static CTestRegistry global_reg = {0};
  return &global_reg;
}