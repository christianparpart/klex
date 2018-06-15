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

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

Lexer::Lexer(LexerDef info)
    : transitions_{std::move(info.transitions)},
      initialStateId_{info.initialStateId},
      acceptStates_{std::move(info.acceptStates)},
      word_{},
      ownedStream_{},
      stream_{nullptr},
      oldOffset_{},
      offset_{},
      line_{1},
      column_{0},
      token_{ErrorTag} {
}

Lexer::Lexer(LexerDef info, std::unique_ptr<std::istream> stream)
    : Lexer{std::move(info)} {
  open(std::move(stream));
}

Lexer::Lexer(LexerDef info, std::istream& stream)
    : Lexer{std::move(info)} {
}

void Lexer::open(std::unique_ptr<std::istream> stream) {
  ownedStream_ = std::move(stream);
  stream_ = ownedStream_.get();
  oldOffset_ = 0;
  offset_ = 0;
  line_ = 1;
  column_ = 0;
}

constexpr StateId BadState { 101010 }; //static_cast<StateId>(-2);

static std::string stateName(StateId s) {
  switch (s) {
    case BadState:
      return "Bad";
    case ErrorState:
      return "Error";
    default:
      return std::to_string(s);
  }
}

[[maybe_unused]] static std::string toString(std::deque<StateId> stack) {
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

Tag Lexer::recognize() {
  for (;;) {
    if (Tag tag = recognizeOne(); tag != IgnoreTag) {
      return tag;
    }
  }
}

Tag Lexer::recognizeOne() {
  // init
  oldOffset_ = offset_;
  word_.clear();
  StateId state = initialStateId_;
  std::deque<StateId> stack;
  stack.push_back(BadState);

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
    DEBUG("recognize: state {} char '{}' {}", stateName(state), prettySymbol(ch), isAcceptState(state) ? "accepting" : "");
    state = transitions_.apply(state, ch);
  }

  // trackback
  if (state == BadState) {
    line_ = savedLine;
    column_ = savedCol;
  }
  while (state != BadState && !isAcceptState(state)) {
    DEBUG("recognize: trackback: current state {} {}; stack: {}",
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
  DEBUG("recognize: final state {} {}", stateName(state), isAcceptState(state) ? "accepting" : "non-accepting");

  if (isAcceptState(state))
    return token_ = type(state);

  return token_ = ErrorTag;
}

int Lexer::type(StateId acceptState) const {
  if (auto i = acceptStates_.find(acceptState); i != acceptStates_.end())
    return i->second;

  return ErrorTag;
}

bool Lexer::isAcceptState(StateId id) const {
  return acceptStates_.find(id) != acceptStates_.end();
}

Symbol Lexer::nextChar() {
  if (!buffered_.empty()) {
    offset_++;
    int ch = buffered_.back();
    buffered_.resize(buffered_.size() - 1);
    DEBUG("Lexer:{}: advance '{}'", offset_, prettySymbol(ch));
    return ch;
  }

  if (!stream_->good()) { // EOF or I/O error
    DEBUG("Lexer:{}: advance '{}'", offset_, "EOF");
    return EndOfFileTransition;
  }

  int ch = stream_->get();
  if (ch >= 0)
    offset_++;
  DEBUG("Lexer:{}: advance '{}'", offset_, prettySymbol(ch));
  return ch;
}

void Lexer::rollback() {
  if (word_.back() != -1) {
    offset_--;
    // TODO: rollback (line, column)
    buffered_.push_back(word_.back());
  }
}

} // namespace klex
