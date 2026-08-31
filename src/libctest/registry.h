#ifndef CTEST_REGISTRY_H
#define CTEST_REGISTRY_H

#include <clib/vector.h>
#include <stdbool.h>
#include <stddef.h>

typedef void (*CTestFn)(void);

typedef struct {
  const char *test_name;
  CTestFn fn;
} TestCase;

typedef struct TestSuite {
  Vector cases;
  const char *suite_name;
  CTestFn setup_suite_fn;
  CTestFn teardown_suite_fn;
  CTestFn setup_fn;
  CTestFn teardown_fn;
} TestSuite;

typedef struct CTestRegistry {
  Vector suites;
  bool initialised;
} CTestRegistry;

void ctest_registry_init(CTestRegistry *reg);
void ctest_registry_add(CTestRegistry *reg, const char *suite_name,
                        const char *test_name, CTestFn fn);
void ctest_registry_clear(CTestRegistry *reg);
TestSuite *ctest_registry_get_or_add_suite(CTestRegistry *reg,
                                           const char *suite_name);

void ctest_registry_set_setup_suite(CTestRegistry *reg, const char *suite,
                                    CTestFn fn);
void ctest_registry_set_teardown_suite(CTestRegistry *reg, const char *suite,
                                       CTestFn fn);
void ctest_registry_set_setup(CTestRegistry *reg, const char *suite,
                              CTestFn fn);
void ctest_registry_set_teardown(CTestRegistry *reg, const char *suite,
                                 CTestFn fn);

CTestRegistry *ctest_get_global_registry(void);

#endif
