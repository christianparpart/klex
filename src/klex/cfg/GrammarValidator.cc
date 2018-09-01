// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/Grammar.h>
#include <klex/cfg/GrammarValidator.h>
#include <variant>

using namespace std;
using namespace klex;
using namespace klex::cfg;

void GrammarValidator::validate(const Grammar& G)
{
	for (const Production& p : G.productions)
		for (const Symbol& b : p.handle.symbols)
			if (holds_alternative<NonTerminal>(b))
				if (!G.containsProduction(get<NonTerminal>(b)))
					report_->typeError(SourceLocation{/*TODO: b.location()*/},
									   "Non-terminal {} is missing a production rule.", b);

	// TODO: check for unwanted infinite recursions
	// such as: E ::= E
}
