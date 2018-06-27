// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Lexer.h>
#include <algorithm>
#include <deque>
#include <iostream>
#include <sstream>

namespace klex {

template<typename Token, const bool Debug>
inline Lexer<Token, Debug>::Lexer(LexerDef info, DebugLogger logger)
    : transitions_{std::move(info.transitions)},
      initialStates_{info.initialStates},
      acceptStates_{std::move(info.acceptStates)},
      tagNames_{std::move(info.tagNames)},
      debug_{logger},
      initialStateId_{1},
      word_{},
      ownedStream_{},
      stream_{nullptr},
      oldOffset_{},
      offset_{},
      line_{1},
      column_{0},
      token_{0} {
}

template<typename Token, const bool Debug>
inline Lexer<Token, Debug>::Lexer(LexerDef info, std::unique_ptr<std::istream> stream, DebugLogger logger)
    : Lexer{std::move(info), std::move(logger)} {
  open(std::move(stream));
}

template<typename Token, const bool Debug>
inline Lexer<Token, Debug>::Lexer(LexerDef info, std::istream& stream, DebugLogger logger)
    : Lexer{std::move(info), std::move(logger)} {
}

template<typename Token, const bool Debug>
inline void Lexer<Token, Debug>::open(std::unique_ptr<std::istream> stream) {
  ownedStream_ = std::move(stream);
  stream_ = ownedStream_.get();
  oldOffset_ = 0;
  offset_ = 0;
  line_ = 1;
  column_ = 0;
}

template<typename Token, const bool Debug>
inline std::string Lexer<Token, Debug>::stateName(StateId s, const std::string_view& n) {
  switch (s) {
    case BadState:
      return "Bad";
    case ErrorState:
      return "Error";
    default:
      return fmt::format("{}{}", n, std::to_string(s));
  }
}

template<typename Token, const bool Debug>
inline std::string Lexer<Token, Debug>::toString(const std::deque<StateId>& stack) {
  std::stringstream sstr;
  sstr << "{";
  int i = 0;
  for (const auto s : stack) {
    if (i) sstr << ",";
    sstr << stateName(s);
    i++;
  }

  sstr << "}";
  return sstr.str();
}

template<typename Token, const bool Debug>
inline Token Lexer<Token, Debug>::recognize() {
  for (;;) {
    if (Token tag = recognizeOne(); static_cast<Tag>(tag) != IgnoreTag) {
      return tag;
    }
  }
}

template<typename Token, const bool Debug>
inline Token Lexer<Token, Debug>::recognizeOne() {
  // init
  oldOffset_ = offset_;
  word_.clear();
  StateId state = initialStateId_;
  std::deque<StateId> stack;
  stack.push_back(BadState);

  if constexpr (Debug) debugf("recognize: startState {}, offset {} [{}:{}]",
                              stateName(state), offset_, line_, column_);

  // advance
  unsigned int savedLine = line_;
  unsigned int savedCol = column_;
  while (state != ErrorState) {
    Symbol ch = nextChar(); // one of: input character, ERROR or EOF
    word_.push_back(ch);

    if (isAcceptState(state))
      stack.clear();

    stack.push_back(state);
    savedLine = line_;
    savedCol = column_;
    state = delta(state, ch);
  }

  // trackback
  if (state == BadState) {
    line_ = savedLine;
    column_ = savedCol;
  }

  // backtrack to last (right-most) accept state
  while (state != BadState && !isAcceptState(state)) {
    if constexpr(Debug) debugf("recognize: backtrack: current state {} {}; stack: {}",
                               stateName(state),
                               isAcceptState(state) ? "accepting" : "non-accepting",
                               toString(stack));

    state = stack.back();
    stack.pop_back();
    if (!word_.empty()) {
      rollback();
      word_.resize(word_.size() - 1);
    }
  }

  // backtrack to right-most non-lookahead position in input stream
  if (auto i = backtracking_.find(state); i != backtracking_.end()) {
    const StateId tmp = state;
    const StateId backtrackState = i->second;
    if constexpr(Debug) debugf("recognize: backtracking from {} to {}", stateName(state), stateName(backtrackState));
    while (!stack.empty() && state != backtrackState) {
      state = stack.back();
      stack.pop_back();
      if (!word_.empty()) {
        rollback();
        word_.resize(word_.size() - 1);
      }
    }
    state = tmp;
  }

  if constexpr(Debug) debugf("recognize: final state {} {}",
                             stateName(state),
                             isAcceptState(state) ? "accepting" : "non-accepting");

  if (!isAcceptState(state))
    throw LexerError{offset_, line_, column_};

  if (auto i = acceptStates_.find(state); i != acceptStates_.end())
    return token_ = static_cast<Token>(i->second);

  // should never happen
  fprintf(stderr, "Internal bug. Accept state hit, but no tag assigned.\n");
  abort();
}

template<typename Token, const bool Debug>
inline StateId Lexer<Token, Debug>::delta(StateId currentState, Symbol inputSymbol) const {
  const StateId nextState = transitions_.apply(currentState, inputSymbol);
  if constexpr(Debug) debugf("recognize: state {:>4} --{:-^7}--> {:<6} {}",
                             stateName(currentState),
                             prettySymbol(inputSymbol),
                             stateName(nextState),
                             isAcceptState(nextState) ? "(accepting)" : "");

  return nextState;
}

template<typename Token, const bool Debug>
inline bool Lexer<Token, Debug>::isAcceptState(StateId id) const {
  return acceptStates_.find(id) != acceptStates_.end();
}

template<typename Token, const bool Debug>
inline Symbol Lexer<Token, Debug>::nextChar() {
  if (!buffered_.empty()) {
    offset_++;
    int ch = buffered_.back();
    buffered_.resize(buffered_.size() - 1);
    // if constexpr(Debug) debugf("Lexer:{}: advance '{}'", offset_, prettySymbol(ch));
    return ch;
  }

  if (!stream_->good()) { // EOF or I/O error
    // if constexpr(Debug) debugf("Lexer:{}: advance '{}'", offset_, "EOF");
    return Symbols::EndOfFile;
  }

  int ch = stream_->get();
  if (ch < 0)
    return Symbols::EndOfFile;

  offset_++;
  // if constexpr(Debug) debugf("Lexer:{}: advance '{}'", offset_, prettySymbol(ch));
  return ch;
}

template<typename Token, const bool Debug>
inline void Lexer<Token, Debug>::rollback() {
  if (word_.back() != -1) {
    offset_--;
    // TODO: rollback (line, column)
    buffered_.push_back(word_.back());
  }
}

} // namespace klex
