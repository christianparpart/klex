// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT


#include <klex/builder.h>
#include <klex/lexer.h>
#include <klex/fa.h>
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
  flags.parse(argc, argv);

  if (flags.getBool("help")) {
    std::cout << flags.helpText();
    return EXIT_SUCCESS;
  }
#if 0
  // TODO
  klex::Builder builder;
  klex::Rule patternParser{std::ifstream(flags.getString("file"))};
  for (klex::Pattern pattern : patternParser)
    builder.declare(pattern.tag(), pattern.regex());

  if (int n = flags.getNumber("print-fa"); n >= 1 && n <= 3) {
    klex::fa::FiniteAutomaton fa = builder.buildAutomaton(static_cast<klex::Builder::Stage>(n));
    //fa.renumber();

    std::cout << klex::fa::dot({klex::fa::DotGraph{fa, "n", ""}}, "", !flags.getBool("no-group-edges"));
  } 
#endif

  return EXIT_SUCCESS;
}

