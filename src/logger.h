#ifndef LOGGER_API_H
#define LOGGER_API_H

// THIS FILE IS THE DOOR BETWEEN LIBRARY AND YOUR PROGRAM
// YOU HAVE TO INCLUDE THIS FILE
// IF YOU'RE USING C++, YOU CAN ALSO INCLUDE LOGGERSTREAM TO USE "<<" FOR LOGGING (C++ EXCLUSIVE)

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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	LOGGER_API int lg_init(const char* logs_dir,
													int use_local_time, int should_throw_error);
	LOGGER_API int lg_log(const char* msg, const char* level);
	LOGGER_API int lg_info(const char* msg);
	LOGGER_API int lg_error(const char* msg);
	LOGGER_API int lg_warn(const char* msg);
	LOGGER_API int lg_destruct(void);

	// make strip macros doesnt be duplicated
#ifndef LOGGER_PREFIX_MODIFIERS_GUARD
#define LOGGER_PREFIX_MODIFIERS_GUARD

// strips ONLY log functions
#ifdef LOGGER_STRIP_PREFIXES
  #define log   lg_log
  #define info  lg_info
  #define warn  lg_warn
  #define error lg_error
#endif // LOGGER_STRIP_PREFIXES

// minify prefix from lg_ to l
// DO NOT use logger stream if you enable this in C++ (it'll collide)
#ifdef LOGGER_MINIFY_PREFIXES
  #define llog   lg_log
  #define linfo  lg_info
  #define lwarn  lg_warn
  #define lerror lg_error
#endif // LOGGER_MINIFY_PREFIXES

#endif // LOGGER_PREFIX_MODIFIERS_GUARD

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LOGGER_API_H
