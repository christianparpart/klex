#include <lexer/fa.h>

namespace lexer::fa {

void State::linkSuccessor(Condition condition, State* state) {
  successors_[condition] = state;
}

std::unique_ptr<FiniteAutomaton> Generator::generate(const RegExpr* re) {
  fa_ = std::make_unique<FiniteAutomaton>();

  // TODO

  return std::move(fa_);
}

void Generator::visit(AlternationExpr& alternationExpr) {
}

void Generator::visit(ConcatenationExpr& concatenationExpr) {
  std::unique_ptr<FiniteAutomaton> lhs = generate(concatenationExpr.leftExpr());
  std::unique_ptr<FiniteAutomaton> rhs = generate(concatenationExpr.rightExpr());

  for (State* l: lhs->terminationStates()) {
    l->linkSuccessor(EpsilonTransition, rhs.startState());
  }
}

void Generator::visit(CharacterExpr& characterExpr) {
  State* start = createState();
  State* end = createState();
  start->linkSuccessor(characterExpr.value(), end);

  state_ = start;
}

void Generator::visit(ClosureExpr& closureExpr) {
}

} // namespace lexer::fa
