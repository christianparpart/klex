// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h>
#include <klex/Alphabet.h>

#include <vector>
#include <optional>

namespace klex {

class DFA;

class DFAMinimizer {
 public:
  explicit DFAMinimizer(const DFA& dfa);

  DFA construct();

 private:
  using PartitionVec = std::vector<StateIdVec>;

  StateIdVec nonAcceptStates() const;
  bool containsInitialState(const StateIdVec& S) const;
  PartitionVec::iterator findGroup(StateId s);
  std::optional<int> partitionId(StateId s) const;
  PartitionVec split(const StateIdVec& S) const;
  DFA constructFromPartitions(const PartitionVec& P) const;

  static void dumpGroups(const PartitionVec& T);

 private:
  const DFA& dfa_;
  Alphabet alphabet_;
  PartitionVec T;
  PartitionVec P;
};

} // namespace klex

