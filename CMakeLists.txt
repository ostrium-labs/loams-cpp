cmake_minimum_required(VERSION 3.20)
project(loams-cpp-examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# The SDK, from one of two places:
#   -DLOAMS_SDK_DIR=/path/to/loams/sdks/cpp   an SDK v0.1.2 source checkout
#   find_package(loams 0.1.2 EXACT CONFIG)     an installed SDK release
set(LOAMS_SDK_DIR "" CACHE PATH "Path to sdks/cpp of a loams checkout")
if(LOAMS_SDK_DIR)
  set(LOAMS_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory("${LOAMS_SDK_DIR}" loams-sdk EXCLUDE_FROM_ALL)
else()
  find_package(loams 0.1.2 EXACT CONFIG REQUIRED)
endif()

foreach(example quickstart sql kb)
  add_executable(${example} examples/${example}/main.cpp)
  target_link_libraries(${example} PRIVATE loams::loams)
endforeach()
