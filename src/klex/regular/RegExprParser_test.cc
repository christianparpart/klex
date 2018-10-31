// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/regular/RegExpr.h>
#include <klex/regular/RegExprParser.h>
#include <klex/util/testing.h>
#include <memory>

using namespace std;
using namespace klex::regular;

TEST(regular_RegExprParser, namedCharacterClass_graph)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:graph:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("!-~", e->value().to_string());
}

TEST(regular_RegExprParser, whitespaces_concatination)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("a b");
	ASSERT_TRUE(dynamic_cast<const ConcatenationExpr*>(re.get()) != nullptr);
	EXPECT_EQ("ab", re->to_string());
}

TEST(regular_RegExprParser, whitespaces_alternation)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("a | b");
	ASSERT_TRUE(dynamic_cast<const ConcatenationExpr*>(re.get()) != nullptr);
	EXPECT_EQ("a|b", re->to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_digit)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:digit:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("0-9", e->value().to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_alnum)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:alnum:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("0-9A-Za-z", e->value().to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_alpha)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:alpha:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("A-Za-z", e->value().to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_blank)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:blank:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\t\\s", e->value().to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_cntrl)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:cntrl:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\0-\\x1f\\x7f", e->value().to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_print)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:print:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	// logf("value: <{}>", e->value().to_string());
	EXPECT_EQ("\\s-~", e->value().to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_punct)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:punct:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	// logf("value: <{}>", e->value().to_string());
	EXPECT_EQ("!-/:-@[-`{-~", e->value().to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_space)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:space:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\0\\t-\\r", e->value().to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_unknown)
{
	EXPECT_THROW(RegExprParser{}.parse("[[:unknown:]]"), RegExprParser::UnexpectedToken);
}

TEST(regular_RegExprParser, namedCharacterClass_upper)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:upper:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("A-Z", e->value().to_string());
}

TEST(regular_RegExprParser, namedCharacterClass_mixed)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:lower:]0-9]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("0-9a-z", e->value().to_string());
}

TEST(regular_RegExprParser, characterClass_complement)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[^\\n]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_TRUE(e->value().isDot());
	EXPECT_EQ(".", e->value().to_string());
}

TEST(regular_RegExprParser, escapeSequences_invalid)
{
	EXPECT_THROW(RegExprParser{}.parse("[\\z]"), RegExprParser::UnexpectedToken);
}

TEST(regular_RegExprParser, escapeSequences_abfnrstv)
{
	EXPECT_EQ("\\a", RegExprParser{}.parse("[\\a]")->to_string());
	EXPECT_EQ("\\b", RegExprParser{}.parse("[\\b]")->to_string());
	EXPECT_EQ("\\f", RegExprParser{}.parse("[\\f]")->to_string());
	EXPECT_EQ("\\n", RegExprParser{}.parse("[\\n]")->to_string());
	EXPECT_EQ("\\r", RegExprParser{}.parse("[\\r]")->to_string());
	EXPECT_EQ("\\s", RegExprParser{}.parse("[\\s]")->to_string());
	EXPECT_EQ("\\t", RegExprParser{}.parse("[\\t]")->to_string());
	EXPECT_EQ("\\v", RegExprParser{}.parse("[\\v]")->to_string());
}

TEST(regular_RegExprParser, newline)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("\n");
	auto e = dynamic_cast<const CharacterExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ('\n', e->value());
}

TEST(regular_RegExprParser, escapeSequences_hex)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[\\x20]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\s", e->value().to_string());

	EXPECT_THROW(RegExprParser{}.parse("[\\xZZ]"), RegExprParser::UnexpectedToken);
	EXPECT_THROW(RegExprParser{}.parse("[\\xAZ]"), RegExprParser::UnexpectedToken);
	EXPECT_THROW(RegExprParser{}.parse("[\\xZA]"), RegExprParser::UnexpectedToken);
}

TEST(regular_RegExprParser, escapeSequences_nul)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[\\0]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\0", e->value().to_string());
}

TEST(regular_RegExprParser, escapeSequences_octal)
{
	// with leading zero
	unique_ptr<RegExpr> re = RegExprParser{}.parse("[\\040]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\s", e->value().to_string());

	// with leading non-zero
	re = RegExprParser{}.parse("[\\172]");
	e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("z", e->value().to_string());

	// invalids
	EXPECT_THROW(RegExprParser{}.parse("[\\822]"), RegExprParser::UnexpectedToken);
	EXPECT_THROW(RegExprParser{}.parse("[\\282]"), RegExprParser::UnexpectedToken);
	EXPECT_THROW(RegExprParser{}.parse("[\\228]"), RegExprParser::UnexpectedToken);
	EXPECT_THROW(RegExprParser{}.parse("[\\082]"), RegExprParser::UnexpectedToken);
	EXPECT_THROW(RegExprParser{}.parse("[\\028]"), RegExprParser::UnexpectedToken);
}

TEST(regular_RegExprParser, doubleQuote)
{
	// as concatenation character
	unique_ptr<RegExpr> re = RegExprParser{}.parse(R"(\")");
	auto e = dynamic_cast<const CharacterExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ('"', e->value());

	// as character class
	re = RegExprParser{}.parse(R"([\"])");
	auto c = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(c != nullptr);
	EXPECT_EQ(R"(")", c->value().to_string());
}

TEST(regular_RegExprParser, dot)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse(".");
	auto e = dynamic_cast<const DotExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ(".", e->to_string());
}

TEST(regular_RegExprParser, optional)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("a?");
	auto e = dynamic_cast<const ClosureExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("a?", e->to_string());
}

TEST(regular_RegExprParser, bol)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("^a");
	auto cat = dynamic_cast<const ConcatenationExpr*>(re.get());
	ASSERT_TRUE(cat != nullptr);

	auto bol = dynamic_cast<const BeginOfLineExpr*>(cat->leftExpr());
	ASSERT_TRUE(bol != nullptr);
	EXPECT_EQ("^", bol->to_string());
}

TEST(regular_RegExprParser, eol)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("a$");
	auto cat = dynamic_cast<const ConcatenationExpr*>(re.get());
	ASSERT_TRUE(cat != nullptr);

	auto eol = dynamic_cast<const EndOfLineExpr*>(cat->rightExpr());
	ASSERT_TRUE(eol != nullptr);
	EXPECT_EQ("a$", re->to_string());
}

TEST(regular_RegExprParser, eof)
{
	unique_ptr<RegExpr> re = RegExprParser{}.parse("<<EOF>>");
	auto eof = dynamic_cast<const EndOfFileExpr*>(re.get());
	ASSERT_TRUE(eof != nullptr);
	EXPECT_EQ("<<EOF>>", re->to_string());
}

TEST(regular_RegExprParser, alternation)
{
	EXPECT_EQ("a|b", RegExprParser{}.parse("a|b")->to_string());
	EXPECT_EQ("(a|b)c", RegExprParser{}.parse("(a|b)c")->to_string());
	EXPECT_EQ("a(b|c)", RegExprParser{}.parse("a(b|c)")->to_string());
}

TEST(regular_RegExprParser, lookahead)
{
	auto re = RegExprParser{}.parse("ab/cd");
	auto e = dynamic_cast<const LookAheadExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("ab/cd", e->to_string());
	EXPECT_EQ("(a/b)|b", RegExprParser{}.parse("(a/b)|b")->to_string());
	EXPECT_EQ("a|(b/c)", RegExprParser{}.parse("a|(b/c)")->to_string());
}

TEST(regular_RegExprParser, closure)
{
	auto re = RegExprParser{}.parse("(abc)*");
	auto e = dynamic_cast<const ClosureExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ(0, e->minimumOccurrences());
	EXPECT_EQ(numeric_limits<unsigned>::max(), e->maximumOccurrences());
	EXPECT_EQ("(abc)*", re->to_string());
}

TEST(regular_RegExprParser, positive)
{
	auto re = RegExprParser{}.parse("(abc)+");
	auto e = dynamic_cast<const ClosureExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ(1, e->minimumOccurrences());
	EXPECT_EQ(numeric_limits<unsigned>::max(), e->maximumOccurrences());
	EXPECT_EQ("(abc)+", re->to_string());
}

TEST(regular_RegExprParser, closure_range)
{
	auto re = RegExprParser{}.parse("a{2,4}");
	auto e = dynamic_cast<const ClosureExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ(2, e->minimumOccurrences());
	EXPECT_EQ(4, e->maximumOccurrences());
	EXPECT_EQ("a{2,4}", re->to_string());
}

TEST(regular_RegExprParser, empty)
{
	auto re = RegExprParser{}.parse("(a|)");
	EXPECT_EQ("a|", re->to_string());  // grouping '(' & ')' is not preserved as node in the parse tree.
}

TEST(regular_RegExprParser, UnexpectedToken_grouping)
{
	EXPECT_THROW(RegExprParser{}.parse("(a"), RegExprParser::UnexpectedToken);
}

TEST(regular_RegExprParser, UnexpectedToken_literal)
{
	EXPECT_THROW(RegExprParser{}.parse("\"a"), RegExprParser::UnexpectedToken);
}
