#include <ctest/ctest.h>
#include <stdarg.h>
#include <stdlib.h>

#define CUSTOM_MSG_BUFFER_SIZE 512
#define MKDIR_MODE_FLAGS                                                       \
  S_IRWXU | S_IRWXG | S_IRWXO // read/write/execute by owner/group/others
#define BINARY_F_MODE_FLAGS 0755

int ctest_run_count = 0;
int ctest_fail_count = 0;

static int max_path_len = 256;

static int ctest_capture_pipefds[2] = {-1, -1};
static int ctest_capture_saved_stdout = -1;

void ctest_report_failure(const char *file, int line, const char *expr,
                          const char *msg, ...) {
  char custom_msg[CUSTOM_MSG_BUFFER_SIZE] = {0};

  va_list args;
  va_start(args, msg);
  vsnprintf(custom_msg, sizeof(custom_msg), msg, args);
  va_end(args);

  const char *runner_active = getenv("CTEST_RUNNER");

  if (runner_active && strcmp(runner_active, "1") == 0) {
    fprintf(stdout, "FAIL|%s|%d|%s|%s\n", file, line, expr, custom_msg);
    fflush(stdout);
  } else {
    fprintf(stderr, "%s  [FAIL] %s" CTEST_COLOR_RESET "\n", CTEST_COLOR_RED,
            custom_msg);
    fprintf(stderr, "         Expression: %s\n", expr);
    fprintf(stderr, "         Location  : Line %d in %s\n\n", line, file);
  }
}

void ctest_summary(void) {
  const char *runner_active = getenv("CTEST_RUNNER");

  if (runner_active && strcmp(runner_active, "1") == 0) {
    fprintf(stdout, "SUMMARY|%d|%d\n", ctest_run_count, ctest_fail_count);
    fflush(stdout);
    return;
  }

  printf("\n---------------------------\n");
  if (ctest_fail_count == 0) {
    printf(CTEST_COLOR_GREEN "ALL %d TESTS PASSED" CTEST_COLOR_RESET "\n",
           ctest_run_count);
  } else {
    printf(CTEST_COLOR_RED "TESTS FAILED: %d / %d" CTEST_COLOR_RESET "\n",
           ctest_fail_count, ctest_run_count);
  }
  printf("---------------------------\n");
}

int ctest_mute_output(int std_stream_flag) {
  int saved_descriptor = dup(std_stream_flag);
  if (saved_descriptor < 0)
    return -1;

  int dev_null = open("/dev/null", O_WRONLY);
  if (dev_null >= 0) {
    dup2(dev_null, std_stream_flag);
    close(dev_null);
  }
  return saved_descriptor;
}

void ctest_unmute_output(int saved_descriptor, int std_stream_flag) {
  if (saved_descriptor >= 0) {
    dup2(saved_descriptor, std_stream_flag);
    close(saved_descriptor);
  }
}

void ctest_setup_mock_dir(const char *path) { mkdir(path, MKDIR_MODE_FLAGS); }

void ctest_teardown_mock_dir(const char *path) { rmdir(path); }

int ctest_setup_mock_file(const char *path, const char *content) {
  FILE *f = fopen(path, "w");
  if (f == NULL)
    return -1;

  if (content != NULL)
    fprintf(f, "%s", content);
  fclose(f);
  return 0;
}

void ctest_teardown_mock_file(const char *path) { unlink(path); }

int ctest_setup_mock_binary(const char *path, const char *c_code) {
  char src_path[max_path_len];
  snprintf(src_path, max_path_len, "%s.c", path);

  FILE *f = fopen(src_path, "w");
  if (f == NULL)
    return -1;

  fputs(c_code, f);
  fclose(f);

  char cmd[max_path_len << 1];
  snprintf(cmd, sizeof(cmd), "gcc -o %s %s", path, src_path);
  if (system(cmd) != 0) {
    unlink(src_path);
    return -1;
  }
  chmod(path, BINARY_F_MODE_FLAGS);
  return 0;
}

void ctest_teardown_mock_binary(const char *path) {
  char src_path[max_path_len];
  snprintf(src_path, max_path_len, "%s.c", path);
  unlink(src_path);
  unlink(path);
}

int ctest_capture_stdout_start(void) {
  fflush(stdout);

  if (pipe(ctest_capture_pipefds) == -1)
    return -1;

  ctest_capture_saved_stdout = dup(STDOUT_FILENO);
  if (ctest_capture_saved_stdout == -1) {
    close(ctest_capture_pipefds[0]);
    close(ctest_capture_pipefds[1]);
    return -1;
  }

  if (dup2(ctest_capture_pipefds[1], STDOUT_FILENO) == -1) {
    close(ctest_capture_saved_stdout);
    close(ctest_capture_pipefds[0]);
    close(ctest_capture_pipefds[1]);
    ctest_capture_saved_stdout = -1;
    return -1;
  }

  close(ctest_capture_pipefds[1]);
  return 0;
}

ssize_t ctest_capture_stdout_end(char *buf, size_t buf_size) {
  if (ctest_capture_saved_stdout == -1)
    return -1;

  fflush(stdout);

  dup2(ctest_capture_saved_stdout, STDOUT_FILENO);
  close(ctest_capture_saved_stdout);
  ctest_capture_saved_stdout = -1;

  if (buf == NULL || buf_size == 0) {
    close(ctest_capture_pipefds[0]);
    return 0;
  }

  memset(buf, 0, buf_size);
  ssize_t bytes_read = read(ctest_capture_pipefds[0], buf, buf_size - 1);
  if (bytes_read < 0)
    bytes_read = 0;

  buf[bytes_read] = '\0';
  close(ctest_capture_pipefds[0]);
  return bytes_read;
}
