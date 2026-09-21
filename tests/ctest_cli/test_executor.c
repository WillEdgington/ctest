#define _GNU_SOURCE
#include "ctest_cli/executor.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SUITE_NAME test_executor

static const char *mock_passing_bin = "./mock_passing_bin_35893333";
static const char *mock_passing_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "5" CTEST_TEST_DELIM "0\\n\");\n"
    "  return 0;\n"
    "}\n";

static const char *mock_failing_bin = "./mock_failing_bin_78677545";
static const char *mock_failing_c_code =
    "#include <stdio.h>\n"
    "int main(void) {\n"
    "  printf(\"FAIL" CTEST_TEST_DELIM "<file>" CTEST_TEST_DELIM
    "12" CTEST_TEST_DELIM "<expression>" CTEST_TEST_DELIM "<message>\\n\");\n"
    "  printf(\"SUMMARY" CTEST_TEST_DELIM "2" CTEST_TEST_DELIM "1\\n\");\n"
    "  return 0;\n"
    "}\n";

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
      ctest_launch_suite(mock_passing_bin, CTEST_VERBOSITY_NORMAL, &slot), 0,
      "ctest_launch_suite() call with valid binary path should return 0 "
      "(success)");
  ASSERT_INT_EQ(slot.is_active, 1,
                "WorkerSlot.is_active should be 1 after successful launch");
  ASSERT(
      slot.pid >= 0,
      "WorkerSlot.pid should be a positive process ID after successful launch");
  ASSERT(slot.read_fd >= 0, "WorkerSlot.read_fd should be a valid file "
                            "descriptor after successful launch");
  ASSERT_STR_EQ(slot.bin_path, mock_passing_bin,
                "WorkerSlot.bin_path should be equal to the input binary_path "
                "after successful launch");
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
  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  int bytes_read =
      ctest_harvest_output(&slot, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT_INT_EQ(bytes_read, 0,
                "ctest_harvest_output() call on unlaunched slot should return "
                "0 immediately");
  ASSERT_INT_EQ((int)metrics.total_runs, 0,
                "unlaunched slot harvest should not alter suite metrics");

  slot.read_fd = -1;
  bytes_read =
      ctest_harvest_output(&slot, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT_INT_EQ(
      bytes_read, 0,
      "ctest_harvest_output() call on closed fd should return 0 immediately");
  ASSERT_INT_EQ(metrics.total_runs, 0,
                "slot harvest with closed fd should not alter suite metrics");

  vector_free(&ledger);
}

// non-blocking partial & empty reads (EAGAIN/EWOULDBLOCK):
// precondition: active slot, pipe has no data available at call time
// should return -1 (or 0 if EOF) without blocking

CTEST(SUITE_NAME, test_harvest_output_non_blocking_partial_reads) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};

  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  int pipefds[2];
  pipe(pipefds);

  int flags = fcntl(pipefds[0], F_GETFL, 0);
  fcntl(pipefds[0], F_SETFL, flags | O_NONBLOCK);

  slot.is_active = 1;
  slot.read_fd = pipefds[0];

  int res =
      ctest_harvest_output(&slot, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT_INT_EQ(res, -1,
                "ctest_harvest_output() should return -1 on EAGAIN when pipe "
                "has no data");

  close(pipefds[0]);
  close(pipefds[1]);
  vector_free(&ledger);
}

CTEST(SUITE_NAME, test_harvest_output_empty_reads) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};

  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  int pipefds[2];
  pipe(pipefds);

  int flags = fcntl(pipefds[0], F_GETFL, 0);
  fcntl(pipefds[0], F_SETFL, flags | O_NONBLOCK);

  // close write end to simulate EOF
  close(pipefds[1]);

  slot.is_active = 1;
  slot.read_fd = pipefds[0];

  int res =
      ctest_harvest_output(&slot, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT_INT_EQ(res, 0, "ctest_harvest_output() should return 0 on EOF");

  close(pipefds[0]);
  vector_free(&ledger);
}

// complete line stream processing:
// precondition: pipe stream contains fully framed PASSED, FAIL or SUMMARY
// protocol lines ending in \n SUMMARY should update metrics->total_runs and
// metrics->total_failures FAIL should push to failure_ledger should return
// total count of bytes processed during read iteration

CTEST(SUITE_NAME, test_harvest_output_complete_line_stream) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_failing_bin, CTEST_VERBOSITY_NORMAL, &slot);

  // let it run
  int status;
  waitpid(slot.pid, &status, 0);

  SuiteMetrics metrics = {0};
  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  int nbytes =
      ctest_harvest_output(&slot, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT(nbytes > 0, "ctest_harvest_output() should read bytes from executed "
                     "test suite output");
  ASSERT_INT_EQ(
      metrics.total_runs, 2,
      "SuiteMetrics.total_runs should match the number of test runs in suite");
  ASSERT_INT_EQ(metrics.total_failures, 1,
                "SuiteMetrics.total_failures should match the number of test "
                "failures in suite");
  ASSERT_INT_EQ(ledger.count, metrics.total_failures,
                "Item count for failure ledger should be equal to total fails");

  close(slot.read_fd);
  vector_free(&ledger);
}

// partial line buffering across harvest calls:
// precondition: pipe stream delivers a protocol line in chunks
// first invocation buffers incomplete line in slot->line_buf (without updating
// metrics) second invocation completes line, triggers parser, updates metrics,
// and rests slot->buf_pos to 0

CTEST(SUITE_NAME, test_harvest_output_partial_line_buffering) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};

  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  int pipefds[2];
  pipe(pipefds);

  int flags = fcntl(pipefds[0], F_GETFL, 0);
  fcntl(pipefds[0], F_SETFL, flags | O_NONBLOCK);

  slot.is_active = 1;
  slot.read_fd = pipefds[0];

  char *chunk1 = "this line is incomplete...";
  write(pipefds[1], chunk1, strlen(chunk1));

  int nbytes =
      ctest_harvest_output(&slot, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);
  ASSERT_INT_EQ(nbytes, strlen(chunk1),
                "ctest_harvest_output() should return the correct number of "
                "bytes harvested");
  ASSERT_INT_EQ(slot.buf_pos, strlen(chunk1),
                "WorkerSlot.buf_pos should equal the current line size when "
                "line incomplete");

  char *chunk2 = " Not anymore.\n";
  write(pipefds[1], chunk2, strlen(chunk2));

  nbytes =
      ctest_harvest_output(&slot, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);
  ASSERT_INT_EQ(slot.buf_pos, 0,
                "WorkerSlot.buf_pos should be reset to 0 once line completes");

  close(pipefds[0]);
  close(pipefds[1]);
  vector_free(&ledger);
}

// -- ctest_finalise_suite() tests --

// inactive slot guard:
// precondition: set slot->is_active to 0
// should return 0 immediately without performing waitpid or file descriptor
// cleanup

CTEST(SUITE_NAME, test_finalise_suite_unlaunched_slot) {
  WorkerSlot slot = {0};
  SuiteMetrics metrics = {0};
  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  ASSERT_INT_EQ(
      ctest_finalise_suite(&slot, 0, CTEST_VERBOSITY_NORMAL, &metrics, &ledger),
      0,
      "ctest_finalise_suite() call with inactive slot input should return 0 "
      "immediately");

  vector_free(&ledger);
}

// normal clean suite exit:
// precondition: child process exited normally with code 0, duration <
// timeout_sec should reap process metrics->state should remain SUITE_DEFAULT
// pipe descriptor should be closed (slot->read_fd == -1)
// slot->is_active should be set to 0

CTEST(SUITE_NAME, test_finalise_suite_normal_suite_exit) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_passing_bin, CTEST_VERBOSITY_NORMAL, &slot);

  SuiteMetrics metrics = {0};
  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  // allow child process to finish writing to pipe
  usleep(10000);

  ctest_harvest_output(&slot, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);
  ctest_finalise_suite(&slot, 0, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT_INT_EQ(slot.is_active, 0,
                "WorkerSlot.is_active should be 0 after finalisation");
  ASSERT_INT_EQ(slot.read_fd, -1,
                "WorkerSlot.read_fd should be -1 after closing pipe");
  ASSERT_INT_EQ(metrics.state, SUITE_DEFAULT,
                "SuiteState should remain SUITE_DEFAULT on normal exit");
  vector_free(&ledger);
}

// abnormal suite exit:
// precondition: child process terminated by signal
// metrics->state should be set to SUITE_CRASH
// a crash should be pushed to the failure_ledger
// slot->is_active should be set to 0

CTEST(SUITE_NAME, test_finalise_suite_abnormal_exit) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_crashing_bin, CTEST_VERBOSITY_NORMAL, &slot);

  SuiteMetrics metrics = {0};
  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  int status;
  waitpid(slot.pid, &status, 0);

  ctest_finalise_suite(&slot, 0, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT_INT_EQ(
      slot.is_active, 0,
      "WorkerSlot.is_active should be 0 after finalising crashed child");
  ASSERT_INT_EQ(metrics.state, SUITE_CRASH,
                "SuiteState should be updated to SUITE_CRASH");
  vector_free(&ledger);
}

// timeout enforcement
// precondition: elapsed time (now - start_time) exceeds timeout_sec (or
// slot->timed_out == 1) SIGKILL should be sent to childprocess group
// (-slot->pid) metrics->state should be set to SUITE_TIMEOUT a timeout record
// should be pushed to the failure_ledger child should be reaped slot->is_active
// should be set to 0

CTEST(SUITE_NAME, test_finalise_suite_timeout_exit) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_passing_bin, CTEST_VERBOSITY_NORMAL, &slot);

  // set time to now - 2 seconds
  clock_gettime(CLOCK_MONOTONIC, &slot.start_time);
  slot.start_time.tv_sec -= 2;

  SuiteMetrics metrics = {0};
  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  // set timeout_sec to 1 second
  ctest_finalise_suite(&slot, 1, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT_INT_EQ(slot.is_active, 0,
                "WorkerSlot.is_active should be 0 after timing out child");
  ASSERT_INT_EQ(slot.timed_out, 1,
                "WorkerSlot.timed_out flag should be set to 1");
  ASSERT_INT_EQ(metrics.state, SUITE_TIMEOUT,
                "SuiteState should be updated to SUITE_TIMEOUT");
  vector_free(&ledger);
}

// buffer flush on completion:
// precondition: slot->line_buf contains partial data not terminated by \n prior
// to suite completion remaining buffer content should be null-terminated and
// processed through line parser before process termination checks

CTEST(SUITE_NAME, test_finalise_suite_buf_flush_on_completion) {
  WorkerSlot slot = {0};
  ctest_launch_suite(mock_passing_bin, CTEST_VERBOSITY_NORMAL, &slot);

  SuiteMetrics metrics = {0};
  Vector ledger;
  vector_init(&ledger, sizeof(char *));

  char *buf_str = "this buffer is populated";
  snprintf(slot.line_buf, sizeof(slot.line_buf), "%s", buf_str);
  slot.buf_pos = strlen(buf_str);

  ctest_finalise_suite(&slot, 0, CTEST_VERBOSITY_NORMAL, &metrics, &ledger);

  ASSERT_INT_EQ(slot.buf_pos, 0,
                "Buffer position should reset to 0 after finalisation flush");
  ASSERT_INT_EQ(slot.is_active, 0,
                "Slot should be marked inactive after finalisation");

  vector_free(&ledger);
}

// -- ctest_suite_metrics_cleanup() --

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
                "SuiteMetrics.total_runs should reset after "
                "ctest_suite_metrics_cleanup() call");
  ASSERT_INT_EQ(metrics.total_failures, 0,
                "SuiteMetrics.total_failures should reset after "
                "ctest_suite_metrics_cleanup() call");
  ASSERT_INT_EQ(metrics.test_results.count, 0,
                "SuiteMetrics.test_results.count should reset after "
                "ctest_suite_metrics_cleanup() call");
  ASSERT_PTR_NULL(metrics.test_results.items,
                  "SuiteMetrics.test_results.items should be NULL after "
                  "ctest_suite_metrics_cleanup() call");
}
