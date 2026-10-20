# Set VCPKG_ROOT to your vcpkg installation directory or add the following to your cmake call:
# -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg>/scripts/buildsystems/vcpkg.cmake

cmake_minimum_required(VERSION 3.16)
set(CMAKE_VERBOSE_MAKEFILE ON)

if(DEFINED ENV{VCPKG_ROOT}
   AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
   AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
   message(STATUS "Using VCPKG toolchain file")
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
endif()
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

project(cpp-thd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

###########################################
# Sanitizers
###########################################
option(ENABLE_THREAD_SANITIZER "Build with ThreadSanitizer" ON)
if(ENABLE_THREAD_SANITIZER)
  if(NOT UNIX OR NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "ThreadSanitizer is supported only with GCC or Clang on Unix-like platforms")
  endif()
  message(STATUS "ThreadSanitizer is enabled")
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()

###########################################
# Catch2
###########################################
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Catch2.cmake")
enable_testing()

add_subdirectory(threads)
add_subdirectory(threads-exceptions)
add_subdirectory(synchronization-locking)
add_subdirectory(experimental/synchronized)
add_subdirectory(event-synchronization)
add_subdirectory(futures)

# Exercises
add_subdirectory(_exercises/monte-carlo-pi)
add_subdirectory(_exercises/immutablility)
add_subdirectory(_exercises/synchronization)
add_subdirectory(_exercises/thread-safe-queue)