#pragma once

#include <lexer/alphabet.h>
#include <lexer/regexpr.h>
#include <lexer/util/UnboxedRange.h>

#include <fmt/format.h>
#include <list>
#include <map>
#include <memory>
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
  explicit State(std::string label) : label_{label}, successors_{} {}

  const std::string& label() const noexcept { return label_; }
  void relabel(std::string label) { label_ = std::move(label); }

  EdgeList& successors() noexcept { return successors_; }
  const EdgeList& successors() const noexcept { return successors_; }

  void linkTo(State* state) { linkTo(EpsilonTransition, state); }
  void linkTo(Condition condition, State* state);

  std::list<std::string> to_strings() const;

 private:
  std::string label_;
  EdgeList successors_;
};

using OwnedStateList = std::list<std::unique_ptr<State>>;
using StateList = std::list<State*>;

// represents a finite automaton (NFA or DFA)
class FiniteAutomaton {
 public:
  FiniteAutomaton(const FiniteAutomaton&) = delete;
  FiniteAutomaton& operator=(const FiniteAutomaton&) = delete;
  FiniteAutomaton(FiniteAutomaton&&) = default;
  FiniteAutomaton& operator=(FiniteAutomaton&&) = default;
  ~FiniteAutomaton() = default;

  FiniteAutomaton()
      : FiniteAutomaton{nullptr, {}, {}} {}

  FiniteAutomaton(State* initialState, OwnedStateList states, StateList acceptStates)
      : states_{std::move(states)},
        initialState_{initialState},
        acceptStates_{std::move(acceptStates)} {}

  FiniteAutomaton(std::tuple<OwnedStateList, State*, State*> thompsonConstruct)
      : states_{std::move(std::get<0>(thompsonConstruct))},
        initialState_{std::get<1>(thompsonConstruct)},
        acceptStates_{{std::get<2>(thompsonConstruct)}} {}

  Alphabet alphabet() const;
  State* initialState() const { return initialState_; }
  auto states() const { return util::unbox(states_); }
  const StateList& acceptStates() const noexcept { return acceptStates_; }

  // relables all states with given prefix and an monotonically ascending number
  void relabel(std::string_view prefix);

  // creates a dot-file that can be visualized with dot/xdot CLI tools
  std::string dot() const;

  // applies "Subset Construction", effectively creating an DFA
  std::unique_ptr<FiniteAutomaton> minimize() const;

  static std::string dot(const OwnedStateList& states, State* initialState,
                         const StateList& acceptStates);

 private:
  void relabel(State* s, std::string_view prefix, std::set<State*>* registry);

 private:
  OwnedStateList states_;
  State* initialState_;
  StateList acceptStates_;
};

class ThompsonConstruct {
 public:
  ThompsonConstruct()
      : states_{},
        startState_{nullptr},
        endState_{nullptr} {
  }

  explicit ThompsonConstruct(Condition value)
      : startState_(createState()),
        endState_(createState()) {
    startState_->linkTo(value, endState_);
  }

  ThompsonConstruct(ThompsonConstruct&&) = default;
  ThompsonConstruct& operator=(ThompsonConstruct&&) = default;

  ThompsonConstruct(const ThompsonConstruct&) = delete;
  ThompsonConstruct& operator=(const ThompsonConstruct&) = delete;

  ThompsonConstruct& concatenate(ThompsonConstruct rhs);
  ThompsonConstruct& alternate(ThompsonConstruct other);
  ThompsonConstruct& repeat(unsigned factor);
  ThompsonConstruct& repeat(unsigned minimum, unsigned maximum);

  auto states() { return util::unbox(states_); }

  std::string dot() const;

  std::tuple<OwnedStateList, State*, State*> release();

 private:
  State* createState();

 private:
  OwnedStateList states_;
  State* startState_;
  State* endState_;
};

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

} // namespace lexer::fa
