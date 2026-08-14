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
          "    }\n"
          "  },\n",
          suites, crashes, timeouts, runs, passed, failed);
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

static char *process_char(char *ptr, char c) {
  switch (c) {
  case '"':
    *ptr++ = '\\';
    *ptr++ = '"';
    break;
  case '\\':
    *ptr++ = '\\';
    *ptr++ = '\\';
    break;
  case '\n':
    *ptr++ = '\\';
    *ptr++ = 'n';
    break;
  case '\r':
    *ptr++ = '\\';
    *ptr++ = 'r';
    break;
  case '\t':
    *ptr++ = '\\';
    *ptr++ = 't';
    break;
  default:
    *ptr++ = c;
    break;
  }
  return ptr;
}

// ANSI CSI sequences terminate on a final byte in ASCII range 0x40-0x7E (@
// through ~)
static inline int is_csi_final_byte(char c) { return c >= '@' && c <= '~'; }

static const char *skip_ansi(const char *ptr) {
  if (ptr[0] == '\033' && ptr[1] == '[') {
    const char *cur = ptr + 2;
    while (*cur != '\0' && !is_csi_final_byte(*cur)) {
      cur++;
    }
    return (*cur != '\0') ? cur + 1 : cur;
  }
  return ptr;
}

static void print_ledger_item(FILE *file, const char *message) {
  // worse case: whole message is_escaped and of max length (double max length)
  char buf[CTEST_MAX_FAIL_LINE_LEN << 1];

  const char *m_ptr = message;
  char *b_ptr = buf;

  char c;
  while (*m_ptr != '\0') {
    const char *nxt = skip_ansi(m_ptr);
    if (nxt != m_ptr) {
      m_ptr = nxt;
      continue;
    }

    c = *m_ptr;
    b_ptr = process_char(b_ptr, c);
    m_ptr++;
  }
  *b_ptr = '\0';

  fprintf(file, "\"%s\"\n", buf);
}

/*
Example:
"  [FAIL] 2 == 5\n         Expression: 2 == 5\n         Location  : Line 42 in
tests/test_math.c\n",
*/

static void print_ledger(FILE *file, const Vector *failure_ledger) {
  if (failure_ledger->count == 0) {
    fprintf(file, "  \"failures\": []\n");
    return;
  }

  fprintf(file, "  \"failures\": [\n");

  Iter it = vector_iter((Vector *)failure_ledger);
  while (it.next(&it) == 0) {
    fprintf(file, "    ");
    print_ledger_item(file, (const char *)it.current.value);
    fprintf(file, "%s\n", it.index + 1 < failure_ledger->count ? "," : "");
  }
  fprintf(file, "  ]\n");
}

/*
Example:
  "failures": [
    "  [FAIL] 2 == 5\n         Expression: 2 == 5\n         Location  : Line 42
in tests/test_math.c\n", "  [TIME] Suite execution timed out\n         Limit :
Exceeded 2 second(s) threshold\n         Location  : tests/test_slow.c\n"
  ]
*/

int ctest_json_write(const char *filepath, const SessionMetrics *metrics,
                     const Vector *failure_ledger) {
  if (filepath == NULL || metrics == NULL)
    return -1;

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
    "  [FAIL] 2 == 5\n         Expression: 2 == 5\n         Location  : Line 42
in tests/test_math.c\n", "  [TIME] Suite execution timed out\n         Limit :
Exceeded 2 second(s) threshold\n         Location  : tests/test_slow.c\n"
  ]
}
*/
