#ifndef LOGGER_H
#define LOGGER_H

/**
   THIS IS STB-STYLE LIBRARY HEADER OF LOGGER.
   IT IS PURELY C. (EXCEPT SOME C++ COMPATIBILITY STUFF)
   YOU CAN USE IT LIKE A TRUE STB STYLE HEADER.
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
  #ifdef LOGGER_IMPLEMENTATION
   #define LOGGER_API 
  #elif defined(LOGGER_BUILD)
    #define LOGGER_API __declspec(dllexport)
  #else
    #define LOGGER_API __declspec(dllimport)
  #endif
#else // unix:
  #define LOGGER_API __attribute__((visibility("default")))
#endif

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
  This is the main producer function. It accepts format
  resolved message and manipulates it by customization.
  It pushes final message to be printed to the ring buffer.
  Then invokes the writer thread.
*/
LOGGER_API lg_result_t lg_producer(const char* level, const char* msg);

/*
  This is the format resolver for lg_producer.
  C macros use this. It's sensitive to "%" char.
  It may crash your app if you're using this on FFI or wrongly on C
*/
LOGGER_API lg_result_t lg_vproducer(const char* level, const char* fmt, ...);

/*
  Wrapper log functions for FFI, if you dont use C/C++ or
  using your language's FFI, you must use these:
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
  lg_vproducer(level, fmt, ##__VA_ARGS__)

#define lg_info(fmt, ...) lg_vproducer("INFO", fmt, ##__VA_ARGS__)
#define lg_error(fmt, ...) lg_vproducer("ERROR", fmt, ##__VA_ARGS__)
#define lg_warn(fmt, ...) lg_vproducer("WARNING", fmt, ##__VA_ARGS__)

// you can add your custom level like this:
#define lg_custom(fmt, ...) lg_vproducer("CUSTOM", fmt, ##__VA_ARGS__)

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
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

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

#ifdef _WIN32
  #include <windows.h>
  #include <direct.h> // _mkdir

  #define MKDIR(path) _mkdir(path)
  #define PATH_SEP '\\'

  typedef HANDLE pthread_t;
  typedef CRITICAL_SECTION pthread_mutex_t;
  typedef CONDITION_VARIABLE pthread_cond_t;

  // mutex
  #define pthread_mutex_init(m, a) InitializeCriticalSection(m)
  #define pthread_mutex_destroy(m) DeleteCriticalSection(m)
  #define pthread_mutex_lock(m) EnterCriticalSection(m)
  #define pthread_mutex_unlock(m) LeaveCriticalSection(m)

  // threads
  // transpilation of win32 createthread funcs to UNIX
  static DWORD WINAPI lg_thread_trampoline(LPVOID arg) {
    void* (*fn)(void*) = ((void**)arg)[0];
    void* real_arg     = ((void**)arg)[1]; // unpacking fn and arg
    free(arg);
    fn(real_arg);
    return 0;
  }

  // t: HANDLE / pthread_t
  #define pthread_join(t, retval)                 \
    do {                                          \
      (void)(retval);                             \
      WaitForSingleObject((t), INFINITE);         \
      CloseHandle((t));                           \
    } while (0)

  static int pthread_create(HANDLE* t, void* attr,
                          void* (*func)(void*), void* arg) {
    (void)attr;
    void** pack = (void**)malloc(sizeof(void*) * 2);
    if (pack == NULL) return 1;
    pack[0] = (void*)func;
    pack[1] = arg;
    *t = CreateThread(
      NULL, 0, lg_thread_trampoline, pack, 0, NULL
      );

    if (!*t) {
      free(pack);
      return 1;
    }
    
    return 0;
  }

  // cond
  #define pthread_cond_init(c, a) InitializeConditionVariable(c)
  #define pthread_cond_destroy(c) do { (void)c; } while (0)
  #define pthread_cond_wait(c, m) SleepConditionVariableCS(c, m, INFINITE)
  #define pthread_cond_signal(c) WakeConditionVariable(c)
#else // UNIX
  #include <sys/stat.h> // for dirs
  #include <sys/time.h> // for time
  #include <pthread.h>
  #define MKDIR(path) mkdir(path, 0755)
  #define PATH_SEP '/'
#endif // _WIN32

// C and C++ compatibility bullshit
#ifdef __cplusplus
  // atomic definitions
  #include <atomic>
  #define ATOMIC(T) std::atomic<T>
  #define aload(x) std::atomic_load_explicit(&(x), std::memory_order_acquire)
  #define astore(x, v) std::atomic_store_explicit(&(x), (v), std::memory_order_release)

  // alloca definition
  #ifdef _MSC_VER
    #include <malloc.h>
    #define LG_ALLOCA _alloca
  #else
    #include <alloca.h>
    #define LG_ALLOCA alloca
  #endif // _MSC_VER
#else // C:
  // atomic definitions
  #include <stdatomic.h>
  #define ATOMIC(T) _Atomic(T)
  #define aload(x) atomic_load_explicit(&(x), memory_order_acquire)
  #define astore(x, v) atomic_store_explicit(&(x), (v), memory_order_release)
#endif // __cplusplus

static ATOMIC(bool) isAlive = false;
static ATOMIC(bool) isLocalTime = false;
static ATOMIC(bool) isPrintStdout = false;
static FILE* logFile = NULL;
static int (*customLogFunc)(const char*, const char*,
                            const char*, char*, size_t) = NULL;

/*
  Gets time using kernel in this format: %Y.%m.%d-%H.%M.%S.%MS (%MS is millis in 3 digits)
  or YYYY.MM.DD-HH.MM.SS.SSS
  example: 2026.01.11-21.44.40.255 means January 11th, 2026, 21:44:40 or 9:44:40 pm, ms: 255
  If isLocalTime = false, spits out time in UTC format.
  Accepts buffer to write and its size
*/
static bool get_time_str(char* buf, size_t size);

// checks if dir is a valid directory (exists and directory)
static int check_dir(const char* path);

// recursively creates folders (behaves like mkdir -p)
static bool mkdir_p(const char *path);

// global mutex to prevent race conditions
static pthread_mutex_t mtx;
static pthread_mutex_t wmtx;

// writer thread mutex and cond
static pthread_cond_t cond;
static pthread_t writer_tid;

// ring buffer and its cursors
static log_entry_t ring[LOGGER_RING_SIZE]; // ring buffer
static ATOMIC(size_t) pcurr; // producer cursor
static ATOMIC(size_t) wcurr; // writer cursor

// consumer func, writes entries on the ring to stdout or file
static void* lg_consumer(void* arg) {
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
        fwrite(slot->message, 1, slot->length, f); // fprintf
        if (aload(isPrintStdout) == 1)
          fwrite(slot->message, 1, slot->length, stdout); // printf
        slot->length = 0;
      }
      w += 1;
    }
    astore(wcurr, w);
    // if we dont have any thing to do and logger is dead, so break the main loop
    // if logger is destructed and we have work to do, first we finishing our job and die
    if (!aload(isAlive) && aload(wcurr) == aload(pcurr)) {
      LG_DEBUG("Writer thread is exiting");
      fflush(f);
      if (aload(isPrintStdout))
        fflush(stdout);
      break;
    }
  }

  return NULL;
}

// main init func
lg_result_t lg_init(const char* logs_dir, LoggerConfig config) {
  pthread_mutex_init(&mtx, NULL);
  pthread_mutex_lock(&mtx);

  // silently return success if logger is alive
  if (aload(isAlive)) {
    pthread_mutex_unlock(&mtx);
    return LG_SUCCESS;
  }

  astore(isPrintStdout, config.printStdout != 0);
  astore(isLocalTime, config.localTime != 0);
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
#ifdef __cplusplus // some C++ shit
  char* file_path = (char*)LG_ALLOCA(len);
#else
  char file_path[len];
#endif
  
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
  
  astore(isAlive, true);
  
  // create writer thread
  pthread_mutex_init(&wmtx, NULL);
  pthread_cond_init(&cond, NULL);
  if (pthread_create(&writer_tid, NULL, lg_consumer, logFile) != 0) {
    astore(isAlive, false);
    fclose(logFile);
    LG_DEBUG_ERR("Cannot create writer process thread!");
    return LG_FATAL_ERROR;
  }
  
  pthread_mutex_unlock(&mtx);
  return LG_SUCCESS;
}

// main log function with variadics - used at macros
// gets resolved message and calls lg_producer
lg_result_t lg_vproducer(const char* level, const char* fmt, ...) {
  // variadic resolving
  va_list args;
  va_start(args, fmt);
  char msg[LOGGER_MAX_MSG_SIZE]; // we allocate enough memory
  int mn = vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  if (mn < 0) {
    LG_DEBUG_ERR("Cannot resolve print format");
    return LG_RUNTIME_ERROR;
  }

  return lg_producer(level, msg);
}

// main log function, invokes the writer - used at FFIs
// accepts format-resolved message to print
lg_result_t lg_producer(const char* level, const char* msg) {
  if (!msg || !level) return LG_RUNTIME_ERROR;

  // getting time str
  char time_str[24];
  if (!get_time_str(time_str, sizeof(time_str))) {
    LG_DEBUG_ERR("Cannot get time string");
    return LG_RUNTIME_ERROR;
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
    return LG_RUNTIME_ERROR;
  }

  // truncating the string
  size_t buflen = (size_t)n < sizeof(buf)
    ? (size_t)n : sizeof(buf) - 1;
  
  pthread_mutex_lock(&mtx);

  // doesnt log when its not alive
  if (!aload(isAlive)) {
    LG_DEBUG_ERR("Cannot log because logger is ded :skull:");
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
  
  memcpy(ring[next].message, buf, buflen);
  ring[next].message[buflen] = '\0';
  ring[next].length = buflen;

  // advance producer
  astore(pcurr, p + 1);
  
  pthread_mutex_unlock(&mtx);

  // signal the writer
  pthread_mutex_lock(&wmtx);
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&wmtx);

  return LG_SUCCESS;
}

lg_result_t lg_destruct(void) {
  pthread_mutex_lock(&mtx);

  pthread_mutex_lock(&wmtx);
  // if it is not alive, do not try to destruct
  if (!aload(isAlive)) {
    LG_DEBUG_ERR("Logger is already dead!");
    pthread_mutex_unlock(&wmtx);
    pthread_mutex_unlock(&mtx);
    return LG_RUNTIME_ERROR;
  }
  astore(isAlive, false);
  
  // invoke the writer thread and it'll see its being destructed
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&wmtx);
  
  pthread_join(writer_tid, NULL); // wait the thread to clear

  // clear config after thread exit
  astore(isPrintStdout, false);
  astore(isLocalTime  , false);
  
  // close the file if its not closed
  if (logFile != NULL) {
    if (fclose(logFile) != 0) {
      LG_DEBUG_ERR("Log file cannot be closed!");
      pthread_mutex_unlock(&mtx);
      return LG_FATAL_ERROR;
    }
    logFile = NULL;
  }

  // consumer things destroy
  pthread_mutex_destroy(&wmtx);
  pthread_cond_destroy(&cond);
  
  pthread_mutex_unlock(&mtx);
  pthread_mutex_destroy(&mtx);
  return LG_SUCCESS;
}

int lg_is_alive() {
  return aload(isAlive);
}

lg_result_t lg_log_s(const char* level, const char* msg) {
  if (!msg) return LG_RUNTIME_ERROR;
  return lg_producer(level, msg);
}

lg_result_t lg_info_s(const char* msg) {
  if (!msg) return LG_RUNTIME_ERROR;
  return lg_producer("INFO", msg);
}

lg_result_t lg_error_s(const char* msg) {
  if (!msg) return LG_RUNTIME_ERROR;
  return lg_producer("ERROR", msg);
}

lg_result_t lg_warn_s(const char* msg) {
  if (!msg) return LG_RUNTIME_ERROR;
  return lg_producer("WARN", msg);
}

static bool get_time_str(char* buf, size_t size) {
  if (!buf || size == 0) return false;

#ifdef _WIN32
  SYSTEMTIME st;
  if (aload(isLocalTime) == 1) GetLocalTime(&st);
  else GetSystemTime(&st);
  int n = snprintf(buf, size, "%04d.%02d.%02d-%02d.%02d.%02d.%03d",
                   st.wYear, // 2026
                   st.wMonth, // 1
                   st.wDay, // 21
                   st.wHour, // 22
                   st.wMinute, // 16
                   st.wSecond, // 40
                   st.wMilliseconds // 450
    );
  if (n <= 0 || (size_t)n >= size) return false;
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
  if (n <= 0 || (size_t)n >= size) return false;
  //            ^^^^^^^^^^^^^^^^^, buffer has not enough capacity
#endif // _WIN32
  return true;
}

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

static bool mkdir_p(const char *path) {
  if (!path || !*path) return false;

  char tmp[PATH_MAX];
  size_t size = sizeof(tmp);
  int n = snprintf(tmp, size, "%s", path);
  if (n < 0 || (size_t)n >= size) return false;
 
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
      return false;
    }
    *p = PATH_SEP; // fix that \0
  }

  LG_DEBUG("Trying to create: %s", tmp);
  return MKDIR(tmp) == 0 || errno == EEXIST;
}

#endif // LOGGER_IMPLEMENTATION

#endif // LOGGER_H
