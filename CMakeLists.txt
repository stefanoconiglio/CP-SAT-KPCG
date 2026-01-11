cmake_minimum_required(VERSION 3.16)

project(CP_SAT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ORTOOLS_ROOT)
  list(APPEND CMAKE_PREFIX_PATH "${ORTOOLS_ROOT}")
endif()

find_package(ortools REQUIRED CONFIG)

add_executable(CP-SAT CP-SAT.cpp)
target_link_libraries(CP-SAT PRIVATE ortools::ortools)
