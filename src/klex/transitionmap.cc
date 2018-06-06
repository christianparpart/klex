// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/transitionmap.h>
#include <klex/fa.h>
#include <algorithm>

namespace klex {

void TransitionMap::define(fa::StateId currentState, fa::Symbol charCat, fa::StateId nextState) {
  mapping_[currentState][charCat] = nextState;
}

fa::StateId TransitionMap::apply(fa::StateId currentState, fa::Symbol charCat) const {
  if (auto i = mapping_.find(currentState); i != mapping_.end())
    if (auto k = i->second.find(charCat); k != i->second.end())
      return k->second;

  return ErrorState;
}

std::vector<fa::StateId> TransitionMap::states() const {
  std::vector<fa::StateId> v;
  v.reserve(mapping_.size());
  for (const auto& i : mapping_)
    v.push_back(i.first);
  std::sort(v.begin(), v.end());
  return v;
}

std::map<fa::Symbol, fa::StateId> TransitionMap::map(fa::StateId s) const {
  std::map<fa::Symbol, fa::StateId> m;
  if (auto mapping = mapping_.find(s); mapping != mapping_.end())
    for (const auto& i : mapping->second)
      m[i.first] = i.second;
  return m;
}

} // namespace klex

