# My Logger Library for C/C++ and (maybe) Rust

I dont test this library in Rust btw.

This Logger supports apple systems (APPLE), windows (WIN32) and linux
You can build and generate `.so`, `.dll`/`.dll.a` and `.dylib` on your machine.

It does only have header files with implementation, but if you define LOGGER_IMPLEMENTATION during compilation of header, you can get shared object.

It is written in purely C. Contains C++ stuff for extra options

## Quick Links

- [Pure C STB-Style Header](./logger.h)
- [C++ Stream (<<) support](./loggerstream.hpp)
- [Usage in C](usage/c)
- [Usage in C++](usage/c++)
- [Header-only in C++, exported via C ABI (Deprecated)](./deprecated/logger_stb.hpp)
- [Deprecated Source code (only-C++ 1.0.0)](deprecated/)

## Library Linking/Usage

Use STB style header (`logger.h`) in any language that supports C ABI. (C, Rust, C++ etc.)

And you can use loggerstream.hpp if you use C++ in your project or app to enable this kind of stuff:
```cpp
sinfo << "Hello, World!";
serr  << "Error occured!";
swarn << "Warning:" << "Use loggerstream.hpp in C++";
```

Streams also supports concatination of strings and delimiter char is ` `, this means swarn in that example prints out `Warning: Use loggerstream.hpp in C++` and inserts newline (`\n`) automatically.

In streams, we have support for Qt string (QString) if you use Qt Core library.
