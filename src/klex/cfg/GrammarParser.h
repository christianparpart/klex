// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/cfg/Grammar.h>
#include <klex/cfg/GrammarLexer.h>
#include <fmt/format.h>
#include <iostream>

namespace klex {
class Report;
}

namespace klex::cfg {

/**
 * Parses a context-free-grammar specification.
 */
class GrammarParser
{
  public:
	GrammarParser(GrammarLexer&& lexer, Report* report);
	GrammarParser(std::string source, Report* report);

	Grammar parse();
	void parseRule();
	Handle parseHandle();

  private:
	using Token = GrammarLexer::Token;

	void parseTokenBlock();

	const std::string& currentLiteral() const noexcept { return lexer_.currentLiteral(); }
	Token currentToken() const noexcept { return lexer_.currentToken(); }
	void consumeToken();
	void consumeToken(Token expectedToken);

	std::optional<const regular::Rule*> findExplicitTerminal(const std::string& terminalName) const;

  private:
	Report* report_;
	GrammarLexer lexer_;
	Grammar grammar_;
};

}  // namespace klex::cfg

// vim:ts=4:sw=4:noet
