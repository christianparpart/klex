// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/LexerDef.h>

#include <iostream>
#include <stdexcept>
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

  //! @returns the last recognized token.
  Tag token() const noexcept { return token_; }

  const std::string& name() const { return name(token_); }

  const std::string& name(Tag tag) const {
    if (auto i = tagNames_.find(tag); i != tagNames_.end())
      return i->second;

    throw std::invalid_argument{"tag"};
  }

  /**
   * Runtime exception that is getting thrown when a word could not be recognized.
   */
  class LexerError : public std::runtime_error {
   public:
    LexerError(unsigned int offset, unsigned int line, unsigned int column)
        : std::runtime_error{fmt::format("[{}:{}] Failed to lexically recognize a word.")},
          offset_{offset}, line_{line}, column_{column} {}

   private:
    unsigned int offset_;
    unsigned int line_;
    unsigned int column_;
  };

 private:
  Symbol nextChar();
  void rollback();
  bool isAcceptState(StateId state) const;

 private:
  TransitionMap transitions_;
  StateId initialStateId_;
  std::map<StateId, Tag> acceptStates_;
  std::map<Tag, std::string> tagNames_;
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

/**
 * Specialization of LexerBase that strongly types your costom Token type.
 */
template<typename Token = Tag>
class Lexer : public LexerBase {
 public:
  /**
   * Constructs the Lexer with the given information table.
   */
  explicit Lexer(LexerDef info)
      : LexerBase{std::move(info)} {}

  /**
   * Constructs the Lexer with the given information table and input stream.
   */
  Lexer(LexerDef info, std::unique_ptr<std::istream> input)
      : LexerBase{std::move(info), std::move(input)} {}

  /**
   * Constructs the Lexer with the given information table and input stream.
   */
  Lexer(LexerDef info, std::istream& input)
      : LexerBase{std::move(info), input} {}

  /**
   * Recognizes one token (ignored patterns are skipped).
   */
  Token recognize() { return static_cast<Token>(LexerBase::recognize()); }

  //! @returns the last recognized token.
  Token token() const noexcept { return static_cast<Token>(LexerBase::token()); }
};

} // namespace klex
