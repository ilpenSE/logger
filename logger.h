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
  REQUIRES MINIMUM C99 VERSION
  TESTED ON AMD64 GNU/LINUX WITH GCC/CLANG AND WINDOWS WITH MSVC

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

typedef enum {
  LG_INFO = 1 << 0,
  LG_ERROR = 1 << 1,
  LG_WARNING = 1 << 2,
  LG_CUSTOM = 1 << 3,
  // Add more levels here
} lg_log_level;

/*
  Log policies when ring buffer is full
  LG_DROP (default): Drops the log when buffer is full, you lost some log
  messages LG_SMASH_OLDEST: Remove oldest one and keep going
*/
typedef enum {
  LG_DROP = 1 << 0,
  LG_SMASH_OLDEST = 1 << 1,
} lg_log_policy;

typedef struct Logger Logger;
typedef struct lg_string lg_string;
typedef struct lg_msg_pack lg_msg_pack;

/*
  The string struct, used at message pack
  We use lg_string to not collide with str.h
*/
struct lg_string {
  char* data;
  size_t cap;
  size_t len;
};

// holds 2 dynamic strings
struct lg_msg_pack {
  lg_string stdout_str;
  lg_string file_str;
};

typedef int (*log_formatter_t)(const int isLocalTime, const lg_log_level level,
                              const char* msg, lg_msg_pack* pack);

/*
 * Config struct, this struct can be used in lg_init.
 * localTime: uses your localtime using kernel if it's true
 * printStdout: the logger tries to print message to stdout
   if it's true, otherwise it doesnt
 * logFormatter: this function describes the logging format.
   By default, it is like:
   time level msg
   its parameters: level, formatted message, pack
   returns: nothing (void)
   The pack is that you put stdout and file string messages.
   You can distinguish messages that is gonna be printed to
   stdout or file. Go to pack and string defitions.
   If you wonder how you implement that, go format_msg
   static function or look at the usage folder
   SINCE V3.6: time_str is removed from parameters, that's
   because you can customize time string (already format_msg
   calls get_time_str) and producer doesn't call it anymore
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
  lg_log_policy logPolicy;
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
                          lg_log_policy log_policy,
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
LOGGERDEF int lg_producer(Logger* inst, const lg_log_level level,
                          const char* msg, size_t msglen);

/*
  This is the format resolver for lg_producer.
  C macros use this. It's sensitive to "%" char.
  It may crash your app if you're using this on FFI or wrongly on C
*/
LOGGERDEF int lg_vproducer(Logger* inst, const lg_log_level level,
                          const char* fmt, ...) PRINTF_LIKE(3, 4);

/*
  Wrapper log functions for FFI, if you dont use C/C++ or
  using your language's FFI, you must use these:
*/
LOGGERDEF int lg_flogi(Logger* inst, const lg_log_level level, const char* msg);
LOGGERDEF int lg_flog(const lg_log_level level, const char* msg);

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
  Manipulates lg_log_level enum into string
*/
LOGGERDEF const char* lg_lvl_to_str(const lg_log_level level);

/*
  Simply acts like snprintf and applies it to the lg_string
  Processes "%" style printf format (variadics), it may cause UBs
*/
LOGGERDEF void lg_str_format_into(lg_string* s, const char* fmt, ...)
  PRINTF_LIKE(2, 3);

/*
  lg_str_format_into but already formatted strings
  Use it at FFI because you can't use variadics there
*/
LOGGERDEF void lg_str_write_into(lg_string* s,
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
#endif

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

// File extension for files and extension string's size
#define LOGGER_FILE_EXT ".log"
#define LOGGER_FILE_EXT_SZ 4

// log entry struct to be used at ring buffer
typedef struct {
  // main, plain message (resolved from variadics)
  char msg[LOGGER_MAX_MSG_SIZE];
  size_t length;
  lg_log_level level;
} log_entry_t;

// the ANSI bash color codes/escape characters
#define CLR_RED "\x1b[31m"     // for ERRORs
#define CLR_GREEN "\x1b[32m"   // for INFOs
#define CLR_YELLOW "\x1b[33m"  // for WARNs
#define CLR_AQUA "\x1b[36m"    // for TIMESTAMPs
#define CLR_RST "\x1b[0m"      // for plain message

// stb style implementation macros
#ifdef LOGGER_IMPLEMENTATION

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

// default and custom formatter distinguisher, used at consumer
static int format_msg(const int isLocalTime, const lg_log_level level,
                      const char* msg, lg_msg_pack* pack);

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

#ifdef _WIN32
#include <direct.h>  // _mkdir
#include <windows.h>

#define MKDIR(path) _mkdir(path)
#define PATH_SEP '\\'
#define PATH_MAX (MAX_PATH * 2) // for wchar_t

// TRANSPILATION FROM WINAPI TO POSIX FOR PTHREADS
typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

// mutex
static int pthread_mutex_lock(pthread_mutex_t *m) {
  EnterCriticalSection(m);
  return 0;
}
static int pthread_mutex_unlock(pthread_mutex_t *m) {
  LeaveCriticalSection(m);
  return 0;
}
static int pthread_mutex_init(pthread_mutex_t *m, void *attr) {
  LG_UNUSED(attr);
  if (!InitializeCriticalSectionAndSpinCount(m, 0x400))
    return ENOMEM;
  return 0;
}
static int pthread_mutex_destroy(pthread_mutex_t *m) {
  DeleteCriticalSection(m);
  return 0;
}

// threads
static DWORD WINAPI lg_thread_trampoline(LPVOID arg)
{
  // unpacking fn and arg
  typedef void* (*fn_t)(void*);
  fn_t fn = (fn_t)((void**)arg)[0];  // thread function
  void* rarg = ((void**)arg)[1];     // real_arg
  free(arg);
  fn(rarg);
  return 0;
}

// t: HANDLE / pthread_t
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
  *t = CreateThread(NULL, 0, lg_thread_trampoline, pack, 0, NULL);

  if (!*t) {
    free(pack);
    return 1;
  }
  return 0;
}

// cond
static int pthread_cond_init(pthread_cond_t* c, void* a) {
  LG_UNUSED(a);
  InitializeConditionVariable(c);
  return 0;
}
static int pthread_cond_signal(pthread_cond_t *c) {
  WakeConditionVariable(c);
  return 0;
}
#define pthread_cond_destroy(c) 0
#define pthread_cond_wait(c, m) !SleepConditionVariableCS(c, m, INFINITE)
#else  // UNIX
#include <pthread.h>
#include <sys/stat.h>  // for dirs
#include <sys/time.h>  // for time
#include <dirent.h>
#include <unistd.h>

#define MKDIR(path) mkdir(path, 0755)
#define PATH_SEP '/'
#endif  // _WIN32

/*
  Instance struct, keeps track the global variables before
*/
struct Logger {
  bool isAlive;
  bool isLocalTime;
  bool isPrintStdout;
  FILE* logFile;
  log_formatter_t customLogFunc;
  lg_log_policy logPolicy;
  int maxLogFiles; // non-positive = unlimited

  // global and writer mutex
  pthread_mutex_t mtx;
  pthread_mutex_t wmtx;

  // writer thread and cond
  pthread_cond_t wcond;
  pthread_cond_t pcond;
  pthread_t writer_tid;

  // ring buffer and its cursors
  log_entry_t ring[LOGGER_RING_SIZE];  // ring buffer
  size_t pcurr;                        // producer cursor
  size_t wcurr;                        // writer (consumer) cursor
};

// consumer func, writes entries on the ring to stdout or file
static void* lg_consumer(void* arg)
{
  Logger* inst = (Logger*)arg;

  while (1) {
    pthread_mutex_lock(&inst->wmtx);
    while (inst->wcurr == inst->pcurr && inst->isAlive) {
      pthread_cond_wait(&inst->wcond, &inst->wmtx);
    }
    // fetch these here (behind the mutex)
    size_t w = inst->wcurr;
    size_t p = inst->pcurr;
    int alive = inst->isAlive;
    bool is_stdout = inst->isPrintStdout;
    pthread_mutex_unlock(&inst->wmtx);

    while (w < p) {
      log_entry_t* slot = &inst->ring[w % LOGGER_RING_SIZE];
      size_t len = slot->length;
      if (len != 0) {
        char filebuf[LOGGER_MAX_MSG_SIZE];
        char stdoutbuf[LOGGER_MAX_MSG_SIZE];

        // C89 compatible initializer
        lg_msg_pack pack;
        pack.stdout_str.data = is_stdout ? stdoutbuf : NULL;
        pack.stdout_str.cap = sizeof(stdoutbuf);
        pack.stdout_str.len = 0;
        pack.file_str.data = filebuf;
        pack.file_str.cap = sizeof(filebuf);
        pack.file_str.len = 0;

        log_formatter_t custom_fn = inst->customLogFunc;
        bool status = false;
        if (custom_fn) status = custom_fn(inst->isLocalTime, slot->level, slot->msg, &pack);
        else status = format_msg(inst->isLocalTime, slot->level, slot->msg, &pack);
        if (!status) {
          LG_DEBUG_ERR("Cannot format the message, dropping it!");
          continue;
        }

        fwrite(pack.file_str.data, 1, pack.file_str.len, inst->logFile); // file
        if (is_stdout) {
          fwrite(pack.stdout_str.data, 1, pack.stdout_str.len, stdout); // stdout
        }
        slot->length = 0;
      }
      w += 1;
    }
    inst->wcurr = w;
    
    /*
    if we dont have any thing to do and logger is dead,
    so break the main loop if logger is destroyed and
    we have work to do, first we finishing our job and die
    */
    if (!alive && w == p) {
      LG_DEBUG("Writer thread is exiting");
      fflush(inst->logFile);
      if (is_stdout) fflush(stdout);
      break;
    }
  }

  return NULL;
}

// active logger instance if NULL, init tries to set it
static Logger* active_instance = NULL;

int lg_init_flat(Logger* inst, const char* logs_dir,
                int local_time, int print_stdout, int max_log_files,
                lg_log_policy log_policy,
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
  pthread_mutex_init(&inst->mtx, NULL);

  inst->isPrintStdout = config.printStdout != 0;
  inst->isLocalTime = config.localTime != 0;
  inst->maxLogFiles = config.maxFiles;
  inst->customLogFunc = config.logFormatter;
  inst->logPolicy = config.logPolicy;
  inst->pcurr = 0;
  inst->wcurr = 0;

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
    // get .log files in that logs folder
    char oldestFile[PATH_MAX];
    int files = count_logs_and_get_oldest(dir, oldestFile, sizeof(oldestFile));
    if (files < 0) {
      LG_DEBUG_ERR("Cannot count files and get oldest in %s", dir);
      return false;
    }

    // Remove the oldest file when max files are exceeded
    if (files >= config.maxFiles) {
      remove(oldestFile);
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
  inst->logFile = fopen(file_path, "wb");
  if (!inst->logFile) {
    LG_DEBUG_ERR("Cannot open the log file: %s", file_path);
    return false;
  }

  inst->isAlive = true;

  // create condition variables
  pthread_cond_init(&inst->wcond, NULL);
  pthread_cond_init(&inst->pcond, NULL);

  // create writer thread
  pthread_mutex_init(&inst->wmtx, NULL);
  if (pthread_create(&inst->writer_tid, NULL, lg_consumer, (void*)inst) != 0) {
    inst->isAlive = false;
    fclose(inst->logFile);
    LG_DEBUG_ERR("Cannot create writer process thread!");
    return false;
  }

  if (active_instance == NULL) active_instance = inst;

  return true;
}

// main log function with variadics - used at macros
// gets resolved message and calls lg_producer
int lg_vproducer(Logger* inst, const lg_log_level level, const char* fmt, ...)
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
int lg_producer(Logger* inst, const lg_log_level level, const char* msg, size_t msglen)
{
  if (!msg || msglen > LOGGER_MAX_MSG_SIZE) return false;

  if (!inst) {
    if (active_instance)
      inst = active_instance;
    else {
      LG_DEBUG_ERR("There is no active instance!");
      return false;
    }
  }

  // doesnt log when its not alive
  pthread_mutex_lock(&inst->mtx);
  if (!inst->isAlive) {
    pthread_mutex_unlock(&inst->mtx);
    LG_DEBUG_ERR("Cannot log because the instance is dead!");
    return false;
  }

  // unwrap producer and writer cursor and compute next index
  size_t p = inst->pcurr;
  size_t w = inst->wcurr;
  size_t next = p % LOGGER_RING_SIZE;

  // slot overflow check
  if (p - w >= LOGGER_RING_SIZE) {
    // buffer full, policies: DROP (default), OVERWRITE
    switch (inst->logPolicy) {
    case LG_DROP:
      goto drop;
    case LG_SMASH_OLDEST:
      goto smash_oldest;
    default:
      LG_DEBUG_ERR("Unkown log policy, defaulting to drop");
      goto drop;
    }

drop:
    LG_DEBUG_ERR("Ring buffer is full, dropping the log");
    pthread_mutex_unlock(&inst->mtx);
    return false;

smash_oldest:
    inst->wcurr += 1;
    LG_DEBUG_ERR("Ring buffer full, overwriting oldest entry");
    goto policy_done;
  }
policy_done:
  // copy message and put zero on ring
  memcpy(inst->ring[next].msg, msg, msglen);
  inst->ring[next].msg[msglen] = '\0';
  // copy other things
  inst->ring[next].level = level;
  inst->ring[next].length = msglen;

  // advance producer
  inst->pcurr = p + 1;
  pthread_mutex_unlock(&inst->mtx);

  // signal the writer
  pthread_mutex_lock(&inst->wmtx);
  pthread_cond_signal(&inst->wcond);
  pthread_mutex_unlock(&inst->wmtx);

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

  pthread_mutex_lock(&inst->mtx);
  pthread_mutex_lock(&inst->wmtx);
  // if it is not alive, do not try to destruct
  if (!inst->isAlive) {
    LG_DEBUG_ERR("Logger is already dead!");
    pthread_mutex_unlock(&inst->wmtx);
    pthread_mutex_unlock(&inst->mtx);
    return false;
  }
  inst->isAlive = false;

  // invoke the writer thread and it'll see its being destructed
  pthread_cond_signal(&inst->wcond);
  pthread_mutex_unlock(&inst->wmtx);

  pthread_join(inst->writer_tid, NULL);  // wait the thread to clear

  // clear config after thread exit
  inst->isPrintStdout = false;
  inst->isLocalTime = false;

  // close the file if its not closed
  if (inst->logFile != NULL) {
    if (fclose(inst->logFile) != 0) {
      LG_DEBUG_ERR("Log file cannot be closed!");
      pthread_mutex_unlock(&inst->mtx);
      return false;
    }
    inst->logFile = NULL;
  }

  // consumer things destroy
  pthread_mutex_destroy(&inst->wmtx);
  pthread_cond_destroy(&inst->wcond);
  pthread_cond_destroy(&inst->pcond);

  pthread_mutex_unlock(&inst->mtx);
  pthread_mutex_destroy(&inst->mtx);
  return true;
}

int lg_is_alive(const Logger* inst)
{
  if (!inst) {
    Logger* ins = lg_get_active_instance();
    return ins ? ins->isAlive : 0;
  }
  return inst->isAlive;
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

int lg_flogi(Logger* inst, const lg_log_level level, const char* msg)
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
int lg_flog(const lg_log_level level, const char* msg)
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

const char* lg_lvl_to_str(const lg_log_level level)
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
      memcpy(oldest_path, full_path, n);
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

void lg_str_format_into(lg_string* s, const char* fmt, ...)
{
  if (!s) return;
  if (!s->data || s->cap == 0) {
    s->len = 0;
    return;
  }

  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(s->data, s->cap, fmt, ap);
  va_end(ap);

  if (n < 0) {
    s->len = 0;
  } else if ((size_t)n >= s->cap) {
    size_t cap = s->cap;
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

void lg_str_write_into(lg_string* s, const char* str)
{
  if (!str) return;
  lg_str_format_into(s, "%s", str);
}

// message formatter helper - used at consumer
static int format_msg(const int isLocalTime, const lg_log_level level,
                      const char* msg, lg_msg_pack* pack)
{
  // getting time string
  char time_str[LOGGER_TIME_STR_SIZE];
  if (!lg_get_time_str(time_str, isLocalTime)) return false;

  // prepairing stdout msg
  if (pack->stdout_str.data) {
    #ifdef LOGGER_DONT_COLORIZE
    lg_str_format_into(
      &pack->stdout_str,
      "%s [%s] %s\n",
      time_str, lg_lvl_to_str(level), msg
    );
    #else
    const char* clr;
    switch (level) {
    case LG_ERROR:
      clr = CLR_RED;
      break;
    case LG_INFO:
      clr = CLR_GREEN;
      break;
    case LG_WARNING:
      clr = CLR_YELLOW;
      break;
    default:
      clr = CLR_RST;
      break;
    }
    lg_str_format_into(
      &pack->stdout_str,
      CLR_AQUA "%s %s[%s]" CLR_RST " %s\n",
      time_str, clr, lg_lvl_to_str(level), msg
    );
    #endif
  }

  // prepairing file msg
  if (pack->file_str.data) {
    // no escape chars in file
    lg_str_format_into(
      &pack->file_str,
      "%s [%s] %s\n", time_str,
      lg_lvl_to_str(level), msg);
  }
  return true;
}

#endif  // LOGGER_IMPLEMENTATION

#endif  // LOGGER_H
