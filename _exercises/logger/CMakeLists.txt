cmake_minimum_required(VERSION 3.16)

get_filename_component(DIRECTORY_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" PROJECT_NAME ${DIRECTORY_NAME})
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#----------------------------------------
# set compiler
#----------------------------------------
if (MSVC)
    add_compile_options(-D_SCL_SECURE_NO_WARNINGS)
endif()

#----------------------------------------
# set Threads
#----------------------------------------
find_package(Threads REQUIRED)

#----------------------------------------
# Application
#----------------------------------------

# Sources
aux_source_directory(. SRC_LIST)

# Headers
file(GLOB HEADERS_LIST "*.h" "*.hpp")

# Application
add_executable(${PROJECT_NAME} ${SRC_LIST} ${HEADERS_LIST})
target_link_libraries(${PROJECT_NAME} Threads::Threads) 

# Setting C++ standard
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)