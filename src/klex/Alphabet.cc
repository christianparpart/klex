// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Alphabet.h>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace klex {

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

void Alphabet::insert(char ch) {
  if (alphabet_.find(ch) == alphabet_.end()) {
    DEBUG("Alphabet: insert '{:}'", (ch));
    alphabet_.insert(ch);
  }
}

void Alphabet::merge(const set_type& syms) {
  for (Symbol s : syms) {
    insert(s);
  }
}

std::string Alphabet::to_string() const {
  std::stringstream sstr;

  sstr << '{';

  for (Symbol c : alphabet_) {
    if (c == '\0')
      sstr << "ε";
    else if (std::isprint(c))
      sstr << c;
    else {
      char buf[10];
      snprintf(buf, sizeof(buf), "\\x%02x", c);
      sstr << buf;
      //sstr << "\\x" << std::hex << std::setfill('0') << std::setw(2) << uint8_t(c);
    }
  }

  sstr << '}';

  return std::move(sstr.str());
}

} // namespace klex
