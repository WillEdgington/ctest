#ifndef CTEST_CONFIG_H
#define CTEST_CONFIG_H

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_TARGET_DIR "./tests"

typedef struct {
  const char *target_dir;
  const char *filter_pattern;
  const char *json_output_path;
  int verbose;
  unsigned int timeout_sec;
  size_t jobs;
} CTestConfig;

void ctest_config_init(CTestConfig *config);
int ctest_config_parse(int argc, char *argv[], CTestConfig *config);
void ctest_config_print_usage(const char *exec_name, FILE *stream);

#endif
