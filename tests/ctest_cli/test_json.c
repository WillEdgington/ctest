#include "ctest_cli/json.h"
#include "ctest_cli/runner.h"
#include "ctest_cli/session.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>

#define SUITE_NAME test_json

static int FILE_BUF_LEN = 4096;

CTEST(SUITE_NAME, test_json_write_null_args) {
  SessionMetrics metrics = {0};
  Vector ledger;
  vector_init(&ledger, CTEST_MAX_FAIL_LINE_LEN);

  ASSERT_INT_EQ(ctest_json_write(NULL, &metrics, &ledger), -1,
                "NULL filepath returns error code -1");
  ASSERT_INT_EQ(ctest_json_write("test_out.json", NULL, &ledger), -1,
                "NULL metrics returns error code -1");
  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_json_write_invalid_filepath) {
  SessionMetrics metrics = {0};
  Vector ledger;
  vector_init(&ledger, CTEST_MAX_FAIL_LINE_LEN);

  int status = ctest_json_write("/non_existent_dir_43233355/out.json", &metrics,
                                &ledger);
  ASSERT_INT_EQ(status, -1,
                "Writing to invalid file path fails gracefully (return -1)");

  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_json_write_valid_report) {
  const char *test_path = "sandbox_test_report_14437587.json";

  SessionMetrics metrics = {.total_suites = 2,
                            .total_runs = 10,
                            .total_failures = 1,
                            .total_crashes = 0,
                            .total_timeouts = 1};

  Vector ledger;
  vector_init(&ledger, CTEST_MAX_FAIL_LINE_LEN);

  const char fail_1[CTEST_MAX_FAIL_LINE_LEN] =
      "  " CTEST_COLOR_RED "[FAIL]" CTEST_COLOR_RESET
      " Message with \"quotes\" & line \n"
      "         Expression: 1 == 2\n"
      "         Location  : Line 15 in test_fail.c\n";

  const char fail_2[CTEST_MAX_FAIL_LINE_LEN] =
      "  " CTEST_COLOR_CYAN "[TIME]" CTEST_COLOR_RESET
      " Suite execution timed out\n"
      "         Limit     : Exceeded 5 second(s) threshold\n"
      "         Location  : test_slow.c\n";

  vector_push(&ledger, fail_1);
  vector_push(&ledger, fail_2);

  int status = ctest_json_write(test_path, &metrics, &ledger);
  ASSERT_INT_EQ(status, 0, "Valid report should return success (0)");

  FILE *f = fopen(test_path, "r");
  ASSERT_PTR_NOT_NULL(f, "Output JSON file should exist on disk");
  if (f != NULL) {
    char buf[FILE_BUF_LEN];
    size_t bytes_read = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    unlink(test_path);

    ASSERT(bytes_read > 0, "JSON output file is non_empty");

    ASSERT_PTR_NOT_NULL(strstr(buf, "\"total\": 2"),
                        "JSON output file contains total suites");
    ASSERT_PTR_NOT_NULL(
        strstr(buf, "\"passed\": 9"),
        "JSON output file contains calculated passed assertions");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"failed\": 1"),
                        "JSON output file contains failed assertion count");
    ASSERT_PTR_NOT_NULL(strstr(buf, "\"timeouts\": 1"),
                        "JSON output file contains timeout count");

    ASSERT_PTR_NULL(strstr(buf, CTEST_COLOR_RED),
                    "ANSI colour codes stripped in JSON output file");
    ASSERT_PTR_NOT_NULL(
        strstr(buf, "Message with \\\"quotes\\\" & line \\n"),
        "Escaped quotes and newline in string in JSON output file");
  }

  vector_free(&ledger);
}
