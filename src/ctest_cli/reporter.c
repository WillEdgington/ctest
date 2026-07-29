#include "reporter.h"
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
  if (metrics->crashed != 0) {
    printf(CTEST_COLOR_YELLOW "[CRASH] " CTEST_COLOR_RESET);
  } else if (metrics->total_failures > 0) {
    printf(CTEST_COLOR_RED "[FAIL]  " CTEST_COLOR_RESET);
  } else {
    printf(CTEST_COLOR_GREEN "[PASS]  " CTEST_COLOR_RESET);
  }
}

static void print_test_outcomes(const SuiteMetrics *metrics) {
  int runs = metrics->total_runs;
  int fails = metrics->total_failures;
  int crashed = metrics->crashed;
  int passed = runs > fails ? runs - fails : 0;

  if (passed > 0) {
    printf(CTEST_COLOR_GREEN);
    for (int i = 0; i < passed; i++)
      printf(".");
  }
  if (fails > 0) {
    printf(CTEST_COLOR_RED);
    for (int i = 0; i < fails; i++)
      printf("F");
  }
  if (crashed == 1) {
    printf(CTEST_COLOR_YELLOW "C");
  }
  printf(CTEST_COLOR_RESET);
}

void ctest_report_start_banner(const char *root_dir) {
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

void ctest_report_suite_metrics(const char *binary_path,
                                const SuiteMetrics *metrics) {
  print_suite_conclusion(metrics);
  printf("%s: ", binary_path);
  print_test_outcomes(metrics);
  printf("\n");
  /*
  [PASS]  tests/ctest_cli/test_discovery: ......
  [FAIL]  tests/ctest_cli/test_discovery: ....FF
  [CRASH] tests/ctest_cli/test_discovery: ..FC
  */
}

void ctest_report_ledger(const Vector *ledger) {
  if (ledger->count == 0)
    return;
  print_horiz_marg(CTEST_COLOR_RED " FAILURE REPORT " CTEST_COLOR_RESET);

  Iter it = vector_iter((Vector *)ledger);
  while (it.next(&it) == 0) {
    printf("\n%s", (char *)it.current.value);
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

void ctest_report_summary(const SessionMetrics *session) {
  size_t suites = session->total_suites;
  size_t tests = session->total_runs;
  size_t failed = session->total_failures;
  size_t crashed = session->total_crashes;
  size_t passed = tests > failed ? tests - failed : 0;

  char buffer[buffer_len];
  if (session->total_failures == 0 && session->total_crashes == 0) {
    snprintf(buffer, sizeof(buffer),
             CTEST_COLOR_GREEN "ALL " CTEST_COLOR_RESET "%zu" CTEST_COLOR_GREEN
                               " SUITES PASSED" CTEST_COLOR_RESET,
             suites);
  } else {
    snprintf(buffer, sizeof(buffer),
             "SUITES: %zu  TESTS: %zu  PASSED: " CTEST_COLOR_GREEN
             "%zu" CTEST_COLOR_RESET "  FAILED: " CTEST_COLOR_RED
             "%zu" CTEST_COLOR_RESET "  CRASHED: " CTEST_COLOR_YELLOW
             "%zu" CTEST_COLOR_RESET,
             suites, tests, passed, failed, crashed);
  }

  print_horiz_marg(NULL);
  print_centred_string(buffer, NULL);
  print_horiz_marg(NULL);
  /*
  All Passed:
  ==============================================================================
                              ALL X SUITES PASSED
  ==============================================================================
  Some Passed:
  ==============================================================================
                SUITES: X  TESTS: X  PASSED: X  FAILED: X  CRASHED: X
  ==============================================================================
  */
}
