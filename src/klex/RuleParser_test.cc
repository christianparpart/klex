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
