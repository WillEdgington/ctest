#define _GNU_SOURCE
#include "executor.h"
#include "config.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static size_t TMP_BUF_LEN = 256;

static void parse_ipc_fields(const char *line, char *file, size_t file_sz,
                             int *line_num, char *expr, size_t expr_sz,
                             char *msg, size_t msg_sz) {
  // PATTERN: <status><d><file><d><line-number><d><expression><d><message>
  const char *p = strchr(line, CTEST_TEST_DELIM[0]);
  if (p == NULL)
    return;
  p++;

  const char *p1 = strchr(p, CTEST_TEST_DELIM[0]);
  if (p1 != NULL) {
    snprintf(file, file_sz, "%.*s", (int)(p1 - p), p);
    *line_num = atoi(p1 + 1);
    const char *p2 = strchr(p1 + 1, CTEST_TEST_DELIM[0]);
    if (p2 != NULL) {
      const char *p3 = strchr(p2 + 1, CTEST_TEST_DELIM[0]);
      if (p3 != NULL) {
        snprintf(expr, expr_sz, "%.*s", (int)(p3 - (p2 + 1)), p2 + 1);
        snprintf(msg, msg_sz, "%s", p3 + 1);
        msg[strcspn(msg, "\r\n")] = '\0';
      } else {
        snprintf(expr, expr_sz, "%s", p2 + 1);
        expr[strcspn(expr, "\r\n")] = '\0';
      }
    }
  }
}

static void process_output_line(const char *line, SuiteMetrics *metrics,
                                Vector *failure_ledger,
                                CTestVerbosity verbosity) {
  /*
  --verbose assertion print:
    [<status>] <message> (expression) | Line <line-number> in <file>

  Failure ledger print:
    [FAIL] <message|expression>
           Expression: <expression>
           Location  : Line <line-number> in <file>
  */
  if (strncmp(line, "PASS" CTEST_TEST_DELIM, 5) == 0 &&
      verbosity == CTEST_VERBOSITY_VERBOSE) {
    char file[TMP_BUF_LEN], expr[TMP_BUF_LEN], msg[TMP_BUF_LEN];
    int line_num = 0;
    parse_ipc_fields(line, file, sizeof(file), &line_num, expr, sizeof(expr),
                     msg, sizeof(msg));

    printf("  " CTEST_COLOR_GREEN "[PASS]" CTEST_COLOR_RESET
           " %s (%s) | Line %d in %s\n",
           msg[0] ? msg : "---", expr, line_num, file);
  } else if (strncmp(line, "FAIL" CTEST_TEST_DELIM, 5) == 0) {
    char file[TMP_BUF_LEN], expr[TMP_BUF_LEN], msg[TMP_BUF_LEN];
    int line_num = 0;
    parse_ipc_fields(line, file, sizeof(file), &line_num, expr, sizeof(expr),
                     msg, sizeof(msg));

    if (verbosity == CTEST_VERBOSITY_VERBOSE)
      printf("  " CTEST_COLOR_RED "[FAIL]" CTEST_COLOR_RESET
             " %s (%s) | Line %d in %s\n",
             msg[0] ? msg : "---", expr, line_num, file);

    char formatted_fail[CTEST_MAX_LINE_LEN];
    snprintf(formatted_fail, sizeof(formatted_fail),
             "  " CTEST_COLOR_RED "[FAIL]" CTEST_COLOR_RESET " %s\n"
             "         Expression: %s\n"
             "         Location  : Line %d in %s\n",
             msg[0] ? msg : expr, expr, line_num, file);

    if (vector_push(failure_ledger, formatted_fail) == -1)
      fprintf(stderr, "ctest: failed to push to the failure ledger\n");
  } else if (strncmp(line, "SUMMARY" CTEST_TEST_DELIM, 8) == 0) {
    int runs, fails;
    if (sscanf(line, "SUMMARY" CTEST_TEST_DELIM "%d" CTEST_TEST_DELIM "%d",
               &runs, &fails) == 2) {
      metrics->total_runs = (size_t)runs;
      metrics->total_failures = (size_t)fails;
    }
  }
}

int ctest_launch_suite(const char *binary_path, CTestVerbosity verbosity,
                       WorkerSlot *slot) {
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
    if (verbosity == CTEST_VERBOSITY_VERBOSE)
      setenv("CTEST_VERBOSE", "1", 1);

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

int ctest_harvest_output(WorkerSlot *slot, CTestVerbosity verbosity,
                         SuiteMetrics *metrics, Vector *failure_ledger) {
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
            process_output_line(slot->line_buf, metrics, failure_ledger,
                                verbosity);
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
                         CTestVerbosity verbosity, SuiteMetrics *out_metrics,
                         Vector *failure_ledger) {
  if (slot->is_active == 0)
    return 0;

  if (slot->buf_pos > 0) {
    slot->line_buf[slot->buf_pos] = '\0';
    process_output_line(slot->line_buf, out_metrics, failure_ledger, verbosity);
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

    char timeout_msg[CTEST_MAX_LINE_LEN];
    snprintf(timeout_msg, sizeof(timeout_msg),
             "  " CTEST_COLOR_CYAN "[TIME]" CTEST_COLOR_RESET
             " Suite execution timed out\n"
             "         Limit     : Exceeded %u second(s) threshold\n"
             "         Location  : %s\n",
             timeout_sec, slot->bin_path);

    if (vector_push(failure_ledger, timeout_msg) == -1)
      fprintf(stderr, "ctest: failed to push to the failure ledger\n");

    int status;
    waitpid(slot->pid, &status, 0);
  } else {
    // wait for child process to finish
    int status;
    waitpid(slot->pid, &status, 0);
    if (WIFSIGNALED(status)) {
      // format crash
      out_metrics->state = SUITE_CRASH;

      char crash_msg[CTEST_MAX_LINE_LEN];
      snprintf(crash_msg, sizeof(crash_msg),
               "  " CTEST_COLOR_YELLOW "[CRASH]" CTEST_COLOR_RESET
               " Suite execution terminated unexpectedly\n"
               "         Signal    : Terminated by signal %d\n"
               "         Location  : %s\n",
               WTERMSIG(status), slot->bin_path);
      if (vector_push(failure_ledger, crash_msg) == -1)
        fprintf(stderr, "ctest: failed to push to the failure ledger\n");
    }
  }

  slot->is_active = 0;
  return 0;
}

SuiteMetrics ctest_execute_suite(const char *binary_path,
                                 unsigned int timeout_sec,
                                 CTestVerbosity verbosity,
                                 Vector *failure_ledger) {
  SuiteMetrics metrics = {0};
  WorkerSlot slot = {0};

  if (ctest_launch_suite(binary_path, verbosity, &slot) != 0)
    return metrics;

  struct pollfd pfd = {.fd = slot.read_fd, .events = POLLIN};
  struct timespec now;

  while (slot.is_active) {
    int poll_timeout = -1; // infinite wait by default if timeout_sec == 0

    if (timeout_sec > 0) {
      clock_gettime(CLOCK_MONOTONIC, &now);
      long elapsed_ms = (now.tv_sec - slot.start_time.tv_sec) * 1000L +
                        (now.tv_nsec - slot.start_time.tv_nsec) / 1000000L;
      long remaining_ms = (long)(timeout_sec * 1000L) - elapsed_ms;

      if (remaining_ms <= 0) {
        slot.timed_out = 1;
        break;
      }
      poll_timeout = (int)remaining_ms;
    }

    int poll_res = poll(&pfd, 1, poll_timeout);

    if (poll_res < 0) {
      if (errno == EINTR)
        continue;
      break;
    }

    if (poll_res == 0) {
      slot.timed_out = 1;
      break;
    }

    // process events when POLLIN, POLLHUP, or POLLERR occur
    if (pfd.revents & (POLLIN | POLLHUP | POLLERR)) {
      int bytes =
          ctest_harvest_output(&slot, verbosity, &metrics, failure_ledger);
      if (bytes == 0) // EOF confirmed
        break;
    }
  }

  ctest_finalise_suite(&slot, timeout_sec, verbosity, &metrics, failure_ledger);
  return metrics;
}
