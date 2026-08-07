#define _GNU_SOURCE
#include "executor.h"
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

static void process_output_line(const char *line, SuiteMetrics *metrics,
                                Vector *failure_ledger) {
  char *fail_ptr = strstr(line, "FAIL|");
  char *summ_ptr = strstr(line, "SUMMARY|");

  if (fail_ptr != NULL) {
    if (vector_push(failure_ledger, fail_ptr) == -1) {
      fprintf(stderr, "ctest: failed to push to the failure ledger\n");
    }
  } else if (summ_ptr != NULL) {
    int runs, fails;
    if (sscanf(summ_ptr, "SUMMARY|%d|%d", &runs, &fails) == 2) {
      metrics->total_runs = (size_t)runs;
      metrics->total_failures = (size_t)fails;
    }
  }
}

SuiteMetrics ctest_execute_suite(const char *binary_path,
                                 unsigned int timeout_sec,
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
        char chunk[256];
        ssize_t nbytes = read(pipefds[0], chunk, sizeof(chunk));
        if (nbytes > 0) {
          for (ssize_t i = 0; i < nbytes; i++) {
            char c = chunk[i];
            if (buf_pos < sizeof(line_buf) - 1) {
              line_buf[buf_pos++] = c;
            }
            if (c == '\n') {
              line_buf[buf_pos] = '\0';
              process_output_line(line_buf, &metrics, failure_ledger);
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
      process_output_line(line_buf, &metrics, failure_ledger);
    }

    close(pipefds[0]);

    if (timed_out == 1) {
      metrics.state = SUITE_TIMEOUT;

      kill(-pid, SIGKILL);

      char timeout_msg[CTEST_MAX_LINE_LEN];
      snprintf(
          timeout_msg, sizeof(timeout_msg),
          CTEST_COLOR_CYAN
          "TIMEOUT|%s|Execution timed out after %u second(s)" CTEST_COLOR_RESET
          "\n",
          binary_path, timeout_sec);

      if (vector_push(failure_ledger, timeout_msg) == -1) {
        fprintf(stderr, "ctest: failed to push to the failure ledger\n");
      }

      int status;
      waitpid(pid, &status, 0);
    } else {
      int status;
      waitpid(pid, &status, 0);

      if (WIFSIGNALED(status)) {
        metrics.state = SUITE_CRASH;

        char crash_msg[CTEST_MAX_LINE_LEN];
        snprintf(crash_msg, CTEST_MAX_LINE_LEN,
                 CTEST_COLOR_YELLOW
                 "CRASH|%s|Terminated by signal %d" CTEST_COLOR_RESET "\n",
                 binary_path, WTERMSIG(status));
        if (vector_push(failure_ledger, crash_msg) == -1) {
          fprintf(stderr, "ctest: failed to push to the failure ledger\n");
        }
      }
    }
  }

  return metrics;
}
