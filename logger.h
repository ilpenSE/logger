#ifndef LOGGER_H
#define LOGGER_H

/**
   THIS IS STB-STYLE LIBRARY HEADER OF LOGGER.
   IT IS PURELY C. (EXCEPT SOME C++ COMPATIBILITY STUFF)
   YOU CAN USE IT LIKE A TRUE STB STYLE HEADER.
   WARNING: THIS DOES NOT SUPPORT WINSLOP ENVIRONMENT! LIKE MSVC OR WIN32 THREADS!
   ONLY SUPPORTS POSIX ENVIRONMENTS AND NOT TESTED ON ARM CPUS
   TESTED ON AMD64 GNU-LINUX WITH GCC AND CLANG

   MACROS THAT YOU CAN USE:
   LOGGER_IMPLEMENTATION -> Implementation of this header (USE THIS ON COMPILATION OR USAGE)
   LOGGER_STRIP_PREFIXES -> Strips "lg_" prefixes on log macros except lg_log
   LOGGER_MINIFY_PREFIXES -> Minifies "lg_" prefix into just "l" including lg_log
   LOGGER_DEBUG -> If this enabled, logger's internal errors will be shown at stderr. You may doesnt want to use this at production
   
   USAGE OF STB STYLE HEADER:
   #define LOGGER_IMPLEMENTATION
   #define LOGGER_STRIP_PREFIXES
   #define LOGGER_MINIFY_PREFIXES
   #include "logger.h"

   int main() {
   if (lg_init("logs", {.localTime=1, .printStdout=1 .logFormatter=NULL}) != 1) return 1;
   char name[33];
   scanf("%32s", &name);
   lg_info("Hello, %s!", name);
   info("Hello, %s!", name);
   linfo("Hello, %s!", name);
   if (strcmp(name, "ilpeN") == 0)
     info("Hello, sir!");
   if (lg_destruct() != 1) return 1;
   }

   In this asynchronous logger, we have main and writer thread.
   Main thread manages lifetime and putting logs into ring buffer.
   Writer thread writes messages that in the ring buffer to log file
   and stdout if you provided.
   Sometimes, your log messages may be disappeared if you stress-test it.
   That's because the ring buffer is a fixed-size buffer and it is full.
   I have decided the "DROP THE LOG" policy at those situations but if you
   provide printStdout as 1, since the writer thread gonna be slow,
   you just dont get any dropped log. To see in where it dropped, you can define
   LOGGER_DEBUG in your compilation (with -DLOGGER_DEBUG) or runtime (#define LOGGER_DEBUG)
   
   The config printStdout significantly slows down both of the threads
   if you're using this at prod, dont forget to make printStdout as 0

   If you're using implementation macro, you dont have to dynamically
   link the library but you have to use c types and functions again.

   This header is safe to use in C++ (atomic keywords will be dispatched)
   but if you use C, you have to use at least C11 standart
*/

// for cross-platform compatibility
#ifdef _WIN32
  #ifdef LOGGER_BUILD
    #define LOGGER_API __declspec(dllexport)
  #else
    #define LOGGER_API __declspec(dllimport)
  #endif // LOGGER_BUILD
#else
  #define LOGGER_API __attribute__((visibility("default")))
#endif // _WIN32

// some global includes here
#include <stdarg.h>
#include <stddef.h>

// the logger function results via enums
typedef enum {
  LG_FATAL_ERROR = -1, // if this happens, destruct immediatel
  LG_RUNTIME_ERROR = 0, // can be thrown in runtime, not fatal to crash program
  LG_SUCCESS = 1,
  LG_NOT_VALID_DIR = 2,
  LG_NOT_OPEN_FILE = 3,
  LG_RING_FULL = 4
} lg_result_t;

/*
 * Config struct, this struct can be used in lg_init.
 * localTime: this members also known as "use_local_time", if it is 1, time is calculated on local timezone
 * printStdout: if it is in fact true, logger tries to print message to stdout otherwise it doesnt
 * logFormatter: this function describes the logging format. By default, it is like:
     time level msg
     its parameters: time str, level, formatted message, out buffer, out buffer size
     returns: return value of snprintf or how many characters be wanted to be written
     btw: you are given by time str, level and formatted message (fmt + va_list'ed)
     you can customize it and you have to write your message into out buffer the size of buffer is size
*/
typedef struct {
  int localTime;
  int printStdout;
  int (*logFormatter)(const char* time_str, const char* level, const char* msg,
                      char* out, size_t out_size);
} LoggerConfig;

#ifdef __cplusplus
extern "C" {
#endif

/*
   Main initializer function
   @param logs_dir: const char*, Points a directory path
   (if not exists, it'll try to create). It can be relative or absolute path.
   @param config: LoggerConfig,
   localTime: Changes behavior of get_time_str(). If 1 or true, that function
   will try to generate time string BY YOUR LOCAL TIME ZONE. If it is false or 0, it'll use
   UTC as a convention.
   printStdout: If this is true or 1, writer thread actually tries to write log message to stdout. By default, it just doesnt write to stdout because it is too slow
   logFormatter: Custom log message formatter function, you are given by time_str that comes from get_time_str, level and formatted message, output buffer and output buffer's size. Your job is to implement and give this function if you'll use customization and write your message in snprintf. You can make this NULL and it uses default formatting (time_str [level] msg)
   @returns return value of snprintf that you used to write out buffer or how many chars written to out
   RECOMMENDED: on your app's entry point, check its return value like if(!lg_init(...))
   You dont have to use lg_destruct() if init failed. Because if init failed, isAlive set to be 0
   and lg_destruct() simply wont work if logger is sleeping
*/
LOGGER_API lg_result_t lg_init(const char* logs_dir, LoggerConfig config);

/*
  Destructs logger instance and closes the log file that the instance working on
*/
LOGGER_API lg_result_t lg_destruct(void);

// returns if logger is alive
LOGGER_API int lg_is_alive();

/*
  LOG FUNCTIONS, if you have EYES and a BRAIN you can read lg_xxx part and
  see that which function log in which level.
  The "lg_log" requires level if you have special levels
  lg_xxx (except log) is wrapper of lg_log
  @since 2.1: these functions accept variadic args like in printf
  @since 2.2: this function DOES NOT do any write call it just appends message entry to ring buffer and invokes writer thread (producer)
*/
LOGGER_API lg_result_t lg_vlog(const char* level, const char* fmt, ...);

  /*
    Wrapper log functions for FFI, if you dont use C/C++ and using your language's FFI,
    you must use these kind of stuff
   */
LOGGER_API lg_result_t lg_info_s(const char* msg);
LOGGER_API lg_result_t lg_error_s(const char* msg);
LOGGER_API lg_result_t lg_warn_s(const char* msg);
  
#ifdef __cplusplus
}
#endif

// log functions to be going to used is macros now
// main function is lg_vlog
#define lg_log(level, fmt, ...) \
  lg_vlog(level, fmt, ##__VA_ARGS__)

#define lg_info(fmt, ...) lg_log("INFO", fmt, ##__VA_ARGS__)
#define lg_error(fmt, ...) lg_log("ERROR", fmt, ##__VA_ARGS__)
#define lg_warn(fmt, ...) lg_log("WARNING", fmt, ##__VA_ARGS__)

// you can add your custom level like this:
#define lg_custom(fmt, ...) lg_log("CUSTOM", fmt, ##__VA_ARGS__)

// strips ONLY log functions
#ifdef LOGGER_STRIP_PREFIXES
#define info  lg_info
#define warn  lg_warn
#define error lg_error
#endif // LOGGER_STRIP_PREFIXES

// minify prefix from lg_ to l
#ifdef LOGGER_MINIFY_PREFIXES
#define linfo  lg_info
#define lwarn  lg_warn
#define lerror lg_error
#endif // LOGGER_MINIFY_PREFIXES

// do not define llog multiple times
// btw lg_log's stripped/minified version should be "llog" instead of "log"
// bcs math.h collides with this
#if defined(LOGGER_MINIFY_PREFIXES) || defined(LOGGER_STRIP_PREFIXES)
#define llog lg_log
#endif // LOGGER_STRIP_PREFIXES OR LOGGER_MINIFY_PREFIXES

// logger max message size (you can change it)
#define LOGGER_MAX_MSG_SIZE 1024
// ring buffer's size
#define LOGGER_RING_SIZE 1024

// log entry struct to be used at ring buffer
typedef struct {
  char message[LOGGER_MAX_MSG_SIZE];
  size_t length;
} log_entry_t;

// stb style implementation macros
#ifdef LOGGER_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h> // TODO: Make this threads for NON-UNIX OS's like micr*slop's AI-bloated winslop

#ifdef _WIN32
#include <windows.h>
#include <direct.h> // _mkdir

#define MKDIR(path) _mkdir(path)
#define PATH_SEP '\\'

#else
#include <sys/stat.h> // for dirs
#include <sys/time.h> // for time

#define MKDIR(path) mkdir(path, 0755)
#define PATH_SEP '/'

#endif // _WIN32

// i stands for "internal", so lgierror means logger internal error
// lgierror - prints out where it happened
#define lgierror(fmt, ...) \
  do { \
    fprintf(stderr, "%s:%d: ERROR: " fmt "\n", \
            __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

// lgiprint - auto adds \n at the end and where
#define lgiprint(fmt, ...) \
  do { \
    printf("%s:%d: INFO: " fmt "\n", \
           __FILE__, __LINE__, ##__VA_ARGS__);  \
  } while(0)

// define this macro to enable error messages in stderr
// some sort of new version of "LOGGER_VERBOSE" but not exactly the same thing
#ifdef LOGGER_DEBUG
#define LG_DEBUG_ERR(fmt, ...) lgierror(fmt, ##__VA_ARGS__)
#define LG_DEBUG(fmt, ...) lgiprint(fmt, ##__VA_ARGS__)
#else
#define LG_DEBUG_ERR(fmt, ...) ((void)0) // swallow
#define LG_DEBUG(fmt, ...) ((void)0)
#endif // LOGGER_DEBUG

// on c++, we dont have _Atomic so we define it
#ifdef __cplusplus
#include <atomic>
#define ATOMIC(T) std::atomic<T>
#else
#include <stdatomic.h>
#define ATOMIC(T) _Atomic(T)
#endif // __cplusplus

static ATOMIC(int) isAlive = 0;
static ATOMIC(int) isLocalTime = 0;
static ATOMIC(int) isPrintStdout = 0;
static FILE* logFile = NULL;
static int (*customLogFunc)(const char*, const char*,
                            const char*, char*, size_t) = NULL;

// some quick atomic loads and sets
#ifdef __cplusplus
#define aload(x) std::atomic_load_explicit(&(x), std::memory_order_acquire)
#define astore(x, v) std::atomic_store_explicit(&(x), (v), std::memory_order_release)
#else
#define aload(x) atomic_load_explicit(&(x), memory_order_acquire)
#define astore(x, v) atomic_store_explicit(&(x), (v), memory_order_release)
#endif

/*
  Gets time using kernel in this format: %Y.%m.%d-%H.%M.%S.%MS (%MS is millis in 3 digits)
  or YYYY.MM.DD-HH.MM.SS.SSS
  example: 2026.01.11-21.44.40.255 means January 11th, 2026, 21:44:40 or 9:44:40 pm, ms: 255
  If isLocalTime = 0, spits out time in UTC format.
  Accepts buffer to write and its size
*/
static lg_result_t get_time_str(char* buf, size_t size) {
  if (!buf || size == 0) return LG_RUNTIME_ERROR;

#ifdef _WIN32
  SYSTEMTIME st;
  if (aload(isLocalTime) == 1) GetLocalTime(&st);
  else GetSystemTime(&st);
  int n = snprintf(buf, size, "%04d.%02d.%02d-%02d.%02d.%02d.%03ld",
                   st.wYear, // 2026
                   st.wMonth, // 1
                   st.wDay, // 21
                   st.wHour, // 22
                   st.wMinute, // 16
                   st.wSecond, // 40
                   st.wMilliseconds // 450
    );
  if (n <= 0 || (size_t)n >= size) return LG_RUNTIME_ERROR;
#else // unix
  // get nanosec with struct
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  // get other fields except ns
  struct tm tm_val;
  if (aload(isLocalTime) == 1) localtime_r(&ts.tv_sec, &tm_val);
  else gmtime_r(&ts.tv_sec, &tm_val);

  long ms = ts.tv_nsec / 1000000; // nanosecond -> millisecond

  int n = snprintf(buf, size, "%04d.%02d.%02d-%02d.%02d.%02d.%03ld",
                   tm_val.tm_year + 1900,
                   tm_val.tm_mon + 1,
                   tm_val.tm_mday,
                   tm_val.tm_hour,
                   tm_val.tm_min,
                   tm_val.tm_sec,
                   ms
    );
  if (n <= 0 || (size_t)n >= size) return LG_RUNTIME_ERROR;
  //            ^^^^^^^^^^^^^^^^^, buffer has not enough capacity
#endif // _WIN32
  return LG_SUCCESS;
}

// checks if dir is a valid directory (exists and directory)
static int check_dir(const char* path) {
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(path);
  if (attr == INVALID_FILE_ATTRIBUTES) return 0; // does not exists
  if (attr & FILE_ATTRIBUTE_DIRECTORY) return 1; // exists AND directory (valid)
  return -1; // exists but is not directory
#else
  struct stat st;
  if (stat(path, &st) != 0) return 0; // not exists
  if (S_ISDIR(st.st_mode)) return 1; // exists AND directory (valid)
  return -1; // exists but not a directory
#endif
}

// recursively creates folders (behaves like mkdir -p)
static int mkdir_p(const char *path) {
  if (!path || !*path) return 0;

  char tmp[PATH_MAX];
  size_t size = sizeof(tmp);
  int n = snprintf(tmp, size, "%s", path);
  if (n < 0 || (size_t)n >= size) return -1;
 
  // go char by char
  // it expects "/" at the end to create valid directories
  // if you dont provide trailing slash, you cannot make the folder at the end
  for (char *p = tmp; *p; p++) {
    if (*p != PATH_SEP) continue;
    if (p != tmp && *(p-1) == PATH_SEP) continue; // skip double slashes
    *p = '\0'; // give MKDIR string that we passed before
    if (!*tmp) { // tmp is empty, skip
      *p = PATH_SEP;
      continue;
    }
    LG_DEBUG("Trying to create: %s", tmp);
    int status = MKDIR(tmp);
    // swallow already exists errors
    if (status != 0 && errno != EEXIST) {
      LG_DEBUG_ERR("Failed to create path: %s", path);
      return -1;
    }
    *p = PATH_SEP; // fix that \0
  }

  LG_DEBUG("Trying to create: %s", tmp);
  return MKDIR(tmp) == 0 || errno == EEXIST;
}

// global mutex to prevent race conditions
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t wmtx = PTHREAD_MUTEX_INITIALIZER;

// writer thread mutex and cond
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_t writer_tid;

// ring buffer and its cursors
static log_entry_t ring[LOGGER_RING_SIZE]; // ring buffer
static ATOMIC(size_t) pcurr; // producer cursor
static ATOMIC(size_t) wcurr; // writer cursor

static void* writer_thread_func(void* arg) {
  FILE* f = (FILE*)arg;
  
  while (1) {
    pthread_mutex_lock(&wmtx);
    while (aload(wcurr) == aload(pcurr) && aload(isAlive)) {
      pthread_cond_wait(&cond, &wmtx);
    }
    pthread_mutex_unlock(&wmtx);

    size_t w = aload(wcurr);
    size_t p = aload(pcurr);

    while (w < p) {
      log_entry_t* slot = &ring[w % LOGGER_RING_SIZE];
      size_t len = slot->length;
      if (len != 0) {
        //LG_DEBUG("Writer recieved \"%s\", length = %zu", slot->message, slot->length);
        fwrite(slot->message, 1, slot->length, f); // fprintf
        if (aload(isPrintStdout) == 1)
          fwrite(slot->message, 1, slot->length, stdout); // printf

        //LG_DEBUG("Writer wrote \"%s\", length = %zu", slot->message, slot->length);
        slot->length = 0;
      }
      w += 1;
    }
    astore(wcurr, w);
    // if we dont have any thing to do and logger is dead, so break the main loop
    // if logger is destructed and we have work to do, first we finishing our job and die
    if (aload(isAlive) == 0 && aload(wcurr) == aload(pcurr)) {
      LG_DEBUG("Writer thread is exiting");
      fflush(f);
      if (aload(isPrintStdout) == 1)
        fflush(stdout);
      break;
    }
  }

  return NULL;
}

// main init func
lg_result_t lg_init(const char* logs_dir, LoggerConfig config) {
  pthread_mutex_lock(&mtx);

  // silently return success if logger is alive
  if (aload(isAlive)) {
    pthread_mutex_unlock(&mtx);
    return LG_SUCCESS;
  }

  astore(isPrintStdout, config.printStdout);
  astore(isLocalTime, config.localTime);
  customLogFunc = config.logFormatter;

  atomic_init(&pcurr, 0);
  atomic_init(&wcurr, 0);

  int dir_status = check_dir(logs_dir); // -1 = NOT valid directory, 0 = NOT exists

  // handle not a valid directory
  if (dir_status == -1) {
    LG_DEBUG_ERR("Provided path is not a valid directory to create: %s", logs_dir);
    pthread_mutex_unlock(&mtx);
    return LG_NOT_VALID_DIR;
  }

  // handle just non-exist directory (best effort)
  if (dir_status == 0) {
    if (!mkdir_p(logs_dir)) {
      LG_DEBUG_ERR("Cannot create provided path: %s", logs_dir);
      pthread_mutex_unlock(&mtx);
      return LG_NOT_VALID_DIR;
    }
  }

  // get time str and length
  char time_str[24];
  if (get_time_str(time_str, sizeof(time_str)) != 1) {
    pthread_mutex_unlock(&mtx);
    return LG_FATAL_ERROR;
  }

  size_t len = strlen(logs_dir) + 29;
  char file_path[len]; // TODO: for clang++, it generates warning like this:
  /*
    In file included from main.cpp:9:
../../logger.h:470:18: warning: variable length arrays in C++ are a Clang extension [-Wvla-cxx-extension]
  470 |   char file_path[len];
      |                  ^~~
../../logger.h:470:18: note: read of non-const variable 'len' is not allowed in a constant expression
../../logger.h:469:10: note: declared here
  469 |   size_t len = strlen(logs_dir) + 29;
      |          ^
1 warning generated.
Make this warning disappear and dont use C++ std::vector or smth
   */
  
  // normalize logs dir name (add trailing slash)
  char last_elem = logs_dir[strlen(logs_dir) - 1];
  int n = 0;
  if (last_elem == '/' || last_elem == '\\') {
    n = snprintf(file_path, len, "%s%s.log", logs_dir, time_str);
  } else {
    n = snprintf(file_path, len, "%s/%s.log", logs_dir, time_str);
  }
  
  // if file name shitted
  if (n <= 0 || (size_t)n >= len) {
    pthread_mutex_unlock(&mtx);
    return LG_FATAL_ERROR;
  }
  
  // open file in write mode
  logFile = fopen(file_path, "w");
  if (!logFile) {
    LG_DEBUG_ERR("Cannot open the log file: %s", file_path);
    pthread_mutex_unlock(&mtx);
    return LG_NOT_OPEN_FILE;
  }
  
  astore(isAlive, 1);
  // create writer thread
  if (pthread_create(&writer_tid, NULL, writer_thread_func, logFile) != 0) {
    astore(isAlive, 0);
    fclose(logFile);
    LG_DEBUG_ERR("Cannot create writer process thread!");
    return LG_FATAL_ERROR;
  }
  
  pthread_mutex_unlock(&mtx);
  return LG_SUCCESS;
}

// main log function, prepairs the message and invokes the writer
lg_result_t lg_vlog(const char* level, const char* fmt, ...) {
  pthread_mutex_lock(&mtx);

  // doesnt log when it is destructed or some weird states
  if (aload(isAlive) != 1) {
    LG_DEBUG_ERR("Cannot log because logger is ded :skull:");
    pthread_mutex_unlock(&mtx);
    return LG_RUNTIME_ERROR;
  }
  
  // variadic processing
  va_list args;
  va_start(args, fmt);
  char msg[LOGGER_MAX_MSG_SIZE]; // we allocate enough memory
  int mn = vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  if (mn < 0) {
    LG_DEBUG_ERR("Cannot resolve print format");
    pthread_mutex_unlock(&mtx);
    return LG_RUNTIME_ERROR;
  }
  
  char time_str[24];
  if (get_time_str(time_str, sizeof(time_str)) != 1) {
    LG_DEBUG_ERR("Cannot get time string");
    pthread_mutex_unlock(&mtx);
    return LG_RUNTIME_ERROR;
  }
  
  // unwrap producer cursor and compute next index
  size_t p = aload(pcurr);
  size_t next = p % LOGGER_RING_SIZE;

  // slot overflow check
  size_t w = aload(wcurr);
  if (p - w >= LOGGER_RING_SIZE) {
    // buffer full, policy: drop
    LG_DEBUG_ERR("Ring buffer is full, dropping the log");
    pthread_mutex_unlock(&mtx);
    return LG_RING_FULL;
  }
  
  // log message customization starts here
  char buf[LOGGER_MAX_MSG_SIZE];
  int n;
  if (customLogFunc == NULL) {
    n = snprintf(buf, sizeof(buf), "%s [%s] %s\n", time_str, level, msg);
  } else {
    n = customLogFunc(time_str, level, msg, buf, sizeof(buf));
  }

  if (n < 0) {
    LG_DEBUG_ERR("Encoding or writing to message buffer error on lg_vlog!");
    pthread_mutex_unlock(&mtx);
    return LG_RUNTIME_ERROR;
  }

  // truncate string
  size_t actual_len = (size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf) - 1;
  memcpy(ring[next].message, buf, actual_len);
  ring[next].message[actual_len] = '\0'; // dont forget to add null-terminator

#ifdef __cplusplus
  std::atomic_thread_fence(std::memory_order_release);
#else
  atomic_thread_fence(memory_order_release);
#endif
  
  ring[next].length = actual_len;
  //LG_DEBUG("Producer appended to ring: \"%s\", length = %zu", ring[next].message, actual_len);
  // advance producer and invoke writer thread
  astore(pcurr, p + 1);
  pthread_mutex_unlock(&mtx);

  pthread_mutex_lock(&wmtx);
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&wmtx);
  //LG_DEBUG("Producer invoked writer thread");

  return LG_SUCCESS;
}

int lg_is_alive() {
  return aload(isAlive);
}

lg_result_t lg_destruct(void) {
  pthread_mutex_lock(&mtx);

  pthread_mutex_lock(&wmtx);
  // if it is not alive, do not try to destruct
  if (aload(isAlive) == 0) {
    LG_DEBUG_ERR("Logger is already dead!");
    pthread_mutex_unlock(&wmtx);
    pthread_mutex_unlock(&mtx);
    return LG_RUNTIME_ERROR;
  }
  astore(isAlive, 0);
  
  // invoke the writer thread and it'll see its being destructed
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&wmtx);
  
  pthread_join(writer_tid, NULL); // wait the thread to clear

  // clear config after thread exit
  astore(isPrintStdout, 0);
  astore(isLocalTime, 0);
  
  // close the file if its not closed
  if (logFile != NULL) {
    if (fclose(logFile) != 0) {
      pthread_mutex_unlock(&mtx);
      LG_DEBUG_ERR("Log file cannot be closed!");
      return LG_FATAL_ERROR;
    }
    logFile = NULL; // clear ptr
  }
  
  pthread_mutex_unlock(&mtx);
  return LG_SUCCESS;
}

lg_result_t lg_log_s(const char* level, const char* msg) {
  return lg_vlog(level, "%s", msg);
}

lg_result_t lg_info_s(const char* msg) {
  return lg_log_s("INFO", msg);
}

lg_result_t lg_error_s(const char* msg) {
  return lg_log_s("ERROR", msg);
}

lg_result_t lg_warn_s(const char* msg) {
  return lg_log_s("WARN", msg);
}

#endif // LOGGER_IMPLEMENTATION

#endif // LOGGER_H
