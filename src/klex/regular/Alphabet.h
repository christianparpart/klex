// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/regular/Symbols.h>
#include <set>
#include <string>
#include <fmt/format.h>

namespace klex::regular {

/**
 * Represents the alphabet of a finite automaton or regular expression.
 */
class Alphabet {
 public:
  using set_type = std::set<Symbol>;
  using iterator = set_type::iterator;

  size_t size() const noexcept { return alphabet_.size(); }

  void insert(Symbol ch);

  std::string to_string() const;

  const iterator begin() const { return alphabet_.begin(); }
  const iterator end() const { return alphabet_.end(); }

 private:
  set_type alphabet_;
};

} // namespace klex::regular

namespace fmt {
  template<>
  struct formatter<klex::regular::Alphabet> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const klex::regular::Alphabet& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{}", v.to_string());
    }
  };
}
