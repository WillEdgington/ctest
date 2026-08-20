#ifndef CTEST_POOL_H
#define CTEST_POOL_H

#include "config.h"
#include "session.h"
#include <clib/vector.h>

int ctest_pool_run(const Vector *test_bins, const CTestConfig *config,
                   SessionMetrics *session, Vector *failure_ledger);

#endif
