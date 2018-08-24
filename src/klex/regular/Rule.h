// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/regular/LexerDef.h>  // IgnoreTag
#include <klex/regular/RegExpr.h>
#include <klex/regular/RegExprParser.h>
#include <klex/regular/State.h>  // Tag
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace klex::regular {

class RegExpr;

struct Rule {
	unsigned int line;
	unsigned int column;
	Tag tag;
	std::vector<std::string> conditions;
	std::string name;
	std::string pattern;
	std::unique_ptr<RegExpr> regexpr = nullptr;

	Rule clone() const
	{
		return regexpr ? Rule{line,
							  column,
							  tag,
							  conditions,
							  name,
							  pattern,
							  RegExprParser{}.parse(pattern, line, column)}
					   : Rule{line, column, tag, conditions, name, pattern, nullptr};
	}

	Rule() = default;

	Rule(unsigned _line, unsigned _column, Tag _tag, std::vector<std::string> _conditions, std::string _name,
		 std::string _pattern, std::unique_ptr<RegExpr> _regexpr = nullptr)
		: line{_line},
		  column{_column},
		  tag{_tag},
		  conditions{_conditions},
		  name{_name},
		  pattern{_pattern},
		  regexpr{std::move(_regexpr)}
	{
	}

	Rule(const Rule& v)
		: line{v.line},
		  column{v.column},
		  tag{v.tag},
		  conditions{v.conditions},
		  name{v.name},
		  pattern{v.pattern},
		  regexpr{v.regexpr ? RegExprParser{}.parse(pattern, line, column) : nullptr}
	{
	}

	Rule& operator=(const Rule& v)
	{
		line = v.line;
		column = v.column;
		tag = v.tag;
		conditions = v.conditions;
		name = v.name;
		pattern = v.pattern;
		regexpr = v.regexpr ? RegExprParser{}.parse(pattern, line, column) : nullptr;
		return *this;
	}

	bool operator<(const Rule& rhs) const noexcept { return tag < rhs.tag; }
	bool operator<=(const Rule& rhs) const noexcept { return tag <= rhs.tag; }
	bool operator==(const Rule& rhs) const noexcept { return tag == rhs.tag; }
	bool operator!=(const Rule& rhs) const noexcept { return tag != rhs.tag; }
	bool operator>=(const Rule& rhs) const noexcept { return tag >= rhs.tag; }
	bool operator>(const Rule& rhs) const noexcept { return tag > rhs.tag; }
};

using RuleList = std::vector<Rule>;

inline bool ruleContainsBeginOfLine(const Rule& r)
{
	return containsBeginOfLine(r.regexpr.get());
}

}  // namespace klex::regular

namespace fmt {
template <>
struct formatter<klex::regular::Rule> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::regular::Rule& v, FormatContext& ctx)
	{
		// TODO: inject conditions in the beginning, if present
		if (v.tag == klex::regular::IgnoreTag)
			return format_to(ctx.begin(), "{}({}) ::= {}", v.name, "ignore", v.pattern);
		else
			return format_to(ctx.begin(), "{}({}) ::= {}", v.name, v.tag, v.pattern);
	}
};
}  // namespace fmt
