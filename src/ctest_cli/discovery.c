#include "discovery.h"
#include <clib/vector.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int traverse_directory(const char *dir, Vector *v) {
  DIR *dir_stream = opendir(dir);
  if (dir_stream == NULL)
    return 0;

  struct dirent *entry;
  struct stat statbuf;
  char p_buf[DISCOVERY_MAX_PATH_LEN];

  while ((entry = readdir(dir_stream)) != NULL) {
    char *e_name = entry->d_name;

    if (strcmp(e_name, "..") == 0 || strcmp(e_name, ".") == 0)
      continue;

    int p_len = snprintf(p_buf, DISCOVERY_MAX_PATH_LEN, "%s/%s", dir, e_name);
    if (p_len >= DISCOVERY_MAX_PATH_LEN || p_len < 0)
      continue;

    if (stat(p_buf, &statbuf) != 0)
      continue;

    if (S_ISREG(statbuf.st_mode)) {
      if ((statbuf.st_mode & S_IXUSR) && strncmp(e_name, "test_", 5) == 0)
        if (vector_push(v, p_buf) == -1) {
          closedir(dir_stream);
          return -1;
        }
    } else if (S_ISDIR(statbuf.st_mode)) {
      if (traverse_directory(p_buf, v) == -1) {
        closedir(dir_stream);
        return -1;
      }
    }
  }

  return closedir(dir_stream);
}

Vector *ctest_discover_tests(const char *root_dir) {
  Vector *v = malloc(sizeof(Vector));
  if (vector_init(v, DISCOVERY_MAX_PATH_LEN) != 0) {
    free(v);
    return NULL;
  }

  if (traverse_directory(root_dir, v) != 0) {
    vector_free(v);
    free(v);
    return NULL;
  }
  return v;
}
