#include "registry.h"
#include <clib/vector.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void ctest_registry_init(CTestRegistry *reg) {
  if (reg != NULL && reg->initialised == false) {
    if (vector_init(&reg->cases, sizeof(TestCase)) != 0) {
      fprintf(
          stderr,
          "ctest: registry: failed to initialise Vector for CTestRegistry\n");
    }
    reg->initialised = true;
  }
}

void ctest_registry_add(CTestRegistry *reg, const char *suite_name,
                        const char *test_name, CTestFn fn) {
  if (reg == NULL)
    return;

  ctest_registry_init(reg);

  TestCase testcase = {
      .suite_name = suite_name, .test_name = test_name, .fn = fn};

  if (vector_push(&reg->cases, (const void *)&testcase) != 0)
    fprintf(stderr,
            "ctest: registry: failed to add to Vector in CTestRegistry\n");
}

void ctest_registry_clear(CTestRegistry *reg) {
  if (reg != NULL && reg->initialised) {
    vector_free(&reg->cases);
    memset(&reg->cases, 0, sizeof(Vector));
    reg->initialised = false;
  }
}

const TestCase *ctest_registry_get(const CTestRegistry *reg, size_t index) {
  if (reg == NULL || !reg->initialised)
    return NULL;
  return (const TestCase *)vector_get(&reg->cases, index);
}

CTestRegistry *ctest_get_global_registry(void) {
  static CTestRegistry global_reg = {0};
  return &global_reg;
}
