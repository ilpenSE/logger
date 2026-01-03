#ifndef LOGGER_API_H
#define LOGGER_API_H

// THIS FILE IS THE DOOR BETWEEN LIBRARY AND YOUR PROGRAM
// YOU HAVE TO INCLUDE THIS FILE
// IF YOU'RE USING C++, YOU CAN ALSO INCLUDE LOGGERSTREAM TO USE "<<" FOR LOGGING (C++ EXCLUSIVE)

// for windows compatibility
#if defined(_WIN32)
  #if defined(LOGGER_BUILD)
    #define LOGGER_API __declspec(dllexport)
  #else
    #define LOGGER_API __declspec(dllimport)
  #endif
#else
  #define LOGGER_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

LOGGER_API int  lg_init(const char* logs_dir,
    int use_local_time, int should_throw_error);
LOGGER_API int  lg_info(const char* msg);
LOGGER_API int  lg_error(const char* msg);
LOGGER_API int  lg_warn(const char* msg);
LOGGER_API int  lg_destruct(void);

#ifdef __cplusplus
}
#endif

#endif
