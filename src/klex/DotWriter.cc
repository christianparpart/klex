// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/DotWriter.h>

#include <algorithm>
#include <sstream>
#include <fmt/format.h>

namespace klex {

static std::string symbolText(Symbol input) {
  switch (input) {
    case Symbols::EndOfFile:
      return "<EOF>";
    case Symbols::Epsilon:
      return "ε";
    case ' ':
      return "\\s";
    case '\t':
      return "\\t";
    case '\n':
      return "\\n";
    default:
      return fmt::format("{}", (char) input);
  }
}

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

static std::string prettyCharRange(Symbol ymin, Symbol ymax) {
  std::stringstream sstr;
  switch (std::abs(ymax - ymin)) {
    case 0:
      sstr << symbolText(ymin);
      break;
    case 1:
      sstr << symbolText(ymin)
           << symbolText(ymin + 1);
      break;
    case 2:
      sstr << symbolText(ymin)
           << symbolText(ymin + 1)
           << symbolText(ymax);
      break;
    default:
      sstr << symbolText(ymin) << '-' << symbolText(ymax);
      break;
  }
  return sstr.str();
}

static std::string groupCharacterClassRanges(std::vector<Symbol> chars) {
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
        sstr << prettyCharRange(ymin, ymax);
      }
      ymin = ymax = c;
    }
    i++;
  }
  sstr << prettyCharRange(ymin, ymax);

  return sstr.str();
}

void DotWriter::start() {
  stream_ << "digraph {\n";
  stream_ << "  rankdir=LR;\n";
  //stream_ << "  label=\"" << escapeString("FA" /*TODO*/) << "\";\n";
}

void DotWriter::visitNode(int number, bool start, bool accept) {
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

void DotWriter::visitEdge(int from, int to, Symbol s) {
  transitionGroups_[to].push_back(s);
}

void DotWriter::endVisitEdge(int from, int to) {
	auto& tgroup = transitionGroups_[to];
  if (!tgroup.empty()) {
    std::string label = groupCharacterClassRanges(std::move(tgroup));
    stream_ << fmt::format("  {}{} -> {}{} [label=\"{}\"];\n",
                           stateLabelPrefix_, from,
                           stateLabelPrefix_, to,
                           escapeString(label));
    tgroup.clear();
  }
}

void DotWriter::end() {
  stream_ << "}\n";
}

} // namespace klex
