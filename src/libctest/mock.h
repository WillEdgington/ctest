#ifndef CTEST_MOCK_H
#define CTEST_MOCK_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { CTEST_MOCK_FILE, CTEST_MOCK_BIN, CTEST_MOCK_DIR } CTestMockType;

typedef struct {
  char *path;
  CTestMockType type;
} CTestMockEntry;

bool ctest_mock_path_exists(const char *path);

int ctest_mock_push(const char *path, CTestMockType type);
int ctest_mock_pop(CTestMockEntry *out_entry);
int ctest_mock_remove_by_path(const char *path);
void ctest_mock_clear(void);

size_t ctest_mock_count(void);
const CTestMockEntry *ctest_mock_get(size_t index);

#endif