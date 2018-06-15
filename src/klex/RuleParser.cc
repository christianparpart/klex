// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/RuleParser.h>
#include <klex/LexerDef.h> // special tags
#include <sstream>
#include <iostream>
#include <cstring>

namespace klex {

RuleParser::RuleParser(std::unique_ptr<std::istream> input)
    : stream_{std::move(input)},
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
      rules.emplace_back(parseRule());
    }
  }

  return rules;
}

Rule RuleParser::parseRule() {
  // Rule ::= TOKEN RuleOptions? SP '::=' SP RegEx SP? LF
  // RuleOptions ::= '(' RuleOption (',' RuleOption)*
  // RuleOption ::= ignore

  std::string token = consumeToken();
  bool ignore = false;
  if (currentChar_ == '(') {
    consumeChar();
    unsigned optionOffset = offset_;
    std::string option = consumeToken();
    consumeChar(')');

    if (option == "ignore")
      ignore = true;
    else
      throw InvalidRuleOption{optionOffset, option};
  }
  consumeSP();
  consumeAssoc();
  consumeSP();
  unsigned int line = line_;
  unsigned int column = column_;
  std::string pattern = parseExpression();
  consumeSpace();
  consumeChar('\n');

  Tag tag{};
  if (ignore)
    tag = IgnoreTag;
  else if (pattern == "<<EOF>>")
    tag = EofTag;
  else
    tag = nextTag_++;

  return Rule{line, column, tag, token, pattern};
}

std::string RuleParser::parseExpression() {
  // expression ::= " .... "
  //              | ....

  std::stringstream sstr;

  if (currentChar_ == '"') {
    sstr << consumeChar();
    // TODO: count "'s and avoid breaking on escaped "
    while (!eof() && !strchr("\t\n\r\"", currentChar_)) {
      sstr << consumeChar();
    }
    sstr << consumeChar('"');
  } else {
    while (!eof() && !strchr("\t\n\r# ", currentChar_)) {
      sstr << consumeChar();
    }
  }

  return sstr.str();
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

  if (!std::isalpha(currentChar_))
    throw UnexpectedToken{offset_, currentChar_, "Token"};

  do sstr << consumeChar();
  while (std::isalnum(currentChar_));

  return sstr.str();
}

void RuleParser::consumeSP() {
  // require at least one SP character (0x20 or 0x09)
  if (currentChar_ != ' ' && currentChar_ != '\t')
    throw UnexpectedChar{line_, column_, currentChar_, ' '};

  do consumeChar();
  while (currentChar_ == ' ' || currentChar_ == '\t');
}

void RuleParser::consumeAssoc() {
  consumeChar(':');
  consumeChar(':');
  consumeChar('=');
}

} // namespace klex
