#include <klex/regexpr.h>

#include <sstream>
#include <limits>
#include <iostream>

#include <fmt/format.h>

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

/*
  REGULAR EXPRESSION SYNTAX:
  --------------------------

  expr                    := alternation
  alternation             := concatenation ('|' concatenation)*
  concatenation           := closure (closure)*
  closure                 := atom ['*' | '?' | '{' NUM [',' NUM] '}']
  atom                    := character | characterClass | '(' expr ')'
  characterClass          := '[' ['^'] characterClassFragment+ ']'
  characterClassFragment  := character | character '-' character
*/

namespace klex {

// {{{ AST
void AlternationExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string AlternationExpr::to_string() const {
  std::stringstream sstr;

  if (precedence() > left_->precedence()) {
    sstr << '(' << left_->to_string() << ')';
  } else
    sstr << left_->to_string();

  sstr << "|";

  if (precedence() > right_->precedence()) {
    sstr << '(' << right_->to_string() << ')';
  } else
    sstr << right_->to_string();

  return sstr.str();
}

void ConcatenationExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string ConcatenationExpr::to_string() const {
  std::stringstream sstr;

  if (precedence() > left_->precedence()) {
    sstr << '(' << left_->to_string() << ')';
  } else
    sstr << left_->to_string();

  if (precedence() > right_->precedence()) {
    sstr << '(' << right_->to_string() << ')';
  } else
    sstr << right_->to_string();

  return sstr.str();
}

void CharacterExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string CharacterExpr::to_string() const {
  return std::string(1, value_);
}

void ClosureExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string ClosureExpr::to_string() const {
  std::stringstream sstr;

  // TODO: optimize superfluous ()'s
  if (precedence() > subExpr_->precedence())
    sstr << '(' << subExpr_->to_string() << ')';
  else
    sstr << subExpr_->to_string();

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
  //DEBUG("consume: '{}'", (char)ch);
  return ch;
}

void RegExprParser::consume(int expected) {
  int actual = currentChar();
  consume();
  if (actual != expected) {
    throw UnexpectedToken{actual, expected};
  }
}

std::unique_ptr<RegExpr> RegExprParser::parse(std::string_view expr) {
  input_ = std::move(expr);
  currentChar_ = input_.begin();

  return parseExpr();
}

std::unique_ptr<RegExpr> RegExprParser::parseExpr() {
  return parseAlternation();
}

std::unique_ptr<RegExpr> RegExprParser::parseAlternation() {
  std::unique_ptr<RegExpr> lhs = parseConcatenation();

  while (currentChar() == '|') {
    consume();
    std::unique_ptr<RegExpr> rhs = parseConcatenation();
    lhs = std::make_unique<AlternationExpr>(std::move(lhs), std::move(rhs));
  }

  return lhs;
}

std::unique_ptr<RegExpr> RegExprParser::parseConcatenation() {
  // FOLLOW-set, the set of terminal tokens that can occur right after a concatenation
  static const std::string_view follow = "|)";
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
      consume();
      return std::make_unique<ClosureExpr>(std::move(subExpr), 0, 1);
    case '*':
      consume();
      return std::make_unique<ClosureExpr>(std::move(subExpr), 0);
    case '+':
      consume();
      return std::make_unique<ClosureExpr>(std::move(subExpr), 1);
    case '{': {
      consume();
      int m = parseInt();
      if (currentChar() == ',') {
        consume();
        int n = parseInt();
        consume('}');
        return std::make_unique<ClosureExpr>(std::move(subExpr), m, n);
      } else {
        consume('}');
        return std::make_unique<ClosureExpr>(std::move(subExpr), m, m);
      }
    }
    default:
      return subExpr;
  }
}

unsigned RegExprParser::parseInt() {
  unsigned n = 0;
  while (std::isdigit(currentChar())) {
    n *= 10;
    n += currentChar() - '0';
    consume();
  }
  return n;
}

std::unique_ptr<RegExpr> RegExprParser::parseAtom() {
  switch (currentChar()) {
    case '(': {
      consume();
      std::unique_ptr<RegExpr> subExpr = parseExpr();
      consume(')');
      return subExpr;
    }
    case '[':
      return parseCharacterClass();
    case '\\':
      consume();
      return std::make_unique<CharacterExpr>(consume());
    default:
      return std::make_unique<CharacterExpr>(consume());
  }
}

std::unique_ptr<RegExpr> RegExprParser::parseCharacterClass() {
  consume();
  std::unique_ptr<RegExpr> e = parseCharacterClassFragment();
  while (currentChar() != ']')
    e = std::make_unique<AlternationExpr>(std::move(e), parseCharacterClassFragment());
  consume(']');
  return e;
}

std::unique_ptr<RegExpr> RegExprParser::parseCharacterClassFragment() {
  if (currentChar() == '\\') {
    consume();
    return std::make_unique<CharacterExpr>(consume());
  }

  char c1 = consume();
  if (currentChar() != '-')
    return std::make_unique<CharacterExpr>(c1);

  consume();
  char c2 = consume();

  std::unique_ptr<RegExpr> e = std::make_unique<CharacterExpr>(c1);
  for (char c_i = c1 + 1; c_i <= c2; c_i++)
    e = std::make_unique<AlternationExpr>(std::move(e),
            std::make_unique<CharacterExpr>(c_i));
  return e;
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

} // namespace klex
