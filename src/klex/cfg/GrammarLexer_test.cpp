// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/GrammarLexer.h>
#include <klex/util/literals.h>
#include <klex/util/testing.h>

using namespace std;
using namespace klex;
using namespace klex::util::literals;

using cfg::Grammar;
using cfg::GrammarLexer;

TEST(cfg_GrammarLexer, literals)
{
    GrammarLexer lexer(R"('1' '23' '456' "789")");

    ASSERT_EQ(GrammarLexer::Token::Literal, lexer.recognize());
    ASSERT_EQ("1", lexer.currentLiteral());

    ASSERT_EQ(GrammarLexer::Token::Literal, lexer.recognize());
    ASSERT_EQ("23", lexer.currentLiteral());

    ASSERT_EQ(GrammarLexer::Token::Literal, lexer.recognize());
    ASSERT_EQ("456", lexer.currentLiteral());

    ASSERT_EQ(GrammarLexer::Token::Literal, lexer.recognize());
    ASSERT_EQ("789", lexer.currentLiteral());

    ASSERT_EQ(GrammarLexer::Token::Eof, lexer.recognize());
}

TEST(cfg_GrammarLexer, tokenization)
{
    GrammarLexer lexer(R"(:
			:Expr			::= Expr '+' Term			{addExpr}
			:						| Expr '-' Term			{subExpr}
			:						;
			:)"_multiline);

    ASSERT_EQ(GrammarLexer::Token::Identifier, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::Assoc, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::Identifier, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::Literal, lexer.recognize());
    ASSERT_EQ("+", lexer.currentLiteral());
    ASSERT_EQ(GrammarLexer::Token::Identifier, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::SetOpen, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::Identifier, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::SetClose, lexer.recognize());

    ASSERT_EQ(GrammarLexer::Token::Or, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::Identifier, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::Literal, lexer.recognize());
    ASSERT_EQ("-", lexer.currentLiteral());
    ASSERT_EQ(GrammarLexer::Token::Identifier, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::SetOpen, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::Identifier, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::SetClose, lexer.recognize());

    ASSERT_EQ(GrammarLexer::Token::Semicolon, lexer.recognize());
    ASSERT_EQ(GrammarLexer::Token::Eof, lexer.recognize());
}

// vim:ts=4:sw=4:noet
