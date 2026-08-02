#include "runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  const char *target_dir = "./tests";

  if (argc == 2) {
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
      fprintf(stderr, "Usage: %s [test_directory]\n", argv[0]);
      return 2;
    }
    target_dir = argv[1];
  } else if (argc > 2) {
    fprintf(stderr, "ctest: invalid arguments\n");
    fprintf(stderr, "Usage: %s [test_directory]\n", argv[0]);
    return 2;
  }

  int status = ctest_run_session(target_dir);

  if (status == -1) {
    fprintf(stderr, "ctest: error executing test session target '%s'\n",
            target_dir);
    return 2;
  }

  return (status == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
