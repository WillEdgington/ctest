#include "config.h"
#include "runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  CTestConfig config;
  ctest_config_init(&config);

  int status = ctest_config_parse(argc, argv, &config);
  if (status != 0)
    return status == -1 ? 2 : 0;

  status = ctest_run_session(config.target_dir);

  if (status == -1) {
    fprintf(stderr, "ctest: error executing test session target '%s'\n",
            config.target_dir);
    return 2;
  }

  return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
