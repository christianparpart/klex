// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Symbols.h>
#include <sstream>

namespace klex {

std::string prettySymbol(Symbol input) {
  switch (input) {
    case Symbols::Error:
      return "<<ERROR>>";
    case Symbols::EndOfFile:
      return "<<EOF>>";
    case Symbols::Epsilon:
      return "ε";
    case ' ':
      return "\\s";
    case '\t':
      return "\\t";
    case '\n':
      return "\\n";
    default:
      if (std::isprint(input))
        return fmt::format("{}", (char) input);
      else
        return fmt::format("\\x{:x02}", input);
  }
}

static std::string _prettyCharRange(Symbol ymin, Symbol ymax) {
  std::stringstream sstr;
  switch (std::abs(ymax - ymin)) {
    case 0:
      sstr << prettySymbol(ymin);
      break;
    case 1:
      sstr << prettySymbol(ymin)
           << prettySymbol(ymin + 1);
      break;
    case 2:
      sstr << prettySymbol(ymin)
           << prettySymbol(ymin + 1)
           << prettySymbol(ymax);
      break;
    default:
      sstr << prettySymbol(ymin) << '-' << prettySymbol(ymax);
      break;
  }
  return sstr.str();
}

static std::string _groupCharacterClassRanges(const std::vector<bool>& syms) {
  // {1,3,5,a,b,c,d,e,f,z]
  // ->
  // {{1}, {3}, {5}, {a-f}, {z}}

  std::stringstream sstr;
  Symbol ymin = '\0';
  Symbol ymax = ymin;
  int k = 0;

  for (size_t i = 0, e = syms.size(); i != e; ++i) {
    if (!syms[i])
      continue;

    const Symbol c = (Symbol) i;
    if (c == ymax + 1) {  // range growing
      ymax = c;
    }
    else { // gap found
      if (k) {
        sstr << _prettyCharRange(ymin, ymax);
      }
      ymin = ymax = c;
    }
    k++;
  }
  sstr << _prettyCharRange(ymin, ymax);

  return sstr.str();
}

SymbolSet::SymbolSet(DotMode) : set_(256, true) {
  clear('\n');
}

bool SymbolSet::empty() const {
  return size() == 0;
}

size_t SymbolSet::size() const {
  size_t n = 0;
  for (auto i = begin(), e = end(); i != e; ++i)
    n++;
  return n;
}

bool SymbolSet::isDot() const noexcept {
  static SymbolSet dot{SymbolSet::Dot};
  return *this == dot;
}

std::string SymbolSet::to_string() const {
  if (isDot())
    return ".";

  return _groupCharacterClassRanges(set_);
}

void SymbolSet::complement() {
  for (size_t i = 0, e = set_.size(); i != e; ++i) {
    set_[i] = !set_[i];
  }

  // recalculate hash
  hash_ = 2166136261;
  for (Symbol s : *this) {
    hash_ = (hash_ * 16777619) ^ s;
  }
}

} // namespace klex
