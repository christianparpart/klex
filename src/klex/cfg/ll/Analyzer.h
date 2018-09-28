// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/Report.h>
#include <klex/cfg/ll/SyntaxTable.h>
#include <klex/regular/Lexer.h>

#include <deque>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace klex::cfg::ll {

template <typename SemanticValue>
class Analyzer {
  public:
	using Terminal = regular::Tag;  // typename regular::Lexer<regular::Tag>::value_type;
	using NonTerminal = int;
	using Action = int;
	using Lexer = regular::Lexer<regular::Tag, regular::StateId, true, false>;
	using ActionHandler = std::function<SemanticValue(int, const Analyzer<SemanticValue>&)>;

	struct StackValue {
		int value;
		operator int() const noexcept { return value; }
		StackValue(int _value) : value{_value} {}
	};

	Analyzer(SyntaxTable table, Report* report, std::string input,
			 ActionHandler actionHandler = ActionHandler());

	const Lexer& lexer() const noexcept { return lexer_; }
	const std::string& lastLiteral() const noexcept { return lastLiteral_; }

	const std::string& actionName(int id) const noexcept { return def_.actionNames[id - def_.actionMin()]; }

	const SemanticValue& value(int offset) const
	{
		return valueStack_[valueStackBase_ + static_cast<size_t>(offset)];
	}

	void analyze();

  private:
	std::vector<SemanticValue> valueStack_;
	size_t valueStackBase_;

	std::optional<SyntaxTable::Expression> getHandleFor(StackValue nonterminal,
														Terminal currentTerminal) const;

	bool isAction(StackValue v) const noexcept;
	bool isTerminal(StackValue v) const noexcept;
	bool isNonTerminal(StackValue v) const noexcept;

	void log(const std::string& msg);

	std::string dumpStack() const;
	std::string stackValue(StackValue sv) const;
	std::string handleString(const SyntaxTable::Expression& handle) const;

  private:
	const SyntaxTable def_;
	Lexer lexer_;
	std::string lastLiteral_;
	Report* report_;
	std::deque<StackValue> stack_;
	ActionHandler actionHandler_;
};

}  // namespace klex::cfg::ll

namespace fmt {
template <>
struct formatter<typename klex::cfg::ll::Analyzer<int>::StackValue> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::cfg::ll::Analyzer<int>::StackValue& v, FormatContext& ctx)
	{
		return format_to(ctx.begin(), "{}", "hello");
	}
};
}  // namespace fmt

#include <klex/cfg/ll/Analyzer-inl.h>

// vim:ts=4:sw=4:noet
