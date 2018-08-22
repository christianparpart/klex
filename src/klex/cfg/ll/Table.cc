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

SyntaxTable SyntaxTable::construct(const Grammar& grammar)
{
	GrammarMetadata metadata = grammar.metadata();
	map<NonTerminal, int> idNonTerminals = createIdMap(metadata.nonterminals);
	map<Terminal, int> idTerminals = createIdMap(metadata.terminals);
	map<Production, int> idProductions = createIdMap(grammar.productions);
	map<NonTerminal, int> idProductionsByName;
	for (auto i = grammar.productions.begin(); i != grammar.productions.end(); ++i)
		idProductionsByName[NonTerminal{i->name}] = distance(grammar.productions.begin(), i);

	// {{{ dump rules
	printf("PRODUCTIONS:\n");
	printf(" ID | NON-TERMINAL  | EXPRESSION           | FIRST                | FOLLOW\n");
	printf("----+---------------+----------------------+----------------------+--------------------\n");
	for (size_t i = 0; i < grammar.productions.size(); ++i)
		printf("%3zu | %13s | %-20s | %-20s | %s\n",
				i,
				grammar.productions[i].name.c_str(),
				fmt::format("{}", grammar.productions[i].handle).c_str(),
				//fmt::format("{{{}}}", metadata.FIRST[i]).c_str(),
				fmt::format("{{{}}}", metadata.first[NonTerminal{grammar.productions[i].name}]).c_str(),
				fmt::format("{{{}}}", metadata.follow[NonTerminal{grammar.productions[i].name}]).c_str()
		);
	printf("\n");
	// }}}

	SyntaxTable st;

#if 0
	for (size_t p_i = 0; p_i < grammar.productions.size(); ++p_i)
	{
		const Production& p = grammar.productions[p_i];
		const NonTerminal nt { p.name };
		if (!metadata.epsilon.count(NonTerminal{p.name}))
		{
			printf("%s FIRST: %zu\n", p.name.c_str(), metadata.first1[p_i].size());
			for (const Terminal& w : metadata.first1[p_i])
				st.table[idNonTerminals[nt]][idTerminals[w]] = p_i;
		}
		else
		{
			printf("%s FOLLOW: %zu\n", p.name.c_str(), metadata.first1[p_i].size());
			for (const Terminal& w : metadata.first1[p_i])
				st.table[idNonTerminals[nt]][idTerminals[w]] = p_i;
		}
	}
#else
	for (const NonTerminal& nt : metadata.nonterminals)
	{
		const int nt_ = idNonTerminals[nt];
		for (const Production* p : grammar.getProductions(nt))
		{
			for (const Terminal& w : metadata.first1[idProductionsByName[nt]])
			{
				// assert(st.table[nt_].find(idTerminals[w]) == st.table[nt_].end());
				if (!(st.table[nt_].find(idTerminals[w]) == st.table[nt_].end()))
					printf("!!!! Table[%d][%s] = %d\n", nt_, w.literal.c_str(), idProductions[*p]);
				else
					printf("Table[%d][%s] = %d\n", nt_, w.literal.c_str(), idProductions[*p]);
				st.table[nt_][idTerminals[w]] = idProductions[*p];
			}

			// if (metadata.first1[nt].contains(eof))
			// 	st.table[nt_][eof_] = p_;
		}
	}
#endif

	// {{{ dump syntax table
	// table-header
	printf("SYNTAX TABLE:\n");
	printf("%16s |", "NT \\ T");
	for (const Terminal& t : metadata.terminals)
		printf("%10s |", fmt::format("{}", t).c_str());
	printf("\n");
	printf("-----------------+");;
	for (size_t i = 0; i < metadata.terminals.size(); ++i)
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
		for (const Terminal& t : metadata.terminals)
			if (optional<int> p = st.lookup(idNonTerminals[nt], idTerminals[t]); p.has_value())
				printf("%10d |", *p);
			else
				printf("           |");
		printf("\n");
	}
	// }}}

	return move(st);
}

// vim:ts=4:sw=4:noet
