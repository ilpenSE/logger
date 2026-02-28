#ifndef LOGGER_H
#define LOGGER_H

/*
   THIS IS STB-STYLE LIBRARY HEADER OF LOGGER.
   IT IS WRITTEN IN PURE C, SAFE TO USE IN C++
   REQUIRES MINIMUM C99 VERSION
   TESTED ON AMD64 GNU-LINUX WITH GCC AND CLANG

   MACROS THAT YOU CAN USE:
   LOGGER_IMPLEMENTATION -> Implementation of this header (USE THIS ON COMPILATION OR USAGE)
   LOGGER_MINIFY_PREFIXES -> Minifies "lg_" prefix into just "l" including lg_log
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

   Main initializer function that you may use in C/C++:
   int lg_init(Logger* instance, const char* logs_dir, LoggerConfig config);

   The LoggerConfig struct:
   typedef struct {
     int localTime;
     int printStdout;
     lg_log_policy policy;
     log_formatter_t logFormatter;
   } LoggerConfig;
   
   Flatted version of LoggerConfig
   it's for languages that doesnt support C structs (e.g: Bun FFI in JavaScript)
   int lg_init_flat(Logger* inst, const char* logs_dir,
      int local_time, int print_stdout, lg_log_policy policy, log_formatter_t log_formatter);
   
   Main destroyer function that destroys logger instances (DOES NOT MANAGE MEMORY!)
   int lg_destroy(Logger* instance);

   Check if specific instance is dead or alive (if instance = NULL, checks active instance)
   int lg_is_alive(const Logger* instance);

   Main producer function that puts message, time string and level into the ring
   (NOT RECOMMENDED TO USE DIRECTLY, USE MACROS OR F-FUNCTIONS)
   int lg_producer(Logger* inst, const lg_log_level level, const char* msg);
   
   Wrapper for producer, takes variadics and processes it, used at macros
   int lg_vproducer(Logger* inst, const lg_log_level level, const char* fmt, ...);

   Functions that used at FFIs (F-functions), the level-less and level-aware functions here:
   int lg_flog(const lg_log_level level, const char* msg);
   int lg_finfo(const char* msg);
   int lg_fwarn(const char* msg);
   int lg_ferror(const char* msg);

   F-functions with explicit instance, you can put NULL there if you wanna use active instance
   (thats how functions above works)
   int lg_flogi(Logger* inst, const lg_log_level level, const char* msg);
   int lg_finfoi(Logger* inst, const char* msg);
   int lg_ferrori(Logger* inst, const char* msg);
   int lg_fwarni(Logger* inst, const char* msg);
   
   Getter and setter for active instance (lg_init automatically sets active instance if it's NULL)
   int lg_set_active_instance(Logger* inst);
   Logger* lg_get_active_instance();

   Helper functions that you can use:

   Converts level enum to string
   const char* lg_lvl_to_str(const lg_log_level level);
   
   Used at consumer and you can use these on your custom formatter, go check static format_msg
   void lg_str_format_into(lg_string* s, const char* fmt, ...);
   void lg_str_write_into(lg_string* s, const char* already_formatted_str);

   Allocator and freer for heap allocated instances or foreign languages
   Logger* lg_alloc();
   void lg_free(Logger* inst);

   Explanation of this logger library (how it works):
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

   We have support for multi instances, but if you dont want to use multi instance,
   just create and allocate an instance on heap (use lg_alloc())) and init it.
   To destroy it, call lg_destroy() and free it (you can call lg_free())
   If you are using non C/C++ language, you have to use lg_alloc and lg_free

   The config printStdout significantly slows down both of the threads
   if you're using this at prod, dont forget to make printStdout as 0

   If you're using implementation macro, you dont have to dynamically
   link the library but you have to use C types and functions again.
*/

#ifdef __cplusplus
  #define LOGGERDEF extern "C"
#else
  #define LOGGERDEF extern
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
  LG_DROP (default): Drops the log when buffer is full, you lost some log messages
  LG_SMASH_OLDEST: Remove oldest one and keep going
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

typedef void (*log_formatter_t)(const char* time_str, const lg_log_level level, const char* msg,
                               lg_msg_pack* pack);

/*
 * Config struct, this struct can be used in lg_init.
 * localTime: this members also known as "use_local_time", if it is 1, time is calculated on local timezone
 * printStdout: if it is true, logger tries to print message to stdout otherwise it doesnt
 * logFormatter: this function describes the logging format. By default, it is like:
     time level msg
     its parameters: time str, level, formatted message, pack
     returns: nothing (void)
     the pack is that you put stdout and file string messages. You can distinguish messages
     that is gonna be printed stdout or file. Go to pack and string defitions.
*/
typedef struct {
  int localTime;
  int printStdout;
  lg_log_policy policy;
  log_formatter_t logFormatter;
} LoggerConfig;

/*
   Main initializer function, Creates Logger instance
   @param logs_dir: const char*, Points a directory path
   (if not exists, it'll try to create). It can be relative or absolute path.
   
   @param config: LoggerConfig,
   //////////////////////////////
   localTime: Changes behavior of get_time_str(). If 1 or true, that function
   will try to generate time string BY YOUR LOCAL TIME ZONE. If it is false or 0, it'll use
   UTC as a convention.
   //////////////////////////////
   printStdout: If this is true or 1, writer thread actually tries to write log message to stdout.
   By default, it just doesnt write to stdout because it is too slow
   //////////////////////////////
   logFormatter: Custom log message formatter function, you are given by time_str that comes from
   get_time_str, level and formatted message and the message pack. Your job is to implement and
   give this function if you'll use customization and write your message into the pack by using
   str_format_into. You can make this NULL and it uses default formatting (time_str [level] msg)
   
   RECOMMENDED: on your app's entry point, check its return value like if(!lg_init(...))
   You dont have to use lg_destroy() if init failed. Because if init failed, isAlive set to be 0
   and lg_destroy() simply wont work if the logger instance is dead
*/
LOGGERDEF int lg_init(Logger* instance, const char* logs_dir, LoggerConfig config);

/*
  lg_init but Flattened the config struct (mostly used at FFIs)
*/
LOGGERDEF int lg_init_flat(Logger* inst, const char* logs_dir,
                            int local_time, int print_stdout, lg_log_policy policy, log_formatter_t log_formatter);

/*
  Destroys specific logger instance and closes the log file that the instance working on
  if instance = NULL, it tries to destroy active one
  active_instance WONT become NULL
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
LOGGERDEF int lg_producer(Logger* inst, const lg_log_level level, const char* msg);

/*
  This is the format resolver for lg_producer.
  C macros use this. It's sensitive to "%" char.
  It may crash your app if you're using this on FFI or wrongly on C
*/
LOGGERDEF int lg_vproducer(Logger* inst, const lg_log_level level, const char* fmt, ...);

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
  Manipulates lg_log_level enum into string-literals (const char*)
*/
LOGGERDEF const char* lg_lvl_to_str(const lg_log_level level);

/*
  Simply acts like snprintf and applies it to the string data
*/
LOGGERDEF void lg_str_format_into(lg_string* s, const char* fmt, ...);

/*
  lg_str_format_into but already formatted strings
  Can be used at FFIs - because you cannot use variadics in ffi
*/
LOGGERDEF void lg_str_write_into(lg_string* s, const char* already_formatted_str);

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

#define lg_infoi(instance, fmt, ...) lg_logi(instance, LG_INFO, fmt, ##__VA_ARGS__)
#define lg_errori(instance, fmt, ...) lg_logi(instance, LG_ERROR, fmt, ##__VA_ARGS__)
#define lg_warni(instance, fmt, ...) lg_logi(instance, LG_WARNING, fmt, ##__VA_ARGS__)

// log functions to be going to used is macros now
// main function is lg_vproducer
#define lg_log(level, fmt, ...) \
  lg_logi(lg_get_active_instance(), level, fmt, ##__VA_ARGS__)

#define lg_info(fmt, ...) lg_log(LG_INFO, fmt, ##__VA_ARGS__)
#define lg_error(fmt, ...) lg_log(LG_ERROR, fmt, ##__VA_ARGS__)
#define lg_warn(fmt, ...) lg_log(LG_WARNING, fmt, ##__VA_ARGS__)

// you can add your custom level like this:
#define lg_custom(fmt, ...) lg_log(LG_CUSTOM, fmt, ...)

// minify prefix from lg_ to l
#ifdef LOGGER_MINIFY_PREFIXES
#define llog    lg_log
#define linfo   lg_info
#define lwarn   lg_warn
#define lerror  lg_error
#define INFO    LG_INFO
#define ERROR   LG_ERROR
#define WARNING LG_WARNING
#endif

// logger max message size (you can change it)
#define LOGGER_MAX_MSG_SIZE 1024
// ring buffer's size
#define LOGGER_RING_SIZE 1024

// log entry struct to be used at ring buffer
typedef struct {
  // main, plain message (resolved the printf format)
  char msg[LOGGER_MAX_MSG_SIZE];
  size_t length;
  
  char time_str[24];
  lg_log_level level;
} log_entry_t;

// the ANSI console color codes, these are escape chars that works on UNIX
#define CLR_RED "\x1b[31m" // for ERRORs
#define CLR_GREEN "\x1b[32m" // for INFOs
#define CLR_YELLOW "\x1b[33m" // for WARNs
#define CLR_AQUA "\x1b[36m" // for TIMESTAMPs
#define CLR_RST "\x1b[0m" // for plain message

// stb style implementation macros
#ifdef LOGGER_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

// Static function forward-declerations
/*
  Gets time using kernel in this format: %Y.%m.%d-%H.%M.%S.%MS (%MS is millis in 3 digits)
  or YYYY.MM.DD-HH.MM.SS.SSS
  example: 2026.01.11-21.44.40.255 means January 11th, 2026, 21:44:40 or 9:44:40 pm, ms: 255
  If isLocalTime = false, spits out time in UTC format.
  Accepts buffer to write and its size
*/
static bool get_time_str(char* buf, size_t size, bool isLocalTime);

// checks if dir is a valid directory (exists and directory)
static int check_dir(const char* path);

// recursively creates folders (behaves like mkdir -p)
static bool mkdir_p(const char *path);

// default and custom formatter distinguisher, used at consumer
static void format_msg(const char* time_str, const lg_log_level level,
                      const char* msg, lg_msg_pack* pack);

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
  #define PATH_MAX MAX_PATH * 2 // for wchar_t

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

  // global mutex to prevent race conditions
  pthread_mutex_t mtx;
  pthread_mutex_t wmtx;

  // writer thread mutex and cond
  pthread_cond_t wcond;
  pthread_cond_t pcond;
  pthread_t writer_tid;

  // ring buffer and its cursors
  log_entry_t ring[LOGGER_RING_SIZE]; // ring buffer
  size_t pcurr; // producer cursor
  size_t wcurr; // writer cursor
};

// consumer func, writes entries on the ring to stdout or file
static void* lg_consumer(void* arg) {
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
        if (custom_fn) custom_fn(slot->time_str, slot->level, slot->msg, &pack);
        else format_msg(slot->time_str, slot->level, slot->msg, &pack);

        fwrite(pack.file_str.data, 1, pack.file_str.len, inst->logFile); // file
        if (is_stdout) {
          fwrite(pack.stdout_str.data, 1, pack.stdout_str.len, stdout); // stdout
        }
        slot->length = 0;
      }
      w += 1;
    }
    inst->wcurr = w;

    // if we dont have any thing to do and logger is dead, so break the main loop
    // if logger is destructed and we have work to do, first we finishing our job and die
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
                 int local_time, int print_stdout, lg_log_policy policy, log_formatter_t log_formatter) {
  LoggerConfig cfg;
  cfg.localTime = local_time;
  cfg.printStdout = print_stdout;
  cfg.policy = policy;
  cfg.logFormatter = log_formatter;
  return lg_init(inst, logs_dir, cfg);
}

// main init func
int lg_init(Logger* inst, const char* logs_dir, LoggerConfig config) {
  if (!inst || !logs_dir) return 0;
  pthread_mutex_init(&inst->mtx, NULL);

  inst->isPrintStdout = config.printStdout != 0;
  inst->isLocalTime = config.localTime != 0;
  inst->customLogFunc = config.logFormatter;
  inst->logPolicy = config.policy;

  inst->pcurr = 0;
  inst->wcurr = 0;

  int dir_status = check_dir(logs_dir); // -1 = NOT valid directory, 0 = NOT exists

  // handle not a valid directory
  if (dir_status == -1) {
    LG_DEBUG_ERR("Provided path is not a valid directory to create: %s", logs_dir);
    return 0;
  }

  // handle just non-exist directory (best effort)
  if (dir_status == 0) {
    if (!mkdir_p(logs_dir)) {
      LG_DEBUG_ERR("Cannot create provided path: %s", logs_dir);
      return 0;
    }
  }

  // get time str and length
  char time_str[24];
  if (!get_time_str(time_str, sizeof(time_str), config.localTime != 0))
    return 0;

  // produce file path with a fixed size (removed that complex path normalization and alloca)
  char file_path[PATH_MAX];
  int n = snprintf(file_path, sizeof(file_path),
                   "%s%c%s.log", logs_dir, PATH_SEP, time_str);  
  if (n <= 0 || (size_t)n >= sizeof(file_path)) return 0;
  
  // open file in write binary mode
  inst->logFile = fopen(file_path, "wb");
  if (!inst->logFile) {
    LG_DEBUG_ERR("Cannot open the log file: %s", file_path);
    return 0;
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
    return 0;
  }

  if (active_instance == NULL) active_instance = inst;
  
  return 1;
}

// main log function with variadics - used at macros
// gets resolved message and calls lg_producer
int lg_vproducer(Logger* inst, const lg_log_level level, const char* fmt, ...) {
  if (!fmt) return 0;

  // variadic resolving
  va_list args;
  va_start(args, fmt);
  char msg[LOGGER_MAX_MSG_SIZE]; // we allocate enough memory
  int mn = vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  if (mn < 0) {
    LG_DEBUG_ERR("Cannot resolve print format");
    return 0;
  }

  return lg_producer(inst, level, msg);
}

// main log function, invokes the writer - used at FFIs
// accepts format-resolved message to print
int lg_producer(Logger* inst, const lg_log_level level, const char* msg) {
  if (!msg) return 0;

  if (!inst) {
    if (active_instance) inst = active_instance;
    else {
      LG_DEBUG_ERR("There is no active instance!");
      return 0;
    }
  }

  // doesnt log when its not alive
  pthread_mutex_lock(&inst->mtx);
  if (!inst->isAlive) {
    LG_DEBUG_ERR("Cannot log because the instance is dead!");
    return 0;
  }
  pthread_mutex_unlock(&inst->mtx);

  // getting time str
  // it is not in mutex lock bcs this func is a bit chunky
  char time_str[24];
  if (!get_time_str(time_str, sizeof(time_str), inst->isLocalTime))
    return 0;
  
  pthread_mutex_lock(&inst->mtx);
  // unwrap producer and writer cursor and compute next index
  size_t p = inst->pcurr;
  size_t w = inst->wcurr;
  size_t next = p % LOGGER_RING_SIZE;

  // slot overflow check
  if (p - w >= LOGGER_RING_SIZE) {
    // buffer full, policies: DROP (default), OVERWRITE
    switch (inst->logPolicy) {
    case LG_DROP: goto drop;
    case LG_SMASH_OLDEST: goto smash_oldest;
    default: LG_DEBUG_ERR("Unkown log policy, defaulting to drop"); goto drop;
    }

  drop:
    LG_DEBUG_ERR("Ring buffer is full, dropping the log");
    pthread_mutex_unlock(&inst->mtx);
    return 0;

  smash_oldest:
    inst->wcurr += 1;
    LG_DEBUG_ERR("Ring buffer full, overwriting oldest entry");
    goto policy_done;
  }
policy_done:
  pthread_mutex_unlock(&inst->mtx);

  // get msg length with strlen
  size_t msglen = strlen(msg);
  if (msglen > LOGGER_MAX_MSG_SIZE)
    return 0;

  pthread_mutex_lock(&inst->mtx);
  // copy message and put zero on ring
  memcpy(inst->ring[next].msg, msg, msglen);
  inst->ring[next].msg[msglen] = '\0';
  // copy time string (already zero guarenteed)
  memcpy(inst->ring[next].time_str, time_str, sizeof(time_str));
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

  return 1;
}

int lg_set_active_instance(Logger* inst) {
  if (!inst) return 0;
  active_instance = inst;
  return 1;
}

Logger* lg_get_active_instance() {
  return active_instance;
}

int lg_destroy(Logger* inst) {
  if (!inst) {
    if (active_instance) inst = active_instance;
    else return 0;
  }

  pthread_mutex_lock(&inst->mtx);
  pthread_mutex_lock(&inst->wmtx);
  // if it is not alive, do not try to destruct
  if (!inst->isAlive) {
    LG_DEBUG_ERR("Logger is already dead!");
    pthread_mutex_unlock(&inst->wmtx);
    pthread_mutex_unlock(&inst->mtx);
    return 0;
  }
  inst->isAlive = false;
  
  // invoke the writer thread and it'll see its being destructed
  pthread_cond_signal(&inst->wcond);
  pthread_mutex_unlock(&inst->wmtx);
  
  pthread_join(inst->writer_tid, NULL); // wait the thread to clear

  // clear config after thread exit
  inst->isPrintStdout = false;
  inst->isLocalTime = false;
  
  // close the file if its not closed
  if (inst->logFile != NULL) {
    if (fclose(inst->logFile) != 0) {
      LG_DEBUG_ERR("Log file cannot be closed!");
      pthread_mutex_unlock(&inst->mtx);
      return 0;
    }
    inst->logFile = NULL;
  }

  // consumer things destroy
  pthread_mutex_destroy(&inst->wmtx);
  pthread_cond_destroy(&inst->wcond);
  pthread_cond_destroy(&inst->pcond);
  
  pthread_mutex_unlock(&inst->mtx);
  pthread_mutex_destroy(&inst->mtx);
  return 1;
}

int lg_is_alive(const Logger* inst) {
  if (!inst) {
    Logger* ins = lg_get_active_instance();
    return ins ? ins->isAlive : 0;
  }
  return inst->isAlive;
}

Logger* lg_alloc() {
  Logger* tmp = (Logger*)calloc(1, sizeof(Logger));
  if (!tmp) {
    LG_DEBUG_ERR("Cannot allocate Logger instance!");
    return NULL;
  }
  return tmp;
}
void lg_free(Logger* inst) {
  free(inst);
}

int lg_flogi(Logger* inst, const lg_log_level level, const char* msg) {
  if (!msg) return 0;
  if (!inst) return lg_producer(active_instance, level, msg);
  return lg_producer(inst, level, msg);
}

// Explicit instances
int lg_finfoi(Logger* inst, const char* msg) {
  return lg_flogi(inst, LG_INFO, msg);
}
int lg_ferrori(Logger* inst, const char* msg) {
  return lg_flogi(inst, LG_ERROR, msg);
}
int lg_fwarni(Logger* inst, const char* msg) {
  return lg_flogi(inst, LG_WARNING, msg);
}

// Implicit instances
int lg_flog(const lg_log_level level, const char* msg) {
  return lg_flogi(NULL, level, msg);
}
int lg_finfo(const char* msg) {
  return lg_flogi(NULL, LG_INFO, msg);
}
int lg_ferror(const char* msg) {
  return lg_flogi(NULL, LG_ERROR, msg);
}
int lg_fwarn(const char* msg) {
  return lg_flogi(NULL, LG_WARNING, msg);
}

const char* lg_lvl_to_str(const lg_log_level level) {
  switch (level) {
  case LG_INFO: return "INFO";
  case LG_ERROR: return "ERROR";
  case LG_WARNING: return "WARNING";
  case LG_CUSTOM: return "CUSTOM";
  default: return "INFO";
  }
}

static bool get_time_str(char* buf, size_t size, bool isLocalTime) {
  if (!buf || size == 0) return false;

#ifdef _WIN32
  SYSTEMTIME st;
  if (isLocalTime) GetLocalTime(&st);
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
  if (isLocalTime) localtime_r(&ts.tv_sec, &tm_val);
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

void lg_str_format_into(lg_string* s, const char* fmt, ...) {
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
      s->data[cap - 2] = '\n'; // guarentees \n at the end
      s->data[cap - 1] = '\0';
      s->len = cap - 1;
    } else {
      s->len = 0;
    }
  } else {
    s->len = (size_t)n;
  }
}

void lg_str_write_into(lg_string* s, const char* str) {
  if (!str) return;
  return lg_str_format_into(s, "%s", str);
}

// message formatter helper - used at consumer
static void format_msg(const char* time_str, const lg_log_level level,
                      const char* msg, lg_msg_pack* pack) {
  // prepairing stdout msg
  if (pack->stdout_str.data != NULL && pack->stdout_str.cap != 0) {
    const char *clr;
    switch (level) {
    case LG_ERROR: clr = CLR_RED; break;
    case LG_INFO: clr = CLR_GREEN; break;
    case LG_WARNING: clr = CLR_YELLOW; break;
    default: clr = CLR_RST; break;
    }
      
    lg_str_format_into(
      &pack->stdout_str,
      CLR_AQUA "%s %s[%s]" CLR_RST " %s\n",
      time_str, clr, lg_lvl_to_str(level), msg
      );
  }

  // prepairing file msg
  if (pack->file_str.data != NULL && pack->file_str.cap != 0) {
    // no escape chars in file
    lg_str_format_into(
      &pack->file_str,
      "%s [%s] %s\n",
      time_str, lg_lvl_to_str(level), msg
      );
  }
}

#endif // LOGGER_IMPLEMENTATION

#endif // LOGGER_H
