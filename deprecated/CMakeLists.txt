cmake_minimum_required(VERSION 3.16)
project(loggercpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_LOGGER_VERBOSE "Enable verbose logger library" OFF)
if (ENABLE_LOGGER_VERBOSE)
  set(LIBNAME "loggerv")
else()
  set(LIBNAME "logger")
endif()

add_library(${LIBNAME} SHARED
    logger.cpp
    logger_api.cpp
)

target_include_directories(${LIBNAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${LIBNAME} PRIVATE LOGGER_BUILD)

# add LOGGER_VERBOSE macro
if (ENABLE_LOGGER_VERBOSE)
  target_compile_definitions(${LIBNAME} PRIVATE LOGGER_VERBOSE_LIB)
endif()

# Linux
if(UNIX AND NOT APPLE)
  target_compile_options(${LIBNAME} PRIVATE -fvisibility=hidden)
endif()

# Windows: dll + dll.a
if(WIN32)
  set_target_properties(${LIBNAME} PROPERTIES
        PREFIX ""              # no liblogger.dll
        OUTPUT_NAME ${LIBNAME} # logger.dll
    )
endif()

