include_guard(GLOBAL)

message(STATUS "Installing dependencies...")

find_package(Catch2 3 CONFIG QUIET)

if(Catch2_FOUND)
  message(STATUS "Using installed Catch2 package")
endif()

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
