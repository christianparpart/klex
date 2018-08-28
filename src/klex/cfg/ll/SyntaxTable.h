// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/regular/LexerDef.h>

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

// using Symbol = int;
// using Handle = std::vector<Symbol>;

/** LL(1)-compatible syntax table.
 */
struct SyntaxTable {
	using Expression = std::vector<int>;  // non-terminals & terminals
	using LookAheadMap = std::unordered_map<int /*lookahead*/, int /*production*/>;
	using NonTerminalMap = std::unordered_map<int /*nonterminals*/, LookAheadMap>;
	using ProductionVec = std::vector<Expression>;

	std::vector<std::string> nonterminalNames;
	std::vector<size_t> productionNonTerminals;  // mapping to nonterminalNames[] for productionId
	ProductionVec productions;
	NonTerminalMap table;
	regular::LexerDef lexerDef;

	std::optional<int> lookup(int nonterminal, int lookahead) const;

	size_t nonterminalCount() const { return nonterminalNames.size(); }
	bool isNonTerminal(int id) const { return id < static_cast<int>(nonterminalNames.size()); }
	bool isTerminal(int id) const { return lexerDef.isValidTag(id); }

	std::string dump(const Grammar& grammar) const;

	static SyntaxTable construct(const Grammar& grammar);
};

}  // namespace klex::cfg::ll

// vim:ts=4:sw=4:noet
