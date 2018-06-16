// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/State.h>
#include <sstream>

namespace klex {

std::string to_string(const std::vector<StateId>& S, std::string_view stateLabelPrefix) {
  std::vector<StateId> names = S;

  std::sort(names.begin(), names.end());

  std::stringstream sstr;
  sstr << "{";
  int i = 0;
  for (StateId name : names) {
    if (i)
      sstr << ", ";
    sstr << stateLabelPrefix << name;
    i++;
  }
  sstr << "}";

  return sstr.str();
}

} // namespace klex
