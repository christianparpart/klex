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
	using LookAheadMap = std::unordered_map<int /*lookahead*/, int /*production*/>;
	using NonTerminalMap = std::unordered_map<int /*nonterminals*/, LookAheadMap>;

	NonTerminalMap table;

	std::optional<int> lookup(int nonterminal, int lookahead) const;

	std::string dump(const Grammar& grammar) const;

	static SyntaxTable construct(const Grammar& grammar);
};

} // namespace klex::cfg::ll

// vim:ts=4:sw=4:noet
