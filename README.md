# The Logger Library

- ![Language](https://img.shields.io/badge/language-C-blue)
![Language](https://img.shields.io/badge/sometimes-C++-blue)
- ![Platform](https://img.shields.io/badge/platform-all-brightgreen)
- ![Asynchronous](https://img.shields.io/badge/asynchronous-purple?style=for-the-badge)
![Thread Safe](https://img.shields.io/badge/thread%20safe-brightgreen?style=for-the-badge)
![Flexible](https://img.shields.io/badge/flexible-blue?style=for-the-badge)
![STB-Style](https://img.shields.io/badge/stb-single%20header-yellow?style=for-the-badge)

This Logger supports apple systems, windows and unix or unix-like systems.

It requires minimum C99 version of C.

You can build and generate `.so`, `.dll` and `.dylib` on your machine.

It does only have header files with implementation, but if you define LOGGER_IMPLEMENTATION during compilation of header, you can get shared object.

It is written in purely C. Contains C++ stuff for extra options (not included in logger.h, logger.h is pure C)

Since it is written in C, you can port this library every fucking programming language supports C ABI (most of them). 
You can compile this single header and turn it into shared object file.

## Quick Links

- [Pure C STB-Style Header](./logger.h)
- [C++ Stream (<<) support](./loggerstream.hpp)
- [Usage in C](usage/c)
- [Usage in C++](usage/c++)
- [Usage in Python](usage/python)
- [Usage in Rust](usage/rust)
- [Usage in JS](usage/javascript)
- [Header-only in C++, exported via C ABI (Deprecated)](https://github.com/ilpenSE/logger/tree/deprecated)

## Multiple Instances

- This library comes with multiple-instance support since 3.0
- You can set or get active (current) instance with `lg_get_active_instance` and `lg_set_active_instance`
- `lg_init` tries to set active instance if active's NULL
- `lg_destroy` DOES NOT make active instance NULL and frees the memory, it just clears its fields, shutdowns writer thread and flags it is dead (isAlive=0). You have to free or allocate your memory (we have `lg_alloc` and `lg_free` you can use them in anywhere else)
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

- NOTE: Every logger instance initialization creates a thread which is expensive to have.

## Customization

- You can customize log message. The customization limited by just layout actually. You can change time string, level and formatted message layout.
- Default Layout: time_str [level] msg
- If you want to use custom log layout declare formatter function ([see this](logger.h#L62)) and assign it in logger config. Don't forget newline char.

```c
int myFormatter(const char* time_str, const char* level, const char* message, char* out, size_t size) {
  return snprintf(out, size, "%s %s: %s\n", time_str, level, message);
  // like this: "2026.01.22-00.45.05.994 WARNING: a warning message"
}

LoggerConfig conf = { .localTime = 1, .printStdout = 1 .logFormatter = myFormatter };
lg_init("logs", conf);
```

- And you can always add new level and new logger stream instance!
- All you have to do is define macros [like this](logger.h#L121)
- And define stream macro [like this](loggerstream.hpp#L43)
- That's it, you can use your custom level

## Library Linking/Usage

Compile STB style header (`logger.h`) and use in any language that supports C ABI. (C, Rust, C++ etc.)

You can generate shared objects with these commands:

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
if you wanna have debug information

## C++ Exclusives

Use loggerstream.hpp if you use C++ in your project or app to enable this kind of stuff:
```cpp
sinfo << "Hello, World!";
serr  << "Error occured!";
swarn << "Warning:" << "Use loggerstream.hpp in C++";
```

Streams also supports concatination of strings and delimiter char is ` `, this means swarn in that example prints out `Warning: Use loggerstream.hpp in C++` and inserts newline (`\n`) automatically.

In streams, we have support for Qt string (QString) if you use Qt Core library.
