cmake_minimum_required(VERSION 3.16)

project(thread_safe_queue)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(thread_safe_queue_lib INTERFACE)
target_include_directories(thread_safe_queue_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
