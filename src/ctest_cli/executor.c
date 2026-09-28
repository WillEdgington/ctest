#define _GNU_SOURCE
#include "executor.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int TMP_BUF_LEN = 512;

// 0 for pass, 1 for fail, -1 for error/not recognised
static int parse_ipc_fields(const char *line, char *file, size_t file_sz,
                            size_t *line_num, char *expr, size_t expr_sz,
                            char *msg, size_t msg_sz) {
  // PATTERN: <status><d><file><d><line-number><d><expression><d><message>
  int test_status;
  if (strncmp(line, "PASS" CTEST_TEST_DELIM, strlen("PASS" CTEST_TEST_DELIM)) ==
      0) {
    test_status = 0;
  } else if (strncmp(line, "FAIL" CTEST_TEST_DELIM,
                     strlen("FAIL" CTEST_TEST_DELIM)) == 0) {
    test_status = 1;
  } else {
    return -1;
  }

  const char *p = strchr(line, CTEST_TEST_DELIM[0]);
  p++;

  const char *p1 = strchr(p, CTEST_TEST_DELIM[0]);
  if (p1 == NULL)
    return -1;

  snprintf(file, file_sz, "%.*s", (int)(p1 - p), p);
  *line_num = (size_t)atoi(p1 + 1);
  const char *p2 = strchr(p1 + 1, CTEST_TEST_DELIM[0]);
  if (p2 == NULL)
    return -1;

  const char *p3 = strchr(p2 + 1, CTEST_TEST_DELIM[0]);
  if (p3 != NULL) {
    snprintf(expr, expr_sz, "%.*s", (int)(p3 - (p2 + 1)), p2 + 1);
    snprintf(msg, msg_sz, "%s", p3 + 1);
    msg[strcspn(msg, "\r\n")] = '\0';
  } else {
    snprintf(expr, expr_sz, "%s", p2 + 1);
    expr[strcspn(expr, "\r\n")] = '\0';
    msg[0] = '\0';
  }
  return test_status;
}

static void process_output_line(const char *line, SuiteMetrics *metrics,
                                Vector *failure_ledger) {
  FailureEntry fail_entry = {0};
  int test_status = parse_ipc_fields(
      line, fail_entry.file_path, sizeof(fail_entry.file_path),
      &fail_entry.test_case.line_num, fail_entry.test_case.expr,
      sizeof(fail_entry.test_case.expr), fail_entry.test_case.msg,
      sizeof(fail_entry.test_case.msg));

  if (test_status == -1) {
    return;
  }

  metrics->total_runs++;
  fail_entry.test_case.status = test_status == 0 ? TEST_PASS : TEST_FAIL;

  if (fail_entry.test_case.status == TEST_FAIL) {
    metrics->total_failures++;
    fail_entry.type = FAILURE_TEST_CASE;
    if (vector_push(failure_ledger, &fail_entry) != 0) {
      fprintf(stderr,
              "ctest: executor: failed to push to the failure ledger\n");
    }
  }

  if (vector_push(&metrics->test_results, &fail_entry.test_case) != 0) {
    fprintf(stderr,
            "ctest: executor: failed to push to the test results vector\n");
  }

  if (!metrics->file_path[0]) {
    strcpy(metrics->file_path, fail_entry.file_path);
  }
}

int ctest_launch_suite(const char *binary_path, WorkerSlot *slot) {
  int pipefds[2];
  if (pipe(pipefds) == -1) {
    perror("ctest: pipe creation failed");
    return -1;
  }

  // set pipefds to non-blocking
  int flags = fcntl(pipefds[0], F_GETFL, 0);
  if (fcntl(pipefds[0], F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("ctest: failed to set pipe fd to non-blocking");
    close(pipefds[0]);
    close(pipefds[1]);
    return -1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("ctest: fork failed");
    close(pipefds[0]);
    close(pipefds[1]);
    return -1;
  }

  if (pid == 0) {
    setpgid(0, 0);
    close(pipefds[0]);

    dup2(pipefds[1], STDOUT_FILENO);
    close(pipefds[1]);

    setenv("CTEST_RUNNER", "1", 1);

    char *args[] = {(char *)binary_path, NULL};
    execvp(binary_path, args);
    perror("ctest: execv failed");
    _exit(1);
  }

  close(pipefds[1]);

  slot->pid = pid;
  slot->read_fd = pipefds[0];
  slot->bin_path = binary_path;
  slot->buf_pos = 0;
  slot->is_active = 1;
  slot->timed_out = 0;
  clock_gettime(CLOCK_MONOTONIC, &slot->start_time);
  return 0;
}

int ctest_harvest_output(WorkerSlot *slot, SuiteMetrics *metrics,
                         Vector *failure_ledger) {
  if (slot->is_active == 0 || slot->read_fd < 0)
    return 0;

  int total_bytes_read = 0;
  char chunk[TMP_BUF_LEN];

  while (1) {
    ssize_t nbytes = read(slot->read_fd, chunk, sizeof(chunk));
    if (nbytes > 0) {
      total_bytes_read += nbytes;
      for (ssize_t i = 0; i < nbytes; i++) {
        char c = chunk[i];
        if (slot->buf_pos < sizeof(slot->line_buf) - 1) {
          slot->line_buf[slot->buf_pos++] = c;
          if (c == '\n') {
            slot->line_buf[slot->buf_pos] = '\0';
            process_output_line(slot->line_buf, metrics, failure_ledger);
            slot->buf_pos = 0;
          }
        }
      }
    } else if (nbytes == 0) {
      // EOF reached
      return (total_bytes_read > 0) ? total_bytes_read : 0;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // no more data currently available in buffer
        return (total_bytes_read > 0) ? total_bytes_read : -1;
      }
      return -1; // read error
    }
  }
}

int ctest_finalise_suite(WorkerSlot *slot, unsigned int timeout_sec,
                         SuiteMetrics *out_metrics, Vector *failure_ledger) {
  if (slot->is_active == 0)
    return 0;

  if (slot->buf_pos > 0) {
    slot->line_buf[slot->buf_pos] = '\0';
    process_output_line(slot->line_buf, out_metrics, failure_ledger);
    slot->buf_pos = 0;
  }

  if (slot->read_fd >= 0) {
    close(slot->read_fd);
    slot->read_fd = -1;
  }

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  unsigned int elapsed_sec =
      (unsigned int)(now.tv_sec - slot->start_time.tv_sec);
  if ((timeout_sec > 0 && elapsed_sec >= timeout_sec) || slot->timed_out)
    slot->timed_out = 1;

  if (slot->timed_out == 1) {
    // format timeout
    out_metrics->state = SUITE_TIMEOUT;
    kill(-slot->pid, SIGKILL);

    // create FailureEntry structure for timeout
    FailureEntry timeout_entry = {0};
    timeout_entry.type = FAILURE_SUITE_TIMEOUT;
    timeout_entry.timeout_sec = timeout_sec;
    strcpy(timeout_entry.file_path, slot->bin_path);

    if (vector_push(failure_ledger, &timeout_entry) != 0)
      fprintf(stderr,
              "ctest: executor: failed to push to the failure ledger\n");

    int status;
    waitpid(slot->pid, &status, 0);
  } else {
    // wait for child process to finish
    int status;
    waitpid(slot->pid, &status, 0);
    if (WIFSIGNALED(status)) {
      // format crash
      out_metrics->state = SUITE_CRASH;

      // create FailureEntry structure for crash
      FailureEntry crash_entry = {0};
      crash_entry.type = FAILURE_SUITE_CRASH;
      crash_entry.signal_num = WTERMSIG(status);
      strcpy(crash_entry.file_path, slot->bin_path);

      if (vector_push(failure_ledger, &crash_entry) != 0)
        fprintf(stderr,
                "ctest: executor: failed to push to the failure ledger\n");
    }
  }

  slot->is_active = 0;
  return 0;
}

int ctest_suite_metrics_init(SuiteMetrics *metrics) {
  memset(metrics, 0, sizeof(SuiteMetrics));
  if (vector_init(&metrics->test_results, sizeof(TestCaseResult)) != 0) {
    fprintf(stderr, "ctest: failed to initialise vector for test results\n");
    return -1;
  }
  return 0;
}

void ctest_suite_metrics_cleanup(SuiteMetrics *metrics) {
  vector_free(&metrics->test_results);
  memset(metrics, 0, sizeof(SuiteMetrics));
}
