// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/regular/Compiler.h>
#include <klex/regular/DFA.h>
#include <klex/regular/DotWriter.h>
#include <klex/regular/Lexable.h>
#include <klex/util/Flags.h>

#include <fmt/format.h>
#include <algorithm>
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

using Lexable = klex::regular::Lexable<Token>;
using Lexer = Lexable::iterator;
using Number = long long int;

std::string_view to_string(Token t)
{
	switch (t)
	{
		case Token::INVALID:
			return "<<INVALID>>";
		case Token::Eof:
			return "<<EOF>>";
		case Token::RndOpen:
			return "'('";
		case Token::RndClose:
			return "')'";
		case Token::Plus:
			return "'+'";
		case Token::Minus:
			return "'-'";
		case Token::Mul:
			return "'*'";
		case Token::Div:
			return "'/'";
		case Token::Number:
			return "<<NUMBER>>";
		default:
			abort();
	}
}

namespace fmt {
template <>
struct formatter<Token> : formatter<std::string_view> {
	template <typename FormatContext>
	auto format(Token v, FormatContext& ctx)
	{
		return formatter<std::string_view>::format(to_string(v), ctx);
	}
};
}  // namespace fmt

Number expr(Lexer&);

void consume(Lexer& lexer, Token t)
{
	if (lexer.token() != t)
		throw std::runtime_error{fmt::format("Unexpected token {}. Expected {} instead.", lexer.token(), t)};
	++lexer;
}

auto primaryExpr(Lexer& lexer)
{
	switch (lexer.token())
	{
		case Token::Number:
		{
			std::string s;
			std::for_each(begin(literal(lexer)), end(literal(lexer)), [&](char ch) {
				if (ch != '_')
					s += ch;
			});
			auto y = Number{std::stoi(s)};
			++lexer;
			return y;
		}
		case Token::Minus:
			return -1 * primaryExpr(++lexer);
		case Token::RndOpen:
		{
			auto y = expr(++lexer);
			consume(lexer, Token::RndClose);
			return y;
		}
		default:
			throw std::runtime_error{
				fmt::format("Unexpected token {}. Expected primary expression instead.", lexer.token())};
	}
}

auto mulExpr(Lexer& lexer)
{
	auto lhs = primaryExpr(lexer);
	for (;;)
	{
		switch (lexer.token())
		{
			case Token::Mul:
				lhs = lhs * primaryExpr(++lexer);
				break;
			case Token::Div:
				lhs = lhs / primaryExpr(++lexer);
				break;
			default:
				return lhs;
		}
	}
}

auto addExpr(Lexer& lexer)
{
	auto lhs = mulExpr(lexer);
	for (;;)
	{
		switch (lexer.token())
		{
			case Token::Plus:
				lhs = lhs + mulExpr(++lexer);
				break;
			case Token::Minus:
				lhs = lhs - mulExpr(++lexer);
				break;
			default:
				return lhs;
		}
	}
}

Number expr(Lexer& lexer)
{
	return addExpr(lexer);
}

Number parseExpr(Lexable&& lexer)
{
	auto it = begin(lexer);
	auto n = expr(it);
	consume(it, Token::Eof);
	return n;
}

int main(int argc, const char* argv[])
{
	auto flags = klex::util::Flags{};
	flags.defineBool("dfa", 'x', "Dumps DFA dotfile and exits.");
	flags.enableParameters("EXPRESSION", "Mathematical expression to calculate");
	flags.parse(argc, argv);

	auto cc = klex::regular::Compiler{};
	cc.parse(std::make_unique<std::stringstream>(RULES));

	if (flags.getBool("dfa"))
	{
		auto writer = klex::regular::DotWriter{std::cout, "n"};
		auto dfa = cc.compileMinimalDFA();
		dfa.visit(writer);
		return EXIT_SUCCESS;
	}

	auto input = std::string{argc == 1 ? std::string("2+3*4") : flags.parameters()[0]};
	auto ld = cc.compile();

	auto n = parseExpr(Lexable{ld, std::make_unique<std::stringstream>(input)});
	std::cerr << fmt::format("{} = {}\n", input, n);

	return EXIT_SUCCESS;
}
