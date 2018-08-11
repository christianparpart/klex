// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <algorithm>
#include <optional>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

namespace klex::cfg {
	struct Grammar;
}

namespace klex::cfg::ll {

using Symbol = int;
using Handle = std::vector<Symbol>;

template<typename T>
struct TokenTraits {
	static bool isEof(T X);
	static bool isTerminal(T X);
	static bool isNonTerminal(T X);
};

/** LL(1)-compatible syntax table.
 */
struct SyntaxTable {
	// non-terminals ::= { x | x in keys of table }
	// terminals ::= { x | x in table and not in non-terminal }

	std::unordered_map<int /*(LHS) non-terminal*/,
					std::unordered_map<int /*RHS's first-set's terminal*/,
							 		   int /*RHS's production*/>> table;

	std::optional<int> lookup(int nonterminal, int lookahead) const {
		auto i = table.find(nonterminal);
		if (i == table.end())
			return std::nullopt;

		auto k = i->second.find(lookahead);
		if (k == i->second.end())
			return std::nullopt;

		return k->second;
	}

	static SyntaxTable construct(const Grammar& grammar);
};

} // namespace klex::cfg::ll

// vim:ts=4:sw=4:noet
