// THIS IS PRE-RELEASE OF LOGGER!
// NOT ALL ENVIRONMENTS CAN SUPPORT!
// TESTED ON X86_64 GNU/LINUX

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

/*
  THIS IS STB-STYLE LIBRARY HEADER OF LOGGER.
  IT IS WRITTEN IN PURE C, SAFE TO USE IN C++
  REQUIRES MINIMUM C11 VERSION
  TESTED ON AMD64 GNU/LINUX WITH GCC/CLANG

  MACROS THAT YOU CAN USE:
  LOGGER_IMPLEMENTATION -> Implementation of this header (USE THIS ONCOMPILATION OR USAGE)
  LOGGER_MINIFY_PREFIXES -> Minifies "lg_" prefix into just "l" including lg_log
  LOGGER_DONT_COLORIZE -> Disables the colorized stdout messages at default formatter
  LOGGER_DEBUG -> If this enabled, logger's internal errors will be shown at stderr.
  --> You may doesnt want to use this at production

  USAGE IN C:
  #define LOGGER_IMPLEMENTATION
  #define LOGGER_MINIFY_PREFIXES
  #include <stdio.h>

  #include "logger.h"

  int main() {
    Logger* lg = lg_alloc();
    if (!lg_init(lg, "logs", {.localTime=1, .printStdout=1}))
      return 1;
    char name[33];
    scanf("%32s", &name);
    lg_info("Hello, %s!", name);
    linfo("Here is the logger!");
    if (!lg_destroy(lg)) return 1;
  }
*/

// Better version of hybrid solution of STB and DLLs on every platform
// This way, you can generate dll/so and use it or just use header
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

// some global includes here
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// These variables can be fine-tuned due to your traffic
// High traffic = high these constants
// Low traffic = lower values
#define LOGGER_WAIT_NO_PAUSE_MAGIC 100
#define LOGGER_WAIT_PAUSE_MAGIC 1000

/*
  Defines time_str from get_time_str size (WITH ZERO AT THE END!)
  All of time_str related functions uses this, if you
  ever change lg_get_time_str function, update this
*/
#define LOGGER_TIME_STR_SIZE 24

// logger max message size (you can change it)
#define LOGGER_MAX_MSG_SIZE 1024
// ring buffer's size
#define LOGGER_RING_SIZE 1024

// Maximum amount of files that can be in the sink (not really useful now)
#define LOGGER_MAX_FILES 8

// File extension for files and extension string's size
#define LOGGER_FILE_EXT ".log"
#define LOGGER_FILE_EXT_SZ 4

#define LOGGER_CONTAINS_FLAG(main, flag) (main & (1u << flag))

// THE ANSI bash color codes/escape characters
#define LOGGER_CLR_RED "\x1b[31m"     // for ERRORs
#define LOGGER_CLR_GREEN "\x1b[32m"   // for INFOs
#define LOGGER_CLR_YELLOW "\x1b[33m"  // for WARNs
#define LOGGER_CLR_AQUA "\x1b[36m"    // for TIMESTAMPs
#define LOGGER_CLR_RST "\x1b[0m"      // for plain message

typedef enum {
  LG_INFO = 1 << 0,
  LG_ERROR = 1 << 1,
  LG_WARNING = 1 << 2,
  LG_CUSTOM = 1 << 3,
  // Add more levels here
} LgLogLevel;

/*
  Log policies when ring buffer is full
  LG_DROP (default): Drops the log when buffer is full, you lost some log messages
  LG_BLOCK: Blocks the producer thread until there's a unclaimed slot exist
  LG_PRIORITY_BASED: Blocks the producer on log level errors, otherwise it drops
*/
typedef enum {
  LG_DROP = 1 << 0,
  LG_BLOCK = 1 << 1,
  LG_PRIORITY_BASED = 1 << 2,
} LgLogPolicy;

/*
  Describes type of the formatted message
  Use this in formatters
*/
typedef enum {
  LG_OUT_TTY = 0,
  LG_OUT_FILE = 1,
  LG_OUT_NET = 2,
  // Add more out types here
  // Dont forget to update this:
  LG_OUT_MAX = 3,
} LgOutType;

typedef struct Logger Logger;

typedef struct LgString {
  char data[LOGGER_MAX_MSG_SIZE];
  size_t len;
} LgString;

typedef LgString LgMsgPack[LG_OUT_MAX];

/*
  Formatter callback type
*/
typedef int (*log_formatter_t)(
  int isLocalTime,
  LgLogLevel level,
  const char* msg,
  uint32_t needed, // needed slots
  LgMsgPack out
);

/*
 * Config struct, this struct can be used in lg_init.
 * localTime: uses your localtime using kernel if it's true
 * printStdout: the logger tries to print message to stdout
   if it's true, otherwise it doesnt
 * logFormatter: TBD
 * logPolicy: the log policy which determines what the logger
   does when the ring buffer is empty
 * maxFiles: maximum amount of .log files that is in the
   logs folder. Non-positive values'll be treated as unlimited
   If this is exceeded, it'll remove the oldest log file
*/
typedef struct {
  int localTime;
  int printStdout;
  int maxFiles;
  LgLogPolicy logPolicy;
  log_formatter_t logFormatter;
} LoggerConfig;

// portable printf-format style checker (only available on gcc and clang)
#if defined(__clang__) || defined(__GNUC__)
  #ifdef __cplusplus
    #define PRINTF_LIKE(fmt, args) [[gnu::format(printf, fmt, args)]]
  #else
    #define PRINTF_LIKE(fmt, args) __attribute__((format(printf, fmt, args)))
  #endif
#else
  #define PRINTF_LIKE(fmt, args)
#endif

/*
  Main initializer function, Creates Logger instance
  @param logs_dir: const char*, Points a directory path
  (if not exists, it'll try to create). It can be relative or absolute path.
  @param config: LoggerConfig

  RECOMMENDED: on your app's entry point, check its return value like
  if(!lg_init(...)) You dont have to use lg_destroy() if init failed. Because
  if init failed, isAlive set to be 0 and lg_destroy() simply wont work if the
  logger instance is dead
*/
LOGGERDEF int lg_init(Logger* instance, const char* logs_dir,
                      LoggerConfig config);

/*
  lg_init but Flattened the config struct (mostly used at FFIs)
*/
LOGGERDEF int lg_init_flat(Logger* inst, const char* logs_dir,
                          int local_time, int print_stdout, int max_log_files,
                          LgLogPolicy log_policy,
                          log_formatter_t log_formatter);

/*
  Destroys specific logger instance and closes the log file that the instance
  working on if instance = NULL, it tries to destroy active one active_instance
  WONT become NULL
*/
LOGGERDEF int lg_destroy(Logger* instance);

// returns if logger is alive
LOGGERDEF int lg_is_alive(const Logger* instance);

/*
  This is the main producer function. It accepts format
  resolved message and manipulates it by customization.
  It pushes final message to be printed to the ring buffer.
  Then invokes the writer thread.
*/
LOGGERDEF int lg_producer(Logger* inst, const LgLogLevel level,
                          const char* msg, size_t msglen);

/*
  This is the format resolver for lg_producer.
  C macros use this. It's sensitive to "%" char.
  It may crash your app if you're using this on FFI or wrongly on C
*/
LOGGERDEF int lg_vproducer(Logger* inst, const LgLogLevel level,
                          const char* fmt, ...) PRINTF_LIKE(3, 4);

/*
  Wrapper log functions for FFI, if you dont use C/C++ or
  using your language's FFI, you must use these:
*/
LOGGERDEF int lg_flogi(Logger* inst, const LgLogLevel level, const char* msg);
LOGGERDEF int lg_flog(const LgLogLevel level, const char* msg);

// Explicit instances
LOGGERDEF int lg_finfoi(Logger* inst, const char* msg);
LOGGERDEF int lg_ferrori(Logger* inst, const char* msg);
LOGGERDEF int lg_fwarni(Logger* inst, const char* msg);

// Implicit instances (uses active one)
LOGGERDEF int lg_finfo(const char* msg);
LOGGERDEF int lg_ferror(const char* msg);
LOGGERDEF int lg_fwarn(const char* msg);

/*
  Sets active Logger instance
*/
LOGGERDEF int lg_set_active_instance(Logger* inst);

/*
  Gets active Logger instance
*/
LOGGERDEF Logger* lg_get_active_instance();

/*
  Manipulates LgLogLevel enum into string
*/
LOGGERDEF const char* lg_lvl_to_str(const LgLogLevel level);

/*
  Simply acts like snprintf and applies it to the lg_string
  Processes "%" style printf format (variadics), it may cause UBs
*/
LOGGERDEF void lg_str_format_into(LgString* s, const char* fmt, ...)
  PRINTF_LIKE(2, 3);

/*
  lg_str_format_into but already formatted strings
  Use it at FFI because you can't use variadics there
*/
LOGGERDEF void lg_str_write_into(LgString* s,
                                const char* already_formatted_str);

/*
  Gets time using kernel in this format:
  %Y.%m.%d-%H.%M.%S.%MS (%MS is millis in 3 digits)
  example: 2026.01.11-21.44.40.255 means
  January 11th, 2026, 21:44:40 or 9:44:40 pm, ms: 255

  If isLocalTime = false, spits out time in UTC format.
  Accepts buffer to write and its size
*/
LOGGERDEF int lg_get_time_str(char* buf, int isLocalTime);

/*
  For FFI, Calls calloc and returns it
*/
LOGGERDEF Logger* lg_alloc();

/*
  For FFI, Calls free
*/
LOGGERDEF void lg_free(Logger* inst);

// Log with an explicit logger instance
#define lg_logi(instance, level, fmt, ...) \
  lg_vproducer(instance, level, fmt, ##__VA_ARGS__)

#define lg_infoi(instance, fmt, ...) \
  lg_logi(instance, LG_INFO, fmt, ##__VA_ARGS__)
#define lg_errori(instance, fmt, ...) \
  lg_logi(instance, LG_ERROR, fmt, ##__VA_ARGS__)
#define lg_warni(instance, fmt, ...) \
  lg_logi(instance, LG_WARNING, fmt, ##__VA_ARGS__)

// log functions to be going to used is macros now
// main function is lg_vproducer
#define lg_log(level, fmt, ...) \
  lg_logi(lg_get_active_instance(), level, fmt, ##__VA_ARGS__)

#define lg_info(fmt, ...) lg_log(LG_INFO, fmt, ##__VA_ARGS__)
#define lg_error(fmt, ...) lg_log(LG_ERROR, fmt, ##__VA_ARGS__)
#define lg_warn(fmt, ...) lg_log(LG_WARNING, fmt, ##__VA_ARGS__)

// you can add your custom level like this:
#define lg_custom(fmt, ...) lg_log(LG_CUSTOM, fmt, ##__VA_ARGS__)

// minify prefix from lg_ to l
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

// stb style implementation macros
#ifdef LOGGER_IMPLEMENTATION

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Transpilation from C++11 to C11
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
#else
#include <stdatomic.h>
#define ATOMIC(T) _Atomic(T)
#endif

#define LOGGER_CACHE_LINE 64
#define LOGGER_ALIGN __attribute__((aligned(LOGGER_CACHE_LINE)))

typedef struct {
  char msg[LOGGER_MAX_MSG_SIZE];
  size_t length;
  LgLogLevel level;
} LogPayload;

typedef struct {
  ATOMIC(size_t) seq;
  char payload[];
} LogSlot;

typedef struct {
  size_t capacity; // power of 2
  size_t mask; // capacity - 1 

  LOGGER_ALIGN ATOMIC(size_t) head; // Producer
  LOGGER_ALIGN size_t tail; // Consumer
  uint8_t slots[];
} LogQueue;

// Static function forward-declerations
// checks if dir is a valid directory (exists and directory)
static int check_dir(const char* path);

// normalizes path to acceptable ones (>1 slash skips, removing trailing slash)
static bool normalize_path(const char* path, char* out, size_t size);

// count .log files and print oldest one to oldest_path in that folder
// negative return value is error status
static int count_logs_and_get_oldest(const char* path, char* oldest_path, size_t oldest_path_size);

// recursively creates folders (behaves like mkdir -p)
static bool mkdir_p(char* path);

// default/fallback log formatter
static int format_msg(int isLocalTime, LgLogLevel level,
                     const char* msg, uint32_t needed, LgMsgPack pack);

// Extracts file pointer to LgOutType
static LgOutType lg_get_out_type(FILE* f);

// Return exact size of slot with padding, payload and seq
static size_t lg_slot_stride();

// Return the slot by idx from the given queue
static LogSlot* lg_slot_get(LogQueue* q, size_t idx);

// Extract payload from that slot
static LogPayload* lg_slot_payload(LogSlot* s);

// Create MPSC queue with given capacity (capacity must be power of 2)
static LogQueue* lg_queue_create(size_t capacity);

// Pop something from the MPSC queue (Consumer)
static bool lg_queue_pop(LogQueue* q, LogPayload** out);

// Processes the payload (writes message to proper fds)
static bool lg_process_payload(Logger* inst, LogPayload* payload);

// Adaptive waiters, not only spins, it spins until reaches
// LOGGER_NO_PAUSE_MAGIC and LOGGER_PAUSE_MAGIC values
// Pause is pause instruction no pause -> just spins
// No pause means spin and pause
// If pause magic exceeded, it sleeps (Sleep or nanosleep)
static void lg_adaptive_wait(int* spins);

// i stands for "internal", so lgierror means logger internal error
// lgierror - prints out where it happened
#define lgierror(fmt, ...)                                          \
  do {                                                              \
    fprintf(stderr, "%s:%d: ERROR: " fmt "\n", __FILE__, __LINE__,  \
            ##__VA_ARGS__);                                         \
  } while (0)

// lgiprint - auto adds \n at the end and where
#define lgiprint(fmt, ...)                                              \
  do {                                                                  \
    printf("%s:%d: INFO: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define LG_UNUSED(x) (void)(x)

// define this macro to enable error messages in stderr
// some sort of new version of "LOGGER_VERBOSE" but not exactly the same thing
#ifdef LOGGER_DEBUG
#define LG_DEBUG_ERR(fmt, ...) lgierror(fmt, ##__VA_ARGS__)
#define LG_DEBUG(fmt, ...) lgiprint(fmt, ##__VA_ARGS__)
#else
#define LG_DEBUG_ERR(fmt, ...) // swallow
#define LG_DEBUG(fmt, ...)
#endif  // LOGGER_DEBUG

// sleep for us microseconds
#ifdef _WIN32
#include <direct.h>  // _mkdir
#include <windows.h>
#include <intrin.h>
#include <io.h>

#define MKDIR(path) _mkdir(path)
#define PATH_SEP '\\'
#define PATH_MAX (MAX_PATH * 2) // for wchar_t
#define LOGGER_PAUSE_INS() _mm_pause()
#define isatty _isatty
#define fileno _fileno

// Sleep() has terrible resolution (milliseconds)
// But who uses winbloat for production-ready logger?
#define LOGGER_SLEEP(us)                        \
  do {                                          \
    Sleep(1);                                   \
  } while (0)

#else // POSIX:
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>  // for dirs
#include <sys/time.h>  // for time
#include <dirent.h>
#include <unistd.h>

#define LOGGER_SLEEP(us)                                            \
  do {                                                              \
    struct timespec ts = {(us) / 1000000, ((us) % 1000000) * 1000}; \
    nanosleep(&ts, NULL);                                           \
  } while (0)
#define MKDIR(path) mkdir(path, 0755)
#define PATH_SEP '/'

#ifdef __x86_64__
  #define LOGGER_PAUSE_INS() __asm__ volatile("pause" ::: "memory")
#elif defined(__aarch64__)
  #define LOGGER_PAUSE_INS() __asm__ volatile("yield" ::: "memory")
#else
#error "No such supported platform for pause instruction"
#endif

#endif

/*
  Instance struct, tracks the context of the instance
  The public and private logic is that its NOT RECOMMENDED
  to touch private fields because they can change any time
  and if you change one there this may cause UB or crash
  Public fields on the other hand, you init the logger
  and they'll never change
  Use wrapper functions to get private fields if needed
*/
struct Logger {
  ///////////////// PUBLIC FIELDS (SAFE TO TOUCH)
  bool isLocalTime;
  bool isPrintStdout;
  LgLogPolicy logPolicy;
  int maxLogFiles; // non-positive = unlimited
  log_formatter_t customLogFunc;

  ///////////////// PRIVATE FIELDS (DO NOT TOUCH OUTSIDE)
  ATOMIC(bool) isAlive;
  FILE* files[LOGGER_MAX_FILES];
  size_t files_count;
  unsigned int out_needed; // needed file flags for formatter
  pthread_t writer_th;
  LogQueue* queue;
};

// consumer func, writes entries on the ring to stdout or file
static void* lg_consumer(void* arg) {
  Logger* inst = (Logger*)arg;
  LogPayload* entry = NULL;
  int spins = 0;

  while (atomic_load_explicit(&inst->isAlive, memory_order_acquire)) {
    if (lg_queue_pop(inst->queue, &entry)) {
      spins = 0;
      lg_process_payload(inst, entry);
    } else {
      lg_adaptive_wait(&spins);
    }
  }

  // isAlive = false, drain remaining entries
  while (lg_queue_pop(inst->queue, &entry)) {
    lg_process_payload(inst, entry);
  }
  return NULL;
}

// active logger instance if NULL, init tries to set it
static Logger* active_instance = NULL;

int lg_init_flat(Logger* inst, const char* logs_dir,
                int local_time, int print_stdout, int max_log_files,
                LgLogPolicy log_policy,
                log_formatter_t log_formatter)
{
  LoggerConfig cfg;
  cfg.localTime = local_time;
  cfg.printStdout = print_stdout;
  cfg.maxFiles = max_log_files;
  cfg.logPolicy = log_policy;
  cfg.logFormatter = log_formatter;
  return lg_init(inst, logs_dir, cfg);
}

// main init func
int lg_init(Logger* inst, const char* logs_dir, LoggerConfig config)
{
  if (!inst || !logs_dir) return false;

  inst->isPrintStdout = config.printStdout != 0;
  inst->isLocalTime = config.localTime != 0;
  inst->maxLogFiles = config.maxFiles;
  inst->customLogFunc = config.logFormatter;
  inst->logPolicy = config.logPolicy;

  char dir[PATH_MAX];
  if (!normalize_path(logs_dir, dir, sizeof(dir))) {
    LG_DEBUG_ERR("Provided path is corrupted: %s", logs_dir);
    return false;
  }

  int dir_status = check_dir(dir);  // -1 = NOT valid directory, 0 = NOT exists
  // handle not a valid directory
  if (dir_status == -1) {
    LG_DEBUG_ERR("Provided path is not a valid directory to create: %s", dir);
    return false;
  }

  // handle just non-exist directory (best effort)
  if (dir_status == 0) {
    if (!mkdir_p(dir)) {
      LG_DEBUG_ERR("Cannot create provided path: %s", dir);
      return false;
    }
  } else {
    if (config.maxFiles > 0) {
      // get .log files in that logs folder
      char oldestFile[PATH_MAX];
      int files = count_logs_and_get_oldest(dir, oldestFile, sizeof(oldestFile));
      if (files < 0) {
        LG_DEBUG_ERR("Cannot count files and get oldest in %s", dir);
        return false;
      }

      // Remove the oldest file when max files are exceeded
      if (config.maxFiles > 0 && files >= config.maxFiles) {
        remove(oldestFile);
      }
    }
  }

  // get time str and length
  char time_str[LOGGER_TIME_STR_SIZE];
  if (!lg_get_time_str(time_str, config.localTime)) return false;

  // produce file path with a fixed size
  char file_path[PATH_MAX];
  int n = snprintf(file_path, sizeof(file_path),
                   "%s%s" LOGGER_FILE_EXT, dir, time_str);
  if (n <= 0 || (size_t)n >= sizeof(file_path)) return false;

  // open file in write binary mode
  FILE* logFile = fopen(file_path, "wb");
  if (!logFile) {
    LG_DEBUG_ERR("Cannot open the log file: %s", file_path);
    return false;
  }

  inst->queue = lg_queue_create(LOGGER_RING_SIZE);
  if (!inst->queue) {
    fclose(logFile);
    LG_DEBUG_ERR("LogQueue cannot be created!");
    return false;
  }

  atomic_store_explicit(&inst->isAlive, true, memory_order_release);

  // create writer thread
  if (pthread_create(&inst->writer_th, NULL, lg_consumer, (void*)inst) != 0) {
    fclose(logFile);
    free(inst->queue);
    atomic_store_explicit(&inst->isAlive, false, memory_order_release);
    LG_DEBUG_ERR("Cannot create writer process thread!");
    return false;
  }

  // We hardcoded file sinks for now (4.0-pre2)
  inst->files[0] = logFile;
  if (config.printStdout) {
    inst->files[1] = stdout;
    inst->files_count = 2;
  } else
    inst->files_count = 1;

  unsigned int needed = 0;
  for (size_t i = 0; i < inst->files_count; i++)
    if (inst->files[i])
      needed |= (1u << lg_get_out_type(inst->files[i]));
  inst->out_needed = needed;

  if (active_instance == NULL) active_instance = inst;
  return true;
}

// main log function with variadics - used at macros
// gets resolved message and calls lg_producer
int lg_vproducer(Logger* inst, const LgLogLevel level, const char* fmt, ...)
{
  if (!fmt) return false;

  // variadic resolving
  va_list args;
  va_start(args, fmt);
  char msg[LOGGER_MAX_MSG_SIZE];  // we allocate enough memory
  int mn = vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  if (mn < 0) {
    LG_DEBUG_ERR("Cannot resolve print format");
    return false;
  }

  return lg_producer(inst, level, msg, mn);
}

// main log function, invokes the writer - used at FFIs
// accepts format-resolved message to print
int lg_producer(Logger* inst, const LgLogLevel level, const char* msg, size_t msglen)
{
  if (!msg || msglen >= LOGGER_MAX_MSG_SIZE) return false;

  if (!inst) {
    if (active_instance)
      inst = active_instance;
    else {
      LG_DEBUG_ERR("There is no active instance!");
      return false;
    }
  }

  // doesnt log when its not alive
  if (!lg_is_alive(inst)) {
    LG_DEBUG_ERR("Cannot log because the instance is dead!");
    return false;
  }

  LogQueue *q = inst->queue;
  size_t pos = atomic_load_explicit(&q->head, memory_order_relaxed);
  size_t seq;
  LogSlot* s;
  int spins = 0;
  for (;;) {
    s = lg_slot_get(q, pos);
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
        lg_adaptive_wait(&spins);
        break;
      case LG_PRIORITY_BASED:
        if (level == LG_ERROR) {
          lg_adaptive_wait(&spins);
          break;
        } else return false;
      default:
        return false;
      }
    } else { // pos stale, re-read
      pos = atomic_load_explicit(&q->head, memory_order_relaxed);
    }
  }

  // Copy the data to payload
  LogPayload *pyld = lg_slot_payload(s);
  memcpy(pyld->msg, msg, msglen + 1);
  pyld->length = msglen;
  pyld->level = level;

  // Slot ready signal to consumer
  atomic_store_explicit(&s->seq, pos + 1, memory_order_release);
  return true;
}

int lg_set_active_instance(Logger* inst)
{
  if (!inst) return false;
  active_instance = inst;
  return 1;
}

Logger* lg_get_active_instance()
{
  return active_instance;
}

int lg_destroy(Logger* inst)
{
  if (!inst) {
    if (active_instance)
      inst = active_instance;
    else
      return false;
  }

  // if it is not alive, do not try to destruct
  if (!inst->isAlive) {
    LG_DEBUG_ERR("Logger is already dead!");
    return false;
  }
  atomic_store_explicit(&inst->isAlive, false, memory_order_release);
  pthread_join(inst->writer_th, NULL);

  // clear config after thread exit
  inst->isPrintStdout = false;
  inst->isLocalTime = false;

  // close the files if they're not closed
  for (size_t i = 0; i < inst->files_count; i++) {
    FILE* f = inst->files[i];
    if (!f) continue;
    if (f != stdout && f != stderr) {
      if (fclose(f) != 0) {
        LG_DEBUG_ERR("Log file cannot be closed!");
        return false;
      }
    }
    inst->files[i] = NULL;
  }
  return true;
}

int lg_is_alive(const Logger* inst)
{
  const Logger* ins = inst ? inst : lg_get_active_instance();
  if (!ins) return 0;
  return atomic_load_explicit(&ins->isAlive, memory_order_relaxed);
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
  free(inst->queue);
  free(inst);
}

int lg_flogi(Logger* inst, const LgLogLevel level, const char* msg)
{
  if (!msg) return false;
  if (!inst) return lg_producer(active_instance, level, msg, strlen(msg));
  return lg_producer(inst, level, msg, strlen(msg));
}

// Explicit instances
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
    return "INFO";
  }
}

int lg_get_time_str(char* buf, int isLocalTime)
{
  if (!buf) return false;

#ifdef _WIN32
  SYSTEMTIME st;
  if (isLocalTime)
    GetLocalTime(&st);
  else
    GetSystemTime(&st);
  int n = snprintf(buf, LOGGER_TIME_STR_SIZE, "%04d.%02d.%02d-%02d.%02d.%02d.%03d",
                   st.wYear,         // 2026
                   st.wMonth,        // 1
                   st.wDay,          // 21
                   st.wHour,         // 22
                   st.wMinute,       // 16
                   st.wSecond,       // 40
                   st.wMilliseconds  // 450
                  );
  if (n <= 0 || (size_t)n >= LOGGER_TIME_STR_SIZE) return false;
#else   // unix
  // get nanosec with struct
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  // get other fields except ns
  struct tm tm_val;
  if (isLocalTime)
    localtime_r(&ts.tv_sec, &tm_val);
  else
    gmtime_r(&ts.tv_sec, &tm_val);

  long ms = ts.tv_nsec / 1000000;  // nanosecond -> millisecond

  int n = snprintf(buf, LOGGER_TIME_STR_SIZE, "%04d.%02d.%02d-%02d.%02d.%02d.%03ld",
                  tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday,
                  tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec, ms);
  if (n <= 0 || (size_t)n >= LOGGER_TIME_STR_SIZE) return false;
#endif  // _WIN32
  return true;
}

static int check_dir(const char* path)
{
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(path);
  if (attr == INVALID_FILE_ATTRIBUTES) return 0;  // does not exists
  if (attr & FILE_ATTRIBUTE_DIRECTORY)
    return 1;  // exists AND directory (valid)
  return -1;   // exists but is not directory
#else
  struct stat st;
  if (stat(path, &st) != 0) return 0;  // not exists
  if (S_ISDIR(st.st_mode)) return 1;   // exists AND directory (valid)
  return -1;                           // exists but not a directory
#endif
}

static bool mkdir_p(char* path)
{
  if (!path || !*path) return false;

  // go char by char
  for (char* p = path + 1; *p; p++) {
    if (*p != PATH_SEP) continue;
    *p = '\0';
    LG_DEBUG("Trying to create: %s", path);
    int status = MKDIR(path);
    if (status != 0 && errno != EEXIST) {
      LG_DEBUG_ERR("Failed to create path: %s", path);
      return false;
    }
    *p = PATH_SEP;
  }
  return true;
}

static int count_logs_and_get_oldest(const char* path, char* oldest_path, size_t opsz) {
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

static bool normalize_path(const char* path, char* out, size_t size)
{
  if (!path || !*path) return false;
  char* dst = out;
  char* end = out + size - 1;

  for (const char* src = path; *src && dst < end; src++) {
    // double slash skip
    if (*src == PATH_SEP && dst != out && *(dst-1) == PATH_SEP) continue;
    *dst++ = *src;
  }
  *dst = '\0';

  // add trailing slash
  if (dst + 1 >= end) return false; // no space for slash and zero
  size_t len = dst - out;
  out[len] = PATH_SEP;
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

  if (n < 0) {
    s->len = 0;
  } else if ((size_t)n >= cap) {
    if (cap >= 2) {
      s->data[cap - 2] = '\n';  // guarentees \n at the end
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
  if (!str) return;
  lg_str_format_into(s, "%s", str);
}

static bool lg_process_payload(Logger* inst, LogPayload* payload) {
  if (payload->length == 0) return false;

  // Custom format logic
  LgMsgPack pack = {};
  log_formatter_t formatter_fn = inst->customLogFunc ? inst->customLogFunc : format_msg;
  if (!formatter_fn(inst->isLocalTime, payload->level, payload->msg, inst->out_needed, pack)) {
    LG_DEBUG_ERR("Cannot format the message, dropping it!");
    return false;
  }

  // Fwrite to all not-null files
  for (size_t i = 0; i < inst->files_count; i++) {
    FILE* f = inst->files[i];
    if (!f) continue;
    LgString* str = &pack[lg_get_out_type(f)];
    fwrite(str->data, 1, str->len, f);
  }
  payload->length = 0;
  return true;
}

static LgOutType lg_get_out_type(FILE* f) {
  return isatty(fileno(f)) ? LG_OUT_TTY : LG_OUT_FILE;
}

// message formatter helper - used at consumer
static int format_msg(
  int isLocalTime,
  LgLogLevel level,
  const char* msg,
  uint32_t needed,
  LgMsgPack pack
) {
  // getting time string
  char time_str[LOGGER_TIME_STR_SIZE];
  if (!lg_get_time_str(time_str, isLocalTime)) return false;

  LgString* stdout_str = &pack[LG_OUT_TTY];
  LgString* file_str   = &pack[LG_OUT_FILE];

  // prepairing stdout msg
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_TTY)) {
#ifdef LOGGER_DONT_COLORIZE
    lg_str_format_into(
      stdout_str,
      "%s [%s] %s\n",
      time_str, lg_lvl_to_str(level), msg
      );
#else
    const char* clr;
    switch (level) {
    case LG_ERROR:
      clr = LOGGER_CLR_RED;
      break;
    case LG_INFO:
      clr = LOGGER_CLR_GREEN;
      break;
    case LG_WARNING:
      clr = LOGGER_CLR_YELLOW;
      break;
    default:
      clr = LOGGER_CLR_RST;
      break;
    }
    lg_str_format_into(
      stdout_str,
      LOGGER_CLR_AQUA "%s %s[%s]" LOGGER_CLR_RST " %s\n",
      time_str, clr, lg_lvl_to_str(level), msg
      );
#endif
  }

  // prepairing file msg
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_FILE)) {
    // no escape chars in file
    lg_str_format_into(
      file_str,
      "%s [%s] %s\n", time_str,
      lg_lvl_to_str(level), msg);
  }
  return true;
}

static size_t lg_slot_stride() {
  size_t total = sizeof(LogSlot) + sizeof(LogPayload);
  return (total + LOGGER_CACHE_LINE - 1) & ~(size_t)(LOGGER_CACHE_LINE - 1);
}

static LogSlot* lg_slot_get(LogQueue* q, size_t idx) {
  size_t stride = lg_slot_stride();
  return (LogSlot*)(q->slots + (idx & q->mask) * stride);
}

static LogPayload* lg_slot_payload(LogSlot* s) {
  return (LogPayload*)((uint8_t*)s + sizeof(LogSlot));
}

static LogQueue* lg_queue_create(size_t capacity) {
  if ((capacity & (capacity - 1)) != 0) return NULL; // power of 2 check

  size_t stride = lg_slot_stride();
  size_t total  = sizeof(LogQueue) + capacity * stride;

  // void* aligned_alloc(size_t alignment, size_t size); (since C11, stdlib.h)
  LogQueue* q = (LogQueue*)aligned_alloc(LOGGER_CACHE_LINE, total);
  if (!q) return NULL;

  // Set the fields
  q->capacity  = capacity;
  q->mask      = capacity - 1;
  q->tail = 0;
  atomic_store_explicit(&q->head, 0, memory_order_relaxed);

  // Initialize every slot's seq by index
  for (size_t i = 0; i < capacity; i++) {
    LogSlot* s = (LogSlot*)(q->slots + i * stride);
    atomic_store_explicit(&s->seq, i, memory_order_relaxed);
  }

  atomic_thread_fence(memory_order_seq_cst); // init barrier
  return q;
}

/*
 * CONSUMER - being called by single thread
 * out: buffer to write payload into
 * (in logger, this can be stdout/logFile stream)
 */
static bool lg_queue_pop(LogQueue* q, LogPayload** out) {
  size_t pos = q->tail;
  LogSlot*  s   = lg_slot_get(q, pos);

  size_t seq = atomic_load_explicit(&s->seq, memory_order_acquire);

  // seq == pos + 1 -> producer finished
  if (seq != pos + 1) {
    return false; // not ready yet
  }

  *out = lg_slot_payload(s);

  /*
   * Free the slot: seq = pos + capacity
   * Therefore, slot can be reused
   */
  atomic_store_explicit(&s->seq, pos + q->capacity, memory_order_release);

  q->tail = pos + 1;
  return true;
}

static void lg_adaptive_wait(int* spins) {
  if (!spins) return;
  if (*spins < LOGGER_WAIT_NO_PAUSE_MAGIC) {
    *spins += 1;
    // no pause
  } else if (*spins < LOGGER_WAIT_PAUSE_MAGIC) {
    *spins += 1;
    LOGGER_PAUSE_INS();
  } else {
    struct timespec ts = {0, 1000};
    nanosleep(&ts, NULL); // real sleep
  }
}

#endif  // LOGGER_IMPLEMENTATION

#endif  // LOGGER_H
