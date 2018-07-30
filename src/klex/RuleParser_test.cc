// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/util/testing.h>
#include <klex/RuleParser.h>
#include <memory>
#include <sstream>

using namespace klex;

TEST(RuleParser, simple) {
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    main ::= blah
  )")};
  klex::RuleList rules = rp.parseRules();
  ASSERT_EQ(1, rules.size());
  EXPECT_EQ("blah", rules[0].pattern);
}

TEST(RuleParser, simple_trailing_spaces) {
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    main ::= blah  
  )")};
  klex::RuleList rules = rp.parseRules();
  ASSERT_EQ(1, rules.size());
  EXPECT_EQ("blah", rules[0].pattern);
}

TEST(RuleParser, quotedPattern) {
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    main ::= "blah"
  )")};
  klex::RuleList rules = rp.parseRules();
  ASSERT_EQ(1, rules.size());
  EXPECT_EQ("\"blah\"", rules[0].pattern);
}

TEST(RuleParser, multiQuotedPattern) {
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    rule ::= "b"la"h"
  )")};
  klex::RuleList rules = rp.parseRules();
  ASSERT_EQ(1, rules.size());
  EXPECT_EQ(R"("b"la"h")", rules[0].pattern);
}

TEST(RuleParser, doubleQuote) {
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    rule ::= \"
  )")};
  klex::RuleList rules = rp.parseRules();
  ASSERT_EQ(1, rules.size());
  EXPECT_EQ(R"(\")", rules[0].pattern);
}

TEST(RuleParser, spaceRule) {
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    rule ::= [ \n\t]+
  )")};
  klex::RuleList rules = rp.parseRules();
  ASSERT_EQ(1, rules.size());
  EXPECT_EQ(R"([ \n\t]+)", rules[0].pattern);
}

TEST(RuleParser, stringRule) {
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    rule ::= \"[^\"]*\"
  )")};
  klex::RuleList rules = rp.parseRules();
  ASSERT_EQ(1, rules.size());
  EXPECT_EQ(R"(\"[^\"]*\")", rules[0].pattern);
}

TEST(RuleParser, ref) {
  RuleParser rp{std::make_unique<std::stringstream>(R"(
    Foo(ref) ::= foo
    Bar(ref) ::= bar
    FooBar   ::= {Foo}_{Bar}
  )")};
  RuleList rules = rp.parseRules();
  ASSERT_EQ(1, rules.size());
  EXPECT_EQ("(foo)_(bar)", rules[0].pattern);
}

TEST(RuleParser, ref_duplicated) {
  RuleParser rp{std::make_unique<std::stringstream>(R"(
    Foo(ref) ::= foo
    Foo(ref) ::= bar
    FooBar   ::= {Foo}
  )")};
  EXPECT_THROW(rp.parseRules(), RuleParser::DuplicateRule);
}

TEST(RuleParser, multiline_alt) {
  RuleParser rp{std::make_unique<std::stringstream>(R"(
    Rule1       ::= foo
                  | bar
    Rule2(ref)  ::= fnord
                  | hard
    Rule3       ::= {Rule2}
                  | {Rule2}
  )")};
  RuleList rules = rp.parseRules();
  ASSERT_EQ(2, rules.size());
  EXPECT_EQ("foo|bar", rules[0].pattern);
  EXPECT_EQ("(fnord|hard)|(fnord|hard)", rules[1].pattern);
}

TEST(RuleParser, condition1) {
  RuleParser rp{std::make_unique<std::stringstream>(R"(
    <foo>Rule1    ::= foo
    <bar>Rule2    ::= bar
  )")};
  RuleList rules = rp.parseRules();

  ASSERT_EQ(2, rules.size());
  EXPECT_EQ("foo", rules[0].pattern);
  EXPECT_EQ("bar", rules[1].pattern);

  ASSERT_EQ(1, rules[0].conditions.size());
  EXPECT_EQ("foo", rules[0].conditions[0]);

  ASSERT_EQ(1, rules[1].conditions.size());
  EXPECT_EQ("bar", rules[1].conditions[0]);
}

TEST(RuleParser, condition2) {
  RuleParser rp{std::make_unique<std::stringstream>(R"(
    <foo>Rule1      ::= foo
    <foo,bar>Rule2  ::= bar
  )")};
  RuleList rules = rp.parseRules();

  ASSERT_EQ(2, rules.size());
  EXPECT_EQ("foo", rules[0].pattern);
  EXPECT_EQ("bar", rules[1].pattern);

  ASSERT_EQ(1, rules[0].conditions.size());
  EXPECT_EQ("foo", rules[0].conditions[0]);

  ASSERT_EQ(2, rules[1].conditions.size());
  // in sorted order
  EXPECT_EQ("bar", rules[1].conditions[0]);
  EXPECT_EQ("foo", rules[1].conditions[1]);
}

TEST(RuleParser, conditional_star) {
  RuleParser rp{std::make_unique<std::stringstream>(R"(
    Zero      ::= zero
    <one>One  ::= one
    <two>Two  ::= two
    <*>Tri    ::= tri
  )")};
  RuleList rules = rp.parseRules();

  ASSERT_EQ(4, rules.size());

  EXPECT_EQ("zero", rules[0].pattern);
  ASSERT_EQ(1, rules[0].conditions.size());
  EXPECT_EQ("INITIAL", rules[0].conditions[0]);

  EXPECT_EQ("one", rules[1].pattern);
  ASSERT_EQ(1, rules[1].conditions.size());
  EXPECT_EQ("one", rules[1].conditions[0]);

  EXPECT_EQ("two", rules[2].pattern);
  ASSERT_EQ(1, rules[2].conditions.size());
  EXPECT_EQ("two", rules[2].conditions[0]);

  EXPECT_EQ("tri", rules[3].pattern);
  ASSERT_EQ(3, rules[3].conditions.size());
  EXPECT_EQ("INITIAL", rules[3].conditions[0]);
  EXPECT_EQ("one", rules[3].conditions[1]);
  EXPECT_EQ("two", rules[3].conditions[2]);
}

TEST(RuleParser, grouped_conditions) {
  RuleParser rp{std::make_unique<std::stringstream>(R"(
    Rule1       ::= foo
    <blah> {
      Rule2     ::= bar
    }
  )")};
  RuleList rules = rp.parseRules();

  ASSERT_EQ(2, rules.size());
  EXPECT_EQ("foo", rules[0].pattern);
  EXPECT_EQ("bar", rules[1].pattern);

  ASSERT_EQ(1, rules[1].conditions.size());
  EXPECT_EQ("blah", rules[1].conditions[0]);
}

TEST(RuleParser, InvalidRefRuleWithConditions) {
  ASSERT_THROW(
      klex::RuleParser{std::make_unique<std::stringstream>("<cond>main(ref) ::= blah\n")}.parseRules(),
      RuleParser::InvalidRefRuleWithConditions);
}

TEST(RuleParser, InvalidRuleOption) {
  ASSERT_THROW(
      klex::RuleParser{std::make_unique<std::stringstream>("A(invalid) ::= a\n")}.parseRules(),
      RuleParser::InvalidRuleOption);
}

TEST(RuleParser, DuplicateRule) {
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    foo ::= abc
    foo ::= def
  )")};
  ASSERT_THROW(rp.parseRules(), RuleParser::DuplicateRule);
}

TEST(RuleParser, UnexpectedChar) {
  ASSERT_THROW(klex::RuleParser{std::make_unique<std::stringstream>("A :=")}.parseRules(), RuleParser::UnexpectedChar);
  ASSERT_THROW(klex::RuleParser{std::make_unique<std::stringstream>("<x A ::= a")}.parseRules(), RuleParser::UnexpectedChar);
  ASSERT_THROW(klex::RuleParser{std::make_unique<std::stringstream>("A ::= a")}.parseRules(), RuleParser::UnexpectedChar);
}

TEST(RuleParser, UnexpectedToken) {
  ASSERT_THROW(klex::RuleParser{std::make_unique<std::stringstream>("<x,y,> A ::= a")}.parseRules(), RuleParser::UnexpectedToken);
  ASSERT_THROW(klex::RuleParser{std::make_unique<std::stringstream>("<> A ::= a")}.parseRules(), RuleParser::UnexpectedToken);
  ASSERT_THROW(klex::RuleParser{std::make_unique<std::stringstream>(" ::= a")}.parseRules(), RuleParser::UnexpectedToken);
}
