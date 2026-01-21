#ifndef LOGGER_H
#define LOGGER_H

/**
	 THIS IS STB-STYLE LIBRARY HEADER OF LOGGER. IT IS PURELY C.
	 YOU CAN USE IT LIKE A TRUE STB STYLE HEADER.

	 USAGE OF STB STYLE HEADER:
	 #define LOGGER_IMPLEMENTATION -> this macro enables the implementation of logger.
	 #define LOGGER_STRIP_PREFIXES -> completely removes "lg_" prefix (except lg_log)
	 #define LOGGER_MINIFY_PREFIXES -> minifies "lg_" into "l", like linfo instead of lg_info
	 #include "logger.h"

	 int main() {
	 if (lg_init("logs", {.localTime=1, .logFormatter=NULL}) != 1) return 1;
	 lg_info("Hello, World!");
	 info("Hello, World!");
	 linfo("Hello, World!");
	 if (lg_destruct() != 1) return 1;
	 }

	 AND IF YOU'RE USING IMPLEMENTATION MACRO, YOU DONT HAVE TO DYANAMIC LINK THE LIBRARY
	 BUT YOU HAVE TO USE C TYPES AND FUNCTIONS AGAIN.
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
	LG_NOT_OPEN_FILE = 3
} lg_result_t;

/*
 * Config struct, this struct can be used in lg_init.
 * localTime: this members also known as "use_local_time", if it is 1, time is calculated on local timezone
 * logFormatter: this function describes the logging format. By default, it is like:
     time level msg
		 its parameters: time str, level, formatted message, out buffer, out buffer size
		 returns: status code (1 = success)
		 btw: you are given by time str, level and formatted message (fmt + va_list'ed)
		 you can customize it and you have to write your message into out buffer the size of buffer is size
*/
typedef struct {
	int localTime;
	int (*logFormatter)(const char*, const char*, const char*, char*, size_t);
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
	 logFormatter: Custom log message formatter function, you are given by time_str that comes from get_time_str, level and formatted message AND output buffer and output buffer's size. Your job is to implement and give this function if you'll use customization and write your message in snprintf. You can make this NULL and it uses default formatting (time_str [level] msg)
	 @returns if initializition succeed, if it's not, none of the log functions will work.
	 RECOMMENDED: on your app's entry point, check its return value like if(!lg_init(...))
	 You dont have to use lg_destruct() if init failed. Because if init failed, is_alive set to be 0
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
*/
LOGGER_API lg_result_t lg_vlog(const char* level, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

// log functions to be going to used is macros now
// main function is lg_vlog
#define lg_log(level, fmt, ...) \
	lg_vlog(level, fmt, ##__VA_ARGS__)

#define lg_info(fmt, ...) \
	lg_vlog("INFO", fmt, ##__VA_ARGS__)

#define lg_error(fmt, ...) \
  lg_vlog("ERROR", fmt, ##__VA_ARGS__)

#define lg_warn(fmt, ...) \
  lg_vlog("WARNING", fmt, ##__VA_ARGS__)

// you can add your custom level like this:
#define lg_custom(fmt, ...) \
	lg_vlog("CUSTOM", fmt, ##__VA_ARGS__)

// logger max message size (you can change it)
// this is used in custom formatting
#define LOGGER_MAX_MSG_SIZE 1024

// stb style implementation macros
#ifdef LOGGER_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

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
		fprintf(stderr, "%s:%d: logger runtime error: " fmt "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__); \
	} while (0)

// lgiprint - auto adds \n at the end
#define lgiprint(fmt, ...) \
	do { \
		printf(fmt"\n", ##__VA_ARGS__);	\
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

static int isAlive = 0;
static int isLocalTime = 0;
static FILE* logFile = NULL;
static int (*customLogFunc)(const char*, const char*,
														const char*, char*, size_t) = NULL;

/*
	Gets time using kernel in this format: %Y.%m.%d-%H.%M.%S.%MS (%MS is millis in 3 digits)
	or YYYY.MM.DD-HH.MM.SS.SSS
	example: 2026.01.11-21.44.40.255 means January 11th, 2026, 21:44:40 or 9:44:40 pm, ms: 255
	If lg_isLocalTime = 0, spits out time in UTC format.
	Accepts buffer to write and its size
*/
static lg_result_t get_time_str(char* buf, size_t size) {
	if (!buf || size == 0) return LG_RUNTIME_ERROR;

#ifdef _WIN32
	SYSTEMTIME st;
	if (isLocalTime) GetLocalTime(&st);
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

// main init with is_force
lg_result_t lg_init(const char* logs_dir, LoggerConfig config) {
	if (isAlive) return LG_SUCCESS;
	isLocalTime = config.localTime;
	customLogFunc = config.logFormatter;
	
	int dir_status = check_dir(logs_dir); // -1 = NOT valid directory, 0 = NOT exists

	// handle not a valid directory
	if (dir_status == -1) {
		LG_DEBUG_ERR("Provided path is not a valid directory to create: %s", logs_dir);
		return LG_NOT_VALID_DIR;
	}

	// handle just non-exist directory (best effort)
	if (dir_status == 0) {
		if (!mkdir_p(logs_dir)) {
		  LG_DEBUG_ERR("Cannot create provided path: %s", logs_dir);
			return LG_NOT_VALID_DIR;
		}
	}

	// get time str and length
	char time_str[24];
	if (get_time_str(time_str, sizeof(time_str)) != 1) return LG_FATAL_ERROR;

	size_t len = strlen(logs_dir) + 29;
  // +1 -> possible '/'
  // +4 -> ".log"
  // +1 -> '\0'
	// +23 -> time_str's length = strlen(time_str)
	char file_path[len];
	
	// normalize logs dir name (add trailing slash)
	// always remind: logs dir must be like this: "/home/ilpen/myapp/logs/" or "./logs/" or "logs/"
	// it is full, absolute path, ended up with "/"
	char last_elem = logs_dir[strlen(logs_dir) - 1];
	int n = 0;
	if (last_elem == '/' || last_elem == '\\') {
    n = snprintf(file_path, len, "%s%s.log", logs_dir, time_str);
	} else {
    n = snprintf(file_path, len, "%s/%s.log", logs_dir, time_str);
	}
	// if file name shitted
	if (n <= 0 || (size_t)n >= len) return LG_FATAL_ERROR;

	// open file in write mode
	logFile = fopen(file_path, "w");
	if (!logFile) {
		LG_DEBUG_ERR("Cannot open the log file: %s", file_path);
		return LG_NOT_OPEN_FILE;
	}
	
	isAlive = 1;
	return LG_SUCCESS;
}

// main log function with write and prettify
lg_result_t lg_vlog(const char* level, const char* fmt, ...) {
	// doesnt log when it is destructed or some weird states
	if (isAlive != 1 || logFile == NULL) return LG_RUNTIME_ERROR;

	// variadic processing
	va_list args;
	va_start(args, fmt);
	char msg[1024]; // we allocate enough memory
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);
	
	char time_str[24];
	if (get_time_str(time_str, sizeof(time_str)) != 1) return LG_RUNTIME_ERROR;

  // log message customization starts here
	if (customLogFunc == NULL) {
		int len = strlen(level) + strlen(msg) + 29;
		char fixed[len];
		
		int n = snprintf(fixed, sizeof(fixed), "%s [%s] %s\n", time_str, level, msg);
		if (n < 0 || n >= len)
			return LG_RUNTIME_ERROR;  // buffer too smol

		printf("%s", fixed);
		fprintf(logFile, "%s", fixed);
	} else {
		// if you use custom func, you are limited to fixed save due to some "C" features
		char buf[LOGGER_MAX_MSG_SIZE];
		int res = customLogFunc(time_str, level, msg, buf, sizeof(buf));
		if (res != 1) return LG_RUNTIME_ERROR;
		printf("%s", buf);
		fprintf(logFile, "%s", buf);
	}
	return LG_SUCCESS;
}

int lg_is_alive() { return isAlive; }

lg_result_t lg_destruct(void) {
	// if it is not alive, do not try to destruct
	if (isAlive == 0) return LG_RUNTIME_ERROR;
	
	// close the file if its not closed
	if (logFile != NULL) {
		if (fclose(logFile) != 0) {
			LG_DEBUG_ERR("Log file cannot be closed!");
			return LG_FATAL_ERROR;
		}
		logFile = NULL; // clear ptr
	}
	
	isAlive = 0;
	return LG_SUCCESS;
}


#ifndef LOGGER_PREFIX_MODIFIERS_GUARD
#define LOGGER_PREFIX_MODIFIERS_GUARD

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

#endif // LOGGER_PREFIX_MODIFIERS_GUARD

#endif // LOGGER_IMPLEMENTATION

#endif // LOGGER_H
