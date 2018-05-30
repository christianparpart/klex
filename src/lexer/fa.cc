#include <lexer/fa.h>

#include <algorithm>
#include <deque>
#include <iostream>
#include <sstream>
#include <vector>

#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)

namespace lexer::fa {

// ---------------------------------------------------------------------------
// State

void State::linkTo(Symbol input, State* state) {
  successors_.emplace_back(input, state);
}

std::string prettySymbol(Symbol input) {
  if (input == EpsilonTransition)
    return "ε";
  else
    return fmt::format("{}", input);
}

std::list<std::string> State::to_strings() const {
  std::list<std::string> list;

  for (const Edge& succ: successors_) {
    list.emplace_back(fmt::format("{}: --({})--> {}", label_,
                                                      prettySymbol(succ.first),
                                                      succ.second->label()));
  }

  if (list.empty())
    list.emplace_back(fmt::format("{}:", label_));

  return list;
}

std::string to_string(const StateSet& S) {
  std::vector<std::string> names;
  for (const State* s : S)
    names.push_back(s->label());

  std::sort(names.begin(), names.end());

  std::stringstream sstr;
  sstr << "{";
  int i = 0;
  for (const std::string& name : names) {
    if (i)
      sstr << ", ";
    sstr << name;
    i++;
  }
  sstr << "}";

  return sstr.str();
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
      if (edge.first != EpsilonTransition) {
        alphabet.insert(edge.first);
      }
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
    //result.insert(s);
    for (Edge& edge : s->successors()) {
      if (edge.first == EpsilonTransition) {
        result.insert(edge.second);
        result.merge(epsilonClosure({edge.second}));
      }
    }
  }

  return result;
}

StateSet delta(const StateSet& q, Symbol c) {
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

static bool containsAcceptingState(const StateSet& Q) {
  for (State* q : Q)
    if (q->isAccepting())
      return true;

  return false;
}

State* FiniteAutomaton::createState() {
  static unsigned int n = 10000;
  std::string name = fmt::format("n{}", n);
  n++;
  return createState(name);
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

//! Finds @p t in @p Q and returns its offset (aka configuration number) or -1 if not found.
int configurationNumber(const std::deque<StateSet>& Q, const StateSet& t) {
  int i = 0;
  for (const StateSet& q_i : Q) {
    if (q_i == t) {
      return i;
    }
    i++;
  }

  return -1;
}

struct TransitionTable {
  struct Input {
    int configurationNumber;
    Symbol symbol;
    int targetNumber;
  };

  void insert(int q, Symbol c, int t) {
    auto i = std::find_if(transitions.begin(), transitions.end(),
                          [=](const auto& input) {
        return input.first.configurationNumber == q && input.first.symbol == c;
    });
    if (i == transitions.end()) {
      transitions.emplace_back(Input{q, c}, t);
    } else {
      DEBUG("TransitionTable[q{}][{}] = q{}; already present", q, prettySymbol(c), t);
    }
  }

  std::list<std::pair<Input, int>> transitions;
};

/*
  REGEX:      a | b

  NFA:        n0 --> n1 --(a)--> n2  --> n3
               \                     /
                \--> n4 --(b)--> n5 /

  DFA:        n0 --(a)-----> n1
               \         /
                \--(b)--/

  TABLE:
    set name | DFA state | NFA states | "a" | "b" 
 */

FiniteAutomaton FiniteAutomaton::deterministic() const {
  StateSet q_0 = epsilonClosure({initialState_});
  DEBUG("q_0 = epsilonClosure({}) = {}", to_string({initialState_}), q_0);
  std::deque<StateSet> Q = {q_0};          // resulting states
  std::deque<StateSet> workList = {q_0};
  TransitionTable T;

  DEBUG(" {:<8} | {:<14} | {:<24} | {:<}", "set name", "DFA state", "NFA states", "ε-closures(q, *)");
  DEBUG("{}", "------------------------------------------------------------------------");

  while (!workList.empty()) {
    StateSet q = workList.front();    // each set q represents a valid configuration from the NFA
    workList.pop_front();
    const int q_i = configurationNumber(Q, q);

    std::stringstream dbg;
    for (Symbol c : alphabet()) {
      StateSet t = epsilonClosure(delta(q, c));

      // [[maybe_unused]] StateSet _delta = delta(q, c);
      // DEBUG("  delta({}, '{}') = {}", q, prettySymbol(c), _delta);
      // DEBUG("  epsilonClosure({}) = {}", _delta, t);

      int t_i = configurationNumber(Q, t);
      if (t_i == -1) {
        Q.push_back(t);
        workList.push_back(t);
        t_i = configurationNumber(Q, t);
        dbg << prettySymbol(c) << ": " << to_string(t) << ", ";
      } else {
        dbg << prettySymbol(c) << ": none, ";
      }

      //DEBUG("  T[q{}][{}] = q{}", q_i, prettySymbol(c), t_i);
      T.insert(q_i, c, t_i); // T[q][c] = t;
    }
    DEBUG(" q{:<7} | d{:<13} | {:24} | {}", q_i, q_i, to_string(q), dbg.str());
  }
  // Q now contains all the valid configurations and T all transitions between them

  FiniteAutomaton dfa;

  // map q_i to d_i and flag accepting states
  for (StateSet& q : Q) {
    // d_i represents the corresponding state in the DFA for all states of q from the NFA
    State* d_i = dfa.createState();

    // if q contains an accepting state, then d is an accepting state in the DFA
    if (containsAcceptingState(q))
      d_i->setAccept(true);
  }

  // observe mapping from q_i to d_i
  for (const std::pair<TransitionTable::Input, int>& t: T.transitions) {
    const int q_i = t.first.configurationNumber;
    const Symbol c = t.first.symbol;
    const int t_i = t.second;
    DEBUG("map n{} |--({})--> d{}", q_i, c, t_i);

    //q->linkTo(c, t);
  }

  // q_0 becomes d_0 (initial state)
  dfa.setInitialState(dfa.findState(initialState_->label()));
  //dfa.setInitialState("n0");

  return dfa;
}

std::string FiniteAutomaton::dot(const std::string_view& label) const {
  return dot(label, states_, initialState_, acceptStates_);
}

std::string FiniteAutomaton::dot(const std::string_view& label,
                                 const OwnedStateSet& states,
                                 State* initialState,
                                 const StateSet& acceptStates) {
  std::stringstream sstr;

  sstr << "digraph {\n";
  sstr << "  rankdir=LR;\n";
  sstr << "  label=\"" << label << "\";\n";

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
      const Symbol input = edge.first;
      const State* succ = edge.second;

      sstr << "  " << state->label() << " -> " << succ->label();
      sstr << " [label=\"" << prettySymbol(input) << "\"]";
      sstr << ";\n";
    }
  }

  sstr << "}\n";

  return sstr.str();
}

// ---------------------------------------------------------------------------
// ThompsonConstruct

std::string ThompsonConstruct::dot(const std::string_view& label) const {
  return FiniteAutomaton::dot(label, states_, startState_, {endState_});
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
