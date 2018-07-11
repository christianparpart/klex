// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cassert>

namespace klex {

//! input symbol as used for transitions
using Symbol = int;

std::string prettySymbol(Symbol input);
std::string prettyCharRange(Symbol ymin, Symbol ymax);
std::string groupCharacterClassRanges(const std::vector<bool>& syms);
std::string groupCharacterClassRanges(std::vector<Symbol> syms);

// new way of wrapping up Symbols
struct Symbols {
  constexpr static Symbol Epsilon = 257;
  constexpr static Symbol Error = 258;
  constexpr static Symbol BeginOfLine = 259;
  constexpr static Symbol EndOfLine = 260;
  constexpr static Symbol EndOfFile = 261;
  constexpr static Symbol Character(char ch) { return Symbol(ch); }

  // numeric limits
  static constexpr Symbol min() { return 0; }
  static constexpr Symbol max() { return 261; }

  constexpr static bool isSpecial(Symbol s) {
    switch (s) {
      case Symbols::EndOfFile:
      case Symbols::EndOfLine:
      case Symbols::BeginOfLine:
      case Symbols::Epsilon:
      case Symbols::Error:
        return true;
      default:
        return false;
    }
  }
};

/**
 * Represents a set of symbols.
 */
class SymbolSet {
 public:
  enum DotMode { Dot };

  explicit SymbolSet(DotMode);
  SymbolSet() : set_(Symbols::max(), false), size_{0}, hash_{2166136261} {}

  explicit SymbolSet(std::initializer_list<Symbol> list) : SymbolSet() {
    std::for_each(list.begin(), list.end(), [this](Symbol s) { insert(s); });
  }

  bool empty() const noexcept { return size_ == 0; }
  size_t size() const noexcept { return size_; }

  //! Transforms into the complement set.
  void complement();

  //! Inserts given Symbol @p s into this set.
  void insert(Symbol s) {
    if (!contains(s)) {
      set_[s] = true;
      hash_ = (hash_ * 16777619) ^ s;
      size_++;
    }
  }

  //! Inserts a range of Simples between [a, b].
  void insert(const std::pair<Symbol, Symbol>& range) {
    for (Symbol s = range.first; s <= range.second; ++s) {
      insert(s);
    }
  }

  //! Removes given Symbol @p s from this set.
  void clear(Symbol s);

  //! @returns whether or not given Symbol @p s is in this set.
  bool contains(Symbol s) const {
    assert(s >= Symbols::min() && s <= Symbols::max() && "Only ASCII and special symbols allowed.");
    return set_[(size_t) s];
  }

  //! Tests whether or not this SymbolSet can be represented as dot (.), i.e. all but \n.
  bool isDot() const noexcept;

  //! @returns a human readable representation of this set
  std::string to_string() const;

  bool operator==(const SymbolSet& rhs) const noexcept { return hash_ == rhs.hash_ && set_ == rhs.set_; }
  bool operator!=(const SymbolSet& rhs) const noexcept { return !(*this == rhs); }

  class const_iterator { // {{{
   public:
    const_iterator(std::vector<bool>::const_iterator beg,
                   std::vector<bool>::const_iterator end,
                   size_t n)
        : beg_{std::move(beg)}, end_{std::move(end)}, offset_{n} {
      while (beg_ != end_ && !*beg_) {
        ++beg_;
        ++offset_;
      }
    }

    Symbol operator*() const { return static_cast<Symbol>(offset_); }

    const_iterator& operator++(int) {
      do {
        ++beg_;
        ++offset_;
      } while (beg_ != end_ && !*beg_);
      return *this;
    }

    const_iterator& operator++() {
      do {
        beg_++;
        offset_++;
      } while (beg_ != end_ && !*beg_);
      return *this;
    }

    bool operator==(const const_iterator& rhs) const noexcept { return beg_ == rhs.beg_; }
    bool operator!=(const const_iterator& rhs) const noexcept { return beg_ != rhs.beg_; }

   private:
    std::vector<bool>::const_iterator beg_;
    std::vector<bool>::const_iterator end_;
    size_t offset_;
  }; // }}}

  const_iterator begin() const { return const_iterator(set_.begin(), set_.end(), 0); }
  const_iterator end() const { return const_iterator(set_.end(), set_.end(), set_.size()); }

  size_t hash() const noexcept { return hash_; }

  using Vector = std::vector<Symbol>;

  Vector vector() const {
    Vector abc;
    abc.reserve(size());

    for (Symbol s : *this)
      abc.push_back(s);

    return std::move(abc);
  }

 private:
  void recalculateHash();

 private:
  // XXX we chose vector<bool> as it is an optimized bit vector
  std::vector<bool> set_;
  size_t size_;
  size_t hash_;
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

namespace std {
  template<>
  struct hash<klex::SymbolSet> {
    size_t operator()(const klex::SymbolSet& set) const {
      return set.hash();
    }
  };
}
