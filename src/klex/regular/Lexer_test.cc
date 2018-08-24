// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/regular/Compiler.h>
#include <klex/regular/DFA.h>
#include <klex/regular/DotWriter.h>
#include <klex/regular/Lexer.h>
#include <klex/regular/MultiDFA.h>
#include <klex/util/literals.h>
#include <klex/util/testing.h>

using namespace klex::regular;
using namespace klex::util::literals;

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
  CD            ::= cd
  CDEF          ::= cdef
  EOL_LF        ::= eol$
  XAnyLine      ::= x.*
)";

enum class LookaheadToken { Eof = 1, ABBA, AB_CD, CD, CDEF, EOL_LF, XAnyLine };
namespace fmt {  // it sucks that I've to specify that here
template <>
struct formatter<LookaheadToken> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const LookaheadToken& v, FormatContext& ctx)
	{
		switch (v)
		{
			case LookaheadToken::Eof:
				return format_to(ctx.begin(), "Eof");
			case LookaheadToken::ABBA:
				return format_to(ctx.begin(), "abba");
			case LookaheadToken::AB_CD:
				return format_to(ctx.begin(), "ab/cd");
			case LookaheadToken::CD:
				return format_to(ctx.begin(), "cd");
			case LookaheadToken::CDEF:
				return format_to(ctx.begin(), "cdef");
			case LookaheadToken::EOL_LF:
				return format_to(ctx.begin(), "eol$");
			case LookaheadToken::XAnyLine:
				return format_to(ctx.begin(), "<XAnyLine>");
			default:
				return format_to(ctx.begin(), "<{}>", static_cast<unsigned>(v));
		}
	}
};
}  // namespace fmt

TEST(Lexer, lookahead)
{
	Compiler cc;
	cc.parse(RULES);

	// Lexer<LookaheadToken> lexer { cc.compile(), "abba abcd cdef" };
	LexerDef lexerDef = cc.compile();
	logf("LexerDef:\n{}", lexerDef.to_string());
	Lexer<LookaheadToken, StateId, false, true> lexer{lexerDef, "abba abcdef",
													  [this](const std::string& msg) { log(msg); }};

	ASSERT_EQ(LookaheadToken::ABBA, lexer.recognize());
	ASSERT_EQ(LookaheadToken::AB_CD, lexer.recognize());
	ASSERT_EQ(LookaheadToken::CDEF, lexer.recognize());
	ASSERT_EQ(LookaheadToken::Eof, lexer.recognize());
}

TEST(Lexer, LexerError)
{
	Compiler cc;
	cc.parse(RULES);

	using LookaheadLexer = Lexer<LookaheadToken, StateId, false, false>;
	LookaheadLexer lexer{cc.compile(), "invalid"};

	EXPECT_THROW(lexer.recognize(), LookaheadLexer::LexerError);
}

TEST(Lexer, evaluateDotToken)
{
	Compiler cc;
	cc.parse(RULES);

	Lexer<LookaheadToken, StateId, false, false> lexer{cc.compile(), "xanything"};

	ASSERT_EQ(LookaheadToken::XAnyLine, lexer.recognize());
	ASSERT_EQ(LookaheadToken::Eof, lexer.recognize());
}

TEST(Lexer, match_eol)
{
	Compiler cc;
	cc.parse(RULES);

	LexerDef lexerDef = cc.compile();
	logf("LexerDef:\n{}", lexerDef.to_string());
	Lexer<LookaheadToken, StateId, false, true> lexer{lexerDef, "abba eol\nabba",
													  [this](const std::string& msg) { log(msg); }};

	ASSERT_EQ(LookaheadToken::ABBA, lexer.recognize());
	EXPECT_EQ(0, lexer.offset().first);
	ASSERT_EQ(4, lexer.offset().second);

	ASSERT_EQ(LookaheadToken::EOL_LF, lexer.recognize());
	EXPECT_EQ(5, lexer.offset().first);
	ASSERT_EQ(8, lexer.offset().second);

	ASSERT_EQ(LookaheadToken::ABBA, lexer.recognize());
	EXPECT_EQ(9, lexer.offset().first);    // EOF
	ASSERT_EQ(13, lexer.offset().second);  // EOF

	ASSERT_EQ(LookaheadToken::Eof, lexer.recognize());
}

TEST(Lexer, bol)
{
	Compiler cc;
	cc.parse(R"(|Spacing(ignore)  ::= [\s\t\n]+
              |Pragma           ::= ^pragma
              |Test             ::= test
              |Unknown          ::= .
              |Eof              ::= <<EOF>>
              |)"_multiline);

	LexerDef ld = cc.compileMulti();
	logf("LexerDef:\n{}", ld.to_string());
	Lexer<Tag, StateId, true, true> lexer{ld, "pragma", [this](const std::string& msg) { log(msg); }};
	ASSERT_EQ(1, lexer.recognize());  // ^pragma
	ASSERT_EQ(4, lexer.recognize());  // EOS
}

TEST(Lexer, bol_no_match)
{
	Compiler cc;
	cc.parse(R"(|Spacing(ignore)  ::= [\s\t\n]+
              |Pragma           ::= ^pragma
              |Test             ::= test
              |Unknown          ::= .
              |Eof              ::= <<EOF>>
              |)"_multiline);

	LexerDef ld = cc.compileMulti();
	logf("LexerDef:\n{}", ld.to_string());
	Lexer<Tag, StateId, true, true> lexer{ld, "test pragma", [this](const std::string& msg) { log(msg); }};
	ASSERT_EQ(2, lexer.recognize());  // test

	// pragma (char-wise) - must not be recognized as ^pragma
	ASSERT_EQ(3, lexer.recognize());
	ASSERT_EQ(3, lexer.recognize());
	ASSERT_EQ(3, lexer.recognize());
	ASSERT_EQ(3, lexer.recognize());
	ASSERT_EQ(3, lexer.recognize());
	ASSERT_EQ(3, lexer.recognize());

	ASSERT_EQ(4, lexer.recognize());  // EOS
}

TEST(Lexer, bol_line2)
{
	Compiler cc;
	cc.parse(R"(|Spacing(ignore)  ::= [\s\t\n]+
              |Pragma           ::= ^pragma
              |Test             ::= test
              |Eof              ::= <<EOF>>
              |)"_multiline);

	LexerDef ld = cc.compileMulti();
	logf("LexerDef:\n{}", ld.to_string());
	Lexer<Tag, StateId, true, true> lexer{ld, "test\npragma", [this](const std::string& msg) { log(msg); }};
	ASSERT_EQ(2, lexer.recognize());  // test
	ASSERT_EQ(1, lexer.recognize());  // ^pragma
	ASSERT_EQ(3, lexer.recognize());  // EOS
}

TEST(Lexer, bol_and_other_conditions)
{
	Compiler cc;
	cc.parse(R"(|Spacing(ignore)  ::= [\s\t\n]+
              |Pragma           ::= ^pragma
              |Test             ::= test
              |Eof              ::= <<EOF>>
              |<Asm>Jump        ::= jmp)"_multiline);
	LexerDef ld = cc.compileMulti();
	logf("LexerDef:\n{}", ld.to_string());

	Lexer<Tag, StateId, true, true> lexer{ld, "pragma test", [this](const std::string& msg) { log(msg); }};
	ASSERT_EQ(1, lexer.recognize());  // ^pragma
	ASSERT_EQ(2, lexer.recognize());  // test
	ASSERT_EQ(3, lexer.recognize());  // <<EOF>>
}

TEST(Lexer, bol_rules_on_non_bol_lexer)
{
	Compiler cc;
	cc.parse(R"(|Spacing(ignore)  ::= [\s\t\n]+
              |Eof              ::= <<EOF>>
              |Test             ::= "test"
              |Pragma           ::= ^"pragma"
              |Unknown          ::= .
              |)"_multiline);

	LexerDef lexerDef = cc.compile();
	using SimpleLexer = Lexer<Tag, StateId, false, false>;
	ASSERT_THROW(SimpleLexer(lexerDef, "pragma"), std::invalid_argument);
}

TEST(Lexer, non_bol_rules_on_non_bol_lexer)
{
	Compiler cc;
	cc.parse(R"(|Spacing(ignore)  ::= [\s\t\n]+
              |Eof              ::= <<EOF>>
              |Test             ::= "test"
              |Unknown          ::= .
              |)"_multiline);

	LexerDef lexerDef = cc.compile();
	Lexer<Tag, StateId, false, false> lexer{lexerDef, " test "};

	ASSERT_EQ(2, lexer.recognize());  // "test"
	ASSERT_EQ(1, lexer.recognize());  // <<EOF>>
}

TEST(Lexer, non_bol_rules_on_bol_lexer)
{
	Compiler cc;
	cc.parse(R"(|Spacing(ignore)  ::= [\s\t\n]+
              |Eof              ::= <<EOF>>
              |Test             ::= "test"
              |Unknown          ::= .
              |)"_multiline);

	LexerDef lexerDef = cc.compile();
	Lexer<Tag, StateId, false, false> lexer{lexerDef, " test "};

	ASSERT_EQ(2, lexer.recognize());  // "test"
	ASSERT_EQ(1, lexer.recognize());  // <<EOF>>
}

TEST(Lexer, iterator)
{
	Compiler cc;
	cc.parse(std::make_unique<std::stringstream>(R"(
      Spacing(ignore) ::= [\s\t\n]+
      A               ::= a
      B               ::= b
      Eof             ::= <<EOF>>
  )"));

	Lexer<Tag> lexer{cc.compile(), std::make_unique<std::stringstream>("a b b a")};

	Lexer<Tag>::iterator i = lexer.begin();
	Lexer<Tag>::iterator e = lexer.end();

	// a
	ASSERT_EQ(1, *i);
	ASSERT_TRUE(i != e);

	// b
	i++;
	ASSERT_EQ(2, *i);
	ASSERT_TRUE(i != e);

	// b
	i++;
	ASSERT_EQ(2, *i);
	ASSERT_TRUE(i != e);

	// a
	i++;
	ASSERT_EQ(1, *i);
	ASSERT_TRUE(i != e);

	// <<EOF>>
	i++;
	ASSERT_EQ(3, *i);
	ASSERT_TRUE(i != e);

	i++;
	ASSERT_EQ(3, *i);  // still EOF
	ASSERT_TRUE(i == e);
}

TEST(Lexer, empty_alt)
{
	Compiler cc;
	cc.parse(R"(|Spacing(ignore) ::= [\s\t\n]+
              |Test            ::= aa(bb|)
              |Eof             ::= <<EOF>>
              |)"_multiline);

	LexerDef ld = cc.compileMulti();
	logf("LexerDef:\n{}", ld.to_string());
	Lexer<Tag, StateId, false, true> lexer{ld, "aabb aa aabb", [this](const std::string& msg) { log(msg); }};

	ASSERT_EQ(1, lexer.recognize());
	ASSERT_EQ(1, lexer.recognize());
	ASSERT_EQ(1, lexer.recognize());
	ASSERT_EQ(2, lexer.recognize());  // EOF
}

TEST(Lexer, ignore_many)
{
	Compiler cc;
	cc.parse(R"(|Spacing(ignore)  ::= [\s\t\n]+
              |Comment(ignore)  ::= #.*
              |Eof              ::= <<EOF>>
              |Foo              ::= foo
              |Bar              ::= bar
              |)"_multiline);

	LexerDef lexerDef = cc.compileMulti();
	logf("LexerDef:\n{}", lexerDef.to_string());
	Lexer<int, StateId, false, true> lexer{lexerDef,
										   R"(|# some foo
                                              |foo
                                              |
                                              |# some bar
                                              |bar
                                              |)"_multiline,
										   [this](const std::string& msg) { log(msg); }};

	ASSERT_EQ(2, lexer.recognize());
	ASSERT_EQ("foo", lexer.word());

	ASSERT_EQ(3, lexer.recognize());
	ASSERT_EQ("bar", lexer.word());

	ASSERT_EQ(1, lexer.recognize());  // EOF
}

TEST(Lexer, realworld_ipv4)
{
	Compiler cc;
	cc.parse(R"(|
              |Spacing(ignore)   ::= [\s\t\n]+
              |Eof               ::= <<EOF>>
              |IPv4Octet(ref)    ::= [0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5]
              |IPv4(ref)         ::= {IPv4Octet}(\.{IPv4Octet}){3}
              |IPv4Literal       ::= {IPv4}
              |)"_multiline);

	Lexer<int, StateId, false, true> lexer{cc.compile(),
										   R"(0.0.0.0 4.2.2.1 10.10.40.199 255.255.255.255)",
										   [this](const std::string& msg) { log(msg); }};

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
namespace fmt {  // it sucks that I've to specify that here
template <>
struct formatter<RealWorld> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const RealWorld& v, FormatContext& ctx)
	{
		switch (v)
		{
			case RealWorld::Eof:
				return format_to(ctx.begin(), "Eof");
			case RealWorld::IPv4:
				return format_to(ctx.begin(), "IPv4");
			case RealWorld::IPv6:
				return format_to(ctx.begin(), "IPv6");
			default:
				return format_to(ctx.begin(), "<{}>", static_cast<unsigned>(v));
		}
	}
};
}  // namespace fmt

TEST(Lexer, realworld_ipv6)
{
	Compiler cc;
	cc.parse(R"(|
      |Spacing(ignore)   ::= [\s\t\n]+
      |Eof               ::= <<EOF>>
      |
      |IPv4Octet(ref)    ::= [0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5]
      |IPv4(ref)         ::= {IPv4Octet}(\.{IPv4Octet}){3}
      |IPv4Literal       ::= {IPv4}
      |
      |ipv6Part(ref)     ::= [[:xdigit:]]{1,4}
      |IPv6              ::= {ipv6Part}(:{ipv6Part}){7,7}
      |                    | ({ipv6Part}:){1,7}:
      |                    | :(:{ipv6Part}){1,7}
      |                    | ::
      |                    | ({ipv6Part}:){1}(:{ipv6Part}){0,6}
      |                    | ({ipv6Part}:){2}(:{ipv6Part}){0,5}
      |                    | ({ipv6Part}:){3}(:{ipv6Part}){0,4}
      |                    | ({ipv6Part}:){4}(:{ipv6Part}){0,3}
      |                    | ({ipv6Part}:){5}(:{ipv6Part}){0,2}
      |                    | ({ipv6Part}:){6}(:{ipv6Part}){0,1}
      |                    | ::[fF]{4}:{IPv4}
  )"_multiline);

	static const std::string TEXT = R"(|0:0:0:0:0:0:0:0
                                     |1234:5678:90ab:cdef:aaaa:bbbb:cccc:dddd
                                     |2001:0db8:85a3:0000:0000:8a2e:0370:7334
                                     |1234:5678::
                                     |0::
                                     |::0
                                     |::
                                     |1::3:4:5:6:7:8
                                     |1::4:5:6:7:8
                                     |1::5:6:7:8
                                     |1::8
                                     |1:2::4:5:6:7:8
                                     |1:2::5:6:7:8
                                     |1:2::8
                                     |::ffff:127.0.0.1
                                     |::ffff:c000:0280
                                     |)"_multiline;

	LexerDef lexerDef = cc.compileMulti();
	Lexer<RealWorld, StateId, false, true> lexer{lexerDef, TEXT,
												 [this](const std::string& msg) { log(msg); }};

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

TEST(Lexer, internal)
{
	ASSERT_EQ("Eof", fmt::format("{}", LookaheadToken::Eof));
	ASSERT_EQ("abba", fmt::format("{}", LookaheadToken::ABBA));
	ASSERT_EQ("ab/cd", fmt::format("{}", LookaheadToken::AB_CD));
	ASSERT_EQ("cd", fmt::format("{}", LookaheadToken::CD));
	ASSERT_EQ("cdef", fmt::format("{}", LookaheadToken::CDEF));
	ASSERT_EQ("eol$", fmt::format("{}", LookaheadToken::EOL_LF));
	ASSERT_EQ("<XAnyLine>", fmt::format("{}", LookaheadToken::XAnyLine));
	ASSERT_EQ("<724>", fmt::format("{}", static_cast<LookaheadToken>(724)));

	ASSERT_EQ("Eof", fmt::format("{}", RealWorld::Eof));
	ASSERT_EQ("IPv4", fmt::format("{}", RealWorld::IPv4));
	ASSERT_EQ("IPv6", fmt::format("{}", RealWorld::IPv6));
	ASSERT_EQ("<724>", fmt::format("{}", static_cast<RealWorld>(724)));
}
