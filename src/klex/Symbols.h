// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <fmt/format.h>

#include <list>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace klex {

//! input symbol as used for transitions
using Symbol = int;

std::string prettySymbol(Symbol input);

// new way of wrapping up Symbols
struct Symbols {
  constexpr static Symbol Epsilon = 0x00;
  constexpr static Symbol EndOfFile = 0x03;
  constexpr static Symbol Error = 0x07;
  constexpr static Symbol Character(char ch) { return Symbol(ch); }
};

// represents an epsilon-transition
constexpr Symbol EpsilonTransition = Symbols::Epsilon;
constexpr Symbol EndOfFileTransition = Symbols::EndOfFile;
constexpr Symbol ErrorTransition = Symbols::Error;

/**
 * Represents a set of symbols.
 */
class SymbolSet {
 public:
  enum DotMode { Dot };

  explicit SymbolSet(DotMode);
  SymbolSet() : set_(256, false) {}

  //! Inserts given Symbol @p s into this set.
  void insert(Symbol s) { set_[(size_t) s] = true; }

  //! Removes given Symbol @p s from this set.
  void clear(Symbol s) { set_[(size_t) s] = false; }

  //! @returns whether or not given Symbol @p s is in this set.
  bool contains(Symbol s) const { return set_[(size_t) s]; }

  //! Tests whether or not this SymbolSet can be represented as dot (.), i.e. all but \n.
  bool isDot() const noexcept;

  //! @returns a human readable representation of this set
  std::string to_string() const;

  bool operator==(const SymbolSet& rhs) const noexcept { return set_ == rhs.set_; }
  bool operator!=(const SymbolSet& rhs) const noexcept { return set_ != rhs.set_; }

 private:
  // XXX we chose vector<bool> as it is an optimized bit vector
  std::vector<bool> set_;
};

} // namespace klex

namespace fmt {
  template<>
  struct formatter<klex::SymbolSet> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const klex::SymbolSet& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{}", v.to_string());
    }
  };
}

