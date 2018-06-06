// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/alphabet.h>
#include <klex/regexpr.h>
#include <klex/util/UnboxedRange.h>

#include <fmt/format.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <tuple>
#include <vector>

namespace klex::fa {

class State;

// input symbol as used for transitions
using Symbol = char;

// an edge for transitions
struct Edge {
  Symbol symbol;
  State* state;

  Edge(Symbol _symbol, State* _state) : symbol{_symbol}, state{_state} {}
};

using EdgeList = std::list<Edge>;

// represents an epsilon-transition
constexpr Symbol EpsilonTransition = '\0';

using StateId = unsigned int;
using Tag = int;

class State {
 public:
  explicit State(StateId id) : State{id, false, 0} {}
  State(StateId id, bool accepting, Tag tag)
      : id_{id},
        accepting_{accepting},
        tag_{tag},
        priority_{1},
        transitions_{} {}

  StateId id() const noexcept { return id_; }
  void setId(StateId id) { id_ = id; }

  EdgeList& transitions() noexcept { return transitions_; }
  const EdgeList& transitions() const noexcept { return transitions_; }

  State* transition(Symbol input) const;

  void linkTo(State* state) { linkTo(EpsilonTransition, state); }
  void linkTo(Symbol condition, State* state);

  void setAccept(bool accepting) { accepting_ = accepting; }
  bool isAccepting() const noexcept { return accepting_; }

  Tag tag() const noexcept { return tag_; }
  void setTag(Tag tag) { tag_ = tag; }

  int priority() const noexcept { return priority_; }
  void setPriority(int priority) { priority_ = priority; }

 private:
  StateId id_;
  bool accepting_;
  Tag tag_;
  int priority_; //! smallest number reflects the highest priority
  EdgeList transitions_;
};

using OwnedStateSet = std::set<std::unique_ptr<State>>;
using StateSet = std::set<State*>;

/*!
 * Represents a finite automaton (NFA or DFA).
 *
 * @see Generator
 * @see ThompsonConstruct
 */
class FiniteAutomaton {
 public:
  FiniteAutomaton(const FiniteAutomaton& other) : FiniteAutomaton{} { *this = other; }
  FiniteAutomaton& operator=(const FiniteAutomaton& other);
  FiniteAutomaton(FiniteAutomaton&&) = default;
  FiniteAutomaton& operator=(FiniteAutomaton&&) = default;
  ~FiniteAutomaton() = default;

  FiniteAutomaton()
      : FiniteAutomaton{nullptr, {}} {}

  FiniteAutomaton(State* initialState, OwnedStateSet states)
      : states_{std::move(states)},
        initialState_{initialState} {}

  FiniteAutomaton(std::tuple<OwnedStateSet, State*> thompsonConstruct)
      : states_{std::move(std::get<0>(thompsonConstruct))},
        initialState_{std::get<1>(thompsonConstruct)} {}

  State* findState(StateId id) const;

  //! Retrieves the alphabet of this finite automaton.
  Alphabet alphabet() const;

  //! Retrieves the initial state.
  State* initialState() const { return initialState_; }
  void setInitialState(State* s);

  //! Retrieves the list of available states.
  auto states() const { return util::unbox(states_); }

  //! Retrieves the list of accepting states.
  StateSet acceptStates() const;

  /**
   * Rewrites all indices so that the initial state starts with ID 0,
   * following the transitions counting upwards
   */
  void renumber();

  //! applies "Subset Construction", effectively creating an DFA
  FiniteAutomaton deterministic() const;

  //! applies Hopcroft's DFA minimization algorithm
  FiniteAutomaton minimize() const;

  std::tuple<OwnedStateSet, State*> release() {
    std::tuple<OwnedStateSet, State*> result { std::move(states_), initialState_ };

    states_.clear();
    initialState_ = nullptr;

    return result;
  }

  //! @returns true if @p targetState is only reached via epsilon transition
  bool isReceivingEpsilon(const State* targetState) const;

  State* createState();

 private:
  State* createState(StateId id);
  void renumber(State* s, std::set<State*>* registry);

 private:
  OwnedStateSet states_;
  State* initialState_;
};

/**
 * NFA Builder with the Thompson's Construction properties.
 *
 * <ul>
 *   <li> There is exactly one initial state and exactly one accepting state..
 *   <li> No transition other than the initial transition enters the initial state.
 *   <li> The accepting state has no leaving edges
 *   <li> An ε-transition always connects two states that were (earlier in the construction process)
 *        the initial state and the accepting state of NFAs for some component REs.
 *   <li> Each state has at most two entering states and at most two leaving states.
 * </ul>
 */
class ThompsonConstruct {
 public:
  ThompsonConstruct(const ThompsonConstruct& other) = delete;
  ThompsonConstruct& operator=(const ThompsonConstruct& other) = delete;

  ThompsonConstruct(ThompsonConstruct&&) = default;
  ThompsonConstruct& operator=(ThompsonConstruct&&) = default;

  //! Constructs an empty NFA.
  ThompsonConstruct()
      : nextId_{0},
        states_{},
        initialState_{nullptr} {
  }

  //! Constructs an NFA for a single character transition
  explicit ThompsonConstruct(Symbol value)
      : nextId_{0},
        states_{},
        initialState_{createState()} {
    State* acceptState = createState();
    acceptState->setAccept(true);
    initialState_->linkTo(value, acceptState);
  }

  /**
   * Constructs this object with the given input automaton, enforcing Thompson's Construction
   * properties onto it.
   */
  explicit ThompsonConstruct(FiniteAutomaton fa);

  bool empty() const noexcept { return states_.empty(); }

  State* initialState() const { return initialState_; }
  State* acceptState() const;

  void setTag(Tag tag);

  ThompsonConstruct clone() const;

  //! Concatenates the right FA's initial state with this FA's accepting state.
  ThompsonConstruct& concatenate(ThompsonConstruct rhs);

  //! Reconstructs this FA to alternate between this FA and the @p other FA.
  ThompsonConstruct& alternate(ThompsonConstruct other);

  //! Reconstructs this FA to allow optional input. X -> X?
  ThompsonConstruct& optional();

  //! Reconstructs this FA with the given @p quantifier factor.
  ThompsonConstruct& times(unsigned quantifier);

  //! Reconstructs this FA to allow recurring input. X -> X*
  ThompsonConstruct& recurring();

  //! Reconstructs this FA to be recurring at least once. X+ = XX*
  ThompsonConstruct& positive();

  //! Reconstructs this FA to be repeatable between range [minimum, maximum].
  ThompsonConstruct& repeat(unsigned minimum, unsigned maximum);

  //! Retrieves the list of states this FA contains.
  auto states() const { return util::unbox(states_); }

  //! Moves internal structures into a FiniteAutomaton and returns that.
  FiniteAutomaton release();

 private:
  State* createState();
  State* createState(StateId id, bool accepting, Tag tag);
  State* findState(StateId id) const;

 private:
  StateId nextId_;
  OwnedStateSet states_;
  State* initialState_;
};

/*!
 * Generates a finite automaton from the given input (a regular expression).
 */
class Generator : public RegExprVisitor {
 public:
  explicit Generator()
      : fa_{} {}

  FiniteAutomaton generate(const RegExpr* re);
  ThompsonConstruct construct(const RegExpr* re);

 private:
  void visit(AlternationExpr& alternationExpr) override;
  void visit(ConcatenationExpr& concatenationExpr) override;
  void visit(CharacterExpr& characterExpr) override;
  void visit(ClosureExpr& closureExpr) override;

 private:
  ThompsonConstruct fa_;
};

/**
 * Returns a human readable string of the StateSet @p S, such as "{n0, n1, n2}".
 */
std::string to_string(const StateSet& S, std::string_view stateLabelPrefix = "n");
std::string to_string(const OwnedStateSet& S, std::string_view stateLabelPrefix = "n");

/**
 * Builds a list of states that can be exclusively reached from S via epsilon-transitions.
 */
StateSet epsilonClosure(const StateSet& S);

/**
 * Returns whether or not the StateSet @p Q contains at least one State that is also "accepting".
 */
bool containsAcceptingState(const StateSet& Q);

/**
 * Finds @p t in @p Q and returns its offset (aka configuration number) or -1 if not found.
 */
int configurationNumber(const std::vector<StateSet>& Q, const StateSet& t);

/**
 * Computes a valid configuration the FA can reach with the given input @p q and @p c.
 * 
 * @param q valid input configuration of the original NFA.
 * @param c the input character that the FA would consume next
 *
 * @return set of states that the FA can reach from @p c given the input @p c.
 */
StateSet delta(const StateSet& q, Symbol c);

//! Helper struct for the dot(std::list<DotGraph>) utility function.
struct DotGraph {
  const FiniteAutomaton& fa;
  std::string_view stateLabelPrefix;
  std::string_view graphLabel;
};

/**
 * Creates a dot-file for multiple FiniteAutomaton in one graph (each FA represent one sub-graph).
 */
std::string dot(std::list<DotGraph> list, std::string_view label = "", bool groupEdges = true);

std::string prettySymbol(Symbol input, bool dot = false);
std::string groupCharacterClassRanges(std::vector<Symbol> chars, bool dot = false);

} // namespace klex::fa

namespace fmt {
  template<>
  struct formatter<klex::fa::StateSet> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const klex::fa::StateSet& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{}", klex::fa::to_string(v));
    }
  };

  template<>
  struct formatter<klex::fa::OwnedStateSet> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const klex::fa::OwnedStateSet& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{}", klex::fa::to_string(v));
    }
  };
}
