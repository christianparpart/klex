// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/LexerDef.h>
#include <fmt/format.h>

#include <cassert>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace klex {

/**
 * Lexer API for recognizing words.
 */
template<typename Token = Tag,
         typename Machine = StateId,
         const bool RequiresBeginOfLine = true,
         const bool Debug = false>
class Lexer {
 private:
  using DebugLogger = std::function<void(const std::string&)>;

  template<typename... Args>
  inline void debugf(const char* msg, Args... args) const {
    if constexpr (Debug) {
      if (debug_) {
        debug_(fmt::format(msg, args...));
      }
    }
  }

 public:
  /**
   * Constructs the Lexer with the given information table.
   */
  explicit Lexer(LexerDef info, DebugLogger logger = DebugLogger{});

  /**
   * Constructs the Lexer with the given information table and input stream.
   */
  Lexer(LexerDef info, std::unique_ptr<std::istream> input, DebugLogger logger = DebugLogger{});

  /**
   * Constructs the Lexer with the given information table and input stream.
   */
  Lexer(LexerDef info, std::istream& input, DebugLogger logger = DebugLogger{});

  /**
   * Constructs the Lexer with the given information table and input stream.
   */
  Lexer(LexerDef info, std::string input, DebugLogger logger = DebugLogger{});

  /**
   * Open given input stream.
   */
  void open(std::unique_ptr<std::istream> input);

  /**
   * Recognizes one token (ignored patterns are skipped).
   */
  Token recognize();

  /**
   * Recognizes one token, regardless of it is to be ignored or not.
   */
  Token recognizeOne();

  //! the underlying word of the currently recognized token
  const std::string& word() const { return word_; }

  //! @returns the absolute offset of the file the lexer is currently reading from.
  std::pair<unsigned, unsigned> offset() const noexcept { return std::make_pair(oldOffset_, offset_); }

  //! @returns the current line the lexer is reading from.
  unsigned line() const noexcept { return line_; }

  //! @returns the current column of the current line the lexer is reading from.
  unsigned column() const noexcept { return column_; }

  //! @returns the last recognized token.
  Token token() const noexcept { return token_; }

  //! @returns the name of the current token.
  const std::string& name() const { return name(token_); }

  //! @returns the name of the token represented by Token @p t.
  const std::string& name(Token t) const {
    auto i = tagNames_.find(static_cast<Tag>(t));
    assert(i != tagNames_.end());
    return i->second;
  }

  /**
   * Retrieves the next state for given input state and input symbol.
   *
   * @param currentState the current State the DFA is in to.
   * @param inputSymbol the input symbol that is used for transitioning from current state to the next state.
   * @returns the next state to transition to.
   */
  inline StateId delta(StateId currentState, Symbol inputSymbol) const;

  /**
   * Sets the active deterministic finite automaton to use for recognizing words.
   *
   * @param machine the DFA machine to use for recognizing words.
   */
  Machine setMachine(Machine machine) {
    // since Machine is a 1:1 mapping into the State's ID, we can simply cast here.
    initialStateId_ = static_cast<StateId>(machine);
  }

  /**
   * Retrieves the default DFA machine that is used to recognize words.
   */
  Machine defaultMachine() const {
    auto i = initialStates_.find("INITIAL");
    assert(i != initialStates_.end());
    return static_cast<Machine>(i->second);
  }

  /**
   * Runtime exception that is getting thrown when a word could not be recognized.
   */
  class LexerError : public std::runtime_error {
   public:
    LexerError(unsigned int offset, unsigned int line, unsigned int column)
        : std::runtime_error{fmt::format("[{}:{}] Failed to lexically recognize a word.", line, column)},
          offset_{offset}, line_{line}, column_{column} {}

   private:
    unsigned int offset_;
    unsigned int line_;
    unsigned int column_;
  };

 private:
  Symbol nextChar();
  void rollback();
  StateId getInitialState() const noexcept;
  bool isAcceptState(StateId state) const;
  static std::string stateName(StateId s, const std::string_view& n = "n");
  static constexpr StateId BadState = 101010;
  std::string toString(const std::deque<StateId>& stack);

  int currentChar() const noexcept { return currentChar_; }

  Token token(StateId s) const {
    auto i = acceptStates_.find(s);
    assert(i != acceptStates_.end());
    return static_cast<Token>(i->second);
  }

 private:
  const TransitionMap transitions_;
  const std::map<std::string, StateId> initialStates_;
  const bool containsBeginOfLineStates_;
  const std::map<StateId, Tag> acceptStates_;
  const BacktrackingMap backtracking_;
  const std::map<Tag, std::string> tagNames_;
  const DebugLogger debug_;
  Machine initialStateId_;
  std::string word_;
  std::unique_ptr<std::istream> ownedStream_;
  std::istream* stream_;
  std::vector<int> buffered_;
  unsigned oldOffset_;
  unsigned offset_;
  unsigned line_;
  unsigned column_;
  bool isBeginOfLine_;
  int currentChar_;
  Token token_;
};

} // namespace klex

#include <klex/Lexer-inl.h>
