// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/RuleParser.h>
#include <klex/RegExpr.h>
#include <klex/RegExprParser.h>
#include <klex/LexerDef.h>        // special tags
#include <sstream>
#include <iostream>
#include <cstring>

#include <klex/Symbols.h>

namespace klex {

RuleParser::RuleParser(std::unique_ptr<std::istream> input)
    : stream_{std::move(input)},
      refRules_{},
      lastParsedRule_{nullptr},
      lastParsedRuleIsRef_{false},
      currentChar_{0},
      line_{1},
      column_{0},
      offset_{0},
      nextTag_{FirstUserTag} {
  consumeChar();
}

RuleList RuleParser::parseRules() {
  RuleList rules;

  for (;;) {
    consumeSpace();
    if (eof()) {
      break;
    } else if (currentChar() == '\n') {
      consumeChar();
    } else {
      parseRule(rules);
    }
  }

  return rules;
}

void RuleParser::parseRule(RuleList& rules) {
  // Rule         ::= RuleConditionList? BasicRule
  //                | RuleConditionList '{' BasicRule* '}' LF?
  // BasicRule    ::= TOKEN RuleOptions? SP '::=' SP RegEx SP? LF
  // RuleOptions  ::= '(' RuleOption (',' RuleOption)*
  // RuleOption   ::= ignore

  consumeSP();
  if (currentChar_ == '|' && lastParsedRule_ != nullptr) {
    consumeChar();
    consumeSP();
    const std::string pattern = parseExpression();
    lastParsedRule_->pattern += '|' + pattern;
    return;
  }

  // finalize ref-rule by surrounding it with round braces
  if (lastParsedRuleIsRef_)
    lastParsedRule_->pattern = fmt::format("({})", lastParsedRule_->pattern);

  std::vector<std::string> conditions = parseRuleConditions();
  consumeSP();
  if (!conditions.empty() && currentChar() == '{') {
    consumeChar();
    consumeAnySP(); // allow whitespace, including LFs
    while (!eof() && currentChar() != '}') {
      parseBasicRule(rules, std::vector<std::string>(conditions));
      consumeSP(); //  part of the next line, allow indentation
    }
    consumeChar('}');
    consumeSP();
    consumeChar('\n');
  } else {
    parseBasicRule(rules, std::move(conditions));
  }
}

void RuleParser::parseBasicRule(RuleList& rules, std::vector<std::string>&& conditions) {
  const unsigned int beginLine = line_;
  const unsigned int beginColumn = column_;

  std::string token = consumeToken();
  bool ignore = false;
  bool ref = false;
  if (currentChar_ == '(') {
    consumeChar();
    unsigned optionOffset = offset_;
    std::string option = consumeToken();
    consumeChar(')');

    if (option == "ignore")
      ignore = true;
    else if (option == "ref")
      ref = true;
    else
      throw InvalidRuleOption{optionOffset, option};
  }
  consumeSP();
  consumeAssoc();
  consumeSP();
  const unsigned int line = line_;
  const unsigned int column = column_;
  const std::string pattern = parseExpression();
  consumeChar('\n');

  const Tag tag = [&] {
    if (ignore || ref)
      return IgnoreTag;
    else if (auto i = std::find_if(rules.begin(), rules.end(),
                                   [&](const auto& r) { return r.name == token; }); i != rules.end())
      return i->tag;
    else
      return nextTag_++;
  }();

  if (ref && !conditions.empty())
    throw InvalidRefRuleWithConditions{beginLine, beginColumn,
                                       Rule{line, column, tag, std::move(conditions), token, pattern}};

  if (conditions.empty())
    conditions.emplace_back("INITIAL");

  std::sort(conditions.begin(), conditions.end());

  if (!ref) {
    if (auto i = std::find_if(rules.begin(), rules.end(),
                              [&](const Rule& r) { return r.name == token; }); i != rules.end()) {
      throw DuplicateRule{Rule{line, column, tag, std::move(conditions), token, pattern}, *i};
    } else {
      rules.emplace_back(Rule{line, column, tag, conditions, token, pattern});
      lastParsedRule_ = &rules.back();
      lastParsedRuleIsRef_ = false;
    }
  } else if (auto i = refRules_.find(token); i != refRules_.end()) {
    throw DuplicateRule{Rule{line, column, tag, std::move(conditions), token, pattern}, i->second};
  } else {
    // TODO: throw if !conditions.empty();
    refRules_[token] = {line, column, tag, {}, token, pattern};
    lastParsedRule_ = &refRules_[token];
    lastParsedRuleIsRef_ = true;
  }
}

std::vector<std::string> RuleParser::parseRuleConditions() {
  // RuleConditionList ::= '<' TOKEN (',' SP* TOKEN) '>'
  if (currentChar() != '<')
    return {};

  consumeChar();
  std::vector<std::string> conditions { consumeToken() };

  while (currentChar() == ',') {
    consumeChar();
    consumeSP();
    conditions.emplace_back(consumeToken());
  }

  consumeChar('>');

  return std::move(conditions);
}

std::string RuleParser::parseExpression() {
  // expression ::= " .... "
  //              | ....

  std::stringstream sstr;

  size_t i = 0;
  size_t lastGraph = 0;
  while (currentChar_  != '\n') {
    if (std::isgraph(currentChar_))
      lastGraph = i + 1;
    i++;
    sstr << consumeChar();
  }
  std::string pattern = sstr.str().substr(0, lastGraph); // skips trailing spaces

  // replace all occurrences of {ref}
  for (const std::pair<const std::string, Rule>& ref : refRules_) {
    const Rule& rule = ref.second;
    const std::string name = fmt::format("{{{}}}", rule.name);
    // for (size_t i = 0; (i = pattern.find(name, i)) != std::string::npos; i += rule.pattern.size()) {
    //   pattern.replace(i, name.size(), rule.pattern);
    // }
    size_t i = 0;
    while ((i = pattern.find(name, i)) != std::string::npos) {
      pattern.replace(i, name.size(), rule.pattern);
      i += rule.pattern.size();
    }
  }

  return pattern;
}

// skips space until LF or EOF
void RuleParser::consumeSpace() {
  for (;;) {
    switch (currentChar_) {
      case ' ':
      case '\t':
      case '\r':
        consumeChar();
        break;
      case '#':
        while (!eof() && currentChar_ != '\n') {
          consumeChar();
        }
        break;
      default:
        return;
    }
  }
}

char RuleParser::currentChar() const noexcept {
  return currentChar_;
}

char RuleParser::consumeChar(char ch) {
  if (currentChar_ != ch)
    throw UnexpectedChar{line_, column_, currentChar_, ch};

  return consumeChar();
}

char RuleParser::consumeChar() {
  char t = currentChar_;

  currentChar_ = stream_->get();
  if (!stream_->eof()) {
    offset_++;
    if (t == '\n') {
      line_++;
      column_ = 1;
    } else {
      column_++;
    }
  }

  return t;
}

bool RuleParser::eof() const noexcept {
  return currentChar_ < 0 || stream_->eof();
}

std::string RuleParser::consumeToken() {
  std::stringstream sstr;

  if (!std::isalpha(currentChar_) || currentChar_ == '_')
    throw UnexpectedToken{offset_, currentChar_, "Token"};

  do sstr << consumeChar();
  while (std::isalnum(currentChar_) || currentChar_ == '_');

  return sstr.str();
}

void RuleParser::consumeAnySP() {
  while (currentChar_ == ' ' || currentChar_ == '\t' || currentChar_ == '\n')
    consumeChar();
}

void RuleParser::consumeSP() {
  while (currentChar_ == ' ' || currentChar_ == '\t')
    consumeChar();
}

void RuleParser::consumeAssoc() {
  consumeChar(':');
  consumeChar(':');
  consumeChar('=');
}

} // namespace klex
