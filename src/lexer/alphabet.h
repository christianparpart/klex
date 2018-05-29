#pragma once

#include <set>
#include <string>

namespace lexer {

class Alphabet {
 public:
  void insert(char ch) { alphabet_.insert(ch); }

  std::string to_string() const;

 private:
  std::set<char> alphabet_;
};

} // namespace lexer
