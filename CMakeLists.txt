cmake_minimum_required(VERSION 3.24)
project(minitoolchain
        VERSION 0.2.0
        DESCRIPTION "A complete toolchain for a custom 64-bit virtual ISA"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MINITOOL_BUILD_TESTS      "Build the test suite"            ON)
option(MINITOOL_WERROR           "Treat warnings as errors"        ON)
option(MINITOOL_ENABLE_ASAN      "AddressSanitizer + LeakSanitizer" OFF)
option(MINITOOL_ENABLE_UBSAN     "UndefinedBehaviorSanitizer"      OFF)
option(MINITOOL_ENABLE_COVERAGE  "Coverage instrumentation"        OFF)

# ---------------------------------------------------------------- warnings ---
add_library(minitool_warnings INTERFACE)
target_compile_options(minitool_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
    -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused
    -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion)
if(MINITOOL_WERROR)
    target_compile_options(minitool_warnings INTERFACE -Werror)
endif()

# ------------------------------------------------------------- sanitizers ---
add_library(minitool_sanitizers INTERFACE)
set(_mt_san "")
if(MINITOOL_ENABLE_ASAN)
    list(APPEND _mt_san address)
endif()
if(MINITOOL_ENABLE_UBSAN)
    list(APPEND _mt_san undefined)
endif()
if(_mt_san)
    list(JOIN _mt_san "," _mt_san_joined)
    target_compile_options(minitool_sanitizers INTERFACE
        -fsanitize=${_mt_san_joined} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_link_options(minitool_sanitizers INTERFACE -fsanitize=${_mt_san_joined})
endif()
if(MINITOOL_ENABLE_COVERAGE)
    target_compile_options(minitool_sanitizers INTERFACE --coverage -O0 -g)
    target_link_options(minitool_sanitizers INTERFACE --coverage)
endif()

# ---------------------------------------------------------- project target ---
add_library(minitool_options INTERFACE)
target_link_libraries(minitool_options INTERFACE minitool_warnings minitool_sanitizers)
target_include_directories(minitool_options INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

add_subdirectory(src)

if(MINITOOL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "minitoolchain ${PROJECT_VERSION}")
message(STATUS "  build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  tests      : ${MINITOOL_BUILD_TESTS}")
message(STATUS "  asan/ubsan : ${MINITOOL_ENABLE_ASAN}/${MINITOOL_ENABLE_UBSAN}")
