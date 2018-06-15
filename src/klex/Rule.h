// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h> // fa::Tag
#include <string>
#include <vector>

namespace klex {

struct Rule {
  unsigned int line;
  unsigned int column;
  Tag tag;
  std::string name;
  std::string pattern;
};

using RuleList = std::vector<Rule>;

} // namespace klex

namespace fmt {
  template<>
  struct formatter<klex::Rule> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const klex::Rule& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{{{}, \"{}\", \"{}\"}}",
          v.tag,
          v.name,
          v.pattern);
    }
  };
}
