#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#define LOGGER_IMPLEMENTATION
#include "logger.h"
#include "utils.h"

int failure(const char* program_name, const char* arg) {
  printf("[ERROR] Wrong argument for: %s\n", arg);
  printf("Usage:\n");
  printf("  %s <thread mode> <policy> <is stderr>\n", program_name);
  printf("  Options:\n");
  printf("    thread mode: multi | single -> Desribes type of test in threads level\n");
  printf("    policy:      drop  | block  -> Desribes policy for logging\n");
  printf("    is stderr:   true  | false  -> Include stderr sink?\n");
  return 1;
}

int main(int argc, char** argv) {
  const char* program = argv[0];
  LgLogPolicy policy = LG_DROP;
  bool is_stderr = false;
  bool is_single = true;

  if (argc >= 2) {
    if (strcmp(argv[1], "single") == 0) is_single = true;
    else if (strcmp(argv[1], "multi") == 0) is_single = false;
    else return failure(program, "thread mode");
  }
  if (argc >= 3) {
    policy = str2policy(argv[2]);
    if (policy == LG_PRIORITY_BASED) return failure(program, "policy");
  }
  if (argc >= 4) {
    if (strcmp(argv[3], "true") == 0) is_stderr = true;
    else if (strcmp(argv[3], "false") == 0) is_stderr = false;
    else return failure(program, "is stderr");
  }

  LoggerConfig cfg = {
    .localTime = true,
    .maxFiles = 5,
    .generateDefaultFile = false,
    .logPolicy = policy,
    .logFormatter = NULL,
  };
  if (is_stderr)
    lg_append_sink(&cfg, stderr, LG_OUT_TTY);

  Logger lg = {0};
  if (!lg_init(&lg, "logs", cfg)) return 1;

  TestResult res = {0};
  if (is_single)
    res = single_thread_test(&lg);
  else
    res = multi_thread_test(&lg);

  printf("Thread count: %d\n", is_single ? 1 : THREAD_COUNT);
  printf("Is stderr?: %s\n", is_stderr ? "YES" : "NO");
  printf("Log Policy: %s\n", policy2str(policy));

  printf("Dropped %ld logs\n", res.dropped);
  printf("Successful %ld logs\n", res.successful);
  printf("Elapsed %ld ns\n", res.elapsed_ns);

  if (isatty(fileno(stdout)))
    printf("Throughput: " BASH_GOLD "%ld logs/sec" BASH_RST "\n", res.throughput);
  else
    printf("Throughput: %ld logs/sec\n", res.throughput);

  if (!lg_destroy(&lg)) return 1;
  return 0;
}
