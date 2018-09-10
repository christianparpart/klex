// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/regular/LexerDef.h>
#include <klex/util/iterator.h>
#include <algorithm>

namespace klex::cfg::ll {

template <typename SemanticValue>
Analyzer<SemanticValue>::Analyzer(SyntaxTable _st, Report* _report, std::string _source)
	: def_{std::move(_st)}, lexer_{def_.lexerDef, std::move(_source)}, report_{_report}, stack_{}
{
}

template <typename SemanticValue>
std::optional<SyntaxTable::Expression> Analyzer<SemanticValue>::getHandleFor(NonTerminal nonterminal,
																			 Terminal currentTerminal) const
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

	// TODO: put start symbol onto stack

	for (;;)
	{
		if (currentToken == eof && stack_.empty())
			// if (currentToken == eof && holds_alternative<Terminal>(X) && get<Terminal>(X) == *currentToken)
			// if (X == *currentToken && currentToken == eof)
			return;  // fully parsed program, and success

		const StackValue X = stack_.top();

		if (holds_alternative<Terminal>(X) && get<Terminal>(X) == *currentToken)
		{
			stack_.pop();
			++currentToken;
		}
		else if (holds_alternative<NonTerminal>(X))
		{
			assert(holds_alternative<NonTerminal>(X));

			if (optional<SyntaxTable::Expression> handle = getHandleFor(get<NonTerminal>(X), *currentToken);
				handle.has_value())
			{
				// XXX applying production ``X -> handle``
				stack_.pop();
				for (const auto x : reversed(*handle))
				{
					;  // stack_.push(x);
				}
			}
			else
			{
				// XXX parse error. Cannot reduce non-terminal X. No production found.
				report_->syntaxError(SourceLocation{/*TODO*/},
									 "Syntax error detected.");  // TODO: more elaborated diagnostics
			}
		}
		else if (holds_alternative<Action>(X))
		{
			// TODO: run action
			stack_.pop();
		}
	}
}

}  // namespace klex::cfg::ll
