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

template <typename T, typename V = int>
inline map<T, V> createIdMap(const vector<T>& items, V first)
{
	map<T, V> out;
	for (auto i = begin(items); i != end(items); ++i)
		out[*i] = first + distance(begin(items), i);
	return move(out);
}

optional<int> SyntaxTable::lookup(int nonterminal, int lookahead) const
{
	auto i = table.find(nonterminal);
	if (i == table.end())
		return nullopt;

	auto k = i->second.find(lookahead);
	if (k == i->second.end())
		return nullopt;

	return k->second;
}

SyntaxTable SyntaxTable::construct(const Grammar& grammar)
{
	map<NonTerminal, int> idNonTerminals = createIdMap(grammar.nonterminals, 0);
	map<Terminal, int> idTerminals = createIdMap(grammar.terminals, (int) grammar.nonterminals.size());
	map<NonTerminal, int> idProductionsByName;
	for (auto i = begin(grammar.productions); i != end(grammar.productions); ++i)
		idProductionsByName[NonTerminal{i->name}] = distance(begin(grammar.productions), i);

	SyntaxTable st;
	st.nonterminalNames.resize(grammar.nonterminals.size());

	for (const NonTerminal& nt : grammar.nonterminals)
	{
		const int nt_ = idNonTerminals[nt];
		st.nonterminalNames[nt_] = nt.name;
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
		size_t nextTerminalId = grammar.nonterminals.size() + 1;

		static const string eofRule = "Eof ::= <<EOF>>";
		for (const regular::Rule& rule : regular::RuleParser{eofRule, nextTerminalId++}.parseRules())
			terminalRules.emplace_back(rule);

		for (const Terminal& w : grammar.terminals)
			if (holds_alternative<regular::Rule>(w.literal))
				terminalRules.emplace_back(get<regular::Rule>(w.literal));
			else
			{
				const string rule = fmt::format("{} ::= \"{}\"", w.name, get<string>(w.literal));
				for (const regular::Rule& rule : regular::RuleParser{rule, nextTerminalId++}.parseRules())
					terminalRules.emplace_back(rule);
			}

		// compile terminals
		regular::Compiler rgc;
		rgc.declareAll(move(terminalRules));

		regular::Compiler::OvershadowMap overshadows;
		regular::LexerDef lexerDef = rgc.compileMulti(&overshadows);

		st.lexerDef = move(lexerDef);
	}

	// add productions to SyntaxTable
	for (const Production& p : grammar.productions)
	{
		SyntaxTable::Expression expr;
		expr.reserve(p.handle.symbols.size());

		for (const Symbol& b : p.handle.symbols)
			if (holds_alternative<NonTerminal>(b))
				expr.emplace_back(idNonTerminals[get<NonTerminal>(b)]);
			else
				expr.emplace_back(idTerminals[get<Terminal>(b)]);

		st.productions.emplace_back(move(expr));
	}

	return move(st);
}

string SyntaxTable::dump(const Grammar& grammar) const
{
	map<NonTerminal, int> idNonTerminals = createIdMap(grammar.nonterminals, 0);
	map<Terminal, int> idTerminals = createIdMap(grammar.terminals, (int) grammar.nonterminals.size());

	stringstream os;

	auto bprintf = [&](const char* fmt, ...) {
		va_list va;
		char buf[256];
		va_start(va, fmt);
		vsnprintf(buf, sizeof(buf), fmt, va);
		va_end(va);
		os << (char*) buf;
	};

#if 1
	bprintf("PRODUCTIONS:\n");
	for (const auto& p : productions)
	{
		bprintf("%10s ::= ", "<?>");
		for (const auto& b : p)
		{
			if (isNonTerminal(b))
				bprintf(" <%s>", nonterminalNames[b].c_str());
			else if (isTerminal(b))
				bprintf(" %s", lexerDef.tagName(b).c_str());
			else
				bprintf(" %d", b);
		}
		bprintf("\n");
	}
#endif

	// table-header
	bprintf("%16s |", "NT \\ T");
	for (const Terminal& t : grammar.terminals)
		if (holds_alternative<string>(t.literal))
			bprintf("%10s |", fmt::format("{}", get<string>(t.literal)).c_str());
		else
			bprintf("%10s |", fmt::format("{}", t).c_str());
	bprintf("\n");
	bprintf("-----------------+");
	;
	for (size_t i = 0; i < grammar.terminals.size(); ++i)
		bprintf("-----------+");
	;
	bprintf("\n");

	// table-body
	set<NonTerminal> check;
	for (const Production& production : grammar.productions)
	{
		const NonTerminal nt{production.name};
		if (check.count(nt))
			continue;
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
