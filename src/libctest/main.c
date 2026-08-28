#include "registry.h"
#include <ctest/ctest.h>
#include <stddef.h>

__attribute__((weak)) int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  CTestRegistry *reg = ctest_get_global_registry();
  if (reg != NULL && reg->initialised) {
    for (size_t i = 0; i < reg->cases.count; i++) {
      const TestCase *tc = ctest_registry_get(reg, i);
      if (tc != NULL && tc->fn != NULL)
        tc->fn();
    }
  }

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
