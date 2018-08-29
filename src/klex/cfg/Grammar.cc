// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/Grammar.h>
#include <klex/util/iterator.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <map>
#include <set>
#include <vector>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using klex::util::reversed;

// {{{ helper
namespace {

template <typename T>
inline vector<T> to_vector(set<T> S)
{
	vector<T> out;
	out.reserve(S.size());
	for (const T& v : S)
		out.emplace_back(v);
	return move(out);
}

template <typename T, typename V = int>
inline map<T, V> createIdMap(const vector<T>& items)
{
	map<T, V> out;
	for (auto i = begin(items); i != end(items); ++i)
		out[*i] = distance(begin(items), i);
	return move(out);
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

bool klex::cfg::operator<(const Symbol& a, const Symbol& b)
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

std::string klex::cfg::to_string(const Handle& handle)
{
	std::stringstream sstr;

	int i = 0;
	for (const klex::cfg::Symbol& symbol : handle.symbols)
	{
		if (i++)
			sstr << ' ';
		sstr << fmt::format("{}", symbol);
	}

	if (!handle.ref.empty())
		sstr << " {" << handle.ref << "}";

	return sstr.str();
}

vector<Terminal> Production::first1() const
{
	vector<Terminal> result = first;
	if (epsilon)
		set_union(begin(result), end(result), begin(follow), end(follow), back_inserter(result));

	return move(result);
}

vector<Production*> Grammar::getProductions(const NonTerminal& nt)
{
	vector<Production*> result;

	for (Production& production : productions)
		if (production.name == nt.name)
			result.push_back(&production);

	return move(result);
}

vector<const Production*> Grammar::getProductions(const NonTerminal& nt) const
{
	vector<const Production*> result;

	for (const Production& production : productions)
		if (production.name == nt.name)
			result.push_back(&production);

	return move(result);
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
	productions[0].handle.symbols.emplace_back(
		Terminal{regular::Rule{0, 0, /*TODO:tag*/ 0, {"INITIAL"}, "EOF", "<<EOF>>"}, "EOF"});
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

	nonterminals = [&]() {
		vector<NonTerminal> nonterminals;
		for (const Production& production : productions)
			if (find(begin(nonterminals), end(nonterminals), production.name) == end(nonterminals))
				nonterminals.emplace_back(NonTerminal{production.name});
		return move(nonterminals);
	}();

	terminals = [&]() {
		set<Terminal> terminals;
		for (const Production& production : productions)
			for (const Symbol& b : production.handle.symbols)
				if (holds_alternative<Terminal>(b))
					terminals.insert(get<Terminal>(b));

		vector<Terminal> terms = to_vector(move(terminals));

		for_each(begin(terms), end(terms), TerminalNameCurator{});

		// Add those explicit terminals that are flagged as "ignore".
		for (regular::Rule& rule : explicitTerminals)
			if (rule.isIgnored())
				terms.emplace_back(Terminal{rule, rule.name});

		return move(terms);
	}();

	while (true)
	{
		bool updated = false;

		for (Production& production : productions)
		{
			const NonTerminal nt{production.name};
			const vector<Symbol>& expression = production.handle.symbols;

			// FIRST-set
			bool found = false;
			for (const Symbol& b : expression)
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
			for (const Symbol& b : reversed(expression))
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

	sstr << " ID | NON-TERMINAL  | EXPRESSION           | FIRST                      | FOLLOW                "
			"     | FIRST+\n";
	sstr << "----+---------------+----------------------+----------------------------+-----------------------"
			"-----+-----------\n";
	for (auto p = begin(productions); p != end(productions); ++p)
	{
		char buf[255];
		snprintf(buf, sizeof(buf), "%3zu | %13s | %-20s | %6s%-20s | %-26s | %s",
				 distance(productions.begin(), p), p->name.c_str(), fmt::format("{}", p->handle).c_str(),
				 p->epsilon ? "{eps} " : "", fmt::format("{{{}}}", p->first).c_str(),
				 fmt::format("{{{}}}", p->follow).c_str(), fmt::format("{{{}}}", p->first1()).c_str());
		sstr << buf;
		if (next(p) != end(productions))
			sstr << '\n';
	}
	return move(sstr.str());
}

// vim:ts=4:sw=4:noet
