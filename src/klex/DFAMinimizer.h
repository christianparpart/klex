// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h>

#include <list>
#include <vector>

namespace klex {

class DFA;

class DFAMinimizer {
 public:
  explicit DFAMinimizer(const DFA& dfa);

  DFA construct();

 private:
  StateIdVec nonAcceptStates() const;
  bool containsInitialState(const StateIdVec& S) const;
  std::list<StateIdVec>::iterator findGroup(StateId s);
  int partitionId(StateId s) const;
  std::list<StateIdVec> split(const StateIdVec& S) const;

 private:
  const DFA& dfa_;
  std::list<StateIdVec> T;
  std::list<StateIdVec> P;
};

} // namespace klex

