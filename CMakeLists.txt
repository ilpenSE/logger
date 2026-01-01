cmake_minimum_required(VERSION 3.16)
project(loggercpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(logger SHARED
    logger.cpp
    logger_api.cpp
)

target_include_directories(logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# DLL export macro
target_compile_definitions(logger PRIVATE LOGGER_BUILD)

# Linux: liblogger.so
# Windows: logger.dll
