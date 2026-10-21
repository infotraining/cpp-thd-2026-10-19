cmake_minimum_required(VERSION 3.16)

project(thread_pool)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(thread_pool_lib INTERFACE)
target_include_directories(thread_pool_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(thread_pool_lib INTERFACE concurrent_queue_lib)
