// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Report.h>
#include <klex/cfg/Grammar.h>
#include <klex/cfg/GrammarLexer.h>
#include <klex/cfg/GrammarParser.h>
#include <klex/regular/RuleParser.h>

#include <algorithm>

using namespace std;
using namespace klex;
using namespace klex::cfg;

#define DEBUG(msg, ...) \
	do                  \
	{                   \
	} while (0)
// #define DEBUG(msg, ...) do { fmt::print((msg), __VA_ARGS__); fmt::print("\n"); } while (0)

GrammarParser::GrammarParser(GrammarLexer _lexer, Report* _report) : report_{_report}, lexer_{move(_lexer)}
{
}

Grammar GrammarParser::parse()
{
	consumeToken();

	while (currentToken() != Token::Eof)
	{
		switch (currentToken())
		{
			case Token::Token:
				parseTokenBlock();
				break;
			case Token::Identifier:
				// GrammarRule ::= NonTerminal '::=' Handle ('|' Handle)* ';'
				parseRule();
				break;
			default:
				report_->syntaxError(SourceLocation{}, "Unexpected token {}. Expecting a rule instead.",
									 currentToken());
				consumeToken();
				abort();
				break;
		}
	}

	consumeToken(Token::Eof);

	return move(grammar_);
}

void GrammarParser::consumeToken()
{
	lexer_.recognize();
	// DEBUG("consumeToken: {} \"{}\"", lexer_.currentToken(), lexer_.currentLiteral());
}

void GrammarParser::consumeToken(Token expectedToken)
{
	if (lexer_.currentToken() != expectedToken)
		report_->syntaxError(SourceLocation{}, "Expected token {} but got {}.", expectedToken,
							 lexer_.currentToken());

	consumeToken();
}

void GrammarParser::parseRule()
{
	// GrammarRule  ::= NonTerminal '::=' Handle ('|' Handle)* ';'

	string name{currentLiteral()};
	consumeToken(Token::Identifier);
	consumeToken(Token::Assoc);

	grammar_.productions.emplace_back(Production{name, parseHandle()});
	DEBUG("parsed production: {}", grammar_.productions.back());

	while (currentToken() == Token::Or)
	{
		consumeToken();
		grammar_.productions.emplace_back(Production{name, parseHandle()});
		DEBUG("parsed production: {}", grammar_.productions.back());
	}

	consumeToken(Token::Semicolon);
}

Handle GrammarParser::parseHandle()
{
	// Handle     ::= (Terminal | NonTerminal)* HandleRef?
	// HandleRef  ::= '{' Identifier '}'

	// N.B.: FOLLOW-set { ';', '|' }

	Handle handle;

	for (;;)
	{
		switch (currentToken())
		{
			case Token::Literal:
				handle.symbols.emplace_back(Terminal{currentLiteral(), ""});
				consumeToken();
				break;
			case Token::Identifier:
				if (auto i =
						find_if(begin(grammar_.explicitTerminals), end(grammar_.explicitTerminals),
								[&](const regular::Rule& w) -> bool { return w.name == currentLiteral(); });
					i != end(grammar_.explicitTerminals))
				{
					printf("terminal: %s\n", currentLiteral().c_str());
					handle.symbols.emplace_back(Terminal{*i, currentLiteral()});
				}
				else
				{
					printf("nonterminal: %s\n", currentLiteral().c_str());
					handle.symbols.emplace_back(NonTerminal{currentLiteral()});
				}
				consumeToken();
				break;
			case Token::SetOpen:
			{
				consumeToken();
				handle.ref = currentLiteral();
				consumeToken(Token::Identifier);
				consumeToken(Token::SetClose);
				return move(handle);
			}
			case Token::Semicolon:
			case Token::Or:
				return move(handle);
			case Token::Eof:
			case Token::Illegal:
			default:
				report_->syntaxError(
					SourceLocation{}, "Unexpected token {}. Expected instead one of: {}, {}, {}, {}.",
					currentToken(), Token::Or, Token::Semicolon, Token::Literal, Token::Identifier);
				return move(handle);
		}
	}

	// TODO: parse '{' Identifier '}'
}

void GrammarParser::parseTokenBlock()
{
	// TokenBlock ::= 'token' '{' TokenDef '}'
	// TokenDef   ::= IDENT '::=' RegExpr
	// RegExpr    ::= <regular expression>

	consumeToken();  // "token"
	consumeToken(Token::SetOpen);

	string klexDef;
	while (currentToken() == Token::Identifier)
	{
		klexDef += currentLiteral();
		klexDef += lexer_.consumeLiteralUntilLF();

		consumeToken();  // parses first token on next line
	}

	regular::RuleList rules = regular::RuleParser{klexDef}.parseRules();  // TODO: currentLine, currentColumn

	for (const regular::Rule& rule : rules)
		fmt::print("explict terminal: {}\n", rule);

	grammar_.explicitTerminals.insert(end(grammar_.explicitTerminals), begin(rules), end(rules));

	consumeToken(Token::SetClose);
}

// vim:ts=4:sw=4:noet
