#include "runner.h"

#define DEFAULT_DIR "./tests"

int main(void) {
  int status = ctest_run_session(DEFAULT_DIR);
  return status < 0 ? 2 : status;
}
