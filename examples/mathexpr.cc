#include <klex/Lexer.h>
#include <klex/Compiler.h>
#include <string_view>
#include <iostream>
#include <fmt/format.h>

enum class Token { INVALID, RndOpen, RndClose, Plus, Minus, Mul, Div, Number };
constexpr std::string_view patterns[] { "\\(", "\\)", "\\+", "-", "\\*", "/", "[0-9]+" };

int main(int argc, const char* argv[]) {
  klex::Compiler cc;
  int i = 1;
  for (std::string_view p : patterns)
    cc.declare(i++, p);

  cc.declare(klex::IgnoreTag, "[ ]+");

  klex::Lexer lexer { cc.compile(), std::cin };

  for (int t = lexer.recognizeOne(); t > -1; t = lexer.recognize()) {
    std::cerr << fmt::format("[{}-{}]: token {} (\"{}\")\n",
                             lexer.offset().first,
                             lexer.offset().second,
                             t, lexer.word());
  }

  return 0;
}
