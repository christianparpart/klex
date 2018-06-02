#pragma once

#include <iostream>
#include <string_view>
#include <lexer/fa.h>
#include <memory>
#include <map>
#include <vector>

namespace lexer {

using CharCatId = unsigned int;
constexpr CharCatId ErrorCharCat = static_cast<CharCatId>(-1);
constexpr fa::StateId ErrorState = static_cast<fa::StateId>(-1);

class TransitionMap {
 public:
  void define(fa::StateId currentState, CharCatId currentCharCat, fa::StateId nextState) {
    mapping_[currentState][currentCharCat] = nextState;
  }

  fa::StateId apply(fa::StateId currentStateId, CharCatId currentCharCat) const {
    if (auto i = mapping_.find(currentStateId); i != mapping_.end())
      if (auto k = i->second.find(currentCharCat); k != i->second.end())
        return k->second;

    return ErrorState;
  }

  std::vector<fa::StateId> states() const {
    std::vector<fa::StateId> v;
    v.reserve(mapping_.size());
    for (const auto& i : mapping_)
      v.push_back(i.first);
    return v;
  }

  std::map<fa::Symbol, fa::StateId> map(fa::StateId s) {
    std::map<fa::Symbol, fa::StateId> m;
    for (const auto& i : mapping_[s])
      m[i.first] = i.second;
    return m;
  }

 private:
  std::map<fa::StateId, std::map<CharCatId, fa::StateId>> mapping_;
};

class Lexer {
 public:
  Lexer(TransitionMap transitions, fa::StateId initialStateId, std::vector<fa::StateId> acceptStates);

  void open(std::unique_ptr<std::istream> input);

  // parses a token and returns its ID (or -1 on lexical error)
  int recognize();

  // the underlying lexeme of the currently recognized token
  std::string lexeme() const { return lexeme_; }

  unsigned line() const noexcept { return line_; }
  unsigned column() const noexcept { return column_; }

 private:
  int nextChar();
  void rollback();
  bool isAcceptState(fa::StateId state) const;
  int type(fa::StateId acceptState) const;

 private:
  TransitionMap transitions_;
  fa::StateId initialStateId_;
  std::vector<fa::StateId> acceptStates_;
  std::string lexeme_;
  std::unique_ptr<std::istream> stream_;
  std::vector<int> buffered_;
  unsigned offset_;
  unsigned line_;
  unsigned column_;
};

} // namespace lexer
