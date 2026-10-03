#include "ctest_cli/config.h"
#include <ctest/ctest.h>

#define SUITE_NAME test_config

CTEST(SUITE_NAME, test_config_init_defaults) {
  CTestConfig config;
  ctest_config_init(&config);

  ASSERT_STR_EQ(config.target_dir, DEFAULT_TARGET_DIR,
                "Default target directory configuration should be './tests'");
  ASSERT_STR_EQ(config.filter_pattern, "",
                "Default filter pattern configuration should be empty string");
  ASSERT_INT_EQ(config.timeout_sec, 0,
                "Default timeout configuration should be 0 (no timeout)");
  ASSERT_INT_EQ(config.verbosity, CTEST_VERBOSITY_NORMAL,
                "Default verbosity level configuration should be normal "
                "(CTEST_VERBOSITY_NORMAL)");
  ASSERT_INT_EQ(config.jobs, 1, "Default jobs configuration should be 1");
  ASSERT_STR_EQ(
      config.json_output_path, "",
      "Default json output path configuration should be empty string");
}

CTEST(SUITE_NAME, test_config_parse_default_args) {
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

CTEST(SUITE_NAME, test_config_parse_valid_flags) {
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

CTEST(SUITE_NAME, test_config_parse_help_flag) {
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

CTEST(SUITE_NAME, test_config_parse_invalid_flag) {
  CTestConfig config;
  ctest_config_init(&config);

  char *argv[] = {"ctest", "--unknown-flag", NULL};
  int argc = 2;

  int saved_stderr = ctest_mute_output(STDERR_FILENO);
  int status = ctest_config_parse(argc, argv, &config);
  ctest_unmute_output(saved_stderr, STDERR_FILENO);

  ASSERT_INT_EQ(status, -1, "Parsed invalid flag should error (return -1)");
}

CTEST(SUITE_NAME, test_config_parse_extra_positional_args) {
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

CTEST(SUITE_NAME, test_config_parse_missing_option_arg) {
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

// -- ctest_config_load_file() tests --

CTEST(SUITE_NAME, test_config_load_file_missing_file) {
  char *non_existent_path = "./.ctestconfig_non_existent_09035833";
  CTestConfig config = {.target_dir = "./tests",
                        .json_output_path = "./test.json",
                        .verbosity = CTEST_VERBOSITY_QUIET,
                        .timeout_sec = 10,
                        .jobs = 8};

  ctest_config_load_file(&config, non_existent_path);

  ASSERT_STR_EQ(config.target_dir, "./tests",
                "CTestConfig.target_dir stays the same after attempting to "
                "load non-existent config file");
  ASSERT_STR_EQ(config.filter_pattern, "",
                "CTestConfig.filter_pattern stays the same after attempting to "
                "load non-existent config file");
  ASSERT_STR_EQ(config.json_output_path, "./test.json",
                "CTestConfig.json_output_path stays the same after attempting "
                "to load non-existent config file");
  ASSERT_INT_EQ(config.verbosity, CTEST_VERBOSITY_QUIET,
                "CTestConfig.verbosity stays the same after attempting to load "
                "non-existent config file");
  ASSERT_INT_EQ(config.timeout_sec, 10,
                "CTestConfig.timeout_sec stays the same after attempting to "
                "load non-existent config file");
  ASSERT_INT_EQ(config.jobs, 8,
                "CTestConfig.jobs stays the same after attempting to load "
                "non-existent config file");
}

CTEST(SUITE_NAME, test_config_load_file_all_keys_valid) {
  char *config_file = "./.ctestconfig_00935389";
  char *config_content = "jobs = 4\n"
                         "timeout = 10\n"
                         "target_dir = ./custom_tests\n"
                         "verbosity = verbose\n"
                         "filter = *est_co*\n"
                         "json = report.json";

  ctest_setup_mock_file(config_file, config_content);

  CTestConfig config = {0};

  ctest_config_load_file(&config, config_file);

  ASSERT_INT_EQ(
      config.jobs, 4,
      "Parsed \"jobs\" key should assign parsed value to CTestConfig.jobs");
  ASSERT_INT_EQ(config.timeout_sec, 10,
                "Parsed \"timeout\" key should assign parsed value to "
                "CTestConfig.timeout_sec");
  ASSERT_STR_EQ(config.target_dir, "./custom_tests",
                "Parsed \"target_dir\" key should assign parsed value to "
                "CTestConfig.target_dir");
  ASSERT_INT_EQ(config.verbosity, CTEST_VERBOSITY_VERBOSE,
                "Parsed \"verbosity\" key should assign parsed value to "
                "CTestConfig.verbosity");
  ASSERT_STR_EQ(config.filter_pattern, "*est_co*",
                "Parsed \"filter\" key should assign parsed value to "
                "CTestConfig.filter_pattern");
  ASSERT_STR_EQ(config.json_output_path, "report.json",
                "Parsed \"json\" key should assign parsed value to "
                "CTestConfig.json_output_path");
}

CTEST(SUITE_NAME, test_config_load_file_partial_keys) {
  char *config_file = "./.ctestconfig_82539331";
  char *config_content = "filter = *_filt*\n"
                         "jobs = 8";

  ctest_setup_mock_file(config_file, config_content);

  CTestConfig config = {.filter_pattern = "_replace_this",
                        .target_dir = "./tests"};

  ctest_config_load_file(&config, config_file);

  ASSERT_INT_EQ(config.jobs, 8,
                "Parsed value should assign to CTestConfig member associated "
                "with key when config file does not contain every config key");
  ASSERT_STR_EQ(config.filter_pattern, "*_filt*",
                "Parsed value should replace pre-assigned value for "
                "CTestConfig member associated with key");
  ASSERT_STR_EQ(config.target_dir, "./tests",
                "CTestConfig member value should not change if there is not an "
                "associated key/value pair in the config file");
}

CTEST(SUITE_NAME, test_config_load_file_whitespace_stripping) {
  char *config_file = "./.ctestconfig_00850300";
  char *config_content = "\n"
                         "\t\r   \t\n"
                         "                      \n"
                         "  jobs \t  = 8\n"
                         "\t    \r      \n"
                         "target_dir   =   \r ./tests_here \t  \n";

  ctest_setup_mock_file(config_file, config_content);

  CTestConfig config = {0};
  ctest_config_load_file(&config, config_file);

  ASSERT_INT_EQ(config.jobs, 8,
                "Parsed value should assign correctly when file and key "
                "contains whitespace");
  ASSERT_STR_EQ(config.target_dir, "./tests_here",
                "Whitespace should be removed from value before assigning to "
                "correct CTestConfig member");
}

CTEST(SUITE_NAME, test_config_load_file_comment_stripping) {
  char *config_file = "./.ctestconfig_77778352";
  char *config_content = "# start of file\n"
                         "timeout = 5 # 5 seconds\n"
                         "; semicolon comment\n"
                         "json = tests.json ; where json output\n"
                         "    ;; ; semicolonsss\n\n"
                         "# target_dir = ./this_is_ignored\n";

  ctest_setup_mock_file(config_file, config_content);

  CTestConfig config = {.target_dir = "./tests"};

  ctest_config_load_file(&config, config_file);

  ASSERT_INT_EQ(config.timeout_sec, 5,
                "Parsed value should assign correctly when line has a comment "
                "('#') as a suffix");
  ASSERT_STR_EQ(config.json_output_path, "tests.json",
                "Parsed value should assign correctly when line has a comment "
                "(';') as a suffix");
  ASSERT_STR_EQ(config.target_dir, "./tests",
                "Commented out key/value pair should not alter CTestConfig "
                "member that is associated with it");
}

CTEST(SUITE_NAME, test_config_load_file_syntax_errors) {
  char *config_file = "./.ctestconfig_25892538";
  char *config_content = "random text without equals sign\n"
                         "jobs = \n"
                         " = orphan_value\n"
                         "timeout = 15\n";

  ctest_setup_mock_file(config_file, config_content);

  CTestConfig config = {.jobs = 2, .timeout_sec = 0};
  ctest_config_load_file(&config, config_file);

  ASSERT_INT_EQ(config.jobs, 2,
                "Invalid lines and empty values should leave pre-existing "
                "config values intact");
  ASSERT_INT_EQ(
      config.timeout_sec, 15,
      "Valid lines following syntax errors should still process correctly");
}

CTEST(SUITE_NAME, test_config_load_file_invalid_numeric_values) {
  char *config_file = "./.ctestconfig_85203995";
  char *config_content = "jobs = -5\n"
                         "timeout = 10 sec\n"
                         "target_dir = ./custom_tests\n";

  ctest_setup_mock_file(config_file, config_content);

  CTestConfig config = {.jobs = 4, .timeout_sec = 30};
  ctest_config_load_file(&config, config_file);

  ASSERT_INT_EQ(config.jobs, 4,
                "Negative value for unsigned numeric field should be rejected");
  ASSERT_INT_EQ(
      config.timeout_sec, 30,
      "Trailing non-numeric text suffix should invalidate number parsing");
  ASSERT_STR_EQ(config.target_dir, "./custom_tests",
                "Valid assignments in same file should still apply");
}

CTEST(SUITE_NAME, test_config_load_file_unknown_keys) {
  char *config_file = "./.ctestconfig_90305000";
  char *config_content = "favourite_team = mutd\n"
                         "jobs = 8\n"
                         "custom_setting = true\n";

  ctest_setup_mock_file(config_file, config_content);

  CTestConfig config = {0};
  ctest_config_load_file(&config, config_file);

  ASSERT_INT_EQ(config.jobs, 8,
                "Unknown key-value pairs should be silently ignored while "
                "valid keys parse");
}

CTEST(SUITE_NAME, test_config_load_file_duplicate_keys) {
  char *config_file = "./.ctestconfig_00010593";
  char *config_content = "jobs = 2\n"
                         "verbosity = quiet\n"
                         "jobs = 8\n"
                         "verbosity = verbose\n";

  ctest_setup_mock_file(config_file, config_content);

  CTestConfig config = {0};
  ctest_config_load_file(&config, config_file);

  ASSERT_INT_EQ(
      config.jobs, 8,
      "Later key assignment in same file should override earlier value");
  ASSERT_INT_EQ(config.verbosity, CTEST_VERBOSITY_VERBOSE,
                "Later verbosity assignment should override earlier value");
}