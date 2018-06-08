// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/dot.h>
#include <algorithm>
#include <sstream>
#include <map>
#include <vector>

namespace klex {

static std::string prettyCharRange(Symbol ymin, Symbol ymax, bool dot) {
  std::stringstream sstr;
  switch (std::abs(ymax - ymin)) {
    case 0:
      sstr << prettySymbol(ymin, dot);
      break;
    case 1:
      sstr << prettySymbol(ymin, dot)
           << prettySymbol(ymin + 1, dot);
      break;
    case 2:
      sstr << prettySymbol(ymin, dot)
           << prettySymbol(ymin + 1, dot)
           << prettySymbol(ymax, dot);
      break;
    default:
      sstr << prettySymbol(ymin, dot) << '-' << prettySymbol(ymax, dot);
      break;
  }
  return sstr.str();
}

template<typename StringType>
static std::string escapeString(const StringType& str) {
  std::stringstream sstr;
  for (char ch : str) {
    switch (ch) {
      case '\r':
        sstr << "\\r";
        break;
      case '\n':
        sstr << "\\n";
        break;
      case '\t':
        sstr << "\\t";
        break;
      case '"':
        sstr << "\\\"";
        break;
      case ' ':
        sstr << ' ';
        break;
      default:
        if (std::isprint(ch)) {
          sstr << ch;
        } else {
          sstr << fmt::format("\\x{:02x}", ch);
        }
    }
  }
  return sstr.str();
}

std::string groupCharacterClassRanges(std::vector<Symbol> chars, bool dot) {
  // we took a copy in tgroup here, so I can sort() later
  std::sort(chars.begin(), chars.end());

  // {1,3,5,a,b,c,d,e,f,z]
  // ->
  // {{1}, {3}, {5}, {a-f}, {z}}

  std::stringstream sstr;
  Symbol ymin = '\0';
  Symbol ymax = ymin;
  int i = 0;

  for (Symbol c : chars) {
    if (c == ymax + 1) {  // range growing
      ymax = c;
    }
    else { // gap found
      if (i) {
        sstr << prettyCharRange(ymin, ymax, dot);
      }
      ymin = ymax = c;
    }
    i++;
  }
  sstr << prettyCharRange(ymin, ymax, dot);

  return sstr.str();
}


std::string dot(std::list<DotGraph> graphs, std::string_view label, bool groupEdges) {
  std::stringstream sstr;

  sstr << "digraph {\n";

  int clusterId = 0;
  sstr << "  rankdir=LR;\n";
  sstr << "  label=\"" << escapeString(label) << "\";\n";
  for (const DotGraph& graph : graphs) {
    sstr << "  subgraph cluster_" << clusterId << " {\n";
    sstr << "    label=\"" << escapeString(graph.graphLabel) << "\";\n";
    clusterId++;

    // acceptState
    for (const std::unique_ptr<State>& s: graph.states) {
      if (s->isAccepting()) {
        sstr << "    node [shape=doublecircle";
        //(FIXME, BUGGY?) sstr << ",label=\"" << fmt::format("{}{}:{}", graph.stateLabelPrefix, s->id(), s->tag()) << "\"";
        sstr << "]; "
             << graph.stateLabelPrefix << s->id() << ";\n";
      }
    }

    // initialState
    sstr << "    \"\" [shape=plaintext];\n";
    sstr << "    node [shape=circle];\n";
    sstr << "    ";
    if (graphs.size() == 1)
      sstr << "\"\" -> ";
    sstr << graph.stateLabelPrefix << graph.initialState->id() << ";\n";

    // all states and their edges
    for (const std::unique_ptr<State>& state: graph.states) {
      if (state->tag() != 0) {
        sstr << "    " << graph.stateLabelPrefix << state->id() << " ["
             << "color=blue"
             << "]\n";
      }
      if (groupEdges) {
        std::map<State* /*target state*/, std::vector<Symbol> /*transition symbols*/> transitionGroups;

        for (const Edge& transition: state->transitions())
          transitionGroups[transition.state].emplace_back(transition.symbol);

        for (const std::pair<State*, std::vector<Symbol>>& tgroup: transitionGroups) {
          std::string label = groupCharacterClassRanges(tgroup.second, true);
          const State* targetState = tgroup.first;
          sstr << "    " << graph.stateLabelPrefix << state->id()
               << " -> " << graph.stateLabelPrefix << targetState->id();
          sstr << "   [label=\"" << escapeString(label) << "\"]";
          sstr << ";\n";
        }
      } else {
        for (const Edge& transition : state->transitions()) {
          std::string label = prettySymbol(transition.symbol, true);
          const State* targetState = transition.state;
          sstr << "    " << graph.stateLabelPrefix << state->id()
               << " -> " << graph.stateLabelPrefix << targetState->id();
          sstr << "   [label=\"" << escapeString(label) << "\"]";
          sstr << ";\n";
        }
      }
    }

    sstr << "  }\n";
  }

  sstr << "}\n";

  return sstr.str();
}


} // namespace klex
