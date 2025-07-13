cmake_minimum_required(VERSION 3.22.1)
project(Slash)

set(CMAKE_CXX_STANDARD 20)

add_executable(Slash main.cpp
        abstractions/definitions.h
        abstractions/iofuncs.h
        abstractions/iofuncs.cpp
        command.h
        command.cpp)
add_executable(echo commands/echo.cpp abstractions/iofuncs.cpp
        commands/ren.cpp
        commands/read.cpp
        commands/ansi.cpp
        commands/clear.cpp
        commands/ls.cpp
        commands/del.cpp
)
