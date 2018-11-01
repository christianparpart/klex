// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/regular/LexerDef.h>
#include <fmt/format.h>

#include <cassert>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace klex::regular {

template<typename Token = Tag>
struct TokenInfo {
	Token token;
	std::string literal;
	size_t offset;

	operator Token() const noexcept { return token; }
};

template<typename Token>
inline const std::string& to_string(const TokenInfo<Token>& info) noexcept
{
	return info.literal;
}

/**
 * Lexer API for recognizing words.
 */
template <typename Token = Tag, typename Machine = StateId, const bool RequiresBeginOfLine = true,
		  const bool Debug = false>
class Lexer {
  public:
	using value_type = Token;
	using DebugLogger = std::function<void(const std::string&)>;
	using TokenInfo = klex::regular::TokenInfo<Token>;

	/**
	 * Constructs the Lexer with the given information table.
	 */
	explicit Lexer(LexerDef info, DebugLogger logger = DebugLogger{});

	/**
	 * Constructs the Lexer with the given information table and input stream.
	 */
	Lexer(LexerDef info, std::unique_ptr<std::istream> input, DebugLogger logger = DebugLogger{});

	/**
	 * Constructs the Lexer with the given information table and input stream.
	 */
	Lexer(LexerDef info, std::istream& input, DebugLogger logger = DebugLogger{});

	/**
	 * Constructs the Lexer with the given information table and input stream.
	 */
	Lexer(LexerDef info, std::string input, DebugLogger logger = DebugLogger{});

	/**
	 * Open given input stream.
	 */
	void reset(std::unique_ptr<std::istream> input);
	void reset(const std::string& input);

	/**
	 * Recognizes one token (ignored patterns are skipped).
	 */
	Token recognize();

	/**
	 * Recognizes one token, regardless of it is to be ignored or not.
	 */
	Token recognizeOne();

	//! the underlying word of the currently recognized token
	const std::string& word() const { return word_; }

	//! @returns the absolute offset of the file the lexer is currently reading from.
	std::pair<unsigned, unsigned> offset() const noexcept { return std::make_pair(oldOffset_, offset_); }

	//! @returns the last recognized token.
	Token token() const noexcept { return token_; }

	//! @returns the name of the current token.
	const std::string& name() const { return name(token_); }

	//! Tests whether given token is actually being recognized by this lexer.
	bool isToken(Tag t) const noexcept
	{
		for (const std::pair<StateId, Tag>& s_A : acceptStates_)
			if (s_A.second == t)
				return true;

		return false;
	}

	//! @returns the name of the token represented by Token @p t.
	const std::string& name(Token t) const
	{
		auto i = tagNames_.find(static_cast<Tag>(t));
		assert(i != tagNames_.end());
		return i->second;
	}

	/**
	 * Retrieves the next state for given input state and input symbol.
	 *
	 * @param currentState the current State the DFA is in to.
	 * @param inputSymbol the input symbol that is used for transitioning from current state to the next
	 * state.
	 * @returns the next state to transition to.
	 */
	inline StateId delta(StateId currentState, Symbol inputSymbol) const;

	/**
	 * Sets the active deterministic finite automaton to use for recognizing words.
	 *
	 * @param machine the DFA machine to use for recognizing words.
	 */
	Machine setMachine(Machine machine)
	{
		// since Machine is a 1:1 mapping into the State's ID, we can simply cast here.
		initialStateId_ = static_cast<StateId>(machine);
	}

	/**
	 * Retrieves the default DFA machine that is used to recognize words.
	 */
	Machine defaultMachine() const
	{
		auto i = initialStates_.find("INITIAL");
		assert(i != initialStates_.end());
		return static_cast<Machine>(i->second);
	}

	/**
	 * Runtime exception that is getting thrown when a word could not be recognized.
	 */
	struct LexerError : public std::runtime_error {
		LexerError(unsigned int _offset)
			: std::runtime_error{fmt::format("[{}] Failed to lexically recognize a word.", _offset)},
			  offset{_offset}
		{
		}

		unsigned int offset;
	};

	struct iterator {
		Lexer& lx;
		int end;
		TokenInfo info;

		const TokenInfo& operator*() const { return info; }

		iterator& operator++()
		{
			if (lx.eof())
				++end;

			info.token = lx.recognize();
			info.literal = lx.word();
			info.offset = lx.offset().first;

			return *this;
		}

		iterator& operator++(int) { return ++*this; }
		bool operator==(const iterator& rhs) const noexcept { return end == rhs.end; }
		bool operator!=(const iterator& rhs) const noexcept { return !(*this == rhs); }
	};

	iterator begin()
	{
		const Token t = recognize();
		return iterator{*this, 0, t, word()};
	}

	iterator end() { return iterator{*this, 2, 0, ""}; }

	bool eof() const { return !stream_->good(); }

	size_t fileSize() const noexcept { return fileSize_; }

  private:
	template <typename... Args>
	inline void debugf(const char* msg, Args... args) const
	{
		if constexpr (Debug)
			if (debug_)
				debug_(fmt::format(msg, args...));
	}

	Symbol nextChar();
	void rollback();
	StateId getInitialState() const noexcept;
	bool isAcceptState(StateId state) const;
	static std::string stateName(StateId s, const std::string_view& n = "n");
	static constexpr StateId BadState = 101010;
	std::string toString(const std::deque<StateId>& stack);

	int currentChar() const noexcept { return currentChar_; }

	Token token(StateId s) const
	{
		auto i = acceptStates_.find(s);
		assert(i != acceptStates_.end());
		return static_cast<Token>(i->second);
	}

	size_t getFileSize();

  private:
	const TransitionMap transitions_;
	const std::map<std::string, StateId> initialStates_;
	const bool containsBeginOfLineStates_;
	const std::map<StateId, Tag> acceptStates_;
	const BacktrackingMap backtracking_;
	const std::map<Tag, std::string> tagNames_;
	const DebugLogger debug_;
	Machine initialStateId_;
	std::string word_;
	std::unique_ptr<std::istream> ownedStream_;
	std::istream* stream_;
	std::vector<int> buffered_;
	unsigned oldOffset_;
	unsigned offset_;
	size_t fileSize_;  // cache
	bool isBeginOfLine_;
	int currentChar_;
	Token token_;
};

template <typename Token = Tag, typename Machine = StateId, const bool RequiresBeginOfLine = true,
		  const bool Debug = false>
inline const std::string& to_string(const typename Lexer<Token, Machine, RequiresBeginOfLine, Debug>::iterator& it) noexcept
{
	return it.info.literal;
}

}  // namespace klex::regular

#include <klex/regular/Lexer-inl.h>
