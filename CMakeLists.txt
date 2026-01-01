cmake_minimum_required(VERSION 3.16)
project(slash)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

add_executable(slash
  src/iofuncs.cpp
  src/main.cpp
)

target_include_directories(slash PRIVATE include)