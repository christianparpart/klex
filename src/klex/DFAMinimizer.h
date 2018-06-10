// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <list>
#include <vector>

namespace klex {

class DFA;
class State;

class DFAMinimizer {
 public:
  explicit DFAMinimizer(const DFA& dfa);

  DFA construct();

 private:
  std::vector<const State*> nonAcceptStates() const;
  bool containsInitialState(const std::vector<const State*>& S) const;
  std::list<std::vector<const State*>>::iterator findGroup(const State* s);
  int partitionId(State* s) const;
  std::list<std::vector<const State*>> split(const std::vector<const State*>& S) const;

 private:
  const DFA& dfa_;
  std::list<std::vector<const State*>> T;
  std::list<std::vector<const State*>> P;

};

} // namespace klex

