#include <klex/builder.h>
#include <klex/fa.h>
#include <klex/lexer.h>
#include <klex/lexerdef.h>
#include <klex/regexpr.h>
#include <iostream>

namespace klex {

void Builder::declare(fa::Tag tag, std::string_view pattern) {
  std::unique_ptr<RegExpr> expr = RegExprParser{}.parse(pattern);
  fa::FiniteAutomaton nfa = fa::Generator{}.generate(expr.get());
  fa::FiniteAutomaton dfa = nfa.deterministic();
  fa::FiniteAutomaton mfa = dfa.minimize();

  // std::cerr << fmt::format("Builder.declare: prio {}, tag {} RE {}\n", nextPriority_, tag, pattern);
  for (fa::State* s : mfa.acceptStates()) {
    s->setTag(tag);
    s->setPriority(nextPriority_);
  }

  nextPriority_++;

  fa::ThompsonConstruct tc { std::move(mfa) };

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

LexerDef Builder::compile() {
  const fa::FiniteAutomaton dfa = buildAutomaton(Stage::Deterministic);
  const Alphabet alphabet = dfa.alphabet();
  TransitionMap transitionMap;

  //std::cout << fa::dot({klex::fa::DotGraph{dfa, "n", ""}}, "", true);

  for (const fa::State* state : dfa.states()) {
    //std::cerr << fmt::format("Walking through state {} (with {} links)\n", state->id(), state->transitions().size());
    for (fa::Symbol c : alphabet) {
      if (const fa::State* nextState = state->transition(c); nextState != nullptr) {
        transitionMap.define(state->id(), c, nextState->id());
      }
    }
  }

  std::map<fa::StateId, fa::Tag> acceptStates;
  for (const fa::State* s : dfa.acceptStates())
    acceptStates.emplace(s->id(), s->tag());

  return LexerDef{std::move(transitionMap), dfa.initialState()->id(), std::move(acceptStates)};
}

} // namespace klex
