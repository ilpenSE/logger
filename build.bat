@echo off
setlocal

set CC=cl
set CFLAGS=/std:c11 /W4 /DLOGGER_IMPLEMENTATION /D_CRT_SECURE_NO_WARNINGS /D_REENTRANT /Fo"build\\"

if "%1"=="debug" (
  set CFLAGS=%CFLAGS% /DLOGGER_DEBUG /Zi /Od /LDd /MDd
) else (
  set CFLAGS=%CFLAGS% /O2 /LD /MD
)

if not exist build mkdir build

%CC% %CFLAGS% /TC logger.h /Fe:build\logger.dll

endlocal
