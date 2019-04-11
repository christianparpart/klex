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

using namespace klex;
using namespace klex::cfg;
using namespace klex::cfg::ll;

std::string_view const grammarSpec = R"(
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

int main(int argc, const char* argv[])
{
	auto flags = klex::util::Flags{};
	flags.defineBool("dfa", 'x', "Dumps DFA dotfile and exits.");
	flags.enableParameters("EXPRESSION", "Mathematical expression to calculate");
	flags.parse(argc, argv);

	klex::ConsoleReport report;
	klex::cfg::GrammarParser grammarParser(std::string(grammarSpec), &report);
	klex::cfg::Grammar g = grammarParser.parse();
	klex::cfg::LeftRecursion{g}.direct();
	g.finalize();  // ?

	std::cout << g.dump() << "\n";

	klex::cfg::ll::SyntaxTable st = klex::cfg::ll::SyntaxTable::construct(g);

	st.dump(g);

	const std::string inputStr = "2 + 3 * 4";

	using SemanticValue = int;
	klex::cfg::ll::Analyzer<SemanticValue>::ActionHandlerMap actions = {
		{st.actionId("add"),
		 [](const Analyzer<SemanticValue>::Context&) -> SemanticValue {
			 std::cout << fmt::format("call action add\n");
			 return SemanticValue{42};
		 }},
		{st.actionId("mul"),
		 [](const Analyzer<SemanticValue>::Context&) -> SemanticValue {
			 std::cout << fmt::format("call action mul\n");
			 return SemanticValue{42};
		 }},
		{st.actionId("num"),
		 [](const Analyzer<SemanticValue>::Context&) -> SemanticValue {
			 std::cout << fmt::format("call action num\n");
			 return SemanticValue{42};
		 }}};

	klex::cfg::ll::Analyzer<SemanticValue> analyzer(st, &report, inputStr, actions);

#if 1 == 0
	analyzer.action("add", [](const auto& args) { return args[1] + args[2]; });
	analyzer.action("sub", [](const auto& args) { return args[1] - args[2]; });
	analyzer.action("mul", [](const auto& args) { return args[1] * args[2]; });
	analyzer.action("div", [](const auto& args) { return args[1] / args[2]; });
	analyzer.action("num", [](const auto& args) { return stoi(literal(args[1])); });
	analyzer.action("var", [](const auto& args) {
		if (literal(args[1]) == "pi")
			return 3.14;
		else
			abort();
	});
#endif

	std::optional<SemanticValue> result = analyzer.analyze();
	if (result)
		std::cout << "Result: " << *result << "\n";
	else
		std::cout << "No Result.\n";

	return EXIT_SUCCESS;
}
