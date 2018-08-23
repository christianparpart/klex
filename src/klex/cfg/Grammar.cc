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

namespace { // {{{ helper

template<typename T>
inline vector<T> to_vector(set<T> S) {
	vector<T> out;
	out.reserve(S.size());
	for (const T& v : S)
		out.emplace_back(v);
	return move(out);
}

template<typename T, typename V = int>
inline map<T, V> createIdMap(const vector<T>& items)
{
	map<T, V> out;
	for (auto i = items.begin(); i != items.end(); ++i)
		out[*i] = distance(items.begin(), i);
	return move(out);
}

template<typename T>
inline bool merge(vector<T>* target, vector<T> source) {
	sort(target->begin(), target->end());
	sort(source.begin(), source.end());

	const size_t n = target->size();
	vector<T> dest;
	set_union(target->begin(), target->end(),
			  source.begin(), source.end(),
			  back_inserter(dest));
	*target = move(dest);
	return target->size() > n;
}

} // }}} helper

vector<Terminal> Production::first1() const
{
	vector<Terminal> result = first;
	if (epsilon)
		set_union(result.begin(), result.end(),
				  follow.begin(), follow.end(),
				  back_inserter(result));

	return move(result);
}

std::vector<Production*> Grammar::getProductions(const NonTerminal& nt) {
	std::vector<Production*> result;

	for (Production& production : productions)
		if (production.name == nt.name)
			result.push_back(&production);

	return std::move(result);
}

std::vector<const Production*> Grammar::getProductions(const NonTerminal& nt) const {
	std::vector<const Production*> result;

	for (const Production& production : productions)
		if (production.name == nt.name)
			result.push_back(&production);

	return std::move(result);
}

std::vector<Terminal> Grammar::firstOf(const Symbol& b) const {
	if (holds_alternative<Terminal>(b))
		return vector<Terminal>{ get<Terminal>(b) };

	std::set<Terminal> first;
	for (const Production* p : getProductions(get<NonTerminal>(b)))
		for (const Terminal& w : p->first)
			first.emplace(w);

	return to_vector(move(first));
}

std::vector<Terminal> Grammar::followOf(const NonTerminal& nt) const {
	std::set<Terminal> follow;

	for (const Production* p : getProductions(nt))
		for (const Terminal& w : p->follow)
			follow.emplace(w);

	return to_vector(move(follow));
}

void Grammar::clearMetadata()
{
	int nextId = 0;
	for (Production& p : productions)
	{
		p.id = nextId++;
		p.epsilon = false;
		p.first.clear();
		p.follow.clear();
	}
	nonterminals.clear();
	terminals.clear();
}

void Grammar::finalize()
{
	clearMetadata();

	nonterminals = [&]() {
		vector<NonTerminal> nonterminals;
		for (const Production& production : productions)
			if (find(nonterminals.begin(), nonterminals.end(), production.name) == nonterminals.end())
				nonterminals.emplace_back(NonTerminal{production.name});
		return move(nonterminals);
	}();

	terminals = [&]() {
		set<Terminal> terminals;
		for (const Production& production : productions)
			for (const Symbol& b : production.handle.symbols)
				if (holds_alternative<Terminal>(b))
					terminals.insert(get<Terminal>(b));
		return to_vector(move(terminals));
	}();

	while (true)
	{
		bool updated = false;

		for (Production& production : productions)
		{
			NonTerminal nt { production.name };
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


	sstr << " ID | NON-TERMINAL  | EXPRESSION           | FIRST                      | FOLLOW                     | FIRST+\n";
	sstr << "----+---------------+----------------------+----------------------------+----------------------------+-----------\n";
	for (auto p = productions.begin(); p != productions.end(); ++p)
	{
		char buf[255];
		snprintf(buf, sizeof(buf),
				"%3zu | %13s | %-20s | %6s%-20s | %-26s | %s",
				distance(productions.begin(), p),
				p->name.c_str(),
				fmt::format("{}", p->handle).c_str(),
				p->epsilon ? "{eps} " : "",
				fmt::format("{{{}}}", p->first).c_str(),
				fmt::format("{{{}}}", p->follow).c_str(),
				fmt::format("{{{}}}", p->first1()).c_str()
		);
		sstr << (char*) buf;
		if (next(p) != productions.end())
			sstr << '\n';
	}
	return move(sstr.str());
}

// vim:ts=4:sw=4:noet
