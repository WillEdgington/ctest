#define _GNU_SOURCE
#include "mock.h"
#include <clib/iter.h>
#include <clib/vector.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Vector mock_vec = {0};

static int mock_vec_init() {
  if (vector_init(&mock_vec, sizeof(CTestMockEntry)) != 0) {
    fprintf(stderr,
            "ctest: mock: unable to initialise Vector for mock entries\n");
    return -1;
  }
  return 0;
}

static int create_mock_entry(CTestMockEntry *entry, const char *path,
                             CTestMockType type) {
  char *path_dup = strdup(path);
  if (path_dup == NULL) {
    fprintf(stderr, "ctest: mock: unable to duplicate path of mock entry");
    return -1;
  }

  entry->path = path_dup;
  entry->type = type;
  return 0;
}

bool ctest_mock_path_exists(const char *path) {
  // F_OK: test for files existence
  if (path == NULL || access(path, F_OK) != 0)
    return false;
  return true;
}

int ctest_mock_push(const char *path, CTestMockType type) {
  if (path == NULL || ctest_mock_path_exists(path))
    return -1;

  if (mock_vec.items == NULL) {
    if (mock_vec_init() != 0)
      return -1;
  }

  CTestMockEntry entry;
  if (create_mock_entry(&entry, path, type) != 0)
    return -1;

  if (vector_push(&mock_vec, &entry) != 0) {
    fprintf(stderr,
            "ctest: mock: could not push entry to vector for mock entries\n");
    return -1;
  }

  return 0;
}

const CTestMockEntry *ctest_mock_get(size_t index) {
  return (const CTestMockEntry *)vector_get(&mock_vec, index);
}

int ctest_mock_pop(CTestMockEntry *out_entry) {
  if (out_entry == NULL)
    return -1;

  return vector_pop(&mock_vec, (void *)out_entry);
}

int ctest_mock_remove_by_path(const char *path) {
  if (path == NULL || mock_vec.items == NULL || mock_vec.count == 0)
    return -1;

  for (size_t i = 0; i < mock_vec.count; i++) {
    CTestMockEntry *entry = (CTestMockEntry *)vector_get(&mock_vec, i);
    if (entry != NULL && entry->path != NULL &&
        strcmp(entry->path, path) == 0) {
      free(entry->path); // free saved path

      // move everything down
      size_t remaining = mock_vec.count - 1 - i;
      if (remaining > 0) {
        void *dst = vector_get(&mock_vec, i);
        void *src = vector_get(&mock_vec, i + 1);
        memmove(dst, src, remaining * mock_vec.item_size);
      }

      mock_vec.count--;
      return 0; // found path and removed entry
    }
  }

  // path not found
  return -1;
}

void ctest_mock_clear(void) {
  if (mock_vec.items == NULL)
    return;

  Iter it = vector_iter(&mock_vec);
  while (it.next(&it) == 0) {
    CTestMockEntry *entry = (CTestMockEntry *)it.current.value;
    free(entry->path);
  }

  vector_free(&mock_vec);
}

size_t ctest_mock_count(void) { return mock_vec.count; }
