#include <klex/DotVisitor.h>
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

void DotfileWriter::start() {
  stream_ << "digraph {\n";
  stream_ << "  rankdir=LR;\n";
  //stream_ << "  label=\"" << escapeString("FA" /*TODO*/) << "\";\n";
}

void DotfileWriter::visitNode(int number, bool start, bool accept) {
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

void DotfileWriter::visitEdge(int from, int to, std::string_view label) {
  stream_ << fmt::format("  {}{} -> {}{} [label=\"{}\"];\n",
                         stateLabelPrefix_, from,
                         stateLabelPrefix_, to,
                         escapeString(label));
}

void DotfileWriter::end() {
  stream_ << "}\n";
}

} // namespace klex
