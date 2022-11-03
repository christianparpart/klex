// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/Grammar.h>
#include <klex/cfg/LeftRecursion.h>

#include <algorithm>

using namespace std;

namespace klex::cfg
{

LeftRecursion::LeftRecursion(Grammar& _grammar): grammar_ { _grammar }
{
}

bool LeftRecursion::isLeftRecursive(const Grammar& grammar)
{
    const vector<NonTerminal> nonterminals = cfg::nonterminals(grammar);

    return any_of(begin(nonterminals), end(nonterminals), [&](const NonTerminal& nt) {
        const vector<const Production*> productions = grammar.getProductions(nt);

        return any_of(begin(productions), end(productions), [](const Production* p) {
            auto syms = symbols(p->handle);

            return !syms.empty() && holds_alternative<NonTerminal>(syms[0])
                   && get<NonTerminal>(syms[0]) == p->name && syms.size() > 1;
        });
    });
}

void LeftRecursion::direct()
{
    for (const NonTerminal& nt: cfg::nonterminals(grammar_))
        eliminateDirect(nt);
}

void LeftRecursion::indirect()
{
    const vector<NonTerminal> nonterminals = cfg::nonterminals(grammar_);

    for (size_t i = 0; i < nonterminals.size(); ++i)
    {
        for (size_t k = 0; k < i; ++k)
        {
            for (Production* p: select(nonterminals[i], nonterminals[k]))
            {
                (void) p; // TODO
                for (Production* q: grammar_.getProductions(nonterminals[k]))
                {
                    (void) q; // TODO
                    // replace first non-terminal
                    ; // p->replaceSymbolAt(0, NonTerminal{q->name});
                }
            }
        }

        eliminateDirect(nonterminals[i]);
    }
}

list<Production*> LeftRecursion::select(const NonTerminal& lhs, const NonTerminal& first)
{
    list<Production*> out;

    for (Production* p: grammar_.getProductions(lhs))
        if (const optional<NonTerminal> nt = firstNonTerminal(p->handle); nt.has_value() && *nt == first)
            out.emplace_back(p);

    return out;
}

void LeftRecursion::eliminateDirect(const NonTerminal& nt)
{
    if (auto [head, tail] = split(grammar_.getProductions(nt)); !tail.empty())
    {
        const NonTerminal tailSymbol = createRelatedNonTerminal(nt);
        for (Production* p: head) // b -> b A'
            p->handle.emplace_back(tailSymbol);

        for (Production* p: tail)
        {
            p->name = tailSymbol.name;
            p->handle.emplace_back(tailSymbol);
            p->handle.erase(p->handle.begin());
        }

        // inject new epsilon-production.
        grammar_.productions.emplace_back(Production { tailSymbol.name, {} });
        // TODO: don't emplace at the back of all but at the back of the last NT's tail symbol.
        // TODO: fix injected EOF rule, omfg
    }
}

NonTerminal LeftRecursion::createRelatedNonTerminal(const NonTerminal& nt) const
{
    string tail = nt.name + "_";

    while (any_of(begin(grammar_.productions), end(grammar_.productions), [&](const Production& p) {
        return p.name == tail;
    }))
        tail += "_";

    return NonTerminal { tail };
}

pair<vector<Production*>, vector<Production*>> LeftRecursion::split(vector<Production*> productions) const
{
    vector<Production*> head;
    vector<Production*> tail;

    for (Production* p: productions)
    {
        const optional<NonTerminal> nt = firstNonTerminal(p->handle);
        if (nt.has_value() && *nt == p->name && symbols(p->handle).size() > 1)
            tail.emplace_back(p);
        else
            head.emplace_back(p);
    }

    return make_pair(std::move(head), std::move(tail));
}

} // namespace klex::cfg
