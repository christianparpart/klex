#include <lexer/fa.h>
#include <sstream>
#include <deque>
#include <iostream>

namespace lexer::fa {

// ---------------------------------------------------------------------------
// State

void State::linkTo(Condition condition, State* state) {
  successors_.emplace_back(condition, state);
}

std::list<std::string> State::to_strings() const {
  std::list<std::string> list;

  for (const Edge& succ: successors_) {
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

Alphabet FiniteAutomaton::alphabet() const {
  Alphabet alphabet;
  for (const State* state : states()) {
    for (const Edge& edge : state->successors()) {
      alphabet.insert(edge.first);
    }
  }
  return alphabet;
}

void FiniteAutomaton::relabel(State* s, std::string_view prefix, std::set<State*>* registry) {
  s->relabel(fmt::format("{}{}", prefix, registry->size()));
  registry->insert(s);
  for (const Edge& succ : s->successors()) {
    if (registry->find(succ.second) == registry->end()) {
      relabel(succ.second, prefix, registry);
    }
  }
}

std::unique_ptr<FiniteAutomaton> FiniteAutomaton::minimize() const {
#if 1==0
  auto epsilonClosure(StateList S) -> StateList {
  };
  auto delta(StateList q, char c) -> StateList {
  };

  StateList q_0 = epsilonClosure({initialState_});
  StateList Q = q_0;
  std::deque<StateList> workList = {q_0};
  TransitionTable T;
  Alphabet alphas = alphabet();

  for (char c : alphas) {
    StateList q = workList.front();
    workList.pop_front();

    StateList t = epsilonClosure(delta(q, c));

    T[q][c] = t;

    if (!t.empty()) {
      Q.push_back(t);
      workList.push_back(t);
    }
  }

  // next: create states and their edges based on the above output
#else
  return nullptr;
#endif
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
    for (const Edge& edge: state->successors()) {
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

FiniteAutomaton ThompsonConstruct::release() {
  auto t = std::make_tuple(std::move(states_), startState_, endState_);
  startState_ = nullptr;
  endState_ = nullptr;
  return FiniteAutomaton{std::move(t)};
}

State* ThompsonConstruct::createState() {
  static unsigned int n = 0;
  std::string name = fmt::format("n{}", n);
  n++;
  states_.emplace_back(std::make_unique<State>(name));
  return states_.back().get();
}

ThompsonConstruct& ThompsonConstruct::concatenate(ThompsonConstruct rhs) {
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
