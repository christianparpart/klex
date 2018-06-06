#pragma once

#include <iostream>
#include <string_view>
#include <lexer/fa.h>
#include <lexer/lexerdef.h>
#include <memory>
#include <map>
#include <vector>

namespace lexer {

/**
 * Lexer API for recognizing words.
 */
class Lexer {
 public:
  /**
   * Constructs the Lexer with the given information table.
   *
   * @see Builder
   */
  explicit Lexer(LexerDef info);

  void open(std::unique_ptr<std::istream> input);

  /**
   * Parses a token and returns its tag (or -1 on lexical error).
   */
  fa::Tag recognize();

  //! the underlying word of the currently recognized token
  std::string word() const { return word_; }

  //! @returns the absolute offset of the file the lexer is currently reading from.
  std::pair<unsigned, unsigned> offset() const noexcept { return std::make_pair(oldOffset_, offset_); }

  //! @returns the current line the lexer is reading from.
  unsigned line() const noexcept { return line_; }

  //! @returns the current column of the current line the lexer is reading from.
  unsigned column() const noexcept { return column_; }

 private:
  int nextChar();
  void rollback();
  bool isAcceptState(fa::StateId state) const;
  int type(fa::StateId acceptState) const;

 private:
  TransitionMap transitions_;
  fa::StateId initialStateId_;
  std::map<fa::StateId, fa::Tag> acceptStates_;
  std::string word_;
  std::unique_ptr<std::istream> stream_;
  std::vector<int> buffered_;
  unsigned oldOffset_;
  unsigned offset_;
  unsigned line_;
  unsigned column_;
};

} // namespace lexer
