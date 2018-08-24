// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/cfg/ll/SyntaxTable.h>
#include <klex/util/iterator.h>
#include <klex/Report.h>

#include <algorithm>
#include <optional>
#include <stack>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace klex::cfg::ll {

template<typename Lexer, typename SemanticValue>
class Analyzer {
public:
	using Terminal = typename Lexer::value_type;
	using NonTerminal = int;
	using StackValue = std::variant<Terminal, Symbol>;

	Analyzer(SyntaxTable table, Lexer lexer, Report* report);

	void analyze();

private:
	std::optional<SyntaxTable::Expression> getHandleFor(int nonterminal, int currentTerminal) const;

private:
	const SyntaxTable grammar_;
	Lexer lexer_;
	Report* report_;
	std::stack<StackValue> stack_;
};

// ---------------------------------------------------------------------------------------------------------

template<typename Lexer, typename SemanticValue>
Analyzer<Lexer, SemanticValue>::Analyzer(SyntaxTable grammar, Lexer lexer, Report* report)
	: grammar_{std::move(grammar)},
	  lexer_{std::move(lexer)},
	  report_{report},
	  stack_{} {
}

template<typename Lexer, typename SemanticValue>
void Analyzer<Lexer, SemanticValue>::analyze()
{
	using ::klex::util::reversed;

	const auto eof = lexer_.end();
	auto currentToken = lexer_.begin();

	for (;;) {
		const StackValue X = stack_.top();

		// if (currentToken == eof && holds_alternative<Terminal>(X) && get<Terminal>(X) == *currentToken)
		if (X == *currentToken && currentToken == eof)
			return; // fully parsed program, and success

		if (std::holds_alternative<Terminal>(*currentToken)
			&& X == std::get<Terminal>(*currentToken))
		{
			stack_.pop();
			++currentToken;
		}

		if (std::holds_alternative<NonTerminal>(X)) {
			std::optional<Handle> handle = getHandleFor(X, *currentToken);
			if (handle.has_value()) {
				// XXX applying production ``X -> handle``
				stack_.pop();
				for (auto symbol : reversed(*handle)) {
					stack_.push(symbol);
				}
			} else {
				// XXX parse error. Cannot reduce non-terminal X. No production found.
				report_->syntaxError(SourceLocation{/*TODO*/}, "Syntax error detected."); // TODO: more elaborated diagnostics
			}
		}
	}
}

} // namespace klex::cfg::ll

// vim:ts=4:sw=4:noet
