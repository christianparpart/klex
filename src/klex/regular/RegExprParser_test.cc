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

using namespace klex::regular;

TEST(RegExprParser, namedCharacterClass_graph)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:graph:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("!-~", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_digit)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:digit:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("0-9", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_alnum)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:alnum:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("0-9A-Za-z", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_alpha)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:alpha:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("A-Za-z", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_blank)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:blank:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\t\\s", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_cntrl)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:cntrl:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\0-\\x1f\\x7f", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_print)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:print:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	// logf("value: <{}>", e->value().to_string());
	EXPECT_EQ("\\s-~", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_punct)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:punct:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	// logf("value: <{}>", e->value().to_string());
	EXPECT_EQ("!-/:-@[-`{-~", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_space)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:space:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\0\\t-\\r", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_unknown)
{
	EXPECT_THROW(RegExprParser{}.parse("[[:unknown:]]"), RegExprParser::UnexpectedToken);
}

TEST(RegExprParser, namedCharacterClass_upper)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:upper:]]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("A-Z", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_mixed)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:lower:]0-9]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("0-9a-z", e->value().to_string());
}

TEST(RegExprParser, characterClass_complement)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[^\\n]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_TRUE(e->value().isDot());
	EXPECT_EQ(".", e->value().to_string());
}

TEST(RegExprParser, escapeSequences_invalid)
{
	EXPECT_THROW(RegExprParser{}.parse("[\\z]"), RegExprParser::UnexpectedToken);
}

TEST(RegExprParser, escapeSequences_abfnrstv)
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

TEST(RegExprParser, newline)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("\n");
	auto e = dynamic_cast<const CharacterExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ('\n', e->value());
}

TEST(RegExprParser, escapeSequences_hex)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[\\x20]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\s", e->value().to_string());

	EXPECT_THROW(RegExprParser{}.parse("[\\xZZ]"), RegExprParser::UnexpectedToken);
	EXPECT_THROW(RegExprParser{}.parse("[\\xAZ]"), RegExprParser::UnexpectedToken);
	EXPECT_THROW(RegExprParser{}.parse("[\\xZA]"), RegExprParser::UnexpectedToken);
}

TEST(RegExprParser, escapeSequences_nul)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[\\0]");
	auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("\\0", e->value().to_string());
}

TEST(RegExprParser, escapeSequences_octal)
{
	// with leading zero
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[\\040]");
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

TEST(RegExprParser, doubleQuote)
{
	// as concatenation character
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse(R"(\")");
	auto e = dynamic_cast<const CharacterExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ('"', e->value());

	// as character class
	re = RegExprParser{}.parse(R"([\"])");
	auto c = dynamic_cast<const CharacterClassExpr*>(re.get());
	ASSERT_TRUE(c != nullptr);
	EXPECT_EQ(R"(")", c->value().to_string());
}

TEST(RegExprParser, dot)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse(".");
	auto e = dynamic_cast<const DotExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ(".", e->to_string());
}

TEST(RegExprParser, optional)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("a?");
	auto e = dynamic_cast<const ClosureExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("a?", e->to_string());
}

TEST(RegExprParser, bol)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("^a");
	auto cat = dynamic_cast<const ConcatenationExpr*>(re.get());
	ASSERT_TRUE(cat != nullptr);

	auto bol = dynamic_cast<const BeginOfLineExpr*>(cat->leftExpr());
	ASSERT_TRUE(bol != nullptr);
	EXPECT_EQ("^", bol->to_string());
}

TEST(RegExprParser, eol)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("a$");
	auto cat = dynamic_cast<const ConcatenationExpr*>(re.get());
	ASSERT_TRUE(cat != nullptr);

	auto eol = dynamic_cast<const EndOfLineExpr*>(cat->rightExpr());
	ASSERT_TRUE(eol != nullptr);
	EXPECT_EQ("a$", re->to_string());
}

TEST(RegExprParser, eof)
{
	std::unique_ptr<RegExpr> re = RegExprParser{}.parse("<<EOF>>");
	auto eof = dynamic_cast<const EndOfFileExpr*>(re.get());
	ASSERT_TRUE(eof != nullptr);
	EXPECT_EQ("<<EOF>>", re->to_string());
}

TEST(RegExprParser, alternation)
{
	EXPECT_EQ("a|b", RegExprParser{}.parse("a|b")->to_string());
	EXPECT_EQ("(a|b)c", RegExprParser{}.parse("(a|b)c")->to_string());
	EXPECT_EQ("a(b|c)", RegExprParser{}.parse("a(b|c)")->to_string());
}

TEST(RegExprParser, lookahead)
{
	auto re = RegExprParser{}.parse("ab/cd");
	auto e = dynamic_cast<const LookAheadExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ("ab/cd", e->to_string());
	EXPECT_EQ("(a/b)|b", RegExprParser{}.parse("(a/b)|b")->to_string());
	EXPECT_EQ("a|(b/c)", RegExprParser{}.parse("a|(b/c)")->to_string());
}

TEST(RegExprParser, closure)
{
	auto re = RegExprParser{}.parse("(abc)*");
	auto e = dynamic_cast<const ClosureExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ(0, e->minimumOccurrences());
	EXPECT_EQ(std::numeric_limits<unsigned>::max(), e->maximumOccurrences());
	EXPECT_EQ("(abc)*", re->to_string());
}

TEST(RegExprParser, positive)
{
	auto re = RegExprParser{}.parse("(abc)+");
	auto e = dynamic_cast<const ClosureExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ(1, e->minimumOccurrences());
	EXPECT_EQ(std::numeric_limits<unsigned>::max(), e->maximumOccurrences());
	EXPECT_EQ("(abc)+", re->to_string());
}

TEST(RegExprParser, closure_range)
{
	auto re = RegExprParser{}.parse("a{2,4}");
	auto e = dynamic_cast<const ClosureExpr*>(re.get());
	ASSERT_TRUE(e != nullptr);
	EXPECT_EQ(2, e->minimumOccurrences());
	EXPECT_EQ(4, e->maximumOccurrences());
	EXPECT_EQ("a{2,4}", re->to_string());
}

TEST(RegExprParser, empty)
{
	auto re = RegExprParser{}.parse("(a|)");
	EXPECT_EQ("a|", re->to_string());  // grouping '(' & ')' is not preserved as node in the parse tree.
}

TEST(RegExprParser, UnexpectedToken_grouping)
{
	EXPECT_THROW(RegExprParser{}.parse("(a"), RegExprParser::UnexpectedToken);
}

TEST(RegExprParser, UnexpectedToken_literal)
{
	EXPECT_THROW(RegExprParser{}.parse("\"a"), RegExprParser::UnexpectedToken);
}
