#ifndef CTEST_H
#define CTEST_H

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CTEST_COLOR_RED "\x1b[31m"
#define CTEST_COLOR_GREEN "\x1b[32m"
#define CTEST_COLOR_YELLOW "\x1b[33m"
#define CTEST_COLOR_CYAN "\x1b[36m"
#define CTEST_COLOR_RESET "\x1b[0m"

// This has to be defined as something that has a low chance of being printed
// otherwise
#define CTEST_TEST_DELIM "\037"

extern int ctest_run_count;
extern int ctest_fail_count;

void ctest_report_result(bool passed, const char *file, int line,
                         const char *expr, const char *msg, ...);

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    ctest_run_count++;                                                         \
    bool passed = (bool)(condition);                                           \
    if (!passed) {                                                             \
      ctest_fail_count++;                                                      \
    }                                                                          \
    ctest_report_result(passed, __FILE__, __LINE__, #condition, message);      \
  } while (0)

#define ASSERT_INT_EQ(actual, expected, message)                               \
  do {                                                                         \
    ctest_run_count++;                                                         \
    long long act_val = (long long)(actual);                                   \
    long long exp_val = (long long)(expected);                                 \
    bool passed = (act_val == exp_val);                                        \
    if (!passed)                                                               \
      ctest_fail_count++;                                                      \
    ctest_report_result(passed, __FILE__, __LINE__, #actual " == " #expected,  \
                        message);                                              \
  } while (0)

#define ASSERT_STR_EQ(actual, expected, message)                               \
  do {                                                                         \
    ctest_run_count++;                                                         \
    const char *act_s = (actual);                                              \
    const char *exp_s = (expected);                                            \
    bool passed = false;                                                       \
    if (act_s == NULL && exp_s == NULL) {                                      \
      passed = true;                                                           \
    } else if (act_s != NULL && exp_s != NULL && strcmp(act_s, exp_s) == 0) {  \
      passed = true;                                                           \
    }                                                                          \
    if (!passed)                                                               \
      ctest_fail_count++;                                                      \
    ctest_report_result(passed, __FILE__, __LINE__,                            \
                        "strcmp(" #actual ", " #expected ") == 0", message);   \
                                                                               \
  } while (0)

#define ASSERT_DOUBLE_EQ(actual, expected, epsilon, message)                   \
  do {                                                                         \
    ctest_run_count++;                                                         \
    double act_d = (double)(actual);                                           \
    double exp_d = (double)(expected);                                         \
    double eps_d = (double)(epsilon);                                          \
    bool passed = (fabs(act_d - exp_d) <= eps_d);                              \
    if (!passed)                                                               \
      ctest_fail_count++;                                                      \
    ctest_report_result(passed, __FILE__, __LINE__,                            \
                        "|" #actual " - " #expected "| <= " #epsilon,          \
                        message);                                              \
  } while (0)

#define ASSERT_PTR_NOT_NULL(ptr, message)                                      \
  do {                                                                         \
    ctest_run_count++;                                                         \
    bool passed = (ptr != NULL);                                               \
    if (!passed)                                                               \
      ctest_fail_count++;                                                      \
    ctest_report_result(passed, __FILE__, __LINE__, #ptr " != NULL", message); \
  } while (0)

#define ASSERT_PTR_NULL(ptr, message)                                          \
  do {                                                                         \
    ctest_run_count++;                                                         \
    bool passed = (ptr == NULL);                                               \
    if (!passed)                                                               \
      ctest_fail_count++;                                                      \
    ctest_report_result(passed, __FILE__, __LINE__, #ptr " == NULL", message); \
  } while (0)

// expose necessary internal implementation details of registry module (for use
// by macros)
typedef struct CTestRegistry CTestRegistry;
typedef struct TestSuite TestSuite;
typedef void (*CTestFn)(void);
CTestRegistry *ctest_get_global_registry(void);
void ctest_registry_add(CTestRegistry *reg, const char *suite_name,
                        const char *test_name, CTestFn fn);
void ctest_registry_set_setup_suite(CTestRegistry *reg, const char *suite,
                                    CTestFn fn);
void ctest_registry_set_teardown_suite(CTestRegistry *reg, const char *suite,
                                       CTestFn fn);
void ctest_registry_set_setup(CTestRegistry *reg, const char *suite,
                              CTestFn fn);
void ctest_registry_set_teardown(CTestRegistry *reg, const char *suite,
                                 CTestFn fn);

// intermediary macro needed so potential input macros can be evaluated
#define CTEST_CONCAT_IMPL(a, b, c) a##_##b##_##c
#define CTEST_CONCAT(a, b, c) CTEST_CONCAT_IMPL(a, b, c)

// suite hooks setup
#define CTEST_SETUP_SUITE(suite)                                               \
  static void CTEST_CONCAT(ctest_fn, setup_suite, suite)(void);                \
  __attribute__((constructor)) static void CTEST_CONCAT(                       \
      ctest_init, setup_suite, suite)(void) {                                  \
    ctest_registry_set_setup_suite(                                            \
        ctest_get_global_registry(), #suite,                                   \
        CTEST_CONCAT(ctest_fn, setup_suite, suite));                           \
  }                                                                            \
  static void CTEST_CONCAT(ctest_fn, setup_suite, suite)(void)

// suite hooks teardown
#define CTEST_TEARDOWN_SUITE(suite)                                            \
  static void CTEST_CONCAT(ctest_fn, teardown_suite, suite)(void);             \
  __attribute__((constructor)) static void CTEST_CONCAT(                       \
      ctest_init, teardown_suite, suite)(void) {                               \
    ctest_registry_set_teardown_suite(                                         \
        ctest_get_global_registry(), #suite,                                   \
        CTEST_CONCAT(ctest_fn, teardown_suite, suite));                        \
  }                                                                            \
  static void CTEST_CONCAT(ctest_fn, teardown_suite, suite)(void)

// test hooks setup
#define CTEST_SETUP(suite)                                                     \
  static void CTEST_CONCAT(ctest_fn, setup, suite)(void);                      \
  __attribute__((constructor)) static void CTEST_CONCAT(ctest_init, setup,     \
                                                        suite)(void) {         \
    ctest_registry_set_setup(ctest_get_global_registry(), #suite,              \
                             CTEST_CONCAT(ctest_fn, setup, suite));            \
  }                                                                            \
  static void CTEST_CONCAT(ctest_fn, setup, suite)(void)

// test hooks teardown
#define CTEST_TEARDOWN(suite)                                                  \
  static void CTEST_CONCAT(ctest_fn, teardown, suite)(void);                   \
  __attribute__((constructor)) static void CTEST_CONCAT(ctest_init, teardown,  \
                                                        suite)(void) {         \
    ctest_registry_set_teardown(ctest_get_global_registry(), #suite,           \
                                CTEST_CONCAT(ctest_fn, teardown, suite));      \
  }                                                                            \
  static void CTEST_CONCAT(ctest_fn, teardown, suite)(void)

// ctest test block
#define CTEST(suite, name)                                                     \
  static void CTEST_CONCAT(ctest_fn, suite, name)(void);                       \
  __attribute__((constructor)) static void CTEST_CONCAT(ctest_init, suite,     \
                                                        name)(void) {          \
    ctest_registry_add(ctest_get_global_registry(), #suite, #name,             \
                       CTEST_CONCAT(ctest_fn, suite, name));                   \
  }                                                                            \
  static void CTEST_CONCAT(ctest_fn, suite, name)(void)

// test suite summarisation helper (./src/libctest/libctest.c)

void ctest_summary(void);

// output handling methods (./src/libctest/libctest.c)

int ctest_mute_output(int std_stream_flag);
void ctest_unmute_output(int saved_descriptor, int std_stream_flag);

int ctest_capture_stdout_start(void);
ssize_t ctest_capture_stdout_end(char *buf, size_t buf_size);

// mock lifecycle methods (./src/libctest/mock.c)

int ctest_setup_mock_dir(const char *path);
int ctest_teardown_mock_dir(const char *path);

int ctest_setup_mock_file(const char *path, const char *content);
int ctest_teardown_mock_file(const char *path);

int ctest_setup_mock_binary(const char *path, const char *c_code);
int ctest_teardown_mock_binary(const char *path);

int ctest_teardown_all_mocks(void);

// child process handling helpers (./src/libctest/process.c)

typedef struct {
  bool exited_normally;
  int exit_code;
  bool terminated_by_signal;
  int term_signal;
} CTestProcessResult;

int ctest_run_in_child(void (*func)(void *arg), void *arg,
                       CTestProcessResult *out_res);

#define ASSERT_SIGNAL(func, arg, expected_sig, message)                        \
  do {                                                                         \
    ctest_run_count++;                                                         \
    CTestProcessResult _res;                                                   \
    bool _run_ok = (ctest_run_in_child((func), (arg), &_res) == 0);            \
    bool passed = _run_ok && _res.terminated_by_signal &&                      \
                  (_res.term_signal == (expected_sig));                        \
    if (!passed)                                                               \
      ctest_fail_count++;                                                      \
    ctest_report_result(passed, __FILE__, __LINE__,                            \
                        "signal(" #func ") == " #expected_sig, message);       \
  } while (0)

#define ASSERT_EXIT_CODE(func, arg, expected_code, message)                    \
  do {                                                                         \
    ctest_run_count++;                                                         \
    CTestProcessResult _res;                                                   \
    bool _run_ok = (ctest_run_in_child((func), (arg), &_res) == 0);            \
    bool passed = _run_ok && _res.exited_normally &&                           \
                  (_res.exit_code == (expected_code));                         \
    if (!passed)                                                               \
      ctest_fail_count++;                                                      \
    ctest_report_result(passed, __FILE__, __LINE__,                            \
                        "exit_code(" #func ") == " #expected_code, message);   \
  } while (0)

#endif
