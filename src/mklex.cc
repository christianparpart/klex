// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/regular/Compiler.h>
#include <klex/regular/DFA.h>
#include <klex/regular/DFAMinimizer.h>
#include <klex/regular/DotWriter.h>
#include <klex/regular/Lexer.h>
#include <klex/regular/NFA.h>
#include <klex/regular/RegExpr.h>
#include <klex/regular/RegExprParser.h>
#include <klex/regular/Rule.h>
#include <klex/regular/RuleParser.h>
#include <klex/util/Flags.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

using namespace std;
using namespace klex::regular;
using namespace klex::util;

string charLiteral(Symbol ch)
{
	switch (ch)
	{
		case Symbols::EndOfFile:
		case Symbols::Error:
			return fmt::format("{}", (int) ch);
		case ' ':
			return string{"' '"};
		case '\t':
			return string{"'\\t'"};
		case '\n':
			return string{"'\\n'"};
		case '\'':
			return string{"'\\''"};
		case '\\':
			return string{"'\\\\'"};
		default:
			if (ch >= 0 && ch <= 255 && isprint(ch))
				return fmt::format("'{}'", (char) ch);
			else
				return fmt::format("{}", (int) ch);
	}
}

struct PerfTimer {
	using Duration = chrono::duration<double>;
	using TimePoint = chrono::time_point<chrono::high_resolution_clock>;

	explicit PerfTimer(bool _enabled) : enabled{_enabled}
	{
		if (_enabled)
			start = chrono::high_resolution_clock::now();
	}

	bool enabled;
	TimePoint start;
	TimePoint end;

	void lap(string_view message, size_t count, string_view item)
	{
		if (enabled)
		{
			end = chrono::high_resolution_clock::now();
			const Duration duration = end - start;
			swap(end, start);

			cerr << fmt::format("{}: {} seconds ({} {})\n", message, duration.count(), count, item);
		}
	}
};

pair<string, string> splitNamespace(const string& fullyQualifiedName)
{
	size_t n = fullyQualifiedName.rfind("::");
	if (n != string::npos)
		return make_pair(fullyQualifiedName.substr(0, n), fullyQualifiedName.substr(n + 2));
	else
		return make_pair(string(), fullyQualifiedName);
}

void generateTableDefCxx(ostream& os, const LexerDef& lexerDef, const RuleList& rules,
						 const string& fullyQualifiedSymbolName)
{
	auto [ns, tableName] = splitNamespace(fullyQualifiedSymbolName);

	os << "#include <klex/regular/LexerDef.h>\n";
	os << "\n";

	if (!ns.empty())
		os << "namespace " << ns << " {\n\n";

	os << "klex::regular::LexerDef " << tableName << " {\n";
	os << "  // initial states\n";
	os << "  std::map<std::string, klex::regular::StateId> {\n";
	for (const pair<const string, StateId>& s0 : lexerDef.initialStates)
		os << fmt::format("    {{ \"{}\", {} }},\n", s0.first, s0.second);
	os << "  },\n";
	os << "  // containsBeginOfLineStates\n";
	os << "  " << (lexerDef.containsBeginOfLineStates ? "true" : "false") << ",\n";
	os << "  // state transition table \n";
	os << "  klex::regular::TransitionMap::Container {\n";
	for (StateId stateId : lexerDef.transitions.states())
	{
		os << "    { " << fmt::format("{:>3}", stateId) << ", {";
		int c = 0;
		for (pair<Symbol, StateId> t : lexerDef.transitions.map(stateId))
		{
			if (c)
				os << ", ";
			os << "{" << charLiteral(t.first) << ", " << t.second << "}";
			c++;
		}
		os << "}},\n";
	}
	os << "  },\n";
	os << "  // accept state to action label mappings\n";
	os << "  klex::regular::AcceptStateMap {\n";
	for (const pair<StateId, Tag>& accept : lexerDef.acceptStates)
	{
		os << fmt::format("    {{ {:>3}, {:>3} }}, //", accept.first, accept.second);
		set<string> names;
		for_each(rules.begin(), rules.end(), [&](const auto& rule) {
			if (accept.second == rule.tag)
				names.emplace(rule.name);
		});
		for_each(names.begin(), names.end(), [&](const auto& name) { os << " " << name; });
		os << "\n";
	}
	os << "  },\n";
	os << "  // backtracking map\n";
	os << "  klex::regular::BacktrackingMap {\n";
	for (const pair<StateId, StateId>& backtrack : lexerDef.backtrackingStates)
	{
		os << fmt::format("    {{ {:>3}, {:>3} }},\n", backtrack.first, backtrack.second);
	}
	os << "  },\n";
	os << "  // tag-to-name mappings\n";
	os << "  std::map<klex::regular::Tag, std::string> {\n";
	for (const pair<Tag, string>& tagName : lexerDef.tagNames)
	{
		if (tagName.first != IgnoreTag)
			os << fmt::format("    {{ {}, \"{}\" }},\n", tagName.first, tagName.second);
	}
	os << "  }\n";
	os << "};\n";

	if (!ns.empty())
		os << "\n} // namespace " << ns << "\n";
}

bool compareRuleNameSize(const Rule& a, const Rule& b)
{
	return a.name.size() < b.name.size();
}

bool compareNameSize(const pair<string, StateId>& a, const pair<string, StateId>& b)
{
	return a.first.size() < b.first.size();
}

void generateTokenDefCxx(ostream& os, const RuleList& rules, const string& tokenTypeName,
						 const string& machineTypeName, const map<string, StateId>& initialStates)
{
	auto [ns, typeName] = splitNamespace(tokenTypeName);

	os << "#pragma once\n\n";
	os << "#include <cstdlib>       // for abort()\n";
	os << "#include <string_view>\n\n";
	if (!ns.empty())
		os << "namespace " << ns << " {\n\n";

	// -------------------------------------------------------------------------------------------------
	os << "enum class " << typeName << " {\n";
	const size_t tokenSize = max_element(rules.begin(), rules.end(), compareRuleNameSize)->name.size();
	const string tokenFmt = fmt::format("  {{:<{}}} = {{:<5}} // {{}} \n", tokenSize);
	for (const Rule& rule : rules)
	{
		if (rule.tag != IgnoreTag)
			os << fmt::format(tokenFmt, rule.name, fmt::format("{},", rule.tag), rule.pattern);
	}
	os << "};\n\n";

	// -------------------------------------------------------------------------------------------------
	os << "enum class " << machineTypeName << " {\n";
	const size_t machineSize =
		max_element(initialStates.begin(), initialStates.end(), compareNameSize)->first.size();
	const string machineFmt = fmt::format("  {{:<{}}} = {{:}},\n", machineSize);

	for (const pair<const string, StateId>& s0 : initialStates)
		os << fmt::format(machineFmt, s0.first, s0.second);
	os << "};\n\n";

	// -------------------------------------------------------------------------------------------------
	os << "inline constexpr std::string_view to_string(" << typeName << " t) {\n";
	os << "  switch (t) { \n";
	for (const Rule& rule : rules)
		if (rule.tag != IgnoreTag)
			os << "    case " << typeName << "::" << rule.name << ": return \"" << rule.name << "\";\n";
	os << "    default: abort();\n";
	os << "  }\n";
	os << "}\n";

	if (!ns.empty())
		os << "\n} // namespace " << ns << "\n";
}

optional<int> prepareAndParseCLI(Flags& flags, int argc, const char* argv[])
{
	flags.defineBool("verbose", 'v', "Prints some more verbose output");
	flags.defineBool("help", 'h', "Prints this help and exits");
	flags.defineString("file", 'f', "PATTERN_FILE", "Input file with lexer rules");
	flags.defineString("output-table", 't', "FILE",
					   "Output file that will contain the compiled tables (use - to represent stderr)");
	flags.defineString("output-token", 'T', "FILE",
					   "Output file that will contain the compiled tables (use - to represent stderr)");
	flags.defineString("table-name", 'n', "IDENTIFIER",
					   "Symbol name for generated table (may include namespace).", "lexerDef");
	flags.defineString("token-name", 'N', "IDENTIFIER",
					   "Symbol name for generated token enum type (may include namespace).", "Token");
	flags.defineString("machine-name", 'M', "IDENTIFIER",
					   "Symbol name for generated machine enum type (must not include namespace).",
					   "Machine");
	flags.defineString("debug-dfa", 'x', "DOT_FILE",
					   "Writes dot graph of final finite automaton. Use - to represent stdout.", "");
	flags.defineBool("debug-nfa", 'd',
					 "Writes dot graph of non-deterministic finite automaton to stdout and exits.");
	flags.defineBool("no-dfa-minimize", 0, "Do not minimize the DFA");
	flags.defineBool("perf", 'p', "Print performance counters to stderr.");

	try
	{
		flags.parse(argc, argv);
	}
	catch (const Flags::Error& e)
	{
		cerr << "Failed to parse command line parameters. " << e.what() << "\n";
		return EXIT_FAILURE;
	}

	if (flags.getBool("help"))
	{
        static string_view const title =
            "mklex - klex lexer generator\n"
            "(c) 2018 Christian Parpart <christian@parpart.family>\n"
            "\n";
        cerr << flags.helpText(title) << "\n";
		return EXIT_SUCCESS;
	}

	return nullopt;
}

int main(int argc, const char* argv[])
{
	Flags flags;
	if (optional<int> rc = prepareAndParseCLI(flags, argc, argv); rc)
		return rc.value();

	fs::path klexFileName = flags.getString("file");

	PerfTimer perfTimer{flags.getBool("perf")};
	Compiler builder;
	builder.parse(make_unique<ifstream>(klexFileName.string()));
	const RuleList& rules = builder.rules();
	perfTimer.lap("NFA construction", builder.size(), "states");

	if (flags.getBool("debug-nfa"))
	{
		NFA nfa = NFA::join(builder.automata());
		DotWriter writer{cout, "n"};
		nfa.visit(writer);
		return EXIT_SUCCESS;
	}

	Compiler::OvershadowMap overshadows;
	MultiDFA multiDFA = builder.compileMultiDFA(&overshadows);
	perfTimer.lap("DFA construction", multiDFA.dfa.size(), "states");

	// check for unmatchable rules
	for (const pair<Tag, Tag>& overshadow : overshadows)
	{
		const Rule& shadowee = *find_if(rules.begin(), rules.end(),
										[&](const Rule& rule) { return rule.tag == overshadow.first; });
		const Rule& shadower = *find_if(rules.begin(), rules.end(),
										[&](const Rule& rule) { return rule.tag == overshadow.second; });
		cerr << fmt::format("[{}:{}] Rule {} cannot be matched as rule [{}:{}] {} takes precedence.\n",
							shadowee.line, shadowee.column, shadowee.name, shadower.line, shadower.column,
							shadower.name);
	}
	if (!overshadows.empty())
		return EXIT_FAILURE;

	if (!flags.getBool("no-dfa-minimize"))
	{
		multiDFA = DFAMinimizer{multiDFA}.constructMultiDFA();
		perfTimer.lap("DFA minimization", multiDFA.dfa.size(), "states");
	}

	if (string dotfile = flags.getString("debug-dfa"); !dotfile.empty())
	{
		if (dotfile == "-")
		{
			DotWriter writer{cout, "n", multiDFA.initialStates};
			multiDFA.dfa.visit(writer);
		}
		else
		{
			DotWriter writer{dotfile, "n", multiDFA.initialStates};
			multiDFA.dfa.visit(writer);
		}
	}

	LexerDef lexerDef = Compiler::generateTables(multiDFA, builder.containsBeginOfLine(), builder.names());
	if (string tableFile = flags.getString("output-table"); tableFile != "-")
	{
		if (auto p = fs::path{tableFile}.remove_filename(); p != "")
			fs::create_directories(p);
		ofstream ofs{tableFile};
		generateTableDefCxx(ofs, lexerDef, rules, flags.getString("table-name"));
	}
	else
	{
		generateTableDefCxx(cerr, lexerDef, rules, flags.getString("table-name"));
	}

	if (string tokenFile = flags.getString("output-token"); tokenFile != "-")
	{
		if (auto p = fs::path{tokenFile}.remove_filename(); p != "")
			fs::create_directories(p);
		ofstream ofs{tokenFile};
		generateTokenDefCxx(ofs, rules, flags.getString("token-name"), flags.getString("machine-name"),
							lexerDef.initialStates);
	}
	else
	{
		generateTokenDefCxx(cerr, rules, flags.getString("token-name"), flags.getString("machine-name"),
							lexerDef.initialStates);
	}

	return EXIT_SUCCESS;
}
