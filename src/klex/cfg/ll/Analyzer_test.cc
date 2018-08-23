// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/util/testing.h>
#include <klex/util/literals.h>
#include <klex/cfg/ll/Analyzer.h>
#include <klex/cfg/ll/SyntaxTable.h>
#include <klex/cfg/GrammarParser.h>
#include <klex/cfg/GrammarLexer.h>
#include <klex/cfg/Grammar.h>
#include <klex/Report.h>
#include <variant>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::cfg::ll;
using namespace klex::util::literals;

const std::string balancedParentheses = "A ::= '(' A ')' | '(' ')'";

enum class Symbol {
	// NT
	Expr,
	Term,
	Factor,
	// Terminals
	Add,
	Sub,
	Mul,
	Div,
	Number,
	RndOpen,
	RndClose,
	Eof,
};

// template<> bool TokenTraits<Symbol>::isEof(Symbol X) {
//	 return X == Symbol::Eof;
// }
// 
// template<> bool TokenTraits<Symbol>::isNonTerminal(Symbol X) {
//	 switch (X) {
//		 case Symbol::Expr:
//		 case Symbol::Term:
//		 case Symbol::Factor:
//			 return true;
//		 default:
//			 return false;
//	 }
// }
// 
// template<> bool TokenTraits<Symbol>::isTerminal(Symbol X) {
//	 return !isEof(X) && !isNonTerminal(X);
// }

klex::cfg::ll::SyntaxTable expressionGrammar()
{
	// BNF grammar (left-recursion eliminated, left-factorized)
	// ========================================================
	//
	// [0]	 Start	::= Expr;
	// [1]	 Expr	 ::= Term Expr'
	// [2]	 Expr'	::= '+' Expr'
	// [3]						| '-' Expr'
	// [4]						| <<epsilon>>;
	// [5]	 Term	 ::= Factor Term'
	// [6]	 Term'	::= '*' Term'
	// [7]						| '/' Term'
	// [8]						| <<epsilon>>;
	// [9]	 Factor ::= '(' Expr ')'
	// [10]					 | NUMBER

	// Nonterminal	| FIRST-set	 | FOLLOW-set
	// -------------+-------------+------------
	// Expr				 | NUMBER '('	| EOF
	// Expr				 | NUMBER '('	| EOF ')'
	// Expr'				| '+' '-'	E	| EOF ')
	// Term				 | NUMBER '('	| EOF ')' '+' '-'
	// Term'				| '*' '/' E	 | EOF ')' '+' '-'
	// Factor			 | NUMBER '('	| EOF ')' '+' '-' '*' '/'

	// grammar in LL(1) syntax table
	// =============================
	//
	// NT \ T	 | NUMBER | '+' | '-' | '*' | '/' | '(' | ')' | EOF
	// ---------+--------+-----+-----+-----+-----+-----+-----+------
	// Start		|		1	 |		 |		 |		 |		 |	1	|		 |
	// Expr		 |		1	 |		 |		 |		 |		 |	1	|		 |
	// Expr'		|				|	2	|	3	|		 |		 |		 |	4	| 4
	// Term		 |	 10	 |		 |		 |		 |		 |	9	|		 |
	// Term'		|				|		 |		 |	6	|	7	|		 |	8	| 8
	// Factor	 |	 10	 |		 |		 |		 |		 |	9	|		 |

	static const int table[10][8] = {
		{	1, -1, -1, -1, -1,	1, -1, -1 }, // Start
		{	1, -1, -1, -1, -1,	1, -1, -1 }, // Expr
		{ -1,	2,	3, -1, -1, -1,	4,	4 }, // Expr'
		{ 10, -1, -1, -1, -1,	9, -1, -1 }, // Term
		{ -1, -1, -1,	6,	7, -1,	8,	8 }, // Term'
		{ 10, -1, -1, -1, -1,	9, -1, -1 }, // Factor
	};

	klex::cfg::ll::SyntaxTable st;
	// st.table = {
	//	 { 1, { 1, 2, 3 }}, // Expr	 -> Term Expr'
	//	 { 2, { 1, 2, 3 }}, // Expr'	-> '+' Expr
	//	 { 2, { 1, 2, 3 }}, // Expr'' -> '-' Expr
	// };

	return std::move(st);
}

// vim:ts=4:sw=4:noet
