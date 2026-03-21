/*
  The MIT License
  Copyright (c) 2026, ilpeN

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  TLDR:
    do whatever you want, just keep the license text
*/

#ifndef LOGGER_H
#define LOGGER_H

#ifdef _WIN32
#define LOGGER_EXPORT __declspec(dllexport)
#else
#define LOGGER_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
#define LOGGER_EXTERN extern "C"
#else
#define LOGGER_EXTERN extern
#endif

#ifdef LOGGER_IMPLEMENTATION
#define LOGGERDEF LOGGER_EXTERN LOGGER_EXPORT
#else
#define LOGGERDEF LOGGER_EXTERN
#endif

#define LOGGER_INTERNAL static

#include <stdio.h>
#include <stddef.h>

#if __STDC_VERSION__ >= 199901L
#include <stdint.h>
#else
#ifndef uint8_t
typedef unsigned char uint8_t;
#endif
#ifndef uint32_t
typedef unsigned int uint32_t;
#endif
#endif

/* These variables can be fine-tuned due to your traffic */
/* High traffic = high these constants */
/* Low traffic = lower values */
#define LOGGER_WAIT_NO_PAUSE_MAGIC 128
#define LOGGER_WAIT_PAUSE_MAGIC 1024

/*
  Defines time_str from lg_get_time_str size (WITH ZERO AT THE END!)
  All of time_str related functions uses this, if you
  ever change lg_get_time_str function, update this
*/
#define LOGGER_TIME_STR_SIZE 24

/* logger max message size (you can change it) */
#define LOGGER_MAX_MSG_SIZE 256

/* Maximum amount of files that can be in the sink */
#define LOGGER_MAX_SINKS 8

#define LOGGER_FILE_EXT ".log"
#define LOGGER_FILE_EXT_SZ 4

#define LOGGER_CONTAINS_FLAG(main, flag) (main & (1u << flag))

/* The ANSI bash color codes/escape characters */
#define LOGGER_CLR_RED "\x1b[31m"
#define LOGGER_CLR_GREEN "\x1b[32m"
#define LOGGER_CLR_YELLOW "\x1b[33m"
#define LOGGER_CLR_AQUA "\x1b[36m"
#define LOGGER_CLR_RST "\x1b[0m"

typedef enum {
  LG_INFO = 0,
  LG_ERROR = 1,
  LG_WARNING = 2,
  LG_CUSTOM = 3,
  /* Add more levels here */
} LgLogLevel;

/* Log with an explicit logger instance */
#define lg_logi(instance, level, fmt, ...) \
  lg_vlog_(instance, level, fmt, ##__VA_ARGS__)

#define lg_infoi(instance, fmt, ...) \
  lg_logi(instance, LG_INFO, fmt, ##__VA_ARGS__)
#define lg_errori(instance, fmt, ...) \
  lg_logi(instance, LG_ERROR, fmt, ##__VA_ARGS__)
#define lg_warni(instance, fmt, ...) \
  lg_logi(instance, LG_WARNING, fmt, ##__VA_ARGS__)

#define lg_log(level, fmt, ...) \
  lg_logi(lg_get_active_instance(), level, fmt, ##__VA_ARGS__)
#define lg_info(fmt, ...) lg_log(LG_INFO, fmt, ##__VA_ARGS__)
#define lg_error(fmt, ...) lg_log(LG_ERROR, fmt, ##__VA_ARGS__)
#define lg_warn(fmt, ...) lg_log(LG_WARNING, fmt, ##__VA_ARGS__)

/* you can add your custom level like this: */
#define lg_custom(fmt, ...) lg_log(LG_CUSTOM, fmt, ##__VA_ARGS__)

typedef enum {
  LG_DROP = 0,
  LG_BLOCK = 1,
  LG_PRIORITY_BASED = 2,
} LgLogPolicy;

typedef enum {
  LG_OUT_TTY = 0,
  LG_OUT_FILE = 1,
  LG_OUT_NET = 2,
  /* Add more out types here */
  /* Dont forget to update LOGGER_MAX_OUT_TYPES */
} LgOutType;

#define LOGGER_MAX_OUT_TYPES 3

typedef struct Logger Logger;

typedef struct LgString {
  char data[LOGGER_MAX_MSG_SIZE];
  size_t len;
} LgString;

typedef struct {
  FILE* file;
  LgOutType type;
} LgSink;

typedef struct {
  LgSink items[LOGGER_MAX_SINKS];
  size_t count;
} LgSinks;

typedef LgString LgMsgPack[LOGGER_MAX_OUT_TYPES];

typedef int (*log_formatter_t)(
  const char* time_str,
  LgLogLevel level,
  const char* msg,
  uint32_t needed, /* needed file types */
  LgMsgPack out
);

typedef struct {
  int localTime;
  int maxFiles;
  int generateDefaultFile;
  LgSinks sinks;
  LgLogPolicy logPolicy;
  log_formatter_t logFormatter;
} LoggerConfig;

/* portable printf-format style checker (only available on gcc and clang) */
#if defined(__clang__) || defined(__GNUC__)
  #define PRINTF_LIKE(fmt, args) __attribute__((format(printf, fmt, args)))
#else
  #define PRINTF_LIKE(fmt, args)
#endif

LOGGERDEF int lg_init(Logger* instance, const char* logs_dir,
                     LoggerConfig config);

LOGGERDEF int lg_init_defaults(Logger* instance, const char* logs_dir);

LOGGERDEF int lg_init_flat(Logger* inst, const char* logs_dir,
                          int local_time, int max_log_files, int generateDefaultFile,
                          LgSinks sinks, LgLogPolicy log_policy,
                          log_formatter_t log_formatter);

/*
  IMPORTANT:
  lg_destroy is safe to call from any single thread,
  but the caller is responsible for ensuring no other
  threads are actively pushing logs at the time of the call.
*/
LOGGERDEF int lg_destroy(Logger* instance);

LOGGERDEF int lg_is_alive(const Logger* instance);

LOGGERDEF int lg_log_(Logger* inst, const LgLogLevel level,
                     const char* msg, size_t msglen);

LOGGERDEF int lg_vlog_(Logger* inst, const LgLogLevel level,
                      const char* fmt, ...) PRINTF_LIKE(3, 4);

LOGGERDEF int lg_flogi(Logger* inst, const LgLogLevel level, const char* msg);
LOGGERDEF int lg_flog(const LgLogLevel level, const char* msg);

/* Explicit instances */
LOGGERDEF int lg_finfoi(Logger* inst, const char* msg);
LOGGERDEF int lg_ferrori(Logger* inst, const char* msg);
LOGGERDEF int lg_fwarni(Logger* inst, const char* msg);

/* Implicit instances (uses active one) */
LOGGERDEF int lg_finfo(const char* msg);
LOGGERDEF int lg_ferror(const char* msg);
LOGGERDEF int lg_fwarn(const char* msg);

LOGGERDEF int lg_set_active_instance(Logger* inst);

LOGGERDEF Logger* lg_get_active_instance();

LOGGERDEF const char* lg_lvl_to_str(const LgLogLevel level);

LOGGERDEF LoggerConfig lg_get_defaults();

LOGGERDEF int lg_append_sink(LoggerConfig* config, FILE* f, LgOutType type);

LOGGERDEF void lg_str_format_into(LgString* s, const char* fmt, ...)
  PRINTF_LIKE(2, 3);

LOGGERDEF void lg_str_write_into(LgString* s,
                                const char* already_formatted_str);

LOGGERDEF int lg_get_time_str(Logger* inst, char* buf);

LOGGERDEF Logger* lg_alloc();

LOGGERDEF void lg_free(Logger* inst);

/*
  File wrappers for FFI
  get_stdout and get_stdder returns stdout and stderr
  lg_fopen wraps fopen therefore if foreign language
  has GC or something, it doesnt delete and preventing
  double free in lg_destroy
*/
LOGGERDEF FILE* lg_get_stdout();
LOGGERDEF FILE* lg_get_stderr();
LOGGERDEF FILE* lg_fopen(const char* path);

#ifdef LOGGER_MINIFY_PREFIXES
#define llog lg_log
#define linfo lg_info
#define lwarn lg_warn
#define lerror lg_error
#define LINFO LG_INFO
#define LERROR LG_ERROR
#define LWARNING LG_WARNING
#define CLR_RED LOGGER_CLR_RED
#define CLR_GREEN LOGGER_CLR_GREEN
#define CLR_YELLOW LOGGER_CLR_YELLOW
#define CLR_AQUA LOGGER_CLR_AQUA
#define CLR_RST LOGGER_CLR_RST
#endif
// IMPLEMENTATION BEGIN (DO NOT CHANGE THIS LINE!)
#ifdef LOGGER_IMPLEMENTATION
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdalign.h>
#include <stdarg.h>

#ifdef __cplusplus
#include <atomic>
#define ATOMIC(T) std::atomic<T>
#define memory_order_relaxed std::memory_order_relaxed
#define memory_order_seq_cst std::memory_order_seq_cst
#define memory_order_acquire std::memory_order_acquire
#define memory_order_release std::memory_order_release
#define atomic_thread_fence std::atomic_thread_fence
#define atomic_store_explicit std::atomic_store_explicit
#define atomic_load_explicit std::atomic_load_explicit
#define atomic_fetch_add_explicit std::atomic_fetch_add_explicit
#define atomic_compare_exchange_weak_explicit std::atomic_compare_exchange_weak_explicit
#define atomic_compare_exchange_strong_explicit std::atomic_compare_exchange_strong_explicit
#else
#include <stdatomic.h>
#define ATOMIC(T) _Atomic(T)
#endif

// DO NOT change these
#define LOGGER_CACHE_LINE 64
#define LOGGER_ALIGN alignas(LOGGER_CACHE_LINE)

typedef struct {
  char msg[LOGGER_MAX_MSG_SIZE];
  size_t length;
  LgLogLevel level;
} LogPayload;

typedef struct {
  LOGGER_ALIGN ATOMIC(size_t) seq;
  LogPayload payload;
} LogSlot;

// Maximum amount of slots can be written at once
#define LOGGER_MAX_BATCH 32

// Size of ring buffer you can change it
// but make sure that it is power of 2
#define LOGGER_RING_SIZE 1024
#define LOGGER_RING_MASK (LOGGER_RING_SIZE - 1)
#if (LOGGER_RING_SIZE & LOGGER_RING_MASK) != 0
  #error "Ring buffer's size is not power of 2!"
#endif
#define LOGGER_RING_STRIDE ((sizeof(LogSlot) + LOGGER_CACHE_LINE - 1) \
                            & ~(size_t)(LOGGER_CACHE_LINE - 1))
#define LOGGER_RING_TOTAL_SIZE (LOGGER_RING_SIZE * LOGGER_RING_STRIDE)

typedef struct {
  LOGGER_ALIGN ATOMIC(size_t) head;
  LOGGER_ALIGN size_t tail;
  uint8_t slots[LOGGER_RING_TOTAL_SIZE];
} LogQueue;

// Static function forward-declerations
LOGGER_INTERNAL int lgi_check_dir(const char* path);

LOGGER_INTERNAL bool lgi_normalize_path(const char* path, char* out, size_t size);

LOGGER_INTERNAL int lgi_count_logs_and_get_oldest(const char* path, char* oldest_path, size_t oldest_path_size);

LOGGER_INTERNAL bool lgi_mkdir_p(char* path);

LOGGER_INTERNAL int lgi_def_format_msg(const char* time_str, LgLogLevel level,
                                      const char* msg, uint32_t needed, LgMsgPack pack);

LOGGER_INTERNAL void lgi_queue_create(LogQueue* q);
LOGGER_INTERNAL size_t lgi_queue_pop_batch(LogQueue* q, size_t* start_pos, size_t max_batch);
LOGGER_INTERNAL bool lgi_queue_ppr_batch(Logger* inst, size_t* start_pos);
LOGGER_INTERNAL void lgi_queue_release(LogQueue* q, size_t pos);

LOGGER_INTERNAL void lgi_adaptive_wait(int* spins);

LOGGER_INTERNAL inline LogSlot* lgi_slot_get(LogQueue* q, size_t idx)
{
  return (LogSlot*)(q->slots + (idx & LOGGER_RING_MASK) * LOGGER_RING_STRIDE);
}

// Manual writes for lg_get_time_str
LOGGER_INTERNAL inline void lgi_time_write2(char* p, int v)
{
  p[0] = (char)('0' + v / 10);
  p[1] = (char)('0' + v % 10);
}
LOGGER_INTERNAL inline void lgi_time_write4(char* p, int v)
{
  lgi_time_write2(p, v / 100);
  lgi_time_write2(p + 2, v % 100);
}
LOGGER_INTERNAL inline void lgi_time_write3(char* p, int v)
{
  p[0] = (char)('0' + v / 100);
  p[1] = (char)('0' + (v / 10) % 10);
  p[2] = (char)('0' + v % 10);
}

// Manual appending, used at default format_msg
LOGGER_INTERNAL inline void lgi_str_append_n(char** p, char* end, const char* s)
{
  while (*s && *p < end) *(*p)++ = *s++;
}
LOGGER_INTERNAL inline void lgi_str_close(char** p, char* end) {
  if (*p < end) *(*p)++ = '\n';
  **p = '\0';
}

#define LG_UNUSED(x) (void)(x)
#define LG_STRINGIFY(x) #x
// Fuck you MSVC with C++
#ifdef __cplusplus
  #define LG_STRUCT(T, ...) (T{__VA_ARGS__})
#else
  #define LG_STRUCT(T, ...) ((T){__VA_ARGS__})
#endif

#ifdef LOGGER_DEBUG
#define LG_DEBUG_ERR(fmt, ...) do {                     \
    fprintf(stderr, "%s:%d: [DEBUG/ERROR]: " fmt "\n",  \
            __FILE__, __LINE__, ##__VA_ARGS__);         \
  } while (0)
#define LG_DEBUG(fmt, ...) do {                 \
    printf("%s:%d: [DEBUG/INFO]: " fmt "\n",    \
           __FILE__, __LINE__, ##__VA_ARGS__);  \
  } while (0)
#else
#define LG_DEBUG_ERR(fmt, ...) // swallow
#define LG_DEBUG(fmt, ...)
#endif // LOGGER_DEBUG

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <malloc.h>
#include <io.h>

#define LOGGER_MKDIR(path) _mkdir(path)
#define LOGGER_PATH_SEP '\\'
#ifndef PATH_MAX
#define PATH_MAX (MAX_PATH * 2)
#endif

// Sleep() has terrible resolution (milliseconds)
// But who uses winbloat for production-ready logger?
#define LOGGER_SLEEP(us) do { Sleep(1); } while (0)

typedef HANDLE pthread_t;
static DWORD WINAPI lgi_thread_trampoline(LPVOID arg)
{
  typedef void* (*fn_t)(void*);
  fn_t fn = (fn_t)((void**)arg)[0];
  void* rarg = ((void**)arg)[1];
  free(arg);
  fn(rarg);
  return 0;
}

static int pthread_join(pthread_t t, void **retval)
{
  LG_UNUSED(retval);
  DWORD result = WaitForSingleObject(t, INFINITE);
  if (result != WAIT_OBJECT_0) {
    CloseHandle(t);
    return EINVAL;
  }

  if (!CloseHandle(t)) {
    return EINVAL;
  }
  return 0;
}

static int pthread_create(pthread_t* t, void* attr,
                         void* (*func)(void*), void* arg)
{
  LG_UNUSED(attr);
  void** pack = (void**)malloc(sizeof(void*) * 2);
  if (pack == NULL) return 1;
  pack[0] = (void*)func;
  pack[1] = arg;
  *t = CreateThread(NULL, 0, lgi_thread_trampoline, pack, 0, NULL);

  if (!*t) {
    free(pack);
    return 1;
  }
  return 0;
}

#else // POSIX:
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <dirent.h>
#include <unistd.h>

// sleep for us microseconds
#define LOGGER_SLEEP(us)                                                \
  do {                                                                  \
    struct timespec ts = {(us) / 1000000L, ((us) % 1000000L) * 1000L};  \
    nanosleep(&ts, NULL);                                               \
  } while (0)
#define LOGGER_MKDIR(path) mkdir(path, 0755)
#define LOGGER_PATH_SEP '/'
#endif

#ifdef _MSC_VER
  #include <intrin.h>
  #if defined(_M_X64) || defined(_M_IX86)
    #define LOGGER_PAUSE_INS() _mm_pause() // MSVC X86/X64 intrinsic
  #elif defined(_M_ARM64)
    #define LOGGER_PAUSE_INS() __yield()  // MSVC ARM64 intrinsic
  #else
    #error "Unsupported MSVC platform for pause instruction"
  #endif
#else
  #if defined(__x86_64__) || defined(__i386__) // amd64/x86_64 or i386/x86/x86_32
    #define LOGGER_PAUSE_INS() __asm__ volatile("pause" ::: "memory")
  #elif defined(__aarch64__) // ARM64
    #define LOGGER_PAUSE_INS() __asm__ volatile("yield" ::: "memory")
  #else
    #error "No such supported platform for pause instruction"
  #endif
#endif

/*
  Instance struct, tracks the context of the instance
  DO NOT touch anything by yourself, these can be changed
  anytime, use wrapper functions to get or set them
*/
struct Logger {
  LOGGER_ALIGN ATOMIC(bool) isAlive;
  bool isLocalTime;
  bool generateDefaultFile;
  LgLogPolicy logPolicy;
  int maxLogFiles; // non-positive = unlimited
  LgSink  sinks[LOGGER_MAX_SINKS + 1];
  size_t  sinks_count;
  log_formatter_t customLogFunc;
  uint32_t out_needed; // needed file flags for formatter
  pthread_t writer_th;
  LOGGER_ALIGN LogQueue queue;
#ifdef _POSIX_VERSION
  time_t cached_sec;
  struct tm cached_tm;
#endif
};

// consumer func, writes entries on the ring to stdout or file
LOGGER_INTERNAL void* lgi_consumer(void* arg) {
  Logger* inst = (Logger*)arg;
  int spins = 0;
  size_t start_pos = 0;

  while (atomic_load_explicit(&inst->isAlive, memory_order_acquire)) {
    if (lgi_queue_ppr_batch(inst, &start_pos)) spins = 0;
    else lgi_adaptive_wait(&spins);
  }

  while (lgi_queue_ppr_batch(inst, &start_pos))
    ;; // drain loop

  LG_DEBUG("Writer thread is exiting");
  return NULL;
}

LOGGER_INTERNAL ATOMIC(Logger*) active_instance = NULL;

int lg_init_flat(Logger* inst, const char* logs_dir,
                int local_time, int max_log_files, int generateDefaultFile,
                LgSinks sinks, LgLogPolicy log_policy,
                log_formatter_t log_formatter)
{
  LoggerConfig cfg;
  cfg.localTime = local_time;
  cfg.maxFiles = max_log_files;
  cfg.generateDefaultFile = generateDefaultFile;
  cfg.sinks = sinks;
  cfg.logPolicy = log_policy;
  cfg.logFormatter = log_formatter;
  return lg_init(inst, logs_dir, cfg);
}

int lg_init_defaults(Logger* instance, const char* logs_dir)
{
  return lg_init(instance, logs_dir, lg_get_defaults());
}

int lg_init(Logger* inst, const char* logs_dir, LoggerConfig config)
{
  // freaking c++
  size_t is_gen_def_file = 0;
  size_t scnt = 0;
  uint32_t needed = 0;
  Logger* expected = NULL;
  FILE* logFile;

  if (!inst || !logs_dir) goto fail;
  if (config.sinks.count > LOGGER_MAX_SINKS) {
    LG_DEBUG_ERR("Max amount of file sinks can be " LG_STRINGIFY(LOGGER_MAX_SINKS));
    goto fail;
  }

  is_gen_def_file = config.generateDefaultFile != 0;
  inst->isLocalTime = config.localTime != 0;
  inst->maxLogFiles = config.maxFiles;
  inst->generateDefaultFile = is_gen_def_file;
  inst->logPolicy = config.logPolicy;
  inst->customLogFunc = config.logFormatter;
#ifdef _POSIX_VERSION
  inst->cached_sec = 0;
#endif

  if (is_gen_def_file) {
    char dir[PATH_MAX];
    if (!lgi_normalize_path(logs_dir, dir, sizeof(dir))) {
      LG_DEBUG_ERR("Provided path is corrupted: %s", logs_dir);
      goto fail;
    }

    int dir_status = lgi_check_dir(dir);
    // handle not a valid directory
    if (dir_status == -1) {
      LG_DEBUG_ERR("Provided path is not a valid directory to create: %s", dir);
      goto fail;
    }

    // handle just non-exist directory (best effort)
    if (dir_status == 0) {
      if (!lgi_mkdir_p(dir)) {
        LG_DEBUG_ERR("Cannot create provided path: %s", dir);
        goto fail;
      }
    } else {
      if (config.maxFiles > 0) {
        // get .log files in that logs folder
        char oldestFile[PATH_MAX];
        int files = lgi_count_logs_and_get_oldest(dir, oldestFile, sizeof(oldestFile));
        if (files < 0) goto fail;

        // Remove the oldest file when max files are exceeded
        if (config.maxFiles > 0 && files >= config.maxFiles) {
          remove(oldestFile);
        }
      }
    }

    // get time str and length
    char time_str[LOGGER_TIME_STR_SIZE];
    if (!lg_get_time_str(inst, time_str)) goto fail;

    // produce file path with a fixed size
    char file_path[PATH_MAX];
    int n = snprintf(file_path, sizeof(file_path),
                     "%s%s" LOGGER_FILE_EXT, dir, time_str);
    if (n <= 0 || (size_t)n >= sizeof(file_path)) goto fail;

    // open file in write binary mode
    logFile = fopen(file_path, "wb");
    if (!logFile) {
      LG_DEBUG_ERR("Cannot open the log file: %s", file_path);
      goto fail;
    }
  } else logFile = NULL;

  lgi_queue_create(&inst->queue);

  scnt = config.sinks.count;
  memcpy(inst->sinks, config.sinks.items, scnt * sizeof(LgSink));
  inst->sinks_count = is_gen_def_file + scnt;
  if (is_gen_def_file) {
    inst->sinks[scnt] = LG_STRUCT(LgSink, logFile, LG_OUT_FILE);
  }

  for (size_t i = 0; i < inst->sinks_count; i++) {
    needed |= (1u << inst->sinks[i].type);
  }
  inst->out_needed = needed;

  atomic_store_explicit(&inst->isAlive, true, memory_order_release);
  if (pthread_create(&inst->writer_th, NULL, lgi_consumer, (void*)inst) != 0) {
    LG_DEBUG_ERR("Cannot create writer thread!");
    goto fail_thread;
  }

  atomic_compare_exchange_strong_explicit(
    &active_instance, &expected, inst,
    memory_order_release, memory_order_relaxed
  );
  return true;

fail_thread:
  atomic_store_explicit(&inst->isAlive, false, memory_order_release);
  if (logFile) fclose(logFile);
fail:
  return false;
}

int lg_vlog_(Logger* inst, const LgLogLevel level, const char* fmt, ...)
{
  if (!fmt) return false;

  // variadic resolving
  va_list args;
  va_start(args, fmt);
  char msg[LOGGER_MAX_MSG_SIZE];
  int mn = vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  if (mn < 0) {
    LG_DEBUG_ERR("Cannot resolve print format");
    return false;
  }

  return lg_log_(inst, level, msg, mn);
}

int lg_log_(Logger* inst, const LgLogLevel level, const char* msg, size_t msglen)
{
  if (!msg || msglen >= LOGGER_MAX_MSG_SIZE) return false;

  if (!inst || !lg_is_alive(inst)) {
    LG_DEBUG_ERR("Cannot log because the instance is dead!");
    return false;
  }

  LogQueue *q = &inst->queue;
  size_t pos = atomic_load_explicit(&q->head, memory_order_relaxed);
  size_t seq;
  LogSlot* s;
  int spins = 0;
  for (;;) {
    s = lgi_slot_get(q, pos);
    seq = atomic_load_explicit(&s->seq, memory_order_acquire);
    intptr_t diff = (intptr_t)(seq - pos);

    if (diff == 0) {
      if (atomic_compare_exchange_weak_explicit(
            &q->head, &pos, pos + 1,
            memory_order_relaxed, memory_order_relaxed)) {
        break; // claim success
      }
      // another producer claimed, retry
    } else if (diff < 0) {
      // ring is full
      switch(inst->logPolicy) {
      case LG_BLOCK:
        lgi_adaptive_wait(&spins);
        break;
      case LG_PRIORITY_BASED:
        if (level == LG_ERROR) {
          lgi_adaptive_wait(&spins);
          break;
        } else return false;
      default:
        return false;
      }
    } else { // pos stale, re-read
      pos = atomic_load_explicit(&q->head, memory_order_relaxed);
    }
  }

  LogPayload *pyld = &s->payload;
  memcpy(pyld->msg, msg, msglen + 1);
  pyld->length = msglen;
  pyld->level = level;

  // Slot is ready signal to consumer
  atomic_store_explicit(&s->seq, pos + 1, memory_order_release);
  return true;
}

int lg_set_active_instance(Logger* inst)
{
  if (!inst) return false;
  atomic_store_explicit(&active_instance, inst, memory_order_release);
  return 1;
}

Logger* lg_get_active_instance()
{
  return atomic_load_explicit(&active_instance, memory_order_acquire);
}

int lg_destroy(Logger* inst)
{
  if (!inst) return false;
  if (!lg_is_alive(inst)) {
    LG_DEBUG_ERR("Logger is already dead!");
    return false;
  }
  atomic_store_explicit(&inst->isAlive, false, memory_order_release);
  pthread_join(inst->writer_th, NULL);

  // close the files if they're not closed
  for (size_t i = 0; i < inst->sinks_count; i++) {
    LgSink* s = &inst->sinks[i];
    FILE* f = s->file;
    if (!f) continue;
    if (f != stderr && f != stdout && f != stdin) {
      if (fclose(f) != 0) {
        LG_DEBUG_ERR("Log file cannot be closed!");
        return false;
      }
    } else fflush(f);
    inst->sinks[i].file = NULL;
  }

  Logger* expected = inst;
  atomic_compare_exchange_strong_explicit(
    &active_instance, &expected, NULL,
    memory_order_release, memory_order_relaxed
  );
  return true;
}

int lg_is_alive(const Logger* inst)
{
  const Logger* ins = inst ? inst : lg_get_active_instance();
  if (!ins) return false;
  return atomic_load_explicit(&ins->isAlive, memory_order_acquire);
}

Logger* lg_alloc()
{
  Logger* tmp = (Logger*)calloc(1, sizeof(Logger));
  if (!tmp) {
    LG_DEBUG_ERR("Cannot allocate Logger instance!");
    return NULL;
  }
  return tmp;
}

void lg_free(Logger* inst)
{
  free(inst);
}

// Explicit instances
int lg_flogi(Logger* inst, const LgLogLevel level, const char* msg)
{
  if (!msg) return false;
  if (!inst) return lg_log_(lg_get_active_instance(), level, msg, strlen(msg));
  return lg_log_(inst, level, msg, strlen(msg));
}
int lg_finfoi(Logger* inst, const char* msg)
{
  return lg_flogi(inst, LG_INFO, msg);
}
int lg_ferrori(Logger* inst, const char* msg)
{
  return lg_flogi(inst, LG_ERROR, msg);
}
int lg_fwarni(Logger* inst, const char* msg)
{
  return lg_flogi(inst, LG_WARNING, msg);
}

// Implicit instances
int lg_flog(const LgLogLevel level, const char* msg)
{
  return lg_flogi(NULL, level, msg);
}
int lg_finfo(const char* msg)
{
  return lg_flogi(NULL, LG_INFO, msg);
}
int lg_ferror(const char* msg)
{
  return lg_flogi(NULL, LG_ERROR, msg);
}
int lg_fwarn(const char* msg)
{
  return lg_flogi(NULL, LG_WARNING, msg);
}

const char* lg_lvl_to_str(const LgLogLevel level)
{
  switch (level) {
  case LG_INFO:
    return "INFO";
  case LG_ERROR:
    return "ERROR";
  case LG_WARNING:
    return "WARNING";
  case LG_CUSTOM:
    return "CUSTOM";
  default:
    return "NULL";
  }
}

LoggerConfig lg_get_defaults() {
  LgSinks sinks = { {LG_STRUCT(LgSink, stdout, LG_OUT_TTY) }, 1};
  LoggerConfig cfg;
  cfg.localTime = true;
  cfg.maxFiles = 0;
  cfg.generateDefaultFile = true;
  cfg.sinks = sinks;
  cfg.logPolicy = LG_DROP;
  cfg.logFormatter = NULL;
  return cfg;
}

int lg_append_sink(LoggerConfig* config, FILE* f, LgOutType type) {
  if (!config) return false;
  if (config->sinks.count >= LOGGER_MAX_SINKS) return false;
  config->sinks.items[config->sinks.count++] = LG_STRUCT(LgSink, f, type);
  return true;
}

int lg_get_time_str(Logger* inst, char* buf)
{
#ifdef _WIN32
  SYSTEMTIME st;
  if (inst->isLocalTime) GetLocalTime(&st);
  else GetSystemTime(&st);
  int year = st.wYear;
  int month = st.wMonth;
  int day = st.wDay;
  int hours = st.wHour;
  int minutes = st.wMinute;
  int seconds = st.wSecond;
  long millis = st.wMilliseconds;
#else

  struct timespec ts;
#if defined(CLOCK_REALTIME_COARSE) && !defined(LOGGER_GET_REAL_TIME)
  // get coarse time - much more efficient (Linux only)
  clock_gettime(CLOCK_REALTIME_COARSE, &ts);
#else
  // get exact time (macOS and other platforms)
  clock_gettime(CLOCK_REALTIME, &ts);
#endif

  if (ts.tv_sec != inst->cached_sec) {
    if (inst->isLocalTime)
      localtime_r(&ts.tv_sec, &inst->cached_tm);
    else
      gmtime_r(&ts.tv_sec, &inst->cached_tm);
    inst->cached_sec = ts.tv_sec;
  }
  int year = inst->cached_tm.tm_year + 1900;
  int month = inst->cached_tm.tm_mon + 1;
  int day = inst->cached_tm.tm_mday;
  int hours = inst->cached_tm.tm_hour;
  int minutes = inst->cached_tm.tm_min;
  int seconds = inst->cached_tm.tm_sec;
  long millis = ts.tv_nsec / 1000000;
#endif // _WIN32

  lgi_time_write4(buf, year);
  buf[4]  = '.';
  lgi_time_write2(buf+5, month);
  buf[7]  = '.';
  lgi_time_write2(buf+8, day);
  buf[10] = '-';
  lgi_time_write2(buf+11, hours);
  buf[13] = '.';
  lgi_time_write2(buf+14, minutes);
  buf[16] = '.';
  lgi_time_write2(buf+17, seconds);
  buf[19] = '.';
  lgi_time_write3(buf+20, millis);
  buf[23] = '\0';
  return true;
}

LOGGER_INTERNAL int lgi_check_dir(const char* path)
{
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(path);
  if (attr == INVALID_FILE_ATTRIBUTES) return 0;
  if (attr & FILE_ATTRIBUTE_DIRECTORY)
    return 1;
  return -1; 
#else
  struct stat st;
  if (stat(path, &st) != 0) return 0;  // not exists
  if (S_ISDIR(st.st_mode)) return 1;   // exists AND directory (valid)
  return -1;                           // exists but not a directory
#endif
}

LOGGER_INTERNAL bool lgi_mkdir_p(char* path)
{
  if (!path || !*path) return false;

  // go char by char
  for (char* p = path + 1; *p; p++) {
    if (*p != LOGGER_PATH_SEP) continue;
    *p = '\0';
    LG_DEBUG("Trying to create: %s", path);
    int status = LOGGER_MKDIR(path);
    if (status != 0 && errno != EEXIST) {
      LG_DEBUG_ERR("Failed to create path: %s", path);
      return false;
    }
    *p = LOGGER_PATH_SEP;
  }
  return true;
}

LOGGER_INTERNAL int lgi_count_logs_and_get_oldest(const char* path, char* oldest_path, size_t opsz) {
  int count = 0;
#ifdef _WIN32
  char pattern[PATH_MAX];
  snprintf(pattern, sizeof(pattern), "%s\\*" LOGGER_FILE_EXT, path);

  WIN32_FIND_DATAA fdata;
  HANDLE h = FindFirstFileA(pattern, &fdata);
  if (h == INVALID_HANDLE_VALUE) return -1;

  FILETIME oldest_ft;
  oldest_ft.dwLowDateTime = MAXDWORD;
  oldest_ft.dwHighDateTime = MAXDWORD;
  do {
    count++;

    FILETIME ft = fdata.ftLastWriteTime;
    if (oldest_path && CompareFileTime(&ft, &oldest_ft) < 0) {
      oldest_ft = ft;
      snprintf(oldest_path, opsz, "%s\\%s", path, fdata.cFileName);
    }
  } while (FindNextFileA(h, &fdata));

  FindClose(h);
#else
  LG_UNUSED(opsz);
  DIR* dir = opendir(path);
  if (!dir) return -1;
  time_t oldest_mtime = LLONG_MAX;

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    const char* name = entry->d_name;
    size_t len = strlen(name);
    if (len <= LOGGER_FILE_EXT_SZ ||
        memcmp(name + len - LOGGER_FILE_EXT_SZ, LOGGER_FILE_EXT, LOGGER_FILE_EXT_SZ) != 0)
      continue;

    count += 1;
    char full_path[PATH_MAX];
    int n = snprintf(full_path, sizeof(full_path), "%s/%s", path, name);
    if (n < 0 || (size_t)n >= sizeof(full_path)) continue;
    struct stat st;
    if (stat(full_path, &st) == 0 && st.st_mtime < oldest_mtime) {
      oldest_mtime = st.st_mtime;
      memcpy(oldest_path, full_path, n + 1);
    }
  }

  if (closedir(dir) != 0) return -1;
#endif
  return count;
}

LOGGER_INTERNAL bool lgi_normalize_path(const char* path, char* out, size_t size)
{
  if (!path || !*path) return false;
  char* dst = out;
  char* end = out + size - 1;

  for (const char* src = path; *src && dst < end; src++) {
    // double slash skip
    if (*src == LOGGER_PATH_SEP
        && dst != out
        && *(dst-1) == LOGGER_PATH_SEP) continue;
    *dst++ = *src;
  }
  *dst = '\0';

  // add trailing slash
  if (dst + 1 >= end) return false;
  size_t len = dst - out;
  out[len] = LOGGER_PATH_SEP;
  out[len + 1] = '\0';
  return true;
}

void lg_str_format_into(LgString* s, const char* fmt, ...)
{
  if (!s) return;
  size_t cap = sizeof(s->data);

  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(s->data, cap, fmt, ap);
  va_end(ap);

  // guarentees \n at the end
  if (n < 0) {
    s->len = 0;
  } else if ((size_t)n >= cap) {
    if (cap >= 2) {
      s->data[cap - 2] = '\n';
      s->data[cap - 1] = '\0';
      s->len = cap - 1;
    } else {
      s->len = 0;
    }
  } else {
    s->len = (size_t)n;
  }
}

void lg_str_write_into(LgString* s, const char* str)
{
  if (!str || !s) return;
  size_t len = strlen(str);
  if (len == 0) return;
  if (len >= sizeof(s->data)) len = sizeof(s->data) - 2;
  memcpy(s->data, str, len);
  s->data[len - 1] = '\n';
  s->data[len] = '\0';
  s->len = len;
}

LOGGER_INTERNAL int lgi_def_format_msg(
  const char* time_str, LgLogLevel level,
  const char* msg, uint32_t needed, LgMsgPack pack)
{
  const char* level_str = lg_lvl_to_str(level);

  // Colorized TTY formatting (you can disable)
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_TTY)) {
    LgString* s = &pack[LG_OUT_TTY];
    char* p = s->data;
    char* end = p + sizeof(s->data) - 1;
#ifndef LOGGER_DONT_COLORIZE
    const char* clr;
    switch (level) {
      case LG_ERROR:   clr = LOGGER_CLR_RED;    break;
      case LG_INFO:    clr = LOGGER_CLR_GREEN;   break;
      case LG_WARNING: clr = LOGGER_CLR_YELLOW;  break;
      default:         clr = LOGGER_CLR_RST;     break;
    }
    lgi_str_append_n(&p, end, LOGGER_CLR_AQUA);
    lgi_str_append_n(&p, end, time_str);
    lgi_str_append_n(&p, end, " ");
    lgi_str_append_n(&p, end, clr);
    lgi_str_append_n(&p, end, "[");
    lgi_str_append_n(&p, end, level_str);
    lgi_str_append_n(&p, end, "]");
    lgi_str_append_n(&p, end, LOGGER_CLR_RST);
    lgi_str_append_n(&p, end, " ");
#else
    lgi_str_append_n(&p, end, time_str);
    lgi_str_append_n(&p, end, " [");
    lgi_str_append_n(&p, end, level_str);
    lgi_str_append_n(&p, end, "] ");
#endif
    lgi_str_append_n(&p, end, msg);
    lgi_str_close(&p, end);
    s->len = (size_t)(p - s->data);
  }

  // Raw message and other specifiers, no color or markup language
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_FILE)) {
    LgString* s = &pack[LG_OUT_FILE];
    char* p = s->data;
    char* end = p + sizeof(s->data) - 1;
    lgi_str_append_n(&p, end, time_str);
    lgi_str_append_n(&p, end, " [");
    lgi_str_append_n(&p, end, level_str);
    lgi_str_append_n(&p, end, "] ");
    lgi_str_append_n(&p, end, msg);
    lgi_str_close(&p, end);
    s->len = (size_t)(p - s->data);
  }

  // JSON style formatting
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_NET)) {
    LgString* s = &pack[LG_OUT_NET];
    char* p = s->data;
    char* end = p + sizeof(s->data) - 1;
    lgi_str_append_n(&p, end, "{\"timestamp\":\"");
    lgi_str_append_n(&p, end, time_str);
    lgi_str_append_n(&p, end, "\",\"level\":\"");
    lgi_str_append_n(&p, end, level_str);
    lgi_str_append_n(&p, end, "\",\"message\":\"");
    lgi_str_append_n(&p, end, msg);
    lgi_str_append_n(&p, end, "\"}");
    lgi_str_close(&p, end);
    s->len = (size_t)(p - s->data);
  }
  return true;
}

LOGGER_INTERNAL void lgi_queue_create(LogQueue* q) {
  q->tail = 0;
  atomic_store_explicit(&q->head, 0, memory_order_relaxed);

  for (size_t i = 0; i < LOGGER_RING_SIZE; i++) {
    LogSlot* s = (LogSlot*)(q->slots + i * LOGGER_RING_STRIDE);
    atomic_store_explicit(&s->seq, i, memory_order_relaxed);
  }

  atomic_thread_fence(memory_order_seq_cst);
}

LOGGER_INTERNAL size_t lgi_queue_pop_batch(LogQueue* q, size_t* start_pos, size_t max_batch)
{
  size_t pos = q->tail;
  size_t count = 0;

  while (count < max_batch) {
    LogSlot* s = lgi_slot_get(q, pos + count);
    size_t seq = atomic_load_explicit(&s->seq, memory_order_acquire);
    if (seq != (pos + count + 1)) {
      break; // not ready yet
    }
    count++;
  }

  if (count > 0) {
    *start_pos = pos;
    q->tail = pos + count;
  }
  return count;
}

LOGGER_INTERNAL bool lgi_queue_ppr_batch(Logger* inst, size_t* start_pos) {
#ifdef _WIN32
  size_t max_batches = 1;
#else
  size_t max_batches = LOGGER_MAX_BATCH;
#endif
  size_t count = lgi_queue_pop_batch(&inst->queue, start_pos, max_batches);
  if (count == 0) return false;
  char time_str[LOGGER_TIME_STR_SIZE];
  log_formatter_t fn = inst->customLogFunc ? inst->customLogFunc : lgi_def_format_msg;

#ifdef _WIN32
  LgMsgPack pack = {};
  LogSlot* s = lgi_slot_get(&inst->queue, *start_pos);
  if (!lg_get_time_str(inst, time_str)) goto release;

  if (!fn(time_str, s->payload.level, s->payload.msg, inst->out_needed, pack))
    goto release;

  for (size_t i = 0; i < inst->sinks_count; i++) {
    LgSink* sk = &inst->sinks[i];
    if (!sk->file) continue;
    LgString* str = &pack[sk->type];
    fwrite(str->data, sizeof(*str->data), str->len, sk->file);
  }

release:
  lgi_queue_release(&inst->queue, *start_pos);

#else
  LgMsgPack msg_packs[LOGGER_MAX_BATCH];
  struct iovec vecs[LOGGER_MAX_OUT_TYPES][LOGGER_MAX_BATCH];
  int vec_counts[LOGGER_MAX_OUT_TYPES] = {0};

  for (size_t i = 0; i < count; i++) {
    size_t pos = *start_pos + i;
    LogSlot* s = lgi_slot_get(&inst->queue, pos);

    if (!lg_get_time_str(inst, time_str)) goto release;
    if (!fn(time_str, s->payload.level, s->payload.msg, inst->out_needed, msg_packs[i]))
      goto release;

    for (size_t t = 0; t < LOGGER_MAX_OUT_TYPES; t++) {
      if (msg_packs[i][t].len == 0) continue;
      vecs[t][vec_counts[t]].iov_base = msg_packs[i][t].data;
      vecs[t][vec_counts[t]].iov_len  = msg_packs[i][t].len;
      vec_counts[t]++;
    }

  release:
    lgi_queue_release(&inst->queue, pos);
  }

  for (size_t i = 0; i < inst->sinks_count; i++) {
    LgSink* sk = &inst->sinks[i];
    if (!sk->file || vec_counts[sk->type] == 0) continue;
    writev(fileno(sk->file), vecs[sk->type], vec_counts[sk->type]);
  }
#endif

  return true;
}

LOGGER_INTERNAL void lgi_queue_release(LogQueue* q, size_t pos) {
  LogSlot* s = lgi_slot_get(q, pos);
  atomic_store_explicit(&s->seq, pos + LOGGER_RING_SIZE, memory_order_release);
}

LOGGER_INTERNAL void lgi_adaptive_wait(int* spins) {
  if (*spins < LOGGER_WAIT_NO_PAUSE_MAGIC) {
    *spins += 1;
  } else if (*spins < LOGGER_WAIT_PAUSE_MAGIC) {
    *spins += 1;
    LOGGER_PAUSE_INS();
  } else {
    // Kernel'll floor this value to ~70-80 us
    // so this constant doesnt matter if it's in between 50-100 us.
    LOGGER_SLEEP(60);
  }
}

FILE* lg_get_stdout() { return stdout; }
FILE* lg_get_stderr() { return stderr; }

FILE* lg_fopen(const char* path) {
  return fopen(path, "wb");
}

#endif  // LOGGER_IMPLEMENTATION
// IMPLEMENTATION END (DO NOT CHANGE THIS LINE!)
#endif  /* LOGGER_H */
