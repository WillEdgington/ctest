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

SuiteMetrics ctest_execute_suite(const char *binary_path,
                                 unsigned int timeout_sec,
                                 CTestVerbosity verbosity,
                                 Vector *failure_ledger) {
  SuiteMetrics metrics = {0};
  int pipefds[2];
  if (pipe(pipefds) == -1) {
    perror("ctest: pipe creation failed");
    return metrics;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("ctest: fork failed");
    close(pipefds[0]);
    close(pipefds[1]);
    return metrics;
  }

  if (pid == 0) {
    setpgid(0, 0);

    close(pipefds[0]);
    if (dup2(pipefds[1], STDOUT_FILENO) == -1) {
      perror("ctest: dup2 failed");
      _exit(1);
    }
    close(pipefds[1]);

    setenv("CTEST_RUNNER", "1", 1);
    if (verbosity == CTEST_VERBOSITY_VERBOSE)
      setenv("CTEST_VERBOSE", "1", 1);

    char *args[] = {(char *)binary_path, NULL};
    execvp(binary_path, args);
    perror("ctest: execv failed");
    _exit(1);
  } else {
    close(pipefds[1]);

    struct pollfd pfd = {.fd = pipefds[0], .events = POLLIN};
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    char line_buf[CTEST_MAX_LINE_LEN];
    size_t buf_pos = 0;
    int timed_out = 0;

    while (1) {
      int timeout_ms = -1; // block indefinitely (-1)

      if (timeout_sec > 0) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start_time.tv_sec) * 1000L +
                          (now.tv_nsec - start_time.tv_nsec) / 1000000L;
        long remaining_ms = (long)(timeout_sec * 1000L) - elapsed_ms;

        if (remaining_ms <= 0) {
          timed_out = 1;
          break;
        }
        timeout_ms = (int)remaining_ms;
      }

      int poll_res = poll(&pfd, 1, timeout_ms);
      if (poll_res < 0) {
        if (errno == EINTR)
          continue;
        perror("ctest: poll failed");
        break;
      }
      if (poll_res == 0) {
        timed_out = 1;
        break;
      }

      if (pfd.revents & POLLIN) {
        char chunk[TMP_BUF_LEN];
        ssize_t nbytes = read(pipefds[0], chunk, sizeof(chunk));
        if (nbytes > 0) {
          for (ssize_t i = 0; i < nbytes; i++) {
            char c = chunk[i];
            if (buf_pos < sizeof(line_buf) - 1) {
              line_buf[buf_pos++] = c;
            }
            if (c == '\n') {
              line_buf[buf_pos] = '\0';
              process_output_line(line_buf, &metrics, failure_ledger,
                                  verbosity);
              buf_pos = 0;
            }
          }
        } else if (nbytes == 0) {
          break;
        } else {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
          break;
        }
      } else if (pfd.revents & (POLLHUP | POLLERR)) {
        break;
      }
    }

    if (buf_pos > 0) {
      line_buf[buf_pos] = '\0';
      process_output_line(line_buf, &metrics, failure_ledger, verbosity);
    }

    close(pipefds[0]);

    if (timed_out == 1) {
      metrics.state = SUITE_TIMEOUT;
      kill(-pid, SIGKILL);

      char timeout_msg[CTEST_MAX_LINE_LEN];
      snprintf(timeout_msg, sizeof(timeout_msg),
               "  " CTEST_COLOR_CYAN "[TIME]" CTEST_COLOR_RESET
               " Suite execution timed out\n"
               "         Limit     : Exceeded %u second(s) threshold\n"
               "         Location  : %s\n",
               timeout_sec, binary_path);

      if (vector_push(failure_ledger, timeout_msg) == -1)
        fprintf(stderr, "ctest: failed to push to the failure ledger\n");

      int status;
      waitpid(pid, &status, 0);
    } else {
      int status;
      waitpid(pid, &status, 0);

      if (WIFSIGNALED(status)) {
        metrics.state = SUITE_CRASH;

        char crash_msg[CTEST_MAX_LINE_LEN];
        snprintf(crash_msg, sizeof(crash_msg),
                 "  " CTEST_COLOR_YELLOW "[CRASH]" CTEST_COLOR_RESET
                 " Suite execution terminated unexpectedly\n"
                 "         Signal    : Terminated by signal %d\n"
                 "         Location  : %s\n",
                 WTERMSIG(status), binary_path);
        if (vector_push(failure_ledger, crash_msg) == -1)
          fprintf(stderr, "ctest: failed to push to the failure ledger\n");
      }
    }
  }

  return metrics;
}
