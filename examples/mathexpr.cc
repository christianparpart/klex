// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Compiler.h>
#include <klex/DFA.h>
#include <klex/DFAMinimizer.h>
#include <klex/RuleParser.h>
#include <klex/DotWriter.h>
#include <klex/Lexer.h>
#include <klex/util/Flags.h>
#include <string_view>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <fmt/format.h>

enum class Token { INVALID, Eof, RndOpen, RndClose, Plus, Minus, Mul, Div, Number };
using Lexer = klex::Lexer<Token>;
using Number = int;

std::string to_string(Token t) {
  switch (t) {
    case Token::Eof: return "<<EOF>>";
    case Token::RndOpen: return "'('";
    case Token::RndClose: return "')'";
    case Token::Plus: return "'+'";
    case Token::Minus: return "'-'";
    case Token::Mul: return "'*'";
    case Token::Div: return "'/'";
    case Token::Number: return "<<NUMBER>>";
    case Token::INVALID: return "<<INVALID>>";
    default: abort();
  }
}

namespace fmt {
  template<>
  struct formatter<Token> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const Token& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{}", to_string(v));
    }
  };
}

Number expr(Lexer&);

void consume(Lexer& lexer, Token t) {
  if (lexer.token() != t)
    throw std::runtime_error{fmt::format("Unexpected token {}. Expected {} instead.",
                                         (int)lexer.token(), t)};
  lexer.recognize();
}

Number primaryExpr(Lexer& lexer) {
  switch (lexer.token()) {
    case Token::Number:
      return std::stoi(lexer.word());
    case Token::RndOpen: {
      Number y = expr(lexer);
      consume(lexer, Token::RndClose);
      return y;
    } 
    default:
      throw std::runtime_error{fmt::format("Unexpected token {}. Expected primary expression instead.",
                                           lexer.token())};
  }
}

Number mulExpr(Lexer& lexer) {
  Number lhs = primaryExpr(lexer);
  for (;;) {
    switch (lexer.recognize()) {
      case Token::Mul:
        lhs = lhs + primaryExpr(lexer);
        break;
      case Token::Div:
        lhs = lhs - primaryExpr(lexer);
        break;
      default:
        return lhs;
    }
  }
}

Number addExpr(Lexer& lexer) {
  Number lhs = mulExpr(lexer);
  for (;;) {
    switch (lexer.recognize()) {
      case Token::Plus:
        lhs = lhs + mulExpr(lexer);
        break;
      case Token::Minus:
        lhs = lhs - mulExpr(lexer);
        break;
      default:
        return lhs;
    }
  }
}

Number expr(Lexer& lexer) {
  return addExpr(lexer);
}

int main(int argc, const char* argv[]) {
  klex::util::Flags flags;
  flags.defineBool("dfa", 'x', "Dumps DFA dotfile and exits.");
  flags.parse(argc, argv);

  // TODO: ensure rule position equals token ID
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    Space(ignore) ::= [\s\t]+
    Eof           ::= <<EOF>>
    Plus          ::= "+"
    Minus         ::= "-"
    Mul           ::= "*"
    Div           ::= "/"
    RndOpen       ::= "("
    RndClose      ::= \)
    Number        ::= [0-9]+
    INVALID       ::= .
  )")};
  klex::Tag i = 10;
  klex::Compiler cc;
  for (const klex::Rule& rule : rp.parseRules())
    cc.declare(i++, rule.pattern);

  if (flags.getBool("dfa")) {
    klex::DotWriter writer{ std::cout };
    klex::DFA dfa = klex::DFAMinimizer{cc.compileDFA()}.construct();
    dfa.visit(writer);
    return EXIT_SUCCESS;
  }

  const klex::LexerDef def = cc.compile();

  //Lexer lexer { def, std::cin };
  Lexer lexer { def, std::make_unique<std::stringstream>("  12 + 3 * 4") };

  for (Token t = lexer.recognize(); t != Token::Eof; t = lexer.recognize()) {
    std::cerr << fmt::format("[{}-{}]: token {} (\"{}\")\n",
                             lexer.offset().first,
                             lexer.offset().second,
                             int(t), lexer.word());
  }
  printf("last token: %d\n", (int) lexer.token());

  return EXIT_SUCCESS;
}
