// This file is part of the "x0" project, http://github.com/christianparpart/x0>
//   (c) 2009-2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/util/testing.h>
#include <klex/NFA.h>
#include <klex/Alphabet.h>

using namespace klex;

int main(int argc, const char* argv[]) {
  return klex::util::testing::main(argc, argv);
}

TEST(NFA, emptyCtor) {
  NFA nfa;
  ASSERT_EQ(0, nfa.size());
  ASSERT_TRUE(nfa.empty());
}

TEST(NFA, characterCtor) {
  NFA nfa{'a'};
  ASSERT_EQ(2, nfa.size());
  ASSERT_EQ(0, nfa.initialStateId());
  ASSERT_EQ(1, nfa.acceptStateId());
  ASSERT_EQ(StateIdVec{1}, nfa.delta(StateIdVec{0}, 'a'));
}

TEST(NFA, epsilonClosure) {
  NFA nfa{'a'};
  ASSERT_EQ(0, nfa.initialStateId());
  ASSERT_EQ(1, nfa.acceptStateId());
  ASSERT_EQ(StateIdVec{0}, nfa.epsilonClosure(StateIdVec{0}));
}

TEST(NFA, delta) {
  NFA nfa{'a'};
  ASSERT_EQ(0, nfa.initialStateId());
  ASSERT_EQ(1, nfa.acceptStateId());
  ASSERT_EQ(StateIdVec{1}, nfa.delta(StateIdVec{0}, 'a'));
  logf("blubb {}", nfa.size());
}

TEST(NFA, alphabet) {
  ASSERT_EQ("{}", NFA{}.alphabet().to_string());
  ASSERT_EQ("{a}", NFA{'a'}.alphabet().to_string());
}
