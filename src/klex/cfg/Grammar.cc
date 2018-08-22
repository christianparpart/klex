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
inline bool merge(set<T>* target, std::set<T> source) {
	const size_t n = target->size();
	target->merge(std::move(source));
	return target->size() > n;
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
inline vector<T> to_vector(set<T> S) {
	vector<T> out;
	out.reserve(S.size());
	for (const T& v : S)
		out.emplace_back(v);
	return move(out);
}

template<typename T, typename U>
inline U unwrapSet(set<T>&& values) {
	set<U> out;
	for (const auto& v : values)
		out.insert(get<U>(v));
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
			for (const Symbol& b : production.handle.symbols)
				if (holds_alternative<Terminal>(b))
					terminals.insert(get<Terminal>(b));

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

	while (true)
	{
		bool updated = false;

		for (const Production& production : productions)
		{
			NonTerminal nt { production.name };
			const vector<Symbol>& expression = production.handle.symbols;

			// FIRST-set
			bool found = false;
			for (const Symbol& b : expression)
			{
				updated |= merge(&first[nt], first[b]);
				if (holds_alternative<Terminal>(b) || !epsilon.count(get<NonTerminal>(b)))
				{
					found = true;
					break;
				}
			}
			if (!found)
				updated |= merge(&epsilon, set<NonTerminal>{nt});

			// FOLLOW-set
			set<Terminal> trailer = follow[nt];
			for (const Symbol& b : reversed(expression)) {
				if (holds_alternative<Terminal>(b))
					trailer = first[b];
				else {
					updated |= merge(&follow[get<NonTerminal>(b)], trailer);

					if (epsilon.count(get<NonTerminal>(b)) == 0)
						trailer = first[b];
					else
						trailer.merge(copy(first[b]));
				}
			}
		}

		if (!updated)
			break;
	}

	vector<set<Terminal>> first1;
	first1.reserve(productions.size());
	for (const Production& production : productions)
	{
		NonTerminal nt { production.name };
		const vector<Symbol>& expression = production.handle.symbols;
		set<Terminal> S;
		for (const Terminal& t : first[Symbol{nt}])
			S.insert(t);
		if (epsilon.count(nt))
			for (const Terminal& t : follow[nt])
				S.insert(t);
		first1.emplace_back(move(S));
	}

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

// vim:ts=4:sw=4:noet
