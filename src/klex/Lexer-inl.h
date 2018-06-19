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

inline LexerBase::LexerBase(LexerDef info)
    : transitions_{std::move(info.transitions)},
      initialStateId_{info.initialStateId},
      acceptStates_{std::move(info.acceptStates)},
      tagNames_{std::move(info.tagNames)},
      word_{},
      ownedStream_{},
      stream_{nullptr},
      oldOffset_{},
      offset_{},
      line_{1},
      column_{0},
      token_{0} {
}

inline LexerBase::LexerBase(LexerDef info, std::unique_ptr<std::istream> stream)
    : LexerBase{std::move(info)} {
  open(std::move(stream));
}

inline LexerBase::LexerBase(LexerDef info, std::istream& stream)
    : LexerBase{std::move(info)} {
}

inline void LexerBase::open(std::unique_ptr<std::istream> stream) {
  ownedStream_ = std::move(stream);
  stream_ = ownedStream_.get();
  oldOffset_ = 0;
  offset_ = 0;
  line_ = 1;
  column_ = 0;
}

inline std::string LexerBase::stateName(StateId s) {
  switch (s) {
    case BadState:
      return "Bad";
    case ErrorState:
      return "Error";
    default:
      return std::to_string(s);
  }
}

inline std::string LexerBase::toString(const std::deque<StateId>& stack) {
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

inline Tag LexerBase::recognize() {
  for (;;) {
    if (Tag tag = recognizeOne(); tag != IgnoreTag) {
      return tag;
    }
  }
}

inline Tag LexerBase::recognizeOne() {
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
    // DEBUG("recognize: state {} char '{}' {}", stateName(state), prettySymbol(ch), isAcceptState(state) ? "accepting" : "");
    state = transitions_.apply(state, ch);
  }

  // trackback
  if (state == BadState) {
    line_ = savedLine;
    column_ = savedCol;
  }

  while (state != BadState && !isAcceptState(state)) {
    // DEBUG("recognize: trackback: current state {} {}; stack: {}",
    //       stateName(state),
    //       isAcceptState(state) ? "accepting" : "non-accepting",
    //       toString(stack));

    state = stack.back();
    stack.pop_back();
    if (!word_.empty()) {
      rollback();
      word_.resize(word_.size() - 1);
    }
  }
  // DEBUG("recognize: final state {} {}", stateName(state), isAcceptState(state) ? "accepting" : "non-accepting");

  if (!isAcceptState(state))
    throw LexerError{offset_, line_, column_};

  if (auto i = acceptStates_.find(state); i != acceptStates_.end())
    return token_ = i->second;

  // should never happen
  fprintf(stderr, "Internal bug. Accept state hit, but no tag assigned.\n");
  abort();
}

inline bool LexerBase::isAcceptState(StateId id) const {
  return acceptStates_.find(id) != acceptStates_.end();
}

inline Symbol LexerBase::nextChar() {
  if (!buffered_.empty()) {
    offset_++;
    int ch = buffered_.back();
    buffered_.resize(buffered_.size() - 1);
    // DEBUG("Lexer:{}: advance '{}'", offset_, prettySymbol(ch));
    return ch;
  }

  if (!stream_->good()) { // EOF or I/O error
    // DEBUG("Lexer:{}: advance '{}'", offset_, "EOF");
    return Symbols::EndOfFile;
  }

  int ch = stream_->get();
  if (ch >= 0)
    offset_++;
  // DEBUG("Lexer:{}: advance '{}'", offset_, prettySymbol(ch));
  return ch;
}

inline void LexerBase::rollback() {
  if (word_.back() != -1) {
    offset_--;
    // TODO: rollback (line, column)
    buffered_.push_back(word_.back());
  }
}

} // namespace klex
