#pragma once
#include "logger.h"
#include <string.h>

#define THREAD_COUNT 10
#define LOG_COUNT_PER_THREAD 100000
#define LOG_COUNT_SINGLE 1000000

#define CLOCK_START(start) clock_gettime(CLOCK_MONOTONIC, &(start));
#define CLOCK_END(end) clock_gettime(CLOCK_MONOTONIC, &(end));
#define UNUSED(x) (void)x

#define BASH_GOLD "\e[1;38;2;255;165;0m"
#define BASH_RST "\e[0m"

static inline const char* policy2str(LgLogPolicy policy) {
  switch(policy) {
  case LG_DROP: return "Drop";
  case LG_BLOCK: return "Block";
  case LG_PRIORITY_BASED: return "Priority Based";
  default: return NULL;
  }
}

static inline LgLogPolicy str2policy(const char* str) {
  if (strcmp(str, "drop") == 0) return LG_DROP;
  if (strcmp(str, "block") == 0) return LG_BLOCK;
  return LG_PRIORITY_BASED;
}

typedef struct {
  size_t dropped;
  size_t throughput;
  long elapsed_ns;
  size_t successful;
} TestResult;

TestResult single_thread_test(Logger* lg);
TestResult multi_thread_test(Logger* lg);
