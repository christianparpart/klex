// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/cfg/ll/Table.h>
#include <klex/Report.h>

#include <algorithm>
#include <optional>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

namespace klex::cfg::ll {

template<typename Lexer, typename SemanticValue>
class Analyzer {
public:
	using Token = typename Lexer::value_type;
	using Traits = TokenTraits<Token>;

	Analyzer(SyntaxTable table, Lexer lexer, Report* report);

	void analyze();

private:
	std::optional<std::vector<int>> getHandleFor(int nonterminal, int currentTerminal) const;

private:
	const SyntaxTable grammar_;
	Lexer lexer_;
	Report* report_;
	std::stack<int> stack_;
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
void Analyzer<Lexer, SemanticValue>::analyze() {
	auto currentToken = lexer_.begin();
	for (;;) {
		auto X = stack_.top();

		if (X == currentToken && Traits::isEof(*currentToken))
			return; // fully parsed program, and success

		if (X == currentToken && Traits::isTerminal(*currentToken)) {
			stack_.pop();
			++currentToken;
		}

		if (Traits::isNonTerminal(X)) {
			std::optional<Handle> handle = getHandleFor(X, *currentToken);
			if (handle.has_value()) {
				// XXX applying production ``X -> handle``
				stack_.pop();
				for (auto x = handle->rbegin(); x != handle->rend(); x++) {
					stack_.push(*x);
				}
			} else {
				// XXX parse error. Cannot reduce non-terminal X. No production found.
			}
		}
	}
}

} // namespace klex::cfg::ll

// vim:ts=4:sw=4:noet
