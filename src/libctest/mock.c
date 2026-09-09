#define _GNU_SOURCE
#include "mock.h"
#include <clib/iter.h>
#include <clib/vector.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int max_path_len = 256;

static Vector mock_vec = {0};

// Internal mock stack helpers

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
    fprintf(stderr, "ctest: mock: unable to duplicate path of mock entry\n");
    return -1;
  }

  entry->path = path_dup;
  entry->type = type;
  return 0;
}

static bool ctest_mock_path_exists(const char *path) {
  // F_OK: test for files existence
  if (path == NULL || access(path, F_OK) != 0)
    return false;
  return true;
}

static int ctest_mock_push(const char *path, CTestMockType type) {
  if (path == NULL || *path == '\0' || ctest_mock_path_exists(path))
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

static const CTestMockEntry *ctest_mock_get(size_t index) {
  return (const CTestMockEntry *)vector_get(&mock_vec, index);
}

static int ctest_mock_remove_by_path(const char *path) {
  if (path == NULL || mock_vec.items == NULL || mock_vec.count == 0)
    return -1;

  for (size_t i = 0; i < mock_vec.count; i++) {
    CTestMockEntry *entry = (CTestMockEntry *)vector_get(&mock_vec, i);
    if (entry != NULL && entry->path != NULL &&
        strcmp(entry->path, path) == 0) {
      free(entry->path);
      vector_remove(&mock_vec, i, NULL);
      return 0;
    }
  }

  // path not found
  return -1;
}

static int teardown_dir(const char *path) {
  if (rmdir(path) != 0 && errno != ENOENT)
    return -1;
  return 0;
}

static int teardown_file(const char *path) {
  if (unlink(path) != 0 && errno != ENOENT)
    return -1;
  return 0;
}

static int teardown_binary(const char *path) {
  char src_path[max_path_len];
  snprintf(src_path, max_path_len, "%s.c", path);

  int res_src = 0;
  if (unlink(src_path) != 0 && errno != ENOENT) {
    res_src = -1;
  }

  int res_bin = 0;
  if (unlink(path) != 0 && errno != ENOENT) {
    res_bin = -1;
  }

  return (res_src == 0 && res_bin == 0) ? 0 : -1;
}

static int teardown_mock(CTestMockEntry *entry) {
  switch (entry->type) {
  case CTEST_MOCK_DIR:
    return teardown_dir(entry->path);
  case CTEST_MOCK_FILE:
    return teardown_file(entry->path);
  case CTEST_MOCK_BIN:
    return teardown_binary(entry->path);
  default:
    return -1;
  }
}

static bool has_file_suffix(const char *path) {
  if (*path == '\0')
    return false;

  const char *last_slash = strrchr(path, '/');
  const char *filename = (last_slash != NULL) ? last_slash + 1 : path;

  if (filename[0] == '.') {
    if (filename[1] == '\0' || (filename[1] == '.' && filename[2] == '\0'))
      return false;
    filename++;
  }

  return strchr(filename, '.') != NULL;
}

// Public mock API implementation

int ctest_setup_mock_dir(const char *path) {
  if (ctest_mock_push(path, CTEST_MOCK_DIR) != 0)
    return -1;

  if (mkdir(path, MKDIR_MODE_FLAGS) != 0) {
    ctest_mock_remove_by_path(path);
    return -1;
  }
  return 0;
}

int ctest_teardown_mock_dir(const char *path) {
  if (path == NULL || mock_vec.count == 0)
    return -1;

  for (size_t i = mock_vec.count; i > 0; i--) {
    const CTestMockEntry *entry = ctest_mock_get(i - 1);
    if (entry->type == CTEST_MOCK_DIR && strcmp(entry->path, path) == 0) {
      if (teardown_dir(path) != 0)
        return -1;

      free(entry->path);
      vector_remove(&mock_vec, i - 1, NULL);
      return 0;
    }
  }
  return -1; // not found
}

int ctest_setup_mock_file(const char *path, const char *content) {
  if (ctest_mock_push(path, CTEST_MOCK_FILE) != 0)
    return -1;

  FILE *f = fopen(path, "w");
  if (f == NULL) {
    ctest_mock_remove_by_path(path);
    return -1;
  }

  if (content != NULL)
    fprintf(f, "%s", content);
  fclose(f);
  return 0;
}

int ctest_teardown_mock_file(const char *path) {
  if (path == NULL || mock_vec.count == 0)
    return -1;

  for (size_t i = mock_vec.count; i > 0; i--) {
    const CTestMockEntry *entry = ctest_mock_get(i - 1);
    if (entry->type == CTEST_MOCK_FILE && strcmp(entry->path, path) == 0) {
      if (teardown_file(path) != 0)
        return -1;

      free(entry->path);
      vector_remove(&mock_vec, i - 1, NULL);
      return 0;
    }
  }
  return -1; // not found
}

int ctest_setup_mock_binary(const char *path, const char *c_code) {
  if (c_code == NULL || ctest_mock_push(path, CTEST_MOCK_BIN) != 0)
    return -1;

  if (has_file_suffix(path)) {
    ctest_mock_remove_by_path(path);
    return -1;
  }

  char src_path[max_path_len];
  snprintf(src_path, max_path_len, "%s.c", path);

  FILE *f = fopen(src_path, "w");
  if (f == NULL) {
    ctest_mock_remove_by_path(path);
    return -1;
  }

  fputs(c_code, f);
  fclose(f);

  char cmd[max_path_len << 1];

  // 2>/dev/null: redirects compiler error stream to /dev/null (mutes)
  snprintf(cmd, sizeof(cmd), "gcc -o %s %s 2>/dev/null", path, src_path);
  if (system(cmd) != 0) {
    ctest_mock_remove_by_path(path);
    unlink(src_path);
    return -1;
  }

  chmod(path, BINARY_F_MODE_FLAGS);
  return 0;
}

int ctest_teardown_mock_binary(const char *path) {
  if (path == NULL || mock_vec.count == 0)
    return -1;

  for (size_t i = mock_vec.count; i > 0; i--) {
    const CTestMockEntry *entry = ctest_mock_get(i - 1);
    if (entry->type == CTEST_MOCK_BIN && strcmp(entry->path, path) == 0) {
      if (teardown_binary(path) != 0)
        return -1;
      free(entry->path);
      vector_remove(&mock_vec, i - 1, NULL);
      return 0;
    }
  }

  return -1; // not found
}

int ctest_teardown_all_mocks(void) {
  CTestMockEntry entry = {0};
  int status = 0;
  while (vector_pop(&mock_vec, &entry) == 0) {
    if (teardown_mock(&entry) != 0)
      status = -1;

    free(entry.path);
  }

  vector_free(&mock_vec);

  return status;
}
