// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/DotWriter.h>
#include <klex/Symbols.h>

#include <algorithm>
#include <cassert>
#include <sstream>
#include <fmt/format.h>

namespace klex {

template<typename StringType>
static std::string escapeString(const StringType& str) {
  std::stringstream stream_;
  for (char ch : str) {
    switch (ch) {
      case '\\':
        stream_ << "\\\\";
        break;
      case '\r':
        stream_ << "\\r";
        break;
      case '\n':
        stream_ << "\\n";
        break;
      case '\t':
        stream_ << "\\t";
        break;
      case '"':
        stream_ << "\\\"";
        break;
      case ' ':
        stream_ << ' ';
        break;
      default:
        stream_ << ch;
        break;
    }
  }
  return stream_.str();
}

void DotWriter::start(StateId initialState) {
  initialState_ = initialState;
  stream_ << "digraph {\n";
  stream_ << "  rankdir=LR;\n";
  //stream_ << "  label=\"" << escapeString("FA" /*TODO*/) << "\";\n";
}

void DotWriter::visitNode(StateId number, bool start, bool accept) {
  if (start) {
    const std::string_view shape = accept ? "doublecircle" : "circle";
    stream_ << "  \"\" [shape=plaintext];\n";
    stream_ << "  node [shape=" << shape << ",color=red];\n";
    stream_ << "  \"\" -> " << stateLabelPrefix_ << number << ";\n";
    stream_ << "  node [color=black];\n";
  } else if (accept) {
    stream_ << "  node [shape=doublecircle]; " << stateLabelPrefix_ << number << ";\n";
    stream_ << "  node [shape=circle,color=black];\n";
  } else {
    // stream_ << stateLabelPrefix_ << number << ";\n";
  }
}

void DotWriter::visitEdge(StateId from, StateId to, Symbol s) {
  transitionGroups_[to].push_back(s);
}

void DotWriter::endVisitEdge(StateId from, StateId to) {
	auto& tgroup = transitionGroups_[to];
  if (!tgroup.empty()) {
    if (from == initialState_ && initialStates_ != nullptr) {
      for (Symbol s : tgroup) {
        const std::string label = [this, s]() {
          for (const auto& p : *initialStates_)
            if (p.second == static_cast<StateId>(s))
              return fmt::format("<{}>", p.first);
          return prettySymbol(s);
        }();
        stream_ << fmt::format("  {}{} -> {}{} [label=\"{}\"];\n",
                               stateLabelPrefix_, from,
                               stateLabelPrefix_, to,
                               escapeString(label));
      }
    } else {
      std::string label = groupCharacterClassRanges(std::move(tgroup));
      stream_ << fmt::format("  {}{} -> {}{} [label=\"{}\"];\n",
                             stateLabelPrefix_, from,
                             stateLabelPrefix_, to,
                             escapeString(label));
    }
    tgroup.clear();
  }
}

void DotWriter::end() {
  stream_ << "}\n";
}

} // namespace klex
