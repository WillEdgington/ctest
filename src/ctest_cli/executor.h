#ifndef CTEST_EXECUTOR_H
#define CTEST_EXECUTOR_H

#include <clib/vector.h>

#define CTEST_MAX_LINE_LEN 512

typedef enum { SUITE_DEFAULT, SUITE_CRASH, SUITE_TIMEOUT } SuiteState;

typedef struct {
  SuiteState state;
  size_t total_runs;
  size_t total_failures;
} SuiteMetrics;

SuiteMetrics ctest_execute_suite(const char *binary_path,
                                 unsigned int timeout_sec,
                                 Vector *failure_ledger);

#endif
