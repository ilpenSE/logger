# My Logger Library for C/C++ and (maybe) Rust

I dont test this library in Rust btw.

This Logger supports apple systems (APPLE), windows (WIN32) and linux
You can build and generate `.so`, `.dll`/`.dll.a` and `.dylib`

It is written in C++, exported via `extern "C"` if you're not using C.

## Quick Links

- [Source code (C++)](src/)
- [Usage in stb style (C++)](stbstyle)
- [Usage in C/C++, with dynamic linking](usage/)
- [Precompiled lib binaries (so/dll)](libs/)

## Library Linking/Usage

- [C/C++ Dynamic Linking Usage](usage/)
- [C++ exclusive STB-Style Usage](stbstyle/)

If you're using C++ 17 or higher, you can use STB-style headers located [there](stbstyle/). 
(C++ 17 or higher part is real, because the fucking `inline` keyword only available on C++17+)

If you're not using C++, you have to link with so/dll and header.

## To Build

On src directory
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
Or if you want verbosed:
```bash
mkdir build
cd build
cmake -S .. -DENABLE_LOGGER_VERBOSE=ON
cmake --build .
```

if you want to use verbosed version of this logger (only runtime errors will be thrown to stderr if you enable) just add `-DENABLE_LOGGER_VERBOSE=ON`
