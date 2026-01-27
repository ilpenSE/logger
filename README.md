# The Logger Library

- ![Language](https://img.shields.io/badge/language-C-blue)
![Language](https://img.shields.io/badge/sometimes-C++-blue)
- ![Platform](https://img.shields.io/badge/platform-all-brightgreen)
- ![Asynchronous](https://img.shields.io/badge/asynchronous-purple?style=for-the-badge)
![Thread Safe](https://img.shields.io/badge/thread%20safe-brightgreen?style=for-the-badge)
![Flexible](https://img.shields.io/badge/flexible-blue?style=for-the-badge)
![STB-Style](https://img.shields.io/badge/stb-single%20header-yellow?style=for-the-badge)

This Logger supports apple systems (APPLE), windows (WIN32) and linux
You can build and generate `.so`, `.dll`/`.dll.a` and `.dylib` on your machine.

It does only have header files with implementation, but if you define LOGGER_IMPLEMENTATION during compilation of header, you can get shared object.

It is written in purely C. Contains C++ stuff for extra options (not included in logger.h, logger.h is pure C)

Since it is written in C, you can port this library every fucking programming language supports C ABI (most of them) And if you worry about C standart, this library supports min C11. But you can compile this single header and turn it into shared object file.

## Quick Links

- [Pure C STB-Style Header](./logger.h)
- [C++ Stream (<<) support](./loggerstream.hpp)
- [Usage in C](usage/c)
- [Usage in C++](usage/c++)
- [Header-only in C++, exported via C ABI (Deprecated)](./deprecated/logger_stb.hpp)
- [Deprecated Source code (only-C++ 1.0.0)](deprecated/)

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

Use STB style header (`logger.h`) in any language that supports C ABI. (C, Rust, C++ etc.)

If you're not using C/C++ and want to use this library, you can compile it like this:
```bash
cc -x c -c logger.h -std=c11 -pthread -DLOGGER_IMPLEMENTATION
```

And you can use loggerstream.hpp if you use C++ in your project or app to enable this kind of stuff:
```cpp
sinfo << "Hello, World!";
serr  << "Error occured!";
swarn << "Warning:" << "Use loggerstream.hpp in C++";
```

Streams also supports concatination of strings and delimiter char is ` `, this means swarn in that example prints out `Warning: Use loggerstream.hpp in C++` and inserts newline (`\n`) automatically.

In streams, we have support for Qt string (QString) if you use Qt Core library.
