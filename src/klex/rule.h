#pragma once

#include <klex/fa.h>
#include <string>

namespace klex {

struct Rule {
  int priority;
  fa::Tag tag;
  std::string pattern;
};

} // namespace klex
