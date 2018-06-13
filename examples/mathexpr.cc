#include <klex/Lexer.h>
#include <klex/Compiler.h>
#include <string_view>
#include <iostream>
#include <fmt/format.h>

enum class Token { INVALID, Eof, RndOpen, RndClose, Plus, Minus, Mul, Div, Number };
constexpr std::string_view patterns[] { "<<EOF>>", "\\(", "\\)", "\\+", "-", "\\*", "/", "[0-9]+" };

int main(int argc, const char* argv[]) {
  klex::Compiler cc;
  int i = 1;
  for (std::string_view p : patterns)
    cc.declare(i++, p);

  cc.declare(klex::IgnoreTag, "[ ]+");
  cc.declare(klex::EofTag, "<<EOF>>");

  klex::Lexer lexer { cc.compile(), std::cin };

  // for (int t = lexer.recognizeOne(); t > Token::Eof; t = lexer.recognize()) {
  int t;
  do {
    t = lexer.recognizeOne();
    std::cerr << fmt::format("[{}-{}]: token {} (\"{}\")\n",
                             lexer.offset().first,
                             lexer.offset().second,
                             klex::Tag{t}, lexer.word());
  } while (Token{t} != Token::Eof && t != -1);

  return 0;
}
