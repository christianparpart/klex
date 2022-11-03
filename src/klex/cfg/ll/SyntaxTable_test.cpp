// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Report.h>
#include <klex/cfg/GrammarLexer.h>
#include <klex/cfg/GrammarParser.h>
#include <klex/cfg/ll/SyntaxTable.h>
#include <klex/util/literals.h>
#include <klex/util/testing.h>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::cfg::ll;
using namespace klex::util::literals;

TEST(cfg_ll_SyntaxTable, construct_right_recursive)
{
	BufferedReport report;
	Grammar grammar = GrammarParser(GrammarLexer{
		R"(`token {
		   `  Spacing(ignore) ::= [\s\t]+
		   `  Number          ::= [0-9]+
		   `}
		   `
		   `Start  ::= Expr;
		   `Expr   ::= Term Expr_;
		   `Expr_  ::= '+' Term Expr_
		   `         | ;
		   `Term   ::= Factor Term_;
		   `Term_  ::= '*' Factor Term_
		   `         | ;
		   `Factor ::= '(' Expr ')'
		   `         | Number
		   `         ;
		   `)"_multiline}, &report).parse();

	ASSERT_FALSE(report.containsFailures());

	grammar.finalize();
	log("Grammar:");
	log(grammar.dump());

	ll::SyntaxTable st = ll::SyntaxTable::construct(grammar);

	log("Syntax Table:");
	log(st.dump(grammar));

	// TODO
}

// vim:ts=4:sw=4:noet
