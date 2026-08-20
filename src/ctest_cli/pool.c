#define _GNU_SOURCE
#include "pool.h"
#include "config.h"
#include "executor.h"
#include "filter.h"
#include "reporter.h"
#include "session.h"

#include <clib/vector.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static WorkerSlot *g_active_workers = NULL;
static size_t g_num_workers = 0;

// volatile: force compiler to read the variable from main memory everytime
// sig_atomic_t: guaranteed to be r/w in a single atomic CPU instruction
static volatile sig_atomic_t g_interrupted = 0;

static void pool_signal_handler(int sig) {
  g_interrupted = sig;
  if (g_active_workers != NULL) {
    for (size_t i = 0; i < g_num_workers; i++) {
      if (g_active_workers[i].is_active && g_active_workers[i].pid > 0) {
        // send SIGKILL to entire process groupt
        kill(-g_active_workers[i].pid, SIGKILL);
      }
    }
  }
}

static void setup_signals(struct sigaction *old_int, struct sigaction *old_term,
                          WorkerSlot *workers, size_t jobs) {
  // zero sigaction (remove active flags)
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = pool_signal_handler;
  sigemptyset(&sa.sa_mask);

  g_active_workers = workers;
  g_num_workers = jobs;
  g_interrupted = 0;

  sigaction(SIGINT, &sa, old_int);
  sigaction(SIGTERM, &sa, old_term);
}

static void restore_signals(const struct sigaction *old_int,
                            const struct sigaction *old_term) {
  sigaction(SIGINT, old_int, NULL);
  sigaction(SIGTERM, old_term, NULL);
  g_active_workers = NULL;
  g_num_workers = 0;
}

static void launch_workers(WorkerSlot *workers, SuiteMetrics *slot_metrics,
                           size_t jobs, const Vector *test_bins, size_t *bin_i,
                           const CTestConfig *config, size_t *active_count) {
  for (size_t i = 0; i < jobs && *bin_i < test_bins->count && !g_interrupted;
       i++) {
    if (workers[i].is_active)
      continue;

    const char *bin_path = NULL;
    // iterate through test_bins until you find a valid path (given filter) or
    // run out
    while (*bin_i < test_bins->count) {
      const char *path = (const char *)vector_get(test_bins, *bin_i);
      (*bin_i)++;
      if (ctest_filter_matches(path, config->filter_pattern)) {
        bin_path = path;
        break;
      }
    }

    // if found valid path
    if (bin_path != NULL) {
      ctest_report_suite_start(bin_path, config->verbosity);

      // zero the worker
      memset(&workers[i], 0, sizeof(WorkerSlot));
      memset(&slot_metrics[i], 0, sizeof(SuiteMetrics));

      if (ctest_launch_suite(bin_path, config->verbosity, &workers[i]) == 0)
        (*active_count)++;
    }
  }
}

static void build_poll_fds(const WorkerSlot *workers, size_t jobs,
                           struct pollfd *fds) {
  for (size_t i = 0; i < jobs; i++) {
    if (workers[i].is_active && workers[i].read_fd >= 0) {
      fds[i].fd = workers[i].read_fd;

      // POLLIN: kernel pipe buffer has >= 0 bytes available to read
      // POLLHUP: the child process closed its descriptor
      // POLLERR: error condition on file descriptor
      fds[i].events = POLLIN | POLLHUP | POLLERR;
      fds[i].revents = 0; // zero out kernel detected events (same below)
    } else {
      fds[i].fd = -1;    // poll ignores array index
      fds[i].events = 0; // zero out events we want to see
      fds[i].revents = 0;
    }
  }
}

static void process_workers(WorkerSlot *workers, SuiteMetrics *slot_metrics,
                            size_t jobs, const struct pollfd *fds,
                            const CTestConfig *config, SessionMetrics *session,
                            Vector *failure_ledger, size_t *active_count) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  for (size_t i = 0; i < jobs; i++) {
    if (!workers[i].is_active)
      continue;

    int harvest_res = -1;
    if (fds[i].revents & (POLLIN | POLLHUP | POLLERR))
      harvest_res = ctest_harvest_output(&workers[i], config->verbosity,
                                         &slot_metrics[i], failure_ledger);

    if (config->timeout_sec > 0 && !workers[i].timed_out) {
      double elapsed = (now.tv_sec - workers[i].start_time.tv_sec) +
                       (now.tv_nsec - workers[i].start_time.tv_nsec) / 1e9;
      if (elapsed >= (double)config->timeout_sec)
        workers[i].timed_out = 1;
    }

    int is_eof = (harvest_res == 0);
    int is_hup_err =
        (fds[i].revents & (POLLHUP | POLLERR)) && (harvest_res <= 0);

    // suite execution finished
    if (workers[i].timed_out || is_eof || is_hup_err) {
      ctest_finalise_suite(&workers[i], config->timeout_sec, config->verbosity,
                           &slot_metrics[i], failure_ledger);

      ctest_report_suite_metrics(workers[i].bin_path, &slot_metrics[i],
                                 config->verbosity);
      ctest_update_session(session, &slot_metrics[i]);

      // zero the worker
      memset(&workers[i], 0, sizeof(WorkerSlot));
      memset(&slot_metrics[i], 0, sizeof(SuiteMetrics));
      (*active_count)--;
    }
  }
}

static void terminate_remaining_workers(WorkerSlot *workers, size_t jobs) {
  for (size_t i = 0; i < jobs; i++) {
    if (workers[i].is_active && workers[i].pid > 0) {
      kill(-workers[i].pid, SIGKILL);
      int status;
      waitpid(workers[i].pid, &status, 0);
      workers[i].is_active = 0;
    }
  }
}

int ctest_pool_run(const Vector *test_bins, const CTestConfig *config,
                   SessionMetrics *session, Vector *failure_ledger) {
  // other validations are handled by the function caller
  if (test_bins->count == 0)
    return 0;

  size_t jobs = config->jobs;
  // chose to allocate to heap instead of stack to avoid capping/overflow, could
  // be debated though
  WorkerSlot *workers = calloc(jobs, sizeof(WorkerSlot));
  if (workers == NULL) {
    fprintf(stderr, "ctest: failed to allocate worker pool memory\n");
    return -1;
  }
  SuiteMetrics *slot_metrics = calloc(jobs, sizeof(SuiteMetrics));
  if (slot_metrics == NULL) {
    fprintf(stderr, "ctest: failed to allocate metrics pool memory\n");
    free(workers);
    return -1;
  }

  struct pollfd fds[jobs];
  struct sigaction old_sa_int, old_sa_term;

  setup_signals(&old_sa_int, &old_sa_term, workers, jobs);

  size_t bin_i = 0;
  size_t active_count = 0;

  while ((bin_i < test_bins->count || active_count > 0) && !g_interrupted) {
    launch_workers(workers, slot_metrics, jobs, test_bins, &bin_i, config,
                   &active_count);

    if (active_count == 0)
      break;

    build_poll_fds(workers, jobs, fds);

    int poll_res = poll(fds, (nfds_t)jobs, 10);
    if (poll_res < 0 && errno == EINTR && g_interrupted)
      break;

    process_workers(workers, slot_metrics, jobs, fds, config, session,
                    failure_ledger, &active_count);
  }

  terminate_remaining_workers(workers, jobs);
  restore_signals(&old_sa_int, &old_sa_term);

  free(workers);
  free(slot_metrics);

  return g_interrupted ? -1 : 0;
}
