// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Report.h>
#include <klex/cfg/Grammar.h>
#include <klex/cfg/GrammarLexer.h>
#include <klex/cfg/GrammarParser.h>
#include <klex/cfg/ll/Analyzer.h>
#include <klex/cfg/ll/SyntaxTable.h>
#include <klex/regular/Compiler.h>
#include <klex/regular/Lexer.h>
#include <klex/util/literals.h>
#include <klex/util/testing.h>
#include <variant>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::cfg::ll;
using namespace klex::util::literals;

const std::string balancedParentheses = "A ::= '(' A ')' | '(' ')'";

TEST(cfg_ll_Analyzer, ETF)
{
	ConsoleReport report;
	Grammar grammar = GrammarParser(
						  GrammarLexer{
							  R"(`token {
		   `  Spacing(ignore) ::= [\s\t\n]+
		   `  Number          ::= [0-9]+
		   `}
		   `Start     ::= Expr;
		   `Expr      ::= Term Expr_;
		   `Expr_     ::= '+' Term Expr_
		   `            | ;
		   `Term      ::= Factor Term_;
		   `Term_     ::= '*' Factor Term_
		   `            | ;
		   `Factor    ::= Number
		   `            | '(' Expr ')'
		   `            ;
		   `)"_multiline},
						  &report)
						  .parse();

	ASSERT_FALSE(report.containsFailures());
	grammar.finalize();
	log("GRAMMAR:");
	log(grammar.dump());

	SyntaxTable st = SyntaxTable::construct(grammar);

	log("SYNTAX TABLE:");
	log(st.dump(grammar));

	Analyzer<int> parser(move(st), &report, "2 + 3 * 4");

	parser.analyze();

	ASSERT_FALSE(report.containsFailures());
}

TEST(cfg_ll_Analyzer, ETF_with_actions)
{
	BufferedReport report;
	Grammar grammar = GrammarParser(
						  R"(`token {
		   `  Spacing(ignore) ::= [\s\t\n]+
		   `  Number          ::= [0-9]+
		   `}
		   `Start     ::= E;
		   `E         ::= T E_
		   `            ;
		   `E_        ::= '+' T E_    {add}
		   `            |
		   `            ;
		   `T         ::= F T_
		   `            ;
		   `T_        ::= '*' F T_    {mul}
		   `            |
		   `            ;
		   `F         ::= Number      {num}
		   `            | '(' E ')'
		   `            ;
		   `)"_multiline,
						  &report)
						  .parse();

	ASSERT_FALSE(report.containsFailures());
	grammar.finalize();
	// log("GRAMMAR:");
	// log(grammar.dump());

	SyntaxTable st = SyntaxTable::construct(grammar);
	// log("SYNTAX TABLE:");
	// log(st.dump(grammar));

	stack<int> stack;
	const map<int, function<int(const Analyzer<int>&)>> actionMap{
		{st.actionId("num"),
		 [&](const Analyzer<int>& analyzer) -> int {
			 stack.push(stoi(analyzer.lastLiteral()));
			 return stack.top();
		 }},
		{st.actionId("add"),
		 [&](const Analyzer<int>& analyzer) -> int {
			 const int a = stack.top();
			 stack.pop();
			 const int b = stack.top();
			 stack.pop();
			 stack.push(a + b);
			 return stack.top();
		 }},
		{st.actionId("mul"),
		 [&](const Analyzer<int>& analyzer) -> int {
			 const int a = stack.top();
			 stack.pop();
			 const int b = stack.top();
			 stack.pop();
			 stack.push(a * b);
			 return stack.top();
		 }},
	};

	const auto actionHandler = [&](int id, const Analyzer<int>& analyzer) -> int {
		if (const auto x = actionMap.find(id); x != actionMap.end())
		{
			log(fmt::format("-> run action({}): {}", id, analyzer.actionName(id)));
			return x->second(analyzer);
		}
		return 0;
	};

	Analyzer<int> parser(move(st), &report, "2 + 3 * 4", actionHandler);
	parser.analyze();

	ASSERT_FALSE(report.containsFailures());
	ASSERT_EQ(1, stack.size());
	EXPECT_EQ(14, stack.top());
}

// vim:ts=4:sw=4:noet
