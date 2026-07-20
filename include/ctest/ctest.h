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
#define CTEST_COLOR_RESET "\x1b[0m"

extern int ctest_run_count;
extern int ctest_fail_count;

void ctest_report_failure(const char *file, int line, const char *expr,
                          const char *msg, ...);

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    ctest_run_count++;                                                         \
    if (!(condition)) {                                                        \
      ctest_fail_count++;                                                      \
      ctest_report_failure(__FILE__, __LINE__, #condition, message);           \
    }                                                                          \
  } while (0)

#define ASSERT_INT_EQ(actual, expected, message)                               \
  do {                                                                         \
    ctest_run_count++;                                                         \
    long long act_val = (long long)(actual);                                   \
    long long exp_val = (long long)(expected);                                 \
    if (act_val != exp_val) {                                                  \
      ctest_fail_count++;                                                      \
      char details[256];                                                       \
      snprintf(details, sizeof(details), "%s (Expected %lld, got %lld)",       \
               message, exp_val, act_val);                                     \
      ctest_report_failure(__FILE__, __LINE__, #actual " == " #expected,       \
                           details);                                           \
    }                                                                          \
  } while (0)

#define ASSERT_STR_EQ(actual, expected, message)                               \
  do {                                                                         \
    ctest_run_count++;                                                         \
    const char *act_s = (actual);                                              \
    const char *exp_s = (expected);                                            \
    if (act_s == NULL && exp_s == NULL)                                        \
      return;                                                                  \
    if (act_s == NULL || exp_s == NULL || strcmp(act_s, exp_s) != 0) {         \
      ctest_fail_count++;                                                      \
      char details[512];                                                       \
      snprintf(details, sizeof(details), "%s (Expected \"%s\", got \"%s\")",   \
               message, exp_s ? exp_s : "NULL", act_s ? act_s : "NULL");       \
      ctest_report_failure(__FILE__, __LINE__,                                 \
                           "strcmp(" #actual ", " #expected ") == 0",          \
                           details);                                           \
    }                                                                          \
  } while (0)

#define ASSERT_DOUBLE_EQ(actual, expected, epsilon, message)                   \
  do {                                                                         \
    ctest_run_count++;                                                         \
    double act_d = (double)(actual);                                           \
    double exp_d = (double)(expected);                                         \
    if (fabs(act_d - exp_d) > (double)(epsilon)) {                             \
      ctest_fail_count++;                                                      \
      char details[256];                                                       \
      snprintf(details, sizeof(details),                                       \
               "%s (Expected %f, got %f within eps %f)", message, exp_d,       \
               act_d, (double)epsilon);                                        \
      ctest_report_failure(__FILE__, __LINE__,                                 \
                           "|" #actual " - " #expected "| <= " #epsilon,       \
                           details);                                           \
    }                                                                          \
  } while (0)

#define ASSERT_PTR_NOT_NULL(ptr, message)                                      \
  do {                                                                         \
    ctest_run_count++;                                                         \
    if ((ptr) == NULL) {                                                       \
      ctest_fail_count++;                                                      \
      ctest_report_failure(__FILE__, __LINE__, #ptr " != NULL", message);      \
    }                                                                          \
  } while (0)

#define ASSERT_PTR_NULL(ptr, message)                                          \
  do {                                                                         \
    ctest_run_count++;                                                         \
    if ((ptr) != NULL) {                                                       \
      ctest_fail_count++;                                                      \
      ctest_report_failure(__FILE__, __LINE__, #ptr " == NULL", message);      \
    }                                                                          \
  } while (0)

void ctest_summary(void);

int ctest_mute_output(int std_stream_flag);
void ctest_unmute_output(int saved_descriptor, int std_stream_flag);

void ctest_setup_mock_dir(const char *path);
void ctest_teardown_mock_dir(const char *path);
int ctest_setup_mock_file(const char *path, const char *content);
void ctest_teardown_mock_file(const char *path);
int ctest_setup_mock_binary(const char *path, const char *c_code);
void ctest_teardown_mock_binary(const char *path);

#endif
