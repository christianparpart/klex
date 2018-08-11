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
map<T, V> createIdMap(const vector<T>& items)
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

	SyntaxTable st;

	for (const NonTerminal& nt : metadata.nonterminals)
	{
		const int nt_ = idNonTerminals[nt];
		for (const Production* p : grammar.getProductions(nt))
		{
			for (const Terminal& t : metadata.first1[nt])
				st.table[nt_][idTerminals[t]] = idProductions[*p];

			// if (metadata.first1[nt].contains(eof))
			// 	st.table[nt_][eof_] = p_;
		}
	}

	// dump rules
	printf("PRODUCTIONS:\n");
	printf(" ID | NON-TERMINAL  | EXPRESSION           | FIRST                | FOLLOW\n");
	printf("----+---------------+----------------------+----------------------+--------------------\n");
	for (size_t i = 0; i < grammar.productions.size(); ++i)
		printf("%3zu | %13s | %-20s | %-20s | %s\n",
				i,
				grammar.productions[i].name.c_str(),
				fmt::format("{}", grammar.productions[i].handle).c_str(),
				fmt::format("{{{}}}", metadata.first[NonTerminal{grammar.productions[i].name}]).c_str(),
				fmt::format("{{{}}}", metadata.follow[NonTerminal{grammar.productions[i].name}]).c_str()
		);
	printf("\n");

	// dump table-header
	printf("SYNTAX TABLE:\n");
	printf("%16s |", "NT \\ T");
	for (const Terminal& t : metadata.terminals)
		printf("%10s |", fmt::format("{}", t).c_str());
	printf("\n");
	printf("-----------------+");;
	for (size_t i = 0; i < metadata.terminals.size(); ++i)
		printf("-----------+");;
	printf("\n");

	// dump table-body
	for (const NonTerminal& nt : metadata.nonterminals)
	{
		printf("%16s |", nt.name.c_str());
		for (const Terminal& t : metadata.terminals)
			if (optional<int> p = st.lookup(idNonTerminals[nt], idTerminals[t]); p.has_value())
				printf("%10d |", *p);
			else
				printf("           |");
		printf("\n");
	}

	return move(st);
}

// vim:ts=4:sw=4:noet
