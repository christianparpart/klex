// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/Symbols.h>

#include <fmt/format.h>

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace klex {

using Tag = int;
using StateId = size_t;
using StateIdVec = std::vector<StateId>;

using AcceptMap = std::map<StateId, Tag>;

/**
 * Returns a human readable string of @p S, such as "{n0, n1, n2}".
 */
std::string to_string(const StateIdVec& S, std::string_view stateLabelPrefix = "n");

} // namespace klex

namespace fmt {
  template<>
  struct formatter<std::vector<klex::StateId>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const std::vector<klex::StateId>& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{}", klex::to_string(v));
    }
  };
}
