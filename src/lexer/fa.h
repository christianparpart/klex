#pragma once

#include <lexer/regexpr.h>
#include <lexer/util/UnboxedRange.h>

#include <fmt/format.h>
#include <string_view>
#include <map>
#include <memory>
#include <list>

namespace lexer::fa {

using Condition = char;

// represents an epsilon-transition
constexpr Condition EpsilonTransition = '\0';

class State {
 public:
  explicit State(std::string label) : label_{label}, successors_{} {}

  const std::string& label() const noexcept { return label_; }
  void relabel(std::string label) { label_ = std::move(label); }

  std::list<std::pair<Condition, State*>>& successors() noexcept { return successors_; }
  const std::list<std::pair<Condition, State*>>& successors() const noexcept { return successors_; }

  void linkTo(State* state) { linkTo(EpsilonTransition, state); }
  void linkTo(Condition condition, State* state);

  std::list<std::string> to_strings() const;

 private:
  std::string label_;
  std::list<std::pair<Condition, State*>> successors_;
};

// represents a finite automaton (NFA or DFA)
class FiniteAutomaton {
 public:
  FiniteAutomaton();

  State* createState();

 private:
  std::list<std::unique_ptr<State>> states_;
  State* initialState_;
  std::list<State*> acceptStates_;
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
  ThompsonConstruct& repeat(unsigned minimum);
  ThompsonConstruct& repeat(unsigned minimum, unsigned maximum);

  ThompsonConstruct& relabel(std::string_view prefix);

  auto states() { return util::unbox(states_); }

  std::string dot() const;

  // applies "Subset Construction"
  std::unique_ptr<FiniteAutomaton> minimize() const;

 private:
  State* createState();
  void relabel(State* s, std::string_view prefix, unsigned* base,
               std::set<State*>* registry);

 private:
  std::list<std::unique_ptr<State>> states_;
  State* startState_;
  State* endState_;
};

class Generator : public RegExprVisitor {
 public:
  explicit Generator(std::string prefix)
      : prefix_{prefix}, label_{0}, fa_{} {}

  ThompsonConstruct generate(const RegExpr* re);

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

} // namespace lexer::fa
