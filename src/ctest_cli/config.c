#define _GNU_SOURCE
#include "config.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                       {"verbose", no_argument, 0, 'v'},
                                       {"quiet", no_argument, 0, 'q'},
                                       {"filter", required_argument, 0, 'f'},
                                       {"timeout", required_argument, 0, 't'},
                                       {"jobs", required_argument, 0, 'j'},
                                       {"json", required_argument, 0, 1000},
                                       {0, 0, 0, 0}};

static void assign_jobs(CTestConfig *config, const char *val) {
  char *end;
  unsigned long jobs = strtoul(val, &end, 10);

  if (*end == '\0' && jobs <= 0xffffffff && val[0] != '-') {
    config->jobs = (unsigned int)jobs;
  }
}

static void assign_timeout_sec(CTestConfig *config, const char *val) {
  char *end;
  unsigned long timeout_sec = strtoul(val, &end, 10);

  if (*end == '\0' && timeout_sec <= 0xffffffff && val[0] != '-') {
    config->timeout_sec = (unsigned int)timeout_sec;
  }
}

static void assign_target_dir(CTestConfig *config, const char *val) {
  snprintf(config->target_dir, sizeof(config->target_dir), "%s", val);
}

static void assign_filter_pattern(CTestConfig *config, const char *val) {
  snprintf(config->filter_pattern, sizeof(config->filter_pattern), "%s", val);
}

static void assign_json_output_path(CTestConfig *config, const char *val) {
  snprintf(config->json_output_path, sizeof(config->json_output_path), "%s",
           val);
}

static void assign_verbosity(CTestConfig *config, const char *val) {
  if (strcmp(val, "normal") == 0 || strcmp(val, "NORMAL") == 0 ||
      strcmp(val, "n") == 0 || strcmp(val, "N") == 0) {
    config->verbosity = CTEST_VERBOSITY_NORMAL;
  } else if (strcmp(val, "quiet") == 0 || strcmp(val, "QUIET") == 0 ||
             strcmp(val, "q") == 0 || strcmp(val, "Q") == 0) {
    config->verbosity = CTEST_VERBOSITY_QUIET;
  } else if (strcmp(val, "verbose") == 0 || strcmp(val, "VERBOSE") == 0 ||
             strcmp(val, "v") == 0 || strcmp(val, "V") == 0) {
    config->verbosity = CTEST_VERBOSITY_VERBOSE;
  }
}

void ctest_config_init(CTestConfig *config) {
  if (config == NULL)
    return;
  config->timeout_sec = 0;
  config->jobs = 1;
  config->verbosity = CTEST_VERBOSITY_NORMAL;
  config->filter_pattern[0] = '\0';
  config->json_output_path[0] = '\0';

  snprintf(config->target_dir, sizeof(config->target_dir), DEFAULT_TARGET_DIR);
}

int ctest_config_parse(int argc, char *argv[], CTestConfig *config) {
  if (config == NULL)
    return -1;

  optind = 1;
  opterr = 0;
  int opt;

  while ((opt = getopt_long(argc, argv, "hvqf:t:j:", long_options, NULL)) !=
         -1) {
    switch (opt) {
    case 'h':
      ctest_config_print_usage(argv[0], stdout);
      return 1;
    case 'v':
      config->verbosity = CTEST_VERBOSITY_VERBOSE;
      break;
    case 'q':
      config->verbosity = CTEST_VERBOSITY_QUIET;
      break;
    case 'f':
      assign_filter_pattern(config, optarg);
      break;
    case 't':
      assign_timeout_sec(config, optarg);
      break;
    case 'j':
      assign_jobs(config, optarg);
      break;
    case 1000:
      assign_json_output_path(config, optarg);
      break;
    case '?':
    default:
      fprintf(stderr, "ctest: invalid option or missing argument\n");
      ctest_config_print_usage(argv[0], stderr);
      return -1;
    }
  }

  int positional_count = argc - optind;
  if (positional_count == 1) {
    snprintf(config->target_dir, sizeof(config->target_dir), "%s",
             argv[optind]);
  } else if (positional_count > 1) {
    fprintf(stderr, "ctest: unexpected extra positional argument '%s'\n",
            argv[optind + 1]);
    ctest_config_print_usage(argv[0], stderr);
    return -1;
  }

  return 0;
}

void ctest_config_print_usage(const char *exec_name, FILE *stream) {
  const char *prog = (exec_name && exec_name[0] != '\0') ? exec_name : "ctest";

  fprintf(stream, "Usage: %s [options] [test_directory]\n\n", prog);
  fprintf(stream, "Positional Arguments:\n");
  fprintf(stream, "  test_directory          Path to directory containing test "
                  "binaries (default: ./tests)\n\n");
  fprintf(stream, "Options:\n");
  fprintf(stream,
          "  -h, --help              Show this help message and exit\n");
  fprintf(
      stream,
      "  -v, --verbose           Print detailed execution output per suite\n");
  fprintf(stream, "  -q, --quiet             Suppress per-suite progress; show "
                  "summary only\n");
  fprintf(
      stream,
      "  -f, --filter <pattern>  Run only test suites matching <pattern>\n");
  fprintf(stream, "  -t, --timeout <sec>     Execution timeout per test suite "
                  "in seconds\n");
  fprintf(stream,
          "  -j, --jobs <N>          Number of concurrent suite workers\n");
  fprintf(stream, "      --json <file>       Export metrics and failure ledger "
                  "to JSON file\n");
}

static void strip_comment(char *line) {
  if (!line)
    return;
  for (char *p = line; *p != '\0'; p++) {
    if (*p == '#' || *p == ';') {
      *p = '\0';
      break;
    }
  }
}

static char *trim_whitespace(char *str) {
  if (!str)
    return NULL;

  while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') {
    str++;
  }

  if (*str == '\0') {
    return str;
  }

  char *end = str + strlen(str) - 1;
  while (end > str &&
         (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
    *end = '\0';
    end--;
  }

  return str;
}

static void process_kv_pair(CTestConfig *config, const char *key,
                            const char *val) {
  if (strcmp(key, "jobs") == 0) {
    assign_jobs(config, val);
  } else if (strcmp(key, "timeout") == 0) {
    assign_timeout_sec(config, val);
  } else if (strcmp(key, "target_dir") == 0) {
    assign_target_dir(config, val);
  } else if (strcmp(key, "verbosity") == 0) {
    assign_verbosity(config, val);
  } else if (strcmp(key, "filter") == 0) {
    assign_filter_pattern(config, val);
  } else if (strcmp(key, "json") == 0) {
    assign_json_output_path(config, val);
  }
}

void ctest_config_load_file(CTestConfig *config, const char *file_path) {
  if (config == NULL || file_path == NULL) {
    return;
  }

  FILE *f = fopen(file_path, "rb");
  if (f == NULL) {
    return;
  }

  fseek(f, 0, SEEK_END);
  long f_size = ftell(f);
  if (f_size <= 0) {
    fclose(f);
    return;
  }
  rewind(f);

  char *buffer = malloc((size_t)f_size + 1);
  if (buffer == NULL) {
    fclose(f);
    return;
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

    strip_comment(line);

    char *trimmed = trim_whitespace(line);
    if (*trimmed != '\0') {
      char *eq = strchr(trimmed, '=');
      if (eq != NULL) {
        *eq = '\0';
        char *key = trim_whitespace(trimmed);
        char *val = trim_whitespace(eq + 1);

        if (*key != '\0' && *val != '\0') {
          process_kv_pair(config, key, val);
        }
      }
    }

    line = next_line;
  }

  free(buffer);
}
