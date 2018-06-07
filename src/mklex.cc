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
#include <klex/util/Flags.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, const char* argv[]) {
  klex::util::Flags flags;
  flags.defineBool("verbose", 'v', "Prints some more verbose output");
  flags.defineBool("help", 'h', "Prints this help and exits");
  flags.defineString("file", 'f', "PATTERN_FILE", "Input file with lexer rules");
  flags.defineString("output", 'o', "OUTPUT_FILE", "Output file that will contain the compiled tables");
  flags.defineString("output-token", 0, "OUTPUT_FILE", "Output file that will contain the compiled tables");
  flags.defineString("fa-dot", 't', "Writes dot graph of final finite automaton. Use - to represent stdout.");
  flags.defineNumber("fa-optimize", 'O', "LEVEL", "Finite Automaton's optimization level (0, 1, 2)", 2);
  flags.defineBool("fa-renumber", 'r', "Renumbers states ordered ascending when generating dot file");
  flags.parse(argc, argv);

  if (flags.getBool("help")) {
    std::cout << flags.helpText();
    return EXIT_SUCCESS;
  }

  klex::Builder builder;
  klex::Rule ruleParser{std::ifstream(flags.getString("file"))};
  for (const klex::Rule& rule : ruleParser)
    builder.declare(rule.tag, rule.pattern);

  int optimizationLevel = flags.getNumber("fa-optimize");
  if (std::string dotfile = flags.getString("fa-dot"); !dotfile.empty()) {
    klex::fa::FiniteAutomaton fa = builder.buildAutomaton(
        static_cast<klex::Builder::Stage>(optimizationLevel));

    if (flags.getBool("fa-renumber"))
      fa.renumber();

    std::cout << klex::fa::dot({klex::fa::DotGraph{fa, "n", ""}}, "", !flags.getBool("no-group-edges"));
  } 

  // TODO: generate table-header and token-header file

  return EXIT_SUCCESS;
}

