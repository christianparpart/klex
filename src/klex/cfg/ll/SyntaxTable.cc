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
#include <klex/util/iterator.h>

#include <algorithm>
#include <cstdarg>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::cfg::ll;

template <typename T, typename V = int, typename F = function<bool(const T&)>>
inline map<T, V> createIdMap(const vector<T>& items, V first, F select = [](const T&) { return true; })
{
	map<T, V> out;
	for (auto i = begin(items); i != end(items); ++i)
		if (select(*i))
			out[*i] = first + static_cast<V>(distance(begin(items), i));
	return out;
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

struct TerminalRuleBuilder {
	int nextTerminalId;

	regular::Rule operator()(const Terminal& w)
	{
		if (holds_alternative<regular::Rule>(w.literal))
		{
			regular::Rule literal = get<regular::Rule>(w.literal);
			if (literal.tag != regular::IgnoreTag)
				literal.tag = nextTerminalId++;
			return literal;
		}
		else
		{
			// const string rule = fmt::format("<INITIAL>{} ::= \"{}\"", w.name, get<string>(w.literal));
			// regular::RuleList rules = regular::RuleParser{rule, nextTerminalId}.parseRules();
			// TODO: don't do duplicates
			regular::RuleList rules {{
				0, 0,
				nextTerminalId,
				{"INITIAL"},
				w.name,
				fmt::format("\"{}\"", get<string>(w.literal)),
			}};
			nextTerminalId++;
			return rules.front();
		}
	}
};

SyntaxTable SyntaxTable::construct(const Grammar& grammar)
{
	const auto idNonTerminals = createIdMap(grammar.nonterminals, 0);

	const auto idTerminals =
		createIdMap(grammar.terminals, (int) grammar.nonterminals.size(), [](const Terminal& w) -> bool {
			return !holds_alternative<regular::Rule>(w.literal)
				   || get<regular::Rule>(w.literal).tag != regular::IgnoreTag;
		});

	const auto idActions =
		createIdMap(actions(grammar), static_cast<int>(idNonTerminals.size() + idTerminals.size()));

	const auto idProductionsByName = [&]() {
		map<NonTerminal, int> result;
		for (auto i = begin(grammar.productions); i != end(grammar.productions); ++i)
			result[NonTerminal{i->name}] = static_cast<int>(distance(begin(grammar.productions), i));
		return result;
	}();

	SyntaxTable st;

	st.names.resize(idNonTerminals.size() + idTerminals.size() + idActions.size());

	for (auto&& [nt, id] : idNonTerminals)
		st.names[id] = nt.name;

	for (auto&& [t, id] : idTerminals)
		st.names[id] = t.name;

	st.actionNames.resize(idActions.size());
	for (auto&& [act, id] : idActions)
	{
		st.names[id] = act.id;
		const size_t i = id - idNonTerminals.size() - idTerminals.size();
		st.actionNames[i] = act.id;
	}

	// terminals
	for (const Terminal& terminal : grammar.terminals)
		if (holds_alternative<string>(terminal.literal)
			|| get<regular::Rule>(terminal.literal).tag != regular::IgnoreTag)
			st.terminalNames.emplace_back(terminal.name);

	{
		// compile terminals
		regular::Compiler rgc;
		rgc.declareAll(terminalRules(grammar, static_cast<int>(grammar.nonterminals.size())));

		regular::Compiler::OvershadowMap overshadows;
		st.lexerDef = rgc.compileMulti(&overshadows);

		// TODO: care about `overshadows`
		// for (const auto& shadow: overshadows)
		// 	fmt::print("overshadow {} - {}\n", shadow.first, shadow.second);
		assert(overshadows.empty() && "Overshadowing lexical rules found.");
	}

	// syntax table
	st.nonterminalNames.resize(grammar.nonterminals.size());
	for (const NonTerminal& nt : grammar.nonterminals)
	{
		const int nt_ = idNonTerminals.at(nt);
		st.nonterminalNames[nt_] = nt.name;
		st.names[nt_] = nt.name;
		for (const Production* p : grammar.getProductions(nt))
		{
			for (const Terminal& w : p->first1())
			{
				assert(st.table[nt_].find(idTerminals.at(w)) == st.table[nt_].end());
				st.table[nt_][idTerminals.at(w)] = p->id;
			}

			// TODO if (p->first1().contains(eof))
			// 	st.table[nt_][eof_] = p_;
		}
	}

	// add productions to SyntaxTable
	for (const Production& p : grammar.productions)
	{
		SyntaxTable::Expression expr;
		expr.reserve(symbols(p.handle).size());

		for (const HandleElement b : p.handle)
			if (holds_alternative<NonTerminal>(b))
				expr.emplace_back(idNonTerminals.at(get<NonTerminal>(b)));
			else if (holds_alternative<Terminal>(b))
				expr.emplace_back(idTerminals.at(get<Terminal>(b)));
			else
				expr.emplace_back(idActions.at(get<Action>(b)));

		st.productionNames.emplace_back(p.name);
		st.productions.emplace_back(move(expr));
	}

	// TODO: action names

	st.startSymbol = idNonTerminals.at(NonTerminal{grammar.productions[0].name});

	return st;
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

	bprintf("PRODUCTIONS:\n");
	size_t p_i = 0;
	for (const auto& p : productions)
	{
		bprintf("%10s ::= ", productionNames[p_i].c_str());
		p_i++;
		for (const auto& b : p)
		{
			if (isNonTerminal(b))
				bprintf(" %s", nonterminalName(b).c_str());
			else if (isTerminal(b))
				bprintf(" %s", terminalName(b).c_str());
			else if (isAction(b))
				bprintf(" !%s", actionName(b).c_str());
			else
				bprintf(" %d", b);
		}
		bprintf("\n");
	}

	// table-header
	bprintf("%16s |", "NT \\ T");
#if 0
	for (const string& w : terminalNames)
		bprintf("%10s |", w.c_str());
#else
	for (const Terminal& t : grammar.terminals)
		if (holds_alternative<string>(t.literal))
			bprintf("%10s |", fmt::format("{}", get<string>(t.literal)).c_str());
		else
			bprintf("%10s |", fmt::format("{}", t).c_str());
#endif
	bprintf("\n");
	bprintf("-----------------+");
	for (size_t i = 0; i < terminalNames.size(); ++i)
		bprintf("-----------+");
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
