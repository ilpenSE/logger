// THIS IS PRE-RELEASE OF LOGGER!
// NOT ALL ENVIRONMENTS CAN SUPPORT!
// TESTED ON X86_64 GNU/LINUX AND WINDOWS

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
  TESTED ON AMD64 (X86_64) GNU/LINUX WITH GCC/CLANG, MINGW AND MSVC-WINE

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

#define LOGGER_INTERNAL static

// some global includes here
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

// These variables can be fine-tuned due to your traffic
// High traffic = high these constants
// Low traffic = lower values
#define LOGGER_WAIT_NO_PAUSE_MAGIC 100
#define LOGGER_WAIT_PAUSE_MAGIC 1000

/*
  Defines time_str from lg_get_time_str size (WITH ZERO AT THE END!)
  All of time_str related functions uses this, if you
  ever change lg_get_time_str function, update this
*/
#define LOGGER_TIME_STR_SIZE 24

// logger max message size (you can change it)
#define LOGGER_MAX_MSG_SIZE 256
// ring buffer's size
#define LOGGER_RING_SIZE 1024

// Maximum amount of files that can be in the sink (not really useful now)
#define LOGGER_MAX_SINKS 8

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
  // Dont forget to update LOGGER_MAX_OUT_TYPES
} LgOutType;

#define LOGGER_MAX_OUT_TYPES 3

typedef struct Logger Logger;

typedef struct LgString {
  char data[LOGGER_MAX_MSG_SIZE];
  size_t len;
} LgString;

/*
  Sink for files and their types.
  FILE* file: if this is NULL, the whole sink is invalid,
              points to the file to write into
  LgOutType type: this determines what type of string can
                  be write into the file (used at formatter)
*/
typedef struct {
  FILE* file;
  LgOutType type;
} LgSink;

// Array of LgSink
typedef struct {
  LgSink items[LOGGER_MAX_SINKS];
  size_t count;
} LgSinks;

typedef LgString LgMsgPack[LOGGER_MAX_OUT_TYPES];

/*
  Formatter callback type
*/
typedef int (*log_formatter_t)(
  const char* time_str,
  LgLogLevel level,
  const char* msg,
  uint32_t needed, // needed file types
  LgMsgPack out
);

/*
 * Config struct, this struct can be used in lg_init.
 * localTime: uses your localtime using kernel if it's true
 * generateDefaultFile: if true, generates logs directory and
   file in it. It automatically inserts file to the sinks
 * maxFiles: maximum amount of .log files that is in the
   logs folder. Non-positive values'll be treated as unlimited
   If this is exceeded, it'll remove the oldest log file
 * sinks: determines where to write log messages into
   define this in your space BUT give count correct.
   Otherwise this can crash, and if you provide generateDefaultFile
   as true, you dont have to think about extra room for
   default log file, init handles it.
 * logFormatter: custom formatter callback for log messages.
   If null, fallbacks to the default formatter (LOGGER_INTERNAL lgi_def_format_msg).
   The "needed" bitmask tells you which LgOutType slots must be filled,
   only generate output for types present in the bitmask.
   Use LOGGER_CONTAINS_FLAG macro or needed & (1u << type) in if branch.
   Every sink whose type matches a filled slot will write that message.
   Unfilled slots are ignored, no unnecessary formatting is done.
   See LgOutType for available types and lg_str_format_into to fill LgString.
 * logPolicy: the log policy which determines what the logger
   does when the MPSC lock-free ring buffer is full.
*/
typedef struct {
  int localTime;
  int maxFiles;
  int generateDefaultFile;
  LgSinks sinks;
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
  Initialize the logger with default config struct
*/
LOGGERDEF int lg_init_defaults(Logger* instance, const char* logs_dir);

/*
  lg_init but Flattened the config struct (mostly used at FFIs)
*/
LOGGERDEF int lg_init_flat(Logger* inst, const char* logs_dir,
                          int local_time, int max_log_files, int generateDefaultFile,
                          LgSinks sinks, LgLogPolicy log_policy,
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
LOGGERDEF int lg_log_(Logger* inst, const LgLogLevel level,
                     const char* msg, size_t msglen);

/*
  This is the format resolver for lg_log_.
  C macros use this. It's sensitive to "%" char.
  It may crash your app if you're using this on FFI or wrongly on C
*/
LOGGERDEF int lg_vlog_(Logger* inst, const LgLogLevel level,
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
  Returns default LoggerConfig struct
  You can change things its return value
*/
LOGGERDEF LoggerConfig lg_get_defaults();

/*
  Pushes new sink into the config
*/
LOGGERDEF int lg_append_sink(LoggerConfig* config, FILE* f, LgOutType type);

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

// Log with an explicit logger instance
#define lg_logi(instance, level, fmt, ...) \
  lg_vlog_(instance, level, fmt, ##__VA_ARGS__)

#define lg_infoi(instance, fmt, ...) \
  lg_logi(instance, LG_INFO, fmt, ##__VA_ARGS__)
#define lg_errori(instance, fmt, ...) \
  lg_logi(instance, LG_ERROR, fmt, ##__VA_ARGS__)
#define lg_warni(instance, fmt, ...) \
  lg_logi(instance, LG_WARNING, fmt, ##__VA_ARGS__)

// log functions to be going to used is macros now
// main function is lg_vlog_
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdalign.h>

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

#define LOGGER_ALIGN alignas(LOGGER_CACHE_LINE)

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
  size_t stride; // width of the full slot with padding, seq payload

  LOGGER_ALIGN ATOMIC(size_t) head; // Producer
  LOGGER_ALIGN size_t tail; // Consumer
  uint8_t slots[];
} LogQueue;

// Static function forward-declerations
// checks if dir is a valid directory (exists and directory)
LOGGER_INTERNAL int lgi_check_dir(const char* path);

// normalizes path to acceptable ones (>1 slash skips, removing trailing slash)
LOGGER_INTERNAL bool lgi_normalize_path(const char* path, char* out, size_t size);

// count .log files and print oldest one to oldest_path in that folder
// negative return value is error status
LOGGER_INTERNAL int lgi_count_logs_and_get_oldest(const char* path, char* oldest_path, size_t oldest_path_size);

// recursively creates folders (behaves like mkdir -p)
LOGGER_INTERNAL bool lgi_mkdir_p(char* path);

// default/fallback log formatter
LOGGER_INTERNAL int lgi_def_format_msg(const char* time_str, LgLogLevel level,
                                      const char* msg, uint32_t needed, LgMsgPack pack);

// Create MPSC queue with given capacity (capacity must be power of 2)
LOGGER_INTERNAL LogQueue* lgi_queue_create(size_t capacity);

// Pop something from the MPSC queue (Consumer)
LOGGER_INTERNAL bool lgi_queue_pop(LogQueue* q, LogPayload** out, size_t* pos);

// Release the lock, queue_pop locks the slot so if you are done with
// processing payload, you call this and it's guaranteed to process the payload
// FIX for dropping logs even on the LG_BLOCK policy
LOGGER_INTERNAL void lgi_queue_release(LogQueue* q, size_t pos);

// POP - PROCESS - RELEASE
LOGGER_INTERNAL inline bool lgi_queue_ppr(Logger* inst, LogPayload** entry, size_t* pos);

// Processes the payload (writes message to proper fds)
LOGGER_INTERNAL bool lgi_process_payload(Logger* inst, LogPayload* payload);

// Adaptive waiters, not only spins, it spins until reaches
// LOGGER_NO_PAUSE_MAGIC and LOGGER_PAUSE_MAGIC values
// Pause is pause instruction no pause -> just spins
// No pause means spin and pause
// If pause magic exceeded, it sleeps (Sleep or nanosleep)
LOGGER_INTERNAL void lgi_adaptive_wait(int* spins);

// Return the slot by idx from the given queue
LOGGER_INTERNAL inline LogSlot* lgi_slot_get(LogQueue* q, size_t idx)
{
  return (LogSlot*)(q->slots + (idx & q->mask) * q->stride);
}

// Extract payload from that slot
LOGGER_INTERNAL inline LogPayload* lgi_slot_payload(LogSlot* s)
{
  return (LogPayload*)((uint8_t*)s + sizeof(LogSlot));
}

// Manual writes for lg_get_time_str
LOGGER_INTERNAL inline void lgi_write2(char* p, int v)
{
  p[0] = (char)('0' + v / 10);
  p[1] = (char)('0' + v % 10);
}

LOGGER_INTERNAL inline void lgi_write4(char* p, int v)
{
  lgi_write2(p, v / 100);
  lgi_write2(p + 2, v % 100);
}

LOGGER_INTERNAL inline void lgi_write3(char* p, int v)
{
  p[0] = (char)('0' + v / 100);
  p[1] = (char)('0' + (v / 10) % 10);
  p[2] = (char)('0' + v % 10);
}

// Some other macro-utilities
#define LG_UNUSED(x) (void)(x)
#define LG_STRINGIFY(x) #x
#define LG_TOSTRING(x) LG_STRINGIFY(x)
// Fuck you MSVC with C++
#ifdef __cplusplus
  #define LG_STRUCT(T, ...) (T{__VA_ARGS__})
#else
  #define LG_STRUCT(T, ...) ((T){__VA_ARGS__})
#endif

// define this macro to enable error messages in stderr
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
#endif  // LOGGER_DEBUG

#ifdef _WIN32
#include <direct.h>  // _mkdir
#include <windows.h>
#include <malloc.h>
#include <io.h>

#define LOGGER_MKDIR(path) _mkdir(path)
#define LOGGER_PATH_SEP '\\'
#ifndef PATH_MAX
#define PATH_MAX (MAX_PATH * 2) // for wchar_t
#endif

// Sleep() has terrible resolution (milliseconds)
// But who uses winbloat for production-ready logger?
#define LOGGER_SLEEP(ns) do { Sleep(1); } while (0)

// threads transpilation for windows to posix
typedef HANDLE pthread_t;

static DWORD WINAPI lgi_thread_trampoline(LPVOID arg)
{
  // unpacking fn and arg
  typedef void* (*fn_t)(void*);
  fn_t fn = (fn_t)((void**)arg)[0];  // thread function
  void* rarg = ((void**)arg)[1];     // real_arg
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
#include <sys/stat.h>  // for dirs
#include <sys/time.h>  // for time
#include <dirent.h>
#include <unistd.h>

// sleep for ns nanoseconds
#define LOGGER_SLEEP(ns)                                            \
  do {                                                              \
    struct timespec ts = {(ns) / 1000000000L, (ns) % 1000000000L};  \
    nanosleep(&ts, NULL);                                           \
  } while (0)
#define LOGGER_MKDIR(path) mkdir(path, 0755)
#define LOGGER_PATH_SEP '/'
#endif

// Aligned alloc
#if defined(_MSC_VER) || defined(__MINGW32__)
  #define ALIGNED_ALLOC(align, size) _aligned_malloc(size, align)
  #define ALIGNED_FREE(ptr)          _aligned_free(ptr)
#else
  #define ALIGNED_ALLOC(align, size) aligned_alloc(align, size)
  #define ALIGNED_FREE(ptr)          free(ptr)
#endif

// Pause instruction
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
  LgSink  sinks[LOGGER_MAX_SINKS + 1]; // extra 1 for default file for any condition
  size_t  sinks_count;
  log_formatter_t customLogFunc;
  uint32_t out_needed; // needed file flags for formatter
  pthread_t writer_th;
  LogQueue* queue;
};

// consumer func, writes entries on the ring to stdout or file
LOGGER_INTERNAL void* lgi_consumer(void* arg) {
  Logger* inst = (Logger*)arg;
  LogPayload* entry = NULL;
  int spins = 0;

  size_t pos;
  while (atomic_load_explicit(&inst->isAlive, memory_order_acquire)) {
    if (lgi_queue_ppr(inst, &entry, &pos)) spins = 0;
    else lgi_adaptive_wait(&spins);
  }

  // isAlive = false, drain remaining entries
  while (lgi_queue_ppr(inst, &entry, &pos))
    ;;

  LG_DEBUG("Writer thread is exiting");
  return NULL;
}

// active logger instance if NULL, init tries to set it
LOGGER_INTERNAL Logger* active_instance = NULL;

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

// main init func
int lg_init(Logger* inst, const char* logs_dir, LoggerConfig config)
{
  if (!inst || !logs_dir) return false;
  if (config.sinks.count > LOGGER_MAX_SINKS) {
    LG_DEBUG_ERR("Max amount of file sinks can be " LG_TOSTRING(LOGGER_MAX_SINKS));
    return false;
  }

  size_t is_gen_def_file = config.generateDefaultFile;
  inst->isLocalTime = config.localTime != 0;
  inst->maxLogFiles = config.maxFiles;
  inst->generateDefaultFile = is_gen_def_file;
  inst->logPolicy = config.logPolicy;
  inst->customLogFunc = config.logFormatter;

  FILE* logFile;
  if (is_gen_def_file) {
    char dir[PATH_MAX];
    if (!lgi_normalize_path(logs_dir, dir, sizeof(dir))) {
      LG_DEBUG_ERR("Provided path is corrupted: %s", logs_dir);
      return false;
    }

    int dir_status = lgi_check_dir(dir);  // -1 = NOT valid directory, 0 = NOT exists
    // handle not a valid directory
    if (dir_status == -1) {
      LG_DEBUG_ERR("Provided path is not a valid directory to create: %s", dir);
      return false;
    }

    // handle just non-exist directory (best effort)
    if (dir_status == 0) {
      if (!lgi_mkdir_p(dir)) {
        LG_DEBUG_ERR("Cannot create provided path: %s", dir);
        return false;
      }
    } else {
      if (config.maxFiles > 0) {
        // get .log files in that logs folder
        char oldestFile[PATH_MAX];
        int files = lgi_count_logs_and_get_oldest(dir, oldestFile, sizeof(oldestFile));
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
    logFile = fopen(file_path, "wb");
    if (!logFile) {
      LG_DEBUG_ERR("Cannot open the log file: %s", file_path);
      return false;
    }
  } else logFile = NULL;

  inst->queue = lgi_queue_create(LOGGER_RING_SIZE);
  if (!inst->queue) {
    if (logFile) fclose(logFile);
    LG_DEBUG_ERR("LogQueue cannot be created!");
    return false;
  }

  // Copy user sinks to instance and push default file
  size_t cnt = config.sinks.count;
  memcpy(inst->sinks, config.sinks.items, cnt * sizeof(LgSink));
  inst->sinks_count = is_gen_def_file + cnt;
  if (is_gen_def_file) {
    inst->sinks[cnt] = LG_STRUCT(LgSink, logFile, LG_OUT_FILE);
  }

  uint32_t needed = 0;
  for (size_t i = 0; i < inst->sinks_count; i++) {
    needed |= (1u << inst->sinks[i].type);
  }
  inst->out_needed = needed;

  atomic_store_explicit(&inst->isAlive, true, memory_order_release);
  // create writer thread
  if (pthread_create(&inst->writer_th, NULL, lgi_consumer, (void*)inst) != 0) {
    if (logFile) fclose(logFile);
    free(inst->queue);
    atomic_store_explicit(&inst->isAlive, false, memory_order_release);
    LG_DEBUG_ERR("Cannot create writer process thread!");
    return false;
  }

  if (active_instance == NULL) active_instance = inst;
  return true;
}

// main log function with variadics - used at macros
// gets resolved message and calls lg_log_
int lg_vlog_(Logger* inst, const LgLogLevel level, const char* fmt, ...)
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

  return lg_log_(inst, level, msg, mn);
}

// main log function, invokes the writer - used at FFIs
// accepts format-resolved message to print
int lg_log_(Logger* inst, const LgLogLevel level, const char* msg, size_t msglen)
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

  // Copy the data to payload
  LogPayload *pyld = lgi_slot_payload(s);
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
  ALIGNED_FREE(inst->queue);
  free(inst);
}

// Explicit instances
int lg_flogi(Logger* inst, const LgLogLevel level, const char* msg)
{
  if (!msg) return false;
  if (!inst) return lg_log_(active_instance, level, msg, strlen(msg));
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

// %04d.%02d.%02d-%02d.%02d.%02d.%03ld
int lg_get_time_str(char* buf, int isLocalTime)
{
  if (!buf) return false;
#ifdef _WIN32
  SYSTEMTIME st;
  if (isLocalTime) GetLocalTime(&st);
  else GetSystemTime(&st);
  int year = st.wYear;
  int month = st.wMonth;
  int day = st.wDay;
  int hours = st.wHour;
  int minutes = st.wMinute;
  int seconds = st.wSecond;
  long millis = st.wMilliseconds;
#else   // unix
  // cached tm
  static time_t cached_sec = 0;
  static struct tm cached_tm;

#ifdef LOGGER_GET_REAL_TIME
  // get exact time (performs syscall)
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
#else
  // get coarse time - much more efficient
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME_COARSE, &ts);
#endif

  // Update cache if cache is stale
  if (ts.tv_sec != cached_sec) {
    if (isLocalTime)
      localtime_r(&ts.tv_sec, &cached_tm);
    else
      gmtime_r(&ts.tv_sec, &cached_tm);
    cached_sec = ts.tv_sec;
  }
  int year = cached_tm.tm_year + 1900;
  int month = cached_tm.tm_mon + 1;
  int day = cached_tm.tm_mday;
  int hours = cached_tm.tm_hour;
  int minutes = cached_tm.tm_min;
  int seconds = cached_tm.tm_sec;
  long millis = ts.tv_nsec / 1000000;
#endif  // _WIN32

  // Manual writing
  lgi_write4(buf, year);
  buf[4]  = '.';
  lgi_write2(buf+5, month);
  buf[7]  = '.';
  lgi_write2(buf+8, day);
  buf[10] = '-';
  lgi_write2(buf+11, hours);
  buf[13] = '.';
  lgi_write2(buf+14, minutes);
  buf[16] = '.';
  lgi_write2(buf+17, seconds);
  buf[19] = '.';
  lgi_write3(buf+20, millis);
  buf[23] = '\0';
  return true;
}

LOGGER_INTERNAL int lgi_check_dir(const char* path)
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
  if (dst + 1 >= end) return false; // no space for slash and zero
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

void lg_str_write_into(LgString* s, const char* str) {
  if (!str || !s) return;
  size_t len = strlen(str);
  if (len == 0) return;
  if (len >= sizeof(s->data)) len = sizeof(s->data) - 2;
  memcpy(s->data, str, len);
  s->data[len - 1] = '\n';
  s->data[len] = '\0';
  s->len = len;
}

LOGGER_INTERNAL bool lgi_process_payload(Logger* inst, LogPayload* payload) {
  // getting time string
  char time_str[LOGGER_TIME_STR_SIZE];
  if (!lg_get_time_str(time_str, inst->isLocalTime)) return false;

  // Custom format logic
  LgMsgPack pack = {};
  log_formatter_t formatter_fn = inst->customLogFunc ? inst->customLogFunc : lgi_def_format_msg;
  if (!formatter_fn(time_str, payload->level, payload->msg, inst->out_needed, pack)) {
    LG_DEBUG_ERR("Cannot format the message, dropping it!");
    return false;
  }

  // Fwrite to all not-null files
  for (size_t i = 0; i < inst->sinks_count; i++) {
    LgSink* s = &inst->sinks[i];
    if (!s->file) continue;
    LgString* str = &pack[s->type];
    fwrite(str->data, 1, str->len, s->file);
  }
  return true;
}

// message formatter helper - used at consumer
LOGGER_INTERNAL int lgi_def_format_msg(
  const char* time_str,
  LgLogLevel level,
  const char* msg,
  uint32_t needed,
  LgMsgPack pack
) {
  LgString* tty_str  = &pack[LG_OUT_TTY];
  LgString* file_str = &pack[LG_OUT_FILE];
  LgString* net_str = &pack[LG_OUT_NET];
  const char* level_str = lg_lvl_to_str(level);

  // preparing stdout msg
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_TTY)) {
#ifdef LOGGER_DONT_COLORIZE
    lg_str_format_into(
      tty_str,
      "%s [%s] %s\n"
      time_str, clr, level_str, msg
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
      tty_str,
      LOGGER_CLR_AQUA "%s %s[%s]" LOGGER_CLR_RST " %s\n",
      time_str, clr, level_str, msg
    );
#endif
  }

  // preparing file msg
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_FILE)) {
    // no escape chars in file
    lg_str_format_into(
      file_str,
      "%s [%s] %s\n",
      time_str, level_str, msg
    );
  }

  // preparing network msg (JSON)
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_NET)) {
    lg_str_format_into(
      net_str,
      "{\"timestamp\":\"%s\",\"level\":\"%s\",\"message\":\"%s\"}\n",
      time_str, level_str, msg
    );
  }
  return true;
}

LOGGER_INTERNAL LogQueue* lgi_queue_create(size_t capacity) {
  if ((capacity & (capacity - 1)) != 0) return NULL; // power of 2 check

  size_t stride = (sizeof(LogSlot) + sizeof(LogPayload) + LOGGER_CACHE_LINE - 1)
                  & ~(size_t)(LOGGER_CACHE_LINE - 1);
  size_t total  = sizeof(LogQueue) + capacity * stride;

  // TODO: Make this stack-allocated and fixed-size
  LogQueue* q = (LogQueue*)ALIGNED_ALLOC(LOGGER_CACHE_LINE, total);
  if (!q) return NULL;

  // Set the fields
  q->capacity = capacity;
  q->mask     = capacity - 1;
  q->stride   = stride;
  q->tail     = 0;
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
LOGGER_INTERNAL bool lgi_queue_pop(LogQueue* q, LogPayload** out, size_t* out_pos) {
  size_t pos  = q->tail;
  LogSlot* s  = lgi_slot_get(q, pos);
  size_t seq = atomic_load_explicit(&s->seq, memory_order_acquire);

  if (seq != pos + 1) {
    return false; // not ready yet
  }

  *out = lgi_slot_payload(s);
  *out_pos = pos;

  q->tail = pos + 1;
  return true;
}

LOGGER_INTERNAL void lgi_queue_release(LogQueue* q, size_t pos) {
  LogSlot* s = lgi_slot_get(q, pos);
  atomic_store_explicit(&s->seq, pos + q->capacity, memory_order_release);
}

LOGGER_INTERNAL inline bool lgi_queue_ppr(Logger* inst, LogPayload** entry, size_t* pos) {
  if (!lgi_queue_pop(inst->queue, entry, pos)) return false;
  lgi_process_payload(inst, *entry);
  lgi_queue_release(inst->queue, *pos);
  return true;
}

LOGGER_INTERNAL void lgi_adaptive_wait(int* spins) {
  if (!spins) return;
  if (*spins < LOGGER_WAIT_NO_PAUSE_MAGIC) {
    *spins += 1;
    // no pause
  } else if (*spins < LOGGER_WAIT_PAUSE_MAGIC) {
    *spins += 1;
    LOGGER_PAUSE_INS();
  } else {
    LOGGER_SLEEP(1);
  }
}

FILE* lg_get_stdout() { return stdout; }
FILE* lg_get_stderr() { return stderr; }

FILE* lg_fopen(const char* path) {
  return fopen(path, "wb");
}

#endif  // LOGGER_IMPLEMENTATION

#endif  // LOGGER_H
