#include <lexer/lexer.h>
#include <algorithm>
#include <deque>
#include <iostream>

namespace lexer {

Lexer::Lexer(TransitionMap transitions, fa::StateId initialStateId, std::vector<fa::StateId> acceptStates)
    : transitions_{std::move(transitions)},
      initialStateId_{initialStateId},
      acceptStates_{std::move(acceptStates)},
      lexeme_{},
      stream_{},
      line_{},
      column_{} {
  for (fa::StateId s : transitions_.states()) {
    std::cerr << fmt::format("n{}: (", s);
    for (std::pair<fa::Symbol, fa::StateId> p : transitions_.map(s)) {
      std::cerr << fmt::format(" '{}': n{};", p.first, p.second);
    }
    std::cerr << fmt::format(")\n");
  }
}

void Lexer::open(std::unique_ptr<std::istream> stream) {
  stream_ = std::move(stream);
  offset_ = 0;
  line_ = 1;
  column_ = 0;
}

int Lexer::recognize() {
  // init
  lexeme_.clear();
  fa::StateId state = initialStateId_;
  std::deque<fa::StateId> stack;
  stack.push_front(BadState);

  // advance
  while (state != ErrorState) {
    fa::Symbol ch = nextChar();
    lexeme_.push_back(ch);

    if (isAcceptState(state))
      stack.clear();

    stack.push_front(state);
    state = transitions_.apply(state, ch);
  }

  // trackback
  while (state != BadState && !isAcceptState(state)) {
    state = stack.front();
    stack.pop_front();
    rollback();
    lexeme_.resize(lexeme_.size() - 1);
  }

  if (isAcceptState(state))
    return type(state);

  return -1;
}

int Lexer::type(fa::StateId acceptState) const {
  if (acceptState != ErrorState)
    return acceptState; // TODO: map state to actual input enum

  return -1;
}

bool Lexer::isAcceptState(fa::StateId id) const {
  return std::find(acceptStates_.begin(), acceptStates_.end(), id) != acceptStates_.end();
}

int Lexer::nextChar() {
  offset_++;
  if (!buffered_.empty()) {
    int ch = buffered_.back();
    buffered_.resize(buffered_.size() - 1);
    std::cerr << fmt::format("Lexer:{}: advance '{}'\n", offset_, (char)ch);
    return ch;
  }

  if (!stream_->good()) // EOF
    return -1;

  int ch = stream_->get();
  std::cerr << fmt::format("Lexer:{}: advance '{}'\n", offset_, (char)ch);
  return ch;
}

void Lexer::rollback() {
  offset_--;
  buffered_.push_back(lexeme_.back());
}

} // namespace lexer
