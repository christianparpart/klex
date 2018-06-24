// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/util/testing.h>
#include <klex/Compiler.h>
#include <klex/Lexer.h>

using namespace klex;

enum class LookaheadToken { Eof = 1, ABBA, AB_CD, CDEF };
namespace fmt { // it sucks that I've to specify that here
  template<>
  struct formatter<LookaheadToken> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const LookaheadToken& v, FormatContext &ctx) {
      switch (v) {
        case LookaheadToken::Eof: return format_to(ctx.begin(), "Eof");
        case LookaheadToken::ABBA: return format_to(ctx.begin(), "abba");
        case LookaheadToken::AB_CD: return format_to(ctx.begin(), "ab/cd");
        case LookaheadToken::CDEF: return format_to(ctx.begin(), "cdef");
        default: abort();
      }
    }
  };
}

TEST(Lexer, lookahead) {
  klex::Compiler cc;
  cc.parse(std::make_unique<std::stringstream>(R"(
    Space(ignore) ::= [\s\t]+
    Eof           ::= <<EOF>>
    ABBA          ::= abba
    AB_CD         ::= ab/cd
    CDEF          ::= cdef
  )"));
  Lexer<LookaheadToken> lexer { cc.compile(), std::make_unique<std::stringstream>("abba abcd cdef") };

  ASSERT_EQ(LookaheadToken::ABBA, lexer.recognize());
  ASSERT_EQ(LookaheadToken::AB_CD, lexer.recognize());
  ASSERT_EQ(LookaheadToken::CDEF, lexer.recognize());
  ASSERT_EQ(LookaheadToken::Eof, lexer.recognize());
}
