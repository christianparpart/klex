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

	std::vector<std::string> terminalNames;
	std::vector<std::string> nonterminalNames;
	std::vector<std::string> productionNames;
	std::vector<std::string> actionNames;
	ProductionVec productions;
	NonTerminalMap table;
	int startSymbol;
	regular::LexerDef lexerDef;

	std::optional<int> lookup(int nonterminal, int lookahead) const;

	size_t nonterminalCount() const noexcept { return nonterminalNames.size(); }
	size_t terminalCount() const noexcept { return terminalNames.size(); }

	size_t nonterminalMin() const noexcept { return 0; }
	size_t nonterminalMax() const noexcept { return nonterminalMin() + nonterminalNames.size() - 1; }

	size_t terminalMin() const noexcept { return nonterminalMax() + 1; }
	size_t terminalMax() const noexcept { return terminalMin() + terminalNames.size() - 1; }

	size_t actionMin() const noexcept { return terminalMax() + 1; }

	const std::string& terminalName(int s) const noexcept { return terminalNames[s - terminalMin()]; }

	const std::string& nonterminalName(int s) const noexcept
	{
		return nonterminalNames[s - nonterminalMin()];
	}

	const std::string& actionName(int s) const noexcept { return actionNames[s - actionMin()]; }

	bool isNonTerminal(int id) const noexcept
	{
		const int minValue = 0;
		const int maxValue = minValue + static_cast<int>(nonterminalCount()) - 1;
		return id >= minValue && id <= maxValue;
	}

	bool isTerminal(int id) const noexcept
	{
		const int minValue = nonterminalCount();
		const int maxValue = minValue + static_cast<int>(terminalCount()) - 1;
		return id >= minValue && id <= maxValue;
	}

	bool isAction(int id) const noexcept
	{
		const int minValue = static_cast<int>(terminalCount() + nonterminalCount());
		const int maxValue = minValue + actionNames.size() - 1;
		return id >= minValue && id <= maxValue;
	}

	std::string dump(const Grammar& grammar) const;

	static SyntaxTable construct(const Grammar& grammar);
};

}  // namespace klex::cfg::ll

// vim:ts=4:sw=4:noet
