// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/GrammarParser.h>
#include <klex/cfg/GrammarLexer.h>
#include <klex/Report.h>
#include <klex/util/testing.h>
#include <klex/util/literals.h>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::util::literals;

const static std::string simpleGrammarSpec =
	R"(`Start ::= A | B;
	   `A     ::= 'a';
	   `B     ::= 'b'     {b1}
	   `        | 'b' B   {b2};
	   `)"_multiline;

TEST(cfg_GrammarParser, parserSimple)
{
	BufferedReport report;
	GrammarParser parser(GrammarLexer{simpleGrammarSpec}, &report);
	Grammar grammar = parser.parse();
	ASSERT_EQ(5, grammar.productions.size());

	ASSERT_EQ("Start", grammar.productions[0].name);
	ASSERT_EQ("A", to_string(grammar.productions[0].handle));
	ASSERT_EQ("Start", grammar.productions[1].name);
	ASSERT_EQ("B", to_string(grammar.productions[1].handle));

	ASSERT_EQ("A", grammar.productions[2].name);
	ASSERT_EQ("\"a\"", to_string(grammar.productions[2].handle));

	ASSERT_EQ("B", grammar.productions[3].name);
	ASSERT_EQ("\"b\" {b1}", to_string(grammar.productions[3].handle));

	ASSERT_EQ("B", grammar.productions[4].name);
	ASSERT_EQ("\"b\" B {b2}", to_string(grammar.productions[4].handle));
}

TEST(cfg_GrammarParser, customTokens)
{
	BufferedReport report;
	Grammar grammar = GrammarParser(GrammarLexer{
			R"(`token {
			   `  SPACE(ignore) ::= [\s\t]+
			   `  NUMBER ::= [0-9]+
			   `}
			   `
			   `Start ::= '(' Number ')';
			   `)"_multiline}, &report).parse();

	ASSERT_FALSE(report.containsFailures());
	grammar.finalize();
}

// vim:ts=4:sw=4:noet
