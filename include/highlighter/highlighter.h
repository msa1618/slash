/*
Highlighter valid settings:

commands.program.valid
commands.program.invalid
commands.program.builtin
commands.program.colon

commands.subcommand
commands.flag
commands.flag.long
commands.flag.short

operators
operators.redirectors
operators.redirectors.normal
operators.redirectors.append
operators.redirectors.force
operators.redirectors.input
operators.herestring
operators.heredoc
operators.heredoc.terminator
operators.plus
operators.minus
operators.div
operators.mul
operators.and
operators.or
operators.not
operators.xor
operators.and_logical
operators.or_logical
operators.parentheses
operators.brackets
operators.braces
operators.comparison
operators.assign
operators.not_equal
operators.greater
operators.less
operators.greq
operators.leq
operators.dot
operators.dollar
operators.parameter_expansion
operators.substitution
operators.doublebracket
operators.semicolon

identifiers
identifiers.variables
identifiers.functions
identifiers.type

comments
comments.todo

numbers
numbers.integer
numbers.integer.hex
numbers.integer.oct
numbers.integer.bin
numbers.integer.dec
numbers.float

boolean

keywords
keywords.(all valid keywords)
*/

#include <yaml-cpp/yaml.h>
#include <string>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include "../parser/lexer.h"