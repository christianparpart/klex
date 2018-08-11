// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <map>
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

	bool operator<(const Terminal& rhs) const noexcept { return literal < rhs.literal; }
};

struct NonTerminal {
	std::string name;				// such as "IfStmt"
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

	bool operator<(const Production& rhs) const noexcept { return name < rhs.name; }
};

struct GrammarMetadata {
	std::vector<NonTerminal> nonterminals;
	std::vector<Terminal> terminals;

	std::map<Symbol, std::set<Terminal>> first;
	std::map<NonTerminal, std::set<Terminal>> follow;
	std::set<NonTerminal> epsilon;
	std::map<Symbol, std::set<Terminal>> first1;
};

struct Grammar {
	std::vector<Production> productions;

	std::vector<const Production*> getProductions(const NonTerminal& nt) const;

	GrammarMetadata metadata() const;
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
