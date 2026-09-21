#ifndef CTEST_EXECUTOR_H
#define CTEST_EXECUTOR_H

#include "config.h"
#include <clib/vector.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#define CTEST_MAX_LINE_LEN 512
#define CTEST_MAX_FIELD_LEN 256

typedef enum { TEST_PASS, TEST_FAIL } TestStatus;

typedef struct {
  char expr[CTEST_MAX_FIELD_LEN];
  char msg[CTEST_MAX_FIELD_LEN];
  size_t line_num;
  TestStatus status;
} TestCaseResult;

typedef enum { SUITE_DEFAULT, SUITE_CRASH, SUITE_TIMEOUT } SuiteState;

typedef struct {
  char file_path[CTEST_MAX_FIELD_LEN];
  Vector test_results;
  size_t total_runs;
  size_t total_failures;
  SuiteState state;
} SuiteMetrics;

typedef struct {
  struct timespec start_time;
  char line_buf[CTEST_MAX_LINE_LEN];
  const char *bin_path;
  pid_t pid;
  int read_fd;
  int is_active;
  int timed_out;
  size_t buf_pos;
} WorkerSlot;

int ctest_launch_suite(const char *binary_path, CTestVerbosity verbosity,
                       WorkerSlot *slot);

int ctest_harvest_output(WorkerSlot *slot, CTestVerbosity verbosity,
                         SuiteMetrics *metrics, Vector *failure_ledger);

int ctest_finalise_suite(WorkerSlot *slot, unsigned int timeout_sec,
                         CTestVerbosity verbosity, SuiteMetrics *out_metrics,
                         Vector *failure_ledger);

int ctest_suite_metrics_init(SuiteMetrics *metrics);

void ctest_suite_metrics_cleanup(SuiteMetrics *metrics);

#endif
