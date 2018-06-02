#include <lexer/builder.h>
#include <lexer/fa.h>
#include <lexer/lexer.h>
#include <lexer/regexpr.h>
#include <iostream>

namespace lexer {

void Builder::declare(int id, std::string_view pattern) {
  std::unique_ptr<RegExpr> expr = RegExprParser{}.parse(pattern);
  fa::ThompsonConstruct tc = fa::Generator{}.construct(expr.get());

  if (fa_.empty()) {
    fa_ = std::move(tc);
  } else {
    fa_.alternate(std::move(tc));
  }
}

fa::FiniteAutomaton Builder::buildAutomaton(Stage stage) {
  fa::FiniteAutomaton nfa = fa_.clone().release();
  if (stage == Stage::ThompsonConstruct)
    return std::move(nfa);

  fa::FiniteAutomaton dfa = nfa.deterministic();
  if (stage == Stage::Deterministic)
    return std::move(dfa);

  fa::FiniteAutomaton dfamin = dfa.minimize();
  return std::move(dfamin);
}

Lexer Builder::compile() {
  const fa::FiniteAutomaton dfa = buildAutomaton(Stage::Minimized);
  const Alphabet alphabet = dfa.alphabet();
  TransitionMap transitionMap;

  for (const fa::State* state : dfa.states()) {
    fmt::print("Walking through state {} (with {} links)\n", state->id(), state->transitions().size());
    for (fa::Symbol c : alphabet) {
      if (const fa::State* nextState = state->transition(c); nextState != nullptr) {
        transitionMap.define(state->id(), c, nextState->id());
      }
    }
  }

  std::vector<fa::StateId> acceptStates;
  for (const fa::State* s : dfa.acceptStates())
    acceptStates.push_back(s->id());

  return Lexer{std::move(transitionMap), 0, std::move(acceptStates)};
}

} // namespace lexer
