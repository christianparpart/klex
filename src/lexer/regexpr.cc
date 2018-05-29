#include "regexpr.h"
#include <sstream>
#include <limits>

namespace lexer {

// {{{ AST
void AlternationExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

void ConcatenationExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

void CharacterExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

void ClosureExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string ClosureExpr::to_string() const {
  std::stringstream sstr;

  sstr << '(';
  sstr << subExpr_->to_string();
  sstr << ')';

  if (minimumOccurrences_ == 0 && maximumOccurrences_ == 1)
    sstr << '?';
  else if (minimumOccurrences_ == 0 && maximumOccurrences_ == std::numeric_limits<unsigned>::max())
    sstr << '*';
  else if (minimumOccurrences_ == 1 && maximumOccurrences_ == std::numeric_limits<unsigned>::max())
    sstr << '+';
  else
    sstr << '{' << minimumOccurrences_ << ',' << maximumOccurrences_ << '}';

  return sstr.str();
}
// }}}
// {{{ RegExprParser
RegExprParser::RegExprParser()
    : input_{},
      currentChar_{input_.end()} {
}

int RegExprParser::currentChar() const {
  if (currentChar_ != input_.end())
    return *currentChar_;
  else
    return -1;
}

int RegExprParser::consume() {
  if (currentChar_ == input_.end())
    return -1;

  int ch = *currentChar_;
  ++currentChar_;
  return ch;
}

int RegExprParser::nextChar() {
  consume();
  return currentChar();
}

void RegExprParser::consume(int expected) {
  int actual = currentChar();
  consume();
  if (actual != expected) {
    throw UnexpectedToken{actual, expected};
  }
}

std::unique_ptr<RegExpr> RegExprParser::parse(const std::string& expr) {
  input_ = expr;
  currentChar_ = input_.begin();

  return parseExpr();
}

std::unique_ptr<RegExpr> RegExprParser::parseExpr() {
  return parseAlternation();
}

std::unique_ptr<RegExpr> RegExprParser::parseAlternation() {
  std::unique_ptr<RegExpr> lhs = parseConcatenation();

  while (currentChar() == '|') {
    nextChar();
    std::unique_ptr<RegExpr> rhs = parseConcatenation();
    lhs = std::make_unique<AlternationExpr>(std::move(lhs), std::move(rhs));
  }

  return std::move(lhs);
}

std::unique_ptr<RegExpr> RegExprParser::parseConcatenation() {
  static const std::string follow = "|)"; // FOLLOW-set
  std::unique_ptr<RegExpr> lhs = parseClosure();

  while (!eof() && follow.find(currentChar()) == follow.npos) {
    std::unique_ptr<RegExpr> rhs = parseClosure();
    lhs = std::make_unique<ConcatenationExpr>(std::move(lhs), std::move(rhs));
  }

  return lhs;
}

std::unique_ptr<RegExpr> RegExprParser::parseClosure() {
  std::unique_ptr<RegExpr> subExpr = parseAtom();

  switch (currentChar()) {
    case '?':
      nextChar();
      return std::make_unique<ClosureExpr>(std::move(subExpr), 0, 1);
    case '*':
      nextChar();
      return std::make_unique<ClosureExpr>(std::move(subExpr));
    default:
      return subExpr;
  }
}

std::unique_ptr<RegExpr> RegExprParser::parseAtom() {
  switch (currentChar()) {
    case '(': {
      nextChar();
      std::unique_ptr<RegExpr> subExpr = parseExpr();
      consume(')');
      return subExpr;
    }
    // case '[': {
    //   consume(']');
    //   return subExpr;
    // }
    case '\\':
      consume();
      return std::make_unique<CharacterExpr>(consume());
    default:
      return std::make_unique<CharacterExpr>(consume());
  }
}
// }}}
// {{{ RegExprEvaluator
RegExprEvaluator::RegExprEvaluator()
    : pattern_{},
      offset_{},
      result_{false} {
}

int RegExprEvaluator::consume() {
  int ch = currentChar();
  if (!eof())
    ++offset_;
  return ch;
}

bool RegExprEvaluator::match(std::string_view pattern, RegExpr* expr) {
  pattern_ = pattern;
  offset_ = 0;

  return evaluate(expr);
}

bool RegExprEvaluator::evaluate(RegExpr* expr) {
  const size_t backtrack = offset_;

  expr->accept(*this);

  if (!result_)
    offset_ = backtrack;

  return result_;
}

void RegExprEvaluator::visit(AlternationExpr& alternationExpr) {
  result_ = evaluate(alternationExpr.leftExpr()) || evaluate(alternationExpr.rightExpr());
}

void RegExprEvaluator::visit(ConcatenationExpr& concatenationExpr) {
  result_ = evaluate(concatenationExpr.leftExpr()) && evaluate(concatenationExpr.rightExpr());
}

void RegExprEvaluator::visit(CharacterExpr& characterExpr) {
  if (currentChar() == characterExpr.value()) {
    consume();
    result_ = true;
  } else {
    result_ = false;
  }
}

void RegExprEvaluator::visit(ClosureExpr& closureExpr) {
  // TODO, think backtracking

  unsigned int n = 0;
  while (n < closureExpr.maximumOccurrences() && evaluate(closureExpr.subExpr()))
    n++;

  if (closureExpr.minimumOccurrences() <= n && n <= closureExpr.maximumOccurrences())
    result_ = true;
  else
    result_ = false;
}
// }}}

} // namespace lexer
