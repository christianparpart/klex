// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/Grammar.h>
#include <klex/cfg/LeftRecursion.h>
#include <klex/util/iterator.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <map>
#include <set>
#include <vector>

using namespace std;
using klex::util::indexed;
using klex::util::reversed;
using klex::util::find_last;

namespace klex::cfg {

// {{{ helper
namespace {

template <typename T>
inline vector<T> to_vector(set<T> S)
{
	vector<T> out;
	out.reserve(S.size());
	for (const T& v : S)
		out.emplace_back(v);
	return out;
}

template <typename T, typename V = int>
inline map<T, V> createIdMap(const vector<T>& items)
{
	map<T, V> out;
	for (auto i = begin(items); i != end(items); ++i)
		out[*i] = distance(begin(items), i);
	return out;
}

template <typename T>
inline bool merge(vector<T>* target, vector<T> source)
{
	sort(begin(*target), end(*target));
	sort(begin(source), end(source));

	const size_t n = target->size();
	vector<T> dest;
	set_union(begin(*target), end(*target), begin(source), end(source), back_inserter(dest));
	*target = move(dest);
	return target->size() > n;
}

}  // namespace
// }}} helper

bool operator<(const Symbol& a, const Symbol& b)
{
	using namespace std;
	const string& lhs = holds_alternative<Terminal>(a)
							? holds_alternative<regular::Rule>(get<Terminal>(a).literal)
								  ? get<regular::Rule>(get<Terminal>(a).literal).pattern
								  : get<string>(get<Terminal>(a).literal)
							: get<NonTerminal>(a).name;
	const string& rhs = holds_alternative<Terminal>(b)
							? holds_alternative<regular::Rule>(get<Terminal>(b).literal)
								  ? get<regular::Rule>(get<Terminal>(b).literal).pattern
								  : get<string>(get<Terminal>(b).literal)
							: get<NonTerminal>(b).name;
	return lhs < rhs;
}

string to_string(const Handle& handle)
{
	stringstream sstr;

	for (const auto&& [i, x] : indexed(handle))
	{
		if (i)
			sstr << ' ';
		sstr << fmt::format("{}", x);
	}

	return sstr.str();
}

optional<NonTerminal> firstNonTerminal(const Handle& handle)
{
	for (const Symbol b : symbols(handle))
		if (holds_alternative<NonTerminal>(b))
			return get<NonTerminal>(b);

	return nullopt;
}

string to_string(const Production& p)
{
	return fmt::format("{} ::= {};", p.name, p.handle);
}

vector<Terminal> Production::first1() const
{
	vector<Terminal> result = first;
	if (epsilon)
		set_union(begin(result), end(result), begin(follow), end(follow), back_inserter(result));

	return result;
}

vector<Production*> Grammar::getProductions(const NonTerminal& nt)
{
	vector<Production*> result;

	for (Production& production : productions)
		if (production.name == nt.name)
			result.push_back(&production);

	return result;
}

vector<const Production*> Grammar::getProductions(const NonTerminal& nt) const
{
	vector<const Production*> result;

	for (const Production& production : productions)
		if (production.name == nt.name)
			result.push_back(&production);

	return result;
}

vector<Terminal> Grammar::firstOf(const Symbol& b) const
{
	if (holds_alternative<Terminal>(b))
		return vector<Terminal>{get<Terminal>(b)};

	set<Terminal> first;
	for (const Production* p : getProductions(get<NonTerminal>(b)))
		for (const Terminal& w : p->first)
			first.emplace(w);

	return to_vector(move(first));
}

vector<Terminal> Grammar::followOf(const NonTerminal& nt) const
{
	set<Terminal> follow;

	for (const Production* p : getProductions(nt))
		for (const Terminal& w : p->follow)
			follow.emplace(w);

	return to_vector(move(follow));
}

void Grammar::injectEof()
{
	Production& start = productions[0];
	Handle& handle = start.handle;

	auto i = find_last(handle, [](const HandleElement& e) {
		return holds_alternative<Terminal>(e) || holds_alternative<NonTerminal>(e);
	});

	if (i != end(handle))
		++i;

	handle.insert(i, Terminal{regular::Rule(0, 0, /*TODO:tag*/ 0, {"INITIAL"}, "EOF", "<<EOF>>"), "EOF"});
}

struct ProductionIdBuilder {
	int nextId = 0;
	void operator()(Production& p) { p.id = nextId++; }
};

struct TerminalNameCurator {
	int nextId = 0;

	void operator()(Terminal& w)
	{
		static map<string, string> wellKnown = {
			{"+", "PLUS"},        {"-", "MINUS"},
			{"*", "MUL"},         {"/", "DIV"},
			{"(", "RND_OPEN"},    {")", "RND_CLOSE"},
			{"[", "BR_OPEN"},     {"]", "BR_CLOSE"},
			{"{", "CR_OPEN"},     {"}", "CR_CLOSE"},
			{"<", "LESS"},        {">", "GREATER"},
			{"<=", "LESS_EQUAL"}, {">=", "GREATER_EQUAL"},
			{"==", "EQUAL"},      {"!=", "NOT_EQUAL"},
			{"=", "EQ"},          {"!", "NOT"},
		};

		if (holds_alternative<string>(w.literal))
		{
			if (auto i = wellKnown.find(get<string>(w.literal)); i != wellKnown.end())
				w.name = i->second;
			else
				w.name = fmt::format("T_{}", nextId++);
		}
	}
};

void Grammar::finalize()
{
	assert(nonterminals.empty());
	assert(terminals.empty());

	injectEof();

	for_each(begin(productions), end(productions), ProductionIdBuilder{});

	terminals = cfg::terminals(*this);
	nonterminals = cfg::nonterminals(*this);

	while (true)
	{
		bool updated = false;

		for (Production& production : productions)
		{
			const NonTerminal nt{production.name};
			const Handle& expression = production.handle;

			// FIRST-set
			bool found = false;
			for (const Symbol b : symbols(expression))
			{
				updated |= merge(&production.first, firstOf(b));
				if (!containsEpsilon(b))
				{
					found = true;
					break;
				}
			}
			if (!found)
				production.epsilon = true;

			// FOLLOW-set
			vector<Terminal> trailer = followOf(nt);
			for (const Symbol b : reversed(symbols(expression)))
			{
				if (holds_alternative<Terminal>(b))
					trailer = firstOf(b);
				else
				{
					for (Production* p : getProductions(get<NonTerminal>(b)))
						updated |= merge(&p->follow, trailer);

					if (!containsEpsilon(b))
						trailer = firstOf(b);
					else
						merge(&trailer, firstOf(b));
				}
			}
		}

		if (!updated)
			break;
	}
}

string Grammar::dump() const
{
	stringstream sstr;

	sstr << fmt::format(" {:<2} | {:<13} | {:<22} | {:<26} | {:<26} | {:}\n",
			"ID", "NON-TERMINAL", "EXPRESSION", "FIRST", "FOLLOW", "FIRST+");
	sstr << fmt::format("-{:-<2}-+-{:-<13}-+-{:-<22}-+-{:-<26}-+-{:-<26}-+-{:-<10}\n",
			"-", "-", "-", "-", "-", "-");

	for (auto p = begin(productions); p != end(productions); ++p)
		sstr << fmt::format(" {:>2} | {:<13} | {:<22} | {:<6}{:<20} | {:<26} | {:}\n",
							distance(productions.begin(), p),
							p->name,
							fmt::format("{}", p->handle),
							p->epsilon ? "{eps} " : "",
							fmt::format("{}", p->first),
							fmt::format("{}", p->follow),
							fmt::format("{}", p->first1()));

	return sstr.str();
}

vector<Terminal> terminals(const Grammar& grammar)
{
	set<Terminal> terminals;
	for (const Production& production : grammar.productions)
		for (const Symbol b : symbols(production.handle))
			if (holds_alternative<Terminal>(b))
				terminals.insert(get<Terminal>(b));

	vector<Terminal> terms = to_vector(move(terminals));

	for_each(begin(terms), end(terms), TerminalNameCurator{});

	// Add those explicit terminals that are flagged as "ignore".
	for (const regular::Rule& rule : grammar.explicitTerminals)
		if (rule.isIgnored())
			terms.emplace_back(Terminal{rule, rule.name});

	return terms;
}

vector<NonTerminal> nonterminals(const Grammar& grammar)
{
	vector<NonTerminal> nts;

	for (const Production& production : grammar.productions)
		if (find(begin(nts), end(nts), production.name) == end(nts))
			nts.emplace_back(NonTerminal{production.name});

	return nts;
}

vector<Action> actions(const Grammar& grammar)
{
	set<Action> acts;

	for (const Production& production : grammar.productions)
		for (const HandleElement& e : production.handle)
			if (holds_alternative<Action>(e))
				acts.insert(get<Action>(e));

	return to_vector(move(acts));
}

bool isLeftRecursive(const Grammar& grammar)
{
	return LeftRecursion::isLeftRecursive(grammar);
}

regular::RuleList terminalRules(const Grammar& grammar, int nextTerminalId)
{
	regular::RuleList rules;
	set<string> autoLiterals;
	for (const Terminal& w: grammar.terminals)
	{
		if (holds_alternative<regular::Rule>(w.literal))
		{
			regular::Rule literal = get<regular::Rule>(w.literal);
			if (literal.tag != regular::IgnoreTag)
				literal.tag = nextTerminalId++;
			rules.emplace_back(move(literal));
		}
		else if (!autoLiterals.count(get<string>(w.literal)))
		{
			autoLiterals.emplace(get<string>(w.literal));
			const string pattern = fmt::format("\"{}\"", get<string>(w.literal));
			rules.emplace_back(regular::Rule{0, 0, nextTerminalId, {"INITIAL"}, w.name, pattern});
			nextTerminalId++;
		}
	}
	return rules;
}

}  // namespace klex::cfg

// vim:ts=4:sw=4:noet
