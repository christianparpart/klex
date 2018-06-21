// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/LexerDef.h>    // IgnoreTag
#include <klex/State.h>       // Tag
#include <string>
#include <vector>

namespace klex {

struct Rule {
  unsigned int line;
  unsigned int column;
  Tag tag;
  std::string name;
  std::string pattern;

  bool operator<(const Rule& rhs) const noexcept { return tag < rhs.tag; }
  bool operator<=(const Rule& rhs) const noexcept { return tag <= rhs.tag; }
  bool operator==(const Rule& rhs) const noexcept { return tag == rhs.tag; }
  bool operator!=(const Rule& rhs) const noexcept { return tag != rhs.tag; }
  bool operator>=(const Rule& rhs) const noexcept { return tag >= rhs.tag; }
  bool operator>(const Rule& rhs) const noexcept { return tag > rhs.tag; }
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
      if (v.tag == klex::IgnoreTag)
        return format_to(ctx.begin(), "{}({}) ::= {}", v.name, "ignore", v.pattern);
      else
        return format_to(ctx.begin(), "{}({}) ::= {}", v.name, v.tag, v.pattern);
    }
  };
}
