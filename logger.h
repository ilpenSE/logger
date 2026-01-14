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
	 lg_init();
	 lg_info("Hello, World!");
	 info("Hello, World!");
	 linfo("Hello, World!");
	 lg_destruct();
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

// the logger function results via enums
typedef enum {
	LG_FATAL_ERROR = -1, // if this happens, destruct immediatel
	LG_RUNTIME_ERROR = 0, // can be thrown in runtime, not fatal to crash program
	LG_SUCCESS = 1,
	LG_NOT_VALID_DIR = 2,
	LG_NOT_OPEN_FILE = 3
} lg_result_t;

/**
	 Main initializer function
	 @param logs_dir: const char*, Points a directory path
	   (if not exists, it'll try to create). It can be relative or absolute path.
	 @param use_local_time: int, Changes behavior of lg_get_time(). If 1 or true, that function
	   will try to generate time string BY YOUR LOCAL TIME ZONE. If it is false or 0, it'll use
		 UTC as a convention.
	 @returns if initializition succeed, if it's not, none of the log functions will work.
	   RECOMMENDED: on your app's entry point, check its return value like if(!lg_init(...))
		 You dont have to use lg_destruct() if init failed. Because if init failed, is_alive set to be 0
		 and lg_destruct() simply wont work if logger is sleeping
*/
LOGGER_API lg_result_t lg_init(const char* logs_dir, int use_local_time);

/**
	 MAY SPAWN RACE CONDITIONS IF YOU DONT KNOW WHAT YOU'RE DOING!
	 (multiple loggers races about is_alive thingy)
	 DOES NOT RECOMMENDED TO USE, USE lg_init
	 Main initializer function but takes extra "is_forced" parameter which determines
	 that you have forcefully initialize a logger instance at all costs.
	 Normally, if it is set 0, checks that a logger instance is running (is_alive == 1)
	 if an instance running, it DOES NOT try to CREATE NEW INSTANCE.
	 But if you set is_forced to be 1, it doesnt give a fuck about is_alive or any running instances.
*/
LOGGER_API lg_result_t lg_init_(const char* logs_dir, int use_local_time, int is_forced);

/**
	 Forcefully initialize goddamn logger. Wrapper of lg_init_. return lg_init_(..., 1);
 */
LOGGER_API lg_result_t lg_force_init(const char* logs_dir, int use_local_time);

/**
	 Destructs logger instance and closes the log file that the instance working on
*/
LOGGER_API lg_result_t lg_destruct(void);

// returns if logger is alive
LOGGER_API int lg_is_alive();

/**
	 Gets time using kernel in this format: %Y.%m.%d-%H.%M.%S.%MS (%MS is millis in 3 digits)
	 or YYYY.MM.DD-HH.MM.SS.SSS
	 example: 2026.01.11-21.44.40.255 means January 11th, 2026, 21:44:40 or 9:44:40 pm, ms: 255
	 If lg_isLocalTime = 0, spits out time in UTC format.
*/
LOGGER_API char* lg_get_time();

/**
	 LOG FUNCTIONS, if you have EYES and a BRAIN you can read lg_xxx part and
	 see that which function log in which level.
	 The "lg_log" requires level if you have special levels
	 lg_xxx (except log) is wrapper of lg_log
 */
LOGGER_API lg_result_t lg_log(const char* msg, const char* level);
LOGGER_API lg_result_t lg_info(const char* msg);
LOGGER_API lg_result_t lg_warn(const char* msg);
LOGGER_API lg_result_t lg_error(const char* msg);

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

static int is_alive = 0;
static int lg_isLocalTime = 0;
static FILE* lg_logFile;

// by default, it is not forced to init
lg_result_t lg_init(const char* logs_dir, int use_local_time) {
	return lg_init_(logs_dir, use_local_time, 0);
}

// but you can still force to init when something might weird happen
lg_result_t lg_force_init(const char* logs_dir, int use_local_time) {
	return lg_init_(logs_dir, use_local_time, 1);
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
lg_result_t lg_init_(const char* logs_dir, int use_local_time, int is_forced) {
	if (is_alive && !is_forced) return LG_SUCCESS;
	is_alive = 1;
	lg_isLocalTime = use_local_time;

	int dir_status = check_dir(logs_dir); // -1 = NOT valid directory, 0 = NOT exists

	if (dir_status == -1) {
		LG_DEBUG_ERR("Provided path is not a valid directory to create: %s", logs_dir);
		return LG_NOT_VALID_DIR;
	}

	// check if logs_dir exists, if it is not, try to create (best effort)
	if (dir_status == 0) {
		if (!mkdir_p(logs_dir)) {
		  LG_DEBUG_ERR("Cannot create provided path: %s", logs_dir);
			return LG_NOT_VALID_DIR;
		}
	}

	// get time str and length
	char* time_str = lg_get_time(); // eg: "2026.01.11-12.34.56.203"
	size_t len = strlen(logs_dir) + 1 + strlen(time_str) + 4 + 1; 
  // +1 -> possible '/'
  // +4 -> ".log"
  // +1 -> '\0'
	char file_path[len];
	
	// normalize logs dir name (add trailing slash)
	// always remind: logs dir must be like this: "/home/ilpen/myapp/logs/" or "./logs/" or "logs/"
	// it is full, absolute path, ended up with "/"
	// defining format string that to be used at snprintf
	char last_elem = logs_dir[strlen(logs_dir) - 1];
	if (last_elem == '/' || last_elem == '\\') {
    snprintf(file_path, len, "%s%s.log", logs_dir, time_str);
	} else {
    snprintf(file_path, len, "%s/%s.log", logs_dir, time_str);
	}

	lg_logFile = fopen(file_path, "w");
	if (!lg_logFile) {
		LG_DEBUG_ERR("Cannot open the log file: %s", file_path);
		return LG_NOT_OPEN_FILE;
	}
	return LG_SUCCESS;
}

lg_result_t lg_destruct(void) {
	// if it is not alive, do not try to destruct
	if (is_alive == 0) return LG_RUNTIME_ERROR;
	
	// close the file if its not closed
	if (lg_logFile != NULL) {
		if (fclose(lg_logFile) != 0) {
			LG_DEBUG_ERR("Log file cannot be closed!");
			return LG_FATAL_ERROR;
		}
		lg_logFile = NULL; // clear ptr
	}
	
	is_alive = 0;
	return LG_SUCCESS;
}

lg_result_t lg_log(const char* msg, const char* level) {
	// if it is not alive or file is destroyed, do not log
	if (is_alive != 1 || lg_logFile == NULL) return LG_RUNTIME_ERROR;

	char* time_str = lg_get_time();
	char fixed[strlen(time_str) + strlen(level) + strlen(msg) + 6];
	// TIME_LENGTH [LEVEL_LENGTH] MSG_LENGTH
	// other stuff with \0: 6
	snprintf(fixed, sizeof(fixed), "%s [%s] %s\n", time_str, level, msg);

	fprintf(lg_logFile, "%s", fixed);
	printf("%s", fixed);
	return LG_SUCCESS;
}

lg_result_t lg_info(const char* msg) {
	return lg_log(msg, "INFO");
}

lg_result_t lg_error(const char* msg) {
	return lg_log(msg, "ERROR");
}

lg_result_t lg_warn(const char* msg) {
	return lg_log(msg, "WARNING");
}

int lg_is_alive() { return is_alive; }

// cross-platform milisecond getter
static long get_millis(int is_local) {
#ifdef _WIN32
	SYSTEMTIME st;
	if (is_local) GetLocalTime(&st);
	else GetSystemTime(&st);
	return st.wMilliseconds;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec / 1000; // microsecond -> millisecond
#endif // _WIN32
}

char* lg_get_time() {
	size_t len = 94; // fmt string has 23 fixed size and ofc \0
	                 // but we add extra +70 byte for security reasons
	char* buf = (char*)malloc(len * sizeof(char));
	if (!buf) return NULL; // buy more ram if you hit here
	
	time_t t = time(NULL);
	struct tm tm_val;

#ifdef _WIN32
	if (lg_isLocalTime) localtime_s(&tm_val, &t);
	else gmtime_s(&tm_val, &t);
#else
	if (lg_isLocalTime) localtime_r(&t, &tm_val);
	else gmtime_r(&t, &tm_val);
#endif // _WIN32

	long ms = get_millis(lg_isLocalTime);

	snprintf(buf, len, "%04d.%02d.%02d-%02d.%02d.%02d.%03ld",
					 tm_val.tm_year + 1900,
					 tm_val.tm_mon + 1,
					 tm_val.tm_mday,
					 tm_val.tm_hour,
					 tm_val.tm_min,
					 tm_val.tm_sec,
					 ms
		);
	return buf;
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
