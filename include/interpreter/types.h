#ifndef SLASH_TYPES_H
#define SLASH_TYPES_H

#include <parser/nodes/declaration.h>

namespace Types {
  AST::Type string("String");
  AST::Type integer("Integer");
  AST::Type floats("Float");
  AST::Type bools("Boolean");
}

#endif