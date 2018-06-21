// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Compiler.h>
#include <klex/DFA.h>
#include <klex/DotWriter.h>
#include <klex/Lexer.h>
#include <klex/util/Flags.h>

#include <algorithm>
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>

enum class Token { Eof = 1, Plus, Minus, Mul, Div, RndOpen, RndClose, Number, INVALID };
std::string RULES = R"(
    Space(ignore) ::= [\s\t]+
    Eof           ::= <<EOF>>
    Plus          ::= "+"
    Minus         ::= "-"
    Mul           ::= "*"
    Div           ::= "/"
    RndOpen       ::= "("
    RndClose      ::= \)
    Number        ::= ([0-9]+|[0-9]{1,3}(_[0-9]{3})*)
    INVALID       ::= .
)";

using Lexer = klex::Lexer<Token>;
using Number = long long int;

std::string to_string(Token t) {
  switch (t) {
    case Token::INVALID: return "<<INVALID>>";
    case Token::Eof: return "<<EOF>>";
    case Token::RndOpen: return "'('";
    case Token::RndClose: return "')'";
    case Token::Plus: return "'+'";
    case Token::Minus: return "'-'";
    case Token::Mul: return "'*'";
    case Token::Div: return "'/'";
    case Token::Number: return "<<NUMBER>>";
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
                                         lexer.token(), t)};
  lexer.recognize();
}

Number primaryExpr(Lexer& lexer) {
  switch (lexer.token()) {
    case Token::Minus:
      lexer.recognize();
      return -1 * primaryExpr(lexer);
    case Token::Number: {
      std::string s;
      std::for_each(lexer.word().begin(), lexer.word().end(), [&](char ch) { if (ch != '_') s += ch; });
      Number y = std::stoi(s);
      lexer.recognize();
      return y;
    }
    case Token::RndOpen: {
      lexer.recognize();
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
    switch (lexer.token()) {
      case Token::Mul:
        lexer.recognize();
        lhs = lhs * primaryExpr(lexer);
        break;
      case Token::Div:
        lexer.recognize();
        lhs = lhs / primaryExpr(lexer);
        break;
      default:
        return lhs;
    }
  }
}

Number addExpr(Lexer& lexer) {
  Number lhs = mulExpr(lexer);
  for (;;) {
    switch (lexer.token()) {
      case Token::Plus:
        lexer.recognize();
        lhs = lhs + mulExpr(lexer);
        break;
      case Token::Minus:
        lexer.recognize();
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
  flags.enableParameters("EXPRESSION", "Mathematical expression to calculate");
  flags.parse(argc, argv);

  klex::Compiler cc;
  cc.parse(std::make_unique<std::stringstream>(RULES));

  if (flags.getBool("dfa")) {
    klex::DotWriter writer { std::cout };
    klex::DFA dfa = cc.compileMinimalDFA();
    dfa.visit(writer);
    return EXIT_SUCCESS;
  }

  const klex::LexerDef def = cc.compile();

  // Lexer lexer { def, std::cin };
  std::string input = argc == 1 ? std::string("2+3*4") : flags.parameters()[0];
  Lexer lexer { def, std::make_unique<std::stringstream>(input) };

  lexer.recognize();
  Number n = expr(lexer);
  consume(lexer, Token::Eof);
  std::cerr << fmt::format("{} = {}\n", input, n);

  return EXIT_SUCCESS;
}
