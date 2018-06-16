#include <klex/util/testing.h>
#include <klex/RuleParser.h>
#include <memory>
#include <sstream>

TEST(RuleParser, quotedPattern) {
  klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
    main ::= "blah"
  )")};
  klex::RuleList rules = rp.parseRules();
  ASSERT_EQ(1, rules.size());
  ASSERT_EQ("\"blah\"", rules[0].pattern);
}

// TEST(RuleParser, multiQuotedPattern) {
//   klex::RuleParser rp{std::make_unique<std::stringstream>(R"(
//     rule ::= "b"la"h"
//   )")};
//   klex::RuleList rules = rp.parseRules();
//   ASSERT_EQ(1, rules.size());
//   ASSERT_EQ("\"b\"la\"h\"", rules[0].pattern);
// }
