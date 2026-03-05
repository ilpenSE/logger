# The Logger Library

- ![Language](https://img.shields.io/badge/language-C-blue)
![Language](https://img.shields.io/badge/safe%20in-C++-blue)
- ![Platform](https://img.shields.io/badge/platform-all-brightgreen)
- ![Asynchronous](https://img.shields.io/badge/asynchronous-purple?style=for-the-badge)
![Thread Safe](https://img.shields.io/badge/thread%20safe-brightgreen?style=for-the-badge)
![Flexible](https://img.shields.io/badge/flexible-blue?style=for-the-badge)
![STB-Style](https://img.shields.io/badge/stb-single%20header-yellow?style=for-the-badge)

- This Logger supports apple systems, windows and unix or unix-like systems.
- It requires minimum C99 version of C.
- You can build and generate `.so`, `.dll` and `.dylib` on your machine.

- It does only have header files with implementation,
but if you define `LOGGER_IMPLEMENTATION` during compilation of header, you can get shared object.
- It is written in purely C. Contains C++ stuff for extra options (not included in logger.h, logger.h is pure C)
- Since it is written in C, you can port this library every language supports C FFI (most of them).
You can compile this single header and turn it into shared object file.

## Quick Links

- [Pure C STB-Style Header](./logger.h)
- [C++ Stream (<<) support](./loggerstream.hpp)
- [Usage in C](usage/c)
- [Usage in C++](usage/c++)
- [Usage in Python](usage/python)
- [Usage in Rust](usage/rust)
- [Header-only in C++, exported via C ABI (Deprecated)](https://github.com/ilpenSE/logger/tree/deprecated)

# Library Linking/Usage

- You can use this as a true stb-style single-header library like this:
```c
#define LOGGER_IMPLEMENTATION
#include "logger.h"
#include <stdbool.h>

int main() {
  Logger* lg = lg_alloc();
  LoggerConfig config = {
    .localTime = true, .printStdout = true,
    .policy = LG_DROP, .logFormatter = NULL
  };
  lg_init(lg, "logs", config);

  lg_info("Hello, World!");

  lg_destroy(lg);
  lg_free(lg);
}
```

- Compile the header (`logger.h`) and use in any language that supports C FFI. (C, Rust, C++ etc.)
- You can generate shared objects with these commands:

(These commands generate shared objects at build folder)

For GCC and UNIX:
```bash
make
```
or
```bash
make debug=1
```
if you wanna have debug information

For MSVC and WINDOWS:
```bash
build.bat
```
or
```bash
build.bat debug
```
for debug information

# API Documentation
- Main initializer function that you may use in C/C++:

`int lg_init(Logger* instance, const char* logs_dir, LoggerConfig config);`

- The LoggerConfig struct:
```c
typedef struct {
  int localTime;
  int printStdout;
  lg_log_policy policy;
  log_formatter_t logFormatter;
} LoggerConfig;
```

- Flatted version of LoggerConfig
- It's for languages that doesnt support C structs (e.g: Bun FFI in JavaScript)

```c
int lg_init_flat(Logger* inst, const char* logs_dir,
                  int local_time, int print_stdout,
                  lg_log_policy policy, log_formatter_t log_formatter);
```

- Main destroyer function that destroys logger instances (DOES NOT MANAGE MEMORY!)

`int lg_destroy(Logger* instance);`

- Check if specific instance is dead or alive (if instance = NULL, checks active instance)

`int lg_is_alive(const Logger* instance);`

- Main producer function that puts message, time string and level into the ring
- (NOT RECOMMENDED TO USE DIRECTLY, USE MACROS OR F-FUNCTIONS)

`int lg_producer(Logger* inst, const lg_log_level level, const char* msg);`

- Wrapper for producer, takes variadics and processes it, used at macros

`int lg_vproducer(Logger* inst, const lg_log_level level, const char* fmt, ...);`

- Functions that are used at FFIs (F-functions), the level-less and level-aware functions here:

`int lg_flog(const lg_log_level level, const char* msg);`

`int lg_finfo(const char* msg);`

`int lg_fwarn(const char* msg);`

`int lg_ferror(const char* msg);`

- F-functions with explicit instance, you can put NULL there if you wanna use active instance
- (thats how functions above works)

`int lg_flogi(Logger* inst, const lg_log_level level, const char* msg);`

`int lg_finfoi(Logger* inst, const char* msg);`

`int lg_ferrori(Logger* inst, const char* msg);`

`int lg_fwarni(Logger* inst, const char* msg);`

- Getter and setter for active instance
- (lg_init automatically sets active instance if it's NULL)

`int lg_set_active_instance(Logger* inst);`

`Logger* lg_get_active_instance();`

## Helper functions that you can use:
- Converts level enum to string

`const char* lg_lvl_to_str(const lg_log_level level);`

- Gets the time string format which is used at logs and log file names

`int lg_get_time_str(char* buf, int isLocalTime);`

- Used at consumer and you can use these on your custom formatter, go check static format_msg

`void lg_str_format_into(lg_string* s, const char* fmt, ...);`

`void lg_str_write_into(lg_string* s, const char* already_formatted_str);`

- Allocator and freer for heap allocated instances or foreign languages

`Logger* lg_alloc();`

`void lg_free(Logger* inst);`

# Explanation of this logger library (how it works):

- In this asynchronous logger, we have main and writer thread.
- Main thread manages lifetime and putting logs into ring buffer.
- Writer thread writes messages that in the ring buffer to log file
and stdout if you provided.
- Sometimes, your log messages may disappear if you stress-test it.
That's because the ring buffer is a **fixed-size** buffer and it is full.
I have decided the **DROP THE LOG** policy by default at those situations
but you can change it in config. To see in where it dropped, you can define
`LOGGER_DEBUG` in your compilation (with `-DLOGGER_DEBUG`) or runtime (`#define LOGGER_DEBUG`)
- The config printStdout significantly slows down writer thread.
If you're using this at prod, dont forget to make `printStdout` false

## Multiple Instances (since v3.0)

- This library comes with **multiple-instance** support
- You can set or get active (current) instance with `lg_get_active_instance` and `lg_set_active_instance`
- `lg_init` tries to set active instance if active's NULL
- `lg_destroy` DOES NOT make active instance NULL and frees the memory, it just clears its fields, shutdowns writer thread and flags it is dead (`isAlive=0`). You have to free or allocate your memory (we have `lg_alloc` and `lg_free` you can use them in anywhere else)
- You can make `lg_destroy` parameter NULL and it tries to destroy the active instance like you dont have to do `lg_destroy(lg_get_active_instance())`

```c
LoggerConfig cfg = (LoggerConfig) {
  .localTime=1,
  .printStdout=1,
  .policy=LG_DROP,
  .logFormatter=NULL
};
Logger* lg1 = lg_alloc();
Logger* lg2 = lg_alloc();

lg_init(lg1, "logs1", cfg);
lg_init(lg2, "logs2", cfg);
// active: lg1

lg_info("This is in logs1 folder.");
lg_infoi(lg2, "This is in logs2 folder.");
// the "i" part stands for "explicit instance"

lg_infoi(NULL, "This is in logs2 folder.");
// this also works and it's using active one

lg_finfo("Hello from function"); // normal functions, used at FFIs
```

- **NOTE**: Every logger instance initialization creates a thread which is expensive to have.

## Customization (since v2.1)

- You can customize log message. The customization limited by just layout.
You can change time string, level and formatted message layout.
- Default Layout: `time_str [level] msg`
- You can have your custom time string format (dont worry about size of the time_str buffer)
- Note that time_str is evaluated at consumer (writer) thread. It may not show correct time when you call producer.
- If you want to use custom log layout declare formatter function ([example in default](logger.h#L1019)) and assign it in logger config. Don't forget newline char.

Latest usage in C:
```c
int myFormatter(const int local_time, const lg_log_level level,
                   const char* msg, lg_msg_pack* pack) {
    char time_str[24];
    if (!lg_get_time_str(time_str, local_time)) return 0;
    const char* lvl = lg_lvl_to_str(level);

    if (pack->stdout_str.data) {
        lg_str_format_into(&pack->stdout_str, "%s %s: %s\n", time_str, lvl, msg);
    }

    if (pack->file_str.data) {
        lg_str_format_into(&pack->file_str, "%s %s: %s\n", time_str, lvl, msg);
    }
}

Logger* lg = lg_alloc();
LoggerConfig conf = {
  .localTime = 1, .printStdout = 1,
  .policy = LG_DROP, .logFormatter = myFormatter
};
lg_init(lg, "logs", conf);
```

- And you can always add new level and new logger stream instance!
- All you have to do is define macros [like this](logger.h#L276)
- And define stream macro [like this](loggerstream.hpp#L49)
- That's it, you can use your custom level

# C++ Exclusives

- Use loggerstream.hpp if you use C++ in your project or app to enable this kind of stuff:
```cpp
sinfo << "Hello, World!";
serr  << "Error occured!";
swarn << "Warning:" << "Use loggerstream.hpp in C++";
```

- Streams also supports concatination of strings and delimiter char is ` `,
this means swarn in that example prints out `Warning: Use loggerstream.hpp in C++`
and inserts newline (`\n`) automatically.
And in streams, we have support for Qt string (QString) if you use Qt Core library.

# About

- [License (MIT)](./LICENSE)
- [YouTube](https://youtube.com/@ilpenwastaken)
- [Instagram](https://instagram.com/ilpenwastaken)
- [X](https://x.com/ilpenwastaken)
