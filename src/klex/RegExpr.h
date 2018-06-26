// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/Symbols.h>

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <string_view>

#include <fmt/format.h>

namespace klex {

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

class LookAheadExpr : public RegExpr {
 public:
  LookAheadExpr(std::unique_ptr<RegExpr> lhs, std::unique_ptr<RegExpr> rhs)
      : RegExpr{0},
        left_{std::move(lhs)},
        right_{std::move(rhs)} {}

  RegExpr* leftExpr() const { return left_.get(); }
  RegExpr* rightExpr() const { return right_.get(); }

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;

 private:
  std::unique_ptr<RegExpr> left_;
  std::unique_ptr<RegExpr> right_;
};

class AlternationExpr : public RegExpr {
 public:
  AlternationExpr(std::unique_ptr<RegExpr> lhs, std::unique_ptr<RegExpr> rhs)
      : RegExpr{1},
        left_{std::move(lhs)},
        right_{std::move(rhs)} {}

  RegExpr* leftExpr() const { return left_.get(); }
  RegExpr* rightExpr() const { return right_.get(); }

  std::string to_string() const override;
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

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;

 private:
  std::unique_ptr<RegExpr> left_;
  std::unique_ptr<RegExpr> right_;
};

class ClosureExpr : public RegExpr {
 public:
  explicit ClosureExpr(std::unique_ptr<RegExpr> subExpr)
      : ClosureExpr{std::move(subExpr), 0, std::numeric_limits<unsigned>::max()} {}

  explicit ClosureExpr(std::unique_ptr<RegExpr> subExpr, unsigned low)
      : ClosureExpr{std::move(subExpr), low, std::numeric_limits<unsigned>::max()} {}

  explicit ClosureExpr(std::unique_ptr<RegExpr> subExpr, unsigned low, unsigned high)
      : RegExpr{3},
        subExpr_{std::move(subExpr)},
        minimumOccurrences_{low},
        maximumOccurrences_{high} {
    if (minimumOccurrences_ > maximumOccurrences_) {
      throw std::invalid_argument{"low,high"};
    }
  }

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

class CharacterExpr : public RegExpr {
 public:
  explicit CharacterExpr(Symbol value)
      : RegExpr{4},
        value_{value} {}

  Symbol value() const noexcept { return value_; }

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;

 private:
  Symbol value_;
};

class CharacterClassExpr : public RegExpr {
 public:
  explicit CharacterClassExpr(SymbolSet value)
      : RegExpr{4},
        value_{value} {}

  const SymbolSet& value() const noexcept { return value_; }

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;

 private:
  SymbolSet value_;
};

class DotExpr : public RegExpr {
 public:
  explicit DotExpr()
      : RegExpr{4} {}

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;
};

class BeginOfLineExpr : public RegExpr {
 public:
  explicit BeginOfLineExpr()
      : RegExpr{4} {}

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;
};

class EndOfLineExpr : public RegExpr {
 public:
  explicit EndOfLineExpr()
      : RegExpr{4} {}

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;
};

class EndOfFileExpr : public RegExpr {
 public:
  explicit EndOfFileExpr()
      : RegExpr{4} {}

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;
};

class EmptyExpr : public RegExpr {
 public:
  explicit EmptyExpr()
      : RegExpr{4} {}

  std::string to_string() const override;
  void accept(RegExprVisitor& visitor) override;
};

class RegExprVisitor {
 public:
  virtual ~RegExprVisitor() {}

  virtual void visit(LookAheadExpr& lookaheadExpr) = 0;
  virtual void visit(ConcatenationExpr& concatenationExpr) = 0;
  virtual void visit(AlternationExpr& alternationExpr) = 0;
  virtual void visit(CharacterExpr& characterExpr) = 0;
  virtual void visit(CharacterClassExpr& characterClassExpr) = 0;
  virtual void visit(ClosureExpr& closureExpr) = 0;
  virtual void visit(BeginOfLineExpr& eolExpr) = 0;
  virtual void visit(EndOfLineExpr& eolExpr) = 0;
  virtual void visit(EndOfFileExpr& eofExpr) = 0;
  virtual void visit(DotExpr& dotExpr) = 0;
  virtual void visit(EmptyExpr& emptyExpr) = 0;
};

} // namespace klex
