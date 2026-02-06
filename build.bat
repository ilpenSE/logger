@echo off
color a
title Logger Build with MSVC
cls
setlocal

set CC=cl
set CFLAGS=/std:c11 /W4 /DLOGGER_IMPLEMENTATION /DLOGGER_BUILD

if "%1"=="debug" (
  set CFLAGS=%CFLAGS% /DLOGGER_DEBUG /Zi /Od /LDd /MDd
) else (
  set CFLAGS=%CFLAGS% /O2 /LD /MD
)

if not exist build mkdir build

%CC% %CFLAGS% logger.h /Fe:build\logger.dll

endlocal
