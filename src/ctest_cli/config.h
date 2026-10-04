#ifndef CTEST_CONFIG_H
#define CTEST_CONFIG_H

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_TARGET_DIR "./tests"
#define CONFIG_FIELD_LEN 1024

typedef enum {
  CTEST_VERBOSITY_NORMAL,
  CTEST_VERBOSITY_QUIET,
  CTEST_VERBOSITY_VERBOSE
} CTestVerbosity;

typedef struct {
  char target_dir[CONFIG_FIELD_LEN];
  char filter_pattern[CONFIG_FIELD_LEN];
  char json_output_path[CONFIG_FIELD_LEN];
  CTestVerbosity verbosity;
  unsigned int timeout_sec;
  unsigned int jobs;
} CTestConfig;

void ctest_config_init(CTestConfig *config);
int ctest_config_parse(int argc, char *argv[], CTestConfig *config);
void ctest_config_print_usage(const char *exec_name, FILE *stream);
void ctest_config_load_file(CTestConfig *config, const char *file_path);

#endif
