#ifndef CTEST_EXECUTOR_H
#define CTEST_EXECUTOR_H

#include <clib/vector.h>

#define CTEST_MAX_LINE_LEN 512

typedef struct {
  size_t total_runs;
  size_t total_failures;
  int crashed;
} SuiteMetrics;

SuiteMetrics ctest_execute_suite(const char *binary_path,
                                 Vector *failure_ledger);

#endif