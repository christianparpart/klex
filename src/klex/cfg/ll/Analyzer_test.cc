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
	Grammar grammar = GrammarParser(R"(`token {
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
		   `)"_multiline,
									&report)
						  .parse();

	ASSERT_FALSE(report.containsFailures());
	grammar.finalize();
	log("GRAMMAR:");
	log(grammar.dump());

	SyntaxTable st = SyntaxTable::construct(grammar);

	log("SYNTAX TABLE:");
	log(st.dump(grammar));

	Analyzer<int> parser(move(st), &report, "2 + 3");

	const optional<int> result = parser.analyze();

	ASSERT_FALSE(report.containsFailures());
	ASSERT_TRUE(result.has_value());
}

TEST(cfg_ll_Analyzer, action1)
{
	BufferedReport report;
	Grammar grammar = GrammarParser(R"(`
			   `token {
			   `  Spacing(ignore) ::= [\s\t\n]+
			   `  Number          ::= [0-9]+
			   `}
			   `Start     ::= F '+' F    {add};
			   `F         ::= Number     {num};
			   `)"_multiline,
									&report)
						  .parse();
	ASSERT_FALSE(report.containsFailures());
	grammar.finalize();

	log("GRAMMAR:");
	log(grammar.dump());

	SyntaxTable st = SyntaxTable::construct(grammar);

	log("SYNTAX TABLE:");
	log(st.dump(grammar));

	deque<vector<int>> valueStack;
	valueStack.emplace_back(vector<int>());
	const auto actionHandler = [&](int id, const Analyzer<int>& analyzer) -> int {
		log(fmt::format("-> run action({}): {}", id, analyzer.actionName(id)));
		if (analyzer.actionName(id) == "add")
			// S = F '+' F <<EOF>> {add}
			return analyzer.semanticValue(-2) + analyzer.semanticValue(-4);
		else if (analyzer.actionName(id) == "num")
			return stoi(analyzer.lastLiteral());  // return valueStack[-1]
		else
		{
			log("!!! UNKNOWN ACTION !!!");
			return -1;
		}
	};

	Analyzer<int> parser(move(st), &report, "2 + 3", actionHandler);
	optional<int> result = parser.analyze();

	ASSERT_TRUE(result.has_value());
	ASSERT_EQ(5, *result);
}

TEST(cfg_ll_Analyzer, ETF_with_actions)
{
	ConsoleReport report;
	Grammar grammar = GrammarParser(
		R"(`token {
		   `  Spacing(ignore) ::= [\s\t\n]+
		   `  Number          ::= [0-9]+
		   `}
		   `Start     ::= Expr;
		   `Expr      ::= Term Expr_
		   `            ;
		   `Expr_     ::= '+' Term Expr_    {add}
		   `            |
		   `            ;
		   `Term      ::= Factor Term_
		   `            ;
		   `Term_     ::= '*' Factor Term_  {mul}
		   `            |
		   `            ;
		   `Factor    ::= Number            {num}
		   `            | '(' Expr ')'
		   `            ;
		   `)"_multiline,
						  &report)
						  .parse();

	ASSERT_FALSE(report.containsFailures());
	grammar.finalize();
	log("GRAMMAR:");
	log(grammar.dump());

	SyntaxTable st = SyntaxTable::construct(grammar);
	log("SYNTAX TABLE:");
	log(st.dump(grammar));

	stack<int> stack;
	const map<int, function<int(const Analyzer<int>&)>> actionMap{
		{st.actionId("num"),
		 [&](const Analyzer<int>& analyzer) -> int {
			 return stoi(analyzer.lastLiteral());
		 }},
		{st.actionId("add"),
		 [&](const Analyzer<int>& analyzer) -> int {
			 return analyzer.semanticValue(-2) + analyzer.semanticValue(-4);
		 }},
		{st.actionId("mul"),
		 [&](const Analyzer<int>& analyzer) -> int {
			 return analyzer.semanticValue(-2) * analyzer.semanticValue(-4);
		 }},
	};

	const auto actionHandler = [&](int id, const Analyzer<int>& analyzer) -> int {
		if (const auto x = actionMap.find(id); x != actionMap.end())
		{
			log(fmt::format("-> run action({}): {}", id, analyzer.actionName(id)));
			return x->second(analyzer);
		}
		assert(!"woot");
		return 0;
	};

	ASSERT_FALSE(report.containsFailures());
	Analyzer<int> parser(move(st), &report, "2 + 3 * 4", actionHandler);
	optional<int> result = parser.analyze();

	EXPECT_FALSE(report.containsFailures());
	ASSERT_TRUE(result.has_value());
	// TODO EXPECT_EQ(14, *result);
}

// vim:ts=4:sw=4:noet
