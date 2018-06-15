// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Compiler.h>
#include <klex/DFA.h>
#include <klex/DFAMinimizer.h>
#include <klex/DotWriter.h>
#include <klex/Lexer.h>
#include <klex/util/Flags.h>
#include <string_view>
#include <iostream>
#include <memory>
#include <sstream>
#include <fmt/format.h>

enum class Token { INVALID, Eof, RndOpen, RndClose, Plus, Minus, Mul, Div, Number };
constexpr std::string_view patterns[] { "<<EOF>>", "\\(", "\\)", "\\+", "-", "\\*", "/", "[0-9]+" };

int main(int argc, const char* argv[]) {
  klex::util::Flags flags;
  flags.defineBool("dfa", 'x', "Dumps DFA dotfile and exits.");
  flags.parse(argc, argv);

  klex::Compiler cc;
  int i = 1;
  for (std::string_view p : patterns)
    cc.declare(i++, p);

  cc.declare(klex::IgnoreTag, "[ \\t]+");
  cc.declare(klex::EofTag, "<<EOF>>");

  if (flags.getBool("dfa")) {
    klex::DotWriter writer{ std::cout };
    klex::DFA dfa = klex::DFAMinimizer{cc.compileDFA()}.construct();
    dfa.visit(writer);
    return EXIT_SUCCESS;
  }

  klex::LexerDef def = cc.compile();

  //klex::Lexer lexer { def, std::cin };
  klex::Lexer lexer { def, std::make_unique<std::stringstream>("  12 + 3 * 4") };

  for (Token t = (Token)lexer.recognize(); !(int(t) < 0) && t != Token::Eof; t = (Token)lexer.recognize()) {
    std::cerr << fmt::format("[{}-{}]: token {} (\"{}\")\n",
                             lexer.offset().first,
                             lexer.offset().second,
                             int(t), lexer.word());
  }
  printf("last token: %d\n", lexer.token());

  return EXIT_SUCCESS;
}
