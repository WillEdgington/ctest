#ifndef CTEST_CACHE_H
#define CTEST_CACHE_H

#include "config.h"
#include "executor.h"
#include "session.h"
#include <clib/vector.h>

#define CTEST_CACHE_PATH ".ctest_cache"

int ctest_cache_load(const char *cache_file_path);
void ctest_cache_check_and_bypass(Vector *test_bins, SessionMetrics *session,
                                  CTestVerbosity verbosity);
int ctest_cache_update(const char *bin_path, unsigned long mtime,
                       const SuiteMetrics *metrics);
int ctest_cache_save_and_free(const char *cache_file_path);

#endif
