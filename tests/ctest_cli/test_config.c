#include "ctest_cli/config.h"
#include <ctest/ctest.h>

#define SUITE_NAME test_config

CTEST(SUITE_NAME, test_config_init_defaults) {
  CTestConfig config;
  ctest_config_init(&config);

  ASSERT_STR_EQ(config.target_dir, DEFAULT_TARGET_DIR,
                "Default target directory configuration should be './tests'");
  ASSERT_PTR_NULL(config.filter_pattern,
                  "Default filter pattern configuration should be NULL");
  ASSERT_INT_EQ(config.timeout_sec, 0,
                "Default timeout configuration should be 0 (no timeout)");
  ASSERT_INT_EQ(config.verbosity, CTEST_VERBOSITY_NORMAL,
                "Default verbosity level configuration should be normal "
                "(CTEST_VERBOSITY_NORMAL)");
  ASSERT_INT_EQ(config.jobs, 1, "Default jobs configuration should be 1");
  ASSERT_PTR_NULL(config.json_output_path,
                  "Default json output path configuration should be NULL");
}

CTEST(SUITE_NAME, test_parse_default_args) {
  CTestConfig config;
  ctest_config_init(&config);

  char *argv[] = {"ctest", NULL};
  int argc = 1;

  ASSERT_INT_EQ(
      ctest_config_parse(argc, argv, &config), 0,
      "Command with no added args (just 'ctest') should parse error free");
  ASSERT_STR_EQ(config.target_dir, DEFAULT_TARGET_DIR,
                "Command with no added args should have default target "
                "directory ('./tests') configurations");
}

CTEST(SUITE_NAME, test_parse_custom_directory) {
  CTestConfig config;
  ctest_config_init(&config);

  char *argv[] = {"ctest", "custom/path", NULL};
  int argc = 2;

  ASSERT_INT_EQ(ctest_config_parse(argc, argv, &config), 0,
                "Command with directory arg should parse error free");
  ASSERT_STR_EQ(config.target_dir, argv[1],
                "Parsed custom directory should be assigned to target "
                "directory configuration");
}

CTEST(SUITE_NAME, test_parse_valid_flags) {
  CTestConfig config;
  ctest_config_init(&config);

  char *argv[] = {"ctest", "-v", "-f",     "suite_name",  "-t",         "30",
                  "-j",    "4",  "--json", "report.json", "tests/unit", NULL};
  int argc = 10;

  ASSERT_INT_EQ(
      ctest_config_parse(argc, argv, &config), 0,
      "Command populated by multiple valid flags should parse error free");
  ASSERT_INT_EQ(config.verbosity, CTEST_VERBOSITY_VERBOSE,
                "Parsed verbose flag ('-v') should set verbosity level to "
                "verbose (CTEST_VERBOSITY_VERBOSE)");
  ASSERT_STR_EQ(config.filter_pattern, "suite_name",
                "Parsed filter pattern value ('-f <pattern>') should be "
                "assigned to filter pattern configuration");
  ASSERT_INT_EQ(config.timeout_sec, 30,
                "Parsed timeout value ('-t <sec>') should be assigned to the "
                "timeout configuration");
  ASSERT_INT_EQ(
      config.jobs, 4,
      "Parsed jobs value ('-j <jobs>') should assign to job configuration");
  ASSERT_STR_EQ(config.json_output_path, "report.json",
                "Parsed json output path ('--json <path>') should assign to "
                "json output path configuration");

  ctest_config_init(&config);

  char *argv_q[] = {"ctest", "-q"};
  int argc_q = 2;

  ctest_config_parse(argc_q, argv_q, &config);

  ASSERT_INT_EQ(config.verbosity, CTEST_VERBOSITY_QUIET,
                "Parsed quite flag ('-q') should set verbosity level to quiet "
                "(CTEST_VERBOSITY_QUIET)");
}

CTEST(SUITE_NAME, test_parse_help_flag) {
  CTestConfig config;
  ctest_config_init(&config);

  char *argv[] = {"ctest", "-h", NULL};
  int argc = 2;

  int saved_stdout = ctest_mute_output(STDOUT_FILENO);
  int status = ctest_config_parse(argc, argv, &config);
  ctest_unmute_output(saved_stdout, STDOUT_FILENO);

  ASSERT_INT_EQ(
      status, 1,
      "Parsed help flag should signal that help was requested (status 1)");
}

CTEST(SUITE_NAME, test_parse_invalid_flag) {
  CTestConfig config;
  ctest_config_init(&config);

  char *argv[] = {"ctest", "--unknown-flag", NULL};
  int argc = 2;

  int saved_stderr = ctest_mute_output(STDERR_FILENO);
  int status = ctest_config_parse(argc, argv, &config);
  ctest_unmute_output(saved_stderr, STDERR_FILENO);

  ASSERT_INT_EQ(status, -1, "Parsed invalid flag should error (return -1)");
}

CTEST(SUITE_NAME, test_parse_extra_positional_args) {
  CTestConfig config;
  ctest_config_init(&config);

  char *argv[] = {"ctest", "dir1", "dir2", NULL};
  int argc = 3;

  int saved_stderr = ctest_mute_output(STDERR_FILENO);
  int status = ctest_config_parse(argc, argv, &config);
  ctest_unmute_output(saved_stderr, STDERR_FILENO);

  ASSERT_INT_EQ(status, -1,
                "Parsed extra positional arg should error (return -1)");
}

CTEST(SUITE_NAME, test_parse_missing_option_arg) {
  CTestConfig config;
  ctest_config_init(&config);

  char *argv[] = {"ctest", "-f", NULL};
  int argc = 2;

  int saved_stderr = ctest_mute_output(STDERR_FILENO);
  int status = ctest_config_parse(argc, argv, &config);
  ctest_unmute_output(saved_stderr, STDERR_FILENO);

  ASSERT_INT_EQ(status, -1,
                "Parsed flag with missing value should error (return -1)");
}
