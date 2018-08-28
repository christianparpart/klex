// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/Report.h>
#include <klex/cfg/ll/SyntaxTable.h>
#include <klex/regular/Lexer.h>
#include <klex/regular/LexerDef.h>
#include <klex/util/iterator.h>

#include <algorithm>
#include <optional>
#include <stack>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace klex::cfg::ll {

template <typename SemanticValue>
class Analyzer {
  public:
	using Terminal = typename regular::Lexer<regular::Tag>::value_type;
	using NonTerminal = int;
	using StackValue = std::variant<Terminal, NonTerminal>;

	Analyzer(SyntaxTable table, Report* report, std::string input);

	void analyze();

  private:
	std::optional<SyntaxTable::Expression> getHandleFor(int nonterminal, int currentTerminal) const;

  private:
	const SyntaxTable def_;
	regular::Lexer<regular::Tag> lexer_;
	Report* report_;
	std::stack<StackValue> stack_;
};

// ---------------------------------------------------------------------------------------------------------

template <typename SemanticValue>
Analyzer<SemanticValue>::Analyzer(SyntaxTable _st, Report* _report, std::string _source)
	: def_{std::move(_st)}, lexer_{def_.lexerDef, std::move(_source)}, report_{_report}, stack_{}
{
}

template <typename SemanticValue>
std::optional<SyntaxTable::Expression> Analyzer<SemanticValue>::getHandleFor(int nonterminal,
																			 int currentTerminal) const
{
	if (std::optional<int> p_i = def_.lookup(nonterminal, currentTerminal); p_i.has_value())
	{
		const SyntaxTable::Expression& p = def_.productions[*p_i];
		// TODO
	}

	return std::nullopt;
}

template <typename SemanticValue>
void Analyzer<SemanticValue>::analyze()
{
	using namespace std;
	using ::klex::util::reversed;

	const auto eof = lexer_.end();
	auto currentToken = lexer_.begin();

	for (;;)
	{
		const StackValue X = stack_.top();

		if (currentToken == eof && holds_alternative<Terminal>(X) && get<Terminal>(X) == *currentToken)
			// if (X == *currentToken && currentToken == eof)
			return;  // fully parsed program, and success

		if (holds_alternative<Terminal>(X) && get<Terminal>(X) == *currentToken)
		{
			stack_.pop();
			++currentToken;
		}
		else  // if (holds_alternative<NonTerminal>(X))
		{
			assert(holds_alternative<NonTerminal>(X));

			optional<SyntaxTable::Expression> handle = getHandleFor(X, *currentToken);
			if (handle.has_value())
			{
				// XXX applying production ``X -> handle``
				stack_.pop();
				for (const auto symbol : reversed(*handle))
				{
					;  // stack_.push(symbol);
				}
			}
			else
			{
				// XXX parse error. Cannot reduce non-terminal X. No production found.
				report_->syntaxError(SourceLocation{/*TODO*/},
									 "Syntax error detected.");  // TODO: more elaborated diagnostics
			}
		}
	}
}

}  // namespace klex::cfg::ll

// vim:ts=4:sw=4:noet
