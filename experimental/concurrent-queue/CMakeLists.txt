cmake_minimum_required(VERSION 3.16)

if(DEFINED ENV{VCPKG_ROOT}
   AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
   AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
endif()
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

##################
# Target
get_filename_component(DIRECTORY_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" TARGET_MAIN ${DIRECTORY_NAME})

project(${TARGET_MAIN} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

####################
find_package(Threads REQUIRED)

add_subdirectory(src)

#----------------------------------------
# Application
#----------------------------------------
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} Threads::Threads concurrent_queue_lib)


#----------------------------------------
# Tests
#----------------------------------------
enable_testing()
add_subdirectory(tests)