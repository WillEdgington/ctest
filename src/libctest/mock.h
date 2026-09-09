#ifndef CTEST_MOCK_H
#define CTEST_MOCK_H

#include <ctest/ctest.h>
#include <stdbool.h>
#include <stddef.h>

#define MKDIR_MODE_FLAGS                                                       \
  S_IRWXU | S_IRWXG | S_IRWXO // read/write/execute by owner/group/others
#define BINARY_F_MODE_FLAGS 0755

typedef enum { CTEST_MOCK_FILE, CTEST_MOCK_BIN, CTEST_MOCK_DIR } CTestMockType;

typedef struct {
  char *path;
  CTestMockType type;
} CTestMockEntry;

#endif
