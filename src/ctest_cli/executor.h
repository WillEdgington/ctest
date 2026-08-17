#ifndef CTEST_EXECUTOR_H
#define CTEST_EXECUTOR_H

#include "config.h"
#include <clib/vector.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

#define CTEST_MAX_LINE_LEN 512

typedef enum { SUITE_DEFAULT, SUITE_CRASH, SUITE_TIMEOUT } SuiteState;

typedef struct {
  SuiteState state;
  size_t total_runs;
  size_t total_failures;
} SuiteMetrics;

typedef struct {
  pid_t pid;
  int read_fd;
  struct timespec start_time;
  const char *bin_path;
  char line_buf[CTEST_MAX_LINE_LEN];
  size_t buf_pos;
  int is_active;
  int timed_out;
} WorkerSlot;

int ctest_launch_suite(const char *binary_path, CTestVerbosity verbosity,
                       WorkerSlot *slot);

int ctest_harvest_output(WorkerSlot *slot, CTestVerbosity verbosity,
                         SuiteMetrics *metrics, Vector *failure_ledger);

int ctest_finalise_suite(WorkerSlot *slot, unsigned int timeout_sec,
                         CTestVerbosity verbosity, SuiteMetrics *out_metrics,
                         Vector *failure_ledger);

SuiteMetrics ctest_execute_suite(const char *binary_path,
                                 unsigned int timeout_sec,
                                 CTestVerbosity verbosity,
                                 Vector *failure_ledger);

#endif
