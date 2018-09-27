// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/regular/LexerDef.h>
#include <klex/util/iterator.h>
#include <algorithm>
#include <functional>
#include <sstream>

namespace klex::cfg::ll {

// --------------------------------------------------------------------------------------------------------

template <typename SemanticValue>
Analyzer<SemanticValue>::Analyzer(SyntaxTable _st, Report* _report, std::string _source,
								  ActionHandler actionHandler)
	: def_{std::move(_st)},
	  lexer_{def_.lexerDef, std::move(_source),
			 std::bind(&Analyzer<SemanticValue>::log, this, std::placeholders::_1)},
	  report_{_report},
	  stack_{},
	  actionHandler_{move(actionHandler)}
{
	log(def_.lexerDef.to_string());
}

template <typename Container>
std::string containerToString(const Container& values)
{
	std::stringstream sstr;

	for (auto&& [i, value] : util::indexed(values))
		if (i)
			sstr << ", " << value;
		else
			sstr << value;

	return sstr.str();
}

template <typename SemanticValue>
void Analyzer<SemanticValue>::analyze()
{
	using namespace std;
	using ::klex::util::reversed;

	const auto eof = std::end(lexer_);
	auto currentToken = std::begin(lexer_);

	deque<vector<SemanticValue>> valueStack;

	// put start symbol onto stack
	stack_.emplace_back(def_.startSymbol);
	valueStack.emplace_back(vector<SemanticValue>());

	for (const auto&& [i, t] : util::indexed(def_.terminalNames))
		log(fmt::format("terminal[{}] = {}", i, t));

	for (const auto&& [i, a] : util::indexed(def_.actionNames))
		log(fmt::format("action[{}] = {}", i, a));

	for (;;)
	{
		log(fmt::format("currentToken: {}, stack: {}", def_.terminalName(*currentToken), dumpStack()));

		if (currentToken == eof && stack_.empty())
			// if (currentToken == eof && isTerminal(X) && X == *currentToken)
			// if (X == *currentToken && currentToken == eof)
			return;  // fully parsed program, and success

		const StackValue X = stack_.back();

		if (X < 0)
		{
			log(fmt::format("Rewind value-stack to depth={}: {}.", valueStack.size() - 1,
							containerToString(valueStack.back())));
			stack_.pop_back();
			if (valueStack.back().empty())
				valueStack.pop_back();
			else
			{
				SemanticValue v = valueStack.back().back();
				valueStack.pop_back();
				valueStack.back().emplace_back(v);
			}
		}
		else if (isTerminal(X))
		{
			stack_.pop_back();
			if (X != *currentToken)
			{
				report_->syntaxError(SourceLocation{/*TODO*/},
									 "Unexpected token {}. Expected token {} instead.",
									 def_.terminalName(*currentToken), def_.terminalName(X));
				// TODO: proper error recovery
			}

			// log(fmt::format("- eat terminal: {} '{}'", def_.terminalName(X), currentToken.literal));
			lastLiteral_ = currentToken.literal;
			++currentToken;
		}
		else if (isNonTerminal(X))
		{
			if (optional<SyntaxTable::Expression> handle = getHandleFor(X, *currentToken); handle.has_value())
			{
				log(fmt::format("Apply production for: ({}, {}) -> {}", def_.nonterminalName(X),
								def_.terminalName(*currentToken), handleString(*handle)));
				stack_.pop_back();

				stack_.push_back(-1);  // XXX magic
				valueStack.emplace_back(vector<SemanticValue>());

				for (const int x : reversed(*handle))
					stack_.push_back(StackValue{x});
			}
			else
			{
				// XXX parse error. Cannot reduce non-terminal X. No production found.
				report_->syntaxError(SourceLocation{/*TODO*/},
									 "Syntax error detected at non-terminal {} with terminal {}.", X,
									 *currentToken);  // TODO: more elaborated diagnostics
				return;
			}
		}
		else  // if (isAction(X))
		{
			assert(isAction(X));
			stack_.pop_back();
			log(fmt::format("Run action {} at valueStack depth={}: {}.", actionName(X),
							valueStack.size(), containerToString(valueStack.back())));
			if (actionHandler_)
				valueStack.back().emplace_back(actionHandler_(X, *this));
		}
	}
}

template <typename SemanticValue>
std::optional<SyntaxTable::Expression> Analyzer<SemanticValue>::getHandleFor(StackValue nonterminal,
																			 Terminal currentTerminal) const
{
	if (std::optional<int> p_i = def_.lookup(nonterminal, currentTerminal); p_i.has_value())
		return def_.productions[*p_i];

	return std::nullopt;
}

template <typename SemanticValue>
bool Analyzer<SemanticValue>::isTerminal(StackValue v) const noexcept
{
	return def_.isTerminal(v);
}

template <typename SemanticValue>
bool Analyzer<SemanticValue>::isNonTerminal(StackValue v) const noexcept
{
	return def_.isNonTerminal(v);
}

template <typename SemanticValue>
bool Analyzer<SemanticValue>::isAction(StackValue v) const noexcept
{
	return def_.isAction(v);
}

template <typename SemanticValue>
void Analyzer<SemanticValue>::log(const std::string& msg)
{
	fmt::print("Analyzer: {}\n", msg);
}

template <typename SemanticValue>
std::string Analyzer<SemanticValue>::dumpStack() const
{
	std::stringstream os;
	for (const auto&& [i, sv] : util::indexed(stack_))
	{
		if (i)
			os << ' ';

		os << stackValue(sv);
	}
	return os.str();
}

template <typename SemanticValue>
std::string Analyzer<SemanticValue>::stackValue(StackValue sv) const
{
	assert(isNonTerminal(sv) || isTerminal(sv) || isAction(sv) || sv < 0);

	if (sv < 0)
		return "#";  // XXX rewind-tag

	if (isNonTerminal(sv))
		return fmt::format("<{}>", def_.nonterminalName(sv));
	else if (isTerminal(sv))
		return def_.terminalName(sv);
	else
		return fmt::format("!{}", def_.actionName(sv));
}

template <typename SemanticValue>
std::string Analyzer<SemanticValue>::handleString(const SyntaxTable::Expression& handle) const
{
	std::stringstream os;

	for (const auto&& [i, v] : util::indexed(handle))
		if (i)
			os << ' ' << stackValue(StackValue{v});
		else
			os << stackValue(StackValue{v});

	return os.str();
}

}  // namespace klex::cfg::ll
