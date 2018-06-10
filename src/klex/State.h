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

using EdgeList = std::vector<Edge>;

// represents an epsilon-transition
constexpr Symbol EpsilonTransition = '\0';

class State {
 public:
  State() : State{(StateId)-1, false, 0} {}
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

class StateVec {
 public:
  bool empty() const noexcept { return states_.empty(); }
  size_t size() const noexcept { return states_.size(); }
  StateId nextId() const noexcept { return (StateId) size(); }

  State* operator[](StateId id) {
    return states_[id].get();
  }

  bool contains(StateId id) const noexcept {
    return id < nextId();
  }

  State* create() {
    return create(false, Tag{0});
  }

  State* create(bool accepting, Tag acceptTag) {
    StateId id = nextId();
    states_.emplace_back(std::make_unique<State>(id, acceptTag, accepting));
    return states_.back().get();
  }

  State* find(StateId id) const {
    for (const std::unique_ptr<State>& s : states_)
      if (s->id() == id)
        return s.get();

    return nullptr;
  }

  void append(StateVec other) {
    states_.reserve(size() + other.size());
    for (size_t i = 0, e = other.size(); i != e; ++i) {
      other.states_[i]->setId(nextId());
      states_.emplace_back(std::move(other.states_[i]));
    }
  }

  class const_iterator {
   public:
    using WrappedIterator = std::vector<std::unique_ptr<State>>::const_iterator;

    explicit const_iterator(WrappedIterator wrapped) : it_{wrapped} {}

    const State* operator*() const { return it_->get(); }
    const State* operator->() const { return it_->get(); }
    bool operator==(const const_iterator& other) const noexcept { return it_ == other.it_; }
    bool operator!=(const const_iterator& other) const noexcept { return it_ != other.it_; }
    const_iterator& operator++() { ++it_; return *this; }
    const_iterator& operator++(int) { it_++; return *this; }

   private:
    WrappedIterator it_;
  };

  class iterator {
   public:
    using WrappedIterator = std::vector<std::unique_ptr<State>>::iterator;

    explicit iterator(WrappedIterator wrapped) : it_{wrapped} {}

    State* operator*() const { return it_->get(); }
    State* operator->() const { return it_->get(); }
    bool operator==(const iterator& other) const noexcept { return it_ == other.it_; }
    bool operator!=(const iterator& other) const noexcept { return it_ != other.it_; }
    iterator& operator++() { ++it_; return *this; }
    iterator& operator++(int) { it_++; return *this; }

   private:
    WrappedIterator it_;
  };

  iterator begin() { return iterator(states_.begin()); }
  iterator end() { return iterator(states_.end()); }

  const_iterator begin() const { return const_iterator(states_.begin()); }
  const_iterator end() const { return const_iterator(states_.end()); }

  // using const_iterator = std::vector<std::unique_ptr<State>>::const_iterator;
  // const_iterator begin() const { return states_.begin(); }
  // const_iterator end() const { return states_.end(); }

 private:
  std::vector<std::unique_ptr<State>> states_;
};

#if 0
using OwnedStateSet = std::set<std::unique_ptr<State>>;
using StateSet = std::set<State*>;
#else
using OwnedStateSet = std::list<std::unique_ptr<State>>;
using StateSet = std::list<State*>;
#endif

/**
 * Returns a human readable string of the StateSet @p S, such as "{n0, n1, n2}".
 */
std::string to_string(const OwnedStateSet& S, std::string_view stateLabelPrefix = "n");
std::string to_string(const StateSet& S, std::string_view stateLabelPrefix = "n");
std::string to_string(const std::vector<const State*>& S, std::string_view stateLabelPrefix = "n");

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
