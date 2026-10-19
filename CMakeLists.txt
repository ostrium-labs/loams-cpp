cmake_minimum_required(VERSION 3.20)
project(loams-cpp-examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# The SDK, from one of two places:
#   -DLOAMS_SDK_DIR=/path/to/loams/sdks/cpp   the monorepo checkout (the 0.1.0 release
#                                             tarball is not published yet)
#   find_package(loams CONFIG)                an installed loams-cpp-<version> release
set(LOAMS_SDK_DIR "" CACHE PATH "Path to sdks/cpp of a loams checkout")
if(LOAMS_SDK_DIR)
  set(LOAMS_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory(${LOAMS_SDK_DIR} loams-sdk EXCLUDE_FROM_ALL)
else()
  find_package(loams CONFIG REQUIRED)
endif()

foreach(example quickstart sql kb)
  add_executable(${example} examples/${example}/main.cpp)
  target_include_directories(${example} PRIVATE common)
  target_link_libraries(${example} PRIVATE loams::loams)
endforeach()
