#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#define LOGGER_IMPLEMENTATION
#define LOGGER_DEBUG
#include "logger.h"

static const char* policy2str(LgLogPolicy policy) {
  switch(policy) {
  case LG_DROP: return "Drop";
  case LG_BLOCK: return "Block";
  case LG_PRIORITY_BASED: return "Priority Based";
  default: return "NULL";
  }
}

static LgLogPolicy str2policy(const char* str) {
  if (strcmp(str, "drop") == 0) return LG_DROP;
  if (strcmp(str, "block") == 0) return LG_BLOCK;
  if (strcmp(str, "priority_based") == 0) return LG_PRIORITY_BASED;
  return LG_DROP;
}

static Logger* lg;

int main(int argc, char** argv) {
  LgLogPolicy policy = LG_DROP;
  bool is_stderr = false;
  if (argc >= 2) policy = str2policy(argv[1]);
  if (argc >= 3) is_stderr = strcmp(argv[2], "true") == 0;

  LoggerConfig cfg = {
    .localTime = true,
    .maxFiles = 5,
    .generateDefaultFile = true,
    .logPolicy = policy,
    .logFormatter = NULL,
  };

  if (is_stderr) {
    lg_append_sink(&cfg, stderr, LG_OUT_TTY);
  }

  lg = lg_alloc();
  if (!lg_init(lg, "logs", cfg)) return 1;

  struct timespec start, end;
  size_t dropped = 0;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int i = 0; i < 1e6; i++) {
    if (!lg_info("Hello, World!"))
      dropped += 1;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  long elapsed_ns =
    (end.tv_sec - start.tv_sec) * 1000000000L +
    (end.tv_nsec - start.tv_nsec);

  printf("Is stderr?: %s\n", is_stderr ? "YES" : "NO");
  printf("Log Policy: %s\n", policy2str(policy));
  printf("Dropped: %ld logs\n", dropped);
  printf("Elapsed: %ld ns\n", elapsed_ns);
  printf("Per log: %.2f ns\n", (double)elapsed_ns / 1000000);
  if (!lg_destroy(lg)) return 1;
  return 0;
}
