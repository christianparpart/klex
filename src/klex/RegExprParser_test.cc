// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/util/testing.h>
#include <klex/RegExprParser.h>
#include <klex/RegExpr.h>
#include <memory>

using namespace klex;

TEST(RegExprParser, namedCharacterClass) {
  std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:digit:]]");
  auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
  ASSERT_TRUE(e != nullptr);
  EXPECT_EQ("0-9", e->value().to_string());
}

TEST(RegExprParser, namedCharacterClass_mixed) {
  std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[[:lower:]0-9]");
  auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
  ASSERT_TRUE(e != nullptr);
  EXPECT_EQ("0-9a-z", e->value().to_string());
}

TEST(RegExprParser, escapeSequences) {
  std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[\\v]");
  auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
  ASSERT_TRUE(e != nullptr);
  EXPECT_EQ("\\v", e->value().to_string());
}

TEST(RegExprParser, escapeSequences_hex) {
  std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[\\x20]");
  auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
  ASSERT_TRUE(e != nullptr);
  EXPECT_EQ("\\s", e->value().to_string());
}

TEST(RegExprParser, escapeSequences_nul) {
  std::unique_ptr<RegExpr> re = RegExprParser{}.parse("[\\0]");
  auto e = dynamic_cast<const CharacterClassExpr*>(re.get());
  ASSERT_TRUE(e != nullptr);
  EXPECT_EQ("\\0", e->value().to_string());
}

TEST(RegExprParser, escapeSequences_octal) {
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
}
