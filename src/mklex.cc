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
#include <klex/NFA.h>
#include <klex/RegExpr.h>
#include <klex/RegExprParser.h>
#include <klex/Rule.h>
#include <klex/RuleParser.h>
#include <klex/util/Flags.h>

#include <chrono>
#include <optional>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

std::string charLiteral(klex::Symbol ch) {
  switch (ch) {
    case klex::Symbols::EndOfFile:
    case klex::Symbols::Error:
      return fmt::format("{}", (int)ch);
    case ' ':
      return std::string{"' '"};
    case '\t':
      return std::string{"'\\t'"};
    case '\n':
      return std::string{"'\\n'"};
    case '\'':
      return std::string{"'\\''"};
    case '\\':
      return std::string{"'\\\\'"};
    default:
      if (ch >= 0 && ch <= 255 && std::isprint(ch))
        return fmt::format("'{}'", (char)ch);
      else
        return fmt::format("{}", (int)ch);
  }
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

  void lap(std::string_view message, size_t count, std::string_view item) {
    if (enabled) {
      end = std::chrono::high_resolution_clock::now();
      const Duration duration = end - start;
      std::swap(end, start);

      std::cerr << fmt::format("{}: {} seconds ({} {})\n", message, duration.count(), count, item);
    }
  }
};

std::pair<std::string, std::string> splitNamespace(const std::string& fullyQualifiedName) {
  size_t n = fullyQualifiedName.rfind("::");
  if (n != std::string::npos)
    return std::make_pair(fullyQualifiedName.substr(0, n), fullyQualifiedName.substr(n + 2));
  else
    return std::make_pair(std::string(), fullyQualifiedName);
}

void generateTableDefCxx(std::ostream& os, const klex::LexerDef& lexerDef, const klex::RuleList& rules,
                         const std::string& fullyQualifiedSymbolName) {
  auto [ns, tableName] = splitNamespace(fullyQualifiedSymbolName);

  os << "#include <klex/LexerDef.h>\n";
  os << "\n";

  if (!ns.empty())
    os << "namespace " << ns << " {\n\n";

  os << "klex::LexerDef " << tableName << " {\n";
  os << "  // initial states\n";
  os << "  std::map<std::string, klex::StateId> {\n";
  for (const std::pair<const std::string, klex::StateId>& s0 : lexerDef.initialStates)
    os << fmt::format("    {{ \"{}\", {} }},\n", s0.first, s0.second);
  os << "  },\n";
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
    std::set<std::string> names;
    std::for_each(rules.begin(), rules.end(), [&](const auto& rule) {
      if (accept.second == rule.tag)
        names.emplace(rule.name);
    });
    std::for_each(names.begin(), names.end(), [&](const auto& name) { os << " " << name; });
    os << "\n";
  }
  os << "  },\n";
  os << "  // backtracking map\n";
  os << "  klex::BacktrackingMap {\n";
  for (const std::pair<klex::StateId, klex::StateId>& backtrack : lexerDef.backtrackingStates) {
    os << fmt::format("    {{ {:>3}, {:>3} }},\n", backtrack.first, backtrack.second);
  }
  os << "  },\n";
  os << "  // tag-to-name mappings\n";
  os << "  std::map<klex::Tag, std::string> {\n";
  for (const std::pair<klex::Tag, std::string>& tagName : lexerDef.tagNames) {
    if (tagName.first != klex::IgnoreTag)
      os << fmt::format("    {{ {}, \"{}\" }},\n", tagName.first, tagName.second);
  }
  os << "  }\n";
  os << "};\n";

  if (!ns.empty())
    os << "\n} // namespace " << ns << "\n";
}

void generateTokenDefCxx(std::ostream& os, const klex::RuleList& rules, const std::string& tokenTypeName,
    const std::string& machineTypeName,
    const std::map<std::string, klex::StateId>& initialStates) {
  auto [ns, typeName] = splitNamespace(tokenTypeName);

  os << "#pragma once\n\n";
  os << "#include <cstdlib>       // for abort()\n";
  os << "#include <string_view>\n\n";
  if (!ns.empty())
    os << "namespace " << ns << " {\n\n";

  // -------------------------------------------------------------------------------------------------
  os << "enum class " << typeName << " {\n";
  const size_t tokenSize = std::max_element(rules.begin(), rules.end(),
      [](const auto& a, const auto& b) { return a.name.size() < b.name.size(); })->name.size();
  const std::string tokenFmt = fmt::format("  {{:<{}}} = {{:<5}} // {{}} \n", tokenSize);
  for (const klex::Rule& rule : rules) {
    if (rule.tag != klex::IgnoreTag)
      os << fmt::format(tokenFmt, rule.name, fmt::format("{},", rule.tag), rule.pattern);
  }
  os << "};\n\n";

  // -------------------------------------------------------------------------------------------------
  os << "enum class " << machineTypeName << " {\n";
  const size_t machineSize = std::max_element(initialStates.begin(), initialStates.end(),
      [](const auto& a, const auto& b) { return a.first.size() < b.first.size(); })->first.size();
  const std::string machineFmt = fmt::format("  {{:<{}}} = {{:}},\n", machineSize);

  for (const std::pair<const std::string, klex::StateId>& s0 : initialStates)
    os << fmt::format(machineFmt, s0.first, s0.second);
  os << "};\n\n";

  // -------------------------------------------------------------------------------------------------
  os << "inline constexpr std::string_view to_string(" << typeName << " t) {\n";
  os << "  switch (t) { \n";
  for (const klex::Rule& rule : rules)
    if (rule.tag != klex::IgnoreTag)
      os << "    case " << typeName << "::" << rule.name << ": return \"" << rule.name << "\";\n";
  os << "    default: abort();\n";
  os << "  }\n";
  os << "}\n";

  if (!ns.empty())
    os << "\n} // namespace " << ns << "\n";
}

std::optional<int> prepareAndParseCLI(klex::util::Flags& flags, int argc, const char* argv[]) {
  flags.defineBool("verbose", 'v', "Prints some more verbose output");
  flags.defineBool("help", 'h', "Prints this help and exits");
  flags.defineString("file", 'f', "PATTERN_FILE", "Input file with lexer rules");
  flags.defineString("output-table", 't', "FILE", "Output file that will contain the compiled tables (use - to represent stderr)");
  flags.defineString("output-token", 'T', "FILE", "Output file that will contain the compiled tables (use - to represent stderr)");
  flags.defineString("table-name", 'n', "IDENTIFIER", "Symbol name for generated table (may include namespace).", "lexerDef");
  flags.defineString("token-name", 'N', "IDENTIFIER", "Symbol name for generated token enum type (may include namespace).", "Token");
  flags.defineString("machine-name", 'M', "IDENTIFIER", "Symbol name for generated machine enum type (must not include namespace).", "Machine");
  flags.defineString("debug-dfa", 'x', "DOT_FILE", "Writes dot graph of final finite automaton. Use - to represent stdout.", "");
  flags.defineBool("debug-nfa", 'd', "Writes dot graph of non-deterministic finite automaton to stdout and exits.");
  flags.defineBool("no-dfa-minimize", 0, "Do not minimize the DFA");
  flags.defineBool("perf", 'p', "Print performance counters to stderr.");
  flags.parse(argc, argv);

  if (flags.getBool("help")) {
    std::cout << "mklex - klex lexer generator\n"
              << "(c) 2018 Christian Parpart <christian@parpart.family>\n"
              << "\n"
              << flags.helpText()
              << "\n";
    return EXIT_SUCCESS;
  }

  return std::nullopt;
}

int main(int argc, const char* argv[]) {
  klex::util::Flags flags;
  if (std::optional<int> rc = prepareAndParseCLI(flags, argc, argv); rc)
    return rc.value();

  fs::path klexFileName = flags.getString("file");

  PerfTimer perfTimer { flags.getBool("perf") };
  klex::Compiler builder;
  builder.parse(std::make_unique<std::ifstream>(klexFileName.string()));
  const klex::RuleList& rules = builder.rules();
  perfTimer.lap("NFA construction", builder.size(), "states");

  // TODO
  if (flags.getBool("debug-nfa")) {
    klex::NFA nfa = klex::NFA::join(builder.automata());
    klex::DotWriter writer{ std::cout, "n" };
    nfa.visit(writer);
    return EXIT_SUCCESS;
  }

  klex::Compiler::OvershadowMap overshadows;
  klex::MultiDFA multiDFA = builder.compileMultiDFA(&overshadows);
  perfTimer.lap("DFA construction", multiDFA.dfa.size(), "states");

  // check for unmatchable rules
  for (const std::pair<klex::Tag, klex::Tag>& overshadow : overshadows) {
    const klex::Rule& shadowee = *std::find_if(rules.begin(), rules.end(),
                                      [&](const klex::Rule& rule) { return rule.tag == overshadow.first; });
    const klex::Rule& shadower = *std::find_if(rules.begin(), rules.end(),
                                      [&](const klex::Rule& rule) { return rule.tag == overshadow.second; });
    std::cerr << fmt::format("[{}:{}] Rule {} cannot be matched as rule [{}:{}] {} takes precedence.\n",
                             shadowee.line, shadowee.column, shadowee.name,
                             shadower.line, shadower.column, shadower.name);
  }
  if (!overshadows.empty())
    return EXIT_FAILURE;

  if (!flags.getBool("no-dfa-minimize")) {
    multiDFA = std::move(klex::DFAMinimizer{multiDFA}.constructMultiDFA());
    perfTimer.lap("DFA minimization", multiDFA.dfa.size(), "states");
  }

  if (std::string dotfile = flags.getString("debug-dfa"); !dotfile.empty()) {
    if (dotfile == "-") {
      klex::DotWriter writer{ std::cout, "n", multiDFA.initialStates };
      multiDFA.dfa.visit(writer);
    } else {
      klex::DotWriter writer{ dotfile, "n", multiDFA.initialStates };
      multiDFA.dfa.visit(writer);
    }
  }

  klex::LexerDef lexerDef = klex::Compiler::generateTables(multiDFA, builder.names());
  if (std::string tableFile = flags.getString("output-table"); tableFile != "-") {
    if (auto p = fs::path{tableFile}.remove_filename(); p != "")
      fs::create_directories(p);
    std::ofstream ofs {tableFile};
    generateTableDefCxx(ofs, lexerDef, rules, flags.getString("table-name"));
  } else {
    generateTableDefCxx(std::cerr, lexerDef, rules, flags.getString("table-name"));
  }

  if (std::string tokenFile = flags.getString("output-token"); tokenFile != "-") {
    if (auto p = fs::path{tokenFile}.remove_filename(); p != "")
      fs::create_directories(p);
    std::ofstream ofs {tokenFile};
    generateTokenDefCxx(ofs, rules, flags.getString("token-name"), flags.getString("machine-name"), lexerDef.initialStates);
  } else {
    generateTokenDefCxx(std::cerr, rules, flags.getString("token-name"), flags.getString("machine-name"), lexerDef.initialStates);
  }

  return EXIT_SUCCESS;
}
