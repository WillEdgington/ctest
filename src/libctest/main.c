#include "registry.h"
#include <ctest/ctest.h>
#include <stddef.h>

__attribute__((weak)) int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  CTestRegistry *reg = ctest_get_global_registry();
  if (reg != NULL && reg->initialised) {
    for (size_t i = 0; i < reg->suites.count; i++) {
      TestSuite *suite = (TestSuite *)vector_get(&reg->suites, i);
      if (suite == NULL)
        continue;

      if (suite->setup_suite_fn != NULL)
        suite->setup_suite_fn();

      for (size_t j = 0; j < suite->cases.count; j++) {
        TestCase *tc = (TestCase *)vector_get(&suite->cases, j);
        if (tc == NULL || tc->fn == NULL)
          continue;

        if (suite->setup_fn != NULL)
          suite->setup_fn();

        tc->fn();

        if (suite->teardown_fn != NULL)
          suite->teardown_fn();
      }

      if (suite->teardown_suite_fn != NULL)
        suite->teardown_suite_fn();
    }
  }

  ctest_summary();
  return ctest_fail_count == 0 ? 0 : 1;
}
