#include <lexer/fa.h>
#include <sstream>
#include <iostream>

namespace lexer::fa {

// ---------------------------------------------------------------------------
// State

void State::linkTo(Condition condition, State* state) {
  successors_.emplace_back(condition, state);
}

std::list<std::string> State::to_strings() const {
  std::list<std::string> list;

  for (const std::pair<Condition, State*>& succ: successors_) {
    std::string cond = succ.first == EpsilonTransition
        ? fmt::format("{}", "epsilon")
        : fmt::format("{}", static_cast<char>(succ.first));
    list.emplace_back(fmt::format("{}: --({})--> {}", label_, cond, succ.second->label()));
  }

  if (list.empty())
    list.emplace_back(fmt::format("{}:", label_));

  return list;
}

// ---------------------------------------------------------------------------
// FiniteAutomaton

void FiniteAutomaton::relabel(std::string_view prefix) {
  std::set<State*> registry;
  relabel(initialState_, prefix, &registry);
}

void FiniteAutomaton::relabel(State* s, std::string_view prefix, std::set<State*>* registry) {
  s->relabel(fmt::format("{}{}", prefix, registry->size()));
  registry->insert(s);
  for (const std::pair<Condition, State*>& succ : s->successors()) {
    if (registry->find(succ.second) == registry->end()) {
      relabel(succ.second, prefix, registry);
    }
  }
}

std::string FiniteAutomaton::dot() const {
  return dot(states_, initialState_, acceptStates_);
}

std::string FiniteAutomaton::dot(const OwnedStateList& states, State* initialState,
                                 const StateList& acceptStates) {
  std::stringstream sstr;

  sstr << "digraph {\n";
  sstr << "  rankdir=LR;\n";

  // endState
  for (State* endState: acceptStates)
    sstr << "  node [shape=doublecircle]; " << endState->label() << ";\n";

  // startState
  sstr << "  \"\" [shape=plaintext];\n";
  sstr << "  node [shape=circle];\n";
  sstr << "  \"\" -> " << initialState->label() << ";\n";

  // all states and their edges
  for (const std::unique_ptr<State>& state: states) {
    for (const std::pair<Condition, State*>& edge: state->successors()) {
      const Condition cond = edge.first;
      const State* succ = edge.second;

      sstr << "  " << state->label() << " -> " << succ->label();
      if (cond == EpsilonTransition) {
         sstr << " [label=\"ε\"]";
      } else {
         sstr << " [label=\"" << static_cast<char>(cond) << "\"]";
      }
      sstr << ";\n";
    }
  }

  sstr << "}\n";

  return sstr.str();
}

// ---------------------------------------------------------------------------
// ThompsonConstruct

std::string ThompsonConstruct::dot() const {
  return FiniteAutomaton::dot(states_, startState_, {endState_});
}

std::tuple<OwnedStateList, State*, State*> ThompsonConstruct::release() {
  auto t = std::make_tuple(std::move(states_), startState_, endState_);
  startState_ = nullptr;
  endState_ = nullptr;
  return t;
}

State* ThompsonConstruct::createState() {
  static unsigned int n = 0;
  std::string name = fmt::format("n{}", n);
  n++;
  states_.emplace_back(std::make_unique<State>(name));
  return states_.back().get();
}

ThompsonConstruct& ThompsonConstruct::concatenate(ThompsonConstruct rhs) {
  std::cerr << "lhs: " << dot() << "\n";
  std::cerr << "rhs: " << rhs.dot() << "\n";
  endState_->linkTo(rhs.startState_);
  endState_ = rhs.endState_;

  for (std::unique_ptr<State>& s: rhs.states_)
    states_.emplace_back(std::move(s));

  return *this;
}

ThompsonConstruct& ThompsonConstruct::alternate(ThompsonConstruct other) {
  State* newStart = createState();
  newStart->linkTo(startState_);
  newStart->linkTo(other.startState_);

  State* newEnd = createState();
  endState_->linkTo(newEnd);
  other.endState_->linkTo(newEnd);

  startState_ = newStart;
  endState_ = newEnd;

  for (std::unique_ptr<State>& s: other.states_)
    states_.emplace_back(std::move(s));

  return *this;
}

ThompsonConstruct& ThompsonConstruct::repeat(unsigned minimum, unsigned maximum) {
  // TODO

  // cases:
  //   {0,1}
  //   {0,inf}
  //   {1,inf}
  //   {n,n}
  //   {m,n} with m < n

  // a* == a{0,inf}
  if (minimum == 0 && maximum == std::numeric_limits<unsigned>::max()) {
    State* newStart = createState();
    State* newEnd = createState();
    newStart->linkTo(startState_);
    newStart->linkTo(newEnd);
    endState_->linkTo(startState_);
    endState_->linkTo(newEnd);

    startState_ = newStart;
    endState_ = newEnd;
  }

  // for (unsigned i = 0; i < minimum; ++i) {
  //   // a{2} = aa
  //   // a{3} = aa
  //   // a -> a -> a -> s_a
  // }

  return *this;
}

// ---------------------------------------------------------------------------
// Generator

FiniteAutomaton Generator::generate(const RegExpr* re) {
  return FiniteAutomaton{construct(re).release()};
}

ThompsonConstruct Generator::construct(const RegExpr* re) {
  const_cast<RegExpr*>(re)->accept(*this);

  return std::move(fa_);
}

void Generator::visit(AlternationExpr& alternationExpr) {
  ThompsonConstruct lhs = construct(alternationExpr.leftExpr());
  ThompsonConstruct rhs = construct(alternationExpr.rightExpr());
  lhs.alternate(std::move(rhs));
  fa_ = std::move(lhs);
}

void Generator::visit(ConcatenationExpr& concatenationExpr) {
  ThompsonConstruct lhs = construct(concatenationExpr.leftExpr());
  ThompsonConstruct rhs = construct(concatenationExpr.rightExpr());
  lhs.concatenate(std::move(rhs));
  fa_ = std::move(lhs);
}

void Generator::visit(CharacterExpr& characterExpr) {
  ThompsonConstruct fa{characterExpr.value()};

  fa_ = std::move(fa);
}

void Generator::visit(ClosureExpr& closureExpr) {
  ThompsonConstruct fa = construct(closureExpr.subExpr());
  fa.repeat(closureExpr.minimumOccurrences(), closureExpr.maximumOccurrences());

  fa_ = std::move(fa);
}

} // namespace lexer::fa
