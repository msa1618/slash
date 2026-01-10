cmake_minimum_required(VERSION 3.16)
project(slash)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

add_executable(slash
  src/errors.cpp
  src/interpreter/commands.cpp
  src/interpreter/expander.cpp
  src/interpreter/interpreter.cpp
  src/interpreter/condloop.cpp
  src/interpreter/builtins.cpp
  src/history_manager.cpp
  src/prompt/prompt.cpp
  src/global.cpp
  src/parser/lexer.cpp
  src/parser/parser.cpp
  src/parser/word.cpp
  src/prompt/sead.cpp
  src/session.cpp
  src/iofuncs.cpp
  src/main.cpp
  src/terminal.cpp
)

target_include_directories(slash PRIVATE include)