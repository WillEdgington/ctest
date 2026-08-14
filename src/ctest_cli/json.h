#ifndef CTEST_JSON_H
#define CTEST_JSON_H

#include "session.h"
#include <clib/vector.h>

int ctest_json_write(const char *filepath, const SessionMetrics *metrics,
                     const Vector *failure_ledger);

#endif
