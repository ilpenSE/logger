@echo off
set CFLAGS=/W4 /I"../.." /Fo"build\\" /Fe:build\app.exe /D_CRT_SECURE_NO_WARNINGS
set SRC=main.c test.c

if not exist build mkdir build
cl %CFLAGS% %SRC%

move build\app.exe .