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

#include <algorithm>

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
	ConsoleReport report;
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

TEST(cfg_GrammarParser, unresolved_nonterminals)
{
	BufferedReport report;
	Grammar grammar = GrammarParser(GrammarLexer{"Start ::= Another"}, &report).parse();
	ASSERT_TRUE(report.containsFailures());

	// TODO: make sure the failure reported is the unresolved-nonterminals case.
}

TEST(cfg_GrammarParser, action)
{
	ConsoleReport report;
	GrammarParser parser = GrammarParser("E ::= 'a' {a};", &report);
	Grammar grammar = parser.parse();
	ASSERT_FALSE(report.containsFailures());
}

TEST(cfg_GrammarParser, action_on_epsilon)
{
	ConsoleReport report;
	GrammarParser parser = GrammarParser("Rule ::= {action};", &report);
	Grammar grammar = parser.parse();
	ASSERT_FALSE(report.containsFailures());
}

struct CheckTerminalPattern {
	string pattern;
	bool operator()(const Terminal& w) const { return pattern == w.pattern(); }
};

TEST(cfg_GrammarParser, customTokens)
{
	BufferedReport report;
	Grammar grammar = GrammarParser(GrammarLexer{
			R"(`token {
			   `  Spacing(ignore) ::= [\s\t]+
			   `  Number          ::= [0-9]+
			   `}
			   `
			   `Start ::= '(' Number ')';
			   `)"_multiline}, &report).parse();

	ASSERT_FALSE(report.containsFailures());
	grammar.finalize();

	log(grammar.dump());

	for (const Terminal& w : grammar.terminals)
		logf("Terminal: {}", w);

	// verify presense of all terminals in the grammar
	ASSERT_EQ(5, grammar.terminals.size());
	ASSERT_TRUE(any_of(begin(grammar.terminals), end(grammar.terminals), CheckTerminalPattern{"[0-9]+"}));
	ASSERT_TRUE(any_of(begin(grammar.terminals), end(grammar.terminals), CheckTerminalPattern{"[\\s\\t]+"}));
	ASSERT_TRUE(any_of(begin(grammar.terminals), end(grammar.terminals), CheckTerminalPattern{"("}));
	ASSERT_TRUE(any_of(begin(grammar.terminals), end(grammar.terminals), CheckTerminalPattern{")"}));
	ASSERT_TRUE(any_of(begin(grammar.terminals), end(grammar.terminals), CheckTerminalPattern{"<<EOF>>"}));

	// verify production rule to be in the form as the input mandates
	const auto symbols = klex::cfg::symbols(grammar.productions[0].handle);
	ASSERT_EQ(4, symbols.size());

	ASSERT_TRUE(holds_alternative<Terminal>(symbols[0]));
	ASSERT_TRUE(holds_alternative<Terminal>(symbols[1]));
	ASSERT_TRUE(holds_alternative<Terminal>(symbols[2]));
	ASSERT_TRUE(holds_alternative<Terminal>(symbols[3]));

	ASSERT_EQ("(", get<Terminal>(symbols[0]).pattern());

	ASSERT_EQ("Number", get<Terminal>(symbols[1]).name);
	ASSERT_EQ("[0-9]+", get<Terminal>(symbols[1]).pattern());

	ASSERT_EQ(")", get<Terminal>(symbols[2]).pattern());

	ASSERT_EQ("EOF", get<Terminal>(symbols[3]).name);
	ASSERT_EQ("<<EOF>>", get<Terminal>(symbols[3]).pattern());
}

// vim:ts=4:sw=4:noet
