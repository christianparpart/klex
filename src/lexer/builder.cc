#include <lexer/builder.h>
#include <lexer/regexpr.h>
#include <lexer/fa.h>
#include <iostream>

namespace lexer {

void Builder::declare(int id, std::string_view pattern) {
  std::unique_ptr<RegExpr> expr = RegExprParser{}.parse(pattern);
  fa::ThompsonConstruct tc = fa::Generator{}.construct(expr.get());

  if (fa_.empty()) {
    fa_ = std::move(tc);
  } else {
    fa_.alternate(std::move(tc));
  }
}

fa::FiniteAutomaton Builder::buildAutomaton() {
  return fa::FiniteAutomaton{fa_.clone().release()};
}

} // namespace lexer
