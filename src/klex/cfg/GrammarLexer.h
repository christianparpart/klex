// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/cfg/Grammar.h>
#include <fmt/format.h>

namespace klex::cfg {

class GrammarLexer
{
  public:
	explicit GrammarLexer(std::string content);

	enum class Token {
		Illegal,
		Spacing,     // [\s\t\n]+
		Identifier,  // [a-z][a-z0-9]*
		Token,       // 'token'
		Literal,     // '[^']*'|"[^"]*"
		Or,          // '|'
		Semicolon,   // ';'
		Assoc,       // '::='
		SetOpen,     // '{'
		SetClose,    // '}'
		Eof,         // <<EOF>>
	};

	bool eof() const noexcept { return offset_ >= content_.size(); }
	size_t currentOffset() const { return offset_; }
	Token currentToken() const { return currentToken_; }
	const std::string& currentLiteral() const noexcept { return currentLiteral_; }

	Token recognize();

	std::string consumeLiteralUntilLF();  // NB. only used for sub-language (klex)

  private:
	Token recognizeOne();
	Token consumeIdentifier();
	Token consumeLiteral();
	int currentChar() const;
	int peekChar(size_t offset) const;
	int consumeChar(size_t count = 1);

  private:
	std::string content_;
	size_t offset_;
	std::string currentLiteral_;
	Token currentToken_;
};

inline std::string to_string(klex::cfg::GrammarLexer::Token v)
{
	switch (v)
	{
		case klex::cfg::GrammarLexer::Token::Spacing:
			return "Spacing";
		case klex::cfg::GrammarLexer::Token::Identifier:
			return "Identifier";
		case klex::cfg::GrammarLexer::Token::Token:
			return "Token";
		case klex::cfg::GrammarLexer::Token::Literal:
			return "Literal";
		case klex::cfg::GrammarLexer::Token::Or:
			return "'|'";
		case klex::cfg::GrammarLexer::Token::Semicolon:
			return "';'";
		case klex::cfg::GrammarLexer::Token::Assoc:
			return "'::='";
		case klex::cfg::GrammarLexer::Token::SetOpen:
			return "'{'";
		case klex::cfg::GrammarLexer::Token::SetClose:
			return "'}'";
		case klex::cfg::GrammarLexer::Token::Eof:
			return "<<EOF>>";
		// case klex::cfg::GrammarLexer::Illegal:
		default:
			return "Illegal";
	}
}

}  // namespace klex::cfg

namespace fmt {
template <>
struct formatter<klex::cfg::GrammarLexer::Token> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const klex::cfg::GrammarLexer::Token& v, FormatContext& ctx)
	{
		return format_to(ctx.out(), "{}", to_string(v));
	}
};
}  // namespace fmt

// vim:ts=4:sw=4:noet
