// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/util/testing.h>
#include <klex/Compiler.h>
#include <klex/DFA.h>
#include <klex/DotWriter.h>
#include <klex/Lexer.h>
#include <klex/MultiDFA.h>

using namespace klex;

/* FEATURE UNITTEST CHECKLIST:
 *
 * - [ ] concatenation
 * - [ ] alternation
 * - [ ] {n}
 * - [ ] {m,n}
 * - [ ] {m,}
 * - [ ] ?
 * - [ ] character class, [a-z], [a-z0-9]
 * - [ ] character class by name, such as [[:upper:]]
 * - [ ] inverted character class, [^a-z], [^a-z0-9]
 * - [ ] generic lookahead r/s
 * - [ ] EOL lookahead r$
 * - [ ] BOL lookbehind ^r
 */

const std::string RULES = R"(
  Space(ignore) ::= [\s\t\n]+
  Eof           ::= <<EOF>>
  ABBA          ::= abba
  AB_CD         ::= ab/cd
  CDEF          ::= cdef
  EOL_LF        ::= eol$
)";

enum class LookaheadToken { Eof = 1, ABBA, AB_CD, CDEF, EOL_LF };
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
        case LookaheadToken::EOL_LF: return format_to(ctx.begin(), "eol$");
        default: return format_to(ctx.begin(), "<{}>", static_cast<unsigned>(v));
      }
    }
  };
}

TEST(Lexer, lookahead) {
  klex::Compiler cc;
  cc.parse(std::make_unique<std::stringstream>(RULES));
  Lexer<LookaheadToken> lexer { cc.compile(), std::make_unique<std::stringstream>("abba abcd cdef") };

  ASSERT_EQ(LookaheadToken::ABBA, lexer.recognize());
  ASSERT_EQ(LookaheadToken::AB_CD, lexer.recognize());
  ASSERT_EQ(LookaheadToken::CDEF, lexer.recognize());
  ASSERT_EQ(LookaheadToken::Eof, lexer.recognize());
}

TEST(Lexer, match_eol) {
  klex::Compiler cc;
  cc.parse(std::make_unique<std::stringstream>(RULES));

  LexerDef lexerDef = cc.compile();
  logf("LexerDef:\n{}", lexerDef.to_string());
  Lexer<LookaheadToken, true> lexer { lexerDef,
                                      std::make_unique<std::stringstream>("abba eol\nabba"),
                                      [this](const std::string& msg) { log(msg); } };

  ASSERT_EQ(LookaheadToken::ABBA, lexer.recognize());
  ASSERT_EQ(0, lexer.offset().first);
  ASSERT_EQ(4, lexer.offset().second);

  ASSERT_EQ(LookaheadToken::EOL_LF, lexer.recognize());
  ASSERT_EQ(5, lexer.offset().first);
  ASSERT_EQ(9, lexer.offset().second);

  ASSERT_EQ(LookaheadToken::ABBA, lexer.recognize());
  ASSERT_EQ(12, lexer.offset().second);

  ASSERT_EQ(LookaheadToken::Eof, lexer.recognize());
}

TEST(Lexer, empty_alt) {
  Compiler cc;
  cc.parse(std::make_unique<std::stringstream>(R"(
      Spacing(ignore) ::= [\s\t\n]+
      Test            ::= aa(bb|)
      Eof             ::= <<EOF>>
  )"));

  Lexer<Tag> lexer { cc.compile(),
                     std::make_unique<std::stringstream>("aabb aa aabb") };

  ASSERT_EQ(1, lexer.recognize());
  ASSERT_EQ(1, lexer.recognize());
  ASSERT_EQ(1, lexer.recognize());
  ASSERT_EQ(2, lexer.recognize());
}

TEST(Lexer, realworld_ipv4) {
  Compiler cc;
  cc.parse(std::make_unique<std::stringstream>(R"(
      Spacing(ignore)   ::= [\s\t\n]+
      Eof               ::= <<EOF>>
      IPv4Octet(ref)    ::= [0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5]
      IPv4(ref)         ::= {IPv4Octet}(\.{IPv4Octet}){3}
      IPv4Literal       ::= {IPv4}
  )"));

  // DotWriter dw{std::cerr};
  // cc.compileMinimalDFA().visit(dw);

  LexerDef lexerDef = cc.compile();
  logf("LexerDef:\n{}", lexerDef.to_string());
  Lexer<int, true> lexer { lexerDef,
                           std::make_unique<std::stringstream>(
                               R"(0.0.0.0 4.2.2.1 10.10.40.199 255.255.255.255)"),
                           [this](const std::string& msg) { log(msg); } };

  ASSERT_EQ(2, lexer.recognize());
  ASSERT_EQ("0.0.0.0", lexer.word());

  ASSERT_EQ(2, lexer.recognize());
  ASSERT_EQ("4.2.2.1", lexer.word());

  ASSERT_EQ(2, lexer.recognize());
  ASSERT_EQ("10.10.40.199", lexer.word());

  ASSERT_EQ(2, lexer.recognize());
  ASSERT_EQ("255.255.255.255", lexer.word());

  ASSERT_EQ(1, lexer.recognize());
}
