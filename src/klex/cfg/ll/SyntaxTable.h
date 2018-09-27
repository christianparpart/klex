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

	std::vector<std::string> names;
	std::vector<std::string> terminalNames;
	std::vector<std::string> nonterminalNames;
	std::vector<std::string> actionNames;
	std::vector<std::string> productionNames;
	ProductionVec productions;
	NonTerminalMap table;
	int startSymbol;
	regular::LexerDef lexerDef;

	int actionId(const std::string& name) const
	{
		return actionMin() + std::distance(std::begin(actionNames),
				std::find_if(std::begin(actionNames), std::end(actionNames),
							 [&](const std::string& n) { return n == name; }));
	}

	std::optional<int> lookup(int nonterminal, int lookahead) const;

	size_t nonterminalCount() const noexcept { return nonterminalNames.size(); }
	size_t terminalCount() const noexcept { return terminalNames.size(); }

	int nonterminalMin() const noexcept { return 0; }
	int nonterminalMax() const noexcept
	{
		return nonterminalMin() + static_cast<int>(nonterminalNames.size()) - 1;
	}

	int terminalMin() const noexcept { return nonterminalMax() + 1; }
	int terminalMax() const noexcept { return terminalMin() + static_cast<int>(terminalNames.size()) - 1; }

	int actionMin() const noexcept { return terminalMax() + 1; }
	int actionMax() const noexcept { return actionMin() + static_cast<int>(actionNames.size()) - 1; }

	bool isNonTerminal(int id) const noexcept { return id >= nonterminalMin() && id <= nonterminalMax(); }
	bool isTerminal(int id) const noexcept { return id >= terminalMin() && id <= terminalMax(); }
	bool isAction(int id) const noexcept { return id >= actionMin() && id <= actionMax(); }

	const std::string& terminalName(int s) const noexcept { return terminalNames[s - terminalMin()]; }

	const std::string& nonterminalName(int s) const noexcept
	{
		return nonterminalNames[s - nonterminalMin()];
	}

	const std::string& actionName(int s) const noexcept { return actionNames[s - actionMin()]; }

	std::string dump(const Grammar& grammar) const;

	static SyntaxTable construct(const Grammar& grammar);
};

}  // namespace klex::cfg::ll

// vim:ts=4:sw=4:noet
