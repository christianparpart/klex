// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/fa.h>

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

namespace klex::fa {

// ---------------------------------------------------------------------------
// utils

std::string prettySymbol(Symbol input, bool dot) {
  switch (input) {
    case -1:
      return "<EOF>";
    case EpsilonTransition:
      return "ε";
    case ' ':
      if (dot)
        return "\\\\s";
      else
        return "\\s";
    case '\t':
      if (dot)
        return "\\\\t";
      else
        return "\\t";
    case '\n':
      if (dot)
        return "\\\\n";
      else
        return "\\n";
    default:
      return fmt::format("{}", input);
  }
}

std::string _groupCharacterClassRanges(std::vector<Symbol> chars) {
  std::sort(chars.begin(), chars.end());
  std::stringstream sstr;
  for (Symbol c : chars)
    sstr << prettySymbol(c);
  return sstr.str();
}

static std::string prettyCharRange(Symbol ymin, Symbol ymax, bool dot) {
  std::stringstream sstr;
  switch (std::abs(ymax - ymin)) {
    case 0:
      sstr << prettySymbol(ymin, dot);
      break;
    case 1:
      sstr << prettySymbol(ymin, dot)
           << prettySymbol(ymin + 1, dot);
      break;
    case 2:
      sstr << prettySymbol(ymin, dot)
           << prettySymbol(ymin + 1, dot)
           << prettySymbol(ymax, dot);
      break;
    default:
      sstr << prettySymbol(ymin, dot) << '-' << prettySymbol(ymax, dot);
      break;
  }
  return sstr.str();
}

std::string groupCharacterClassRanges(std::vector<Symbol> chars, bool dot) {
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
        sstr << prettyCharRange(ymin, ymax, dot);
      }
      ymin = ymax = c;
    }
    i++;
  }
  sstr << prettyCharRange(ymin, ymax, dot);

  return sstr.str();
}

template<typename StringType>
static std::string escapeString(const StringType& str) {
  std::stringstream sstr;
  for (char ch : str) {
    switch (ch) {
      case '\r':
        sstr << "\\r";
        break;
      case '\n':
        sstr << "\\n";
        break;
      case '\t':
        sstr << "\\t";
        break;
      case '"':
        sstr << "\\\"";
        break;
      case ' ':
        sstr << ' ';
        break;
      default:
        if (std::isprint(ch)) {
          sstr << ch;
        } else {
          sstr << fmt::format("\\x{:02x}", ch);
        }
    }
  }
  return sstr.str();
}

std::string dot(std::list<DotGraph> graphs, std::string_view label, bool groupEdges) {
  std::stringstream sstr;

  sstr << "digraph {\n";

  int clusterId = 0;
  sstr << "  rankdir=LR;\n";
  sstr << "  label=\"" << escapeString(label) << "\";\n";
  for (const DotGraph& graph : graphs) {
    sstr << "  subgraph cluster_" << clusterId << " {\n";
    sstr << "    label=\"" << escapeString(graph.graphLabel) << "\";\n";
    clusterId++;

    // acceptState
    for (const State* s: graph.fa.states()) {
      if (s->isAccepting()) {
        sstr << "    node [shape=doublecircle";
        //(FIXME, BUGGY?) sstr << ",label=\"" << fmt::format("{}{}:{}", graph.stateLabelPrefix, s->id(), s->tag()) << "\"";
        sstr << "]; "
             << graph.stateLabelPrefix << s->id() << ";\n";
      }
    }

    // initialState
    sstr << "    \"\" [shape=plaintext];\n";
    sstr << "    node [shape=circle];\n";
    sstr << "    ";
    if (graphs.size() == 1)
      sstr << "\"\" -> ";
    sstr << graph.stateLabelPrefix << graph.fa.initialState()->id() << ";\n";

    // all states and their edges
    for (const State* state: graph.fa.states()) {
      if (state->tag() != 0) {
        sstr << "    " << graph.stateLabelPrefix << state->id() << " ["
             << "color=blue"
             << "]\n";
      }
      if (groupEdges) {
        std::map<State* /*target state*/, std::vector<Symbol> /*transition symbols*/> transitionGroups;

        for (const Edge& transition: state->transitions())
          transitionGroups[transition.state].emplace_back(transition.symbol);

        for (const std::pair<State*, std::vector<Symbol>>& tgroup: transitionGroups) {
          std::string label = groupCharacterClassRanges(tgroup.second, true);
          const State* targetState = tgroup.first;
          sstr << "    " << graph.stateLabelPrefix << state->id()
               << " -> " << graph.stateLabelPrefix << targetState->id();
          sstr << "   [label=\"" << escapeString(label) << "\"]";
          sstr << ";\n";
        }
      } else {
        for (const Edge& transition : state->transitions()) {
          std::string label = prettySymbol(transition.symbol, true);
          const State* targetState = transition.state;
          sstr << "    " << graph.stateLabelPrefix << state->id()
               << " -> " << graph.stateLabelPrefix << targetState->id();
          sstr << "   [label=\"" << escapeString(label) << "\"]";
          sstr << ";\n";
        }
      }
    }

    sstr << "  }\n";
  }

  sstr << "}\n";

  return sstr.str();
}

StateId nextId(const OwnedStateSet& states) {
  StateId id = 0;

  for (const std::unique_ptr<State>& s : states)
    if (id <= s->id())
      id = s->id() + 1;

  return id;
}

std::string to_string(const OwnedStateSet& S, std::string_view stateLabelPrefix) {
  std::vector<StateId> names;
  for (const std::unique_ptr<State>& s : S)
    names.push_back(s->id());

  std::sort(names.begin(), names.end());

  std::stringstream sstr;
  sstr << "{";
  int i = 0;
  for (StateId name : names) {
    if (i)
      sstr << ", ";
    sstr << stateLabelPrefix << name;
    i++;
  }
  sstr << "}";

  return sstr.str();
}

std::string to_string(const StateSet& S, std::string_view stateLabelPrefix) {
  std::vector<StateId> names;
  for (const State* s : S)
    names.push_back(s->id());

  std::sort(names.begin(), names.end());

  std::stringstream sstr;
  sstr << "{";
  int i = 0;
  for (StateId name : names) {
    if (i)
      sstr << ", ";
    sstr << stateLabelPrefix << name;
    i++;
  }
  sstr << "}";

  return sstr.str();
}

// ---------------------------------------------------------------------------
// State

#define VERIFY_STATE_AVAILABILITY(freeId, set)                                  \
  do {                                                                          \
    if (std::find_if((set).begin(), (set).end(),                                \
          [&](const auto& s) { return s->id() == freeId; }) != (set).end()) {   \
      std::cerr << fmt::format(                                                 \
          "VERIFY_STATE_AVAILABILITY({0}) failed. Id {0} is already in use.\n", \
          (freeId));                                                            \
      abort();                                                                  \
    }                                                                           \
  } while (0)

State* State::transition(Symbol input) const {
  for (const Edge& transition : transitions_)
    if (input == transition.symbol)
      return transition.state;

  return nullptr;
}

void State::linkTo(Symbol input, State* state) {
  transitions_.emplace_back(input, state);
}

// ---------------------------------------------------------------------------
// FiniteAutomaton

FiniteAutomaton& FiniteAutomaton::operator=(const FiniteAutomaton& other) {
  states_.clear();
  for (State* s : other.states())
    createState(s->id())->setAccept(s->isAccepting());

  // map links
  for (const std::unique_ptr<State>& s : other.states_)
    for (const Edge& t : s->transitions())
      findState(s->id())->linkTo(t.symbol, findState(t.state->id()));

  initialState_ = findState(other.initialState()->id());

  return *this;
}

void FiniteAutomaton::renumber() {
  std::set<State*> registry;
  renumber(initialState_, &registry);
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

void FiniteAutomaton::renumber(State* s, std::set<State*>* registry) {
  StateId id = registry->size();
  VERIFY_STATE_AVAILABILITY(id, *registry);
  s->setId(id);
  registry->insert(s);
  for (const Edge& transition : s->transitions()) {
    if (registry->find(transition.state) == registry->end()) {
      renumber(transition.state, registry);
    }
  }
}

State* FiniteAutomaton::createState() {
  return createState(nextId(states_));
}

State* FiniteAutomaton::createState(StateId id) {
  for (State* s : states())
    if (s->id() == id)
      throw std::invalid_argument{fmt::format("StateId: {}", id)};

  return states_.insert(std::make_unique<State>(id)).first->get();
}

State* FiniteAutomaton::findState(StateId id) const {
  auto s = std::find_if(states_.begin(), states_.end(),
                        [id](const auto& s) { return s->id() == id;});
  if (s != states_.end())
    return s->get();

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

/**
 * Determines the tag to use for the deterministic state representing @p q from non-deterministic FA @p fa.
 *
 * @param fa the owning finite automaton being operated on
 * @param q the set of states that reflect a single state in the DFA equal to the input FA
 * @param tag address to the Tag the resulting will be stored to
 *
 * @returns whether or not the tag could be determined.
 */
bool determineTag(const FiniteAutomaton& fa, StateSet q, Tag* tag) {
  // eliminate target-states that originate from epsilon transitions or have no tag set at all
  for (auto i = q.begin(), e = q.end(); i != e; ) {
    State* s = *i;
    if (fa.isReceivingEpsilon(s) || !s->tag()) {
      i = q.erase(i);
    } else {
      i++;
    }
  }

  if (q.empty()) {
    // fprintf(stderr, "determineTag: all of q was epsiloned\n");
    *tag = 0;
    return false;
  }

  const int priority = (*std::min_element(
      q.begin(), q.end(),
      [](auto x, auto y) { return x->priority() < y->priority(); }))->priority();

  // eliminate lower priorities
  for (auto i = q.begin(), e = q.end(); i != e; ) {
    State* s = *i;
    if (s->priority() != priority) {
      i = q.erase(i);
    } else {
      i++;
    }
  }
  if (q.empty()) {
    // fprintf(stderr, "determineTag: lowest priority found: %d, but no states left?\n", priority);
    *tag = 0;
    return true;
  }

  *tag = (*std::min_element(
        q.begin(),
        q.end(),
        [](auto x, auto y) { return x->tag() < y->tag(); }))->tag();

  return true;
}

FiniteAutomaton FiniteAutomaton::deterministic() const {
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
    State* d_i = dfa.createState(q_i);
    // std::cerr << fmt::format("map q{} to d{} for {} states, {}.\n", q_i, d_i->id(), q.size(), to_string(q, "d"));

    // if q contains an accepting state, then d is an accepting state in the DFA
    if (containsAcceptingState(q)) {
      d_i->setAccept(true);
      Tag tag{};
      if (determineTag(*this, q, &tag)) {
        // std::cerr << fmt::format("determineTag: q{} tag {} from {}.\n", q_i, tag, q);
        d_i->setTag(tag);
      } else {
        // std::cerr << fmt::format("DFA accepting state {} merged from input states with different tags {}.\n", q_i, to_string(q));
      }
    }

    q_i++;
  }

  // observe mapping from q_i to d_i
  for (const std::pair<TransitionTable::Input, int>& transition: T.transitions) {
    const int q_i = transition.first.configurationNumber;
    const Symbol c = transition.first.symbol;
    const int t_i = transition.second;
    if (t_i != -1) {
      DEBUG("map d{} |--({})--> d{}", q_i, prettySymbol(c), t_i);
      State* q = dfa.findState(q_i);
      State* t = dfa.findState(t_i);

      q->linkTo(c, t);
    }
  }

  // q_0 becomes d_0 (initial state)
  dfa.setInitialState(dfa.findState(0));

  return dfa;
}

bool FiniteAutomaton::isReceivingEpsilon(const State* t) const {
  for (const std::unique_ptr<State>& s : states_)
    for (const Edge& edge : s->transitions())
      if (edge.state == t && edge.symbol == EpsilonTransition)
        return true;

  return false;
}

// Builds a list of states that can be exclusively reached from S via epsilon-transitions.
StateSet epsilonClosure(const StateSet& S) {
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
}

// Computes a valid configuration the FA can reach with the given input @p q and @p c.
// 
// @param q valid input configuration of the original NFA.
// @param c the input character that the FA would consume next
//
// @return set of states that the FA can reach from @p c given the input @p c.
StateSet delta(const StateSet& q, Symbol c) {
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
}

int configurationNumber(const std::vector<StateSet>& Q, const StateSet& t) {
  int i = 0;
  for (const StateSet& q_i : Q) {
    if (q_i == t) {
      return i;
    }
    i++;
  }

  return -1;
}

bool containsAcceptingState(const StateSet& Q) {
  for (State* q : Q)
    if (q->isAccepting())
      return true;

  return false;
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
    State* q = dfamin.createState(p_i);
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
    State* t0 = dfamin.findState(p_i);
    for (const Edge& transition : s->transitions()) {
      if (int t_i = partitionId(transition.state); t_i != -1) {
        DEBUG("map p{} --({})--> p{}", p_i, prettySymbol(transition.symbol), t_i);
        State* t1 = dfamin.findState(t_i);
        t0->linkTo(transition.symbol, t1);
      }
    }
    p_i++;
  }

  // TODO
  // construct states & links out of P

  return dfamin;
}

StateSet FiniteAutomaton::acceptStates() const {
  StateSet result;

  for (State* s : states())
    if (s->isAccepting())
      result.insert(s);

  return result;
}

// ---------------------------------------------------------------------------
// ThompsonConstruct

ThompsonConstruct::ThompsonConstruct(FiniteAutomaton fa)
    : ThompsonConstruct{} {
  if (StateSet acceptStates = fa.acceptStates(); acceptStates.size() != 1) {
    State* newEnd = fa.createState();
    for (State* a : acceptStates) {
      a->setAccept(false);
      a->linkTo(newEnd);
    }
    newEnd->setAccept(true);
  }

  std::tuple<OwnedStateSet, State*> owned = fa.release();
  states_ = std::move(std::get<0>(owned));
  initialState_ = std::get<1>(owned);
  nextId_ = nextId(states_);
}

void ThompsonConstruct::setTag(Tag tag) {
  for (const std::unique_ptr<State>& s : states_) {
    assert(s->tag() == 0 && "State.tag() must not be set more than once");
    s->setTag(tag);
  }
}

ThompsonConstruct ThompsonConstruct::clone() const {
  ThompsonConstruct output;

  // clone states
  for (const std::unique_ptr<State>& s : states_) {
    State* u = output.createState(s->id(), s->isAccepting(), s->tag());
    if (s.get() == initialState()) {
      output.initialState_ = u;
    }
  }

  // map links
  for (const std::unique_ptr<State>& s : states_) {
    State* u = output.findState(s->id());
    for (const Edge& transition : s->transitions()) {
      State* v = output.findState(transition.state->id());
      u->linkTo(transition.symbol, v);
      // findState(s->id())->linkTo(t.symbol, findState(t.state->id()));
    }
  }

  output.nextId_ = nextId_;

  return output;
}

State* ThompsonConstruct::acceptState() const {
  for (const std::unique_ptr<State>& s : states_)
    if (s->isAccepting())
      return s.get();

  fprintf(stderr, "Internal Bug! Thompson's Consruct without an end state.\n");
  abort();
}

FiniteAutomaton ThompsonConstruct::release() {
  auto t = std::make_tuple(std::move(states_), initialState_);
  initialState_ = nullptr;
  return FiniteAutomaton{std::move(t)};
}

State* ThompsonConstruct::createState() {
  return createState(nextId_++, false, 0);
}

State* ThompsonConstruct::createState(StateId id, bool accepting, Tag tag) {
  if (findState(id) != nullptr)
    throw std::invalid_argument{fmt::format("StateId: {}", id)};

  return states_.insert(std::make_unique<State>(id, accepting, tag)).first->get();
}

State* ThompsonConstruct::findState(StateId id) const {
  for (const std::unique_ptr<State>& s : states_)
    if (s->id() == id)
      return s.get();

  return nullptr;
}

ThompsonConstruct& ThompsonConstruct::concatenate(ThompsonConstruct rhs) {
  acceptState()->linkTo(rhs.initialState_);
  acceptState()->setAccept(false);

  // renumber first with given base
  for (const std::unique_ptr<State>& s : rhs.states_) {
    VERIFY_STATE_AVAILABILITY(nextId_, states_);
    s->setId(nextId_++);
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

  // renumber first with given base
  for (const std::unique_ptr<State>& s : other.states_) {
    VERIFY_STATE_AVAILABILITY(nextId_, states_);
    s->setId(nextId_++);
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

  if (factor == 1)
    return *this;

  ThompsonConstruct base = clone();
  for (unsigned n = 2; n <= factor; ++n)
    concatenate(base.clone());

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

} // namespace klex::fa
