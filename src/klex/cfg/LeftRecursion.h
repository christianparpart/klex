// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/cfg/Grammar.h>
#include <list>
#include <utility>
#include <vector>

namespace klex::cfg {

/**
 * Eliminates left-recursion by rewriting a Grammar into an equivalent right-recursion grammar.
 *
 * @note This transformation is required for LL parsers.
 */
class LeftRecursion {
  public:
	explicit LeftRecursion(Grammar& _grammar);

	static bool isLeftRecursive(const Grammar& grammar);

	void direct();
	void indirect();

  private:
	std::list<Production*> select(const NonTerminal& lhs, const NonTerminal& first);
	void eliminateDirect(const NonTerminal& nt);

	/**
	 * Creates a unique nonterminal symbol that by name relates to @p nt.
	 */
	[[nodiscard]] NonTerminal createRelatedNonTerminal(const NonTerminal& nt) const;

	/**
	 * Splits all productions of the same nonterminal into a vector of left-recursives and the rest.
	 */
	[[nodiscard]] std::pair<std::vector<Production*>, std::vector<Production*>> split(std::vector<Production*> productions) const;

  private:
	Grammar& grammar_;
};

}  // namespace klex::cfg
