#pragma once

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <string_view>

#include <fmt/format.h>

namespace lexer {

class Alphabet {
 public:
  void add(char ch);

  std::string to_string() const;

 private:
  std::set<char> alphabet_;
};

class RegExprVisitor;

class RegExpr {
 public:
  explicit RegExpr(int precedence) : precedence_{precedence} {}
  virtual ~RegExpr() {}

  virtual std::string to_string() const = 0;
  virtual void accept(RegExprVisitor& visitor) = 0;

  int precedence() const noexcept { return precedence_; }

 protected:
  const int precedence_;
};

class AlternationExpr : public RegExpr {
 public:
  AlternationExpr(std::unique_ptr<RegExpr> lhs, std::unique_ptr<RegExpr> rhs)
      : RegExpr{1},
        left_{std::move(lhs)},
        right_{std::move(rhs)} {}

  RegExpr* leftExpr() const { return left_.get(); }
  RegExpr* rightExpr() const { return right_.get(); }

  std::string to_string() const override { return left_->to_string() + "|" + right_->to_string(); }
  void accept(RegExprVisitor& visitor) override;

 private:
  std::unique_ptr<RegExpr> left_;
  std::unique_ptr<RegExpr> right_;
};

class ConcatenationExpr : public RegExpr {
 public:
  ConcatenationExpr(std::unique_ptr<RegExpr> lhs, std::unique_ptr<RegExpr> rhs)
      : RegExpr{2},
        left_{std::move(lhs)},
        right_{std::move(rhs)} {}

  RegExpr* leftExpr() const { return left_.get(); }
  RegExpr* rightExpr() const { return right_.get(); }

  std::string to_string() const override { return left_->to_string() + right_->to_string(); }
  void accept(RegExprVisitor& visitor) override;

 private:
  std::unique_ptr<RegExpr> left_;
  std::unique_ptr<RegExpr> right_;
};

class CharacterExpr : public RegExpr {
 public:
  explicit CharacterExpr(char value)
      : RegExpr{2},
        value_{value} {}

  char value() const noexcept { return value_; }

  std::string to_string() const override { return std::string(1, value_); }
  void accept(RegExprVisitor& visitor) override;

 private:
  char value_;
};

class ClosureExpr : public RegExpr {
 public:
  explicit ClosureExpr(std::unique_ptr<RegExpr> subExpr)
      : ClosureExpr{std::move(subExpr), 0, std::numeric_limits<unsigned>::max()} {}

  explicit ClosureExpr(std::unique_ptr<RegExpr> subExpr, unsigned low, unsigned high)
      : RegExpr{3},
        subExpr_{std::move(subExpr)},
        minimumOccurrences_{low},
        maximumOccurrences_{high} {}

  RegExpr* subExpr() const { return subExpr_.get(); }
  unsigned minimumOccurrences() const noexcept { return minimumOccurrences_; }
  unsigned maximumOccurrences() const noexcept { return maximumOccurrences_; }

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;

 private:
  std::unique_ptr<RegExpr> subExpr_;
  unsigned minimumOccurrences_;
  unsigned maximumOccurrences_;
};

class RegExprParser {
 public:
  RegExprParser();

  std::unique_ptr<RegExpr> parse(const std::string& expr);

  class UnexpectedToken : public std::runtime_error {
   public:
    UnexpectedToken(int actual, int expected)
        : std::runtime_error{fmt::format("Unexpected token {}. Found {} instead.", actual, expected)}
    {}
  };

 private:
  int currentChar() const;
  int nextChar();
  bool eof() const noexcept { return currentChar() == -1; }
  void consume(int ch);
  int consume();

  std::unique_ptr<RegExpr> parseExpr();
  std::unique_ptr<RegExpr> parseAlternation();
  std::unique_ptr<RegExpr> parseConcatenation();
  std::unique_ptr<RegExpr> parseClosure();
  std::unique_ptr<RegExpr> parseAtom();

 private:
  std::string input_;
  std::string::iterator currentChar_;
};

class RegExprVisitor {
 public:
  virtual ~RegExprVisitor() {}

  virtual void visit(AlternationExpr& alternationExpr) = 0;
  virtual void visit(ConcatenationExpr& concatenationExpr) = 0;
  virtual void visit(CharacterExpr& characterExpr) = 0;
  virtual void visit(ClosureExpr& closureExpr) = 0;
};

class RegExprEvaluator : protected RegExprVisitor {
 public:
  RegExprEvaluator();

  bool match(std::string_view pattern, RegExpr* expr);

 private:
  bool evaluate(RegExpr* expr);
  void visit(AlternationExpr& alternationExpr) override;
  void visit(ConcatenationExpr& concatenationExpr) override;
  void visit(CharacterExpr& characterExpr) override;
  void visit(ClosureExpr& closureExpr) override;

  bool eof() const { return offset_ == pattern_.size(); }
  int currentChar() const { return !eof() ? pattern_[offset_] : -1; }
  int consume() {
    int ch = currentChar();
    if (!eof())
      ++offset_;
    return ch;
  }

 private:
  std::string_view pattern_;
  size_t offset_;
  bool result_;
};

} // namespace lexer
