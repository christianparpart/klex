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
#include <vector>

namespace klex {

//! input symbol as used for transitions
using Symbol = char;

std::string prettySymbol(Symbol input, bool dot = false);
std::string groupCharacterClassRanges(std::vector<Symbol> chars, bool dot = false);

using StateId = unsigned int;
using Tag = int;

class State;

// an edge for transitions
struct Edge {
  Symbol symbol;
  State* state;

  Edge(Symbol _symbol, State* _state) : symbol{_symbol}, state{_state} {}
};

using EdgeList = std::list<Edge>;

// represents an epsilon-transition
constexpr Symbol EpsilonTransition = '\0';

class State {
 public:
  explicit State(StateId id) : State{id, false, 0} {}
  State(StateId id, bool accepting, Tag tag)
      : id_{id},
        accepting_{accepting},
        tag_{tag},
        transitions_{} {}

  StateId id() const noexcept { return id_; }
  void setId(StateId id) { id_ = id; }

  EdgeList& transitions() noexcept { return transitions_; }
  const EdgeList& transitions() const noexcept { return transitions_; }

  State* transition(Symbol input) const;

  void linkTo(State* state) { linkTo(EpsilonTransition, state); }
  void linkTo(Symbol condition, State* state);

  void setAccept(bool accepting) { accepting_ = accepting; }
  bool isAccepting() const noexcept { return accepting_; }

  Tag tag() const noexcept { return tag_; }
  void setTag(Tag tag) { tag_ = tag; }

 private:
  StateId id_;
  bool accepting_;
  Tag tag_; //! smallest tag reflects the highest priority
  EdgeList transitions_;
};

using OwnedStateSet = std::set<std::unique_ptr<State>>;
using StateSet = std::set<State*>;

/**
 * Returns a human readable string of the StateSet @p S, such as "{n0, n1, n2}".
 */
std::string to_string(const StateSet& S, std::string_view stateLabelPrefix = "n");
std::string to_string(const OwnedStateSet& S, std::string_view stateLabelPrefix = "n");

#define VERIFY_STATE_AVAILABILITY(freeId, set)                                  \
  do {                                                                          \
    if (std::find_if((set).begin(), (set).end(),                                \
          [&](const auto& s) { return s->id() == freeId; }) != (set).end()) {   \
      std::cerr << fmt::format(                                                 \
          "VERIFY_STATE_AVAILABILITY({0}) failed. Id {0} is already in use.\n", \
          (freeId));                                                            \
      abort();                                                                  \
    }                                                                           \
  } while (0)

} // namespace klex

namespace fmt {
  template<>
  struct formatter<klex::StateSet> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const klex::StateSet& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{}", klex::to_string(v));
    }
  };

  template<>
  struct formatter<klex::OwnedStateSet> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const klex::OwnedStateSet& v, FormatContext &ctx) {
      return format_to(ctx.begin(), "{}", klex::to_string(v));
    }
  };
}
