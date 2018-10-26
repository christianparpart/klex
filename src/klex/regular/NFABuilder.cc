// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/regular/DFA.h>
#include <klex/regular/NFABuilder.h>

using namespace std;

namespace klex::regular {

NFA NFABuilder::construct(const RegExpr* re, Tag tag)
{
	const_cast<RegExpr*>(re)->accept(*this);

	// fa_.setAccept(acceptState_.value_or(fa_.acceptStateId()), tag);
	if (acceptState_)
	{
		fa_.setAccept(acceptState_.value(), tag);
	}
	else
	{
		fa_.setAccept(tag);
	}

	return move(fa_);
}

NFA NFABuilder::construct(const RegExpr* re)
{
	const_cast<RegExpr*>(re)->accept(*this);
	return move(fa_);
}

void NFABuilder::visit(LookAheadExpr& lookaheadExpr)
{
	// fa_ = move(construct(lookaheadExpr.leftExpr()).lookahead(construct(lookaheadExpr.rightExpr())));
	NFA lhs = construct(lookaheadExpr.leftExpr());
	NFA rhs = construct(lookaheadExpr.rightExpr());
	lhs.lookahead(move(rhs));
	fa_ = move(lhs);
}

void NFABuilder::visit(AlternationExpr& alternationExpr)
{
	NFA lhs = construct(alternationExpr.leftExpr());
	NFA rhs = construct(alternationExpr.rightExpr());
	lhs.alternate(move(rhs));
	fa_ = move(lhs);
}

void NFABuilder::visit(ConcatenationExpr& concatenationExpr)
{
	NFA lhs = construct(concatenationExpr.leftExpr());
	NFA rhs = construct(concatenationExpr.rightExpr());
	lhs.concatenate(move(rhs));
	fa_ = move(lhs);
}

void NFABuilder::visit(CharacterExpr& characterExpr)
{
	fa_ = NFA{characterExpr.value()};
}

void NFABuilder::visit(CharacterClassExpr& characterClassExpr)
{
	fa_ = NFA{characterClassExpr.value()};
}

void NFABuilder::visit(ClosureExpr& closureExpr)
{
	const unsigned xmin = closureExpr.minimumOccurrences();
	const unsigned xmax = closureExpr.maximumOccurrences();
	constexpr unsigned Infinity = numeric_limits<unsigned>::max();

	if (xmin == 0 && xmax == 1)
		fa_ = move(construct(closureExpr.subExpr()).optional());
	else if (xmin == 0 && xmax == Infinity)
		fa_ = move(construct(closureExpr.subExpr()).recurring());
	else if (xmin == 1 && xmax == Infinity)
		fa_ = move(construct(closureExpr.subExpr()).positive());
	else if (xmin < xmax)
		fa_ = move(construct(closureExpr.subExpr()).repeat(xmin, xmax));
	else if (xmin == xmax)
		fa_ = move(construct(closureExpr.subExpr()).times(xmin));
	else
		throw invalid_argument{"closureExpr"};
}

void NFABuilder::visit(BeginOfLineExpr& bolExpr)
{
	fa_ = NFA{Symbols::Epsilon};
}

void NFABuilder::visit(EndOfLineExpr& eolExpr)
{
	// NFA lhs;
	// NFA rhs{'\n'};
	// lhs.lookahead(move(rhs));
	// fa_ = move(lhs);
	fa_ = move(NFA{}.lookahead(NFA{'\n'}));
}

void NFABuilder::visit(EndOfFileExpr& eofExpr)
{
	fa_ = NFA{Symbols::EndOfFile};
}

void NFABuilder::visit(DotExpr& dotExpr)
{
	// any character except LF
	fa_ = NFA{'\t'};
	for (int ch = 32; ch < 127; ++ch)
	{
		fa_.addTransition(fa_.initialStateId(), ch, fa_.acceptStateId());
	}
}

void NFABuilder::visit(EmptyExpr& emptyExpr)
{
	fa_ = NFA{Symbols::Epsilon};
}

}  // namespace klex::regular
