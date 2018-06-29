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
  // LexerDef lexerDef = cc.compile();
  // logf("LexerDef:\n{}", lexerDef.to_string());
  // Lexer<LookaheadToken, StateId, true> lexer { lexerDef,
  //                                              std::make_unique<std::stringstream>("abba abcd cdef"),
  //                                              [this](const std::string& msg) { log(msg); } };

  ASSERT_EQ(LookaheadToken::ABBA, lexer.recognize());
  ASSERT_EQ(LookaheadToken::AB_CD, lexer.recognize());
  ASSERT_EQ(LookaheadToken::CDEF, lexer.recognize());
  ASSERT_EQ(LookaheadToken::Eof, lexer.recognize());
}

TEST(Lexer, match_eol) {
  klex::Compiler cc;
  cc.parse(std::make_unique<std::stringstream>(RULES));

  Lexer<LookaheadToken> lexer { cc.compile(), std::make_unique<std::stringstream>("abba eol\nabba") };
  // LexerDef lexerDef = cc.compile();
  // logf("LexerDef:\n{}", lexerDef.to_string());
  // Lexer<LookaheadToken, StateId, true> lexer { lexerDef,
  //                                              std::make_unique<std::stringstream>("abba eol\nabba"),
  //                                              [this](const std::string& msg) { log(msg); } };

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

  Lexer<int> lexer { cc.compile(),
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

enum class RealWorld { Eof = 1, IPv4, IPv6 };
namespace fmt { // it sucks that I've to specify that here
  template<>
  struct formatter<RealWorld> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const RealWorld& v, FormatContext &ctx) {
      switch (v) {
        case RealWorld::Eof: return format_to(ctx.begin(), "Eof");
        case RealWorld::IPv4: return format_to(ctx.begin(), "IPv4");
        case RealWorld::IPv6: return format_to(ctx.begin(), "IPv6");
        default: return format_to(ctx.begin(), "<{}>", static_cast<unsigned>(v));
      }
    }
  };
}

TEST(Lexer, realworld_ipv6) {
  Compiler cc;
  cc.parse(std::make_unique<std::stringstream>(R"(
      Spacing(ignore)   ::= [\s\t\n]+
      Eof               ::= <<EOF>>

      IPv4Octet(ref)    ::= [0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5]
      IPv4(ref)         ::= {IPv4Octet}(\.{IPv4Octet}){3}
      IPv4Literal       ::= {IPv4}

      ipv6Part(ref)     ::= [[:xdigit:]]{1,4}
      IPv6              ::= {ipv6Part}(:{ipv6Part}){7,7}
                          | ({ipv6Part}:){1,7}:
                          | :(:{ipv6Part}){1,7}
                          | ::
                          | ({ipv6Part}:){1}(:{ipv6Part}){0,6}
                          | ({ipv6Part}:){2}(:{ipv6Part}){0,5}
                          | ({ipv6Part}:){3}(:{ipv6Part}){0,4}
                          | ({ipv6Part}:){4}(:{ipv6Part}){0,3}
                          | ({ipv6Part}:){5}(:{ipv6Part}){0,2}
                          | ({ipv6Part}:){6}(:{ipv6Part}){0,1}
                          | ::[fF]{4}:{IPv4}
  )"));

  static const std::string TEXT = R"(0:0:0:0:0:0:0:0
                                     1234:5678:90ab:cdef:aaaa:bbbb:cccc:dddd
                                     2001:0db8:85a3:0000:0000:8a2e:0370:7334
                                     1234:5678::
                                     0::
                                     ::0
                                     ::
                                     1::3:4:5:6:7:8
                                     1::4:5:6:7:8
                                     1::5:6:7:8
                                     1::8
                                     1:2::4:5:6:7:8
                                     1:2::5:6:7:8
                                     1:2::8
                                     ::ffff:127.0.0.1
                                     ::ffff:c000:0280
                                     )";

  LexerDef lexerDef = cc.compile();
#if 1
  Lexer<RealWorld, StateId, true> lexer { lexerDef,
                                          std::make_unique<std::stringstream>(TEXT),
                                          [this](const std::string& msg) { log(msg); } };
#else
  Lexer<RealWorld> lexer { cc.compile(), std::make_unique<std::stringstream>(TEXT), [](auto) {} };
#endif

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("0:0:0:0:0:0:0:0", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("1234:5678:90ab:cdef:aaaa:bbbb:cccc:dddd", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("2001:0db8:85a3:0000:0000:8a2e:0370:7334", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("1234:5678::", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("0::", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("::0", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("::", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("1::3:4:5:6:7:8", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("1::4:5:6:7:8", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("1::5:6:7:8", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("1::8", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("1:2::4:5:6:7:8", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("1:2::5:6:7:8", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("1:2::8", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("::ffff:127.0.0.1", lexer.word());

  ASSERT_EQ(RealWorld::IPv6, lexer.recognize());
  ASSERT_EQ("::ffff:c000:0280", lexer.word());

  ASSERT_EQ(RealWorld::Eof, lexer.recognize());
}
