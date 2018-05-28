#pragma once

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
  explicit State(std::string id) : id_{id}, successors_{} {}

  const std::string& id() const noexcept { return id_; }
  const std::map<Condition, State*>& successors() const noexcept { return successors_; }

  void linkSuccessor(Condition condition, State* state);

 private:
  std::string id_;
  std::map<Condition, State*> successors_;
};

// represents a finite automaton (NFA or DFA)
class FiniteAutomaton {
 public:
  FiniteAutomaton();

  State* createState(std::string id);

  // applies "Subset Construction"
  std::unique_ptr<FiniteAutomaton> minimize() const;

  // merges an alternation FA into this FA
  void mergeAlternation(std::unique_ptr<FiniteAutomaton> alternation);

 private:
  std::list<std::unique_ptr<State>> states_;
  State* initialState_;
  std::list<State*> acceptStates_;
};

class Generator : public RegExprVisitor {
 public:
  explicit Generator(std::string prefix)
      : prefix_{prefix}, id_{0}, fa_{} {}

  std::unique_ptr<FiniteAutomaton> generate(const RegExpr* re);

  State* createState() {
    return fa_->createState(fmt::format("{}{}", prefix_, id_++));
  }

 private:
  void visit(AlternationExpr& alternationExpr) override;
  void visit(ConcatenationExpr& concatenationExpr) override;
  void visit(CharacterExpr& characterExpr) override;
  void visit(ClosureExpr& closureExpr) override;

 private:
  std::string prefix_; 
  unsigned id_;
  std::unique_ptr<FiniteAutomaton> fa_;
  State* state_;
};

} // namespace lexer::fa
