// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Alphabet.h>
#include <sstream>

namespace klex {

std::string Alphabet::to_string() const {
  std::stringstream sstr;

  sstr << '{';

  for (Symbol c : alphabet_) {
    if (c == '\0')
      sstr << "ε";
    else
      sstr << c;
  }

  sstr << '}';

  return std::move(sstr.str());
}

} // namespace klex
