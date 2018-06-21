// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h>
#include <klex/Rule.h>
#include <klex/LexerDef.h>
#include <klex/NFA.h>
#include <klex/DFABuilder.h>

#include <istream>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace klex {

class DFA;

/**
 * Top-Level API for compiling lexical patterns into table definitions for Lexer.
 *
 * @see Lexer
 */
class Compiler {
 public:
  using TagNameMap = std::map<Tag, std::string>;
  using OvershadowMap = DFABuilder::OvershadowMap;

  Compiler() : fa_{} {}

  /**
   * Parses a @p stream of textual rule definitions to construct their internal data structures.
   */
  void parse(std::unique_ptr<std::istream> stream);

  /**
   * Parses a list of @p rules to construct their internal data structures.
   */
  void declareAll(RuleList rules);

  /**
   * Parses a single @p rule to construct their internal data structures.
   */
  void declare(Rule rule);

  const RuleList& rules() const noexcept { return rules_; }
  const NFA& nfa() const { return fa_; }
  const TagNameMap& names() const noexcept { return names_; }

  /**
   * Compiles all previousely parsed rules into a DFA.
   */
  DFA compileDFA(OvershadowMap* overshadows = nullptr);

  /**
   * Compiles all previousely parsed rules into a minimal DFA.
   */
  DFA compileMinimalDFA();

  /**
   * Compiles all previousely parsed rules into a suitable data structure for Lexer.
   *
   * @see Lexer
   */
  LexerDef compile();

  /**
   * Translates the given DFA @p dfa with a given TagNameMap @p names into trivial table mappings.
   *
   * @see Lexer
   */
  static LexerDef generateTables(const DFA& dfa, const TagNameMap& names);

 private:
  RuleList rules_;
  NFA fa_;
  TagNameMap names_;
};

} // namespace klex

