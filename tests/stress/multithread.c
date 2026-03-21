#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include "logger.h"
#include "utils.h"

_Atomic int ready = 0;
_Atomic int go = 0;

typedef struct {
  size_t dropped;
  char _pad[64 - sizeof(size_t)];
} ThreadCtx;

void* thread_func(void* arg) {
  atomic_fetch_add(&ready, 1);
  while (!atomic_load(&go));

  ThreadCtx* ctx = (ThreadCtx*)arg;
  for (int i = 0; i < LOG_COUNT_PER_THREAD; i++) {
    if (!lg_info("Hello, World!"))
      ctx->dropped += 1;
  }
  return NULL;
}

TestResult multi_thread_test(Logger* lg) {
  UNUSED(lg);
  ThreadCtx ctxs[THREAD_COUNT] = {0};
  pthread_t threads[THREAD_COUNT];

  for (size_t i = 0; i < THREAD_COUNT; i++)
    pthread_create(&threads[i], NULL, thread_func, &ctxs[i]);
  while (atomic_load(&ready) != THREAD_COUNT); // wait for getting everyone ready

  struct timespec start, end;
  CLOCK_START(start);
  {
    atomic_store(&go, 1);
    for (int i = 0; i < THREAD_COUNT; i++)
      pthread_join(threads[i], NULL);
  }
  CLOCK_END(end);

  size_t dropped = 0;
  for (size_t i = 0; i < THREAD_COUNT; i++)
    dropped += ctxs[i].dropped;

  size_t total = LOG_COUNT_PER_THREAD * THREAD_COUNT;
  size_t successful = total - dropped;
  long elapsed_ns =
    (end.tv_sec - start.tv_sec) * 1000000000L +
    (end.tv_nsec - start.tv_nsec);
  double elapsed_sec = elapsed_ns / 1e9;
  return (TestResult) {
    .dropped = dropped,
    .successful = successful,
    .throughput = successful / elapsed_sec,
    .elapsed_ns = elapsed_ns,
  };
}
