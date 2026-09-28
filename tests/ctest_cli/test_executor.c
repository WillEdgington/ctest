#define _GNU_SOURCE
#include "ctest_cli/executor.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SUITE_NAME test_executor

// 5 passes, return 0
static const char *mock_passing_bin = "./mock_passing_bin_35893333";
static const char *mock_passing_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "5" CTEST_TEST_DELIM "0\\n\");\n"
    "  return 0;\n"
    "}\n";

// 1 pass, 1 fail, return 1
static const char *mock_failing_bin = "./mock_failing_bin_78677545";
static const char *mock_failing_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"FAIL" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"PASS" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "<line-number>" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM
    "<message>\\n\");\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "2" CTEST_TEST_DELIM "1\\n\");\n"
    "  return 1;\n"
    "}\n";

// trigger SIGSEGV
static const char *mock_crashing_bin = "./mock_crashing_bin_48691321";
static const char *mock_crashing_c_code = "#include <stdlib.h>\n"
                                          "int main(void) {\n"
                                          "  int *p = NULL;\n"
                                          "  *p = 42; /* Triggers SIGSEGV */\n"
                                          "  return 0;\n"
                                          "}\n";

CTEST_SETUP_SUITE(SUITE_NAME) {
  ctest_setup_mock_binary(mock_passing_bin, mock_passing_c_code);
  ctest_setup_mock_binary(mock_failing_bin, mock_failing_c_code);
  ctest_setup_mock_binary(mock_crashing_bin, mock_crashing_c_code);
}

// -- ctest_suite_metrics_cleanup() tests --

CTEST(SUITE_NAME, test_suite_metrics_lifecycle_helpers) {
  SuiteMetrics metrics;

  ASSERT_INT_EQ(ctest_suite_metrics_init(&metrics), 0,
                "ctest_suite_metrics_init() call should return 0 (success) "
                "when vector initialisation is possible");

  TestCaseResult result = {.status = TEST_FAIL,
                           .line_num = 56,
                           .expr = "1 != 1",
                           .msg = "One should not equal one"};
  vector_push(&metrics.test_results, &result);
  metrics.total_runs++;
  metrics.total_failures++;

  ASSERT_INT_EQ(
      metrics.test_results.count, 1,
      "SuiteMetrics.test_results.count should increase by one after push");

  ctest_suite_metrics_cleanup(&metrics);

  ASSERT_INT_EQ(metrics.total_runs, 0,
                "SuiteMetrics.total_runs should reset after cleanup");
  ASSERT_INT_EQ(metrics.total_failures, 0,
                "SuiteMetrics.total_failures should reset after cleanup");
  ASSERT_INT_EQ(metrics.test_results.count, 0,
                "SuiteMetrics.test_results.count should reset after cleanup");
  ASSERT_PTR_NULL(
      metrics.test_results.items,
      "SuiteMetrics.test_results.items should be NULL after cleanup");
}

// -- ctest_launch_suite() tests --

// successful launch:
// precondition: call ctest_launch_suite() with valid binary_path,
// CTEST_VERBOSITY_NORMAL, and a new worker slot should return 0 slot->is_active
// should become 1 slot->pid should be a positive process ID slot->read_fd is a
// valid fd configured for non-blocking I/O slot->bin_path equals binary_path
// slot->buf_pos should be 0

CTEST(SUITE_NAME, test_launch_suite_successful_launch) {
  WorkerSlot slot = {0};

  ASSERT_INT_EQ(
      ctest_launch_suite(mock_passing_bin, &slot), 0,
      "ctest_launch_suite() call with valid binary path should return 0");
  ASSERT_INT_EQ(slot.is_active, 1,
                "WorkerSlot.is_active should be 1 after successful launch");
  ASSERT(
      slot.pid >= 0,
      "WorkerSlot.pid should be a positive process ID after successful launch");
  ASSERT(slot.read_fd >= 0, "WorkerSlot.read_fd should be a valid file "
                            "descriptor after successful launch");
  ASSERT_STR_EQ(slot.bin_path, mock_passing_bin,
                "WorkerSlot.bin_path should equal input binary_path");
  ASSERT_INT_EQ(
      slot.buf_pos, 0,
      "WorkerSlot.buf_pos should be set to 0 after successful launch");

  int status;
  waitpid(slot.pid, &status, 0);
  close(slot.read_fd);
}

// unlaunched slot:
// precondition: set slot->is_active to 0 or slot->read_fd to < 0 and call
// ctest_harvest_output() should return 0 without attempting pipe read or
// altering metrics/ledger

CTEST(SUITE_NAME, test_harvest_output_unlaunched_slot) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);

  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  int bytes_read = ctest_harvest_output(&slot, &metrics, &ledger);

  ASSERT_INT_EQ(bytes_read, 0,
                "ctest_harvest_output() on unlaunched slot should return 0");
  ASSERT_INT_EQ((int)metrics.total_runs, 0,
                "unlaunched slot harvest should not alter suite metrics");

  slot.read_fd = -1;
  bytes_read = ctest_harvest_output(&slot, &metrics, &ledger);

  ASSERT_INT_EQ(bytes_read, 0,
                "ctest_harvest_output() on closed fd should return 0");
  ASSERT_INT_EQ(metrics.total_runs, 0,
                "slot harvest with closed fd should not alter suite metrics");

  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// non-blocking partial & empty reads (EAGAIN/EWOULDBLOCK):
// precondition: active slot, pipe has no data available at call time
// should return -1 (or 0 if EOF) without blocking

CTEST(SUITE_NAME, test_harvest_output_non_blocking_partial_reads) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);

  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  int pipefds[2];
  pipe(pipefds);

  int flags = fcntl(pipefds[0], F_GETFL, 0);
  fcntl(pipefds[0], F_SETFL, flags | O_NONBLOCK);

  slot.is_active = 1;
  slot.read_fd = pipefds[0];

  int res = ctest_harvest_output(&slot, &metrics, &ledger);

  ASSERT_INT_EQ(res, -1,
                "ctest_harvest_output() should return -1 on EAGAIN when pipe "
                "has no data");

  close(pipefds[0]);
  close(pipefds[1]);
  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

CTEST(SUITE_NAME, test_harvest_output_empty_reads) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);

  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  int pipefds[2];
  pipe(pipefds);

  int flags = fcntl(pipefds[0], F_GETFL, 0);
  fcntl(pipefds[0], F_SETFL, flags | O_NONBLOCK);

  close(pipefds[1]); // EOF

  slot.is_active = 1;
  slot.read_fd = pipefds[0];

  int res = ctest_harvest_output(&slot, &metrics, &ledger);

  ASSERT_INT_EQ(res, 0, "ctest_harvest_output() should return 0 on EOF");

  close(pipefds[0]);
  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// complete line stream processing:
// precondition: pipe stream contains fully framed PASSED, FAIL or SUMMARY
// protocol lines ending in \n SUMMARY should update metrics->total_runs and
// metrics->total_failures FAIL should push to failure_ledger should return
// total count of bytes processed during read iteration

CTEST(SUITE_NAME, test_harvest_output_complete_line_stream) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_failing_bin, &slot);

  int status;
  waitpid(slot.pid, &status, 0);

  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  int nbytes = ctest_harvest_output(&slot, &metrics, &ledger);

  ASSERT(nbytes > 0,
         "ctest_harvest_output() should read bytes from test output");
  ASSERT_INT_EQ(metrics.total_runs, 2,
                "SuiteMetrics.total_runs should match run count");
  ASSERT_INT_EQ(metrics.total_failures, 1,
                "SuiteMetrics.total_failures should match failure count");
  ASSERT_INT_EQ(ledger.count, metrics.total_failures,
                "Item count for failure ledger should equal total failures");

  close(slot.read_fd);
  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// partial line buffering across harvest calls:
// precondition: pipe stream delivers a protocol line in chunks
// first invocation buffers incomplete line in slot->line_buf (without updating
// metrics) second invocation completes line, triggers parser, updates metrics,
// and rests slot->buf_pos to 0

CTEST(SUITE_NAME, test_harvest_output_partial_line_buffering) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);

  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  int pipefds[2];
  pipe(pipefds);

  int flags = fcntl(pipefds[0], F_GETFL, 0);
  fcntl(pipefds[0], F_SETFL, flags | O_NONBLOCK);

  slot.is_active = 1;
  slot.read_fd = pipefds[0];

  char *chunk1 = "this line is incomplete...";
  write(pipefds[1], chunk1, strlen(chunk1));

  int nbytes = ctest_harvest_output(&slot, &metrics, &ledger);
  ASSERT_INT_EQ(nbytes, strlen(chunk1),
                "ctest_harvest_output() should return byte count");
  ASSERT_INT_EQ(slot.buf_pos, strlen(chunk1),
                "WorkerSlot.buf_pos should reflect partial line buffer size");

  char *chunk2 = " Not anymore.\n";
  write(pipefds[1], chunk2, strlen(chunk2));

  nbytes = ctest_harvest_output(&slot, &metrics, &ledger);
  ASSERT_INT_EQ(slot.buf_pos, 0,
                "WorkerSlot.buf_pos should reset to 0 once line completes");

  close(pipefds[0]);
  close(pipefds[1]);
  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// test that IPC stream correctly populates ledger and SuiteMetrics structure

CTEST(SUITE_NAME, test_harvest_output_parsed_struct_fidelity) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  int pipefds[2];
  pipe(pipefds);
  int flags = fcntl(pipefds[0], F_GETFL, 0);
  fcntl(pipefds[0], F_SETFL, flags | O_NONBLOCK);

  slot.is_active = 1;
  slot.read_fd = pipefds[0];

  const char *stream =
      "PASS" CTEST_TEST_DELIM "src/test_pass.c" CTEST_TEST_DELIM
      "12" CTEST_TEST_DELIM "x == 10" CTEST_TEST_DELIM "x ok\n"
      "FAIL" CTEST_TEST_DELIM "src/test_fail.c" CTEST_TEST_DELIM
      "45" CTEST_TEST_DELIM "y == 20" CTEST_TEST_DELIM "y wrong\n";

  write(pipefds[1], stream, strlen(stream));
  ctest_harvest_output(&slot, &metrics, &ledger);

  ASSERT_INT_EQ(metrics.test_results.count, 2, "Should parse two test results");

  TestCaseResult *res0 = (TestCaseResult *)vector_get(&metrics.test_results, 0);
  ASSERT_INT_EQ(res0->status, TEST_PASS, "First test should be TEST_PASS");
  ASSERT_INT_EQ(res0->line_num, 12, "Line number should be 12");
  ASSERT_STR_EQ(res0->expr, "x == 10", "Expression should match");
  ASSERT_STR_EQ(res0->msg, "x ok", "Message should match");

  TestCaseResult *res1 = (TestCaseResult *)vector_get(&metrics.test_results, 1);
  ASSERT_INT_EQ(res1->status, TEST_FAIL, "Second test should be TEST_FAIL");
  ASSERT_INT_EQ(res1->line_num, 45, "Line number should be 45");
  ASSERT_STR_EQ(res1->expr, "y == 20", "Expression should match");
  ASSERT_STR_EQ(res1->msg, "y wrong", "Message should match");

  ASSERT_INT_EQ(ledger.count, 1, "Should push exactly 1 failure to ledger");
  FailureEntry *entry = (FailureEntry *)vector_get(&ledger, 0);
  ASSERT_INT_EQ(entry->type, FAILURE_TEST_CASE,
                "Entry type should be FAILURE_TEST_CASE");
  ASSERT_STR_EQ(entry->file_path, "src/test_fail.c",
                "File path in ledger should match");
  ASSERT_INT_EQ(entry->test_case.line_num, 45, "Ledger line num should match");
  ASSERT_STR_EQ(entry->test_case.expr, "y == 20", "Ledger expr should match");
  ASSERT_STR_EQ(entry->test_case.msg, "y wrong", "Ledger msg should match");

  close(pipefds[0]);
  close(pipefds[1]);
  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// -- ctest_finalise_suite() tests --

// inactive slot guard:
// precondition: set slot->is_active to 0
// should return 0 immediately without performing waitpid or file descriptor
// cleanup

CTEST(SUITE_NAME, test_finalise_suite_unlaunched_slot) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  ASSERT_INT_EQ(
      ctest_finalise_suite(&slot, 0, &metrics, &ledger), 0,
      "ctest_finalise_suite() on inactive slot should return 0 immediately");

  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// normal clean suite exit:
// precondition: child process exited normally with code 0, duration <
// timeout_sec should reap process metrics->state should remain SUITE_DEFAULT
// pipe descriptor should be closed (slot->read_fd == -1)
// slot->is_active should be set to 0

CTEST(SUITE_NAME, test_finalise_suite_normal_suite_exit) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_passing_bin, &slot);

  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  // allow child process to finish writing to pipe
  usleep(10000);

  ctest_harvest_output(&slot, &metrics, &ledger);
  ctest_finalise_suite(&slot, 0, &metrics, &ledger);

  ASSERT_INT_EQ(slot.is_active, 0,
                "WorkerSlot.is_active should be 0 after finalisation");
  ASSERT_INT_EQ(slot.read_fd, -1,
                "WorkerSlot.read_fd should be -1 after closing pipe");
  ASSERT_INT_EQ(metrics.state, SUITE_DEFAULT,
                "SuiteState should remain SUITE_DEFAULT on normal exit");
  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// abnormal suite exit:
// precondition: child process terminated by signal
// metrics->state should be set to SUITE_CRASH
// a crash should be pushed to the failure_ledger
// slot->is_active should be set to 0

CTEST(SUITE_NAME, test_finalise_suite_abnormal_exit) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_crashing_bin, &slot);

  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  ctest_finalise_suite(&slot, 0, &metrics, &ledger);

  ASSERT_INT_EQ(
      slot.is_active, 0,
      "WorkerSlot.is_active should be 0 after finalising crashed child");
  ASSERT_INT_EQ(metrics.state, SUITE_CRASH,
                "SuiteState should be updated to SUITE_CRASH");

  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// timeout enforcement
// precondition: elapsed time (now - start_time) exceeds timeout_sec (or
// slot->timed_out == 1) SIGKILL should be sent to childprocess group
// (-slot->pid) metrics->state should be set to SUITE_TIMEOUT a timeout record
// should be pushed to the failure_ledger child should be reaped slot->is_active
// should be set to 0

CTEST(SUITE_NAME, test_finalise_suite_timeout_exit) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_passing_bin, &slot);

  // set time to now - 2 seconds
  clock_gettime(CLOCK_MONOTONIC, &slot.start_time);
  slot.start_time.tv_sec -= 2;

  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  // set timeout_sec to 1 second
  ctest_finalise_suite(&slot, 1, &metrics, &ledger);

  ASSERT_INT_EQ(slot.is_active, 0,
                "WorkerSlot.is_active should be 0 after timing out child");
  ASSERT_INT_EQ(slot.timed_out, 1,
                "WorkerSlot.timed_out flag should be set to 1");
  ASSERT_INT_EQ(metrics.state, SUITE_TIMEOUT,
                "SuiteState should be updated to SUITE_TIMEOUT");
  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// buffer flush on completion:
// precondition: slot->line_buf contains partial data not terminated by \n prior
// to suite completion remaining buffer content should be null-terminated and
// processed through line parser before process termination checks

CTEST(SUITE_NAME, test_finalise_suite_buf_flush_on_completion) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_passing_bin, &slot);

  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  char *buf_str = "this buffer is populated";
  snprintf(slot.line_buf, sizeof(slot.line_buf), "%s", buf_str);
  slot.buf_pos = strlen(buf_str);

  ctest_finalise_suite(&slot, 0, &metrics, &ledger);

  ASSERT_INT_EQ(slot.buf_pos, 0,
                "Buffer position should reset to 0 after finalisation flush");
  ASSERT_INT_EQ(slot.is_active, 0,
                "Slot should be marked inactive after finalisation");

  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// test crash entry pushes to ledger

CTEST(SUITE_NAME, test_finalise_suite_crash_ledger_inspection) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_crashing_bin, &slot);

  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  ctest_finalise_suite(&slot, 0, &metrics, &ledger);

  ASSERT_INT_EQ(ledger.count, 1, "Crash should add 1 entry to failure ledger");
  FailureEntry *entry = (FailureEntry *)vector_get(&ledger, 0);
  ASSERT_INT_EQ(entry->type, FAILURE_SUITE_CRASH,
                "Ledger entry type should be FAILURE_SUITE_CRASH");
  ASSERT_INT_EQ(entry->signal_num, SIGSEGV,
                "Ledger crash signal should be SIGSEGV");
  ASSERT_STR_EQ(entry->file_path, mock_crashing_bin,
                "Ledger entry file path should match bin path");

  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}

// test timeout entry pushes to ledger

CTEST(SUITE_NAME, test_finalise_suite_timeout_ledger_inspection) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_passing_bin, &slot);

  clock_gettime(CLOCK_MONOTONIC, &slot.start_time);
  slot.start_time.tv_sec -= 5;

  SuiteMetrics metrics = {0};
  ctest_suite_metrics_init(&metrics);
  Vector ledger;
  vector_init(&ledger, sizeof(FailureEntry));

  ctest_finalise_suite(&slot, 2, &metrics, &ledger);

  ASSERT_INT_EQ(ledger.count, 1,
                "Timeout should add 1 entry to failure ledger");
  FailureEntry *entry = (FailureEntry *)vector_get(&ledger, 0);
  ASSERT_INT_EQ(entry->type, FAILURE_SUITE_TIMEOUT,
                "Ledger entry type should be FAILURE_SUITE_TIMEOUT");
  ASSERT_INT_EQ(entry->timeout_sec, 2,
                "Ledger timeout_sec should match threshold");
  ASSERT_STR_EQ(entry->file_path, mock_passing_bin,
                "Ledger entry file path should match bin path");

  vector_free(&ledger);
  ctest_suite_metrics_cleanup(&metrics);
}
