cmake_minimum_required(VERSION 3.16)

project(concurrent_queue)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(concurrent_queue_lib INTERFACE)
target_include_directories(concurrent_queue_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
