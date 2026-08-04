#define _GNU_SOURCE
#include "config.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                       {"verbose", no_argument, 0, 'v'},
                                       {"filter", required_argument, 0, 'f'},
                                       {"timeout", required_argument, 0, 't'},
                                       {"jobs", required_argument, 0, 'j'},
                                       {"json", required_argument, 0, 1000},
                                       {0, 0, 0, 0}};

void ctest_config_init(CTestConfig *config) {
  if (config == NULL)
    return;
  config->target_dir = DEFAULT_TARGET_DIR;
  config->filter_pattern = NULL;
  config->timeout_sec = 0;
  config->jobs = 1;
  config->verbose = 0;
  config->json_output_path = NULL;
}

int ctest_config_parse(int argc, char *argv[], CTestConfig *config) {
  if (config == NULL)
    return -1;

  optind = 1;
  opterr = 0;
  int opt;

  while ((opt = getopt_long(argc, argv, "hvf:t:j:", long_options, NULL)) !=
         -1) {
    switch (opt) {
    case 'h':
      ctest_config_print_usage(argv[0], stdout);
      return 1;
    case 'v':
      config->verbose = 1;
      break;
    case 'f':
      config->filter_pattern = optarg;
      break;
    case 't':
      config->timeout_sec = (unsigned int)strtoul(optarg, NULL, 10);
      break;
    case 'j':
      config->jobs = (size_t)strtoul(optarg, NULL, 10);
      if (config->jobs == 0)
        config->jobs = 1;
      break;
    case 1000:
      config->json_output_path = optarg;
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
    config->target_dir = argv[optind];
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
  fprintf(stream, "  -v, --verbose           Enable verbose runner output\n");
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
