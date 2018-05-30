#pragma once

#include <set>
#include <string>

namespace lexer {

/**
 * Represents the alphabet of a finite automaton or regular expression.
 */
class Alphabet {
 public:
  using Symbol = char;
  using set_type = std::set<Symbol>;
  using iterator = set_type::iterator;

  void insert(char ch) { alphabet_.insert(ch); }

  std::string to_string() const;

  iterator begin() { return alphabet_.begin(); }
  iterator end() { return alphabet_.end(); }

 private:
  set_type alphabet_;
};

} // namespace lexer
