#pragma once

#include <lexer/alphabet.h>
#include <lexer/regexpr.h>
#include <lexer/util/UnboxedRange.h>

#include <fmt/format.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <tuple>

namespace lexer::fa {

using Condition = char;
class State;
using Edge = std::pair<Condition, State*>;
using EdgeList = std::list<Edge>;


// represents an epsilon-transition
constexpr Condition EpsilonTransition = '\0';

class State {
 public:
  explicit State(std::string label) : State{std::move(label), false} {}
  State(std::string label, bool accepting) : label_{label}, accepting_{accepting}, successors_{} {}

  const std::string& label() const noexcept { return label_; }
  void relabel(std::string label) { label_ = std::move(label); }

  EdgeList& successors() noexcept { return successors_; }
  const EdgeList& successors() const noexcept { return successors_; }

  void linkTo(State* state) { linkTo(EpsilonTransition, state); }
  void linkTo(Condition condition, State* state);

  void setAccept(bool accepting) { accepting_ = accepting; }
  bool isAccepting() const noexcept { return accepting_; }

  std::list<std::string> to_strings() const;

 private:
  std::string label_;
  bool accepting_;
  EdgeList successors_;
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
  FiniteAutomaton(const FiniteAutomaton&) = delete;
  FiniteAutomaton& operator=(const FiniteAutomaton&) = delete;
  FiniteAutomaton(FiniteAutomaton&&) = default;
  FiniteAutomaton& operator=(FiniteAutomaton&&) = default;
  ~FiniteAutomaton() = default;

  FiniteAutomaton()
      : FiniteAutomaton{nullptr, {}, {}} {}

  FiniteAutomaton(State* initialState, OwnedStateSet states, StateSet acceptStates)
      : states_{std::move(states)},
        initialState_{initialState},
        acceptStates_{std::move(acceptStates)} {}

  FiniteAutomaton(std::tuple<OwnedStateSet, State*, State*> thompsonConstruct)
      : states_{std::move(std::get<0>(thompsonConstruct))},
        initialState_{std::get<1>(thompsonConstruct)},
        acceptStates_{{std::get<2>(thompsonConstruct)}} {}

  State* createState(std::string label);
  State* findState(std::string_view label) const;

  //! Retrieves the alphabet of this finite automaton.
  Alphabet alphabet() const;

  //! Retrieves the initial state.
  State* initialState() const { return initialState_; }
  void setInitialState(State* s);

  //! Retrieves the list of available states.
  auto states() const { return util::unbox(states_); }

  //! Retrieves the list of accepting states.
  const StateSet& acceptStates() const noexcept { return acceptStates_; }

  //! Relables all states with given prefix and an monotonically increasing number.
  void relabel(std::string_view prefix);

  //! Creates a dot-file that can be visualized with dot/xdot CLI tools.
  std::string dot() const;

  //! applies "Subset Construction", effectively creating an DFA
  FiniteAutomaton minimize() const;

  /*!
   * Creates a dot-file that can be visualized with dot/xdot CLI tools.
   *
   * @param states list of states of the FA to visualize
   * @param initialState special state to be marked as initial state
   * @param acceptStates special states to be marked as accepting states
   */
  static std::string dot(const OwnedStateSet& states, State* initialState,
                         const StateSet& acceptStates);

 private:
  void relabel(State* s, std::string_view prefix, std::set<State*>* registry);

 private:
  OwnedStateSet states_;
  State* initialState_;
  StateSet acceptStates_;
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
  //! Constructs an empty NFA.
  ThompsonConstruct()
      : states_{},
        startState_{nullptr},
        endState_{nullptr} {
  }

  //! Constructs an NFA for a single character transition
  explicit ThompsonConstruct(Condition value)
      : startState_(createState()),
        endState_(createState()) {
    startState_->linkTo(value, endState_);
  }

  ThompsonConstruct(ThompsonConstruct&&) = default;
  ThompsonConstruct& operator=(ThompsonConstruct&&) = default;

  ThompsonConstruct(const ThompsonConstruct&) = delete;
  ThompsonConstruct& operator=(const ThompsonConstruct&) = delete;

  //! Concatenates the right FA's initial state with this FA's accepting state.
  ThompsonConstruct& concatenate(ThompsonConstruct rhs);

  //! Reconstructs this FA to alternate between this FA and the @p other FA.
  ThompsonConstruct& alternate(ThompsonConstruct other);

  //! Reconstructs this FA with the given @p quantifier factor.
  ThompsonConstruct& repeat(unsigned quantifier);

  //! Reconstructs this FA to be repeatable between range [minimum, maximum].
  ThompsonConstruct& repeat(unsigned minimum, unsigned maximum);

  //! Retrieves the list of states this FA contains.
  auto states() const { return util::unbox(states_); }

  //! Creates a dot-file that can be visualized with dot/xdot CLI tools.
  std::string dot() const;

  //! Moves internal structures into a FiniteAutomaton and returns that.
  FiniteAutomaton release();

 private:
  State* createState();

 private:
  OwnedStateSet states_;
  State* startState_;
  State* endState_;
};

/*!
 * Generates a finite automaton from the given input (a regular expression).
 */
class Generator : public RegExprVisitor {
 public:
  explicit Generator(std::string prefix)
      : prefix_{prefix}, label_{0}, fa_{} {}

  FiniteAutomaton generate(const RegExpr* re);

 private:
  ThompsonConstruct construct(const RegExpr* re);
  void visit(AlternationExpr& alternationExpr) override;
  void visit(ConcatenationExpr& concatenationExpr) override;
  void visit(CharacterExpr& characterExpr) override;
  void visit(ClosureExpr& closureExpr) override;

 private:
  std::string prefix_; 
  unsigned label_;
  ThompsonConstruct fa_;
};

/**
 * Builds a list of states that can be exclusively reached from S via epsilon-transitions.
 */
StateSet epsilonClosure(const StateSet& S);

/**
 * Computes a valid configuration the FA can reach with the given input @p q and @p c.
 *
 * @param q valid input configuration of the original NFA.
 * @param c the input character that the FA would consume next
 *
 * @return set of states that the FA can reach from @p c given the input @p c.
 */
StateSet delta(const StateSet& q, char c);

} // namespace lexer::fa
