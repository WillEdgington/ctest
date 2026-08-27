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
typedef void (*CTestFn)(void);
CTestRegistry *ctest_get_global_registry(void);
void ctest_registry_add(CTestRegistry *reg, const char *suite_name,
                        const char *test_name, CTestFn fn);

// intermediary macro needed so potential input macros can be evaluated
#define CTEST_CONCAT_IMPL(a, b, c) a##_##b##_##c
#define CTEST_CONCAT(a, b, c) CTEST_CONCAT_IMPL(a, b, c)

// define test case method interface, add test case to global reg before main(),
// define test case method fully
#define CTEST(suite, name)                                                     \
  static void CTEST_CONCAT(ctest_fn, suite, name)(void);                       \
  __attribute__((constructor)) static void CTEST_CONCAT(ctest_init, suite,     \
                                                        name)(void) {          \
    ctest_registry_add(ctest_get_global_registry(), #suite, #name,             \
                       CTEST_CONCAT(ctest_fn, suite, name));                   \
  }                                                                            \
  static void CTEST_CONCAT(ctest_fn, suite, name)(void)

void ctest_summary(void);

int ctest_mute_output(int std_stream_flag);
void ctest_unmute_output(int saved_descriptor, int std_stream_flag);

void ctest_setup_mock_dir(const char *path);
void ctest_teardown_mock_dir(const char *path);
int ctest_setup_mock_file(const char *path, const char *content);
void ctest_teardown_mock_file(const char *path);
int ctest_setup_mock_binary(const char *path, const char *c_code);
void ctest_teardown_mock_binary(const char *path);
int ctest_capture_stdout_start(void);
ssize_t ctest_capture_stdout_end(char *buf, size_t buf_size);

#endif
