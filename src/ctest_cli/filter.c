#include "filter.h"
#include <fnmatch.h>
#include <string.h>

int ctest_filter_matches(const char *target_path, const char *pattern) {
  if (pattern == NULL || *pattern == '\0')
    return 1;

  if (strpbrk(pattern, "*?[") != NULL)
    return fnmatch(pattern, target_path, 0) == 0 ? 1 : 0;
  return strstr(target_path, pattern) != NULL ? 1 : 0;
}
