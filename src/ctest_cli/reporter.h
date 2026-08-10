#ifndef CTEST_REPORTER_H
#define CTEST_REPORTER_H

#include "config.h"
#include "executor.h"
#include "session.h"
#include <clib/vector.h>

void ctest_report_start_banner(const char *root_dir, CTestVerbosity verbosity);
void ctest_report_suite_metrics(const char *binary_path,
                                const SuiteMetrics *metrics,
                                CTestVerbosity verbosity);
void ctest_report_ledger(const Vector *ledger, CTestVerbosity verbosity);
void ctest_report_summary(const SessionMetrics *session,
                          CTestVerbosity verbosity);

#endif
