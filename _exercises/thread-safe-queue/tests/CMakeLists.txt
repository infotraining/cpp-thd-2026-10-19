cmake_minimum_required(VERSION 3.16)

if(DEFINED ENV{VCPKG_ROOT}
   AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
   AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
endif()
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

project(thread_safe_queue_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

find_package(Catch2 3 CONFIG QUIET)

if(NOT Catch2_FOUND)
  if(NOT DEFINED FETCHCONTENT_BASE_DIR)
    set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/dependencies" CACHE PATH
      "Directory used to populate dependencies")
  endif()

  include(FetchContent)

  message(STATUS "Providing Catch2 with FetchContent")

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.0
  )
  FetchContent_MakeAvailable(Catch2)
endif()

enable_testing()

add_executable(thread_safe_queue_tests thread_safe_queue_tests.cpp)
target_link_libraries(thread_safe_queue_tests PRIVATE thread_safe_queue_lib Threads::Threads Catch2::Catch2WithMain)
add_test(NAME thread_safe_queue_tests COMMAND thread_safe_queue_tests)