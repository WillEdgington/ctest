#ifndef CTEST_DISCOVERY_H
#define CTEST_DISCOVERY_H

#include <clib/vector.h>

#define DISCOVERY_MAX_PATH_LEN 256

Vector *ctest_discover_tests(const char *root_dir);

#endif
