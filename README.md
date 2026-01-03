# My Logger

This Logger supports apple systems (APPLE), windows (WIN32) and linux
You can build and generate `.so`, `.dll`/`.dll.a` and `.dylib`

It is written in C++, exported via `extern "C"` if you're not using C.

## To Build

On current directory
```bash
cd build
cmake ..
make --build .
```
or if you're using mingw:
```bash
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```
