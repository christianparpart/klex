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

StateSet epsilonClosure(const StateSet& S) {
  StateSet result;

  for (State* s : S) {
    result.insert(s);
    for (Edge& edge : s->successors()) {
      if (edge.first == EpsilonTransition) {
        result.insert(edge.second);
        result.merge(epsilonClosure({edge.second}));
      }
    }
  }

  return result;
}

StateSet delta(const StateSet& q, char c) {
  StateSet result;
  for (State* s : q) {
    for (Edge& edge : s->successors()) {
      if (edge.first == EpsilonTransition) {
        result.merge(delta({edge.second}, c));
      } else if (edge.first == c) {
        result.insert(edge.second);
      }
    }
  }
  return result;
}

struct TransitionTable {
  struct Input {
    State* state;
    char ch;
  };

  void insert(const StateSet& q, char c, const StateSet& t) {
    for (State* s : q)
      transitions.emplace_back(Input{s, c}, t);
  }

  std::list<std::pair<Input, StateSet>> transitions;
};

static bool containsAcceptingState(const StateSet& Q) {
  for (State* q : Q)
    if (q->isAccepting())
      return true;

  return false;
}

State* FiniteAutomaton::createState(std::string label) {
  for (State* s : states())
    if (s->label() == label)
      return s;

  return states_.insert(std::make_unique<State>(label)).first->get();
}

State* FiniteAutomaton::findState(std::string_view label) const {
  for (State* s : states())
    if (s->label() == label)
      return s;

  return nullptr;
}

void FiniteAutomaton::setInitialState(State* s) {
  // TODO: assert (s is having no predecessors)
  initialState_ = s;
}

FiniteAutomaton FiniteAutomaton::minimize() const {
  StateSet q_0 = epsilonClosure({initialState_});
  std::list<StateSet> Q = {q_0};                                  // resulting states
  std::list<StateSet> workList = {q_0};
  TransitionTable T;

  while (!workList.empty()) {
    StateSet q = workList.front();    // each set q represents a valid configuration from the NFA
    workList.pop_front();

    for (char c : alphabet()) {
      StateSet t = epsilonClosure(delta(q, c));

      // T[q][c] = t;

      if (!t.empty()) {
        Q.push_back(t);
        workList.push_back(t);
      }
    }
  }
  // Q now contains all the valid configurations and T all transitions between them

  FiniteAutomaton dfa;
  for (StateSet& q : Q) {
    const bool q_containsAcceptingState = containsAcceptingState(q);
    for (State* q_i : q) {
      State* d_i = dfa.createState(q_i->label());

      // if q contains an accepting state, then d is an accepting state in the DFA
      if (q_containsAcceptingState)
        d_i->setAccept(true);

      // TODO: observe mapping from q_i to d_i
      //...
    }
  }

  // q_0 becomes d_0 (initial state)
  dfa.setInitialState(dfa.findState(initialState_->label()));

  return dfa;
}

std::string FiniteAutomaton::dot() const {
  return dot(states_, initialState_, acceptStates_);
}

std::string FiniteAutomaton::dot(const OwnedStateSet& states, State* initialState,
                                 const StateSet& acceptStates) {
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
  return states_.insert(std::make_unique<State>(name)).first->get();
}

ThompsonConstruct& ThompsonConstruct::concatenate(ThompsonConstruct rhs) {
  endState_->linkTo(rhs.startState_);
  endState_ = rhs.endState_;
  states_.merge(std::move(rhs.states_));

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

  states_.merge(std::move(other.states_));

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
