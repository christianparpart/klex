// This file is part of the "x0" project, http://github.com/christianparpart/x0>
//   (c) 2009-2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/util/testing.h>
#include <klex/NFA.h>
#include <klex/State.h>
#include <klex/Alphabet.h>

using namespace klex;

TEST(NFA, emptyCtor) {
  const NFA nfa;
  ASSERT_EQ(0, nfa.size());
  ASSERT_TRUE(nfa.empty());
}

TEST(NFA, characterCtor) {
  const NFA nfa{'a'};
  ASSERT_EQ(2, nfa.size());
  ASSERT_EQ(0, nfa.initialStateId());
  ASSERT_EQ(1, nfa.acceptStateId());
  ASSERT_EQ(StateIdVec{1}, nfa.delta(StateIdVec{0}, 'a'));
}

TEST(NFA, concatenate) {
  const NFA ab = NFA{ 'a' }.concatenate(NFA{'b'});
  ASSERT_EQ(4, ab.size());
  ASSERT_EQ(0, ab.initialStateId());
  ASSERT_EQ(3, ab.acceptStateId());

  // TODO: check ab.initial == A.initial
  // TODO: check A.accept == B.initial
  // TODO: check ab.accept == B.accept
}

TEST(NFA, alternate) {
  const NFA ab = NFA{ 'a' }.alternate(NFA{'b'});
  ASSERT_EQ(6, ab.size());
  ASSERT_EQ(2, ab.initialStateId());
  ASSERT_EQ(3, ab.acceptStateId());

  // TODO: check acceptState transitions to A and B
  // TODO: check A and B's outgoing edges to final acceptState
}

TEST(NFA, epsilonClosure) {
  const NFA nfa{'a'};
  ASSERT_EQ(0, nfa.initialStateId());
  ASSERT_EQ(1, nfa.acceptStateId());
  ASSERT_EQ(StateIdVec{0}, nfa.epsilonClosure(StateIdVec{0}));

  const NFA abc = NFA{'a'}.concatenate(NFA{'b'}.alternate(NFA{'c'}).recurring());
  ASSERT_EQ(StateIdVec{0}, abc.epsilonClosure(StateIdVec{0}));

  const StateIdVec e1 {1, 2, 4, 6, 8, 9};
  ASSERT_EQ(e1, abc.epsilonClosure(StateIdVec{1}));
}

TEST(NFA, delta) {
  const NFA nfa{'a'};
  ASSERT_EQ(0, nfa.initialStateId());
  ASSERT_EQ(1, nfa.acceptStateId());
  ASSERT_EQ(StateIdVec{1}, nfa.delta(StateIdVec{0}, 'a'));
}

TEST(NFA, alphabet) {
  ASSERT_EQ("{}", NFA{}.alphabet().to_string());
  ASSERT_EQ("{a}", NFA{'a'}.alphabet().to_string());
  ASSERT_EQ("{ab}", NFA{'a'}.concatenate(NFA{'b'}).alphabet().to_string());
  ASSERT_EQ("{abc}", NFA{'a'}.concatenate(NFA{'b'}).alternate(NFA{'c'}).alphabet().to_string());
}