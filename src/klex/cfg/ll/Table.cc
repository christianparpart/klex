// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/ll/Table.h>
#include <klex/cfg/Grammar.h>

#include <algorithm>
#include <map>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::cfg::ll;

template<typename T, typename V = int>
inline map<T, V> createIdMap(const vector<T>& items)
{
	map<T, V> out;
	for (auto i = items.begin(); i != items.end(); ++i)
		out[*i] = distance(items.begin(), i);
	return move(out);
}

void SyntaxTable::dump(const Grammar& grammar) const
{
	map<NonTerminal, int> idNonTerminals = createIdMap(grammar.nonterminals);
	map<Terminal, int> idTerminals = createIdMap(grammar.terminals);

	// table-header
	printf("SYNTAX TABLE:\n");
	printf("%16s |", "NT \\ T");
	for (const Terminal& t : grammar.terminals)
		printf("%10s |", fmt::format("{}", t).c_str());
	printf("\n");
	printf("-----------------+");;
	for (size_t i = 0; i < grammar.terminals.size(); ++i)
		printf("-----------+");;
	printf("\n");

	// table-body
	set<NonTerminal> check;
	for (const Production& production : grammar.productions)
	{
		const NonTerminal nt { production.name };
		if (check.count(nt)) continue;
		check.insert(nt);
		printf("%16s |", nt.name.c_str());
		for (const Terminal& t : grammar.terminals)
			if (optional<int> p = lookup(idNonTerminals[nt], idTerminals[t]); p.has_value())
				printf("%10d |", *p);
			else
				printf("           |");
		printf("\n");
	}
}

SyntaxTable SyntaxTable::construct(const Grammar& grammar)
{
	map<NonTerminal, int> idNonTerminals = createIdMap(grammar.nonterminals);
	map<Terminal, int> idTerminals = createIdMap(grammar.terminals);
	map<NonTerminal, int> idProductionsByName;
	for (auto i = grammar.productions.begin(); i != grammar.productions.end(); ++i)
		idProductionsByName[NonTerminal{i->name}] = distance(grammar.productions.begin(), i);

	SyntaxTable st;

	for (const NonTerminal& nt : grammar.nonterminals)
	{
		const int nt_ = idNonTerminals[nt];
		LookAheadMap& ntRow = st.table[nt_];
		for (const Production* p : grammar.getProductions(nt))
		{
			for (const Terminal& w : p->first1())
			{
				assert(ntRow.find(idTerminals[w]) == ntRow.end());
				ntRow[idTerminals[w]] = p->id;
			}

			// if (p->first1().contains(eof))
			// 	st.table[nt_][eof_] = p_;
		}
	}

	return move(st);
}

// vim:ts=4:sw=4:noet
