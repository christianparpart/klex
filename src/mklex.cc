// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/builder.h>
#include <klex/lexer.h>
#include <klex/fa.h>
#include <klex/rule.h>
#include <klex/ruleparser.h>
#include <klex/util/Flags.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

std::string charLiteral(int ch) {
  if (std::isprint(ch))
    return fmt::format("'{}'", (char)ch);

  switch (ch) {
    case ' ':
      return std::string{' ', 1};
    case '\t':
      return std::string{"'\\t'"};
    case '\n':
      return std::string{"'\\n'"};
    default:
      return fmt::format("0x{:>02x}", ch);
  }
}

void generateTableDefCxx(std::ostream& os, const klex::LexerDef& lexerDef, const klex::RuleList& rules,
                         const std::string& symbolName) {
  os << "#include <klex/lexerdef.h>\n";
  os << "\n";
  os << "klex::LexerDef " << symbolName << " {\n";
  os << "  // initial state\n";
  os << "  " << lexerDef.initialStateId << ",\n";
  os << "  // state transition table \n";
  os << "  klex::TransitionMap {\n";
  for (klex::fa::StateId stateId : lexerDef.transitions.states()) {
    os << "    { " << stateId << ", {";
    int c = 0;
    for (std::pair<klex::fa::Symbol, klex::fa::StateId> t : lexerDef.transitions.map(stateId)) {
      if (c) os << ", ";
      os << "{" << charLiteral(t.first) << ", " << t.second << "}";
      c++;
    }
    os << "},\n";
  }
  os << "  },\n";
  os << "  // accept state to action label mappings\n";
  os << "  klex::AcceptStateMap {\n";
  for (const std::pair<klex::fa::StateId, klex::fa::Tag>& accept : lexerDef.acceptStates) {
    os << fmt::format("    {{ {:>3}, {:>3} }}, //", accept.first, accept.second);
    for (const klex::Rule& rule : rules) {
      if (accept.second == rule.tag) {
        os << " " << rule.name;
      }
    }
    os << "\n";
  }
  os << "  }\n";
  os << "};\n";
}

void generateTokenDefCxx(std::ostream& os, const klex::RuleList& rules, const std::string& symbol) {
  // TODO: is symbol contains ::, everything before the last :: is considered a (nested) namespace
  os << "#pragma once\n";
  os << "enum class " << symbol << " {\n";
  for (const klex::Rule& rule : rules) {
    os << fmt::format("  {:<20} = {}, // {} \n", rule.name, rule.tag, rule.pattern);
  }
  os << "};\n";
}

int main(int argc, const char* argv[]) {
  klex::util::Flags flags;
  flags.defineBool("verbose", 'v', "Prints some more verbose output");
  flags.defineBool("help", 'h', "Prints this help and exits");
  flags.defineString("file", 'f', "PATTERN_FILE", "Input file with lexer rules");
  flags.defineString("output-table", 't', "FILE", "Output file that will contain the compiled tables");
  flags.defineString("output-token", 'T', "FILE", "Output file that will contain the compiled tables");
  flags.defineString("table-name", 'n', "IDENTIFIER", "Symbol name for generated table.", "lexerDef");
  flags.defineString("token-name", 'N', "IDENTIFIER", "Symbol name for generated token enum type.", "Token");
  flags.defineString("fa-dot", 'x', "DOT_FILE", "Writes dot graph of final finite automaton. Use - to represent stdout.", "");
  flags.defineBool("fa-renumber", 'r', "Renumbers states ordered ascending when generating dot file");
  flags.parse(argc, argv);

  if (flags.getBool("help")) {
    std::cout << flags.helpText();
    return EXIT_SUCCESS;
  }

#if 0
  klex::RuleParser ruleParser{std::make_unique<std::ifstream>(flags.getString("file"))};
  RuleList rules = ruleParser.parseRules();
#else
  klex::RuleList rules {
    {1, 10, "Spacing", "[ \\t\\n]+"},
    {2, 11, "If", "if"},
    {3, 12, "Then", "then"},
    {4, 13, "Else", "else"},
    {5, 14, "Identifier", "[a-z][a-z]*"},
    {6, 15, "NumberLiteral", "0|[1-9][0-9]*"},
  };
#endif

  klex::Builder builder;
  for (const klex::Rule& rule : rules)
    builder.declare(rule.tag, rule.pattern);

  klex::fa::FiniteAutomaton fa = builder.buildAutomaton(klex::Builder::Stage::Deterministic);

  if (std::string dotfile = flags.getString("fa-dot"); !dotfile.empty()) {
    if (flags.getBool("fa-renumber")) {
      fa.renumber();
    }
    if (dotfile == "-") {
      std::cout << klex::fa::dot({klex::fa::DotGraph{fa, "n", ""}}, "", true);
    } else {
      std::ofstream{dotfile} << klex::fa::dot({klex::fa::DotGraph{fa, "n", ""}}, "", true);
    }
  }

  if (std::string tokenFile = flags.getString("output-token"); tokenFile != "-") {
    std::ofstream ofs {tokenFile};
    generateTokenDefCxx(ofs, rules, flags.getString("token-name"));
  } else {
    generateTokenDefCxx(std::cerr, rules, flags.getString("token-name"));
  }

  klex::LexerDef lexerDef = klex::Builder::compile(fa);
  if (std::string tableFile = flags.getString("output-table"); tableFile != "-") {
    std::ofstream ofs {tableFile};
    generateTableDefCxx(ofs, lexerDef, rules, flags.getString("table-name"));
  } else {
    generateTableDefCxx(std::cerr, lexerDef, rules, flags.getString("table-name"));
  }

  return EXIT_SUCCESS;
}
