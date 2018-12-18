// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/regular/Alphabet.h>
#include <klex/regular/NFA.h>
#include <klex/regular/RegExpr.h>

#include <fmt/format.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <tuple>
#include <vector>

namespace klex::regular {

class DFA;

/*!
 * Generates a finite automaton from the given input (a regular expression).
 */
class NFABuilder {
  public:
	explicit NFABuilder() : fa_{} {}

	NFA construct(const RegExpr& re, Tag tag);
	NFA construct(const RegExpr& re);
	void operator()(const LookAheadExpr& lookaheadExpr);
	void operator()(const ConcatenationExpr& concatenationExpr);
	void operator()(const AlternationExpr& alternationExpr);
	void operator()(const CharacterExpr& characterExpr);
	void operator()(const CharacterClassExpr& characterClassExpr);
	void operator()(const ClosureExpr& closureExpr);
	void operator()(const BeginOfLineExpr& bolExpr);
	void operator()(const EndOfLineExpr& eolExpr);
	void operator()(const EndOfFileExpr& eofExpr);
	void operator()(const DotExpr& dotExpr);
	void operator()(const EmptyExpr& emptyExpr);

  private:
	NFA fa_;
	std::optional<StateId> acceptState_;
};

}  // namespace klex::regular
