#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include "logger.h"
#include "utils.h"

TestResult single_thread_test(Logger* lg) {
  UNUSED(lg);

  for (int i = 0; i < 10000; i++)
    lg_info("warmup");

  struct timespec start, end;
  size_t dropped = 0;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int i = 0; i < LOG_COUNT_SINGLE; i++) {
    if (!lg_finfo("Hello, World!"))
      dropped += 1;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  size_t successful = LOG_COUNT_SINGLE - dropped;
  long elapsed_ns =
    (end.tv_sec - start.tv_sec) * 1000000000L +
    (end.tv_nsec - start.tv_nsec);
  double elapsed_sec = elapsed_ns / 1e9;
  return (TestResult) {
    .dropped = dropped,
    .successful = successful,
    .elapsed_ns = elapsed_ns,
    .throughput = successful / elapsed_sec,
  };
}
