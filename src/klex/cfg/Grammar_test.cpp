// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Report.h>
#include <klex/cfg/Grammar.h>
#include <klex/cfg/GrammarLexer.h>
#include <klex/cfg/GrammarParser.h>
#include <klex/util/literals.h>
#include <klex/util/testing.h>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::util::literals;

TEST(cfg_Grammar, handle_symbols)
{
    Handle h;
    h.emplace_back(Terminal { "a", "" });
    h.emplace_back(Action { "a1" });
    h.emplace_back(NonTerminal { "A" });

    ASSERT_TRUE(holds_alternative<Terminal>(symbols(h)[0]));
    ASSERT_TRUE(holds_alternative<NonTerminal>(symbols(h)[1]));
    ASSERT_EQ(2, symbols(h).size());

    // Make sure symbols() still reports as expected when handle doesn't contain symbols (but an action).
    h.clear();
    h.emplace_back(Action { "a1" });
    auto syms = symbols(h);
    EXPECT_EQ(0, syms.size());
    EXPECT_TRUE(syms.begin() == syms.end());
    for (const Symbol sym: syms)
        REPORT_ERROR("Should have never been invoked");
}

TEST(cfg_Grammar, missing_production)
{
    BufferedReport report;
    Grammar grammar = GrammarParser(GrammarLexer { "Start ::= Expr;" }, &report).parse();
    ASSERT_TRUE(report.containsFailures());
    // TODO: assert actgual error message
}

TEST(cfg_Grammar, right_recursive)
{
    ConsoleReport report;
    Grammar grammar = GrammarParser(
                          GrammarLexer {
                              R"(`token {
		   `  NUMBER ::= [0-9]+
		   `}
		   `Start     ::= Expr;
		   `Expr      ::= Term Expr_;
		   `Expr_     ::= '+' Term Expr_
		   `            | ;
		   `Term      ::= Factor Term_;
		   `Term_     ::= '*' Factor Term_
		   `            | ;
		   `Factor    ::= '(' Expr ')'
		   `            | NUMBER
		   `            ;
		   `)"_multiline },
                          &report)
                          .parse();

    ASSERT_FALSE(report.containsFailures());
    grammar.finalize();

    log("Grammar:");
    log(grammar.dump());

    ASSERT_EQ("\"(\"", fmt::format("{}", grammar.firstOf(Terminal { "(" })));
    ASSERT_EQ("\")\"", fmt::format("{}", grammar.firstOf(Terminal { ")" })));
    ASSERT_EQ("\"+\"", fmt::format("{}", grammar.firstOf(Terminal { "+" })));
    ASSERT_EQ("\"*\"", fmt::format("{}", grammar.firstOf(Terminal { "*" })));
    ASSERT_EQ(
        "EOF",
        fmt::format(
            "{}", grammar.firstOf(Terminal { regular::Rule { 0, 0, 0, { "INITIAL" }, "EOF", "<<EOF>>" } })));

    // ASSERT_EQ(2, metadata.epsilon.size());
    // ASSERT_TRUE(metadata.epsilon.find(NonTerminal{"Expr_"}) != metadata.epsilon.end());
    // ASSERT_TRUE(metadata.epsilon.find(NonTerminal{"Term_"}) != metadata.epsilon.end());

    ASSERT_EQ("", fmt::format("{}", grammar.followOf(NonTerminal { "Start" })));
    ASSERT_EQ("\")\", EOF", fmt::format("{}", grammar.followOf(NonTerminal { "Expr" })));
    ASSERT_EQ("\")\", EOF", fmt::format("{}", grammar.followOf(NonTerminal { "Expr_" })));
    ASSERT_EQ("\")\", \"+\", EOF", fmt::format("{}", grammar.followOf(NonTerminal { "Term" })));
    ASSERT_EQ("\")\", \"+\", EOF", fmt::format("{}", grammar.followOf(NonTerminal { "Term_" })));
    ASSERT_EQ("\")\", \"*\", \"+\", EOF", fmt::format("{}", grammar.followOf(NonTerminal { "Factor" })));
}

TEST(cfg_Grammar, with_complex_tokens)
{
    ConsoleReport report;
    Grammar grammar = GrammarParser(
                          GrammarLexer {
                              R"(`token {
		   `  NUMBER ::= [0-9]+
		   `}
		   `
		   `Start     ::= Expr;
		   `Expr      ::= Term Expr_;
		   `Expr_     ::= '+' Term Expr_
		   `            | ;
		   `Term      ::= Factor Term_;
		   `Term_     ::= '*' Factor Term_
		   `            | ;
		   `Factor    ::= '(' Expr ')'
		   `            | NUMBER
		   `            ;
		   `)"_multiline },
                          &report)
                          .parse();

    ASSERT_FALSE(report.containsFailures());
    grammar.finalize();

    log("Grammar:");
    log(grammar.dump());

    ASSERT_EQ(9, grammar.productions.size());

    ASSERT_EQ(1, symbols(grammar.productions.back().handle).size());
    // ASSERT_EQ("[0-9]+", get<Terminal>(grammar.productions.back().handle.symbols.front()).pattern());
    ASSERT_TRUE(holds_alternative<Terminal>(symbols(grammar.productions[8].handle)[0]));
    ASSERT_TRUE(
        holds_alternative<regular::Rule>(get<Terminal>(symbols(grammar.productions[8].handle)[0]).literal));
    ASSERT_EQ("[0-9]+",
              get<regular::Rule>(get<Terminal>(symbols(grammar.productions[8].handle)[0]).literal).pattern);
}

TEST(cfg_Grammar, left_recursive)
{
    ConsoleReport report;
    Grammar grammar = GrammarParser(
                          GrammarLexer {
                              R"(`Start     ::= Expr "<<EOF>>";
		   `Expr      ::= Expr '+' Term
		   `            | Term;
		   `Term      ::= Term '*' Factor
		   `            | Factor;
		   `Factor    ::= "NUMBER"
		   `            | '(' Expr ')'
		   `            ;
		   `)"_multiline },
                          &report)
                          .parse();

    ASSERT_FALSE(report.containsFailures());
    grammar.finalize();

    log("Grammar:");
    log(grammar.dump());

    ASSERT_EQ("\"(\"", fmt::format("{}", grammar.firstOf(Terminal { "(" })));
    ASSERT_EQ("\")\"", fmt::format("{}", grammar.firstOf(Terminal { ")" })));
    ASSERT_EQ("\"+\"", fmt::format("{}", grammar.firstOf(Terminal { "+" })));
    ASSERT_EQ("\"*\"", fmt::format("{}", grammar.firstOf(Terminal { "*" })));
    ASSERT_EQ("\"<<EOF>>\"", fmt::format("{}", grammar.firstOf(Terminal { "<<EOF>>" })));

    // TODO ASSERT_EQ(0, metadata.epsilon.size());

    ASSERT_EQ("", fmt::format("{}", grammar.followOf(NonTerminal { "Start" })));
    ASSERT_EQ("\")\", \"+\", \"<<EOF>>\"", fmt::format("{}", grammar.followOf(NonTerminal { "Expr" })));
    ASSERT_EQ("\")\", \"*\", \"+\", \"<<EOF>>\"",
              fmt::format("{}", grammar.followOf(NonTerminal { "Term" })));
    ASSERT_EQ("\")\", \"*\", \"+\", \"<<EOF>>\"",
              fmt::format("{}", grammar.followOf(NonTerminal { "Factor" })));
}

// vim:ts=4:sw=4:noet
