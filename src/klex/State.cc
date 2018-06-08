// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/State.h>
#include <sstream>

namespace klex {

State* State::transition(Symbol input) const {
  for (const Edge& transition : transitions_)
    if (input == transition.symbol)
      return transition.state;

  return nullptr;
}

void State::linkTo(Symbol input, State* state) {
  transitions_.emplace_back(input, state);
}

std::string to_string(const OwnedStateSet& S, std::string_view stateLabelPrefix) {
  std::vector<StateId> names;
  for (const std::unique_ptr<State>& s : S)
    names.push_back(s->id());

  std::sort(names.begin(), names.end());

  std::stringstream sstr;
  sstr << "{";
  int i = 0;
  for (StateId name : names) {
    if (i)
      sstr << ", ";
    sstr << stateLabelPrefix << name;
    i++;
  }
  sstr << "}";

  return sstr.str();
}

std::string to_string(const StateSet& S, std::string_view stateLabelPrefix) {
  std::vector<StateId> names;
  for (const State* s : S)
    names.push_back(s->id());

  std::sort(names.begin(), names.end());

  std::stringstream sstr;
  sstr << "{";
  int i = 0;
  for (StateId name : names) {
    if (i)
      sstr << ", ";
    sstr << stateLabelPrefix << name;
    i++;
  }
  sstr << "}";

  return sstr.str();
}

std::string prettySymbol(Symbol input, bool dot) {
  switch (input) {
    case -1:
      return "<EOF>";
    case EpsilonTransition:
      return "ε";
    case ' ':
      if (dot)
        return "\\\\s";
      else
        return "\\s";
    case '\t':
      if (dot)
        return "\\\\t";
      else
        return "\\t";
    case '\n':
      if (dot)
        return "\\\\n";
      else
        return "\\n";
    default:
      return fmt::format("{}", input);
  }
}

} // namespace klex
