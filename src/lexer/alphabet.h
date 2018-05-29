#pragma once

#include <set>
#include <string>

namespace lexer {

class Alphabet {
 public:
  using set_type = std::set<char>;
  using iterator = set_type::iterator;

  void insert(char ch) { alphabet_.insert(ch); }

  std::string to_string() const;

  iterator begin() { return alphabet_.begin(); }
  iterator end() { return alphabet_.end(); }


 private:
  std::set<char> alphabet_;
};

} // namespace lexer
