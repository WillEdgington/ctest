#include "json.h"
#include "runner.h"
#include "session.h"
#include <clib/vector.h>
#include <stdio.h>
#include <stdlib.h>

static void print_session_summary(FILE *file, const SessionMetrics *metrics) {
  size_t suites = metrics->total_suites;
  size_t crashes = metrics->total_crashes;
  size_t timeouts = metrics->total_timeouts;
  size_t runs = metrics->total_runs;
  size_t failed = metrics->total_failures;
  size_t passed = runs - failed;
  double duration_ms = ctest_session_get_duration_ms(metrics);

  fprintf(file,
          "  \"summary\": {\n"
          "    \"suites\": {\n"
          "      \"total\": %ld,\n"
          "      \"crashes\": %ld,\n"
          "      \"timeouts\": %ld\n"
          "    },\n"
          "    \"assertions\": {\n"
          "      \"total\": %ld,\n"
          "      \"passed\": %ld,\n"
          "      \"failed\": %ld\n"
          "    },\n"
          "    \"duration_ms\": %.2f\n"
          "  },\n",
          suites, crashes, timeouts, runs, passed, failed, duration_ms);
}

/*
Example:
  "summary": {
    "suites": {
      "total": 5,
      "crashes": 0,
      "timeouts": 1
    },
    "assertions": {
      "total": 24,
      "passed": 22,
      "failed": 2
    }
  },
*/

static void print_escaped_string(FILE *file, const char *str) {
  if (str == NULL) {
    fputs("\"\"", file);
    return;
  }

  fputc('"', file);
  for (const char *p = str; *p != '\0'; p++) {
    switch (*p) {
    case '"':
      fputs("\\\"", file);
      break;
    case '\\':
      fputs("\\\\", file);
      break;
    case '\b':
      fputs("\\b", file);
      break;
    case '\f':
      fputs("\\f", file);
      break;
    case '\n':
      fputs("\\n", file);
      break;
    case '\r':
      fputs("\\r", file);
      break;
    case '\t':
      fputs("\\t", file);
      break;
    default:
      if ((unsigned char)*p < 0x20) {
        fprintf(file, "\\u%04x", (unsigned char)*p);
      } else {
        fputc(*p, file);
      }
      break;
    }
  }
  fputc('"', file);
}

static void print_fail_case_entry(FILE *file, const FailureEntry *entry) {
  fprintf(file, "    {\n");
  fprintf(file, "      \"type\": \"test_case\",\n");
  fprintf(file, "      \"file\": \"%s\",\n", entry->file_path);
  fprintf(file, "      \"line\": %zu,\n", entry->test_case.line_num);
  fprintf(file, "      \"expression\": ");
  print_escaped_string(file, entry->test_case.expr);
  fprintf(file, ",\n");
  fprintf(file, "      \"message\": ");
  print_escaped_string(file, entry->test_case.msg);
  fprintf(file, "\n");
  fprintf(file, "    }");
}

static void print_timeout_entry(FILE *file, const FailureEntry *entry) {
  fprintf(file, "    {\n");
  fprintf(file, "      \"type\": \"timeout\",\n");
  fprintf(file, "      \"file\": \"%s\",\n", entry->file_path);
  fprintf(file, "      \"timeout_sec\": %u\n", entry->timeout_sec);
  fprintf(file, "    }");
}

static void print_crash_entry(FILE *file, const FailureEntry *entry) {
  fprintf(file, "    {\n");
  fprintf(file, "      \"type\": \"crash\",\n");
  fprintf(file, "      \"file\": \"%s\",\n", entry->file_path);
  fprintf(file, "      \"signal\": %d\n", entry->signal_num);
  fprintf(file, "    }");
}

static void print_failure_entry(FILE *file, const FailureEntry *entry) {
  switch (entry->type) {
  case FAILURE_TEST_CASE:
    print_fail_case_entry(file, entry);
    break;

  case FAILURE_SUITE_TIMEOUT:
    print_timeout_entry(file, entry);
    break;

  case FAILURE_SUITE_CRASH:
    print_crash_entry(file, entry);
    break;
  }
}

static void print_ledger(FILE *file, const Vector *failure_ledger) {
  if (failure_ledger->count == 0) {
    fprintf(file, "  \"failures\": []\n");
    return;
  }

  fprintf(file, "  \"failures\": [\n");
  for (size_t i = 0; i < failure_ledger->count; i++) {
    const FailureEntry *entry =
        (const FailureEntry *)vector_get(failure_ledger, i);
    print_failure_entry(file, entry);
    fprintf(file, "%s\n", (i + 1 < failure_ledger->count) ? "," : "");
  }
  fprintf(file, "  ]\n");
}

/*
Example:
  "failures": [
    {
      "type": "test_case",
      "file": "tests/test_math.c",
      "line": 42,
      "expression": "2 == 5",
      "message": "Mismatch"
    },
    {
      "type": "timeout",
      "file": "tests/test_slow.c",
      "timeout_sec": 2
    },
    {
      "type": "crash",
      "file": "tests/test_crash.c",
      "signal": 11
    }
  ]
*/

int ctest_json_write(const char *filepath, const SessionMetrics *metrics,
                     const Vector *failure_ledger) {
  FILE *file = fopen(filepath, "w");
  if (file == NULL)
    return -1;

  fprintf(file, "{\n");

  print_session_summary(file, metrics);
  print_ledger(file, failure_ledger);

  fprintf(file, "}\n");

  fflush(file);
  return fclose(file);
}

/*
Example:
{
  "summary": {
    "suites": {
      "total": 5,
      "crashes": 0,
      "timeouts": 1
    },
    "assertions": {
      "total": 24,
      "passed": 22,
      "failed": 2
    }
  },
  "failures": [
    {
      "type": "test_case",
      "file": "tests/test_math.c",
      "line": 42,
      "expression": "2 == 5",
      "message": "Mismatch"
    },
    {
      "type": "timeout",
      "file": "tests/test_slow.c",
      "timeout_sec": 2
    },
    {
      "type": "crash",
      "file": "tests/test_crash.c",
      "signal": 11
    }
  ]
}
*/
