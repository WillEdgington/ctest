#ifndef CTEST_REGISTRY_H
#define CTEST_REGISTRY_H

#include <clib/vector.h>
#include <stdbool.h>
#include <stddef.h>

typedef void (*CTestFn)(void);

typedef struct {
  const char *suite_name;
  const char *test_name;
  CTestFn fn;
} TestCase;

typedef struct {
  Vector cases;
  bool initialised;
} CTestRegistry;

void ctest_registry_init(CTestRegistry *reg);
void ctest_registry_add(CTestRegistry *reg, const char *suite_name,
                        const char *test_name, CTestFn fn);
void ctest_registry_clear(CTestRegistry *reg);
const TestCase *ctest_registry_get(const CTestRegistry *reg, size_t index);

CTestRegistry *ctest_get_global_registry(void);

#endif
