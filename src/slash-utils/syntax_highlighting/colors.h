#ifndef SLASH_COLORS_H
#define SLASH_COLORS_H

#include <string>

const std::string COLOR_NUMS               = "\x1b[38;2;236;157;237m";
const std::string COLOR_KEYWORDS           = "\x1b[38;2;39;125;161m";
const std::string COLOR_FUNCS              = "\x1b[38;2;255;159;28m";
const std::string COLOR_QUOTES             = "\x1b[38;2;103;148;54m";
const std::string COLOR_COMMENTS           = "\x1b[38;2;88;129;87m";
const std::string COLOR_TYPES              = "\x1b[38;2;255;0;255m";
const std::string COLOR_ANNOTATIONS        = "\x1b[38;2;142;202;230m";
const std::string COLOR_PACKAGES           = "\x1b[38;2;255;143;171m";
const std::string COLOR_CONSTANTS          = "\x1b[38;2;60;110;113m";
const std::string COLOR_VARS               = "\x1b[38;5;117m";
const std::string COLOR_CLASSES            = "\x1b[38;2;22;219;101m";
const std::string COLOR_VAR_INSIDE_QUOTES  = "\x1b[38;2;255;122;162m";

// Special, not found everywhere
const std::string COLOR_ATTRS       = "\x1b[38;2;180;180;180m";
const std::string COLOR_NAMESPACES  = "\x1b[38;2;179;182;10m";
const std::string COLOR_RUST_MACRO_RULES = "\x1b[38;2;255;255;0m";
const std::string COLOR_PREP_DIRECTIVES = "\x1b[38;2;165;56;96m";

#endif // SLASH_COLORS_H