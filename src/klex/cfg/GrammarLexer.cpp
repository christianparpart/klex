// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/cfg/GrammarLexer.h>

#include <fmt/format.h>

#include <cassert>
#include <cctype>
#include <iostream>

using namespace std;
using namespace klex;
using namespace klex::cfg;

GrammarLexer::GrammarLexer(string content):
    content_ { std::move(content) }, offset_ { 0 }, currentLiteral_ {}, currentToken_ { Token::Illegal }
{
}

GrammarLexer::Token GrammarLexer::recognize()
{
    for (;;)
    {
        if (Token t = recognizeOne(); t != Token::Spacing)
        {
            // cout << "recognize: " << fmt::format("{}", t) << "\n";
            return currentToken_ = t;
        }
    }
}

GrammarLexer::Token GrammarLexer::recognizeOne()
{
    currentLiteral_.clear();

    switch (currentChar())
    {
        case -1: return Token::Eof;
        case ' ':
        case '\t':
        case '\n':
            do
                consumeChar();
            while (!eof() && isspace(currentChar()));
            return Token::Spacing;
        case '{': consumeChar(); return Token::SetOpen;
        case '}': consumeChar(); return Token::SetClose;
        case '|': consumeChar(); return Token::Or;
        case ';': consumeChar(); return Token::Semicolon;
        case ':':
            if (peekChar(1) == ':' && peekChar(2) == '=')
            {
                consumeChar(3);
                return Token::Assoc;
            }
            return Token::Illegal;
        case '\'':
        case '"': return consumeLiteral();
        default:
            if (isalpha(currentChar()) || currentChar() == '_')
            {
                return consumeIdentifier();
            }
            consumeChar();
            return Token::Illegal;
    }
}

string GrammarLexer::consumeLiteralUntilLF()
{
    currentLiteral_.clear();

    while (!eof() && currentChar() != '\n')
    {
        currentLiteral_ += static_cast<char>(currentChar());
        consumeChar();
    }

    if (!eof())
    {
        currentLiteral_ += static_cast<char>(currentChar());
        consumeChar();
    }

    return currentLiteral_;
}

GrammarLexer::Token GrammarLexer::consumeIdentifier()
{
    assert(!eof() && (isalpha(currentChar()) || currentChar() == '_'));

    do
    {
        currentLiteral_ += static_cast<char>(currentChar());
        consumeChar();
    } while (!eof() && (isalnum(currentChar()) || currentChar() == '_'));

    if (currentLiteral_ == "token")
        return Token::Token;

    return Token::Identifier;
}

// ' ... ' | " ... "
GrammarLexer::Token GrammarLexer::consumeLiteral()
{
    assert(!eof() && (currentChar() == '"' || currentChar() == '\''));
    const int delimiter = currentChar();
    consumeChar();

    while (!eof() && currentChar() != delimiter)
    {
        currentLiteral_ += static_cast<char>(currentChar());
        consumeChar();
    }

    if (eof())
        return Token::Illegal; // Unexpected EOF

    consumeChar(); // delimiter

    return Token::Literal;
}

int GrammarLexer::currentChar() const
{
    if (offset_ < content_.size())
        return content_[offset_];
    else
        return -1; // EOF
}

int GrammarLexer::peekChar(size_t offset) const
{
    if (offset_ + offset < content_.size())
        return content_[offset_ + offset];
    else
        return -1; // EOF
}

int GrammarLexer::consumeChar(size_t count)
{
    offset_ += min(count, content_.size() - offset_);
    return currentChar();
}

// vim:ts=4:sw=4:noet
