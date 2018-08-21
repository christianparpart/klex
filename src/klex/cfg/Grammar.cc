// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/Grammar.h>

#include <algorithm>
#include <map>
#include <set>
#include <vector>

using namespace std;
using namespace klex;
using namespace klex::cfg;

/// @returns true when @p target has changed after merge, false otherwise.
template<typename T>
inline bool merge(set<T>& target, std::set<T> source) {
	const size_t n = target.size();
	target.merge(std::move(source));
	return target.size() > n;
}

template<typename T>
inline set<T> copy(set<T> s) {
	return move(s);
}

template<typename Container>
struct _reversed {
	const Container& container;

	auto begin() { return container.crbegin(); }
	auto end() { return container.crend(); }
};

template<typename Container>
inline auto reversed(const Container& c) { return _reversed<Container>{c}; }

std::vector<const Production*> Grammar::getProductions(const NonTerminal& nt) const {
  std::vector<const Production*> result;

  for (const Production& production : productions)
    if (production.name == nt.name)
      result.push_back(&production);

  return std::move(result);
}

template<typename T>
vector<T> to_vector(set<T> S) {
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

GrammarMetadata Grammar::metadata() const {
	std::vector<NonTerminal> nonterminals = [&]() {
		set<NonTerminal> nonterminals;
		for (const Production& production : productions)
			nonterminals.insert(NonTerminal{production.name});

		return to_vector(move(nonterminals));
	}();

	vector<Terminal> terminals = [&]() {
		set<Terminal> terminals;
		for (const Production& production : productions)
			for (const Symbol& symbol : production.handle.symbols)
				if (holds_alternative<Terminal>(symbol))
					terminals.insert(get<Terminal>(symbol));

		return to_vector(move(terminals));
	}();

	map<Symbol, set<Terminal>> first;
	for_each(nonterminals.begin(), nonterminals.end(), [&](const NonTerminal& nt) { first[nt]; });
	for_each(terminals.begin(), terminals.end(), [&](const Terminal& t) { first[t] = {t}; });

	map<NonTerminal, set<Terminal>> follow;
	for_each(nonterminals.begin(), nonterminals.end(), [&](NonTerminal nt) { follow[nt]; });
	const NonTerminal startSymbol { productions[0].name };

	set<NonTerminal> epsilon; // contains set of non-terminals that contain epsilon symbols

	vector<set<Terminal>> FIRST;
	FIRST.resize(productions.size());

	while (true) {
		bool changed = false;
		for (size_t p = 0; p < productions.size(); ++p)
		{
			const Production& production = productions[p];
			const NonTerminal& nt { production.name };
			const vector<Symbol>& expression = production.handle.symbols;

			if (!expression.empty())
			{
				set<Terminal> rhs = FIRST[p];
				for (size_t i = 1; i < expression.size() - 1 && containsEpsilon(expression[i + 1]); ++i)
					changed |= merge(rhs, FIRST[i + 1]);
				// TODO if (i == k && 
				changed |= merge(FIRST[p], rhs);
			}
			else
				changed |= merge(epsilon, set<NonTerminal>{nt});
		}
		if (!changed)
			break;
	}

	while (true) {
		bool changed = false;

		for (const Production& production : productions) {
			NonTerminal nt { production.name };
			const vector<Symbol>& expression = production.handle.symbols;

			// FIRST-set
			if (expression.empty())
				changed |= merge(epsilon, set<NonTerminal>{nt});
			else
				for (const Symbol& symbol : expression) {
					changed |= merge(first[nt], first[symbol]);
					if (holds_alternative<NonTerminal>(symbol))
						if (epsilon.count(get<NonTerminal>(symbol)) == 0)
							break;
				}

			// FOLLOW-set
			set<Terminal> trailer = follow[nt];
			for (const Symbol& symbol : reversed(expression)) {
				if (holds_alternative<Terminal>(symbol))
					trailer = first[symbol];
				else {
					changed |= merge(follow[get<NonTerminal>(symbol)], trailer);

					if (epsilon.count(get<NonTerminal>(symbol)) == 0)
						trailer = first[symbol];
					else
						trailer.merge(copy(first[symbol]));
				}
			}
		}

		map<Symbol, set<Terminal>> first1;
		first1 = first;
		for (auto && [nt, follows] : follow)
			if (epsilon.find(nt) != epsilon.end())
				for (auto && f : follows)
					first1[nt].insert(f);

		if (!changed)
			return GrammarMetadata {
				move(nonterminals),
				move(terminals),
				move(first),
				move(follow),
				move(epsilon),
				move(first1),
				move(FIRST),
			};
	}
}

// vim:ts=4:sw=4:noet
