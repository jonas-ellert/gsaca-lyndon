cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)
project(gsaca-lyndon CXX C)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

function(print TYPE TEXT)
    string(ASCII 27 Esc)
    message(${TYPE} "${TEXT}")
endfunction()
print(STATUS "")
print(STATUS "Executing CMakeList.txt in directory ${PROJECT_SOURCE_DIR}...")

# Require C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Require G++/GCC 7.3
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "7.3")
        print(FATAL_ERROR "g++ version 7.3 or greater required!")
    endif()
else()
    print(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} is not supported!")
endif()
print(STATUS "g++ version: ${CMAKE_CXX_COMPILER_VERSION}")
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    if(${CMAKE_C_COMPILER_VERSION} VERSION_LESS "7.3")
        print(FATAL_ERROR "gcc version 7.3 or greater required!")
    endif()
else()
    print(FATAL_ERROR "Compiler ${CMAKE_C_COMPILER_ID} is not supported!")
endif()
print(STATUS "gcc version: ${CMAKE_C_COMPILER_VERSION}")

# Find GMP libraries and header

find_path(GMP_INCLUDE_DIR NAMES gmpxx.h)
find_library(GMP_LIBRARIES NAMES gmp gmp)
find_library(GMPXX_LIBRARIES NAMES gmpxx gmpxx)
if(NOT GMP_LIBRARIES)
  message(FATAL_ERROR "gmpxx library not found!")
endif()

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
print(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# Compiler flags         -Winline \ -Wno-error=inline \
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -fopenmp \
        -Wall \
        -Wextra \
        -Wpedantic \
        -fdiagnostics-color=auto \
        --param large-function-growth=10000 \
        --param inline-unit-growth=1000")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} \
        -O3 \
        -ffast-math \
        -funroll-loops \
        -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} \
        -O0 \
        -ggdb")

print(STATUS "")
print(STATUS "Adding gsaca-lyndon includes...")
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/benchmark/include)

print(STATUS "Adding sdsl-lite as an external project...")
set(SDSL_PREFIX ${PROJECT_SOURCE_DIR}/external/sdsl)
include(ExternalProject)
ExternalProject_Add(
        fetch_sdsl PREFIX ${SDSL_PREFIX}
        GIT_REPOSITORY https://github.com/simongog/sdsl-lite
        GIT_TAG ddb0fbbc33bb183baa616f17eb48e261ac2a3672 # 1.8.1
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${SDSL_PREFIX}
        UPDATE_COMMAND ""
)
print(STATUS "Adding include directory and libraries for sdsl, gtest and divsufsort")
include_directories(${SDSL_PREFIX}/include)
link_directories(${SDSL_PREFIX}/lib)

print(STATUS "Adding tlx as an external project...")
set(TLX_PREFIX ${PROJECT_SOURCE_DIR}/external/tlx)
ExternalProject_Add(
        fetch_tlx PREFIX ${TLX_PREFIX}
        GIT_REPOSITORY https://github.com/tlx/tlx
        GIT_TAG 1052dc757aad89b5d2bf442a17c22c4c0e697bf0
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${TLX_PREFIX}
        UPDATE_COMMAND ""
)
print(STATUS "Adding include directory and libraries for tlx...")
include_directories(${TLX_PREFIX}/include)
link_directories(${TLX_PREFIX}/lib)
link_directories(${TLX_PREFIX}/lib64)

print(STATUS "Adding include directory and libraries for gsaca...")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi")
add_library(gsaca ${PROJECT_SOURCE_DIR}/external/gsaca/gsaca.c)
include_directories(${PROJECT_SOURCE_DIR}/external/gsaca)

print(STATUS "Adding include directory for parallel divsufsort...")
add_subdirectory(${PROJECT_SOURCE_DIR}/external/parallel_divsufsort)
include_directories(${PROJECT_SOURCE_DIR}/external/parallel_divsufsort/include)

print(STATUS "Adding include directory and libraries for malloc_count...")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi")
add_library(malloc_count ${PROJECT_SOURCE_DIR}/external/malloc_count/malloc_count.c)
include_directories(${PROJECT_SOURCE_DIR}/external/malloc_count)

print(STATUS "Adding include directory for ips4o...")
include_directories(${PROJECT_SOURCE_DIR}/external/ips4o)

print(STATUS "Creating benchmark target...")
add_executable(benchmark ${PROJECT_SOURCE_DIR}/benchmark/src/benchmark.cpp)
add_dependencies(benchmark fetch_sdsl fetch_tlx)
target_link_libraries(benchmark stdc++fs malloc_count rt dl divsufsort divsufsort64 tlx gsaca atomic parallel_divsufsort lprange)

add_executable(benchmark-large-alphabet ${PROJECT_SOURCE_DIR}/benchmark/src/benchmark-large-alphabet.cpp)
add_dependencies(benchmark-large-alphabet fetch_sdsl fetch_tlx)
target_link_libraries(benchmark-large-alphabet stdc++fs malloc_count rt dl divsufsort divsufsort64 tlx gsaca atomic parallel_divsufsort lprange)

print(STATUS "")
print(STATUS "DONE!")
print(STATUS "")
