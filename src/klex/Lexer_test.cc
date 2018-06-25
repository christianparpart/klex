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
  EOL_LF        ::= eol/\n
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
        case LookaheadToken::EOL_LF: return format_to(ctx.begin(), "eol/\\n");
        default: abort();
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

// TODO
TEST(Lexer, match_eol) {
  klex::Compiler cc;
  cc.parse(std::make_unique<std::stringstream>(RULES));
  LexerDef lexerDef = cc.compile();
  logf("LexerDef:\n{}", lexerDef.to_string());
  Lexer<LookaheadToken, true> lexer { cc.compile(),
                                      std::make_unique<std::stringstream>("abba eol\nabba"),
                                      [this](const std::string& msg) { log(msg); } };

  for (LookaheadToken t = lexer.recognizeOne(); t != LookaheadToken::Eof; t = lexer.recognizeOne()) {
    logf("token: {}", t);
  }

  // ASSERT_EQ(LookaheadToken::ABBA, lexer.recognizeOne());
  // ASSERT_EQ(LookaheadToken::EOL_LF, lexer.recognizeOne());
  // ASSERT_EQ(LookaheadToken::ABBA, lexer.recognizeOne());
  // ASSERT_EQ(LookaheadToken::Eof, lexer.recognizeOne());
}