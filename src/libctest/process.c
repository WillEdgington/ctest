#include <ctest/ctest.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int ctest_run_in_child(void (*func)(void *arg), void *arg,
                       CTestProcessResult *out_res) {
  if (func == NULL || out_res == NULL)
    return -1;

  memset(out_res, 0, sizeof(CTestProcessResult));

  // flush stdio streams prior to fork (prevent buf duplication in child)
  fflush(NULL);

  pid_t pid = fork();
  if (pid < 0)
    return -1;

  if (pid == 0) {
    func(arg);

    // if function does not exit the child, force exit (with status 0)
    _exit(0);
  }

  // wait for child process to complete
  int status = 0;
  if (waitpid(pid, &status, 0) < 0)
    return -1;

  if (WIFEXITED(status)) {
    out_res->exited_normally = true;
    out_res->exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    out_res->terminated_by_signal = true;
    out_res->term_signal = WTERMSIG(status);
  }

  return 0;
}
