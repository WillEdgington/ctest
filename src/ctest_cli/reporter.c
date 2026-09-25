#include "reporter.h"
#include "config.h"
#include "executor.h"
#include "session.h"
#include <clib/iter.h>
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static size_t line_len = 128;
static size_t buffer_len = 256;
static char *h_marg = "=";

static size_t visible_strlen(const char *str) {
  size_t len = 0;
  int in_esc = 0;

  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\x1b') {
      in_esc = 1;
    } else if (in_esc) {
      if ((str[i] >= 'A' && str[i] <= 'Z') ||
          (str[i] >= 'a' && str[i] <= 'z')) {
        in_esc = 0;
      }
    } else {
      len++;
    }
  }
  return len;
}

static void print_centred_string(const char *str, char *delim_char) {
  size_t str_len = visible_strlen(str);
  if (str_len >= line_len) {
    printf("%s\n", str);
    return;
  }

  char d_char[2] = " \0";
  if (delim_char != NULL)
    d_char[0] = delim_char[0];

  size_t spaces = line_len - str_len;
  for (size_t i = 0; i < spaces >> 1; i++)
    printf("%s", d_char);

  printf("%s", str);

  for (size_t i = spaces >> 1; i < spaces; i++)
    printf("%s", d_char);
  printf("\n");
}

static void print_horiz_marg(const char *str) {
  if (str == NULL) {
    for (size_t i = 0; i < line_len; i++)
      printf("%s", h_marg);
    printf("\n");
    return;
  }
  print_centred_string(str, h_marg);
}

static void print_suite_conclusion(const SuiteMetrics *metrics) {
  if (metrics->state == SUITE_CRASH) {
    printf(CTEST_COLOR_YELLOW "[CRASH] " CTEST_COLOR_RESET);
  } else if (metrics->state == SUITE_TIMEOUT) {
    printf(CTEST_COLOR_CYAN "[TIME]  " CTEST_COLOR_RESET);
  } else if (metrics->total_failures > 0) {
    printf(CTEST_COLOR_RED "[FAIL]  " CTEST_COLOR_RESET);
  } else {
    printf(CTEST_COLOR_GREEN "[PASS]  " CTEST_COLOR_RESET);
  }
}

static void print_test_outcomes(const SuiteMetrics *metrics) {
  if (metrics->test_results.count > 0) {
    TestStatus cur_status;

    for (size_t i = 0; i < metrics->test_results.count; i++) {
      TestCaseResult *test_case = vector_get(&metrics->test_results, i);

      if (i == 0 || cur_status != test_case->status) {
        cur_status = test_case->status;
        printf("%s",
               cur_status == TEST_PASS ? CTEST_COLOR_GREEN : CTEST_COLOR_RED);
      }

      printf("%s", cur_status == TEST_PASS ? "." : "F");
    }
  }

  switch (metrics->state) {
  case SUITE_CRASH:
    printf(CTEST_COLOR_YELLOW "C");
    break;
  case SUITE_TIMEOUT:
    printf(CTEST_COLOR_CYAN "T");
    break;
  default:
    break;
  }
  printf(CTEST_COLOR_RESET);
}

void ctest_report_start_banner(const char *root_dir, CTestVerbosity verbosity) {
  // do not print start banner when --quiet flag is present
  if (verbosity == CTEST_VERBOSITY_QUIET)
    return;

  char buffer[buffer_len];
  snprintf(buffer, sizeof(buffer), "--- RUNNING CTEST ON: %s ---", root_dir);

  print_horiz_marg(NULL);
  print_centred_string(buffer, NULL);
  print_horiz_marg(NULL);
  /*
  ==============================================================================
                      --- RUNNING CTEST ON: root_dir ---
  ==============================================================================
  */
}

static void report_suite_metrics_normal(const char *binary_path,
                                        const SuiteMetrics *metrics) {
  print_suite_conclusion(metrics);
  printf("%s: ", binary_path);
  print_test_outcomes(metrics);
  printf("\n");

  /*
  [PASS]  tests/ctest_cli/test_discovery: ......
  [FAIL]  tests/ctest_cli/test_discovery: ...F.F
  [CRASH] tests/ctest_cli/test_discovery: F..C
  [TIME]  tests/ctest_cli/test_discovery: ..FF.T
  */
}

static void print_test_case(const char *file_path,
                            const TestCaseResult *test_case) {
  if (test_case->status == TEST_PASS) {
    printf(CTEST_COLOR_GREEN "[PASS]" CTEST_COLOR_RESET
                             " %s (%s) | Line %zu in %s\n",
           test_case->msg[0] ? test_case->msg : "---", test_case->expr,
           test_case->line_num, file_path);
  } else {
    printf(CTEST_COLOR_RED "[FAIL]" CTEST_COLOR_RESET
                           " %s (%s) | Line %zu in %s\n",
           test_case->msg[0] ? test_case->msg : "---", test_case->expr,
           test_case->line_num, file_path);
  }
  /*
  [<status>] <message> (expression) | Line <line-number> in <file>
  */
}

static void report_suite_metrics_verbose(const char *binary_path,
                                         const SuiteMetrics *metrics) {
  printf("\n[RUN] %s\n", binary_path);

  const char *file_path = metrics->file_path;
  Iter it = vector_iter((Vector *)&metrics->test_results);
  while (it.next(&it) == 0) {
    TestCaseResult *test_case = (TestCaseResult *)it.current.value;
    printf("  ");
    print_test_case(file_path, test_case);
  }

  print_suite_conclusion(metrics);
  printf("\n");
  /*

  [RUN] <binary_path>
    [<status>] <message> (expression) | Line <line-number> in <file>
    ...
  [<metrics->state>]
  */
}

void ctest_report_suite_metrics(const char *binary_path,
                                const SuiteMetrics *metrics,
                                CTestVerbosity verbosity) {
  switch (verbosity) {
  case CTEST_VERBOSITY_NORMAL:
    report_suite_metrics_normal(binary_path, metrics);
    break;
  case CTEST_VERBOSITY_VERBOSE:
    report_suite_metrics_verbose(binary_path, metrics);
    break;
  default:
    break;
  }
}

static void print_failure_entry(FailureEntry *fail_entry) {
  switch (fail_entry->type) {
  case FAILURE_TEST_CASE:
    printf("\n  " CTEST_COLOR_RED "[FAIL]" CTEST_COLOR_RESET " %s\n"
           "         Expression: %s\n"
           "         Location  : Line %zu in %s\n",
           fail_entry->test_case.msg[0] ? fail_entry->test_case.msg
                                        : fail_entry->test_case.expr,
           fail_entry->test_case.expr, fail_entry->test_case.line_num,
           fail_entry->file_path);
    break;
  case FAILURE_SUITE_TIMEOUT:
    printf("\n  " CTEST_COLOR_CYAN "[TIME]" CTEST_COLOR_RESET
           " Suite execution timed out\n"
           "         Limit     : Exceeded %u second(s) threshold\n"
           "         Location  : %s\n",
           fail_entry->timeout_sec, fail_entry->file_path);
    break;
  case FAILURE_SUITE_CRASH:
    printf("\n  " CTEST_COLOR_YELLOW "[CRASH]" CTEST_COLOR_RESET
           " Suite execution terminated unexpectedly\n"
           "         Signal    : Terminated by signal %d\n"
           "         Location  : %s\n",
           fail_entry->signal_num, fail_entry->file_path);
    break;
  default:
    break;
  }
}

void ctest_report_ledger(const Vector *ledger, CTestVerbosity verbosity) {
  // do not print an empty failure report
  if (ledger->count == 0)
    return;

  // do not print header when --quiet flag present
  if (verbosity != CTEST_VERBOSITY_QUIET)
    print_horiz_marg(CTEST_COLOR_RED " FAILURE REPORT " CTEST_COLOR_RESET);

  Iter it = vector_iter((Vector *)ledger);
  while (it.next(&it) == 0) {
    FailureEntry *fail_entry = (FailureEntry *)it.current.value;
    print_failure_entry(fail_entry);
  }
  printf("\n");
  /*
  ('FAILURE REPORT' in red)
  =============================== FAILURE REPORT ===============================

  ledger[0] output
  ledger[1] output
  etc...
  */
}

static void report_summary_quiet(const SessionMetrics *session) {
  size_t suites = session->total_suites;
  size_t tests = session->total_runs;
  size_t failed = session->total_failures;
  size_t crashed = session->total_crashes;
  size_t timeouts = session->total_timeouts;
  size_t passed = tests > failed ? tests - failed : 0;

  if (failed == 0 && crashed == 0 && timeouts == 0) {
    printf(CTEST_COLOR_GREEN "ALL " CTEST_COLOR_RESET "%zu" CTEST_COLOR_GREEN
                             " SUITES PASSED" CTEST_COLOR_RESET "\n",
           suites);
  } else {
    printf("SUITES: %zu, TESTS: %zu, PASSED: " CTEST_COLOR_GREEN
           "%zu" CTEST_COLOR_RESET ", FAILED: " CTEST_COLOR_RED
           "%zu" CTEST_COLOR_RESET ", CRASHED: " CTEST_COLOR_YELLOW
           "%zu" CTEST_COLOR_RESET ", TIMEOUTS: " CTEST_COLOR_CYAN
           "%zu" CTEST_COLOR_RESET "\n",
           suites, tests, passed, failed, crashed, timeouts);
  }
  /*
  All Passed:
  ALL X SUITES PASSED
  Some Passed:
  SUITES: X, TESTS: X, PASSED: X, FAILED: X, CRASHED: X, TIMEOUTS: X
  */
}

void ctest_report_summary(const SessionMetrics *session,
                          CTestVerbosity verbosity) {
  if (verbosity == CTEST_VERBOSITY_QUIET) {
    report_summary_quiet(session);
    return;
  }

  size_t suites = session->total_suites;
  size_t tests = session->total_runs;
  size_t failed = session->total_failures;
  size_t crashed = session->total_crashes;
  size_t timeouts = session->total_timeouts;
  size_t passed = tests > failed ? tests - failed : 0;

  char buffer[buffer_len];
  if (failed == 0 && crashed == 0 && timeouts == 0) {
    snprintf(buffer, sizeof(buffer),
             CTEST_COLOR_GREEN "ALL " CTEST_COLOR_RESET "%zu" CTEST_COLOR_GREEN
                               " SUITES PASSED" CTEST_COLOR_RESET,
             suites);
  } else {
    snprintf(buffer, sizeof(buffer),
             "SUITES: %zu  TESTS: %zu  PASSED: " CTEST_COLOR_GREEN
             "%zu" CTEST_COLOR_RESET "  FAILED: " CTEST_COLOR_RED
             "%zu" CTEST_COLOR_RESET "  CRASHED: " CTEST_COLOR_YELLOW
             "%zu" CTEST_COLOR_RESET "  TIMEOUTS: " CTEST_COLOR_CYAN
             "%zu" CTEST_COLOR_RESET,
             suites, tests, passed, failed, crashed, timeouts);
  }

  print_horiz_marg(NULL);
  print_centred_string(buffer, NULL);
  print_horiz_marg(NULL);
  /*
  All Passed (roughly):
  ==============================================================================
                              ALL X SUITES PASSED
  ==============================================================================
  Some Passed (roughly):
  ==============================================================================
        SUITES: X  TESTS: X  PASSED: X  FAILED: X  CRASHED: X  TIMEOUTS: X
  ==============================================================================
  */
}
