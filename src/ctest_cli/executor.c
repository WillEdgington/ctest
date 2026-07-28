#define _GNU_SOURCE
#include "executor.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

SuiteMetrics ctest_execute_suite(const char *binary_path,
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

    FILE *stream = fdopen(pipefds[0], "r");
    if (stream != NULL) {
      char line[CTEST_MAX_LINE_LEN];
      while (fgets(line, sizeof(line), stream) != NULL) {
        char *fail_ptr = strstr(line, "FAIL|");
        char *summary_ptr = strstr(line, "SUMMARY|");

        if (fail_ptr != NULL) {
          if (vector_push(failure_ledger, fail_ptr) == -1) {
            fprintf(stderr, "ctest: failed to push to the failure ledger\n");
          }
        } else if (summary_ptr != NULL) {
          int runs, fails;
          if (sscanf(summary_ptr, "SUMMARY|%d|%d", &runs, &fails) == 2) {
            metrics.total_runs = (size_t)runs;
            metrics.total_failures = (size_t)fails;
          }
        }
      }
      fclose(stream);
    } else {
      close(pipefds[0]);
    }
    int status;
    waitpid(pid, &status, 0);

    if (WIFSIGNALED(status)) {
      metrics.crashed = 1;

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
  return metrics;
}
