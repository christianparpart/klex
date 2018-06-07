// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/rule.h>
#include <string>

namespace klex {

class RuleParser {
 public:
  explicit RuleParser(std::unique_ptr<std::istream> input);

  std::list<Rule> parseRules();

 private:

 private:
  std::unique_ptr<std::istream> input_;
};

}
