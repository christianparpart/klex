// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/LexerDef.h>

#include <iostream>
#include <map>
#include <memory>
#include <string_view>
#include <vector>

namespace klex {

/**
 * Lexer API for recognizing words.
 */
class LexerBase {
 public:
  /**
   * Constructs the Lexer with the given information table.
   */
  explicit LexerBase(LexerDef info);

  /**
   * Constructs the Lexer with the given information table and input stream.
   */
  LexerBase(LexerDef info, std::unique_ptr<std::istream> input);

  /**
   * Constructs the Lexer with the given information table and input stream.
   */
  LexerBase(LexerDef info, std::istream& input);

  /**
   * Open given input stream.
   */
  void open(std::unique_ptr<std::istream> input);

  /**
   * Recognizes one token (ignored patterns are skipped).
   *
   * @return -1 on parse error, or Rule tag for recognized pattern.
   */
  Tag recognize();

  /**
   * Recognizes one token, regardless of it is to be ignored or not.
   *
   * @return -1 on parse error, or Rule tag for recognized pattern.
   */
  Tag recognizeOne();

  //! the underlying word of the currently recognized token
  std::string word() const { return word_; }

  //! @returns the absolute offset of the file the lexer is currently reading from.
  std::pair<unsigned, unsigned> offset() const noexcept { return std::make_pair(oldOffset_, offset_); }

  //! @returns the current line the lexer is reading from.
  unsigned line() const noexcept { return line_; }

  //! @returns the current column of the current line the lexer is reading from.
  unsigned column() const noexcept { return column_; }

  //! @returns the last recognized token (which includes ErrorTag and EofTag).
  Tag token() const noexcept { return token_; }

 private:
  Symbol nextChar();
  void rollback();
  bool isAcceptState(StateId state) const;
  int type(StateId acceptState) const;

 private:
  TransitionMap transitions_;
  StateId initialStateId_;
  std::map<StateId, Tag> acceptStates_;
  std::string word_;
  std::unique_ptr<std::istream> ownedStream_;
  std::istream* stream_;
  std::vector<int> buffered_;
  unsigned oldOffset_;
  unsigned offset_;
  unsigned line_;
  unsigned column_;
  Tag token_;
};

template<typename Token = Tag>
class Lexer : public LexerBase {
 public:
  explicit Lexer(LexerDef info)
      : LexerBase{std::move(info)} {}

  Lexer(LexerDef info, std::unique_ptr<std::istream> input)
      : LexerBase{std::move(info), std::move(input)} {}

  Lexer(LexerDef info, std::istream& input)
      : LexerBase{std::move(info), input} {}

  Token recognize() { return static_cast<Token>(LexerBase::recognize()); }
  Token token() const noexcept { return static_cast<Token>(LexerBase::token()); }
};

} // namespace klex
