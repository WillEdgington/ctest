#include "ctest_cli/executor.h"
#include "ctest_cli/json.h"
#include "ctest_cli/runner.h"
#include "ctest_cli/session.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SUITE_NAME test_json

static const size_t FILE_BUF_LEN = 4096;

CTEST(SUITE_NAME, test_json_write_invalid_filepath) {
  SessionMetrics metrics = {0};
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  int status = ctest_json_write("/non_existent_dir_43233355/out.json", &metrics,
                                &ledger);
  ASSERT_INT_EQ(status, -1,
                "Writing to invalid file path fails gracefully (return -1)");

  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_json_write_empty_ledger) {
  const char *test_path = "sandbox_test_empty_ledger_88392019.json";
  SessionMetrics metrics = {
      .total_suites = 1, .total_runs = 5, .total_failures = 0};

  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  int status = ctest_json_write(test_path, &metrics, &ledger);
  ASSERT_INT_EQ(status, 0, "Empty ledger writes report successfully (0)");

  FILE *f = fopen(test_path, "r");
  ASSERT_PTR_NOT_NULL(f, "JSON report should be created on disk");
  if (f != NULL) {
    char buf[FILE_BUF_LEN];
    size_t bytes_read = fread(buf, 1, sizeof(buf) - 1, f);
    buf[bytes_read] = '\0';
    fclose(f);
    unlink(test_path);

    ASSERT_PTR_NOT_NULL(strstr(buf, "\"failures\": []"),
                        "Empty ledger produces empty failures JSON array");
  }

  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_json_write_all_failure_types) {
  const char *test_path = "sandbox_test_report_14437587.json";

  SessionMetrics metrics = {.total_suites = 3,
                            .total_runs = 10,
                            .total_failures = 1,
                            .total_crashes = 1,
                            .total_timeouts = 1};

  // duration: 2212.34ms
  metrics.end_time.tv_sec = 2;
  metrics.end_time.tv_nsec = 212340000;
  metrics.start_time.tv_sec = 0;
  metrics.start_time.tv_nsec = 0;

  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  FailureEntry fail_case = {0};
  fail_case.type = FAILURE_TEST_CASE;
  snprintf(fail_case.file_path, sizeof(fail_case.file_path),
           "tests/test_math.c");
  fail_case.test_case.line_num = 42;
  fail_case.test_case.status = TEST_FAIL;
  snprintf(fail_case.test_case.expr, sizeof(fail_case.test_case.expr),
           "a == b");
  snprintf(fail_case.test_case.msg, sizeof(fail_case.test_case.msg),
           "Mismatch with \"quotes\" & line \n break");

  FailureEntry fail_timeout = {0};
  fail_timeout.type = FAILURE_SUITE_TIMEOUT;
  snprintf(fail_timeout.file_path, sizeof(fail_timeout.file_path),
           "tests/test_slow.c");
  fail_timeout.timeout_sec = 5;

  FailureEntry fail_crash = {0};
  fail_crash.type = FAILURE_SUITE_CRASH;
  snprintf(fail_crash.file_path, sizeof(fail_crash.file_path),
           "tests/test_crash.c");
  fail_crash.signal_num = 11;

  vector_push(&ledger, &fail_case);
  vector_push(&ledger, &fail_timeout);
  vector_push(&ledger, &fail_crash);

  int status = ctest_json_write(test_path, &metrics, &ledger);
  ASSERT_INT_EQ(status, 0, "Valid report with mixed failures should return 0");

  FILE *f = fopen(test_path, "r");
  ASSERT_PTR_NOT_NULL(f, "Output JSON file should exist on disk");
  if (f != NULL) {
    char buf[FILE_BUF_LEN];
    size_t bytes_read = fread(buf, 1, sizeof(buf) - 1, f);
    buf[bytes_read] = '\0';
    fclose(f);
    unlink(test_path);

    ASSERT(bytes_read > 0, "JSON output file is non-empty");

    // session metrics checks
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"total\": 3"),
                        "JSON contains total suites count");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"passed\": 9"),
                        "JSON contains calculated passed assertions (10 - 1)");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"failed\": 1"),
                        "JSON contains failed assertion count");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"crashes\": 1"),
                        "JSON contains crash count");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"timeouts\": 1"),
                        "JSON contains timeout count");
    ASSERT_PTR_NOT_NULL(
        strstr(buf, "\"duration_ms\": 2212.34"),
        "JSON contains session duration in milliseconds to 2 decimal places");

    // test case failure object checks
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"type\": \"test_case\""),
                        "JSON contains test_case failure type");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"file\": \"tests/test_math.c\""),
                        "JSON contains test case file path");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"line\": 42"),
                        "JSON contains test case line number");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"expression\": \"a == b\""),
                        "JSON contains test case expression");
    ASSERT_PTR_NOT_NULL(
        strstr(
            buf,
            "\"message\": \"Mismatch with \\\"quotes\\\" & line \\n break\""),
        "JSON contains properly escaped test case message");

    // timeout failure object checks
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"type\": \"timeout\""),
                        "JSON contains timeout failure type");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"file\": \"tests/test_slow.c\""),
                        "JSON contains timeout file path");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"timeout_sec\": 5"),
                        "JSON contains timeout threshold value");

    // crash failure object checks
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"type\": \"crash\""),
                        "JSON contains crash failure type");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"file\": \"tests/test_crash.c\""),
                        "JSON contains crash file path");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"signal\": 11"),
                        "JSON contains crash signal number");
  }

  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_json_write_escaping_special_chars) {
  const char *test_path = "sandbox_test_escaping_99182371.json";
  SessionMetrics metrics = {
      .total_suites = 1, .total_runs = 1, .total_failures = 1};

  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  FailureEntry entry = {0};
  entry.type = FAILURE_TEST_CASE;
  snprintf(entry.file_path, sizeof(entry.file_path), "tests/test_escape.c");
  entry.test_case.line_num = 100;
  entry.test_case.status = TEST_FAIL;
  snprintf(entry.test_case.expr, sizeof(entry.test_case.expr),
           "str == \"hello\\world\"");
  snprintf(entry.test_case.msg, sizeof(entry.test_case.msg),
           "Tab\tNewline\nCR\rBS\bFF\f");

  vector_push(&ledger, &entry);

  int status = ctest_json_write(test_path, &metrics, &ledger);
  ASSERT_INT_EQ(status, 0, "Escaping test suite writes file successfully");

  FILE *f = fopen(test_path, "r");
  ASSERT_PTR_NOT_NULL(f, "JSON file created for escaping test");
  if (f != NULL) {
    char buf[FILE_BUF_LEN];
    size_t bytes_read = fread(buf, 1, sizeof(buf) - 1, f);
    buf[bytes_read] = '\0';
    fclose(f);
    unlink(test_path);

    ASSERT_PTR_NOT_NULL(
        strstr(buf, "\"expression\": \"str == \\\"hello\\\\world\\\"\""),
        "Backslashes and quotes escaped in expression field");
    ASSERT_PTR_NOT_NULL(
        strstr(buf, "\"message\": \"Tab\\tNewline\\nCR\\rBS\\bFF\\f\""),
        "Tab, newline, CR, backspace, and form feed escaped in message field");
  }

  vector_free(&ledger);
}
