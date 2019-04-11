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

#include <klex/cfg/GrammarParser.h>
#include <klex/cfg/LeftRecursion.h>
#include <klex/cfg/ll/Analyzer.h>

#include <klex/Report.h>
#include <klex/util/Flags.h>

#include <fmt/format.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::cfg::ll;

string_view const grammarSpec = R"(
token {
  Spacing(ignore) ::= [\s\t\n]+
  Number          ::= 0|[1-9][0-9]*
  Ident           ::= [a-z]+
  Eof             ::= <<EOF>>
}

# NTS     ::= HANDLES            {ACTION_LABELS}
Start     ::= Expr # TODO: Eof
            ;
Expr      ::= Expr '+' Term      {add}
            | Expr '-' Term      {sub}
            | Term
            ;
Term      ::= Term '*' Factor    {mul}
            | Term '/' Factor    {div}
            | Factor
            ;
Factor    ::= Number             {num}
            | Ident              {var}
            | '(' Expr ')'
            ;
)";

using SemanticValue = int;

int main(int argc, const char* argv[])
{
	auto flags = klex::util::Flags{};
	flags.defineBool("dfa", 'x', "Dumps DFA dotfile and exits.");
	flags.enableParameters("EXPRESSION", "Mathematical expression to calculate");
	flags.parse(argc, argv);

	auto report = klex::ConsoleReport{};
	auto grammarParser = GrammarParser{string(grammarSpec), &report};
	auto grammar = grammarParser.parse();

	LeftRecursion{grammar}.direct();
	grammar.finalize();

	cout << grammar.dump() << "\n";

	auto const st = SyntaxTable::construct(grammar);
	st.dump(grammar);

	auto const inputStr = string{"2 + 3 * 4"};

	auto const actions = Analyzer<SemanticValue>::ActionNameMap{
		{"add", [](const auto& args) { return args(-2) + args(-4); }},
		{"mul", [](const auto& args) { return args(-2) * args(-4); }},
		{"num", [](const auto& args) { return stoi(args.lastLiteral()); }},
		{"var", [](const auto& args) { return 3.14; /* TODO */ }}
	};

	auto analyzer = Analyzer<SemanticValue>{st, &report, inputStr, actions};

	optional<SemanticValue> result = analyzer.analyze();
	if (result)
		cout << "Result: " << *result << "\n";
	else
		cout << "No Result.\n";

	return EXIT_SUCCESS;
}
