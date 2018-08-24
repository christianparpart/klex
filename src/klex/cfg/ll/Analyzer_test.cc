// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/util/testing.h>
#include <klex/util/literals.h>
#include <klex/cfg/ll/Analyzer.h>
#include <klex/cfg/ll/SyntaxTable.h>
#include <klex/cfg/GrammarParser.h>
#include <klex/cfg/GrammarLexer.h>
#include <klex/cfg/Grammar.h>
#include <klex/regular/Compiler.h>
#include <klex/regular/Lexer.h>
#include <klex/Report.h>
#include <variant>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::cfg::ll;
using namespace klex::util::literals;

const std::string balancedParentheses = "A ::= '(' A ')' | '(' ')'";

TEST(cfg_ll_Analyzer, ETF)
{
	enum class ExprToken { Eof = 1, Plus, Minus, Mul, Div, Number };
	using Lexer = regular::Lexer<ExprToken>;

	regular::Compiler rgc;
	rgc.parse(R"(`Eof             ::= <<EOF>>
				 `Spacing(ignore) ::= [\s\t\n]+
				 `Plus            ::= "+"
				 `Minus           ::= "-"
				 `Mul             ::= "*"
				 `Div             ::= "/"
				 `Number          ::= [0-9]+
				 `)"_multiline);

	ConsoleReport report;
	Grammar grammar = GrammarParser(GrammarLexer{
		R"(`Start     ::= Expr "<<EOF>>";
		   `Expr      ::= Term Expr_;
		   `Expr_     ::= '+' Term Expr_
		   `            | ;
		   `Term      ::= Factor Term_;
		   `Term_     ::= '*' Factor Term_
		   `            | ;
		   `Factor    ::= "NUMBER"
		   `            | '(' Expr ')'
		   `            ;
		   `)"_multiline}, &report).parse();

	ASSERT_FALSE(report.containsFailures());
	grammar.finalize();

	SyntaxTable st = SyntaxTable::construct(grammar);

	log("GRAMMAR:");
	log(st.dump(grammar));

	Lexer lexer { rgc.compile(),
				  "2 + 3 * 4", 
				  [this](const std::string& msg) { log(msg); } };

	Analyzer<Lexer, int> parser(move(st), move(lexer), &report);

	// parser.analyze();
}

// vim:ts=4:sw=4:noet