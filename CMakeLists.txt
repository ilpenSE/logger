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

target_compile_definitions(logger PRIVATE LOGGER_BUILD)

# Linux
if(UNIX AND NOT APPLE)
    target_compile_options(logger PRIVATE -fvisibility=hidden)
endif()

# Windows: dll + dll.a
if(WIN32)
    set_target_properties(logger PROPERTIES
        PREFIX ""                         # no liblogger.dll
        OUTPUT_NAME "logger"              # logger.dll
    )
endif()

