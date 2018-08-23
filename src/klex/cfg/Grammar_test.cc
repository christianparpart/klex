// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Report.h>
#include <klex/cfg/Grammar.h>
#include <klex/cfg/GrammarLexer.h>
#include <klex/cfg/GrammarParser.h>
#include <klex/util/literals.h>
#include <klex/util/testing.h>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::util::literals;

TEST(cfg_Grammar, metadata_right_recursive) {
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

	grammar.dump();

	ASSERT_EQ("\"(\"", fmt::format("{}", grammar.firstOf(Terminal{"("})));
	ASSERT_EQ("\")\"", fmt::format("{}", grammar.firstOf(Terminal{")"})));
	ASSERT_EQ("\"+\"", fmt::format("{}", grammar.firstOf(Terminal{"+"})));
	ASSERT_EQ("\"*\"", fmt::format("{}", grammar.firstOf(Terminal{"*"})));
	ASSERT_EQ("\"<<EOF>>\"", fmt::format("{}", grammar.firstOf(Terminal{"<<EOF>>"})));

	// ASSERT_EQ(2, metadata.epsilon.size());
	// ASSERT_TRUE(metadata.epsilon.find(NonTerminal{"Expr_"}) != metadata.epsilon.end());
	// ASSERT_TRUE(metadata.epsilon.find(NonTerminal{"Term_"}) != metadata.epsilon.end());

	ASSERT_EQ("", fmt::format("{}", grammar.followOf(NonTerminal{"Start"})));
	ASSERT_EQ("\")\", \"<<EOF>>\"", fmt::format("{}", grammar.followOf(NonTerminal{"Expr"})));
	ASSERT_EQ("\")\", \"<<EOF>>\"", fmt::format("{}", grammar.followOf(NonTerminal{"Expr_"})));
	ASSERT_EQ("\")\", \"+\", \"<<EOF>>\"", fmt::format("{}", grammar.followOf(NonTerminal{"Term"})));
	ASSERT_EQ("\")\", \"+\", \"<<EOF>>\"", fmt::format("{}", grammar.followOf(NonTerminal{"Term_"})));
	ASSERT_EQ("\")\", \"*\", \"+\", \"<<EOF>>\"", fmt::format("{}", grammar.followOf(NonTerminal{"Factor"})));
}

TEST(cfg_Grammar, metadata_left_recursive) {
	ConsoleReport report;
	Grammar grammar = GrammarParser(GrammarLexer{
		R"(`Start     ::= Expr "<<EOF>>";
		   `Expr      ::= Expr '+' Term
		   `            | Term;
		   `Term      ::= Term '*' Factor
		   `            | Factor;
		   `Factor    ::= "NUMBER"
		   `            | '(' Expr ')'
		   `            ;
		   `)"_multiline}, &report).parse();

	ASSERT_FALSE(report.containsFailures());
	grammar.finalize();

	grammar.dump();

	ASSERT_EQ("\"(\"", fmt::format("{}", grammar.firstOf(Terminal{"("})));
	ASSERT_EQ("\")\"", fmt::format("{}", grammar.firstOf(Terminal{")"})));
	ASSERT_EQ("\"+\"", fmt::format("{}", grammar.firstOf(Terminal{"+"})));
	ASSERT_EQ("\"*\"", fmt::format("{}", grammar.firstOf(Terminal{"*"})));
	ASSERT_EQ("\"<<EOF>>\"", fmt::format("{}", grammar.firstOf(Terminal{"<<EOF>>"})));

	// TODO ASSERT_EQ(0, metadata.epsilon.size());

	ASSERT_EQ("", fmt::format("{}", grammar.followOf(NonTerminal{"Start"})));
	ASSERT_EQ("\")\", \"+\", \"<<EOF>>\"", fmt::format("{}", grammar.followOf(NonTerminal{"Expr"})));
	ASSERT_EQ("\")\", \"*\", \"+\", \"<<EOF>>\"", fmt::format("{}", grammar.followOf(NonTerminal{"Term"})));
	ASSERT_EQ("\")\", \"*\", \"+\", \"<<EOF>>\"", fmt::format("{}", grammar.followOf(NonTerminal{"Factor"})));
}

// vim:ts=4:sw=4:noet
