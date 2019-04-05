// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/regular/Rule.h>

#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace klex::cfg {

/**
 * Terminal represents a terminal symbol within a grammar's production rule.
 *
 * @see NonTerminal, Production, Grammar.
 */
struct Terminal {
	std::variant<regular::Rule, std::string> literal;  // such as [0-9]+ or "if"

	std::string name;  // such as "KW_IF"

	const std::string& pattern() const
	{
		if (std::holds_alternative<std::string>(literal))
			return std::get<std::string>(literal);
		else
			return std::get<regular::Rule>(literal).pattern;
	}
};

/**
 * NonTerminal represents a non-terminal symbol within a grammar's production rule.
 *
 * @see Terminal, Production, Grammar.
 */
struct NonTerminal {
	std::string name;  // such as "IfStmt"

	bool operator==(const std::string& other) const { return name == other; }
	bool operator==(const NonTerminal& other) const { return name == other.name; }
};

/**
 * Symbol is a terminal or non-terminal within a grammar rule (Production).
 *
 * @see Terminal, NonTerminal, Production.
 */
using Symbol = std::variant<NonTerminal, Terminal>;

//! @returns true if symbol @p a is by string-comparison smaller then symbol @p b.
bool operator<(const Symbol& a, const Symbol& b);

struct Action {
	std::string id;

	bool operator==(const Action& other) const noexcept { return id == other.id; }
	bool operator!=(const Action& other) const noexcept { return id != other.id; }
	bool operator<(const Action& other) const noexcept { return id < other.id; }
};

using HandleElement = std::variant<Terminal, NonTerminal, Action>;

/**
 * Handle is the right-hand-side of a Production rule.
 *
 * @see Production, Terminal, NonTerminal
 */
using Handle = std::vector<HandleElement>;

struct _Symbols {
	const Handle& handle;
	struct const_iterator {
		Handle::const_iterator i;
		Handle::const_iterator e;

		const_iterator(Handle::const_iterator _i, Handle::const_iterator _e) : i{_i}, e{_e}
		{
			while (i != e && std::holds_alternative<Action>(*i))
				++i;
		}

		Symbol operator*() const
		{
			if (std::holds_alternative<Terminal>(*i))
				return std::get<Terminal>(*i);
			else if (std::holds_alternative<NonTerminal>(*i))
				return std::get<NonTerminal>(*i);
			else
				return Symbol();
		}

		const_iterator& operator++(int)
		{
			++*this;
			return *this;
		}

		const_iterator& operator++()
		{
			i++;
			while (i != e && std::holds_alternative<Action>(*i))
				i++;
			return *this;
		}

		bool operator==(const const_iterator& rhs) const noexcept { return i == rhs.i; }
		bool operator!=(const const_iterator& rhs) const noexcept { return i != rhs.i; }
	};

	bool empty() const noexcept;
	size_t size() const noexcept;

	Symbol operator[](size_t i) const
	{
		for (const auto& x : handle)
			if (std::holds_alternative<Terminal>(x) || std::holds_alternative<NonTerminal>(x))
			{
				if (i > 0)
					i--;
				else if (std::holds_alternative<Terminal>(x))
					return std::get<Terminal>(x);
				else
					return std::get<NonTerminal>(x);
			}

		throw std::runtime_error{"Range error"};
	}

	const_iterator begin() const { return const_iterator{handle.cbegin(), handle.cend()}; }
	const_iterator end() const { return const_iterator{handle.cend(), handle.cend()}; }

	struct const_reverse_iterator {
		Handle::const_reverse_iterator i;
		Handle::const_reverse_iterator e;

		const_reverse_iterator(Handle::const_reverse_iterator _i, Handle::const_reverse_iterator _e) : i{_i}, e{_e}
		{
			while (i != e && std::holds_alternative<Action>(*i))
				++i;
		}

		Symbol operator*() const
		{
			if (std::holds_alternative<Terminal>(*i))
				return std::get<Terminal>(*i);
			else
				return std::get<NonTerminal>(*i);
		}

		const_reverse_iterator& operator++()
		{
			i++;
			while (i != e && std::holds_alternative<Action>(*i))
				i++;
			return *this;
		}
		bool operator==(const const_reverse_iterator& rhs) const noexcept { return i == rhs.i; }
		bool operator!=(const const_reverse_iterator& rhs) const noexcept { return i != rhs.i; }
	};

	const_reverse_iterator crbegin() const
	{
		return const_reverse_iterator{handle.crbegin(), handle.crend()};
	}

	const_reverse_iterator crend() const { return const_reverse_iterator{handle.crend(), handle.crend()}; }
};

std::optional<NonTerminal> firstNonTerminal(const Handle& h);

inline _Symbols symbols(const Handle& handle)
{
	return _Symbols{handle};
}

//! @returns a human readable form of @p handle.
std::string to_string(const Handle& handle);

/**
 * Production declares a production rule related to a Grammar.
 *
 * @see Grammar, Terminal, NonTerminal
 */
struct Production {
	std::string name;  //!< Represents the productions non-terminal symbol name.
	Handle handle;     //!< Represents the productions right-hand-side's expression, also known as handle.

	int id = -1;                        //!< Unique ID identifying this production.
	bool epsilon = false;               //!< Indicates whether or not this rule contains an epsilon.
	std::vector<Terminal> first = {};   //!< Accumulated set of terminals representing the FIRST-set.
	std::vector<Terminal> follow = {};  //!< Accumulated set of terminals representing the FOLLOW-set.

	std::vector<Terminal> first1() const;  //!< @returns the FIRST+-set of this production's handle.
};

//! @returns a human readable form of Production @p p.
std::string to_string(const Production& p);

/**
 * Context-free grammar.
 *
 * @see Production
 */
struct Grammar {
	//! List of terminals with explicit definitions.
	std::vector<regular::Rule> explicitTerminals;

	//! List of grammar productions rules.
	std::vector<Production> productions;

	//! Accumulated list of non-terminals, filled by finalize().
	std::vector<NonTerminal> nonterminals;

	//! Accumulated list of terminals (including explicitely specified terminals), filled by finalize().
	std::vector<Terminal> terminals;

	//! @returns a set of Production alternating rules that represent given NonTerminal @p nt.
	std::vector<const Production*> getProductions(const NonTerminal& nt) const;

	//! @returns a set of Production alternating rules that represent given NonTerminal @p nt.
	std::vector<Production*> getProductions(const NonTerminal& nt);

	//! @returns boolean, indicating whether or not given symbol contains an epsilon.
	bool containsEpsilon(const Symbol& s) const
	{
		return std::holds_alternative<NonTerminal>(s) && containsEpsilon(std::get<NonTerminal>(s));
	}

	//! @returns boolean, indicating whether or not given symbol contains an epsilon.
	bool containsEpsilon(const NonTerminal& nt) const
	{
		for (const Production* p : getProductions(nt))
			if (p->epsilon)
				return true;

		return false;
	}

	//! @returns true if given non-terminal corresponds to a production, false otherwise.
	bool containsProduction(const NonTerminal& nt) const
	{
		return std::any_of(productions.begin(), productions.end(),
						   [&](const Production& p) { return p.name == nt.name; });
	}

	//! @returns boolean, indicating whether or not a terminal by given symbolic name has been declared.
	bool containsExplicitTerminalWithName(const std::string& terminalName) const
	{
		using namespace std;
		return any_of(begin(explicitTerminals), end(explicitTerminals),
					  [&](const regular::Rule& rule) -> bool { return rule.name == terminalName; });
	}

	//! @returns a set of terminals representing the FIRST-set of Symbol @p b.
	std::vector<Terminal> firstOf(const Symbol& b) const;

	//! @returns a set of terminals representing the FOLLOW-set of NonTerminal @p nt.
	std::vector<Terminal> followOf(const NonTerminal& nt) const;

	//! Injects the EOF-terminal at the end of the start production.
	void injectEof();

	//! Finalizes this grammar, i.e. filling out any computed metadata about this grammar (such as
	//! FIRST/FOLLOW sets).
	void finalize();

	//! @returns the state of this Grammar in a print-compatbile form.
	std::string dump() const;
};

std::vector<Terminal> terminals(const Grammar& grammar);
std::vector<NonTerminal> nonterminals(const Grammar& grammar);
std::vector<Action> actions(const Grammar& grammar);

bool isLeftRecursive(const Grammar& grammar);

}  // namespace klex::cfg

namespace std {

template <>
struct iterator_traits<klex::cfg::_Symbols::const_iterator> {
	using difference_type = std::ptrdiff_t;
	using value_type = klex::cfg::Symbol;
	using pointer = value_type*;
	using reference = value_type&;
	using iterator_category = std::forward_iterator_tag;
};

}  // namespace std

// {{{ fmtlib integration
namespace fmt {
template <>
struct formatter<klex::cfg::Terminal> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::cfg::Terminal& v, FormatContext& ctx)
	{
		if (!v.name.empty())
			return format_to(ctx.out(), "{}", v.name);
		else if (std::holds_alternative<klex::regular::Rule>(v.literal))
			return format_to(ctx.out(), "{}", std::get<klex::regular::Rule>(v.literal).name);
		else
			return format_to(ctx.out(), "\"{}\"", std::get<std::string>(v.literal));
	}
};

template <>
struct formatter<klex::cfg::NonTerminal> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::cfg::NonTerminal& v, FormatContext& ctx)
	{
		return format_to(ctx.out(), "{}", v.name);
	}
};

template <>
struct formatter<klex::cfg::Action> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::cfg::Action& v, FormatContext& ctx)
	{
		return format_to(ctx.out(), "{{{}}}", v.id);
	}
};

template <>
struct formatter<klex::cfg::HandleElement> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::cfg::HandleElement& v, FormatContext& ctx)
	{
		if (std::holds_alternative<klex::cfg::Terminal>(v))
			return format_to(ctx.out(), "{}", std::get<klex::cfg::Terminal>(v));
		else if (std::holds_alternative<klex::cfg::NonTerminal>(v))
			return format_to(ctx.out(), "{}", std::get<klex::cfg::NonTerminal>(v));
		else
			return format_to(ctx.out(), "{}", std::get<klex::cfg::Action>(v));
	}
};

template <>
struct formatter<klex::cfg::Symbol> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::cfg::Symbol& v, FormatContext& ctx)
	{
		if (std::holds_alternative<klex::cfg::Terminal>(v))
			return format_to(ctx.out(), "{}", std::get<klex::cfg::Terminal>(v));
		else
			return format_to(ctx.out(), "{}", std::get<klex::cfg::NonTerminal>(v));
	}
};

template <>
struct formatter<klex::cfg::Handle> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::cfg::Handle& handle, FormatContext& ctx)
	{
		return format_to(ctx.out(), "{}", to_string(handle));
	}
};

template <>
struct formatter<klex::cfg::Production> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::cfg::Production& v, FormatContext& ctx)
	{
		return format_to(ctx.out(), "{}", v.name, v.handle);
	}
};

template <>
struct formatter<std::vector<klex::cfg::Terminal>> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const std::vector<klex::cfg::Terminal>& terminals, FormatContext& ctx)
	{
		std::stringstream sstr;
		size_t i = 0;
		for (const klex::cfg::Terminal& t : terminals)
			if (i++)
				sstr << ", " << fmt::format("{}", t);
			else
				sstr << fmt::format("{}", t);

		return format_to(ctx.out(), "{}", sstr.str());
	}
};

template <>
struct formatter<std::set<klex::cfg::Terminal>> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const std::set<klex::cfg::Terminal>& terminals, FormatContext& ctx)
	{
		std::stringstream sstr;
		size_t i = 0;
		for (const klex::cfg::Terminal& t : terminals)
			if (i++)
				sstr << ", " << fmt::format("{}", t);
			else
				sstr << fmt::format("{}", t);

		return format_to(ctx.out(), "{}", sstr.str());
	}
};

template <>
struct formatter<std::set<klex::cfg::NonTerminal>> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const std::set<klex::cfg::NonTerminal>& nonterminals, FormatContext& ctx)
	{
		std::stringstream sstr;
		size_t i = 0;
		for (const klex::cfg::NonTerminal& nt : nonterminals)
			if (i++)
				sstr << ", " << fmt::format("{}", nt);
			else
				sstr << fmt::format("{}", nt);

		return format_to(ctx.out(), "{}", sstr.str());
	}
};
}  // namespace fmt

// }}}

#include <klex/cfg/Grammar-inl.h>

// vim:ts=4:sw=4:noet
