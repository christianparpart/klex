#include <lexer/lexer.h>
#include <algorithm>
#include <deque>
#include <iostream>
#include <sstream>

namespace lexer {

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
      stream_{},
      oldOffset_{},
      offset_{},
      line_{},
      column_{} {
  // for (fa::StateId s : transitions_.states()) {
  //   std::cerr << fmt::format("n{}: (", s);
  //   for (std::pair<fa::Symbol, fa::StateId> p : transitions_.map(s)) {
  //     std::cerr << fmt::format(" '{}': n{};", fa::prettySymbol(p.first), p.second);
  //   }
  //   std::cerr << fmt::format(")\n");
  // }
}

void Lexer::open(std::unique_ptr<std::istream> stream) {
  stream_ = std::move(stream);
  oldOffset_ = 0;
  offset_ = 0;
  line_ = 1;
  column_ = 0;
}

constexpr fa::StateId BadState { 101010 }; //static_cast<fa::StateId>(-2);

static std::string stateName(fa::StateId s) {
  switch (s) {
    case BadState:
      return "Bad";
    case ErrorState:
      return "Error";
    default:
      return std::to_string(s);
  }
}

static std::string toString(std::deque<fa::StateId> stack) {
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

fa::Tag Lexer::recognize() {
  // init
  oldOffset_ = offset_;
  word_.clear();
  fa::StateId state = initialStateId_;
  std::deque<fa::StateId> stack;
  stack.push_back(BadState);

  // advance
  while (state != ErrorState) {
    fa::Symbol ch = nextChar();
    word_.push_back(ch);

    if (isAcceptState(state))
      stack.clear();

    stack.push_back(state);
    DEBUG("recognize: state {} char '{}' {}", stateName(state), fa::prettySymbol(ch), isAcceptState(state) ? "accepting" : "");
    state = transitions_.apply(state, ch);
  }

  // trackback
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
    return type(state);

  return -1;
}

int Lexer::type(fa::StateId acceptState) const {
  if (auto i = acceptStates_.find(acceptState); i != acceptStates_.end())
    return i->second;

  return -1;
}

bool Lexer::isAcceptState(fa::StateId id) const {
  return acceptStates_.find(id) != acceptStates_.end();
}

int Lexer::nextChar() {
  if (!buffered_.empty()) {
    offset_++;
    int ch = buffered_.back();
    buffered_.resize(buffered_.size() - 1);
    //std::cerr << fmt::format("Lexer:{}: advance '{}'\n", offset_, fa::prettySymbol(ch));
    return ch;
  }

  if (!stream_->good()) { // EOF or I/O error
    //std::cerr << fmt::format("Lexer:{}: advance '{}'\n", offset_, "EOF");
    return -1;
  }

  int ch = stream_->get();
  if (ch >= 0)
    offset_++;
  //std::cerr << fmt::format("Lexer:{}: advance '{}'\n", offset_, fa::prettySymbol(ch));
  return ch;
}

void Lexer::rollback() {
  if (word_.back() != -1) {
    offset_--;
    buffered_.push_back(word_.back());
  }
}

} // namespace lexer
