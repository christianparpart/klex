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
std::optional<SemanticValue> Analyzer<SemanticValue>::analyze()
{
	using namespace std;
	using ::klex::util::reversed;

	const auto eof = std::end(lexer_);
	auto currentToken = std::begin(lexer_);

	// put start symbol onto stack
	stack_.emplace_back(def_.startSymbol);

	for (;;)
	{
		log(fmt::format("current token    : {}", def_.terminalName(*currentToken)));
		log(fmt::format("  state stack    : {}", dumpStateStack()));
		log(fmt::format("  semantic stack : {}", dumpSemanticStack()));

		if (currentToken == eof && stack_.empty())  // fully parsed program, and success?
		{
			// if (currentToken == eof && isTerminal(X) && X == *currentToken)
			// if (X == *currentToken && currentToken == eof)
			if (!valueStack_.empty())
				return move(valueStack_.back());
			else
				return SemanticValue{};
		}

		const StateValue X = stack_.back();

		if (X < 0)
		{
			stack_.pop_back();
			SemanticValue y = valueStack_.back();
			valueStack_.resize(valueStack_.size() + X);
			valueStack_.emplace_back(move(y));
			log("    rewinding");
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

			valueStack_.emplace_back(SemanticValue{}); // TODO: FIXME <<EOF>> handling

			log(fmt::format("    eat terminal: {} '{}'", def_.terminalName(X), currentToken.literal));
			lastLiteral_ = currentToken.literal;
			++currentToken;
		}
		else if (isNonTerminal(X))
		{
			if (optional<SyntaxTable::Expression> handle = getHandleFor(X, *currentToken); handle.has_value())
			{
				log(fmt::format("    Apply production for: ({}, {}) -> {}", def_.nonterminalName(X),
								def_.terminalName(*currentToken), handleString(*handle)));
				stack_.pop_back();

				if (!handle->empty())
				{
					stack_.push_back(-handle->size());  // XXX valueStack rewind-magic
					for (const int x : reversed(*handle))
						stack_.push_back(StateValue{x});
				}
				else
				{
					// epsilon-rule being applied
					assert(!valueStack_.empty());
					valueStack_.emplace_back(move(valueStack_.back()));
				}
			}
			else
			{
				// XXX parse error. Cannot reduce non-terminal X. No production found.
				report_->syntaxError(SourceLocation{/*TODO*/},
									 "Syntax error detected at non-terminal {} with terminal {}.",
									 def_.nonterminalName(X), def_.terminalName(*currentToken));
				return std::nullopt;
			}
		}
		else  // if (isAction(X))
		{
			assert(isAction(X));
			log(fmt::format("    running action: {}", actionName(X)));
			stack_.pop_back();
			if (actionHandler_)
				valueStack_.emplace_back(actionHandler_(X, *this));
			else
				valueStack_.emplace_back(SemanticValue{});
		}
	}
}

template <typename SemanticValue>
std::optional<SyntaxTable::Expression> Analyzer<SemanticValue>::getHandleFor(StateValue nonterminal,
																			 Terminal currentTerminal) const
{
	if (std::optional<int> p_i = def_.lookup(nonterminal, currentTerminal); p_i.has_value())
		return def_.productions[*p_i];

	return std::nullopt;
}

template <typename SemanticValue>
bool Analyzer<SemanticValue>::isTerminal(StateValue v) const noexcept
{
	return def_.isTerminal(v);
}

template <typename SemanticValue>
bool Analyzer<SemanticValue>::isNonTerminal(StateValue v) const noexcept
{
	return def_.isNonTerminal(v);
}

template <typename SemanticValue>
bool Analyzer<SemanticValue>::isAction(StateValue v) const noexcept
{
	return def_.isAction(v);
}

template <typename SemanticValue>
void Analyzer<SemanticValue>::log(const std::string& msg)
{
	fmt::print("Analyzer: {}\n", msg);
}

template <typename SemanticValue>
std::string Analyzer<SemanticValue>::dumpStateStack() const
{
	return util::join(util::translate(stack_, [this](StateValue sv) { return stateValue(sv); }), " ");
}

template <typename SemanticValue>
std::string Analyzer<SemanticValue>::dumpSemanticStack() const
{
	return util::join(util::translate(valueStack_, [](SemanticValue sv) { return fmt::format("{}", sv); }),
					  " ");
}

template <typename SemanticValue>
std::string Analyzer<SemanticValue>::stateValue(StateValue sv) const
{
	assert(isNonTerminal(sv) || isTerminal(sv) || isAction(sv) || sv < 0);

	if (sv < 0)  // XXX rewind-tag
		return fmt::format("#{}", (int) sv);

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
	return util::join(util::translate(handle, [this](auto v) { return stateValue(v); }), " ");
}

}  // namespace klex::cfg::ll
