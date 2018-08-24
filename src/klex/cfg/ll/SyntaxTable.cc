// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/Grammar.h>
#include <klex/cfg/ll/SyntaxTable.h>
#include <klex/regular/Compiler.h>
#include <klex/regular/LexerDef.h>
#include <klex/regular/Rule.h>
#include <klex/regular/RuleParser.h>

#include <algorithm>
#include <cstdarg>
#include <iomanip>
#include <iostream>
#include <map>
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

std::optional<int> SyntaxTable::lookup(int nonterminal, int lookahead) const {
	auto i = table.find(nonterminal);
	if (i == table.end())
		return std::nullopt;

	auto k = i->second.find(lookahead);
	if (k == i->second.end())
		return std::nullopt;

	return k->second;
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

	// terminals
	{
		regular::RuleList terminalRules;

		for (const regular::Rule& rule : regular::RuleParser{"Eof ::= <<EOF>>"}.parseRules())
			terminalRules.emplace_back(rule);

		for (const Terminal& w : grammar.terminals)
			if (holds_alternative<regular::Rule>(w.literal))
				terminalRules.emplace_back(get<regular::Rule>(w.literal));
			else
				for (const regular::Rule& rule : regular::RuleParser{fmt::format("{} ::= \"{}\"", w.name, get<string>(w.literal))}.parseRules())
					terminalRules.emplace_back(rule);

		// TODO: custom tokens: `token { }`

		regular::Compiler rgc;
		rgc.declareAll(move(terminalRules));

		regular::Compiler::OvershadowMap overshadows;
		regular::LexerDef lexerDef = rgc.compileMulti(&overshadows);

		// TODO: move lexerDef into SyntaxTable
		// st.lexer = move(lexerDef);
	}

	return move(st);
}

string SyntaxTable::dump(const Grammar& grammar) const
{
	map<NonTerminal, int> idNonTerminals = createIdMap(grammar.nonterminals);
	map<Terminal, int> idTerminals = createIdMap(grammar.terminals);

	stringstream os;

	auto bprintf = [&](const char* fmt, ...) {
		va_list va;
		char buf[256];
		va_start(va, fmt);
		vsnprintf(buf, sizeof(buf), fmt, va);
		va_end(va);
		os << (char*) buf;
	};
	// table-header
	bprintf("%16s |", "NT \\ T");
	for (const Terminal& t : grammar.terminals)
		bprintf("%10s |", fmt::format("{}", t).c_str());
	bprintf("\n");
	bprintf("-----------------+");;
	for (size_t i = 0; i < grammar.terminals.size(); ++i)
		bprintf("-----------+");;
	bprintf("\n");

	// table-body
	set<NonTerminal> check;
	for (const Production& production : grammar.productions)
	{
		const NonTerminal nt { production.name };
		if (check.count(nt)) continue;
		check.insert(nt);
		bprintf("%16s |", nt.name.c_str());
		for (const Terminal& t : grammar.terminals)
			if (optional<int> p = lookup(idNonTerminals[nt], idTerminals[t]); p.has_value())
				bprintf("%10d |", *p);
			else
				bprintf("           |");
		bprintf("\n");
	}

	return os.str();
}

// vim:ts=4:sw=4:noet
