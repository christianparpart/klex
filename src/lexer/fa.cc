#include <lexer/fa.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <deque>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

namespace lexer::fa {

// ---------------------------------------------------------------------------
// State

State* State::transition(Symbol input) const {
  for (const Edge& transition : transitions_)
    if (input == transition.symbol)
      return transition.state;

  return nullptr;
}

void State::linkTo(Symbol input, State* state) {
  transitions_.emplace_back(input, state);
}

std::string prettySymbol(Symbol input) {
  if (input == EpsilonTransition)
    return "ε";
  else
    return fmt::format("{}", input);
}

std::list<std::string> State::to_strings() const {
  std::list<std::string> list;

  for (const Edge& transition: transitions_) {
    list.emplace_back(fmt::format("{}: --({})--> {}", label_,
                                                      prettySymbol(transition.symbol),
                                                      transition.state->label()));
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

FiniteAutomaton& FiniteAutomaton::operator=(const FiniteAutomaton& other) {
  states_.clear();
  for (State* s : other.states())
    createState(s->label())->setAccept(s->isAccepting());

  // map links
  for (const std::unique_ptr<State>& s : other.states_)
    for (const Edge& t : s->transitions())
      findState(s->label())->linkTo(t.symbol, findState(t.state->label()));

  initialState_ = findState(other.initialState()->label());

  return *this;
}

void FiniteAutomaton::relabel(std::string_view prefix) {
  std::set<State*> registry;
  relabel(initialState_, prefix, &registry);
}

Alphabet FiniteAutomaton::alphabet() const {
  Alphabet alphabet;
  for (const State* state : states()) {
    for (const Edge& transition : state->transitions()) {
      if (transition.symbol != EpsilonTransition) {
        alphabet.insert(transition.symbol);
      }
    }
  }
  return alphabet;
}

void FiniteAutomaton::relabel(State* s, std::string_view prefix, std::set<State*>* registry) {
  s->relabel(fmt::format("{}{}", prefix, registry->size()));
  registry->insert(s);
  for (const Edge& transition : s->transitions()) {
    if (registry->find(transition.state) == registry->end()) {
      relabel(transition.state, prefix, registry);
    }
  }
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
      throw std::invalid_argument{label};

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
  REGEX:      a(b|c)*

  NFA:        n0 --(a)--> n1 --> n2 -----------------------------------> "n7"
                                  \                                       ^
                                   \---> n3 <------------------------    /
                                         \ \                         \  /
                                          \ \----> n4 --(b)--> n5 --> n6
                                           \                          ^
                                            \----> n8 --(c)--> n9 ---/

  DFA:
                                            <---
              d0 --(a)--> "d1" ----(b)--> "d2"--(b)
                             \             |^
                              \         (c)||(b)
                               \           v|
                                \--(c)--> "d3"--(c)
                                            <---


  TABLE:

    set   | DFA   | NFA                 |
    name  | state | state               | 'a'                 | 'b'                 | 'c'
    --------------------------------------------------------------------------------------------------------
    q0    | d0    | {n0}                | {n1,n2,n3,n4,n7,n8} | -none-              | -none-
    q1    | d1    | {n1,n2,n3,n4,n7,n8} | -none-              | {n3,n4,n5,n6,n7,n8} | {n3,n4,n6,n7,n8,n9}
    q2    | d2    | {n3,n4,n5,n6,n7,n8} | -none-              | q2                  | q3
    q3    | d3    | {n3,n4,n6,n7,n8,n9} | -none-              | q2                  | q3

 */

FiniteAutomaton FiniteAutomaton::deterministic() const {
  // Builds a list of states that can be exclusively reached from S via epsilon-transitions.
  std::function<StateSet(const StateSet&)> epsilonClosure = [&epsilonClosure](const StateSet& S) -> StateSet {
    StateSet result;

    for (State* s : S) {
      result.insert(s);
      for (Edge& transition : s->transitions()) {
        if (transition.symbol == EpsilonTransition) {
          result.merge(epsilonClosure({transition.state}));
        }
      }
    }

    return result;
  };

  // Computes a valid configuration the FA can reach with the given input @p q and @p c.
  // 
  // @param q valid input configuration of the original NFA.
  // @param c the input character that the FA would consume next
  //
  // @return set of states that the FA can reach from @p c given the input @p c.
  //
  std::function<StateSet(const StateSet&, Symbol)> delta = [&delta](const StateSet& q, Symbol c) -> StateSet {
    StateSet result;
    for (State* s : q) {
      for (Edge& transition: s->transitions()) {
        if (transition.symbol == EpsilonTransition) {
          result.merge(delta({transition.state}, c));
        } else if (transition.symbol == c) {
          result.insert(transition.state);
        }
      }
    }
    return result;
  };

  // Finds @p t in @p Q and returns its offset (aka configuration number) or -1 if not found.
  auto configurationNumber = [](const std::vector<StateSet>& Q, const StateSet& t) -> int {
    int i = 0;
    for (const StateSet& q_i : Q) {
      if (q_i == t) {
        return i;
      }
      i++;
    }

    return -1;
  };

  auto containsAcceptingState = [](const StateSet& Q) -> bool {
    for (State* q : Q)
      if (q->isAccepting())
        return true;

    return false;
  };

  StateSet q_0 = epsilonClosure({initialState_});
  DEBUG("q_0 = epsilonClosure({}) = {}", to_string({initialState_}), q_0);
  std::vector<StateSet> Q = {q_0};          // resulting states
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

      int t_i = configurationNumber(Q, t);

      if (!dbg.str().empty()) dbg << ", ";
      if (t_i != -1) {
        dbg << prettySymbol(c) << ": q" << t_i;
      } else if (t.empty()) {
        dbg << prettySymbol(c) << ": none";
      } else {
        Q.push_back(t);
        workList.push_back(t);
        t_i = configurationNumber(Q, t);
        dbg << prettySymbol(c) << ": " << to_string(t);
      }
      T.insert(q_i, c, t_i); // T[q][c] = t;
    }
    DEBUG(" q{:<7} | d{:<13} | {:24} | {}", q_i, q_i, to_string(q), dbg.str());
  }
  // Q now contains all the valid configurations and T all transitions between them

  FiniteAutomaton dfa;

  // map q_i to d_i and flag accepting states
  int q_i = 0;
  for (StateSet& q : Q) {
    // d_i represents the corresponding state in the DFA for all states of q from the NFA
    std::string id = fmt::format("d{}", q_i);
    State* d_i = dfa.createState(id);

    // if q contains an accepting state, then d is an accepting state in the DFA
    if (containsAcceptingState(q))
      d_i->setAccept(true);

    q_i++;
  }

  // observe mapping from q_i to d_i
  for (const std::pair<TransitionTable::Input, int>& transition: T.transitions) {
    const int q_i = transition.first.configurationNumber;
    const Symbol c = transition.first.symbol;
    const int t_i = transition.second;
    if (t_i != -1) {
      DEBUG("map d{} |--({})--> d{}", q_i, c, t_i);
      State* q = dfa.findState(fmt::format("d{}", q_i));
      State* t = dfa.findState(fmt::format("d{}", t_i));

      q->linkTo(c, t);
    }
  }

  // q_0 becomes d_0 (initial state)
  dfa.setInitialState(dfa.findState("d0"));

  return dfa;
}

FiniteAutomaton FiniteAutomaton::minimize() const {
  auto nonAcceptStates = [&]() -> StateSet {
    StateSet result;

    for (State* s : states())
      if (!s->isAccepting())
        result.insert(s);

    return result;
  };

  std::list<StateSet> T = {acceptStates(), nonAcceptStates()};
  std::list<StateSet> P = {};

  auto partitionId = [&](State* s) -> int {
    if (s != nullptr) {
      int i = 0;
      for (const StateSet& p : P) {
        if (p.find(s) != p.end())
          return i;
        else
          i++;
      }
    }
    return -1;
  };

  auto containsInitialState = [this](const StateSet& S) -> bool {
    for (State* s : S)
      if (s == initialState_)
        return true;
    return false;
  };

  auto split = [&](const StateSet& S) -> std::list<StateSet> {
    DEBUG("split: {}", to_string(S));

    for (Symbol c : alphabet()) {
      // if c splits S into s_1 and s_2
      //      that is, phi(s_1, c) and phi(s_2, c) reside in two different p_i's (partitions)
      // then return {s_1, s_2}

      std::map<int /*target partition set*/ , StateSet /*source states*/> t_i;
      for (State* s : S) {
        State* t = s->transition(c);
        int p_i = partitionId(t);
        t_i[p_i].insert(s);
      }
      if (t_i.size() != 1) {
        DEBUG("  split: on character '{}' into {} sets", (char)c, t_i.size());
        std::list<StateSet> result;
        for (const std::pair<int, StateSet>& t : t_i) {
          result.emplace_back(std::move(t.second));
        }
        return result;
      }
    }
    return {S};
  };

  while (P != T) {
    P = std::move(T);
    T = {};

    for (StateSet& p : P) {
      T.splice(T.end(), split(p));
    }
  }

  // -------------------------------------------------------------------------
  DEBUG("minimization terminated with {} unique partition sets", P.size());
  FiniteAutomaton dfamin;

  // instanciate states
  int p_i = 0;
  for (const StateSet& p : P) {
    State* s = *p.begin();
    State* q = dfamin.createState(fmt::format("p{}", p_i));
    q->setAccept(s->isAccepting());
    DEBUG("Creating p{}: {} {}", p_i, s->isAccepting() ? "accepting" : "rejecting",
                                      containsInitialState(p) ? "initial" : "");
    if (containsInitialState(p)) {
      dfamin.setInitialState(q);
    }
    p_i++;
  }

  // setup transitions
  p_i = 0;
  for (const StateSet& p : P) {
    State* s = *p.begin();
    State* t0 = dfamin.findState(fmt::format("p{}", p_i));
    for (const Edge& transition : s->transitions()) {
      if (int t_i = partitionId(transition.state); t_i != -1) {
        DEBUG("map p{} --({})--> p{}", p_i, transition.symbol, t_i);
        State* t1 = dfamin.findState(fmt::format("p{}", t_i));
        t0->linkTo(transition.symbol, t1);
      }
    }
    p_i++;
  }

  // TODO
  // construct states & links out of P

  return dfamin;
}

std::string FiniteAutomaton::dot(const std::string_view& label) const {
  return dot(label, states_, initialState_);
}

StateSet FiniteAutomaton::acceptStates() const {
  StateSet result;

  for (State* s : states())
    if (s->isAccepting())
      result.insert(s);

  return result;
}

std::string groupCharacterClassRanges(std::vector<Symbol> chars) {
  // we took a copy in tgroup here, so I can sort() later
  std::sort(chars.begin(), chars.end());

  // {1,3,5,a,b,c,d,e,f,z]
  // ->
  // {{1}, {3}, {5}, {a-f}, {z}}

  std::stringstream sstr;
  Symbol ymin = '\0';
  Symbol ymax = ymin;
  int i = 0;

  for (Symbol c : chars) {
    if (c == ymax + 1) {  // range growing
      ymax = c;
    }
    else { // gap found
      if (i) {
        if (ymin != ymax)
          sstr << prettySymbol(ymin) << '-' << prettySymbol(ymax);
        else
          sstr << prettySymbol(ymin);
      }
      ymin = ymax = c;
    }
    i++;
  }
  if (ymin != ymax)
    sstr << prettySymbol(ymin) << '-' << prettySymbol(ymax);
  else
    sstr << prettySymbol(ymin);

  return sstr.str();
}

std::string FiniteAutomaton::dot(const std::string_view& label,
                                 const OwnedStateSet& states,
                                 State* initialState) {
  std::stringstream sstr;

  sstr << "digraph {\n";
  sstr << "  rankdir=LR;\n";
  sstr << "  label=\"" << label << "\";\n";

  // acceptState
  for (const std::unique_ptr<State>& s: states)
    if (s->isAccepting())
      sstr << "  node [shape=doublecircle]; " << s->label() << ";\n";

  // initialState
  sstr << "  \"\" [shape=plaintext];\n";
  sstr << "  node [shape=circle];\n";
  sstr << "  \"\" -> " << initialState->label() << ";\n";

  // all states and their edges
  for (const std::unique_ptr<State>& state: states) {
    std::map<State* /*target state*/, std::vector<Symbol> /*transition symbols*/> transitionGroups;

    for (const Edge& transition: state->transitions())
      transitionGroups[transition.state].emplace_back(transition.symbol);

    for (const std::pair<State*, std::vector<Symbol>>& tgroup: transitionGroups) {
      std::string label = groupCharacterClassRanges(tgroup.second);
      const State* targetState = tgroup.first;
      sstr << "  " << state->label() << " -> " << targetState->label();
      sstr << " [label=\"" << label << "\"]";
      sstr << ";\n";
    }
  }

  sstr << "}\n";

  return sstr.str();
}

// ---------------------------------------------------------------------------
// ThompsonConstruct

ThompsonConstruct ThompsonConstruct::clone() const {
  ThompsonConstruct output;

  // clone states
  for (const std::unique_ptr<State>& s : states_) {
    State* u = output.createState(s->label());
    u->setAccept(s->isAccepting());
    if (s.get() == initialState()) {
      output.initialState_ = u;
    }
  }

  // map links
  for (const std::unique_ptr<State>& s : states_) {
    State* u = output.findState(s->label());
    for (const Edge& transition : s->transitions()) {
      State* v = output.findState(transition.state->label());
      u->linkTo(transition.symbol, v);
      // findState(s->label())->linkTo(t.symbol, findState(t.state->label()));
    }
  }

  return output;
}

State* ThompsonConstruct::acceptState() const {
  for (const std::unique_ptr<State>& s : states_)
    if (s->isAccepting())
      return s.get();

  fprintf(stderr, "Internal Bug! Thompson's Consruct without an end state.\n");
  abort();
}

std::string ThompsonConstruct::dot(const std::string_view& label) const {
  return FiniteAutomaton::dot(label, states_, initialState_);
}

FiniteAutomaton ThompsonConstruct::release() {
  auto t = std::make_tuple(std::move(states_), initialState_);
  initialState_ = nullptr;
  return FiniteAutomaton{std::move(t)};
}

State* ThompsonConstruct::createState() {
  static unsigned int n = 0;
  for (;;) {
    std::string name = fmt::format("n{}", n);
    n++;
    if (!findState(name)) {
      return states_.insert(std::make_unique<State>(name)).first->get();
    }
  }
}

State* ThompsonConstruct::createState(std::string name) {
  if (findState(name) != nullptr)
    throw std::invalid_argument{name};

  return states_.insert(std::make_unique<State>(std::move(name))).first->get();
}

State* ThompsonConstruct::findState(std::string_view label) const {
  for (const std::unique_ptr<State>& s : states_)
    if (s->label() == label)
      return s.get();

  return nullptr;
}

ThompsonConstruct& ThompsonConstruct::concatenate(ThompsonConstruct rhs) {
  acceptState()->linkTo(rhs.initialState_);
  acceptState()->setAccept(false);

  // relabel first with given base
  if (int n = states_.size(); n != 0) {
    for (const std::unique_ptr<State>& s : rhs.states_) {
      s->relabel(fmt::format("n{}", n));
      n++;
    }
  }

  states_.merge(std::move(rhs.states_));

  return *this;
}

ThompsonConstruct& ThompsonConstruct::alternate(ThompsonConstruct other) {
  State* newStart = createState();
  newStart->linkTo(initialState_);
  newStart->linkTo(other.initialState_);

  State* newEnd = createState();
  acceptState()->linkTo(newEnd);
  other.acceptState()->linkTo(newEnd);

  initialState_ = newStart;
  acceptState()->setAccept(false);
  other.acceptState()->setAccept(false);
  newEnd->setAccept(true);

  // relabel first with given base
  if (int n = states_.size(); n != 0) {
    for (const std::unique_ptr<State>& s : other.states_) {
      s->relabel(fmt::format("n{}", n));
      n++;
    }
  }

  states_.merge(std::move(other.states_));

  return *this;
}

ThompsonConstruct& ThompsonConstruct::optional() {
  State* newStart = createState();
  State* newEnd = createState();

  newStart->linkTo(initialState_);
  newStart->linkTo(newEnd);
  acceptState()->linkTo(newEnd);

  initialState_ = newStart;
  acceptState()->setAccept(false);
  newEnd->setAccept(true);

  return *this;
}

ThompsonConstruct& ThompsonConstruct::recurring() {
  // {0, inf}
  State* newStart = createState();
  State* newEnd = createState();
  newStart->linkTo(initialState_);
  newStart->linkTo(newEnd);
  acceptState()->linkTo(initialState_);
  acceptState()->linkTo(newEnd);

  initialState_ = newStart;
  acceptState()->setAccept(false);
  newEnd->setAccept(true);

  return *this;
}

ThompsonConstruct& ThompsonConstruct::positive() {
  return concatenate(std::move(clone().recurring()));
}

ThompsonConstruct& ThompsonConstruct::times(unsigned factor) {
  assert(factor != 0);

  if (factor > 1) {
    ThompsonConstruct base = clone();
    for (unsigned n = 2; n <= factor; ++n) {
      concatenate(base.clone());
      std::cerr << dot(fmt::format("{} times factorized", n)) << "\n";
    }
  }

  return *this;
}

ThompsonConstruct& ThompsonConstruct::repeat(unsigned minimum, unsigned maximum) {
  assert(minimum <= maximum);

  ThompsonConstruct factor = clone();

  times(minimum);
  for (unsigned n = minimum + 1; n <= maximum; n++)
    alternate(std::move(factor.clone().times(n)));

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
  fa_ = ThompsonConstruct{characterExpr.value()};
}

void Generator::visit(ClosureExpr& closureExpr) {
  const unsigned xmin = closureExpr.minimumOccurrences();
  const unsigned xmax = closureExpr.maximumOccurrences();
  constexpr unsigned Infinity = std::numeric_limits<unsigned>::max();

  if (xmin == 0 && xmax == 1)
    fa_ = std::move(construct(closureExpr.subExpr()).optional());
  else if (xmin == 0 && xmax == Infinity)
    fa_ = std::move(construct(closureExpr.subExpr()).recurring());
  else if (xmin == 1 && xmax == Infinity)
    fa_ = std::move(construct(closureExpr.subExpr()).positive());
  else if (xmin < xmax)
    fa_ = std::move(construct(closureExpr.subExpr()).repeat(xmin, xmax));
  else if (xmin == xmax)
    fa_ = std::move(construct(closureExpr.subExpr()).times(xmin));
  else
    throw std::invalid_argument{"closureExpr"};
}

} // namespace lexer::fa
