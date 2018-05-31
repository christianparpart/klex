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

class State {
 public:
  explicit State(std::string label) : State{std::move(label), false} {}
  State(std::string label, bool accepting) : label_{label}, accepting_{accepting}, transitions_{} {}

  const std::string& label() const noexcept { return label_; }
  void relabel(std::string label) { label_ = std::move(label); }

  EdgeList& transitions() noexcept { return transitions_; }
  const EdgeList& transitions() const noexcept { return transitions_; }

  void linkTo(State* state) { linkTo(EpsilonTransition, state); }
  void linkTo(Symbol condition, State* state);

  void setAccept(bool accepting) { accepting_ = accepting; }
  bool isAccepting() const noexcept { return accepting_; }

  std::list<std::string> to_strings() const;

 private:
  std::string label_;
  bool accepting_;
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

  State* createState();
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
  StateSet acceptStates() const;

  //! Relables all states with given prefix and an monotonically increasing number.
  void relabel(std::string_view prefix);

  //! Creates a dot-file that can be visualized with dot/xdot CLI tools.
  std::string dot(const std::string_view& label) const;

  //! applies "Subset Construction", effectively creating an DFA
  FiniteAutomaton deterministic() const;

  //! applies the DFA minimization algorithm
  FiniteAutomaton minimize() const;

  /*!
   * Creates a dot-file that can be visualized with dot/xdot CLI tools.
   *
   * @param some descriptive label (such as the RE)
   * @param states list of states of the FA to visualize
   * @param initialState special state to be marked as initial state
   */
  static std::string dot(const std::string_view& label,
                         const OwnedStateSet& states, State* initialState);

 private:
  void relabel(State* s, std::string_view prefix, std::set<State*>* registry);

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
  ThompsonConstruct(const ThompsonConstruct& other) : ThompsonConstruct{} { *this = other; }
  ThompsonConstruct& operator=(const ThompsonConstruct& other);

  ThompsonConstruct(ThompsonConstruct&&) = default;
  ThompsonConstruct& operator=(ThompsonConstruct&&) = default;

  //! Constructs an empty NFA.
  ThompsonConstruct()
      : states_{},
        initialState_{nullptr} {
  }

  //! Constructs an NFA for a single character transition
  explicit ThompsonConstruct(Symbol value)
      : initialState_{createState()} {
    State* acceptState = createState();
    acceptState->setAccept(true);
    initialState_->linkTo(value, acceptState);
  }

  State* initialState() const { return initialState_; }
  State* acceptState() const;

  ThompsonConstruct clone() const { return ThompsonConstruct{*this}; }

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

  //! Creates a dot-file that can be visualized with dot/xdot CLI tools.
  std::string dot(const std::string_view& label) const;

  //! Moves internal structures into a FiniteAutomaton and returns that.
  FiniteAutomaton release();

 private:
  State* createState();
  State* createState(std::string name);
  State* findState(std::string_view label) const;

 private:
  OwnedStateSet states_;
  State* initialState_;
};

/*!
 * Generates a finite automaton from the given input (a regular expression).
 */
class Generator : public RegExprVisitor {
 public:
  explicit Generator(std::string prefix)
      : prefix_{prefix}, label_{0}, fa_{} {}

  FiniteAutomaton generate(const RegExpr* re);
  ThompsonConstruct construct(const RegExpr* re);

 private:
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
StateSet delta(const StateSet& q, Symbol c);

std::string to_string(const StateSet& S);

} // namespace lexer::fa

namespace fmt {
  template<>
  struct formatter<lexer::fa::StateSet> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const lexer::fa::StateSet& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{}", lexer::fa::to_string(v));
    }
  };
}
