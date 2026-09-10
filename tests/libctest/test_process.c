#include <ctest/ctest.h>
#include <signal.h>
#include <stdlib.h>

#define SUITE_NAME test_process

static void child_func_normal(void *arg) { (void)arg; }
static void child_func_non_zero_exit(void *arg) {
  (void)arg;
  exit(42);
}
static void child_func_segv_termination(void *arg) {
  (void)arg;
  // stop ASan intercepting SIGSEGV in child
  signal(SIGSEGV, SIG_DFL);

  raise(SIGSEGV);
}

static void child_func_abort_termination(void *arg) {
  (void)arg;

  // stop ASan intercepting SIGABRT in child
  signal(SIGABRT, SIG_DFL);

  abort();
}

static int g_test_memory_var = 100;

static void child_func_mutate_memory(void *arg) {
  (void)arg;
  g_test_memory_var = 999;
}

// ctest_run_in_child() tests

// NULL func pointer, returns -1
// NULL out_res struct, returns -1

CTEST(SUITE_NAME, test_run_in_child_null_func) {
  CTestProcessResult out_res;
  ASSERT_INT_EQ(ctest_run_in_child(NULL, NULL, &out_res), -1,
                "ctest_run_in_child() call should return -1 (fail) if function "
                "pointer is NULL");
}

CTEST(SUITE_NAME, test_run_in_child_null_out_res) {
  ASSERT_INT_EQ(ctest_run_in_child(child_func_normal, NULL, NULL), -1,
                "ctest_run_in_child() call should return -1 (fail) if process "
                "out_res pointer is NULL");
}

// func returns normally:
// out_res->exited_normally == true
// out_res->exit_code == 0
// out_res->terminated_by_signal == false

CTEST(SUITE_NAME, test_run_in_child_normal_exit) {
  CTestProcessResult out_res;
  ASSERT_INT_EQ(ctest_run_in_child(child_func_normal, NULL, &out_res), 0,
                "ctest_run_in_child() call should return 0 (success) using a "
                "standard function (expected exit 0)");
  ASSERT(out_res.exited_normally == true,
         "out_res.exited_normally should be true after running function with "
         "expected normal exit as child process");
  ASSERT_INT_EQ(out_res.exit_code, 0,
                "out_res.exit_code should be 0 for after running function with "
                "expected normal exit as a child process");
  ASSERT(out_res.terminated_by_signal == false,
         "out_res.terminated_by_signal should be false after running function "
         "not expected to terminate");
}

// func calls exit(42):
// out_res->exited_normally == true
// out_res->exit_code == 42
// out_res->terminated_by_signal == false

CTEST(SUITE_NAME, test_run_in_child_non_zero_exit) {
  CTestProcessResult out_res;
  ASSERT_INT_EQ(ctest_run_in_child(child_func_non_zero_exit, NULL, &out_res), 0,
                "ctest_run_in_child() call should return 0 (success) using a "
                "function that has a non-zero exit");
  ASSERT(out_res.exited_normally == true,
         "out_res.exited_normally should be true after running function with "
         "non-zero exit");
  ASSERT_INT_EQ(out_res.exit_code, 42,
                "out_res.exit_code should be equal to the expected exit code "
                "of the function ran (42)");
  ASSERT(out_res.terminated_by_signal == false,
         "out_res.terminated_by_signal should be false after running function "
         "with non-zero exit");
}

// func calls raise(SIGSEGV):
// out_res->exited_normally == false
// out_res->terminated_by_signal == true
// out_res->term_signal == SIGSEGV

CTEST(SUITE_NAME, test_run_in_child_segv_termination) {
  CTestProcessResult out_res;
  ASSERT_INT_EQ(ctest_run_in_child(child_func_segv_termination, NULL, &out_res),
                0,
                "ctest_run_in_child() call should return 0 (success) using a "
                "function that terminates (SIGSEGV)");
  ASSERT(out_res.terminated_by_signal == true,
         "out_res.terminated_by_signal should be true after running function "
         "that terminates");
  ASSERT(out_res.term_signal == SIGSEGV,
         "out_res.term_signal should be SIGSEGV after running function that "
         "raises SIGSEGV");
  ASSERT(out_res.exited_normally == false,
         "out_res.exited_normally should be false after running function that "
         "terminates");
}

// func calls abort():
// out_res->exited_normally == false
// out_res->terminated_by_signal == true
// out_res->term_signal == SIGABRT

CTEST(SUITE_NAME, test_run_in_child_abort_termination) {
  CTestProcessResult out_res;
  ASSERT_INT_EQ(
      ctest_run_in_child(child_func_abort_termination, NULL, &out_res), 0,
      "ctest_run_in_child() call should return 0 (success) using a function "
      "that aborts (SIGABRT)");
  ASSERT(out_res.terminated_by_signal == true,
         "out_res.terminated_by_signal should be true after running function "
         "that aborts");
  ASSERT(out_res.term_signal == SIGABRT,
         "out_res.term_signal should be SIGABRT after running function that "
         "raises SIGABRT");
  ASSERT(out_res.exited_normally == false,
         "out_res.exited_normally should be false after running function that "
         "terminates");
}

// func mutates global variable:
// parent process variable should remain unchanged

CTEST(SUITE_NAME, test_run_in_child_memory_isolation) {
  CTestProcessResult out_res;
  g_test_memory_var = 100;

  ctest_run_in_child(child_func_mutate_memory, NULL, &out_res);
  ASSERT_INT_EQ(g_test_memory_var, 100,
                "Parent global variable should remain unchanged after child "
                "process mutation");
}

static void child_func_read_arg_and_exit(void *arg) {
  if (arg == NULL) {
    _exit(1);
  }

  int val = *(int *)arg;
  _exit(val);
}

CTEST(SUITE_NAME, test_run_in_child_arg_input_passes_through) {
  CTestProcessResult out_res;
  int input_val = 35;

  ctest_run_in_child(child_func_read_arg_and_exit, &input_val, &out_res);

  ASSERT_INT_EQ(out_res.exit_code, 35,
                "Child process should receive input_val via arg and exit with "
                "status equal to input_val (35)");
}
