// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/regular/Rule.h>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <sstream>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace klex::cfg {

struct Terminal {
	std::string literal;			// such as "if"
	std::string name;				// such as "KW_IF"

	// pre-parsed rule (XXX: could be made `variant<Rule, string> literal` instead
	std::optional<regular::Rule> rule;

	bool operator<(const Terminal& rhs) const noexcept { return literal < rhs.literal; }
};

struct NonTerminal {
	std::string name;				// such as "IfStmt"

	bool operator==(const std::string& other) const { return name == other; }
};

using Symbol = std::variant<NonTerminal, Terminal>;

inline bool operator<(const Symbol& a, const Symbol& b) {
	using namespace std;
	const string& lhs = holds_alternative<Terminal>(a) ? get<Terminal>(a).literal : get<NonTerminal>(a).name;
	const string& rhs = holds_alternative<Terminal>(b) ? get<Terminal>(b).literal : get<NonTerminal>(b).name;
	return lhs < rhs;
}

struct Handle {
	std::vector<Symbol> symbols;
	std::string ref;
};

inline std::string to_string(const Handle& handle) {
	std::stringstream sstr;

	int i = 0;
	for (const klex::cfg::Symbol& symbol : handle.symbols) {
		if (i++) sstr << ' ';
		sstr << fmt::format("{}", symbol);
	}

	if (!handle.ref.empty())
		sstr << " {" << handle.ref << "}";

	return sstr.str();
}

struct Production {
	std::string name;
	Handle handle;

	int id;
	bool epsilon;
	std::vector<Terminal> first;
	std::vector<Terminal> follow;

	std::vector<Terminal> first1() const;
};

struct Grammar {
	std::vector<regular::Rule> explicitTerminals;     //!< List of terminals with explicit definitions.
	std::vector<Production> productions;              //!< List of grammar productions rules.

	std::vector<NonTerminal> nonterminals;            //!< Accumulated list of non-terminals, filled by finalize().
	std::vector<Terminal> terminals;                  //!< Accumulated list of terminals (including explicitely specified terminals), filled by finalize().

	std::vector<const Production*> getProductions(const NonTerminal& nt) const;
	std::vector<Production*> getProductions(const NonTerminal& nt);

	bool containsEpsilon(const Symbol& s) const {
		return std::holds_alternative<NonTerminal>(s)
			&& containsEpsilon(std::get<NonTerminal>(s));
	}

	bool containsEpsilon(const NonTerminal& nt) const {
		for (const Production* p : getProductions(nt))
			if (p->epsilon)
				return true;

		return false;
	}

	std::vector<Terminal> firstOf(const Symbol& b) const;
	std::vector<Terminal> followOf(const NonTerminal& nt) const;

	void clearMetadata();
	void finalize();

	std::string dump() const;
};

} // namespace klex::cfg

// {{{ fmtlib integration
namespace fmt {
	template<>
	struct formatter<klex::cfg::Terminal> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		constexpr auto format(const klex::cfg::Terminal& v, FormatContext &ctx) {
			if (!v.name.empty())
				return format_to(ctx.begin(), "{}", v.name);
			else
				return format_to(ctx.begin(), "\"{}\"", v.literal);
		}
	};

	template<>
	struct formatter<klex::cfg::NonTerminal> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		constexpr auto format(const klex::cfg::NonTerminal& v, FormatContext &ctx) {
			return format_to(ctx.begin(), "{}", v.name);
		}
	};

	template<>
	struct formatter<klex::cfg::Symbol> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		constexpr auto format(const klex::cfg::Symbol& v, FormatContext &ctx) {
			if (std::holds_alternative<klex::cfg::Terminal>(v))
				return format_to(ctx.begin(), "{}", std::get<klex::cfg::Terminal>(v));
			else
				return format_to(ctx.begin(), "{}", std::get<klex::cfg::NonTerminal>(v));
		}
	};

	template<>
	struct formatter<klex::cfg::Handle> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		constexpr auto format(const klex::cfg::Handle& handle, FormatContext &ctx) {
			return format_to(ctx.begin(), "{}", to_string(handle));
		}
	};

	template<>
	struct formatter<klex::cfg::Production> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		constexpr auto format(const klex::cfg::Production& v, FormatContext &ctx) {
			return format_to(ctx.begin(), "{:} ::= {};", v.name, v.handle);
		}
	};

	template<>
	struct formatter<std::vector<klex::cfg::Terminal>> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		constexpr auto format(const std::vector<klex::cfg::Terminal>& terminals, FormatContext &ctx) {
			std::stringstream sstr;
			size_t i = 0;
			for (const klex::cfg::Terminal& t : terminals)
				if (i++)
					sstr << ", " << fmt::format("{}", t);
				else
					sstr << fmt::format("{}", t);

			return format_to(ctx.begin(), "{}", sstr.str());
		}
	};

	template<>
	struct formatter<std::set<klex::cfg::Terminal>> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		constexpr auto format(const std::set<klex::cfg::Terminal>& terminals, FormatContext &ctx) {
			std::stringstream sstr;
			size_t i = 0;
			for (const klex::cfg::Terminal& t : terminals)
				if (i++)
					sstr << ", " << fmt::format("{}", t);
				else
					sstr << fmt::format("{}", t);

			return format_to(ctx.begin(), "{}", sstr.str());
		}
	};

	template<>
	struct formatter<std::set<klex::cfg::NonTerminal>> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		constexpr auto format(const std::set<klex::cfg::NonTerminal>& nonterminals, FormatContext &ctx) {
			std::stringstream sstr;
			size_t i = 0;
			for (const klex::cfg::NonTerminal& nt : nonterminals)
				if (i++)
					sstr << ", " << fmt::format("{}", nt);
				else
					sstr << fmt::format("{}", nt);

			return format_to(ctx.begin(), "{}", sstr.str());
		}
	};
}
// }}}

// vim:ts=4:sw=4:noet
