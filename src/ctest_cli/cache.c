#include "cache.h"
#include "config.h"
#include "discovery.h"
#include "executor.h"
#include "reporter.h"
#include "session.h"
#include <clib/arena.h>
#include <clib/hashmap.h> // chaining hashmap (buckets of LL)
#include <clib/iter.h>
#include <clib/vector.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// .ctest_cache line: "binary_path:mtime:tests_passed\n"

static Arena cache_arena = {0};
static HashMap cache_map = {0};

typedef struct {
  unsigned long mtime;
  unsigned int tests_passed;
} CacheVal;

#define CACHE_ARENA_SLAB_SIZE 8192 // 8 KB

// DJB2
static size_t cache_map_hash_fn(const void *key, size_t key_size) {
  (void)key_size;
  const char *str = (const char *)key;
  size_t hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

static int cache_map_compare_fn(const void *k1, const void *k2,
                                size_t key_size) {
  (void)key_size;
  return strcmp((const char *)k1, (const char *)k2);
}

static int cache_map_init(void) {
  if (arena_init(&cache_arena, CACHE_ARENA_SLAB_SIZE) != 0) {
    return -1;
  }

  if (hashmap_init(&cache_map, DISCOVERY_MAX_PATH_LEN, sizeof(CacheVal),
                   &cache_arena) != 0) {
    return -1;
  }

  hashmap_set_functions(&cache_map, cache_map_hash_fn, cache_map_compare_fn);
  return 0;
}

static int cache_map_add(const char *binary_path, unsigned long mtime,
                         unsigned int tests_passed) {
  CacheVal cache_val = {.mtime = mtime, .tests_passed = tests_passed};

  char bin_path_buf[DISCOVERY_MAX_PATH_LEN];
  snprintf(bin_path_buf, sizeof(bin_path_buf), "%s", binary_path);
  bin_path_buf[DISCOVERY_MAX_PATH_LEN - 1] = '\0';

  if (hashmap_put(&cache_map, &bin_path_buf, &cache_val) != 0) {
    return -1;
  }
  return 0;
}

static void cache_free(void) {
  hashmap_free(&cache_map);
  arena_free(&cache_arena);
}

static int strtomtime(const char *mtime_str, unsigned long *mtime) {
  char *end;
  *mtime = strtoul(mtime_str, &end, 10);

  if (*end != '\0') {
    // TODO: report error in parsing mtime
    return -1;
  }
  return 0;
}

static int strtotp(const char *tests_passed_str, unsigned int *tests_passed) {
  char *end;
  unsigned long tp_tmp = strtoul(tests_passed_str, &end, 10);

  if (*end != '\0' || tp_tmp > 0xffffffff || tests_passed_str[0] == '-') {
    // TODO: report error in parsing tests_passed
    return -1;
  }
  *tests_passed = (unsigned int)tp_tmp;
  return 0;
}

static int parse_cached_line(char *line, const char *eol) {
  // create char *binary_path
  char *bin_path = line;
  if (eol != NULL && eol <= bin_path) {
    // TODO: report lack of binary_path record (malformed line)
    return -1;
  }

  // create unsigned long mtime
  char *mtime_ptr = strchr(bin_path, ':');
  if (mtime_ptr == NULL || (eol != NULL && eol <= mtime_ptr + 1)) {
    return -1;
  }
  *mtime_ptr = '\0';
  mtime_ptr++;

  char *tests_passed_ptr = strchr(mtime_ptr, ':');
  if (tests_passed_ptr == NULL ||
      (eol != NULL && eol <= tests_passed_ptr + 1)) {
    return -1;
  }

  *tests_passed_ptr = '\0';
  tests_passed_ptr++;

  // transform/validate
  unsigned long mtime;
  if (strtomtime(mtime_ptr, &mtime) != 0) {
    return -1;
  }

  unsigned int tests_passed;
  if (strtotp(tests_passed_ptr, &tests_passed) != 0) {
    return -1;
  }

  return cache_map_add(bin_path, mtime, tests_passed);
}

int ctest_cache_load(const char *cache_file_path) {
  if (cache_map.buckets == NULL) {
    if (cache_map_init() != 0) {
      return -1;
    }
  }

  FILE *f = fopen(cache_file_path, "rb");
  if (f == NULL) {
    return 0;
  }

  fseek(f, 0, SEEK_END);
  long f_size = ftell(f);
  if (f_size <= 0) {
    fclose(f);
    return f_size == 0 ? 0 : -1;
  }
  rewind(f);

  char *buffer = malloc((size_t)f_size + 1);
  if (buffer == NULL) {
    fclose(f);
    return -1;
  }

  size_t read_bytes = fread(buffer, 1, (size_t)f_size, f);
  buffer[read_bytes] = '\0';
  fclose(f);

  char *line = buffer;
  while (line && *line != '\0') {
    char *next_line = strchr(line, '\n');
    if (next_line) {
      *next_line = '\0';
      next_line++;
    }

    parse_cached_line(line, next_line);
    line = next_line;
  }

  free(buffer);
  return 0;
}

void ctest_cache_check_and_bypass(Vector *test_bins, SessionMetrics *session,
                                  CTestVerbosity verbosity) {
  for (int i = test_bins->count - 1; i >= 0; i--) {
    char *test_bin = (char *)vector_get(test_bins, (size_t)i);
    CacheVal *cache_val = (CacheVal *)hashmap_get(&cache_map, (void *)test_bin);
    if (cache_val != NULL) {
      struct stat st;
      if (stat(test_bin, &st) == 0 &&
          (unsigned long)st.st_mtime == cache_val->mtime) {
        SuiteMetrics metrics = {0};
        metrics.total_runs = cache_val->tests_passed;
        ctest_report_suite_metrics(test_bin, &metrics, verbosity);

        vector_remove(test_bins, (size_t)i, NULL);
        session->total_suites++;
        session->total_runs += cache_val->tests_passed;
      }
    }
  }
}

int ctest_cache_update(const char *bin_path, unsigned long mtime,
                       const SuiteMetrics *metrics) {
  if (cache_map.buckets == NULL) {
    return -1;
  }

  if (metrics->state == SUITE_DEFAULT && metrics->total_failures == 0) {
    // suite runs fine, all passed
    return cache_map_add(bin_path, mtime, metrics->total_runs);
  }

  // fail/timeout/crash
  hashmap_remove(&cache_map, bin_path);
  return 0;
}

int ctest_cache_save_and_free(const char *cache_file_path) {
  FILE *f = fopen(cache_file_path, "w");
  if (f == NULL) {
    cache_free();
    return -1;
  }

  Iter it = hashmap_iter(&cache_map);
  while (it.next(&it) == 0) {
    char *bin_path = (char *)it.current.key;
    CacheVal cache_val = *(CacheVal *)it.current.value;

    fprintf(f, "%s:%lu:%u\n", bin_path, cache_val.mtime,
            cache_val.tests_passed);
  }

  fclose(f);
  cache_free();
  return 0;
}
