#include "slash-greeting.h"

int greet() {
  io::print("Welcome to " + yellow + "slash v0.9.0! " + reset + "Type " + green + "help " + reset + "for guidance on using slash.\n");
  return 0;
}