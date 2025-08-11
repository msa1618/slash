cmake_minimum_required(VERSION 3.16)
project(slash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
 # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -O1")
#  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
#endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)

find_package(nlohmann_json REQUIRED)
find_package(ftxui REQUIRED)
find_package(Boost REQUIRED COMPONENTS regex)

add_executable(slash
        src/main.cpp
        src/core/execution.h
        src/core/execution.cpp
        src/core/prompt.cpp
        src/core/prompt.h
        src/core/startup.h
        src/core/startup.cpp
        src/abstractions/iofuncs.cpp
        src/abstractions/iofuncs.h
        src/abstractions/info.cpp
        src/abstractions/info.h
        "src/abstractions/json.hpp"
        src/abstractions/json.cpp
        src/abstractions/json.h
        src/git/git.cpp
        src/git/git.h

        src/builtin-cmds/cd.cpp
        src/builtin-cmds/cd.h
        src/core/parser.cpp
        src/core/parser.h
        src/builtin-cmds/var.cpp
        src/builtin-cmds/var.h
        src/builtin-cmds/alias.cpp
        src/builtin-cmds/alias.h
)

target_link_libraries(slash PRIVATE git2)
target_link_libraries(slash PRIVATE Boost::regex)
# target_link_libraries(slash PRIVATE nlohmann_json::nlohmann_json)