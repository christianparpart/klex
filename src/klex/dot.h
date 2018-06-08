// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h>

#include <list>
#include <string>
#include <string_view>

namespace klex {

//! Helper struct for the dot(std::list<DotGraph>) utility function.
struct DotGraph {
  const State* initialState;
  const OwnedStateSet& states;
  std::string_view stateLabelPrefix;
  std::string_view graphLabel;
};

/**
 * Creates a dot-file for multiple FiniteAutomaton in one graph (each FA represent one sub-graph).
 */
std::string dot(std::list<DotGraph> list, std::string_view label = "", bool groupEdges = true);

} // namespace klex
