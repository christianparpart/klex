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

#include <optional>
#include <stack>
#include <utility>
#include <variant>

namespace klex::cfg::ll {

template <typename SemanticValue>
class Analyzer {
  public:
	using Terminal = typename regular::Lexer<regular::Tag>::value_type;
	using NonTerminal = unsigned int;
	using Action = long long int;
	using StackValue = std::variant<Terminal, NonTerminal, Action>;

	Analyzer(SyntaxTable table, Report* report, std::string input);

	void analyze();

  private:
	std::optional<SyntaxTable::Expression> getHandleFor(NonTerminal nonterminal,
														Terminal currentTerminal) const;

  private:
	const SyntaxTable def_;
	regular::Lexer<regular::Tag> lexer_;
	Report* report_;
	std::stack<StackValue> stack_;
};

}  // namespace klex::cfg::ll

#include <klex/cfg/ll/Analyzer-inl.h>

// vim:ts=4:sw=4:noet
