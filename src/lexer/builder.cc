#include <lexer/builder.h>
#include <lexer/fa.h>
#include <lexer/lexer.h>
#include <lexer/lexerdef.h>
#include <lexer/regexpr.h>
#include <iostream>

namespace lexer {

void Builder::declare(fa::Tag tag, std::string_view pattern) {
  std::unique_ptr<RegExpr> expr = RegExprParser{}.parse(pattern);
  fa::ThompsonConstruct tc = fa::Generator{}.construct(expr.get());

  tc.acceptState()->setTag(tag);

  if (fa_.empty()) {
    fa_ = std::move(tc);
  } else {
    fa_.alternate(std::move(tc));
  }
}

fa::FiniteAutomaton Builder::buildAutomaton(Stage stage) {
  fa::FiniteAutomaton nfa = fa_.clone().release();
  nfa.renumber();
  if (stage == Stage::ThompsonConstruct)
    return std::move(nfa);

  fa::FiniteAutomaton dfa = nfa.deterministic();
  dfa.renumber();
  if (stage == Stage::Deterministic)
    return std::move(dfa);

  fa::FiniteAutomaton dfamin = dfa.minimize();
  dfamin.renumber();
  return std::move(dfamin);
}

LexerDef Builder::compile() {
  const fa::FiniteAutomaton dfa = buildAutomaton(Stage::Minimized);
  const Alphabet alphabet = dfa.alphabet();
  TransitionMap transitionMap;

  std::cout << fa::dot({lexer::fa::DotGraph{dfa, "n", ""}}, "", true);

  for (const fa::State* state : dfa.states()) {
    std::cerr << fmt::format("Walking through state {} (with {} links)\n", state->id(), state->transitions().size());
    for (fa::Symbol c : alphabet) {
      if (const fa::State* nextState = state->transition(c); nextState != nullptr) {
        transitionMap.define(state->id(), c, nextState->id());
      }
    }
  }

  std::map<fa::StateId, fa::Tag> acceptStates;
  for (const fa::State* s : dfa.acceptStates())
    acceptStates.emplace(s->id(), s->tag());

  return LexerDef{std::move(transitionMap), 0, std::move(acceptStates)};
}

} // namespace lexer
