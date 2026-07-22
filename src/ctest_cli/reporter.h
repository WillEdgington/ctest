#ifndef CTEST_REPORTER_H
#define CTEST_REPORTER_H

#include "executor.h"
#include "session.h"
#include <clib/vector.h>

void ctest_report_start_banner(const char *root_dir);
void ctest_report_suite_metrics(const char *binary_path,
                                const SuiteMetrics *metrics);
void ctest_report_ledger(const Vector *ledger);
void ctest_report_summary(const SessionMetrics *session);

#endif
