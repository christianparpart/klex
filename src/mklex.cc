// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Compiler.h>
#include <klex/DFA.h>
#include <klex/DFAMinimizer.h>
#include <klex/DotWriter.h>
#include <klex/Lexer.h>
#include <klex/Rule.h>
#include <klex/RuleParser.h>
#include <klex/util/Flags.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

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
  os << "#include <klex/LexerDef.h>\n";
  os << "\n";
  os << "klex::LexerDef " << symbolName << " {\n";
  os << "  // initial state\n";
  os << "  " << lexerDef.initialStateId << ",\n";
  os << "  // state transition table \n";
  os << "  klex::TransitionMap::Container {\n";
  for (klex::StateId stateId : lexerDef.transitions.states()) {
    os << "    { " << fmt::format("{:>3}", stateId) << ", {";
    int c = 0;
    for (std::pair<klex::Symbol, klex::StateId> t : lexerDef.transitions.map(stateId)) {
      if (c) os << ", ";
      os << "{" << charLiteral(t.first) << ", " << t.second << "}";
      c++;
    }
    os << "}},\n";
  }
  os << "  },\n";
  os << "  // accept state to action label mappings\n";
  os << "  klex::AcceptStateMap {\n";
  for (const std::pair<klex::StateId, klex::Tag>& accept : lexerDef.acceptStates) {
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

struct PerfTimer {
  using Duration = std::chrono::duration<double>;
  using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

  explicit PerfTimer(bool _enabled) : enabled{_enabled} {
    if (_enabled)
      start = std::chrono::high_resolution_clock::now();
  }

  bool enabled;
  TimePoint start;
  TimePoint end;

  void lap(std::string_view message) {
    if (enabled) {
      end = std::chrono::high_resolution_clock::now();
      const Duration duration = end - start;
      std::swap(end, start);

      std::cerr << message << ": " << duration.count() << " seconds\n";
    }
  }
};

void generateTokenDefCxx(std::ostream& os, const klex::RuleList& rules, const std::string& symbol) {
  // TODO: is symbol contains ::, everything before the last :: is considered a (nested) namespace
  os << "#pragma once\n";
  os << "enum class " << symbol << " {\n";
  for (const klex::Rule& rule : rules) {
    os << fmt::format("  {:<20} = {:<4}, // {} \n", rule.name, rule.tag, rule.pattern);
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
  flags.defineString("debug-dfa", 'x', "DOT_FILE", "Writes dot graph of final finite automaton. Use - to represent stdout.", "");
  flags.defineBool("perf", 'p', "Print performance counters to stderr.");
  flags.defineBool("debug-nfa", 'd', "Print NFA and exit.");
  flags.parse(argc, argv);

  if (flags.getBool("help")) {
    std::cout << "mklex - klex lexer generator\n"
              << "(c) 2018 Christian Parpart <christian@parpart.family>\n"
              << "\n"
              << flags.helpText();
    return EXIT_SUCCESS;
  }

  fs::path klexFileName = flags.getString("file");

  klex::RuleParser ruleParser{std::make_unique<std::ifstream>(klexFileName.string())};
  PerfTimer perfTimer { flags.getBool("perf") };
  klex::RuleList rules = ruleParser.parseRules();
  perfTimer.lap("Rule parsing");

  klex::Compiler builder;
  for (const klex::Rule& rule : rules)
    builder.declare(rule.tag, rule.pattern);
  perfTimer.lap("NFA construction");

  if (flags.getBool("debug-nfa")) {
    klex::DotWriter writer{ std::cout };
    builder.nfa().visit(writer);
    return EXIT_SUCCESS;
  }

  klex::DFA dfa = builder.compileDFA();
  perfTimer.lap("DFA construction");

  klex::DFA dfamin = klex::DFAMinimizer{dfa}.construct();
  perfTimer.lap("DFA minimization");

  if (std::string dotfile = flags.getString("debug-dfa"); !dotfile.empty()) {
    if (dotfile == "-") {
      klex::DotWriter writer{ std::cout };
      dfamin.visit(writer);
    } else {
      klex::DotWriter writer{ dotfile };
      dfamin.visit(writer);
    }
  }

  if (std::string tokenFile = flags.getString("output-token"); tokenFile != "-") {
    fs::create_directories(fs::path{tokenFile}.remove_filename());
    std::ofstream ofs {tokenFile};
    generateTokenDefCxx(ofs, rules, flags.getString("token-name"));
  } else {
    generateTokenDefCxx(std::cerr, rules, flags.getString("token-name"));
  }

  klex::LexerDef lexerDef = klex::Compiler::compile(dfamin);
  if (std::string tableFile = flags.getString("output-table"); tableFile != "-") {
    fs::create_directories(fs::path{tableFile}.remove_filename());
    std::ofstream ofs {tableFile};
    generateTableDefCxx(ofs, lexerDef, rules, flags.getString("table-name"));
  } else {
    generateTableDefCxx(std::cerr, lexerDef, rules, flags.getString("table-name"));
  }

  return EXIT_SUCCESS;
}
